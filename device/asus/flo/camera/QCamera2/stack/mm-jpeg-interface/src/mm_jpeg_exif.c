/* Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
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


#define LOWER(a)               ((a) & 0xFFFF)
#define UPPER(a)               (((a)>>16) & 0xFFFF)
#define CHANGE_ENDIAN_16(a)  ((0x00FF & ((a)>>8)) | (0xFF00 & ((a)<<8)))


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


int32_t releaseExifEntry(QOMX_EXIF_INFO *p_exif_info)
{
  uint32_t i = 0;
  for (i = 0; i < p_exif_info->numOfEntries; i++) {
  switch (p_exif_info->exif_data[i].tag_entry.type) {
  case EXIF_BYTE: {
    if (p_exif_info->exif_data[i].tag_entry.count > 1 &&
      p_exif_info->exif_data[i].tag_entry.data._bytes != NULL) {
      free(p_exif_info->exif_data[i].tag_entry.data._bytes);
      p_exif_info->exif_data[i].tag_entry.data._bytes = NULL;
    }
  }
  break;
  case EXIF_ASCII: {
    if (p_exif_info->exif_data[i].tag_entry.data._ascii != NULL) {
      free(p_exif_info->exif_data[i].tag_entry.data._ascii);
      p_exif_info->exif_data[i].tag_entry.data._ascii = NULL;
    }
  }
  break;
  case EXIF_SHORT: {
    if (p_exif_info->exif_data[i].tag_entry.count > 1 &&
      p_exif_info->exif_data[i].tag_entry.data._shorts != NULL) {
      free(p_exif_info->exif_data[i].tag_entry.data._shorts);
      p_exif_info->exif_data[i].tag_entry.data._shorts = NULL;
    }
  }
  break;
  case EXIF_LONG: {
    if (p_exif_info->exif_data[i].tag_entry.count > 1 &&
      p_exif_info->exif_data[i].tag_entry.data._longs != NULL) {
      free(p_exif_info->exif_data[i].tag_entry.data._longs);
      p_exif_info->exif_data[i].tag_entry.data._longs = NULL;
    }
  }
  break;
  case EXIF_RATIONAL: {
    if (p_exif_info->exif_data[i].tag_entry.count > 1 &&
      p_exif_info->exif_data[i].tag_entry.data._rats != NULL) {
      free(p_exif_info->exif_data[i].tag_entry.data._rats);
      p_exif_info->exif_data[i].tag_entry.data._rats = NULL;
    }
  }
  break;
  case EXIF_UNDEFINED: {
    if (p_exif_info->exif_data[i].tag_entry.data._undefined != NULL) {
      free(p_exif_info->exif_data[i].tag_entry.data._undefined);
      p_exif_info->exif_data[i].tag_entry.data._undefined = NULL;
    }
  }
  break;
  case EXIF_SLONG: {
    if (p_exif_info->exif_data[i].tag_entry.count > 1 &&
      p_exif_info->exif_data[i].tag_entry.data._slongs != NULL) {
      free(p_exif_info->exif_data[i].tag_entry.data._slongs);
      p_exif_info->exif_data[i].tag_entry.data._slongs = NULL;
    }
  }
  break;
  case EXIF_SRATIONAL: {
    if (p_exif_info->exif_data[i].tag_entry.count > 1 &&
      p_exif_info->exif_data[i].tag_entry.data._srats != NULL) {
      free(p_exif_info->exif_data[i].tag_entry.data._srats);
      p_exif_info->exif_data[i].tag_entry.data._srats = NULL;
    }
  }
  break;
  }

  } /*end of switch*/

  return 0;
}
