/* Copyright (c) 2012-2014, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "mm_jpeg_dbg.h"
#include "mm_jpeg.h"

#include <errno.h>
#include <math.h>


#define LOWER(a)               ((a) & 0xFFFF)
#define UPPER(a)               (((a)>>16) & 0xFFFF)
#define CHANGE_ENDIAN_16(a)  ((0x00FF & ((a)>>8)) | (0xFF00 & ((a)<<8)))
#define ROUND(a)((a >= 0) ? (long)(a + 0.5) : (long)(a - 0.5))


/** addExifEntry:
 *
 *  Arguments:
 *   @exif_info : Exif info struct
 *   @p_session: job session
 *   @tagid   : exif tag ID
 *   @type    : data type
 *   @count   : number of data in uint of its type
 *   @data    : input data ptr
 *
 *  Retrun     : int32_t type of status
 *               0  -- success
 *              none-zero failure code
 *
 *  Description:
 *       Function to add an entry to exif data
 *
 **/
int32_t addExifEntry(QOMX_EXIF_INFO *p_exif_info, exif_tag_id_t tagid,
  exif_tag_type_t type, uint32_t count, void *data)
{
    int32_t rc = 0;
    int32_t numOfEntries = p_exif_info->numOfEntries;
    QEXIF_INFO_DATA *p_info_data = p_exif_info->exif_data;
    if(numOfEntries >= MAX_EXIF_TABLE_ENTRIES) {
        ALOGE("%s: Number of entries exceeded limit", __func__);
        return -1;
    }

    p_info_data[numOfEntries].tag_id = tagid;
    p_info_data[numOfEntries].tag_entry.type = type;
    p_info_data[numOfEntries].tag_entry.count = count;
    p_info_data[numOfEntries].tag_entry.copy = 1;
    switch (type) {
    case EXIF_BYTE: {
      if (count > 1) {
        uint8_t *values = (uint8_t *)malloc(count);
        if (values == NULL) {
          ALOGE("%s: No memory for byte array", __func__);
          rc = -1;
        } else {
          memcpy(values, data, count);
          p_info_data[numOfEntries].tag_entry.data._bytes = values;
        }
      } else {
        p_info_data[numOfEntries].tag_entry.data._byte = *(uint8_t *)data;
      }
    }
    break;
    case EXIF_ASCII: {
      char *str = NULL;
      str = (char *)malloc(count + 1);
      if (str == NULL) {
        ALOGE("%s: No memory for ascii string", __func__);
        rc = -1;
      } else {
        memset(str, 0, count + 1);
        memcpy(str, data, count);
        p_info_data[numOfEntries].tag_entry.data._ascii = str;
      }
    }
    break;
    case EXIF_SHORT: {
      if (count > 1) {
        uint16_t *values = (uint16_t *)malloc(count * sizeof(uint16_t));
        if (values == NULL) {
          ALOGE("%s: No memory for short array", __func__);
          rc = -1;
        } else {
          memcpy(values, data, count * sizeof(uint16_t));
          p_info_data[numOfEntries].tag_entry.data._shorts = values;
        }
      } else {
        p_info_data[numOfEntries].tag_entry.data._short = *(uint16_t *)data;
      }
    }
    break;
    case EXIF_LONG: {
      if (count > 1) {
        uint32_t *values = (uint32_t *)malloc(count * sizeof(uint32_t));
        if (values == NULL) {
          ALOGE("%s: No memory for long array", __func__);
          rc = -1;
        } else {
          memcpy(values, data, count * sizeof(uint32_t));
          p_info_data[numOfEntries].tag_entry.data._longs = values;
        }
      } else {
        p_info_data[numOfEntries].tag_entry.data._long = *(uint32_t *)data;
      }
    }
    break;
    case EXIF_RATIONAL: {
      if (count > 1) {
        rat_t *values = (rat_t *)malloc(count * sizeof(rat_t));
        if (values == NULL) {
          ALOGE("%s: No memory for rational array", __func__);
          rc = -1;
        } else {
          memcpy(values, data, count * sizeof(rat_t));
          p_info_data[numOfEntries].tag_entry.data._rats = values;
        }
      } else {
        p_info_data[numOfEntries].tag_entry.data._rat = *(rat_t *)data;
      }
    }
    break;
    case EXIF_UNDEFINED: {
      uint8_t *values = (uint8_t *)malloc(count);
      if (values == NULL) {
        ALOGE("%s: No memory for undefined array", __func__);
        rc = -1;
      } else {
        memcpy(values, data, count);
        p_info_data[numOfEntries].tag_entry.data._undefined = values;
      }
    }
    break;
    case EXIF_SLONG: {
      if (count > 1) {
        int32_t *values = (int32_t *)malloc(count * sizeof(int32_t));
        if (values == NULL) {
          ALOGE("%s: No memory for signed long array", __func__);
          rc = -1;
        } else {
          memcpy(values, data, count * sizeof(int32_t));
          p_info_data[numOfEntries].tag_entry.data._slongs = values;
        }
      } else {
        p_info_data[numOfEntries].tag_entry.data._slong = *(int32_t *)data;
      }
    }
    break;
    case EXIF_SRATIONAL: {
      if (count > 1) {
        srat_t *values = (srat_t *)malloc(count * sizeof(srat_t));
        if (values == NULL) {
          ALOGE("%s: No memory for signed rational array", __func__);
          rc = -1;
        } else {
          memcpy(values, data, count * sizeof(srat_t));
          p_info_data[numOfEntries].tag_entry.data._srats = values;
        }
      } else {
        p_info_data[numOfEntries].tag_entry.data._srat = *(srat_t *)data;
      }
    }
    break;
    }

    // Increase number of entries
    p_exif_info->numOfEntries++;
    return rc;
}

/** releaseExifEntry
 *
 *  Arguments:
 *   @p_exif_data : Exif info struct
 *
 *  Retrun     : int32_t type of status
 *               0  -- success
 *              none-zero failure code
 *
 *  Description:
 *       Function to release an entry from exif data
 *
 **/
int32_t releaseExifEntry(QEXIF_INFO_DATA *p_exif_data)
{
 switch (p_exif_data->tag_entry.type) {
  case EXIF_BYTE: {
    if (p_exif_data->tag_entry.count > 1 &&
      p_exif_data->tag_entry.data._bytes != NULL) {
      free(p_exif_data->tag_entry.data._bytes);
      p_exif_data->tag_entry.data._bytes = NULL;
    }
  }
  break;
  case EXIF_ASCII: {
    if (p_exif_data->tag_entry.data._ascii != NULL) {
      free(p_exif_data->tag_entry.data._ascii);
      p_exif_data->tag_entry.data._ascii = NULL;
    }
  }
  break;
  case EXIF_SHORT: {
    if (p_exif_data->tag_entry.count > 1 &&
      p_exif_data->tag_entry.data._shorts != NULL) {
      free(p_exif_data->tag_entry.data._shorts);
      p_exif_data->tag_entry.data._shorts = NULL;
    }
  }
  break;
  case EXIF_LONG: {
    if (p_exif_data->tag_entry.count > 1 &&
      p_exif_data->tag_entry.data._longs != NULL) {
      free(p_exif_data->tag_entry.data._longs);
      p_exif_data->tag_entry.data._longs = NULL;
    }
  }
  break;
  case EXIF_RATIONAL: {
    if (p_exif_data->tag_entry.count > 1 &&
      p_exif_data->tag_entry.data._rats != NULL) {
      free(p_exif_data->tag_entry.data._rats);
      p_exif_data->tag_entry.data._rats = NULL;
    }
  }
  break;
  case EXIF_UNDEFINED: {
    if (p_exif_data->tag_entry.data._undefined != NULL) {
      free(p_exif_data->tag_entry.data._undefined);
      p_exif_data->tag_entry.data._undefined = NULL;
    }
  }
  break;
  case EXIF_SLONG: {
    if (p_exif_data->tag_entry.count > 1 &&
      p_exif_data->tag_entry.data._slongs != NULL) {
      free(p_exif_data->tag_entry.data._slongs);
      p_exif_data->tag_entry.data._slongs = NULL;
    }
  }
  break;
  case EXIF_SRATIONAL: {
    if (p_exif_data->tag_entry.count > 1 &&
      p_exif_data->tag_entry.data._srats != NULL) {
      free(p_exif_data->tag_entry.data._srats);
      p_exif_data->tag_entry.data._srats = NULL;
    }
  }
  break;
  } /*end of switch*/
  return 0;
}
/** process_sensor_data:
 *
 *  Arguments:
 *   @p_sensor_params : ptr to sensor data
 *
 *  Return     : int32_t type of status
 *               NO_ERROR  -- success
 *              none-zero failure code
 *
 *  Description:
 *       process sensor data
 *
 *  Notes: this needs to be filled for the metadata
 **/
int process_sensor_data(cam_sensor_params_t *p_sensor_params,
  QOMX_EXIF_INFO *exif_info)
{
  int rc = 0;
  rat_t val_rat;

  if (NULL == p_sensor_params) {
    ALOGE("%s %d: Sensor params are null", __func__, __LINE__);
    return 0;
  }

  ALOGD("%s:%d] From metadata aperture = %f ", __func__, __LINE__,
    p_sensor_params->aperture_value );

  if (p_sensor_params->aperture_value >= 1.0) {
    double apex_value;
    apex_value = (double)2.0 * log(p_sensor_params->aperture_value) / log(2.0);
    val_rat.num = (uint32_t)(apex_value * 100);
    val_rat.denom = 100;
    rc = addExifEntry(exif_info, EXIFTAGID_APERTURE, EXIF_RATIONAL, 1, &val_rat);
    if (rc) {
      ALOGE("%s:%d]: Error adding Exif Entry", __func__, __LINE__);
    }

    val_rat.num = (uint32_t)(p_sensor_params->aperture_value * 100);
    val_rat.denom = 100;
    rc = addExifEntry(exif_info, EXIFTAGID_F_NUMBER, EXIF_RATIONAL, 1, &val_rat);
    if (rc) {
      ALOGE("%s:%d]: Error adding Exif Entry", __func__, __LINE__);
    }
  }

  /*Flash*/
  short val_short;
  if (p_sensor_params->flash_state == CAM_FLASH_STATE_FIRED) {
    val_short = 1;
  } else {
    val_short = 0;
  }
  //val_short =  (p_sensor_params->flash_mode << 3) | val_short;
  ALOGI("%s: Flash value %d flash mode %d flash state %d", __func__, val_short,
    p_sensor_params->flash_mode, p_sensor_params->flash_state);
  rc = addExifEntry(exif_info, EXIFTAGID_FLASH, EXIF_SHORT, 1, &val_short);
  if (rc) {
    ALOGE("%s %d]: Error adding flash exif entry", __func__, __LINE__);
  }
  return rc;
}
/** process_3a_data:
 *
 *  Arguments:
 *   @p_3a_params : ptr to 3a data
 *
 *  Return     : int32_t type of status
 *               NO_ERROR  -- success
 *              none-zero failure code
 *
 *  Description:
 *       process 3a data
 *
 *  Notes: this needs to be filled for the metadata
 **/
int process_3a_data(cam_3a_params_t *p_3a_params, QOMX_EXIF_INFO *exif_info)
{
  int rc = 0;
  srat_t val_srat;
  rat_t val_rat;
  double shutter_speed_value;

  if (NULL == p_3a_params) {
    ALOGE("%s %d: 3A params are null", __func__, __LINE__);
    return 0;
  }

  ALOGD("%s:%d] exp_time %f, iso_value %d, wb_mode %d", __func__, __LINE__,
    p_3a_params->exp_time, p_3a_params->iso_value, p_3a_params->wb_mode);

  /*Exposure time*/
  if (p_3a_params->exp_time == 0) {
      val_rat.num = 0;
      val_rat.denom = 0;
  } else {
      val_rat.num = 1;
      val_rat.denom = ROUND(1.0/p_3a_params->exp_time);
  }
  ALOGD("%s: numer %d denom %d", __func__, val_rat.num, val_rat.denom );

  rc = addExifEntry(exif_info, EXIFTAGID_EXPOSURE_TIME, EXIF_RATIONAL,
    (sizeof(val_rat)/(8)), &val_rat);
  if (rc) {
    ALOGE("%s:%d]: Error adding Exif Entry Exposure time",
      __func__, __LINE__);
  }

  /* Shutter Speed*/
  if (p_3a_params->exp_time > 0) {
    shutter_speed_value = log10(1/p_3a_params->exp_time)/log10(2);
    val_srat.num = shutter_speed_value * 1000;
    val_srat.denom = 1000;
  } else {
    val_srat.num = 0;
    val_srat.denom = 0;
  }
  rc = addExifEntry(exif_info, EXIFTAGID_SHUTTER_SPEED, EXIF_SRATIONAL,
    (sizeof(val_srat)/(8)), &val_srat);
  if (rc) {
    ALOGE("%s:%d]: Error adding Exif Entry", __func__, __LINE__);
  }

  /*ISO*/
  short val_short;
  val_short = p_3a_params->iso_value;
  rc = addExifEntry(exif_info, EXIFTAGID_ISO_SPEED_RATING, EXIF_SHORT,
    sizeof(val_short)/2, &val_short);
  if (rc) {
    ALOGE("%s:%d]: Error adding Exif Entry", __func__, __LINE__);
  }

  /*WB mode*/
  if (p_3a_params->wb_mode == CAM_WB_MODE_AUTO)
    val_short = 0;
  else
    val_short = 1;
  rc = addExifEntry(exif_info, EXIFTAGID_WHITE_BALANCE, EXIF_SHORT,
    sizeof(val_short)/2, &val_short);
  if (rc) {
    ALOGE("%s:%d]: Error adding Exif Entry", __func__, __LINE__);
  }

  return rc;
}

/** process_meta_data_v1:
 *
 *  Arguments:
 *   @p_meta : ptr to metadata
 *   @exif_info: Exif info struct
 *
 *  Return     : int32_t type of status
 *               NO_ERROR  -- success
 *              none-zero failure code
 *
 *  Description:
 *       process awb debug info
 *
 **/
int process_meta_data_v1(cam_metadata_info_t *p_meta, QOMX_EXIF_INFO *exif_info,
  mm_jpeg_exif_params_t *p_cam_exif_params)
{
  int rc = 0;

  if (!p_meta) {
    ALOGE("%s %d:Meta data is NULL", __func__, __LINE__);
    return 0;
  }
  cam_3a_params_t *p_3a_params = p_meta->is_3a_params_valid ?
    &p_meta->cam_3a_params : NULL;

  if (NULL != p_3a_params) {
    rc = process_3a_data(p_3a_params, exif_info);
    if (rc) {
      ALOGE("%s %d: Failed to extract 3a params", __func__, __LINE__);
    }
  }
  cam_sensor_params_t *p_sensor_params = p_meta->is_sensor_params_valid ?
    &p_meta->sensor_params : NULL;

  if (NULL != p_sensor_params) {
    rc = process_sensor_data(p_sensor_params, exif_info);
    if (rc) {
      ALOGE("%s %d: Failed to extract sensor params", __func__, __LINE__);
    }
  }
  return rc;
}

/** process_meta_data_v3:
 *
 *  Arguments:
 *   @p_meta : ptr to metadata
 *   @exif_info: Exif info struct
 *
 *  Return     : int32_t type of status
 *               NO_ERROR  -- success
 *              none-zero failure code
 *
 *  Description:
 *       Extract exif data from the metadata
 **/
int process_meta_data_v3(metadata_buffer_t *p_meta, QOMX_EXIF_INFO *exif_info,
  mm_jpeg_exif_params_t *p_cam_exif_params)
{
  int rc = 0;
  cam_sensor_params_t p_sensor_params;
  cam_3a_params_t p_3a_params;

  if (!p_meta) {
    ALOGE("%s %d:Meta data is NULL", __func__, __LINE__);
    return 0;
  }

  /* Process 3a data */
  int32_t *iso =
    (int32_t *)POINTER_OF(CAM_INTF_META_SENSOR_SENSITIVITY, p_meta);

  int64_t *sensor_exposure_time =
    (int64_t *)POINTER_OF(CAM_INTF_META_SENSOR_EXPOSURE_TIME, p_meta);

  cam_wb_mode_type *wb_mode =
    (cam_wb_mode_type *)POINTER_OF(CAM_INTF_PARM_WHITE_BALANCE, p_meta);

  memset(&p_3a_params,  0,  sizeof(cam_3a_params_t));
  if (NULL != iso) {
    p_3a_params.iso_value= *iso;
  } else {
    ALOGE("%s: Cannot extract Iso value", __func__);
  }

  if (NULL != sensor_exposure_time) {
    p_3a_params.exp_time = (double)(*sensor_exposure_time / 1000000000.0);
  } else {
    ALOGE("%s: Cannot extract Exp time value", __func__);
  }

  if (NULL != wb_mode) {
    p_3a_params.wb_mode = *wb_mode;
  } else {
    ALOGE("%s: Cannot extract white balance mode", __func__);
  }

  rc = process_3a_data(&p_3a_params, exif_info);
  if (rc) {
    ALOGE("%s %d: Failed to add 3a exif params", __func__, __LINE__);
  }

  /* Process sensor data */
  float *aperture = (float *)POINTER_OF(CAM_INTF_META_LENS_APERTURE, p_meta);
  uint8_t *flash_mode = (uint8_t *) POINTER_OF(CAM_INTF_META_FLASH_MODE, p_meta);
  uint8_t *flash_state =
    (uint8_t *) POINTER_OF(CAM_INTF_META_FLASH_STATE, p_meta);

  memset(&p_sensor_params, 0, sizeof(cam_sensor_params_t));

  if (NULL != aperture) {
     p_sensor_params.aperture_value = *aperture;
  } else {
    ALOGE("%s: Cannot extract Aperture value", __func__);
  }

  if (NULL != flash_mode) {
     p_sensor_params.flash_mode = *flash_mode;
  } else {
    ALOGE("%s: Cannot extract flash mode value", __func__);
  }

  if (NULL != flash_state) {
    p_sensor_params.flash_state = *flash_state;
  } else {
    ALOGE("%s: Cannot extract flash state value", __func__);
  }

  rc = process_sensor_data(&p_sensor_params, exif_info);
  if (rc) {
      ALOGE("%s %d: Failed to extract sensor params", __func__, __LINE__);
  }

  return rc;
}
