
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
* @file G726Enc_Test.c
*
* This file implements G726 Encoder Component Test Application to verify
* which is fully compliant with the Khronos OpenMAX (TM) 1.0 Specification
*
* @path  $(CSLPATH)\OMAPSW_MPU\linux\audio\src\openmax_il\g726_enc\tests
*
* @rev  1.0
*/
/* ----------------------------------------------------------------------------
*!
*! Revision History
*! ===================================
*! Gyancarlo Garcia: Initial Verision
*! 05-Oct-2007
*! 
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
#include <time.h>
#ifdef OMX_GETTIME
    #include <OMX_Common_Utils.h>
    #include <OMX_GetTime.h>     /*Headers for Performance & measuremet    */
#endif
int preempted=0;

#undef  WAITFORRESOURCES
#define MAX_CYCLES  20
#undef  FLUSHINPAUSE

FILE *fpRes = NULL;

/* ======================================================================= */
/**
 * @def G726ENC_INPUT_BUFFER_SIZE        Default input buffer size
 */
/* ======================================================================= */
#define G726ENC_INPUT_BUFFER_SIZE  16
/*16 = 8 input samples of 2 bytes each*/
/* ======================================================================= */
/**
 * @def G726ENC_INPUT_BUFFER_SIZE_DASF     Default input buffer size for DASF tests
 */
/* ======================================================================= */
#define G726ENC_INPUT_BUFFER_SIZE_DASF 80

/* ======================================================================= */
/**
 * @def G726ENC_OUTPUT_BUFFER_SIZE   Default output buffer size
 */
/* ======================================================================= */
#define G726ENC_OUTPUT_BUFFER_SIZE 60 
/*60 was choseen since is multiple of 2,3,4 and 5, that are the 
                                         smallest frame size for 16kps, 24kbps, 32kbps and 40kbps respectively, 
                                         so the same outputSize can be used for all the bitrates*/

/* ======================================================================= */
/*
 * @def    G726ENC_APP_ID  App ID Value setting
 */
/* ======================================================================= */
#define G726ENC_APP_ID 100

#define HALF_FRAME 1
#define TWO_FRAMES 2

#define SLEEP_TIME 5

#define FIFO1 "/dev/fifo.1"
#define FIFO2 "/dev/fifo.2"
#undef APP_DEBUG

#undef APP_MEMCHECK

/*#define USE_BUFFER*/
#undef USE_BUFFER

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

/* ======================================================================= */
/**
 * @def    APP_DEBUGMEM   Turns memory leaks messaging on and off.
 *         NBAMRENC_DEBUGMEM must be defined in OMX Comp in order to get
 *         this functionality On.
 */
/* ======================================================================= */
#undef APP_DEBUGMEM
/*#define APP_DEBUGMEM*/

#ifdef APP_DEBUGMEM
void *arr[500] = {NULL};
int lines[500] = {0};
int bytes[500] = {0};
char file[500][50]  = {""};
int ind=0;

#define SafeMalloc(x) DebugMalloc(__LINE__,__FILE__,x)
#define SafeFree(z) DebugFree(z,__LINE__,__FILE__)

void * DebugMalloc(int line, char *s, int size)
{
   void *p = NULL;    
   int e=0;
   p = calloc(1,size);
   if(p==NULL){
       printf("__ Memory not available\n");
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
         printf("__ Allocating %d bytes on address %p, line %d file %s\n", size, p, line, s);
         return p;
   }
}

int DebugFree(void *dp, int line, char *s){
    int q = 0;
    if(dp==NULL){
                 printf("__ NULL can't be deleted\n");
                 return 0;
    }
    for(q=0;q<500;q++){
        if(arr[q]==dp){
           printf("__ Deleting %d bytes on address %p, line %d file %s\n", bytes[q],dp, line, s);
           free(dp);
           dp = NULL;
           lines[q]=0;
           strcpy(file[q],"");
           break;
        }            
     }    
     if(500==q)
         printf("\n\__ nPointer not found. Line:%d    File%s!!\n\n",line, s);
}
#else
#define SafeMalloc(x) calloc(1,x)
#define SafeFree(z) free(z)
#endif


/* ======================================================================= */
/**
 *  M A C R O S FOR MALLOC and MEMORY FREE and CLOSING PIPES
 */
/* ======================================================================= */

#define OMX_NBAPP_CONF_INIT_STRUCT(_s_, _name_)    \
    memset((_s_), 0x0, sizeof(_name_));    \
    (_s_)->nSize = sizeof(_name_);        \
    (_s_)->nVersion.s.nVersionMajor = 0x1;    \
    (_s_)->nVersion.s.nVersionMinor = 0x0;    \
    (_s_)->nVersion.s.nRevision = 0x0;        \
    (_s_)->nVersion.s.nStep = 0x0

#define OMX_NBAPP_INIT_STRUCT(_s_, _name_)    \
    memset((_s_), 0x0, sizeof(_name_));    \

#define OMX_NBAPP_MALLOC_STRUCT(_pStruct_, _sName_)   \
    _pStruct_ = (_sName_*)SafeMalloc(sizeof(_sName_));      \
    if(_pStruct_ == NULL){      \
        printf("***********************************\n"); \
        printf("%d :: Malloc Failed\n",__LINE__); \
        printf("***********************************\n"); \
        eError = OMX_ErrorInsufficientResources; \
        goto EXIT;      \
    } \
    APP_MEMPRINT("%d :: ALLOCATING MEMORY = %p\n",__LINE__,_pStruct_);


/* ======================================================================= */
/**
 * @def NBAPP_MAX_NUM_OF_BUFS       Maximum number of buffers
 * @def    NBAPP_NUM_OF_CHANNELS         Number of Channels
 * @def NBAPP_SAMPLING_FREQUENCY    Sampling frequency
 */
/* ======================================================================= */
#define NBAPP_MAX_NUM_OF_BUFS 10
#define NBAPP_NUM_OF_CHANNELS 1
#define NBAPP_SAMPLING_FREQUENCY 8000

typedef struct AUDIO_INFO {
    OMX_U32 acdnMode;
    OMX_U32 dasfMode;
    OMX_U32 nIpBufs;
    OMX_U32 nIpBufSize;
    OMX_U32 nOpBufs;
    OMX_U32 nOpBufSize;
    OMX_U32 nMFrameMode;
} AUDIO_INFO;

static OMX_BOOL bInvalidState;

OMX_ERRORTYPE StopComponent(OMX_HANDLETYPE *pHandle);
OMX_ERRORTYPE PauseComponent(OMX_HANDLETYPE *pHandle);
OMX_ERRORTYPE PlayComponent(OMX_HANDLETYPE *pHandle);
OMX_ERRORTYPE send_input_buffer(OMX_HANDLETYPE pHandle, OMX_BUFFERHEADERTYPE* pBuffer, FILE *fIn);

int maxint(int a, int b);

int inputPortDisabled = 0;
int outputPortDisabled = 0;

OMX_STRING strG726Encoder = "OMX.TI.G726.encode";

int IpBuf_Pipe[2] = {0};
int OpBuf_Pipe[2] = {0};
int Event_Pipe[2] = {0};

fd_set rfds;
int done = 0;
int FirstTime = 1;
int mframe=0;
int nRead = 0;
int DasfMode =0;
unsigned long int vez=0;

OMX_U8 NextBuffer[G726ENC_INPUT_BUFFER_SIZE*3];
OMX_U32 fdwrite = 0;
OMX_U32 fdread = 0;        

pthread_mutex_t WaitForState_mutex;
pthread_cond_t  WaitForState_threshold;
OMX_U8          WaitForState_flag = 0;
OMX_U8		TargetedState = 0;  

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
     OMX_COMPONENTTYPE *pComponent = (OMX_COMPONENTTYPE *)pHandle;
     eError = OMX_GetState(pComponent, &CurState);
     if (CurState == OMX_StateInvalid && bInvalidState == OMX_TRUE)
	 {
		 eError = OMX_ErrorInvalidState;
	 }
     if(CurState != DesiredState){
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
        OMX_PTR pEventData)
{
   APP_DPRINT( "%d :: App: Entering EventHandler \n", __LINE__);
   OMX_ERRORTYPE eError = OMX_ErrorNone;
   OMX_COMPONENTTYPE *pComponent = (OMX_COMPONENTTYPE *)hComponent;
   OMX_STATETYPE state = OMX_StateInvalid;
   OMX_U8 writeValue = 0;

   eError = OMX_GetState(pComponent, &state);
   if(eError != OMX_ErrorNone) {
       APP_DPRINT("%d :: App: Error returned from OMX_GetState\n",__LINE__);
       goto EXIT;
   }
   APP_DPRINT( "%d :: App: Component eEvent = %d\n", __LINE__,eEvent);
   switch (eEvent) {
       APP_DPRINT( "%d :: App: Component State Changed To %d\n", __LINE__,state);
       case OMX_EventResourcesAcquired:
            writeValue = 1;
            write(Event_Pipe[1], &writeValue, sizeof(OMX_U8));
            preempted=0;
       break;       
       case OMX_EventCmdComplete:
           APP_DPRINT( "%d :: App: Component State Changed To %d\n", __LINE__,state);
        if (nData1 == OMX_CommandPortDisable) {
            if (nData2 == OMX_DirInput) {
                inputPortDisabled = 1;
            }
            if (nData2 == OMX_DirOutput) {
                outputPortDisabled = 1;
            }
        }
        if ((nData1 == OMX_CommandStateSet) && (TargetedState == nData2) && (WaitForState_flag)){
                        WaitForState_flag = 0;
                        pthread_mutex_lock(&WaitForState_mutex);
                        pthread_cond_signal(&WaitForState_threshold);/*Sending Waking Up Signal*/
                        pthread_mutex_unlock(&WaitForState_mutex);
                }             
        if (nData1 == OMX_CommandFlush){
            if (nData2 == OMX_DirInput) {
                printf ("App: EventHandler - Flush completed on input port\n");
            }
            else if (nData2 == OMX_DirOutput) {
                printf ("App: EventHandler - Flush completed on output port\n");
            }                        
        }                     
           break;
       case OMX_EventError:
		   if (nData1 == OMX_ErrorInvalidState) {
		   		bInvalidState =OMX_TRUE;
		   }            
		   else if(nData1 == OMX_ErrorResourcesPreempted) {
                      preempted=1;

		      writeValue = 0;  
                    write(Event_Pipe[1], &writeValue, sizeof(OMX_U8));
		   }
		   else if (nData1 == OMX_ErrorResourcesLost) { 
                    WaitForState_flag = 0;
                    pthread_mutex_lock(&WaitForState_mutex);
                    pthread_cond_signal(&WaitForState_threshold);/*Sending Waking Up Signal*/
                    pthread_mutex_unlock(&WaitForState_mutex);            
           }            
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
#ifdef WAITFORRESOURCES              
            writeValue = 2;  
            write(Event_Pipe[1], &writeValue, sizeof(OMX_U8));
#endif              
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
    if (!preempted)     
        write(IpBuf_Pipe[1], &pBuffer, sizeof(pBuffer));
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
    OMX_CALLBACKTYPE G726CaBa = {(void *)EventHandler,
                (void*)EmptyBufferDone,
                                (void*)FillBufferDone};
    OMX_HANDLETYPE pHandle;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 AppData = G726ENC_APP_ID;
    OMX_PARAM_PORTDEFINITIONTYPE* pCompPrivateStruct = NULL;
    OMX_AUDIO_PARAM_G726TYPE *pG726Param = NULL;
    
    OMX_COMPONENTTYPE *pComponent = NULL;
    OMX_STATETYPE state = OMX_StateInvalid;
    OMX_BUFFERHEADERTYPE* pInputBufferHeader[NBAPP_MAX_NUM_OF_BUFS] = {NULL};
    OMX_BUFFERHEADERTYPE* pOutputBufferHeader[NBAPP_MAX_NUM_OF_BUFS] = {NULL};
#ifdef USE_BUFFER
    OMX_U8* pInputBuffer[NBAPP_MAX_NUM_OF_BUFS] = {NULL};
    OMX_U8* pOutputBuffer[NBAPP_MAX_NUM_OF_BUFS] = {NULL};
#endif
    TI_OMX_DSP_DEFINITION* audioinfo = NULL;

    FILE* fIn = NULL;
    FILE* fOut = NULL;
    struct timeval tv;
    int retval, i = 0, j = 0, k = 0, kk = 0, tcID = 0;
    int frmCount = 0;
    int frmCnt = 1;
    int testcnt = 0;
    int testcnt1 = 0;
    int fdmax = 0;
    int nFrameCount = 1;
    int nFrameLen = 0;
    int nIpBuff = 1;
    int nOutBuff = 1;
    OMX_INDEXTYPE index = 0;
    int status = 0;
    int    NoDataRead = 0;
    int numInputBuffers=0,numOutputBuffers=0;
    OMX_AUDIO_CONFIG_VOLUMETYPE* pCompPrivateStructGain = NULL;
    TI_OMX_DATAPATH dataPath;
    TI_OMX_STREAM_INFO *streaminfo=NULL;
    
    bInvalidState=OMX_FALSE;
    
    pthread_mutex_init(&WaitForState_mutex, NULL);
    pthread_cond_init (&WaitForState_threshold, NULL);
    WaitForState_flag = 0;        

    printf("------------------------------------------------------\n");
    printf("This is Main Thread In G726 ENCODER Test Application:\n");
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
    if(argc < 9) {
        printf("%d :: Usage: [TestApp][Input File][Output File][FUNC_ID_X][FM/DM][ACDN_ON/ACDN_OFF][16/24/3240][# INPUT BUF][# OUTPUT BUF][# REP]\n",__LINE__);
        goto EXIT;
    }

    /* check to see that the input file exists */
    struct stat sb = {0};
    status = stat(argv[1], &sb);
    if( status != 0 ) {
        printf("Cannot find file %s (%u)\n", argv[1], errno);
        goto EXIT;
    }

    /* Open the file of data to be encoded. */    
    printf("%d :: App: Input file is %s\n", __LINE__, argv[1]);
    fIn = fopen(argv[1], "r");
    if( fIn == NULL ) {
            APP_DPRINT("Error:  failed to open the input file %s\n", argv[1]);
            goto EXIT;
    }

    /* Open the file of data to be written. */
    printf("%d :: App: g726 output file is %s\n", __LINE__, argv[2]);
    fOut = fopen(argv[2], "w");
    if( fOut == NULL ) {
            APP_DPRINT("Error:  failed to open the output file %s\n", argv[2]);
            goto EXIT;
    }

    if(!strcmp(argv[3],"FUNC_ID_1")) {
        printf("%d :: ### TESTCASE 1: Playing component till EOF ###\n",__LINE__);
        testcnt = 1;
        testcnt1 = 1;
        tcID = 1;
    } else if(!strcmp(argv[3],"FUNC_ID_2")) {
        printf("%d :: ### TESTCASE 2: Stoping component at the middle of stream ###\n",__LINE__);
        testcnt = 1;
        testcnt1 = 1;
        tcID = 2;
    } else if(!strcmp(argv[3],"FUNC_ID_3")) {
        printf("%d :: ### TESTCASE 3: Runing pause & resume test ###\n",__LINE__);
        testcnt = 1;
        testcnt1 = 1;
        tcID = 3;
    } else if(!strcmp(argv[3],"FUNC_ID_4")) {
        printf("%d :: ### TESTCASE 4: Runing stop & resume test ###\n",__LINE__);
        testcnt = 2;
        testcnt1 = 1;
        tcID = 4;
        printf("######## testcnt = %d #########\n",testcnt);
    }
    if(!strcmp(argv[3],"FUNC_ID_5")){
        printf("%d :: ### TESTCASE 5: Repetitive Test without deleting the component ###\n",__LINE__);
        if (argc > 9)
        {
            testcnt = (int) atoi (argv[9]);
        }
        else
        {
            testcnt = MAX_CYCLES;
        }
        testcnt1 = 1;
        tcID = 5;
    }

    if(!strcmp(argv[3],"FUNC_ID_6")) {
        printf("%d :: ### TESTCASE 6: Repetitive Test with Deleting the component ###\n",__LINE__);
        testcnt = 1;
        if (argc > 9)
        {
            testcnt1 = (int) atoi( argv[9]);
        }
        else
        {
            testcnt1 = MAX_CYCLES;
        }
        tcID = 6;
    }

    if(!strcmp(argv[3],"FUNC_ID_7")) {
        printf("%d :: ### TESTCASE 7: Testing Component by sending 2 frames by buffer ###\n",__LINE__);
        testcnt = 1;
        testcnt1 = 1;
        tcID = 1;
        mframe = TWO_FRAMES;
    }
   
    if(!strcmp(argv[3],"FUNC_ID_8")) {
        printf("%d :: ### TESTCASE 8: Testing Component by sending 1/2 frame by buffer ###\n",__LINE__);
        testcnt = 1;
        testcnt1 = 1;
        tcID = 1;
        mframe = HALF_FRAME;
    }

    if(!strcmp(argv[3],"FUNC_ID_9")) {
        printf("%d :: ### TESTCASE 8: Runing Component with Volume Test ###\n",__LINE__);
        testcnt = 1;
        testcnt1 = 1;
        tcID = 1;
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
        
        retval = pipe(Event_Pipe);
        if(retval != 0) {
            APP_DPRINT( "Error:Empty Event Pipe failed to open\n");
            goto EXIT;
        }           
        /* save off the "max" of the handles for the selct statement */
        fdmax = maxint(IpBuf_Pipe[0], OpBuf_Pipe[0]);
        fdmax = maxint(fdmax,Event_Pipe[0]);

        eError = TIOMX_Init();

        if(eError != OMX_ErrorNone) {
            APP_DPRINT("%d :: Error returned by OMX_Init()\n",__LINE__);
            goto EXIT;
        }

        if(j >= 1) {
            printf ("%d :: Encoding the file in TESTCASE 6\n",__LINE__);
            fIn = fopen(argv[1], "r");
            if( fIn == NULL ) {
                fprintf(stderr, "Error:  failed to open the file %s for read only access\n",argv[1]);
                goto EXIT;
            }
    
            fOut = fopen("TC6_G7261.G726", "w");
            if( fOut == NULL ) {
                fprintf(stderr, "Error:  failed to create the output file %s\n",argv[2]);
                goto EXIT;
            }
         }

        /* Load the G726 Encoder Component */
	#ifdef OMX_GETTIME
		GT_START();
        eError = TIOMX_GetHandle(&pHandle, strG726Encoder, &AppData, &G726CaBa);
		GT_END("Call to GetHandle");
	#else 
        eError = TIOMX_GetHandle(&pHandle, strG726Encoder, &AppData, &G726CaBa);
	#endif
        if((eError != OMX_ErrorNone) || (pHandle == NULL)) {
            APP_DPRINT("Error in Get Handle function\n");
            printf("Error in Get Handle function\n");
            goto EXIT;
        }

        OMX_NBAPP_MALLOC_STRUCT(audioinfo, TI_OMX_DSP_DEFINITION);
        OMX_NBAPP_INIT_STRUCT(audioinfo, TI_OMX_DSP_DEFINITION);

        /* Setting Input and Output Buffers features for the Component */
        numInputBuffers = atoi(argv[7]);
        printf("%d :: App: number of input buffers = %d \n",__LINE__,numInputBuffers);        
        printf("%d :: App: input buffer size = %d \n",__LINE__,G726ENC_INPUT_BUFFER_SIZE);

        numOutputBuffers = atoi(argv[8]);
        printf("%d :: App: number of output buffers = %d \n",__LINE__,numOutputBuffers);
        printf("%d :: App: output buffer size = %d \n",__LINE__,G726ENC_OUTPUT_BUFFER_SIZE);

        OMX_NBAPP_MALLOC_STRUCT(pCompPrivateStruct, OMX_PARAM_PORTDEFINITIONTYPE);
        OMX_NBAPP_CONF_INIT_STRUCT(pCompPrivateStruct, OMX_PARAM_PORTDEFINITIONTYPE);

        OMX_NBAPP_MALLOC_STRUCT(pG726Param, OMX_AUDIO_PARAM_G726TYPE);
        OMX_NBAPP_CONF_INIT_STRUCT(pG726Param, OMX_AUDIO_PARAM_G726TYPE);

        APP_DPRINT("%d :: Setting input port config\n",__LINE__);
        pCompPrivateStruct->nSize                              = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
        pCompPrivateStruct->nVersion.s.nVersionMajor           = 0xF1;
        pCompPrivateStruct->nVersion.s.nVersionMinor           = 0xF2;
        pCompPrivateStruct->nPortIndex                         = OMX_DirInput;
        pCompPrivateStruct->eDir                               = OMX_DirInput;
        pCompPrivateStruct->nBufferCountActual                 = numInputBuffers;
        pCompPrivateStruct->nBufferCountMin                    = numInputBuffers;
        pCompPrivateStruct->nBufferSize                        = G726ENC_INPUT_BUFFER_SIZE;
        pCompPrivateStruct->bEnabled                           = OMX_TRUE;
        pCompPrivateStruct->bPopulated                         = OMX_FALSE;
        pCompPrivateStruct->eDomain                            = OMX_PortDomainAudio;
        pCompPrivateStruct->format.audio.eEncoding             = OMX_AUDIO_CodingG726;
        pCompPrivateStruct->format.audio.pNativeRender         = NULL;
        pCompPrivateStruct->format.audio.bFlagErrorConcealment = OMX_FALSE;    /*Send input port config*/

        if(!(strcmp(argv[4],"FM"))) {
            audioinfo->dasfMode = 0;
            APP_DPRINT("\n%d :: App: audioinfo->dasfMode = %ld \n",__LINE__,audioinfo->dasfMode);
        } else if(!(strcmp(argv[4],"DM"))){
            audioinfo->dasfMode = 1;
            DasfMode = 1;
            APP_DPRINT("\n%d :: App: audioinfo->dasfMode = %ld \n",__LINE__,audioinfo->dasfMode);
            APP_DPRINT("%d :: G726 ENCODER RUNNING UNDER DASF MODE \n",__LINE__);
            pCompPrivateStruct->nBufferCountActual = 0;
            pCompPrivateStruct->nBufferSize = G726ENC_INPUT_BUFFER_SIZE_DASF;
        } else {
            eError = OMX_ErrorBadParameter;
            printf("\n%d :: App: audioinfo->dasfMode Sending Bad Parameter\n",__LINE__);
            printf("%d :: App: Should Be One of these Modes FM, DM\n",__LINE__);
            goto EXIT;
        }


        if (argc == 11)
        {
            /* Application wants to decide upon rtp */
            audioinfo->rtpMode = atoi (argv[10]);

        }
        else
        {
            /* if user hasn't decided, let's go linear */
            audioinfo->rtpMode = 0;
        }

        if(audioinfo->dasfMode) { /*Dasf Mode*/
            if((atoi(argv[7])) != 0) {
                eError = OMX_ErrorBadParameter;             
                printf("%d :: App: For DASF mode argv[7] Should Be --> 0\n",__LINE__);
                goto EXIT;
            }
        } else {
            if((atoi(argv[7])) == 0) {  /*File Mode*/
                eError = OMX_ErrorBadParameter;
                printf("%d :: App: For FILE mode argv[7] Should be greater than zero depends on number of buffers user want to encode\n",__LINE__);
                goto EXIT;
            }
        }

        if(!(strcmp(argv[5],"ACDN_OFF"))) {
              audioinfo->acousticMode = 0;
              APP_DPRINT("\n%d :: App: audioinfo->acdnMode = %ld \n",__LINE__,audioinfo->acousticMode);
        } else if(!(strcmp(argv[5],"ACDN_ON"))) {
              audioinfo->acousticMode = 1;
              APP_DPRINT("\n%d :: App: audioinfo->acdnMode = %ld \n",__LINE__,audioinfo->acousticMode);
        } else {
              eError = OMX_ErrorBadParameter;
              printf("\n%d :: App: audioinfo->acdnMode Sending Bad Parameter\n",__LINE__);
              printf("%d :: App: Should Be One of these Modes ACDN_ON, ACDN_OFF, not %s\n",__LINE__, argv[5]);
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
            goto EXIT;
        }

        APP_MEMPRINT("%d :: Setting output port config\n",__LINE__);
        pCompPrivateStruct->nSize                              = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
        pCompPrivateStruct->nVersion.s.nVersionMajor           = 0xF1;
        pCompPrivateStruct->nVersion.s.nVersionMinor           = 0xF2;
        pCompPrivateStruct->nPortIndex                         = OMX_DirOutput;
        pCompPrivateStruct->eDir                               = OMX_DirOutput;
        pCompPrivateStruct->nBufferCountActual                 = numOutputBuffers;
        pCompPrivateStruct->nBufferCountMin                    = numOutputBuffers;
        pCompPrivateStruct->nBufferSize                        = G726ENC_OUTPUT_BUFFER_SIZE;
        pCompPrivateStruct->bEnabled                           = OMX_TRUE;
        pCompPrivateStruct->bPopulated                         = OMX_FALSE;
        pCompPrivateStruct->eDomain                            = OMX_PortDomainAudio;
        pCompPrivateStruct->format.audio.eEncoding             = OMX_AUDIO_CodingG726;
        pCompPrivateStruct->format.audio.pNativeRender         = NULL;
        pCompPrivateStruct->format.audio.bFlagErrorConcealment = OMX_FALSE;    /*Send output port config*/
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
            goto EXIT;
        }

        if(!(strcmp(argv[6],"16"))) {
              pG726Param->eG726Mode = OMX_AUDIO_G726Mode16;
              APP_DPRINT("\n%d :: App: eBitRate set to 16 kbps\n",__LINE__);
        } 
        else if(!(strcmp(argv[6],"24"))) {
              pG726Param->eG726Mode = OMX_AUDIO_G726Mode24;
              APP_DPRINT("\n%d :: App: eBitRate set to 24 kbps\n",__LINE__);
        }
        else if(!(strcmp(argv[6],"32"))) {
              pG726Param->eG726Mode = OMX_AUDIO_G726Mode32;
              APP_DPRINT("\n%d :: App: eBitRate set to 32 kbps\n",__LINE__);
        }        
        else if(!(strcmp(argv[6],"40"))) {
              pG726Param->eG726Mode = OMX_AUDIO_G726Mode40;
              APP_DPRINT("\n%d :: App: eBitRate set to 40 kbps\n",__LINE__);
        }         
        else{
              pG726Param->eG726Mode = OMX_AUDIO_G726ModeUnused;
              APP_DPRINT("\n%d :: App: UnSuported bit Rate, setting to OMX_AUDIO_G726ModeUnused\n",__LINE__);
         }

        /* Send  G726 config for output */
        pG726Param->nSize                    = sizeof(OMX_AUDIO_PARAM_G726TYPE);
        pG726Param->nVersion.s.nVersionMajor = 0xF1;
        pG726Param->nVersion.s.nVersionMinor = 0xF2;
        pG726Param->nPortIndex               = OMX_DirOutput;
        pG726Param->nChannels                = NBAPP_NUM_OF_CHANNELS;
	#ifdef OMX_GETTIME
		GT_START();
        eError = OMX_SetParameter (pHandle, OMX_IndexParamAudioG726, pG726Param);
		GT_END("Set Parameter Test-SetParameter");
	#else
        eError = OMX_SetParameter (pHandle, OMX_IndexParamAudioG726, pG726Param);
	#endif
        if (eError != OMX_ErrorNone) {
            eError = OMX_ErrorBadParameter;
            APP_DPRINT("%d :: OMX_ErrorBadParameter\n",__LINE__);
            goto EXIT;
        }
        
#ifdef DSP_RENDERING_ON 
    if (audioinfo->dasfMode){
          if((fdwrite=open(FIFO1,O_WRONLY))<0) {
         	   APP_DPRINT("%d :: [G726E Component] - failure to open WRITE pipe\n",__LINE__);
          }
          if((fdread=open(FIFO2,O_RDONLY))<0) {
      	       APP_DPRINT("%d :: [G726E Component] - failure to open READ pipe\n",__LINE__);
          }
          APP_DPRINT("%d :: OMX_ComponentInit\n", __LINE__);

         cmd_data.hComponent = pHandle;
         cmd_data.AM_Cmd = AM_CommandIsInputStreamAvailable;
         cmd_data.param1 = 0;

         if((write(fdwrite, &cmd_data, sizeof(cmd_data)))<0) {
             APP_DPRINT("%d :: [G726E Component] - send command to audio manager\n",__LINE__);
         }
         if((read(fdread, &cmd_data, sizeof(cmd_data)))<0) {
      	     printf("%d :: [G726E Component] - failure to get data from the audio manager!!! \n",__LINE__);
   	         goto EXIT;
         }
    	 if(cmd_data.streamID == 0) {
		     printf("%d :: [G726E Component] - no input stream available!!!!!!!\n",__LINE__);
		     eError = OMX_ErrorInsufficientResources;
		      goto EXIT;
        
	     } else {
		       APP_DPRINT("%d :: [G726E Component] - input stream available\n",__LINE__);
		       audioinfo->streamId=cmd_data.streamID;
		       eError = OMX_ErrorNone;
    	}         
    }
#endif
    
    eError = OMX_GetExtensionIndex(pHandle, "OMX.TI.index.config.tispecific",&index);
        if (eError != OMX_ErrorNone) {
            APP_DPRINT("Error returned from OMX_GetExtensionIndex\n");
            goto EXIT;
        }

    eError = OMX_SetConfig (pHandle, index, audioinfo);
        if(eError != OMX_ErrorNone) {
            eError = OMX_ErrorBadParameter;
            APP_DPRINT("%d :: Error from OMX_SetConfig() function\n",__LINE__);
            goto EXIT;
    }     
    if (audioinfo->acousticMode == OMX_TRUE) {
        printf("Using Acoustic Device Node Path\n");
        dataPath = DATAPATH_ACDN;
    }
    else if (audioinfo->dasfMode) {
#ifdef RTM_PATH  
        printf("Using Real Time Mixer Path\n");
        dataPath = DATAPATH_APPLICATION_RTMIXER;
#endif

#ifdef ETEEDN_PATH
        printf("Using Eteeden Path\n");
        dataPath = DATAPATH_APPLICATION;
#endif        
    }
    eError = OMX_GetExtensionIndex(pHandle, "OMX.TI.index.config.G726.datapath",&index);
	if (eError != OMX_ErrorNone) {
		printf("Error getting extension index\n");
		goto EXIT;
	}
	eError = OMX_SetConfig (pHandle, index, &dataPath);
    if(eError != OMX_ErrorNone) {
        eError = OMX_ErrorBadParameter;
        APP_DPRINT("%d :: AmrDecTest.c :: Error from OMX_SetConfig() function\n",__LINE__);
        goto EXIT;
    }       
    
        if (audioinfo->dasfMode ==  1) { 
             /* get streamID back to application */
             OMX_NBAPP_MALLOC_STRUCT(streaminfo, TI_OMX_STREAM_INFO);
        
     	    eError = OMX_GetExtensionIndex(pHandle, "OMX.TI.index.config.G726.streamIDinfo",&index);
          	if (eError != OMX_ErrorNone) {
      	     	printf("Error getting extension index\n");
        		goto EXIT;
          	}
           
           	eError = OMX_GetConfig (pHandle, index, streaminfo);
            if(eError != OMX_ErrorNone) {
                 eError = OMX_ErrorBadParameter;
                 APP_DPRINT("%d :: AmrEncTest.c :: Error from OMX_GetConfig() function\n",__LINE__);
                 goto EXIT;
            }
            printf("***************StreamId=%ld******************\n", streaminfo->streamId);
       }
           
        APP_DPRINT("%d Sending Component to OMX_StateIdle\n",__LINE__);
	#ifdef OMX_GETTIME
		GT_START();
	#endif
        eError = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
        if(eError != OMX_ErrorNone) {
            APP_DPRINT("Error from SendCommand-Idle(Init) State function\n");
            goto EXIT;
        }
        
#ifndef USE_BUFFER
        APP_DPRINT("%d :: About to call OMX_AllocateBuffer\n",__LINE__);
        for(i = 0; i < numInputBuffers; i++) {
            /* allocate input buffer */
            APP_DPRINT("%d :: About to call OMX_AllocateBuffer for pInputBufferHeader[%d]\n",__LINE__, i);
            eError = OMX_AllocateBuffer(pHandle, &pInputBufferHeader[i], 0, NULL, G726ENC_INPUT_BUFFER_SIZE);
            if(eError != OMX_ErrorNone) {
                APP_DPRINT("%d :: Error returned by OMX_AllocateBuffer for pInputBufferHeader[%d]\n",__LINE__, i);
                goto EXIT;
            }
        }
        
        for(i = 0; i < numOutputBuffers; i++) {
            /* allocate output buffer */
            APP_DPRINT("%d :: About to call OMX_AllocateBuffer for pOutputBufferHeader[%d]\n",__LINE__, i);
            eError = OMX_AllocateBuffer(pHandle, &pOutputBufferHeader[i], 1, NULL, G726ENC_OUTPUT_BUFFER_SIZE);
            if(eError != OMX_ErrorNone) {
                APP_DPRINT("%d :: Error returned by OMX_AllocateBuffer for pOutputBufferHeader[%d]\n",__LINE__, i);
                goto EXIT;
            }
        }
#else
        for(i = 0; i < numInputBuffers; i++) {
            pInputBuffer[i] = (OMX_U8*)SafeMalloc(G726ENC_INPUT_BUFFER_SIZE);
            APP_MEMPRINT("%d :: [TESTAPP ALLOC] pInputBuffer[%d] = %p\n",__LINE__,i,pInputBuffer[i]);
            if(NULL == pInputBuffer[i]) {
                APP_DPRINT("%d :: Malloc Failed\n",__LINE__);
                eError = OMX_ErrorInsufficientResources;
                goto EXIT;
            }
            /*  allocate input buffer */
            APP_DPRINT("%d :: About to call OMX_UseBuffer\n",__LINE__);
            eError = OMX_UseBuffer(pHandle, &pInputBufferHeader[i], 0, NULL, G726ENC_INPUT_BUFFER_SIZE, pInputBuffer[i]);
            if(eError != OMX_ErrorNone) {
                APP_DPRINT("%d :: Error returned by OMX_UseBuffer()\n",__LINE__);
                goto EXIT;
            }
        }

        for(i = 0; i < numOutputBuffers; i++) {
            pOutputBuffer[i] = SafeMalloc(G726ENC_OUTPUT_BUFFER_SIZE + 256);
            APP_MEMPRINT("%d :: [TESTAPP ALLOC] pOutputBuffer[%d] = %p\n",__LINE__,i,pOutputBuffer[i]);
            if(NULL == pOutputBuffer[i]) {
                APP_DPRINT("%d :: Malloc Failed\n",__LINE__);
                eError = OMX_ErrorInsufficientResources;
                goto EXIT;
            }
            pOutputBuffer[i] = pOutputBuffer[i] + 128;
    
            /* allocate output buffer */
            APP_DPRINT("%d :: About to call OMX_UseBuffer\n",__LINE__);
            eError = OMX_UseBuffer(pHandle, &pOutputBufferHeader[i], 1, NULL, G726ENC_OUTPUT_BUFFER_SIZE, pOutputBuffer[i]);
            if(eError != OMX_ErrorNone) {
                APP_DPRINT("%d :: Error returned by OMX_UseBuffer()\n",__LINE__);
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
            APP_DPRINT( "Error:  hG726Encoder->WaitForState reports an eError %X\n", eError);
            goto EXIT;
        }
        APP_DPRINT("%d Component on OMX_StateIdle\n",__LINE__);

        for(i = 0; i < testcnt; i++) {
            frmCnt = 1;
            nFrameCount = 1;
            nOutBuff = 1;
            nIpBuff  = 1;
            if(i >= 1) {
                printf("%d :: Encoding the file in TESTCASE 5 OR TESTCSE 4\n",__LINE__);
                fIn = fopen(argv[1], "r");
                if(fIn == NULL) {
                    fprintf(stderr, "Error:  failed to open the file %s for readonly access\n", argv[1]);
                    goto EXIT;
                }
                fOut = fopen("TC5_G7261.G726", "w");
                if(fOut == NULL) {
                    fprintf(stderr, "Error:  failed to create the output file %s\n", argv[2]);
                    goto EXIT;
                }
            }

            done = 0;
            APP_DPRINT("%d :: App: Sending OMX_StateExecuting Command\n",__LINE__);

            APP_DPRINT("%d Sending Component to OMX_Executing\n",__LINE__);
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
            APP_DPRINT("%d Component on OMX_Executing\n",__LINE__);
            if(eError != OMX_ErrorNone) {
                APP_DPRINT( "Error:  hG726Encoder->WaitForState reports an eError %X\n", eError);
                goto EXIT;
            }

            if(audioinfo->dasfMode == 1) {
                printf("%d :: App: No.of Buffers Encoding = %d\n",__LINE__, atoi(argv[8]));
            }

            pComponent = (OMX_COMPONENTTYPE *)pHandle;

            if(audioinfo->dasfMode == 0) {
                for (k=0; k < numInputBuffers; k++) {
                    OMX_BUFFERHEADERTYPE* pBuffer = pInputBufferHeader[k];			
		            pBuffer->nFlags=0;
                            APP_DPRINT("%d About to call send_input_buffer\n",__LINE__);/*glen*/
						#ifdef OMX_GETTIME
					        if (k==0)
					        { 
					           GT_FlagE=1;  /* 1 = First Buffer,  0 = Not First Buffer  */
					           GT_START(); /* Empty Bufffer */
							}
						#endif
                            eError =  send_input_buffer(pHandle, pBuffer, fIn); 
                }
            }

            for (kk = 0; kk < numOutputBuffers; kk++) {
                APP_DPRINT("%d :: App: Calling FillThisBuffer \n",__LINE__);
                APP_DPRINT("%d About to call FillThisBuffern \n",__LINE__);/*glen*/
			#ifdef OMX_GETTIME
		        if (kk==0)
		        { 
		           GT_FlagF=1;  /* 1 = First Buffer,  0 = Not First Buffer  */
		           GT_START(); /* Fill Buffer */
				}
			#endif
                OMX_FillThisBuffer(pHandle, pOutputBufferHeader[kk]);
            }

            eError = OMX_GetState(pComponent, &state);
            if(eError != OMX_ErrorNone) {
                APP_DPRINT("%d :: OMX_GetState has returned status %X\n",__LINE__, eError);
                goto EXIT;
            }
            retval = 1;
#ifdef WAITFORRESOURCES            
            while( 1 ){
            if((eError == OMX_ErrorNone) && (state != OMX_StateIdle) && (state != OMX_StateInvalid) ){
#else
            while((eError == OMX_ErrorNone) && (state != OMX_StateIdle) && (state != OMX_StateInvalid)){
            if( 1 ){
#endif
                FD_ZERO(&rfds);
                FD_SET(IpBuf_Pipe[0], &rfds);
                FD_SET(OpBuf_Pipe[0], &rfds);
                FD_SET(Event_Pipe[0], &rfds);                
                tv.tv_sec = 1;
                tv.tv_usec = 0;                

                retval = select(fdmax+1, &rfds, NULL, NULL, &tv);
                if(retval == -1) {
                    perror("select()");
                    APP_DPRINT( " :: Error \n");
                    break;
                }

                if(!retval){
                    NoDataRead++;
#ifdef WAITFORRESOURCES                    
                    if(NoDataRead > 2000){
#else
                    if(NoDataRead > 5){
#endif
                        printf("Stoping component since No data is read from the pipes\n");          
                        StopComponent(pHandle); 
                    }
                }
                else{
                     NoDataRead=0;
                } 

                switch (tcID) {
		        case 1:
		        case 2:
		        case 3:
                        case 4:             
		        case 5:
		        case 6:
                        case 7:     
			if(audioinfo->dasfMode == 0) {
				if(FD_ISSET(IpBuf_Pipe[0], &rfds)) {                                           
					OMX_BUFFERHEADERTYPE* pBuffer = NULL;           
					read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
                                        frmCount++;
					if (frmCount==15 && tcID ==3){ /*Pause the component*/
						printf("App: Pausing Component for 5 Seconds....\n\n\n");
						PauseComponent(pHandle);
						sleep(3);
#ifdef FLUSHINPAUSE
						printf("App: Sending Flush to input port\n");
						eError = OMX_SendCommand(pHandle, OMX_CommandFlush, 0, NULL);
                        if(eError != OMX_ErrorNone) {
                            APP_DPRINT("%d:: Error from SendCommand OMX_CommandFlush\n",__LINE__);
                            goto EXIT;
                        }
#endif
                        sleep(2);
						printf("App: Resume Component\n");
						PlayComponent(pHandle);
					}
					if (frmCount==50 && tcID ==4){ /*Stop the component*/
                                                printf("Stoping the Component And Starting Again\n");
                                                printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");
                                                StopComponent(pHandle);
                                                tcID = 1;
						break;
                                        }
                                        if (frmCount==100 && tcID ==2){ /*Stop the component*/
                                                printf("Stoping the Component at the middle of stream\n");
                                                StopComponent(pHandle);
						break;
                                        }
					eError =  send_input_buffer(pHandle, pBuffer, fIn);
				        }
			} else {
                                       if (nFrameCount==15 && tcID ==3){  /*Pause the component*/
                                                tcID = 1;
						printf("App: Pausing Component for 5 Seconds....\n\n\n");
						PauseComponent(pHandle);
						sleep(3);
#ifdef FLUSHINPAUSE
						printf("App: Sending Flush to output port\n");
						eError = OMX_SendCommand(pHandle, OMX_CommandFlush, 1, NULL);
                        if(eError != OMX_ErrorNone) {
                            APP_DPRINT("%d:: Error from SendCommand OMX_CommandFlush\n",__LINE__);
                            goto EXIT;
                        }
#endif
                        sleep(2);
						printf("App: Resume Component\n");
						PlayComponent(pHandle);						
					}
					if (nFrameCount==50 && tcID ==4){ /*Stop the component*/
					        printf("Stoping the Component And Starting Again\n");
                            printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n"); 
						StopComponent(pHandle);
                                                tcID = 1;
						nFrameCount = 0;
						break;
                                        }
                                        if (nFrameCount==100 && tcID ==2){ /*Stop the component*/
                                                printf("Stoping the Component at the middle of stream\n");
                                                StopComponent(pHandle);
						break;
                                        }

					if(nFrameCount == 50 && tcID == 7)												     {
							/* set high gain for record stream */
							printf("[G726 encoder] --- will set stream gain to high\n");
            				                pCompPrivateStructGain->sVolume.nValue = 0x8000;
							eError = OMX_SetConfig(pHandle, OMX_IndexConfigAudioVolume, pCompPrivateStructGain);
							if (eError != OMX_ErrorNone) 
							{
								eError = OMX_ErrorBadParameter;
								goto EXIT;
							}
					}
					if(nFrameCount == 250 && tcID == 7)
					{
							/* set low gain for record stream */
							printf("[G726 encoder] --- will set stream gain to low\n");
            				                pCompPrivateStructGain->sVolume.nValue = 0x2000;
							eError = OMX_SetConfig(pHandle, OMX_IndexConfigAudioVolume, pCompPrivateStructGain);
							if (eError != OMX_ErrorNone) 
							{
								eError = OMX_ErrorBadParameter;
								goto EXIT;
							}							
					}                    
				}
		break;		
		default:
				APP_DPRINT("%d :: ### Simple DEFAULT Case Here ###\n",__LINE__);
		}
                /*----- output buffer ----*/  
                if( FD_ISSET(OpBuf_Pipe[0], &rfds) ) {
                    OMX_BUFFERHEADERTYPE* pBuf = NULL;
                    read(OpBuf_Pipe[0], &pBuf, sizeof(pBuf));
                    APP_DPRINT("%d :: App: pBuf->nFilledLen = %ld\n",__LINE__, pBuf->nFilledLen);
                    nFrameLen = pBuf->nFilledLen;
                    APP_DPRINT("%d :: App: nFrameLen = %d \n",__LINE__, nFrameLen);
                    if (nFrameLen != 0) {
                        APP_DPRINT("%d :: Writing OutputBuffer No: %d to the file nWrite = %d \n",__LINE__, nOutBuff, nFrameLen);
                        APP_DPRINT("Writing %d Bytes to the output File #%d\n", nFrameLen,nFrameCount);
                        fwrite(pBuf->pBuffer, 1, nFrameLen, fOut);
                        fflush(fOut);
                    }
                    nFrameCount++;
                    APP_DPRINT("%d :: App: pBuf->nFlags = %ld\n",__LINE__, pBuf->nFlags);

                    if(pBuf->nFlags == OMX_BUFFERFLAG_EOS) {
				          printf("%d :: App: OMX_BUFFERFLAG_EOS is received\n",__LINE__);
				          printf("%d :: App: Shutting down ---------- \n",__LINE__);				   
				          StopComponent(pHandle);
				          pBuf->nFlags = 0;
            	    }
			        else{
				          nOutBuff++;
                          OMX_FillThisBuffer(pComponent, pBuf);
 			              APP_DPRINT("%d :: App: pBuf->nFlags = %ld\n",__LINE__, pBuf->nFlags);
        			}
                    if(audioinfo->dasfMode) { 
     				    APP_DPRINT("%d :: NBAMR ENCODER RUNNING UNDER DASF MODE \n",__LINE__);
					    if(nFrameCount >= 400) {
                                                    printf("400 Frames Reached, Stoping Component from App\n");/*glen*/
						    StopComponent(pHandle);  
					    }
					    APP_DPRINT("%d :: NBAMR ENCODER READING DATA FROM DASF  \n",__LINE__);
                    }
                }                
/*-------*/
        if( FD_ISSET(Event_Pipe[0], &rfds) ) {
            OMX_U8 pipeContents = 0;
            read(Event_Pipe[0], &pipeContents, sizeof(OMX_U8));
            APP_DPRINT("%d :: received RM event: %d\n",__LINE__, pipeContents);            
            if (pipeContents == 0) {
    
                printf("Test app received OMX_ErrorResourcesPreempted\n");
                WaitForState(pHandle,OMX_StateIdle);
                for (i=0; i < numInputBuffers; i++) {
                    eError = OMX_FreeBuffer(pHandle,OMX_DirInput,pInputBufferHeader[i]);
                    if( (eError != OMX_ErrorNone)) {
                        APP_DPRINT ("Error in Free Handle function\n");
                    }
                }

                for (i=0; i < numOutputBuffers; i++) {
                    eError = OMX_FreeBuffer(pHandle,OMX_DirOutput,pOutputBufferHeader[i]);
                    if( (eError != OMX_ErrorNone)) {
                        APP_DPRINT ("%d:: Error in Free Handle function\n",__LINE__);
                    }
                }
                         
#ifdef USE_BUFFER
                /* newfree the App Allocated Buffers */
                APP_DPRINT("%d :: Freeing the App Allocated Buffers in TestApp\n",__LINE__);
                for(i=0; i < numInputBuffers; i++) {
                    APP_MEMPRINT("%d::: [TESTAPPFREE] pInputBuffer[%d] = %p\n",__LINE__,i,pInputBuffer[i]);
                    if(pInputBuffer[i] != NULL){
                        pInputBuffer[i] = pInputBuffer[i] - 128;
                        newfree(pInputBuffer[i]);
                        pInputBuffer[i] = NULL;
                        }
                    }
                    for(i=0; i < numOutputBuffers; i++) {
                        APP_MEMPRINT("%d::: [TESTAPPFREE] pOutputBuffer[%d] = %p\n",__LINE__,i, pOutputBuffer[i]);
                        if(pOutputBuffer[i] != NULL){
                            pOutputBuffer[i] = pOutputBuffer[i] - 128;                            
                            newfree(pOutputBuffer[i]);
                            pOutputBuffer[i] = NULL;
                        }
                    }
#endif                        

                    OMX_SendCommand(pHandle,OMX_CommandStateSet,OMX_StateLoaded,NULL);
                    WaitForState(pHandle,OMX_StateLoaded);
                    OMX_SendCommand(pHandle,OMX_CommandStateSet,OMX_StateWaitForResources,NULL);
                    WaitForState(pHandle,OMX_StateWaitForResources);
                    }
                    else if (pipeContents == 1) {
                        printf("Test app received OMX_ErrorResourcesAcquired\n");

        OMX_SendCommand(pHandle,OMX_CommandStateSet,OMX_StateIdle,NULL);
	for (i=0; i < numInputBuffers; i++) {
		/* allocate input buffer */
		eError = OMX_AllocateBuffer(pHandle,&pInputBufferHeader[i],0,NULL,G726ENC_INPUT_BUFFER_SIZE*3); /*To have enought space for    */
		if(eError != OMX_ErrorNone) {
	        APP_DPRINT("%d :: Error returned by OMX_AllocateBuffer()\n",__LINE__);
		}


	}

		WaitForState(pHandle,OMX_StateIdle);
    OMX_SendCommand(pHandle,OMX_CommandStateSet,OMX_StateExecuting,NULL);
    WaitForState(pHandle,OMX_StateExecuting);
    rewind(fIn);
        for (i=0; i < numInputBuffers;i++) {    
            send_input_buffer(pHandle, pInputBufferHeader[i], fIn);
        }
                    
                    }
                if (pipeContents == 2) {
                
                    StopComponent(pHandle);
                    
                for (i=0; i < numInputBuffers; i++) {
                    eError = OMX_FreeBuffer(pHandle,OMX_DirInput,pInputBufferHeader[i]);
                    if( (eError != OMX_ErrorNone)) {
                        APP_DPRINT ("%d :: TestAPP :: Error in Free Handle function\n",__LINE__);
                    }
                }

                for (i=0; i < numOutputBuffers; i++) {
                    eError = OMX_FreeBuffer(pHandle,OMX_DirOutput,pOutputBufferHeader[i]);
                    if( (eError != OMX_ErrorNone)) {
                        APP_DPRINT ("%d:: Error in Free Handle function\n",__LINE__);
                    }
                }
                
#if 1
    eError = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateLoaded, NULL);
    if(eError != OMX_ErrorNone) {
        APP_DPRINT ("%d :: Error from SendCommand-Idle State function\n",__LINE__);
				        printf("goto EXIT %d\n",__LINE__);
        
        goto EXIT;
    }
    eError = WaitForState(pHandle, OMX_StateLoaded);
#ifdef OMX_GETTIME
	GT_END("Call to SendCommand <OMX_StateLoaded>");
#endif
    if(eError != OMX_ErrorNone) {
        APP_DPRINT( "%d :: Error:  WaitForState reports an eError %X\n",__LINE__, error);
				        printf("goto EXIT %d\n",__LINE__);
        
        goto EXIT;
    }
#endif
goto SHUTDOWN;
                }
        }                   
/*-------*/      
                
                eError = OMX_GetState(pComponent, &state);
                if(eError != OMX_ErrorNone) {
                        APP_DPRINT("%d :: OMX_GetState has returned status %X\n",__LINE__, eError);
                        goto EXIT;
                }                
            }
else if (preempted) {
    sched_yield();
}
else {
    goto SHUTDOWN;
}

} /* While Loop Ending Here */    
            FirstTime =1;
            APP_DPRINT("%d :: App: The current state of the component = %d \n",__LINE__,state);
            fclose(fOut);
            fclose(fIn);

            APP_DPRINT("%d :: App: G726 Encoded = %d Frames \n",__LINE__,(nOutBuff));
        } /*Test Case 4 & 5 Inner for loop ends here  */

        APP_DPRINT ("%d :: App: Sending the OMX_StateLoaded Command\n",__LINE__);
	#ifdef OMX_GETTIME
		GT_START();
	#endif
        eError = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateLoaded, NULL);
        if(eError != OMX_ErrorNone) {
            APP_DPRINT("%d:: Error from SendCommand-Idle State function\n",__LINE__);
            goto EXIT;
        }

#ifndef WAITFORRESOURCES
        /* free the Allocate and Use Buffers */
        APP_DPRINT("%d :: App: Freeing the Allocate OR Use Buffers in TestApp\n",__LINE__);
        for(i=0; i < numInputBuffers; i++) {
            APP_DPRINT("%d :: App: About to free pInputBufferHeader[%d]\n",__LINE__, i);
            eError = OMX_FreeBuffer(pHandle, OMX_DirInput, pInputBufferHeader[i]);
            if((eError != OMX_ErrorNone)) {
                APP_DPRINT("%d:: Error in FreeBuffer function\n",__LINE__);
                goto EXIT;
            }
        }
        for(i=0; i < numOutputBuffers; i++) {
            APP_DPRINT("%d :: App: About to free pOutputBufferHeader[%d]\n",__LINE__, i);
            eError = OMX_FreeBuffer(pHandle, OMX_DirOutput, pOutputBufferHeader[i]);
            if((eError != OMX_ErrorNone)) {
                APP_DPRINT("%d :: Error in Free Buffer function\n",__LINE__);
                goto EXIT;
            }
        }
  #ifdef USE_BUFFER
        /* free the App Allocated Buffers */
        APP_DPRINT("%d :: App: Freeing the App Allocated Buffers in TestApp\n",__LINE__);
        for(i=0; i < numInputBuffers; i++) {
            APP_MEMPRINT("%d :: App: [TESTAPPFREE] pInputBuffer[%d] = %p\n",__LINE__,i,pInputBuffer[i]);
            if(pInputBuffer[i] != NULL){
                SafeFree(pInputBuffer[i]);
                pInputBuffer[i] = NULL;
            }
        }

        for(i=0; i < numOutputBuffers; i++) {
            pOutputBuffer[i] = pOutputBuffer[i] - 128;
            APP_MEMPRINT("%d :: App: [TESTAPPFREE] pOutputBuffer[%d] = %p\n",__LINE__,i, pOutputBuffer[i]);
            if(pOutputBuffer[i] != NULL){
                SafeFree(pOutputBuffer[i]);
                pOutputBuffer[i] = NULL;
            }
        }
  #endif
#endif

  	eError = WaitForState(pHandle, OMX_StateLoaded);
	#ifdef OMX_GETTIME
		GT_END("Call to SendCommand <OMX_StateLoaded>, PortReset, clear buffers");
	#endif		
		if(eError != OMX_ErrorNone) {
			APP_DPRINT( "Error:  G726Encoder->WaitForState reports an error %X\n", eError);
			goto EXIT;
		}

#ifdef WAITFORRESOURCES
    eError = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateWaitForResources, NULL);
    if(eError != OMX_ErrorNone) {
        APP_DPRINT ("%d :: Error from SendCommand-Idle State function\n",__LINE__);
				        printf("goto EXIT %d\n",__LINE__);
        
        goto EXIT;
    }
    eError = WaitForState(pHandle, OMX_StateWaitForResources);

    /* temporarily put this here until I figure out what should really happen here */
    sleep(10);
    /* temporarily put this here until I figure out what should really happen here */
#endif    

        APP_DPRINT ("%d :: App: Sending the OMX_CommandPortDisable Command\n",__LINE__);
        eError = OMX_SendCommand(pHandle, OMX_CommandPortDisable, -1, NULL);
        if(eError != OMX_ErrorNone) {
            APP_DPRINT("%d:: Error from SendCommand OMX_CommandPortDisable\n",__LINE__);
            goto EXIT;
        }
SHUTDOWN: 

        if(audioinfo->dasfMode){  
            close(fdwrite);
            close(fdread);
            if (streaminfo != NULL){
               SafeFree(streaminfo);
            }
        }
        
        APP_DPRINT("%d :: App: Free the Component handle\n",__LINE__);
        /* Unload the G726 Encoder Component */
        eError = TIOMX_FreeHandle(pHandle);
        if((eError != OMX_ErrorNone)) {
            APP_DPRINT("%d :: Error in Free Handle function\n",__LINE__);
            goto EXIT;
        }
        APP_DPRINT("%d :: App: Free Handle returned Successfully\n",__LINE__);
        
        APP_DPRINT("%d :: App: Calling OMX_Deinit()\n",__LINE__);
        eError = TIOMX_Deinit();
        if(eError != OMX_ErrorNone) {
            APP_DPRINT("%d :: Error returned by OMX_Deinit()\n",__LINE__);
            goto EXIT;
        }

        APP_DPRINT("%d :: App: Freeing the Memory Allocated in TestApp\n",__LINE__);
		
        APP_MEMPRINT("%d :: App: [TESTAPPFREE] %p\n",__LINE__,pG726Param);
        if(pG726Param != NULL){
            SafeFree(pG726Param);
            pG726Param = NULL;
        }
        APP_MEMPRINT("%d :: App: [TESTAPPFREE] %p\n",__LINE__,pCompPrivateStruct);
        if(pCompPrivateStruct != NULL){
            SafeFree(pCompPrivateStruct);
            pCompPrivateStruct = NULL;
        }   
        APP_MEMPRINT("%d :: App: [TESTAPPFREE] %p\n",__LINE__,audioinfo);
        if(audioinfo != NULL){
            SafeFree(audioinfo);
            audioinfo = NULL;
        }

        APP_DPRINT("%d :: App: Closing the Input and Output Pipes\n",__LINE__);
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
    } /*Outer for loop ends here */
    
    pthread_mutex_destroy(&WaitForState_mutex);
    pthread_cond_destroy(&WaitForState_threshold);
        
    printf("%d :: *********************************************************************\n",__LINE__);
    printf("%d :: NOTE: An output file %s has been created in file system\n",__LINE__,argv[2]);
    printf("%d :: *********************************************************************\n",__LINE__);
    if (testcnt == MAX_CYCLES) {
        printf("%d :: *********************************************************************\n",__LINE__);
        printf("%d :: NOTE: An output file TC5_G7261.G726 has been created in file system\n",__LINE__);
        printf("%d :: *********************************************************************\n",__LINE__);
    }
    if (testcnt1 == MAX_CYCLES) {
        printf("%d :: *********************************************************************\n",__LINE__);
        printf("%d :: NOTE: An output file TC6_G7261.G726 has been created in file system\n",__LINE__);
        printf("%d :: *********************************************************************\n",__LINE__);
    }
EXIT:
#ifdef APP_DEBUGMEM    
    printf("\n__ Printing memory not deleted\n");
    for(i=0;i<500;i++){
        if (lines[i]!=0){
             printf("__ %d Bytes allocated on [%p],   File:%s Line: %d\n",bytes[i],arr[i],file[i],lines[i]); 
             }
    }
#endif 
#ifdef OMX_GETTIME
	GT_END("G726Enc test <End>");
	OMX_ListDestroy(pListHead);	
#endif 	
    return eError;
}

OMX_ERRORTYPE send_input_buffer(OMX_HANDLETYPE pHandle, OMX_BUFFERHEADERTYPE* pBuffer, FILE *fIn)
{
	OMX_ERRORTYPE error = OMX_ErrorNone;
	OMX_COMPONENTTYPE *pComponent = (OMX_COMPONENTTYPE *)pHandle; 

	if(FirstTime){
		if(mframe == TWO_FRAMES){
			nRead = fread(pBuffer->pBuffer, 1, G726ENC_INPUT_BUFFER_SIZE*2, fIn);
		}
                else if(mframe == HALF_FRAME){
			nRead = fread(NextBuffer, 1, G726ENC_INPUT_BUFFER_SIZE/2, fIn);
	        }
		else{
			nRead = fread(pBuffer->pBuffer, 1, G726ENC_INPUT_BUFFER_SIZE, fIn);
		}		
		pBuffer->nFilledLen = nRead;
	}
	else{
		memcpy(pBuffer->pBuffer, NextBuffer,nRead);
		pBuffer->nFilledLen = nRead;
	}
	
	if(mframe == TWO_FRAMES){
			nRead = fread(NextBuffer, 1, G726ENC_INPUT_BUFFER_SIZE*2, fIn);
	}
        else if(mframe == HALF_FRAME){
			nRead = fread(NextBuffer, 1, G726ENC_INPUT_BUFFER_SIZE/2, fIn);
	}
	else{
			nRead = fread(NextBuffer, 1, G726ENC_INPUT_BUFFER_SIZE, fIn);
	}	
	
	if(nRead < G726ENC_INPUT_BUFFER_SIZE && !DasfMode && (mframe != HALF_FRAME)){
		pBuffer->nFlags = OMX_BUFFERFLAG_EOS;
	}else{
          pBuffer->nFlags = 0;
          }
		
        if( ((mframe != HALF_FRAME) && (pBuffer->nFilledLen>= G726ENC_INPUT_BUFFER_SIZE)) || 
              ((mframe == HALF_FRAME) && (pBuffer->nFilledLen> 0))){
    	
        if(pBuffer->nFlags == OMX_BUFFERFLAG_EOS){
                           APP_DPRINT("Sending Last Input Buffer from App\n");
        }
        vez++;
        APP_DPRINT("Sending %d bytes to Comp, Time: %ld\n", (int)pBuffer->nFilledLen,vez);
        pBuffer->nTimeStamp = rand() % 100;
        if (!preempted) {
             error = OMX_EmptyThisBuffer(pComponent, pBuffer);
             if (error == OMX_ErrorIncorrectStateOperation) 
                 error = 0;
        }		
    }
   	FirstTime=0;
	return error;	
}

OMX_ERRORTYPE StopComponent(OMX_HANDLETYPE *pHandle)
{
    OMX_ERRORTYPE error = OMX_ErrorNone;
#ifdef OMX_GETTIME
	GT_START();
#endif
    error = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateIdle, NULL);
	if(error != OMX_ErrorNone) {
                    fprintf (stderr,"\nError from SendCommand-Idle(Stop) State function!!!!!!!!\n");
                    goto EXIT;
		}
	error =	WaitForState(pHandle, OMX_StateIdle);
#ifdef OMX_GETTIME
	GT_END("Call to SendCommand <OMX_StateIdle>");
#endif
    if(error != OMX_ErrorNone) {
					fprintf(stderr, "\nError:  G726Encoder->WaitForState reports an error %X!!!!!!!\n", error);
					goto EXIT;
	}
EXIT:
    return error;
}

OMX_ERRORTYPE PauseComponent(OMX_HANDLETYPE *pHandle)
{
    OMX_ERRORTYPE error = OMX_ErrorNone;
#ifdef OMX_GETTIME
	GT_START();
#endif
    error = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StatePause, NULL);
	if(error != OMX_ErrorNone) {
                    fprintf (stderr,"\nError from SendCommand-Idle(Stop) State function!!!!!!!!\n");
                    goto EXIT;
		}
	error =	WaitForState(pHandle, OMX_StatePause);
#ifdef OMX_GETTIME
	GT_END("Call to SendCommand <OMX_StatePause>");
#endif
    if(error != OMX_ErrorNone) {
					fprintf(stderr, "\nError:  G726Encoder->WaitForState reports an error %X!!!!!!!\n", error);
					goto EXIT;
	}
EXIT:
    return error;
}

OMX_ERRORTYPE PlayComponent(OMX_HANDLETYPE *pHandle)
{
    OMX_ERRORTYPE error = OMX_ErrorNone;
#ifdef OMX_GETTIME
	GT_START();
#endif
    error = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateExecuting, NULL);
	if(error != OMX_ErrorNone) {
                    fprintf (stderr,"\nError from SendCommand-Idle(Stop) State function!!!!!!!!\n");
                    goto EXIT;
		}
	error =	WaitForState(pHandle, OMX_StateExecuting);
#ifdef OMX_GETTIME
	GT_END("Call to SendCommand <OMX_StateExecuting>");
#endif
    if(error != OMX_ErrorNone) {
					fprintf(stderr, "\nError:  G726Encoder->WaitForState reports an error %X!!!!!!!\n", error);
					goto EXIT;
	}
EXIT:
    return error;

}
