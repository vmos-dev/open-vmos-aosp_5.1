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

#ifndef __H_QOMX_COREEXTENSIONS_H__
#define __H_QOMX_COREEXTENSIONS_H__ 


 
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
 * Qualcom vendor extensions.
 */
#define OMX_QCOM_INDEX_PARAM_INDEXEXTRADATA "OMX.QCOM.index.param.IndexExtraData" /**< reference: QOMX_INDEXEXTRADATATYPE */  
#define OMX_QCOM_INDEX_PARAM_HELDBUFFERCOUNT "OMX.QCOM.index.param.HeldBufferCount" /**< reference: QOMX_HELDBUFFERCOUNTTYPE */

/**
 * Buffer header nFlags field extension.
 *
 * The source of a stream sets the TIMESTAMPINVALID flag to
 * indicate that the buffer header nTimeStamp field does not
 * hold valid timestamp information. The component that updates
 * the nTimeStamp field to reflect a valid timestamp shall clear
 * this flag.
 */
#define QOMX_BUFFERFLAG_TIMESTAMPINVALID 0x80000000 

/**
 * Buffer header nFlags field extension.
 * 
 * The READONLY flag is set when the component emitting the
 * buffer on an output port identifies the buffer's contents to
 * be read-only. The IL client or input port that receives a
 * filled read-only buffer cannot alter the contents of the
 * buffer. This flag can be cleared by the component when the
 * emptied buffer is returned to it.
 */
#define QOMX_BUFFERFLAG_READONLY         0x40000000 

/**
 * Buffer header nFlags field extension.
 * 
 * The ENDOFSUBFRAME flag is an optional flag that is set by an
 * output port when the last byte that a buffer payload contains
 * is an end-of-subframe. Any component that implements setting
 * the ENDOFSUBFRAME flag on an output port shall set this flag
 * for every buffer sent from the output port containing an
 * end-of-subframe.
 * 
 * A subframe is defined by the next level of natural
 * partitioning in a logical unit for a given format. For
 * example, a subframe in an H.264 access unit is defined as the
 * "network abstraction layer" unit, or NAL unit.
 */
#define QOMX_BUFFERFLAG_ENDOFSUBFRAME    0x20000000 

/**
 * A component sends this error to the IL client (via the EventHandler callback)
 * in the event that application of a config or parameter has failed some time 
 * after the return of OMX_SetConfig or OMX_SetParameter. This may happen when a
 * component transitions between states and discovers some incompatibility
 * between multiple settings. Configuration indicies sent via extra data may also
 * fail when set to a down stream component. The index that failed will be
 * included as the nData2 parameter of the EventHandler callback.
 */
#define QOMX_ErrorAsyncIndexFailed (OMX_ErrorVendorStartUnused+1)

/* In some scenarios there may be a possibilty to run out of the storage space 
 * and components may want to notify this error to IL client to take appropriate
 * action by the IL client.
 *
 * For example, In recording scenario, MUX component can know the available 
 * space in the recording media and can compute peridically to accommodate the
 * meta data before we reach to a stage where we end up no space to write even 
 * the meta data. When the space limit reached in recording media, MUX component
 * would like to notify the IL client with  QOMX_ErrorSpaceLimitReached.
 * After this error all the buffers that are returned will have nFilledLen 
 * unchanges i.e not consumed.
 */
#define QOMX_ErrorStorageLimitReached (OMX_ErrorVendorStartUnused + 2)

/**
 * This structure is used to enable/disable the generation or
 * consumption of the QOMX_ExtraDataOMXIndex extra data type for
 * the specified OpenMax index.
 * 
 * STRUCT MEMBERS:
 *  nSize      : Size of the structure in bytes
 *  nVersion   : OMX specification version info
 *  nPortIndex : Port that this structure applies to
 *  bEnabled   : Enable/Disable the extra data processing
 *  nIndex     : The index associated with the extra data
 */
typedef struct QOMX_INDEXEXTRADATATYPE {
    OMX_U32 nSize;       
    OMX_VERSIONTYPE nVersion;   
    OMX_U32 nPortIndex; 
    OMX_BOOL bEnabled;
    OMX_INDEXTYPE nIndex;
} QOMX_INDEXEXTRADATATYPE;

/**
 * This structure is used to indicate the maximum number of buffers 
 * that a port will hold during data flow. 
 * 
 * STRUCT MEMBERS:
 *  nSize              : Size of the structure in bytes
 *  nVersion           : OMX specification version info
 *  nPortIndex         : Port that this structure applies to
 *  nHeldBufferCount   : Read-only, maximum number of buffers that will be held
 */
typedef struct QOMX_HELDBUFFERCOUNTTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nHeldBufferCount;
} QOMX_HELDBUFFERCOUNTTYPE;

#if defined( __cplusplus )
}
#endif /* end of macro __cplusplus */

#endif /* end of macro __H_QOMX_COREEXTENSIONS_H__ */
