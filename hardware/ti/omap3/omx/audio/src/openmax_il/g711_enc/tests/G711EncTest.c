
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
/* =============================================================================
 *             Texas Instruments OMAP (TM) Platform Software
 *  (c) Copyright Texas Instruments, Incorporated.  All Rights Reserved.
 *
 *  Use of this software is controlled by the terms and conditions found
 *  in the license agreement under which this software has been supplied.
 * =========================================================================== */
/**
 * @file G711Enc_Test.c
 *
 * This file implements G711 Encoder Component Test Application to verify
 * which is fully compliant with the Khronos OpenMAX (TM) 1.0 Specification
 *
 * @path  $(CSLPATH)\OMAPSW_MPU\linux\audio\src\openmax_il\g711_enc\tests
 *
 * @rev  1.0
 */
/* ----------------------------------------------------------------------------
 *!
 *! Revision History
 *! ===================================
 *! 12-Dec-2006: Initial Version
 *! This is newest file
 * =========================================================================== */
/* ------compilation control switches -------------------------*/
/****************************************************************
 *  INCLUDE FILES
 ****************************************************************/
/* ----- system and platform files ----------------------------*/

#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <errno.h>
#include <linux/vt.h>
#include <signal.h>
#include <sys/stat.h>
#include <pthread.h>
#include <linux/soundcard.h>

#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

#include <OMX_Index.h>
#include <OMX_Types.h>
#include <OMX_Core.h>
#include <OMX_Audio.h>
#include <TIDspOmx.h>
#include <OMX_Component.h>


#ifdef OMX_GETTIME  
#include <OMX_Common_Utils.h>
#include <OMX_GetTime.h>     /*Headers for Performance & measuremet    */
#endif

/* ======================================================================= */
/**
 * @def G711ENC_INPUT_BUFFER_SIZE        Default input buffer size
 */
/* ======================================================================= */
#define G711ENC_INPUT_BUFFER_SIZE 160
/* ======================================================================= */
/**
 * @def G711ENC_OUTPUT_BUFFER_SIZE   Default output buffer size
 */
/* ======================================================================= */
#define G711ENC_OUTPUT_BUFFER_SIZE 80 /*Component default output buffer size*/

/* ======================================================================= */
/*
 * @def    G711ENC_APP_ID  App ID Value setting
 */
/* ======================================================================= */
#define G711ENC_APP_ID 100

/* ======================================================================= */
/*
 * @def    FIFO Communication with audiomanager
 */
/* ======================================================================= */
#define FIFO1 "/dev/fifo.1"
#define FIFO2 "/dev/fifo.2"

#undef APP_DEBUG

#undef APP_MEMCHECK

#undef USE_BUFFER

/*For timestamp and tickcount*/
#undef APP_TIME_TIC_DEBUG

#ifdef APP_DEBUG
#define APP_DPRINT(...)    fprintf(stderr,__VA_ARGS__)
#else
#define APP_DPRINT(...)
#endif

#ifdef APP_MEMCHECK
#define APP_MEMPRINT(...)    fprintf(stderr,__VA_ARGS__)
#else
#define APP_MEMPRINT(...)
#endif

#ifdef APP_TIME_TIC_DEBUG
#define TIME_PRINT(...)   fprintf(stderr,__VA_ARGS__)
#define TICK_PRINT(...)   fprintf(stderr,__VA_ARGS__)
#else
#define TIME_PRINT(...)
#define TICK_PRINT(...)
#endif

#ifdef OMX_GETTIME
OMX_ERRORTYPE eError = OMX_ErrorNone;
int GT_FlagE = 0;  /* Fill Buffer 1 = First Buffer,  0 = Not First Buffer  */
int GT_FlagF = 0;  /*Empty Buffer  1 = First Buffer,  0 = Not First Buffer  */
static OMX_NODE* pListHead = NULL;
#endif

pthread_mutex_t WaitForState_mutex;
pthread_cond_t  WaitForState_threshold;
OMX_U8          WaitForState_flag = 0;
OMX_U8          TargetedState = 0;

typedef struct AUDIO_INFO {
    OMX_U32 acdnMode;
    OMX_U32 dasfMode;
    OMX_U32 nIpBufs;
    OMX_U32 nIpBufSize;
    OMX_U32 nOpBufs;
    OMX_U32 nOpBufSize;
    OMX_U32 nMFrameMode;
} AUDIO_INFO;

/* ======================================================================= */
/**
 *  M A C R O S FOR MALLOC and MEMORY FREE and CLOSING PIPES
 */
/* ======================================================================= */

#define OMX_G711ENC_CONF_INIT_STRUCT(_s_, _name_)   \
    memset((_s_), 0x0, sizeof(_name_));             \
    (_s_)->nSize = sizeof(_name_);                  \
    (_s_)->nVersion.s.nVersionMajor = 0x1;          \
    (_s_)->nVersion.s.nVersionMinor = 0x0;          \
    (_s_)->nVersion.s.nRevision = 0x0;              \
    (_s_)->nVersion.s.nStep = 0x0

#define OMX_G711ENC_INIT_STRUCT(_s_, _name_)    \
    memset((_s_), 0x0, sizeof(_name_));    \

#define OMX_G711ENC_MALLOC_STRUCT(_pStruct_, _sName_)               \
    _pStruct_ = (_sName_*)malloc(sizeof(_sName_));                  \
    if(_pStruct_ == NULL){                                          \
        printf("***********************************\n");            \
        printf("%d Malloc Failed\n",__LINE__);                      \
        printf("***********************************\n");            \
        eError = OMX_ErrorInsufficientResources;                    \
        goto EXIT;                                                  \
    }                                                               \
    APP_MEMPRINT("%d ALLOCATING MEMORY = %p\n",__LINE__,_pStruct_);

/* ======================================================================= */
/**
 * @def G711ENC_MAX_NUM_OF_BUFS       Maximum number of buffers
 * @def G711ENC_NUM_OF_CHANNELS         Number of Channels
 * @def G711ENC_SAMPLING_FREQUENCY    Sampling frequency
 */
/* ======================================================================= */
#define G711ENC_MAX_NUM_OF_BUFS 10
#define G711ENC_NUM_OF_CHANNELS 1
#define G711ENC_SAMPLING_FREQUENCY 8000

int maxint(int a, int b);

int inputPortDisabled = 0;
int outputPortDisabled = 0;
OMX_BOOL playcompleted = 0;

OMX_STRING strG711Encoder = "OMX.TI.G711.encode";

int IpBuf_Pipe[2] = {0};
int OpBuf_Pipe[2] = {0};
int Event_Pipe[2] = {0};

OMX_STATETYPE gState  = OMX_StateInvalid;

fd_set rfds;
static OMX_BOOL bInvalidState;

/******************************************************************************/
OMX_S16 numInputBuffers = 0;
OMX_S16 numOutputBuffers = 0;
#ifdef USE_BUFFER
OMX_U8* pInputBuffer[10] = {NULL};
OMX_U8* pOutputBuffer[10] = {NULL};
#endif
OMX_BUFFERHEADERTYPE* pInputBufferHeader[10] = {NULL};
OMX_BUFFERHEADERTYPE* pOutputBufferHeader[10] = {NULL};

int timeToExit = 0;
/* RM control */
int preempted = 0;
/******************************************************************************/

OMX_ERRORTYPE send_input_buffer (OMX_HANDLETYPE pHandle, 
                                 OMX_BUFFERHEADERTYPE* pBuffer, 
                                 FILE *fIn);
/* safe routine to get the maximum of 2 integers */
int maxint(int a, int b)
{
    return (a>b) ? a : b;
}

/* This method will wait for the component to get to the state
 * specified by the DesiredState input. */
static OMX_ERRORTYPE WaitForState(OMX_HANDLETYPE pHandle,
                                  OMX_STATETYPE DesiredState)
{
    OMX_STATETYPE CurState = OMX_StateInvalid;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    eError = OMX_GetState(pHandle, &CurState);
    if(eError != OMX_ErrorNone) {
        APP_DPRINT("%d [TEST APP] Error returned from GetState\n",__LINE__);
        return eError;

    }
    if(CurState != DesiredState){
        WaitForState_flag = 1;
        TargetedState = DesiredState;
        pthread_mutex_lock(&WaitForState_mutex); 
        pthread_cond_wait(&WaitForState_threshold, 
                          &WaitForState_mutex);
        pthread_mutex_unlock(&WaitForState_mutex);      
    }

    return eError;
}

OMX_ERRORTYPE EventHandler(OMX_HANDLETYPE hComponent,
                           OMX_PTR pAppData,
                           OMX_EVENTTYPE eEvent,
                           OMX_U32 nData1,
                           OMX_U32 nData2,
                           OMX_PTR pEventData)
{
    OMX_U8 writeValue = 0;  

    APP_DPRINT("%d [TEST APP] Component eEvent = %d\n", __LINE__,eEvent);
    APP_DPRINT("%d [TEST APP] Entering EventHandler \n", __LINE__);
    switch (eEvent) {
   
    case OMX_EventCmdComplete:
        gState = (OMX_STATETYPE)nData2;
        APP_DPRINT( "%d [TEST APP] Component eEvent Completed  = %d\n", __LINE__,eEvent);
        if (nData1 == OMX_CommandPortDisable) {
            if (nData2 == OMX_DirInput) {
                inputPortDisabled = 1;
                APP_DPRINT( "%d [TEST APP] Input Port disabled \n", __LINE__);
            }
            if (nData2 == OMX_DirOutput) {
                outputPortDisabled = 1;
                APP_DPRINT( "%d [TEST APP] output Port disabled \n", __LINE__);
            }
        }
        if ((nData1 == OMX_CommandStateSet) && (TargetedState == nData2) && 
            (WaitForState_flag)){
            WaitForState_flag = 0;
            pthread_mutex_lock(&WaitForState_mutex);
            pthread_cond_signal(&WaitForState_threshold);
            pthread_mutex_unlock(&WaitForState_mutex);
        }
        APP_DPRINT( "%d [TEST APP] Exit OMX_EventCmdComplete = %ld\n", __LINE__,nData2);
        break;
       
    case OMX_EventError:
        printf( "%d [TEST APP] Enter to OMX_EventError = %d\n", __LINE__,eEvent);
          
        if (nData1 == OMX_ErrorInvalidState) {
            bInvalidState = OMX_TRUE;
        }
        else if(nData1 == OMX_ErrorResourcesPreempted) {
            writeValue = 0;  
            preempted = 1;
            write(Event_Pipe[1], &writeValue, sizeof(OMX_U8));
        }
        else if (nData1 == OMX_ErrorResourcesLost) { 
            WaitForState_flag = 0;
            pthread_mutex_lock(&WaitForState_mutex);
            pthread_cond_signal(&WaitForState_threshold);
            pthread_mutex_unlock(&WaitForState_mutex);
        }
        else if(nData1 == OMX_ErrorResourcesPreempted) {
            writeValue = 0;  
            preempted = 1;
            write(Event_Pipe[1], &writeValue, sizeof(OMX_U8));
        }
        APP_DPRINT( "%d [TEST APP] Component OMX_EventError = %d\n", __LINE__,eEvent);
        break;
       
    case OMX_EventMax:
        APP_DPRINT( "%d [TEST APP] Component OMX_EventMax = %d\n", __LINE__,eEvent);
        break;
       
    case OMX_EventMark:
        APP_DPRINT( "%d [TEST APP] Component OMX_EventMark = %d\n", __LINE__,eEvent);
        break;
       
    case OMX_EventPortSettingsChanged:
        APP_DPRINT( "%d [TEST APP] Component OMX_EventPortSettingsChanged = %d\n", __LINE__,eEvent);
        break;
        
    case OMX_EventBufferFlag:
        if(nData2 == 0)
            printf("EOS received from INPUT %ld %ld\n",nData1,nData2);
        playcompleted = 1;
        writeValue = 2;  
        write(Event_Pipe[1], &writeValue, sizeof(OMX_U8));
        APP_DPRINT( "%d [TEST APP] Component OMX_EventBufferFlag = %d\n", __LINE__,eEvent);
        break;
       
    case OMX_EventResourcesAcquired:
        APP_DPRINT( "%d [TEST APP] Component OMX_EventResourcesAcquired = %d\n", __LINE__,eEvent);
        writeValue = 1;
        preempted = 0;
        write(Event_Pipe[1], &writeValue, sizeof(OMX_U8));
        APP_DPRINT( "%d [TEST APP] Component OMX_EventResourcesAquired = %d\n", __LINE__,eEvent);
        break;
      
    default:
        break;

    }
    return OMX_ErrorNone;
}

void FillBufferDone (OMX_HANDLETYPE hComponent, OMX_PTR ptr, OMX_BUFFERHEADERTYPE* pBuffer)
{
    /*add on: TimeStamp & TickCount EmptyBufferDone*/
    TIME_PRINT("TimeStamp Output: %lld\n",pBuffer->nTimeStamp);
    TICK_PRINT("TickCount Output: %ld\n\n",pBuffer->nTickCount);
    write(OpBuf_Pipe[1], &pBuffer, sizeof(pBuffer));
#ifdef OMX_GETTIME
    if (GT_FlagF == 1 ){ /* First Buffer Reply*/  /* 1 = First Buffer,  0 = Not First Buffer  */
        GT_END("Call to FillBufferDone  <First: FillBufferDone>");
        GT_FlagF = 0 ;   /* 1 = First Buffer,  0 = Not First Buffer  */
    }
#endif
}

void EmptyBufferDone(OMX_HANDLETYPE hComponent, OMX_PTR ptr, OMX_BUFFERHEADERTYPE* pBuffer)
{ 

    if (!preempted) {
        write(IpBuf_Pipe[1], &pBuffer, sizeof(pBuffer));
    }
#ifdef OMX_GETTIME
    if (GT_FlagE == 1 ){ /* First Buffer Reply*/  /* 1 = First Buffer,  0 = Not First Buffer  */
        GT_END("Call to EmptyBufferDone <First: EmptyBufferDone>");
        GT_FlagE = 0;   /* 1 = First Buffer,  0 = Not First Buffer  */
    }
#endif
}
typedef struct G711ENC_FTYPES{
    OMX_S16   FrameSizeType;
    OMX_S16   VAUMode;
    OMX_S16   VAUThresOffset;
    OMX_S16   VAUNum;
    OMX_S16   NMUNoise;
    OMX_S16   LPOrder;
}G711ENC_FTYPES;
/* ----------------------------------------------------------------------------
 * main() 
 *
 * This function is called at application startup 
 * and drives the G711 Encoder OMX component
 * ---------------------------------------------------------------------------- */
int main(int argc, char* argv[])
{
    OMX_CALLBACKTYPE G711CaBa = {(void *)EventHandler,
                                 (void*)EmptyBufferDone, 
                                 (void*)FillBufferDone};
    OMX_HANDLETYPE pHandle = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 AppData = G711ENC_APP_ID;
    OMX_PARAM_PORTDEFINITIONTYPE* pCompPrivateStruct = NULL;
    OMX_AUDIO_PARAM_PCMMODETYPE *pG711Param = NULL;
    OMX_BUFFERHEADERTYPE* pInputBufferHeader[G711ENC_MAX_NUM_OF_BUFS] = {NULL};
    OMX_BUFFERHEADERTYPE* pOutputBufferHeader[G711ENC_MAX_NUM_OF_BUFS] = {NULL};

    G711ENC_FTYPES *g711eframeinfo =  malloc(sizeof(G711ENC_FTYPES));

#ifdef USE_BUFFER
    OMX_U8* pInputBuffer[G711ENC_MAX_NUM_OF_BUFS] = {NULL};
    OMX_U8* pOutputBuffer[G711ENC_MAX_NUM_OF_BUFS] = {NULL};
#endif
    AUDIO_INFO* audioinfo = NULL;
    TI_OMX_DSP_DEFINITION tiOmxDspDefinition;
    FILE* fIn = NULL;
    FILE* fOut = NULL;
    struct timeval tv;
    int retval= 0, i= 0, j= 0, k= 0, kk= 0, tcID = 0;
    int frmCount = 0;
    int frmCnt = 1;
    int testcnt = 1;
    int testcnt1 = 1;
    int fdmax = 0;
    int nFrameCount = 1;
    int nFrameLen = 0;
    int nIpBuff = 1;
    int nOutBuff = 1;
    OMX_INDEXTYPE index = 0;
    int status = 0;
    TI_OMX_DATAPATH dataPath;

    int G711E_fdwrite = 0;
    int G711E_fdread = 0;

    printf("------------------------------------------------------\n");
    printf("This is Main Thread In G711 ENCODER Test Application:\n");
    printf("Test Core 1.5 - " __DATE__ ":" __TIME__ "\n");
    printf("------------------------------------------------------\n");
#ifdef OMX_GETTIME
    printf("Line %d\n",__LINE__);
    GTeError = OMX_ListCreate(&pListHead);
    printf("Line %d\n",__LINE__);
    printf("eError = %d\n",GTeError);
    GT_START();
    printf("Line %d\n",__LINE__);
#endif
    /* check the input parameters */
    if(argc != 18) {
        printf("[TestApp] [Input File] [Output File] [FUNC_ID_X] [FM/DM] [ACDNO\
N/ACDNOFF] [NB BUFFERS (DASF mode)] [NB INPUT BUF] [INPUT BUF SIZE] [NB OUTPUT \
BUF] [OUTPUT BUF SIZE] [ALaw/MULaw] [Frame Size Type] [VAU Mode] [VAU OFFSET] [\
VAU NUM] [NMU Mode] [LP Order]\n");
        goto EXIT;
    }
  
    pthread_mutex_init(&WaitForState_mutex, NULL);
    pthread_cond_init (&WaitForState_threshold, NULL);

    /* check to see that the input file exists */
    struct stat sb = {0};
    status = stat(argv[1], &sb);
    if( status != 0 ) {
        printf("Cannot find file %s. (%u)\n", argv[1], errno);
        goto EXIT;
    }

    if(!strcmp(argv[3],"FUNC_ID_1")) {
        printf("### Testing TESTCASE 1 PLAY TILL END ###\n");
        tcID = 1;
    } else if(!strcmp(argv[3],"FUNC_ID_2")) {
        printf("### Testing TESTCASE 2 STOP IN THE END ###\n");
        tcID = 2;
    } else if(!strcmp(argv[3],"FUNC_ID_3")) {
        printf("### Testing TESTCASE 3 PAUSE - RESUME IN BETWEEN ###\n");
        tcID = 3;
    } else if(!strcmp(argv[3],"FUNC_ID_4")) {
        printf("### Testing TESTCASE 4 STOP IN BETWEEN ###\n");
        testcnt = 2;
        tcID = 4;
    }
    if(!strcmp(argv[3],"FUNC_ID_5")){
        printf("### Testing TESTCASE 5 ENCODE without Deleting component Here ###\n");
        testcnt = 20;
        tcID = 5;
    }
    if(!strcmp(argv[3],"FUNC_ID_6")) {
        printf("### Testing TESTCASE 6 ENCODE with Deleting component Here ###\n");
        testcnt1 = 20;
        tcID = 6;
    }

    /*----------------------------------------------
      Main Loop for Deleting component test
      ----------------------------------------------*/
    for(j = 0; j < testcnt1; j++) {
        if(tcID == 6)
            printf ("Encoding the file for %d Time in TESTCASE 6\n",j);

        /* Open communication with Audio manger */
#ifdef DSP_RENDERING_ON          
        if((G711E_fdwrite=open(FIFO1,O_WRONLY))<0) {
            printf("APP: - failure to open WRITE pipe\n");
        }
        else {
            APP_DPRINT("APP: - opened WRITE pipe\n");
        }

        if((G711E_fdread=open(FIFO2,O_RDONLY))<0) {
            printf("APP: - failure to open READ pipe\n");
            goto EXIT;
        }
        else {
            APP_DPRINT("APP: - opened READ pipe\n");
        }
#endif
        /* Create a pipe used to queue data from the callback. */
        retval = pipe(IpBuf_Pipe);
        if( retval != 0) {
            APP_DPRINT("Error:Fill Data Pipe failed to open\n");
            goto EXIT;
        }

        retval = pipe(OpBuf_Pipe);
        if( retval != 0) {
            APP_DPRINT("Error:Empty Data Pipe failed to open\n");
            goto EXIT;
        }
        retval = pipe(Event_Pipe);
        if( retval != 0) {
            APP_DPRINT( "%d %s Error: Empty Data Pipe failed to open\n",__LINE__, __FUNCTION__);
            goto EXIT;
        }
        /* save off the "max" of the handles for the selct statement */
        fdmax = maxint(IpBuf_Pipe[0], OpBuf_Pipe[0]);
        fdmax = maxint(fdmax,Event_Pipe[0]);

        eError = TIOMX_Init();
        if(eError != OMX_ErrorNone) {
            APP_DPRINT("%d Error returned by OMX_Init()\n",__LINE__);
            goto EXIT;
        }

        /* Load the G711 Encoder Component */
#ifdef OMX_GETTIME
        GT_START();
        eError = OMX_GetHandle(&pHandle, strG711Encoder, &AppData, &G711CaBa);
        GT_END("Call to GetHandle");
#else 
        eError = TIOMX_GetHandle(&pHandle, strG711Encoder, &AppData, &G711CaBa);
#endif
    
        if((eError != OMX_ErrorNone) || (pHandle == NULL)) {
            APP_DPRINT("Error in Get Handle function\n");
            goto EXIT;
        }
        APP_DPRINT("%d [TEST APP] Got Phandle =  %p \n",__LINE__,pHandle);
    
        OMX_G711ENC_MALLOC_STRUCT(audioinfo, AUDIO_INFO);
        OMX_G711ENC_INIT_STRUCT(audioinfo, AUDIO_INFO);
    
        /* Setting Input and Output Buffers features for the Component */
        audioinfo->nIpBufs = atoi(argv[7]);
        if(audioinfo->nIpBufs > 4 && audioinfo->nIpBufs < 1){
            APP_DPRINT( "Cannot support %li Input buffers\n", audioinfo->nIpBufs);
            goto EXIT;          
        }        
        APP_DPRINT("%d [TEST APP] number of input buffers = %ld \n",__LINE__,audioinfo->nIpBufs);
        audioinfo->nIpBufSize = atoi(argv[8]);
        APP_DPRINT("%d [TEST APP] input buffer size = %ld \n",__LINE__,audioinfo->nIpBufSize);
        audioinfo->nOpBufs = atoi(argv[9]);
        if(audioinfo->nOpBufs > 4){
            APP_DPRINT( "Cannot support %ld Output buffers\n", audioinfo->nOpBufs);
            goto EXIT;          
        }        
        APP_DPRINT("%d [TEST APP] number of output buffers = %ld \n",__LINE__,audioinfo->nOpBufs);
        audioinfo->nOpBufSize = atoi(argv[10]);
        APP_DPRINT("%d [TEST APP] output buffer size = %ld \n",__LINE__,audioinfo->nOpBufSize);


        OMX_G711ENC_MALLOC_STRUCT(pCompPrivateStruct, OMX_PARAM_PORTDEFINITIONTYPE);
        OMX_G711ENC_CONF_INIT_STRUCT(pCompPrivateStruct, OMX_PARAM_PORTDEFINITIONTYPE);

        pCompPrivateStruct->nPortIndex = OMX_DirInput;
        eError = OMX_GetParameter (pHandle, 
                                   OMX_IndexParamPortDefinition, 
                                   pCompPrivateStruct);
        if (eError != OMX_ErrorNone) {
            eError = OMX_ErrorBadParameter;
            APP_DPRINT("%d OMX_ErrorBadParameter\n",__LINE__);
            goto EXIT;
        }   
        APP_DPRINT("%d Setting input port config\n",__LINE__);
        pCompPrivateStruct->nBufferCountActual                 = audioinfo->nIpBufs;
        pCompPrivateStruct->nBufferCountMin                    = audioinfo->nIpBufs;
        pCompPrivateStruct->nBufferSize                        = audioinfo->nIpBufSize;


#ifdef OMX_GETTIME
        GT_START();
        eError = OMX_SetParameter (pHandle, OMX_IndexParamPortDefinition, pCompPrivateStruct);
        GT_END("Set Parameter Test-SetParameter");
#else 
        eError = OMX_SetParameter (pHandle, OMX_IndexParamPortDefinition, pCompPrivateStruct);
#endif
    
        if (eError != OMX_ErrorNone) {
            eError = OMX_ErrorBadParameter;
            APP_DPRINT("%d OMX_ErrorBadParameter\n",__LINE__);
            goto EXIT;
        }   

        if(!(strcmp(argv[4],"FM"))) {
            audioinfo->dasfMode = 0;
            tiOmxDspDefinition.dasfMode = OMX_FALSE;
            APP_DPRINT("%d G711 ENCODER RUNNING UNDER FILE MODE \n",__LINE__);

        } else if(!(strcmp(argv[4],"DM"))){
            audioinfo->dasfMode = 1;
            tiOmxDspDefinition.dasfMode = OMX_TRUE;
            APP_DPRINT("%d G711 ENCODER RUNNING UNDER DASF MODE \n",__LINE__);

        } else {
            eError = OMX_ErrorBadParameter;
            printf("\n%d [TEST APP] audioinfo->dasfMode Sending Bad Parameter\n",__LINE__);
            printf("%d [TEST APP] Should Be One of these Modes FM, DM\n",__LINE__);
            goto EXIT;
        }

        if(audioinfo->dasfMode == 0) {
            if((atoi(argv[6])) != 0) {
                eError = OMX_ErrorBadParameter;
                printf("\n%d [TEST APP] No. of Buffers Sending Bad Parameter\n",__LINE__);
                printf("%d [TEST APP] For FILE mode argv[6] Should Be --> 0\n",__LINE__);
                printf("%d [TEST APP] For DASF mode argv[6] Should be greater than zero depends on number of buffers user want to encode\n",__LINE__);
                goto EXIT;
            }
        } else {
            if((atoi(argv[6])) == 0) {
                eError = OMX_ErrorBadParameter;
                printf("\n%d [TEST APP] No. of Buffers Sending Bad Parameter\n",__LINE__);
                printf("%d [TEST APP] For DASF mode argv[6] Should be greater than zero depends on number of buffers user want to encode\n",__LINE__);
                printf("%d [TEST APP] For FILE mode argv[6] Should Be --> 0\n",__LINE__);
                goto EXIT;
            }
        }

        if(!(strcmp(argv[5],"ACDNOFF"))) {
            audioinfo->acdnMode = 0;
            tiOmxDspDefinition.acousticMode = OMX_FALSE;
            APP_DPRINT("\n%d [TEST APP] audioinfo->acdnMode = %ld \n",__LINE__,audioinfo->acdnMode);
        } else if(!(strcmp(argv[5],"ACDNON"))) {
            audioinfo->acdnMode = 1;
            tiOmxDspDefinition.acousticMode = OMX_TRUE;
            APP_DPRINT("\n%d [TEST APP] audioinfo->acdnMode = %ld \n",__LINE__,audioinfo->acdnMode);
        } else {
            eError = OMX_ErrorBadParameter;
            printf("\n%d [TEST APP] audioinfo->acdnMode Sending Bad Parameter\n",__LINE__);
            printf("%d [TEST APP] Should Be One of these Modes ACDNON, ACDNOFF\n",__LINE__);
            goto EXIT;
        }


        pCompPrivateStruct->nPortIndex = OMX_DirOutput;
        eError = OMX_GetParameter (pHandle, 
                                   OMX_IndexParamPortDefinition, 
                                   pCompPrivateStruct);
        if (eError != OMX_ErrorNone) {
            eError = OMX_ErrorBadParameter;
            APP_DPRINT("%d OMX_ErrorBadParameter\n",__LINE__);
            goto EXIT;
        }

        APP_MEMPRINT("%d Setting output port config\n",__LINE__);
        pCompPrivateStruct->nBufferCountActual                 = audioinfo->nOpBufs;
        pCompPrivateStruct->nBufferCountMin                    = audioinfo->nOpBufs;
        pCompPrivateStruct->nBufferSize                        = audioinfo->nOpBufSize;

#ifdef OMX_GETTIME
        GT_START();
        eError = OMX_SetParameter (pHandle, OMX_IndexParamPortDefinition, pCompPrivateStruct);
        GT_END("Set Parameter Test-SetParameter");
#else 
        eError = OMX_SetParameter (pHandle, OMX_IndexParamPortDefinition, pCompPrivateStruct);
#endif
        if (eError != OMX_ErrorNone) {
            eError = OMX_ErrorBadParameter;
            APP_DPRINT("%d OMX_ErrorBadParameter\n",__LINE__);
            goto EXIT;
        }

        OMX_G711ENC_MALLOC_STRUCT(pG711Param, OMX_AUDIO_PARAM_PCMMODETYPE);
        OMX_G711ENC_CONF_INIT_STRUCT(pG711Param, OMX_AUDIO_PARAM_PCMMODETYPE);

        /* Send  G711 config for output */
        pG711Param->nPortIndex = OMX_DirOutput;
        eError = OMX_GetParameter (pHandle, OMX_IndexParamAudioPcm, pG711Param);
        if (eError != OMX_ErrorNone) {
            APP_DPRINT("%d Get paramter reported %d\n",__LINE__,eError);
            goto EXIT;
        }

        pG711Param->nChannels = G711ENC_NUM_OF_CHANNELS;

        /* extract compression format from command line */
        if (!(strcmp(argv[11],"ALaw")))
            pG711Param->ePCMMode             = OMX_AUDIO_PCMModeALaw;
        else if (!(strcmp(argv[11],"MULaw")))
            pG711Param->ePCMMode             = OMX_AUDIO_PCMModeMULaw;
        else {
            printf("\n%d [TEST APP] Bad Parameter: \n",__LINE__);
            printf("%d [TEST APP] Please enter proper G711 mode: ALaw or MULaw\n",__LINE__);
            goto EXIT;
        }
#ifdef OMX_GETTIME
        GT_START();
        eError = OMX_SetParameter (pHandle, OMX_IndexParamAudioPcm, pG711Param);
        GT_END("Set Parameter Test-SetParameter");
#else 
        eError = OMX_SetParameter (pHandle, OMX_IndexParamAudioPcm, pG711Param);
#endif
        if (eError != OMX_ErrorNone) {
            eError = OMX_ErrorBadParameter;
            APP_DPRINT("%d OMX_ErrorBadParameter\n",__LINE__);
            goto EXIT;
        }

        /* Send  G711 config for input */
        pG711Param->nPortIndex = OMX_DirInput;
        eError = OMX_GetParameter (pHandle,OMX_IndexParamAudioPcm, pG711Param);
        if (eError != OMX_ErrorNone) {
            eError = OMX_ErrorBadParameter;
            printf ("%d [TEST APP] OMX_ErrorBadParameter\n",__LINE__);
            goto EXIT;
        }

#ifdef OMX_GETTIME
        GT_START();
        eError = OMX_SetParameter (pHandle,OMX_IndexParamAudioPcm, pG711Param);
        GT_END("Set Parameter Test-SetParameter");
#else 
        eError = OMX_SetParameter (pHandle,OMX_IndexParamAudioPcm, pG711Param);
#endif
        if (eError != OMX_ErrorNone) {
            eError = OMX_ErrorBadParameter;
            printf ("%d [TEST APP] OMX_ErrorBadParameter\n",__LINE__);
            goto EXIT;
        }

        /** Getting the frame params */
        g711eframeinfo->FrameSizeType = atoi(argv[12]);
        g711eframeinfo->VAUMode = atoi(argv[13]);
        g711eframeinfo->VAUThresOffset = atoi(argv[14]);
        g711eframeinfo->VAUNum = atoi(argv[15]);
        g711eframeinfo->NMUNoise = atoi(argv[16]);
        g711eframeinfo->LPOrder = atoi(argv[17]);
  
        eError = OMX_GetExtensionIndex(pHandle, "OMX.TI.index.config.g711.frameparamters",&index);
        if (eError != OMX_ErrorNone) {
            printf("Error getting extension index\n");
            goto EXIT;
        }

        eError = OMX_SetConfig (pHandle, index, g711eframeinfo);
        if(eError != OMX_ErrorNone) {
            eError = OMX_ErrorBadParameter;
            APP_DPRINT("%d Error from OMX_SetConfig() function\n",__LINE__);
            goto EXIT;
        }


        eError = OMX_GetExtensionIndex(pHandle, "OMX.TI.index.config.tispecific",&index);
        if (eError != OMX_ErrorNone) {
            APP_DPRINT("Error returned from OMX_GetExtensionIndex\n");
            goto EXIT;
        }

#ifdef DSP_RENDERING_ON
        if((write(G711E_fdwrite, &cmd_data, sizeof(cmd_data)))<0) {
            printf("%d [TEST APP] - failure to send command to audio manager\n", __LINE__);
            goto EXIT;
        }
        if((read(G711E_fdread, &cmd_data, sizeof(cmd_data)))<0) {
            printf("%d [TEST APP] - failure to get data from the audio manager\n", __LINE__);
            goto EXIT;
        }
#endif

        eError = OMX_SetConfig (pHandle, index, &tiOmxDspDefinition);
        if(eError != OMX_ErrorNone) {
            eError = OMX_ErrorBadParameter;
            APP_DPRINT("%d Error from OMX_SetConfig() function\n",__LINE__);
            goto EXIT;
        }

        /* Data path  for DASF */
        if (audioinfo->dasfMode) {
            printf("***************StreamId=%d******************\n", (int)tiOmxDspDefinition.streamId);
#ifdef RTM_PATH    
            dataPath = DATAPATH_APPLICATION_RTMIXER;
#endif

#ifdef ETEEDN_PATH
            dataPath = DATAPATH_APPLICATION;
#endif        
        }

        eError = OMX_GetExtensionIndex(pHandle, "OMX.TI.index.config.g711.datapath",&index);
        if (eError != OMX_ErrorNone) {
            printf("Error getting extension index\n");
            goto EXIT;
        }

        eError = OMX_SetConfig (pHandle, index, &dataPath);
        if(eError != OMX_ErrorNone) {
            eError = OMX_ErrorBadParameter;
            APP_DPRINT("%d G711EencTest.c :: Error from OMX_SetConfig() function\n",__LINE__);
            goto EXIT;
        }

#ifdef OMX_GETTIME
        GT_START();
#endif
        eError = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
        if(eError != OMX_ErrorNone) {
            APP_DPRINT("Error from SendCommand-Idle(Init) State function\n");
            goto EXIT;
        }
#ifndef USE_BUFFER
        if(!audioinfo->dasfMode){
            for(i = 0; i < audioinfo->nIpBufs; i++) {
                /* allocate input buffer */
                APP_DPRINT("%d About to call OMX_AllocateBuffer for pInputBufferHeader[%d]\n\n",__LINE__, i);
                eError = OMX_AllocateBuffer(pHandle, &pInputBufferHeader[i], 0, NULL, audioinfo->nIpBufSize);
                if(eError != OMX_ErrorNone) {
                    APP_DPRINT("%d Error returned by OMX_AllocateBuffer for pInputBufferHeader[%d]\n",__LINE__, i);
                    goto EXIT;
                }
            }
        }

        for(i = 0; i < audioinfo->nOpBufs; i++) {
            /* allocate output buffer */
            APP_DPRINT("%d About to call OMX_AllocateBuffer for pOutputBufferHeader[%d]\n\n",__LINE__, i);
            eError = OMX_AllocateBuffer(pHandle, &pOutputBufferHeader[i], 1, NULL, pCompPrivateStruct->nBufferSize);
            if(eError != OMX_ErrorNone) {
                APP_DPRINT("%d Error returned by OMX_AllocateBuffer for pOutputBufferHeader[%d]\n",__LINE__, i);
                goto EXIT;
            }
        }
#else
        if(!audioinfo->dasfMode){
            for(i = 0; i < audioinfo->nIpBufs; i++) {
                pInputBuffer[i] = (OMX_U8*)malloc(audioinfo->nIpBufSize);
                APP_MEMPRINT("%d [TESTAPP ALLOC] pInputBuffer[%d] = %p\n",__LINE__,i,pInputBuffer[i]);
                if(NULL == pInputBuffer[i]) {
                    APP_DPRINT("%d Malloc Failed\n",__LINE__);
                    eError = OMX_ErrorInsufficientResources;
                    goto EXIT;
                }
                /*  allocate input buffer */
                APP_DPRINT("%d About to call OMX_UseBuffer\n",__LINE__);
                eError = OMX_UseBuffer(pHandle, &pInputBufferHeader[i], 0, NULL, audioinfo->nIpBufSize, pInputBuffer[i]);
                if(eError != OMX_ErrorNone) {
                    APP_DPRINT("%d Error returned by OMX_UseBuffer()\n",__LINE__);
                    goto EXIT;
                }
            }
        }
        for(i = 0; i < audioinfo->nOpBufs; i++) {
            pOutputBuffer[i] = malloc (pCompPrivateStruct->nBufferSize + 256);
            APP_MEMPRINT("%d [TESTAPP ALLOC] pOutputBuffer[%d] = %p\n",__LINE__,i,pOutputBuffer[i]);
            if(NULL == pOutputBuffer[i]) {
                APP_DPRINT("%d Malloc Failed\n",__LINE__);
                eError = OMX_ErrorInsufficientResources;
                goto EXIT;
            }
            pOutputBuffer[i] = pOutputBuffer[i] + 128;
    
            /* allocate output buffer */
            APP_DPRINT("%d About to call OMX_UseBuffer\n",__LINE__);
            eError = OMX_UseBuffer(pHandle, &pOutputBufferHeader[i], 1, NULL, pCompPrivateStruct->nBufferSize, pOutputBuffer[i]);
            if(eError != OMX_ErrorNone) {
                APP_DPRINT("%d Error returned by OMX_UseBuffer()\n",__LINE__);
                goto EXIT;
            }
        }
#endif
        /* Wait for startup to complete */
        eError = WaitForState(pHandle, OMX_StateIdle);
#ifdef OMX_GETTIME
        GT_END("Call to SendCommand <OMX_StateIdle>");
#endif    
        if(eError != OMX_ErrorNone) {
            APP_DPRINT( "Error:  G711Encoder->WaitForState reports an eError %X\n", eError);
            goto EXIT;
        }


        /*----------------------------------------------
          Main Loop for Non Deleting component test
          ----------------------------------------------*/
        for(i = 0; i < testcnt; i++) {
            if(tcID == 5)
                printf ("Encoding the file for %d Time in TESTCASE 5\n",i);

            if(i > 0){
                /* Create a pipe used to queue data from the callback. */
                retval = pipe(IpBuf_Pipe);
                if( retval != 0) {
                    APP_DPRINT( "%d APP: Error: Fill Data Pipe failed to open\n",__LINE__);
                    goto EXIT;
                }
                retval = pipe(OpBuf_Pipe);
                if( retval != 0) {
                    APP_DPRINT( "%d APP: Error: Empty Data Pipe failed to open\n",__LINE__);
                    goto EXIT;
                }
                retval = pipe(Event_Pipe);
                if( retval != 0) {
                    APP_DPRINT( "%d APP: Error: Event Pipe failed to open\n",__LINE__);
                    goto EXIT;
                }
            }

            frmCnt = 1;
            nFrameCount = 1;
            nOutBuff = 1;
            nIpBuff  = 1;

            fIn = fopen(argv[1], "r");
            if( fIn == NULL ) {
                fprintf(stderr, "Error:  failed to open the file %s for read only access\n",argv[1]);
                goto EXIT;
            }
            fOut = fopen(argv[2], "w");
            if( fOut == NULL ) {
                printf("Error:  failed to open the output file %s\n", argv[2]);
                goto EXIT;
            }
            APP_DPRINT("%d [TEST APP] Sending OMX_StateExecuting Command\n",__LINE__);
#ifdef OMX_GETTIME
            GT_START();
#endif
            eError = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
            if(eError != OMX_ErrorNone) {
                printf("Error from SendCommand-Executing State function\n");
                goto EXIT;
            }
            eError = WaitForState(pHandle, OMX_StateExecuting);
#ifdef OMX_GETTIME
            GT_END("Call to SendCommand <OMX_StateExecuting>");
#endif      
            if(eError != OMX_ErrorNone) {
                APP_DPRINT( "Error:  G711Encoder->WaitForState reports an eError %X\n", eError);
                goto EXIT;
            }

            if(audioinfo->dasfMode == 1) {
                printf("%d [TEST APP] No.of Buffers Encoding = %d\n",__LINE__, atoi(argv[6]));
            }

            if(audioinfo->dasfMode == 0) {
                for (k=0; k < audioinfo->nIpBufs; k++) {
#ifdef OMX_GETTIME
                    if (k==0){ 
                        GT_FlagE=1;  /* 1 = First Buffer,  0 = Not First Buffer  */
                        GT_START(); /* Empty Bufffer */
                    }
#endif  
                    send_input_buffer(pHandle,pInputBufferHeader[k],fIn);
                    nIpBuff++;
                }
            }

            for (kk = 0; kk < audioinfo->nOpBufs; kk++) {
                APP_DPRINT("%d [TEST APP] Calling OMX_FillThisBuffer \n",__LINE__);
#ifdef OMX_GETTIME
                if (kk==0){ 
                    GT_FlagF=1;  /* 1 = First Buffer,  0 = Not First Buffer  */
                    GT_START(); /* Fill Buffer */
                }
#endif          
                OMX_FillThisBuffer(pHandle, pOutputBufferHeader[kk]);
            }

            while((eError == OMX_ErrorNone) && (gState != OMX_StateIdle) && 
                  (gState != OMX_StateInvalid)) {

                FD_ZERO(&rfds);
                FD_SET(IpBuf_Pipe[0], &rfds);
                FD_SET(OpBuf_Pipe[0], &rfds);
                FD_SET(Event_Pipe[0], &rfds);

                tv.tv_sec = 1;
                tv.tv_usec = 0;
                frmCount++;

                retval = select(fdmax+1, &rfds, NULL, NULL, &tv);
                if(retval == -1) {
                    perror("select()");
                    APP_DPRINT( " :: Error \n");
                    break;
                }

                if(retval == 0) {
                    APP_DPRINT("%d BasicFn App Timeout !!!!!!!!!!! \n",__LINE__);
                }

                switch (tcID) {
                case 1:
                case 2:
                case 5:
                case 6:
                    if(audioinfo->dasfMode == 0) {
                        if(FD_ISSET(IpBuf_Pipe[0], &rfds)) {
                            OMX_BUFFERHEADERTYPE* pBuffer;
                            read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
                            pBuffer->nFlags = 0;
                            send_input_buffer(pHandle,pBuffer,fIn);
                            nIpBuff++;
                        }
                        if(tcID == 2 && nIpBuff == 200) {
                            printf("%d [TEST APP] Sending Stop.........From APP\n",__LINE__);
                            printf("%d [TEST APP] Shutting down ---------- \n",__LINE__);
#ifdef OMX_GETTIME
                            GT_START();
#endif
                            eError = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateIdle, NULL);
                            if(eError != OMX_ErrorNone) {
                                fprintf (stderr,"Error from SendCommand-Idle(Stop) State function\n");
                                goto EXIT;
                            }
                            eError = WaitForState(pHandle, OMX_StateIdle);
#ifdef OMX_GETTIME
                            GT_END("Call to SendCommand <OMX_StateIdle>");
#endif
                            if (eError != OMX_ErrorNone ){
                                printf("Error:WaitForState has timed out %d", eError);
                                goto EXIT;
                            }
                        }
                    } else {
                        APP_DPRINT("%d G711 ENCODER RUNNING UNDER DASF MODE \n",__LINE__);
                        if(nFrameCount == atoi(argv[6])) {
                            APP_DPRINT("%d [TEST APP] Sending Stop.........From APP \n",__LINE__);
                            APP_DPRINT("%d [TEST APP] Shutting down ---------- \n",__LINE__);
#ifdef OMX_GETTIME
                            GT_START();
#endif
                            eError = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateIdle, NULL);
                            if(eError != OMX_ErrorNone) {
                                fprintf (stderr,"Error from SendCommand-Idle(Stop) State function\n");
                                goto EXIT;
                            }
                            eError = WaitForState(pHandle, OMX_StateIdle);
#ifdef OMX_GETTIME
                            GT_END("Call to SendCommand <OMX_StateIdle>");
#endif
                            if ( eError != OMX_ErrorNone ){
                                printf("Error:WaitForState has timed out %d", eError);
                                goto EXIT;
                            }
                        }
                        APP_DPRINT("%d G711 ENCODER READING DATA FROM DASF  \n",__LINE__);
                    }
                    break;

                case 3:
                    if(audioinfo->dasfMode == 0) {
                        APP_DPRINT("%d G711 ENCODER RUNNING UNDER FILE 2 FILE MODE \n",__LINE__);
                        if(FD_ISSET(IpBuf_Pipe[0], &rfds)) {
                            OMX_BUFFERHEADERTYPE* pBuffer;
                            read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
                            pBuffer->nFlags = 0;
                            send_input_buffer(pHandle,pBuffer,fIn);
                        }
                    } else {
                        APP_DPRINT("%d G711 ENCODER RUNNING UNDER DASF MODE \n",__LINE__);
                        if(nFrameCount == atoi(argv[6])) {
                            printf("%d [TEST APP] Sending Stop.........From APP \n",__LINE__);
                            printf("%d [TEST APP] Shutting down ---------- \n",__LINE__);
#ifdef OMX_GETTIME
                            GT_START();
#endif
                            eError = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateIdle, NULL);
                            if(eError != OMX_ErrorNone) {
                                fprintf (stderr,"Error from SendCommand-Idle(Stop) State function\n");
                                goto EXIT;
                            }
                            eError = WaitForState(pHandle, OMX_StateIdle);
#ifdef OMX_GETTIME
                            GT_END("Call to SendCommand <OMX_StateIdle>");
#endif
                            if (eError != OMX_ErrorNone ){
                                printf("Error:WaitForState has timed out %d", eError);
                                goto EXIT;
                            }
                        }
                        APP_DPRINT("%d G711 ENCODER READING DATA FROM DASF  \n",__LINE__);
                    }
                    if (frmCount == 15) {
                        printf ("%d [TEST APP] $$$$$ Sending Resume command to Codec $$$$$$$\n",__LINE__);
#ifdef OMX_GETTIME
                        GT_START();
#endif
                        eError = OMX_SendCommand(pHandle, OMX_CommandStateSet,OMX_StateExecuting, NULL);
                        if(eError != OMX_ErrorNone) {
                            fprintf (stderr,"Error from SendCommand-Executing State function\n");
                            goto EXIT;
                        }
                        /* Wait for startup to complete */
                        eError = WaitForState(pHandle, OMX_StateExecuting);
#ifdef OMX_GETTIME
                        GT_END("Call to SendCommand <OMX_StateExecuting>");
#endif
                        if(eError != OMX_ErrorNone) {
                            fprintf(stderr, "Error:  hPcmDecoder->WaitForState reports an eError %X\n", eError);
                            goto EXIT;
                        }
                    }
                    if(frmCount == 10) {
                        printf ("%d [TEST APP] $$$$$ Sending Pause command to Codec $$$$$$\n",__LINE__);
#ifdef OMX_GETTIME
                        GT_START();
#endif
                        eError = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StatePause, NULL);
                        if(eError != OMX_ErrorNone) {
                            printf("Error from SendCommand-Pasue State function\n");
                            goto EXIT;
                        }
                        /* Wait for startup to complete */
                        eError = WaitForState(pHandle, OMX_StatePause);
#ifdef OMX_GETTIME
                        GT_END("Call to SendCommand <OMX_StatePause>");
#endif
                        if(eError != OMX_ErrorNone) {
                            printf("G711Encoder->WaitForState reports error\n");
                            goto EXIT;
                        }
                    }
                    break;
                case 4:
                    if(audioinfo->dasfMode == 0) {
                        APP_DPRINT("%d G711 ENCODER RUNNING UNDER FILE 2 FILE MODE \n",__LINE__);
                        if( FD_ISSET(IpBuf_Pipe[0], &rfds) ) {
                            if(frmCnt > 20) {
                                printf("Stop Playback......\n");
                                eError = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
                                if(eError != OMX_ErrorNone) {
                                    printf("Error from SendCommand-Pasue State function\n");
                                    goto EXIT;
                                }
                                eError = WaitForState(pHandle, OMX_StateIdle);
                                if(eError != OMX_ErrorNone) {
                                    printf("G711Encoder->WaitForState reports error\n");
                                    goto EXIT;
                                }
                                sleep(2);
                                printf("Resume Playback.....\n");
                                tcID = 1;
                                frmCnt++;
                            } else {
                                OMX_BUFFERHEADERTYPE* pBuffer;
                                read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
                                pBuffer->nFlags = 0;
                                send_input_buffer(pHandle,pBuffer,fIn);
                            }
                        }
                        frmCnt++;
                    } else {
                        APP_DPRINT("%d G711 ENCODER RUNNING UNDER DASF MODE \n",__LINE__);
                        if(nFrameCount == atoi(argv[6])) {
                            printf("%d [TEST APP] Shutting down ---------- \n",__LINE__);
#ifdef OMX_GETTIME
                            GT_START();
#endif

                            eError = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateIdle, NULL);
                            if(eError != OMX_ErrorNone) {
                                printf("Error from SendCommand-Idle(Stop) State function\n");
                                goto EXIT;
                            }
                            eError = WaitForState(pHandle, OMX_StateIdle);
#ifdef OMX_GETTIME
                            GT_END("Call to SendCommand <OMX_StateIdle>");
#endif
                            if (eError != OMX_ErrorNone ){
                                printf("Error:WaitForState has timed out %d", eError);
                                goto EXIT;
                            }
                        }
                        if(nFrameCount == 20) {
                            printf("%d [TEST APP] Sending Stop After %d frames \n",__LINE__,nFrameCount);
#ifdef OMX_GETTIME
                            GT_START();
#endif
                            eError = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateIdle, NULL);
                            if(eError != OMX_ErrorNone) {
                                printf("Error from SendCommand-Idle(Stop) State function\n");
                                goto EXIT;
                            }
                            eError = WaitForState(pHandle, OMX_StateIdle);
#ifdef OMX_GETTIME
                            GT_END("Call to SendCommand <OMX_StateIdle>");
#endif
                            if (eError != OMX_ErrorNone ){
                                printf("Error:WaitForState has timed out %d", eError);
                                goto EXIT;
                            }
                        }
                    }
                    break;
                default:
                    APP_DPRINT("%d ### Simple DEFAULT Case Here ###\n",__LINE__);
                }

                if( FD_ISSET(OpBuf_Pipe[0], &rfds) ) {
                    OMX_BUFFERHEADERTYPE* pBuf;
                    read(OpBuf_Pipe[0], &pBuf, sizeof(pBuf));
                    APP_DPRINT("%d [TEST APP] pBuf->nFilledLen = %ld\n",__LINE__, pBuf->nFilledLen);
                    nFrameLen = pBuf->nFilledLen;
                    if (nFrameLen != 0) {
                        APP_DPRINT("%d Writing OutputBuffer No: %d to the file nWrite = %d \n",__LINE__, nOutBuff, nFrameLen);
                        fwrite(pBuf->pBuffer, 1, pBuf->nFilledLen, fOut);
                        fflush(fOut);
                    }
                    nFrameCount++;
                    nOutBuff++;
                    OMX_FillThisBuffer(pHandle, pBuf);
                    APP_DPRINT("%d [TEST APP] pBuf->nFlags = %ld\n",__LINE__, pBuf->nFlags);
                }
                if( FD_ISSET(Event_Pipe[0], &rfds) ) {
                    OMX_U8 pipeContents;
                    read(Event_Pipe[0], &pipeContents, sizeof(OMX_U8));

                    if(pipeContents==2){
                      eError = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
                      if(eError != OMX_ErrorNone) {
                        printf("Error from SendCommand-Idle(Stop) State function\n");
                        goto EXIT;
                      }
                      eError = WaitForState(pHandle, OMX_StateIdle);
#ifdef OMX_GETTIME
                      GT_END("Call to SendCommand <OMX_StateIdle>");
#endif
                      if ( eError != OMX_ErrorNone ){
                        printf("Error:WaitForState has timed out %d", eError);
                        goto EXIT;
                      }
                      audioinfo->dasfMode = 0;
                      //pBuf->nFlags = 0;
                      APP_DPRINT("%d [TEST APP] Shutting down ---------- \n",__LINE__);
                    }
                }
            } /* While Loop Ending Here */

            APP_DPRINT("%d [TEST APP] The current state of the component = %d \n",__LINE__,gState);
            fclose(fOut);
            fclose(fIn);
            playcompleted = 0;

            close(IpBuf_Pipe[0]);
            close(IpBuf_Pipe[1]);
            close(OpBuf_Pipe[0]);
            close(OpBuf_Pipe[1]);
            close(Event_Pipe[0]);
            close(Event_Pipe[1]);

            if(tcID == 5 || tcID == 4) {
                sleep (1);
            } 
            APP_DPRINT("%d [TEST APP] G711 Encoded = %d Frames \n",__LINE__,(nOutBuff));
        } /*Test Case 4 & 5 Inner for loop ends here  */


        APP_DPRINT ("%d [TEST APP] Sending the OMX_CommandPortDisable Command\n",__LINE__);
#ifdef OMX_GETTIME
        GT_START();
#endif
        eError = OMX_SendCommand(pHandle, OMX_CommandPortDisable, -1, NULL);
        if(eError != OMX_ErrorNone) {
            APP_DPRINT("%d:: Error from SendCommand OMX_CommandPortDisable\n",__LINE__);
            goto EXIT;
        }

        APP_DPRINT ("%d [TEST APP] Sending the OMX_StateLoaded Command\n",__LINE__);
        eError = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateLoaded, NULL);
        if(eError != OMX_ErrorNone) {
            APP_DPRINT("%d:: Error from SendCommand-Idle State function\n",__LINE__);
            goto EXIT;
        }

        /* free the Allocate and Use Buffers */
#ifndef USE_BUFFER
        APP_DPRINT("%d [TEST APP] Freeing the Allocate OR Use Buffers in TestApp\n",__LINE__);
        if(!audioinfo->dasfMode){
            for(i=0; i < audioinfo->nIpBufs; i++) {
                APP_DPRINT("%d [TEST APP] About to free pInputBufferHeader[%d]\n",__LINE__, i);
                eError = OMX_FreeBuffer(pHandle, OMX_DirInput, pInputBufferHeader[i]);
                if((eError != OMX_ErrorNone)) {
                    APP_DPRINT("%d:: Error in FreeBuffer function\n",__LINE__);
                    goto EXIT;
                }
            }
        }
        for(i=0; i < audioinfo->nOpBufs; i++) {
            APP_DPRINT("%d [TEST APP] About to free pOutputBufferHeader[%d]\n",__LINE__, i);
            eError = OMX_FreeBuffer(pHandle, OMX_DirOutput, pOutputBufferHeader[i]);
            if((eError != OMX_ErrorNone)) {
                APP_DPRINT("%d Error in Free Buffer function\n",__LINE__);
                goto EXIT;
            }
        }
#else
        /* free the App Allocated Buffers */
        APP_DPRINT("%d [TEST APP] Freeing the App Allocated Buffers in TestApp\n",__LINE__);
        if(!audioinfo->dasfmode){
            for(i=0; i < audioinfo->nIpBufs; i++) {
                APP_MEMPRINT("%d [TEST APP] [TESTAPPFREE] pInputBuffer[%d] = %p\n",__LINE__,i,pInputBuffer[i]);
                if(pInputBuffer[i] != NULL){
                    free(pInputBuffer[i]);
                    pInputBuffer[i] = NULL;
                }
            }
        }
        for(i=0; i < audioinfo->nOpBufs; i++) {
            pOutputBuffer[i] = pOutputBuffer[i] - 128;
            APP_MEMPRINT("%d [TEST APP] [TESTAPPFREE] pOutputBuffer[%d] = %p\n",__LINE__,i, pOutputBuffer[i]);
            if(pOutputBuffer[i] != NULL){
                free(pOutputBuffer[i]);
                pOutputBuffer[i] = NULL;
            }
        }
#endif


        eError = WaitForState(pHandle, OMX_StateLoaded);  
#ifdef OMX_GETTIME
        GT_END("Call to SendCommand <OMX_CommandPortDisable, Buffer set then OMX_StateLoaded>");
#endif
        if(eError != OMX_ErrorNone) {
            APP_DPRINT("APP: Error:  WaitForState reports an error %X\n", eError);
            goto EXIT;
        }

        APP_DPRINT("%d [TEST APP] Freeing the Memory Allocated in TestApp\n",__LINE__);
        APP_MEMPRINT("%d [TEST APP] [TESTAPPFREE] %p\n",__LINE__,pG711Param);
        if(pG711Param != NULL){
            free(pG711Param);
            pG711Param = NULL;
        }
        APP_MEMPRINT("%d [TEST APP] [TESTAPPFREE] %p\n",__LINE__,pCompPrivateStruct);
        if(pCompPrivateStruct != NULL){
            free(pCompPrivateStruct);
            pCompPrivateStruct = NULL;
        }
        APP_MEMPRINT("%d [TEST APP] [TESTAPPFREE] %p\n",__LINE__,audioinfo);
        if(audioinfo != NULL){
            free(audioinfo);
            audioinfo = NULL;
        }

        APP_DPRINT("%d [TEST APP] Closing the Input and Output Pipes\n",__LINE__);
        eError = close (IpBuf_Pipe[0]);
        if (0 != eError && OMX_ErrorNone == eError) {
            eError = OMX_ErrorHardware;
            APP_DPRINT("%d Error while closing IpBuf_Pipe[0]\n",__LINE__);
            goto EXIT;
        }
        eError = close (IpBuf_Pipe[1]);
        if (0 != eError && OMX_ErrorNone == eError) {
            eError = OMX_ErrorHardware;
            APP_DPRINT("%d Error while closing IpBuf_Pipe[1]\n",__LINE__);
            goto EXIT;
        }
        eError = close (OpBuf_Pipe[0]);
        if (0 != eError && OMX_ErrorNone == eError) {
            eError = OMX_ErrorHardware;
            APP_DPRINT("%d Error while closing OpBuf_Pipe[0]\n",__LINE__);
            goto EXIT;
        }
        eError = close (OpBuf_Pipe[1]);
        if (0 != eError && OMX_ErrorNone == eError) {
            eError = OMX_ErrorHardware;
            APP_DPRINT("%d Error while closing OpBuf_Pipe[1]\n",__LINE__);
            goto EXIT;
        }
        eError = close (Event_Pipe[0]);
        if (0 != eError && OMX_ErrorNone == eError) {
            eError = OMX_ErrorHardware;
            APP_DPRINT("%d Error while closing Event_Pipe[0]\n",__LINE__);
            goto EXIT;
        }
        eError = close (Event_Pipe[1]);
        if (0 != eError && OMX_ErrorNone == eError) {
            eError = OMX_ErrorHardware;
            APP_DPRINT("%d Error while closing Event_Pipe[1]\n",__LINE__);
            goto EXIT;
        }
        /* exit audio manger */
#ifdef DSP_RENDERING_ON        
        if((write(G711E_fdwrite, &cmd_data, sizeof(cmd_data)))<0){
            printf("%d [TEST APP] - failure to send command to audio manager\n",__LINE__);
        }
        close(G711E_fdwrite);
        close(G711E_fdread);
#endif

        APP_DPRINT("%d [TEST APP] Free the Component handle\n",__LINE__);
        /* Unload the G711 Encoder Component */
        eError = TIOMX_FreeHandle(pHandle);
        if((eError != OMX_ErrorNone)) {
            APP_DPRINT("%d Error in Free Handle function\n",__LINE__);
            goto EXIT;
        }
        APP_DPRINT("%d [TEST APP] Free Handle returned Successfully\n",__LINE__);
    
    } /*Outer for loop ends here */
  
    pthread_mutex_destroy(&WaitForState_mutex);
    pthread_cond_destroy(&WaitForState_threshold);
  
#ifdef OMX_GETTIME
    GT_END("G711_Enc test <End>");
    OMX_ListDestroy(pListHead); 
#endif
    
    eError = TIOMX_Deinit();
    if( (eError != OMX_ErrorNone)) {
        APP_DPRINT("%d:: APP: Error in Deinit Core function\n",__LINE__);
        goto EXIT;
    }
    
    printf("*********************************************************************\n");
    printf("NOTE: An output file %s has been created in file system\n",argv[2]);
    printf("*********************************************************************\n");

 EXIT:

    return eError;
}

OMX_ERRORTYPE send_input_buffer (OMX_HANDLETYPE pHandle, 
                                 OMX_BUFFERHEADERTYPE* pBuffer, 
                                 FILE *fIn)
{
    OMX_ERRORTYPE error = OMX_ErrorNone;

    pBuffer->nFilledLen = fread(pBuffer->pBuffer, 1, pBuffer->nAllocLen, fIn);

    if(pBuffer->nFilledLen == 0) {
        APP_DPRINT("%d Sending Last Input Buffer from TestApp.............. \n",__LINE__);
        pBuffer->nFlags = OMX_BUFFERFLAG_EOS;
    }else{
        pBuffer->nFlags = 0;
    }
    APP_DPRINT("%d [TEST APP] Input Buffer: Calling OMX_EmptyThisBuffer: %p\n",__LINE__,pBuffer);

    pBuffer->nTimeStamp = rand()% 100;
    pBuffer->nTickCount = rand() % 70; 

    if(!playcompleted){
        error = OMX_EmptyThisBuffer(pHandle, pBuffer);
        if( (error != OMX_ErrorNone)) {
            APP_DPRINT("%d [TEST APP] EmptyThisBuffer\n",__LINE__);
            goto EXIT;
        }
    }


 EXIT:
    return error;
}

