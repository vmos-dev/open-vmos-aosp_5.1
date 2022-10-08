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
	dump_file("board revision",
		"/sys/devices/platform/grouper_misc/grouper_pcbid");
	dump_file("board gpio status",
		"/d/gpio");
	dump_file("soc fuse production mode",
		"/sys/firmware/fuse/odm_production_mode");
	dump_file("emmc revision",
		"/sys/devices/platform/sdhci-tegra.3/mmc_host/mmc0/mmc0:0001/"
		"prv");
	dump_file("emmc capacity",
		"/sys/devices/platform/sdhci-tegra.3/mmc_host/mmc0/mmc0:0001/"
		"sec_count");
	dump_file("wlan", "/sys/module/bcmdhd/parameters/info_string");
	dump_file("touch panel vendor and firmware version",
		"/sys/class/switch/touch/name");
};
