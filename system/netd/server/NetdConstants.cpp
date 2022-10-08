/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <ctype.h>
#include <net/if.h>

#define LOG_TAG "Netd"

#include <cutils/log.h>
#include <logwrap/logwrap.h>

#include "NetdConstants.h"

const char * const OEM_SCRIPT_PATH = "/system/bin/oem-iptables-init.sh";
const char * const IPTABLES_PATH = "/system/bin/iptables";
const char * const IP6TABLES_PATH = "/system/bin/ip6tables";
const char * const TC_PATH = "/system/bin/tc";
const char * const IP_PATH = "/system/bin/ip";
const char * const ADD = "add";
const char * const DEL = "del";

// static void logExecError(const char* argv[], int res, int status) {
//     const char** argp = argv;
//     std::string args = "";
//     while (*argp) {
//         args += *argp;
//         args += ' ';
//         argp++;
//     }
//     ALOGE("exec() res=%d, status=%d for %s", res, status, args.c_str());
// }

static int execIptablesCommand(int argc, const char *argv[], bool silent) {
#if 0
    int res;
    int status;

    res = android_fork_execvp(argc, (char **)argv, &status, false,
        !silent);
    if (res || !WIFEXITED(status) || WEXITSTATUS(status)) {
        if (!silent) {
            logExecError(argv, res, status);
        }
        if (res)
            return res;
        if (!WIFEXITED(status))
            return ECHILD;
    }
    return WEXITSTATUS(status);
#else
    (void) argc;
    (void) argv;
    (void) silent;
    return 0;
#endif
}

static int execIptables(IptablesTarget target, bool silent, va_list args) {
    /* Read arguments from incoming va_list; we expect the list to be NULL terminated. */
    std::list<const char*> argsList;
    argsList.push_back(NULL);
    const char* arg;
    do {
        arg = va_arg(args, const char *);
        argsList.push_back(arg);
    } while (arg);

    int i = 0;
    const char* argv[argsList.size()];
    std::list<const char*>::iterator it;
    for (it = argsList.begin(); it != argsList.end(); it++, i++) {
        argv[i] = *it;
    }

    int res = 0;
    if (target == V4 || target == V4V6) {
        argv[0] = IPTABLES_PATH;
        res |= execIptablesCommand(argsList.size(), argv, silent);
    }
    if (target == V6 || target == V4V6) {
        argv[0] = IP6TABLES_PATH;
        res |= execIptablesCommand(argsList.size(), argv, silent);
    }
    return res;
}

int execIptables(IptablesTarget target, ...) {
    va_list args;
    va_start(args, target);
    int res = execIptables(target, false, args);
    va_end(args);
    return res;
}

int execIptablesSilently(IptablesTarget target, ...) {
    va_list args;
    va_start(args, target);
    int res = execIptables(target, true, args);
    va_end(args);
    return res;
}

int writeFile(const char *path, const char *value, int size) {
    int fd = open(path, O_WRONLY);
    if (fd < 0) {
        ALOGE("Failed to open %s: %s", path, strerror(errno));
        return -1;
    }

    if (write(fd, value, size) != size) {
        ALOGE("Failed to write %s: %s", path, strerror(errno));
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

int readFile(const char *path, char *buf, int *sizep)
{
    int fd = open(path, O_RDONLY);
    int size;

    if (fd < 0) {
        ALOGE("Failed to open %s: %s", path, strerror(errno));
        return -1;
    }

    size = read(fd, buf, *sizep);
    if (size < 0) {
        ALOGE("Failed to write %s: %s", path, strerror(errno));
        close(fd);
        return -1;
    }
    *sizep = size;
    close(fd);
    return 0;
}

/*
 * Check an interface name for plausibility. This should e.g. help against
 * directory traversal.
 */
bool isIfaceName(const char *name) {
    size_t i;
    size_t name_len = strlen(name);
    if ((name_len == 0) || (name_len > IFNAMSIZ)) {
        return false;
    }

    /* First character must be alphanumeric */
    if (!isalnum(name[0])) {
        return false;
    }

    for (i = 1; i < name_len; i++) {
        if (!isalnum(name[i]) && (name[i] != '_') && (name[i] != '-') && (name[i] != ':')) {
            return false;
        }
    }

    return true;
}

int parsePrefix(const char *prefix, uint8_t *family, void *address, int size, uint8_t *prefixlen) {
    if (!prefix || !family || !address || !prefixlen) {
        return -EFAULT;
    }

    // Find the '/' separating address from prefix length.
    const char *slash = strchr(prefix, '/');
    const char *prefixlenString = slash + 1;
    if (!slash || !*prefixlenString)
        return -EINVAL;

    // Convert the prefix length to a uint8_t.
    char *endptr;
    unsigned templen;
    templen = strtoul(prefixlenString, &endptr, 10);
    if (*endptr || templen > 255) {
        return -EINVAL;
    }
    *prefixlen = templen;

    // Copy the address part of the prefix to a local buffer. We have to copy
    // because inet_pton and getaddrinfo operate on null-terminated address
    // strings, but prefix is const and has '/' after the address.
    std::string addressString(prefix, slash - prefix);

    // Parse the address.
    addrinfo *res;
    addrinfo hints = {
        .ai_flags = AI_NUMERICHOST,
    };
    int ret = getaddrinfo(addressString.c_str(), NULL, &hints, &res);
    if (ret || !res) {
        return -EINVAL;  // getaddrinfo return values are not errno values.
    }

    // Convert the address string to raw address bytes.
    void *rawAddress;
    int rawLength;
    switch (res[0].ai_family) {
        case AF_INET: {
            if (*prefixlen > 32) {
                return -EINVAL;
            }
            sockaddr_in *sin = (sockaddr_in *) res[0].ai_addr;
            rawAddress = &sin->sin_addr;
            rawLength = 4;
            break;
        }
        case AF_INET6: {
            if (*prefixlen > 128) {
                return -EINVAL;
            }
            sockaddr_in6 *sin6 = (sockaddr_in6 *) res[0].ai_addr;
            rawAddress = &sin6->sin6_addr;
            rawLength = 16;
            break;
        }
        default: {
            freeaddrinfo(res);
            return -EAFNOSUPPORT;
        }
    }

    if (rawLength > size) {
        freeaddrinfo(res);
        return -ENOSPC;
    }

    *family = res[0].ai_family;
    memcpy(address, rawAddress, rawLength);
    freeaddrinfo(res);

    return rawLength;
}
