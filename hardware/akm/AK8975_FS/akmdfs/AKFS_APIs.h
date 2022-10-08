/******************************************************************************
 * $Id: AKFS_APIs.h 580 2012-03-29 09:56:21Z yamada.rj $
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
#ifndef AKFS_INC_APIS_H
#define AKFS_INC_APIS_H

/* Include files for AK8975 library. */
#include "AKFS_Compass.h"

/*** Constant definition ******************************************************/
#define AKFS_GEOMAG_MAX 70

/*** Type declaration *********************************************************/

/*** Global variables *********************************************************/

/*** Prototype of function ****************************************************/
int16 AKFS_Init(
	const	AKFS_PATNO	hpat,
	const	uint8		regs[]
);

int16 AKFS_Release(void);

int16 AKFS_Start(const char* path);

int16 AKFS_Stop(const char* path);

int16 AKFS_Get_MAGNETIC_FIELD(
	const	int16		mag[3],
	const	int16		status,
			AKFLOAT*	vx,
			AKFLOAT*	vy,
			AKFLOAT*	vz,
			int16*		accuracy
);

int16 AKFS_Get_ACCELEROMETER(
	const   int16		acc[3],
	const	int16		status,
			AKFLOAT*    vx,
			AKFLOAT*    vy,
			AKFLOAT*    vz,
			int16*		accuracy
);

int16 AKFS_Get_ORIENTATION(
			AKFLOAT*	azimuth,
			AKFLOAT*	pitch,
			AKFLOAT*	roll,
			int16*		accuracy
);

#endif

