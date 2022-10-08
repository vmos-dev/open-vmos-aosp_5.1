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

#define LOG_NDEBUG 0

#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <cutils/properties.h>

#define LOG_TAG "NatController"
#include <cutils/log.h>
#include <logwrap/logwrap.h>

#include "NatController.h"
#include "NetdConstants.h"
#include "RouteController.h"

const char* NatController::LOCAL_FORWARD = "natctrl_FORWARD";
const char* NatController::LOCAL_MANGLE_FORWARD = "natctrl_mangle_FORWARD";
const char* NatController::LOCAL_NAT_POSTROUTING = "natctrl_nat_POSTROUTING";
const char* NatController::LOCAL_TETHER_COUNTERS_CHAIN = "natctrl_tether_counters";

NatController::NatController() {
}

NatController::~NatController() {
}

struct CommandsAndArgs {
    /* The array size doesn't really matter as the compiler will barf if too many initializers are specified. */
    const char *cmd[32];
    bool checkRes;
};

int NatController::runCmd(int argc, const char **argv) {
    return 0;
    int res;

    res = android_fork_execvp(argc, (char **)argv, NULL, false, false);

#if !LOG_NDEBUG
    std::string full_cmd = argv[0];
    argc--; argv++;
    /*
     * HACK: Sometimes runCmd() is called with a ridcously large value (32)
     * and it works because the argv[] contains a NULL after the last
     * true argv. So here we use the NULL argv[] to terminate when the argc
     * is horribly wrong, and argc for the normal cases.
     */
    for (; argc && argv[0]; argc--, argv++) {
        full_cmd += " ";
        full_cmd += argv[0];
    }
    ALOGV("runCmd(%s) res=%d", full_cmd.c_str(), res);
#endif
    return res;
}

int NatController::setupIptablesHooks() {
    int res;
    res = setDefaults();
    if (res < 0) {
        return res;
    }

    struct CommandsAndArgs defaultCommands[] = {
        /*
         * First chain is for tethering counters.
         * This chain is reached via --goto, and then RETURNS.
         *
         * Second chain is used to limit downstream mss to the upstream pmtu
         * so we don't end up fragmenting every large packet tethered devices
         * send.  Note this feature requires kernel support with flag
         * CONFIG_NETFILTER_XT_TARGET_TCPMSS=y, which not all builds will have,
         * so the final rule is allowed to fail.
         * Bug 17629786 asks to make the failure more obvious, or even fatal
         * so that all builds eventually gain the performance improvement.
         */
        {{IPTABLES_PATH, "-F", LOCAL_TETHER_COUNTERS_CHAIN,}, 0},
        {{IPTABLES_PATH, "-X", LOCAL_TETHER_COUNTERS_CHAIN,}, 0},
        {{IPTABLES_PATH, "-N", LOCAL_TETHER_COUNTERS_CHAIN,}, 1},
        {{IPTABLES_PATH, "-t", "mangle", "-A", LOCAL_MANGLE_FORWARD, "-p", "tcp", "--tcp-flags",
                "SYN", "SYN", "-j", "TCPMSS", "--clamp-mss-to-pmtu"}, 0},
    };
    for (unsigned int cmdNum = 0; cmdNum < ARRAY_SIZE(defaultCommands); cmdNum++) {
        if (runCmd(ARRAY_SIZE(defaultCommands[cmdNum].cmd), defaultCommands[cmdNum].cmd) &&
            defaultCommands[cmdNum].checkRes) {
                return -1;
        }
    }
    ifacePairList.clear();

    return 0;
}

int NatController::setDefaults() {
    /*
     * The following only works because:
     *  - the defaultsCommands[].cmd array is padded with NULL, and
     *  - the 1st argc of runCmd() will just be the max for the CommandsAndArgs[].cmd, and
     *  - internally it will be memcopied to an array and terminated with a NULL.
     */
    struct CommandsAndArgs defaultCommands[] = {
        {{IPTABLES_PATH, "-F", LOCAL_FORWARD,}, 1},
        {{IPTABLES_PATH, "-A", LOCAL_FORWARD, "-j", "DROP"}, 1},
        {{IPTABLES_PATH, "-t", "nat", "-F", LOCAL_NAT_POSTROUTING}, 1},
    };
    for (unsigned int cmdNum = 0; cmdNum < ARRAY_SIZE(defaultCommands); cmdNum++) {
        if (runCmd(ARRAY_SIZE(defaultCommands[cmdNum].cmd), defaultCommands[cmdNum].cmd) &&
            defaultCommands[cmdNum].checkRes) {
                return -1;
        }
    }

    natCount = 0;

    return 0;
}

int NatController::enableNat(const char* intIface, const char* extIface) {
    ALOGV("enableNat(intIface=<%s>, extIface=<%s>)",intIface, extIface);

    if (!isIfaceName(intIface) || !isIfaceName(extIface)) {
        errno = ENODEV;
        return -1;
    }

    /* Bug: b/9565268. "enableNat wlan0 wlan0". For now we fail until java-land is fixed */
    if (!strcmp(intIface, extIface)) {
        ALOGE("Duplicate interface specified: %s %s", intIface, extIface);
        errno = EINVAL;
        return -1;
    }

    // add this if we are the first added nat
    if (natCount == 0) {
        const char *cmd[] = {
                IPTABLES_PATH,
                "-t",
                "nat",
                "-A",
                LOCAL_NAT_POSTROUTING,
                "-o",
                extIface,
                "-j",
                "MASQUERADE"
        };
        if (runCmd(ARRAY_SIZE(cmd), cmd)) {
            ALOGE("Error setting postroute rule: iface=%s", extIface);
            // unwind what's been done, but don't care about success - what more could we do?
            setDefaults();
            return -1;
        }
    }

    if (setForwardRules(true, intIface, extIface) != 0) {
        ALOGE("Error setting forward rules");
        if (natCount == 0) {
            setDefaults();
        }
        errno = ENODEV;
        return -1;
    }

    /* Always make sure the drop rule is at the end */
    const char *cmd1[] = {
            IPTABLES_PATH,
            "-D",
            LOCAL_FORWARD,
            "-j",
            "DROP"
    };
    runCmd(ARRAY_SIZE(cmd1), cmd1);
    const char *cmd2[] = {
            IPTABLES_PATH,
            "-A",
            LOCAL_FORWARD,
            "-j",
            "DROP"
    };
    runCmd(ARRAY_SIZE(cmd2), cmd2);

    if (int ret = RouteController::enableTethering(intIface, extIface)) {
        ALOGE("failed to add tethering rule for iif=%s oif=%s", intIface, extIface);
        if (natCount == 0) {
            setDefaults();
        }
        errno = -ret;
        return -1;
    }

    natCount++;
    return 0;
}

bool NatController::checkTetherCountingRuleExist(const char *pair_name) {
    std::list<std::string>::iterator it;

    for (it = ifacePairList.begin(); it != ifacePairList.end(); it++) {
        if (*it == pair_name) {
            /* We already have this counter */
            return true;
        }
    }
    return false;
}

int NatController::setTetherCountingRules(bool add, const char *intIface, const char *extIface) {

    /* We only ever add tethering quota rules so that they stick. */
    if (!add) {
        return 0;
    }
    char *pair_name;
    asprintf(&pair_name, "%s_%s", intIface, extIface);

    if (checkTetherCountingRuleExist(pair_name)) {
        free(pair_name);
        return 0;
    }
    const char *cmd2b[] = {
            IPTABLES_PATH,
            "-A",
            LOCAL_TETHER_COUNTERS_CHAIN,
            "-i",
            intIface,
            "-o",
            extIface,
            "-j",
          "RETURN"
    };

    if (runCmd(ARRAY_SIZE(cmd2b), cmd2b) && add) {
        free(pair_name);
        return -1;
    }
    ifacePairList.push_front(pair_name);
    free(pair_name);

    asprintf(&pair_name, "%s_%s", extIface, intIface);
    if (checkTetherCountingRuleExist(pair_name)) {
        free(pair_name);
        return 0;
    }

    const char *cmd3b[] = {
            IPTABLES_PATH,
            "-A",
            LOCAL_TETHER_COUNTERS_CHAIN,
            "-i",
            extIface,
            "-o",
            intIface,
            "-j",
            "RETURN"
    };

    if (runCmd(ARRAY_SIZE(cmd3b), cmd3b) && add) {
        // unwind what's been done, but don't care about success - what more could we do?
        free(pair_name);
        return -1;
    }
    ifacePairList.push_front(pair_name);
    free(pair_name);
    return 0;
}

int NatController::setForwardRules(bool add, const char *intIface, const char *extIface) {
    const char *cmd1[] = {
            IPTABLES_PATH,
            add ? "-A" : "-D",
            LOCAL_FORWARD,
            "-i",
            extIface,
            "-o",
            intIface,
            "-m",
            "state",
            "--state",
            "ESTABLISHED,RELATED",
            "-g",
            LOCAL_TETHER_COUNTERS_CHAIN
    };
    int rc = 0;

    if (runCmd(ARRAY_SIZE(cmd1), cmd1) && add) {
        return -1;
    }

    const char *cmd2[] = {
            IPTABLES_PATH,
            add ? "-A" : "-D",
            LOCAL_FORWARD,
            "-i",
            intIface,
            "-o",
            extIface,
            "-m",
            "state",
            "--state",
            "INVALID",
            "-j",
            "DROP"
    };

    const char *cmd3[] = {
            IPTABLES_PATH,
            add ? "-A" : "-D",
            LOCAL_FORWARD,
            "-i",
            intIface,
            "-o",
            extIface,
            "-g",
            LOCAL_TETHER_COUNTERS_CHAIN
    };

    if (runCmd(ARRAY_SIZE(cmd2), cmd2) && add) {
        // bail on error, but only if adding
        rc = -1;
        goto err_invalid_drop;
    }

    if (runCmd(ARRAY_SIZE(cmd3), cmd3) && add) {
        // unwind what's been done, but don't care about success - what more could we do?
        rc = -1;
        goto err_return;
    }

    if (setTetherCountingRules(add, intIface, extIface) && add) {
        rc = -1;
        goto err_return;
    }

    return 0;

err_return:
    cmd2[1] = "-D";
    runCmd(ARRAY_SIZE(cmd2), cmd2);
err_invalid_drop:
    cmd1[1] = "-D";
    runCmd(ARRAY_SIZE(cmd1), cmd1);
    return rc;
}

int NatController::disableNat(const char* intIface, const char* extIface) {
    if (!isIfaceName(intIface) || !isIfaceName(extIface)) {
        errno = ENODEV;
        return -1;
    }

    if (int ret = RouteController::disableTethering(intIface, extIface)) {
        ALOGE("failed to remove tethering rule for iif=%s oif=%s", intIface, extIface);
        errno = -ret;
        return -1;
    }

    setForwardRules(false, intIface, extIface);
    if (--natCount <= 0) {
        // handle decrement to 0 case (do reset to defaults) and erroneous dec below 0
        setDefaults();
    }
    return 0;
}
