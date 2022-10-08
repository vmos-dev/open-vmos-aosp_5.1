
/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
/* ====================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
* ==================================================================== */

/** LCML_codecinterface.h
 *  The LCML header file contains the interface used by 
 *  and the component to access common items.
 */

#ifndef __LCML_CODECINTERFACE_H__
#define __LCML_CODECINTERFACE_H__

#include <OMX_Core.h>
#include <OMX_TI_Debug.h>
/** 
 * Commands to send messages to codec 
 */
typedef enum 
{
    EMMCodecControlSendDspMessage,
    EMMCodecControlPause,
    EMMCodecControlStart,
    MMCodecControlStop,
    EMMCodecControlDestroy,
    EMMCodecControlAlgCtrl,
    EMMCodecControlStrmCtrl,
    EMMCodecControlUsnEos
}TControlCmd;


/**
 * Type fo buffer ENUM
 */
typedef enum 
{
    /*EMMCodecInputBuffer,
    EMMCodecOuputBuffer,
    EMMCodecScratchBuffer*/
    EMMCodecInputBufferMapBufLen  = 1000,
    EMMCodecOutputBufferMapBufLen  = 1001,
    EMMCodecInputBufferMapReuse  = 1002,
    EMMCodecOutputBufferMapReuse  = 1003,
    EMMCodecInputBuffer  = 2000,
    EMMCodecStream0      = 2000,
    EMMCodecOuputBuffer  = 2001,
    EMMCodecStream1      = 2001,
    EMMCodecStream2,
    EMMCodecStream3,
    EMMCodecStream4,
    EMMCodecStream5,
    EMMCodecStream6,
    EMMCodecStream7,
    EMMCodecStream8,
    EMMCodecStream9,
    EMMCodecStream10,
    EMMCodecStream11,
    EMMCodecStream12,
    EMMCodecStream13,
    EMMCodecStream14,
    EMMCodecStream15,
    EMMCodecStream16,
    EMMCodecStream17,
    EMMCodecStream18,
    EMMCodecStream19,
    EMMCodecStream20
}TMMCodecBufferType;


/**
 * Generic interface provided to write and codec needs to implement all 
 * function present to use this structure
 */
typedef struct LCML_CODEC_INTERFACE
{
    OMX_ERRORTYPE (*InitMMCodec)(OMX_HANDLETYPE hComponent,
                                 OMX_STRING codecName,
                                 void *toCodecInitParams,
                                 void *fromCodecInfoStruct,
                                 LCML_CALLBACKTYPE *pCallbacks);
    
    OMX_ERRORTYPE (*InitMMCodecEx)(OMX_HANDLETYPE hComponent,
                                   OMX_STRING  codecName,
                                   void *toCodecInitParams,
                                   void *fromCodecInfoStruct,
                                   LCML_CALLBACKTYPE *pCallbacks,
                                   OMX_STRING  Args);
                                   
    OMX_ERRORTYPE (*WaitForEvent)(OMX_HANDLETYPE hComponent,
                                  TUsnCodecEvent event, 
                                  void *args[10] );

    OMX_ERRORTYPE (*QueueBuffer)(OMX_HANDLETYPE hComponent,
                                 TMMCodecBufferType bufType,
                                 OMX_U8 *buffer, 
                                 OMX_S32 bufferLen,
                                 OMX_S32 bufferSizeUsed,
                                 OMX_U8 *auxInfo,
                                 OMX_S32 auxInfoLen,
                                 OMX_U8 *usrArg);

    OMX_ERRORTYPE (*ControlCodec)(OMX_HANDLETYPE hComponent,
                                  TControlCmd iCodecCmd,
                                  void *args [10]);

    OMX_PTR pCodecPrivate;
    OMX_HANDLETYPE pCodec;
    struct OMX_TI_Debug dbg;

}LCML_CODEC_INTERFACE;

#endif /* __MMCODECINTERFACE_H__ */

