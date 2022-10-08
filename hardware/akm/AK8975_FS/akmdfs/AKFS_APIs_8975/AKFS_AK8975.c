/******************************************************************************
 * $Id: AKFS_AK8975.c 580 2012-03-29 09:56:21Z yamada.rj $
 ******************************************************************************
 *
 * Copyright (C) 2012 Asahi Kasei Microdevices Corporation, Japan
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
#include "AKFS_AK8975.h"
#include "AKFS_Device.h"

/*!
 */
int16 AKFS_DecompAK8975(
	const	int16		mag[3],
	const	int16		status,
	const	uint8vec*	asa,
	const	int16		nhdata,
			AKFVEC		hdata[]
)
{
	/* put st1 and st2 value */
	if (AK8975_ST_ERROR(status)) {
		return AKFS_ERROR;
	}

	/* magnetic */
	AKFS_BufShift(nhdata, 1, hdata);
	hdata[0].u.x = mag[0] * (((asa->u.x)/256.0f) + 0.5f);
	hdata[0].u.y = mag[1] * (((asa->u.y)/256.0f) + 0.5f);
	hdata[0].u.z = mag[2] * (((asa->u.z)/256.0f) + 0.5f);

	return AKFS_SUCCESS;
}

