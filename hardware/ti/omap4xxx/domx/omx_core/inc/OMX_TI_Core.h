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

/*
 *  @file  omx_ti_core.h
 *         This file contains the vendor(TI) specific core extensions
 *
 *  @path domx/system/omx_core/inc
 *
 *  @rev 1.0
 */

/*==============================================================
 *! Revision History
 *! ============================
 *! 19-Jul-2010 admonga@ti.com : Initial version
 *================================================================*/

#ifndef _OMX_TI_CORE_H_
#define _OMX_TI_CORE_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/******************************************************************
    INCLUDE FILES
 ******************************************************************/
#include <OMX_Types.h>
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



typedef enum OMX_TI_ERRORTYPE
{
    /* Vendor specific area for storing TI custom extended events */

    /*Control attribute is pending - Dio_Dequeue will not work until attribute
      is retreived*/
    OMX_TI_WarningAttributePending = (OMX_S32)((OMX_ERRORTYPE)OMX_ErrorVendorStartUnused + 1),
    /*Attribute buffer size is insufficient - reallocate the attribute buffer*/
    OMX_TI_WarningInsufficientAttributeSize,
    /*EOS buffer has been received*/
    OMX_TI_WarningEosReceived,
    /*Port enable is called on an already enabled port*/
    OMX_TI_ErrorPortIsAlreadyEnabled,
    /*Port disable is called on an already disabled port*/
    OMX_TI_ErrorPortIsAlreadyDisabled
} OMX_TI_ERRORTYPE;



typedef enum OMX_TI_EVENTTYPE
{
    /* Vendor specific area for storing indices */
    /*Reference count for the buffer has changed. In the callback, nData1 will
      pBufferHeader, nData2 will be present count*/
    OMX_TI_EventBufferRefCount = (OMX_S32)((OMX_EVENTTYPE)OMX_EventVendorStartUnused + 1)
}OMX_TI_EVENTTYPE;


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _OMX_TI_INDEX_H_ */

