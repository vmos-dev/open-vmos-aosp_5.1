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

#ifndef _FIREWALL_CONTROLLER_H
#define _FIREWALL_CONTROLLER_H

#include <string>

enum FirewallRule { ALLOW, DENY };

#define PROTOCOL_TCP 6
#define PROTOCOL_UDP 17

/*
 * Simple firewall that drops all packets except those matching explicitly
 * defined ALLOW rules.
 */
class FirewallController {
public:
    FirewallController();

    int setupIptablesHooks(void);

    int enableFirewall(void);
    int disableFirewall(void);
    int isFirewallEnabled(void);

    /* Match traffic going in/out over the given iface. */
    int setInterfaceRule(const char*, FirewallRule);
    /* Match traffic coming-in-to or going-out-from given address. */
    int setEgressSourceRule(const char*, FirewallRule);
    /* Match traffic coming-in-from or going-out-to given address, port, and protocol. */
    int setEgressDestRule(const char*, int, int, FirewallRule);
    /* Match traffic owned by given UID. */
    int setUidRule(int, FirewallRule);

    static const char* LOCAL_INPUT;
    static const char* LOCAL_OUTPUT;
    static const char* LOCAL_FORWARD;

};

#endif
