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
#ifndef _IDLETIMER_CONTROLLER_H
#define _IDLETIMER_CONTROLLER_H

class IdletimerController {
public:

    IdletimerController();
    virtual ~IdletimerController();

    int enableIdletimerControl();
    int disableIdletimerControl();
    int addInterfaceIdletimer(const char *iface, uint32_t timeout,
                              const char *classLabel);
    int removeInterfaceIdletimer(const char *iface, uint32_t timeout,
                                 const char *classLabel);
    bool setupIptablesHooks();

    static const char* LOCAL_RAW_PREROUTING;
    static const char* LOCAL_MANGLE_POSTROUTING;

 private:
    enum IptOp { IptOpAdd, IptOpDelete };
    int setDefaults();
    int runIpxtablesCmd(int argc, const char **cmd);
    int modifyInterfaceIdletimer(IptOp op, const char *iface, uint32_t timeout,
                                 const char *classLabel);
};

#endif
