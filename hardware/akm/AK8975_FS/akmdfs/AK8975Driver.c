/******************************************************************************
 * $Id: AK8975Driver.c 580 2012-03-29 09:56:21Z yamada.rj $
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
#include <fcntl.h>
#include "AKFS_Common.h"
#include "AK8975Driver.h"

#define MSENSOR_NAME		"/dev/akm8975_dev"

static int s_fdDev = -1;

/*!
 Open device driver.
 This function opens both device drivers of magnetic sensor and acceleration
 sensor. Additionally, some initial hardware settings are done, such as
 measurement range, built-in filter function and etc.
 @return If this function succeeds, the return value is #AKD_SUCCESS.
 Otherwise the return value is #AKD_FAIL.
 */
int16_t AKD_InitDevice(void)
{
	if (s_fdDev < 0) {
		/* Open magnetic sensor's device driver. */
		if ((s_fdDev = open(MSENSOR_NAME, O_RDWR)) < 0) {
			AKMERROR_STR("open");
			return AKD_FAIL;
		}
	}

	return AKD_SUCCESS;
}

/*!
 Close device driver.
 This function closes both device drivers of magnetic sensor and acceleration
 sensor.
 */
void AKD_DeinitDevice(void)
{
	if (s_fdDev >= 0) {
		close(s_fdDev);
		s_fdDev = -1;
	}
}

/*!
 Writes data to a register of the AK8975.  When more than one byte of data is
 specified, the data is written in contiguous locations starting at an address
 specified in \a address.
 @return If this function succeeds, the return value is #AKD_SUCCESS. Otherwise
 the return value is #AKD_FAIL.
 @param[in] address Specify the address of a register in which data is to be
 written.
 @param[in] data Specify data to write or a pointer to a data array containing
 the data.  When specifying more than one byte of data, specify the starting
 address of the array.
 @param[in] numberOfBytesToWrite Specify the number of bytes that make up the
 data to write.  When a pointer to an array is specified in data, this argument
 equals the number of elements of the array.
 */
int16_t AKD_TxData(
				const BYTE address,
				const BYTE * data,
				const uint16_t numberOfBytesToWrite)
{
	int i;
	char buf[RWBUF_SIZE];

	if (s_fdDev < 0) {
		ALOGE("%s: Device file is not opened.", __FUNCTION__);
		return AKD_FAIL;
	}
	if (numberOfBytesToWrite > (RWBUF_SIZE-2)) {
		ALOGE("%s: Tx size is too large.", __FUNCTION__);
		return AKD_FAIL;
	}

	buf[0] = numberOfBytesToWrite + 1;
	buf[1] = address;

	for (i = 0; i < numberOfBytesToWrite; i++) {
		buf[i + 2] = data[i];
	}
	if (ioctl(s_fdDev, ECS_IOCTL_WRITE, buf) < 0) {
		AKMERROR_STR("ioctl");
		return AKD_FAIL;
	} else {

#if ENABLE_AKMDEBUG
		AKMDATA(AKMDATA_DRV, "addr(HEX)=%02x data(HEX)=", address);
		for (i = 0; i < numberOfBytesToWrite; i++) {
			AKMDATA(AKMDATA_DRV, " %02x", data[i]);
		}
		AKMDATA(AKMDATA_DRV, "\n");
#endif
		return AKD_SUCCESS;
	}
}

/*!
 Acquires data from a register or the EEPROM of the AK8975.
 @return If this function succeeds, the return value is #AKD_SUCCESS. Otherwise
 the return value is #AKD_FAIL.
 @param[in] address Specify the address of a register from which data is to be
 read.
 @param[out] data Specify a pointer to a data array which the read data are
 stored.
 @param[in] numberOfBytesToRead Specify the number of bytes that make up the
 data to read.  When a pointer to an array is specified in data, this argument
 equals the number of elements of the array.
 */
int16_t AKD_RxData(
				const BYTE address,
				BYTE * data,
				const uint16_t numberOfBytesToRead)
{
	int i;
	char buf[RWBUF_SIZE];

	memset(data, 0, numberOfBytesToRead);

	if (s_fdDev < 0) {
		ALOGE("%s: Device file is not opened.", __FUNCTION__);
		return AKD_FAIL;
	}
	if (numberOfBytesToRead > (RWBUF_SIZE-1)) {
		ALOGE("%s: Rx size is too large.", __FUNCTION__);
		return AKD_FAIL;
	}

	buf[0] = numberOfBytesToRead;
	buf[1] = address;

	if (ioctl(s_fdDev, ECS_IOCTL_READ, buf) < 0) {
		AKMERROR_STR("ioctl");
		return AKD_FAIL;
	} else {
		for (i = 0; i < numberOfBytesToRead; i++) {
			data[i] = buf[i + 1];
		}
#if ENABLE_AKMDEBUG
		AKMDATA(AKMDATA_DRV, "addr(HEX)=%02x len=%d data(HEX)=",
				address, numberOfBytesToRead);
		for (i = 0; i < numberOfBytesToRead; i++) {
			AKMDATA(AKMDATA_DRV, " %02x", data[i]);
		}
		AKMDATA(AKMDATA_DRV, "\n");
#endif
		return AKD_SUCCESS;
	}
}

/*!
 Acquire magnetic data from AK8975. If measurement is not done, this function
 waits until measurement completion.
 @return If this function succeeds, the return value is #AKD_SUCCESS. Otherwise
 the return value is #AKD_FAIL.
 @param[out] data A magnetic data array. The size should be larger than #SENSOR_DATA_SIZE.
 */
int16_t AKD_GetMagneticData(BYTE data[SENSOR_DATA_SIZE])
{
	memset(data, 0, SENSOR_DATA_SIZE);

	if (s_fdDev < 0) {
		ALOGE("%s: Device file is not opened.", __FUNCTION__);
		return AKD_FAIL;
	}

	if (ioctl(s_fdDev, ECS_IOCTL_GETDATA, data) < 0) {
		AKMERROR_STR("ioctl");
		return AKD_FAIL;
	}

	AKMDATA(AKMDATA_DRV,
		"bdata(HEX)= %02x %02x %02x %02x %02x %02x %02x %02x\n",
		data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);

	return AKD_SUCCESS;
}

/*!
 Set calculated data to device driver.
 @param[in] buf The order of input data depends on driver's specification.
 */
void AKD_SetYPR(const int buf[YPR_DATA_SIZE])
{
	if (s_fdDev < 0) {
		ALOGE("%s: Device file is not opened.", __FUNCTION__);
	} else {
		if (ioctl(s_fdDev, ECS_IOCTL_SET_YPR, buf) < 0) {
			AKMERROR_STR("ioctl");
		}
	}
}

/*!
 */
int AKD_GetOpenStatus(int* status)
{
	if (s_fdDev < 0) {
		ALOGE("%s: Device file is not opened.", __FUNCTION__);
		return AKD_FAIL;
	}
	if (ioctl(s_fdDev, ECS_IOCTL_GET_OPEN_STATUS, status) < 0) {
		AKMERROR_STR("ioctl");
		return AKD_FAIL;
	}
	return AKD_SUCCESS;
}

/*!
 */
int AKD_GetCloseStatus(int* status)
{
	if (s_fdDev < 0) {
		ALOGE("%s: Device file is not opened.", __FUNCTION__);
		return AKD_FAIL;
	}
	if (ioctl(s_fdDev, ECS_IOCTL_GET_CLOSE_STATUS, status) < 0) {
		AKMERROR_STR("ioctl");
		return AKD_FAIL;
	}
	return AKD_SUCCESS;
}

/*!
 Set AK8975 to the specific mode.
 @return If this function succeeds, the return value is #AKD_SUCCESS. Otherwise
 the return value is #AKD_FAIL.
 @param[in] mode This value should be one of the AK8975_Mode which is defined in
 akm8975.h file.
 */
int16_t AKD_SetMode(const BYTE mode)
{
	if (s_fdDev < 0) {
		ALOGE("%s: Device file is not opened.", __FUNCTION__);
		return AKD_FAIL;
	}

	if (ioctl(s_fdDev, ECS_IOCTL_SET_MODE, &mode) < 0) {
		AKMERROR_STR("ioctl");
		return AKD_FAIL;
	}

	return AKD_SUCCESS;
}

/*!
 Acquire delay
 @return If this function succeeds, the return value is #AKD_SUCCESS. Otherwise
 the return value is #AKD_FAIL.
 @param[out] delay A delay in nanosecond.
 */
int16_t AKD_GetDelay(int64_t delay[AKM_NUM_SENSORS])
{
	if (s_fdDev < 0) {
		ALOGE("%s: Device file is not opened.\n", __FUNCTION__);
		return AKD_FAIL;
	}
	if (ioctl(s_fdDev, ECS_IOCTL_GET_DELAY, delay) < 0) {
		AKMERROR_STR("ioctl");
		return AKD_FAIL;
	}
	return AKD_SUCCESS;
}

/*!
 Get layout information from device driver, i.e. platform data.
 */
int16_t AKD_GetLayout(int16_t* layout)
{
	char tmp;

	if (s_fdDev < 0) {
		ALOGE("%s: Device file is not opened.", __FUNCTION__);
		return AKD_FAIL;
	}

	if (ioctl(s_fdDev, ECS_IOCTL_GET_LAYOUT, &tmp) < 0) {
		AKMERROR_STR("ioctl");
		return AKD_FAIL;
	}

	*layout = tmp;
	return AKD_SUCCESS;
}

/* Get acceleration data. */
int16_t AKD_GetAccelerationData(int16_t data[3])
{
	if (s_fdDev < 0) {
		ALOGE("%s: Device file is not opened.", __FUNCTION__);
		return AKD_FAIL;
	}
	if (ioctl(s_fdDev, ECS_IOCTL_GET_ACCEL, data) < 0) {
		AKMERROR_STR("ioctl");
		return AKD_FAIL;
	}

	AKMDATA(AKMDATA_DRV, "%s: acc=%d, %d, %d\n",
			__FUNCTION__, data[0], data[1], data[2]);

	return AKD_SUCCESS;
}
