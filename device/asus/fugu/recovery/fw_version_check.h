/*
 * Copyright (C) 2014 Intel Corporation
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

#ifndef IFWI_VERSION_CHECK_H
#define IFWI_VERSION_CHECK_H

#include <stdint.h>

struct fw_version {
	uint16_t major;
	uint16_t minor;
};

struct firmware_versions {
	struct fw_version scubootstrap;
	struct fw_version scu;
	struct fw_version ia32;
	struct fw_version valhooks;
	struct fw_version ifwi;
	struct fw_version chaabi;
	struct fw_version mia;
};

/* Query the SCU for current firmware versions and populate
 * the fields in v. Returns nonzero on error */
int get_current_fw_rev(struct firmware_versions *v);

/* Assuming data points to a blob of memory containing an IFWI
 * firmware image, inpsect the FIP header inside it and
 * populate the fields in v. Returns nonzero on error */
int get_image_fw_rev(void *data, unsigned sz, struct firmware_versions *v);

/* Compare versions v1 and v2, and return -1, 1, or 0 if v1 is less than,
 * greater than, or equal to v2, respectively */
int fw_vercmp(struct firmware_versions *v1, struct firmware_versions *v2);

/* Crack ifwi firmware file */
int crack_update_fw(const char *fw_file, struct fw_version *ifwi_version);

/* Crack ifwi firmware file to get the PTI Field. */
int crack_update_fw_pti_field(const char *fw_file, uint8_t * pti_field);

#endif
