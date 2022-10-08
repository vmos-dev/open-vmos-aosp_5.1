
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
 * @file G711DecTest.c
 *
 * This file contains the test application code that invokes the component.
 *
 * @path  $(CSLPATH)\OMAPSW_MPU\linux\audio\src\openmax_il\g711\tests
 *
 * @rev  1.0
 */
/* ----------------------------------------------------------------------------
 *!
 *! Revision History
 *! ===================================
 *! 14-dec-2006 sc: creation  
 * =========================================================================== */


/* standard header files */
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/vt.h>
#include <signal.h>
#include <sys/stat.h>
#include <pthread.h>
#include <stdio.h>
#include <linux/soundcard.h>

/* OMX header files */
#include <OMX_Index.h>
#include <OMX_Types.h>
#include <OMX_Component.h>
#include <OMX_Core.h>
#include <OMX_Audio.h>
#include <TIDspOmx.h>


#ifdef OMX_GETTIME
#include <OMX_Common_Utils.h>
#include <OMX_GetTime.h>     /*Headers for Performance & measuremet    */
#endif

/* ======================================================================= */
/**
 * @def APP_DEBUG Configure Debug traces
 */
/* ======================================================================= */
#undef APP_DEBUG
/*For timestamp and tickcount*/
#undef APP_TIME_TIC_DEBUG
/* ======================================================================= */
/**
 * @def APP_DEBUG Configure Debug traces
 */
/* ======================================================================= */
#undef APP_MEMCHECK
/* ======================================================================= */
/**
 * @def    APP_MEMDEBUG    This Macro turns On the logic to detec memory
 *                         leaks on the App. To debug the component, 
 *                         WMADEC_MEMDEBUG must be defined.
 */
/* ======================================================================= */
#undef APP_MEMDEBUG

void *arr[500] = {NULL}; 
int lines[500]= {0}; 
int bytes[500]= {0}; 
char file[500][50]= {""};

#ifdef APP_MEMDEBUG
int r;
#define newmalloc(x) mymalloc(__LINE__,__FILE__,x)
#define newfree(z) myfree(z,__LINE__,__FILE__)
void * mymalloc(int line, char *s, int size);
int myfree(void *dp, int line, char *s);
#else
#define newmalloc(x) malloc(x)
#define newfree(z) free(z)
#endif

/* ======================================================================= */
/**
 * @def  DASF                           Define a Value for DASF mode 
 */
/* ======================================================================= */
#define DASF 1
/* ======================================================================= */
/**
 * @def  USE_BUFFER                 Buffer allocation method (app vs OMX)
 */
/* ======================================================================= */
#undef USE_BUFFER
/* ======================================================================= */
/**
 * @def  FIFO1, FIFO2                 FIFO
 */
/* ======================================================================= */
#define FIFO1 "/dev/fifo.1"
#define FIFO2 "/dev/fifo.2"
/* ======================================================================= */
/**
 * @def  GAIN                      Define a GAIN value for Configure Audio
 */
/* ======================================================================= */
#define GAIN 95
/* ======================================================================= */
/**
 * @def    INPUT_G711DEC_BUFFER_SIZE             Standart Input Buffer Size
 *                                                (1 frame)
 */
/* ======================================================================= */
#define INPUT_G711DEC_BUFFER_SIZE 80
/* ======================================================================= */
/**
 * @def    OUTPUT_G711DEC_BUFFER_SIZE           Standart Output Buffer Size
 */
/* ======================================================================= */
#define OUTPUT_G711DEC_BUFFER_SIZE 160
/* ======================================================================= */
/**
 * @def    G711DEC_SAMPLING_FREQUENCY          Sampling Frequency   
 */
/* ======================================================================= */
#define G711DEC_SAMPLING_FREQUENCY 8000
/* ======================================================================= */
/**
 * @def    G711_APP_ID          Application Id  
 */
/* ======================================================================= */
#define G711_APP_ID 100
/* ======================================================================= */
/**
 * @def    G711_MAX_NUM_BUFS    Number of buffer
 */
/* ======================================================================= */
#define G711_MAX_NUM_BUFS 10
/* ======================================================================= */
/**
 *  MACROS used for debug
 */
/* ======================================================================= */
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
#define TIME_PRINT(...)     fprintf(stderr,__VA_ARGS__)
#define TICK_PRINT(...)     fprintf(stderr,__VA_ARGS__)
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

/* ======================================================================= */
/**
 *  PRIVATE functions
 */
/* ======================================================================= */
OMX_S16 GetInfoFromBufferHeader(OMX_U8 **pBufPtr, OMX_S16 *pCurBitRate,OMX_S16 *pNextBitRateFlag);
void ResetBufferPointers(OMX_U8 **pBuffer);
OMX_S16 maxint(OMX_S16 a, OMX_S16 b);
OMX_S16 fill_data (OMX_BUFFERHEADERTYPE *pBuf, FILE *fIn);
OMX_ERRORTYPE send_input_buffer (OMX_HANDLETYPE pHandle, OMX_BUFFERHEADERTYPE* pBuffer, FILE *fIn);
void ConfigureAudio();

/* ======================================================================= */
/**
 *  GLOBAL variables
 */
/* ======================================================================= */
FILE *inputToSN = NULL;
OMX_S16 inputPortDisabled = 0;
OMX_S16 outputPortDisabled = 0;
OMX_S16 alternate = 0;
OMX_S16 numRead = 0;
OMX_S16 testCaseNo = 0;
OMX_STRING strG711Decoder = "OMX.TI.G711.decode";

pthread_mutex_t WaitForState_mutex;
pthread_cond_t  WaitForState_threshold;
OMX_U8          WaitForState_flag = 0;
OMX_U8          TargetedState = 0;

/* pipe management */
int IpBuf_Pipe[2] = {0};
int OpBuf_Pipe[2] = {0};
int Event_Pipe[2] = {0};
fd_set rfds;
static OMX_BOOL bInvalidState;
OMX_S16 done = 0;
OMX_S16 dasfmode = 0;
OMX_S16 fsizemode = 0;
/******************************************************************************/
OMX_S16 numInputBuffers = 0;
OMX_S16 numOutputBuffers = 0;
#ifdef USE_BUFFER
OMX_U8* pInputBuffer[10] = {NULL};
OMX_U8* pOutputBuffer[10] = {NULL};
#endif

OMX_BUFFERHEADERTYPE* pInputBufferHeader[10]  = {NULL};
OMX_BUFFERHEADERTYPE* pOutputBufferHeader[10] = {NULL};

int timeToExit = 0;
/* RM control */
int preempted = 0;
/******************************************************************************/


/* ----------------------------------------------------------------------------
 * maxint()
 *
 * safe routine to get the maximum of 2 integers 
 * ---------------------------------------------------------------------------- */
OMX_S16 maxint(OMX_S16 a, OMX_S16 b)
{
    return (a>b) ? a : b;
}

/* ----------------------------------------------------------------------------
 * WaitForState()
 *
 * This function is called by the application.
 * It blocks until the OMX component is not in the DesiredState
 * ---------------------------------------------------------------------------- */
static OMX_ERRORTYPE WaitForState(OMX_HANDLETYPE* pHandle, OMX_STATETYPE DesiredState)
{
    OMX_STATETYPE CurState = OMX_StateInvalid;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_S16 nCnt = 0;
    OMX_COMPONENTTYPE *pComponent = (OMX_COMPONENTTYPE *)pHandle;
    eError = pComponent->GetState(pHandle, &CurState);

    while( (eError == OMX_ErrorNone) && (CurState != DesiredState) && (eError == OMX_ErrorNone) ) {
        // sleep(1);
        if(nCnt++ == 10) {
            APP_DPRINT( "Still Waiting, press CTL-C to continue\n");
        }
        eError = pComponent->GetState(pHandle, &CurState);
    }

    if( eError != OMX_ErrorNone ) return eError;
    return OMX_ErrorNone;
}

/* ----------------------------------------------------------------------------
 * EventHandler() 
 *
 * This function is called by OMX component as 
 * a callback when one of the following OMX_EVENTS
 * occurs:
 *
 * OMX_EventCmdComplete,         component has sucessfully completed a command 
 * OMX_EventError,               component has detected an error condition 
 * OMX_EventMark,                component has detected a buffer mark 
 * OMX_EventPortSettingsChanged, component is reported a port settings change 
 * OMX_EventBufferFlag,          component has detected an EOS  
 * OMX_EventResourcesAcquired,   component has been granted resources and is 
 *                               automatically starting the state change from
 *                               OMX_StateWaitForResources to OMX_StateIdle. 
 * ---------------------------------------------------------------------------- */
OMX_ERRORTYPE EventHandler(OMX_HANDLETYPE hComponent,OMX_PTR pAppData,OMX_EVENTTYPE eEvent,
                           OMX_U32 nData1,OMX_U32 nData2,OMX_PTR pEventData)
{
    OMX_COMPONENTTYPE *pComponent = (OMX_COMPONENTTYPE *)hComponent;
    OMX_STATETYPE state = OMX_StateInvalid;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U8 writeValue = 0;  

    eError = pComponent->GetState (hComponent, &state);
    APP_DPRINT("%d Error returned from GetState\n",__LINE__);
    if(eError != OMX_ErrorNone) {
        APP_DPRINT("%d :: App: Error returned from GetState\n",__LINE__);
    }

    switch (eEvent) {
    case OMX_EventResourcesAcquired:
        writeValue = 1;
        preempted = 0;
        write(Event_Pipe[1], &writeValue, sizeof(OMX_U8));
        APP_DPRINT( "%d :: App: Component OMX_EventResourcesAquired = %d\n", __LINE__,eEvent);
        break;
    case OMX_EventBufferFlag:
        writeValue = 2;  
        write(Event_Pipe[1], &writeValue, sizeof(OMX_U8));
        printf( "%d :: App: Component OMX_EventBufferFlag = %d\n", __LINE__,eEvent);
        break;
    case OMX_EventCmdComplete:
        APP_DPRINT ( "%d :: App: Component State Changed To %d\n", __LINE__,state);
        if (nData1 == OMX_CommandPortDisable) {
            if (nData2 == OMX_DirInput) {
                inputPortDisabled = 1;
            }
            APP_DPRINT( "%d :: App: Component OMX_EventCmdComplete = %d\n", __LINE__,eEvent);
            if (nData2 == OMX_DirOutput) {
                outputPortDisabled = 1;
            }
            APP_DPRINT( "%d :: App: Component OMX_EventCmdComplete = %d\n", __LINE__,eEvent);
        }
        if ((nData1 == OMX_CommandStateSet) && (TargetedState == nData2) && 
            (WaitForState_flag)){
            WaitForState_flag = 0;
            pthread_mutex_lock(&WaitForState_mutex);
            pthread_cond_signal(&WaitForState_threshold);
            pthread_mutex_unlock(&WaitForState_mutex);
        }
        APP_DPRINT( "%d :: App: Component OMX_EventCmdComplete = %d\n", __LINE__,(int)nData2);
        break;

    case OMX_EventError:
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
        APP_DPRINT( "%d :: App: Component OMX_EventError = %d\n", __LINE__,eEvent);
        break;
    
    case OMX_EventMax:
    case OMX_EventMark:
        break;

    default:
        break;
    }

    return eError;
}

/* ----------------------------------------------------------------------------
 * FillBufferDone() 
 *
 * This function is called by OMX component as 
 * a callback when a Buffer has been filled
 * ---------------------------------------------------------------------------- */
void FillBufferDone (OMX_HANDLETYPE hComponent, OMX_PTR ptr, OMX_BUFFERHEADERTYPE* pBuffer)
{
    APP_DPRINT ("APP:::: OUTPUT BUFFER = %p && %p\n",pBuffer, pBuffer->pBuffer);
    APP_DPRINT ("APP:::: pBuffer->nFilledLen = %d\n",(int)pBuffer->nFilledLen);
    /*add on: TimeStamp & TickCount EmptyBufferDone*/ 
    TIME_PRINT("TimeStamp Output: %lld\n",pBuffer->nTimeStamp);
    TICK_PRINT("TickCount Output: %ld\n\n",pBuffer->nTickCount);
    write(OpBuf_Pipe[1], &pBuffer, sizeof(pBuffer));
#ifdef OMX_GETTIME
    if (GT_FlagF == 1 ) /* First Buffer Reply*/  /* 1 = First Buffer,  0 = Not First Buffer  */
    {
        GT_END("Call to FillBufferDone  <First: FillBufferDone>");
        GT_FlagF = 0 ;   /* 1 = First Buffer,  0 = Not First Buffer  */
    }
#endif
}

/* ----------------------------------------------------------------------------
 * EmptyBufferDone() 
 *
 * This function is called by OMX component as 
 * a callback when a Buffer has been emptied
 * ---------------------------------------------------------------------------- */
void EmptyBufferDone(OMX_HANDLETYPE hComponent, OMX_PTR ptr, OMX_BUFFERHEADERTYPE* pBuffer)
{
    APP_DPRINT ("APP:::: INPUT BUFFER = %p && %p\n",pBuffer, pBuffer->pBuffer);
    if (!preempted) {
        write(IpBuf_Pipe[1], &pBuffer, sizeof(pBuffer));
    }
#ifdef OMX_GETTIME
    if (GT_FlagE == 1 ) /* First Buffer Reply*/  /* 1 = First Buffer,  0 = Not First Buffer  */
    {
        GT_END("Call to EmptyBufferDone <First: EmptyBufferDone>");
        GT_FlagE = 0;   /* 1 = First Buffer,  0 = Not First Buffer  */
    }
#endif
}
typedef struct G711DEC_FTYPES{
    unsigned short FrameSizeType;
    unsigned short NmuNLvl;
    unsigned short NoiseLp;
    unsigned long  dBmNoise;
    unsigned short plc;
}G711DEC_FTYPES;
/* ----------------------------------------------------------------------------
 * main() 
 *
 * This function is called at application startup 
 * and drives the G711 Decoder OMX component
 * ---------------------------------------------------------------------------- */
int main(int argc, char* argv[])
{
    OMX_CALLBACKTYPE G711CaBa = {(void *)EventHandler,
                                 (void*)EmptyBufferDone,
                                 (void*)FillBufferDone};
                                                               
    OMX_HANDLETYPE                pHandle;
    OMX_PARAM_PORTDEFINITIONTYPE* pCompPrivateStruct    = NULL; 
    OMX_AUDIO_CONFIG_MUTETYPE*    pCompPrivateStructMute = NULL; 
    OMX_AUDIO_CONFIG_VOLUMETYPE*  pCompPrivateStructVolume = NULL; 
    OMX_AUDIO_PARAM_PCMMODETYPE*  pG711Param = NULL; 
    OMX_COMPONENTTYPE*            pComponent,*pComponent_dasf = NULL; 
    OMX_BUFFERHEADERTYPE*         pInputBufferHeader[G711_MAX_NUM_BUFS] = {NULL};
    OMX_BUFFERHEADERTYPE*         pOutputBufferHeader[G711_MAX_NUM_BUFS] = {NULL}; 

    OMX_ERRORTYPE error = OMX_ErrorNone;
    OMX_U32 AppData = G711_APP_ID;
    TI_OMX_DSP_DEFINITION* audioinfo = malloc(sizeof(TI_OMX_DSP_DEFINITION));
    G711DEC_FTYPES* frameinfo = malloc(sizeof(G711DEC_FTYPES));
    TI_OMX_DATAPATH dataPath;       
    struct timeval tv   ;
    OMX_S16 retval = 0, i = 0, j = 0,k = 0;
    OMX_S16 frmCount = 0;
    OMX_S16 frmCnt = 1;
    OMX_S16 testcnt = 1;
    OMX_S16 testcnt1 = 1;

    OMX_BUFFERHEADERTYPE* pBuf = NULL;

    bInvalidState = OMX_FALSE;
    
    OMX_STATETYPE         state = OMX_StateInvalid;
    OMX_INDEXTYPE         index = 0; 
/*    int g711decfdwrite = 0;
    int g711decfdread = 0;*/

    APP_DPRINT("------------------------------------------------------\n");
    APP_DPRINT("This is Main Thread In G711 DECODER Test Application:\n");
    APP_DPRINT("Test Core 1.5 - " __DATE__ ":" __TIME__ "\n");
    APP_DPRINT("------------------------------------------------------\n");
#ifdef OMX_GETTIME
    printf("Line %d\n",__LINE__);
    GTeError = OMX_ListCreate(&pListHead);
    printf("Line %d\n",__LINE__);
    printf("eError = %d\n",GTeError);
    GT_START();
    printf("Line %d\n",__LINE__);
#endif  
    /* check the input parameters */
    if(argc != 17) {
        printf( "Usage: G711DecTest_common [infile] [outfile] [1-ALaw 2-MuLaw]\
                [sampling freq] [testcase] [dasf mode] [accoustic mode] [nbInputBuf]\
                [Input Buf Size] [nbOutputBuf] [Output Buf size] [Frame Size Type]\
                [NMU Lvl] [Noise LP] [Noise in dBm] [PLC index]\n");
        goto EXIT;
    }

    OMX_S16 numInputBuffers = 0;
    OMX_S16 inputBufferSize = 0;
    OMX_S16 numOutputBuffers = 0;
    OMX_S16 outputBufferSize = 0;
    OMX_S8  fParam1 = 0;
    numInputBuffers = atoi(argv[8]);
    inputBufferSize = atoi(argv[9]);
    numOutputBuffers = atoi(argv[10]);
    outputBufferSize = atoi(argv[11]); 
    fParam1 = atoi(argv[12]);
    
    if(numInputBuffers > 4 && numInputBuffers < 1)
    {
        APP_DPRINT( "Cannot support %u Input buffers\n", numInputBuffers);
        goto EXIT;      
    }
    if(numOutputBuffers > 4 && numOutputBuffers < 0)
    {
        APP_DPRINT( "Cannot support %u Output buffers\n", numOutputBuffers);
        goto EXIT;          
    }
    if(fParam1 > 3 && fParam1 < 0)
    {
        APP_DPRINT( "Cannot support %u such frame size type \n", fParam1);
        printf( "Cannot support %u such frame size type \n", fParam1);

        goto EXIT;          
    }
    
    printf("%d :: App: number of Input buffers = %d \n",__LINE__,numInputBuffers);
    printf("%d :: App: size of input buffers = %d \n",__LINE__,inputBufferSize);
    printf("%d :: App: number of output buffers = %d \n",__LINE__,numOutputBuffers);
    printf("%d :: App: size of Output buffers = %d \n",__LINE__,outputBufferSize);
    
    /* check to see that the input file exists */
    struct stat sb = {0};
    OMX_S16 status = stat(argv[1], &sb);
    if( status != 0 ) {
        APP_DPRINT( "Cannot find file %s. (%u)\n", argv[1], errno);
        goto EXIT;
    }

    /* Open the file of data to be rendered. */
    FILE* fIn = fopen(argv[1], "r");
    if( fIn == NULL ) {
        APP_DPRINT( "Error:  failed to open the file %s for readonly\access\n", argv[1]);
        goto EXIT;
    }

    FILE* fOut = NULL;
    fOut = fopen(argv[2], "w");
    if( fOut == NULL ) {
        APP_DPRINT( "Error:  failed to create the output file %s\n", argv[2]);
        goto EXIT;
    }

    /* Create a pipe used to queue data from the callback. */
    retval = pipe(IpBuf_Pipe);
    if( retval != 0) {
        APP_DPRINT( "Error:Fill Data Pipe failed to open\n");
        goto EXIT;
    }

    retval = pipe(OpBuf_Pipe);
    if( retval != 0) {
        APP_DPRINT( "Error:Empty Data Pipe failed to open\n");
        goto EXIT;
    }
    retval = pipe(Event_Pipe);
    if( retval != 0) {
        APP_DPRINT( "%d %s Error: Empty Data Pipe failed to open\n",__LINE__, __FUNCTION__);
        goto EXIT;
    }

    /* save off the "max" of the handles for the selct statement */
    OMX_S16 fdmax = maxint(IpBuf_Pipe[0], OpBuf_Pipe[0]);
    fdmax = maxint(fdmax,Event_Pipe[0]);
    APP_DPRINT("%d :: G711Test\n",__LINE__);

    error = TIOMX_Init();

    if(error != OMX_ErrorNone) {
        APP_DPRINT("%d :: Error returned by TIOMX_Init()\n",__LINE__);
        goto EXIT;
    }

    APP_DPRINT("%d :: G711Test\n",__LINE__);

    int command = atoi(argv[5]);
    switch (command ) {
    case 1:
        printf ("-------------------------------------\n");
        printf ("Testing Simple PLAY till EOF \n");
        printf ("-------------------------------------\n");
        break;
    case 2:
        printf ("-------------------------------------\n");
        printf ("Testing Stop and Play \n");
        printf ("-------------------------------------\n");
        testcnt = 2;
        break;
    case 3:
        printf ("-------------------------------------\n");
        printf ("Testing PAUSE & RESUME Command\n");
        printf ("-------------------------------------\n");
        break;
    case 4:
        printf ("---------------------------------------------\n");
        printf ("Testing STOP Command by Stopping In-Between\n");
        printf ("---------------------------------------------\n");
        break;
    case 5:
        printf ("-------------------------------------------------\n");
        printf ("Testing Repeated PLAY without Deleting Component\n");
        printf ("-------------------------------------------------\n");
        testcnt = 20;
        break;
    case 6:
        printf ("------------------------------------------------\n");
        printf ("Testing Repeated PLAY with Deleting Component\n");
        printf ("------------------------------------------------\n");
        testcnt1 = 20;
        break;
    case 7:
        printf ("----------------------------------------------------------\n");
        printf ("Testing Multiframe with each buffer size = 2 x frameLength\n");
        printf ("----------------------------------------------------------\n");
        testCaseNo = 7;
        break;
    case 8:
        printf ("------------------------------------------------------------\n");
        printf ("Testing Multiframe with each buffer size = 1/2 x frameLength\n");
        printf ("------------------------------------------------------------\n");
        testCaseNo = 8;
        break;
    case 9:
        printf ("------------------------------------------------------------\n");
        printf ("Testing Multiframe with alternating buffer sizes\n");
        printf ("------------------------------------------------------------\n");
        testCaseNo = 9;
        break;
    case 10:
        printf ("------------------------------------------------------------\n");
        printf ("Testing Mute/Unmute for Playback Stream\n");
        printf ("------------------------------------------------------------\n");
        break;
    case 11:
        printf ("------------------------------------------------------------\n");
        printf ("Testing Set Volume for Playback Stream\n");
        printf ("------------------------------------------------------------\n");
        break;
               
    case 12:
        printf ("------------------------------------------------------------\n");
        printf ("Testing Simple PLAY  \n");
        printf ("------------------------------------------------------------\n");
        break;
               
    }

    APP_DPRINT("%d :: G711Test\n",__LINE__);
    fsizemode = atoi(argv[12]);
    dasfmode = atoi(argv[6]);
    if(dasfmode == 1){
        printf("DASF MODE\n");
#if STATE_TRANSITION_STATE
        ConfigureAudio();
#endif
    }
    else if(dasfmode == 0){
        printf("NON DASF MODE\n");
    }
    else {
        printf("Enter proper DASF mode\n");
        printf("DASF:1\n");
        printf("NON DASF:0\n");
        goto EXIT;
    }

    APP_DPRINT("%d :: G711Test\n",__LINE__);
    
    for(j = 0; j < testcnt1; j++) {
        if(j >= 1) {
            printf ("\n****** Decoding the file %i Time ******* \n", j+1);
            
            /* Create a pipe used to queue data from the callback. */
            retval = pipe( IpBuf_Pipe);
            if( retval != 0) {
                APP_DPRINT( "Error:Fill Data Pipe failed to open\n");
                goto EXIT;
            }

            retval = pipe( OpBuf_Pipe);
            if( retval != 0) {
                APP_DPRINT( "Error:Empty Data Pipe failed to open\n");
                goto EXIT;
            }
            fIn = fopen(argv[1], "r");
            if( fIn == NULL ) {
                fprintf(stderr, "Error:  failed to open the file %s for readonly\
                                                                   access\n", argv[1]);
                goto EXIT;
            }

            fOut = fopen(argv[2], "w");
            if( fOut == NULL ) {
                fprintf(stderr, "Error:  failed to create the output file \n");
                goto EXIT;
            }
            error = TIOMX_Init();
            inputToSN = fopen("outputSecondTime.log","w");

        }
        else {
            inputToSN = fopen("outputFirstTime.log","w");
        }
#ifdef DSP_RENDERING_ON  
        if((g711decfdwrite=open(FIFO1,O_WRONLY))<0)
        {
            printf("[G711TEST] - failure to open WRITE pipe\n");
        }
        else
        {
            printf("[G711TEST] - opened WRITE pipe\n");
        }
        if((g711decfdread=open(FIFO2,O_RDONLY))<0)
        {
            printf("[G711TEST] - failure to open READ pipe\n");
            goto EXIT;
        }
        else
        {
            printf("[G711TEST] - opened READ pipe\n");
        }
#endif        
        /* Load the G711 Encoder Component */
        APP_DPRINT("%d :: G711Test\n",__LINE__);
#ifdef OMX_GETTIME
        GT_START();
        error = OMX_GetHandle(&pHandle, strG711Decoder, &AppData, &G711CaBa);
        GT_END("Call to OMX_GetHandle");
#else
        error = TIOMX_GetHandle(&pHandle, strG711Decoder, &AppData, &G711CaBa);
#endif
        APP_DPRINT("%d :: G711Test\n",__LINE__);
        if((error != OMX_ErrorNone) || (pHandle == NULL)) {
            APP_DPRINT ("Error in Get Handle function\n");
            goto EXIT;
        }

        APP_DPRINT("%d :: G711Test\n",__LINE__);

        pCompPrivateStruct = malloc (sizeof (OMX_PARAM_PORTDEFINITIONTYPE));
        if (pCompPrivateStruct == 0) { 
            printf("Malloc failed\n");
            error = -1;
            goto EXIT;
        }

        if (dasfmode) 
        {
#ifdef RTM_PATH    
            dataPath = DATAPATH_APPLICATION_RTMIXER;
#endif

#ifdef ETEEDN_PATH
            dataPath = DATAPATH_APPLICATION;
#endif        
        }
        /* set playback stream mute/unmute */ 
        pCompPrivateStructMute = malloc (sizeof(OMX_AUDIO_CONFIG_MUTETYPE)); 
        if(pCompPrivateStructMute == NULL) { 
            printf("%d :: App: Malloc Failed\n",__LINE__); 
            goto EXIT; 
        }   
        pCompPrivateStructVolume = malloc (sizeof(OMX_AUDIO_CONFIG_VOLUMETYPE)); 
        if(pCompPrivateStructVolume == NULL) { 
            printf("%d :: App: Malloc Failed\n",__LINE__); 
            goto EXIT; 
        } 
        
        APP_MEMPRINT("%d:::[TESTAPPALLOC] %p\n",__LINE__,pCompPrivateStruct);
        APP_DPRINT("%d :: G711Test\n",__LINE__);
        pCompPrivateStruct->nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
        pCompPrivateStruct->nVersion.s.nVersionMajor = 0xF1; 
        pCompPrivateStruct->nVersion.s.nVersionMinor = 0xF2; 
        APP_DPRINT("%d :: G711Test\n",__LINE__);
        /* Send input port config */
        pCompPrivateStruct->nPortIndex = OMX_DirInput; 
        pCompPrivateStruct->format.audio.cMIMEType = malloc(20);
        strcpy(pCompPrivateStruct->format.audio.cMIMEType,"NONMIME");
        pCompPrivateStruct->eDir = OMX_DirInput; 
        pCompPrivateStruct->nPortIndex = OMX_DirInput; 
        pCompPrivateStruct->nBufferCountActual = numInputBuffers; 
        pCompPrivateStruct->nBufferSize = inputBufferSize; 
        pCompPrivateStruct->format.audio.eEncoding = OMX_AUDIO_CodingG711; 
        pCompPrivateStruct->bEnabled = 1;
        pCompPrivateStruct->bPopulated = 0;
        
#ifdef OMX_GETTIME
        GT_START();
        error = OMX_SetParameter (pHandle, OMX_IndexParamPortDefinition, pCompPrivateStruct);
        GT_END("Set Parameter Test-SetParameter");
#else
        error = OMX_SetParameter (pHandle, OMX_IndexParamPortDefinition, pCompPrivateStruct);
#endif
        if (error != OMX_ErrorNone) {
            error = OMX_ErrorBadParameter;
            printf ("%d:: OMX_ErrorBadParameter\n",__LINE__);
            goto EXIT;
        }
        /* Send output port config */
        pCompPrivateStruct->nPortIndex = OMX_DirOutput; 
        pCompPrivateStruct->eDir = OMX_DirOutput; 
        pCompPrivateStruct->nPortIndex = OMX_DirOutput;
        pCompPrivateStruct->nBufferCountActual = numOutputBuffers; 
        pCompPrivateStruct->nBufferSize = outputBufferSize; 
    
        if(dasfmode == 1) {
            pCompPrivateStruct->nBufferCountActual = 0;
        }
#ifdef OMX_GETTIME
        GT_START();
        error = OMX_SetParameter (pHandle, OMX_IndexParamPortDefinition, pCompPrivateStruct);
        GT_END("Set Parameter Test-SetParameter");
#else
        error = OMX_SetParameter (pHandle, OMX_IndexParamPortDefinition, pCompPrivateStruct);
#endif
        if (error != OMX_ErrorNone) {
            error = OMX_ErrorBadParameter;
            printf ("%d:: OMX_ErrorBadParameter\n",__LINE__);
            goto EXIT;
        }   
        /* default setting for Mute/Unmute */ 
        pCompPrivateStructMute->nSize = sizeof (OMX_AUDIO_CONFIG_MUTETYPE); 
        pCompPrivateStructMute->nVersion.s.nVersionMajor    = 0xF1; 
        pCompPrivateStructMute->nVersion.s.nVersionMinor    = 0xF2; 
        pCompPrivateStructMute->nPortIndex                  = OMX_DirInput; 
        pCompPrivateStructMute->bMute                       = OMX_FALSE; 

        /* default setting for volume */ 
        pCompPrivateStructVolume->nSize = sizeof(OMX_AUDIO_CONFIG_VOLUMETYPE); 
        pCompPrivateStructVolume->nVersion.s.nVersionMajor  = 0xF1; 
        pCompPrivateStructVolume->nVersion.s.nVersionMinor  = 0xF2; 
        pCompPrivateStructVolume->nPortIndex                = OMX_DirInput; 
        pCompPrivateStructVolume->bLinear                   = OMX_FALSE; 
        pCompPrivateStructVolume->sVolume.nValue            = 50;               /* actual volume */ 
        pCompPrivateStructVolume->sVolume.nMin              = 0;                /* min volume */ 
        pCompPrivateStructVolume->sVolume.nMax              = 100;              /* max volume */ 

#ifndef USE_BUFFER
        /* Allocate Input buffers with OMX_AllocateBuffer() API */
        for (i=0; i < numInputBuffers; i++) {
            /* allocate input buffer */
            APP_DPRINT("%d :: About to call OMX_AllocateBuffer\n",__LINE__);
            error = OMX_AllocateBuffer(pHandle,&pInputBufferHeader[i],0,NULL,inputBufferSize);
            APP_DPRINT("%d :: called OMX_AllocateBuffer\n",__LINE__);
            if(error != OMX_ErrorNone) {
                APP_DPRINT("%d :: Error returned by OMX_AllocateBuffer()\n",__LINE__);
                goto EXIT;
            }

        }
        /* Allocate Output buffers with OMX_AllocateBuffer() API */
        for (i=0; i < numOutputBuffers; i++) {
            /* allocate output buffer */
            APP_DPRINT("%d :: About to call OMX_AllocateBuffer\n",__LINE__);
            error = OMX_AllocateBuffer(pHandle,&pOutputBufferHeader[i],1,NULL,outputBufferSize);
            APP_DPRINT("%d :: called OMX_AllocateBuffer\n",__LINE__);
            if(error != OMX_ErrorNone) {
                APP_DPRINT("%d :: Error returned by OMX_AllocateBuffer()\n",__LINE__);
                goto EXIT;
            }
        }
#else
        /* Allocate Input buffers with OMX_UseBuffer() API */
        for (i=0; i < numInputBuffers; i++)
        {
            pInputBuffer[i] = (OMX_U8*)malloc(inputBufferSize + EXTRA_BUFFBYTES);
            APP_MEMPRINT("%d:::[TESTAPPALLOC] %p\n",__LINE__,pInputBuffer[i]);
            pInputBuffer[i] = pInputBuffer[i] + CACHE_ALIGNMENT;

            /* allocate input buffer */
            APP_DPRINT("%d :: About to call OMX_UseBuffer\n",__LINE__);
            error = OMX_UseBuffer(pHandle,&pInputBufferHeader[i],0,NULL,inputBufferSize,pInputBuffer[i]);
            APP_DPRINT("%d :: called OMX_UseBuffer\n",__LINE__);
            if(error != OMX_ErrorNone)
            {
                APP_DPRINT("%d :: Error returned by OMX_UseBuffer()\n",__LINE__);
                goto EXIT;
            }

        }
        /* Allocate Output buffers with OMX_UseBuffer() API */
        for ( i = 0 ; i < numOutputBuffers ; i++ )
        {
            pOutputBuffer[i] = (OMX_U8*)malloc (outputBufferSize + EXTRA_BUFFBYTES);
            APP_MEMPRINT("%d:::[TESTAPPALLOC] %p\n",__LINE__,pOutputBuffer);
            pOutputBuffer[i] = pOutputBuffer[i] + CACHE_ALIGNMENT;

            /* allocate output buffer */
            APP_DPRINT("%d :: About to call OMX_UseBuffer\n",__LINE__);
            error = OMX_UseBuffer(pHandle,&pOutputBufferHeader[i],1,NULL,outputBufferSize,pOutputBuffer[i]);
            APP_DPRINT("%d :: called OMX_UseBuffer\n",__LINE__);
            if(error != OMX_ErrorNone)
            {
                APP_DPRINT("%d :: Error returned by OMX_UseBuffer()\n",__LINE__);
                goto EXIT;
            }

        }
#endif

        /* Send  G711 config for input */
        pG711Param = malloc (sizeof (OMX_AUDIO_PARAM_PCMMODETYPE));
        APP_MEMPRINT("%d:::[TESTAPPALLOC] %p\n",__LINE__,pG711Param);
        if(pG711Param == NULL) { 
            printf("%d :: App: Malloc Failed\n",__LINE__); 
            goto EXIT; 
        } 
        
        pG711Param->nPortIndex = OMX_DirInput;
        OMX_GetParameter(pHandle, OMX_IndexParamAudioPcm, pG711Param);
        pG711Param->nSize = sizeof (OMX_AUDIO_PARAM_PCMMODETYPE);
        pG711Param->nVersion.s.nVersionMajor = 0xF1;
        pG711Param->nVersion.s.nVersionMinor = 0xF2;

        pG711Param->nChannels = 1; /* mono */
        pG711Param->eNumData = OMX_NumericalDataUnsigned;
        pG711Param->eEndian = OMX_EndianLittle;
        pG711Param->bInterleaved = OMX_FALSE;
        pG711Param->nBitPerSample = 8;
        pG711Param->nSamplingRate = 0; /* means undefined in the OMX standard */
        
        for ( i = 0 ; i < OMX_AUDIO_MAXCHANNELS ; i++ )
            pG711Param->eChannelMapping[0] = OMX_AUDIO_ChannelNone;

        /* extract compression format from command line */
        if (atoi(argv[3]) == OMX_AUDIO_PCMModeALaw)
            pG711Param->ePCMMode = OMX_AUDIO_PCMModeALaw;
        else if (atoi(argv[3]) == OMX_AUDIO_PCMModeMULaw)
            pG711Param->ePCMMode = OMX_AUDIO_PCMModeMULaw;

        else {
            printf("Enter proper G711 mode\n");
            printf("A-Law:1\n");
            printf("MU-Law:2\n");
            goto EXIT;
        }
#ifdef OMX_GETTIME
        GT_START();
        error = OMX_SetParameter (pHandle, OMX_IndexParamAudioPcm, pG711Param);
        GT_END("Set Parameter Test-SetParameter");
#else
        error = OMX_SetParameter (pHandle, OMX_IndexParamAudioPcm, pG711Param);
#endif  
        if (error != OMX_ErrorNone) {
            error = OMX_ErrorBadParameter;
            printf ("%d:: OMX_ErrorBadParameter\n",__LINE__);
            goto EXIT;
        }
        /* Send  G711 config for output */
        pG711Param->nPortIndex = OMX_DirOutput;
        pG711Param->nBitPerSample = 16;
        pG711Param->ePCMMode = OMX_AUDIO_PCMModeLinear;
        
        /* extract sampling rate from command line */
        pG711Param->nSamplingRate = atoi(argv[4]); 
#ifdef OMX_GETTIME
        GT_START(); 
        error = OMX_SetParameter (pHandle,OMX_IndexParamAudioPcm, pG711Param);
        GT_END("Set Parameter Test-SetParameter");
#else
        error = OMX_SetParameter (pHandle,OMX_IndexParamAudioPcm, pG711Param);
#endif
        if (error != OMX_ErrorNone) {
            error = OMX_ErrorBadParameter;
            printf ("%d:: OMX_ErrorBadParameter\n",__LINE__);
            goto EXIT;
        }

        pComponent_dasf = (OMX_COMPONENTTYPE *)pHandle;

        /** Getting the frame params */
        frameinfo->FrameSizeType = atoi(argv[12]);
        frameinfo->NmuNLvl = atoi(argv[13]);
        frameinfo->NoiseLp = atoi(argv[14]);
        frameinfo->dBmNoise = atoi(argv[15]);
        frameinfo->plc = atoi(argv[16]);
        
        error = OMX_GetExtensionIndex(pHandle, "OMX.TI.index.config.g711dec.frameparams",&index);
        if (error != OMX_ErrorNone) {
            printf("Error getting extension index\n");
            goto EXIT;
        }
        
        error = OMX_SetConfig (pHandle, index, frameinfo);
        if(error != OMX_ErrorNone) {
            error = OMX_ErrorBadParameter;
            APP_DPRINT("%d :: Error from OMX_SetConfig() function\n",__LINE__);
            goto EXIT;
        }
        
        /* get TeeDN or ACDN mode */
        audioinfo->acousticMode = atoi(argv[7]);

        /* Process DASF mode */
        if(dasfmode == 1){
            audioinfo->dasfMode = 1;
        }
        else if(dasfmode == 0){
            audioinfo->dasfMode = 0;

        }
        else {
            printf("Enter proper DASF mode\n");
            printf("DASF:1\n");
            printf("NON DASF:0\n");
            goto EXIT;
        }   

        /* get extension index to define proprietary DASF settings */
        error = OMX_GetExtensionIndex(pHandle, "OMX.TI.index.config.g711headerinfo",&index);
        if (error != OMX_ErrorNone) {
            printf("Error getting extension index\n");
            goto EXIT;
        }

/*        cmd_data.hComponent = pHandle;
        cmd_data.AM_Cmd = AM_CommandIsOutputStreamAvailable;*/
        
        /* for decoder, using AM_CommandIsInputStreamAvailable */
/*        cmd_data.param1 = 0;*/
#ifdef DSP_RENDERING_ON        
        if((write(g711decfdwrite, &cmd_data, sizeof(cmd_data)))<0) {
            printf("%d [G711 Dec Component] - send command to audio manager\n", __LINE__);
            goto EXIT;
        }
        if((read(g711decfdread, &cmd_data, sizeof(cmd_data)))<0) {
            printf("%d [G711 Dec Component] - failure to get data from the audio manager\n", __LINE__);
            goto EXIT;
        }
#endif  
      
/*        audioinfo->streamId = cmd_data.streamID;*/
        if(audioinfo->dasfMode)
            printf("***************StreamId=%d******************\n", (int)audioinfo->streamId );
        APP_DPRINT("%d :: TestApp: Set Config, pHandle %p\n", __LINE__, pHandle);
        error = OMX_SetConfig(pHandle,index,audioinfo);
        if (error != OMX_ErrorNone) {
            printf("Error in SetConfig\n");
            goto EXIT;
        }
        APP_DPRINT("%d :: TestApp: Get Extension Index, pHandle %p\n", __LINE__, pHandle);
        error = OMX_GetExtensionIndex(pHandle, "OMX.TI.index.config.g711dec.datapath",&index);
        if (error != OMX_ErrorNone) {
            printf("Error getting extension index\n");
            goto EXIT;
        }
        APP_DPRINT("%d :: TestApp: Set Config, pHandle %p\n", __LINE__, pHandle);
        error = OMX_SetConfig (pHandle, index, &dataPath);
        if(error != OMX_ErrorNone) {
            error = OMX_ErrorBadParameter;
            APP_DPRINT("%d :: WmaDecTest.c :: Error from OMX_SetConfig() function\n",__LINE__);
            goto EXIT;
        }    
        
        /* start the OMX component */
        APP_DPRINT("%d :: TestApp: Change state to Idle, pHandle %p\n", __LINE__, pHandle);
#ifdef OMX_GETTIME
        GT_START();
#endif
        error = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
        if(error != OMX_ErrorNone) {
            APP_DPRINT ("Error from SendCommand-Idle(Init) State function\n");
            goto EXIT;
        }

        /* Wait for startup to complete */
        error = WaitForState(pHandle, OMX_StateIdle);
#ifdef OMX_GETTIME
        GT_END("Call to SendCommand <OMX_StateIdle>");
#endif
        if(error != OMX_ErrorNone) {
            APP_DPRINT( "Error:  hG711Encoder->WaitForState reports an error %X\n", error);
            goto EXIT;
        }
        APP_DPRINT("%d :: TestApp: State Changed to Idle\n", __LINE__);
        for(i = 0; i < testcnt; i++) 
        {
            if(i > 0) 
            {
                printf ("\n***** Decoding the file for %i Time ***** \n",i+1);

                close(IpBuf_Pipe[0]);
                close(IpBuf_Pipe[1]);
                close(OpBuf_Pipe[0]);
                close(OpBuf_Pipe[1]);
                close(Event_Pipe[0]);
                close(Event_Pipe[1]);

                /* Create a pipe used to queue data from the callback. */
                retval = pipe(IpBuf_Pipe);
                if( retval != 0) 
                {
                    APP_DPRINT( "Error:Fill Data Pipe failed to open\n");
                    goto EXIT;
                }

                retval = pipe(OpBuf_Pipe);
                if( retval != 0) 
                {
                    APP_DPRINT( "Error:Empty Data Pipe failed to open\n");
                    goto EXIT;
                }
                
                retval = pipe(Event_Pipe);
                if( retval != 0) {
                    APP_DPRINT( "%d Error: Empty Event Pipe failed to open\n",__LINE__);
                    goto EXIT;
                }
                
                fIn = fopen(argv[1], "r");
                if(fIn == NULL) 
                {
                    fprintf(stderr, "Error:  failed to open the file %s for readonly access\n", argv[1]);
                    goto EXIT;
                }
                
                fOut = fopen(argv[2], "w");
                if(fOut == NULL) 
                {
                    fprintf(stderr, "Error:  failed to create the output file \n");
                    goto EXIT;
                }
            }

            done = 0;

            printf ("Basic Function:: Sending OMX_StateExecuting Command\n");
#ifdef OMX_GETTIME
            GT_START();
#endif
            error = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
            if(error != OMX_ErrorNone) 
            {
                APP_DPRINT ("Error from SendCommand-Executing State function\n");
                goto EXIT;
            }
            pComponent = (OMX_COMPONENTTYPE *)pHandle;
            error = WaitForState(pHandle, OMX_StateExecuting);
#ifdef OMX_GETTIME
            GT_END("Call to SendCommand <OMX_StateExecuting>");
#endif
            if(error != OMX_ErrorNone) 
            {
                APP_DPRINT( "Error:  hG711Decoder->WaitForState reports an error %X\n", error);
                goto EXIT;
            }

            for (k=0; k < numInputBuffers; k++) 
            {
#ifdef OMX_GETTIME
                if (k==0)
                { 
                    GT_FlagE=1;  /* 1 = First Buffer,  0 = Not First Buffer  */
                    GT_START(); /* Empty Bufffer */
                }
#endif
                error = send_input_buffer (pHandle, pInputBufferHeader[k], fIn);
            }

            if (dasfmode == 0) 
            {
                for (k=0; k < numOutputBuffers; k++) 
                {
#ifdef OMX_GETTIME
                    if (k==0)
                    { 
                        GT_FlagF=1;  /* 1 = First Buffer,  0 = Not First Buffer  */
                        GT_START(); /* Fill Buffer */
                    }
#endif
                    OMX_FillThisBuffer(pHandle,  pOutputBufferHeader[k]);
                }
            }
            
            error = OMX_GetState(pHandle, &state);
            
            while( (error == OMX_ErrorNone) && (state != OMX_StateIdle) && 
                   (state != OMX_StateInvalid)) {

                FD_ZERO(&rfds);
                FD_SET(IpBuf_Pipe[0], &rfds);
                FD_SET(OpBuf_Pipe[0], &rfds);
                FD_SET(Event_Pipe[0], &rfds);
                tv.tv_sec = 1;
                tv.tv_usec = 0;
                frmCount++;

                retval = select(fdmax+1, &rfds, NULL, NULL, &tv);
                if(retval == -1) 
                {
                    perror("select()");
                    APP_DPRINT ( " : Error \n");
                    break;
                }

                if(retval == 0) {
                    APP_DPRINT ("%d :: BasicFn App Timeout !!!!!!!!!!! \n",__LINE__);
                }

                switch (command) 
                {
                case 1: /* Testing Simple PLAY till EOF */
                case 5: /* Testing Repeated PLAY without Deleting Component */
                case 6: /* Testing Repeated PLAY with Deleting Component */
                case 7: /* Testing Multiframe with each buffer size = 2 x frameLength */
                case 8: /* Testing Multiframe with each buffer size = 1/2 x frameLength */
                case 9: /* Testing Multiframe with alternating buffer sizes */
                    if(FD_ISSET(IpBuf_Pipe[0], &rfds)) {
                        OMX_BUFFERHEADERTYPE* pBuffer;
                        read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));             
                        error = send_input_buffer (pHandle, pBuffer, fIn);
                           
                        if (error != OMX_ErrorNone) {
                            printf ("Error While reading input pipe\n");
                            goto EXIT;
                        }
                        frmCnt++;
                        pBuffer->nFlags = 0;
                    }
                    break;

                case 2: /* Testing Stop and Play */
                case 4: /* Testing STOP Command by Stopping In-Between */
                    if(FD_ISSET(IpBuf_Pipe[0], &rfds)) {
                        OMX_BUFFERHEADERTYPE* pBuffer;
                        read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
                        frmCnt++;
                        pBuffer->nFlags = 0; 
                        if(frmCnt == 50) { /*100 Frames processed */
                            fprintf(stderr, "Shutting down since 100 frames were sent---------- \n");
#ifdef OMX_GETTIME
                            GT_START();
#endif
                            error = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateIdle, NULL);
                            if(error != OMX_ErrorNone) {
                                fprintf (stderr,"Error from SendCommand-Idle(Stop) State function\n");
                                goto EXIT;
                            }
                            error = WaitForState(pHandle, OMX_StateIdle);
#ifdef OMX_GETTIME
                            GT_END("Call to SendCommand <OMX_StateIdle>");
#endif
                            if(error != OMX_ErrorNone) {
                                fprintf(stderr, "Error:  hPcmDecoder->WaitForState reports an error %X\n", error);
                                goto EXIT;
                            }
                            done = 1;   
                        }
                        error = send_input_buffer (pHandle, pBuffer, fIn);
                        if (error != OMX_ErrorNone) {
                            printf ("Error While reading input pipe\n");
                            goto EXIT;
                        }
                    }
                    break;

                case 3: /* Testing PAUSE & RESUME Command */
                    if (frmCount == 16) {  /*100 Frames processed */
                        printf (" Sending Resume command to Codec \n");
#ifdef OMX_GETTIME
                        GT_START();
#endif
                        error = OMX_SendCommand(pHandle, OMX_CommandStateSet,OMX_StateExecuting, NULL);
                        if(error != OMX_ErrorNone) {
                            fprintf (stderr,"Error from SendCommand-Executing State function\n");
                            goto EXIT;
                        }
                        /* Wait for startup to complete */
                        error = WaitForState(pHandle, OMX_StateExecuting);
#ifdef OMX_GETTIME
                        GT_END("Call to SendCommand <OMX_StateExecuting>");
#endif
                        if(error != OMX_ErrorNone) {
                            fprintf(stderr, "Error:  hPcmDecoder->WaitForState reports an error %X\n", error);
                            goto EXIT;
                        }
                    }
                    if(frmCount == 6) {   /*6 Frames processed */
                        printf (" Sending Pause command to Codec \n");
#ifdef OMX_GETTIME
                        GT_START(); 
#endif
                        error = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StatePause, NULL);
                        if(error != OMX_ErrorNone) {
                            fprintf (stderr,"Error from SendCommand-Pasue State function\n");
                            goto EXIT;
                        }
                        /* Wait for startup to complete */
                        error = WaitForState(pHandle, OMX_StatePause);
#ifdef OMX_GETTIME
                        GT_END("Call to SendCommand <OMX_StatePause>");
#endif                      
                        if(error != OMX_ErrorNone) {
                            fprintf(stderr, "Error:  hPcmDecoder->WaitForState reports an error %X\n", error);
                            goto EXIT;
                        }
                    }

                    if(FD_ISSET(IpBuf_Pipe[0], &rfds)) {
                        OMX_BUFFERHEADERTYPE* pBuffer;
                        read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
                        error = send_input_buffer (pHandle, pBuffer, fIn);
                        if (error != OMX_ErrorNone) {
                            printf ("Error While reading input pipe\n");
                            goto EXIT;
                        }
                        frmCnt++;           
                    }
                    break;
                        
                case 10: /* Testing Mute/Unmute for Playback Stream */
                    if(FD_ISSET(IpBuf_Pipe[0], &rfds)) { 
                        OMX_BUFFERHEADERTYPE* pBuffer; 

                        read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer)); 
                        pBuffer->nFlags = 0; 
                        error = send_input_buffer (pHandle, pBuffer, fIn); 
                        if (error != OMX_ErrorNone) { 
                            goto EXIT; 
                        } 
                        frmCnt++; 
                    }    
                    if(frmCnt == 50) { 
                        printf("************Mute the playback stream*****************\n"); 
                        pCompPrivateStructMute->bMute = OMX_TRUE; 
                        error = OMX_SetConfig(pHandle, OMX_IndexConfigAudioMute, pCompPrivateStructMute); 
                        if (error != OMX_ErrorNone) { 
                            error = OMX_ErrorBadParameter; 
                            goto EXIT; 
                        } 
                    } 
                    if(frmCnt == 120) { 
                        printf("************Unmute the playback stream*****************\n"); 
                        pCompPrivateStructMute->bMute = OMX_FALSE; 
                        error = OMX_SetConfig(pHandle, OMX_IndexConfigAudioMute, pCompPrivateStructMute); 
                        if (error != OMX_ErrorNone) { 
                            error = OMX_ErrorBadParameter; 
                            goto EXIT; 
                        } 
                    } 
                    break; 

                case 11: /* test set volume for playback stream */
                    if(FD_ISSET(IpBuf_Pipe[0], &rfds)) { 
                        OMX_BUFFERHEADERTYPE* pBuffer; 

                        read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer)); 
                        pBuffer->nFlags = 0; 
                        error = send_input_buffer (pHandle, pBuffer, fIn); 
                        if (error != OMX_ErrorNone) { 
                            goto EXIT; 
                        } 
                        frmCnt++; 
                    } 
                    if(frmCnt == 10) { 
                        printf("************Set stream volume to high*****************\n"); 
                        pCompPrivateStructVolume->sVolume.nValue = 0x8000; 
                        error = OMX_SetConfig(pHandle, OMX_IndexConfigAudioVolume, pCompPrivateStructVolume); 
                        if (error != OMX_ErrorNone) { 
                            error = OMX_ErrorBadParameter; 
                            goto EXIT; 
                        } 
                    } 
                    if(frmCnt == 85) { 
                        printf("************Set stream volume to low*****************\n"); 
                        pCompPrivateStructVolume->sVolume.nValue = 0x1000; 
                        error = OMX_SetConfig(pHandle, OMX_IndexConfigAudioVolume, pCompPrivateStructVolume); 
                        if (error != OMX_ErrorNone) { 
                            error = OMX_ErrorBadParameter; 
                            goto EXIT; 
                        } 
                    } 
                    break; 
            
                case 12: /* test unsupported content*/
                    if(FD_ISSET(IpBuf_Pipe[0], &rfds)) {
                        int a, b;
                        a = (int)argv[4];
                        b = (int)argv[7];
                        OMX_BUFFERHEADERTYPE* pBuffer;
                        read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer)); 
                        error = send_input_buffer (pHandle, pBuffer, fIn);
                        if(((a != 8000)) || ((b <= 0) || (b >= 3))){
                            printf("************ Unsupported content ****************\n ");
                            goto EXIT;
                        }
                        if (error != OMX_ErrorNone) {
                            printf ("Error While reading input pipe\n");
                            goto EXIT;
                        }
                        frmCnt++;
                    }                       
                default:
                    APP_DPRINT("%d :: ### Running Simple DEFAULT Case Here ###\n",__LINE__);

                } /* Switch Command Ending Here */

                if( FD_ISSET(OpBuf_Pipe[0], &rfds) ) {            
                    read(OpBuf_Pipe[0], &pBuf, sizeof(pBuf));
                    if (OUTPUT_G711DEC_BUFFER_SIZE != pBuf->nFilledLen ) {
                        APP_DPRINT ("%d : WARNING: Different Size, %d\n",__LINE__,(int)pBuf->nFilledLen);
                    }
                    if ((state != OMX_StateExecuting) && (pBuf->nFilledLen > 0))
                        printf("Writing remaining output buffer\n");          
                    fwrite(pBuf->pBuffer, 1, pBuf->nFilledLen, fOut);
                    fflush(fOut);
                    if (state == OMX_StateExecuting ) {
                        OMX_FillThisBuffer(pHandle, pBuf);
                    }
                }
                
                error = pComponent->GetState(pHandle, &state);
                if(error != OMX_ErrorNone) {
                    APP_DPRINT("%d:: Warning:  hG711Encoder->GetState has returned status %X\n",__LINE__, error);
                    goto EXIT;
                } 
                else if (preempted){
                    sched_yield();
                }               
            } /* While Loop Ending Here */

            printf ("The current state of the component = %d \n",state);
            fclose(fOut);
            fclose(fIn);    
            if(( command == 5) || (command == 2)) { /*If test is Stop & Play or Repeated*/
                sleep (3);                                     /*play without deleting the component*/
            } 
        } /* For loop on testcnt ends here */

/*        cmd_data.hComponent = pHandle;
        cmd_data.AM_Cmd = AM_Exit;*/
#ifdef DSP_RENDERING_ON        
        if((write(g711decfdwrite, &cmd_data, sizeof(cmd_data)))<0)
            printf("%d ::TestApp :: [G711 Dec Component] - send command to audio manager\n",__LINE__);
        close(g711decfdwrite);
        close(g711decfdread);
#endif    
        printf ("Sending the StateLoaded Command\n");
#ifdef OMX_GETTIME
        GT_START();
#endif
        error = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateLoaded, NULL);
        if(error != OMX_ErrorNone) {
            APP_DPRINT ("%d:: Error from SendCommand-Idle State function\n",__LINE__);
            goto EXIT;
        }

        error = OMX_SendCommand(pHandle, OMX_CommandPortDisable, -1, NULL);


        if(error != OMX_ErrorNone) {
            APP_DPRINT( "Error:  hG711Encoder->WaitForState reports an error %X\n", error);
            goto EXIT;
        }

        /* free buffers */
        for (i=0; i < numInputBuffers; i++) {
            error = OMX_FreeBuffer(pHandle,OMX_DirInput,pInputBufferHeader[i]);
            if( (error != OMX_ErrorNone)) {
                APP_DPRINT ("%d:: Error in Free Handle function\n",__LINE__);
                goto EXIT;
            }
        }

        for (i=0; i < numOutputBuffers; i++) {
            error = OMX_FreeBuffer(pHandle,OMX_DirOutput,pOutputBufferHeader[i]);
            if( (error != OMX_ErrorNone)) {
                APP_DPRINT ("%d:: Error in Free Handle function\n",__LINE__);
                goto EXIT;
            }
        }

#ifdef USE_BUFFER
        /* free the App Allocated Buffers */
        printf("%d :: App: Freeing the App Allocated Buffers in TestApp\n",__LINE__);
        for(i=0; i < numInputBuffers; i++) {
            pInputBuffer[i] = pInputBuffer[i] - 128;
            APP_MEMPRINT("%d :: App: [TESTAPPFREE] pInputBuffer[%d] = %p\n",__LINE__,i,pInputBuffer[i]);
            if(pInputBuffer[i] != NULL){
                free(pInputBuffer[i]);
                pInputBuffer[i] = NULL;
            }
        }

        for(i=0; i < numOutputBuffers; i++) {
            pOutputBuffer[i] = pOutputBuffer[i] - 128;
            APP_MEMPRINT("%d :: App: [TESTAPPFREE] pOutputBuffer[%d] = %p\n",__LINE__,i, pOutputBuffer[i]);
            if(pOutputBuffer[i] != NULL){
                free(pOutputBuffer[i]);
                pOutputBuffer[i] = NULL;
            }
        }
#endif
    
        error = WaitForState(pHandle, OMX_StateLoaded);
#ifdef OMX_GETTIME
        GT_END("Call to SendCommand <OMX_StateLoaded After freeing input/output BUFFERS & OMX_CommandPortDisable>");
#endif
    
        /* Unload the G711 Encoder Component */
        printf ("Free the Component handle\n");
        error = TIOMX_FreeHandle(pHandle);
        APP_MEMPRINT("%d:::[TESTAPPFREE] %p\n",__LINE__,pG711Param);
        free(pG711Param);
        APP_MEMPRINT("%d:::[TESTAPPFREE] %p\n",__LINE__,pCompPrivateStruct->format.audio.cMIMEType);
        free(pCompPrivateStruct->format.audio.cMIMEType);
        APP_MEMPRINT("%d:::[TESTAPPFREE] %p\n",__LINE__,pCompPrivateStruct);
        free(pCompPrivateStruct);
        if( (error != OMX_ErrorNone)) {
            APP_DPRINT ("%d:: Error in Free Handle function\n",__LINE__);
            goto EXIT;
        }

        close(IpBuf_Pipe[0]);
        close(IpBuf_Pipe[1]);
        close(OpBuf_Pipe[0]);
        close(OpBuf_Pipe[1]);
        
        close(Event_Pipe[0]);
        close(Event_Pipe[1]);
        APP_DPRINT ("%d Free Handle returned Successfully\n",__LINE__);

        fclose(inputToSN);
        APP_DPRINT ("%d:: Free Handle returned Successfully \n\n\n\n",__LINE__);

    } /* For loop on testcnt1 ends here */

 EXIT:
         
#ifdef OMX_GETTIME
    GT_END("G711_DEC test <End>");
    OMX_ListDestroy(pListHead);   
#endif
             
    error = TIOMX_Deinit();
    if( (error != OMX_ErrorNone)) {
        APP_DPRINT("APP: Error in Deinit Core function\n");
        goto EXIT;
    }
    return error;
}

/***************************************************
 * 
 *   Send_input_Buffer send input buffer to OMX_COMPONENT
 *
 *****************************************************/
OMX_ERRORTYPE send_input_buffer(OMX_HANDLETYPE pHandle, OMX_BUFFERHEADERTYPE* pBuffer, FILE *fIn)
{
    OMX_ERRORTYPE error = OMX_ErrorNone;
    OMX_S16 nRead = fill_data (pBuffer, fIn);
    OMX_S16 i = 0;
    APP_DPRINT ("%%%%%%%%%%%%%%%%%%%%%%%%%\n");
    APP_DPRINT ("%d :: pBuffer = %p nRead = %d\n",__LINE__,pBuffer,nRead);
    APP_DPRINT ("%%%%%%%%%%%%%%%%%%%%%%%%%\n");

    if((nRead < numRead) && (done == 0)) {
        fprintf(stderr, "Shutting down Since last buffer was sent---------- \n");
#ifdef OMX_GETTIME
        GT_START();
#endif
        error = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateIdle, NULL);
        if(error != OMX_ErrorNone) {
            fprintf (stderr,"Error from SendCommand-Idle(Stop) State function\n");
            goto EXIT;
        }
        WaitForState(pHandle, OMX_StateIdle);               
#ifdef OMX_GETTIME
        GT_END("Call to SendCommand <OMX_StateIdle>");
#endif
        done = 1;
        pBuffer->nFlags = OMX_BUFFERFLAG_EOS;
        printf("%i %s - EOS Has been ocurred \n",__LINE__,__FUNCTION__);
    }

    else {
        pBuffer->nFilledLen = nRead;
        for (i=0; i < nRead; i++) {
            fprintf(inputToSN,"pBuffer->pBuffer[%d] = %x\n",i,pBuffer->pBuffer[i]);
        }
        pBuffer->nTimeStamp = rand()%100;
        pBuffer->nTickCount = rand() % 70;
        TIME_PRINT("TimeStamp Input: %lld\n",pBuffer->nTimeStamp);
        TICK_PRINT("TickCount Input: %ld\n",pBuffer->nTickCount);
        error = OMX_EmptyThisBuffer(pHandle, pBuffer);
        if (error == OMX_ErrorIncorrectStateOperation) error = 0;
    } 

 EXIT:
    return error;
}


OMX_S16 fill_data (OMX_BUFFERHEADERTYPE *pBuf, FILE *fIn)
{
    OMX_S16 nRead = 0;
    static OMX_S16 totalRead = 0;
    static OMX_S16 fileHdrReadFlag = 0;

    if (!fileHdrReadFlag) {
        fprintf (stderr, "Reading the file\n");
        fileHdrReadFlag = 1;
    }

    if (testCaseNo == 7) { /* Multiframe with each buffer size = 2* framelenght */
        numRead = 2*pBuf->nAllocLen;
    }
    else if (testCaseNo == 8) { /* Multiframe with each buffer size = 2/framelenght */
        numRead = pBuf->nAllocLen/2;
    }
    else if (testCaseNo == 9) { /* Multiframe with alternating buffer size */
        if (alternate == 0) {
            numRead = 2*pBuf->nAllocLen;
            alternate = 1;
        }
        else {
            numRead = pBuf->nAllocLen/2;
            alternate = 0;
        }
    } 
    else {
        numRead = pBuf->nAllocLen;
    }
    nRead = fread(pBuf->pBuffer, 1, numRead , fIn);
    totalRead += nRead;

    pBuf->nFilledLen = nRead;
    return nRead;
}

void ConfigureAudio()
{
    int Mixer = 0, arg = 0, status = 0;

    Mixer = open("/dev/sound/mixer", O_WRONLY);
    if (Mixer < 0) {
        perror("open of /dev/sound/mixer failed");
        exit(1);
    }

    arg = G711DEC_SAMPLING_FREQUENCY;      /* sampling rate */
    printf("Sampling freq set to:%d\n",arg);
    status = ioctl(Mixer, SOUND_PCM_WRITE_RATE, &arg);
    if (status == -1) {
        perror("SOUND_PCM_WRITE_RATE ioctl failed");
        printf("sample rate set to %u\n", arg);
    }
    arg = AFMT_S16_LE;          /* AFMT_S16_LE or AFMT_S32_LE */
    status = ioctl(Mixer, SOUND_PCM_SETFMT, &arg);
    if (status == -1) {
        perror("SOUND_PCM_SETFMT ioctl failed");
        printf("Bitsize set to %u\n", arg);
    }
    arg = 2;            /* Channels mono 1 stereo 2 */
    status = ioctl(Mixer, SOUND_PCM_WRITE_CHANNELS, &arg);
    if (status == -1) {
        perror("SOUND_PCM_WRITE_CHANNELS ioctl failed");
        printf("Channels set to %u\n", arg);
    }
    /* MIN 0 MAX 100 */

    arg = GAIN<<8|GAIN;
    status = ioctl(Mixer, SOUND_MIXER_WRITE_VOLUME, &arg);
    if (status == -1) {
        perror("SOUND_MIXER_WRITE_VOLUME ioctl failed");
        printf("Volume set to %u\n", arg);
    }
}

#ifdef APP_MEMDEBUG
void * mymalloc(int line, char *s, int size)
{
    void *p;    
    int e=0;
    p = malloc(size);
    if(p==NULL){
        printf("Memory not available\n");
        exit(1);
    }
    else{
        while((lines[e]!=0)&& (e<500) ){
            e++;
        }
        arr[e]=p;
        lines[e]=line;
        bytes[e]=size;
        strcpy(file[e],s);
        printf("Allocating %d bytes on address %p, line %d file %s pos %d\n", size, p, line, s, e);
        return p;
    }
}

int myfree(void *dp, int line, char *s)
{
    int q = 0;
    for(q=0;q<500;q++){
        if(arr[q]==dp){
            printf("Deleting %d bytes on address %p, line %d file %s\n", bytes[q],dp, line, s);
            free(dp);
            dp = NULL;
            lines[q]=0;
            strcpy(file[q],"");
            break;
        }            
    }    
    if(500==q)
        printf("\n\nPointer not found. Line:%d    File%s!!\n\n",line, s);

    return 0;
}
#endif
