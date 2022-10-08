/*--------------------------------------------------------------------------
Copyright (c) 2009, The Linux Foundation. All rights reserved.

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
/*============================================================================
                            O p e n M A X   w r a p p e r s
                             O p e n  M A X   C o r e

*//** @file QOMX_AudioExtensions.h
  This module contains the extensions for Audio

*//*========================================================================*/

#ifndef __H_QOMX_AUDIOEXTENSIONS_H__
#define __H_QOMX_AUDIOEXTENSIONS_H__

/*========================================================================

                     INCLUDE FILES FOR MODULE

========================================================================== */
#include <OMX_Audio.h>

/*========================================================================

                      DEFINITIONS AND DECLARATIONS

========================================================================== */

#if defined( __cplusplus )
extern "C"
{
#endif /* end of macro __cplusplus */

/* Audio extension strings */
#define OMX_QCOM_INDEX_PARAM_AMRWBPLUS       "OMX.Qualcomm.index.audio.amrwbplus"
#define OMX_QCOM_INDEX_PARAM_WMA10PRO        "OMX.Qualcomm.index.audio.wma10pro"
#define OMX_QCOM_INDEX_PARAM_SESSIONID       "OMX.Qualcomm.index.audio.sessionId"
#define OMX_QCOM_INDEX_PARAM_VOICERECORDTYPE "OMX.Qualcomm.index.audio.VoiceRecord"

typedef enum QOMX_AUDIO_AMRBANDMODETYPE {
    QOMX_AUDIO_AMRBandModeWB9              = 0x7F000001,/**< AMRWB Mode 9 = SID*/
    QOMX_AUDIO_AMRBandModeWB10             = 0x7F000002,/**< AMRWB Mode 10 = 13600 bps */
    QOMX_AUDIO_AMRBandModeWB11             = 0x7F000003,/**< AMRWB Mode 11 = 18000 bps */
    QOMX_AUDIO_AMRBandModeWB12             = 0x7F000004,/**< AMRWB Mode 12 = 24000 bps */
    QOMX_AUDIO_AMRBandModeWB13             = 0x7F000005,/**< AMRWB Mode 13 = 24000 bps */
    QOMX_AUDIO_AMRBandModeWB14             = 0x7F000006,/**< AMRWB Mode 14 = FRAME_ERASE*/
    QOMX_AUDIO_AMRBandModeWB15             = 0x7F000007,/**< AMRWB Mode 15 = NO_DATA */
}QOMX_AUDIO_AMRBANDMODETYPE;

/**
 * AMR WB PLUS type
 *
 *  STRUCT MEMBERS:
 *  nSize           : Size of the structure in bytes
 *  nVersion        : OMX specification version information
 *  nPortIndex      : Port that this structure applies to
 *  nChannels       : Number of channels
 *  nBitRate        : Bit rate read only field
 *  nSampleRate     : Sampling frequency for the clip(16/24/32/48KHz)
 *  eAMRBandMode    : AMR Band Mode enumeration
 *  eAMRDTXMode     : AMR DTX Mode enumeration
 *  eAMRFrameFormat : AMR frame format enumeration
 */

typedef struct QOMX_AUDIO_PARAM_AMRWBPLUSTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nChannels;
    OMX_U32 nBitRate;
    OMX_U32 nSampleRate;
    QOMX_AUDIO_AMRBANDMODETYPE   eAMRBandMode;
    OMX_AUDIO_AMRDTXMODETYPE     eAMRDTXMode;
    OMX_AUDIO_AMRFRAMEFORMATTYPE eAMRFrameFormat;
} QOMX_AUDIO_PARAM_AMRWBPLUSTYPE;

typedef enum QOMX_AUDIO_WMAFORMATTYPE {
    QOMX_AUDIO_WMAFormat10Pro = 0x7F000001, /**< Windows Media Audio format 10*/
} QOMX_AUDIO_WMAFORMATTYPE;

/**
 * WMA 10 PRO type
 *
 *  STRUCT MEMBERS:
 *  nSize              : Size of the structure in bytes
 *  nVersion           : OMX specification version information
 *  nPortIndex         : Port that this structure applies to
 *  nChannels          : Number of channels
 *  nBitRate           : Bit rate read only field
 *  eFormat            : Version of WMA stream / data
 *  eProfile           : Profile of WMA stream / data
 *  nSamplingRate      : Sampling rate of the source data
 *  nBlockAlign        : block alignment, or block size, in bytes of the audio codec
 *  nEncodeOptions     : WMA Type-specific data
 *  nSuperBlockAlign   : WMA Type-specific data
 *  validBitsPerSample : encoded stream (24-bit or 16-bit)
 *  formatTag          : codec ID(0x162 or 0x166)
 *  advancedEncodeOpt  : bit packed words indicating the features supported for LBR bitstream
 *  advancedEncodeOpt2 : bit packed words indicating the features supported for LBR bitstream
 */
typedef struct QOMX_AUDIO_PARAM_WMA10PROTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U16 nChannels;
    OMX_U32 nBitRate;
    QOMX_AUDIO_WMAFORMATTYPE eFormat;
    OMX_AUDIO_WMAPROFILETYPE eProfile;
    OMX_U32 nSamplingRate;
    OMX_U16 nBlockAlign;
    OMX_U16 nEncodeOptions;
    OMX_U32 nSuperBlockAlign;
    OMX_U32 validBitsPerSample;
    OMX_U32 formatTag;
    OMX_U32 advancedEncodeOpt;
    OMX_U32 advancedEncodeOpt2;
} QOMX_AUDIO_PARAM_WMA10PROTYPE;

/**
 * Stream info data
 *
 *  STRUCT MEMBERS:
 *  sessionId :  session Id for alsa to route data
 */
typedef struct QOMX_AUDIO_STREAM_INFO_DATA {
    OMX_U8  sessionId;
} QOMX_AUDIO_STREAM_INFO_DATA;


/**
 * Record Path
 *
 * STRUCT MEMBERS:
 * recPath : Record Path for encoding
 */
typedef enum{

QOMX_AUDIO_VOICE_TX,
QOMX_AUDIO_VOICE_RX,
QOMX_AUDIO_VOICE_MIXED,

} QOMX_AUDIO_VOICERECORDMODETYPE;
typedef struct QOMX_AUDIO_CONFIG_VOICERECORDTYPE {

OMX_U32                            nSize;
OMX_VERSIONTYPE                    nVersion;
QOMX_AUDIO_VOICERECORDMODETYPE     eVoiceRecordMode;
}  QOMX_AUDIO_CONFIG_VOICERECORDTYPE;




#if defined( __cplusplus )
}
#endif /* end of macro __cplusplus */

#endif /* end of macro __H_QOMX_AUDIOEXTENSIONS_H__ */
