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

#ifndef _OEM_IPTABLES_HOOK_H
#define _OEM_IPTABLES_HOOK_H

#define OEM_IPTABLES_FILTER_OUTPUT "oem_out"
#define OEM_IPTABLES_FILTER_FORWARD "oem_fwd"
#define OEM_IPTABLES_NAT_PREROUTING "oem_nat_pre"

void setupOemIptablesHook();

#endif
