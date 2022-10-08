/******************************************************************************
 * $Id: AKFS_VNorm.c 580 2012-03-29 09:56:21Z yamada.rj $
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
#include "AKFS_VNorm.h"
#include "AKFS_Device.h"

/*!
 */
int16 AKFS_VbNorm(
	const	int16	ndata,		/*!< Size of raw vector buffer */
	const	AKFVEC	vdata[],	/*!< Raw vector buffer */
	const	int16	nbuf,		/*!< Size of data to be buffered */
	const	AKFVEC*	o,			/*!< Offset */
	const	AKFVEC*	s,			/*!< Sensitivity */
	const	AKFLOAT	tgt,		/*!< Target sensitivity */
	const	int16	nvec,		/*!< Size of normalized vector buffer */
			AKFVEC	vvec[]		/*!< Normalized vector buffer */
)
{
	int i;

	/* size check */
	if ((ndata <= 0) || (nvec <= 0) || (nbuf <= 0)) {
		return AKFS_ERROR;
	}
	/* dependency check */
	if ((nbuf < 1) || (ndata < nbuf) || (nvec < nbuf)) {
		return AKFS_ERROR;
	}
	/* sensitivity check */
	if ((s->u.x <= AKFS_EPSILON) ||
		(s->u.y <= AKFS_EPSILON) ||
		(s->u.z <= AKFS_EPSILON) ||
		(tgt <= 0)) {
		return AKFS_ERROR;
	}

	/* calculate and store data to buffer */
	if (AKFS_BufShift(nvec, nbuf, vvec) != AKFS_SUCCESS) {
		return AKFS_ERROR;
	}
	for (i=0; i<nbuf; i++) {
		vvec[i].u.x = ((vdata[i].u.x - o->u.x) / (s->u.x) * (AKFLOAT)tgt);
		vvec[i].u.y = ((vdata[i].u.y - o->u.y) / (s->u.y) * (AKFLOAT)tgt);
		vvec[i].u.z = ((vdata[i].u.z - o->u.z) / (s->u.z) * (AKFLOAT)tgt);
	}

	return AKFS_SUCCESS;
}

/*!
 */
int16 AKFS_VbAve(
	const	int16	nvec,		/*!< Size of normalized vector buffer */
	const	AKFVEC	vvec[],		/*!< Normalized vector buffer */
	const	int16	nave,		/*!< Number of averaeg */
			AKFVEC*	vave		/*!< Averaged vector */
)
{
	int i;

	/* arguments check */
	if ((nave <= 0) || (nvec <= 0) || (nvec < nave)) {
		return AKFS_ERROR;
	}

	/* calculate average */
	vave->u.x = 0;
	vave->u.y = 0;
	vave->u.z = 0;
	for (i=0; i<nave; i++) {
		if ((vvec[i].u.x == AKFS_INIT_VALUE_F) ||
			(vvec[i].u.y == AKFS_INIT_VALUE_F) ||
			(vvec[i].u.z == AKFS_INIT_VALUE_F)) {
				break;
		}
		vave->u.x += vvec[i].u.x;
		vave->u.y += vvec[i].u.y;
		vave->u.z += vvec[i].u.z;
	}
	if (i == 0) {
		vave->u.x = 0;
		vave->u.y = 0;
		vave->u.z = 0;
	} else {
		vave->u.x /= i;
		vave->u.y /= i;
		vave->u.z /= i;
	}
	return AKFS_SUCCESS;
}


