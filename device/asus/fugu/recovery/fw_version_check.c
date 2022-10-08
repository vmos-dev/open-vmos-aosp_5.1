/*
 * Copyright (C) 2011 Intel Corporation
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

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdint.h>

#include "fw_version_check.h"

#define DEVICE_NAME	"/sys/kernel/fw_update/fw_info/fw_version"
#define FIP_PATTERN	0x50494624
#define SCU_IPC_VERSION_LEN_LONG 32
#define READ_SZ 256

struct fip_version_block {
	uint16_t minor;
	uint16_t major;
	uint8_t checksum;
	uint8_t reserved8;
	uint16_t reserved16;
};

struct fip_version_block_chxx {
	uint16_t minor;
	uint16_t major;
	uint8_t checksum;
	uint8_t reserved8;
	uint16_t reserved16;
	uint16_t size;
	uint16_t dest;
};

struct FIP_header {
	uint32_t FIP_SIG;
	struct fip_version_block umip_rev;
	struct fip_version_block spat_rev;
	struct fip_version_block spct_rev;
	struct fip_version_block rpch_rev;
	struct fip_version_block ch00_rev;
	struct fip_version_block mipd_rev;
	struct fip_version_block mipn_rev;
	struct fip_version_block scuc_rev;
	struct fip_version_block hvm_rev;
	struct fip_version_block mia_rev;
	struct fip_version_block ia32_rev;
	struct fip_version_block oem_rev;
	struct fip_version_block ved_rev;
	struct fip_version_block vec_rev;
	struct fip_version_block mos_rev;
	struct fip_version_block pos_rev;
	struct fip_version_block cos_rev;
	struct fip_version_block_chxx ch01_rev;
	struct fip_version_block_chxx ch02_rev;
	struct fip_version_block_chxx ch03_rev;
	struct fip_version_block_chxx ch04_rev;
	struct fip_version_block_chxx ch05_rev;
	struct fip_version_block_chxx ch06_rev;
	struct fip_version_block_chxx ch07_rev;
	struct fip_version_block_chxx ch08_rev;
	struct fip_version_block_chxx ch09_rev;
	struct fip_version_block_chxx ch10_rev;
	struct fip_version_block_chxx ch11_rev;
	struct fip_version_block_chxx ch12_rev;
	struct fip_version_block_chxx ch13_rev;
	struct fip_version_block_chxx ch14_rev;
	struct fip_version_block_chxx ch15_rev;
	struct fip_version_block dnx_rev;
	struct fip_version_block reserved0_rev;
	struct fip_version_block reserved1_rev;
	struct fip_version_block ifwi_rev;
};

static int read_fw_revision(unsigned int *fw_revision, int len)
{
	int i, fw_info, ret;
	const char *sep = " ";
	char *p, *save;
	char buf[READ_SZ];

	fw_info = open(DEVICE_NAME, O_RDONLY);
	if (fw_info < 0) {
		fprintf(stderr, "failed to open %s ", DEVICE_NAME);
		return fw_info;
	}

	ret = read(fw_info, buf, READ_SZ - 1);
	if (ret < 0) {
		fprintf(stderr, "failed to read fw_revision, ret = %d\n", ret);
		goto err;
	}

	buf[ret] = 0;
	p = strtok_r(buf, sep, &save);
	for (i = 0; p && i < len; i++) {
		ret = sscanf(p, "%x", &fw_revision[i]);
		if (ret != 1) {
			fprintf(stderr, "failed to parse fw_revision, ret = %d\n", ret);
			goto err;
		}
		p = strtok_r(NULL, sep, &save);
	}
	ret = 0;

err:
	close(fw_info);
	return ret;
}

/* Bytes in scu_ipc_version after the ioctl():

 * 00 SCU Boot Strap Firmware Minor Revision Low
 * 01 SCU Boot Strap Firmware Minor Revision High
 * 02 SCU Boot Strap Firmware Major Revision Low
 * 03 SCU Boot Strap Firmware Major Revision High

 * 04 SCU Firmware Minor Revision Low
 * 05 SCU Firmware Minor Revision High
 * 06 SCU Firmware Major Revision Low
 * 07 SCU Firmware Major Revision High

 * 08 IA Firmware Minor Revision Low
 * 09 IA Firmware Minor Revision High
 * 10 IA Firmware Major Revision Low
 * 11 IA Firmware Major Revision High

 * 12 Validation Hooks Firmware Minor Revision Low
 * 13 Validation Hooks Firmware Minor Revision High
 * 14 Validation Hooks Firmware Major Revision Low
 * 15 Validation Hooks Firmware Major Revision High

 * 16 IFWI Firmware Minor Revision Low
 * 17 IFWI Firmware Minor Revision High
 * 18 IFWI Firmware Major Revision Low
 * 19 IFWI Firmware Major Revision High

 * 20 Chaabi Firmware Minor Revision Low
 * 21 Chaabi Firmware Minor Revision High
 * 22 Chaabi Firmware Major Revision Low
 * 23 Chaabi Firmware Major Revision High

 * 24 mIA Firmware Minor Revision Low
 * 25 mIA Firmware Minor Revision High
 * 26 mIA Firmware Major Revision Low
 * 27 mIA Firmware Major Revision High

 */
int get_current_fw_rev(struct firmware_versions *v)
{
	int ret;
	unsigned int fw_revision[SCU_IPC_VERSION_LEN_LONG] = { 0 };

	ret = read_fw_revision(fw_revision, SCU_IPC_VERSION_LEN_LONG);
	if (ret)
		return ret;

	v->scubootstrap.minor = fw_revision[1] << 8 | fw_revision[0];
	v->scubootstrap.major = fw_revision[3] << 8 | fw_revision[2];
	v->scu.minor = fw_revision[5] << 8 | fw_revision[4];
	v->scu.major = fw_revision[7] << 8 | fw_revision[6];
	v->ia32.minor = fw_revision[9] << 8 | fw_revision[8];
	v->ia32.major = fw_revision[11] << 8 | fw_revision[10];
	v->valhooks.minor = fw_revision[13] << 8 | fw_revision[12];
	v->valhooks.major = fw_revision[15] << 8 | fw_revision[14];
	v->ifwi.minor = fw_revision[17] << 8 | fw_revision[16];
	v->ifwi.major = fw_revision[19] << 8 | fw_revision[18];
	v->chaabi.minor = fw_revision[21] << 8 | fw_revision[20];
	v->chaabi.major = fw_revision[23] << 8 | fw_revision[22];
	v->mia.minor = fw_revision[25] << 8 | fw_revision[24];
	v->mia.major = fw_revision[27] << 8 | fw_revision[26];

	return ret;
}

int get_image_fw_rev(void *data, unsigned sz, struct firmware_versions *v)
{
	struct FIP_header fip;
	unsigned char *databytes = (unsigned char *)data;
	int magic;
	int magic_found = 0;

	if (v == NULL) {
		fprintf(stderr, "Null pointer !\n");
		return -1;
	} else
		memset((void *)v, 0, sizeof(struct firmware_versions));

	while (sz >= sizeof(fip)) {

		/* Scan for the FIP magic */
		while (sz >= sizeof(fip)) {
			memcpy(&magic, databytes, sizeof(magic));
			if (magic == FIP_PATTERN) {
				magic_found = 1;
				break;
			}
			databytes += sizeof(magic);
			sz -= sizeof(magic);
		}

		if (!magic_found) {
			fprintf(stderr, "Couldn't find FIP magic in image!\n");
			return -1;
		}
		if (sz < sizeof(fip)) {
			break;
		}

		memcpy(&fip, databytes, sizeof(fip));

		/* not available in ifwi file */
		v->scubootstrap.minor = 0;
		v->scubootstrap.major = 0;

		/* don't update if null */
		if (fip.scuc_rev.minor != 0)
			v->scu.minor = fip.scuc_rev.minor;
		if (fip.scuc_rev.major != 0)
			v->scu.major = fip.scuc_rev.major;
		if (fip.ia32_rev.minor != 0)
			v->ia32.minor = fip.ia32_rev.minor;
		if (fip.ia32_rev.major != 0)
			v->ia32.major = fip.ia32_rev.major;
		if (fip.oem_rev.minor != 0)
			v->valhooks.minor = fip.oem_rev.minor;
		if (fip.oem_rev.major != 0)
			v->valhooks.major = fip.oem_rev.major;
		if (fip.ifwi_rev.minor != 0)
			v->ifwi.minor = fip.ifwi_rev.minor;
		if (fip.ifwi_rev.major != 0)
			v->ifwi.major = fip.ifwi_rev.major;
		if (fip.ch00_rev.minor != 0)
			v->chaabi.minor = fip.ch00_rev.minor;
		if (fip.ch00_rev.major != 0)
			v->chaabi.major = fip.ch00_rev.major;
		if (fip.mia_rev.minor != 0)
			v->mia.minor = fip.mia_rev.minor;
		if (fip.mia_rev.major != 0)
			v->mia.major = fip.mia_rev.major;

		databytes += sizeof(magic);
		sz -= sizeof(magic);
	}

	return 0;
}
