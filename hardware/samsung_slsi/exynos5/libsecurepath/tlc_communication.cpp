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

#include <stdlib.h>

#include "tlc_communication.h"

#define LOG_TAG "tlc_communication"
#include "log.h"

mcResult_t tlc_open(mc_comm_ctx *comm_ctx) {
	mcResult_t	mcRet;

	LOG_I("open() called");
	do {
		// -------------------------------------------------------------
		// Step 1: Open the MobiCore device
		LOG_I("Opening MobiCore device");
		mcRet = mcOpenDevice(comm_ctx->device_id);
		if (MC_DRV_OK != mcRet)
			LOG_I("mcOpenDevice result: %d", mcRet);


		// -------------------------------------------------------------
		// Step 2: Allocate WSM buffer for the TCI
		LOG_I("Allocating WSM for TCI");
		mcRet = mcMallocWsm(comm_ctx->device_id, 0, sizeof(tciMessage_t), (uint8_t **)&(comm_ctx->tci_msg), 0);
		if (MC_DRV_OK != mcRet) {
			LOG_E("Allocation of TCI WSM failed: %d", mcRet);
			break;
		}

		// -------------------------------------------------------------
		// Step 3: Open session with the Trustlet
		LOG_I("Opening the session");
		bzero(&(comm_ctx->handle), sizeof(mcSessionHandle_t)); // Clear the session handle

		comm_ctx->handle.deviceId = comm_ctx->device_id; // The device ID (default device is used)

		mcRet = mcOpenSession(&(comm_ctx->handle), &(comm_ctx->uuid), (uint8_t *)(comm_ctx->tci_msg),
				(uint32_t) sizeof(tciMessage_t));
		if (MC_DRV_OK != mcRet) {
			LOG_E("Open session failed: %d", mcRet);
			break;
		}

		LOG_I("tlc_open() succeeded");
	} while (false);

	return mcRet;
}

mcResult_t tlc_close(mc_comm_ctx *comm_ctx) {
	mcResult_t	mcRet;

	LOG_I("close() called");
	do {

		// -------------------------------------------------------------
		// Step 1: Free WSM
		LOG_I("Free WSM");
		mcRet = mcFreeWsm((comm_ctx->device_id), (uint8_t *)(comm_ctx->tci_msg));
		if (MC_DRV_OK != mcRet) {
			LOG_E("Free WSM failed: %d", mcRet);
			break;
		}

		// -------------------------------------------------------------
		// Step 2: Close session with the Trustlet
		LOG_I("Closing the session");
		mcRet = mcCloseSession(&(comm_ctx->handle));
		if (MC_DRV_OK != mcRet) {
			LOG_E("Closing session failed: %d", mcRet);
			break;
		}

		// -------------------------------------------------------------
		// Step 3: Close the MobiCore device
		LOG_I("Closing MobiCore device");
		mcRet = mcCloseDevice(comm_ctx->device_id);
		if (MC_DRV_OK != mcRet) {
			LOG_E("Closing MobiCore device failed: %d", mcRet);
			break;
		}

		LOG_I("tlc_close() succeeded");
	} while (false);

	return mcRet;
}

mcResult_t tlc_communicate(mc_comm_ctx *comm_ctx) {
	mcResult_t	mcRet;

	do {
		// -------------------------------------------------------------
		// Step 1: signal the Trustlet
		mcRet = mcNotify(&(comm_ctx->handle));
		if (MC_DRV_OK != mcRet) {
			LOG_E("Notify failed: %d", mcRet);
			break;
		}
		LOG_I("mcNotify is completed\n");

		// -------------------------------------------------------------
		// Step 2: Wait for the Trustlet response
		mcRet = mcWaitNotification(&(comm_ctx->handle), -1);
		if (MC_DRV_OK != mcRet) {
			LOG_E("Wait for response notification failed: %d", mcRet);
			break;
		}

		LOG_I("mcWaitNotification is completed");

	} while (false);

	return mcRet;
}
