/*
 $License:
    Copyright (C) 2014 InvenSense Corporation, All Rights Reserved.
 $
 */

/******************************************************************************
 *
 * $Id: ml_stored_data.c 6132 2011-10-01 03:17:27Z mcaramello $
 *
 *****************************************************************************/

/**
 * @defgroup ML_STORED_DATA
 *
 * @{
 *      @file     ml_stored_data.c
 *      @brief    functions for reading and writing stored data sets.
 *                Typically, these functions process stored calibration data.
 */

#undef MPL_LOG_NDEBUG
#define MPL_LOG_NDEBUG 0 /* Use 0 to turn on MPL_LOGV output */
#undef MPL_LOG_TAG

#include <stdio.h>

#include "log.h"
#undef MPL_LOG_TAG
#define MPL_LOG_TAG "MPL-storeload"

#include "ml_stored_data.h"
#include "storage_manager.h"
#include "mlos.h"

#define LOADCAL_DEBUG    0
#define STORECAL_DEBUG   0

#define DEFAULT_KEY 29681

#define STORECAL_LOG MPL_LOGI
#define LOADCAL_LOG  MPL_LOGI

inv_error_t inv_read_cal(unsigned char **calData, size_t *bytesRead)
{
    FILE *fp;
    inv_error_t result = INV_SUCCESS;
    size_t fsize;

    fp = fopen(MLCAL_FILE,"rb");
    if (fp == NULL) {
        MPL_LOGE("Cannot open file \"%s\" for read\n", MLCAL_FILE);
        return INV_ERROR_FILE_OPEN;
    }

    // obtain file size
    fseek (fp, 0 , SEEK_END);
    fsize = ftell (fp);
    rewind (fp);
  
    *calData = (unsigned char *)inv_malloc(fsize);
    if (*calData==NULL) {
        MPL_LOGE("Could not allocate buffer of %d bytes - "
                 "aborting\n", fsize);
        fclose(fp);
        return INV_ERROR_MEMORY_EXAUSTED;
    }

    *bytesRead = fread(*calData, 1, fsize, fp);
    if (*bytesRead != fsize) {
        MPL_LOGE("bytes read (%d) don't match file size (%d)\n",
                 *bytesRead, fsize);
        result = INV_ERROR_FILE_READ;
        goto read_cal_end;
    } else {
        MPL_LOGV("Bytes read = %d", *bytesRead);
    }

read_cal_end:
    fclose(fp);
    return result;
}

inv_error_t inv_write_cal(unsigned char *cal, size_t len)
{
    FILE *fp;
    int bytesWritten;
    inv_error_t result = INV_SUCCESS;
   
    if (len <= 0) {
        MPL_LOGE("Nothing to write");
        return INV_ERROR_FILE_WRITE;
    } else {
        MPL_LOGV("cal data size to write = %d", len);
    }
    fp = fopen(MLCAL_FILE,"wb");
    if (fp == NULL) {
        MPL_LOGE("Cannot open file \"%s\" for write\n", MLCAL_FILE);
        return INV_ERROR_FILE_OPEN;
    }
    bytesWritten = fwrite(cal, 1, len, fp);
    if (bytesWritten != len) {
        MPL_LOGE("bytes written (%d) don't match requested length (%d)\n",
                 bytesWritten, len);
        result = INV_ERROR_FILE_WRITE;
    } else {
        MPL_LOGV("Bytes written = %d", bytesWritten);
    }
    fclose(fp);
    return result;
}

/**
 *  @brief  Loads a type 0 set of calibration data.
 *          It parses a binary data set containing calibration data.
 *          The binary data set is intended to be loaded from a file.
 *          This calibrations data format stores values for (in order of
 *          appearance) :
 *          - temperature compensation : temperature data points,
 *          - temperature compensation : gyro biases data points for X, Y,
 *              and Z axes.
 *          - accel biases for X, Y, Z axes.
 *          This calibration data is produced internally by the MPL and its
 *          size is 2777 bytes (header and checksum included).
 *          Calibration format type 1 is currently used for ITG3500
 *
 *  @pre    inv_init_storage_manager()
 *          must have been called.
 *
 *  @param  calData
 *              A pointer to an array of bytes to be parsed.
 *  @param  len
 *              the length of the calibration
 *
 *  @return INV_SUCCESS if successful, a non-zero error code otherwise.
 */
inv_error_t inv_load_cal_V0(unsigned char *calData, size_t len)
{
    inv_error_t result;

    LOADCAL_LOG("Entering inv_load_cal_V0\n");

    /*if (len != expLen) {
        MPL_LOGE("Calibration data type 0 must be %d bytes long (got %d)\n",
                 expLen, len);
        return INV_ERROR_FILE_READ;
    }*/

    result = inv_load_mpl_states(calData, len);
    return result;
}

/**
 *  @brief  Loads a type 1 set of calibration data.
 *          It parses a binary data set containing calibration data.
 *          The binary data set is intended to be loaded from a file.
 *          This calibrations data format stores values for (in order of
 *          appearance) :
 *          - temperature,
 *          - gyro biases for X, Y, Z axes,
 *          - accel biases for X, Y, Z axes.
 *          This calibration data would normally be produced by the MPU Self
 *          Test and its size is 36 bytes (header and checksum included).
 *          Calibration format type 1 is produced by the MPU Self Test and
 *          substitutes the type 0 : inv_load_cal_V0().
 *
 *  @pre    
 *
 *  @param  calData
 *              A pointer to an array of bytes to be parsed.
 *  @param  len
 *              the length of the calibration
 *
 *  @return INV_SUCCESS if successful, a non-zero error code otherwise.
 */
inv_error_t inv_load_cal_V1(unsigned char *calData, size_t len)
{
    return INV_SUCCESS;
}

/**
 * @brief   Loads a set of calibration data.
 *          It parses a binary data set containing calibration data.
 *          The binary data set is intended to be loaded from a file.
 *
 * @pre     
 *          
 *
 * @param   calData
 *              A pointer to an array of bytes to be parsed.
 *
 * @return  INV_SUCCESS if successful, a non-zero error code otherwise.
 */
inv_error_t inv_load_cal(unsigned char *calData)
{
    int calType = 0;
    int len = 0;
    //int ptr;
    //uint32_t chk = 0;
    //uint32_t cmp_chk = 0;

    /*load_func_t loaders[] = {
        inv_load_cal_V0,
        inv_load_cal_V1,
    };
    */

    inv_load_cal_V0(calData, len);

    /* read the header (type and len)
       len is the total record length including header and checksum */
    len = 0;
    len += 16777216L * ((int)calData[0]);
    len += 65536L * ((int)calData[1]);
    len += 256 * ((int)calData[2]);
    len += (int)calData[3];

    calType = ((int)calData[4]) * 256 + ((int)calData[5]);
    if (calType > 5) {
        MPL_LOGE("Unsupported calibration file format %d. "
                 "Valid types 0..5\n", calType);
        return INV_ERROR_INVALID_PARAMETER;
    }

    /* call the proper method to read in the data */
    //return loaders[calType] (calData, len);
    return 0;
}

/**
 *  @brief  Stores a set of calibration data.
 *          It generates a binary data set containing calibration data.
 *          The binary data set is intended to be stored into a file.
 *
 *  @pre    inv_dmp_open()
 *
 *  @param  calData
 *              A pointer to an array of bytes to be stored.
 *  @param  length
 *              The amount of bytes available in the array.
 *
 *  @return INV_SUCCESS if successful, a non-zero error code otherwise.
 */
inv_error_t inv_store_cal(unsigned char *calData, size_t length)
{
    inv_error_t res = 0;
    size_t size;

    STORECAL_LOG("Entering inv_store_cal\n");

    inv_get_mpl_state_size(&size);

    MPL_LOGV("inv_get_mpl_state_size() : size=%d", size);

    /* store data */
    res = inv_save_mpl_states(calData, size);
    if(res != 0) {
        MPL_LOGE("inv_save_mpl_states() failed");
    }

    STORECAL_LOG("Exiting inv_store_cal\n");
    return INV_SUCCESS;
}

/**
 *  @brief  Load a calibration file.
 *
 *  @pre    Must be in INV_STATE_DMP_OPENED state.
 *          inv_dmp_open() or inv_dmp_stop() must have been called.
 *          inv_dmp_start() and inv_dmp_close() must have <b>NOT</b>
 *          been called.
 *
 *  @return 0 or error code.
 */
inv_error_t inv_load_calibration(void)
{
    unsigned char *calData= NULL;
    inv_error_t result = 0;
    size_t bytesRead = 0;

    result = inv_read_cal(&calData, &bytesRead);
    if(result != INV_SUCCESS) {
        MPL_LOGE("Could not load cal file - "
                 "aborting\n");
        goto free_mem_n_exit;
    }

    result = inv_load_mpl_states(calData, bytesRead);
    if (result != INV_SUCCESS) {
        MPL_LOGE("Could not load the calibration data - "
                 "error %d - aborting\n", result);
        goto free_mem_n_exit;
    }

free_mem_n_exit:
    inv_free(calData);
    return result;
}

/**
 *  @brief  Store runtime calibration data to a file
 *
 *  @pre    Must be in INV_STATE_DMP_OPENED state.
 *          inv_dmp_open() or inv_dmp_stop() must have been called.
 *          inv_dmp_start() and inv_dmp_close() must have <b>NOT</b>
 *          been called.
 *
 *  @return 0 or error code.
 */
inv_error_t inv_store_calibration(void)
{
    unsigned char *calData;
    inv_error_t result;
    size_t length;

    result = inv_get_mpl_state_size(&length);
    calData = (unsigned char *)inv_malloc(length);
    if (!calData) {
        MPL_LOGE("Could not allocate buffer of %d bytes - "
                 "aborting\n", length);
        return INV_ERROR_MEMORY_EXAUSTED;
    } else {
        MPL_LOGV("inv_get_mpl state size = %d", length);
    }

    result = inv_save_mpl_states(calData, length);
    if (result != INV_SUCCESS) {
        MPL_LOGE("Could not save mpl states - "
                 "error %d - aborting\n", result);
        goto free_mem_n_exit;
    } else {
        MPL_LOGV("calData from inv_save_mpl_states, size=%d", 
                 strlen((char *)calData));
    }

    result = inv_write_cal(calData, length);
    if (result != INV_SUCCESS) {
        MPL_LOGE("Could not store calibrated data on file - "
                 "error %d - aborting\n", result);
        goto free_mem_n_exit;

    }

free_mem_n_exit:
    inv_free(calData);
    return result;
}

/**
 *  @}
 */
