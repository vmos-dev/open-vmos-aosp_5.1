/******************************************************************************
 * $Id: AKFS_AK8975.h 580 2012-03-29 09:56:21Z yamada.rj $
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
#ifndef AKFS_INC_AK8975_H
#define AKFS_INC_AK8975_H

#include "AKFS_Device.h"

/***** Constant definition ****************************************************/
#define AK8975_BDATA_SIZE			8

#define AK8975_HSENSE_DEFAULT		1
#define AK8975_HSENSE_TARGET		0.3f
#define AK8975_ASENSE_DEFAULT		720
#define AK8975_ASENSE_TARGET		9.80665f

#define AK8975_HDATA_CONVERTER(hi, low, asa) \
	(AKFLOAT)((int16)((((uint16)(hi))<<8)+(uint16)(low))*(((asa)/256.0f) + 0.5f))

#define AK8975_ST_ERROR(st)   (((st)&0x09) != 0x01)

/***** Type declaration *******************************************************/

/***** Prototype of function **************************************************/
AKLIB_C_API_START
int16 AKFS_DecompAK8975(
	const	int16		mag[3],
	const	int16		status,
	const	uint8vec*	asa,
	const	int16		nhdata,
			AKFVEC		hdata[]
);
AKLIB_C_API_END

#endif

