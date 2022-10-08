/******************************************************************************
 * $Id: AK8975Driver.h 580 2012-03-29 09:56:21Z yamada.rj $
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
#ifndef AKMD_INC_AK8975DRIVER_H
#define AKMD_INC_AK8975DRIVER_H

#include "platform/include/linux/akm8975.h"	/* Device driver */
#include <stdint.h>			/* int8_t, int16_t etc. */

/*** Constant definition ******************************************************/
#define AKD_TRUE	1		/*!< Represents true */
#define AKD_FALSE	0		/*!< Represents false */
#define AKD_SUCCESS	1		/*!< Represents success.*/
#define AKD_FAIL	0		/*!< Represents fail. */
#define AKD_ERROR	-1		/*!< Represents error. */

/*! 0:Don't Output data, 1:Output data */
#define AKD_DBG_DATA	0
/*! Typical interval in ns */
#define AK8975_MEASUREMENT_TIME_NS	((AK8975_MEASUREMENT_TIME_US) * 1000)
/*! 720 LSG = 1G = 9.8 m/s2 */
#define LSG			720


/*** Type declaration *********************************************************/
typedef unsigned char BYTE;

/*!
 Open device driver.
 This function opens device driver of acceleration sensor.
 @return If this function succeeds, the return value is #AKD_SUCCESS. Otherwise
 the return value is #AKD_FAIL.
 */
typedef int16_t(*ACCFNC_INITDEVICE)(void);

/*!
 Close device driver.
 This function closes device drivers of acceleration sensor.
 */
typedef void(*ACCFNC_DEINITDEVICE)(void);

/*!
 Acquire acceleration data from acceleration sensor and convert it to Android
 coordinate system.
 @return If this function succeeds, the return value is #AKD_SUCCESS. Otherwise
 the return value is #AKD_FAIL.
 @param[out] data A acceleration data array. The coordinate system of the
 acquired data follows the definition of Android. Unit is SmartCompass.
 */
typedef int16_t(*ACCFNC_GETACCDATA)(short data[3]);


/*** Global variables *********************************************************/

/*** Prototype of Function  ***************************************************/

int16_t AKD_InitDevice(void);

void AKD_DeinitDevice(void);

int16_t AKD_TxData(
	const BYTE address,
	const BYTE* data,
	const uint16_t numberOfBytesToWrite);

int16_t AKD_RxData(
	const BYTE address,
	BYTE* data,
	const uint16_t numberOfBytesToRead);

int16_t AKD_GetMagneticData(BYTE data[SENSOR_DATA_SIZE]);

void AKD_SetYPR(const int buf[YPR_DATA_SIZE]);

int AKD_GetOpenStatus(int* status);

int AKD_GetCloseStatus(int* status);

int16_t AKD_SetMode(const BYTE mode);

int16_t AKD_GetDelay(int64_t delay[AKM_NUM_SENSORS]);

int16_t AKD_GetLayout(int16_t* layout);

int16_t AKD_GetAccelerationData(int16_t data[3]);

#endif /* AKMD_INC_AK8975DRIVER_H */

