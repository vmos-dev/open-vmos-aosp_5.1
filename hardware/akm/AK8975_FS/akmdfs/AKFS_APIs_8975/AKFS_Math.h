/******************************************************************************
 * $Id: AKFS_Math.h 580 2012-03-29 09:56:21Z yamada.rj $
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
#ifndef AKFS_INC_MATH_H
#define AKFS_INC_MATH_H

#include <math.h>
#include "AKFS_Configure.h"

/***** Constant definition ****************************************************/
#define AKFS_PI			3.141592654f
#define RAD2DEG(rad)	((rad)*180.0f/AKFS_PI)

/***** Macro definition *******************************************************/

#ifdef AKFS_PRECISION_DOUBLE
#define AKFS_SIN(x)			sin(x)
#define AKFS_COS(x)			cos(x)
#define AKFS_ASIN(x)		asin(x)
#define AKFS_ACOS(x)		acos(x)
#define AKFS_ATAN2(y, x)	atan2((y), (x))
#define AKFS_SQRT(x)		sqrt(x)
#else
#define AKFS_SIN(x)			sinf(x)
#define AKFS_COS(x)			cosf(x)
#define AKFS_ASIN(x)		asinf(x)
#define AKFS_ACOS(x)		acosf(x)
#define AKFS_ATAN2(y, x)	atan2f((y), (x))
#define AKFS_SQRT(x)		sqrtf(x)
#endif

#endif

