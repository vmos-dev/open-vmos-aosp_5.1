/******************************************************************************
 * $Id: AKFS_Measure.c 580 2012-03-29 09:56:21Z yamada.rj $
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
#ifdef WIN32
#include "AK8975_LinuxDriver.h"
#else
#include "AK8975Driver.h"
#endif

#include "AKFS_Measure.h"
#include "AKFS_Disp.h"
#include "AKFS_APIs.h"

/*!
  Read sensitivity adjustment data from fuse ROM.
  @return If data are read successfully, the return value is #AKM_SUCCESS.
   Otherwise the return value is #AKM_FAIL.
  @param[out] regs The read ASA values. When this function succeeds, ASAX value
   is saved in regs[0], ASAY is saved in regs[1], ASAZ is saved in regs[2].
 */
int16 AKFS_ReadAK8975FUSEROM(
		uint8	regs[3]
)
{
	/* Set to FUSE ROM access mode */
	if (AKD_SetMode(AK8975_MODE_FUSE_ACCESS) != AKD_SUCCESS) {
		AKMERROR;
		return AKM_FAIL;
	}    

	/* Read values. ASAX, ASAY, ASAZ */
	if (AKD_RxData(AK8975_FUSE_ASAX, regs, 3) != AKD_SUCCESS) {
		AKMERROR;
		return AKM_FAIL;
	}    

	/* Set to PowerDown mode */
	if (AKD_SetMode(AK8975_MODE_POWERDOWN) != AKD_SUCCESS) {
		AKMERROR;
		return AKM_FAIL;
	}    

	AKMDEBUG(DBG_LEVEL2, "%s: asa(dec)=%d,%d,%d\n",
			__FUNCTION__, regs[0], regs[1], regs[2]);

	return AKM_SUCCESS;
}

/*!
  Carry out self-test.
  @return If this function succeeds, the return value is #AKM_SUCCESS.
   Otherwise the return value is #AKM_FAIL.
 */
int16 AKFS_SelfTest(void)
{
	BYTE	i2cData[SENSOR_DATA_SIZE];
	BYTE	asa[3];
	AKFLOAT	hdata[3];
	int16	ret;

	/* Set to FUSE ROM access mode */
	if (AKD_SetMode(AK8975_MODE_FUSE_ACCESS) != AKD_SUCCESS) {
		AKMERROR;
		return AKM_FAIL;
	}

	/* Read values from ASAX to ASAZ */
	if (AKD_RxData(AK8975_FUSE_ASAX, asa, 3) != AKD_SUCCESS) {
		AKMERROR;
		return AKM_FAIL;
	}

	/* Set to PowerDown mode */
	if (AKD_SetMode(AK8975_MODE_POWERDOWN) != AKD_SUCCESS) {
		AKMERROR;
		return AKM_FAIL;
	}

	/* Set to self-test mode */
	i2cData[0] = 0x40;
	if (AKD_TxData(AK8975_REG_ASTC, i2cData, 1) != AKD_SUCCESS) {
		AKMERROR;
		return AKM_FAIL;
	}

	/* Set to Self-test mode */
	if (AKD_SetMode(AK8975_MODE_SELF_TEST) != AKD_SUCCESS) {
		AKMERROR;
		return AKM_FAIL;
	}

	/*
	   Wait for DRDY pin changes to HIGH.
	   Get measurement data from AK8975
	 */
	if (AKD_GetMagneticData(i2cData) != AKD_SUCCESS) {
		AKMERROR;
		return AKM_FAIL;
	}

	hdata[0] = AK8975_HDATA_CONVERTER(i2cData[2], i2cData[1], asa[0]);
	hdata[1] = AK8975_HDATA_CONVERTER(i2cData[4], i2cData[3], asa[1]);
	hdata[2] = AK8975_HDATA_CONVERTER(i2cData[6], i2cData[5], asa[2]);

	/* Test */
	ret = 1;
	if ((hdata[0] < AK8975_SELFTEST_MIN_X) ||
		(AK8975_SELFTEST_MAX_X < hdata[0])) {
		ret = 0;
	}
	if ((hdata[1] < AK8975_SELFTEST_MIN_Y) ||
		(AK8975_SELFTEST_MAX_Y < hdata[1])) {
		ret = 0;
	}
	if ((hdata[2] < AK8975_SELFTEST_MIN_Z) ||
		(AK8975_SELFTEST_MAX_Z < hdata[2])) {
		ret = 0;
	}

	AKMDEBUG(DBG_LEVEL2, "Test(%s):%8.2f, %8.2f, %8.2f\n",
		(ret ? "Success" : "fail"), hdata[0], hdata[1], hdata[2]);

	if (ret) {
		return AKM_SUCCESS;
	} else {
		return AKM_FAIL;
	}
}

/*!
  This function calculate the duration of sleep for maintaining
   the loop keep the period.
  This function calculates "minimum - (end - start)".
  @return The result of above equation in nanosecond.
  @param end The time of after execution.
  @param start The time of before execution.
  @param minimum Loop period of each execution.
 */
struct timespec AKFS_CalcSleep(
	const struct timespec* end,
	const struct timespec* start,
	const int64_t minimum
)
{
	int64_t endL;
	int64_t startL;
	int64_t diff;

	struct timespec ret;

	endL = (end->tv_sec * 1000000000) + end->tv_nsec;
	startL = (start->tv_sec * 1000000000) + start->tv_nsec;
	diff = minimum;

	diff -= (endL - startL);

	/* Don't allow negative value */
	if (diff < 0) {
		diff = 0;
	}

	/* Convert to timespec */
	if (diff > 1000000000) {
	ret.tv_sec = diff / 1000000000;
		ret.tv_nsec = diff % 1000000000;
	} else {
		ret.tv_sec = 0;
		ret.tv_nsec = diff;
	}
	return ret;
}

/*!
  Get interval of each sensors from device driver.
  @return If this function succeeds, the return value is #AKM_SUCCESS.
   Otherwise the return value is #AKM_FAIL.
  @param flag This variable indicates what sensor frequency is updated.
  @param minimum This value show the minimum loop period in all sensors.
 */
int16 AKFS_GetInterval(
		uint16*  flag,
		int64_t* minimum
)
{
	/* Accelerometer, Magnetometer, Orientation */
	/* Delay is in nano second unit. */
	/* Negative value means the sensor is disabled.*/
	int64_t delay[AKM_NUM_SENSORS];
	int i;

	if (AKD_GetDelay(delay) < 0) {
		AKMERROR;
		return AKM_FAIL;
	}
	AKMDATA(AKMDATA_GETINTERVAL,"delay[A,M,O]=%lld,%lld,%lld\n",
		delay[0], delay[1], delay[2]);

	/* update */
	*minimum = 1000000000;
	*flag = 0;
	for (i=0; i<AKM_NUM_SENSORS; i++) {
		/* Set flag */
		if (delay[i] >= 0) {
			*flag |= 1 << i;
			if (*minimum > delay[i]) {
				*minimum = delay[i];
			}
		}
	}
	return AKM_SUCCESS;
}

/*!
  If this program run as console mode, measurement result will be displayed
   on console terminal.
  @return If this function succeeds, the return value is #AKM_SUCCESS.
   Otherwise the return value is #AKM_FAIL.
 */
void AKFS_OutputResult(
	const	uint16			flag,
	const	AKSENSOR_DATA*	acc,
	const	AKSENSOR_DATA*	mag,
	const	AKSENSOR_DATA*	ori
)
{
	int buf[YPR_DATA_SIZE];

	/* Store to buffer */
	buf[0] = flag;					/* Data flag */
	buf[1] = CONVERT_ACC(acc->x);	/* Ax */
	buf[2] = CONVERT_ACC(acc->y);	/* Ay */
	buf[3] = CONVERT_ACC(acc->z);	/* Az */
	buf[4] = acc->status;			/* Acc status */
	buf[5] = CONVERT_MAG(mag->x);	/* Mx */
	buf[6] = CONVERT_MAG(mag->y);	/* My */
	buf[7] = CONVERT_MAG(mag->z);	/* Mz */
	buf[8] = mag->status;			/* Mag status */
	buf[9] = CONVERT_ORI(ori->x);	/* yaw */
	buf[10] = CONVERT_ORI(ori->y);	/* pitch */
	buf[11] = CONVERT_ORI(ori->z);	/* roll */

	if (g_opmode & OPMODE_CONSOLE) {
		/* Console mode */
		Disp_Result(buf);
	}

	/* Set result to driver */
		AKD_SetYPR(buf);
}

/*!
 This is the main routine of measurement.
 */
void AKFS_MeasureLoop(void)
{
	BYTE    i2cData[SENSOR_DATA_SIZE]; /* ST1 ~ ST2 */
	int16	mag[3];
	int16	mstat;
	int16	acc[3];
	struct	timespec tsstart= {0, 0};
	struct	timespec tsend = {0, 0};
	struct	timespec doze;
	int64_t	minimum;
	uint16	flag;
	AKSENSOR_DATA sv_acc;
	AKSENSOR_DATA sv_mag;
	AKSENSOR_DATA sv_ori;
	AKFLOAT tmpx, tmpy, tmpz;
	int16 tmp_accuracy;

	minimum = -1;

#ifdef WIN32
	clock_init_time();
#endif

	/* Initialize library functions and device */
	if (AKFS_Start(CSPEC_SETTING_FILE) != AKM_SUCCESS) {
		AKMERROR;
		goto MEASURE_END;
	}

	while (g_stopRequest != AKM_TRUE) {
		/* Beginning time */
		if (clock_gettime(CLOCK_MONOTONIC, &tsstart) < 0) {
			AKMERROR;
			goto MEASURE_END;
		}

		/* Get interval */
		if (AKFS_GetInterval(&flag, &minimum) != AKM_SUCCESS) {
			AKMERROR;
			goto MEASURE_END;
		}

		if ((flag & ACC_DATA_READY) || (flag & ORI_DATA_READY)) {
			/* Get accelerometer */
			if (AKD_GetAccelerationData(acc) != AKD_SUCCESS) {
				AKMERROR;
				goto MEASURE_END;
			}

			/* Calculate accelerometer vector */
			if (AKFS_Get_ACCELEROMETER(acc, 0, &tmpx, &tmpy, &tmpz, &tmp_accuracy) == AKM_SUCCESS) {
				sv_acc.x = tmpx;
				sv_acc.y = tmpy;
				sv_acc.z = tmpz;
				sv_acc.status = tmp_accuracy;
			} else {
				flag &= ~ACC_DATA_READY;
				flag &= ~ORI_DATA_READY;
			}
		}

		if ((flag & MAG_DATA_READY) || (flag & ORI_DATA_READY)) {
			/* Set to measurement mode  */
			if (AKD_SetMode(AK8975_MODE_SNG_MEASURE) != AKD_SUCCESS) {
				AKMERROR;
				goto MEASURE_END;
			}

			/* Wait for DRDY and get data from device */
			if (AKD_GetMagneticData(i2cData) != AKD_SUCCESS) {
				AKMERROR;
				goto MEASURE_END;
			}
			/* raw data to x,y,z value */
			mag[0] = (int)((int16_t)(i2cData[2]<<8)+((int16_t)i2cData[1]));
			mag[1] = (int)((int16_t)(i2cData[4]<<8)+((int16_t)i2cData[3]));
			mag[2] = (int)((int16_t)(i2cData[6]<<8)+((int16_t)i2cData[5]));
			mstat = i2cData[0] | i2cData[7];

			AKMDATA(AKMDATA_BDATA,
				"bData=%02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X\n",
				i2cData[0], i2cData[1], i2cData[2], i2cData[3],
				i2cData[4], i2cData[5], i2cData[6], i2cData[7]);

			/* Calculate magnetic field vector */
			if (AKFS_Get_MAGNETIC_FIELD(mag, mstat, &tmpx, &tmpy, &tmpz, &tmp_accuracy) == AKM_SUCCESS) {
				sv_mag.x = tmpx;
				sv_mag.y = tmpy;
				sv_mag.z = tmpz;
				sv_mag.status = tmp_accuracy;
			} else {
				flag &= ~MAG_DATA_READY;
				flag &= ~ORI_DATA_READY;
			}
		}

		if (flag & ORI_DATA_READY) {
			if (AKFS_Get_ORIENTATION(&tmpx, &tmpy, &tmpz, &tmp_accuracy) == AKM_SUCCESS) {
				sv_ori.x = tmpx;
				sv_ori.y = tmpy;
				sv_ori.z = tmpz;
				sv_ori.status = tmp_accuracy;
			} else {
				flag &= ~ORI_DATA_READY;
			}
		}

		/* Output result */
		AKFS_OutputResult(flag, &sv_acc, &sv_mag, &sv_ori);

		/* Ending time */
		if (clock_gettime(CLOCK_MONOTONIC, &tsend) < 0) {
			AKMERROR;
			goto MEASURE_END;
		}

		/* Calculate duration */
		doze = AKFS_CalcSleep(&tsend, &tsstart, minimum);
		AKMDATA(AKMDATA_LOOP, "Sleep: %6.2f msec\n", (doze.tv_nsec/1000000.0f));
		nanosleep(&doze, NULL);

#ifdef WIN32
		if (_kbhit()) {
			_getch();
			break;
		}
#endif
	}

MEASURE_END:
	/* Set to PowerDown mode */
	if (AKD_SetMode(AK8975_MODE_POWERDOWN) != AKD_SUCCESS) {
		AKMERROR;
		return;
	}

	/* Save parameters */
	if (AKFS_Stop(CSPEC_SETTING_FILE) != AKM_SUCCESS) {
		AKMERROR;
	}
}


