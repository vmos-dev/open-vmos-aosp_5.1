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

*//** @file QOMX_AudioIndexExtensions.h
  This module contains the index extensions for Audio

*//*========================================================================*/


#ifndef __H_QOMX_AUDIOINDEXEXTENSIONS_H__
#define __H_QOMX_AUDIOINDEXEXTENSIONS_H__

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

/**
 * Enumeration used to define Qualcomm's vendor extensions for
 * audio. The audio extensions occupy a range of
 * 0x7F100000-0x7F1FFFFF, inclusive.
 */
typedef enum QOMX_AUDIO_EXTENSIONS_INDEXTYPE
{
    QOMX_IndexParamAudioAmrWbPlus       = 0x7F200000, /**< "OMX.Qualcomm.index.audio.amrwbplus" */
    QOMX_IndexParamAudioWma10Pro        = 0x7F200001, /**< "OMX.Qualcomm.index.audio.wma10pro" */
    QOMX_IndexParamAudioSessionId       = 0x7F200002, /**< "OMX.Qualcomm.index.audio.sessionId" */
    QOMX_IndexParamAudioVoiceRecord     = 0x7F200003, /**< "OMX.Qualcomm.index.audio.VoiceRecord" */
    QOMX_IndexConfigAudioDualMono       = 0x7F200004, /**< "OMX.Qualcomm.index.audio.dualmono" */
    QOMX_IndexParamAudioAc3             = 0x7F200005, /**< "OMX.Qualcomm.index.audio.ac3" */
    QOMX_IndexParamAudioAc3PostProc     = 0x7F200006, /**< "OMX.Qualcomm.index.audio.postproc.ac3" */
    QOMX_IndexParamAudioAacSelectMixCoef = 0x7F200007, /** "OMX.Qualcomm.index.audio.aac_sel_mix_coef**/
    QOMX_IndexParamAudioUnused          = 0x7F2FFFFF
} QOMX_AUDIO_EXTENSIONS_INDEXTYPE;

#if defined( __cplusplus )
}
#endif /* end of macro __cplusplus */

#endif /* end of macro __H_QOMX_AUDIOINDEXEXTENSIONS_H__ */
