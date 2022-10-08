
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

/** LCML_Types.h
 *  The LCML header file contains the definitions used by both the
 *  application and the component to access common items.
 */
#ifndef LCML_TYPES_H
#define LCML_TYPES_H
#include <OMX_Core.h>
#include <dbapi.h>


/* ======================================================================= */
/**
 * Enum to distinguish between type of buffer
 */
/*  ==================================================================== */
typedef enum {
    INPUT_BUFFER,
    OUTPUT_BUFFER
}Buffer_Type;

/* ======================================================================= */
/**
 * Enum for method of data transfer
 */
/*  ==================================================================== */
typedef enum {
    STREAM_METHOD,
    DMM_METHOD,
    DASF
}LCML_TransferMethod;


/* ======================================================================= */
/**
 * Structure to maintain information regarding streams and related buffers.
 */
/*  ==================================================================== */
typedef struct LCML_BUFFERINFO {
    OMX_U32 nBuffers;
    OMX_U32 nSize;
    LCML_TransferMethod DataTrMethod;
}LCML_BUFFERINFO;


/* ======================================================================= */
/**
 * Structure to encapsulate create phase args.
 */
/*  ==================================================================== */
typedef struct LCML_CREATEPHASEARGS {
    ULONG cbData;
    /*for 2420*/
    /*   BYTE cData[LCML_DATA_SIZE];*/
    /*for 2430*/
    unsigned short cData[LCML_DATA_SIZE];
}LCML_CREATEPHASEARGS;
/* ======================================================================= */
/**
 * Enum to distinguish between dlls
 */
/*  ==================================================================== */
typedef enum {
    DLL_NODEOBJECT,
    DLL_PROCESSOR,
    DLL_DEPENDENT
}LCML_DllType;


/* ======================================================================= */
/**
 * Enum to distinguish between types of events
 */
/*  ==================================================================== */
typedef enum {
    MESSAGING_EVENT,
    STREAMING_EVENT
}DSP_EventType;


/* ======================================================================= */
/**
 * Enum to establish component state
 */
/*  ==================================================================== */


/* ADD MORE STATES */
typedef enum {
    DOES_NOT_EXIST,
    ACTIVE,
    INACTIVE
} COMPONENT_Status;

typedef struct NOTIFICATION_OBJ {
    DSP_EventType msgType;
/*    DSP_HNOTIFICATION pNotifiObj;*/
    struct DSP_NOTIFICATION notifiObj;
}NOTIFICATION_OBJ;


/* ======================================================================= */
/**
 * Structure to capture uuid information.
 */
/*  ==================================================================== */
typedef struct LCML_UUIDINFO {
    struct DSP_UUID *uuid;
/*    OMX_STRING DllName;*/
    OMX_U8 DllName[50];
    LCML_DllType eDllType;
}LCML_UUIDINFO;
/* ======================================================================= */
/**
 * Structure to encapsulate dll information.
 */
/*  ==================================================================== */

typedef struct LCML_DLLINFO {
    unsigned int nNumOfDLLs;
    LCML_UUIDINFO AllUUIDs[LCML_MAX_NUM_OF_DLLs];
}LCML_DLLINFO;




/* Stream mode types */
typedef enum {
    MODE_PROCCOPY,   /* Processor(s) copy stream data payloads */
    MODE_ZEROCOPY,   /* Stream buffer pointers swapped, no data copied */
    MODE_LDMA,       /* Local DMA : OMAP's System-DMA device */
    MODE_RDMA        /* Remote DMA: OMAP's DSP-DMA device */
} LCML_STRMMODE;

/* ======================================================================= */
/**
 * Structure to encapsulate dll information.
 */
/*  ==================================================================== */
typedef struct LCML_STRMATTR {
    unsigned int        uSegid;
    unsigned int        uBufsize;
    unsigned int        uNumBufs;
    unsigned int        uAlignment;
    unsigned int        uTimeout;
    LCML_STRMMODE       lMode;
    unsigned int        uDMAChnlId;
    unsigned int        uDMAPriority;
} LCML_STRMATTR;


/* ======================================================================= */
/**
 * Structure to encapsulate dll information.
 */
/*  ==================================================================== */

typedef struct LCML_DEVICERENDERING{
    int TypeofDevice;
    int TypeofRender;   /* 0 for playback and 1 for record*/
    LCML_UUIDINFO AllUUIDs[LCML_MAX_NUM_OF_DLLs];
    LCML_STRMATTR * DspStream;
}LCML_DEVICERENDERING;


/* ======================================================================= */
/**
 * This enum is mean to abtract the enumerations of messages that are
 * recieved from dsp processor.
 */
/*  ==================================================================== */

typedef enum
{
    EMMCodecDspError = -200,
    EMMCodecInternalError = -201,
    EMMCodecInitError = -202,
    EMMCodecDspMessageRecieved = 1,
    EMMCodecBufferProcessed,
    EMMCodecProcessingStarted,
    EMMCodecProcessingPaused,
    EMMCodecProcessingStoped,
    EMMCodecProcessingEof,
    EMMCodecBufferNotProcessed,
    EMMCodecAlgCtrlAck,
    EMMCodecStrmCtrlAck

}TUsnCodecEvent;

/* ======================================================================= */
/**
 * This enum keeps track of the state of the codec in order to allow the messaging thread to decrease the timeout
 * while waiting for events when in the stopped state so that the thread deletion latency will be minimal
 */
/*  ==================================================================== */

typedef enum
{
    EMessagingThreadCodecRunning,
    EMessagingThreadCodecStopped
}LCML_MESSAGINGTHREAD_STATE;

/* ======================================================================= */
/**
 * Structure used to store reserve address of buffer mapped to DSP
 */
/*  ==================================================================== */

typedef struct DMM_BUFFER_OBJ {
    void* pAllocated;
    void* pReserved;
    void* pMapped;
    void* bufReserved;
    void* paramReserved;
/*  void* structReserved;*/
    int nSize;
} DMM_BUFFER_OBJ;

/* ======================================================================= */
/**
 * Structure used to pass in the callback function. LCML call back functions
 * are defined in this manner
 */
/*  ==================================================================== */

typedef struct LCML_CALLBACKTYPE {
    void (*LCML_Callback) (TUsnCodecEvent event,void * args [10]);
}LCML_CALLBACKTYPE;

/* ======================================================================= */
/**
 * Structure maintaining information for LCML.
 */
/*  ==================================================================== */
typedef struct LCML_DSP {
    DSP_HNODE hNode;
	DSP_HNODE hDasfNode;
    DSP_HPROCESSOR hProc;
    DMM_BUFFER_OBJ InDmmBuffer[QUEUE_SIZE];
    DMM_BUFFER_OBJ OutDmmBuffer[QUEUE_SIZE];
    /*COMPONENT_Status CompStatus;*/
    LCML_CALLBACKTYPE Callbacks;
    /*needs to be filled by application*/
    LCML_BUFFERINFO In_BufInfo;
    LCML_BUFFERINFO Out_BufInfo;
    OMX_U16 *pCrPhArgs;
    LCML_DLLINFO NodeInfo;
    LCML_DEVICERENDERING DeviceInfo;
    OMX_U32 SegID;
    OMX_U32 Timeout;
    OMX_U32 Alignment;
    OMX_S32 Priority;
    OMX_U16 numStreams;
    OMX_U32 ProfileID;
}LCML_DSP;


#endif /*LCML_TYPES_H*/
