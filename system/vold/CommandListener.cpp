/*
 * Copyright (C) 2008 The Android Open Source Project
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

#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fs_mgr.h>
#include <string.h>

#define LOG_TAG "VoldCmdListener"
#include <cutils/log.h>

#include <sysutils/SocketClient.h>
#include <private/android_filesystem_config.h>

#include "CommandListener.h"
#include "VolumeManager.h"
#include "ResponseCode.h"
#include "Process.h"
#include "Loop.h"
#include "Devmapper.h"
#include "cryptfs.h"
#include "fstrim.h"

#define DUMP_ARGS 0

CommandListener::CommandListener() :
                 FrameworkListener("vold", true) {
    registerCmd(new DumpCmd());
    registerCmd(new VolumeCmd());
    registerCmd(new AsecCmd());
    registerCmd(new ObbCmd());
    registerCmd(new StorageCmd());
    registerCmd(new CryptfsCmd());
    registerCmd(new FstrimCmd());
}

#if DUMP_ARGS
void CommandListener::dumpArgs(int argc, char **argv, int argObscure) {
    char buffer[4096];
    char *p = buffer;

    memset(buffer, 0, sizeof(buffer));
    int i;
    for (i = 0; i < argc; i++) {
        unsigned int len = strlen(argv[i]) + 1; // Account for space
        if (i == argObscure) {
            len += 2; // Account for {}
        }
        if (((p - buffer) + len) < (sizeof(buffer)-1)) {
            if (i == argObscure) {
                *p++ = '{';
                *p++ = '}';
                *p++ = ' ';
                continue;
            }
            strcpy(p, argv[i]);
            p+= strlen(argv[i]);
            if (i != (argc -1)) {
                *p++ = ' ';
            }
        }
    }
    SLOGD("%s", buffer);
}
#else
void CommandListener::dumpArgs(int /*argc*/, char ** /*argv*/, int /*argObscure*/) { }
#endif

CommandListener::DumpCmd::DumpCmd() :
                 VoldCommand("dump") {
}

int CommandListener::DumpCmd::runCommand(SocketClient *cli,
                                         int /*argc*/, char ** /*argv*/) {
    cli->sendMsg(0, "Dumping loop status", false);
    if (Loop::dumpState(cli)) {
        cli->sendMsg(ResponseCode::CommandOkay, "Loop dump failed", true);
    }
    cli->sendMsg(0, "Dumping DM status", false);
    if (Devmapper::dumpState(cli)) {
        cli->sendMsg(ResponseCode::CommandOkay, "Devmapper dump failed", true);
    }
    cli->sendMsg(0, "Dumping mounted filesystems", false);
    FILE *fp = fopen("/proc/mounts", "r");
    if (fp) {
        char line[1024];
        while (fgets(line, sizeof(line), fp)) {
            line[strlen(line)-1] = '\0';
            cli->sendMsg(0, line, false);;
        }
        fclose(fp);
    }

    cli->sendMsg(ResponseCode::CommandOkay, "dump complete", false);
    return 0;
}

CommandListener::VolumeCmd::VolumeCmd() :
                 VoldCommand("volume") {
}

int CommandListener::VolumeCmd::runCommand(SocketClient *cli,
                                           int argc, char **argv) {
    dumpArgs(argc, argv, -1);

    if (argc < 2) {
        cli->sendMsg(ResponseCode::CommandSyntaxError, "Missing Argument", false);
        return 0;
    }

    VolumeManager *vm = VolumeManager::Instance();
    int rc = 0;

    if (!strcmp(argv[1], "list")) {
        bool broadcast = argc >= 3 && !strcmp(argv[2], "broadcast");
        return vm->listVolumes(cli, broadcast);
    } else if (!strcmp(argv[1], "debug")) {
        if (argc != 3 || (argc == 3 && (strcmp(argv[2], "off") && strcmp(argv[2], "on")))) {
            cli->sendMsg(ResponseCode::CommandSyntaxError, "Usage: volume debug <off/on>", false);
            return 0;
        }
        vm->setDebug(!strcmp(argv[2], "on") ? true : false);
    } else if (!strcmp(argv[1], "mount")) {
        if (argc != 3) {
            cli->sendMsg(ResponseCode::CommandSyntaxError, "Usage: volume mount <path>", false);
            return 0;
        }
        rc = vm->mountVolume(argv[2]);
    } else if (!strcmp(argv[1], "unmount")) {
        if (argc < 3 || argc > 4 ||
           ((argc == 4 && strcmp(argv[3], "force")) &&
            (argc == 4 && strcmp(argv[3], "force_and_revert")))) {
            cli->sendMsg(ResponseCode::CommandSyntaxError, "Usage: volume unmount <path> [force|force_and_revert]", false);
            return 0;
        }

        bool force = false;
        bool revert = false;
        if (argc >= 4 && !strcmp(argv[3], "force")) {
            force = true;
        } else if (argc >= 4 && !strcmp(argv[3], "force_and_revert")) {
            force = true;
            revert = true;
        }
        rc = vm->unmountVolume(argv[2], force, revert);
    } else if (!strcmp(argv[1], "format")) {
        if (argc < 3 || argc > 4 ||
            (argc == 4 && strcmp(argv[3], "wipe"))) {
            cli->sendMsg(ResponseCode::CommandSyntaxError, "Usage: volume format <path> [wipe]", false);
            return 0;
        }
        bool wipe = false;
        if (argc >= 4 && !strcmp(argv[3], "wipe")) {
            wipe = true;
        }
        rc = vm->formatVolume(argv[2], wipe);
    } else if (!strcmp(argv[1], "share")) {
        if (argc != 4) {
            cli->sendMsg(ResponseCode::CommandSyntaxError,
                    "Usage: volume share <path> <method>", false);
            return 0;
        }
        rc = vm->shareVolume(argv[2], argv[3]);
    } else if (!strcmp(argv[1], "unshare")) {
        if (argc != 4) {
            cli->sendMsg(ResponseCode::CommandSyntaxError,
                    "Usage: volume unshare <path> <method>", false);
            return 0;
        }
        rc = vm->unshareVolume(argv[2], argv[3]);
    } else if (!strcmp(argv[1], "shared")) {
        bool enabled = false;
        if (argc != 4) {
            cli->sendMsg(ResponseCode::CommandSyntaxError,
                    "Usage: volume shared <path> <method>", false);
            return 0;
        }

        if (vm->shareEnabled(argv[2], argv[3], &enabled)) {
            cli->sendMsg(
                    ResponseCode::OperationFailed, "Failed to determine share enable state", true);
        } else {
            cli->sendMsg(ResponseCode::ShareEnabledResult,
                    (enabled ? "Share enabled" : "Share disabled"), false);
        }
        return 0;
    } else if (!strcmp(argv[1], "mkdirs")) {
        if (argc != 3) {
            cli->sendMsg(ResponseCode::CommandSyntaxError, "Usage: volume mkdirs <path>", false);
            return 0;
        }
        rc = vm->mkdirs(argv[2]);
    } else {
        cli->sendMsg(ResponseCode::CommandSyntaxError, "Unknown volume cmd", false);
    }

    if (!rc) {
        cli->sendMsg(ResponseCode::CommandOkay, "volume operation succeeded", false);
    } else {
        int erno = errno;
        rc = ResponseCode::convertFromErrno();
        cli->sendMsg(rc, "volume operation failed", true);
    }

    return 0;
}

CommandListener::StorageCmd::StorageCmd() :
                 VoldCommand("storage") {
}

int CommandListener::StorageCmd::runCommand(SocketClient *cli,
                                                      int argc, char **argv) {
    /* Guarantied to be initialized by vold's main() before the CommandListener is active */
    extern struct fstab *fstab;

    dumpArgs(argc, argv, -1);

    if (argc < 2) {
        cli->sendMsg(ResponseCode::CommandSyntaxError, "Missing Argument", false);
        return 0;
    }

    if (!strcmp(argv[1], "mountall")) {
        if (argc != 2) {
            cli->sendMsg(ResponseCode::CommandSyntaxError, "Usage: mountall", false);
            return 0;
        }
        fs_mgr_mount_all(fstab);
        cli->sendMsg(ResponseCode::CommandOkay, "Mountall ran successfully", false);
        return 0;
    }
    if (!strcmp(argv[1], "users")) {
        DIR *dir;
        struct dirent *de;

        if (argc < 3) {
            cli->sendMsg(ResponseCode::CommandSyntaxError, "Missing Argument: user <mountpoint>", false);
            return 0;
        }
        if (!(dir = opendir("/proc"))) {
            cli->sendMsg(ResponseCode::OperationFailed, "Failed to open /proc", true);
            return 0;
        }

        while ((de = readdir(dir))) {
            int pid = Process::getPid(de->d_name);

            if (pid < 0) {
                continue;
            }

            char processName[255];
            Process::getProcessName(pid, processName, sizeof(processName));

            if (Process::checkFileDescriptorSymLinks(pid, argv[2]) ||
                Process::checkFileMaps(pid, argv[2]) ||
                Process::checkSymLink(pid, argv[2], "cwd") ||
                Process::checkSymLink(pid, argv[2], "root") ||
                Process::checkSymLink(pid, argv[2], "exe")) {

                char msg[1024];
                snprintf(msg, sizeof(msg), "%d %s", pid, processName);
                cli->sendMsg(ResponseCode::StorageUsersListResult, msg, false);
            }
        }
        closedir(dir);
        cli->sendMsg(ResponseCode::CommandOkay, "Storage user list complete", false);
    } else {
        cli->sendMsg(ResponseCode::CommandSyntaxError, "Unknown storage cmd", false);
    }
    return 0;
}

CommandListener::AsecCmd::AsecCmd() :
                 VoldCommand("asec") {
}

void CommandListener::AsecCmd::listAsecsInDirectory(SocketClient *cli, const char *directory) {
    DIR *d = opendir(directory);

    if (!d) {
        cli->sendMsg(ResponseCode::OperationFailed, "Failed to open asec dir", true);
        return;
    }

    size_t dirent_len = offsetof(struct dirent, d_name) +
            fpathconf(dirfd(d), _PC_NAME_MAX) + 1;

    struct dirent *dent = (struct dirent *) malloc(dirent_len);
    if (dent == NULL) {
        cli->sendMsg(ResponseCode::OperationFailed, "Failed to allocate memory", true);
        return;
    }

    struct dirent *result;

    while (!readdir_r(d, dent, &result) && result != NULL) {
        if (dent->d_name[0] == '.')
            continue;
        if (dent->d_type != DT_REG)
            continue;
        size_t name_len = strlen(dent->d_name);
        if (name_len > 5 && name_len < 260 &&
                !strcmp(&dent->d_name[name_len - 5], ".asec")) {
            char id[255];
            memset(id, 0, sizeof(id));
            strlcpy(id, dent->d_name, name_len - 4);
            cli->sendMsg(ResponseCode::AsecListResult, id, false);
        }
    }
    closedir(d);

    free(dent);
}
bool isLegalAsecId(const char *id) {
    size_t i;
    size_t len = strlen(id);

    if (len == 0) {
        return false;
    }
    if ((id[0] == '.') || (id[len - 1] == '.')) {
        return false;
    }

    for (i = 0; i < len; i++) {
        if (id[i] == '.') {
            // i=0 is guaranteed never to have a dot. See above.
            if (id[i-1] == '.') return false;
            continue;
        }
        if (id[i] == '_' || id[i] == '-') continue;
        if (id[i] >= 'a' && id[i] <= 'z') continue;
        if (id[i] >= 'A' && id[i] <= 'Z') continue;
        if (id[i] >= '0' && id[i] <= '9') continue;
        return false;
    }

    return true;
}
int CommandListener::AsecCmd::runCommand(SocketClient *cli,
                                                      int argc, char **argv) {
    if (argc < 2) {
        cli->sendMsg(ResponseCode::CommandSyntaxError, "Missing Argument", false);
        return 0;
    }

    VolumeManager *vm = VolumeManager::Instance();
    int rc = 0;

    if (!strcmp(argv[1], "list")) {
        dumpArgs(argc, argv, -1);

        listAsecsInDirectory(cli, Volume::SEC_ASECDIR_EXT);
        listAsecsInDirectory(cli, Volume::SEC_ASECDIR_INT);
    } else if (!strcmp(argv[1], "create")) {
        dumpArgs(argc, argv, 5);
        if (argc != 8) {
            cli->sendMsg(ResponseCode::CommandSyntaxError,
                    "Usage: asec create <container-id> <size_mb> <fstype> <key> <ownerUid> "
                    "<isExternal>", false);
            return 0;
        }

        unsigned int numSectors = (atoi(argv[3]) * (1024 * 1024)) / 512;
        const bool isExternal = (atoi(argv[7]) == 1);
        rc = 0;
        // rc = vm->createAsec(argv[2], numSectors, argv[4], argv[5], atoi(argv[6]), isExternal);
    } else if (!strcmp(argv[1], "resize")) {
        dumpArgs(argc, argv, -1);
        if (argc != 5) {
            cli->sendMsg(ResponseCode::CommandSyntaxError, "Usage: asec resize <container-id> <size_mb> <key>", false);
            return 0;
        }
        unsigned int numSectors = (atoi(argv[3]) * (1024 * 1024)) / 512;
        rc = 0;//vm->resizeAsec(argv[2], numSectors, argv[4]);
    } else if (!strcmp(argv[1], "finalize")) {
        dumpArgs(argc, argv, -1);
        if (argc != 3) {
            cli->sendMsg(ResponseCode::CommandSyntaxError, "Usage: asec finalize <container-id>", false);
            return 0;
        }
        rc = 0;//vm->finalizeAsec(argv[2]);
    } else if (!strcmp(argv[1], "fixperms")) {
        dumpArgs(argc, argv, -1);
        if  (argc != 5) {
            cli->sendMsg(ResponseCode::CommandSyntaxError, "Usage: asec fixperms <container-id> <gid> <filename>", false);
            return 0;
        }

        char *endptr;
        gid_t gid = (gid_t) strtoul(argv[3], &endptr, 10);
        if (*endptr != '\0') {
            cli->sendMsg(ResponseCode::CommandSyntaxError, "Usage: asec fixperms <container-id> <gid> <filename>", false);
            return 0;
        }

        rc = 0;//vm->fixupAsecPermissions(argv[2], gid, argv[4]);
    } else if (!strcmp(argv[1], "destroy")) {
        dumpArgs(argc, argv, -1);
        if (argc < 3) {
            cli->sendMsg(ResponseCode::CommandSyntaxError, "Usage: asec destroy <container-id> [force]", false);
            return 0;
        }
        bool force = false;
        if (argc > 3 && !strcmp(argv[3], "force")) {
            force = true;
        }
        rc = 0;//vm->destroyAsec(argv[2], force);
    } else if (!strcmp(argv[1], "mount")) {
        dumpArgs(argc, argv, 3);
        if (argc != 6) {
            cli->sendMsg(ResponseCode::CommandSyntaxError,
                    "Usage: asec mount <namespace-id> <key> <ownerUid> <ro|rw>", false);
            return 0;
        }
        bool readOnly = true;
        if (!strcmp(argv[5], "rw")) {
            readOnly = false;
        }
//        rc = vm->mountAsec(argv[2], argv[3], atoi(argv[4]), readOnly);
          rc = 0;
    } else if (!strcmp(argv[1], "unmount")) {
        dumpArgs(argc, argv, -1);
        if (argc < 3) {
            cli->sendMsg(ResponseCode::CommandSyntaxError, "Usage: asec unmount <container-id> [force]", false);
            return 0;
        }
        bool force = false;
        if (argc > 3 && !strcmp(argv[3], "force")) {
            force = true;
        }
        rc = 0;//vm->unmountAsec(argv[2], force);
    } else if (!strcmp(argv[1], "rename")) {
        dumpArgs(argc, argv, -1);
        if (argc != 4) {
            cli->sendMsg(ResponseCode::CommandSyntaxError,
                    "Usage: asec rename <old_id> <new_id>", false);
            return 0;
        }
        rc = vm->renameAsec(argv[2], argv[3]);
    } else if (!strcmp(argv[1], "path")) {
        dumpArgs(argc, argv, -1);
        if (argc != 3) {
            cli->sendMsg(ResponseCode::CommandSyntaxError, "Usage: asec path <container-id>", false);
            return 0;
        }
        char path[255];
        if (isLegalAsecId(argv[2]))
        {
            sprintf(path, "/data/app/%s/", argv[2]);// Volume::ASECDIR -> /data/app/
            mkdir(path, 0660);
			cli->sendMsg(ResponseCode::AsecPathResult, path, false);
			return 0;
        }

    } else if (!strcmp(argv[1], "fspath")) {
        dumpArgs(argc, argv, -1);
        if (argc != 3) {
            cli->sendMsg(ResponseCode::CommandSyntaxError, "Usage: asec fspath <container-id>", false);
            return 0;
        }
        char path[255];

        if (isLegalAsecId(argv[2]))
        {
			sprintf(path, "/data/app/%s/", argv[2]);    // Volume::ASECDIR -> /data/app/
            mkdir(path, 0660);
            cli->sendMsg(ResponseCode::AsecPathResult, path, false);
            return 0;
        }
    } else {
        dumpArgs(argc, argv, -1);
        cli->sendMsg(ResponseCode::CommandSyntaxError, "Unknown asec cmd", false);
    }

    if (!rc) {
        cli->sendMsg(ResponseCode::CommandOkay, "asec operation succeeded", false);
    } else {
        rc = ResponseCode::convertFromErrno();
        cli->sendMsg(rc, "asec operation failed", true);
    }

    return 0;
}

CommandListener::ObbCmd::ObbCmd() :
                 VoldCommand("obb") {
}

int CommandListener::ObbCmd::runCommand(SocketClient *cli,
                                                      int argc, char **argv) {
    if (argc < 2) {
        cli->sendMsg(ResponseCode::CommandSyntaxError, "Missing Argument", false);
        return 0;
    }

    VolumeManager *vm = VolumeManager::Instance();
    int rc = 0;

    if (!strcmp(argv[1], "list")) {
        dumpArgs(argc, argv, -1);

        rc = vm->listMountedObbs(cli);
    } else if (!strcmp(argv[1], "mount")) {
            dumpArgs(argc, argv, 3);
            if (argc != 5) {
                cli->sendMsg(ResponseCode::CommandSyntaxError,
                        "Usage: obb mount <filename> <key> <ownerGid>", false);
                return 0;
            }
            rc = 0;
//            rc = vm->mountObb(argv[2], argv[3], atoi(argv[4]));
    } else if (!strcmp(argv[1], "unmount")) {
        dumpArgs(argc, argv, -1);
        if (argc < 3) {
            cli->sendMsg(ResponseCode::CommandSyntaxError, "Usage: obb unmount <source file> [force]", false);
            return 0;
        }
        bool force = false;
        if (argc > 3 && !strcmp(argv[3], "force")) {
            force = true;
        }
        rc = 0;
//        rc = vm->unmountObb(argv[2], force);
    } else if (!strcmp(argv[1], "path")) {
        dumpArgs(argc, argv, -1);
        if (argc != 3) {
            cli->sendMsg(ResponseCode::CommandSyntaxError, "Usage: obb path <source file>", false);
            return 0;
        }
        char path[255];

        if (!(rc = vm->getObbMountPath(argv[2], path, sizeof(path)))) {
            cli->sendMsg(ResponseCode::AsecPathResult, path, false);
            return 0;
        }
    } else {
        dumpArgs(argc, argv, -1);
        cli->sendMsg(ResponseCode::CommandSyntaxError, "Unknown obb cmd", false);
    }

    if (!rc) {
        cli->sendMsg(ResponseCode::CommandOkay, "obb operation succeeded", false);
    } else {
        rc = ResponseCode::convertFromErrno();
        cli->sendMsg(rc, "obb operation failed", true);
    }

    return 0;
}

CommandListener::CryptfsCmd::CryptfsCmd() :
                 VoldCommand("cryptfs") {
}

static int getType(const char* type)
{
    if (!strcmp(type, "default")) {
        return CRYPT_TYPE_DEFAULT;
    } else if (!strcmp(type, "password")) {
        return CRYPT_TYPE_PASSWORD;
    } else if (!strcmp(type, "pin")) {
        return CRYPT_TYPE_PIN;
    } else if (!strcmp(type, "pattern")) {
        return CRYPT_TYPE_PATTERN;
    } else {
        return -1;
    }
}

int CommandListener::CryptfsCmd::runCommand(SocketClient *cli,
                                                      int argc, char **argv) {
    if ((cli->getUid() != 0) && (cli->getUid() != AID_SYSTEM)) {
        cli->sendMsg(ResponseCode::CommandNoPermission, "No permission to run cryptfs commands", false);
        return 0;
    }

    if (argc < 2) {
        cli->sendMsg(ResponseCode::CommandSyntaxError, "Missing Argument", false);
        return 0;
    }

    int rc = 0;

    if (!strcmp(argv[1], "checkpw")) {
        if (argc != 3) {
            cli->sendMsg(ResponseCode::CommandSyntaxError, "Usage: cryptfs checkpw <passwd>", false);
            return 0;
        }
        dumpArgs(argc, argv, 2);
        rc = cryptfs_check_passwd(argv[2]);
    } else if (!strcmp(argv[1], "restart")) {
        if (argc != 2) {
            cli->sendMsg(ResponseCode::CommandSyntaxError, "Usage: cryptfs restart", false);
            return 0;
        }
        dumpArgs(argc, argv, -1);
        rc = cryptfs_restart();
    } else if (!strcmp(argv[1], "cryptocomplete")) {
        if (argc != 2) {
            cli->sendMsg(ResponseCode::CommandSyntaxError, "Usage: cryptfs cryptocomplete", false);
            return 0;
        }
        dumpArgs(argc, argv, -1);
        rc = cryptfs_crypto_complete();
    } else if (!strcmp(argv[1], "enablecrypto")) {
        const char* syntax = "Usage: cryptfs enablecrypto <wipe|inplace> "
                             "default|password|pin|pattern [passwd]";
        if ( (argc != 4 && argc != 5)
             || (strcmp(argv[2], "wipe") && strcmp(argv[2], "inplace")) ) {
            cli->sendMsg(ResponseCode::CommandSyntaxError, syntax, false);
            return 0;
        }
        dumpArgs(argc, argv, 4);

        int tries;
        for (tries = 0; tries < 2; ++tries) {
            int type = getType(argv[3]);
            if (type == -1) {
                cli->sendMsg(ResponseCode::CommandSyntaxError, syntax,
                             false);
                return 0;
            } else if (type == CRYPT_TYPE_DEFAULT) {
              rc = cryptfs_enable_default(argv[2], /*allow_reboot*/false);
            } else {
                rc = cryptfs_enable(argv[2], type, argv[4],
                                    /*allow_reboot*/false);
            }

            if (rc == 0) {
                break;
            } else if (tries == 0) {
                Process::killProcessesWithOpenFiles(DATA_MNT_POINT, 2);
            }
        }
    } else if (!strcmp(argv[1], "changepw")) {
        const char* syntax = "Usage: cryptfs changepw "
                             "default|password|pin|pattern [newpasswd]";
        const char* password;
        if (argc == 3) {
            password = "";
        } else if (argc == 4) {
            password = argv[3];
        } else {
            cli->sendMsg(ResponseCode::CommandSyntaxError, syntax, false);
            return 0;
        }
        int type = getType(argv[2]);
        if (type == -1) {
            cli->sendMsg(ResponseCode::CommandSyntaxError, syntax, false);
            return 0;
        }
        SLOGD("cryptfs changepw %s {}", argv[2]);
        rc = cryptfs_changepw(type, password);
    } else if (!strcmp(argv[1], "verifypw")) {
        if (argc != 3) {
            cli->sendMsg(ResponseCode::CommandSyntaxError, "Usage: cryptfs verifypw <passwd>", false);
            return 0;
        }
        SLOGD("cryptfs verifypw {}");
        rc = cryptfs_verify_passwd(argv[2]);
    } else if (!strcmp(argv[1], "getfield")) {
        char *valbuf;
        int valbuf_len = PROPERTY_VALUE_MAX;

        if (argc != 3) {
            cli->sendMsg(ResponseCode::CommandSyntaxError, "Usage: cryptfs getfield <fieldname>", false);
            return 0;
        }
        dumpArgs(argc, argv, -1);

        // Increase the buffer size until it is big enough for the field value stored.
        while (1) {
            valbuf = (char*)malloc(valbuf_len);
            if (valbuf == NULL) {
                cli->sendMsg(ResponseCode::OperationFailed, "Failed to allocate memory", false);
                return 0;
            }
            rc = cryptfs_getfield(argv[2], valbuf, valbuf_len);
            if (rc != CRYPTO_GETFIELD_ERROR_BUF_TOO_SMALL) {
                break;
            }
            free(valbuf);
            valbuf_len *= 2;
        }
        if (rc == CRYPTO_GETFIELD_OK) {
            cli->sendMsg(ResponseCode::CryptfsGetfieldResult, valbuf, false);
        }
        free(valbuf);
    } else if (!strcmp(argv[1], "setfield")) {
        if (argc != 4) {
            cli->sendMsg(ResponseCode::CommandSyntaxError, "Usage: cryptfs setfield <fieldname> <value>", false);
            return 0;
        }
        dumpArgs(argc, argv, -1);
        rc = cryptfs_setfield(argv[2], argv[3]);
    } else if (!strcmp(argv[1], "mountdefaultencrypted")) {
        SLOGD("cryptfs mountdefaultencrypted");
        dumpArgs(argc, argv, -1);
        rc = cryptfs_mount_default_encrypted();
    } else if (!strcmp(argv[1], "getpwtype")) {
        SLOGD("cryptfs getpwtype");
        dumpArgs(argc, argv, -1);
        switch(cryptfs_get_password_type()) {
        case CRYPT_TYPE_PASSWORD:
            cli->sendMsg(ResponseCode::PasswordTypeResult, "password", false);
            return 0;
        case CRYPT_TYPE_PATTERN:
            cli->sendMsg(ResponseCode::PasswordTypeResult, "pattern", false);
            return 0;
        case CRYPT_TYPE_PIN:
            cli->sendMsg(ResponseCode::PasswordTypeResult, "pin", false);
            return 0;
        case CRYPT_TYPE_DEFAULT:
            cli->sendMsg(ResponseCode::PasswordTypeResult, "default", false);
            return 0;
        default:
          /** @TODO better error and make sure handled by callers */
            cli->sendMsg(ResponseCode::OpFailedStorageNotFound, "Error", false);
            return 0;
        }
    } else if (!strcmp(argv[1], "getpw")) {
        SLOGD("cryptfs getpw");
        dumpArgs(argc, argv, -1);
        char* password = cryptfs_get_password();
        if (password) {
            char* message = 0;
            int size = asprintf(&message, "{{sensitive}} %s", password);
            if (size != -1) {
                cli->sendMsg(ResponseCode::CommandOkay, message, false);
                memset(message, 0, size);
                free (message);
                return 0;
            }
        }
        rc = -1;
    } else if (!strcmp(argv[1], "clearpw")) {
        SLOGD("cryptfs clearpw");
        dumpArgs(argc, argv, -1);
        cryptfs_clear_password();
        rc = 0;
    } else {
        dumpArgs(argc, argv, -1);
        cli->sendMsg(ResponseCode::CommandSyntaxError, "Unknown cryptfs cmd", false);
        return 0;
    }

    // Always report that the command succeeded and return the error code.
    // The caller will check the return value to see what the error was.
    char msg[255];
    snprintf(msg, sizeof(msg), "%d", rc);
    cli->sendMsg(ResponseCode::CommandOkay, msg, false);

    return 0;
}

CommandListener::FstrimCmd::FstrimCmd() :
                 VoldCommand("fstrim") {
}
int CommandListener::FstrimCmd::runCommand(SocketClient *cli,
                                                      int argc, char **argv) {
    if ((cli->getUid() != 0) && (cli->getUid() != AID_SYSTEM)) {
        cli->sendMsg(ResponseCode::CommandNoPermission, "No permission to run fstrim commands", false);
        return 0;
    }

    if (argc < 2) {
        cli->sendMsg(ResponseCode::CommandSyntaxError, "Missing Argument", false);
        return 0;
    }

    int rc = 0;

#if 0   // CRASH
    if (!strcmp(argv[1], "dotrim")) {
        if (argc != 2) {
            cli->sendMsg(ResponseCode::CommandSyntaxError, "Usage: fstrim dotrim", false);
            return 0;
        }
        dumpArgs(argc, argv, -1);
        rc = fstrim_filesystems(0);
    } else if (!strcmp(argv[1], "dodtrim")) {
        if (argc != 2) {
            cli->sendMsg(ResponseCode::CommandSyntaxError, "Usage: fstrim dodtrim", false);
            return 0;
        }
        dumpArgs(argc, argv, -1);
        rc = fstrim_filesystems(1);   /* Do Deep Discard trim */
    } else {
        dumpArgs(argc, argv, -1);
        cli->sendMsg(ResponseCode::CommandSyntaxError, "Unknown fstrim cmd", false);
    }
#endif
    // Always report that the command succeeded and return the error code.
    // The caller will check the return value to see what the error was.
    char msg[255];
    snprintf(msg, sizeof(msg), "%d", rc);
    cli->sendMsg(ResponseCode::CommandOkay, msg, false);

    return 0;
}
