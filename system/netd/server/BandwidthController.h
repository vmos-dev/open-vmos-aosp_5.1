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
#ifndef _BANDWIDTH_CONTROLLER_H
#define _BANDWIDTH_CONTROLLER_H

#include <list>
#include <string>
#include <utility>  // for pair

#include <sysutils/SocketClient.h>

class BandwidthController {
public:
    class TetherStats {
    public:
        TetherStats(void)
                : rxBytes(-1), rxPackets(-1),
                    txBytes(-1), txPackets(-1) {};
        TetherStats(std::string intIfn, std::string extIfn,
                int64_t rxB, int64_t rxP,
                int64_t txB, int64_t txP)
                        : intIface(intIfn), extIface(extIfn),
                            rxBytes(rxB), rxPackets(rxP),
                            txBytes(txB), txPackets(txP) {};
        /* Internal interface. Same as NatController's notion. */
        std::string intIface;
        /* External interface. Same as NatController's notion. */
        std::string extIface;
        int64_t rxBytes, rxPackets;
        int64_t txBytes, txPackets;
        /*
         * Allocates a new string representing this:
         * intIface extIface rx_bytes rx_packets tx_bytes tx_packets
         * The caller is responsible for free()'ing the returned ptr.
         */
        char *getStatsLine(void) const;
    };

    BandwidthController();

    int setupIptablesHooks(void);

    int enableBandwidthControl(bool force);
    int disableBandwidthControl(void);

    int setInterfaceSharedQuota(const char *iface, int64_t bytes);
    int getInterfaceSharedQuota(int64_t *bytes);
    int removeInterfaceSharedQuota(const char *iface);

    int setInterfaceQuota(const char *iface, int64_t bytes);
    int getInterfaceQuota(const char *iface, int64_t *bytes);
    int removeInterfaceQuota(const char *iface);

    int enableHappyBox(void);
    int disableHappyBox(void);
    int addNaughtyApps(int numUids, char *appUids[]);
    int removeNaughtyApps(int numUids, char *appUids[]);
    int addNiceApps(int numUids, char *appUids[]);
    int removeNiceApps(int numUids, char *appUids[]);

    int setGlobalAlert(int64_t bytes);
    int removeGlobalAlert(void);
    int setGlobalAlertInForwardChain(void);
    int removeGlobalAlertInForwardChain(void);

    int setSharedAlert(int64_t bytes);
    int removeSharedAlert(void);

    int setInterfaceAlert(const char *iface, int64_t bytes);
    int removeInterfaceAlert(const char *iface);

    /*
     * For single pair of ifaces, stats should have ifaceIn and ifaceOut initialized.
     * For all pairs, stats should have ifaceIn=ifaceOut="".
     * Sends out to the cli the single stat (TetheringStatsReluts) or a list of stats
     * (TetheringStatsListResult+CommandOkay).
     * Error is to be handled on the outside.
     * It results in an error if invoked and no tethering counter rules exist.
     */
    int getTetherStats(SocketClient *cli, TetherStats &stats, std::string &extraProcessingInfo);

    static const char* LOCAL_INPUT;
    static const char* LOCAL_FORWARD;
    static const char* LOCAL_OUTPUT;
    static const char* LOCAL_RAW_PREROUTING;
    static const char* LOCAL_MANGLE_POSTROUTING;

protected:
    class QuotaInfo {
    public:
      QuotaInfo(std::string ifn, int64_t q, int64_t a)
              : ifaceName(ifn), quota(q), alert(a) {};
        std::string ifaceName;
        int64_t quota;
        int64_t alert;
    };

    enum IptIpVer { IptIpV4, IptIpV6 };
    enum IptOp { IptOpInsert, IptOpReplace, IptOpDelete, IptOpAppend };
    enum IptJumpOp { IptJumpReject, IptJumpReturn, IptJumpNoAdd };
    enum SpecialAppOp { SpecialAppOpAdd, SpecialAppOpRemove };
    enum QuotaType { QuotaUnique, QuotaShared };
    enum RunCmdErrHandling { RunCmdFailureBad, RunCmdFailureOk };
#if LOG_NDEBUG
    enum IptFailureLog { IptFailShow, IptFailHide };
#else
    enum IptFailureLog { IptFailShow, IptFailHide = IptFailShow };
#endif

    int manipulateSpecialApps(int numUids, char *appStrUids[],
                               const char *chain,
                               std::list<int /*appUid*/> &specialAppUids,
                               IptJumpOp jumpHandling, SpecialAppOp appOp);
    int manipulateNaughtyApps(int numUids, char *appStrUids[], SpecialAppOp appOp);
    int manipulateNiceApps(int numUids, char *appStrUids[], SpecialAppOp appOp);

    int prepCostlyIface(const char *ifn, QuotaType quotaType);
    int cleanupCostlyIface(const char *ifn, QuotaType quotaType);

    std::string makeIptablesSpecialAppCmd(IptOp op, int uid, const char *chain);
    std::string makeIptablesQuotaCmd(IptOp op, const char *costName, int64_t quota);

    int runIptablesAlertCmd(IptOp op, const char *alertName, int64_t bytes);
    int runIptablesAlertFwdCmd(IptOp op, const char *alertName, int64_t bytes);

    /* Runs for both ipv4 and ipv6 iptables */
    int runCommands(int numCommands, const char *commands[], RunCmdErrHandling cmdErrHandling);
    /* Runs for both ipv4 and ipv6 iptables, appends -j REJECT --reject-with ...  */
    static int runIpxtablesCmd(const char *cmd, IptJumpOp jumpHandling,
                               IptFailureLog failureHandling = IptFailShow);
    static int runIptablesCmd(const char *cmd, IptJumpOp jumpHandling, IptIpVer iptIpVer,
                              IptFailureLog failureHandling = IptFailShow);


    // Provides strncpy() + check overflow.
    static int StrncpyAndCheck(char *buffer, const char *src, size_t buffSize);

    int updateQuota(const char *alertName, int64_t bytes);

    int setCostlyAlert(const char *costName, int64_t bytes, int64_t *alertBytes);
    int removeCostlyAlert(const char *costName, int64_t *alertBytes);

    /*
     * stats should never have only intIface initialized. Other 3 combos are ok.
     * fp should be a file to the apropriate FORWARD chain of iptables rules.
     * extraProcessingInfo: contains raw parsed data, and error info.
     * This strongly requires that setup of the rules is in a specific order:
     *  in:intIface out:extIface
     *  in:extIface out:intIface
     * and the rules are grouped in pairs when more that one tethering was setup.
     */
    static int parseForwardChainStats(SocketClient *cli, const TetherStats filter, FILE *fp,
                                      std::string &extraProcessingInfo);

    /*
     * Attempt to find the bw_costly_* tables that need flushing,
     * and flush them.
     * If doClean then remove the tables also.
     * Deals with both ip4 and ip6 tables.
     */
    void flushExistingCostlyTables(bool doClean);
    static void parseAndFlushCostlyTables(FILE *fp, bool doRemove);

    /*
     * Attempt to flush our tables.
     * If doClean then remove them also.
     * Deals with both ip4 and ip6 tables.
     */
    void flushCleanTables(bool doClean);

    /*------------------*/

    std::list<std::string> sharedQuotaIfaces;
    int64_t sharedQuotaBytes;
    int64_t sharedAlertBytes;
    int64_t globalAlertBytes;
    /*
     * This tracks the number of tethers setup.
     * The FORWARD chain is updated in the following cases:
     *  - The 1st time a globalAlert is setup and there are tethers setup.
     *  - Anytime a globalAlert is removed and there are tethers setup.
     *  - The 1st tether is setup and there is a globalAlert active.
     *  - The last tether is removed and there is a globalAlert active.
     */
    int globalAlertTetherCount;

    std::list<QuotaInfo> quotaIfaces;
    std::list<int /*appUid*/> naughtyAppUids;
    std::list<int /*appUid*/> niceAppUids;

private:
    static const char *IPT_FLUSH_COMMANDS[];
    static const char *IPT_CLEANUP_COMMANDS[];
    static const char *IPT_SETUP_COMMANDS[];
    static const char *IPT_BASIC_ACCOUNTING_COMMANDS[];

    /* Alphabetical */
    static const char ALERT_GLOBAL_NAME[];
    static const int  MAX_CMD_ARGS;
    static const int  MAX_CMD_LEN;
    static const int  MAX_IFACENAME_LEN;
    static const int  MAX_IPT_OUTPUT_LINE_LEN;
};

#endif
