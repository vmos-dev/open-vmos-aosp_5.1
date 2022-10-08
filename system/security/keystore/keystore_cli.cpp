/*
 * Copyright (C) 2009 The Android Open Source Project
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

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#include <keystore/IKeystoreService.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>

#include <keystore/keystore.h>

using namespace android;

static const char* responses[] = {
    NULL,
    /* [NO_ERROR]           = */ "No error",
    /* [LOCKED]             = */ "Locked",
    /* [UNINITIALIZED]      = */ "Uninitialized",
    /* [SYSTEM_ERROR]       = */ "System error",
    /* [PROTOCOL_ERROR]     = */ "Protocol error",
    /* [PERMISSION_DENIED]  = */ "Permission denied",
    /* [KEY_NOT_FOUND]      = */ "Key not found",
    /* [VALUE_CORRUPTED]    = */ "Value corrupted",
    /* [UNDEFINED_ACTION]   = */ "Undefined action",
    /* [WRONG_PASSWORD]     = */ "Wrong password (last chance)",
    /* [WRONG_PASSWORD + 1] = */ "Wrong password (2 tries left)",
    /* [WRONG_PASSWORD + 2] = */ "Wrong password (3 tries left)",
    /* [WRONG_PASSWORD + 3] = */ "Wrong password (4 tries left)",
};

#define NO_ARG_INT_RETURN(cmd) \
    do { \
        if (strcmp(argv[1], #cmd) == 0) { \
            int32_t ret = service->cmd(); \
            if (ret < 0) { \
                fprintf(stderr, "%s: could not connect: %d\n", argv[0], ret); \
                return 1; \
            } else { \
                printf(#cmd ": %s (%d)\n", responses[ret], ret); \
                return 0; \
            } \
        } \
    } while (0)

#define SINGLE_ARG_INT_RETURN(cmd) \
    do { \
        if (strcmp(argv[1], #cmd) == 0) { \
            if (argc < 3) { \
                fprintf(stderr, "Usage: %s " #cmd " <name>\n", argv[0]); \
                return 1; \
            } \
            int32_t ret = service->cmd(String16(argv[2])); \
            if (ret < 0) { \
                fprintf(stderr, "%s: could not connect: %d\n", argv[0], ret); \
                return 1; \
            } else { \
                printf(#cmd ": %s (%d)\n", responses[ret], ret); \
                return 0; \
            } \
        } \
    } while (0)

#define SINGLE_ARG_PLUS_UID_INT_RETURN(cmd) \
    do { \
        if (strcmp(argv[1], #cmd) == 0) { \
            if (argc < 3) { \
                fprintf(stderr, "Usage: %s " #cmd " <name> <uid>\n", argv[0]); \
                return 1; \
            } \
            int uid = -1; \
            if (argc > 3) { \
                uid = atoi(argv[3]); \
                fprintf(stderr, "Running as uid %d\n", uid); \
            } \
            int32_t ret = service->cmd(String16(argv[2]), uid); \
            if (ret < 0) { \
                fprintf(stderr, "%s: could not connect: %d\n", argv[0], ret); \
                return 1; \
            } else { \
                printf(#cmd ": %s (%d)\n", responses[ret], ret); \
                return 0; \
            } \
        } \
    } while (0)

#define STING_ARG_DATA_STDIN_INT_RETURN(cmd) \
    do { \
        if (strcmp(argv[1], #cmd) == 0) { \
            if (argc < 3) { \
                fprintf(stderr, "Usage: %s " #cmd " <name>\n", argv[0]); \
                return 1; \
            } \
            uint8_t* data; \
            size_t dataSize; \
            read_input(&data, &dataSize); \
            int32_t ret = service->cmd(String16(argv[2]), data, dataSize); \
            if (ret < 0) { \
                fprintf(stderr, "%s: could not connect: %d\n", argv[0], ret); \
                return 1; \
            } else { \
                printf(#cmd ": %s (%d)\n", responses[ret], ret); \
                return 0; \
            } \
        } \
    } while (0)

#define SINGLE_ARG_DATA_RETURN(cmd) \
    do { \
        if (strcmp(argv[1], #cmd) == 0) { \
            if (argc < 3) { \
                fprintf(stderr, "Usage: %s " #cmd " <name>\n", argv[0]); \
                return 1; \
            } \
            uint8_t* data; \
            size_t dataSize; \
            int32_t ret = service->cmd(String16(argv[2]), &data, &dataSize); \
            if (ret < 0) { \
                fprintf(stderr, "%s: could not connect: %d\n", argv[0], ret); \
                return 1; \
            } else if (ret != ::NO_ERROR) { \
                fprintf(stderr, "%s: " #cmd ": %s (%d)\n", argv[0], responses[ret], ret); \
                return 1; \
            } else { \
                fwrite(data, dataSize, 1, stdout); \
                fflush(stdout); \
                free(data); \
                return 0; \
            } \
        } \
    } while (0)

static int saw(sp<IKeystoreService> service, const String16& name, int uid) {
    Vector<String16> matches;
    int32_t ret = service->saw(name, uid, &matches);
    if (ret < 0) {
        fprintf(stderr, "saw: could not connect: %d\n", ret);
        return 1;
    } else if (ret != ::NO_ERROR) {
        fprintf(stderr, "saw: %s (%d)\n", responses[ret], ret);
        return 1;
    } else {
        Vector<String16>::const_iterator it = matches.begin();
        for (; it != matches.end(); ++it) {
            printf("%s\n", String8(*it).string());
        }
        return 0;
    }
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s action [parameter ...]\n", argv[0]);
        return 1;
    }

    sp<IServiceManager> sm = defaultServiceManager();
    sp<IBinder> binder = sm->getService(String16("android.security.keystore"));
    sp<IKeystoreService> service = interface_cast<IKeystoreService>(binder);

    if (service == NULL) {
        fprintf(stderr, "%s: error: could not connect to keystore service\n", argv[0]);
        return 1;
    }

    /*
     * All the commands should return a value
     */

    NO_ARG_INT_RETURN(test);

    SINGLE_ARG_DATA_RETURN(get);

    // TODO: insert

    SINGLE_ARG_PLUS_UID_INT_RETURN(del);

    SINGLE_ARG_PLUS_UID_INT_RETURN(exist);

    if (strcmp(argv[1], "saw") == 0) {
        return saw(service, argc < 3 ? String16("") : String16(argv[2]),
                argc < 4 ? -1 : atoi(argv[3]));
    }

    NO_ARG_INT_RETURN(reset);

    SINGLE_ARG_INT_RETURN(password);

    NO_ARG_INT_RETURN(lock);

    SINGLE_ARG_INT_RETURN(unlock);

    NO_ARG_INT_RETURN(zero);

    // TODO: generate

    SINGLE_ARG_DATA_RETURN(get_pubkey);

    SINGLE_ARG_PLUS_UID_INT_RETURN(del_key);

    // TODO: grant

    // TODO: ungrant

    // TODO: getmtime

    fprintf(stderr, "%s: unknown command: %s\n", argv[0], argv[1]);
    return 1;
}
