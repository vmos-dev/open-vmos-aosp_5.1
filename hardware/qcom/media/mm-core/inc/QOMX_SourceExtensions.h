/*--------------------------------------------------------------------------
Copyright (c) 2011-2012 The Linux Foundation. All rights reserved.

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

#ifndef __H_QOMX_SOURCEEXTENSIONS_H__
#define __H_QOMX_SOURCEEXTENSIONS_H__
/*========================================================================
*//** @file QOMX_SourceExtensions.h

@par FILE SERVICES:
    Qualcomm extensions API for OpenMax IL demuxer component.

    This file contains the description of the Qualcomm OpenMax IL
    demuxer component extention interface, through which the IL client and
    OpenMax components can access additional capabilities of the demuxer.

*//*====================================================================== */


/*========================================================================
                     INCLUDE FILES FOR MODULE
========================================================================== */
#include <OMX_Core.h>
/*========================================================================
                      DEFINITIONS AND DECLARATIONS
========================================================================== */

#if defined( __cplusplus )
extern "C"
{
#endif /* end of macro __cplusplus */
/* Frame size query supported extension string */
#define OMX_QCOM_INDEX_PARAM_FRAMESIZEQUERYSUPPORTED       "OMX.QCOM.index.param.FrameSizeQuerySupported"      /**< reference: QOMX_FRAMESIZETYPE */

/* Content interface extension strings */
#define OMX_QCOM_INDEX_PARAM_CONTENTINTERFACE_IXSTREAM     "OMX.QCOM.index.param.contentinterface.ixstream"    /**< reference: QOMX_CONTENTINTERFACETYPE*/
#define OMX_QCOM_INDEX_PARAM_CONTENTINTERFACE_ISTREAMPORT  "OMX.QCOM.index.param.contentinterface.istreamport" /**< reference: QOMX_CONTENTINTERFACETYPE*/

/* Source seek access extension string */
#define OMX_QCOM_INDEX_PARAM_SEEK_ACCESS            "OMX.QCOM.index.param.SeekAccess"                   /**< reference: QOMX_PARAM_SEEKACCESSTYPE*/

/* Media duration extension string*/
#define OMX_QCOM_INDEX_CONFIG_MEDIADURATION                "OMX.QCOM.index.config.MediaDuration"               /**< reference: OMX_TIME_CONFIG_MEDIADURATIONTYPE*/

/**
 *  Data interface Params
 *
 *  STRUCT MEMBERS:
 *  nSize          : Size of the structure in bytes
 *  nVersion       : OMX specification version information
 *  nInterfaceSize : Size of the data pointed by pInterface
 *  pInterface     : Interface pointer
 */
typedef struct QOMX_CONTENTINTERFACETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nInterfaceSize;
    OMX_U8 pInterface[1];
} QOMX_DATAINTERFACETYPE;

/**
 *  Seek Access Parameters
 *
 *  STRUCT MEMBERS:
 *  nSize          : Size of the structure in bytes
 *  nVersion       : OMX specification version information
 *  nPortIndex     : Index of port
 *  bSeekAllowed   : Flag to indicate whether seek is supported or not
 */
typedef struct QOMX_PARAM_SEEKACCESSTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bSeekAllowed;
} QOMX_PARAM_SEEKACCESSTYPE;

/**
 *  Media Duration parameters
 *
 *  STRUCT MEMBERS:
 *  nSize          : Size of the structure in bytes
 *  nVersion       : OMX specification version information
 *  nPortIndex     : Index of port
 *  nDuration      : Total duration of the media
*/
typedef struct OMX_TIME_CONFIG_MEDIADURATIONTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_TICKS nDuration;
} OMX_TIME_CONFIG_MEDIADURATIONTYPE;

/**
 *  The parameters for QOMX_FRAMESIZETYPE are defined as
 *  follows:
 *
 *  STRUCT MEMBERS:
 *  nSize           : Size of the structure in bytes
 *  nVersion        : OMX specification version information
 *  nPortIndex      : Represents the port that this structure
 *                    applies to
 *  sFrameSize      : Indicates the size of the frame
 *  nFrameSizeIndex : Enumerates the possible frame sizes for
 *                    the given session/URL configuration. The
 *                    caller specifies all fields and the
 *                    OMX_GetParameter call returns the value of
 *                    the frame size. The value of
 *                    nFrameSizeIndex goes from 0 to N-1, where
 *                    N is the number of frame sizes that may be
 *                    emitted by the port. The port does not
 *                    need to report N as the caller can
 *                    determine N by enumerating all the frame
 *                    sizes supported by the port. If the port
 *                    does not have advance knowledge of the
 *                    possible frame sizes, it may report no
 *                    frame sizes. If there are no more frame
 *                    sizes, OMX_GetParameter returns
 *                    OMX_ErrorNoMore.
 */
typedef struct QOMX_FRAMESIZETYPE
{
  OMX_U32 nSize;
  OMX_VERSIONTYPE nVersion;
  OMX_U32 nPortIndex;
  OMX_FRAMESIZETYPE sFrameSize;
  OMX_U32 nFrameSizeIndex;
} QOMX_FRAMESIZETYPE;

#if defined( __cplusplus )
}
#endif /* end of macro __cplusplus */

#endif /* end of macro __H_QOMX_SOURCEEXTENSIONS_H__ */
