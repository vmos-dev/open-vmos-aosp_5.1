
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
 * @file G729Enc_Test.c
 *
 * This file implements G729 Encoder Component Test Application to verify
 * which is fully compliant with the Khronos OpenMAX (TM) 1.0 Specification
 *
 * @path  $(CSLPATH)\OMAPSW_MPU\linux\audio\src\openmax_il\g729_enc\tests
 *
 * @rev  1.0
 */
/* ----------------------------------------------------------------------------
 *!
 *! Revision History
 *! ===================================
 *! 21-sept-2006 bk: updated review findings for alpha release
 *! 24-Aug-2006 bk: Khronos OpenMAX (TM) 1.0 Conformance tests some more
 *! 18-July-2006 bk: Khronos OpenMAX (TM) 1.0 Conformance tests validated for few cases
 *! 21-Jun-2006 bk: Khronos OpenMAX (TM) 1.0 migration done
 *! 22-May-2006 bk: DASF recording quality improved
 *! 19-Apr-2006 bk: DASF recording speed issue resloved
 *! 23-Feb-2006 bk: DASF functionality added
 *! 18-Jan-2006 bk: Repated recording issue fixed and LCML changes taken care
 *! 14-Dec-2005 bk: Initial Version
 *! 16-Nov-2005 bk: Initial Version
 *! 23-Sept-2005 bk: Initial Version
 *! 10-Sept-2005 bk: Initial Version
 *! 10-Sept-2005 bk:
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
#include <OMX_Component.h>
#include <TIDspOmx.h>


#ifdef OMX_GETTIME
#include <OMX_Common_Utils.h>
#include <OMX_GetTime.h>     /*Headers for Performance & measuremet    */
#endif

FILE *fpRes = NULL;

/* ======================================================================= */
/**
 * @def G729ENC_INPUT_BUFFER_SIZE        Default input buffer size
 *              G729ENC_INPUT_BUFFER_SIZE_DASF  Default input buffer size DASF
 */
/* ======================================================================= */
#define G729APP_INPUT_BUFFER_SIZE 160
#define G729APP_INPUT_BUFFER_SIZE_DASF 160
/* ======================================================================= */
/**
 * @def G729ENC_OUTPUT_BUFFER_SIZE   Default output buffer size
 */
/* ======================================================================= */
#define G729APP_OUTPUT_BUFFER_SIZE 66

/* ======================================================================= */
/*
 * @def G729ENC_APP_ID  App ID Value setting
 */
/* ======================================================================= */
#define G729ENC_APP_ID 100
#define SLEEP_TIME 5
#define G729ENC_HEADER_LEN 2
#define G729_NO_TX_FRAME_TYPE   0                /* No Data: 0 bytes */
#define G729_SPEECH_FRAME_TYPE  1                /* Speech: 10 bytes */
#define G729_SID_FRAME_TYPE             2                /* SID: 2 bytes */
#define G729_ERASURE_FRAME_TYPE 3                /* Erasure frame */
#define BIT_0 0x007f
#define BIT_1 0x0081
#define FIFO1 "/dev/fifo.1"
#define FIFO2 "/dev/fifo.2"
#undef APP_DEBUG
#undef APP_MEMCHECK
#undef USE_BUFFER
#define STRESS_TEST_ITERATIONS 20

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

#ifdef OMX_GETTIME
OMX_ERRORTYPE eError = OMX_ErrorNone;
int GT_FlagE = 0;  /* Fill Buffer 1 = First Buffer,  0 = Not First Buffer  */
int GT_FlagF = 0;  /*Empty Buffer  1 = First Buffer,  0 = Not First Buffer  */
static OMX_NODE* pListHead = NULL;
#endif


typedef struct AUDIO_INFO {
    OMX_U32 efrMode;
    OMX_U32 g729Mode;
    OMX_U32 acdnMode;
    OMX_U32 dasfMode;
    OMX_U32 mimeMode;
    OMX_U32 nIpBufs;
    OMX_U32 nOpBufs;
    OMX_U32 nMFrameMode;
} AUDIO_INFO;

/* ======================================================================= */
/**
 *  M A C R O S FOR MALLOC and MEMORY FREE and CLOSING PIPES
 */
/* ======================================================================= */

#define OMX_G729APP_CONF_INIT_STRUCT(_s_, _name_)   \
    memset((_s_), 0x0, sizeof(_name_));             \
    (_s_)->nSize = sizeof(_name_);                  \
    (_s_)->nVersion.s.nVersionMajor = 0x1;          \
    (_s_)->nVersion.s.nVersionMinor = 0x1;          \
    (_s_)->nVersion.s.nRevision = 0x0;              \
    (_s_)->nVersion.s.nStep = 0x0

#define OMX_G729APP_INIT_STRUCT(_s_, _name_)    \
    memset((_s_), 0x0, sizeof(_name_)); \

#define OMX_G729APP_MALLOC_STRUCT(_pStruct_, _sName_)                   \
    _pStruct_ = (_sName_*)malloc(sizeof(_sName_));                      \
    if(_pStruct_ == NULL){                                              \
        printf("***********************************\n");                \
        printf("%d :: Malloc Failed\n",__LINE__);                       \
        printf("***********************************\n");                \
        eError = OMX_ErrorInsufficientResources;                        \
        goto EXIT;                                                      \
    }                                                                   \
    APP_MEMPRINT("%d :: ALLOCATING MEMORY = %p\n",__LINE__,_pStruct_);

/* ======================================================================= */
/** G729APP_COMP_PORT_TYPE  Port types
 *
 *  @param  G729APP_INPUT_PORT                  Input port
 *
 *  @param  G729APP_OUTPUT_PORT                 Output port
 */
/*  ====================================================================== */
/*This enum must not be changed. */
typedef enum G729APP_COMP_PORT_TYPE {
    G729APP_INPUT_PORT = 0,
    G729APP_OUTPUT_PORT
}G729APP_COMP_PORT_TYPE;

/* ======================================================================= */
/**
 * @def G729APP_MAX_NUM_OF_BUFS         Maximum number of buffers
 * @def G729APP_NUM_OF_CHANNELS                 Number of Channels
 * @def G729APP_SAMPLING_FREQUENCY    Sampling frequency
 */
/* ======================================================================= */
#define G729APP_MAX_NUM_OF_BUFS 10
#define G729APP_NUM_OF_CHANNELS 1
#define G729APP_SAMPLING_FREQUENCY 8000

int maxint(int a, int b);

int inputPortDisabled = 0;
int outputPortDisabled = 0;
OMX_STRING strG729Encoder = "OMX.TI.G729.encode";
int IpBuf_Pipe[2] = {0};
int OpBuf_Pipe[2] = {0};
fd_set rfds;
int done = 0;
int doneWriting = 0;
int tcID = 0;
OMX_BOOL bExitOnError = OMX_TRUE;
int ebd = 0;
int fbd = 0;


void writeITUFormat(OMX_U8* buffer, OMX_U32 length, FILE* fOut) ;

#ifdef USE_BUFFER
OMX_ERRORTYPE FreeResources(OMX_AUDIO_PARAM_G729TYPE* pG729Param,
                            OMX_PARAM_PORTDEFINITIONTYPE* pCompPrivateStruct,
                            AUDIO_INFO* audioinfo,
                            OMX_U8* pInputBuffer[G729APP_MAX_NUM_OF_BUFS],
                            OMX_U8* pOutputBuffer[G729APP_MAX_NUM_OF_BUFS]);
#else
OMX_ERRORTYPE FreeResources(OMX_AUDIO_PARAM_G729TYPE* pG729Param,
                            OMX_PARAM_PORTDEFINITIONTYPE* pCompPrivateStruct,
                            AUDIO_INFO* audioinfo,
                            OMX_BUFFERHEADERTYPE* pInputBufferHeader[G729APP_MAX_NUM_OF_BUFS],
                            OMX_BUFFERHEADERTYPE* pOutputBufferHeader[G729APP_MAX_NUM_OF_BUFS],
                            OMX_HANDLETYPE pHandle);
#endif

/* safe routine to get the maximum of 2 integers */
int maxint(int a, int b)
{
    return (a>b) ? a : b;
}

/* This method will wait for the component to get to the state
 * specified by the DesiredState input. */
static OMX_ERRORTYPE WaitForState(OMX_HANDLETYPE* pHandle,
                                  OMX_STATETYPE DesiredState)
{
    OMX_STATETYPE CurState = OMX_StateInvalid;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    int nCnt = 0;
    OMX_COMPONENTTYPE *pComponent = (OMX_COMPONENTTYPE *)pHandle;

    eError = pComponent->GetState(pHandle, &CurState);
    if(eError != OMX_ErrorNone) {
        APP_DPRINT("%d :: App: Error returned from GetState\n",__LINE__);
        goto EXIT;
    }
    while( (eError == OMX_ErrorNone) && (CurState != DesiredState) ) {
        sleep(2);
        if(nCnt++ == 10) {
            APP_DPRINT("%d :: Still Waiting, press CTL-C to continue\n",__LINE__);
        }
        eError = pComponent->GetState(pHandle, &CurState);
        if(eError != OMX_ErrorNone) {
            APP_DPRINT("%d :: App: Error returned from GetState\n",__LINE__);
            goto EXIT;
        }
    }
 EXIT:
    return eError;
}

OMX_ERRORTYPE EventHandler(
                           OMX_HANDLETYPE hComponent,
                           OMX_PTR pAppData,
                           OMX_EVENTTYPE eEvent,
                           OMX_U32 nData1,
                           OMX_U32 nData2,
                           OMX_PTR pEventData)
{
    APP_DPRINT( "%d :: App: Entering EventHandler \n", __LINE__);
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pComponent = (OMX_COMPONENTTYPE *)hComponent;
    OMX_STATETYPE state = OMX_StateInvalid;

    eError = pComponent->GetState (hComponent, &state);
    if(eError != OMX_ErrorNone) {
        APP_DPRINT("%d :: App: Error returned from GetState\n",__LINE__);
        goto EXIT;
    }
    APP_DPRINT( "%d :: App: Component eEvent = %d\n", __LINE__,eEvent);
    switch (eEvent) {
        APP_DPRINT( "%d :: App: Component State Changed To %d\n", __LINE__,state);
    case OMX_EventCmdComplete:
        APP_DPRINT( "%d :: App: Component State Changed To %d\n", __LINE__,state);
        if (nData1 == OMX_CommandPortDisable) {
            if (nData2 == G729APP_INPUT_PORT) {
                inputPortDisabled = 1;
            }
            if (nData2 == G729APP_OUTPUT_PORT) {
                outputPortDisabled = 1;
            }
        }
        break;
    case OMX_EventError:
        break;
    case OMX_EventMax:
        APP_DPRINT( "%d :: App: Component OMX_EventMax = %d\n", __LINE__,eEvent);
        break;
    case OMX_EventMark:
        APP_DPRINT( "%d :: App: Component OMX_EventMark = %d\n", __LINE__,eEvent);
        break;
    case OMX_EventPortSettingsChanged:
        APP_DPRINT( "%d :: App: Component OMX_EventPortSettingsChanged = %d\n", __LINE__,eEvent);
        break;
    case OMX_EventBufferFlag:
        APP_DPRINT( "%d :: App: Component OMX_EventBufferFlag = %d\n", __LINE__,eEvent);
        break;
    case OMX_EventResourcesAcquired:
        APP_DPRINT( "%d :: App: Component OMX_EventResourcesAcquired = %d\n", __LINE__,eEvent);
        break;
    default:
        break;
    }
 EXIT:
    APP_DPRINT( "%d :: App: Exiting EventHandler \n", __LINE__);
    return eError;
}

void FillBufferDone (OMX_HANDLETYPE hComponent, OMX_PTR ptr, OMX_BUFFERHEADERTYPE* pBuffer)
{
    write(OpBuf_Pipe[1], &pBuffer, sizeof(pBuffer));
    fbd++;
#ifdef OMX_GETTIME
    if (GT_FlagF == 1 ) /* First Buffer Reply*/  /* 1 = First Buffer,  0 = Not First Buffer  */
    {
        GT_END("Call to FillBufferDone  <First: FillBufferDone>");
        GT_FlagF = 0 ;   /* 1 = First Buffer,  0 = Not First Buffer  */
    }
#endif
}

void EmptyBufferDone(OMX_HANDLETYPE hComponent, OMX_PTR ptr, OMX_BUFFERHEADERTYPE* pBuffer)
{
    write(IpBuf_Pipe[1], &pBuffer, sizeof(pBuffer));
    ebd++;
#ifdef OMX_GETTIME
    if (GT_FlagE == 1 ) /* First Buffer Reply*/  /* 1 = First Buffer,  0 = Not First Buffer  */
    {
        GT_END("Call to EmptyBufferDone <First: EmptyBufferDone>");
        GT_FlagE = 0;   /* 1 = First Buffer,  0 = Not First Buffer  */
    }
#endif
}

int main(int argc, char* argv[])
{
    OMX_CALLBACKTYPE G729CaBa = {(void *)EventHandler,
                                 (void*)EmptyBufferDone,
                                 (void*)FillBufferDone};
    OMX_HANDLETYPE pHandle;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 AppData = G729ENC_APP_ID;
    OMX_PARAM_PORTDEFINITIONTYPE* pCompPrivateStruct = NULL;
    OMX_AUDIO_PARAM_G729TYPE *pG729Param = NULL;
    OMX_COMPONENTTYPE *pComponent = NULL;
    OMX_STATETYPE state = OMX_StateInvalid;
    OMX_BUFFERHEADERTYPE* pInputBufferHeader[G729APP_MAX_NUM_OF_BUFS] = {NULL};
    OMX_BUFFERHEADERTYPE* pOutputBufferHeader[G729APP_MAX_NUM_OF_BUFS] = {NULL};
#ifdef USE_BUFFER
    OMX_U8* pInputBuffer[G729APP_MAX_NUM_OF_BUFS] = {NULL};
    OMX_U8* pOutputBuffer[G729APP_MAX_NUM_OF_BUFS] = {NULL};
#endif
    AUDIO_INFO* audioinfo = NULL;
    TI_OMX_DSP_DEFINITION tiOmxDspDefinition;
    FILE* fIn = NULL;
    FILE* fOut = NULL;
    struct timeval tv;
    int retval = 0, i = 0, j = 0, k = 0, kk = 0;
    int frmCount = 0;
    int frmCnt = 1;
    int testcnt = 0;
    int testcnt1 = 0;
    int status = 0;
    int fdmax = 0;
    int nRead = 0;
    int nFrameCount = 1;
    int nFrameLen = 0;
    int nIpBuff = 1;
    int nOutBuff = 1;
    OMX_INDEXTYPE index = 0;
    int frameLengthInBytes = 0;
    int frameType = 0;
    int framesPerBuffer=1;
    TI_OMX_DATAPATH dataPath;
    int g729encfdwrite = 0;
    int g729encfdread = 0;

    printf("------------------------------------------------------\n");
    printf("This is Main Thread In G729 ENCODER Test Application:\n");
    printf("Test Core 1.5 - " __DATE__ ":" __TIME__ "\n");
    printf("------------------------------------------------------\n");

#ifdef OMX_GETTIME
    GTeError = OMX_ListCreate(&pListHead);
    printf("eError = %d\n",GTeError);
    GT_START();
#endif

    bExitOnError = OMX_FALSE;

#ifdef DSP_RENDERING_ON
    if((g729encfdwrite=open(FIFO1,O_WRONLY))<0) {
        printf("[G729TEST] - failure to open WRITE pipe\n");
    }
    else {
        printf("[G729TEST] - opened WRITE pipe\n");
    }

    if((g729encfdread=open(FIFO2,O_RDONLY))<0) {
        printf("[G729TEST] - failure to open READ pipe\n");
        goto EXIT;
    }
    else {
        printf("[G729TEST] - opened READ pipe\n");
    }
#endif

    /* check the input parameters */
    if(argc != 11) {
        printf("%d :: Usage: [TestApp] [O/P] [FUNC_ID_X] [FM/DM]   [DTXON/OFF] [ACDNON/OFF] [FRAMES] [1 to N] [1 to N] [MFON]\n",__LINE__);
        goto EXIT;
    }
    /* check to see that the input file exists */
    struct stat sb = {0};
    status = stat(argv[1], &sb);
    if( status != 0 ) {
        APP_DPRINT("Cannot find file %s. (%u)\n", argv[1], errno);
        goto EXIT;
    }
    /* Open the file of data to be encoded. */
    fIn = fopen(argv[1], "r");
    if( fIn == NULL ) {

        APP_DPRINT("Error:  failed to open the input file %s\n", argv[1]);
        goto EXIT;
    }
    /* Open the file of data to be written. */
    fOut = fopen(argv[2], "w");
    if( fOut == NULL ) {

        APP_DPRINT("Error:  failed to open the output file %s\n", argv[2]);
        goto EXIT;
    }

    if(!strcmp(argv[3],"FUNC_ID_0")) {

        printf("%d :: ### Testing TESTCASE 1 PLAY TILL END ###\n",__LINE__);
        testcnt = 1;
        testcnt1 = 1;
        tcID = 0;
    }
    else if(!strcmp(argv[3],"FUNC_ID_1")) {

        printf("%d :: ### Testing TESTCASE 1 PLAY TILL END ###\n",__LINE__);
        testcnt = 1;
        testcnt1 = 1;
        tcID = 1;
    }
    else if(!strcmp(argv[3],"FUNC_ID_2")) {
        printf("%d :: ### Testing TESTCASE 2 STOP IN THE END ###\n",__LINE__);
        testcnt = 1;
        testcnt1 = 1;
        tcID = 2;
    }
    else if(!strcmp(argv[3],"FUNC_ID_3")) {
        printf("%d :: ### Testing TESTCASE 3 PAUSE - RESUME IN BETWEEN ###\n",__LINE__);
        testcnt = 1;
        testcnt1 = 1;
        tcID = 3;
    }
    else if(!strcmp(argv[3],"FUNC_ID_4")) {
        printf("%d :: ### Testing TESTCASE 4 STOP IN BETWEEN ###\n",__LINE__);
        testcnt = 2;
        testcnt1 = 1;
        tcID = 4;
        printf("######## testcnt = %d #########\n",testcnt);
    }
    if(!strcmp(argv[3],"FUNC_ID_5")){
        printf("%d :: ### Testing TESTCASE 5 ENCODE without Deleting component Here ###\n",__LINE__);
        testcnt = STRESS_TEST_ITERATIONS;
        testcnt1 = 1;
        tcID = 5;
    }
    if(!strcmp(argv[3],"FUNC_ID_6")) {
        printf("%d :: ### Testing TESTCASE 6 ENCODE with Deleting component Here ###\n",__LINE__);
        testcnt = 1;
        testcnt1 = STRESS_TEST_ITERATIONS;
        tcID = 6;
    }
    eError = TIOMX_Init();
    if(eError != OMX_ErrorNone) {
        APP_DPRINT("%d :: Error returned by OMX_Init()\n",__LINE__);
        goto EXIT;
    }

    for(j = 0; j < testcnt1; j++) {


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
        /* save off the "max" of the handles for the selct statement */
        fdmax = maxint(IpBuf_Pipe[0], OpBuf_Pipe[0]);
        
        if(j > 0) {
            printf ("%d :: Encoding the file for %d Time in TESTCASE 6\n",__LINE__,j+1);
            fIn = fopen(argv[1], "r");
            if( fIn == NULL ) {
                fprintf(stderr, "Error:  failed to open the file %s for read only access\n",argv[1]);
                goto EXIT;
            }
            fOut = fopen("TC6_G7291.g729", "w");
            if( fOut == NULL ) {
                fprintf(stderr, "Error:  failed to create the output file %s\n",argv[2]);
                goto EXIT;
            }
        }
        /* Load the G729 Encoder Component */
#ifdef OMX_GETTIME
        GT_START();
        eError = OMX_GetHandle(&pHandle, strG729Encoder, &AppData, &G729CaBa);
        GT_END("Call to GetHandle");
#else
        eError = TIOMX_GetHandle(&pHandle, strG729Encoder, &AppData, &G729CaBa);
#endif
        if((eError != OMX_ErrorNone) || (pHandle == NULL)) {
            APP_DPRINT("Error in Get Handle function\n");
            goto EXIT;
        }
        OMX_G729APP_MALLOC_STRUCT(audioinfo, AUDIO_INFO);
        OMX_G729APP_INIT_STRUCT(audioinfo, AUDIO_INFO);
        /* Setting No.Of Input and Output Buffers for the Component */
        if(atoi(argv[8]) <1 || atoi(argv[8]) > 4)
        {
            printf("Please enter a valid number of buffers between 1 and 4\n");
            bExitOnError = 1;
            goto EXIT;
        }
        else
        {
            audioinfo->nIpBufs = atoi(argv[8]);
            APP_DPRINT("\n%d :: App: audioinfo->nIpBufs = %ld \n",__LINE__,audioinfo->nIpBufs);
        }

        if(atoi(argv[9]) <1 || atoi(argv[9]) > 4)
        {
            printf("Please enter a valid number of buffers between 1 and 4\n");
            bExitOnError = 1;
            goto EXIT;
        }
        else
        {
            audioinfo->nOpBufs = atoi(argv[9]);
            APP_DPRINT("\n%d :: App: audioinfo->nOpBufs = %ld \n",__LINE__,audioinfo->nOpBufs);
        }
        if(atoi(argv[10]) <1 || atoi(argv[10]) > 8)
        {
            printf("Please enter a valid number of frames per buffer between 1 and 8\n");
            bExitOnError = 1;
            goto EXIT;
        }
        else
        {
            framesPerBuffer = atoi(argv[10]);
        }
        OMX_G729APP_MALLOC_STRUCT(pCompPrivateStruct, OMX_PARAM_PORTDEFINITIONTYPE);
        OMX_G729APP_CONF_INIT_STRUCT(pCompPrivateStruct, OMX_PARAM_PORTDEFINITIONTYPE);
        OMX_G729APP_MALLOC_STRUCT(pG729Param, OMX_AUDIO_PARAM_G729TYPE);
        OMX_G729APP_CONF_INIT_STRUCT(pG729Param, OMX_AUDIO_PARAM_G729TYPE);
        APP_DPRINT("%d :: Setting input port config\n",__LINE__);
        pCompPrivateStruct->nSize                                                          = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
        pCompPrivateStruct->nVersion.s.nVersionMajor           = 0xF1;
        pCompPrivateStruct->nVersion.s.nVersionMinor               = 0xF2;
        pCompPrivateStruct->nPortIndex                         = G729APP_INPUT_PORT;
        pCompPrivateStruct->eDir                               = OMX_DirInput;
        pCompPrivateStruct->nBufferCountActual                 = audioinfo->nIpBufs;
        pCompPrivateStruct->nBufferCountMin                    = audioinfo->nIpBufs;
        pCompPrivateStruct->nBufferSize                        = G729APP_INPUT_BUFFER_SIZE*framesPerBuffer;
        pCompPrivateStruct->bEnabled                           = OMX_TRUE;
        pCompPrivateStruct->bPopulated                         = OMX_FALSE;
        pCompPrivateStruct->eDomain                            = OMX_PortDomainAudio;
        pCompPrivateStruct->format.audio.eEncoding             = OMX_AUDIO_CodingG729;
        pCompPrivateStruct->format.audio.cMIMEType             = NULL;
        pCompPrivateStruct->format.audio.pNativeRender         = NULL;
        pCompPrivateStruct->format.audio.bFlagErrorConcealment = OMX_FALSE;    /*Send input port config*/
        APP_DPRINT("%d :: Setting input port config\n",__LINE__);
        if(!(strcmp(argv[4],"FM"))) {
            audioinfo->dasfMode = 0;
            tiOmxDspDefinition.dasfMode = OMX_FALSE;
            APP_DPRINT("\n%d :: App: audioinfo->dasfMode = %ld \n",__LINE__,audioinfo->dasfMode);
        }
        else if(!(strcmp(argv[4],"DM"))){
            audioinfo->dasfMode = 1;
            tiOmxDspDefinition.dasfMode = OMX_TRUE;
            APP_DPRINT("\n%d :: App: audioinfo->dasfMode = %ld \n",__LINE__,audioinfo->dasfMode);
            APP_DPRINT("%d :: G729 ENCODER RUNNING UNDER DASF MODE \n",__LINE__);
            pCompPrivateStruct->nBufferCountActual = 0;
        }
        else {
            eError = OMX_ErrorBadParameter;
            printf("\n%d :: App: audioinfo->dasfMode Sending Bad Parameter\n",__LINE__);
            printf("%d :: App: Should Be One of these Modes FM, DM\n",__LINE__);
            bExitOnError = OMX_TRUE;
            goto EXIT;
        }
        if(audioinfo->dasfMode == 0) {
            if((atoi(argv[7])) != 0) {
                eError = OMX_ErrorBadParameter;
                printf("\n%d :: App: No. of Frames Sending Bad Parameter\n",__LINE__);
                printf("%d :: App: For FILE mode argv[7] Should Be --> 0\n",__LINE__);
                printf("%d :: App: For DASF mode argv[7] Should be greater than zero depends on number of frames user want to encode\n",__LINE__);
                bExitOnError = OMX_TRUE;
                goto EXIT;
            }
        }
        else {
            if((atoi(argv[7])) == 0) {
                eError = OMX_ErrorBadParameter;
                printf("\n%d :: App: No. of Frames Sending Bad Parameter\n",__LINE__);
                printf("%d :: App: For DASF mode argv[7] Should be greater than zero depends on number of frames user want to encode\n",__LINE__);
                printf("%d :: App: For FILE mode argv[7] Should Be --> 0\n",__LINE__);
                bExitOnError = OMX_TRUE;
                goto EXIT;
            }
        }
        if(!(strcmp(argv[6],"ACDNOFF"))) {
            audioinfo->acdnMode = 0;
            tiOmxDspDefinition.acousticMode = OMX_FALSE;
            APP_DPRINT("\n%d :: App: audioinfo->acdnMode = %ld \n",__LINE__,audioinfo->acdnMode);
        }
        else if(!(strcmp(argv[6],"ACDNON"))) {
            audioinfo->acdnMode = 1;
            tiOmxDspDefinition.acousticMode = OMX_TRUE;
            APP_DPRINT("\n%d :: App: audioinfo->acdnMode = %ld \n",__LINE__,audioinfo->acdnMode);
        }
        else {
            eError = OMX_ErrorBadParameter;
            printf("\n%d :: App: audioinfo->acdnMode Sending Bad Parameter\n",__LINE__);
            printf("%d :: App: Should Be One of these Modes ACDNON, ACDNOFF\n",__LINE__);
            bExitOnError = OMX_TRUE;
            goto EXIT;
        }
#ifdef OMX_GETTIME
        GT_START();
        eError = OMX_SetParameter (pHandle, OMX_IndexParamPortDefinition, pCompPrivateStruct);
        GT_END("Set Parameter Test-SetParameter");
#else
        eError = OMX_SetParameter (pHandle, OMX_IndexParamPortDefinition, pCompPrivateStruct);
#endif
        if (eError != OMX_ErrorNone) {
            eError = OMX_ErrorBadParameter;
            APP_DPRINT("%d :: OMX_ErrorBadParameter\n",__LINE__);
            bExitOnError = OMX_TRUE;
            goto EXIT;
        }
        APP_MEMPRINT("%d :: Setting output port config\n",__LINE__);
        pCompPrivateStruct->nSize                                                          = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
        pCompPrivateStruct->nVersion.s.nVersionMajor           = 0x1;
        pCompPrivateStruct->nVersion.s.nVersionMinor               = 0x1;
        pCompPrivateStruct->nPortIndex                         = G729APP_OUTPUT_PORT;
        pCompPrivateStruct->eDir                               = OMX_DirOutput;
        pCompPrivateStruct->nBufferCountActual                 = audioinfo->nOpBufs;
        pCompPrivateStruct->nBufferCountMin                    = audioinfo->nOpBufs;
        pCompPrivateStruct->nBufferSize                        = G729APP_OUTPUT_BUFFER_SIZE;
        pCompPrivateStruct->bEnabled                           = OMX_TRUE;
        pCompPrivateStruct->bPopulated                         = OMX_FALSE;
        pCompPrivateStruct->eDomain                            = OMX_PortDomainAudio;
        pCompPrivateStruct->format.audio.eEncoding             = OMX_AUDIO_CodingG729;
        pCompPrivateStruct->format.audio.cMIMEType             = NULL;
        pCompPrivateStruct->format.audio.pNativeRender         = NULL;
        pCompPrivateStruct->format.audio.bFlagErrorConcealment = OMX_FALSE;    /*Send input port config*/
#ifdef OMX_GETTIME
        GT_START();
        eError = OMX_SetParameter (pHandle, OMX_IndexParamPortDefinition, pCompPrivateStruct);
        GT_END("Set Parameter Test-SetParameter");
#else
        eError = OMX_SetParameter (pHandle, OMX_IndexParamPortDefinition, pCompPrivateStruct);
#endif
        if (eError != OMX_ErrorNone) {
            eError = OMX_ErrorBadParameter;
            APP_DPRINT("%d :: OMX_ErrorBadParameter\n",__LINE__);
            bExitOnError = OMX_TRUE;
            goto EXIT;
        }
        pG729Param->nSize                    = sizeof(OMX_AUDIO_PARAM_G729TYPE);
        pG729Param->nVersion.s.nVersionMajor = 0x1;
        pG729Param->nVersion.s.nVersionMinor = 0x1;
        pG729Param->nPortIndex               = G729APP_OUTPUT_PORT;
        pG729Param->nChannels                = G729APP_NUM_OF_CHANNELS;
        /*        APP_DPRINT("\n%d :: App: pG729Param->eG729BandMode --> %d \n",__LINE__,pG729Param->eG729BandMode); */
        if(!(strcmp(argv[5],"DTXON"))) {
            /**< G729 Discontinuous Transmission Mode is enabled  */
            /*            APP_DPRINT("\n%d :: App: pG729Param->eARMDTXMode --> %s \n",__LINE__,argv[5]); */
            pG729Param->bDTX = OMX_TRUE;
        }
        else if(!(strcmp(argv[5],"DTXOFF"))) {
            pG729Param->bDTX = OMX_FALSE;
            /**< G729 Discontinuous Transmission Mode is disabled */
        }
        else {
            eError = OMX_ErrorBadParameter;
            printf("\n%d :: App: pG729Param->eARMDTXMode Sending Bad Parameter\n",__LINE__);
            printf("%d :: App: Should Be One of these Modes DTXON, DTXOFF\n",__LINE__);
            bExitOnError = OMX_TRUE;
            goto EXIT;
        }
#ifdef OMX_GETTIME
        GT_START();
        eError = OMX_SetParameter (pHandle, OMX_IndexParamAudioG729, pG729Param);
        GT_END("Set Parameter Test-SetParameter");
#else
        eError = OMX_SetParameter (pHandle, OMX_IndexParamAudioG729, pG729Param);
#endif
        if (eError != OMX_ErrorNone) {
            eError = OMX_ErrorBadParameter;
            APP_DPRINT("%d :: OMX_ErrorBadParameter\n",__LINE__);
            bExitOnError = OMX_TRUE;
            goto EXIT;
        }

#ifndef USE_BUFFER
        APP_DPRINT("%d :: About to call OMX_AllocateBuffer\n",__LINE__);
        for(i = 0; i < audioinfo->nIpBufs; i++) {
            /* allocate input buffer */
            APP_DPRINT("%d :: About to call OMX_AllocateBuffer for pInputBufferHeader[%d]\n",__LINE__, i);
            eError = OMX_AllocateBuffer(pHandle, &pInputBufferHeader[i], 0, NULL, G729APP_INPUT_BUFFER_SIZE*framesPerBuffer);
            if(eError != OMX_ErrorNone) {
                APP_DPRINT("%d :: Error returned by OMX_AllocateBuffer for pInputBufferHeader[%d]\n",__LINE__, i);
                bExitOnError = OMX_TRUE;
                goto EXIT;
            }
        }
        APP_DPRINT("\n%d :: App: pCompPrivateStruct->nBufferSize --> %ld \n",__LINE__,pCompPrivateStruct->nBufferSize);
        for(i = 0; i < audioinfo->nOpBufs; i++) {
            /* allocate output buffer */
            APP_DPRINT("%d :: About to call OMX_AllocateBuffer for pOutputBufferHeader[%d]\n",__LINE__, i);
            eError = OMX_AllocateBuffer(pHandle, &pOutputBufferHeader[i], 1, NULL, pCompPrivateStruct->nBufferSize);
            if(eError != OMX_ErrorNone) {
                APP_DPRINT("%d :: Error returned by OMX_AllocateBuffer for pOutputBufferHeader[%d]\n",__LINE__, i);
                bExitOnError = OMX_TRUE;
                goto EXIT;
            }
        }
#else
        for(i = 0; i < audioinfo->nIpBufs; i++) {
            pInputBuffer[i] = (OMX_U8*)malloc(G729APP_INPUT_BUFFER_SIZE*framesPerBuffer);
            pInputBuffer[i] += 128;

            APP_MEMPRINT("%d :: [TESTAPP ALLOC] pInputBuffer[%d] = %p\n",__LINE__,i,pInputBuffer[i]);
            if(NULL == pInputBuffer[i]) {
                APP_DPRINT("%d :: Malloc Failed\n",__LINE__);
                eError = OMX_ErrorInsufficientResources;
                goto EXIT;
            }
            /*  allocate input buffer */
            APP_DPRINT("%d :: About to call OMX_UseBuffer\n",__LINE__);
            eError = OMX_UseBuffer(pHandle, &pInputBufferHeader[i], 0, NULL, G729APP_INPUT_BUFFER_SIZE*framesPerBuffer, pInputBuffer[i]);
            if(eError != OMX_ErrorNone) {
                APP_DPRINT("%d :: Error returned by OMX_UseBuffer()\n",__LINE__);
                bExitOnError = OMX_TRUE;
                goto EXIT;
            }
        }

        for(i = 0; i < audioinfo->nOpBufs; i++) {
            pOutputBuffer[i] = malloc (pCompPrivateStruct->nBufferSize + 256);
            APP_MEMPRINT("%d :: [TESTAPP ALLOC] pOutputBuffer[%d] = %p\n",__LINE__,i,pOutputBuffer[i]);
            if(NULL == pOutputBuffer[i]) {
                APP_DPRINT("%d :: Malloc Failed\n",__LINE__);
                eError = OMX_ErrorInsufficientResources;
                bExitOnError = OMX_TRUE;
                goto EXIT;
            }
            pOutputBuffer[i] = pOutputBuffer[i] + 128;

            /* allocate output buffer */
            APP_DPRINT("%d :: About to call OMX_UseBuffer\n",__LINE__);
            eError = OMX_UseBuffer(pHandle, &pOutputBufferHeader[i], 1, NULL, pCompPrivateStruct->nBufferSize, pOutputBuffer[i]);
            if(eError != OMX_ErrorNone) {
                APP_DPRINT("%d :: Error returned by OMX_UseBuffer()\n",__LINE__);
                bExitOnError = OMX_TRUE;
                goto EXIT;
            }
        }
#endif

        eError = OMX_GetExtensionIndex(pHandle, "OMX.TI.index.config.tispecific",&index);
        if (eError != OMX_ErrorNone) {
            APP_DPRINT("Error returned from OMX_GetExtensionIndex\n");
            bExitOnError = OMX_TRUE;
            goto EXIT;
        }

#ifdef DSP_RENDERING_ON
        cmd_data.hComponent = pHandle;
        cmd_data.AM_Cmd = AM_CommandIsInputStreamAvailable;

        cmd_data.param1 = 0;
        if((write(g729encfdwrite, &cmd_data, sizeof(cmd_data)))<0) {
        }
        if((read(g729encfdread, &cmd_data, sizeof(cmd_data)))<0) {
            bExitOnError = OMX_TRUE;
            goto EXIT;
        }
        tiOmxDspDefinition.streamId = cmd_data.streamID;
#endif

        eError = OMX_SetConfig (pHandle, index, &tiOmxDspDefinition);
        if(eError != OMX_ErrorNone) {
            eError = OMX_ErrorBadParameter;
            APP_DPRINT("%d :: Error from OMX_SetConfig() function\n",__LINE__);
            bExitOnError = OMX_TRUE;
            goto EXIT;
        }

        if (tiOmxDspDefinition.dasfMode) {
#ifdef RTM_PATH
            dataPath = DATAPATH_APPLICATION_RTMIXER;
#endif

#ifdef ETEEDN_PATH
            dataPath = DATAPATH_APPLICATION;
#endif
        }

        eError = OMX_GetExtensionIndex(pHandle, "OMX.TI.index.config.g729.datapath",&index);
        if (eError != OMX_ErrorNone) {
            printf("Error getting extension index\n");
            bExitOnError = OMX_TRUE;
            goto EXIT;
        }

        eError = OMX_SetConfig (pHandle, index, &dataPath);
        if(eError != OMX_ErrorNone) {
            eError = OMX_ErrorBadParameter;
            bExitOnError = OMX_TRUE;
            goto EXIT;
        }
#ifdef OMX_GETTIME
        GT_START();
#endif
        eError = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
        if(eError != OMX_ErrorNone) {
            APP_DPRINT("Error from SendCommand-Idle(Init) State function\n");
            bExitOnError = OMX_TRUE;
            goto EXIT;
        }
        /* Wait for startup to complete */
        eError = WaitForState(pHandle, OMX_StateIdle);
#ifdef OMX_GETTIME
        GT_END("Call to SendCommand <OMX_StateIdle>");
#endif
        if(eError != OMX_ErrorNone) {
            APP_DPRINT( "Error:  hG729Encoder->WaitForState reports an eError %X\n", eError);
            bExitOnError = OMX_TRUE;
            goto EXIT;
        }
        for(i = 0; i < testcnt; i++) {
            frmCnt = 1;
            nFrameCount = 1;
            nOutBuff = 1;
            nIpBuff  = 1;
            if(i > 0) {
                printf("%d :: Encoding the file for %d Time in TESTCASE 5 OR TESTCSE 4\n",__LINE__,i+1);
                fIn = fopen(argv[1], "r");
                if(fIn == NULL) {
                    fprintf(stderr, "Error:  failed to open the file %s for readonly access\n", argv[1]);
                    bExitOnError = OMX_TRUE;
                    goto EXIT;
                }
                fOut = fopen("TC5_G7291.g729", "w");
                if(fOut == NULL) {
                    fprintf(stderr, "Error:  failed to create the output file %s\n", argv[2]);
                    bExitOnError = OMX_TRUE;
                    goto EXIT;
                }
            }


            done = 0;
            printf("%d :: App: Sending OMX_StateExecuting Command\n",__LINE__);
            
#ifdef OMX_GETTIME
            GT_START();
#endif

            eError = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
            if(eError != OMX_ErrorNone) {
                printf("Error from SendCommand-Executing State function\n");
                bExitOnError = OMX_TRUE;
                goto EXIT;
            }
            eError = WaitForState(pHandle, OMX_StateExecuting);
            
#ifdef OMX_GETTIME
            GT_END("Call to SendCommand <OMX_StateExecuting>");
#endif

            if(eError != OMX_ErrorNone) {
                APP_DPRINT( "Error:  hG729Encoder->WaitForState reports an eError %X\n", eError);
                bExitOnError = OMX_TRUE;
                goto EXIT;
            }
            if(audioinfo->dasfMode == 1) {
                printf("%d :: App: No.of Frames Encoding = %d\n",__LINE__, atoi(argv[7]));
            }
            pComponent = (OMX_COMPONENTTYPE *)pHandle;
            if(audioinfo->dasfMode == 0) {
                for (k=0; k < audioinfo->nIpBufs; k++) {
                    OMX_BUFFERHEADERTYPE* pBuffer = pInputBufferHeader[k];
                    nRead = fread(pBuffer->pBuffer, 1, pBuffer->nAllocLen, fIn);
                    APP_DPRINT("%d :: App :: Reading InputBuffer = %d from the input file nRead = %d\n",__LINE__,nIpBuff,nRead);
                    pBuffer->nFilledLen = nRead;
                    pBuffer->nTimeStamp = (OMX_S64)rand() % 70;
                    pBuffer->nTickCount = rand() % 70;
                
                    if((nRead == 0) && (done == 0)) {
                        printf("%d :: Sending Last Input Buffer from TestApp.............. \n",__LINE__);
                        done = 1;
                        pBuffer->nFilledLen = 0;
                        pBuffer->nFlags = OMX_BUFFERFLAG_EOS;
                    }
                    APP_DPRINT("%d :: App:: Input Buffer: Calling EmptyThisBuffer: %p\n",__LINE__,pBuffer);
#ifdef OMX_GETTIME
                    if (k==0)
                    {
                        GT_FlagE=1;  /* 1 = First Buffer,  0 = Not First Buffer  */
                        GT_START(); /* Empty Bufffer */
                    }
#endif
                    OMX_EmptyThisBuffer(pHandle, pBuffer);
                    nIpBuff++;
                }
            }

            for (kk = 0; kk < audioinfo->nOpBufs; kk++) {
#ifdef OMX_GETTIME
                if (kk==0)
                {
                    GT_FlagF=1;  /* 1 = First Buffer,  0 = Not First Buffer  */
                    GT_START(); /* Fill Buffer */
                }
#endif
                OMX_FillThisBuffer(pHandle, pOutputBufferHeader[kk]);

            }
            eError = OMX_GetState(pHandle, &state);
            if(eError != OMX_ErrorNone) {
                APP_DPRINT("%d :: pComponent->GetState has returned status %X\n",__LINE__, eError);
                bExitOnError = OMX_TRUE;
                goto EXIT;
            }

            while((eError == OMX_ErrorNone) && (state != OMX_StateIdle)) {
                FD_ZERO(&rfds);
                FD_SET(IpBuf_Pipe[0], &rfds);
                FD_SET(OpBuf_Pipe[0], &rfds);
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
                    sleep(1);
                    APP_DPRINT("%d :: BasicFn App Timeout !!!!!!!!!!! \n",__LINE__);
                }

                switch (tcID) {
                case 0:
                case 1:
                case 2:
                case 5:
                case 6:
                    if(audioinfo->dasfMode == 0) {
                        if(FD_ISSET(IpBuf_Pipe[0], &rfds)) {
                            OMX_BUFFERHEADERTYPE* pBuffer = NULL;
                            read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
                            if(done == 0) {
                                nRead = fread(pBuffer->pBuffer, 1, pBuffer->nAllocLen, fIn);
                                APP_DPRINT("%d :: App :: Reading InputBuffer = %d from the input file nRead = %d\n",__LINE__,nIpBuff,nRead);
                                pBuffer->nFilledLen = nRead;
                                pBuffer->nTimeStamp = (OMX_S64)rand() % 2;
                                pBuffer->nTickCount = (OMX_S64)rand() % 70;
                                if (tcID == 0){
                                    APP_DPRINT("Input time stamp = %ld\n", (long int)pBuffer->nTimeStamp);
                                    APP_DPRINT("Input tick count = %ld\n", pBuffer->nTickCount);                        
                                }
                                if((nRead < 160) && (done == 0)) {
                                    printf("%d :: App: Sending Last Input Buffer from TestApp.............. \n",__LINE__);
                                    done = 1;
                                    pBuffer->nFilledLen = 0;
                                    pBuffer->nFlags = OMX_BUFFERFLAG_EOS;
                                }
                                APP_DPRINT("%d :: App: Input Buffer: Calling EmptyThisBuffer: %p\n",__LINE__,pBuffer);
                                OMX_EmptyThisBuffer(pHandle, pBuffer);
                                nIpBuff++;
                            }
                        }
                    }
                    else {
                        APP_DPRINT("%d :: G729 ENCODER RUNNING UNDER DASF MODE \n",__LINE__);
                        if(nFrameCount == atoi(argv[7])) {
                            printf("%d :: App: Sending Stop.........From APP \n",__LINE__);
                            printf("%d :: App: Shutting down ---------- \n",__LINE__);
#ifdef OMX_GETTIME
                            GT_START();
#endif
                            eError = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateIdle, NULL);
                            if(eError != OMX_ErrorNone) {
                                fprintf (stderr,"Error from SendCommand-Idle(Stop) State function\n");
                                bExitOnError = OMX_TRUE;
                                goto EXIT;
                            }
                            eError = WaitForState(pHandle, OMX_StateIdle);
#ifdef OMX_GETTIME
                            GT_END("Call to SendCommand <OMX_StateIdle>");
#endif
                            if(eError != OMX_ErrorNone) {
                                APP_DPRINT( "Error:  G729Encoder->WaitForState reports an error         %X\n", eError);
                                bExitOnError = OMX_TRUE;
                                goto EXIT;
                            }
                            done = 1;
                        }
                        APP_DPRINT("%d :: G729 ENCODER READING DATA FROM DASF  \n",__LINE__);
                    }
                    break;

                case 3:
                    if(audioinfo->dasfMode == 0) {
                        APP_DPRINT("%d :: G729 ENCODER RUNNING UNDER FILE 2 FILE MODE \n",__LINE__);
                        if(FD_ISSET(IpBuf_Pipe[0], &rfds)) {
                            OMX_BUFFERHEADERTYPE* pBuffer = NULL;
                            read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
                            if(done == 0) {
                                APP_DPRINT("%d :: App: Read from IpBuf_Pipe InBufHeader %p\n",__LINE__,pBuffer);
                                nRead = fread(pBuffer->pBuffer, 1, pBuffer->nAllocLen, fIn);
                                APP_DPRINT("%d :: App: Reading I/P Buffer = %d & Size = %d from IpBuf_Pipe\n",__LINE__,nFrameCount,nRead);
                                pBuffer->nFilledLen = nRead;
                                if((nRead < pBuffer->nAllocLen) && (done == 0)) {
                                    printf("%d :: App: Shutting down ---------- \n",__LINE__);
                                    done = 1;
                                    pBuffer->nFilledLen = 0;
                                    pBuffer->nFlags = OMX_BUFFERFLAG_EOS;
                                    APP_DPRINT("%d :: App: Sending Last Input Buffer from TestApp.............. \n",__LINE__);
                                    APP_DPRINT("%d :: App: Input Buffer: Calling EmptyThisBuffer: %p\n",__LINE__,pBuffer);
                                    OMX_EmptyThisBuffer(pHandle, pBuffer);
#ifdef OMX_GETTIME
                                    GT_START();
#endif
                                    eError = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
                                    if(eError != OMX_ErrorNone) {
                                        printf("Error from SendCommand-Idle(Stop) State function\n");
                                        bExitOnError = OMX_TRUE;
                                        goto EXIT;
                                    }
                                    eError = WaitForState(pHandle, OMX_StateIdle);
#ifdef OMX_GETTIME
                                    GT_END("Call to SendCommand <OMX_StateIdle>");
#endif
                                    if ( eError != OMX_ErrorNone ){
                                        printf("Error:WaitForState has timed out %d", eError);
                                        bExitOnError = OMX_TRUE;
                                        goto EXIT;
                                    }
                                }
                                else {
                                    APP_DPRINT("%d :: App: Input Buffer: Calling EmptyThisBuffer: %p\n",__LINE__,pBuffer);
                                    OMX_EmptyThisBuffer(pHandle, pBuffer);
                                }
                            }
                        }
                    }
                    else {
                        APP_DPRINT("%d :: G729 ENCODER RUNNING UNDER DASF MODE \n",__LINE__);
                        if(nFrameCount == atoi(argv[7])) {
                            printf("%d :: App: Sending Stop.........From APP \n",__LINE__);
                            printf("%d :: App: Shutting down ---------- \n",__LINE__);
#ifdef OMX_GETTIME
                            GT_START();
#endif
                            eError = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateIdle, NULL);
                            if(eError != OMX_ErrorNone) {
                                fprintf (stderr,"Error from SendCommand-Idle(Stop) State function\n");
                                bExitOnError = OMX_TRUE;
                                goto EXIT;
                            }
                            eError = WaitForState(pHandle, OMX_StateIdle);
#ifdef OMX_GETTIME
                            GT_END("Call to SendCommand <OMX_StateIdle>");
#endif
                            if(eError != OMX_ErrorNone) {
                                APP_DPRINT( "Error:  G729Encoder->WaitForState reports an error         %X\n", eError);
                                bExitOnError = OMX_TRUE;
                                goto EXIT;
                            }
                            done = 1;
                        }
                        APP_DPRINT("%d :: G729 ENCODER READING DATA FROM DASF  \n",__LINE__);
                    }
                    if (frmCount == 15) {
                        printf ("%d :: App: $$$$$ Sending Resume command to Codec $$$$$$$\n",__LINE__);
#ifdef OMX_GETTIME
                        GT_START();
#endif
                        eError = OMX_SendCommand(pHandle, OMX_CommandStateSet,OMX_StateExecuting, NULL);
                        if(eError != OMX_ErrorNone) {
                            fprintf (stderr,"Error from SendCommand-Executing State function\n");
                            bExitOnError = OMX_TRUE;
                            goto EXIT;
                        }

                        /* Wait for startup to complete */
                        eError = WaitForState(pHandle, OMX_StateExecuting);
#ifdef OMX_GETTIME
                        GT_END("Call to SendCommand <OMX_StateExecuting>");
#endif
                        if(eError != OMX_ErrorNone) {
                            fprintf(stderr, "Error:  hPcmDecoder->WaitForState reports an eError %X\n", eError);
                            bExitOnError = OMX_TRUE;
                            goto EXIT;
                        }
                    }
                    if(frmCount == 10) {
                        printf ("%d :: App: $$$$$ Sending Pause command to Codec $$$$$$\n",__LINE__);
#ifdef OMX_GETTIME
                        GT_START();
#endif
                        eError = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StatePause, NULL);
                        if(eError != OMX_ErrorNone) {
                            printf("Error from SendCommand-Pasue State function\n");
                            bExitOnError = OMX_TRUE;
                            goto EXIT;
                        }
                        /* Wait for startup to complete */
                        eError = WaitForState(pHandle, OMX_StatePause);
#ifdef OMX_GETTIME
                        GT_END("Call to SendCommand <OMX_StatePause>");
#endif
                        if(eError != OMX_ErrorNone) {
                            printf("hG729Encoder->WaitForState reports error\n");
                            bExitOnError = OMX_TRUE;
                            goto EXIT;
                        }
                    }
                    break;
                case 4:
                    if(audioinfo->dasfMode == 0) {
                        APP_DPRINT("%d :: G729 ENCODER RUNNING UNDER FILE 2 FILE MODE \n",__LINE__);
                        if( FD_ISSET(IpBuf_Pipe[0], &rfds) ) {
                            if(frmCnt > 20) {
                                OMX_BUFFERHEADERTYPE* pBuffer = NULL;
                                read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
                                printf("%d :: App: Shutting down ---------- \n",__LINE__);
                                done = 1;
                                pBuffer->nFilledLen = 0;
                                pBuffer->nFlags = OMX_BUFFERFLAG_EOS;
                                APP_DPRINT("%d :: App:: Input Buffer: Calling EmptyThisBuffer: %p\n",__LINE__,pBuffer);
                                OMX_EmptyThisBuffer(pHandle, pBuffer);
                            }
                            else {
                                OMX_BUFFERHEADERTYPE* pBuffer;
                                read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
                                if(done == 0) {
                                    APP_DPRINT("%d :: App: Read from IpBuf_Pipe InBufHeader %p\n",__LINE__,pBuffer);
                                    nRead = fread(pBuffer->pBuffer, 1, pBuffer->nAllocLen, fIn);
                                    APP_DPRINT("%d :: App: Reading I/P Buffer = %d & Size = %d from IpBuf_Pipe\n",__LINE__,nFrameCount,nRead);
                                    pBuffer->nFilledLen = nRead;
                                    if((nRead == 0) && (done == 0)) {
                                        printf("%d :: App: Shutting down ---------- \n",__LINE__);
                                        done = 1;
                                        pBuffer->nFilledLen = 0;
                                        pBuffer->nFlags = OMX_BUFFERFLAG_EOS;
                                    }
                                    APP_DPRINT("%d :: App: Input Buffer: Calling EmptyThisBuffer: %p\n",__LINE__,pBuffer);
                                    OMX_EmptyThisBuffer(pHandle, pBuffer);
                                }
                            }
                        }
                        frmCnt++;
                    } else {
                        APP_DPRINT("%d :: G729 ENCODER RUNNING UNDER DASF MODE \n",__LINE__);
                        if(nFrameCount == atoi(argv[7])) {
                            printf("%d :: App: Shutting down ---------- \n",__LINE__);
#ifdef OMX_GETTIME
                            GT_START();
#endif
                            eError = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateIdle, NULL);
                            if(eError != OMX_ErrorNone) {
                                printf("Error from SendCommand-Idle(Stop) State function\n");
                                bExitOnError = OMX_TRUE;
                                goto EXIT;
                            }
                            eError = WaitForState(pHandle, OMX_StateIdle);
#ifdef OMX_GETTIME
                            GT_END("Call to SendCommand <OMX_StateIdle>");
#endif
                            if(eError != OMX_ErrorNone) {
                                APP_DPRINT( "Error:  G729Encoder->WaitForState reports an error         %X\n", eError);
                                bExitOnError = OMX_TRUE;
                                goto EXIT;
                            }
                            done = 1;
                        }
                        if(nFrameCount == 20) {
                            printf("%d :: App: Sending Stop After %d frames \n",__LINE__,nFrameCount);
#ifdef OMX_GETTIME
                            GT_START();
#endif
                            eError = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateIdle, NULL);
                            if(eError != OMX_ErrorNone) {
                                printf("Error from SendCommand-Idle(Stop) State function\n");
                                bExitOnError = OMX_TRUE;
                                goto EXIT;
                            }

                            eError = WaitForState(pHandle, OMX_StateIdle);
#ifdef OMX_GETTIME
                            GT_END("Call to SendCommand <OMX_StateIdle>");
#endif
                            if(eError != OMX_ErrorNone) {
                                APP_DPRINT( "Error:  G729Encoder->WaitForState reports an error         %X\n", eError);
                                bExitOnError = OMX_TRUE;
                                goto EXIT;
                            }
                            done = 1;
                        }
                    }
                    break;
                default:
                    APP_DPRINT("%d :: ### Simple DEFAULT Case Here ###\n",__LINE__);
                }

                if( FD_ISSET(OpBuf_Pipe[0], &rfds) ) {
                    OMX_BUFFERHEADERTYPE* pBuf = NULL;
                    read(OpBuf_Pipe[0], &pBuf, sizeof(pBuf));
                    APP_DPRINT("%d :: App: pBuf->nFilledLen = %ld\n",__LINE__, pBuf->nFilledLen);
                    nFrameLen = pBuf->nFilledLen;
                    /* strip off the G.729 Encoder frame format */
                    int subFrameLength = 0;
                    char* temp = (char *)pBuf->pBuffer;
                    while (nFrameLen > 0) {
                        frameType = *(temp);
                        temp++;
                        switch (frameType) {
                        case 0:
                            subFrameLength = 0;
                            break;
                        case 1:
                            subFrameLength = 10;
                            break;
                        case 2:
                            subFrameLength = 2;
                            break;
                        }
                        char Header[] = {0x21, 0x6b};
                        if (!doneWriting) {
                            if (!(subFrameLength == 0 && done)) {

                                fwrite(Header, 1, G729ENC_HEADER_LEN, fOut);
                                fflush(fOut);
                                APP_DPRINT("%d :: App: nFrameLen = %d \n",__LINE__, nFrameLen);
                                APP_DPRINT("%d :: Writing OutputBuffer No: %d to the file nWrite = %d \n",__LINE__, nOutBuff, nFrameLen);
                                frameLengthInBytes = subFrameLength*8;
                                fwrite(&frameLengthInBytes,2,1,fOut);
                                writeITUFormat((OMX_U8*)temp, subFrameLength,fOut);
                                temp+= subFrameLength;
                                fflush(fOut);
                            }
                        }
                        nFrameLen = nFrameLen - subFrameLength -1;
                    }
                    if (done) doneWriting = 1;
                    nFrameCount++;
                    nOutBuff++;
                    OMX_FillThisBuffer(pHandle, pBuf);
                    APP_DPRINT("%d :: App: pBuf->nFlags = %ld\n",__LINE__, pBuf->nFlags);
                    if (tcID == 0){
                        APP_DPRINT("-----------------------------------------------------\n");
                        APP_DPRINT("Output time stamp = %ld\n",(long int) pBuf->nTimeStamp);
                        APP_DPRINT("Output tick count = %ld\n", pBuf->nTickCount);                        
                        APP_DPRINT("-----------------------------------------------------\n");
                    }


                    if(pBuf->nFlags == 1) {
                        if(tcID != 3) {
                            printf("%d :: App: Sending OMX_StateIdle.........From APP \n",__LINE__);
#ifdef OMX_GETTIME
                            GT_START();
#endif
                            eError = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
                            if(eError != OMX_ErrorNone) {
                                printf("Error from SendCommand-Idle(Stop) State function\n");
                                bExitOnError = OMX_TRUE;
                                goto EXIT;
                            }
                            eError = WaitForState(pHandle, OMX_StateIdle);
#ifdef OMX_GETTIME
                            GT_END("Call to SendCommand <OMX_StateIdle>");
#endif
                            if ( eError != OMX_ErrorNone ){
                                printf("Error:WaitForState has timed out %d", eError);
                                bExitOnError = OMX_TRUE;
                                goto EXIT;
                            }
                            audioinfo->dasfMode = 0;
                            pBuf->nFlags = 0;
                            printf("%d :: App: Shutting down ---------- \n",__LINE__);
                        }
                    }
                }
                if(done == 1) {
                    eError = OMX_GetState(pHandle, &state);
                    if(eError != OMX_ErrorNone) {
                        APP_DPRINT("%d :: pComponent->GetState has returned status %X\n",__LINE__, eError);
                        bExitOnError = OMX_TRUE;
                        goto EXIT;
                    }
                }
            } /* While Loop Ending Here */
            printf("%d :: App: The current state of the component = %d \n",__LINE__,state);
            fclose(fOut);
            fclose(fIn);

            if(tcID == 5 || tcID == 4) {
                sleep (2);
            }
            else {
                sleep (0);
            }
            printf("%d :: App: G729 Encoded = %d Frames \n",__LINE__,(nOutBuff));
        } /*Test Case 4 & 5 Inner for loop ends here  */

        printf ("%d :: App: Sending the OMX_StateLoaded Command\n",__LINE__);
#ifdef OMX_GETTIME
        GT_START();
#endif
        eError = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateLoaded, NULL);
        if(eError != OMX_ErrorNone) {
            APP_DPRINT("%d:: Error from SendCommand-Idle State function\n",__LINE__);
            bExitOnError = OMX_TRUE;
            goto EXIT;
        }

        printf ("%d :: App: Sending the OMX_CommandPortDisable Command\n",__LINE__);
        eError = OMX_SendCommand(pHandle, OMX_CommandPortDisable, -1, NULL);
        if(eError != OMX_ErrorNone) {
            APP_DPRINT("%d:: Error from SendCommand OMX_CommandPortDisable\n",__LINE__);
            bExitOnError = OMX_TRUE;
            goto EXIT;
        }


        /* free the Allocate and Use Buffers */
        printf("%d :: App: Freeing the Allocate OR Use Buffers in TestApp\n",__LINE__);
        for(i=0; i < audioinfo->nIpBufs; i++) {
            APP_DPRINT("%d :: App: About to free pInputBufferHeader[%d]\n",__LINE__, i);
            eError = OMX_FreeBuffer(pHandle, G729APP_INPUT_PORT, pInputBufferHeader[i]);
            if((eError != OMX_ErrorNone)) {
                APP_DPRINT("%d:: Error in FreeBuffer function\n",__LINE__);
                goto EXIT;
            }
        }
        for(i=0; i < audioinfo->nOpBufs; i++) {
            APP_DPRINT("%d :: App: About to free pOutputBufferHeader[%d]\n",__LINE__, i);
            eError = OMX_FreeBuffer(pHandle, G729APP_OUTPUT_PORT, pOutputBufferHeader[i]);
            if((eError != OMX_ErrorNone)) {
                APP_DPRINT("%d :: Error in Free Buffer function\n",__LINE__);
                goto EXIT;
            }
        }
#ifdef USE_BUFFER
        /* free the App Allocated Buffers */
        printf("%d :: App: Freeing the App Allocated Buffers in TestApp\n",__LINE__);
        for(i=0; i < audioinfo->nIpBufs; i++) {
            pInputBuffer[i] = pInputBuffer[i] -128; 
            APP_MEMPRINT("%d :: App: [TESTAPPFREE] pInputBuffer[%d] = %p\n",__LINE__,i,pInputBuffer[i]);
            if(pInputBuffer[i] != NULL){
                free(pInputBuffer[i]);
                pInputBuffer[i] = NULL;
            }
        }

        for(i=0; i < audioinfo->nOpBufs; i++) {
            pOutputBuffer[i] = pOutputBuffer[i] - 128;
            APP_MEMPRINT("%d :: App: [TESTAPPFREE] pOutputBuffer[%d] = %p\n",__LINE__,i, pOutputBuffer[i]);
            if(pOutputBuffer[i] != NULL){
                free(pOutputBuffer[i]);
                pOutputBuffer[i] = NULL;
            }
        }
#endif

        eError = WaitForState(pHandle, OMX_StateLoaded);


#ifdef OMX_GETTIME
        GT_END("Call to SendCommand <OMX_StateLoaded>, Disableport and cleared the buffers");
#endif
        if(eError != OMX_ErrorNone) {
            APP_DPRINT( "Error:  G729Encoder->WaitForState reports an error         %X\n", eError);
            bExitOnError = OMX_TRUE;
            goto EXIT;
        }

        APP_MEMPRINT("%d :: App: [TESTAPPFREE] %p\n",__LINE__,pG729Param);
        if(pG729Param != NULL){
            free(pG729Param);
            pG729Param = NULL;
        }
        APP_MEMPRINT("%d :: App: [TESTAPPFREE] %p\n",__LINE__,pCompPrivateStruct);
        if(pCompPrivateStruct != NULL){
            free(pCompPrivateStruct);
            pCompPrivateStruct = NULL;
        }
        APP_MEMPRINT("%d :: App: [TESTAPPFREE] %p\n",__LINE__,audioinfo);
        if(audioinfo != NULL){
            free(audioinfo);
            audioinfo = NULL;
        }
        /* Unload the G729 Encoder Component */
        eError = TIOMX_FreeHandle(pHandle);
        if((eError != OMX_ErrorNone)) {
            APP_DPRINT("%d :: Error in Free Handle function\n",__LINE__);
            goto EXIT;
        }
        
    } /*Outer for loop ends here */
    /* De-Initialize OMX Core */
    eError = TIOMX_Deinit();
    if (eError != OMX_ErrorNone) {
        printf("App::Failed to de-init OMX Core!\n");
    }
    printf("EBD = %d\n FBD = %d\n", ebd, fbd);
    printf("%d :: *********************************************************************\n",__LINE__);
    printf("%d :: NOTE: An output file %s has been created in file system\n",__LINE__,argv[2]);
    printf("%d :: *********************************************************************\n",__LINE__);

 EXIT:

    if (bExitOnError){
        printf("Application exiting due to an error!!\n");
#ifdef USE_BUFFER
        FreeResources(pG729Param, pCompPrivateStruct, audioinfo, pInputBuffer, pOutputBuffer);
#else
        FreeResources(pG729Param, pCompPrivateStruct, audioinfo, pInputBufferHeader, pOutputBufferHeader, pHandle);
#endif
        /* Unload the G729 Encoder Component */
        eError = TIOMX_FreeHandle(pHandle);
        if((eError != OMX_ErrorNone)) {
            APP_DPRINT("%d :: Error in Free Handle function\n",__LINE__);
            goto EXIT;
        }
    }

#ifdef DSP_RENDERING_ON
    cmd_data.hComponent = pHandle;
    cmd_data.AM_Cmd = AM_Exit;
    if((write(g729encfdwrite, &cmd_data, sizeof(cmd_data)))<0)
        printf("%d ::- send command to audio manager\n",__LINE__);

    close(g729encfdwrite);
    close(g729encfdread);
#endif
#ifdef OMX_GETTIME
    GT_END("G729Enc test <End>");
    OMX_ListDestroy(pListHead);
#endif

    return eError;
}


/* This function writes the output into a format that is expected by the decoder */
void writeITUFormat(OMX_U8* buffer, OMX_U32 length, FILE* fOut)
{

    int i = 0,j = 0;
    OMX_U8 theByte = 0;
    OMX_U8 theMask = 0;
    OMX_U8 theBit = 0;
    OMX_U16 theWord = 0;

    for (j=0; j < length; j++) {

        theByte = buffer[j];
        theMask = 128;
        for (i=0; i < 8; i++) {
            theBit = (theMask & theByte) >> (7-i);
            theMask = theMask >> 1;
            if (theBit == 0) {
                theWord = BIT_0;
            }
            else if (theBit == 1) {
                theWord = BIT_1;
            }
            fwrite(&theWord,2,1,fOut);
        }
    }
}

#ifdef USE_BUFFER
OMX_ERRORTYPE FreeResources(OMX_AUDIO_PARAM_G729TYPE* pG729Param,
                            OMX_PARAM_PORTDEFINITIONTYPE* pCompPrivateStruct,
                            AUDIO_INFO* audioinfo,
                            OMX_U8* pInputBuffer[G729APP_MAX_NUM_OF_BUFS],
                            OMX_U8* pOutputBuffer[G729APP_MAX_NUM_OF_BUFS])
{


    OMX_ERRORTYPE eError = OMX_ErrorNone;
    int i = 0;

    /* free the App Allocated Buffers */
    printf("%d :: App: Freeing the App Allocated Buffers in TestApp\n",__LINE__);
    for(i=0; i < audioinfo->nIpBufs; i++) {
        pInputBuffer[i] = pInputBuffer[i] -128;
        APP_MEMPRINT("%d :: App: [TESTAPPFREE] pInputBuffer[%d] = %p\n",__LINE__,i,pInputBuffer[i]);
        if(pInputBuffer[i] != NULL){
            free(pInputBuffer[i]);
            pInputBuffer[i] = NULL;
        }
    }

    for(i=0; i < audioinfo->nOpBufs; i++) {
        pOutputBuffer[i] = pOutputBuffer[i] - 128;
        APP_MEMPRINT("%d :: App: [TESTAPPFREE] pOutputBuffer[%d] = %p\n",__LINE__,i, pOutputBuffer[i]);
        if(pOutputBuffer[i] != NULL){
            free(pOutputBuffer[i]);
            pOutputBuffer[i] = NULL;
        }
    }


    APP_MEMPRINT("%d :: App: [TESTAPPFREE] %p\n",__LINE__,pG729Param);
    if(pG729Param != NULL){
        free(pG729Param);
        pG729Param = NULL;
    }
    APP_MEMPRINT("%d :: App: [TESTAPPFREE] %p\n",__LINE__,pCompPrivateStruct);
    if(pCompPrivateStruct != NULL){
        free(pCompPrivateStruct);
        pCompPrivateStruct = NULL;
    }
    APP_MEMPRINT("%d :: App: [TESTAPPFREE] %p\n",__LINE__,audioinfo);
    if(audioinfo != NULL){
        free(audioinfo);
        audioinfo = NULL;
    }

    printf("%d :: App: Closing the Input and Output Pipes\n",__LINE__);
    eError = close (IpBuf_Pipe[0]);
    if (0 != eError && OMX_ErrorNone == eError) {
        eError = OMX_ErrorHardware;
        APP_DPRINT("%d :: Error while closing IpBuf_Pipe[0]\n",__LINE__);
        goto EXIT;
    }
    eError = close (IpBuf_Pipe[1]);
    if (0 != eError && OMX_ErrorNone == eError) {
        eError = OMX_ErrorHardware;
        APP_DPRINT("%d :: Error while closing IpBuf_Pipe[1]\n",__LINE__);
        goto EXIT;
    }
    eError = close (OpBuf_Pipe[0]);
    if (0 != eError && OMX_ErrorNone == eError) {
        eError = OMX_ErrorHardware;
        APP_DPRINT("%d :: Error while closing OpBuf_Pipe[0]\n",__LINE__);
        goto EXIT;
    }
    eError = close (OpBuf_Pipe[1]);
    if (0 != eError && OMX_ErrorNone == eError) {
        eError = OMX_ErrorHardware;
        APP_DPRINT("%d :: Error while closing OpBuf_Pipe[1]\n",__LINE__);
        goto EXIT;
    }



 EXIT:
    return eError;
}

#else

OMX_ERRORTYPE FreeResources(OMX_AUDIO_PARAM_G729TYPE* pG729Param,
                            OMX_PARAM_PORTDEFINITIONTYPE* pCompPrivateStruct,
                            AUDIO_INFO* audioinfo,
                            OMX_BUFFERHEADERTYPE* pInputBufferHeader[G729APP_MAX_NUM_OF_BUFS],
                            OMX_BUFFERHEADERTYPE* pOutputBufferHeader[G729APP_MAX_NUM_OF_BUFS],
                            OMX_HANDLETYPE pHandle)
{


    OMX_ERRORTYPE eError = OMX_ErrorNone;
    int i = 0;

    /* free the Allocate and Use Buffers */
    printf("%d :: App: Freeing the Allocate OR Use Buffers in TestApp\n",__LINE__);
    for(i=0; i < audioinfo->nIpBufs; i++) {
        APP_DPRINT("%d :: App: About to free pInputBufferHeader[%d]\n",__LINE__, i);
        eError = OMX_FreeBuffer(pHandle, G729APP_INPUT_PORT, pInputBufferHeader[i]);
        if((eError != OMX_ErrorNone)) {
            APP_DPRINT("%d:: Error in FreeBuffer function\n",__LINE__);
            bExitOnError = OMX_TRUE;
            goto EXIT;
        }
    }
    for(i=0; i < audioinfo->nOpBufs; i++) {
        APP_DPRINT("%d :: App: About to free pOutputBufferHeader[%d]\n",__LINE__, i);
        eError = OMX_FreeBuffer(pHandle, G729APP_OUTPUT_PORT, pOutputBufferHeader[i]);
        if((eError != OMX_ErrorNone)) {
            APP_DPRINT("%d :: Error in Free Buffer function\n",__LINE__);
            bExitOnError = OMX_TRUE;
            goto EXIT;
        }
    }

    APP_MEMPRINT("%d :: App: [TESTAPPFREE] %p\n",__LINE__,pG729Param);
    if(pG729Param != NULL){
        free(pG729Param);
        pG729Param = NULL;
    }
    APP_MEMPRINT("%d :: App: [TESTAPPFREE] %p\n",__LINE__,pCompPrivateStruct);
    if(pCompPrivateStruct != NULL){
        free(pCompPrivateStruct);
        pCompPrivateStruct = NULL;
    }
    APP_MEMPRINT("%d :: App: [TESTAPPFREE] %p\n",__LINE__,audioinfo);
    if(audioinfo != NULL){
        free(audioinfo);
        audioinfo = NULL;
    }

    printf("%d :: App: Closing the Input and Output Pipes\n",__LINE__);
    eError = close (IpBuf_Pipe[0]);
    if (0 != eError && OMX_ErrorNone == eError) {
        eError = OMX_ErrorHardware;
        APP_DPRINT("%d :: Error while closing IpBuf_Pipe[0]\n",__LINE__);
        goto EXIT;
    }
    eError = close (IpBuf_Pipe[1]);
    if (0 != eError && OMX_ErrorNone == eError) {
        eError = OMX_ErrorHardware;
        APP_DPRINT("%d :: Error while closing IpBuf_Pipe[1]\n",__LINE__);
        goto EXIT;
    }
    eError = close (OpBuf_Pipe[0]);
    if (0 != eError && OMX_ErrorNone == eError) {
        eError = OMX_ErrorHardware;
        APP_DPRINT("%d :: Error while closing OpBuf_Pipe[0]\n",__LINE__);
        goto EXIT;
    }
    eError = close (OpBuf_Pipe[1]);
    if (0 != eError && OMX_ErrorNone == eError) {
        eError = OMX_ErrorHardware;
        APP_DPRINT("%d :: Error while closing OpBuf_Pipe[1]\n",__LINE__);
        goto EXIT;
    }

 EXIT:

    return eError;
}

#endif
