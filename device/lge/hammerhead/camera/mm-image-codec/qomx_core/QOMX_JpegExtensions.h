/*Copyright (c) 2012, The Linux Foundation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of The Linux Foundation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.*/

#ifndef __QOMX_EXTENSIONS_H__
#define __QOMX_EXTENSIONS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <OMX_Image.h>
#include <qexif.h>

/** qomx_image_eventd
*  Qcom specific events extended from OMX_EVENT
*  @ OMX_EVENT_THUMBNAIL_DROPPED - Indicates that the thumbnail
*                                 size id too big to be included
*                                 in the exif and will be
*                                 dropped
**/
typedef enum {
 OMX_EVENT_THUMBNAIL_DROPPED = OMX_EventVendorStartUnused+1
} QOMX_IMAGE_EXT_EVENTS;

/**
*  The following macros defines the string to be used for
*  getting the extension indices.
**/
#define QOMX_IMAGE_EXT_EXIF_NAME                  "OMX.QCOM.image.exttype.exif"
#define QOMX_IMAGE_EXT_THUMBNAIL_NAME        "OMX.QCOM.image.exttype.thumbnail"
#define QOMX_IMAGE_EXT_BUFFER_OFFSET_NAME "OMX.QCOM.image.exttype.bufferOffset"
#define QOMX_IMAGE_EXT_MOBICAT_NAME            "OMX.QCOM.image.exttype.mobicat"
#define QOMX_IMAGE_EXT_ENCODING_MODE_NAME        "OMX.QCOM.image.encoding.mode"
#define QOMX_IMAGE_EXT_WORK_BUFFER_NAME      "OMX.QCOM.image.exttype.workbuffer"

/** QOMX_IMAGE_EXT_INDEXTYPE
*  This enum is an extension of the OMX_INDEXTYPE enum and
*  specifies Qcom supported extention indexes. These indexes are
*  associated with the extension names and can be used as
*  Indexes in the SetParameter and Getparameter functins to set
*  or get values from qcom specific data structures
**/
typedef enum {
  //Name: OMX.QCOM.image.exttype.exif
  QOMX_IMAGE_EXT_EXIF = 0x07F00000,

  //Name: OMX.QCOM.image.exttype.thumbnail
  QOMX_IMAGE_EXT_THUMBNAIL = 0x07F00001,

  //Name: OMX.QCOM.image.exttype.bufferOffset
  QOMX_IMAGE_EXT_BUFFER_OFFSET = 0x07F00002,

  //Name: OMX.QCOM.image.exttype.mobicat
  QOMX_IMAGE_EXT_MOBICAT = 0x07F00003,

  //Name: OMX.QCOM.image.encoding.approach
  QOMX_IMAGE_EXT_ENCODING_MODE = 0x07F00004,

  //Name: OMX.QCOM.image.exttype.workbuffer
  QOMX_IMAGE_EXT_WORK_BUFFER = 0x07F00004,

} QOMX_IMAGE_EXT_INDEXTYPE;

/** QOMX_BUFFER_INFO
*  The structure specifies informaton
*   associated with the buffers and should be passed as appData
*   in UseBuffer calls to the OMX component with buffer specific
*   data. @ fd - FD of the buffer allocated. If the buffer is
*          allocated on the heap, it can be zero.
*   @offset - Buffer offset
**/

typedef struct {
  OMX_U32 fd;
  OMX_U32 offset;
} QOMX_BUFFER_INFO;

/** QEXIF_INFO_DATA
*   The basic exif structure used to construct
*   information for a single exif tag.
*   @tag_entry
*   @tag_id
**/
typedef struct{
  exif_tag_entry_t tag_entry;
  exif_tag_id_t tag_id;
} QEXIF_INFO_DATA;

/**QOMX_EXIF_INFO
*  The structure contains an array of exif tag
*  structures(qexif_info_data) and should be passed to the OMX
*  layer by the OMX client using the extension index.
*  @exif_data - Array of exif tags
*  @numOfEntries - Number of exif tags entries being passed in
*                 the array
**/
typedef struct {
  QEXIF_INFO_DATA *exif_data;
  OMX_U32 numOfEntries;
} QOMX_EXIF_INFO;

/**QOMX_YUV_FRAME_INFO
*  The structre contains all the offsets
*  associated with the Y and cbcr buffers.
*  @yOffset - Offset within the Y buffer
*  @cbcrOffset - Offset within the cb/cr buffer. The array
*                should be populated in order depending on cb
*                first or cr first in case of planar data. For
*                pseusoplanar, only the first array element
*                needs to be filled and the secnd element should
*                be set to zero.
*  @cbcrStartOffset - Start offset of the cb/cr buffer starting
*                     starting from the Y buffer. The array
*                     should be populated in order depending on
*                     cb first or cr first in case of planar
*                     data. For pseusoplanar, only the first
*                     array element needs to be filled and the
*                     secnd element should be set to zero.
**/
typedef struct {
  OMX_U32 yOffset;
  OMX_U32 cbcrOffset[2];
  OMX_U32 cbcrStartOffset[2];
} QOMX_YUV_FRAME_INFO;

/** qomx_thumbnail_info
*  Includes all information associated with the thumbnail
*  @input_width - Width of the input thumbnail buffer
*  @input_height - Heighr of the input thumbnail buffer
*  @scaling_enabled - Flag indicating if thumbnail scaling is
*  enabled.
*  @quality - JPEG Q factor value in the range of 1-100. A factor of 1
 *               produces the smallest, worst quality images, and a factor
 *               of 100 produces the largest, best quality images.  A
 *               typical default is 75 for small good quality images.
*  @crop_info - Includes the crop width, crop height,
*               horizontal and vertical offsets.
*  @output_width - Output Width of the the thumbnail. This is
*                the width after scaling if scaling is enabled
*                or width after cropping if only cropping is
*                enabled or same same input width otherwise
*  @output_height - Output height of the thumbnail. This is
*                the height after scaling if scaling is enabled
*                or height after cropping if only cropping is
*                enabled or same same input height otherwise
**/
typedef struct {
  OMX_U32 input_width;
  OMX_U32 input_height;
  OMX_U8 scaling_enabled;
  OMX_U32 quality;
  OMX_CONFIG_RECTTYPE crop_info;
  OMX_U32 output_width;
  OMX_U32 output_height;
  QOMX_YUV_FRAME_INFO tmbOffset;
} QOMX_THUMBNAIL_INFO;

/**qomx_mobicat
*  Mobicat data to padded tot he OMX layer
*  @mobicatData - Mobicate data
*  @mobicatDataLength - length of the mobicat data
**/
typedef struct {
  OMX_U8 *mobicatData;
  OMX_U32 mobicatDataLength;
} QOMX_MOBICAT;

/**qomx_workbuffer
*  Ion buffer to be used for the H/W encoder
*  @fd - FD of the buffer allocated
*  @vaddr - Buffer address
**/
typedef struct {
  int fd;
  uint8_t *vaddr;
} QOMX_WORK_BUFFER;

/** QOMX_IMG_COLOR_FORMATTYPE
*  This enum is an extension of the OMX_COLOR_FORMATTYPE enum.
*  It specifies Qcom supported color formats.
**/
typedef enum QOMX_IMG_COLOR_FORMATTYPE {
  OMX_QCOM_IMG_COLOR_FormatYVU420SemiPlanar = OMX_COLOR_FormatVendorStartUnused + 0x300,
  OMX_QCOM_IMG_COLOR_FormatYVU422SemiPlanar,
  OMX_QCOM_IMG_COLOR_FormatYVU422SemiPlanar_h1v2,
  OMX_QCOM_IMG_COLOR_FormatYUV422SemiPlanar_h1v2,
  OMX_QCOM_IMG_COLOR_FormatYVU444SemiPlanar,
  OMX_QCOM_IMG_COLOR_FormatYUV444SemiPlanar,
  OMX_QCOM_IMG_COLOR_FormatYVU420Planar,
  OMX_QCOM_IMG_COLOR_FormatYVU422Planar,
  OMX_QCOM_IMG_COLOR_FormatYVU422Planar_h1v2,
  OMX_QCOM_IMG_COLOR_FormatYUV422Planar_h1v2,
  OMX_QCOM_IMG_COLOR_FormatYVU444Planar,
  OMX_QCOM_IMG_COLOR_FormatYUV444Planar
} QOMX_IMG_COLOR_FORMATTYPE;

/** QOMX_ENCODING_MODE
*  This enum is used to select parallel encoding
*  or sequential encoding for the thumbnail and
*  main image
**/
typedef enum {
  OMX_Serial_Encoding,
  OMX_Parallel_Encoding
} QOMX_ENCODING_MODE;

#ifdef __cplusplus
 }
#endif

#endif
