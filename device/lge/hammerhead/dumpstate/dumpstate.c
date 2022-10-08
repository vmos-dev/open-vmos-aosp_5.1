/*
 * Copyright 2013 The Android Open Source Project
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

void dumpstate_board()
{
    dump_file("INTERRUPTS", "/proc/interrupts");
    dump_file("Power Management Stats", "/proc/msm_pm_stats");
    run_command("SUBSYSTEM TOMBSTONES", 5, SU_PATH, "root", "ls", "-l", "/data/tombstones/ramdump", NULL);
    dump_file("BAM DMUX Log", "/d/ipc_logging/bam_dmux/log");
    dump_file("SMD Log", "/d/ipc_logging/smd/log");
    dump_file("SMD PKT Log", "/d/ipc_logging/smd_pkt/log");
    dump_file("IPC Router Log", "/d/ipc_logging/ipc_router/log");
    run_command("ION HEAPS", 5, SU_PATH, "root", "/system/bin/sh", "-c", "for f in $(ls /d/ion/*); do echo $f; cat $f; done", NULL);
};
