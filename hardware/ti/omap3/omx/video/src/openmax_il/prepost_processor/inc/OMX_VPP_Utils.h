
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
#ifndef OMX_VPP_UTILS__H
#define OMX_VPP_UTILS__H

#include <OMX_Component.h>
#include "LCML_DspCodec.h"
#ifdef RESOURCE_MANAGER_ENABLED
#include <ResourceManagerProxyAPI.h>
#endif
#include <OMX_TI_Common.h>

#ifdef __PERF_INSTRUMENTATION__
#include "perf.h"
#endif

#define KHRONOS_1_2

/*Linked List */

typedef struct Node {
    struct Node *pNextNode;
    void *pValue;
} Node;

typedef struct LinkedList {
    Node *pRoot;
}   LinkedList;

LinkedList AllocList;

void LinkedList_Create(LinkedList *LinkedList);
void LinkedList_AddElement(LinkedList *LinkedList, void *pValue);
void LinkedList_FreeElement(LinkedList *LinkedList, void *pValue);
void LinkedList_FreeAll(LinkedList *LinkedList);
void LinkedList_DisplayAll(LinkedList *LinkedList);
void LinkedList_Destroy(LinkedList *LinkedList);

/*
 *     M A C R O S
 */
/* configuration numbers */

/*difine for Khronos 1.1*/
#define KHRONOS_1_1

/*#define VPP_DEBUG*/

#define VPP_MAJOR_VER 0x01
/*#ifdef  KHRONOS_1_1
#define VPP_MINOR_VER 0x01
#else*/
#define VPP_MINOR_VER 0x00
/*#endif*/
#define VPP_REVISION  0x00
#define VPP_STEP      0x00

#define NORMAL_BUFFER      0
#define OMX_NOPORT         0xFFFFFFFE
#define NUM_OF_PORTS       4

#define NUM_OF_VPP_BUFFERS (4)
#define MAX_VPP_BUFFERS    (4)
#define NUM_OF_VPP_PORTS   (4)
#define MIN_NUM_OF_VPP_BUFFERS 1

#define DEFAULT_WIDTH      (176)
#define DEFAULT_HEIGHT     (144)

#define VPP_ZERO                0
#define VPP_DSPSTOP          0x01
#define VPP_BUFFERBACK    0x02
#define VPP_IDLEREADY      ( VPP_DSPSTOP | VPP_BUFFERBACK )

#define DSP_MMU_FAULT_HANDLING

#ifdef UNDER_CE
#define USN_DLL_NAME "/windows/usn.dll64P"
#define VPP_NODE_DLL "/windows/vpp_sn.dll64P"
#else
#define USN_DLL_NAME "usn.dll64P"
#define VPP_NODE_DLL "vpp_sn.dll64P"
#endif

#ifdef UNDER_CE
  #include <oaf_debug.h> /* defines VPP_DPRINT*/
#else 
    #ifdef  VPP_DEBUG
        #define VPP_DPRINT(...) fprintf(stderr,__VA_ARGS__)
    #else
        #define VPP_DPRINT(...)
    #endif
#endif 
 
#define OMX_INIT_STRUCT(_s_, _name_)  \
{ \
    memset((_s_), 0x0, sizeof(_name_));  \
    (_s_)->nSize = sizeof(_name_);    \
    (_s_)->nVersion.s.nVersionMajor = VPP_MAJOR_VER;  \
    (_s_)->nVersion.s.nVersionMinor = VPP_MINOR_VER;  \
    (_s_)->nVersion.s.nRevision = VPP_REVISION;    \
    (_s_)->nVersion.s.nStep = VPP_STEP; \
}

#define OMX_CHECK_CMD(_ptr1, _ptr2, _ptr3)  \
{            \
    if(!_ptr1 || !_ptr2 || !_ptr3){    \
        eError = OMX_ErrorBadParameter;    \
        goto EXIT;      \
    }            \
}

#define OMX_SET_ERROR_BAIL(_eError, _eCode)\
{            \
    _eError = _eCode;        \
    goto EXIT;      \
}

#define OMX_MALLOC(_pStruct_, _size_)   \
    _pStruct_ = malloc(_size_);  \
    if(_pStruct_ == NULL){  \
        eError = OMX_ErrorInsufficientResources;    \
        goto EXIT;  \
    } \
    memset(_pStruct_, 0, _size_);\
    LinkedList_AddElement(&AllocList, _pStruct_);

#define OMX_FREE(_ptr)   \
{                     \
    if (_ptr != NULL) { \
        LinkedList_FreeElement(&AllocList, _ptr);\
        _ptr = NULL; \
    }                \
}

#define OMX_FREEALL()   \
{                     \
    LinkedList_FreeAll(&AllocList);\
}


/**********************************************************************
 *    GPP Internal data type
 **********************************************************************
 */

typedef enum OMX_VPP_PORT_NUMBER_TYPE {
    OMX_VPP_INPUT_PORT         = 0,
    OMX_VPP_INPUT_OVERLAY_PORT = 1,
    OMX_VPP_RGB_OUTPUT_PORT    = 2,
    OMX_VPP_YUV_OUTPUT_PORT    = 3,
    OMX_VPP_MAXPORT_NUM = 3
} OMX_VPP_PORT_NUMBER_TYPE;

typedef enum {
    IUALG_CMD_SETCOEFF = 100,
    IUALG_CMD_SETIO
}IUALG_VppCmd;

typedef enum VPP_BUFFER_OWNER
{
    VPP_BUFFER_CLIENT = 0x0,
    VPP_BUFFER_COMPONENT_IN,
    VPP_BUFFER_COMPONENT_OUT,
    VPP_BUFFER_DSP,
    VPP_BUFFER_TUNNEL_COMPONENT
} VPP_BUFFER_OWNER;
typedef enum OMX_INDEXVPPTYPE 
{
#ifdef KHRONOS_1_2
    OMX_IndexCustomSetZoomFactor = (OMX_IndexVendorStartUnused + 1),
#else
    OMX_IndexCustomSetZoomFactor = (OMX_IndexIndexVendorStartUnused + 1),
#endif
    OMX_IndexCustomSetZoomLimit,
    OMX_IndexCustomSetZoomSpeed,
    OMX_IndexCustomSetZoomXoffsetFromCenter16,
    OMX_IndexCustomSetZoomYoffsetFromCenter16,
    OMX_IndexCustomSetFrostedGlassOvly,
    OMX_IndexCustomVideoColorRange,
    OMX_IndexCustomRGB4ColorFormat,
    OMX_IndexCustomConfigInputSize
} OMX_INDEXVPPTYPE; 


typedef enum VGPOP_IORange {
    VGPOP_IN_16_235_OUT_16_235,     /*limited range to limited range*/
    VGPOP_IN_00_255_OUT_00_255,      /*full range to full range*/
    VGPOP_IN_00_255_OUT_16_235,         /*full range to limited range*/
    VGPOP_IN_16_235_OUT_00_255          /*limited range to full range*/
} VGPOP_IORange;


 /* Parameter buffer which needs to be passed to DSP */

typedef struct GPPToVPPInputFrameStatus {

    /* INPUT FRAME */
      
    /* input size*/
    OMX_U32      ulInWidth;          /*  picture buffer width          */ 
    OMX_U32      ulInHeight;         /*  picture buffer height         */ 
    OMX_U32      ulCInOffset;        /* offset of the C frame in the   *
                                    * buffer (equal to zero if there *
                                    * is no C frame)                 */ 
    
    /* PROCESSING PARAMETERS */
    
    /*    crop           */ 
    OMX_U32      ulInXstart;          /*  Hin active start             */ 
    OMX_U32      ulInXsize;           /*  Hin active width             */ 
    OMX_U32      ulInYstart;          /*  Vin active start             */ 
    OMX_U32      ulInYsize;           /* Vin active height             */ 

    /*   zoom            */ 
    OMX_U32      ulZoomFactor;        /*zooming ratio (/1024)          */ 
    OMX_U32      ulZoomLimit;         /* zooming ratio limit (/1024)   */ 
    OMX_U32      ulZoomSpeed;         /* speed of ratio change         */ 

    /*  stabilisation             */ 
    OMX_U32      ulXoffsetFromCenter16;    /*  add 1/16/th accuracy offset */ 
    OMX_U32      ulYoffsetFromCenter16;    /* add 1/16/th accuracy offset  */ 

    /*  gain and contrast             */ 
    OMX_U32      ulContrastType;      /*    Contrast method            */ 
    OMX_U32      ulVideoGain;         /* gain on video (Y and C)       */ 

    /*  effect             */ 
    OMX_U32      ulFrostedGlassOvly;  /*  Frosted glass effect overlay          */ 
    OMX_U32      ulLightChroma;       /*  Light chrominance process             */ 
    OMX_U32      ulLockedRatio;       /*  keep H/V ratio                        */ 
    OMX_U32      ulMirror;            /*  to mirror the picture                 */ 
    OMX_U32      ulRGBRotation;          /*  0, 90, 180, 270 deg.                  */ 
    OMX_U32      ulYUVRotation;          /*  0, 90, 180, 270 deg.                  */ 
  
#ifndef _55_
    OMX_U32     eIORange;              /*  Video Color Range Conversion */
    OMX_U32     ulDithering;           /*  dithering                             */ 
    OMX_U32     ulOutPitch;                 /* output pitch (in bytes)*/
    OMX_U32     ulAlphaRGB;                 /* Global A value of an ARGB output*/
#endif
  
}GPPToVPPInputFrameStatus;


/* OUTPPUT BUFFER */

typedef struct GPPToVPPOutputFrameStatus {

    OMX_U32      ulOutWidth;          /*  RGB/YUV picture buffer width           */ 
    OMX_U32      ulOutHeight;         /*  RGB/YUV picture buffer height          */ 
    OMX_U32      ulCOutOffset;        /*  Offset of the C frame in the buffer (equal to 0 if there is no C frame)             */ 
  
}GPPToVPPOutputFrameStatus;

/* ALG CONTROL*/

typedef struct VPPIOConf {

    /*   Optionnal input             */ 
    OMX_U32 overlayInputImage;
    /*  Optionnal output             */ 
    OMX_U32 RGBOutputImage;
    OMX_U32 YUVOutputImage;
    
} VPPIOConf;

/* UUID structure for DSP/BIOS Bridge nodes. COMMON_TI_UUID*/
static const struct DSP_UUID COMMON_TI_UUID = {
        0x79A3C8B3, 0x95F2, 0x403F, 0x9A, 0x4B, {
        0xCF, 0x80, 0x57, 0x73, 0x05, 0x41
    }
};



/*===================================================================*/
/**
 * OMX_VPP_PORT_NUMBER_TYPE enumeration for ports supported by
 * this component.
 */
/*===================================================================*/
typedef struct VPP_OVERLAY {
    OMX_U8  *iRBuff ;
    OMX_U8  *iGBuff;
    OMX_U8  *iBBuff ;
    OMX_U8  *iOvlyConvBufPtr ;
    OMX_U8  iRKey;
    OMX_U8  iGKey;
    OMX_U8  iBKey;
    OMX_U8  iAlign ;
}VPP_OVERLAY;

/* Component buffer */
typedef struct OMX_VPP_COMPONENT_BUFFER {
    OMX_BUFFERHEADERTYPE  *pBufHeader;
    OMX_U32                   nIndex;
    OMX_BOOL                bSelfAllocated;
    OMX_BOOL                bHolding;
    VPP_BUFFER_OWNER  eBufferOwner;
    OMX_U8                      *pBufferStart;
} OMX_VPP_COMPONENT_BUFFER;

typedef struct VPP_BUFFERDATA_PROPAGATION {
    OMX_U32 flag;
    OMX_U32 buffer_idYUV;
    OMX_U32 buffer_idRGB;
    OMX_HANDLETYPE hMarkTargetComponent; 
    OMX_PTR pMarkData;
    OMX_U32 nTickCount;
    OMX_TICKS nTimeStamp;
} VPP_BUFFERDATA_PROPAGATION;

/* Component Port Context */
typedef struct VPP_PORT_TYPE 
{
    OMX_HANDLETYPE               hTunnelComponent;
    OMX_U32                      nTunnelPort;
    OMX_BUFFERSUPPLIERTYPE       eSupplierSetting;
    OMX_BUFFERSUPPLIERTYPE       eSupplierPreference;
    OMX_U32                      nPortIndex;            
    OMX_U32                      nBufferCount;          
    OMX_VPP_COMPONENT_BUFFER     pVPPBufHeader[NUM_OF_VPP_BUFFERS];
    VPP_BUFFERDATA_PROPAGATION sBufferDataProp[NUM_OF_VPP_BUFFERS];
    OMX_PARAM_PORTDEFINITIONTYPE pPortDef;
    OMX_BOOL                     nBufSupplier;
    OMX_U32                         nReturnedBufferCount; /*For tunneling*/
    OMX_MIRRORTYPE          eMirror;
} VPP_PORT_TYPE;

typedef struct VPP_COMPONENT_PRIVATE
{
    /** Array of pointers to BUFFERHEADERTYPE structues 
       This pBufHeader[INPUT_PORT] will point to all the 
       BUFFERHEADERTYPE structures related to input port, 
       not just one structure. Same is for output port
       also. */
    
    
    VPP_PORT_TYPE                   sCompPorts[NUM_OF_VPP_PORTS];

    OMX_CALLBACKTYPE                cbInfo;
    /** Handle for use with async callbacks */

    /** This will contain info like how many buffers
        are there for input/output ports, their size etc, but not
        BUFFERHEADERTYPE POINTERS. */

    OMX_PORT_PARAM_TYPE                 *pPortParamTypeImage;
    OMX_PORT_PARAM_TYPE                 *pPortParamTypeAudio;
    OMX_PORT_PARAM_TYPE                 *pPortParamTypeVideo;
    OMX_PORT_PARAM_TYPE                 *pPortParamTypeOthers;
    
    OMX_VIDEO_PARAM_PORTFORMATTYPE  *pInPortFormat;
    OMX_VIDEO_PARAM_PORTFORMATTYPE  *pInPortOverlayFormat;
    OMX_VIDEO_PARAM_PORTFORMATTYPE  *pOutPortRGBFormat;
    OMX_VIDEO_PARAM_PORTFORMATTYPE  *pOutPortYUVFormat;
    OMX_PRIORITYMGMTTYPE            *pPriorityMgmt;

    /** This is component handle */
    OMX_COMPONENTTYPE               *pHandle;

    /** Current state of this component */
    OMX_STATETYPE   curState;

    /** The state to go **/
    OMX_STATETYPE   toState;

    OMX_STRING      cComponentName;
    OMX_VERSIONTYPE ComponentVersion;
    OMX_VERSIONTYPE SpecVersion;
    
    /** The component thread handle */
    pthread_t       ComponentThread; 
    LCML_DSP_INTERFACE* pLCML;
	void * pDllHandle;

    /** The pipes for sending commands to the thread */
    int            cmdPipe[2];
    int            nCmdDataPipe[2];
    /** The pipes for sending buffers to the thread */
   
    /*The Pipe to send empty output buffers to component*/
    int            nFree_oPipe[2];                         
    /*The pipe for sending Filled Input buffers to component*/
    int            nFilled_iPipe[2];                      
  
    /** Set to indicate component is stopping */
    OMX_U32        bIsStopping;

    OMX_U32        bIsEOFSent;
    OMX_U32        lcml_compID;
    OMX_U32        NumofOutputPort;
    OMX_U32        IsYUVdataout;
    OMX_U32         IsRGBdataout;
    OMX_U32        IsOverlay;
    OMX_CONFIG_RECTTYPE     *pCrop;
    OMX_U8         ExeToIdleFlag;  /* StateCheck */
    OMX_HANDLETYPE pLcmlHandle;
    VPP_OVERLAY    *overlay;
    OMX_U8         *RGBbuffer ;
    OMX_U8         *colorKey;

#ifdef __PERF_INSTRUMENTATION__
    PERF_OBJHANDLE pPERF, pPERFcomp;
#endif

    OMX_U32        lcml_nCntIp; 
    OMX_U32        lcml_nCntOpReceived; 
    OMX_U32         nInputFrame; /*Buffer data propagation*/
    OMX_U32         nOverlayFrame; /*Buffer data propagation*/
    OMX_U32        nInYUVBufferCount; /*Buffer data propagation*/
    OMX_U32         nInRGBBufferCount; /*Buffer data propagation*/
    OMX_U32        nOutYUVBufferCount; /*Buffer data propagation*/
    OMX_U32        nOutRGBBufferCount; /*Buffer data propagation*/
    OMX_PTR        pMarkData;                   /*Buffer data propagation*/
    OMX_HANDLETYPE hMarkTargetComponent; /*Buffer data propagation*/
    VPPIOConf     *tVPPIOConf;
    GPPToVPPInputFrameStatus  *pIpFrameStatus;
    GPPToVPPOutputFrameStatus *pOpYUVFrameStatus;
    GPPToVPPOutputFrameStatus *pOpRGBFrameStatus;
    OMX_CONFIG_SCALEFACTORTYPE      sScale;
    OMX_U8         CodecAlgCtrlAck;
    OMX_BOOL	   bFlushComplete;
	OMX_U32		   nFlushPort;
    OMX_BOOL       bDisable;
    OMX_BOOL       bDisableIncomplete[NUM_OF_VPP_PORTS];
#ifdef RESOURCE_MANAGER_ENABLED
    RMPROXY_CALLBACKTYPE rmproxyCallback;
#endif
    OMX_BOOL bPreempted;
    
    
	pthread_mutex_t buf_mutex;
	pthread_mutex_t vpp_mutex;
	pthread_cond_t  stop_cond;

#ifdef KHRONOS_1_1
    OMX_PARAM_COMPONENTROLETYPE componentRole;
#endif
} VPP_COMPONENT_PRIVATE;

/* structures for custom commands */
typedef struct _VPP_CUSTOM_PARAM_DEFINITION
{
    OMX_U8 cCustomParamName[128];
    OMX_INDEXTYPE nCustomParamIndex;
} VPP_CUSTOM_PARAM_DEFINITION;

/* Function ProtoType */

OMX_ERRORTYPE VPP_Fill_LCMLInitParams(OMX_HANDLETYPE pHandle, OMX_U16 arr[], LCML_DSP *plcml_Init);

OMX_ERRORTYPE VPP_GetBufferDirection(OMX_BUFFERHEADERTYPE *pBufHeader, OMX_DIRTYPE *eDir, OMX_U32 Index);

OMX_ERRORTYPE VPP_LCML_Callback (TUsnCodecEvent event,void * args [10]);

OMX_ERRORTYPE VPP_HandleCommand (VPP_COMPONENT_PRIVATE *pComponentPrivate, OMX_U32 nParam1);

OMX_ERRORTYPE VPP_HandleDataBuf_FromApp(OMX_BUFFERHEADERTYPE *pBufHeader, VPP_COMPONENT_PRIVATE *pComponentPrivate);


OMX_ERRORTYPE VPP_IsValidBuffer(OMX_BUFFERHEADERTYPE *pBufHeader, 
                            VPP_COMPONENT_PRIVATE *pComponentPrivate,
                            OMX_U32 pIndex,
                            OMX_U32 *pCount);
                            

OMX_ERRORTYPE VPP_GetPortDefFromBufHeader(OMX_BUFFERHEADERTYPE *pBufHeader, OMX_PARAM_PORTDEFINITIONTYPE **portDef );

OMX_ERRORTYPE VPP_HandleDataBuf_FromLCML(VPP_COMPONENT_PRIVATE* pComponentPrivate);

OMX_HANDLETYPE VPP_GetLCMLHandle(VPP_COMPONENT_PRIVATE *pComponentPrivate);

OMX_ERRORTYPE VPP_GetCorresponding_LCMLHeader(VPP_COMPONENT_PRIVATE *pComponentPrivate,
                                              OMX_U8 *pBuffer, 
                                              OMX_DIRTYPE eDir,
                                              OMX_VPP_COMPONENT_BUFFER **ppCmpBuf, 
                                              OMX_U32 Index);

OMX_ERRORTYPE VPP_Free_ComponentResources(OMX_HANDLETYPE pComponent);

OMX_ERRORTYPE ComputeTiOverlayImgFormat (VPP_COMPONENT_PRIVATE *pComponentPrivate,
                                         OMX_U8* aPictureArray, 
                                         OMX_U8* aOutImagePtr, 
                                         OMX_U8* aTransparencyKey);

OMX_ERRORTYPE VPP_Start_ComponentThread(OMX_HANDLETYPE pHandle);

OMX_ERRORTYPE VPP_Stop_ComponentThread(OMX_HANDLETYPE pHandle);

OMX_ERRORTYPE VPP_DisablePort (VPP_COMPONENT_PRIVATE* pComponentPrivate, OMX_U32 nParam1);

OMX_ERRORTYPE VPP_EnablePort (VPP_COMPONENT_PRIVATE* pComponentPrivate, OMX_U32 nParam1);

OMX_ERRORTYPE VPP_HandleCommandFlush (VPP_COMPONENT_PRIVATE* pComponentPrivate, OMX_U32 nParam1, OMX_BOOL return_event);

OMX_ERRORTYPE VPP_Process_FilledInBuf(VPP_COMPONENT_PRIVATE* pComponentPrivate);

OMX_ERRORTYPE VPP_Process_FilledOutBuf(VPP_COMPONENT_PRIVATE* pComponentPrivate,
                                       OMX_VPP_COMPONENT_BUFFER *pComponentBuf);

OMX_ERRORTYPE VPP_Process_FreeInBuf(VPP_COMPONENT_PRIVATE* pComponentPrivate,
                                    OMX_VPP_COMPONENT_BUFFER *pComponentBuf);

OMX_ERRORTYPE VPP_Process_FreeOutBuf(VPP_COMPONENT_PRIVATE* pComponentPrivate);

OMX_ERRORTYPE VPP_Initialize_PrivateStruct(VPP_COMPONENT_PRIVATE *pComponentPrivate);

OMX_BOOL IsTIOMXComponent(OMX_HANDLETYPE hComp);

void VPP_InitBufferDataPropagation(VPP_COMPONENT_PRIVATE * pComponentPrivate, OMX_U32 nPortIndex);

#endif
