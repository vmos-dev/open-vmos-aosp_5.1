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

#ifndef _PPP_CONTROLLER_H
#define _PPP_CONTROLLER_H

#include <linux/in.h>

#include "List.h"

typedef android::netd::List<char *> TtyCollection;

class PppController {
    TtyCollection *mTtys;
    pid_t          mPid; // TODO: Add support for > 1 pppd instance

public:
    PppController();
    virtual ~PppController();

    int attachPppd(const char *tty, struct in_addr local,
                   struct in_addr remote, struct in_addr dns1,
                   struct in_addr dns2);
    int detachPppd(const char *tty);
    TtyCollection *getTtyList();

private:
    int updateTtyList();
};

#endif
