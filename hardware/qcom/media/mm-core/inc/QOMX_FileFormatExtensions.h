/*--------------------------------------------------------------------------
Copyright (c) 2011 The Linux Foundation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of The Linux Foundation nor
      the names of its contributors may be used to endorse or promote
      products derived from this software without specific prior written
      permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
--------------------------------------------------------------------------*/

#ifndef __QOMX_FILE_FORMAT_EXTENSIONS_H__
#define __QOMX_FILE_FORMAT_EXTENSIONS_H__

/*============================================================================
*//** @file QOMX_FileFormatExtensions.h
  This header contains constants and type definitions that specify the 
  extensions added to the OpenMAX Vendor specific APIs.
*//*========================================================================*/

/*============================================================================
                              Edit History

when       who     what, where, why
--------   ---     -------------------------------------------------------

============================================================================*/

//////////////////////////////////////////////////////////////////////////////
//                             Include Files
//////////////////////////////////////////////////////////////////////////////
#include "OMX_Core.h"


/* :OMX.QCOM.index.param.container.info*/
#define QOMX_QcomIndexParamContainerInfo 0x7F000009

/**<OMX.Qualcomm.index.video.param.encrypttypeconfigparameters*/
#define QOMX_FilemuxIndexEncryptionTypeConfigParameters 0x7F00000A

#define QOMX_INDEX_CONTAINER_INFO_STRING    "QOMX.Qualcomm.index.param.containerinfo"
#define OMX_QCOM_INDEX_FILE_FORMAT          "OMX.QCOM.index.config.FileFormat"
#define QOMX_INDEX_CONFIG_ENCRYPT_TYPE      "QOMX.Qualcomm.index.config.EncryptType"

/**-----------------------------------------------------------------------------
            OMX.QCOM.index.param.container.info 
--------------------------------------------------------------------------------
*/

typedef enum QOMX_CONTAINER_FORMATTYPE {
    QOMX_FORMAT_RAW,
    QOMX_FORMAT_MP4,
    QOMX_FORMAT_3GP,
    QOMX_FORMAT_3G2,
    QOMX_FORMAT_AMC,
    QOMX_FORMAT_SKM,
    QOMX_FORMAT_K3G,
    QOMX_FORMAT_VOB,
    QOMX_FORMAT_AVI,
    QOMX_FORMAT_ASF,
    QOMX_FORMAT_RM ,
    QOMX_FORMAT_MPEG_ES,
    QOMX_FORMAT_DIVX,
    QOMX_FORMATMPEG_TS,
    QOMX_FORMAT_QT,
    QOMX_FORMAT_M4A,
    QOMX_FORMAT_MP3,
    QOMX_FORMAT_WAVE, 
    QOMX_FORMAT_XMF,
    QOMX_FORMAT_AMR,
    QOMX_FORMAT_AAC,
    QOMX_FORMAT_EVRC,
    QOMX_FORMAT_QCP,
    QOMX_FORMAT_SMF,
    QOMX_FORMAT_OGG,
    QOMX_FORMAT_BMP,
    QOMX_FORMAT_JPG,
    QOMX_FORMAT_JPG2000
}QOMX_CONTAINER_FORMATTYPE;

typedef struct QOMX_CONTAINER_INFOTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    QOMX_CONTAINER_FORMATTYPE eFmtType;
} QOMX_CONTAINER_INFOTYPE;

typedef enum QOMX_FILEFORMATTYPE {
    QOMX_FileFormatNone, /**< no file format naming convention is followed. */
    QOMX_FileFormatDCF, /**< DCF file naming convention. */
    QOMX_FileFormatMax = 0x7FFFFFFF
} QOMX_FILEFORMATTYPE;

/** QOMX_CONFIG_FILEFORMATTYPE is used to determine how the file writer will interpret
the provided content URI and whether it will increment the index of the file name. */
typedef struct QOMX_CONFIG_FILEFORMATTYPE {
    OMX_U32 nSize; /**< size of the structure in bytes */
    OMX_VERSIONTYPE nVersion; /**< OMX specification version information */
    OMX_U32 nPortIndex; /**< port that this structure applies to */
    QOMX_FILEFORMATTYPE eFileFormat; /** file format type */
} QOMX_CONFIG_FILEFORMATTYPE;

/**The QOMX_RECORDINGSTATISTICSINTERVALTYPE structure is used to enable
IL client to indicate the interval of the statistics notification to file mux
component. Time interval will indicate the frequency(in ms) when client needs 
the statistics data*/
typedef struct QOMX_RECORDINGSTATISTICSINTERVALTYPE {
    OMX_U32 nSize; /**< size of the structure in bytes */
    OMX_VERSIONTYPE nVersion;/**< OMX specification version information */
    OMX_TICKS  interval;/**< specifies the time(milliseconds) between updates */
   }QOMX_RECORDINGSTATISTICSINTERVALTYPE;

/**QOMX_RECORDINGSTATISTICSTYPE indicates the current recording
time and space statistics of this session, which can be used by client to 
identify current status of recorded data in milliseconds and bytes */
typedef struct QOMX_RECORDINGSTATISTICSTYPE {
    OMX_U32 nSize;/**< size of the structure in bytes */
    OMX_VERSIONTYPE nVersion;/**< OMX specification version information */
    OMX_TICKS  nRecordedTime; /**  duration that we already recorded*/
    OMX_TICKS  nTimeCanRecord;/** the time we can record at the same bitrate*/
    OMX_U64   nSpaceConsumed;/** space that consumed in bytes*/
    OMX_U64  nSpaceLeft;/** space left in bytes*/
} QOMX_RECORDINGSTATISTICSTYPE;

/**QOMX_ENCRYPT_TYPE indicates the type of encryption */
typedef enum QOMX_ENCRYPT_TYPE {
    QOMX_ENCRYPT_TYPE_HDCP,
    QOMX_ENCRYPT_TYPE_INVALID
}QOMX_ENCRYPT_TYPE;

/**QOMX_ENCRYPTIONTYPE indicates the encrypt type */
typedef struct QOMX_ENCRYPTIONTYPE {
    OMX_U32            nSize;  /**< size of the structure in bytes */
    OMX_VERSIONTYPE    nVersion; /**< OMX specification version information */
    OMX_BOOL           nStreamEncrypted;  /** stream is encrypted or not */
    QOMX_ENCRYPT_TYPE  nType;  /** type of Encryption */
    OMX_U32            nEncryptVersion; /** Encrypt version */
} QOMX_ENCRYPTIONTYPE;
#endif /*__QOMX_FILE_FORMAT_EXTENSIONS_H__*/
