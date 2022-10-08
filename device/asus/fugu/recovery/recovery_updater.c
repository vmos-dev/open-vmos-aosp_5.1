/*
 * Copyright 2014 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cutils/properties.h>
#include <sys/mman.h>
#include "fw_version_check.h"
#include "edify/expr.h"

#define FORCE_RW_OPT            "0"
#define BOOT_IFWI_SIZE          0x400000
#define BOOT_UMIP_SIZE          0x10000
#define BOOT_UMIP_SECTOR_SIZE   0x200
#define BOOT_UMIP_XOR_OFFSET    0x7
#define BOOT_UMIP_3GPP_OFFSET   0x76F
#define BOOT_IFWI_XOR_OFFSET    0x0112d8
#define BOOT_DNX_TIMEOUT_OFFSET 0x400
#define IFWI_OFFSET             0
#define TOKEN_UMIP_AREA_OFFSET  0x4000
#define TOKEN_UMIP_AREA_SIZE    0x2C00
#define FILE_PATH_SIZE          64
#define IFWI_TYPE_LSH           12

static void dump_fw_versions(struct firmware_versions *v)
{
	fprintf(stderr, "Image FW versions:\n");
	fprintf(stderr, "	   ifwi: %04X.%04X\n", v->ifwi.major, v->ifwi.minor);
	fprintf(stderr, "---- components ----\n");
	fprintf(stderr, "	    scu: %04X.%04X\n", v->scu.major, v->scu.minor);
	fprintf(stderr, "    hooks/oem: %04X.%04X\n", v->valhooks.major, v->valhooks.minor);
	fprintf(stderr, "	   ia32: %04X.%04X\n", v->ia32.major, v->ia32.minor);
	fprintf(stderr, "	 chaabi: %04X.%04X\n", v->chaabi.major, v->chaabi.minor);
	fprintf(stderr, "	    mIA: %04X.%04X\n", v->mia.major, v->mia.minor);
}

static int force_rw(char *name) {
	int ret, fd;

	fd = open(name, O_WRONLY);
	if (fd < 0) {
		fprintf(stderr, "force_ro(): failed to open %s\n", name);
		return fd;
	}

	ret = write(fd, FORCE_RW_OPT, sizeof(FORCE_RW_OPT));
	if (ret <= 0) {
		fprintf(stderr, "force_ro(): failed to write %s\n", name);
		close(fd);
		return ret;
	}

	close(fd);
	return 0;
}

int check_ifwi_file_scu_emmc(void *data, size_t size)
{
	struct firmware_versions dev_fw_rev, img_fw_rev;

	if (get_image_fw_rev(data, size, &img_fw_rev)) {
		fprintf(stderr, "Coudn't extract FW version data from image\n");
		return -1;
	}

	dump_fw_versions(&img_fw_rev);
	if (get_current_fw_rev(&dev_fw_rev)) {
		fprintf(stderr, "Couldn't query existing IFWI version\n");
		return -1;
	}
	fprintf(stderr,
		"Attempting to flash ifwi image version %04X.%04X over ifwi current version %04X.%04X\n",
		img_fw_rev.ifwi.major, img_fw_rev.ifwi.minor, dev_fw_rev.ifwi.major, dev_fw_rev.ifwi.minor);

	if (img_fw_rev.ifwi.major != dev_fw_rev.ifwi.major) {
		fprintf(stderr,
			"IFWI FW Major version numbers (file=%04X current=%04X) don't match, Update abort.\n",
			img_fw_rev.ifwi.major, dev_fw_rev.ifwi.major);
		return -1;
	}

	return 1;
}

static uint32_t xor_compute(char *ptr, uint32_t size)
{
	uint32_t xor = 0;
	uint32_t i;

	for (i = 0; i < size; i+=4)
		xor = xor ^ *(uint32_t *)(ptr + i);

	return xor;
}

static uint8_t xor_factorize(uint32_t xor)
{
	return (uint8_t)((xor & 0xff) ^ ((xor >> 8) & 0xff) ^ ((xor >> 16) & 0xff) ^ ((xor >> 24) & 0xff));
}

static void xor_update(char *ptr)
{
	uint16_t i;
	uint32_t xor;

	/* update UMIP xor of sector 2 to 127 */
	for (i = 2; i < 128; i++) {
		xor = xor_compute(ptr + i * BOOT_UMIP_SECTOR_SIZE, BOOT_UMIP_SECTOR_SIZE);
		*(uint32_t *)(ptr + 4 * i) = xor;
	}

	/* update UMIP xor */
	*(ptr + BOOT_UMIP_XOR_OFFSET) = 0;
	xor = xor_compute(ptr, BOOT_UMIP_SIZE);
	*(ptr + BOOT_UMIP_XOR_OFFSET) = xor_factorize(xor);

	/* update IFWI xor */
	*(uint32_t *)(ptr + BOOT_IFWI_XOR_OFFSET) = 0x0;
	xor = xor_compute(ptr, BOOT_IFWI_SIZE);
	*(uint32_t *)(ptr + BOOT_IFWI_XOR_OFFSET) = xor;
}

static int write_umip_emmc(uint32_t addr_offset, void *data, size_t size)
{
	int boot_fd = 0;
	int boot_index;
	char boot_partition[FILE_PATH_SIZE];
	char boot_partition_force_ro[FILE_PATH_SIZE];
	char *ptr;
	char *token_data;

	if (addr_offset == IFWI_OFFSET) {
		token_data = malloc(TOKEN_UMIP_AREA_SIZE);
		if (!token_data) {
			fprintf(stderr, "write_umip_emmc: Malloc error\n");
			return -1;
		}

		if (size > BOOT_IFWI_SIZE) {
			fprintf(stderr, "write_umip_emmc: Truncating last %d bytes from the IFWI\n",
			(size - BOOT_IFWI_SIZE));
			/* Since the last 144 bytes are the FUP header which are not required,*/
			/* we truncate it to fit into the boot partition. */
			size = BOOT_IFWI_SIZE;
		}
	}

	for (boot_index = 0; boot_index < 2; boot_index++) {
		snprintf(boot_partition, FILE_PATH_SIZE, "/dev/block/mmcblk0boot%d", boot_index);
		snprintf(boot_partition_force_ro, FILE_PATH_SIZE, "/sys/block/mmcblk0boot%d/force_ro", boot_index);

		if (force_rw(boot_partition_force_ro)) {
			fprintf(stderr, "write_umip_emmc: unable to force_ro %s\n", boot_partition);
			goto err_boot1;
		}
		boot_fd = open(boot_partition, O_RDWR);
		if (boot_fd < 0) {
			fprintf(stderr, "write_umip_emmc: failed to open %s\n", boot_partition);
			goto err_boot1;
		}

		ptr = (char *)mmap(NULL, BOOT_IFWI_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, boot_fd, 0);
		if (ptr == MAP_FAILED) {
			fprintf(stderr, "write_umip_emmc: mmap failed on boot%d with error : %s\n", boot_index, strerror(errno));
			goto err_boot1;
		}

		if (addr_offset == IFWI_OFFSET)
			memcpy(token_data, ptr + TOKEN_UMIP_AREA_OFFSET, TOKEN_UMIP_AREA_SIZE);

		/* Write the data */
		if (addr_offset + size <= BOOT_IFWI_SIZE)
			if (data == NULL)
				memset(ptr + addr_offset, 0, size);
			else
				memcpy(ptr + addr_offset, data, size);
		else {
			fprintf(stderr, "write_umip_emmc: write failed\n");
			goto err_boot2;
		}

		if (addr_offset == IFWI_OFFSET)
			memcpy(ptr + TOKEN_UMIP_AREA_OFFSET, token_data, TOKEN_UMIP_AREA_SIZE);

		/* Compute and write xor */
		xor_update(ptr);

		munmap(ptr, BOOT_IFWI_SIZE);
		close(boot_fd);
	}

	if (addr_offset == IFWI_OFFSET)
		free(token_data);
	return 0;

err_boot2:
	munmap(ptr, BOOT_IFWI_SIZE);

err_boot1:
	if (addr_offset == IFWI_OFFSET)
		free(token_data);
	close(boot_fd);
	return -1;
}

static int readbyte_umip_emmc(uint32_t addr_offset)
{
	int boot_fd = 0;
	char *ptr;
	int value = 0;

	if (force_rw("/sys/block/mmcblk0boot0/force_ro")) {
		fprintf(stderr, "read_umip_emmc: unable to force_ro\n");
		goto err_boot1;
	}
	boot_fd = open("/dev/block/mmcblk0boot0", O_RDWR);
	if (boot_fd < 0) {
		fprintf(stderr, "read_umip_emmc: failed to open /dev/block/mmcblk0boot0\n");
		goto err_boot1;
	}

	ptr = (char *)mmap(NULL, BOOT_UMIP_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, boot_fd, 0);
	if (ptr == MAP_FAILED) {
		fprintf(stderr, "read_umip_emmc: mmap failed on boot0 with error : %s\n", strerror(errno));
		goto err_boot1;
	}

	/* Read the data */
	if (addr_offset < BOOT_UMIP_SIZE)
		value = (int)*(ptr + addr_offset);
	else {
		fprintf(stderr, "read_umip_emmc: read failed\n");
		goto err_boot2;
	}

	munmap(ptr, BOOT_UMIP_SIZE);
	close(boot_fd);

	return value;

err_boot2:
	munmap(ptr, BOOT_UMIP_SIZE);

err_boot1:
	close(boot_fd);
	return -1;
}

int update_ifwi_file_scu_emmc(void *data, size_t size)
{
	return write_umip_emmc(IFWI_OFFSET, data, size);
}

int flash_ifwi_scu_emmc(void *data, unsigned size)
{
	int ret;

	ret = check_ifwi_file_scu_emmc(data, size);
	if (ret > 0)
		return update_ifwi_file_scu_emmc(data, size);

	return ret;
}

Value* FlashIfwiFuguFn(const char *name, State * state, int argc, Expr * argv[]) {
	Value *ret = NULL;
	char *filename = NULL;
	unsigned char *buffer = NULL;
	int ifwi_size;
	FILE *f = NULL;

	if (argc != 1) {
		ErrorAbort(state, "%s() expected 1 arg, got %d", name, argc);
		return NULL;
	}
	if (ReadArgs(state, argv, 1, &filename) < 0) {
		ErrorAbort(state, "%s() invalid args ", name);
		return NULL;
	}

	if (filename == NULL || strlen(filename) == 0) {
		ErrorAbort(state, "filename argument to %s can't be empty", name);
		goto done;
	}

	if ((f = fopen(filename,"rb")) == NULL) {
		ErrorAbort(state, "Unable to open file %s: %s ", filename, strerror(errno));
		goto done;
	}

	fseek(f, 0, SEEK_END);
	ifwi_size = ftell(f);
	if (ifwi_size < 0) {
		ErrorAbort(state, "Unable to get ifwi_size ");
		goto done;
	};
	fseek(f, 0, SEEK_SET);

	if ((buffer = malloc(ifwi_size)) == NULL) {
		ErrorAbort(state, "Unable to alloc ifwi flash buffer of size %d", ifwi_size);
		goto done;
	}
	fread(buffer, ifwi_size, 1, f);
	fclose(f);

	if(flash_ifwi_scu_emmc(buffer, ifwi_size) !=0) {
		ErrorAbort(state, "Unable to flash ifwi in emmc");
		free(buffer);
		goto done;
	};

	free(buffer);
	ret = StringValue(strdup(""));

done:
	if (filename)
		free(filename);

	return ret;
}

void Register_librecovery_updater_fugu() {
	RegisterFunction("fugu.flash_ifwi", FlashIfwiFuguFn);
}
