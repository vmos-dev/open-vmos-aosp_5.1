/*
 * Copyright 2014 The Android Open Source Project
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
    dump_file("TZ ramoops annotation", "/sys/fs/pstore/annotate-ramoops");
    dump_file("Recent panic log", "/sys/fs/pstore/dmesg-ramoops-0");
    dump_file("cpuinfo", "/proc/cpuinfo");
    dump_file("Interrupts", "/proc/interrupts");
    dump_file("Power Management Stats", "/proc/msm_pm_stats");
    dump_file("RPM Stats", "/d/rpm_stats");
    dump_file("SMB135x Config Regs", "/d/smb135x/config_registers");
    dump_file("SMB135x IRQ Count", "/d/smb135x/irq_count");
    dump_file("SMB135x Status Regs", "/d/smb135x/status_registers");
    dump_file("wlan", "/sys/module/bcmdhd/parameters/info_string");
    dump_file("Battery Statistics", "/sys/class/power_supply/battery/uevent");
    dump_file("PCIe IPC Logging", "/d/ipc_logging/pcie0/log");
    dump_file("HSIC IPC Control Logging", "/d/xhci_msm_hsic_dbg/show_ctrl_events");
    dump_file("HSIC IPC Data Logging", "/d/xhci_msm_hsic_dbg/show_data_events");
    dump_file("ION kmalloc heap", "/d/ion/heaps/kmalloc");
    dump_file("ION multimedia heap", "/d/ion/heaps/mm");
    dump_file("ION peripheral-image-loader heap", "/d/ion/heaps/pil_1");
    dump_file("ION secure-comm heap", "/d/ion/heaps/qsecom");
    dump_file("ION system heap", "/d/ion/heaps/system");
    dump_file("HSIC control events", "/d/xhci_msm_hsic_dbg/show_ctrl_events");
    dump_file("HSIC data events", "/d/xhci_msm_hsic_dbg/show_data_events");
    dump_file("USB PM events", "/d/usb_pm_hsic_dbg/show_usb_pm_events");
    run_command("Subsystem Tombstone list", 5, SU_PATH, "root", "ls", "-l", "/data/tombstones/ramdump", NULL);
    run_command("ION CLIENTS", 5, SU_PATH, "root", "/system/bin/sh", "-c", "for f in $(ls /d/ion/clients/*); do echo $f; cat $f; done", NULL);
    run_command("ION HEAPS",   5, SU_PATH, "root", "/system/bin/sh", "-c", "for f in $(ls /d/ion/heaps/*);   do echo $f; cat $f; done", NULL);
};
