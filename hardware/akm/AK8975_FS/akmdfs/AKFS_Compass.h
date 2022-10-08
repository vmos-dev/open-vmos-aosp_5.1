/******************************************************************************
 * $Id: AKFS_Compass.h 580 2012-03-29 09:56:21Z yamada.rj $
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
#ifndef AKFS_INC_COMPASS_H
#define AKFS_INC_COMPASS_H

#include "AKFS_Common.h"
#include "AKFS_CSpec.h"

#ifdef WIN32
#include "AK8975_LinuxDriver.h"
#else
#include "AK8975Driver.h"
#endif

/****************************************/
/* Include files for AK8975 library.    */
/****************************************/
#include "AKFS_AK8975.h"
#include "AKFS_Configure.h"
#include "AKFS_AOC.h"
#include "AKFS_Device.h"
#include "AKFS_Direction.h"
#include "AKFS_Math.h"
#include "AKFS_VNorm.h"

/*** Constant definition ******************************************************/

/*** Type declaration *********************************************************/
typedef struct _AKSENSOR_DATA{
	AKFLOAT	x;
	AKFLOAT	y;
	AKFLOAT	z;
    int8	status;
} AKSENSOR_DATA;

/*! A parameter structure. */
typedef struct _AK8975PRMS{
	/* Variables for Decomp8975. */
	AKFVEC			mfv_hdata[AKFS_HDATA_SIZE];
	uint8vec		mi_asa;
	uint8			mi_st;

	/* Variables forAOC. */
	AKFS_AOC_VAR	m_aocv;

	/* Variables for Magnetometer buffer. */
	AKFVEC			mfv_hvbuf[AKFS_HDATA_SIZE];
	AKFVEC			mfv_ho;
	AKFVEC			mfv_hs;
	AKFS_PATNO		m_hpat;

	/* Variables for Accelerometer buffer. */
	AKFVEC			mfv_adata[AKFS_ADATA_SIZE];
	AKFVEC			mfv_avbuf[AKFS_ADATA_SIZE];
	AKFVEC			mfv_ao;
	AKFVEC			mfv_as;

	/* Variables for Direction. */
	int16			mi_hnaveD;
	int16			mi_anaveD;
	AKFLOAT			mf_azimuth;
	AKFLOAT			mf_pitch;
	AKFLOAT			mf_roll;

	/* Variables for vector output */
	int16			mi_hnaveV;
	int16			mi_anaveV;
	AKFVEC			mfv_hvec;
	AKFVEC			mfv_avec;
	int16			mi_hstatus;

} AK8975PRMS;

#endif

