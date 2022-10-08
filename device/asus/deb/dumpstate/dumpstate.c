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

#include <dumpstate.h>

void dump_ks_bridges()
{
    int i;
    for (i = 1; i <= 4; ++i) {
        char path[64], title[32];
        sprintf(path, "/sys/kernel/debug/ks_bridge/ks_bridge:%d", i);
        sprintf(title, "KS BRIDGE LOG#%d", i);
        dump_file(title, path);
    }
}

void dumpstate_board()
{
    dump_file("INTERRUPTS", "/proc/interrupts");
    run_command("MODEM TOMBSTONES", 5, SU_PATH, "root", "ls", "-l", "/data/tombstones/mdm", NULL);
    dump_ks_bridges();
    dump_file("eMMC manfid",
        "/sys/devices/platform/msm_sdcc.1/mmc_host/mmc0/mmc0:0001/manfid");
    dump_file("eMMC capacity",
        "/sys/devices/platform/msm_sdcc.1/mmc_host/mmc0/mmc0:0001/sec_count");
};
