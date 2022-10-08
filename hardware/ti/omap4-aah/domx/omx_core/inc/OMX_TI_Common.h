/*
 * Copyright (c) 2010, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 *  @file  omx_ti_common.h
 *
 *
 *  @path \OMAPSW_SysDev\multimedia\omx\khronos1_1\omx_core\inc
 *
 *  @rev 1.0
 */

/*==============================================================
 *! Revision History
 *! ============================
 *! 20-Dec-2008 x0052661@ti.com, initial version
 *================================================================*/

#ifndef _OMX_TI_COMMON_H_
#define _OMX_TI_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/******************************************************************
 *   INCLUDE FILES
 ******************************************************************/
#include <OMX_Core.h>

/*******************************************************************
 * EXTERNAL REFERENCE NOTE: only use if not found in header file
 *******************************************************************/
/*----------         function prototypes      ------------------- */
/*----------         data declarations        ------------------- */
/*******************************************************************
 * PUBLIC DECLARATIONS: defined here, used elsewhere
 *******************************************************************/
/*----------         function prototypes      ------------------- */
/*----------         data declarations        ------------------- */
#define OMX_BUFFERHEADERFLAG_MODIFIED 0x00000100

#define OMX_TI_BUFFERFLAG_READONLY 0x00000200

/* TI extra data will be passed in the platform private structure rather than
 * appended at the end of the buffer. Adding a new custom flag to indicate
 * this.*/
#define OMX_TI_BUFFERFLAG_DETACHEDEXTRADATA 0x00000400

typedef struct OMX_CONFIG_CHANNELNAME {

	OMX_U32 nSize; /**< Size of the structure in bytes */
	OMX_VERSIONTYPE nVersion; /**< OMX specification version information */
	OMX_U32 nPortIndex; /**< Index of the port */
	OMX_STRING cChannelName; /**< Channel name  */

} OMX_CONFIG_CHANNELNAME;

typedef struct OMX_TI_PLATFORMPRIVATE
{
	OMX_U32 nSize;
	OMX_PTR pExtendedPlatformPrivate;
	OMX_BOOL bReadViaCPU;
	OMX_BOOL bWriteViaCPU;
	OMX_PTR pMetaDataBuffer;
	OMX_U32 nMetaDataSize;
	OMX_PTR pAuxBuf1;
	OMX_U32 pAuxBufSize1;
}OMX_TI_PLATFORMPRIVATE;

/*===============================================================*/
/** OMX_TI_PARAM_BUFFERPREANNOUNCE    : This parameter is used to enable/disable
 *                                      buffer pre announcement. Buffer pre
 *                                      announcement is enabled by default i.e.
 *                                      if buffer is being allocated by client
 *                                      then the buffer ptrs will be made known
 *                                      to the component at the time of
 *                                      UseBuffer and these will not change in
 *                                      lifetime of the component. If pre
 *                                      announcement is disabled then new
 *                                      buffers can be allocated by the client
 *                                      at runtime and passed in ETB/FTB. This
 *                                      parameter is valid only in cases where
 *                                      client allocates the buffer
 *                                      (i.e. UseBuffer cases).
 *
 *  @ param nSize                     : Size of the structure.
 *  @ param nVersion                  : Version.
 *  @ param nPortIndex                : Port index on which the parameter will
 *                                      be applied.
 *  @ param bEnabled                  : Whether buffer pre announcement is
 *                                      enabled or not. Set to TRUE (enabled)
 *                                      by default.
 */
/*===============================================================*/
typedef struct OMX_TI_PARAM_BUFFERPREANNOUNCE
{
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bEnabled;
}OMX_TI_PARAM_BUFFERPREANNOUNCE;



/*===============================================================*/
/** OMX_TI_CONFIG_BUFFERREFCOUNTNOTIFYTYPE : This config is used to
 *                                           enable/disable notification when
 *                                           reference count of a buffer changes
 *                                           This happens usually when buffers
 *                                           are locked/unlocked by codecs. By
 *                                           DEFAULT all notifications are
 *                                           DISABLED.
 *
 *  @ param nSize                          : Size of the structure.
 *  @ param nVersion                       : Version.
 *  @ param nPortIndex                     : Port index on which the config will
 *                                           be applied.
 *  @param bNotifyOnIncrease              : Enable notification when reference
 *                                           count is increased.
 *  @ param bNotifyOnDecrease              : Enable notification when reference
 *                                           count is decreased.
 *  @ param nCountForNotification          : Count at which to trigger
 *                                           notification. Count indicates no.
 *                                           of copies of the buffer in
 *                                           circulation e.g.
 *                                           1 - Only the original buffer is in
 *                                               circulation i.e. no buffers are
 *                                               currently locked.
 *                                           2 - There are two copies of the
 *                                               buffer i.e. one original and
 *                                               one copy which has been locked
 *                                               by the codec.
 *                                           And so on
 *                                           SPECIAL CASE
 *                                           0 - Indicates notify always
 *                                               irrespective of count value.
 */
/*===============================================================*/
typedef struct OMX_TI_CONFIG_BUFFERREFCOUNTNOTIFYTYPE
{
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bNotifyOnIncrease;
    OMX_BOOL bNotifyOnDecrease;
    OMX_U32 nCountForNotification;
}OMX_TI_CONFIG_BUFFERREFCOUNTNOTIFYTYPE;





/* OMX_TI_SEVERITYTYPE enumeration is used to indicate severity level of errors returned by TI OpenMax components.
   Critcal      Requires reboot/reset DSP
   Severe       Have to unload components and free memory and try again
   Major        Can be handled without unloading the component
   Minor        Essentially informational
*/
typedef enum OMX_TI_SEVERITYTYPE {
    OMX_TI_ErrorCritical=1,
    OMX_TI_ErrorSevere,
    OMX_TI_ErrorMajor,
    OMX_TI_ErrorMinor
} OMX_TI_SEVERITYTYPE;

/* ============================================================================= */
/*
@brief OMX_TI_PARAM_METADATABUFFERINFO : Structure to access metadata buffer info needed
         by proxy to allocate metadat buffers.

*/
/* ============================================================================= */
typedef struct OMX_TI_PARAM_METADATABUFFERINFO {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bIsMetaDataEnabledOnPort;
    OMX_U32 nMetaDataSize;
} OMX_TI_PARAM_METADATABUFFERINFO;

/*===============================================================*/
/** OMX_TI_BUFFERTYPE                 : This enumberation defines the type of
 *                                      buffer that is exchanged with the OMX
 *                                      component port
 *
 *  OMX_TI_BufferTypeDefault          : Default buffer type accessed via a 
 *                                      single virtual address
 *  OMX_TI_BufferTypeVirtual2D        : Multiple virtual buffers describing a
 *                                      2D buffer
 *  OMX_TI_BufferTypePlatform1D       : Platform specific 1D buffer handle
 *  OMX_TI_BufferTypePlatform2D       : Platform specific buffer handles
 *                                      describing a 2D buffer
 *  OMX_TI_BufferTypePhysicalPageList : List of a given number of physical pages
 *  OMX_TI_BufferTypeHardwareReserved1D:Harware reserve space only that can
 *                                      accomodate a 1D buffer by mapping memory
 *                                      to it
 */
/*===============================================================*/
typedef enum OMX_TI_BUFFERTYPE {
    OMX_TI_BufferTypeDefault = 0,
    OMX_TI_BufferTypeVirtual2D,
    OMX_TI_BufferTypePlatform1D,
    OMX_TI_BufferTypePlatform2D,
    OMX_TI_BufferTypePhysicalPageList,
    OMX_TI_BufferTypeHardwareReserved1D,
    OMX_TI_BufferTypeMax = 0x7FFFFFFF
} OMX_TI_BUFFERTYPE;

/*===============================================================*/
/** OMX_TI_BUFFERDESCRIPTOR_TYPE      : This buffer descriptor structure is used
 *                                      to convey additional buffer information
 *                                      when OMX_TI_IndexUseBufferDescriptor is
 *                                      enabled and it is passed via pBuffer
 *                                      in OMX_BUFFERHEADERTYPE
 *
 *  @ param nSize                     : Size of the structure.
 *  @ param eBufType                  : Specifies type of buffer
 *  @ param nNumOfBuf                 : Number of component buffers of eBufType
 *  @ param pBuf                      : Array of buffers of type eBufType
 */
/*===============================================================*/
typedef struct OMX_TI_BUFFERDESCRIPTOR_TYPE {
    OMX_U32 nSize;
    OMX_TI_BUFFERTYPE eBufType;
    OMX_U32 nNumOfBuf;
    OMX_PTR pBuf[3];
} OMX_TI_BUFFERDESCRIPTOR_TYPE;

/*===============================================================*/
/** OMX_TI_PARAM_USEBUFFERDESCRIPTOR  : This parameter is used to enable/disable
 *                                      buffer descriptor mode. When enabled,
 *                                      the pBuffer in OMX buffer header points
 *                                      to a buffer descriptor structure
 *                                      OMX_TI_BUFFERDESCRIPTOR_TYPE instead of
 *                                      the buffer directly.
 *
 *  @ param nSize                     : Size of the structure.
 *  @ param nVersion                  : Version.
 *  @ param nPortIndex                : Port index on which the parameter will
 *                                      be applied.
 *  @ param bEnabled                  : Whether buffer descriptor mode is
 *                                      enabled or not. Set to FALSE (disabled)
 *                                      by default.
 */
/*===============================================================*/
typedef struct OMX_TI_PARAM_USEBUFFERDESCRIPTOR {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bEnabled;
    OMX_TI_BUFFERTYPE eBufferType;
} OMX_TI_PARAM_USEBUFFERDESCRIPTOR;


/*===============================================================*/
/** OMX_TI_PARAM_COMPONENTBUFALLOCTYPE :This parameter is used to query/set
 *                                      internal buffers used by OMX component
 *                                      after allocation by the user of OMX
 *                                      component during regular OMX buffer
 *                                      allocation/free life cycle
 *
 *  @ param nSize                     : Size of the structure.
 *  @ param nVersion                  : Version.
 *  @ param nPortIndex                : Port index on which the parameter will
 *                                      be applied.
 *  @ param nIndex                    : Present buffer number whose requirement
 *                                      is queried and then set
 *  @ param eBufType                  : Present nIndex'ed buffer type
 *  @ param pBuf                      : Buffer communication
 *  @ param nAllocWidth               : Size of buffer (Width in case of 2D)
 *  @ param nAllocLines               : Size of buffer (1 in case of 1D)
 *  @ param nOffset                   : Offset from which buffer communicated is
 *                                      valid
 */
/*===============================================================*/
typedef struct OMX_TI_PARAM_COMPONENTBUFALLOCTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nIndex;
    OMX_TI_BUFFERTYPE eBufType;
    OMX_PTR pBuf[3];
    OMX_U32 nAllocWidth;
    OMX_U32 nAllocLines;
    OMX_U32 nOffset;
} OMX_TI_PARAM_COMPONENTBUFALLOCTYPE;

/*===============================================================*/
/** OMX_TI_COMPONENT_HANDLE           : This parameter is used to retrieve
 *                                      the component handle by the client.
 *
 *  @ param nSize                     : Size of the structure.
 *  @ param nVersion                  : Version.
 *  @ param pHandle                   : Component Handle
 */
/*===============================================================*/
typedef struct OMX_TI_COMPONENT_HANDLE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_HANDLETYPE pHandle;
} OMX_TI_COMPONENT_HANDLE;

/*******************************************************************
 * PRIVATE DECLARATIONS: defined here, used only here
 *******************************************************************/
/*----------          data declarations        ------------------- */
/*----------          function prototypes      ------------------- */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _OMX_TI_COMMON_H_ */
