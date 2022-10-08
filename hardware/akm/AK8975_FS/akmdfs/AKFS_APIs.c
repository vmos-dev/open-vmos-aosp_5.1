/******************************************************************************
 * $Id: AKFS_APIs.c 580 2012-03-29 09:56:21Z yamada.rj $
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
#include "AKFS_Common.h"
#include "AKFS_Disp.h"
#include "AKFS_FileIO.h"
#include "AKFS_APIs.h"

#ifdef WIN32
#include "AK8975_LinuxDriver.h"
#endif

static AK8975PRMS g_prms;

/*!
  Initialize library. At first, 0 is set to all parameters.  After that, some
  parameters, which should not be 0, are set to specific value. Some of initial
  values can be customized by editing the file \c "AKFS_CSpec.h".
  @return The return value is #AKM_SUCCESS.
  @param[in] hpat Specify a layout pattern number.  The number is determined
  according to the mount orientation of the magnetometer.
  @param[in] regs[3] Specify the ASA values which are read out from
  fuse ROM.  regs[0] is ASAX, regs[1] is ASAY, regs[2] is ASAZ.
 */
int16 AKFS_Init(
	const	AKFS_PATNO	hpat,
	const	uint8		regs[]
)
{
	AKMDATA(AKMDATA_DUMP, "%s: hpat=%d, r[0]=0x%02X, r[1]=0x%02X, r[2]=0x%02X\n",
		__FUNCTION__, hpat, regs[0], regs[1], regs[2]);

	/* Set 0 to the AK8975 structure. */
	memset(&g_prms, 0, sizeof(AK8975PRMS));

	/* Sensitivity */
	g_prms.mfv_hs.u.x = AK8975_HSENSE_DEFAULT;
	g_prms.mfv_hs.u.y = AK8975_HSENSE_DEFAULT;
	g_prms.mfv_hs.u.z = AK8975_HSENSE_DEFAULT;
	g_prms.mfv_as.u.x = AK8975_ASENSE_DEFAULT;
	g_prms.mfv_as.u.y = AK8975_ASENSE_DEFAULT;
	g_prms.mfv_as.u.z = AK8975_ASENSE_DEFAULT;

	/* Initialize variables that initial value is not 0. */
	g_prms.mi_hnaveV = CSPEC_HNAVE_V;
	g_prms.mi_hnaveD = CSPEC_HNAVE_D;
	g_prms.mi_anaveV = CSPEC_ANAVE_V;
	g_prms.mi_anaveD = CSPEC_ANAVE_D;

	/* Copy ASA values */
	g_prms.mi_asa.u.x = regs[0];
	g_prms.mi_asa.u.y = regs[1];
	g_prms.mi_asa.u.z = regs[2];

	/* Copy layout pattern */
	g_prms.m_hpat = hpat;

	return AKM_SUCCESS;
}

/*!
  Release resources. This function is for future expansion.
  @return The return value is #AKM_SUCCESS.
 */
int16 AKFS_Release(void)
{
	return AKM_SUCCESS;
}

/*
  This function is called just before a measurement sequence starts.
  This function reads parameters from file, then initializes algorithm
  parameters.
  @return The return value is #AKM_SUCCESS.
  @param[in] path Specify a path to the settings file.
 */
int16 AKFS_Start(
	const char* path
)
{
	AKMDATA(AKMDATA_DUMP, "%s: path=%s\n", __FUNCTION__, path);

	/* Read setting files from a file */
	if (AKFS_LoadParameters(&g_prms, path) != AKM_SUCCESS) {
		AKMERROR_STR("AKFS_Load");
	}

	/* Initialize buffer */
	AKFS_InitBuffer(AKFS_HDATA_SIZE, g_prms.mfv_hdata);
	AKFS_InitBuffer(AKFS_HDATA_SIZE, g_prms.mfv_hvbuf);
	AKFS_InitBuffer(AKFS_ADATA_SIZE, g_prms.mfv_adata);
	AKFS_InitBuffer(AKFS_ADATA_SIZE, g_prms.mfv_avbuf);

	/* Initialize for AOC */
	AKFS_InitAOC(&g_prms.m_aocv);
	/* Initialize magnetic status */
	g_prms.mi_hstatus = 0;

	return AKM_SUCCESS;
}

/*!
  This function is called when a measurement sequence is done.
  This fucntion writes parameters to file.
  @return The return value is #AKM_SUCCESS.
  @param[in] path Specify a path to the settings file.
 */
int16 AKFS_Stop(
	const char* path
)
{
	AKMDATA(AKMDATA_DUMP, "%s: path=%s\n", __FUNCTION__, path);

	/* Write setting files to a file */
	if (AKFS_SaveParameters(&g_prms, path) != AKM_SUCCESS) {
		AKMERROR_STR("AKFS_Save");
	}

	return AKM_SUCCESS;
}

/*!
  This function is called when new magnetometer data is available.  The output
  vector format and coordination system follow the Android definition.
  @return The return value is #AKM_SUCCESS.
   Otherwise the return value is #AKM_FAIL.
  @param[in] mag A set of measurement data from magnetometer.  X axis value
   should be in mag[0], Y axis value should be in mag[1], Z axis value should be 
   in mag[2].
  @param[in] status A status of magnetometer.  This status indicates the result
   of measurement data, i.e. overflow, success or fail, etc.
  @param[out] vx X axis value of magnetic field vector.
  @param[out] vy Y axis value of magnetic field vector.
  @param[out] vz Z axis value of magnetic field vector.
  @param[out] accuracy Accuracy of magnetic field vector.
 */
int16 AKFS_Get_MAGNETIC_FIELD(
	const	int16		mag[3],
	const	int16		status,
			AKFLOAT*	vx,
			AKFLOAT*	vy,
			AKFLOAT*	vz,
			int16*		accuracy
)
{
	int16 akret;
	int16 aocret;
	AKFLOAT radius;

	AKMDATA(AKMDATA_DUMP, "%s: m[0]=%d, m[1]=%d, m[2]=%d, st=%d\n",
		__FUNCTION__, mag[0], mag[1], mag[2], status);

	/* Decomposition */
	/* Sensitivity adjustment, i.e. multiply ASA, is done in this function. */
	akret = AKFS_DecompAK8975(
		mag,
		status,
		&g_prms.mi_asa,
		AKFS_HDATA_SIZE,
		g_prms.mfv_hdata
	);
	if(akret == AKFS_ERROR) {
		AKMERROR;
		return AKM_FAIL;
	}

	/* Adjust coordination */
	akret = AKFS_Rotate(
		g_prms.m_hpat,
		&g_prms.mfv_hdata[0]
	);
	if (akret == AKFS_ERROR) {
		AKMERROR;
		return AKM_FAIL;
	}

	/* AOC for magnetometer */
	/* Offset estimation is done in this function */
	aocret = AKFS_AOC(
		&g_prms.m_aocv,
		g_prms.mfv_hdata,
		&g_prms.mfv_ho
	);

	/* Subtract offset */
	/* Then, a magnetic vector, the unit is uT, is stored in mfv_hvbuf. */
	akret = AKFS_VbNorm(
		AKFS_HDATA_SIZE,
		g_prms.mfv_hdata,
		1,
		&g_prms.mfv_ho,
		&g_prms.mfv_hs,
		AK8975_HSENSE_TARGET,
		AKFS_HDATA_SIZE,
		g_prms.mfv_hvbuf
	);
	if(akret == AKFS_ERROR) {
		AKMERROR;
		return AKM_FAIL;
	}

	/* Averaging */
	akret = AKFS_VbAve(
		AKFS_HDATA_SIZE,
		g_prms.mfv_hvbuf,
		CSPEC_HNAVE_V,
		&g_prms.mfv_hvec
	);
	if (akret == AKFS_ERROR) {
		AKMERROR;
		return AKM_FAIL;
	}

	/* Check the size of magnetic vector */
	radius = AKFS_SQRT(
			(g_prms.mfv_hvec.u.x * g_prms.mfv_hvec.u.x) +
			(g_prms.mfv_hvec.u.y * g_prms.mfv_hvec.u.y) +
			(g_prms.mfv_hvec.u.z * g_prms.mfv_hvec.u.z));

	if (radius > AKFS_GEOMAG_MAX) {
		g_prms.mi_hstatus = 0;
	} else {
		if (aocret) {
			g_prms.mi_hstatus = 3;
		}
	}

	*vx = g_prms.mfv_hvec.u.x;
	*vy = g_prms.mfv_hvec.u.y;
	*vz = g_prms.mfv_hvec.u.z;
	*accuracy = g_prms.mi_hstatus;

	/* Debug output */
	AKMDATA(AKMDATA_MAG, "Mag(%d):%8.2f, %8.2f, %8.2f\n",
			*accuracy, *vx, *vy, *vz);

	return AKM_SUCCESS;
}

/*!
  This function is called when new accelerometer data is available.  The output
  vector format and coordination system follow the Android definition.
  @return The return value is #AKM_SUCCESS when function succeeds. Otherwise
   the return value is #AKM_FAIL.
  @param[in] acc A set of measurement data from accelerometer.  X axis value
   should be in acc[0], Y axis value should be in acc[1], Z axis value should be 
   in acc[2].
  @param[in] status A status of accelerometer.  This status indicates the result
   of acceleration data, i.e. overflow, success or fail, etc.
  @param[out] vx X axis value of acceleration vector.
  @param[out] vy Y axis value of acceleration vector.
  @param[out] vz Z axis value of acceleration vector.
  @param[out] accuracy Accuracy of acceleration vector.
  This value is always 3.
 */
int16 AKFS_Get_ACCELEROMETER(
	const   int16		acc[3],
	const	int16		status,
			AKFLOAT*	vx,
			AKFLOAT*	vy,
			AKFLOAT*	vz,
			int16*		accuracy
)
{
	int16 akret;

	AKMDATA(AKMDATA_DUMP, "%s: a[0]=%d, a[1]=%d, a[2]=%d, st=%d\n",
		__FUNCTION__, acc[0], acc[1], acc[2], status);

	/* Save data to buffer */
	AKFS_BufShift(
		AKFS_ADATA_SIZE,
		1,
		g_prms.mfv_adata
	);
	g_prms.mfv_adata[0].u.x = acc[0];
	g_prms.mfv_adata[0].u.y = acc[1];
	g_prms.mfv_adata[0].u.z = acc[2];

	/* Subtract offset, adjust sensitivity */
	/* As a result, a unit of acceleration data in mfv_avbuf is '1G = 9.8' */
	akret = AKFS_VbNorm(
		AKFS_ADATA_SIZE,
		g_prms.mfv_adata,
		1,
		&g_prms.mfv_ao,
		&g_prms.mfv_as,
		AK8975_ASENSE_TARGET,
		AKFS_ADATA_SIZE,
		g_prms.mfv_avbuf
	);
	if(akret == AKFS_ERROR) {
		AKMERROR;
		return AKM_FAIL;
	}

	/* Averaging */
	akret = AKFS_VbAve(
		AKFS_ADATA_SIZE,
		g_prms.mfv_avbuf,
		CSPEC_ANAVE_V,
		&g_prms.mfv_avec
	);
	if (akret == AKFS_ERROR) {
		AKMERROR;
		return AKM_FAIL;
	}

	/* Adjust coordination */
	/* It is not needed. Because, the data from AK8975 driver is already
	   follows Android coordinate system. */

	*vx = g_prms.mfv_avec.u.x;
	*vy = g_prms.mfv_avec.u.y;
	*vz = g_prms.mfv_avec.u.z;
	*accuracy = 3;

	/* Debug output */
	AKMDATA(AKMDATA_ACC, "Acc(%d):%8.2f, %8.2f, %8.2f\n",
			*accuracy, *vx, *vy, *vz);

	return AKM_SUCCESS;
}

/*!
  Get orientation sensor's elements. The vector format and coordination system
   follow the Android definition.  Before this function is called, magnetic
   field vector and acceleration vector should be stored in the buffer by 
   calling #AKFS_Get_MAGNETIC_FIELD and #AKFS_Get_ACCELEROMETER.
  @return The return value is #AKM_SUCCESS when function succeeds. Otherwise
   the return value is #AKM_FAIL.
  @param[out] azimuth Azimuthal angle in degree.
  @param[out] pitch Pitch angle in degree.
  @param[out] roll Roll angle in degree.
  @param[out] accuracy Accuracy of orientation sensor.
 */
int16 AKFS_Get_ORIENTATION(
			AKFLOAT*	azimuth,
			AKFLOAT*	pitch,
			AKFLOAT*	roll,
			int16*		accuracy
)
{
	int16 akret;

	/* Azimuth calculation */
	/* Coordination system follows the Android coordination. */
	akret = AKFS_Direction(
		AKFS_HDATA_SIZE,
		g_prms.mfv_hvbuf,
		CSPEC_HNAVE_D,
		AKFS_ADATA_SIZE,
		g_prms.mfv_avbuf,
		CSPEC_ANAVE_D,
		&g_prms.mf_azimuth,
		&g_prms.mf_pitch,
		&g_prms.mf_roll
	);

	if(akret == AKFS_ERROR) {
		AKMERROR;
		return AKM_FAIL;
	}
	*azimuth  = g_prms.mf_azimuth;
	*pitch    = g_prms.mf_pitch;
	*roll     = g_prms.mf_roll;
	*accuracy = g_prms.mi_hstatus;

	/* Debug output */
	AKMDATA(AKMDATA_ORI, "Ori(%d):%8.2f, %8.2f, %8.2f\n",
			*accuracy, *azimuth, *pitch, *roll);

	return AKM_SUCCESS;
}

