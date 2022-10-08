
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

/** LCML_dspcodec.h
 *  The LCML header file contains the definitions used by both the
 *  application and the component to access common items.
 */


#ifndef __LCML_DSPCODEC_H__
#define __LCML_DSPCODEC_H__

#define MAX_OBJS                10
#define MAX_STREAMS             10

/* 720p implementation */
#define MAX_DMM_BUFFERS 20

/*DSP specific*/
#define DSP_DOF_IMAGE           "baseimage.dof"
#define TI_PROCESSOR_DSP        0
#define LCML_MAX_NUM_OF_DLLs    10
#define END_OF_CR_PHASE_ARGS    0xFC25
#define LCML_DATA_SIZE          42
#define DMM_PAGE_SIZE           4096
#define QUEUE_SIZE              20
#define ROUND_TO_PAGESIZE(n)    ((((n)+4095)/DMM_PAGE_SIZE)*DMM_PAGE_SIZE)

#define __ERROR_PROPAGATION__


/*switch on/off here */
#ifndef UNDER_CE
#ifdef ANDROID
    #include <utils/Log.h>
#endif
#else
    #include <oaf_osal.h>
    #include <oaf_debug.h>
#endif


#ifdef __PERF_INSTRUMENTATION__
#include "perf.h"
#endif

#include <LCML_Types.h>
#include <LCML_CodecInterface.h>
#include <pthread.h>

/*DSP specific*/

#define DSP_ERROR_EXIT(err, msg, label)                \
    if (DSP_FAILED (err)) {                        \
        printf("\n****************LCML ERROR : DSP ************************\n");\
        printf("Error: %s : Err Num = %lx", msg, err);  \
        eError = OMX_ErrorHardware;                \
        printf("\n****************LCML ERROR : DSP ************************\n");\
        goto label;                               \
    }                                              /**/

/* ======================================================================= */
/**
 * This enum is mean to abtract the enumerations of messages that are
 * sent to dsp processor.
 */
/*  ====================================================================== */
typedef enum {
    DSPMSG_AUDIO_UID = 1,
    DSPMSG_AUDIO_PARAMETERS,
    DSPMSG_PLAY,
    DSPMSG_PAUSE,
    DSPMSG_STOP,
    DSPMSG_MODE,
    DSPMSG_PLAYCOMPLETED
} DSP_Messages;

/**
 * USN structure
 */
typedef struct
{
    OMX_U32 iBufferPtr; /* storing buffer pointer MAPPED */
    OMX_U32 iBufferSize; /*buffer size */
    OMX_U32 iParamPtr;/*storing param pointer MAPPED */
    OMX_U32 iParamSize;/*param size */
    OMX_U32 iBufSizeUsed; /* Modified as USN chnages*/
    OMX_U32 iEOSFlag;
    OMX_U32 tBufState;
    OMX_U32 bBufActive;
    OMX_U32 unBufID;
    OMX_U32 ulReserved;
    OMX_U32 iArmArg;/* storing dsp mapped address of structure*/
    OMX_U32 iArmbufferArg;/* ARM side buffer pointer*/
    OMX_U32 iArmParamArg;/*ARM side Param pointer*/
    OMX_U32 Bufoutindex;/* buffer index*/
    OMX_U32 BufInindex;/*buffer i/p index*/
    OMX_U32 iUsrArg;/*Usr argument*/
    OMX_U32 iStreamID;
} TArmDspCommunicationStruct;



/*API needs to be exposed to application*/

/** ========================================================================
*  Initialise the OMX Component specific handle to LCML. The memory is
*  allocated and the dsp node is created. Add notification object to listener
*  thread.
*
*  @param  hInterface - Handle to LCML which is allocated and filled
*  @param  codecName - not used
*  @param  toCodecInitParams - not used yet
*  @param  fromCodecInfoStruct - not used yet
*  @param  pCallbacks - List of callback that uses to call OMX
* ==========================================================================*/
#define LCML_InitMMCodec(                                  \
        hInterface,                                        \
        codecName,                                         \
        toCodecInitParams,                                 \
        fromCodecInfoStruct ,                              \
        pCallbacks                                         \
        )                                                  \
    ((LCML_CODEC_INTERFACE*)hInterface)->InitMMCodec(      \
        hInterface,                                        \
        codecName,                                         \
        toCodecInitParams,                                 \
        fromCodecInfoStruct ,                              \
        pCallbacks                                         \
        )                          /* Macro End */


#define LCML_InitMMCodecEx(                                \
        hInterface,                                        \
        codecName,                                         \
        toCodecInitParams,                                 \
        fromCodecInfoStruct ,                              \
        pCallbacks,                                        \
        Args)                                              \
    ((LCML_CODEC_INTERFACE*)hInterface)->InitMMCodecEx(    \
        hInterface,                                        \
        codecName,                                         \
        toCodecInitParams,                                 \
        fromCodecInfoStruct ,                              \
        pCallbacks,                                        \
        Args)                          /* Macro End */

/** ========================================================================
*  The LCML_WaitForEvent Wait for a event sychronously
*  @param  hInterface -  Handle of the component to be accessed.  This is the
*      component handle returned by the call to the GetHandle function.
*  @param  event - Event occured
*  @param  args - Array of "void *" that contains the associated arguments for
*             occured event
*
*  @return OMX_ERRORTYPE
*      If the command successfully executes, the return code will be
*      OMX_NoError.  Otherwise the appropriate OMX error will be returned.
** ==========================================================================*/
#define LCML_WaitForEvent(                                 \
        hInterface,                                        \
        event,                                             \
        args)                                              \
    ((LCML_CODEC_INTERFACE*)hInterface)->ControlCodec(     \
        hInterface,                                        \
        event,                                             \
        args)                          /* Macro End */


/** ========================================================================
*  The LCML_QueueBuffer send data to DSP convert it into USN format and send
*  it to DSP via setbuff
*  @param [in] hInterface -  Handle of the component to be accessed.  This is
*      the component handle returned by the call to the GetHandle function.
*  @param  bufType - type of buffer
*  @param  buffer - pointer to buffer
*  @param  bufferLen - length of  buffer
*  @param  bufferSizeUsed - length of used buffer
*  @param  auxInfo - pointer to parameter
*  @param  auxInfoLen - length of  parameter
*  @param  usrArg - not used
*  @return OMX_ERRORTYPE
*      If the command successfully executes, the return code will be
*      OMX_NoError.  Otherwise the appropriate OMX error will be returned.
* ==========================================================================*/
#define LCML_QueueBuffer(                                  \
        hInterface,                                        \
        bufType,                                           \
        buffer,                                            \
        bufferLen,                                         \
        bufferSizeUsed,                                    \
        auxInfo,                                           \
        auxInfoLen,                                        \
        usrArg)                                            \
    ((LCML_CODEC_INTERFACE*)hInterface)->QueueBuffer(      \
        hInterface,                                        \
        bufType,                                           \
        buffer,                                            \
        bufferLen,                                         \
        bufferSizeUsed,                                    \
        auxInfo,                                           \
        auxInfoLen,                                        \
        usrArg)

/** ========================================================================
*  The LCML_ControlCodec send command to DSP convert it into USN format and
*  send it to DSP
*  @param  hInterface -  Handle of the component to be accessed.  This is the
*      component handle returned by the call to the GetHandle function.
*  @param  bufType - type of buffer
*  @param  iCodecCmd -  command refer TControlCmd
*  @param  args - pointer to send some specific command to DSP
*
*  @return OMX_ERRORTYPE
*      If the command successfully executes, the return code will be
*      OMX_NoError.  Otherwise the appropriate OMX error will be returned.
** ==========================================================================*/
#define LCML_ControlCodec(                                 \
        hInterface,                                        \
        iCodecCmd,                                         \
        args)                                              \
    ((LCML_CODEC_INTERFACE*)hInterface)->ControlCodec(     \
        hInterface,                                        \
        iCodecCmd,                                         \
        args)                          /* Macro End */




/**
* First function needs to be called by application
*/
OMX_ERRORTYPE GetHandle (OMX_HANDLETYPE* hInterface );

/**
* Struct derives codec interface which have interface to implement for using
* generic codec and also have pointer to DSP specific data and have queues for
* storing input and output data
*/
typedef struct LCML_DSP_INTERFACE
{
    OMX_HANDLETYPE pCodecinterfacehandle;  /* handle to interface struct LCML_CODEC_INTERFACE *dspcodecinterface */
    struct LCML_DSP *dspCodec;
    OMX_PTR pComponentPrivate;
    void * iUsrArg;
    /*queue to store USN structure*/
    TArmDspCommunicationStruct* Armoutputstorage[QUEUE_SIZE];
    TArmDspCommunicationStruct* Arminputstorage[QUEUE_SIZE];
    TArmDspCommunicationStruct * commStruct;
    OMX_U32 iBufinputcount;
    OMX_U32 iBufoutputcount;
    OMX_U32 pshutdownFlag;
#ifdef __ERROR_PROPAGATION__
    struct DSP_NOTIFICATION * g_aNotificationObjects[3];
#else
    struct DSP_NOTIFICATION * g_aNotificationObjects[1];
#endif
    pthread_t g_tidMessageThread;
    OMX_U32 algcntlmapped[QUEUE_SIZE];
    DMM_BUFFER_OBJ *pAlgcntlDmmBuf[QUEUE_SIZE];
    OMX_U32 strmcntlmapped[QUEUE_SIZE];
    DMM_BUFFER_OBJ *pStrmcntlDmmBuf[QUEUE_SIZE];
    pthread_mutex_t mutex;
    OMX_U32 flush_pending[4];
    OMX_BOOL bUsnEos;

#ifdef __PERF_INSTRUMENTATION__
    PERF_OBJHANDLE pPERF, pPERFcomp;
#endif
    DMM_BUFFER_OBJ mapped_dmm_buffers[MAX_DMM_BUFFERS];
    OMX_U32 mapped_buffer_count;
    OMX_BOOL ReUseMap;
    pthread_mutex_t m_isStopped_mutex;

}LCML_DSP_INTERFACE;

#endif /* __MMDSPCODEC_H__ */


