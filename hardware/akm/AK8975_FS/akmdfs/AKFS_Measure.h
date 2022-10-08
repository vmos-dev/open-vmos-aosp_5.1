/******************************************************************************
 * $Id: AKFS_Measure.h 580 2012-03-29 09:56:21Z yamada.rj $
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
#ifndef AKFS_INC_MEASURE_H
#define AKFS_INC_MEASURE_H

/* Include files for AK8975 library. */
#include "AKFS_Compass.h"

/*** Constant definition ******************************************************/
#define AK8975_SELFTEST_MIN_X	-100
#define AK8975_SELFTEST_MAX_X	100

#define AK8975_SELFTEST_MIN_Y	-100
#define AK8975_SELFTEST_MAX_Y	100

#define AK8975_SELFTEST_MIN_Z	-1000
#define AK8975_SELFTEST_MAX_Z	-300

#define CONVERT_ACC(a)	((int)((a) * 720 / 9.8f))
#define CONVERT_MAG(m)	((int)((m) / 0.06f))
#define CONVERT_ORI(o)	((int)((o) * 64))

/*** Type declaration *********************************************************/

/*** Global variables *********************************************************/

/*** Prototype of function ****************************************************/
int16 AKFS_ReadAK8975FUSEROM(
		uint8 regs[3]
);

int16 AKFS_SelfTest(void);

struct timespec AKFS_CalcSleep(
	const struct timespec* end,
	const struct timespec* start,
	const int64_t minimum
);

int16 AKFS_GetInterval(
		uint16*  flag,
		int64_t* minimum
);

void AKFS_OutputResult(
	const	uint16			flag,
	const	AKSENSOR_DATA*	acc,
	const	AKSENSOR_DATA*	mag,
	const	AKSENSOR_DATA*	ori
);

void AKFS_MeasureLoop(void);

#endif

