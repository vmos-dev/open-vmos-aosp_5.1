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

/*
 * MODUS OPERANDI
 * --------------
 *
 * IPTABLES command sequence:
 *
 * iptables -F
 *
 * iptables -t raw -F idletimer_PREROUTING
 * iptables -t mangle -F idletimer_POSTROUTING
 *
 *
 * iptables -t raw -N idletimer_PREROUTING
 * iptables -t mangle -N idletimer_POSTROUTING
 *
 * iptables -t raw -D PREROUTING -j idletimer_PREROUTING
 * iptables -t mangle -D POSTROUTING -j idletimer_POSTROUTING
 *
 *
 * iptables -t raw -I PREROUTING -j idletimer_PREROUTING
 * iptables -t mangle -I POSTROUTING -j idletimer_POSTROUTING
 *
 * # For notifications to work the lable name must match the name of a valid interface.
 * # If the label name does match an interface, the rules will be a no-op.
 *
 * iptables -t raw -A idletimer_PREROUTING -i rmnet0 -j IDLETIMER  --timeout 5 --label test-chain --send_nl_msg 1
 * iptables -t mangle -A idletimer_POSTROUTING -o rmnet0 -j IDLETIMER  --timeout 5 --label test-chain --send_nl_msg 1
 *
 * iptables -nxvL -t raw
 * iptables -nxvL -t mangle
 *
 * =================
 *
 * ndc command sequence
 * ------------------
 * ndc idletimer enable
 * ndc idletimer add <iface> <timeout> <class label>
 * ndc idletimer remove <iface> <timeout> <class label>
 *
 * Monitor effect on the iptables chains after each step using:
 *     iptables -nxvL -t raw
 *     iptables -nxvL -t mangle
 *
 * Remember that the timeout value has to be same at the time of the
 * removal.
 *
 * =================
 *
 * Verifying the iptables rule
 * ---------------------------
 * We want to make sure the iptable rules capture every packet. It can be
 * verified with tcpdump. First take a note of the pkts count for the two rules:
 *
 * adb shell iptables -t mangle -L idletimer_mangle_POSTROUTING -v && adb shell iptables -t raw -L idletimer_raw_PREROUTING -v
 *
 * And then, before any network traffics happen on the device, run tcpdump:
 *
 * adb shell tcpdump | tee tcpdump.log
 *
 * After a while run iptables commands again, you could then count the number
 * of incoming and outgoing packets captured by tcpdump, and compare that with
 * the numbers reported by iptables command. There shouldn't be too much
 * difference on these numbers, i.e., with 2000 packets captured it should
 * differ by less than 5.
 *
 * =================
 *
 * Note that currently if the name of the iface is incorrect, iptables
 * will setup rules without checking if it is the name of a valid
 * interface (although no notifications will ever be received).  It is
 * the responsibility of code in Java land to ensure that the interface name
 * is correct. The benefit of this, is that idletimers can be setup on
 * interfaces than come and go.
 *
 * A remove should be called for each add command issued during cleanup, as duplicate
 * entries of the rule may exist and will all have to removed.
 *
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

#define LOG_TAG "IdletimerController"
#include <cutils/log.h>
#include <logwrap/logwrap.h>

#include "IdletimerController.h"
#include "NetdConstants.h"

const char* IdletimerController::LOCAL_RAW_PREROUTING = "idletimer_raw_PREROUTING";
const char* IdletimerController::LOCAL_MANGLE_POSTROUTING = "idletimer_mangle_POSTROUTING";

IdletimerController::IdletimerController() {
}

IdletimerController::~IdletimerController() {
}
/* return 0 or non-zero */
int IdletimerController::runIpxtablesCmd(int argc, const char **argv) {
  return 0;
    int resIpv4, resIpv6;

    // Running for IPv4
    argv[0] = IPTABLES_PATH;
    resIpv4 = android_fork_execvp(argc, (char **)argv, NULL, false, false);

    // Running for IPv6
    argv[0] = IP6TABLES_PATH;
    resIpv6 = android_fork_execvp(argc, (char **)argv, NULL, false, false);

#if !LOG_NDEBUG
    std::string full_cmd = argv[0];
    argc--; argv++;
    for (; argc; argc--, argv++) {
        full_cmd += " ";
        full_cmd += argv[0];
    }
    ALOGV("runCmd(%s) res_ipv4=%d, res_ipv6=%d", full_cmd.c_str(), resIpv4, resIpv6);
#endif

    return (resIpv4 == 0 && resIpv6 == 0) ? 0 : -1;
}

bool IdletimerController::setupIptablesHooks() {
    return true;
}

int IdletimerController::setDefaults() {
  int res;
  const char *cmd1[] = {
      NULL, // To be filled inside runIpxtablesCmd
      "-t",
      "raw",
      "-F",
      LOCAL_RAW_PREROUTING
  };
  res = runIpxtablesCmd(ARRAY_SIZE(cmd1), cmd1);

  if (res)
    return res;

  const char *cmd2[] = {
      NULL, // To be filled inside runIpxtablesCmd
      "-t",
      "mangle",
      "-F",
      LOCAL_MANGLE_POSTROUTING
  };
  res = runIpxtablesCmd(ARRAY_SIZE(cmd2), cmd2);

  return res;
}

int IdletimerController::enableIdletimerControl() {
    int res = setDefaults();
    return res;
}

int IdletimerController::disableIdletimerControl() {
    int res = setDefaults();
    return res;
}

int IdletimerController::modifyInterfaceIdletimer(IptOp op, const char *iface,
                                                  uint32_t timeout,
                                                  const char *classLabel) {
  int res;
  char timeout_str[11]; //enough to store any 32-bit unsigned decimal

  if (!isIfaceName(iface)) {
    errno = ENOENT;
    return -1;
  }

  snprintf(timeout_str, sizeof(timeout_str), "%u", timeout);

  const char *cmd1[] = {
      NULL, // To be filled inside runIpxtablesCmd
      "-t",
      "raw",
      (op == IptOpAdd) ? "-A" : "-D",
      LOCAL_RAW_PREROUTING,
      "-i",
      iface,
      "-j",
      "IDLETIMER",
      "--timeout",
      timeout_str,
      "--label",
      classLabel,
      "--send_nl_msg",
      "1"
  };
  res = runIpxtablesCmd(ARRAY_SIZE(cmd1), cmd1);

  if (res)
    return res;

  const char *cmd2[] = {
      NULL, // To be filled inside runIpxtablesCmd
      "-t",
      "mangle",
      (op == IptOpAdd) ? "-A" : "-D",
      LOCAL_MANGLE_POSTROUTING,
      "-o",
      iface,
      "-j",
      "IDLETIMER",
      "--timeout",
      timeout_str,
      "--label",
      classLabel,
      "--send_nl_msg",
      "1"
  };
  res = runIpxtablesCmd(ARRAY_SIZE(cmd2), cmd2);

  return res;
}

int IdletimerController::addInterfaceIdletimer(const char *iface,
                                               uint32_t timeout,
                                               const char *classLabel) {
  return modifyInterfaceIdletimer(IptOpAdd, iface, timeout, classLabel);
}

int IdletimerController::removeInterfaceIdletimer(const char *iface,
                                                  uint32_t timeout,
                                                  const char *classLabel) {
  return modifyInterfaceIdletimer(IptOpDelete, iface, timeout, classLabel);
}
