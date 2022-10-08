/******************************************************************************
 * $Id: AKFS_Device.h 580 2012-03-29 09:56:21Z yamada.rj $
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
#ifndef AKFS_INC_DEVICE_H
#define AKFS_INC_DEVICE_H

#include <float.h>
#include "AKFS_Configure.h"

/***** Constant definition ****************************************************/
#define AKFS_ERROR			0
#define AKFS_SUCCESS		1

#define AKFS_HDATA_SIZE		32
#define AKFS_ADATA_SIZE		32

/***** Type declaration *******************************************************/
typedef signed char     int8;
typedef signed short    int16;
typedef unsigned char   uint8;
typedef unsigned short  uint16;


#ifdef AKFS_PRECISION_DOUBLE
typedef	double			AKFLOAT;
#define AKFS_EPSILON	DBL_EPSILON
#define AKFS_FMAX		DBL_MAX
#define AKFS_FMIN		DBL_MIN

#else
typedef	float			AKFLOAT;
#define AKFS_EPSILON	FLT_EPSILON
#define AKFS_FMAX		FLT_MAX
#define AKFS_FMIN		FLT_MIN

#endif

/* Treat maximum value as initial value */
#define AKFS_INIT_VALUE_F	AKFS_FMAX

/***** Vector *****/
typedef union _uint8vec{
	struct {
		uint8	x;
		uint8	y;
		uint8	z;
	}u;
	uint8	v[3];
} uint8vec;

typedef union _AKFVEC{
	struct {
		AKFLOAT x;
		AKFLOAT y;
		AKFLOAT z;
	}u;
	AKFLOAT	v[3];
} AKFVEC;

/***** Layout pattern *****/
typedef enum _AKFS_PATNO {
	PAT_INVALID = 0,
	PAT1,	/* obverse: 1st pin is right down */
	PAT2,	/* obverse: 1st pin is left down */
	PAT3,	/* obverse: 1st pin is left top */
	PAT4,	/* obverse: 1st pin is right top */
	PAT5,	/* reverse: 1st pin is left down (from top view) */
	PAT6,	/* reverse: 1st pin is left top (from top view) */
	PAT7,	/* reverse: 1st pin is right top (from top view) */
	PAT8	/* reverse: 1st pin is right down (from top view) */
} AKFS_PATNO;

/***** Prototype of function **************************************************/
AKLIB_C_API_START
int16 AKFS_InitBuffer(
	const	int16	ndata,		/*!< Size of raw vector buffer */
			AKFVEC	vdata[]		/*!< Raw vector buffer */
);

int16 AKFS_BufShift(
	const	int16	len,
	const	int16	shift,
			AKFVEC	v[]
);

int16 AKFS_Rotate(
	const   AKFS_PATNO  pat,
			AKFVEC*     vec
);
AKLIB_C_API_END

#endif

