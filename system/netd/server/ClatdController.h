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

#ifndef _CLATD_CONTROLLER_H
#define _CLATD_CONTROLLER_H

#include <map>

class NetworkController;

class ClatdController {
public:
    explicit ClatdController(NetworkController* controller);
    virtual ~ClatdController();

    int startClatd(char *interface);
    int stopClatd(char* interface);
    bool isClatdStarted(char* interface);

private:
    NetworkController* const mNetCtrl;
    std::map<std::string, pid_t> mClatdPids;
    pid_t getClatdPid(char* interface);
};

#endif
