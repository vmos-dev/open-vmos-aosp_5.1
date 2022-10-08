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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#define LOG_TAG "OemIptablesHook"
#include <cutils/log.h>
#include <logwrap/logwrap.h>
#include "NetdConstants.h"

static int runIptablesCmd(int argc, const char **argv) {
    return 0;
    int res;

    res = android_fork_execvp(argc, (char **)argv, NULL, false, false);
    return res;
}

static bool oemCleanupHooks() {
    const char *cmd1[] = {
            IPTABLES_PATH,
            "-F",
            "oem_out"
    };
    runIptablesCmd(ARRAY_SIZE(cmd1), cmd1);

    const char *cmd2[] = {
            IPTABLES_PATH,
            "-F",
            "oem_fwd"
    };
    runIptablesCmd(ARRAY_SIZE(cmd2), cmd2);

    const char *cmd3[] = {
            IPTABLES_PATH,
            "-t",
            "nat",
            "-F",
            "oem_nat_pre"
    };
    runIptablesCmd(ARRAY_SIZE(cmd3), cmd3);
    return true;
}

static bool oemInitChains() {
    int ret = system(OEM_SCRIPT_PATH);
    if ((-1 == ret) || (0 != WEXITSTATUS(ret))) {
        ALOGE("%s failed: %s", OEM_SCRIPT_PATH, strerror(errno));
        oemCleanupHooks();
        return false;
    }
    return true;
}


void setupOemIptablesHook() {
    if (0 == access(OEM_SCRIPT_PATH, R_OK | X_OK)) {
        // The call to oemCleanupHooks() is superfluous when done on bootup,
        // but is needed for the case where netd has crashed/stopped and is
        // restarted.
        if (oemCleanupHooks() && oemInitChains()) {
            ALOGI("OEM iptable hook installed.");
        }
    }
}
