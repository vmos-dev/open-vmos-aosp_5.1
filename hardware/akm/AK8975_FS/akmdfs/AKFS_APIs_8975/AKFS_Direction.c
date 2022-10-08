/******************************************************************************
 * $Id: AKFS_Direction.c 580 2012-03-29 09:56:21Z yamada.rj $
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
#include "AKFS_Direction.h"
#include "AKFS_VNorm.h"
#include "AKFS_Math.h"

/*
  Coordinate system is right-handed.
  X-Axis: from left to right.
  Y-Axis: from bottom to top.
  Z-Axis: from reverse to obverse.

  azimuth: Rotaion around Z axis, with positive values 
  	when y-axis moves toward the x-axis.
  pitch: Rotation around X axis, with positive values
  	when z-axis moves toward the y-axis.
  roll: Rotation around Y axis, with positive values
  	when x-axis moves toward the z-axis.
*/

/*
   This function is used internaly, so output is RADIAN!
 */
static void AKFS_Angle(
	const	AKFVEC*		avec,
			AKFLOAT*	pitch,	/* radian */
			AKFLOAT*	roll	/* radian */
)
{
	AKFLOAT	av;	/* Size of vector */

	av = AKFS_SQRT((avec->u.x)*(avec->u.x) + (avec->u.y)*(avec->u.y) + (avec->u.z)*(avec->u.z));

	*pitch = AKFS_ASIN(-(avec->u.y) / av);
	*roll  = AKFS_ASIN((avec->u.x) / av);
}

/*
   This function is used internaly, so output is RADIAN!
 */
static void AKFS_Azimuth(
	const	AKFVEC*		hvec,
	const	AKFLOAT		pitch,	/* radian */
	const	AKFLOAT		roll,	/* radian */
			AKFLOAT*	azimuth	/* radian */
)
{
	AKFLOAT sinP; /* sin value of pitch angle */
	AKFLOAT cosP; /* cos value of pitch angle */
	AKFLOAT sinR; /* sin value of roll angle */
	AKFLOAT cosR; /* cos value of roll angle */
	AKFLOAT Xh;   /* X axis element of vector which is projected to horizontal plane */
	AKFLOAT Yh;   /* Y axis element of vector which is projected to horizontal plane */

	sinP = AKFS_SIN(pitch);
	cosP = AKFS_COS(pitch);
	sinR = AKFS_SIN(roll);
	cosR = AKFS_COS(roll);

	Yh = -(hvec->u.x)*cosR + (hvec->u.z)*sinR;
	Xh = (hvec->u.x)*sinP*sinR + (hvec->u.y)*cosP + (hvec->u.z)*sinP*cosR;

	/* atan2(y, x) -> divisor and dividend is opposite from mathematical equation. */
	*azimuth = AKFS_ATAN2(Yh, Xh);
}

int16 AKFS_Direction(
	const	int16		nhvec,
	const	AKFVEC		hvec[],
	const	int16		hnave,
	const	int16		navec,
	const	AKFVEC		avec[],
	const	int16		anave,
			AKFLOAT*	azimuth,
			AKFLOAT*	pitch,
			AKFLOAT*	roll
)
{
	AKFVEC have, aave;
	AKFLOAT azimuthRad;
	AKFLOAT pitchRad;
	AKFLOAT rollRad;

	/* arguments check */
	if ((nhvec <= 0) || (navec <= 0) || (hnave <= 0) || (anave <= 0)) {
		return AKFS_ERROR;
	}
	if ((nhvec < hnave) || (navec < anave)) {
		return AKFS_ERROR;
	}

	/* average */
	if (AKFS_VbAve(nhvec, hvec, hnave, &have) != AKFS_SUCCESS) {
		return AKFS_ERROR;
	}
	if (AKFS_VbAve(navec, avec, anave, &aave) != AKFS_SUCCESS) {
		return AKFS_ERROR;
	}

	/* calculate pitch and roll */
	AKFS_Angle(&aave, &pitchRad, &rollRad);

	/* calculate azimuth */
	AKFS_Azimuth(&have, pitchRad, rollRad, &azimuthRad);

	*azimuth = RAD2DEG(azimuthRad);
	*pitch = RAD2DEG(pitchRad);
	*roll = RAD2DEG(rollRad);

	/* Adjust range of azimuth */
	if (*azimuth < 0) {
		*azimuth += 360.0f;
	}

	return AKFS_SUCCESS;
}


