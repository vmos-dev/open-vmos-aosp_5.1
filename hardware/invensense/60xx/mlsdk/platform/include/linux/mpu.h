/*
 $License:
   Copyright 2011 InvenSense, Inc.

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
  $
 */

#ifndef __MPU_H_
#define __MPU_H_

#ifdef __KERNEL__
#include <linux/types.h>
#include <linux/ioctl.h>
#elif defined LINUX
#include <sys/ioctl.h>
#endif

/* Number of axes on each sensor */
#define GYRO_NUM_AXES               (3)
#define ACCEL_NUM_AXES              (3)
#define COMPASS_NUM_AXES            (3)

struct mpu_read_write {
	/* Memory address or register address depending on ioctl */
	unsigned short address;
	unsigned short length;
	unsigned char *data;
};

enum mpuirq_data_type {
	MPUIRQ_DATA_TYPE_MPU_IRQ,
	MPUIRQ_DATA_TYPE_SLAVE_IRQ,
	MPUIRQ_DATA_TYPE_PM_EVENT,
	MPUIRQ_DATA_TYPE_NUM_TYPES,
};

/* User space PM event notification */
#define MPU_PM_EVENT_SUSPEND_PREPARE (3)
#define MPU_PM_EVENT_POST_SUSPEND    (4)

struct mpuirq_data {
	int interruptcount;
	unsigned long long irqtime;
	int data_type;
	long data;
};

enum ext_slave_config_key {
	MPU_SLAVE_CONFIG_ODR_SUSPEND,
	MPU_SLAVE_CONFIG_ODR_RESUME,
	MPU_SLAVE_CONFIG_FSR_SUSPEND,
	MPU_SLAVE_CONFIG_FSR_RESUME,
	MPU_SLAVE_CONFIG_MOT_THS,
	MPU_SLAVE_CONFIG_NMOT_THS,
	MPU_SLAVE_CONFIG_MOT_DUR,
	MPU_SLAVE_CONFIG_NMOT_DUR,
	MPU_SLAVE_CONFIG_IRQ_SUSPEND,
	MPU_SLAVE_CONFIG_IRQ_RESUME,
	MPU_SLAVE_WRITE_REGISTERS,
	MPU_SLAVE_READ_REGISTERS,
	/* AMI 306 specific config keys */
	MPU_SLAVE_PARAM,
	MPU_SLAVE_WINDOW,
	MPU_SLAVE_READWINPARAMS,
	MPU_SLAVE_SEARCHOFFSET,
	/* AKM specific config keys */
	MPU_SLAVE_READ_SCALE,
	/* YAS specific config keys */
	MPU_SLAVE_OFFSET_VALS,
	MPU_SLAVE_RANGE_CHECK,

	MPU_SLAVE_CONFIG_NUM_CONFIG_KEYS,
};

/* For the MPU_SLAVE_CONFIG_IRQ_SUSPEND and MPU_SLAVE_CONFIG_IRQ_RESUME */
enum ext_slave_config_irq_type {
	MPU_SLAVE_IRQ_TYPE_NONE,
	MPU_SLAVE_IRQ_TYPE_MOTION,
	MPU_SLAVE_IRQ_TYPE_DATA_READY,
};

/* Structure for the following IOCTS's
 * MPU_CONFIG_ACCEL
 * MPU_CONFIG_COMPASS
 * MPU_CONFIG_PRESSURE
 * MPU_GET_CONFIG_ACCEL
 * MPU_GET_CONFIG_COMPASS
 * MPU_GET_CONFIG_PRESSURE
 *
 * @key one of enum ext_slave_config_key
 * @len length of data pointed to by data
 * @apply zero if communication with the chip is not necessary, false otherwise
 *        This flag can be used to select cached data or to refresh cashed data
 *        cache data to be pushed later or push immediately.  If true and the
 *        slave is on the secondary bus the MPU will first enger bypass mode
 *        before calling the slaves .config or .get_config funcion
 * @data pointer to the data to confgure or get
 */
struct ext_slave_config {
	int key;
	int len;
	int apply;
	void *data;
};

enum ext_slave_type {
	EXT_SLAVE_TYPE_GYROSCOPE,
	EXT_SLAVE_TYPE_ACCELEROMETER,
	EXT_SLAVE_TYPE_COMPASS,
	EXT_SLAVE_TYPE_PRESSURE,
	/*EXT_SLAVE_TYPE_TEMPERATURE */

	EXT_SLAVE_NUM_TYPES
};

enum ext_slave_id {
	ID_INVALID = 0,

	ACCEL_ID_LIS331,
	ACCEL_ID_LSM303A,
	ACCEL_ID_LIS3DH,
	ACCEL_ID_KXSD9,
	ACCEL_ID_KXTF9,
	ACCEL_ID_BMA150,
	ACCEL_ID_BMA222,
	ACCEL_ID_BMA250,
	ACCEL_ID_ADXL34X,
	ACCEL_ID_MMA8450,
	ACCEL_ID_MMA845X,
	ACCEL_ID_MPU6050,

	COMPASS_ID_AK8975,
	COMPASS_ID_AMI30X,
	COMPASS_ID_AMI306,
	COMPASS_ID_YAS529,
	COMPASS_ID_YAS530,
	COMPASS_ID_HMC5883,
	COMPASS_ID_LSM303M,
	COMPASS_ID_MMC314X,
	COMPASS_ID_HSCDTD002B,
	COMPASS_ID_HSCDTD004A,

	PRESSURE_ID_BMA085,
};

enum ext_slave_endian {
	EXT_SLAVE_BIG_ENDIAN,
	EXT_SLAVE_LITTLE_ENDIAN,
	EXT_SLAVE_FS8_BIG_ENDIAN,
	EXT_SLAVE_FS16_BIG_ENDIAN,
};

enum ext_slave_bus {
	EXT_SLAVE_BUS_INVALID = -1,
	EXT_SLAVE_BUS_PRIMARY = 0,
	EXT_SLAVE_BUS_SECONDARY = 1
};


/**
 *  struct ext_slave_platform_data - Platform data for mpu3050 and mpu6050
 *  slave devices
 *
 *  @get_slave_descr: Function pointer to retrieve the struct ext_slave_descr
 *                    for this slave
 *  @irq: the irq number attached to the slave if any.
 *  @adapt_num: the I2C adapter number.
 *  @bus: the bus the slave is attached to: enum ext_slave_bus
 *  @address: the I2C slave address of the slave device.
 *  @orientation: the mounting matrix of the device relative to MPU.
 *  @irq_data: private data for the slave irq handler
 *  @private_data: additional data, user customizable.  Not touched by the MPU
 *                 driver.
 *
 * The orientation matricies are 3x3 rotation matricies
 * that are applied to the data to rotate from the mounting orientation to the
 * platform orientation.  The values must be one of 0, 1, or -1 and each row and
 * column should have exactly 1 non-zero value.
 */
struct ext_slave_platform_data {
	struct ext_slave_descr *(*get_slave_descr) (void);
	int irq;
	int adapt_num;
	int bus;
	unsigned char address;
	signed char orientation[9];
	void *irq_data;
	void *private_data;
};

struct fix_pnt_range {
	long mantissa;
	long fraction;
};

static inline long range_fixedpoint_to_long_mg(struct fix_pnt_range rng)
{
	return (long)(rng.mantissa * 1000 + rng.fraction / 10);
}

struct ext_slave_read_trigger {
	unsigned char reg;
	unsigned char value;
};

/**
 *  struct ext_slave_descr - Description of the slave device for programming.
 *
 *  @suspend:	function pointer to put the device in suspended state
 *  @resume:	function pointer to put the device in running state
 *  @read:	function that reads the device data
 *  @init:	function used to preallocate memory used by the driver
 *  @exit:	function used to free memory allocated for the driver
 *  @config:	function used to configure the device
 *  @get_config:function used to get the device's configuration
 *
 *  @name:	text name of the device
 *  @type:	device type. enum ext_slave_type
 *  @id:	enum ext_slave_id
 *  @reg:	starting register address to retrieve data.
 *  @len:	length in bytes of the sensor data.  Should be 6.
 *  @endian:	byte order of the data. enum ext_slave_endian
 *  @range:	full scale range of the slave ouput: struct fix_pnt_range
 *  @trigger:	If reading data first requires writing a register this is the
 *		data to write.
 *
 *  Defines the functions and information about the slave the mpu3050 and
 *  mpu6050 needs to use the slave device.
 */
struct ext_slave_descr {
	int (*init) (void *mlsl_handle,
		     struct ext_slave_descr *slave,
		     struct ext_slave_platform_data *pdata);
	int (*exit) (void *mlsl_handle,
		     struct ext_slave_descr *slave,
		     struct ext_slave_platform_data *pdata);
	int (*suspend) (void *mlsl_handle,
			struct ext_slave_descr *slave,
			struct ext_slave_platform_data *pdata);
	int (*resume) (void *mlsl_handle,
		       struct ext_slave_descr *slave,
		       struct ext_slave_platform_data *pdata);
	int (*read) (void *mlsl_handle,
		     struct ext_slave_descr *slave,
		     struct ext_slave_platform_data *pdata,
		     unsigned char *data);
	int (*config) (void *mlsl_handle,
		       struct ext_slave_descr *slave,
		       struct ext_slave_platform_data *pdata,
		       struct ext_slave_config *config);
	int (*get_config) (void *mlsl_handle,
			   struct ext_slave_descr *slave,
			   struct ext_slave_platform_data *pdata,
			   struct ext_slave_config *config);

	char *name;
	unsigned char type;
	unsigned char id;
	unsigned char read_reg;
	unsigned int read_len;
	unsigned char endian;
	struct fix_pnt_range range;
	struct ext_slave_read_trigger *trigger;
};

/**
 * struct mpu_platform_data - Platform data for the mpu driver
 * @int_config:		Bits [7:3] of the int config register.
 * @orientation:	Orientation matrix of the gyroscope
 * @level_shifter:	0: VLogic, 1: VDD
 * @accel:		Accel platform data
 * @compass:		Compass platform data
 * @pressure:		Pressure platform data
 *
 * Contains platform specific information on how to configure the MPU3050 to
 * work on this platform.  The orientation matricies are 3x3 rotation matricies
 * that are applied to the data to rotate from the mounting orientation to the
 * platform orientation.  The values must be one of 0, 1, or -1 and each row and
 * column should have exactly 1 non-zero value.
 */
struct mpu_platform_data {
	unsigned char int_config;
	signed char orientation[GYRO_NUM_AXES * GYRO_NUM_AXES];
	unsigned char level_shifter;
	struct ext_slave_platform_data accel;
	struct ext_slave_platform_data compass;
	struct ext_slave_platform_data pressure;
};

#if defined __KERNEL__ || defined LINUX
#define MPU_IOCTL (0x81) /* Magic number for MPU Iocts */
/* IOCTL commands for /dev/mpu */
#define MPU_SET_MPU_CONFIG	_IOWR(MPU_IOCTL, 0x00, struct mldl_cfg)
#define MPU_GET_MPU_CONFIG	_IOW(MPU_IOCTL,  0x00, struct mldl_cfg)

#define MPU_SET_PLATFORM_DATA	_IOWR(MPU_IOCTL, 0x01, struct mldl_cfg)

#define MPU_READ		_IOWR(MPU_IOCTL, 0x10, struct mpu_read_write)
#define MPU_WRITE		_IOW(MPU_IOCTL,  0x10, struct mpu_read_write)
#define MPU_READ_MEM		_IOWR(MPU_IOCTL, 0x11, struct mpu_read_write)
#define MPU_WRITE_MEM		_IOW(MPU_IOCTL,  0x11, struct mpu_read_write)
#define MPU_READ_FIFO		_IOWR(MPU_IOCTL, 0x12, struct mpu_read_write)
#define MPU_WRITE_FIFO		_IOW(MPU_IOCTL,  0x12, struct mpu_read_write)

#define MPU_READ_COMPASS	_IOR(MPU_IOCTL, 0x12, unsigned char)
#define MPU_READ_ACCEL		_IOR(MPU_IOCTL, 0x13, unsigned char)
#define MPU_READ_PRESSURE	_IOR(MPU_IOCTL, 0x14, unsigned char)

#define MPU_CONFIG_ACCEL	_IOW(MPU_IOCTL, 0x20, struct ext_slave_config)
#define MPU_CONFIG_COMPASS	_IOW(MPU_IOCTL, 0x21, struct ext_slave_config)
#define MPU_CONFIG_PRESSURE	_IOW(MPU_IOCTL, 0x22, struct ext_slave_config)

#define MPU_GET_CONFIG_ACCEL	_IOWR(MPU_IOCTL, 0x20, struct ext_slave_config)
#define MPU_GET_CONFIG_COMPASS	_IOWR(MPU_IOCTL, 0x21, struct ext_slave_config)
#define MPU_GET_CONFIG_PRESSURE	_IOWR(MPU_IOCTL, 0x22, struct ext_slave_config)

#define MPU_SUSPEND		_IO(MPU_IOCTL, 0x30)
#define MPU_RESUME		_IO(MPU_IOCTL, 0x31)
/* Userspace PM Event response */
#define MPU_PM_EVENT_HANDLED	_IO(MPU_IOCTL, 0x32)

#endif

#endif				/* __MPU_H_ */
