/*
 * Copyright (C) 2014 The Android Open Source Project
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

#ifndef NETD_INCLUDE_FWMARK_H
#define NETD_INCLUDE_FWMARK_H

#include "Permission.h"

#include <stdint.h>

union Fwmark {
    uint32_t intValue;
    struct {
        unsigned netId          : 16;
        bool explicitlySelected :  1;
        bool protectedFromVpn   :  1;
        Permission permission   :  2;
    };
    Fwmark() : intValue(0) {}
};

static const unsigned FWMARK_NET_ID_MASK = 0xffff;

static_assert(sizeof(Fwmark) == sizeof(uint32_t), "The entire fwmark must fit into 32 bits");

#endif  // NETD_INCLUDE_FWMARK_H
