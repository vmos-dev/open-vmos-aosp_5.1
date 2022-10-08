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

#ifndef _NAT_CONTROLLER_H
#define _NAT_CONTROLLER_H

#include <linux/in.h>
#include <list>
#include <string>

class NatController {
public:
    NatController();
    virtual ~NatController();

    int enableNat(const char* intIface, const char* extIface);
    int disableNat(const char* intIface, const char* extIface);
    int setupIptablesHooks();

    static const char* LOCAL_FORWARD;
    static const char* LOCAL_MANGLE_FORWARD;
    static const char* LOCAL_NAT_POSTROUTING;
    static const char* LOCAL_TETHER_COUNTERS_CHAIN;

    // List of strings of interface pairs.
    std::list<std::string> ifacePairList;

private:
    int natCount;

    bool checkTetherCountingRuleExist(const char *pair_name);

    int setDefaults();
    int runCmd(int argc, const char **argv);
    int setForwardRules(bool set, const char *intIface, const char *extIface);
    int setTetherCountingRules(bool add, const char *intIface, const char *extIface);
};

#endif
