/******************************************************************************
 * $Id: AKFS_AOC.h 580 2012-03-29 09:56:21Z yamada.rj $
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
#ifndef AKFS_INC_AOC_H
#define AKFS_INC_AOC_H

#include "AKFS_Device.h"

/***** Constant definition ****************************************************/
#define AKFS_HBUF_SIZE	20
#define AKFS_HOBUF_SIZE	4
#define AKFS_HR_TH		10
#define AKFS_HO_TH		0.15

/***** Macro definition *******************************************************/

/***** Type declaration *******************************************************/
typedef struct _AKFS_AOC_VAR{
	AKFVEC		hbuf[AKFS_HBUF_SIZE];
	AKFVEC		hobuf[AKFS_HOBUF_SIZE];
	AKFLOAT		hraoc;
} AKFS_AOC_VAR;

/***** Prototype of function **************************************************/
AKLIB_C_API_START
int16 AKFS_AOC(
			AKFS_AOC_VAR*	haocv,
	const	AKFVEC*			hdata,
			AKFVEC*			ho
);

void AKFS_InitAOC(
			AKFS_AOC_VAR*	haocv
);

AKLIB_C_API_END

#endif

