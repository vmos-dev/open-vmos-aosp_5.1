
/*
 *  Copyright 2001-2008 Texas Instruments - http://www.ti.com/
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifdef UNDER_CE
#include <windows.h>
#include <oaf_osal.h>
#else
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/vt.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h> 
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <OMX_Index.h>
#include <OMX_Types.h>
#include <OMX_Core.h>
#include <OMX_Audio.h>
#include <OMX_VPP.h>
#include <OMX_IVCommon.h>
#include <OMX_Component.h>
#include "VPPTest.h"
#include <signal.h>

/* DSP recovery includes */
#include <qosregistry.h>
#include <qosti.h>
#include <dbapi.h>
#include <DSPManager.h>
#include <DSPProcessor.h>
#include <DSPProcessor_OEM.h>

#define KHRONOS_1_2


#ifdef UNDER_CE
#define sleep Sleep
#define APP_DPRINT printf
#else
/*#define APP_DEBUG*/

#ifdef  APP_DEBUG
        #define APP_DPRINT(...)    fprintf(stderr,__VA_ARGS__)
#else
        #define APP_DPRINT(...)
#endif
#endif //#ifdef UNDER_CE

#ifdef UNDER_CE
int fill_data (OMX_BUFFERHEADERTYPE *pBuf, HANDLE fIn);
#else
int fill_data (OMX_BUFFERHEADERTYPE *pBuf, FILE *fIn);
#endif

#define DEFAULT_WIDTH      (176)
#define DEFAULT_HEIGHT     (144)


typedef struct _OMX_VPPCustomTYPE
{
    OMX_INDEXTYPE VPPCustomSetZoomFactor;
    OMX_INDEXTYPE VPPCustomSetZoomLimit;
    OMX_INDEXTYPE VPPCustomSetZoomSpeed;
    OMX_INDEXTYPE VPPCustomSetZoomXoffsetFromCenter16;
    OMX_INDEXTYPE VPPCustomSetZoomYoffsetFromCenter16;
    OMX_INDEXTYPE VPPCustomSetFrostedGlassOvly;
    OMX_INDEXTYPE VPPCustomSetColorRange;
    OMX_INDEXTYPE VPP_CustomRGB4ColorFormat;
} OMX_VPPCustomTYPE;

typedef struct EVENT_PRIVATE {
    OMX_EVENTTYPE eEvent;
    OMX_U32 nData1;
    OMX_U32 nData2;
    OMX_PTR pAppData;
    OMX_PTR eInfo;
} EVENT_PRIVATE;
static OMX_ERRORTYPE VPP_SetZoom(OMX_HANDLETYPE pHandle, int speed, int factor, int limit, int xoff, int yoff);
static OMX_ERRORTYPE VPP_SetContrast(OMX_HANDLETYPE pHandle, int Contrast);
static OMX_ERRORTYPE VPP_FrostedGlassEffect(OMX_HANDLETYPE pHandle, int IsOverlay);
static OMX_ERRORTYPE VPP_SetCrop(OMX_HANDLETYPE pHandle, int XStart, int XSize, int YStart, int YSize);
static OMX_ERRORTYPE VPP_SetMirroring(OMX_HANDLETYPE pHandle, int IsRGBOutput);
static OMX_ERRORTYPE VPP_SetRotationAngle(OMX_HANDLETYPE pHandle, int IsRGBOutput,int Angle);
static OMX_ERRORTYPE VPP_SetDithering(OMX_HANDLETYPE pHandle, int IsRGBOutput);
static OMX_ERRORTYPE VPP_SetColorRange(OMX_HANDLETYPE pHandle, int nColorRange); 
OMX_BOOL VPP_Test_Check_Frames(int YUVRGB, int inFrames, int OvlyFrames,int outRGBFrames,int outYUVFrames);

#ifdef DSP_MMU_FAULT_HANDLING
int LoadBaseImage();
#endif

#ifdef UNDER_CE
OMX_STRING strAmrDecoder = "OMX.TI.IMAGE.VGPOP";
#else
OMX_STRING strAmrDecoder = "OMX.TI.VPP";
#endif

int IpBuf_Pipe[2];
int OvlyBuf_Pipe[2];
int OpRGBBuf_Pipe[2];
int OpYUVBuf_Pipe[2];
int Event_Pipe[2];
int nRGBFillBufferDones=0;
int nYUVFillBufferDones=0;
int nInputEmptyBufferDones=0;
int nOvlyEmptyBufferDones=0;

/* Error flag */
OMX_BOOL bError = OMX_FALSE;

#ifndef UNDER_CE
    struct timeval base;
    struct timeval newer;
    /*struct timezone tz;*/
#endif //#ifndef UNDER_CE


static COMPONENT_PORTINDEX_DEF MyVppPortDef;
static OMX_VPPCustomTYPE MyVPPCustomDef;

static long int nTotalTime = 0;


/* safe routine to get the maximum of 2 integers */
int maxint(int a, int b)
{
         return (a>b) ? a : b;
}

/* This method will wait for the component to get to the state
* specified by the DesiredState input. */
static OMX_ERRORTYPE WaitForState(OMX_HANDLETYPE* pHandle, OMX_STATETYPE DesiredState)
{
    OMX_STATETYPE CurState = OMX_StateInvalid;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    int nCnt = 0;
    OMX_COMPONENTTYPE *pComponent = (OMX_COMPONENTTYPE *)pHandle;

    eError = pComponent->GetState(pHandle, &CurState);
    while( (eError == OMX_ErrorNone) &&
    (CurState != DesiredState) && (CurState != OMX_StateInvalid)) {
    sched_yield();
    if ( nCnt++ >= 0xFFFFFFFE ) {
        fprintf(stderr, "VPPTEST:: Press CTL-C to continue\n");
    }
        eError = pComponent->GetState(pHandle, &CurState);
    }

    if (CurState == OMX_StateInvalid && DesiredState != OMX_StateInvalid){
        eError = OMX_ErrorInvalidState;
    } 
    return eError;
}

void EventHandler(OMX_HANDLETYPE hComponent,OMX_PTR pAppData, OMX_EVENTTYPE eEvent,
OMX_U32 nData1,OMX_U32 nData2, OMX_STRING eInfo)
{
    
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    EVENT_PRIVATE MyEvent;

    MyEvent.eEvent = eEvent;
    MyEvent.nData1 = nData1;
    MyEvent.nData2 = nData2;
    MyEvent.pAppData = pAppData;

    switch (eEvent) {
        case OMX_EventCmdComplete:
            APP_DPRINT ("%d :: App: Component State Changed To %d\n", __LINE__,state);               
            
            break;
        case OMX_EventError:
            if(nData1 == OMX_ErrorHardware){
                printf("%d: App: ErrorNotofication Came: \
                            \nComponent Name : %d : Error Num %lx: String :%s\n",
                __LINE__,*((int *)(pAppData)), nData1, eInfo);
                eError = OMX_SendCommand(hComponent, OMX_CommandStateSet, OMX_StateInvalid, NULL);
                if(eError != OMX_ErrorNone) {
                    printf ("Error from SendCommand-Invalid State function\n");
                }
            }
            else{
                printf("%d: App: ErrorNotofication Came: \
                            \nComponent Name : %d : Error Num %lx: String :%s\n",
                            __LINE__,*((int *)(pAppData)), nData1, eInfo);
            }
    
            break;
        case OMX_EventMax:
            break;   
        case OMX_EventMark:
            break;   
        default:
            break;
    }

    write(Event_Pipe[1], &MyEvent, sizeof(EVENT_PRIVATE));

}

#ifndef UNDER_CE
long GetProfiletime()
{
    long int nFrameTime = 0;
    /*struct timeval older;*/
    int nStatus ;
    base.tv_sec = newer.tv_sec;
    base.tv_usec = newer.tv_usec;
    nStatus = gettimeofday(&newer, NULL);
    /*printf("base.tv_sec = %ld, base.tv_usec %ld\n", base.tv_sec, base.tv_usec);*/
    nFrameTime = (newer.tv_sec-base.tv_sec) * 1000000 + (newer.tv_usec-base.tv_usec);
    nTotalTime = nTotalTime + nFrameTime;
    return nFrameTime;

}
#endif /*#ifndef UNDER_CE*/

void FillBufferDone (OMX_HANDLETYPE hComponent, OMX_PTR ptr, OMX_BUFFERHEADERTYPE* pBuffer)
{

    /*PROFILE POINT*/

#ifndef UNDER_CE
    long int pftime = 0;
    pftime = GetProfiletime();
    APP_DPRINT("total time for frame %d \n", nTotalTime);
    APP_DPRINT("total time for each frame %d \n",pftime);
#endif

    if(pBuffer->nOutputPortIndex==MyVppPortDef.rgboutput_port){  
          write(OpRGBBuf_Pipe[1], &pBuffer, sizeof(pBuffer));
          nRGBFillBufferDones++;
    }
    else if(pBuffer->nOutputPortIndex==MyVppPortDef.yuvoutput_port){  
          write(OpYUVBuf_Pipe[1], &pBuffer, sizeof(pBuffer));
          nYUVFillBufferDones++;
    }
    else{
        printf("VPPTEST:: Incorrect Output Port Index\n");
    }
        

}


void EmptyBufferDone(OMX_HANDLETYPE hComponent, OMX_PTR ptr, OMX_BUFFERHEADERTYPE* pBuffer)
{
    if(pBuffer->nInputPortIndex == MyVppPortDef.input_port){
        write(IpBuf_Pipe[1], &pBuffer, sizeof(pBuffer));
        nInputEmptyBufferDones++;
    }
    else if(pBuffer->nInputPortIndex == MyVppPortDef.overlay_port){
        write(OvlyBuf_Pipe[1], &pBuffer,sizeof(pBuffer));
        nOvlyEmptyBufferDones++;
    }   
    else{
        printf("VPPTEST:: Incorrect Input Port Index\n");  
    }
}


static OMX_ERRORTYPE GetComponentPortDef(OMX_HANDLETYPE pHandleVPP, COMPONENT_PORTINDEX_DEF *pVppPortDef)
{
    OMX_PORT_PARAM_TYPE *pTempPortType = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE *pTempVidPortDef = NULL;
    int i;
    OMX_BOOL found_input = OMX_FALSE;

    pTempPortType = calloc(1, sizeof(OMX_PORT_PARAM_TYPE));
    if (!pTempPortType){
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    
    eError = OMX_GetParameter(pHandleVPP, OMX_IndexParamVideoInit, pTempPortType);
    if (eError != OMX_ErrorNone) {
        printf("VPPTEST:: Error at %d\n",__LINE__);
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    APP_DPRINT("VPP_JPEG_DISPLAY:: ports of VPP %lu, start port %lu\n", pTempPortType->nPorts, pTempPortType->nStartPortNumber);

    pTempVidPortDef = calloc (1,sizeof (OMX_PARAM_PORTDEFINITIONTYPE)); 
    if (!pTempVidPortDef){
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    
    found_input = OMX_FALSE;
    for (i = pTempPortType->nStartPortNumber; i < pTempPortType->nPorts; i ++){
        pTempVidPortDef->nPortIndex = i;
        eError = OMX_GetParameter (pHandleVPP, OMX_IndexParamPortDefinition, pTempVidPortDef);
        if ( eError != OMX_ErrorNone ){
            printf("VPPTEST:: Error at %d\n",__LINE__);
            eError = OMX_ErrorBadParameter;
            goto EXIT;
        }      
            
        if ((pTempVidPortDef->eDir == OMX_DirInput) &&(found_input == OMX_FALSE)){
            /* found input */
            pVppPortDef->input_port= i;
            found_input = OMX_TRUE;
            continue;
        }
        if ((pTempVidPortDef->eDir == OMX_DirInput) && (found_input == OMX_TRUE)){
            /* found ovelay port */
            pVppPortDef->overlay_port = i;
            continue;
        }
        if ((pTempVidPortDef->eDir == OMX_DirOutput) &&
            (pTempVidPortDef->format.video.eColorFormat == OMX_COLOR_Format16bitRGB565)){
            /* found RGB output */
            pVppPortDef->rgboutput_port = i;
            continue;
        }
        if ((pTempVidPortDef->eDir == OMX_DirOutput) &&
            ((pTempVidPortDef->format.video.eColorFormat == OMX_COLOR_FormatYCbYCr)||
            (pTempVidPortDef->format.video.eColorFormat == OMX_COLOR_FormatCbYCrY))){
             /* found YUV output */
            pVppPortDef->yuvoutput_port = i;
            continue;
        }         
    }
    
    APP_DPRINT("VPPTEST:: input port is %d\n", pVppPortDef->input_port);
    APP_DPRINT("VPPTEST:: overlay port is %d\n", pVppPortDef->overlay_port);    
    APP_DPRINT("VPPTEST:: RGB output port is %d\n", pVppPortDef->rgboutput_port);   
    APP_DPRINT("VPPTEST:: YUV output port is %d\n", pVppPortDef->yuvoutput_port);

EXIT:
    if (pTempPortType){
        free(pTempPortType);
        pTempPortType = NULL;
    }

    if (pTempVidPortDef){
        free(pTempVidPortDef);
        pTempVidPortDef = NULL;
    }
    return eError;
}



static OMX_ERRORTYPE GetVPPCustomDef(OMX_HANDLETYPE pHandleVPP)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    
    eError = OMX_GetExtensionIndex(pHandleVPP, "OMX.TI.VPP.Param.ZoomFactor", &(MyVPPCustomDef.VPPCustomSetZoomFactor));
    if(eError != OMX_ErrorNone) {
        fprintf (stderr,"VPPTEST:: Error in OMX_GetExtensionIndex function\n");
        goto EXIT;
    }
    APP_DPRINT("OMX.TI.VPP.Param.ZoomFactor is %x\n", MyVPPCustomDef.VPPCustomSetZoomFactor);

    eError = OMX_GetExtensionIndex(pHandleVPP, "OMX.TI.VPP.Param.ZoomLimit", &(MyVPPCustomDef.VPPCustomSetZoomLimit));
    if(eError != OMX_ErrorNone) {
        fprintf (stderr,"VPPTEST:: Error in OMX_GetExtensionIndex function\n");
        goto EXIT;
    }
    APP_DPRINT("OMX.TI.VPP.Param.ZoomLimit is %x\n", MyVPPCustomDef.VPPCustomSetZoomLimit);

    eError = OMX_GetExtensionIndex(pHandleVPP, "OMX.TI.VPP.Param.ZoomSpeed", &(MyVPPCustomDef.VPPCustomSetZoomSpeed));
    if(eError != OMX_ErrorNone) {
        fprintf (stderr,"Error in OMX_GetExtensionIndex function\n");
        goto EXIT;
    }
    APP_DPRINT("OMX.TI.VPP.Param.ZoomSpeed is %x\n", MyVPPCustomDef.VPPCustomSetZoomSpeed);

    eError = OMX_GetExtensionIndex(pHandleVPP, "OMX.TI.VPP.Param.ZoomXoffsetFromCenter16", &(MyVPPCustomDef.VPPCustomSetZoomXoffsetFromCenter16));
    if(eError != OMX_ErrorNone) {
        fprintf (stderr,"VPPTEST:: Error in OMX_GetExtensionIndex function\n");
        goto EXIT;
    }
    APP_DPRINT("OMX.TI.VPP.Param.ZoomXoffsetFromCenter16 is %x\n", MyVPPCustomDef.VPPCustomSetZoomXoffsetFromCenter16);
    
    eError = OMX_GetExtensionIndex(pHandleVPP, "OMX.TI.VPP.Param.ZoomYoffsetFromCenter16", &(MyVPPCustomDef.VPPCustomSetZoomYoffsetFromCenter16));
    if(eError != OMX_ErrorNone) {
        fprintf (stderr,"Error in OMX_GetExtensionIndex function\n");
        goto EXIT;
    }
    APP_DPRINT("OMX.TI.VPP.Param.ZoomYoffsetFromCenter16 is %x\n", MyVPPCustomDef.VPPCustomSetZoomYoffsetFromCenter16);
    
    eError = OMX_GetExtensionIndex(pHandleVPP, "OMX.TI.VPP.Param.FrostedGlassOvly", &(MyVPPCustomDef.VPPCustomSetFrostedGlassOvly));
    if(eError != OMX_ErrorNone) {
        fprintf (stderr,"VPPTEST:: Error in OMX_GetExtensionIndex function\n");
        goto EXIT;
    }
    APP_DPRINT("OMX.TI.VPP.Param.FrostedGlassOvly is %x\n", (MyVPPCustomDef.VPPCustomSetFrostedGlassOvly));


    eError = OMX_GetExtensionIndex(pHandleVPP, "OMX.TI.VPP.Param.VideoColorRange", &(MyVPPCustomDef.VPPCustomSetColorRange));
    if(eError != OMX_ErrorNone) {
        fprintf (stderr,"VPPTEST:: Error in OMX_GetExtensionIndex function\n");
        goto EXIT;
    }
    APP_DPRINT("OMX.TI.VPP.Param.FrostedGlassOvly is %x\n", (MyVPPCustomDef.VPPCustomSetColorRange));
    
    eError = OMX_GetExtensionIndex(pHandleVPP, "OMX.TI.VPP.Param.RGB4ColorFormat", &(MyVPPCustomDef.VPP_CustomRGB4ColorFormat));
    if(eError != OMX_ErrorNone) {
        fprintf (stderr,"VPPTEST:: Error in OMX_GetExtensionIndex function\n");
        goto EXIT;
    }
    APP_DPRINT("OMX.TI.VPP.Param.ZoomFactor is %x\n", MyVPPCustomDef.VPP_CustomRGB4ColorFormat);

EXIT:
        return eError;

}



#ifdef UNDER_CE
int _tmain(int argc, TCHAR **argv) 
#else
int main(int argc, char* argv[])
#endif
{
    OMX_ERRORTYPE           error = OMX_ErrorNone;
    OMX_CALLBACKTYPE AmrCaBa = {(void *)EventHandler,
                                (void*) EmptyBufferDone,
                                (void*)FillBufferDone};
    OMX_HANDLETYPE pHandle;
    OMX_U32 AppData = 100;
    OMX_PARAM_PORTDEFINITIONTYPE* pCompPrivateStruct = NULL;
    OMX_BUFFERHEADERTYPE* InputBufHeader[MAX_VPP_BUFFERS];
    OMX_BUFFERHEADERTYPE* OvlyBufHeader[MAX_VPP_BUFFERS]; 
    OMX_BUFFERHEADERTYPE* OutRGBBufHeader[MAX_VPP_BUFFERS]; 
    OMX_BUFFERHEADERTYPE* OutYUVBufHeader[MAX_VPP_BUFFERS];
     
    OMX_COMPONENTTYPE *pComponent;
    OMX_STATETYPE state;
    int retval;
#ifdef UNDER_CE
    TCHAR* szInFile = NULL;
    TCHAR* szOutFile = NULL; 
    HANDLE fIn = NULL;
    HANDLE fOut = NULL;
    HANDLE fYuvOut = NULL;
    HANDLE fInOvelay = NULL;
    TCHAR stringRGB[30];
    TCHAR stringYUV[30];
    TCHAR overlaystring[30];
    DWORD dwWritten;
#else   
    char* szInFile = NULL;
    char* szOutFile = NULL;
    FILE* fIn = NULL;
    FILE* fOut = NULL;
    FILE* fYuvOut = NULL;
    FILE* fInOvelay = NULL;
    char stringRGB[30];
    char stringYUV[30];
    char overlaystring[30];
#endif
    OMX_U16 inputwidth=0;
    OMX_U16 inputheight=0;
    OMX_U16 outputwidth=0;
    OMX_U16 outputheight=0;
    OMX_U16 inputcolor;
    OMX_U16 rgboutputcolor;
    OMX_U16 yuvoutputcolor;
    int Isoverlay;
    int IsYUVRGB;
    int bitrate=0;
    int iCurrentFrameIn = 0;
    int iCurrentOvlyFrameIn=0;
    int iCurrentRGBFrameOut = 0;
    int iCurrentYUVFrameOut = 0;
    int DEINIT_FLAG = 0;
    int nTypeofAllocation;
    int feature_param[4]={0,0,0,0};   /*Initialize array*/
    int feature;  /*Feature selected, only scaling, zoom, contrast, frosted glass effect, cropping, mirror and rotation*/ 
    OMX_COLOR_FORMATTYPE nColor;        /*Used to pass Color Format for input and output ports*/
    fd_set rfds;
    int fdmax;
    OMX_U8 *pInBuffer = NULL;
    OMX_U8 *pYUVBuffer = NULL;
    OMX_U8 *pRGBBuffer = NULL;
    int nRead = 0;
    int done = 0;
    int frmCount = 0;
    int nCounter =0;
    int count_stop_restart=0;   /* Executing-->Idle-->Executing*/
    int count_stop_load=0;  /* Loaded-->Idle-->Executing-->Idle-->Loaded */
    int max_count_stop_restart=0;
    int max_count_stop_load=0;
    OMX_BUFFERHEADERTYPE* pBuffer = NULL;  /*To Hold Input Buffers*/
    OMX_BUFFERHEADERTYPE* pBuf = NULL;     /*To Hold Output Buffers*/
    OMX_PARAM_PORTDEFINITIONTYPE *portinput;
    int nFillThisBufferYUV=0;
    int nFillThisBufferRGB=0;
    int nTimeouts =0;
    int bPauseResume=OMX_FALSE;
    int bStopRestart=OMX_FALSE;
    int bStopNotFree=OMX_FALSE;
    int bStopExit=OMX_FALSE;
    int MAX_VPP_BUFFERS_IN_USE = MAX_VPP_BUFFERS;
    sigset_t set;	
    
    /* validate command line args */
    if(argc < 13) {
#ifdef UNDER_CE
        printf("usage: %S <input.yuv><file_desc><Inp. Width><Inp. Height><Inp. color><0:no overlay/1:overlay><Out. Width><Out Height><yuv color><rgb color><0 :Internal 1 :external allocation><Feature [0-8]>\n",
                argv[0]);
        printf("./VPPTest_common patterns/qciftest.yuv qcif_qqcif  176 144 1 0 88 72 1 2 0 0\n");
#else
        printf("usage: %s <input.yuv><file_desc><Inp. Width><Inp. Height><Inp. color><0:no overlay/1:overlay><Out. Width><Out Height><yuv color><rgb color><0 :Internal 1 :external allocation><Feature [0-8]>\n",
                argv[0]);               
        printf("./VPPTest_common patterns/qciftest.yuv qcif_qqcif  176 144 1 0 88 72 1 2 0 0\n");
#endif
        return -1;
    }

    szInFile = argv[1];
    szOutFile = argv[2];
#ifdef UNDER_CE
    inputwidth=_wtoi(argv[3]);
    inputheight=_wtoi(argv[4]);
    inputcolor=_wtoi(argv[5]);
    Isoverlay = _wtoi(argv[6]); 
    outputwidth=_wtoi(argv[7]);
    outputheight=_wtoi(argv[8]);    
    yuvoutputcolor= _wtoi(argv[9]);
    rgboutputcolor = _wtoi(argv[10]);
    nTypeofAllocation = _wtoi(argv[11]); 
    feature= (_wtoi(argv[12])& (0x00FF);  /*Use only lower byte*/
    bPauseResume = (_wtoi(argv[12]) & (0x0100);  /*Bit 8 is PauseResumeFlag*/
    bStopRestart = (_wtoi(argv[12]) & (0x0200);  /*Bit 9 is StopRestart Flag*/
    bStopNotFree = (_wtoi(argv[12]) & (0x0400);  /*Bit 10 is Stop without freeing component Flag*/
    bStopExit = (_wtoi(argv[12]) & (0x0800);     /*Bit 11 is Stop without finishing the procesed image*/

#else
    inputwidth=atoi(argv[3]);
    inputheight=atoi(argv[4]);
    inputcolor=atoi(argv[5]);
    Isoverlay =atoi(argv[6]);   
    outputwidth=atoi(argv[7]);
    outputheight=atoi(argv[8]);     
    yuvoutputcolor= atoi(argv[9]);
    rgboutputcolor = atoi(argv[10]);
    nTypeofAllocation = atoi(argv[11]); 
    feature=atoi(argv[12]) & (0x00FF);  /*Use only lower byte*/
    bPauseResume = atoi(argv[12]) & (0x0100);  /*Bit 8 is PauseResumeFlag*/
    bStopRestart = atoi(argv[12]) & (0x0200);  /*Bit 9 is StopRestart Flag*/
    bStopNotFree = atoi(argv[12]) & (0x0400);  /*Bit 10 is Stop without freeing component Flag*/
    bStopExit = atoi(argv[12]) & (0x0800);     /*Bit 11 is Stop without finishing the procesed image*/
#endif

    /* 1600 x 1200 is only a rough guess */
    if ((inputwidth * inputheight) >= (1600 * 1200))
    {
        MAX_VPP_BUFFERS_IN_USE = 1;
    }

    if(yuvoutputcolor == 0 && rgboutputcolor ==0)
    {
        printf("VPPTEST:: At least one output is required\n");
        printf("VPPTEST:: Selecting YUV420 as default.\n");
        yuvoutputcolor=1;
    }
    if(yuvoutputcolor && rgboutputcolor)  /*Simultaneous output*/
    {
        IsYUVRGB=2;
    }
    else if(yuvoutputcolor)  /*Only YUV output*/
    {
        IsYUVRGB=0;
    }
    else
    {
        IsYUVRGB=1;     /*Only RGB output*/
    }
    
    /* Assign Parameters according to feature selected*/
    switch (feature)  /*Assign feature selected feature parameters*/
    {
        case 0 : /*Only Scaling Needed */
        {
            if(argc>13)
            {
                printf("VPPTEST:: Only Scaling Selected, ignoring Extra parameters\n");
            }
                        
            break;
        }
        
        case 1:     /*On Demand Zoom */
        case 2:     /*Dynamic Zoom  */
        {
            if(argc < 15){
#ifdef UNDER_CE
                printf("usage: %S <input.yuv> <output desc> <Inp. width> <Inp. Height> <0:no overlay/1:overlay> <outputwidth> <outputheight> <0:YUV/1:RGB/2:BOTH>  <0 :Internal 1 :external allocation>\
                    <Feature 1= On Demand/2=Dynamic>    <On Demand Zoom/Dynamic Zoom> <Zoom Factor/Zoom Speed> <Zoom Limit>\n",argv[0]);
#else
                printf("usage: %s <input.yuv> <output desc> <Inp. width> <Inp. Height> <0:no overlay/1:overlay> <outputwidth> <outputheight> <0:YUV/1:RGB/2:BOTH>  <0 :Internal 1 :external allocation>\
                    <Feature 1= On Demand/2=Dynamic>    <On Demand Zoom/Dynamic Zoom> <Zoom Factor/Zoom Speed> <Zoom Limit>\n",argv[0]);
#endif
                return -1;
            }
            else{
#ifdef UNDER_CE
                feature_param[0] = _wtoi(argv[13]);  /*Zoom Factor or Zoom Speed*/
                feature_param[1] = _wtoi(argv[14]); /*Zoom Limit */
#else
                feature_param[0] = atoi(argv[13]);  /*Zoom Factor or Zoom Speed*/
                feature_param[1] = atoi(argv[14]);  /*Zoom Limit */
#endif
            }
            if(argc>15){
                printf("On Demand Zoom Selected, Ignoring Extra parameters\n");
            }

            break;
        }
        case 3:  /*Zoom Offset*/
        {
            if(argc<17){
#ifdef UNDER_CE
                printf("usage: %S <input.yuv> <output.rgb> <inputwidth> <inputheight> <0:no overlay/1:overlay>  <outputwidth> <outputheight> <0:YUV/1:RGB/2:BOTH>  <0 :Internal 1 :external allocation>\
                    <Feature 3=Zoom Offset> <Zoom Factor> <Zoom Limit> <X Offset> <Y Offset>\n",argv[0]);
#else
                printf("usage: %s <input.yuv> <output.rgb> <inputwidth> <inputheight> <0:no overlay/1:overlay>  <outputwidth> <outputheight> <0:YUV/1:RGB/2:BOTH>  <0 :Internal 1 :external allocation>\
                    <Feature 3=Zoom Offset> <Zoom Factor> <Zoom Limit> <X Offset> <Y Offset>\n",argv[0]);
#endif
                return -1;
            }
            else{
#ifdef UNDER_CE
                feature_param[0] = _wtoi(argv[13]); /*Zoom Factor*/
                feature_param[1] = _wtoi(argv[14]); /*Zoom Limit*/
                feature_param[2] = _wtoi(argv[15]); /*X Offset*/
                feature_param[3] = _wtoi(argv[16]); /*Y Offset*/
#else               
                feature_param[0] = atoi(argv[13]);  /*Zoom Factor*/
                feature_param[1] = atoi(argv[14]);  /*Zoom Limit*/
                feature_param[2] = atoi(argv[15]);  /*X Offset*/
                feature_param[3] = atoi(argv[16]);  /*Y Offset*/
#endif              
            }
            if(argc>17){
                printf("VPPTEST:: Zoom Offset Selected, Ignoring Extra parameters\n");
            }

            break;  
        }

        case 4 : /* Contrast */
        {
            if(argc<14) 
            {
#ifdef UNDER_CE
                printf("usage: %S <input.yuv> <output.rgb> <width> <height> <0:no overlay/1:overlay>  <outputwidth> <outputheight> <0:YUV/1:RGB/2:BOTH>  <0 :Internal 1 :external allocation>\
                    <Feature 4=Contrast> <Contrast Value [-100 100]>\n",argv[0]);
#else
                printf("usage: %s <input.yuv> <output.rgb> <width> <height> <0:no overlay/1:overlay>  <outputwidth> <outputheight> <0:YUV/1:RGB/2:BOTH>  <0 :Internal 1 :external allocation>\
                    <Feature 4=Contrast> <Contrast Value [-100 100]>\n",argv[0]);
#endif
                return -1;              
            }
            else{
#ifdef UNDER_CE
                feature_param[0] = _wtoi(argv[13]);
#else
                feature_param[0]=atoi(argv[13]);
#endif                  
            }
            if(argc>14){
                printf("VPPTEST:: Contrast Selected, Ignoring extra parameters\n");
            }

            break;
        }

        case 5 :  /*Frosted Glass effect */
        {
            if(Isoverlay ==0){
                printf("VPPTEST:: Overlay is needed for Frosted Glass effect\n");
                return -1;
            }
                
            break;
        }

        case 6 : /*Cropping*/
        {
            if(argc<17){
#ifdef UNDER_CE
                printf("usage: %S <input.yuv> <output.rgb> <width> <height> <0:no overlay/1:overlay>  <outputwidth> <outputheight> <0:YUV/1:RGB/2:BOTH>  <0 :Internal 1 :external allocation>\
                    <Feature 6=Cropping> <Left Coord> <Width> <Top coord> <Height>\n",argv[0]);
#else
                printf("usage: %s <input.yuv> <output.rgb> <width> <height> <0:no overlay/1:overlay>  <outputwidth> <outputheight> <0:YUV/1:RGB/2:BOTH>  <0 :Internal 1 :external allocation>\
                    <Feature 6=Cropping> <Left Coord> <Width> <Top Coord> <Height>\n",argv[0]);
#endif
                return -1;              
            }
            else{
#ifdef UNDER_CE
                feature_param[0] = _wtoi(argv[13]); /*Left Pixel*/
                feature_param[1] = _wtoi(argv[14]); /*Width*/
                feature_param[2] = _wtoi(argv[15]); /*Top Pixel*/
                feature_param[3] = _wtoi(argv[16]); /*Height*/
#else
                feature_param[0] = atoi(argv[13]);  /*Left Pixel*/
                feature_param[1] = atoi(argv[14]);  /*Width*/
                feature_param[2] = atoi(argv[15]);  /*Top Pixel*/
                feature_param[3] = atoi(argv[16]);  /*Height*/
#endif              
            }
            if(argc>17){
                printf("VPPTEST:: Cropping Selected, Ignoring Extra parameters\n");
            }
            
            break;
        }

        case 7 : /* Mirroring */
        {
            if(!IsYUVRGB){
                printf("VPPTEST:: Mirrored Image is only possible for RGB output\n");
                return -1;
            }
            break;
        }
        
        case 8: /*Rotation*/
        {
            if(argc<14){
                printf("usage: %s <input.yuv> <output descriptor> <Inp. width> <Inp. height> <Inp. Color> <0:no overlay/1:overlay>  <outputwidth> <outputheight> <YUV output format> <RGB format>  <0 :Internal 1 :external allocation>\
                    <Feature 8=Rotation> <Rotation Angle = 0,90,180,270>\n",argv[0]);
                return -1;              
            }
            else{
#ifdef UNDER_CE
                feature_param[0]=_wtoi(argv[13]);
#else
                feature_param[0]=atoi(argv[13]);
#endif              
            }

            break;
        }
        case 9 : /* Dithering */
        {
            if(!IsYUVRGB){
                printf("VPPTEST:: Dithering is only possible for RGB output\n");
                return -1;
            }
            break;
        }
        case 10: /*Video Color Range Conversion*/
            {

#ifdef UNDER_CE
                feature_param[0]=_wtoi(argv[13]);
#else
                feature_param[0]=atoi(argv[13]);
#endif      
                break;

            }

        
        default :
        {
#ifdef UNDER_CE
            printf("Not a Valid Option on Selected Feature\n");
            printf("usage: %S <Num><input.yuv> <output.rgb> <width> <height> <0:no overlay/1:overlay>  <outputwidth> <outputheight> <0:YUV/1:RGB/2:BOTH>  <0 :Internal 1 :external allocation>\
                <Feature [0:9]> <...>\n",argv[0]);
#else
            printf("Not a Valid Option on Selected Feature\n");
            printf("usage: %s <Num><input.yuv> <output.rgb> <width> <height> <0:no overlay/1:overlay>  <outputwidth> <outputheight> <0:YUV/1:RGB/2:BOTH>  <0 :Internal 1 :external allocation>\
                <Feature [0:9]> <...>\n",argv[0]);
#endif
                return -1;              
            break;
        }
    }  /*End switch*/

    
    printf("Test Core 1.8 - " __DATE__ ":" __TIME__ "\n");
    printf("------------------------------------------------\n");
      /*Buffer flow can happen here*/
    if(Isoverlay){
#ifdef UNDER_CE
        wsprintf(overlaystring,L"play_qCIF.raw");
        printf("Overlay file: %S\n",overlaystring);
#else       

        sprintf(overlaystring,"/omx/patterns/play_qCIF.raw");
        printf("Overlay file: %s\n",overlaystring);
#endif      
    }
     

#ifndef UNDER_CE    
   /* check to see that the input file exists */
    struct stat sb = {0};
    int status = stat(argv[1], &sb);
    if( status != 0 ){
        fprintf(stderr, "Cannot find file %s. (%u)\n", argv[1], errno);
        goto EXIT;
    }
#endif
 
    /* Create a pipe used to queue data from the callback. */
    retval = pipe(IpBuf_Pipe);
    if(retval != 0){
        fprintf(stderr, "Error:Fill Data Pipe failed to open\n");
        goto EXIT;
    }
    /* Create a pipe used to queue data from the callback. */
    retval = pipe(OvlyBuf_Pipe);
    if(retval != 0){
        fprintf(stderr, "Error:Fill Data Pipe failed to open\n");
        goto EXIT;
    }

    retval = pipe(OpRGBBuf_Pipe);
    if(retval != 0){
        fprintf(stderr, "Error:Empty Data Pipe failed to open\n");
        goto EXIT;
    }
    retval = pipe(OpYUVBuf_Pipe);
    if(retval != 0){
        fprintf(stderr, "Error:Empty Data Pipe failed to open\n");
        goto EXIT;
    }
    /* Create a pipe used to handle events from the callback. */
    retval = pipe(Event_Pipe);
    if(retval != 0){
        fprintf(stderr, "Error:Fill Data Pipe failed to open\n");
        goto EXIT;
    }

    /* save off the "max" of the handles for the selct statement */
    fdmax = maxint(IpBuf_Pipe[0], OvlyBuf_Pipe[0]);
    fdmax = maxint(fdmax, OpRGBBuf_Pipe[0]);
    fdmax = maxint(fdmax, OpYUVBuf_Pipe[0]);
    fdmax = maxint(fdmax, Event_Pipe[0]);

#ifdef DSP_MMU_FAULT_HANDLING
/* LOAD BASE IMAGE FIRST TIME */
        LoadBaseImage();
#endif

    error = TIOMX_Init();
    if(error != OMX_ErrorNone) {
        printf("%d :: Error returned by TIOMX_init()\n",__LINE__);
        goto EXIT;
    }

/********************************************************************************************************/
    /* Load the VPP Component */
    error = TIOMX_GetHandle(&pHandle,strAmrDecoder,&AppData, &AmrCaBa);
    if( (error != OMX_ErrorNone) || (pHandle == NULL) ){
        fprintf (stderr,"Error in Get Handle function\n");
        goto EXIT;
    }
    if(bStopNotFree){
        max_count_stop_load=20;
    }
    else{
        max_count_stop_load=1;
    }
    /********************************Component is loaded here, we can loop after this point****************************************/
    for(count_stop_load=0; count_stop_load<max_count_stop_load; count_stop_load++)
    {
        error = GetComponentPortDef(pHandle, &MyVppPortDef);
        if (error != OMX_ErrorNone){
            fprintf (stderr,"Error in Get Handle function\n");
            goto EXIT;
        }
        
        error = GetVPPCustomDef(pHandle);
        if(error != OMX_ErrorNone)
        {
            fprintf (stderr,"Error in Get Handle function\n");
            goto EXIT;
        }
        
        printf("VPPTEST:: Input Height: %d, Input Width %d\n",inputheight, inputwidth); 
    
        /*Select Input Format and Buffer Size accordingly*/
        switch(inputcolor)
        {
            case 0:     /*YUV 420 Planar*/
                nColor = OMX_COLOR_FormatYUV420PackedPlanar; 
                printf("VPPTEST:: Input YUV420\n");
                break;
            case 1: /*YUV 422 Interleaved (little-endian)*/
                nColor = OMX_COLOR_FormatCbYCrY/*OMX_COLOR_FormatYCbYCr*/; 
                printf("VPPTEST:: Input YUV422 Interleaved (little-endian)\n");
                break;
            case 2: /*YUV 422 Interleaved (big-endian)*/
                nColor = OMX_COLOR_FormatYCbYCr/*OMX_COLOR_FormatYCrYCb*/;  
                printf("VPPTEST:: Input YUV422 Interleaved (big-endian)\n");
                break;
            case 3: /*RGB24 8:8:8*/
                nColor = OMX_COLOR_Format24bitRGB888; 
                printf("VPPTEST:: Input RGB24\n");
                break;
            case 4 : /*RGB16 5:6:5*/ 
                nColor = OMX_COLOR_Format16bitRGB565; 
                printf("VPPTEST:: Input RGB16\n");
                break;
            case 5 : /*RGB12 5:6:5*/
                nColor = OMX_COLOR_Format12bitRGB444; 
                printf("VPPTEST:: Input RGB12\n");
                break;
            case 6 : /*RGB8 3:3:2*/
                nColor = OMX_COLOR_Format8bitRGB332; 
                printf("VPPTEST:: Input RGB8\n");
                break;
            case 7 : /*RGB4  Look-Up-Table*/
                nColor = MyVPPCustomDef.VPP_CustomRGB4ColorFormat; 
                printf("VPPTEST:: Input RGB4\n");
                break;
            case 8 : /*GRAY8 */
                nColor = OMX_COLOR_FormatL8; 
                printf("VPPTEST:: Input GRAY8\n");
                break;
            case 9 : /* GRAY4 */
                nColor = OMX_COLOR_FormatL4; 
                printf("VPPTEST:: Input GRAY4\n");
                break;
            case 10: /* GRAY2 */
                nColor = OMX_COLOR_FormatL2; 
                printf("VPPTEST:: Input GRAY2\n");
                break;
            case 11 : /* Monochrome */
                nColor = OMX_COLOR_FormatMonochrome; 
                printf("VPPTEST:: Input Monochrome\n");
                break;
            default:
                printf("VPPTEST:: Not a valid option for Input Format\n");
                goto EXIT;
                break;
        }
    
    
        printf("VPPTEST:: %d : GetHandle Done..........\n",__LINE__);
        
        pCompPrivateStruct = malloc (sizeof (OMX_PARAM_PORTDEFINITIONTYPE));
        if(!pCompPrivateStruct){
            printf("VPPTEST:: eError = OMX_ErrorInsufficientResources");
            error = OMX_ErrorInsufficientResources;
            goto EXIT;
        }
     
        pCompPrivateStruct->nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
        pCompPrivateStruct->nVersion.s.nVersionMajor = 0x1; 
        pCompPrivateStruct->nVersion.s.nVersionMinor = 0x0; 
        pCompPrivateStruct->nVersion.s.nRevision = 0x0;     
        pCompPrivateStruct->nVersion.s.nStep = 0x0;
        pCompPrivateStruct->nPortIndex = MyVppPortDef.input_port;    
        pCompPrivateStruct->eDir = OMX_DirInput;
        pCompPrivateStruct->nBufferCountMin = 1;
        pCompPrivateStruct->nBufferCountActual = MAX_VPP_BUFFERS_IN_USE; 
        pCompPrivateStruct->bEnabled = OMX_TRUE;
        pCompPrivateStruct->bPopulated = OMX_FALSE;
        pCompPrivateStruct->eDomain = OMX_PortDomainVideo;
        pCompPrivateStruct->format.video.nFrameHeight = inputheight;
        pCompPrivateStruct->format.video.nFrameWidth = inputwidth;
        pCompPrivateStruct->format.video.eColorFormat = nColor;
        pCompPrivateStruct->format.video.nBitrate = bitrate;
        
        //Send input port config
        error = OMX_SetParameter (pHandle,OMX_IndexParamPortDefinition,
                                               pCompPrivateStruct);
        if (error != OMX_ErrorNone) {
            goto EXIT;
        } 
            
        if(Isoverlay){  /* At the moment, only qcif overlay frame is supported*/
            error = OMX_GetParameter (pHandle,OMX_IndexParamPortDefinition, pCompPrivateStruct);
            if(error != OMX_ErrorNone){
                goto EXIT;
            }
            pCompPrivateStruct->nPortIndex = MyVppPortDef.overlay_port;
            pCompPrivateStruct->eDir = OMX_DirInput;
            pCompPrivateStruct->nBufferCountActual = MAX_VPP_BUFFERS_IN_USE; 
            pCompPrivateStruct->format.video.nFrameHeight = DEFAULT_HEIGHT;
            pCompPrivateStruct->format.video.nFrameWidth = DEFAULT_WIDTH;
            pCompPrivateStruct->format.video.eColorFormat = OMX_COLOR_Format24bitRGB888;//OMX_COLOR_FormatYCbYCr 
            error = OMX_SetParameter (pHandle,OMX_IndexParamPortDefinition, pCompPrivateStruct);
            if (error != OMX_ErrorNone){
                goto EXIT;
            }
            printf("VPPTEST:: Overlay Enabled\n");
    
        }
        else {
            OMX_SendCommand(pHandle, OMX_CommandPortDisable, MyVppPortDef.overlay_port, NULL);
        }
    
        /**************************** FOR OUTPUT PORTS ***************************/
        printf("VPPTEST:: Output height: %d, Output Width: %d\n",outputheight, outputwidth);
    
        if(IsYUVRGB){ /*Select output Format and Buffer Size accordingly*/
            switch (rgboutputcolor) {
                case 1: /*RGB24 8:8:8*/
                    nColor = OMX_COLOR_Format24bitRGB888; /* changed from BGR*/ 
                    printf("VPPTEST:: %d::Output RGB24\n",__LINE__);
                    break;
                case 2: /*RGB16 5:6:5*/
                    nColor =OMX_COLOR_Format16bitRGB565;
                    printf("VPPTEST:: Output RGB16\n");
                    break;
                case 3:/*RGB12 4:4:4*/ 
                    nColor=OMX_COLOR_Format12bitRGB444;
                    printf("VPPTEST:: Output RGB12\n");
                    break;
                case 4:/*RGB8 3:3:2*/
                    nColor = OMX_COLOR_Format8bitRGB332;
                    printf("VPPTEST:: Output RGB8\n");
                    break;
                case 5: /*RGB4*/
                    nColor = MyVPPCustomDef.VPP_CustomRGB4ColorFormat;
                    printf("VPPTEST:: Output RGB4\n");
                    break;
                case 6: /*GRAY8 */
                    nColor=OMX_COLOR_FormatL8;
                    printf("VPPTEST:: Output GRAY8\n");
                    break;
                case 7:/*GRAY4*/
                    nColor = OMX_COLOR_FormatL4;
                    printf("VPPTEST:: Output GRAY4\n");
                    break;
                case 8: /*GRAY2*/
                    nColor = OMX_COLOR_FormatL2;
                    printf("VPPTEST:: Output GRAY2\n");
                    break;
                case 9: /*Monochrome*/
                    nColor = OMX_COLOR_FormatMonochrome;
                    printf("VPPTEST:: Output Monochrome\n");
                    break;
                case 10: /*RGB32*/
                    nColor = OMX_COLOR_Format32bitARGB8888; 
                    printf("VPPTEST:: Output RGB32\n");
                    break;
                default:
                    nColor =OMX_COLOR_Format16bitRGB565;
                    printf("VPPTEST:: Not a valid option, default to RGB16\n");
                    break;
            }
        }   
        
        /*Send output port config for RGB port*/
        
        if(IsYUVRGB){
            printf("VPPTEST:: configuring RGB port \n");
            error = OMX_GetParameter (pHandle,OMX_IndexParamPortDefinition,
                                                   pCompPrivateStruct);
            if (error != OMX_ErrorNone) {
                goto EXIT;
            }
            pCompPrivateStruct->nPortIndex = MyVppPortDef.rgboutput_port;
            pCompPrivateStruct->eDir = OMX_DirOutput;
            pCompPrivateStruct->nBufferCountActual = MAX_VPP_BUFFERS_IN_USE;
            pCompPrivateStruct->format.video.nFrameHeight = outputheight;
            pCompPrivateStruct->format.video.nFrameWidth = outputwidth;
            pCompPrivateStruct->format.video.eColorFormat = nColor ;/*OMX_COLOR_FormatUnused */
            error = OMX_SetParameter (pHandle,OMX_IndexParamPortDefinition,
                                                   pCompPrivateStruct);
            if (error != OMX_ErrorNone) {
                goto EXIT;
            }
            printf("VPPTEST:: RGB port has been configured\n");
        }
    
        else{
            OMX_SendCommand(pHandle, OMX_CommandPortDisable, MyVppPortDef.rgboutput_port, NULL);
        }
    
        /*Send output port config for YUV port*/
        if(IsYUVRGB ==0 || IsYUVRGB ==2){
            switch (yuvoutputcolor)
            {
            case 1: 
                nColor=OMX_COLOR_FormatYUV420PackedPlanar;
                printf("VPPTEST:: Output YUV420 Planar\n");
                break;          
            case 2: 
                nColor=OMX_COLOR_FormatYCbYCr;   /*YUV422 (YUYV)*/    
                printf("VPPTEST:: Output YUV422 YUYV\n");
                break;
            case 3: 
                nColor=OMX_COLOR_FormatCbYCrY;  /*YUV422 (UYVY)*/        
                printf("VPPTEST:: Output YUV422 UYVY\n");         
                break;
            default:
                printf("VPPTEST:: Not a valid option, default to YUV420 planar\n");
                nColor=OMX_COLOR_FormatYUV420PackedPlanar;
                break;
            }
            
            error = OMX_GetParameter (pHandle,OMX_IndexParamPortDefinition,
                        pCompPrivateStruct);
            if (error != OMX_ErrorNone) {
                goto EXIT;
            }
            pCompPrivateStruct->nPortIndex = MyVppPortDef.yuvoutput_port;
            pCompPrivateStruct->eDir = OMX_DirOutput;
            pCompPrivateStruct->nBufferCountActual = MAX_VPP_BUFFERS_IN_USE;
            pCompPrivateStruct->format.video.nFrameHeight = outputheight;
            pCompPrivateStruct->format.video.nFrameWidth = outputwidth;
            pCompPrivateStruct->format.video.eColorFormat = nColor;
    
            APP_DPRINT("VPPTEST:: Configuring YUV output port\n");
            error = OMX_SetParameter (pHandle,OMX_IndexParamPortDefinition,
                        pCompPrivateStruct);
            if (error != OMX_ErrorNone) {
                goto EXIT;
            }
            printf("VPPTEST:: YUV output port has been configured\n");
        }
        else{
            OMX_SendCommand(pHandle, OMX_CommandPortDisable, MyVppPortDef.yuvoutput_port, NULL);
        }
        
        APP_DPRINT ("Basic Function:: Sending OMX_StateIdle Command\n");


        printf("VPPTEST:: Num buffers %d\n", MAX_VPP_BUFFERS_IN_USE);
        
        /*Input Buffer Allocation*/
        pCompPrivateStruct->nPortIndex = MyVppPortDef.input_port;
       error = OMX_GetParameter (pHandle, OMX_IndexParamPortDefinition, pCompPrivateStruct);
        if (error != OMX_ErrorNone) {
            goto EXIT;
        }
        
        if(nTypeofAllocation ==0){
            for(nCounter=0; nCounter<MAX_VPP_BUFFERS_IN_USE; nCounter++){  /*MultiBuffer*/
                error = OMX_AllocateBuffer(pHandle, &InputBufHeader[nCounter], MyVppPortDef.input_port, (void *)NULL, pCompPrivateStruct->nBufferSize); 
                if(error != OMX_ErrorNone) {
                    printf("VPPTEST:: VPPTEST:: OMX_AllocateBuffer failed !!\n");
                    goto EXIT;
                }
            }
        }
        else{
            pInBuffer = malloc(pCompPrivateStruct->nBufferSize +256);
            if(pInBuffer == NULL){
                error = OMX_ErrorInsufficientResources;
                goto EXIT;
            }
            pInBuffer += 128;
            for(nCounter=0; nCounter<MAX_VPP_BUFFERS_IN_USE; nCounter++){ /*MultiBuffer*/
                error = OMX_UseBuffer(pHandle, &InputBufHeader[nCounter], 
                            MyVppPortDef.input_port, 
                            (void *)NULL, 
                            pCompPrivateStruct->nBufferSize, 
                            pInBuffer); 
                if(error != OMX_ErrorNone) {
                    printf("VPPTEST:: OMX_UseBuffer failed !!\n");
                    goto EXIT;
                }
            }
        }

        /*Overlay Buffer Allocation*/
        pCompPrivateStruct->nPortIndex = MyVppPortDef.overlay_port;
       error = OMX_GetParameter (pHandle, OMX_IndexParamPortDefinition, pCompPrivateStruct);
        if (error != OMX_ErrorNone) {
            goto EXIT;
        }
        
        if(Isoverlay){
            for(nCounter=0; nCounter<MAX_VPP_BUFFERS_IN_USE; nCounter++){  /*MultiBuffer*/
                error = OMX_AllocateBuffer(pHandle, &OvlyBufHeader[nCounter], 
                            MyVppPortDef.overlay_port, 
                            (void *)NULL, 
                            pCompPrivateStruct->nBufferSize); 
                if(error != OMX_ErrorNone) {
                    printf("VPPTEST:: OMX_AllocateBuffer failed !!\n");
                    goto EXIT;
                }
            }
        }
    
        /*RGB Buffer Allocation*/
        pCompPrivateStruct->nPortIndex = MyVppPortDef.rgboutput_port;
       error = OMX_GetParameter (pHandle, OMX_IndexParamPortDefinition, pCompPrivateStruct);
        if (error != OMX_ErrorNone) {
            goto EXIT;
        }
        
        if(IsYUVRGB){
            if(nTypeofAllocation == 0){
                for(nCounter=0; nCounter<MAX_VPP_BUFFERS_IN_USE; nCounter++){  /*MultiBuffer*/
                    error = OMX_AllocateBuffer(pHandle, &OutRGBBufHeader[nCounter], 
                                MyVppPortDef.rgboutput_port, 
                                (void *)NULL, 
                                pCompPrivateStruct->nBufferSize); 
                    if(error != OMX_ErrorNone) {
                        printf("VPPTEST:: OMX_AllocateBuffer failed !!\n");
                        goto EXIT;
                    }
                }
            }
            else{
                pRGBBuffer = malloc(pCompPrivateStruct->nBufferSize + 256);
                if(pRGBBuffer == NULL){
                    error = OMX_ErrorInsufficientResources;
                    goto EXIT;
                }
                pRGBBuffer += 128;
                for(nCounter = 0; nCounter<MAX_VPP_BUFFERS_IN_USE; nCounter++){ /*MultiBuffer*/
                    error = OMX_UseBuffer(pHandle, &OutRGBBufHeader[nCounter], 
                                MyVppPortDef.rgboutput_port, 
                                (void *)NULL, 
                                pCompPrivateStruct->nBufferSize, 
                                pRGBBuffer); 
                    if(error != OMX_ErrorNone){
                        printf("VPPTEST:: OMX_UseBuffer failed !!\n");
                        goto EXIT;
                    }
                }
            }
        }
        
        /*YUV Buffer Allocation*/
        pCompPrivateStruct->nPortIndex = MyVppPortDef.yuvoutput_port;
       error = OMX_GetParameter (pHandle, OMX_IndexParamPortDefinition, pCompPrivateStruct);
        if (error != OMX_ErrorNone) {
            goto EXIT;
        }
        
        if(IsYUVRGB ==0 || IsYUVRGB ==2){
            if(nTypeofAllocation ==0){ 
                for(nCounter=0; nCounter<MAX_VPP_BUFFERS_IN_USE; nCounter++){  /*MultiBuffer*/
                    error = OMX_AllocateBuffer(pHandle, &OutYUVBufHeader[nCounter], 
                                MyVppPortDef.yuvoutput_port, 
                                (void *)NULL, 
                                pCompPrivateStruct->nBufferSize); 
                    if(error != OMX_ErrorNone) {
                        printf("VPPTEST:: OMX_AllocateBuffer failed !!\n");
                        goto EXIT;
                    }
                }
            }
            else{
                pYUVBuffer = malloc(pCompPrivateStruct->nBufferSize +256);
                if(pYUVBuffer == NULL){
                    error = OMX_ErrorInsufficientResources;
                    goto EXIT;
                }
                pYUVBuffer += 128;
                for(nCounter=0; nCounter<MAX_VPP_BUFFERS_IN_USE; nCounter++){  /*MultiBuffer*/
                    error = OMX_UseBuffer(pHandle, &OutYUVBufHeader[nCounter], 
                                MyVppPortDef.yuvoutput_port, 
                                (void *)NULL, 
                                pCompPrivateStruct->nBufferSize, 
                                pYUVBuffer); 
                    if(error != OMX_ErrorNone) {
                        printf("VPPTEST:: OMX_UseBuffer failed !!\n");
                        goto EXIT;
                    }
                }
            }
        }

        error = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
        if(error != OMX_ErrorNone){
            fprintf (stderr,"VPPTEST:: Error from SendCommand-Idle(Init) State function\n");
            goto EXIT;
        } 
        printf("VPPTEST:: Sending component to Idle State\n");
        /* Wait for startup to complete */
        error = WaitForState(pHandle, OMX_StateIdle);
        if(error != OMX_ErrorNone) {
            fprintf(stderr, "VPPTEST:: Error:  VPP->WaitForState reports an error %X\n", error);
            goto EXIT;
        }
        state = OMX_StateIdle; 
        APP_DPRINT("Component is in Idle State\n");
            
    /*******HERE THE COMPONENT IS ALREADY IN IDLE STATE AND BUFFERS HAVE BEEN ALLOCATED********/
        if(bStopRestart) {
            max_count_stop_restart=20;
        }
        else{
            max_count_stop_restart=1;
        }
    
        for(count_stop_restart=0; count_stop_restart<max_count_stop_restart; count_stop_restart++)  /*Executing-->Idle-->Executing*/
        {
            sched_yield();
            iCurrentFrameIn = 0;
            iCurrentOvlyFrameIn=0;
            iCurrentRGBFrameOut = 0;
            iCurrentYUVFrameOut = 0;
            nFillThisBufferYUV=0;
            nFillThisBufferRGB=0;
        /***********************************OPEN THE NEEDED FILES****************************************************/
#ifdef UNDER_CE
            fIn = CreateFile(argv[1], GENERIC_READ, 0,
                                   NULL,OPEN_EXISTING, 0, NULL);
            if (INVALID_HANDLE_VALUE == fIn){
                APP_DPRINT("Error:  failed to open the file %S for readonly" \
                     "access\n", argv[1]);
                goto EXIT;
            }
            
            if(IsYUVRGB){
                wsprintf(stringRGB, L"%s.raw",szOutFile);  
                
                fOut = CreateFile(stringRGB, GENERIC_WRITE, 0,
                                   NULL,CREATE_ALWAYS, 0, NULL);
                if (INVALID_HANDLE_VALUE == fOut){
                    APP_DPRINT("Error:  failed to create the output file %s\n",
                        stringRGB);
                    goto EXIT;
                }
            }
        
            if(IsYUVRGB ==0 || IsYUVRGB ==2){
                wsprintf(stringYUV, L"%s.yuv",szOutFile);  
                
                fYuvOut = CreateFile(stringYUV, GENERIC_WRITE, 0,
                                   NULL,CREATE_ALWAYS, 0, NULL);
                if( fYuvOut == INVALID_HANDLE_VALUE ){
                    APP_DPRINT("Error:  failed to create the output file %s\n",
                         stringYUV);
                    goto EXIT;
                }
            }
#else        
                
            /* Open the file of data to be rendered. */
            fIn = fopen(argv[1], "r");
            if( fIn == NULL ){
                fprintf(stderr, "VPPTEST:: %d::Error:  failed to open the file %s for readonly access\n",__LINE__,argv[1]);
                goto EXIT;
            }
        
            if(IsYUVRGB ){
        
                sprintf(stringRGB, "%s.raw",szOutFile);  
            
                fOut = fopen(stringRGB, "w");
                if( fOut == NULL ){
                    fprintf(stderr, 
                        "Error:  failed to create the output file %s\n",
                        argv[2]);
                    goto EXIT;
                }
            }
            if(IsYUVRGB ==0 || IsYUVRGB ==2){
                sprintf(stringYUV, "%s.yuv",szOutFile);  
                fYuvOut = fopen(stringYUV, "w");
                if(fYuvOut == NULL){
                    fprintf(stderr, 
                        "Error:  failed to create the output file %s\n",
                        stringYUV);
                    goto EXIT;
                }
            }
#endif      
                
                
                
            /**** ALL THE Configurables Features of VPP will happen here (Zoom, Contrast, Cropping, etc.*/
            APP_DPRINT("Configurating features...\n");
            if(Isoverlay){
                /*This should be enable in case of overlay */
                OMX_CONFIG_COLORKEYTYPE *color = malloc(sizeof(OMX_CONFIG_COLORKEYTYPE));
                if(color == NULL){
                    error = OMX_ErrorInsufficientResources;
                    goto EXIT;
                }
                /*OMX_U8  colorKey[]  = {0,250,5,3};   ARGB*/
                OMX_U8  colorKey[]  = {0,3,5,250}; /*ARGB*/
                color->nARGBMask = (int)colorKey;
                error = OMX_SetConfig (pHandle,OMX_IndexConfigCommonColorKey,
                                                   color);
                free(color);
                if (error != OMX_ErrorNone) {
                    error = OMX_ErrorBadParameter;
                    goto EXIT;
                }
            }
            error=OMX_ErrorNone;
            switch(feature)
            {
                case 0:     /*Only Scaling Selected*/
                    break;  /*No need to configure any other feature*/
                case 1:     /*On Demand Zoom*/
                {
                    error = VPP_SetZoom(pHandle, 0,feature_param[0], feature_param[1],0,0);
                    printf("VPPTEST:: Selected On Demand Zoom feature\n");
                    break;
                }
                case 2: /*Dynamic Zoom*/
                    error = VPP_SetZoom(pHandle,feature_param[0],1024,feature_param[1],0,0);
                    printf("VPPTEST:: Selected Dynamic Zoom Feature\n");
                    break;
        
                case 3:     /*Zoom Offset*/
                {
                /*  error = VPP_SetZoom(pHandle,feature,feature_param);*/
                error = VPP_SetZoom(pHandle,0, feature_param[0], feature_param[1],feature_param[2],feature_param[3]);
                printf("VPPTEST:: Selected Zoom Offset Feature\n");
                /*VPP_SetZoom(OMX_HANDLETYPE pHandle, int speed, int factor, int limit, int xoff, int yoff);*/
                    break;
                }
                case 4:  /*Contrast*/
                {
                    error = VPP_SetContrast(pHandle, feature_param[0]);
                    printf("VPPTEST:: Selected Contrast (Video Gain) Feature\n");
                    break;
                }
                case 5: /*Frosted Glass Overlay Effect*/
                {
                    error = VPP_FrostedGlassEffect(pHandle, Isoverlay);
                    printf("VPPTEST:: Selected Frosted Glass Overlay Effect\n");
                    break;
                }
                case 6: /*Cropping*/
                {
                    error = VPP_SetCrop(pHandle, feature_param[0], feature_param[1], feature_param[2], feature_param[3]);
              printf("VPPTEST:: Selected cropping Feature\n");
                    break;
                }
                case 7: /*Mirroring*/
                {
                    error = VPP_SetMirroring(pHandle,IsYUVRGB);
                    printf("VPPTEST:: Selected Mirroring Feature\n");
                    break;
                }
                case 8 : /*RotationAngle*/
                {
                    error = VPP_SetRotationAngle(pHandle, IsYUVRGB,feature_param[0]);
                    printf("VPPTEST:: Selected Rotation Feature\n");
                    break;
                }
                case 9: /*Dithering*/
                {
                    error = VPP_SetDithering(pHandle,IsYUVRGB);
                    printf("VPPTEST:: Selected Dithering Feature\n");
                    break;  
                
                }
                
                case 10:  /*Color Range Conversion*/
                    {
                        error=VPP_SetColorRange(pHandle,feature_param[0]);
                        printf("VPPTEST:: Selected Color Range\n");
                        break;
                    }
        
        
                default:
                {   error = OMX_ErrorBadParameter;
                    printf("VPPTEST:: Not a valid Option on Feature Selection\n");
                    break;
                }
            }
            
            if (error != OMX_ErrorNone) 
            {
                error = OMX_ErrorBadParameter;
                goto EXIT;
            }
        
            APP_DPRINT ("Basic Function:: Sending OMX_StateExecuting Command\n");
            error = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateExecuting, NULL);
            if(error != OMX_ErrorNone) {
                fprintf (stderr,"VPPTEST:: Error from SendCommand-Executing State function\n");
                goto EXIT;
            }
            pComponent = (OMX_COMPONENTTYPE *)pHandle;
        
        
            error = WaitForState(pHandle, OMX_StateExecuting);
            if(error != OMX_ErrorNone) {
                fprintf(stderr, "VPPTEST:: Error:  VPP->WaitForState reports an error %X\n", error);
                goto EXIT;
            }
            printf("VPPTEST:: Component is now in Executing state\n");
            state = OMX_StateExecuting;
        
            if(Isoverlay){
                fInOvelay = fopen(overlaystring, "r");
                if (!fInOvelay)
                {
                    fprintf(stderr, "Error:  failed to open the file %s for readonly access\n", overlaystring);
                    goto EXIT;   
                }
            }
#ifndef UNDER_CE             
                    /*PROFILE POINT */
                    gettimeofday(&base, NULL);
                    newer.tv_sec = base.tv_sec;
                    newer.tv_usec = base.tv_usec;
                    APP_DPRINT("sec: %ld, usec: %ld\n", base.tv_sec, base.tv_usec);
                    /*profiletime = GetProfiletime());*/
#endif
            for(nCounter=0; nCounter<MAX_VPP_BUFFERS_IN_USE; nCounter++){ /*MultiBuffer*/
                /*Send Input Buffers to the Component */
                /*Provided that at least one frame will be read*/
                nRead = fill_data (InputBufHeader[nCounter],fIn);
                if(nRead == 0){
                    APP_DPRINT("There is no data on input file\n");
                    break; /*If there is no data send no more buffers to the component*/
                            /*Exit for loop*/
                }
                else{
                    ++iCurrentFrameIn;
                    error = OMX_EmptyThisBuffer(pHandle, InputBufHeader[nCounter]);  /*INPUT port*/
                    if (error != OMX_ErrorNone) {
                        printf ("VPPTEST:: Error from OMX_EmptyThisBuffer function 0x%X\n",error);
                        goto EXIT;
                    }
                }
            
                if(Isoverlay)   /*OVERLAY PORT*/{
#ifdef UNDER_CE                   
                    fInOvelay = CreateFile(overlaystring, GENERIC_READ, 0,
                                          NULL,OPEN_EXISTING, 0, NULL);
                    if(fInOvelay == INVALID_HANDLE_VALUE){
                        APP_DPRINT("Error:  failed to open the file %s for readonly\
                                access\n", fInOvelay);
                        goto EXIT;
                    }
#else 
                    fseek(fInOvelay, 0L, SEEK_SET);
                    nRead = fill_data (OvlyBufHeader[nCounter],fInOvelay);
                    if (nRead < (DEFAULT_WIDTH * DEFAULT_HEIGHT)* 3){
                        printf("VPPTEST:: Error in fread for overlay, not enough data\n");
                    }
                    ++iCurrentOvlyFrameIn;
                    error = OMX_EmptyThisBuffer(pHandle, OvlyBufHeader[nCounter]);  /*OVERLAY port*/
                    if (error != OMX_ErrorNone) {
                        printf ("VPPTEST:: Error from OMX_EmptyThisBuffer function 0x%X\n",error);
                        goto EXIT;
                    }
                }
        #endif  
                if(IsYUVRGB){  /*Send RGB output buffers to component*/
                    OutRGBBufHeader[nCounter]->nFilledLen=0;
                    error = OMX_FillThisBuffer(pHandle,OutRGBBufHeader[nCounter]);
                    if (error != OMX_ErrorNone){
                        printf ("VPPTEST:: Error from OMX_FillThisBuffer function 0x%X\n",error);
                        goto EXIT;
                    }
                    nFillThisBufferRGB++;
                }
                if(IsYUVRGB==0 || IsYUVRGB ==2){  /*Send YUV output buffers to component*/
                    OutYUVBufHeader[nCounter]->nFilledLen=0;
                    error = OMX_FillThisBuffer(pHandle,OutYUVBufHeader[nCounter]);
                    if (error != OMX_ErrorNone) {
                        printf ("VPPTEST:: Error from OMX_FillThisBuffer function 0x%X\n",error);
                        goto EXIT;
                    }
                    nFillThisBufferYUV++;
                }
            }  /*end of for*/
            
        /**************************************************************************/
            
            nRead = 0;
            done = 0;
            frmCount = 0;
            OMX_STATETYPE MyState;
        
            MyState = OMX_StateExecuting;
            DEINIT_FLAG = OMX_FALSE;
            while ((error == OMX_ErrorNone ) && (MyState != OMX_StateIdle)) {
                FD_ZERO(&rfds);
                FD_SET(IpBuf_Pipe[0], &rfds);
                FD_SET(OvlyBuf_Pipe[0],&rfds);
                FD_SET(OpRGBBuf_Pipe[0], &rfds);
                FD_SET(OpYUVBuf_Pipe[0],&rfds);
                FD_SET(Event_Pipe[0],&rfds);
                sigemptyset(&set) ;
                sigaddset(&set, SIGALRM);
		  
                retval = pselect(fdmax+1, &rfds, NULL, NULL, NULL,&set);
                if(retval == -1) {
#ifndef UNDER_CE 
                    perror("select()");
#endif            
                    fprintf (stderr, "VPPTEST:: : Error \n");
                    break;
                }
        
                if(retval == 0) {
                    APP_DPRINT("\n\n\n%d ::!!!!!!!     App Timeout !!!!!!!!!!! \n",__LINE__);
                    APP_DPRINT("%d :: ---------------------------------------\n\n\n",__LINE__);
                    if(++nTimeouts>5){
                        APP_DPRINT("VPPTEST:: No more data from Component\n");
                        APP_DPRINT("VPPTEST:: Inputs=%d, Ovlys=%d, OutRGBs=%d,OutYUVs=%d\n",
                                    iCurrentFrameIn, iCurrentOvlyFrameIn, 
                                    iCurrentRGBFrameOut,iCurrentYUVFrameOut);
                        error = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
                        if(error != OMX_ErrorNone) {
                            fprintf (stderr,"VPPTEST:: Error from SendCommand-Idle(Init) State function\n");
                            goto EXIT;
                        }
                        error = WaitForState(pHandle, OMX_StateIdle);
                        if(error != OMX_ErrorNone) {
                            printf("VPPTEST:: Error:  hVPP->WaitForState has timed out %X", error);
                            goto EXIT;
                        }
                        APP_DPRINT("VPP has been set in Idle State now\n");
                        APP_DPRINT("Error on %s-->%s\n",szInFile,szOutFile);
                        MyState = OMX_StateIdle;
                    }
                }
                /**
                * If FD_ISSET then there is data available in the pipe.  
                * Read it and get the buffer data out.  
                * Then re-fill the buffer and send it back.
                **/
                if ( FD_ISSET(Event_Pipe[0], &rfds)) {
                    EVENT_PRIVATE EventPrivate;
                    read(Event_Pipe[0], &EventPrivate, sizeof(EVENT_PRIVATE));
                    switch(EventPrivate.eEvent) {
                        case OMX_EventError:
                            DEINIT_FLAG = OMX_TRUE;
                            bError = OMX_TRUE;
                            printf("APP:: Waiting for OMX_StateInvalid... \n");
                            WaitForState(pHandle, OMX_StateInvalid);
                            printf("APP:: At Invalid state.\n");
                            goto EXIT;
                            break;

                        case OMX_EventBufferFlag:
                            printf("APP:: Unloading component...\n");
                            break;

                        default:
                            break;
                    }
                }

                if(bPauseResume && !(iCurrentFrameIn%20)){
                    printf("VPPTEST:: Pausing the component at %d Frames\n",iCurrentFrameIn);
                    error=OMX_SendCommand(pHandle,OMX_CommandStateSet,OMX_StatePause,NULL);
                    if(error!=OMX_ErrorNone){
                        fprintf(stderr,"VPPTEST:: Error from SendCommand-Pause State Function\n");
                        goto EXIT;
                    }
                    error = WaitForState(pHandle,OMX_StatePause);
                    if(error !=OMX_ErrorNone){
                        printf("VPPTEST:: Error: hVPP->WaitForState has timed out %X", error);
                        goto EXIT;
                    }   
                    printf("VPPTEST:: Sleeping the component for 2 seconds in Pause\n");
                    sleep(2);
                    error = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
                    if(error != OMX_ErrorNone) {
                        fprintf (stderr,"Error from SendCommand-Executing State function\n");
                        goto EXIT;
                    }
                    error = WaitForState(pHandle, OMX_StateExecuting);
                    if(error != OMX_ErrorNone) {
                        printf("VPPTEST:: Error:  hVPP->WaitForState has timed out %X", error);
                        goto EXIT;
                    }
                    printf("VPPTEST:: Component is Executing again\n");
                        
                }
                
                if(bStopExit && !(iCurrentFrameIn%43)){ /*Stopping to get an output file of 40 frames jesa*/
                    printf("VPPTEST:: Stopping at output YUV frame %d and exiting\n", iCurrentYUVFrameOut);
                    if( FD_ISSET(IpBuf_Pipe[0], &rfds) ) {
                        read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
                    }
                    if( FD_ISSET(OvlyBuf_Pipe[0], &rfds) ) {
                        read(OvlyBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
                    }
                    if( FD_ISSET(OpRGBBuf_Pipe[0], &rfds) ) {
                        read(OpRGBBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
                    }
                    if( FD_ISSET(OpYUVBuf_Pipe[0], &rfds) ) {
                        read(OpYUVBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
                    }
                    fprintf(stderr, "VPPTEST:: Shutting down ---------- \n");
                    APP_DPRINT("Number of Processed Frames: Input=%d, Overlay=%d, RGB=%d, YUV=%d\n",
                                iCurrentFrameIn,iCurrentOvlyFrameIn,iCurrentRGBFrameOut,iCurrentYUVFrameOut);
                    APP_DPRINT("Number of frames received: Input=%d, Overlay=%d, RGB=%d, YUV=%d\n",
                                nInputEmptyBufferDones, nOvlyEmptyBufferDones, nRGBFillBufferDones, nYUVFillBufferDones);
                    APP_DPRINT("Number of sent Buffers: Input=%d, Overlay=%d, RGB=%d, YUV=%d\n",
                                iCurrentFrameIn,iCurrentOvlyFrameIn, nFillThisBufferRGB, nFillThisBufferYUV);
                    DEINIT_FLAG = 1;
                }       
        
                if( FD_ISSET(IpBuf_Pipe[0], &rfds) ) {
                    read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
                    portinput = pBuffer->pInputPortPrivate;
                    APP_DPRINT("%d ::App: Read from IpBuf_Pipe InBufHeader %p\n",
                                                                 __LINE__,pBuffer);
                    frmCount++;
            
                    if (!done) {
        
                        APP_DPRINT("%d ::reading INPUT DATA file \n",
                                                                   __LINE__);
                        /*If buffer corresponds to Input Port*/
                        nRead = fill_data (pBuffer,fIn);
                        APP_DPRINT(" ::reading INPUT DATA file pBuffer->nAllocLen %d    read=%d\n",
                                                                  pBuffer->nAllocLen,nRead);
                        if((nRead < pBuffer->nAllocLen) && (done == 0)){
                            fprintf(stderr, "VPPTEST:: Shutting down ---------- \n");
                            APP_DPRINT("Number of Processed Frames: Input=%d, Overlay=%d, RGB=%d, YUV=%d\n",
                                        iCurrentFrameIn,iCurrentOvlyFrameIn,iCurrentRGBFrameOut,iCurrentYUVFrameOut);
                            APP_DPRINT("Number of frames received: Input=%d, Overlay=%d, RGB=%d, YUV=%d\n",
                                        nInputEmptyBufferDones, nOvlyEmptyBufferDones, nRGBFillBufferDones, nYUVFillBufferDones);
                            APP_DPRINT("Number of sent Buffers: Input=%d, Overlay=%d, RGB=%d, YUV=%d\n",
                                    iCurrentFrameIn,iCurrentOvlyFrameIn, nFillThisBufferRGB, nFillThisBufferYUV);
                            done = 1;
                            DEINIT_FLAG = 1;
                            pBuffer->nFlags = OMX_BUFFERFLAG_EOS; 
                         }
                        else{
                            ++iCurrentFrameIn;
                            pBuffer->nFilledLen = nRead;
                            pComponent->EmptyThisBuffer(pHandle,pBuffer);
                            APP_DPRINT("%d :: App: Sent Filled Input Buffer to Comp\n",__LINE__);
                        }
                    }
                }
                
                if(FD_ISSET(OvlyBuf_Pipe[0],&rfds) ){
                    APP_DPRINT("%d ::reading OVERLAY DATA file \n",
                                                         __LINE__);
            /*Read and process Overlay Pipe, only if Input Pipe has been processed already*/
                    if(iCurrentOvlyFrameIn<iCurrentFrameIn){
                        read(OvlyBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
#ifdef UNDER_CE                   
                        fInOvelay = CreateFile(overlaystring, GENERIC_READ, 0,
                                                  NULL,OPEN_EXISTING, 0, NULL);
                        if( fInOvelay == INVALID_HANDLE_VALUE ){
                            APP_DPRINT("Error:  failed to open the file %s for readonly\
                                        access\n", fInOvelay);
                                        goto EXIT;
                        }
#else 
                        fseek(fInOvelay, 0L, SEEK_SET);
                        nRead = fill_data(pBuffer,fInOvelay);
                        if (nRead < (DEFAULT_WIDTH * DEFAULT_HEIGHT)* 3){
                                printf("VPPTEST:: Error in fread for overlay, not enough data\n");
                        }
#endif                  
        
#ifdef UNDER_CE
                        CloseHandle(fInOvelay);
#else                        
        
#endif
                        ++iCurrentOvlyFrameIn;
                        pBuffer->nFilledLen = nRead;
                        pComponent->EmptyThisBuffer(pHandle,pBuffer);
                        APP_DPRINT("%d :: App: Sent Filled Overlay Buffer to Comp\n",__LINE__);
                    }/*end if(iCurrentOvlyFrameIn<iCurrentFrameIn)*/
                } /*end if(FD_ISSET(OvlyBuf_Pipe[0],&rfds) */
        
                if(FD_ISSET(OpRGBBuf_Pipe[0], &rfds) ) {
                    /*Only read and process RGB Output pipe if Input Pipe 
                        has been processed first*/
                    if(iCurrentRGBFrameOut<iCurrentFrameIn){
                        read(OpRGBBuf_Pipe[0], &pBuf, sizeof(pBuf));
                        APP_DPRINT("%d ::App: Read from OpBuf_Pipe OutBufHeader %p\n", __LINE__,pBuf);
                        ++iCurrentRGBFrameOut;
                        if (pBuf->nAllocLen != pBuf->nFilledLen ) {
                            if(pBuf->nFilledLen==0) printf("* WARNING: Frame has been lost (pBuf->nFilledLen is 0)\n");
                           APP_DPRINT("%d : WARNING: Different Size, %ld\n",__LINE__, pBuf->nFilledLen);
                        }
                        else{  
#ifdef UNDER_CE
                            WriteFile(fOut, pBuf->pBuffer, pBuf->nFilledLen, &dwWritten, NULL);
#else               
                fwrite(pBuf->pBuffer, 1, pBuf->nFilledLen, fOut);
#endif      
            
                            /*Send the same number of Output buffers than Input buffers*/
                            if(nFillThisBufferRGB<iCurrentFrameIn){
                                pComponent->FillThisBuffer(pHandle,pBuf);
                                nFillThisBufferRGB++;   
                            }
                        }
                    }/*end if((iCurrentRGBFrameOut<iCurrentFrameIn)*/
                } /*end of FD_ISSET(OpRGBBuf_Pipe[0], &rfds)*/
                
                if(FD_ISSET(OpYUVBuf_Pipe[0], &rfds) ) {
                    /*Read and Process Output YUV Pipe, only if Input Pipe has
                    been read and processed first */
                    if(iCurrentYUVFrameOut < iCurrentFrameIn){
                        read(OpYUVBuf_Pipe[0], &pBuf, sizeof(pBuf));
                        APP_DPRINT("%d ::App: Read from OpYUVBuf_Pipe OutBufHeader %p\n", __LINE__,pBuf);
                        ++iCurrentYUVFrameOut;   
                        if (pBuf->nAllocLen != pBuf->nFilledLen ) {
                             if(pBuf->nFilledLen==0) printf("* WARNING: Frame has been lost (pBuf->nFilledLen is 0)\n");
                            APP_DPRINT("%d : WARNING: Different Size, %ld\n",__LINE__, pBuf->nFilledLen);          
                        }
                        else{       
        
#ifdef UNDER_CE
                            WriteFile(fOut, pBuf->pBuffer, pBuf->nFilledLen, &dwWritten, NULL);
#else
            fwrite(pBuf->pBuffer, 1, pBuf->nFilledLen, fYuvOut);
#endif
        
                    /*Send the same number of output buffers than input buffers
                     to the component*/
                            if(nFillThisBufferYUV<iCurrentFrameIn){
                                pComponent->FillThisBuffer(pHandle,pBuf);   
                                nFillThisBufferYUV++;
                            }
                        }
        
                    }/*end if(iCurrentYUVFrameOut < iCurrentFrameIn)*/
                    APP_DPRINT("%d :: App: %ld bytes are being written\n",__LINE__,(pBuf->nFilledLen/4));
                
                }/*end if (FD_ISSET(OpYUVBuf_Pipe[0], &rfds)*/
        
                    if (DEINIT_FLAG &&  
                        ((iCurrentFrameIn == iCurrentRGBFrameOut) ||
                        (iCurrentFrameIn == iCurrentYUVFrameOut) ||
                        (bStopExit)
                        )){
                        APP_DPRINT("Read full file!!!\n");
                        done = 1;
                        error = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateIdle, NULL);
                        if(error != OMX_ErrorNone) {
                            fprintf (stderr,"Error from SendCommand-Idle(Stop) State function\n");
                            goto EXIT;
                        } 
                        APP_DPRINT("Waiting for OMX_StateIdle... \n");
                        error = WaitForState(pHandle, OMX_StateIdle);
                        if(error != OMX_ErrorNone) {
                            APP_DPRINT("Error:  VPP->WaitForState has timed out %X", 
                                error);
                            goto EXIT;
                        }
                    }/*end of if DEINIT_FLAG*/      
        
        
                if(done == 1) {
                    error = pComponent->GetState(pHandle, &state);
                    if(error != OMX_ErrorNone) {
                        fprintf(stderr, "Warning:  VPP->GetState has returned status %X\n", error);
                        goto EXIT;
                    }
                }
            OMX_GetState(pHandle, &MyState);
        
            }
            APP_DPRINT("VPPTESTDEBUG::Exiting While loop, closing all the file handles\n");
            APP_DPRINT("Number of Processed Frames: Input=%d, Overlay=%d, RGB=%d, YUV=%d\n",
                            iCurrentFrameIn,iCurrentOvlyFrameIn,iCurrentRGBFrameOut,iCurrentYUVFrameOut);
            printf("VPPTEST:: Component is now in Idle State\n");                    
#ifdef UNDER_CE 
            CloseHandle(fIn);
            if(IsYUVRGB )
                CloseHandle(fOut);
            if(IsYUVRGB ==0 || IsYUVRGB ==2)
                CloseHandle(fYuvOut);
#else          
            fclose(fIn);
            if(IsYUVRGB)
                fclose(fOut);
            if (Isoverlay)
                fclose(fInOvelay);
            if(IsYUVRGB ==0 || IsYUVRGB ==2)
            fclose(fYuvOut);
#endif
            if(MyState == OMX_StateInvalid){
                break; /*to exit the Executing-->Idle-->Executing cicle*/
            }
        } /* Executing-->Idle-->Executing*/         
    
    /***************************************************************************************************/ 
        APP_DPRINT("Total processing time = %ld\n", nTotalTime);
    
        OMX_STATETYPE MyState;
        OMX_GetState(pHandle, &MyState);
        
        if(MyState != OMX_StateInvalid){
            APP_DPRINT ("%d :: App: State Of Component Is Idle Now\n",__LINE__);
            APP_DPRINT("Sending the StateLoaded Command\n");
            error = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateLoaded, NULL);
            if(error != OMX_ErrorNone) {
                fprintf (stderr,"Error from SendCommand-Idle State function\n");
                goto EXIT;
            } 
            /*Disable the ports before freeing the buffers*/
            
            error = OMX_SendCommand(pHandle,OMX_CommandPortDisable, MyVppPortDef.input_port, NULL);
            if(error != OMX_ErrorNone){
                APP_DPRINT("VPPTEST:: %d::Error from SendCommand-->PortDisable\n",__LINE__);
                goto EXIT;
            }
            if(Isoverlay){
                error = OMX_SendCommand(pHandle,OMX_CommandPortDisable, MyVppPortDef.overlay_port, NULL);
                if(error != OMX_ErrorNone){
                    APP_DPRINT("VPPTEST:: %d::Error from SendCommand-->PortDisable\n",__LINE__);
                    goto EXIT;
                }
            }
            if(IsYUVRGB){
                error = OMX_SendCommand(pHandle,OMX_CommandPortDisable,MyVppPortDef.rgboutput_port, NULL);
                if(error != OMX_ErrorNone){
                    APP_DPRINT("VPPTEST:: %d::Error from SendCommand-->PortDisable\n",__LINE__);
                    goto EXIT;
                }
            }
            if(IsYUVRGB==0 || IsYUVRGB==2){
                error = OMX_SendCommand(pHandle,OMX_CommandPortDisable,MyVppPortDef.yuvoutput_port, NULL);
                if(error != OMX_ErrorNone){
                    APP_DPRINT("VPPTEST:: %d::Error from SendCommand-->PortDisable\n",__LINE__);
                    goto EXIT;
                }
            }
        }
        if(nTypeofAllocation == 1){
            if(pInBuffer){
                free (pInBuffer);
                pInBuffer = NULL;
            }
            if(pYUVBuffer){
                free(pYUVBuffer);
                pYUVBuffer = NULL;
            }
            if(pRGBBuffer){
                free(pRGBBuffer);
                pYUVBuffer = NULL;
            }
        }
        for(nCounter=0; nCounter<MAX_VPP_BUFFERS_IN_USE; nCounter++){  /* MultiBuffer*/
            error = OMX_FreeBuffer(pHandle, MyVppPortDef.input_port, InputBufHeader[nCounter]) ;
            if(error != OMX_ErrorNone) {
                printf("VPPTEST:: free buffer failed !!\n");
                goto EXIT;
            }
    
            if(Isoverlay){
            error = OMX_FreeBuffer(pHandle, MyVppPortDef.overlay_port, OvlyBufHeader[nCounter]);
                if(error != OMX_ErrorNone) {
                    printf("VPPTEST:: OMX_FreeBuffer failed !!\n");
                    goto EXIT;
                }
            }
    
            if(IsYUVRGB)
            {
                error = OMX_FreeBuffer(pHandle, MyVppPortDef.rgboutput_port, OutRGBBufHeader[nCounter]);
                if(error != OMX_ErrorNone) {
                    printf("VPPTEST:: OMX_FreeBuffer failed !!\n");
                    goto EXIT;
                }
            }
    
            if(IsYUVRGB ==0 || IsYUVRGB ==2){
                error = OMX_FreeBuffer(pHandle, MyVppPortDef.yuvoutput_port, OutYUVBufHeader[nCounter]);
                if(error != OMX_ErrorNone) {
                    printf("VPPTEST:: OMX_FreeBuffer failed !!\n");
                    goto EXIT;
                }
            }
        }//end of for loop
    
        /* Wait for startup to complete */
        error = WaitForState(pHandle, OMX_StateLoaded);
        if((error != OMX_ErrorNone) && (error != OMX_ErrorInvalidState)){
            fprintf(stderr, "Error:  VPP->WaitForState reports an error %X\n", error);
            goto EXIT;
        }
        free(pCompPrivateStruct);   /*Free all m(allocated) resources to avoid memory leaks*/
    
        printf ("VPPTEST:: App: State Of Component Is Loaded Now\n");
       
       if(MyState == OMX_StateInvalid){
            break; /*to exit for (count_stop_load ...) */
       }
       
    }  /*end for(count_stop_load=0; count_stop_load<2; count_stop_load++)*/

EXIT:
    
    printf ("VPPTEST:: Free the Component handle\n"); 
    error = TIOMX_FreeHandle(pHandle);
    if( (error != OMX_ErrorNone)) {
        fprintf (stderr,"Error in Free Handle function\n");
        goto EXIT;
    }
    fprintf (stderr,"Free Handle returned Successfully \n");

#ifdef DSP_MMU_FAULT_HANDLING
    if(bError) {
        LoadBaseImage();
    }
#endif

    
        /* De-Initialize OMX Core */
    error = TIOMX_Deinit();
    if (error != OMX_ErrorNone) {
        printf("VPPTEST:: Failed to de-init OMX Core!\n");
        goto EXIT;
    }
    
    if(error != OMX_ErrorNone){
        if(pCompPrivateStruct){
            free(pCompPrivateStruct);
            pCompPrivateStruct = NULL;
        }
        if(pInBuffer){
            pInBuffer -= 128;
            free(pInBuffer);
            pInBuffer = NULL;
        }
        if(pRGBBuffer){
            pRGBBuffer -= 128;
            free(pRGBBuffer);
            pRGBBuffer = NULL;
        }
        if(pYUVBuffer){
            pYUVBuffer -= 128;
            free(pYUVBuffer);
            pYUVBuffer = NULL;
        }
    }
    return error;
}



#ifndef UNDER_CE
int fill_data (OMX_BUFFERHEADERTYPE *pBuf, FILE *fIn)
#else
int fill_data (OMX_BUFFERHEADERTYPE *pBuf, HANDLE fIn)
#endif
{
    int nRead;
    static int totalRead = 0;
    OMX_U8 *pTmp = NULL;
    pTmp = pBuf->pBuffer;
     APP_DPRINT(" :: ------------- before File Read -------------- %p\n",pBuf);
     APP_DPRINT(" :: ------------- before File Read -------------- %d\n",pBuf->nAllocLen);
     APP_DPRINT(" :: ------------- before File Read -------------- %p\n",pBuf->pBuffer);
#ifndef UNDER_CE
    nRead = fread(pBuf->pBuffer, 1, pBuf->nAllocLen, fIn);
#else
    ReadFile(fIn, pBuf->pBuffer, pBuf->nAllocLen, &nRead, NULL);
#endif
    APP_DPRINT("\n%d :: ------------- App File Read --------------\n",__LINE__);
    APP_DPRINT ("App: Read %d bytes from file\n", nRead);
    APP_DPRINT ("App: pBuf->nAllocLen = %ld\n",pBuf->nAllocLen);
    APP_DPRINT("%d :: ------------- App File Read --------------\n\n",__LINE__);

    pBuf->nFilledLen = nRead;
    totalRead += nRead;
    return nRead;
}



static OMX_ERRORTYPE VPP_SetZoom(OMX_HANDLETYPE pHandle, int speed, int factor, int limit, int xoff, int yoff)
{
    int nZoomSpeed = speed;
    int nZoomFactor = factor<<10;
    int nZoomLimit = limit<<10;
    int nXoff = xoff<<4;
    int nYoff=yoff<<4;

    OMX_ERRORTYPE error = OMX_ErrorNone;

    error = OMX_SetConfig (pHandle, MyVPPCustomDef.VPPCustomSetZoomFactor, &nZoomFactor);
    if(error != OMX_ErrorNone){         
        fprintf(stderr,"Error configuring Zoom Factor\n");                          
        return error;
    }
    error = OMX_SetConfig (pHandle,MyVPPCustomDef.VPPCustomSetZoomSpeed,&nZoomSpeed);
    if(error != OMX_ErrorNone){         
        fprintf(stderr,"Error configuring Zoom Factor\n");                          
        return error;
    }

    error = OMX_SetConfig (pHandle,MyVPPCustomDef.VPPCustomSetZoomLimit,&nZoomLimit);
    if(error != OMX_ErrorNone){                                                     
        fprintf(stderr,"Error configuring Zoom Limit/n");
        return error;
    }
    error = OMX_SetConfig(pHandle,MyVPPCustomDef.VPPCustomSetZoomXoffsetFromCenter16,&nXoff);
    if(error != OMX_ErrorNone){                                 
        fprintf(stderr,"Error Configuring Horizontal Zoom Offset\n.");
        return error;
    }   
    error = OMX_SetConfig(pHandle,MyVPPCustomDef.VPPCustomSetZoomYoffsetFromCenter16,&nYoff);
    if(error != OMX_ErrorNone){                                                                 
        fprintf(stderr,"Error Configuring Vertical Zoom Offset");                               
        return error;
    }

    return error;
}



static OMX_ERRORTYPE VPP_SetContrast(OMX_HANDLETYPE pHandle, int Contrast)
{
    OMX_ERRORTYPE error = OMX_ErrorNone;
    OMX_CONFIG_CONTRASTTYPE *pConfigNewContrast = malloc(sizeof(OMX_CONFIG_CONTRASTTYPE));
    if(pConfigNewContrast == NULL){
        error = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    memset(pConfigNewContrast,0,sizeof(OMX_CONFIG_CONTRASTTYPE));
    pConfigNewContrast->nContrast = Contrast;
    error=OMX_SetConfig(pHandle,OMX_IndexConfigCommonContrast,pConfigNewContrast);
    if(error != OMX_ErrorNone){
        printf("VPPTEST:: VPPTest Error at %d\n",__LINE__);
        error = OMX_ErrorBadParameter;
        goto EXIT;
    }

EXIT:
    if ( error != OMX_ErrorNone){
        if(pConfigNewContrast){
            free(pConfigNewContrast);
            pConfigNewContrast = NULL;
        }
    }
    return error;
}



static OMX_ERRORTYPE VPP_FrostedGlassEffect(OMX_HANDLETYPE pHandle, int IsOverlay)
{
    OMX_ERRORTYPE error = OMX_ErrorNone;
        
    if(IsOverlay){
        int FrostedGlassEffect=1;           
        error=OMX_SetConfig(pHandle,MyVPPCustomDef.VPPCustomSetFrostedGlassOvly,&FrostedGlassEffect);
        if(error != OMX_ErrorNone){
            printf("VPPTEST:: VPPTest Error at %d\n",__LINE__);
            error = OMX_ErrorBadParameter;
            return error;
        }
    }
    else{
        printf("VPPTEST:: Frosted Glass Effect is only upon overlayed images.\n");
        error=OMX_ErrorBadParameter;
    }
    return error;
}



static OMX_ERRORTYPE VPP_SetCrop(OMX_HANDLETYPE pHandle, int XStart, int XSize, int YStart, int YSize)
{
    OMX_ERRORTYPE error = OMX_ErrorNone;
    OMX_CONFIG_RECTTYPE *pConfigNewCropWindow = malloc (sizeof(OMX_CONFIG_RECTTYPE));
    if(pConfigNewCropWindow == NULL){
        error = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    memset(pConfigNewCropWindow, 0, sizeof(OMX_CONFIG_RECTTYPE));
    pConfigNewCropWindow->nLeft   = XStart;
    pConfigNewCropWindow->nTop    = YStart;
    pConfigNewCropWindow->nWidth  = XSize;
    pConfigNewCropWindow->nHeight = YSize;
    error = OMX_SetConfig (pHandle, OMX_IndexConfigCommonInputCrop, pConfigNewCropWindow);
    if (error != OMX_ErrorNone) {
        printf("VPPTest Error at %d\n",__LINE__);
        error = OMX_ErrorBadParameter;
        goto EXIT;
    }

EXIT:
    if(error != OMX_ErrorNone){
        free(pConfigNewCropWindow);
        pConfigNewCropWindow = NULL;
    }
    return error;
}

static OMX_ERRORTYPE VPP_SetMirroring(OMX_HANDLETYPE pHandle, int IsRGBOutput)
{
    OMX_ERRORTYPE error = OMX_ErrorNone;
    OMX_CONFIG_MIRRORTYPE * pConfigNewMirror = NULL;
    if(IsRGBOutput){
        pConfigNewMirror = malloc(sizeof(OMX_CONFIG_MIRRORTYPE));
        if(pConfigNewMirror == NULL){
            error = OMX_ErrorInsufficientResources;
            goto EXIT;
        }
        memset(pConfigNewMirror,0,sizeof(OMX_CONFIG_MIRRORTYPE));
        pConfigNewMirror->nPortIndex = MyVppPortDef.rgboutput_port;
        pConfigNewMirror->eMirror = OMX_MirrorHorizontal;
        error=OMX_SetConfig(pHandle,OMX_IndexConfigCommonMirror,pConfigNewMirror);
        if(error != OMX_ErrorNone){
            printf("VPPTest Error at %d\n",__LINE__);
            error = OMX_ErrorBadParameter;
            goto EXIT;
        }
    }
    else{
        printf("VPPTEST:: Need to use RGB as Output, Mirror Setting unchanged.\n");
        error=OMX_ErrorBadParameter;
        goto EXIT;
    }
EXIT:

    if(pConfigNewMirror){
        free(pConfigNewMirror);
        pConfigNewMirror = NULL;
    }
    return error;
}

static OMX_ERRORTYPE VPP_SetRotationAngle(OMX_HANDLETYPE pHandle, int IsRGBOutput,int Angle)
{
    OMX_ERRORTYPE error = OMX_ErrorNone;
    OMX_CONFIG_ROTATIONTYPE *pConfigNewRotation = malloc(sizeof(OMX_CONFIG_ROTATIONTYPE));
    if(pConfigNewRotation == NULL){
        error = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    memset(pConfigNewRotation,0,sizeof(OMX_CONFIG_ROTATIONTYPE));
    if(Angle == 0 || Angle == 90 || Angle ==180 || Angle ==270){
        pConfigNewRotation->nRotation = Angle;
    }
    else{
        printf("VPPTEST:: Not a valid Rotation Angle, Working with Default Rotation Angle\n");
        goto EXIT;
    }
    
    switch(IsRGBOutput){
        case 0:  /*YUV output only*/ 
            pConfigNewRotation->nPortIndex = MyVppPortDef.yuvoutput_port;
            error=OMX_SetConfig(pHandle,OMX_IndexConfigCommonRotate,pConfigNewRotation);
            if(error != OMX_ErrorNone){
                printf("VPPTest Error at %d\n",__LINE__);
                error = OMX_ErrorBadParameter;
                goto EXIT;
            }
            break;          
        case 1: /*RGB output only*/
            pConfigNewRotation->nPortIndex = MyVppPortDef.rgboutput_port;
            error=OMX_SetConfig(pHandle,OMX_IndexConfigCommonRotate,pConfigNewRotation);
            if(error != OMX_ErrorNone){
                printf("VPPTest Error at %d\n",__LINE__);
                error = OMX_ErrorBadParameter;
                goto EXIT;
            }
        case 2: /*Simultaneous outputs*/
            pConfigNewRotation->nPortIndex = MyVppPortDef.rgboutput_port;
            error=OMX_SetConfig(pHandle,OMX_IndexConfigCommonRotate,pConfigNewRotation);
            if(error != OMX_ErrorNone){
                printf("VPPTest Error at %d\n",__LINE__);
                error = OMX_ErrorBadParameter;
                goto EXIT;
            }
            pConfigNewRotation->nPortIndex = MyVppPortDef.yuvoutput_port;
            error=OMX_SetConfig(pHandle,OMX_IndexConfigCommonRotate,pConfigNewRotation);
            if(error != OMX_ErrorNone){
                printf("VPPTest Error at %d\n",__LINE__);
                error = OMX_ErrorBadParameter;
                goto EXIT;
            }
            break;
    default:  
        break;
    }
EXIT:
    if(pConfigNewRotation){
        free(pConfigNewRotation);
        pConfigNewRotation = NULL;
    }
    return error;

}



static OMX_ERRORTYPE VPP_SetDithering(OMX_HANDLETYPE pHandle, int IsRGBOutput)
{
    OMX_ERRORTYPE error = OMX_ErrorNone;
    OMX_CONFIG_DITHERTYPE * pConfigNewDither = NULL;

    if(IsRGBOutput){
        pConfigNewDither = malloc(sizeof(OMX_CONFIG_DITHERTYPE));
        if(pConfigNewDither == NULL){
            error = OMX_ErrorInsufficientResources;
            goto EXIT;
        }
        memset(pConfigNewDither,0,sizeof(OMX_CONFIG_DITHERTYPE));
        pConfigNewDither->nPortIndex = MyVppPortDef.rgboutput_port;
        pConfigNewDither->eDither = OMX_DitherErrorDiffusion;
        error=OMX_SetConfig(pHandle,OMX_IndexConfigCommonDithering,pConfigNewDither);
        if(error != OMX_ErrorNone){
            printf("VPPTest Error at %d\n",__LINE__);
            error = OMX_ErrorBadParameter;
            goto EXIT;
        }
    }
    else{
        printf("VPPTEST:: Need to use RGB as Output, Dithering not possible.\n");
        error=OMX_ErrorBadParameter;
        goto EXIT;
    }

EXIT:

    if(pConfigNewDither){
        free(pConfigNewDither);
        pConfigNewDither = NULL;
    }
    return error;
}

static OMX_ERRORTYPE VPP_SetColorRange(OMX_HANDLETYPE pHandle, int nColorRange)
{
    OMX_ERRORTYPE error = OMX_ErrorNone;
    
    if(nColorRange<0 || nColorRange>3){
        printf("VPPTEST:: Not a valid option in Color Range Conversion\n");
        error = OMX_ErrorBadParameter;
        return error;
    }
    
    error=OMX_SetConfig(pHandle, MyVPPCustomDef.VPPCustomSetColorRange, &nColorRange);
    if(error != OMX_ErrorNone){
        printf("VPPTest Error at %d\n",__LINE__);
        error = OMX_ErrorBadParameter;
        return error;
    }       
    

    return error;
}

OMX_BOOL VPP_Test_Check_Frames(int YUVRGB, int inFrames, int OvlyFrames,int outRGBFrames,int outYUVFrames)
{
    if(YUVRGB==0){
        if(outYUVFrames < (inFrames-1) || outYUVFrames < (OvlyFrames-1))
        {
            return OMX_TRUE;
        }
        else{
            return OMX_FALSE;
            }
    }       
    else if(YUVRGB==1){
        if(outRGBFrames < (inFrames-1) || outRGBFrames < (OvlyFrames-1)){
           return OMX_TRUE;
        }
        else{
             return OMX_FALSE;
             }
      }
      else{
            if( (outRGBFrames < (inFrames-1) || outRGBFrames < (OvlyFrames-1)) ||
                (outYUVFrames < (inFrames-1) || outYUVFrames < (OvlyFrames-1))){
           return OMX_TRUE;
        }  
        else{
            return OMX_FALSE;
        }
    }
}

#ifdef DSP_MMU_FAULT_HANDLING

int LoadBaseImage() {
    unsigned int uProcId = 0;	/* default proc ID is 0. */
    unsigned int index = 0;
    
    struct DSP_PROCESSORINFO dspInfo;
    DSP_HPROCESSOR hProc;
    DSP_STATUS status = DSP_SOK;
    unsigned int numProcs;
    char* argv[2];
   
    argv[0] = "/lib/dsp/baseimage.dof";
    
    status = (DBAPI)DspManager_Open(0, NULL);
    if (DSP_FAILED(status)) {
        printf("DSPManager_Open failed \n");
        return -1;
    } 
    while (DSP_SUCCEEDED(DSPManager_EnumProcessorInfo(index,&dspInfo,
        (unsigned int)sizeof(struct DSP_PROCESSORINFO),&numProcs))) {
        if ((dspInfo.uProcessorType == DSPTYPE_55) || 
            (dspInfo.uProcessorType == DSPTYPE_64)) {
            uProcId = index;
            status = DSP_SOK;
            break;
        }
        index++;
    }
    status = DSPProcessor_Attach(uProcId, NULL, &hProc);
    if (DSP_SUCCEEDED(status)) {
        status = DSPProcessor_Stop(hProc);
        if (DSP_SUCCEEDED(status)) {
            status = DSPProcessor_Load(hProc,1,(const char **)argv,NULL);
            if (DSP_SUCCEEDED(status)) {
                status = DSPProcessor_Start(hProc);
                if (DSP_SUCCEEDED(status)) {
                } 
                else {
                }
            } 
			else {
            }
            DSPProcessor_Detach(hProc);
        }
        else {
        }
    }
    else {
    }
    fprintf(stderr,"Baseimage Loaded\n");

    return 0;		
}
#endif


