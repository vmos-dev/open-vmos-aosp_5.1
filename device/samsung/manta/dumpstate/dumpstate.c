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

void dumpstate_board()
{
    dump_file("board revision", "/sys/devices/soc0/board_rev");
    dump_file("soc family", "/sys/devices/soc0/family");
    dump_file("soc revision", "/sys/devices/soc0/revision");
    dump_file("soc type", "/sys/devices/soc0/machine");
    dump_file("soc die_id", "/sys/devices/soc0/soc_id");
    dump_file("mmc0 name", "/sys/devices/platform/dw_mmc.0/mmc_host/mmc0/mmc0:0001/name");
    dump_file("mmc0 cid", "/sys/devices/platform/dw_mmc.0/mmc_host/mmc0/mmc0:0001/cid");
    dump_file("mmc0 csd", "/sys/devices/platform/dw_mmc.0/mmc_host/mmc0/mmc0:0001/csd");
    dump_file("mmc0 ext_csd", "/d/mmc0/mmc0:0001/ext_csd");
    dump_file("wlan", "/sys/module/bcmdhd/parameters/info_string");
    dump_file("touchscreen name", "/sys/class/input/input0/name");
    dump_file("manta power", "/d/manta-power");
    dump_file("dock status", "/sys/class/switch/dock/dock_status");
    dump_file("smb347 charger regs", "/d/smb347-regs");
    dump_file("ds2784 fuel gauge regs", "/d/ds2784");
    dump_file("bus traffic shaper", "/d/bts_dev_status");
    dump_file("display controller", "/d/s3c-fb");
    dump_file("ion chunk heap", "/d/ion/ion_chunk_heap");
    dump_file("ion system heap", "/d/ion/ion_noncontig_heap");
    dump_file("ion exynos noncontig heap", "/d/ion/exynos_noncontig_heap");
    dump_file("ion exynos contig heap", "/d/ion/exynos_contig_heap");
    dump_file("shrinkers", "/d/shrinker");
    dump_file("kbase carveout", "/d/kbase_carveout");
    dump_file("HDMI", "/d/exynos5-hdmi");
    dump_file("HDMI mixer", "/d/s5p-mixer");
};
