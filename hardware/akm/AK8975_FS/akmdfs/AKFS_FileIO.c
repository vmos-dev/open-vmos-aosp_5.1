/******************************************************************************
 * $Id: AKFS_FileIO.c 580 2012-03-29 09:56:21Z yamada.rj $
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
#include "AKFS_FileIO.h"

/*** Constant definition ******************************************************/
#ifdef AKFS_PRECISION_DOUBLE
#define AKFS_SCANF_FORMAT	"%63s = %lf"
#else
#define AKFS_SCANF_FORMAT	"%63s = %f"
#endif
#define AKFS_PRINTF_FORMAT	"%s = %f\n"
#define LOAD_BUF_SIZE	64

/*!
 Load parameters from file which is specified with #path.  This function reads 
  data from a beginning of the file line by line, and check parameter name 
  sequentially. In otherword, this function depends on the order of eache 
  parameter described in the file.
 @return If function fails, the return value is #AKM_FAIL. When function fails,
  the output is undefined. Therefore, parameters which are possibly overwritten
  by this function should be initialized again. If function succeeds, the
  return value is #AKM_SUCCESS.
 @param[out] prms A pointer to #AK8975PRMS structure. Loaded parameter is
  stored to the member of this structure.
 @param[in] path A path to the setting file.
 */
int16 AKFS_LoadParameters(AK8975PRMS * prms, const char* path)
{
	int16 ret;
	char buf[LOAD_BUF_SIZE];
	FILE *fp = NULL;

	/* Open setting file for read. */
	if ((fp = fopen(path, "r")) == NULL) {
		AKMERROR_STR("fopen");
		return AKM_FAIL;
	}

	ret = 1;

	/* Load data to HO */
	if (fscanf(fp, AKFS_SCANF_FORMAT, buf, &prms->mfv_ho.u.x) != 2) {
		ret = 0;
	} else {
		if (strncmp(buf, "HO.x", sizeof(buf)) != 0) {
			ret = 0;
		}
	}
	if (fscanf(fp, AKFS_SCANF_FORMAT, buf, &prms->mfv_ho.u.y) != 2) {
		ret = 0;
	} else {
		if (strncmp(buf, "HO.y", sizeof(buf)) != 0) {
			ret = 0;
		}
	}
	if (fscanf(fp, AKFS_SCANF_FORMAT, buf, &prms->mfv_ho.u.z) != 2) {
		ret = 0;
	} else {
		if (strncmp(buf, "HO.z", sizeof(buf)) != 0) {
			ret = 0;
		}
	}

	if (fclose(fp) != 0) {
		AKMERROR_STR("fclose");
		ret = 0;
	}

	if (ret == 0) {
		AKMERROR;
		return AKM_FAIL;
	}

	return AKM_SUCCESS;
}

/*!
 Save parameters to file which is specified with #path. This function saves 
  variables when the offsets of magnetic sensor estimated successfully.
 @return If function fails, the return value is #AKM_FAIL. When function fails,
  the parameter file may collapsed. Therefore, the parameters file should be
  discarded. If function succeeds, the return value is #AKM_SUCCESS.
 @param[out] prms A pointer to #AK8975PRMS structure. Member variables are
  saved to the parameter file.
 @param[in] path A path to the setting file.
 */
int16 AKFS_SaveParameters(AK8975PRMS *prms, const char* path)
{
	int16 ret = 1;
	FILE *fp;

	/*Open setting file for write. */
	if ((fp = fopen(path, "w")) == NULL) {
		AKMERROR_STR("fopen");
		return AKM_FAIL;
	}

	/* Save data to HO */
	if (fprintf(fp, AKFS_PRINTF_FORMAT, "HO.x", prms->mfv_ho.u.x) < 0) { ret = 0; }
	if (fprintf(fp, AKFS_PRINTF_FORMAT, "HO.y", prms->mfv_ho.u.y) < 0) { ret = 0; }
	if (fprintf(fp, AKFS_PRINTF_FORMAT, "HO.z", prms->mfv_ho.u.z) < 0) { ret = 0; }

	if (fclose(fp) != 0) {
		AKMERROR_STR("fclose");
		ret = 0;
	}

	if (ret == 0) {
		AKMERROR;
		return AKM_FAIL;
	}

	return AKM_SUCCESS;
}

