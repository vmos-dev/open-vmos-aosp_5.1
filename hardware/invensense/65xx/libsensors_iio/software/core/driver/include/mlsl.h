/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */

#ifndef __MLSL_H__
#define __MLSL_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  @defgroup   MLSL
 *  @brief      Motion Library - Serial Layer.
 *              The Motion Library System Layer provides the Motion Library
 *              with the communication interface to the hardware.
 *
 *              The communication interface is assumed to support serial
 *              transfers in burst of variable length up to
 *              SERIAL_MAX_TRANSFER_SIZE.
 *              The default value for SERIAL_MAX_TRANSFER_SIZE is 128 bytes.
 *              Transfers of length greater than SERIAL_MAX_TRANSFER_SIZE, will
 *              be subdivided in smaller transfers of length <=
 *              SERIAL_MAX_TRANSFER_SIZE.
 *              The SERIAL_MAX_TRANSFER_SIZE definition can be modified to
 *              overcome any host processor transfer size limitation down to
 *              1 B, the minimum.
 *              An higher value for SERIAL_MAX_TRANSFER_SIZE will favor
 *              performance and efficiency while requiring higher resource usage
 *              (mostly buffering). A smaller value will increase overhead and
 *              decrease efficiency but allows to operate with more resource
 *              constrained processor and master serial controllers.
 *              The SERIAL_MAX_TRANSFER_SIZE definition can be found in the
 *              mlsl.h header file and master serial controllers.
 *              The SERIAL_MAX_TRANSFER_SIZE definition can be found in the
 *              mlsl.h header file.
 *
 *  @{
 *      @file   mlsl.h
 *      @brief  The Motion Library System Layer.
 *
 */

/*
 * NOTE : to properly support Yamaha compass reads,
 *	  the max transfer size should be at least 9 B.
 *	  Length in bytes, typically a power of 2 >= 2
 */
#define SERIAL_MAX_TRANSFER_SIZE 31

#ifndef __KERNEL__
/**
 *  inv_serial_open() - used to open the serial port.
 *  @port	The COM port specification associated with the device in use.
 *  @sl_handle	a pointer to the file handle to the serial device to be open
 *		for the communication.
 *	This port is used to send and receive data to the device.
 *
 *	This function is called by inv_serial_start().
 *	Unlike previous MPL Software releases, explicitly calling
 *	inv_serial_start() is mandatory to instantiate the communication
 *	with the device.
 *
 *  returns INV_SUCCESS if successful, a non-zero error code otherwise.
 */
int inv_serial_open(char const *port, void **sl_handle);

/**
 *  inv_serial_close() - used to close the serial port.
 *  @sl_handle	a file handle to the serial device used for the communication.
 *
 *	This port is used to send and receive data to the device.
 *
 *	This function is called by inv_serial_stop().
 *	Unlike previous MPL Software releases, explicitly calling
 *	inv_serial_stop() is mandatory to properly shut-down the
 *	communication with the device.
 *
 *  returns INV_SUCCESS if successful, a non-zero error code otherwise.
 */
int inv_serial_close(void *sl_handle);

/**
 *  inv_serial_reset() - used to reset any buffering the driver may be doing
 *  returns INV_SUCCESS if successful, a non-zero error code otherwise.
 */
int inv_serial_reset(void *sl_handle);
#endif

/**
 *  inv_serial_single_write() - used to write a single byte of data.
 *  @sl_handle		pointer to the serial device used for the communication.
 *  @slave_addr		I2C slave address of device.
 *  @register_addr	Register address to write.
 *  @data		Single byte of data to write.
 *
 *	It is called by the MPL to write a single byte of data to the MPU.
 *
 *  returns INV_SUCCESS if successful, a non-zero error code otherwise.
 */
int inv_serial_single_write(
	void *sl_handle,
	unsigned char slave_addr,
	unsigned char register_addr,
	unsigned char data);

/**
 *  inv_serial_write() - used to write multiple bytes of data to registers.
 *  @sl_handle	a file handle to the serial device used for the communication.
 *  @slave_addr	I2C slave address of device.
 *  @register_addr	Register address to write.
 *  @length	Length of burst of data.
 *  @data	Pointer to block of data.
 *
 *  returns INV_SUCCESS if successful, a non-zero error code otherwise.
 */
int inv_serial_write(
	void *sl_handle,
	unsigned char slave_addr,
	unsigned short length,
	unsigned char const *data);

/**
 *  inv_serial_read() - used to read multiple bytes of data from registers.
 *  @sl_handle	a file handle to the serial device used for the communication.
 *  @slave_addr	I2C slave address of device.
 *  @register_addr	Register address to read.
 *  @length	Length of burst of data.
 *  @data	Pointer to block of data.
 *
 *  returns INV_SUCCESS == 0 if successful; a non-zero error code otherwise.
 */
int inv_serial_read(
	void *sl_handle,
	unsigned char slave_addr,
	unsigned char register_addr,
	unsigned short length,
	unsigned char *data);

/**
 *  inv_serial_read_mem() - used to read multiple bytes of data from the memory.
 *	    This should be sent by I2C or SPI.
 *
 *  @sl_handle	a file handle to the serial device used for the communication.
 *  @slave_addr	I2C slave address of device.
 *  @mem_addr	The location in the memory to read from.
 *  @length	Length of burst data.
 *  @data	Pointer to block of data.
 *
 *  returns INV_SUCCESS == 0 if successful; a non-zero error code otherwise.
 */
int inv_serial_read_mem(
	void *sl_handle,
	unsigned char slave_addr,
	unsigned short mem_addr,
	unsigned char bank_reg,
	unsigned char addr_reg,
	unsigned char mem_reg,
	unsigned short length,
	unsigned char *data);

/**
 *  inv_serial_write_mem() - used to write multiple bytes of data to the memory.
 *  @sl_handle	a file handle to the serial device used for the communication.
 *  @slave_addr	I2C slave address of device.
 *  @mem_addr	The location in the memory to write to.
 *  @length	Length of burst data.
 *  @data	Pointer to block of data.
 *
 *  returns INV_SUCCESS == 0 if successful; a non-zero error code otherwise.
 */
int inv_serial_write_mem(
	void *sl_handle,
	unsigned char slave_addr,
	unsigned short mem_addr,
	unsigned char bank_reg,
	unsigned char addr_reg,
	unsigned char mem_reg,
	unsigned short length,
	unsigned char *data);

/**
 *  inv_serial_read_fifo() - used to read multiple bytes of data from the fifo.
 *  @sl_handle	a file handle to the serial device used for the communication.
 *  @slave_addr	I2C slave address of device.
 *  @length	Length of burst of data.
 *  @data	Pointer to block of data.
 *
 *  returns INV_SUCCESS == 0 if successful; a non-zero error code otherwise.
 */
int inv_serial_read_fifo(
	void *sl_handle,
	unsigned char slave_addr,
	unsigned char fifo_reg,
	unsigned short length,
	unsigned char *data);

/**
 *  inv_serial_write_fifo() - used to write multiple bytes of data to the fifo.
 *  @sl_handle	a file handle to the serial device used for the communication.
 *  @slave_addr	I2C slave address of device.
 *  @length	Length of burst of data.
 *  @data	Pointer to block of data.
 *
 *  returns INV_SUCCESS == 0 if successful; a non-zero error code otherwise.
 */
int inv_serial_write_fifo(
	void *sl_handle,
	unsigned char slave_addr,
	unsigned char fifo_reg,
	unsigned short length,
	unsigned char const *data);

#ifndef __KERNEL__
/**
 *  inv_serial_read_cfg() - used to get the configuration data.
 *  @cfg	Pointer to the configuration data.
 *  @len	Length of the configuration data.
 *
 *		Is called by the MPL to get the configuration data
 *		used by the motion library.
 *		This data would typically be saved in non-volatile memory.
 *
 *  returns INV_SUCCESS if successful, a non-zero error code otherwise.
 */
int inv_serial_read_cfg(unsigned char *cfg, unsigned int len);

/**
 *  inv_serial_write_cfg() - used to save the configuration data.
 *  @cfg	Pointer to the configuration data.
 *  @len	Length of the configuration data.
 *
 *		Is called by the MPL to save the configuration data used by the
 *		motion library.
 *		This data would typically be saved in non-volatile memory.
 *
 *  returns INV_SUCCESS if successful, a non-zero error code otherwise.
 */
int inv_serial_write_cfg(unsigned char *cfg, unsigned int len);

/**
 *  inv_serial_read_cal() - used to get the calibration data.
 *  @cfg	Pointer to the calibration data.
 *  @len	Length of the calibration data.
 *
 *		It is called by the MPL to get the calibration data used by the
 *		motion library.
 *		This data is typically be saved in non-volatile memory.
 *
 *  returns INV_SUCCESS if successful, a non-zero error code otherwise.
 */
int inv_serial_read_cal(unsigned char *cal, unsigned int len);

/**
 *  inv_serial_write_cal() - used to save the calibration data.
 *
 *  @cfg	Pointer to the calibration data.
 *  @len	Length of the calibration data.
 *
 *	    It is called by the MPL to save the calibration data used by the
 *	    motion library.
 *	    This data is typically be saved in non-volatile memory.
 *  returns INV_SUCCESS if successful, a non-zero error code otherwise.
 */
int inv_serial_write_cal(unsigned char *cal, unsigned int len);

/**
 *  inv_serial_get_cal_length() - Get the calibration length from the storage.
 *  @len	lenght to be returned
 *
 *  returns INV_SUCCESS if successful, a non-zero error code otherwise.
 */
int inv_serial_get_cal_length(unsigned int *len);
#endif
#ifdef __cplusplus
}
#endif
/**
 * @}
 */
#endif				/* __MLSL_H__ */
