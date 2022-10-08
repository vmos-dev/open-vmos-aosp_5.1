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

#ifndef _RESOLVER_CONTROLLER_H_
#define _RESOLVER_CONTROLLER_H_

#include <netinet/in.h>
#include <linux/in.h>

class ResolverController {
public:
    ResolverController() {};
    virtual ~ResolverController() {};

    int setDnsServers(unsigned netid, const char * domains, const char** servers,
            int numservers);
    int clearDnsServers(unsigned netid);
    int flushDnsCache(unsigned netid);
    // TODO: Add deleteDnsCache(unsigned netId)
};

#endif /* _RESOLVER_CONTROLLER_H_ */
