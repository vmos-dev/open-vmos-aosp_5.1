/******************************************************************************
 * $Id: AKFS_VNorm.h 580 2012-03-29 09:56:21Z yamada.rj $
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
#ifndef AKFS_INC_VNORM_H
#define AKFS_INC_VNORM_H

#include "AKFS_Device.h"

/***** Prototype of function **************************************************/
AKLIB_C_API_START
int16 AKFS_VbNorm(
	const	int16	ndata,		/*!< Size of raw vector buffer */
	const	AKFVEC	vdata[],	/*!< Raw vector buffer */
	const	int16	nbuf,		/*!< Size of data to be buffered */
	const	AKFVEC*	o,			/*!< Offset */
	const	AKFVEC*	s,			/*!< Sensitivity */
	const	AKFLOAT	tgt,		/*!< Target sensitivity */
	const	int16	nvec,		/*!< Size of normalized vector buffer */
			AKFVEC	vvec[]		/*!< Normalized vector buffer */
);

int16 AKFS_VbAve(
	const	int16	nvec,		/*!< Size of normalized vector buffer */
	const	AKFVEC	vvec[],		/*!< Normalized vector buffer */
	const	int16	nave,		/*!< Number of averaeg */
			AKFVEC*	vave		/*!< Averaged vector */
);

AKLIB_C_API_END

#endif

