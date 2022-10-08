/*
 * Copyright (C) 2012 Samsung Electronics Co., LTD
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

#ifndef TLWVDRM_API_H_
#define TLWVDRM_API_H_

#include "tci.h"

/**
 * Command ID's for communication Trustlet Connector -> Trustlet.
 */
#define CMD_WV_DRM_ENABLE_PATH_PROTECTION	0x00010000
#define CMD_WV_DRM_DISABLE_PATH_PROTECTION	0x00010001


/**
 * Return codes
 */
#define RET_TL_WV_DRM_OK 0x00000000

/**
 * Error codes
 */
#define RET_ERR_WV_DRM_PROTECT_CONTENT_PATH_INIT 0x00001000
#define RET_ERR_WV_DRM_PROTECT_CONTENT_PATH_TERM 0x00001001


/**
 * Maximum data length.
 */
#define MAX_DATA_LEN 512

/**
 * TCI message data.
 */

typedef struct {
	uint32_t id;
	uint32_t data_len;
	uint8_t	*data_ptr;
	uint8_t	data[MAX_DATA_LEN];
} tci_cmd_t;

typedef struct {
	uint32_t id;
	uint32_t return_code;
	uint32_t data_len;
	uint8_t	*data_ptr;
	uint8_t	data[MAX_DATA_LEN];
} tci_resp_t;

typedef struct {
	union {
		tci_cmd_t cmd;	/**< Command message structure */
		tci_resp_t resp;	/**< Response message structure */
	};
} tciMessage_t;

/**
 * Trustlet UUID.
 */
#define TL_WV_DRM_UUID { { 0, 6, 3, 8, 6, 5, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0 } }

#endif /* TLWVDRM_API_H_ */
