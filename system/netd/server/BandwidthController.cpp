/*
 * Copyright (C) 2011 The Android Open Source Project
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

// #define LOG_NDEBUG 0

/*
 * The CommandListener, FrameworkListener don't allow for
 * multiple calls in parallel to reach the BandwidthController.
 * If they ever were to allow it, then netd/ would need some tweaking.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define __STDC_FORMAT_MACROS 1
#include <inttypes.h>

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/pkt_sched.h>

#define LOG_TAG "BandwidthController"
#include <cutils/log.h>
#include <cutils/properties.h>
#include <logwrap/logwrap.h>

#include "NetdConstants.h"
#include "BandwidthController.h"
#include "NatController.h"  /* For LOCAL_TETHER_COUNTERS_CHAIN */
#include "ResponseCode.h"

/* Alphabetical */
#define ALERT_IPT_TEMPLATE "%s %s -m quota2 ! --quota %" PRId64" --name %s"
const char BandwidthController::ALERT_GLOBAL_NAME[] = "globalAlert";
const char* BandwidthController::LOCAL_INPUT = "bw_INPUT";
const char* BandwidthController::LOCAL_FORWARD = "bw_FORWARD";
const char* BandwidthController::LOCAL_OUTPUT = "bw_OUTPUT";
const char* BandwidthController::LOCAL_RAW_PREROUTING = "bw_raw_PREROUTING";
const char* BandwidthController::LOCAL_MANGLE_POSTROUTING = "bw_mangle_POSTROUTING";
const int  BandwidthController::MAX_CMD_ARGS = 32;
const int  BandwidthController::MAX_CMD_LEN = 1024;
const int  BandwidthController::MAX_IFACENAME_LEN = 64;
const int  BandwidthController::MAX_IPT_OUTPUT_LINE_LEN = 256;

/**
 * Some comments about the rules:
 *  * Ordering
 *    - when an interface is marked as costly it should be INSERTED into the INPUT/OUTPUT chains.
 *      E.g. "-I bw_INPUT -i rmnet0 --jump costly"
 *    - quota'd rules in the costly chain should be before bw_penalty_box lookups.
 *    - bw_happy_box rejects everything by default.
 *    - the qtaguid counting is done at the end of the bw_INPUT/bw_OUTPUT user chains.
 *
 * * global quota vs per interface quota
 *   - global quota for all costly interfaces uses a single costly chain:
 *    . initial rules
 *      iptables -N bw_costly_shared
 *      iptables -I bw_INPUT -i iface0 --jump bw_costly_shared
 *      iptables -I bw_OUTPUT -o iface0 --jump bw_costly_shared
 *      iptables -I bw_costly_shared -m quota \! --quota 500000 \
 *          --jump REJECT --reject-with icmp-net-prohibited
 *      iptables -A bw_costly_shared --jump bw_penalty_box
 *      If the happy box is enabled,
 *        iptables -A bw_penalty_box --jump bw_happy_box
 *
 *    . adding a new iface to this, E.g.:
 *      iptables -I bw_INPUT -i iface1 --jump bw_costly_shared
 *      iptables -I bw_OUTPUT -o iface1 --jump bw_costly_shared
 *
 *   - quota per interface. This is achieve by having "costly" chains per quota.
 *     E.g. adding a new costly interface iface0 with its own quota:
 *      iptables -N bw_costly_iface0
 *      iptables -I bw_INPUT -i iface0 --jump bw_costly_iface0
 *      iptables -I bw_OUTPUT -o iface0 --jump bw_costly_iface0
 *      iptables -A bw_costly_iface0 -m quota \! --quota 500000 \
 *          --jump REJECT --reject-with icmp-port-unreachable
 *      iptables -A bw_costly_iface0 --jump bw_penalty_box
 *
 * * bw_penalty_box handling:
 *  - only one bw_penalty_box for all interfaces
 *   E.g  Adding an app, it has to preserve the appened bw_happy_box, so "-I":
 *    iptables -I bw_penalty_box -m owner --uid-owner app_3 \
 *        --jump REJECT --reject-with icmp-port-unreachable
 *
 * * bw_happy_box handling:
 *  - The bw_happy_box goes at the end of the penalty box.
 *   E.g  Adding a happy app,
 *    iptables -I bw_happy_box -m owner --uid-owner app_3 \
 *        --jump RETURN
 */
const char *BandwidthController::IPT_FLUSH_COMMANDS[] = {
    /*
     * Cleanup rules.
     * Should normally include bw_costly_<iface>, but we rely on the way they are setup
     * to allow coexistance.
     */
    "-F bw_INPUT",
    "-F bw_OUTPUT",
    "-F bw_FORWARD",
    "-F bw_happy_box",
    "-F bw_penalty_box",
    "-F bw_costly_shared",

    "-t raw -F bw_raw_PREROUTING",
    "-t mangle -F bw_mangle_POSTROUTING",
};

/* The cleanup commands assume flushing has been done. */
const char *BandwidthController::IPT_CLEANUP_COMMANDS[] = {
    "-X bw_happy_box",
    "-X bw_penalty_box",
    "-X bw_costly_shared",
};

const char *BandwidthController::IPT_SETUP_COMMANDS[] = {
    "-N bw_happy_box",
    "-N bw_penalty_box",
    "-N bw_costly_shared",
};

const char *BandwidthController::IPT_BASIC_ACCOUNTING_COMMANDS[] = {
    "-A bw_INPUT -m owner --socket-exists", /* This is a tracking rule. */

    "-A bw_OUTPUT -m owner --socket-exists", /* This is a tracking rule. */

    "-A bw_costly_shared --jump bw_penalty_box",

    "-t raw -A bw_raw_PREROUTING -m owner --socket-exists", /* This is a tracking rule. */
    "-t mangle -A bw_mangle_POSTROUTING -m owner --socket-exists", /* This is a tracking rule. */
};

BandwidthController::BandwidthController(void) {
}

int BandwidthController::runIpxtablesCmd(const char *cmd, IptJumpOp jumpHandling,
                                         IptFailureLog failureHandling) {
    int res = 0;

    ALOGV("runIpxtablesCmd(cmd=%s)", cmd);
    res |= runIptablesCmd(cmd, jumpHandling, IptIpV4, failureHandling);
    res |= runIptablesCmd(cmd, jumpHandling, IptIpV6, failureHandling);
    return res;
}

int BandwidthController::StrncpyAndCheck(char *buffer, const char *src, size_t buffSize) {

    memset(buffer, '\0', buffSize);  // strncpy() is not filling leftover with '\0'
    strncpy(buffer, src, buffSize);
    return buffer[buffSize - 1];
}

int BandwidthController::runIptablesCmd(const char *cmd, IptJumpOp jumpHandling,
                                        IptIpVer iptVer, IptFailureLog failureHandling) {
#if 0
    char buffer[MAX_CMD_LEN];
    const char *argv[MAX_CMD_ARGS];
    int argc = 0;
    char *next = buffer;
    char *tmp;
    int res;
    int status = 0;

    std::string fullCmd = cmd;

    switch (jumpHandling) {
    case IptJumpReject:
        /*
         * Must be carefull what one rejects with, as uper layer protocols will just
         * keep on hammering the device until the number of retries are done.
         * For port-unreachable (default), TCP should consider as an abort (RFC1122).
         */
        fullCmd += " --jump REJECT";
        break;
    case IptJumpReturn:
        fullCmd += " --jump RETURN";
        break;
    case IptJumpNoAdd:
        break;
    }

    fullCmd.insert(0, " ");
    fullCmd.insert(0, iptVer == IptIpV4 ? IPTABLES_PATH : IP6TABLES_PATH);

    if (StrncpyAndCheck(buffer, fullCmd.c_str(), sizeof(buffer))) {
        ALOGE("iptables command too long");
        return -1;
    }

    argc = 0;
    while ((tmp = strsep(&next, " "))) {
        argv[argc++] = tmp;
        if (argc >= MAX_CMD_ARGS) {
            ALOGE("iptables argument overflow");
            return -1;
        }
    }

    argv[argc] = NULL;
    res = android_fork_execvp(argc, (char **)argv, &status, false,
            failureHandling == IptFailShow);
    res = res || !WIFEXITED(status) || WEXITSTATUS(status);
    if (res && failureHandling == IptFailShow) {
      ALOGE("runIptablesCmd(): res=%d status=%d failed %s", res, status,
            fullCmd.c_str());
    }
    return res;
#else

    (void) cmd;
    (void) jumpHandling;
    (void) iptVer;
    (void) failureHandling;
    return 0;
#endif
}

void BandwidthController::flushCleanTables(bool doClean) {
    return;
    /* Flush and remove the bw_costly_<iface> tables */
    flushExistingCostlyTables(doClean);

    /* Some of the initialCommands are allowed to fail */
    runCommands(sizeof(IPT_FLUSH_COMMANDS) / sizeof(char*),
            IPT_FLUSH_COMMANDS, RunCmdFailureOk);

    if (doClean) {
        runCommands(sizeof(IPT_CLEANUP_COMMANDS) / sizeof(char*),
                IPT_CLEANUP_COMMANDS, RunCmdFailureOk);
    }
}

int BandwidthController::setupIptablesHooks(void) {

    /* flush+clean is allowed to fail */
    flushCleanTables(true);
    runCommands(sizeof(IPT_SETUP_COMMANDS) / sizeof(char*),
            IPT_SETUP_COMMANDS, RunCmdFailureBad);

    return 0;
}

int BandwidthController::enableBandwidthControl(bool force) {
    int res;
    char value[PROPERTY_VALUE_MAX];

    if (!force) {
            property_get("persist.bandwidth.enable", value, "1");
            if (!strcmp(value, "0"))
                    return 0;
    }

    /* Let's pretend we started from scratch ... */
    sharedQuotaIfaces.clear();
    quotaIfaces.clear();
    naughtyAppUids.clear();
    niceAppUids.clear();
    globalAlertBytes = 0;
    globalAlertTetherCount = 0;
    sharedQuotaBytes = sharedAlertBytes = 0;

    flushCleanTables(false);
    res = runCommands(sizeof(IPT_BASIC_ACCOUNTING_COMMANDS) / sizeof(char*),
            IPT_BASIC_ACCOUNTING_COMMANDS, RunCmdFailureBad);

    return res;

}

int BandwidthController::disableBandwidthControl(void) {

    flushCleanTables(false);
    return 0;
}

int BandwidthController::runCommands(int numCommands, const char *commands[],
                                     RunCmdErrHandling cmdErrHandling) {
    return 0;
    int res = 0;
    IptFailureLog failureLogging = IptFailShow;
    if (cmdErrHandling == RunCmdFailureOk) {
        failureLogging = IptFailHide;
    }
    ALOGV("runCommands(): %d commands", numCommands);
    for (int cmdNum = 0; cmdNum < numCommands; cmdNum++) {
        res = runIpxtablesCmd(commands[cmdNum], IptJumpNoAdd, failureLogging);
        if (res && cmdErrHandling != RunCmdFailureOk)
            return res;
    }
    return 0;
}

std::string BandwidthController::makeIptablesSpecialAppCmd(IptOp op, int uid, const char *chain) {
    std::string res;
    char *buff;
    const char *opFlag;

    switch (op) {
    case IptOpInsert:
        opFlag = "-I";
        break;
    case IptOpAppend:
        ALOGE("Append op not supported for %s uids", chain);
        res = "";
        return res;
        break;
    case IptOpReplace:
        opFlag = "-R";
        break;
    default:
    case IptOpDelete:
        opFlag = "-D";
        break;
    }
    asprintf(&buff, "%s %s -m owner --uid-owner %d", opFlag, chain, uid);
    res = buff;
    free(buff);
    return res;
}

int BandwidthController::enableHappyBox(void) {
    char cmd[MAX_CMD_LEN];
    int res = 0;

    /*
     * We tentatively delete before adding, which helps recovering
     * from bad states (e.g. netd died).
     */

    /* Should not exist, but ignore result if already there. */
    snprintf(cmd, sizeof(cmd), "-N bw_happy_box");
    runIpxtablesCmd(cmd, IptJumpNoAdd);

    /* Should be empty, but clear in case something was wrong. */
    niceAppUids.clear();
    snprintf(cmd, sizeof(cmd), "-F bw_happy_box");
    res |= runIpxtablesCmd(cmd, IptJumpNoAdd);

    snprintf(cmd, sizeof(cmd), "-D bw_penalty_box -j bw_happy_box");
    runIpxtablesCmd(cmd, IptJumpNoAdd);
    snprintf(cmd, sizeof(cmd), "-A bw_penalty_box -j bw_happy_box");
    res |= runIpxtablesCmd(cmd, IptJumpNoAdd);

    /* Reject. Defaulting to prot-unreachable */
    snprintf(cmd, sizeof(cmd), "-D bw_happy_box -j REJECT");
    runIpxtablesCmd(cmd, IptJumpNoAdd);
    snprintf(cmd, sizeof(cmd), "-A bw_happy_box -j REJECT");
    res |= runIpxtablesCmd(cmd, IptJumpNoAdd);

    return res;
}

int BandwidthController::disableHappyBox(void) {
    char cmd[MAX_CMD_LEN];

    /* Best effort */
    snprintf(cmd, sizeof(cmd), "-D bw_penalty_box -j bw_happy_box");
    runIpxtablesCmd(cmd, IptJumpNoAdd);
    niceAppUids.clear();
    snprintf(cmd, sizeof(cmd), "-F bw_happy_box");
    runIpxtablesCmd(cmd, IptJumpNoAdd);
    snprintf(cmd, sizeof(cmd), "-X bw_happy_box");
    runIpxtablesCmd(cmd, IptJumpNoAdd);

    return 0;
}

int BandwidthController::addNaughtyApps(int numUids, char *appUids[]) {
    return manipulateNaughtyApps(numUids, appUids, SpecialAppOpAdd);
}

int BandwidthController::removeNaughtyApps(int numUids, char *appUids[]) {
    return manipulateNaughtyApps(numUids, appUids, SpecialAppOpRemove);
}

int BandwidthController::addNiceApps(int numUids, char *appUids[]) {
    return manipulateNiceApps(numUids, appUids, SpecialAppOpAdd);
}

int BandwidthController::removeNiceApps(int numUids, char *appUids[]) {
    return manipulateNiceApps(numUids, appUids, SpecialAppOpRemove);
}

int BandwidthController::manipulateNaughtyApps(int numUids, char *appStrUids[], SpecialAppOp appOp) {
    return manipulateSpecialApps(numUids, appStrUids, "bw_penalty_box", naughtyAppUids, IptJumpReject, appOp);
}

int BandwidthController::manipulateNiceApps(int numUids, char *appStrUids[], SpecialAppOp appOp) {
    return manipulateSpecialApps(numUids, appStrUids, "bw_happy_box", niceAppUids, IptJumpReturn, appOp);
}


int BandwidthController::manipulateSpecialApps(int numUids, char *appStrUids[],
                                               const char *chain,
                                               std::list<int /*appUid*/> &specialAppUids,
                                               IptJumpOp jumpHandling, SpecialAppOp appOp) {

    int uidNum;
    const char *failLogTemplate;
    IptOp op;
    int appUids[numUids];
    std::string iptCmd;
    std::list<int /*uid*/>::iterator it;

    switch (appOp) {
    case SpecialAppOpAdd:
        op = IptOpInsert;
        failLogTemplate = "Failed to add app uid %s(%d) to %s.";
        break;
    case SpecialAppOpRemove:
        op = IptOpDelete;
        failLogTemplate = "Failed to delete app uid %s(%d) from %s box.";
        break;
    default:
        ALOGE("Unexpected app Op %d", appOp);
        return -1;
    }

    for (uidNum = 0; uidNum < numUids; uidNum++) {
        char *end;
        appUids[uidNum] = strtoul(appStrUids[uidNum], &end, 0);
        if (*end || !*appStrUids[uidNum]) {
            ALOGE(failLogTemplate, appStrUids[uidNum], appUids[uidNum], chain);
            goto fail_parse;
        }
    }

    for (uidNum = 0; uidNum < numUids; uidNum++) {
        int uid = appUids[uidNum];
        for (it = specialAppUids.begin(); it != specialAppUids.end(); it++) {
            if (*it == uid)
                break;
        }
        bool found = (it != specialAppUids.end());

        if (appOp == SpecialAppOpRemove) {
            if (!found) {
                ALOGE("No such appUid %d to remove", uid);
                return -1;
            }
            specialAppUids.erase(it);
        } else {
            if (found) {
                ALOGE("appUid %d exists already", uid);
                return -1;
            }
            specialAppUids.push_front(uid);
        }

        iptCmd = makeIptablesSpecialAppCmd(op, uid, chain);
        if (runIpxtablesCmd(iptCmd.c_str(), jumpHandling)) {
            ALOGE(failLogTemplate, appStrUids[uidNum], uid, chain);
            goto fail_with_uidNum;
        }
    }
    return 0;

fail_with_uidNum:
    /* Try to remove the uid that failed in any case*/
    iptCmd = makeIptablesSpecialAppCmd(IptOpDelete, appUids[uidNum], chain);
    runIpxtablesCmd(iptCmd.c_str(), jumpHandling);
fail_parse:
    return -1;
}

std::string BandwidthController::makeIptablesQuotaCmd(IptOp op, const char *costName, int64_t quota) {
    std::string res;
    char *buff;
    const char *opFlag;

    ALOGV("makeIptablesQuotaCmd(%d, %" PRId64")", op, quota);

    switch (op) {
    case IptOpInsert:
        opFlag = "-I";
        break;
    case IptOpAppend:
        opFlag = "-A";
        break;
    case IptOpReplace:
        opFlag = "-R";
        break;
    default:
    case IptOpDelete:
        opFlag = "-D";
        break;
    }

    // The requried IP version specific --jump REJECT ... will be added later.
    asprintf(&buff, "%s bw_costly_%s -m quota2 ! --quota %" PRId64" --name %s", opFlag, costName, quota,
             costName);
    res = buff;
    free(buff);
    return res;
}

int BandwidthController::prepCostlyIface(const char *ifn, QuotaType quotaType) {
    char cmd[MAX_CMD_LEN];
    int res = 0, res1, res2;
    int ruleInsertPos = 1;
    std::string costString;
    const char *costCString;

    /* The "-N costly" is created upfront, no need to handle it here. */
    switch (quotaType) {
    case QuotaUnique:
        costString = "bw_costly_";
        costString += ifn;
        costCString = costString.c_str();
        /*
         * Flush the bw_costly_<iface> is allowed to fail in case it didn't exist.
         * Creating a new one is allowed to fail in case it existed.
         * This helps with netd restarts.
         */
        snprintf(cmd, sizeof(cmd), "-F %s", costCString);
        res1 = runIpxtablesCmd(cmd, IptJumpNoAdd, IptFailHide);
        snprintf(cmd, sizeof(cmd), "-N %s", costCString);
        res2 = runIpxtablesCmd(cmd, IptJumpNoAdd, IptFailHide);
        res = (res1 && res2) || (!res1 && !res2);

        snprintf(cmd, sizeof(cmd), "-A %s -j bw_penalty_box", costCString);
        res |= runIpxtablesCmd(cmd, IptJumpNoAdd);
        break;
    case QuotaShared:
        costCString = "bw_costly_shared";
        break;
    default:
        ALOGE("Unexpected quotatype %d", quotaType);
        return -1;
    }

    if (globalAlertBytes) {
        /* The alert rule comes 1st */
        ruleInsertPos = 2;
    }

    snprintf(cmd, sizeof(cmd), "-D bw_INPUT -i %s --jump %s", ifn, costCString);
    runIpxtablesCmd(cmd, IptJumpNoAdd, IptFailHide);

    snprintf(cmd, sizeof(cmd), "-I bw_INPUT %d -i %s --jump %s", ruleInsertPos, ifn, costCString);
    res |= runIpxtablesCmd(cmd, IptJumpNoAdd);

    snprintf(cmd, sizeof(cmd), "-D bw_OUTPUT -o %s --jump %s", ifn, costCString);
    runIpxtablesCmd(cmd, IptJumpNoAdd, IptFailHide);

    snprintf(cmd, sizeof(cmd), "-I bw_OUTPUT %d -o %s --jump %s", ruleInsertPos, ifn, costCString);
    res |= runIpxtablesCmd(cmd, IptJumpNoAdd);
    return res;
}

int BandwidthController::cleanupCostlyIface(const char *ifn, QuotaType quotaType) {
    char cmd[MAX_CMD_LEN];
    int res = 0;
    std::string costString;
    const char *costCString;

    switch (quotaType) {
    case QuotaUnique:
        costString = "bw_costly_";
        costString += ifn;
        costCString = costString.c_str();
        break;
    case QuotaShared:
        costCString = "bw_costly_shared";
        break;
    default:
        ALOGE("Unexpected quotatype %d", quotaType);
        return -1;
    }

    snprintf(cmd, sizeof(cmd), "-D bw_INPUT -i %s --jump %s", ifn, costCString);
    res |= runIpxtablesCmd(cmd, IptJumpNoAdd);
    snprintf(cmd, sizeof(cmd), "-D bw_OUTPUT -o %s --jump %s", ifn, costCString);
    res |= runIpxtablesCmd(cmd, IptJumpNoAdd);

    /* The "-N bw_costly_shared" is created upfront, no need to handle it here. */
    if (quotaType == QuotaUnique) {
        snprintf(cmd, sizeof(cmd), "-F %s", costCString);
        res |= runIpxtablesCmd(cmd, IptJumpNoAdd);
        snprintf(cmd, sizeof(cmd), "-X %s", costCString);
        res |= runIpxtablesCmd(cmd, IptJumpNoAdd);
    }
    return res;
}

int BandwidthController::setInterfaceSharedQuota(const char *iface, int64_t maxBytes) {
    char ifn[MAX_IFACENAME_LEN];
    int res = 0;
    std::string quotaCmd;
    std::string ifaceName;
    ;
    const char *costName = "shared";
    std::list<std::string>::iterator it;

    if (!maxBytes) {
        /* Don't talk about -1, deprecate it. */
        ALOGE("Invalid bytes value. 1..max_int64.");
        return -1;
    }
    if (!isIfaceName(iface))
        return -1;
    if (StrncpyAndCheck(ifn, iface, sizeof(ifn))) {
        ALOGE("Interface name longer than %d", MAX_IFACENAME_LEN);
        return -1;
    }
    ifaceName = ifn;

    if (maxBytes == -1) {
        return removeInterfaceSharedQuota(ifn);
    }

    /* Insert ingress quota. */
    for (it = sharedQuotaIfaces.begin(); it != sharedQuotaIfaces.end(); it++) {
        if (*it == ifaceName)
            break;
    }

    if (it == sharedQuotaIfaces.end()) {
        res |= prepCostlyIface(ifn, QuotaShared);
        if (sharedQuotaIfaces.empty()) {
            quotaCmd = makeIptablesQuotaCmd(IptOpInsert, costName, maxBytes);
            res |= runIpxtablesCmd(quotaCmd.c_str(), IptJumpReject);
            if (res) {
                ALOGE("Failed set quota rule");
                goto fail;
            }
            sharedQuotaBytes = maxBytes;
        }
        sharedQuotaIfaces.push_front(ifaceName);

    }

    if (maxBytes != sharedQuotaBytes) {
        res |= updateQuota(costName, maxBytes);
        if (res) {
            ALOGE("Failed update quota for %s", costName);
            goto fail;
        }
        sharedQuotaBytes = maxBytes;
    }
    return 0;

    fail:
    /*
     * TODO(jpa): once we get rid of iptables in favor of rtnetlink, reparse
     * rules in the kernel to see which ones need cleaning up.
     * For now callers needs to choose if they want to "ndc bandwidth enable"
     * which resets everything.
     */
    removeInterfaceSharedQuota(ifn);
    return -1;
}

/* It will also cleanup any shared alerts */
int BandwidthController::removeInterfaceSharedQuota(const char *iface) {
    char ifn[MAX_IFACENAME_LEN];
    int res = 0;
    std::string ifaceName;
    std::list<std::string>::iterator it;
    const char *costName = "shared";

    if (!isIfaceName(iface))
        return -1;
    if (StrncpyAndCheck(ifn, iface, sizeof(ifn))) {
        ALOGE("Interface name longer than %d", MAX_IFACENAME_LEN);
        return -1;
    }
    ifaceName = ifn;

    for (it = sharedQuotaIfaces.begin(); it != sharedQuotaIfaces.end(); it++) {
        if (*it == ifaceName)
            break;
    }
    if (it == sharedQuotaIfaces.end()) {
        ALOGE("No such iface %s to delete", ifn);
        return -1;
    }

    res |= cleanupCostlyIface(ifn, QuotaShared);
    sharedQuotaIfaces.erase(it);

    if (sharedQuotaIfaces.empty()) {
        std::string quotaCmd;
        quotaCmd = makeIptablesQuotaCmd(IptOpDelete, costName, sharedQuotaBytes);
        res |= runIpxtablesCmd(quotaCmd.c_str(), IptJumpReject);
        sharedQuotaBytes = 0;
        if (sharedAlertBytes) {
            removeSharedAlert();
            sharedAlertBytes = 0;
        }
    }
    return res;
}

int BandwidthController::setInterfaceQuota(const char *iface, int64_t maxBytes) {
    char ifn[MAX_IFACENAME_LEN];
    int res = 0;
    std::string ifaceName;
    const char *costName;
    std::list<QuotaInfo>::iterator it;
    std::string quotaCmd;

    if (!isIfaceName(iface))
        return -1;

    if (!maxBytes) {
        /* Don't talk about -1, deprecate it. */
        ALOGE("Invalid bytes value. 1..max_int64.");
        return -1;
    }
    if (maxBytes == -1) {
        return removeInterfaceQuota(iface);
    }

    if (StrncpyAndCheck(ifn, iface, sizeof(ifn))) {
        ALOGE("Interface name longer than %d", MAX_IFACENAME_LEN);
        return -1;
    }
    ifaceName = ifn;
    costName = iface;

    /* Insert ingress quota. */
    for (it = quotaIfaces.begin(); it != quotaIfaces.end(); it++) {
        if (it->ifaceName == ifaceName)
            break;
    }

    if (it == quotaIfaces.end()) {
        /* Preparing the iface adds a penalty/happy box check */
        res |= prepCostlyIface(ifn, QuotaUnique);
        /*
         * The rejecting quota limit should go after the penalty/happy box checks
         * or else a naughty app could just eat up the quota.
         * So we append here.
         */
        quotaCmd = makeIptablesQuotaCmd(IptOpAppend, costName, maxBytes);
        res |= runIpxtablesCmd(quotaCmd.c_str(), IptJumpReject);
        if (res) {
            ALOGE("Failed set quota rule");
            goto fail;
        }

        quotaIfaces.push_front(QuotaInfo(ifaceName, maxBytes, 0));

    } else {
        res |= updateQuota(costName, maxBytes);
        if (res) {
            ALOGE("Failed update quota for %s", iface);
            goto fail;
        }
        it->quota = maxBytes;
    }
    return 0;

    fail:
    /*
     * TODO(jpa): once we get rid of iptables in favor of rtnetlink, reparse
     * rules in the kernel to see which ones need cleaning up.
     * For now callers needs to choose if they want to "ndc bandwidth enable"
     * which resets everything.
     */
    removeInterfaceSharedQuota(ifn);
    return -1;
}

int BandwidthController::getInterfaceSharedQuota(int64_t *bytes) {
    return getInterfaceQuota("shared", bytes);
}

int BandwidthController::getInterfaceQuota(const char *costName, int64_t *bytes) {
    FILE *fp;
    char *fname;
    int scanRes;

    if (!isIfaceName(costName))
        return -1;

    asprintf(&fname, "/proc/net/xt_quota/%s", costName);
    fp = fopen(fname, "r");
    free(fname);
    if (!fp) {
        ALOGE("Reading quota %s failed (%s)", costName, strerror(errno));
        return -1;
    }
    scanRes = fscanf(fp, "%" SCNd64, bytes);
    ALOGV("Read quota res=%d bytes=%" PRId64, scanRes, *bytes);
    fclose(fp);
    return scanRes == 1 ? 0 : -1;
}

int BandwidthController::removeInterfaceQuota(const char *iface) {

    char ifn[MAX_IFACENAME_LEN];
    int res = 0;
    std::string ifaceName;
    const char *costName;
    std::list<QuotaInfo>::iterator it;

    if (!isIfaceName(iface))
        return -1;
    if (StrncpyAndCheck(ifn, iface, sizeof(ifn))) {
        ALOGE("Interface name longer than %d", MAX_IFACENAME_LEN);
        return -1;
    }
    ifaceName = ifn;
    costName = iface;

    for (it = quotaIfaces.begin(); it != quotaIfaces.end(); it++) {
        if (it->ifaceName == ifaceName)
            break;
    }

    if (it == quotaIfaces.end()) {
        ALOGE("No such iface %s to delete", ifn);
        return -1;
    }

    /* This also removes the quota command of CostlyIface chain. */
    res |= cleanupCostlyIface(ifn, QuotaUnique);

    quotaIfaces.erase(it);

    return res;
}

int BandwidthController::updateQuota(const char *quotaName, int64_t bytes) {
    FILE *fp;
    char *fname;

    if (!isIfaceName(quotaName)) {
        ALOGE("updateQuota: Invalid quotaName \"%s\"", quotaName);
        return -1;
    }

    asprintf(&fname, "/proc/net/xt_quota/%s", quotaName);
    fp = fopen(fname, "w");
    free(fname);
    if (!fp) {
        ALOGE("Updating quota %s failed (%s)", quotaName, strerror(errno));
        return -1;
    }
    fprintf(fp, "%" PRId64"\n", bytes);
    fclose(fp);
    return 0;
}

int BandwidthController::runIptablesAlertCmd(IptOp op, const char *alertName, int64_t bytes) {
    int res = 0;
    const char *opFlag;
    char *alertQuotaCmd;

    switch (op) {
    case IptOpInsert:
        opFlag = "-I";
        break;
    case IptOpAppend:
        opFlag = "-A";
        break;
    case IptOpReplace:
        opFlag = "-R";
        break;
    default:
    case IptOpDelete:
        opFlag = "-D";
        break;
    }

    asprintf(&alertQuotaCmd, ALERT_IPT_TEMPLATE, opFlag, "bw_INPUT",
        bytes, alertName);
    res |= runIpxtablesCmd(alertQuotaCmd, IptJumpNoAdd);
    free(alertQuotaCmd);
    asprintf(&alertQuotaCmd, ALERT_IPT_TEMPLATE, opFlag, "bw_OUTPUT",
        bytes, alertName);
    res |= runIpxtablesCmd(alertQuotaCmd, IptJumpNoAdd);
    free(alertQuotaCmd);
    return res;
}

int BandwidthController::runIptablesAlertFwdCmd(IptOp op, const char *alertName, int64_t bytes) {
    int res = 0;
    const char *opFlag;
    char *alertQuotaCmd;

    switch (op) {
    case IptOpInsert:
        opFlag = "-I";
        break;
    case IptOpAppend:
        opFlag = "-A";
        break;
    case IptOpReplace:
        opFlag = "-R";
        break;
    default:
    case IptOpDelete:
        opFlag = "-D";
        break;
    }

    asprintf(&alertQuotaCmd, ALERT_IPT_TEMPLATE, opFlag, "bw_FORWARD",
        bytes, alertName);
    res = runIpxtablesCmd(alertQuotaCmd, IptJumpNoAdd);
    free(alertQuotaCmd);
    return res;
}

int BandwidthController::setGlobalAlert(int64_t bytes) {
    const char *alertName = ALERT_GLOBAL_NAME;
    int res = 0;

    if (!bytes) {
        ALOGE("Invalid bytes value. 1..max_int64.");
        return -1;
    }
    if (globalAlertBytes) {
        res = updateQuota(alertName, bytes);
    } else {
        res = runIptablesAlertCmd(IptOpInsert, alertName, bytes);
        if (globalAlertTetherCount) {
            ALOGV("setGlobalAlert for %d tether", globalAlertTetherCount);
            res |= runIptablesAlertFwdCmd(IptOpInsert, alertName, bytes);
        }
    }
    globalAlertBytes = bytes;
    return res;
}

int BandwidthController::setGlobalAlertInForwardChain(void) {
    const char *alertName = ALERT_GLOBAL_NAME;
    int res = 0;

    globalAlertTetherCount++;
    ALOGV("setGlobalAlertInForwardChain(): %d tether", globalAlertTetherCount);

    /*
     * If there is no globalAlert active we are done.
     * If there is an active globalAlert but this is not the 1st
     * tether, we are also done.
     */
    if (!globalAlertBytes || globalAlertTetherCount != 1) {
        return 0;
    }

    /* We only add the rule if this was the 1st tether added. */
    res = runIptablesAlertFwdCmd(IptOpInsert, alertName, globalAlertBytes);
    return res;
}

int BandwidthController::removeGlobalAlert(void) {

    const char *alertName = ALERT_GLOBAL_NAME;
    int res = 0;

    if (!globalAlertBytes) {
        ALOGE("No prior alert set");
        return -1;
    }
    res = runIptablesAlertCmd(IptOpDelete, alertName, globalAlertBytes);
    if (globalAlertTetherCount) {
        res |= runIptablesAlertFwdCmd(IptOpDelete, alertName, globalAlertBytes);
    }
    globalAlertBytes = 0;
    return res;
}

int BandwidthController::removeGlobalAlertInForwardChain(void) {
    int res = 0;
    const char *alertName = ALERT_GLOBAL_NAME;

    if (!globalAlertTetherCount) {
        ALOGE("No prior alert set");
        return -1;
    }

    globalAlertTetherCount--;
    /*
     * If there is no globalAlert active we are done.
     * If there is an active globalAlert but there are more
     * tethers, we are also done.
     */
    if (!globalAlertBytes || globalAlertTetherCount >= 1) {
        return 0;
    }

    /* We only detete the rule if this was the last tether removed. */
    res = runIptablesAlertFwdCmd(IptOpDelete, alertName, globalAlertBytes);
    return res;
}

int BandwidthController::setSharedAlert(int64_t bytes) {
    if (!sharedQuotaBytes) {
        ALOGE("Need to have a prior shared quota set to set an alert");
        return -1;
    }
    if (!bytes) {
        ALOGE("Invalid bytes value. 1..max_int64.");
        return -1;
    }
    return setCostlyAlert("shared", bytes, &sharedAlertBytes);
}

int BandwidthController::removeSharedAlert(void) {
    return removeCostlyAlert("shared", &sharedAlertBytes);
}

int BandwidthController::setInterfaceAlert(const char *iface, int64_t bytes) {
    std::list<QuotaInfo>::iterator it;

    if (!isIfaceName(iface)) {
        ALOGE("setInterfaceAlert: Invalid iface \"%s\"", iface);
        return -1;
    }

    if (!bytes) {
        ALOGE("Invalid bytes value. 1..max_int64.");
        return -1;
    }
    for (it = quotaIfaces.begin(); it != quotaIfaces.end(); it++) {
        if (it->ifaceName == iface)
            break;
    }

    if (it == quotaIfaces.end()) {
        ALOGE("Need to have a prior interface quota set to set an alert");
        return -1;
    }

    return setCostlyAlert(iface, bytes, &it->alert);
}

int BandwidthController::removeInterfaceAlert(const char *iface) {
    std::list<QuotaInfo>::iterator it;

    if (!isIfaceName(iface)) {
        ALOGE("removeInterfaceAlert: Invalid iface \"%s\"", iface);
        return -1;
    }

    for (it = quotaIfaces.begin(); it != quotaIfaces.end(); it++) {
        if (it->ifaceName == iface)
            break;
    }

    if (it == quotaIfaces.end()) {
        ALOGE("No prior alert set for interface %s", iface);
        return -1;
    }

    return removeCostlyAlert(iface, &it->alert);
}

int BandwidthController::setCostlyAlert(const char *costName, int64_t bytes, int64_t *alertBytes) {
    char *alertQuotaCmd;
    char *chainName;
    int res = 0;
    char *alertName;

    if (!isIfaceName(costName)) {
        ALOGE("setCostlyAlert: Invalid costName \"%s\"", costName);
        return -1;
    }

    if (!bytes) {
        ALOGE("Invalid bytes value. 1..max_int64.");
        return -1;
    }
    asprintf(&alertName, "%sAlert", costName);
    if (*alertBytes) {
        res = updateQuota(alertName, *alertBytes);
    } else {
        asprintf(&chainName, "bw_costly_%s", costName);
        asprintf(&alertQuotaCmd, ALERT_IPT_TEMPLATE, "-A", chainName, bytes, alertName);
        res |= runIpxtablesCmd(alertQuotaCmd, IptJumpNoAdd);
        free(alertQuotaCmd);
        free(chainName);
    }
    *alertBytes = bytes;
    free(alertName);
    return res;
}

int BandwidthController::removeCostlyAlert(const char *costName, int64_t *alertBytes) {
    char *alertQuotaCmd;
    char *chainName;
    char *alertName;
    int res = 0;

    if (!isIfaceName(costName)) {
        ALOGE("removeCostlyAlert: Invalid costName \"%s\"", costName);
        return -1;
    }

    asprintf(&alertName, "%sAlert", costName);
    if (!*alertBytes) {
        ALOGE("No prior alert set for %s alert", costName);
        return -1;
    }

    asprintf(&chainName, "bw_costly_%s", costName);
    asprintf(&alertQuotaCmd, ALERT_IPT_TEMPLATE, "-D", chainName, *alertBytes, alertName);
    res |= runIpxtablesCmd(alertQuotaCmd, IptJumpNoAdd);
    free(alertQuotaCmd);
    free(chainName);

    *alertBytes = 0;
    free(alertName);
    return res;
}

/*
 * Parse the ptks and bytes out of:
 *   Chain natctrl_tether_counters (4 references)
 *       pkts      bytes target     prot opt in     out     source               destination
 *         26     2373 RETURN     all  --  wlan0  rmnet0  0.0.0.0/0            0.0.0.0/0
 *         27     2002 RETURN     all  --  rmnet0 wlan0   0.0.0.0/0            0.0.0.0/0
 *       1040   107471 RETURN     all  --  bt-pan rmnet0  0.0.0.0/0            0.0.0.0/0
 *       1450  1708806 RETURN     all  --  rmnet0 bt-pan  0.0.0.0/0            0.0.0.0/0
 * It results in an error if invoked and no tethering counter rules exist. The constraint
 * helps detect complete parsing failure.
 */
int BandwidthController::parseForwardChainStats(SocketClient *cli, const TetherStats filter,
                                                FILE *fp, std::string &extraProcessingInfo) {
    int res;
    char lineBuffer[MAX_IPT_OUTPUT_LINE_LEN];
    char iface0[MAX_IPT_OUTPUT_LINE_LEN];
    char iface1[MAX_IPT_OUTPUT_LINE_LEN];
    char rest[MAX_IPT_OUTPUT_LINE_LEN];

    TetherStats stats;
    char *buffPtr;
    int64_t packets, bytes;
    int statsFound = 0;

    bool filterPair = filter.intIface[0] && filter.extIface[0];

    char *filterMsg = filter.getStatsLine();
    ALOGV("filter: %s",  filterMsg);
    free(filterMsg);

    stats = filter;

    while (NULL != (buffPtr = fgets(lineBuffer, MAX_IPT_OUTPUT_LINE_LEN, fp))) {
        /* Clean up, so a failed parse can still print info */
        iface0[0] = iface1[0] = rest[0] = packets = bytes = 0;
        res = sscanf(buffPtr, "%" SCNd64" %" SCNd64" RETURN all -- %s %s 0.%s",
                &packets, &bytes, iface0, iface1, rest);
        ALOGV("parse res=%d iface0=<%s> iface1=<%s> pkts=%" PRId64" bytes=%" PRId64" rest=<%s> orig line=<%s>", res,
             iface0, iface1, packets, bytes, rest, buffPtr);
        extraProcessingInfo += buffPtr;

        if (res != 5) {
            continue;
        }
        /*
         * The following assumes that the 1st rule has in:extIface out:intIface,
         * which is what NatController sets up.
         * If not filtering, the 1st match rx, and sets up the pair for the tx side.
         */
        if (filter.intIface[0] && filter.extIface[0]) {
            if (filter.intIface == iface0 && filter.extIface == iface1) {
                ALOGV("2Filter RX iface_in=%s iface_out=%s rx_bytes=%" PRId64" rx_packets=%" PRId64" ", iface0, iface1, bytes, packets);
                stats.rxPackets = packets;
                stats.rxBytes = bytes;
            } else if (filter.intIface == iface1 && filter.extIface == iface0) {
                ALOGV("2Filter TX iface_in=%s iface_out=%s rx_bytes=%" PRId64" rx_packets=%" PRId64" ", iface0, iface1, bytes, packets);
                stats.txPackets = packets;
                stats.txBytes = bytes;
            }
        } else if (filter.intIface[0] || filter.extIface[0]) {
            if (filter.intIface == iface0 || filter.extIface == iface1) {
                ALOGV("1Filter RX iface_in=%s iface_out=%s rx_bytes=%" PRId64" rx_packets=%" PRId64" ", iface0, iface1, bytes, packets);
                stats.intIface = iface0;
                stats.extIface = iface1;
                stats.rxPackets = packets;
                stats.rxBytes = bytes;
            } else if (filter.intIface == iface1 || filter.extIface == iface0) {
                ALOGV("1Filter TX iface_in=%s iface_out=%s rx_bytes=%" PRId64" rx_packets=%" PRId64" ", iface0, iface1, bytes, packets);
                stats.intIface = iface1;
                stats.extIface = iface0;
                stats.txPackets = packets;
                stats.txBytes = bytes;
            }
        } else /* if (!filter.intFace[0] && !filter.extIface[0]) */ {
            if (!stats.intIface[0]) {
                ALOGV("0Filter RX iface_in=%s iface_out=%s rx_bytes=%" PRId64" rx_packets=%" PRId64" ", iface0, iface1, bytes, packets);
                stats.intIface = iface0;
                stats.extIface = iface1;
                stats.rxPackets = packets;
                stats.rxBytes = bytes;
            } else if (stats.intIface == iface1 && stats.extIface == iface0) {
                ALOGV("0Filter TX iface_in=%s iface_out=%s rx_bytes=%" PRId64" rx_packets=%" PRId64" ", iface0, iface1, bytes, packets);
                stats.txPackets = packets;
                stats.txBytes = bytes;
            }
        }
        if (stats.rxBytes != -1 && stats.txBytes != -1) {
            ALOGV("rx_bytes=%" PRId64" tx_bytes=%" PRId64" filterPair=%d", stats.rxBytes, stats.txBytes, filterPair);
            /* Send out stats, and prep for the next if needed. */
            char *msg = stats.getStatsLine();
            if (filterPair) {
                cli->sendMsg(ResponseCode::TetheringStatsResult, msg, false);
                return 0;
            } else {
                cli->sendMsg(ResponseCode::TetheringStatsListResult, msg, false);
                stats = filter;
            }
            free(msg);
            statsFound++;
        }
    }

    /* It is always an error to find only one side of the stats. */
    /* It is an error to find nothing when not filtering. */
    if (((stats.rxBytes == -1) != (stats.txBytes == -1)) ||
        (!statsFound && !filterPair)) {
        return -1;
    }
    cli->sendMsg(ResponseCode::CommandOkay, "Tethering stats list completed", false);
    return 0;
}

char *BandwidthController::TetherStats::getStatsLine(void) const {
    char *msg;
    asprintf(&msg, "%s %s %" PRId64" %" PRId64" %" PRId64" %" PRId64, intIface.c_str(), extIface.c_str(),
            rxBytes, rxPackets, txBytes, txPackets);
    return msg;
}

int BandwidthController::getTetherStats(SocketClient *cli, TetherStats &stats, std::string &extraProcessingInfo) {
    int res;
    std::string fullCmd;
    FILE *iptOutput;

    /*
     * Why not use some kind of lib to talk to iptables?
     * Because the only libs are libiptc and libip6tc in iptables, and they are
     * not easy to use. They require the known iptables match modules to be
     * preloaded/linked, and require apparently a lot of wrapper code to get
     * the wanted info.
     */
    fullCmd = IPTABLES_PATH;
    fullCmd += " -nvx -L ";
    fullCmd += NatController::LOCAL_TETHER_COUNTERS_CHAIN;
    iptOutput = popen(fullCmd.c_str(), "r");
    if (!iptOutput) {
            ALOGE("Failed to run %s err=%s", fullCmd.c_str(), strerror(errno));
            extraProcessingInfo += "Failed to run iptables.";
        return -1;
    }
    res = parseForwardChainStats(cli, stats, iptOutput, extraProcessingInfo);
    pclose(iptOutput);

    /* Currently NatController doesn't do ipv6 tethering, so we are done. */
    return res;
}

void BandwidthController::flushExistingCostlyTables(bool doClean) {
    std::string fullCmd;
    FILE *iptOutput;

    /* Only lookup ip4 table names as ip6 will have the same tables ... */
    fullCmd = IPTABLES_PATH;
    fullCmd += " -S";
    iptOutput = popen(fullCmd.c_str(), "r");
    if (!iptOutput) {
            ALOGE("Failed to run %s err=%s", fullCmd.c_str(), strerror(errno));
        return;
    }
    /* ... then flush/clean both ip4 and ip6 iptables. */
    parseAndFlushCostlyTables(iptOutput, doClean);
    pclose(iptOutput);
}

void BandwidthController::parseAndFlushCostlyTables(FILE *fp, bool doRemove) {
    int res;
    char lineBuffer[MAX_IPT_OUTPUT_LINE_LEN];
    char costlyIfaceName[MAX_IPT_OUTPUT_LINE_LEN];
    char cmd[MAX_CMD_LEN];
    char *buffPtr;

    while (NULL != (buffPtr = fgets(lineBuffer, MAX_IPT_OUTPUT_LINE_LEN, fp))) {
        costlyIfaceName[0] = '\0';   /* So that debugging output always works */
        res = sscanf(buffPtr, "-N bw_costly_%s", costlyIfaceName);
        ALOGV("parse res=%d costly=<%s> orig line=<%s>", res,
            costlyIfaceName, buffPtr);
        if (res != 1) {
            continue;
        }
        /* Exclusions: "shared" is not an ifacename */
        if (!strcmp(costlyIfaceName, "shared")) {
            continue;
        }

        snprintf(cmd, sizeof(cmd), "-F bw_costly_%s", costlyIfaceName);
        runIpxtablesCmd(cmd, IptJumpNoAdd, IptFailHide);
        if (doRemove) {
            snprintf(cmd, sizeof(cmd), "-X bw_costly_%s", costlyIfaceName);
            runIpxtablesCmd(cmd, IptJumpNoAdd, IptFailHide);
        }
    }
}
