
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
* @file WbAmrEnc_Test.c
*
* This file implements WBAMR Encoder Component Specific APIs and its functionality
* that is fully compliant with the Khronos OpenMAX (TM) 1.0 Specification
*
* @path  $(CSLPATH)\OMAPSW_MPU\linux\audio\src\openmax_il\wbamr_enc\tests
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
#include <OMX_Component.h>
#include <OMX_Core.h>
#include <OMX_Audio.h>
#include <TIDspOmx.h>

#ifdef DSP_RENDERING_ON
#include <AudioManagerAPI.h>
#endif
#include <time.h>

#ifdef OMX_GETTIME
#include <OMX_Common_Utils.h>
#include <OMX_GetTime.h>     /*Headers for Performance & measuremet    */
#endif
FILE *fpRes;

/* ======================================================================= */
/**
 * @def WBAPP_NUM_INPUT_BUFFERS   Default number of input buffers
 */
/* ======================================================================= */
#define WBAPP_NUM_INPUT_BUFFERS 1
/* ======================================================================= */
/**
 * @def WBAPP_NUM_INPUT_BUFFERS_DASF  Default No.of input buffers DASF
 */
/* ======================================================================= */
#define WBAPP_NUM_INPUT_BUFFERS_DASF 2
/* ======================================================================= */
/**
 * @def WBAPP_NUM_OUTPUT_BUFFERS   Default number of output buffers
 */
/* ======================================================================= */
#define WBAPP_NUM_OUTPUT_BUFFERS 1
/* ======================================================================= */
/**
 * @def WBAPP_INPUT_BUFFER_SIZE       Default input buffer size
 *      WBAPP_INPUT_BUFFER_SIZE_DASF  Default input buffer size DASF
 */
/* ======================================================================= */
#define WBAPP_INPUT_BUFFER_SIZE 640
#define WBAPP_INPUT_BUFFER_SIZE_DASF 640
/* ======================================================================= */
/**
 * @def WBAPP_OUTPUT_BUFFER_SIZE   Default output buffer size
 */
/* ======================================================================= */
#define WBAPP_OUTPUT_BUFFER_SIZE 116
/* ======================================================================= */
/**
 * @def WBAPP_OUTPUT_BUFFER_SIZE_MIME  Default input buffer size MIME
 */
/* ======================================================================= */
#define WBAPP_OUTPUT_BUFFER_SIZE_MIME 61

/* ======================================================================= */
/*
 * @def WBAMRENC_APP_ID  App ID Value setting
 */
/* ======================================================================= */
#define WBAMRENC_APP_ID 100

#define SLEEP_TIME 5

#define WBAMRENC_MIME_HEADER_LEN 9

#define FIFO1 "/dev/fifo.1"
#define FIFO2 "/dev/fifo.2"

#define APP_INFO

#undef APP_DEBUG

#undef USE_BUFFER

#undef APP_MEMCHECK


#ifdef  APP_INFO
#define APP_IPRINT(...)    fprintf(stderr,__VA_ARGS__)          /* Information prints */
#else
#define APP_IPRINT(...)
#endif


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

#undef APP_DEBUGMEM

#ifdef OMX_GETTIME
OMX_ERRORTYPE eError = OMX_ErrorNone;
int GT_FlagE = 0;  /* Fill Buffer 1 = First Buffer,  0 = Not First Buffer  */
int GT_FlagF = 0;  /*Empty Buffer  1 = First Buffer,  0 = Not First Buffer  */
static OMX_NODE* pListHead = NULL;
#endif

#ifdef APP_DEBUGMEM
void *arr[500];
int lines[500];
int bytes[500];
char file[500][50];
int ind = 0;

#define newmalloc(x) mynewmalloc(__LINE__,__FILE__,x)
#define newfree(z) mynewfree(z,__LINE__,__FILE__)

void * mynewmalloc(int line, char *s, int size) {
    void *p;
    int e = 0;
    p = calloc(1, size);

    if (p == NULL) {
        APP_IPRINT("Memory not available\n");
        exit(1);
    } else {
        while ((lines[e] != 0) && (e < 500) ) {
            e++;
        }

        arr[e] = p;
        lines[e] = line;
        bytes[e] = size;
        strcpy(file[e], s);
        APP_IPRINT("Allocating %d bytes on address %p, line %d file %s pos %d\n", size, p, line, s, e);
        return p;
    }
}

int mynewfree(void *dp, int line, char *s) {
    int q;

    if (dp == NULL) {
        APP_IPRINT("NULL can't be deleted\n");
        return 0;
    }

    for (q = 0; q < 500; q++) {
        if (arr[q] == dp) {
            APP_IPRINT("Deleting %d bytes on address %p, line %d file %s\n", bytes[q], dp, line, s);
            free(dp);
            dp = NULL;
            lines[q] = 0;
            strcpy(file[q], "");
            break;
        }
    }

    if (500 == q)
        APP_IPRINT("\n\nPointer not found. Line:%d    File%s!!\n\n", line, s);

    return 1;
}
#else
#define newmalloc(x) malloc(x)
#define newfree(z) free(z)
#endif

typedef struct WBAMRENC_BUFDATA {
    OMX_U8 nFrames;
} WBAMRENC_BUFDATA;

/* ======================================================================= */
/**
 *  M A C R O S FOR MALLOC and MEMORY FREE and CLOSING PIPES
 */
/* ======================================================================= */

#define OMX_WBAPP_CONF_INIT_STRUCT(_s_, _name_) \
    memset((_s_), 0x0, sizeof(_name_)); \
    (_s_)->nSize = sizeof(_name_);      \
    (_s_)->nVersion.s.nVersionMajor = 0x1;  \
    (_s_)->nVersion.s.nVersionMinor = 0x0;  \
    (_s_)->nVersion.s.nRevision = 0x0;      \
    (_s_)->nVersion.s.nStep = 0x0

#define OMX_WBAPP_INIT_STRUCT(_s_, _name_)  \
    memset((_s_), 0x0, sizeof(_name_)); \
     
#define OMX_WBAPP_MALLOC_STRUCT(_pStruct_, _sName_)   \
    _pStruct_ = (_sName_*)newmalloc(sizeof(_sName_));      \
    if(_pStruct_ == NULL){      \
        APP_IPRINT("***********************************\n"); \
        APP_IPRINT("%d :: Malloc Failed\n",__LINE__); \
        APP_IPRINT("***********************************\n"); \
        eError = OMX_ErrorInsufficientResources; \
        goto EXIT;      \
    } \
    APP_MEMPRINT("%d :: ALLOCATING MEMORY = %p\n",__LINE__,_pStruct_);

/* ======================================================================= */
/** WBAPP_COMP_PORT_TYPE  Port types
 *
 *  @param  WBAPP_INPUT_PORT            Input port
 *
 *  @param  WBAPP_OUTPUT_PORT           Output port
 */
/*  ====================================================================== */
/*This enum must not be changed. */
typedef enum WBAPP_COMP_PORT_TYPE {
    WBAPP_INPUT_PORT = 0,
    WBAPP_OUTPUT_PORT
} WBAPP_COMP_PORT_TYPE;

/* ======================================================================= */
/**
 * @def WBAPP_MAX_NUM_OF_BUFS      Maximum number of buffers
 * @def WBAPP_NUM_CHANNELS         Number of Channels
 * @def WBAPP_SAMPLING_FREQUENCY   Sampling frequency
 */
/* ======================================================================= */
#define WBAPP_MAX_NUM_OF_BUFS 10
#define WBAPP_NUM_OF_CHANNELS 1
#define WBAPP_SAMPLING_FREQUENCY 16000

#undef  WAITFORRESOURCES
pthread_mutex_t WaitForState_mutex;
pthread_cond_t  WaitForState_threshold;
OMX_U8          WaitForState_flag;
OMX_U8      TargetedState;

static OMX_BOOL bInvalidState;
void* ArrayOfPointers[6];
OMX_ERRORTYPE StopComponent(OMX_HANDLETYPE *pHandle);
OMX_ERRORTYPE PauseComponent(OMX_HANDLETYPE *pHandle);
OMX_ERRORTYPE PlayComponent(OMX_HANDLETYPE *pHandle);
OMX_ERRORTYPE send_input_buffer(OMX_HANDLETYPE pHandle, OMX_BUFFERHEADERTYPE* pBuffer, FILE *fIn);
int maxint(int a, int b);

int inputPortDisabled = 0;
int outputPortDisabled = 0;
OMX_U8 NextBuffer[WBAPP_INPUT_BUFFER_SIZE*3];
int FirstTime = 1;
int nRead;
WBAMRENC_BUFDATA* OutputFrames;

#ifdef DSP_RENDERING_ON
AM_COMMANDDATATYPE cmd_data;
#endif

OMX_STRING strWbAmrEncoder = "OMX.TI.WBAMR.encode";

#ifndef USE_BUFFER
int FreeAllResources( OMX_HANDLETYPE *pHandle,
                      OMX_BUFFERHEADERTYPE* pBufferIn,
                      OMX_BUFFERHEADERTYPE* pBufferOut,
                      int NIB, int NOB,
                      FILE* fIn, FILE* fOut);
#else
int  FreeAllResources(OMX_HANDLETYPE *pHandle,
                      OMX_U8* UseInpBuf[],
                      OMX_U8* UseOutBuf[],
                      int NIB, int NOB,
                      FILE* fIn, FILE* fOut);
#endif
int IpBuf_Pipe[2];
int OpBuf_Pipe[2];
int Event_Pipe[2];

fd_set rfds;
int done = 0;
int DasfMode = 0;
int mframe = 0;

int preempted = 0;

/* safe routine to get the maximum of 2 integers */
int maxint(int a, int b) {
    return (a > b) ? a : b;
}

/* This method will wait for the component to get to the state
 * specified by the DesiredState input. */
static OMX_ERRORTYPE WaitForState(OMX_HANDLETYPE* pHandle,
                                  OMX_STATETYPE DesiredState) {
    OMX_STATETYPE CurState = OMX_StateInvalid;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pComponent = (OMX_COMPONENTTYPE *)pHandle;

    eError = pComponent->GetState(pHandle, &CurState);

    if (CurState == OMX_StateInvalid && bInvalidState == OMX_TRUE) {
        eError = OMX_ErrorInvalidState;
    }

    if (CurState != DesiredState) {
        WaitForState_flag = 1;
        TargetedState = DesiredState;
        pthread_mutex_lock(&WaitForState_mutex);
        pthread_cond_wait(&WaitForState_threshold, &WaitForState_mutex);/*Going to sleep till signal arrives*/
        pthread_mutex_unlock(&WaitForState_mutex);
    }

    return eError;

}

OMX_ERRORTYPE EventHandler(
    OMX_HANDLETYPE hComponent,
    OMX_PTR pAppData,
    OMX_EVENTTYPE eEvent,
    OMX_U32 nData1,
    OMX_U32 nData2,
    OMX_PTR pEventData) {
    APP_DPRINT( "%d :: App: Entering EventHandler \n", __LINE__);
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pComponent = (OMX_COMPONENTTYPE *)hComponent;
    OMX_STATETYPE state;

    OMX_U8 writeValue;

    eError = pComponent->GetState (hComponent, &state);

    if (eError != OMX_ErrorNone) {
        APP_DPRINT("%d :: App: Error returned from GetState\n", __LINE__);
        goto EXIT;
    }

    APP_DPRINT( "%d :: App: Component eEvent = %d\n", __LINE__, eEvent);

    switch (eEvent) {
            APP_DPRINT( "%d :: App: Component State Changed To %d\n", __LINE__, state);
        case OMX_EventCmdComplete:
            APP_DPRINT( "%d :: App: Component State Changed To %d\n", __LINE__, state);

            if (nData1 == OMX_CommandPortDisable) {
                if (nData2 == WBAPP_INPUT_PORT) {
                    inputPortDisabled = 1;
                }

                if (nData2 == WBAPP_OUTPUT_PORT) {
                    outputPortDisabled = 1;
                }
            }

            if ((nData1 == OMX_CommandStateSet) && (TargetedState == nData2) &&
                    (WaitForState_flag)) {
                WaitForState_flag = 0;
                pthread_mutex_lock(&WaitForState_mutex);
                pthread_cond_signal(&WaitForState_threshold);
                pthread_mutex_unlock(&WaitForState_mutex);
            }

            break;
        case OMX_EventError:

            if (nData1 == OMX_ErrorInvalidState) {
                APP_IPRINT("Event Handler Invalid!!!\n\n");
                bInvalidState = OMX_TRUE;
            } else if (nData1 == OMX_ErrorResourcesPreempted) {
                preempted = 1;
                writeValue = 0;
                write(Event_Pipe[1], &writeValue, sizeof(OMX_U8));
            } else if (nData1 == OMX_ErrorResourcesLost) {
                WaitForState_flag = 0;
                pthread_mutex_lock(&WaitForState_mutex);
                pthread_cond_signal(&WaitForState_threshold);/*Sending Waking Up Signal*/
                pthread_mutex_unlock(&WaitForState_mutex);
            }

            break;
        case OMX_EventMax:
            APP_DPRINT( "%d :: App: Component OMX_EventMax = %d\n", __LINE__, eEvent);
            break;
        case OMX_EventMark:
            APP_DPRINT( "%d :: App: Component OMX_EventMark = %d\n", __LINE__, eEvent);
            break;
        case OMX_EventPortSettingsChanged:
            APP_DPRINT( "%d :: App: Component OMX_EventPortSettingsChanged = %d\n", __LINE__, eEvent);
            break;
        case OMX_EventBufferFlag:
            APP_DPRINT( "%d :: App: Component OMX_EventBufferFlag = %d\n", __LINE__, eEvent);
            writeValue = 2;
            write(Event_Pipe[1], &writeValue, sizeof(OMX_U8));
            break;
        case OMX_EventResourcesAcquired:
            APP_DPRINT( "%d :: App: Component OMX_EventResourcesAcquired = %d\n", __LINE__, eEvent);
            writeValue = 1;
            write(Event_Pipe[1], &writeValue, sizeof(OMX_U8));
            preempted = 0;

            break;
        default:
            break;

    }

EXIT:
    APP_DPRINT( "%d :: App: Exiting EventHandler \n", __LINE__);
    return eError;
}

void FillBufferDone (OMX_HANDLETYPE hComponent, OMX_PTR ptr, OMX_BUFFERHEADERTYPE* pBuffer) {
    if (!preempted)
        write(OpBuf_Pipe[1], &pBuffer, sizeof(pBuffer));

#ifdef OMX_GETTIME

    if (GT_FlagF == 1 ) /* First Buffer Reply*/ { /* 1 = First Buffer,  0 = Not First Buffer  */
        GT_END("Call to FillBufferDone  <First: FillBufferDone>");
        GT_FlagF = 0 ;   /* 1 = First Buffer,  0 = Not First Buffer  */
    }

#endif
}

void EmptyBufferDone(OMX_HANDLETYPE hComponent, OMX_PTR ptr, OMX_BUFFERHEADERTYPE* pBuffer) {
    write(IpBuf_Pipe[1], &pBuffer, sizeof(pBuffer));
#ifdef OMX_GETTIME

    if (GT_FlagE == 1 ) /* First Buffer Reply*/ { /* 1 = First Buffer,  0 = Not First Buffer  */
        GT_END("Call to EmptyBufferDone <First: EmptyBufferDone>");
        GT_FlagE = 0;   /* 1 = First Buffer,  0 = Not First Buffer  */
    }

#endif
}

int main(int argc, char* argv[]) {
    OMX_CALLBACKTYPE AmrCaBa = {(void *)EventHandler,
                                (void*)EmptyBufferDone,
                                (void*)FillBufferDone
                               };
    OMX_HANDLETYPE pHandle;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 AppData = WBAMRENC_APP_ID;
    OMX_PARAM_PORTDEFINITIONTYPE* pCompPrivateStruct;
    OMX_AUDIO_PARAM_AMRTYPE *pAmrParam;
    OMX_COMPONENTTYPE *pComponent;
    OMX_STATETYPE state;
    OMX_BUFFERHEADERTYPE* pInputBufferHeader[WBAPP_MAX_NUM_OF_BUFS];
    OMX_BUFFERHEADERTYPE* pOutputBufferHeader[WBAPP_MAX_NUM_OF_BUFS];
    bInvalidState = OMX_FALSE;
#ifdef USE_BUFFER
    OMX_U8* pInputBuffer[WBAPP_MAX_NUM_OF_BUFS];
    OMX_U8* pOutputBuffer[WBAPP_MAX_NUM_OF_BUFS];
#endif
    FILE* fIn = NULL;
    FILE* fOut = NULL;
    struct timeval tv;
    int retval, i, j, k, kk, tcID = 0;
    int frmCount = 0;
    int frmCnt = 1;
    int testcnt = 0;
    int testcnt1 = 0;
    int status = 0;
    int fdmax = 0;

    int nFrameCount = 1;
    int nFrameLen = 0;
    int nIpBuff = 1;
    int nOutBuff = 1;
    int numOfInputBuffer = 0;
    int numOfOutputBuffer = 0;
    OMX_INDEXTYPE index;
    int NoDataRead = 0;
    OMX_U32 streamId;
    TI_OMX_DATAPATH dataPath;
    TI_OMX_DSP_DEFINITION *audioinfo;
    OMX_AUDIO_CONFIG_VOLUMETYPE* pCompPrivateStructGain = NULL;
    int wbamrencfdwrite;
    int wbamrencfdread;

    pthread_mutex_init(&WaitForState_mutex, NULL);
    pthread_cond_init (&WaitForState_threshold, NULL);
    WaitForState_flag = 0;

    srand ( time(NULL) );
    APP_IPRINT("------------------------------------------------------\n");
    APP_IPRINT("This is Main Thread In WBAMR ENCODER Test Application:\n");
    APP_IPRINT("Test Core 1.5 - " __DATE__ ":" __TIME__ "\n");
    APP_IPRINT("------------------------------------------------------\n");

#ifdef OMX_GETTIME
    APP_IPRINT("Line %d\n", __LINE__);
    GTeError = OMX_ListCreate(&pListHead);
    APP_IPRINT("Line %d\n", __LINE__);
    APP_IPRINT("eError = %d\n", GTeError);
    GT_START();
    APP_IPRINT("Line %d\n", __LINE__);
#endif


    /* check the input parameters */
    if ((argc < 14) || (argc > 15)) {
        APP_IPRINT("%d :: Usage: [TestApp] [O/P] [FUNC_ID_X] [FM/DM] [WBAMR/EFR] [BITRATE] [DTXON/OFF] [NONMIME/MIME/IF2] [ACDNON/OFF] [FRAMES] [1 to N] [1 to N] [MFON]\n", __LINE__);
        goto EXIT;
    }

    /* check to see that the input file exists */
    struct stat sb = {0};
    status = stat(argv[1], &sb);

    if ( status != 0 ) {
        APP_IPRINT("Cannot find file %s. (%u)\n", argv[1], errno);
        goto EXIT;
    }

    /* Open the file of data to be encoded. */
    fIn = fopen(argv[1], "r");

    if ( fIn == NULL ) {
        APP_IPRINT("Error:  failed to open the input file %s\n", argv[1]);
        goto EXIT;
    }

    /* Open the file of data to be written. */
    fOut = fopen(argv[2], "w");

    if ( fOut == NULL ) {
        APP_IPRINT("Error:  failed to open the output file %s\n", argv[2]);
        goto EXIT;
    }

    if (!strcmp(argv[3], "FUNC_ID_1")) {
        APP_IPRINT("%d :: ### Testing TESTCASE 1 PLAY TILL END ###\n", __LINE__);
        testcnt = 1;
        testcnt1 = 1;
        tcID = 1;
    } else if (!strcmp(argv[3], "FUNC_ID_2")) {
        APP_IPRINT("%d :: ### Testing TESTCASE 2 STOP IN THE END ###\n", __LINE__);
        testcnt = 1;
        testcnt1 = 1;
        tcID = 2;
    } else if (!strcmp(argv[3], "FUNC_ID_3")) {
        APP_IPRINT("%d :: ### Testing TESTCASE 3 PAUSE - RESUME IN BETWEEN ###\n", __LINE__);
        testcnt = 1;
        testcnt1 = 1;
        tcID = 3;
    } else if (!strcmp(argv[3], "FUNC_ID_4")) {
        APP_IPRINT("%d :: ### Testing TESTCASE 4 STOP IN BETWEEN ###\n", __LINE__);
        testcnt = 2;
        testcnt1 = 1;
        tcID = 4;
        APP_IPRINT("######## testcnt = %d #########\n", testcnt);
    } else if (!strcmp(argv[3], "FUNC_ID_5")) {
        APP_IPRINT("%d :: ### Testing TESTCASE 5 ENCODE without Deleting component Here ###\n", __LINE__);

        if (argc == 15) {
            testcnt = atoi(argv[14]);
        } else {
            testcnt = 20;  /*20 cycles by default*/
        }

        testcnt1 = 1;
        tcID = 5;
    } else if (!strcmp(argv[3], "FUNC_ID_6")) {
        APP_IPRINT("%d :: ### Testing TESTCASE 6 ENCODE with Deleting component Here ###\n", __LINE__);

        if (argc == 15) {
            testcnt1 = atoi(argv[14]);
        } else {
            testcnt1 = 20;  /*20 cycles by default*/
        }

        testcnt = 1;
        tcID = 6;
    } else if (!strcmp(argv[3], "FUNC_ID_7")) {
        APP_IPRINT("%d :: ### Testing TESTCASE 7 ENCODE with Volume Control ###\n", __LINE__);
        testcnt = 1;
        testcnt1 = 1;
        tcID = 7;
    } else if (!strcmp(argv[3], "FUNC_ID_8")) {
        APP_IPRINT("%d :: ### Testing PLAY TILL END  WITH TWO FRAMES BY BUFFER###\n", __LINE__);
        testcnt = 1;
        testcnt1 = 1;
        tcID = 1;
        mframe = 1;
    } else {
        APP_IPRINT("%d :: ### Invalid test case###\n", __LINE__);
        goto EXIT;
    }

    for (j = 0; j < testcnt1; j++) {

#ifdef DSP_RENDERING_ON

        if ((wbamrencfdwrite = open(FIFO1, O_WRONLY)) < 0) {
            APP_IPRINT("[AMRTEST] - failure to open WRITE pipe\n");
        } else {
            APP_IPRINT("[AMRTEST] - opened WRITE pipe\n");
        }

        if ((wbamrencfdread = open(FIFO2, O_RDONLY)) < 0) {
            APP_IPRINT("[AMRTEST] - failure to open READ pipe\n");
            goto EXIT;
        } else {
            APP_IPRINT("[AMRTEST] - opened READ pipe\n");
        }

#endif

        /* Create a pipe used to queue data from the callback. */
        retval = pipe(IpBuf_Pipe);

        if ( retval != 0) {
            APP_DPRINT("Error:Fill Data Pipe failed to open\n");
            goto EXIT;
        }

        retval = pipe(OpBuf_Pipe);

        if ( retval != 0) {
            APP_DPRINT("Error:Empty Data Pipe failed to open\n");
            goto EXIT;
        }

        retval = pipe(Event_Pipe);

        if ( retval != 0) {
            APP_DPRINT( "Error:Empty Data Pipe failed to open\n");
            goto EXIT;
        }

        /* save off the "max" of the handles for the selct statement */
        fdmax = maxint(IpBuf_Pipe[0], OpBuf_Pipe[0]);
        fdmax = maxint(fdmax, Event_Pipe[0]);

        eError = TIOMX_Init();

        if (eError != OMX_ErrorNone) {
            APP_DPRINT("%d :: Error returned by OMX_Init()\n", __LINE__);
            goto EXIT;
        }

        TI_OMX_STREAM_INFO *streaminfo;

        OMX_WBAPP_MALLOC_STRUCT(streaminfo, TI_OMX_STREAM_INFO);
        OMX_WBAPP_MALLOC_STRUCT(audioinfo, TI_OMX_DSP_DEFINITION);
        OMX_WBAPP_INIT_STRUCT(audioinfo, TI_OMX_DSP_DEFINITION);

        ArrayOfPointers[0] = (TI_OMX_STREAM_INFO*)streaminfo;
        ArrayOfPointers[1] = (TI_OMX_DSP_DEFINITION*)audioinfo;

        if (j > 0) {
            APP_IPRINT ("%d :: Encoding the file for %d Time in TESTCASE 6\n", __LINE__, j + 1);
            fIn = fopen(argv[1], "r");

            if ( fIn == NULL ) {
                fprintf(stderr, "Error:  failed to open the file %s for read only access\n", argv[1]);
                goto EXIT;
            }

            fOut = fopen("TC6_WbAmr1.amr", "w");

            if ( fOut == NULL ) {
                fprintf(stderr, "Error:  failed to create the output file %s\n", argv[2]);
                goto EXIT;
            }
        }

        /* Load the WBAMR Encoder Component */

#ifdef OMX_GETTIME
        GT_START();
        eError = OMX_GetHandle(&pHandle, strWbAmrEncoder, &AppData, &AmrCaBa);
        GT_END("Call to GetHandle");
#else
        eError = TIOMX_GetHandle(&pHandle, strWbAmrEncoder, &AppData, &AmrCaBa);
#endif

        if ((eError != OMX_ErrorNone) || (pHandle == NULL)) {
            APP_DPRINT("Error in Get Handle function\n");
            goto EXIT;
        }

        /* Setting No.Of Input and Output Buffers for the Component */
        numOfInputBuffer = atoi(argv[11]);
        APP_IPRINT("\n%d :: App: audioinfo->nIpBufs = %d \n", __LINE__, numOfInputBuffer);

        numOfOutputBuffer = atoi(argv[12]);
        APP_IPRINT("\n%d :: App: audioinfo->numOfOutputBuffer = %d \n", __LINE__, numOfOutputBuffer);


        OMX_WBAPP_MALLOC_STRUCT(pCompPrivateStruct, OMX_PARAM_PORTDEFINITIONTYPE);
        OMX_WBAPP_CONF_INIT_STRUCT(pCompPrivateStruct, OMX_PARAM_PORTDEFINITIONTYPE);

        ArrayOfPointers[2] = (OMX_PARAM_PORTDEFINITIONTYPE*)pCompPrivateStruct;
        APP_DPRINT("%d :: Setting input port config\n", __LINE__);
        pCompPrivateStruct->nSize                              = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
        pCompPrivateStruct->nVersion.s.nVersionMajor           = 0xF1;
        pCompPrivateStruct->nVersion.s.nVersionMinor           = 0xF2;
        pCompPrivateStruct->nPortIndex                         = WBAPP_INPUT_PORT;
        pCompPrivateStruct->eDir                               = OMX_DirInput;
        pCompPrivateStruct->nBufferCountActual                 = numOfInputBuffer;
        pCompPrivateStruct->nBufferCountMin                    = numOfInputBuffer;
        pCompPrivateStruct->nBufferSize                        = WBAPP_INPUT_BUFFER_SIZE;
        pCompPrivateStruct->bEnabled                           = OMX_TRUE;
        pCompPrivateStruct->bPopulated                         = OMX_FALSE;
        pCompPrivateStruct->eDomain                            = OMX_PortDomainAudio;
        pCompPrivateStruct->format.audio.eEncoding             = OMX_AUDIO_CodingAMR;
        pCompPrivateStruct->format.audio.cMIMEType             = NULL;
        pCompPrivateStruct->format.audio.pNativeRender         = NULL;
        pCompPrivateStruct->format.audio.bFlagErrorConcealment = OMX_FALSE;    /*Send input port config*/
        APP_DPRINT("%d :: Setting input port config\n", __LINE__);

        if (!(strcmp(argv[8], "NONMIME"))) {
            pCompPrivateStruct->format.audio.cMIMEType = "NONMIME";
            APP_DPRINT("\n%d :: App: pCompPrivateStruct->format.audio.cMIMEType --> %s \n",
                       __LINE__, pCompPrivateStruct->format.audio.cMIMEType);
        } else if (!(strcmp(argv[8], "MIME"))) {
            pCompPrivateStruct->format.audio.cMIMEType = "MIME";
            APP_DPRINT("\n%d :: App: pCompPrivateStruct->format.audio.cMIMEType --> %s \n",
                       __LINE__, pCompPrivateStruct->format.audio.cMIMEType);
        } else if (!(strcmp(argv[8], "IF2"))) {
            pCompPrivateStruct->format.audio.cMIMEType = "IF2";
            APP_DPRINT("\n%d :: App: pCompPrivateStruct->format.audio.cMIMEType --> %s \n",
                       __LINE__, pCompPrivateStruct->format.audio.cMIMEType);

        } else {
            eError = OMX_ErrorBadParameter;
            APP_IPRINT("\n%d :: App: audioinfo->amrMIMEMode Sending Bad Parameter\n", __LINE__);
            APP_IPRINT("%d :: App: Should Be One of these Modes MIME, NONMIME, IF2\n", __LINE__);
            goto EXIT;
        }


        if (!(strcmp(argv[4], "FM"))) {
            audioinfo->dasfMode = 0;
            DasfMode = 0;
            APP_DPRINT("\n%d :: App: audioinfo->dasfMode = %x \n", __LINE__, audioinfo->dasfMode);
        } else if (!(strcmp(argv[4], "DM"))) {
            audioinfo->dasfMode = 1;
            DasfMode = 1;
            APP_DPRINT("\n%d :: App: audioinfo->dasfMode = %x \n", __LINE__, audioinfo->dasfMode);
            APP_DPRINT("%d :: WBAMR ENCODER RUNNING UNDER DASF MODE \n", __LINE__);
            pCompPrivateStruct->nBufferCountActual = 0;
        } else {
            eError = OMX_ErrorBadParameter;
            APP_IPRINT("\n%d :: App: audioinfo->dasfMode Sending Bad Parameter\n", __LINE__);
            APP_IPRINT("%d :: App: Should Be One of these Modes FM, DM\n", __LINE__);
            goto EXIT;
        }


        if (audioinfo->dasfMode == 0) {
            if ((atoi(argv[10])) != 0) {
                eError = OMX_ErrorBadParameter;
                APP_IPRINT("\n%d :: App: No. of Frames Sending Bad Parameter\n", __LINE__);
                APP_IPRINT("%d :: App: For FILE mode argv[10] Should Be --> 0\n", __LINE__);
                APP_IPRINT("%d :: App: For DASF mode argv[10] Should be greater than zero depends on number of frames user want to encode\n", __LINE__);
                goto EXIT;
            }
        } else {
            if ((atoi(argv[10])) == 0) {
                eError = OMX_ErrorBadParameter;
                APP_IPRINT("\n%d :: App: No. of Frames Sending Bad Parameter\n", __LINE__);
                APP_IPRINT("%d :: App: For DASF mode argv[10] Should be greater than zero depends on number of frames user want to encode\n", __LINE__);
                APP_IPRINT("%d :: App: For FILE mode argv[10] Should Be --> 0\n", __LINE__);
                goto EXIT;
            }
        }

        if (!(strcmp(argv[9], "ACDNOFF"))) {
            audioinfo->acousticMode = 0;
            APP_DPRINT("\n%d :: App: audioinfo->acousticMode = %x \n", __LINE__, audioinfo->acousticMode);
        } else if (!(strcmp(argv[9], "ACDNON"))) {
            audioinfo->acousticMode = 1;
            APP_DPRINT("\n%d :: App: audioinfo->acousticMode = %x \n", __LINE__, audioinfo->acousticMode);
        } else {
            eError = OMX_ErrorBadParameter;
            APP_IPRINT("\n%d :: App: audioinfo->acousticMode Sending Bad Parameter\n", __LINE__);
            APP_IPRINT("%d :: App: Should Be One of these Modes ACDNON, ACDNOFF\n", __LINE__);
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
            APP_DPRINT("%d :: OMX_ErrorBadParameter\n", __LINE__);
            goto EXIT;
        }

        APP_MEMPRINT("%d :: Setting output port config\n", __LINE__);
        pCompPrivateStruct->nSize                              = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
        pCompPrivateStruct->nVersion.s.nVersionMajor           = 0xF1;
        pCompPrivateStruct->nVersion.s.nVersionMinor           = 0xF2;
        pCompPrivateStruct->nPortIndex                         = WBAPP_OUTPUT_PORT;
        pCompPrivateStruct->eDir                               = OMX_DirOutput;
        pCompPrivateStruct->nBufferCountActual                 = numOfOutputBuffer;
        pCompPrivateStruct->nBufferCountMin                    = numOfOutputBuffer;
        pCompPrivateStruct->nBufferSize                        = WBAPP_OUTPUT_BUFFER_SIZE;
        pCompPrivateStruct->bEnabled                           = OMX_TRUE;
        pCompPrivateStruct->bPopulated                         = OMX_FALSE;
        pCompPrivateStruct->eDomain                            = OMX_PortDomainAudio;
        pCompPrivateStruct->format.audio.eEncoding             = OMX_AUDIO_CodingAMR;
        pCompPrivateStruct->format.audio.cMIMEType             = NULL;
        pCompPrivateStruct->format.audio.pNativeRender         = NULL;
        pCompPrivateStruct->format.audio.bFlagErrorConcealment = OMX_FALSE;    /*Send input port config*/

        OMX_WBAPP_MALLOC_STRUCT(pAmrParam, OMX_AUDIO_PARAM_AMRTYPE);
        OMX_WBAPP_CONF_INIT_STRUCT(pAmrParam, OMX_AUDIO_PARAM_AMRTYPE);
        ArrayOfPointers[3] = (OMX_AUDIO_PARAM_AMRTYPE *)pAmrParam;

        if (!(strcmp(argv[8], "NONMIME"))) {
            pCompPrivateStruct->format.audio.cMIMEType = "NONMIME";
            pAmrParam->eAMRFrameFormat = OMX_AUDIO_AMRFrameFormatConformance;
            APP_DPRINT("\n%d :: App: pCompPrivateStruct->cMIMEType --> %s \n", __LINE__, argv[3]);
            /**< Codec Configuring to WBAMR Mode Buffer Size to 116 */
            pCompPrivateStruct->nBufferSize = WBAPP_OUTPUT_BUFFER_SIZE;
        }

        if (!(strcmp(argv[8], "MIME"))) {
            pCompPrivateStruct->format.audio.cMIMEType = "MIME";
            pAmrParam->eAMRFrameFormat = OMX_AUDIO_AMRFrameFormatFSF;
            APP_DPRINT("\n%d :: App: pCompPrivateStruct->format.audio.cMIMEType --> %s \n",
                       __LINE__, pCompPrivateStruct->format.audio.cMIMEType);
            /**< Codec Configuring to MIME Mode Buffer Size to 61 */
            pCompPrivateStruct->nBufferSize = WBAPP_OUTPUT_BUFFER_SIZE_MIME;
        } else if (!(strcmp(argv[8], "IF2"))) {
            pCompPrivateStruct->format.audio.cMIMEType = "IF2";
            pAmrParam->eAMRFrameFormat = OMX_AUDIO_AMRFrameFormatIF2;
            APP_DPRINT("\n%d :: App: pCompPrivateStruct->format.audio.cMIMEType --> %s \n",
                       __LINE__, pCompPrivateStruct->format.audio.cMIMEType);
        }

        APP_DPRINT("\n%d :: App: pCompPrivateStruct->nBufferSize --> %ld \n", __LINE__, pCompPrivateStruct->nBufferSize);
#ifdef OMX_GETTIME
        GT_START();
        eError = OMX_SetParameter (pHandle, OMX_IndexParamPortDefinition, pCompPrivateStruct);
        GT_END("Set Parameter Test-SetParameter");
#else
        eError = OMX_SetParameter (pHandle, OMX_IndexParamPortDefinition, pCompPrivateStruct);
#endif

        if (eError != OMX_ErrorNone) {
            eError = OMX_ErrorBadParameter;
            APP_DPRINT("%d :: OMX_ErrorBadParameter\n", __LINE__);
            goto EXIT;
        }

        pAmrParam->nSize                    = sizeof(OMX_AUDIO_PARAM_AMRTYPE);
        pAmrParam->nVersion.s.nVersionMajor = 0xF1;
        pAmrParam->nVersion.s.nVersionMinor = 0xF2;
        pAmrParam->nPortIndex               = WBAPP_INPUT_PORT;
        pAmrParam->nChannels                = WBAPP_NUM_OF_CHANNELS;
#ifdef OMX_GETTIME
        GT_START();
        eError = OMX_SetParameter (pHandle, OMX_IndexParamAudioAmr, pAmrParam);
        GT_END("Set Parameter Test-SetParameter");
#else
        eError = OMX_SetParameter (pHandle, OMX_IndexParamAudioAmr, pAmrParam);
#endif

        if (eError != OMX_ErrorNone) {
            eError = OMX_ErrorBadParameter;
            APP_DPRINT("%d :: OMX_ErrorBadParameter\n", __LINE__);
            goto EXIT;
        }

        pAmrParam->nSize                    = sizeof(OMX_AUDIO_PARAM_AMRTYPE);
        pAmrParam->nVersion.s.nVersionMajor = 0xF1;
        pAmrParam->nVersion.s.nVersionMinor = 0xF2;
        pAmrParam->nPortIndex               = WBAPP_OUTPUT_PORT;
        pAmrParam->nChannels                = WBAPP_NUM_OF_CHANNELS;
        pAmrParam->eAMRBandMode             = OMX_AUDIO_AMRBandModeUnused;
        pAmrParam->eAMRDTXMode              = OMX_AUDIO_AMRDTXModeOff;

        if (!(strcmp(argv[6], "BR2385"))) {
            pAmrParam->eAMRBandMode = OMX_AUDIO_AMRBandModeWB8;
            APP_DPRINT("\n%d :: App: pAmrParam->eAMRBandMode = %d \n", __LINE__, pAmrParam->eAMRBandMode);
        } else if (!(strcmp(argv[6], "BR2305"))) {
            pAmrParam->eAMRBandMode = OMX_AUDIO_AMRBandModeWB7;
            APP_DPRINT("\n%d :: App: pAmrParam->eAMRBandMode = %d \n", __LINE__, pAmrParam->eAMRBandMode);
        } else if (!(strcmp(argv[6], "BR1985"))) {
            pAmrParam->eAMRBandMode = OMX_AUDIO_AMRBandModeWB6;
            APP_DPRINT("\n%d :: App: pAmrParam->eAMRBandMode = %d \n", __LINE__, pAmrParam->eAMRBandMode);
        } else if (!(strcmp(argv[6], "BR1825"))) {
            pAmrParam->eAMRBandMode = OMX_AUDIO_AMRBandModeWB5;
            APP_DPRINT("\n%d :: App: pAmrParam->eAMRBandMode = %d \n", __LINE__, pAmrParam->eAMRBandMode);
        } else if (!(strcmp(argv[6], "BR1585"))) {
            pAmrParam->eAMRBandMode = OMX_AUDIO_AMRBandModeWB4;
            APP_DPRINT("\n%d :: App: pAmrParam->eAMRBandMode = %d \n", __LINE__, pAmrParam->eAMRBandMode);
        } else if (!(strcmp(argv[6], "BR1425"))) {
            pAmrParam->eAMRBandMode = OMX_AUDIO_AMRBandModeWB3;
            APP_DPRINT("\n%d :: App: pAmrParam->eAMRBandMode = %d \n", __LINE__, pAmrParam->eAMRBandMode);
        } else if (!(strcmp(argv[6], "BR1265"))) {
            pAmrParam->eAMRBandMode = OMX_AUDIO_AMRBandModeWB2;
            APP_DPRINT("\n%d :: App: pAmrParam->eAMRBandMode = %d \n", __LINE__, pAmrParam->eAMRBandMode);
        } else if (!(strcmp(argv[6], "BR885"))) {
            pAmrParam->eAMRBandMode = OMX_AUDIO_AMRBandModeWB1;
            APP_DPRINT("\n%d :: App: pAmrParam->eAMRBandMode = %d \n", __LINE__, pAmrParam->eAMRBandMode);
        } else if (!(strcmp(argv[6], "BR660"))) {
            pAmrParam->eAMRBandMode = OMX_AUDIO_AMRBandModeWB0;
            APP_DPRINT("\n%d :: App: pAmrParam->eAMRBandMode = %d \n", __LINE__, pAmrParam->eAMRBandMode);
        } else {
            eError = OMX_ErrorBadParameter;
            APP_IPRINT("\n%d :: App: pAmrParam->eAMRBandMode Sending Bad Parameter\n", __LINE__);
            APP_IPRINT("%d :: App: Should Be One of these BitRates BR2385, BR2305, BR1985, BR1825, BR1585, BR1425, BR1265, BR885, BR660\n", __LINE__);
            goto EXIT;
        }

        APP_DPRINT("\n%d :: App: pAmrParam->eAMRBandMode --> %d \n", __LINE__, pAmrParam->eAMRBandMode);

        if (!(strcmp(argv[7], "DTXON"))) {
            /**< AMR Discontinuous Transmission Mode is enabled  */
            pAmrParam->eAMRDTXMode = OMX_AUDIO_AMRDTXModeOnVAD1;
            APP_DPRINT("\n%d :: App: pAmrParam->eAMRDTXMode --> %s \n", __LINE__, argv[7]);
        } else if (!(strcmp(argv[7], "DTXOFF"))) {
            /**< AMR Discontinuous Transmission Mode is disabled */
            pAmrParam->eAMRDTXMode = OMX_AUDIO_AMRDTXModeOff;
            APP_DPRINT("\n%d :: App: pAmrParam->eAMRDTXMode --> %s \n", __LINE__, argv[7]);
        } else {
            eError = OMX_ErrorBadParameter;
            APP_IPRINT("\n%d :: App: pAmrParam->eAMRDTXMode Sending Bad Parameter\n", __LINE__);
            APP_IPRINT("%d :: App: Should Be One of these Modes DTXON, DTXOFF\n", __LINE__);
            goto EXIT;
        }

#ifdef OMX_GETTIME
        GT_START();
        eError = OMX_SetParameter (pHandle, OMX_IndexParamAudioAmr, pAmrParam);
        GT_END("Set Parameter Test-SetParameter");
#else
        eError = OMX_SetParameter (pHandle, OMX_IndexParamAudioAmr, pAmrParam);
#endif

        if (eError != OMX_ErrorNone) {
            eError = OMX_ErrorBadParameter;
            APP_DPRINT("%d :: OMX_ErrorBadParameter\n", __LINE__);
            goto EXIT;
        }


        /* setting for stream gain */
        pCompPrivateStructGain = newmalloc (sizeof(OMX_AUDIO_CONFIG_VOLUMETYPE));

        if (pCompPrivateStructGain == NULL) {
            APP_DPRINT("%d :: App: Malloc Failed\n", __LINE__);
            goto EXIT;
        }

        ArrayOfPointers[4] = (OMX_AUDIO_CONFIG_VOLUMETYPE*) pCompPrivateStructGain;

        /* default setting for gain */
        pCompPrivateStructGain->nSize = sizeof(OMX_AUDIO_CONFIG_VOLUMETYPE);
        pCompPrivateStructGain->nVersion.s.nVersionMajor    = 0xF1;
        pCompPrivateStructGain->nVersion.s.nVersionMinor    = 0xF2;
        pCompPrivateStructGain->nPortIndex                  = OMX_DirOutput;
        pCompPrivateStructGain->bLinear                     = OMX_FALSE;
        pCompPrivateStructGain->sVolume.nValue              = 50;               /* actual volume */
        pCompPrivateStructGain->sVolume.nMin                = 0;                /* min volume */
        pCompPrivateStructGain->sVolume.nMax                = 100;              /* max volume */


        if (audioinfo->acousticMode == OMX_TRUE) {
            APP_IPRINT("Using Acoustic Devide Node Path\n");
            dataPath = DATAPATH_ACDN;
            fprintf("HERE %d \n", __LINE__);
        } else if (audioinfo->dasfMode) {
#ifdef RTM_PATH
            APP_IPRINT("Using Real Time Mixer Path\n");
            dataPath = DATAPATH_APPLICATION_RTMIXER;
            fprintf("HERE %d \n", __LINE__);
#endif

#ifdef ETEEDN_PATH
            APP_IPRINT("Using Eteedn Path\n");
            dataPath = DATAPATH_APPLICATION;
            fprintf("HERE %d \n", __LINE__);
#endif
        }

        eError = OMX_GetExtensionIndex(pHandle, "OMX.TI.index.config.wbamrheaderinfo", &index);

        if (eError != OMX_ErrorNone) {
            APP_IPRINT("Error getting extension index\n");
            goto EXIT;
        }

#ifdef DSP_RENDERING_ON
        cmd_data.hComponent = pHandle;
        cmd_data.AM_Cmd = AM_CommandIsInputStreamAvailable;

        cmd_data.param1 = 0;

        if ((write(wbamrencfdwrite, &cmd_data, sizeof(cmd_data))) < 0) {
            APP_IPRINT("%d ::WbAmrEncTest.c ::[WBAMR Enc Component] - send command to audio manager\n", __LINE__);
        }

        if ((read(wbamrencfdread, &cmd_data, sizeof(cmd_data))) < 0) {
            APP_IPRINT("%d ::WbAmrEncTest.c ::[WBAMR Enc Component] - failure to get data from the audio manager\n", __LINE__);
            goto EXIT;
        }

        audioinfo->streamId = cmd_data.streamID;
        streamId = audioinfo->streamId;
#endif

        eError = OMX_SetConfig (pHandle, index, audioinfo);

        if (eError != OMX_ErrorNone) {
            eError = OMX_ErrorBadParameter;
            APP_DPRINT("%d :: Error from OMX_SetConfig() function\n", __LINE__);
            goto EXIT;
        }

        eError = OMX_GetExtensionIndex(pHandle, "OMX.TI.index.config.wbamr.datapath", &index);

        if (eError != OMX_ErrorNone) {
            APP_IPRINT("Error getting extension index\n");
            goto EXIT;
        }

        eError = OMX_SetConfig (pHandle, index, &dataPath);

        if (eError != OMX_ErrorNone) {
            eError = OMX_ErrorBadParameter;
            APP_DPRINT("%d :: AmrDecTest.c :: Error from OMX_SetConfig() function\n", __LINE__);
            goto EXIT;
        }

#ifdef OMX_GETTIME
        GT_START();
#endif
        eError = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);

        if (eError != OMX_ErrorNone) {
            APP_DPRINT("Error from SendCommand-Idle(Init) State function\n");
            goto EXIT;
        }

        sleep(1);


#ifndef USE_BUFFER
        APP_DPRINT("%d :: About to call OMX_AllocateBuffer\n", __LINE__);

        if (!DasfMode) {
            /* allocate input buffer */
            for (i = 0; i < numOfInputBuffer; i++) {
                APP_DPRINT("%d :: About to call OMX_AllocateBuffer for pInputBufferHeader[%d]\n", __LINE__, i);
                eError = OMX_AllocateBuffer(pHandle, &pInputBufferHeader[i], 0, NULL, WBAPP_INPUT_BUFFER_SIZE * 2);

                if (eError != OMX_ErrorNone) {
                    APP_DPRINT("%d :: Error returned by OMX_AllocateBuffer for pInputBufferHeader[%d]\n", __LINE__, i);
                    goto EXIT;
                }
            }
        }

        APP_DPRINT("\n%d :: App: pCompPrivateStruct->nBufferSize --> %ld \n", __LINE__, pCompPrivateStruct->nBufferSize);

        for (i = 0; i < numOfOutputBuffer; i++) {
            /* allocate output buffer */
            APP_DPRINT("%d :: About to call OMX_AllocateBuffer for pOutputBufferHeader[%d]\n", __LINE__, i);
            eError = OMX_AllocateBuffer(pHandle, &pOutputBufferHeader[i], 1, NULL, pCompPrivateStruct->nBufferSize * 2);

            if (eError != OMX_ErrorNone) {
                APP_DPRINT("%d :: Error returned by OMX_AllocateBuffer for pOutputBufferHeader[%d]\n", __LINE__, i);
                goto EXIT;
            }
        }

#else

        if (!DasfMode) {
            for (i = 0; i < numOfInputBuffer; i++) {
                pInputBuffer[i] = (OMX_U8*)newmalloc(WBAPP_INPUT_BUFFER_SIZE * 2 + 256);
                APP_MEMPRINT("%d :: [TESTAPP ALLOC] pInputBuffer[%d] = %p\n", __LINE__, i, pInputBuffer[i]);

                if (NULL == pInputBuffer[i]) {
                    APP_DPRINT("%d :: Malloc Failed\n", __LINE__);
                    eError = OMX_ErrorInsufficientResources;
                    goto EXIT;
                }

                pInputBuffer[i] = pInputBuffer[i] + 128;

                /*  allocate input buffer */
                APP_DPRINT("%d :: About to call OMX_UseBuffer\n", __LINE__);
                eError = OMX_UseBuffer(pHandle, &pInputBufferHeader[i], 0, NULL, WBAPP_INPUT_BUFFER_SIZE * 2, pInputBuffer[i]);

                if (eError != OMX_ErrorNone) {
                    APP_DPRINT("%d :: Error returned by OMX_UseBuffer()\n", __LINE__);
                    goto EXIT;
                }
            }
        }

        for (i = 0; i < numOfOutputBuffer; i++) {
            pOutputBuffer[i] = newmalloc (pCompPrivateStruct->nBufferSize * 2 + 256);
            APP_MEMPRINT("%d :: [TESTAPP ALLOC] pOutputBuffer[%d] = %p\n", __LINE__, i, pOutputBuffer[i]);

            if (NULL == pOutputBuffer[i]) {
                APP_DPRINT("%d :: Malloc Failed\n", __LINE__);
                eError = OMX_ErrorInsufficientResources;
                goto EXIT;
            }

            pOutputBuffer[i] = pOutputBuffer[i] + 128;

            /* allocate output buffer */
            APP_DPRINT("%d :: About to call OMX_UseBuffer\n", __LINE__);
            eError = OMX_UseBuffer(pHandle, &pOutputBufferHeader[i], 1, NULL, pCompPrivateStruct->nBufferSize * 2, pOutputBuffer[i]);

            if (eError != OMX_ErrorNone) {
                APP_DPRINT("%d :: Error returned by OMX_UseBuffer()\n", __LINE__);
                goto EXIT;
            }
        }

#endif
        /* Wait for startup to complete */
        eError = WaitForState(pHandle, OMX_StateIdle);
#ifdef OMX_GETTIME
        GT_END("Call to SendCommand <OMX_StateIdle & Allocated the buffers & Cleared it>");
#endif

        if (eError != OMX_ErrorNone) {
            APP_DPRINT( "Error:  WaitForState reports an eError %X\n", eError);
            goto EXIT;
        }

        if (audioinfo->dasfMode) {
            /* get streamID back to application */
            eError = OMX_GetExtensionIndex(pHandle, "OMX.TI.index.config.wbamrstreamIDinfo", &index);

            if (eError != OMX_ErrorNone) {
                APP_IPRINT("Error getting extension index\n");
                goto EXIT;
            }

            eError = OMX_GetConfig (pHandle, index, streaminfo);

            if (eError != OMX_ErrorNone) {
                eError = OMX_ErrorBadParameter;
                APP_DPRINT("%d :: PcmDecTest.c :: Error from OMX_GetConfig() function\n", __LINE__);
                goto EXIT;
            }

            streamId = streaminfo->streamId;
            APP_IPRINT("***************StreamId=%d******************\n", (int)streamId);
        }

        for (i = 0; i < testcnt; i++) {
            frmCnt = 1;
            nFrameCount = 1;
            nOutBuff = 1;
            nIpBuff  = 1;

            if (i > 0) {
                APP_IPRINT ("%d :: Encoding the file for %d Time\n", __LINE__, i + 1);
                fIn = fopen(argv[1], "r");

                if (fIn == NULL) {
                    fprintf(stderr, "Error:  failed to open the file %s for readonly access\n", argv[1]);
                    goto EXIT;
                }

                fOut = fopen("TC5_WbAmr1.amr", "w");

                if (fOut == NULL) {
                    fprintf(stderr, "Error:  failed to create the output file %s\n", argv[2]);
                    goto EXIT;
                }
            }

            APP_IPRINT("%d :: App: pAmrParam->eAMRBandMode --> %d \n", __LINE__, pAmrParam->eAMRBandMode);
            APP_IPRINT("%d :: App: pAmrParam->eAMRDTXMode --> %s \n", __LINE__, argv[4]);
            APP_IPRINT("%d :: App: pCompPrivateStruct->format.audio.cMIMEType --> %s \n", __LINE__, pCompPrivateStruct->format.audio.cMIMEType);

            APP_IPRINT("%d :: App: Sending OMX_StateExecuting Command\n", __LINE__);
#ifdef OMX_GETTIME
            GT_START()
#endif
            eError = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);

            if (eError != OMX_ErrorNone) {
                APP_DPRINT("Error from SendCommand-Executing State function\n");
                goto EXIT;
            }

            eError = WaitForState(pHandle, OMX_StateExecuting);
#ifdef OMX_GETTIME
            GT_END("Call to SendCommand <OMX_StateExecuting>");
#endif

            if (eError != OMX_ErrorNone) {
                APP_DPRINT( "Error:  WaitForState reports an eError %X\n", eError);
                goto EXIT;
            }

            if (audioinfo->dasfMode ) {
                APP_IPRINT("%d :: App: No.of Frames Encoding = %d\n", __LINE__, atoi(argv[10]));
            }

            pComponent = (OMX_COMPONENTTYPE *)pHandle;

            if (audioinfo->dasfMode == 0) {
                for (k = 0; k < numOfInputBuffer; k++) {
                    OMX_BUFFERHEADERTYPE* pBuffer = pInputBufferHeader[k];
                    pBuffer->nFlags = 0;
#ifdef OMX_GETTIME

                    if (k == 0) {
                        GT_FlagE = 1;  /* 1 = First Buffer,  0 = Not First Buffer  */
                        GT_START(); /* Empty Bufffer */
                    }

#endif
                    eError =  send_input_buffer(pHandle, pBuffer, fIn);
                }
            }

            for (kk = 0; kk < numOfOutputBuffer; kk++) {
                APP_DPRINT("%d :: App: Calling FillThisBuffer \n", __LINE__);
#ifdef OMX_GETTIME

                if (kk == 0) {
                    GT_FlagF = 1;  /* 1 = First Buffer,  0 = Not First Buffer  */
                    GT_START(); /* Fill Buffer */
                }

#endif

                pComponent->FillThisBuffer(pHandle, pOutputBufferHeader[kk]);

            }

            eError = pComponent->GetState(pHandle, &state);

            if (eError != OMX_ErrorNone) {
                APP_DPRINT("%d :: pComponent->GetState has returned status %X\n", __LINE__, eError);
                goto EXIT;
            }

            retval = 1;


#ifndef WAITFORRESOURCES

            while ( (eError == OMX_ErrorNone) && (state != OMX_StateIdle)  && (state != OMX_StateInvalid) ) {
                if (1) {
#else

            while (1) {
                if ((eError == OMX_ErrorNone) && (state != OMX_StateIdle) && (state != OMX_StateInvalid) ) {
#endif
                    FD_ZERO(&rfds);
                    FD_SET(IpBuf_Pipe[0], &rfds);
                    FD_SET(OpBuf_Pipe[0], &rfds);
                    FD_SET(Event_Pipe[0], &rfds);

                    tv.tv_sec = 1;
                    tv.tv_usec = 0;
                    frmCount++;
                    retval = select(fdmax + 1, &rfds, NULL, NULL, &tv);

                    if (retval == -1) {
                        perror("select()");
                        APP_DPRINT( " :: Error \n");
                        break;
                    }

                    if (!retval) {
                        NoDataRead++;

                        if (NoDataRead == 2) {
                            APP_IPRINT("Stoping component since No data is read from the pipes\n");
                            StopComponent(pHandle);
                        }
                    } else {
                        NoDataRead = 0;
                    }

                    switch (tcID) {
                        case 1:
                        case 2:
                        case 3:
                        case 4:
                        case 5:
                        case 6:
                        case 7:

                            if (audioinfo->dasfMode == 0) {
                                if (FD_ISSET(IpBuf_Pipe[0], &rfds)) {
                                    OMX_BUFFERHEADERTYPE* pBuffer;
                                    read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));

                                    if ((frmCount == 14 || frmCount == 15) && tcID == 3) { /*Pause the component*/
                                        APP_IPRINT("App: Pausing Component for 2 Seconds\n");
                                        PauseComponent(pHandle);
                                        sleep(2);
                                        APP_IPRINT("App: Resume Component\n");
                                        PlayComponent(pHandle);
                                    }

                                    if (frmCount == 20 && tcID == 4) { /*Stop the component*/
                                        tcID = 1;
                                        StopComponent(pHandle);
                                        break;
                                    }

                                    eError =  send_input_buffer(pHandle, pBuffer, fIn);
                                }
                            } else {
                                if (frmCount == 15 && tcID == 3) { /*Pause the component*/
                                    tcID = 1;
                                    APP_IPRINT("App: Pausing Component for 2 Seconds\n");
                                    PauseComponent(pHandle);
                                    sleep(2);
                                    APP_IPRINT("App: Resume Component\n");
                                    PlayComponent(pHandle);
                                }

                                if (frmCount == 20 && tcID == 4) { /*Stop the component*/
                                    StopComponent(pHandle);
                                    break;
                                }

                                APP_DPRINT("%d :: WBAMR ENCODER RUNNING UNDER DASF MODE \n", __LINE__);

                                if (nFrameCount == 10 && tcID == 7) {
                                    /* set high gain for record stream */
                                    APP_IPRINT("[WBAMR encoder] --- will set stream gain to high\n");
                                    pCompPrivateStructGain->sVolume.nValue = 0x8000;
                                    eError = OMX_SetConfig(pHandle, OMX_IndexConfigAudioVolume, pCompPrivateStructGain);

                                    if (eError != OMX_ErrorNone) {
                                        eError = OMX_ErrorBadParameter;
                                        goto EXIT;
                                    }
                                }

                                if (nFrameCount == 250 && tcID == 7) {
                                    /* set low gain for record stream */
                                    APP_IPRINT("[WBAMR encoder] --- will set stream gain to low\n");
                                    pCompPrivateStructGain->sVolume.nValue = 0x2000;
                                    eError = OMX_SetConfig(pHandle, OMX_IndexConfigAudioVolume, pCompPrivateStructGain);

                                    if (eError != OMX_ErrorNone) {
                                        eError = OMX_ErrorBadParameter;
                                        goto EXIT;
                                    }
                                }

                                if (nFrameCount == atoi(argv[10])) {
                                    StopComponent(pHandle);
                                }

                                APP_DPRINT("%d :: WBAMR ENCODER READING DATA FROM DASF  \n", __LINE__);
                            }

                            break;
                        default:
                            APP_DPRINT("%d :: ### Simple DEFAULT Case Here ###\n", __LINE__);
                    }

                    if ( FD_ISSET(OpBuf_Pipe[0], &rfds) ) {
                        OMX_BUFFERHEADERTYPE* pBuf;
                        read(OpBuf_Pipe[0], &pBuf, sizeof(pBuf));
                        APP_DPRINT("%d :: App: pBuf->nFilledLen = %ld\n", __LINE__, pBuf->nFilledLen);
                        nFrameLen = pBuf->nFilledLen;

                        if (!(strcmp(pCompPrivateStruct->format.audio.cMIMEType, "MIME"))) {
                            if (1 == nFrameCount) {
                                char MimeHeader[] = {0x23, 0x21, 0x41, 0x4d, 0x52, 0x2d, 0x57, 0x42, 0x0a};
                                fwrite(MimeHeader, 1, WBAMRENC_MIME_HEADER_LEN, fOut);
                                fflush(fOut);
                                APP_IPRINT("%d :: App: MIME Supported:: FrameLen = %d\n", __LINE__, nFrameLen);
                            }
                        }

                        APP_DPRINT("%d :: App: nFrameLen = %d \n", __LINE__, nFrameLen);

                        if (nFrameLen != 0) {
                            APP_DPRINT("%d :: Writing OutputBuffer No: %d to the file nWrite = %d \n", __LINE__, nOutBuff, nFrameLen);
                            fwrite(pBuf->pBuffer, 1, nFrameLen, fOut);
                            fflush(fOut);
                        }

                        if (pBuf->nFlags == OMX_BUFFERFLAG_EOS) {
                            APP_IPRINT("%d :: App: OMX_BUFFERFLAG_EOS is received\n", __LINE__);
                            APP_IPRINT("%d :: App: Shutting down ---------- \n", __LINE__);
                            StopComponent(pHandle);
                            pBuf->nFlags = 0;
                        } else {
                            nFrameCount++;
                            nOutBuff++;
                            pComponent->FillThisBuffer(pHandle, pBuf);
                            APP_DPRINT("%d :: App: pBuf->nFlags = %ld\n", __LINE__, pBuf->nFlags);
                        }
                    }

                    if ( FD_ISSET(Event_Pipe[0], &rfds) ) {
                        OMX_U8 pipeContents;
                        read(Event_Pipe[0], &pipeContents, sizeof(OMX_U8));

                        if (pipeContents == 0) {
                            APP_IPRINT("Test app received OMX_ErrorResourcesPreempted\n");
                            WaitForState(pHandle, OMX_StateIdle);

                            for (i = 0; i < numOfInputBuffer; i++) {
                                APP_DPRINT("%d :: App: About to newfree pInputBufferHeader[%d]\n", __LINE__, i);
                                eError = OMX_FreeBuffer(pHandle, WBAPP_INPUT_PORT, pInputBufferHeader[i]);

                                if ((eError != OMX_ErrorNone)) {
                                    APP_DPRINT("%d:: Error in FreeBuffer function\n", __LINE__);
                                    goto EXIT;
                                }

                            }

                            for (i = 0; i < numOfOutputBuffer; i++) {
                                APP_DPRINT("%d :: App: About to newfree pOutputBufferHeader[%d]\n", __LINE__, i);
                                eError = OMX_FreeBuffer(pHandle, WBAPP_OUTPUT_PORT, pOutputBufferHeader[i]);

                                if ((eError != OMX_ErrorNone)) {
                                    APP_DPRINT("%d :: Error in Free Buffer function\n", __LINE__);
                                    goto EXIT;
                                }

                            }

#ifdef USE_BUFFER

                            for (i = 0; i < numOfInputBuffer; i++) {
                                if (pInputBuffer[i] != NULL) {
                                    APP_MEMPRINT("%d :: App: [TESTAPPFREE] pInputBuffer[%d] = %p\n", __LINE__, i, pInputBuffer[i]);
                                    pInputBuffer[i] = pInputBuffer[i] - 128;
                                    newfree(pInputBuffer[i]);
                                    pInputBuffer[i] = NULL;
                                }
                            }

#endif

                            OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateLoaded, NULL);
                            WaitForState(pHandle, OMX_StateLoaded);

                            OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateWaitForResources, NULL);
                            WaitForState(pHandle, OMX_StateWaitForResources);
                        } else if (pipeContents == 1) {
                            APP_IPRINT("Test app received OMX_ErrorResourcesAcquired\n");

                            OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);

                            for (i = 0; i < numOfOutputBuffer; i++) {
                                /* allocate output buffer */
                                APP_DPRINT("%d :: About to call OMX_AllocateBuffer for pOutputBufferHeader[%d]\n", __LINE__, i);
                                eError = OMX_AllocateBuffer(pHandle, &pOutputBufferHeader[i], 1, NULL, pCompPrivateStruct->nBufferSize * 2);

                                if (eError != OMX_ErrorNone) {
                                    APP_DPRINT("%d :: Error returned by OMX_AllocateBuffer for pOutputBufferHeader[%d]\n", __LINE__, i);
                                    goto EXIT;
                                }
                            }

                            WaitForState(pHandle, OMX_StateIdle);

                            OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
                            WaitForState(pHandle, OMX_StateExecuting);

                            rewind(fIn);

                            for (i = 0; i < numOfInputBuffer; i++) {
                                send_input_buffer (pHandle, pOutputBufferHeader[i], fIn);
                            }
                        }

                        if (pipeContents == 2) {

#ifdef OMX_GETTIME
                            GT_START();
#endif

                            OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
                            WaitForState(pHandle, OMX_StateIdle);

#ifdef OMX_GETTIME
                            GT_END("Call to SendCommand <OMX_StateIdle>");
#endif

#ifdef WAITFORRESOURCES

                            for (i = 0; i < numOfInputBuffer; i++) {
                                APP_DPRINT("%d :: App: About to newfree pInputBufferHeader[%d]\n", __LINE__, i);
                                eError = OMX_FreeBuffer(pHandle, WBAPP_INPUT_PORT, pInputBufferHeader[i]);

                                if ((eError != OMX_ErrorNone)) {
                                    APP_DPRINT("%d:: Error in FreeBuffer function\n", __LINE__);
                                    goto EXIT;
                                }

                            }

                            for (i = 0; i < numOfOutputBuffer; i++) {
                                APP_DPRINT("%d :: App: About to newfree pOutputBufferHeader[%d]\n", __LINE__, i);
                                eError = OMX_FreeBuffer(pHandle, WBAPP_OUTPUT_PORT, pOutputBufferHeader[i]);

                                if ((eError != OMX_ErrorNone)) {
                                    APP_DPRINT("%d :: Error in Free Buffer function\n", __LINE__);
                                    goto EXIT;
                                }

                            }

                            OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateLoaded, NULL);
                            WaitForState(pHandle, OMX_StateLoaded);

                            goto SHUTDOWN;
#endif
                        }
                    }


                    eError = pComponent->GetState(pHandle, &state);

                    if (eError != OMX_ErrorNone) {
                        APP_DPRINT("%d :: pComponent->GetState has returned status %X\n", __LINE__, eError);
                        goto EXIT;
                    }

                } else if (preempted) {
                    sched_yield();
                } else {
                    goto SHUTDOWN;
                }

            } /* While Loop Ending Here */

            APP_IPRINT("%d :: App: The current state of the component = %d \n", __LINE__, state);
            fclose(fOut);
            fclose(fIn);
            FirstTime = 1;
            NoDataRead = 0;

            if (tcID == 4)
                tcID = 1;

            APP_IPRINT("%d :: App: WBAMR Encoded = %d Frames \n", __LINE__, (nOutBuff));
        } /*Test Case 4 & 5 Inner for loop ends here  */

        /* newfree the Allocate and Use Buffers */
        APP_IPRINT("%d :: App: Freeing the Allocate OR Use Buffers in TestApp\n", __LINE__);

        if (!DasfMode) {
            for (i = 0; i < numOfInputBuffer; i++) {
                APP_DPRINT("%d :: App: About to newfree pInputBufferHeader[%d]\n", __LINE__, i);
                eError = OMX_FreeBuffer(pHandle, WBAPP_INPUT_PORT, pInputBufferHeader[i]);

                if ((eError != OMX_ErrorNone)) {
                    APP_DPRINT("%d:: Error in FreeBuffer function\n", __LINE__);
                    goto EXIT;
                }
            }
        }

        for (i = 0; i < numOfOutputBuffer; i++) {
            APP_DPRINT("%d :: App: About to newfree pOutputBufferHeader[%d]\n", __LINE__, i);
            eError = OMX_FreeBuffer(pHandle, WBAPP_OUTPUT_PORT, pOutputBufferHeader[i]);

            if ((eError != OMX_ErrorNone)) {
                APP_DPRINT("%d :: Error in Free Buffer function\n", __LINE__);
                goto EXIT;
            }
        }

#ifdef USE_BUFFER
        /* newfree the App Allocated Buffers */
        APP_IPRINT("%d :: App: Freeing the App Allocated Buffers in TestApp\n", __LINE__);

        if (!DasfMode) {
            for (i = 0; i < numOfInputBuffer; i++) {
                if (pInputBuffer[i] != NULL) {
                    APP_MEMPRINT("%d :: App: [TESTAPPFREE] pInputBuffer[%d] = %p\n", __LINE__, i, pInputBuffer[i]);
                    pInputBuffer[i] = pInputBuffer[i] - 128;
                    newfree(pInputBuffer[i]);
                    pInputBuffer[i] = NULL;
                }
            }
        }

        for (i = 0; i < numOfOutputBuffer; i++) {
            if (pOutputBuffer[i] != NULL) {
                APP_MEMPRINT("%d :: App: [TESTAPPFREE] pOutputBuffer[%d] = %p\n", __LINE__, i, pOutputBuffer[i]);
                pOutputBuffer[i] = pOutputBuffer[i] - 128;
                newfree(pOutputBuffer[i]);
                pOutputBuffer[i] = NULL;
            }
        }

#endif
        APP_IPRINT ("%d :: App: Sending the OMX_StateLoaded Command\n", __LINE__);
#ifdef OMX_GETTIME
        GT_START();
#endif
        eError = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateLoaded, NULL);

        if (eError != OMX_ErrorNone) {
            APP_DPRINT("%d:: Error from SendCommand-Idle State function\n", __LINE__);
            goto EXIT;
        }

        eError = WaitForState(pHandle, OMX_StateLoaded);
#ifdef OMX_GETTIME
        GT_END("Call to SendCommand <OMX_StateLoaded>");
#endif

        if (eError != OMX_ErrorNone) {
            APP_DPRINT( "Error:  WaitForState reports an eError %X\n", eError);
            goto EXIT;
        }

        APP_IPRINT ("%d :: App: Sending the OMX_CommandPortDisable Command\n", __LINE__);
        eError = OMX_SendCommand(pHandle, OMX_CommandPortDisable, -1, NULL);

        if (eError != OMX_ErrorNone) {
            APP_DPRINT("%d:: Error from SendCommand OMX_CommandPortDisable\n", __LINE__);
            goto EXIT;
        }


#ifdef WAITFORRESOURCES
        eError = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateWaitForResources, NULL);

        if (eError != OMX_ErrorNone) {
            APP_DPRINT ("%d Error from SendCommand-Idle State function\n", __LINE__);
            goto EXIT;
        }

        eError = WaitForState(pHandle, OMX_StateWaitForResources);

        /* temporarily put this here until I figure out what should really happen here */
        sleep(10);
        /* temporarily put this here until I figure out what should really happen here */
#endif
SHUTDOWN:


        APP_IPRINT("%d :: App: Freeing the Memory Allocated in TestApp\n", __LINE__);

        APP_MEMPRINT("%d :: App: [TESTAPPFREE] %p\n", __LINE__, pAmrParam);

        if (pAmrParam != NULL) {
            newfree(pAmrParam);
            pAmrParam = NULL;
        }

        APP_MEMPRINT("%d :: App: [TESTAPPFREE] %p\n", __LINE__, pCompPrivateStruct);

        if (pCompPrivateStruct != NULL) {
            newfree(pCompPrivateStruct);
            pCompPrivateStruct = NULL;
        }

        APP_MEMPRINT("%d :: App: [TESTAPPFREE] %p\n", __LINE__, audioinfo);

        if (audioinfo != NULL) {
            newfree(audioinfo);
            audioinfo = NULL;
        }

        if (streaminfo != NULL) {
            newfree(streaminfo);
            streaminfo = NULL;
        }

        APP_IPRINT("%d :: App: Closing the Input and Output Pipes\n", __LINE__);
        eError = close (IpBuf_Pipe[0]);

        if (0 != eError && OMX_ErrorNone == eError) {
            eError = OMX_ErrorHardware;
            APP_DPRINT("%d :: Error while closing IpBuf_Pipe[0]\n", __LINE__);
            goto EXIT;
        }

        eError = close (IpBuf_Pipe[1]);

        if (0 != eError && OMX_ErrorNone == eError) {
            eError = OMX_ErrorHardware;
            APP_DPRINT("%d :: Error while closing IpBuf_Pipe[1]\n", __LINE__);
            goto EXIT;
        }

        eError = close (OpBuf_Pipe[0]);

        if (0 != eError && OMX_ErrorNone == eError) {
            eError = OMX_ErrorHardware;
            APP_DPRINT("%d :: Error while closing OpBuf_Pipe[0]\n", __LINE__);
            goto EXIT;
        }

        eError = close (OpBuf_Pipe[1]);

        if (0 != eError && OMX_ErrorNone == eError) {
            eError = OMX_ErrorHardware;
            APP_DPRINT("%d :: Error while closing OpBuf_Pipe[1]\n", __LINE__);
            goto EXIT;
        }


        eError = close(Event_Pipe[0]);

        if (0 != eError && OMX_ErrorNone == eError) {
            eError = OMX_ErrorHardware;
            APP_DPRINT("%d :: Error while closing Event_Pipe[0]\n", __LINE__);
            goto EXIT;
        }

        eError = close(Event_Pipe[1]);

        if (0 != eError && OMX_ErrorNone == eError) {
            eError = OMX_ErrorHardware;
            APP_DPRINT("%d :: Error while closing Event_Pipe[1]\n", __LINE__);
            goto EXIT;
        }


        APP_IPRINT("%d :: App: Free the Component handle\n", __LINE__);
        /* Unload the WBAMR Encoder Component */
        eError = TIOMX_FreeHandle(pHandle);

        if ((eError != OMX_ErrorNone)) {
            APP_DPRINT("%d :: Error in Free Handle function\n", __LINE__);
            goto EXIT;
        }




        APP_IPRINT("%d :: App: Free Handle returned Successfully\n", __LINE__);

        APP_DPRINT("%d :: Deleting %p\n", __LINE__, pCompPrivateStructGain);
        newfree(pCompPrivateStructGain);

#ifdef DSP_RENDERING_ON
        cmd_data.hComponent = pHandle;
        cmd_data.AM_Cmd = AM_Exit;

        if ((write(wbamrencfdwrite, &cmd_data, sizeof(cmd_data))) < 0)
            APP_IPRINT("%d ::- send command to audio manager\n", __LINE__);

        close(wbamrencfdwrite);
        close(wbamrencfdread);
#endif



    } /*Outer for loop ends here */



    APP_IPRINT("%d :: *********************************************************************\n", __LINE__);
    APP_IPRINT("%d :: NOTE: An output file %s has been created in file system\n", __LINE__, argv[2]);
    APP_IPRINT("%d :: *********************************************************************\n", __LINE__);

    eError = TIOMX_Deinit();

    if ( (eError != OMX_ErrorNone)) {
        APP_DPRINT("APP: Error in Deinit Core function\n");
        goto EXIT;
    }

    pthread_mutex_destroy(&WaitForState_mutex);
    pthread_cond_destroy(&WaitForState_threshold);

EXIT:

    if (bInvalidState == OMX_TRUE) {
#ifndef USE_BUFFER
        eError = FreeAllResources(pHandle,
                                  pInputBufferHeader[0],
                                  pOutputBufferHeader[0],
                                  numOfInputBuffer,
                                  numOfOutputBuffer,
                                  fIn,
                                  fOut);
#else
        eError = FreeAllResources(pHandle,
                                  pInputBuffer,
                                  pOutputBuffer,
                                  numOfInputBuffer,
                                  numOfOutputBuffer,
                                  fIn,
                                  fOut);
#endif
    }

#ifdef APP_DEBUGMEM
    APP_IPRINT("\n-Printing memory not deleted-\n");

    for (i = 0; i < 500; i++) {
        if (lines[i] != 0) {
            APP_IPRINT(" --->%d Bytes allocated on File:%s Line: %d\n", bytes[i], file[i], lines[i]);
        }
    }

#endif

#ifdef OMX_GETTIME
    GT_END("WBAMR_ENC test <End>");
    OMX_ListDestroy(pListHead);
#endif
    return eError;
}


OMX_ERRORTYPE send_input_buffer(OMX_HANDLETYPE pHandle, OMX_BUFFERHEADERTYPE* pBuffer, FILE *fIn) {
    OMX_ERRORTYPE error = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pComponent = (OMX_COMPONENTTYPE *)pHandle;

    if (FirstTime) {
        if (mframe) {
            nRead = fread(pBuffer->pBuffer, 1, WBAPP_INPUT_BUFFER_SIZE * 2, fIn);
        } else {
            nRead = fread(pBuffer->pBuffer, 1, WBAPP_INPUT_BUFFER_SIZE, fIn);
        }

        pBuffer->nFilledLen = nRead;
    } else {
        memcpy(pBuffer->pBuffer, NextBuffer, nRead);
        pBuffer->nFilledLen = nRead;
    }

    if (mframe) {
        nRead = fread(NextBuffer, 1, WBAPP_INPUT_BUFFER_SIZE * 2, fIn);
    } else {
        nRead = fread(NextBuffer, 1, WBAPP_INPUT_BUFFER_SIZE, fIn);
    }

    if (nRead < WBAPP_INPUT_BUFFER_SIZE && !DasfMode) {

#ifdef OMX_GETTIME
        GT_START();
#endif
        error = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
        error = WaitForState(pHandle, OMX_StateIdle);
#ifdef OMX_GETTIME
        GT_END("Call to SendCommand <OMX_StateIdle>");
#endif

        if (error != OMX_ErrorNone) {
            APP_DPRINT ("%d :: Error from SendCommand-Idle(Stop) State function\n", __LINE__);
            goto EXIT;
        }

        pBuffer->nFlags = OMX_BUFFERFLAG_EOS;
    } else {
        pBuffer->nFlags = 0;
    }

    if (pBuffer->nFilledLen != 0) {
        if (pBuffer->nFlags == OMX_BUFFERFLAG_EOS) {
            APP_IPRINT("Sending Last Input Buffer from App\n");
        }

        pBuffer->nTimeStamp = rand() % 100;

        if (!preempted) {
            error = pComponent->EmptyThisBuffer(pHandle, pBuffer);

            if (error == OMX_ErrorIncorrectStateOperation)
                error = 0;
        }
    }

    FirstTime = 0;
EXIT:
    return error;
}
OMX_ERRORTYPE StopComponent(OMX_HANDLETYPE *pHandle) {
    OMX_ERRORTYPE error = OMX_ErrorNone;
#ifdef OMX_GETTIME
    GT_START();
#endif

    APP_IPRINT("%d :: APP: Sending Stop.........From APP \n", __LINE__);

    error = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);

    if (error != OMX_ErrorNone) {
        fprintf(stderr, "\nError from SendCommand-Idle(Stop) State function!!!!!!!!\n");
        goto EXIT;
    }

    error = WaitForState(pHandle, OMX_StateIdle);
#ifdef OMX_GETTIME
    GT_END("Call to SendCommand <OMX_StateIdle>");
#endif

    if (error != OMX_ErrorNone) {
        fprintf(stderr, "\nError:  WaitForState reports an error %X!!!!!!!\n", error);
        goto EXIT;
    }

EXIT:
    return error;
}

OMX_ERRORTYPE PauseComponent(OMX_HANDLETYPE *pHandle) {
    OMX_ERRORTYPE error = OMX_ErrorNone;
#ifdef OMX_GETTIME
    GT_START();
#endif
    error = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StatePause, NULL);

    if (error != OMX_ErrorNone) {
        fprintf (stderr, "\nError from SendCommand-Pause State function!!!!!!\n");
        goto EXIT;
    }

    error = WaitForState(pHandle, OMX_StatePause);

#ifdef OMX_GETTIME
    GT_END("Call to SendCommand <OMX_StatePause>");
#endif

    if (error != OMX_ErrorNone) {
        fprintf(stderr, "\nError:  WaitForState reports an error %X!!!!!!!\n", error);
        goto EXIT;
    }

EXIT:
    return error;
}

OMX_ERRORTYPE PlayComponent(OMX_HANDLETYPE *pHandle) {
    OMX_ERRORTYPE error = OMX_ErrorNone;
#ifdef OMX_GETTIME
    GT_START();
#endif
    error = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);

    if (error != OMX_ErrorNone) {
        fprintf (stderr, "\nError from SendCommand-Executing State function!!!!!!!\n");
        goto EXIT;
    }

    error = WaitForState(pHandle, OMX_StateExecuting);
#ifdef OMX_GETTIME
    GT_END("Call to SendCommand <OMX_StateExecuting>");
#endif

    if (error != OMX_ErrorNone) {
        fprintf(stderr, "\nError:  WaitForState reports an error %X!!!!!!!\n", error);
        goto EXIT;
    }

EXIT:
    return error;
}
/*=================================================================

                            Freeing All allocated resources

==================================================================*/
#ifndef USE_BUFFER
int FreeAllResources( OMX_HANDLETYPE *pHandle,
                      OMX_BUFFERHEADERTYPE* pBufferIn,
                      OMX_BUFFERHEADERTYPE* pBufferOut,
                      int NIB, int NOB,
                      FILE* fileIn, FILE* fileOut) {
    APP_IPRINT("%d::Freeing all resources by state invalid \n", __LINE__);
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U16 i;

    if (!DasfMode) {
        for (i = 0; i < NIB; i++) {
            APP_IPRINT("%d :: APP: About to free pInputBufferHeader[%d]\n", __LINE__, i);
            eError = OMX_FreeBuffer(pHandle, OMX_DirInput, pBufferIn + i);

        }
    }

    for (i = 0; i < NOB; i++) {
        APP_IPRINT("%d :: APP: About to free pOutputBufferHeader[%d]\n", __LINE__, i);
        eError = OMX_FreeBuffer(pHandle, OMX_DirOutput, pBufferOut + i);
    }

    /*i value is fixed by the number calls to malloc in the App */
    for (i = 0; i < 5; i++) {
        if (ArrayOfPointers[i] != NULL)
            free(ArrayOfPointers[i]);
    }

    TIOMX_FreeHandle(pHandle);

    return eError;
}


/*=================================================================

                            Freeing the resources with USE_BUFFER define

==================================================================*/
#else

int FreeAllResources(OMX_HANDLETYPE *pHandle,
                     OMX_U8* UseInpBuf[],
                     OMX_U8* UseOutBuf[],
                     int NIB, int NOB,
                     FILE* fileIn, FILE* fileOut) {

    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U16 i;
    APP_IPRINT("%d::Freeing all resources by state invalid \n", __LINE__);

    /* free the UseBuffers */
    for (i = 0; i < NIB; i++) {
        UseInpBuf[i] = UseInpBuf[i] - 128;
        APP_IPRINT("%d :: [TESTAPPFREE] pInputBuffer[%d] = %p\n", __LINE__, i, (UseInpBuf[i]));

        if (UseInpBuf[i] != NULL) {
            newfree(UseInpBuf[i]);
            UseInpBuf[i] = NULL;
        }
    }

    for (i = 0; i < NOB; i++) {
        UseOutBuf[i] = UseOutBuf[i] - 128;
        APP_IPRINT("%d :: [TESTAPPFREE] pOutputBuffer[%d] = %p\n", __LINE__, i, UseOutBuf[i]);

        if (UseOutBuf[i] != NULL) {
            newfree(UseOutBuf[i]);
            UseOutBuf[i] = NULL;
        }
    }

    /*i value is fixed by the number calls to malloc in the App */
    for (i = 0; i < 5; i++) {
        if (ArrayOfPointers[i] != NULL)
            free(ArrayOfPointers[i]);
    }

    OMX_FreeHandle(pHandle);

    return eError;
}

#endif
