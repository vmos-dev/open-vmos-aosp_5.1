
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
* @file AmrEnc_Test.c
*
* This file implements NBAMR Encoder Component Test Application to verify
* which is fully compliant with the Khronos OpenMAX (TM) 1.0 Specification
*
* @path  $(CSLPATH)\OMAPSW_MPU\linux\audio\src\openmax_il\nbamr_enc\tests
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
#include <OMX_Component.h>
#include <OMX_Core.h>
#include <OMX_Audio.h>
#include <TIDspOmx.h>
/* #include <AudioManagerAPI.h> */
#include <time.h>

#ifdef OMX_GETTIME
    #include <OMX_Common_Utils.h>
    #include <OMX_GetTime.h>     /*Headers for Performance & measuremet    */
#endif

FILE *fpRes;

/* ======================================================================= */
/**
 * @def NBAMRENC_NUM_INPUT_BUFFERS   Default number of input buffers
 */
/* ======================================================================= */
#define NBAPP_NUM_INPUT_BUFFERS 1
/* ======================================================================= */
/**
 * @def NBAMRENC_NUM_INPUT_BUFFERS_DASF  Default No.of input buffers DASF
 */
/* ======================================================================= */
#define NBAPP_NUM_INPUT_BUFFERS_DASF 2
/* ======================================================================= */
/**
 * @def NBAMRENC_NUM_OUTPUT_BUFFERS   Default number of output buffers
 */
/* ======================================================================= */
#define NBAPP_NUM_OUTPUT_BUFFERS 1
/* ======================================================================= */
/**
 * @def NBAMRENC_INPUT_BUFFER_SIZE   	 Default input buffer size
 *		NBAMRENC_INPUT_BUFFER_SIZE_DASF  Default input buffer size DASF
 */
/* ======================================================================= */
#define NBAPP_INPUT_BUFFER_SIZE 320
#define NBAPP_INPUT_BUFFER_SIZE_DASF 320
/* ======================================================================= */
/**
 * @def NBAMRENC_OUTPUT_BUFFER_SIZE   Default output buffer size
 */
/* ======================================================================= */
#define NBAPP_OUTPUT_BUFFER_SIZE 118
/* ======================================================================= */
/**
 * @def NBAMRENC_OUTPUT_BUFFER_SIZE_MIME  Default input buffer size MIME
 */
/* ======================================================================= */
#define NBAPP_OUTPUT_BUFFER_SIZE_MIME 34

/* ======================================================================= */
/*
 * @def	NBAMRENC_APP_ID  App ID Value setting
 */
/* ======================================================================= */
#define NBAMRENC_APP_ID 100

#define SLEEP_TIME 5

#define NBAMRENC_MIME_HEADER_LEN 6

#define FIFO1 "/dev/fifo.1"
#define FIFO2 "/dev/fifo.2"

#define APP_INFO

#undef APP_DEBUG

#undef APP_MEMCHECK

#undef USE_BUFFER
/*#define USE_BUFFER*/

#define STRESS_TEST_ITERATIONS 20

#ifdef  APP_INFO
        #define APP_IPRINT(...)    fprintf(stderr,__VA_ARGS__)			/* Information prints */
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
void *arr[500];
int lines[500];
int bytes[500];
char file[500][50];
int ind=0;

#define newmalloc(x) mynewmalloc(__LINE__,__FILE__,x)
#define newfree(z) mynewfree(z,__LINE__,__FILE__)

void * mynewmalloc(int line, char *s, int size)
{
   void *p;    
   int e=0;
   p = calloc(1,size);
   if(p==NULL){
       APP_IPRINT("Memory not available\n");
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
         APP_IPRINT("Allocating %d bytes on address %p, line %d file %s pos %d\n", size, p, line, s, e);
         return p;
   }
}

int mynewfree(void *dp, int line, char *s){
    int q;
    if(dp==NULL){
                 APP_IPRINT("NULL can't be deleted\n");
                 return 0;
    }
    for(q=0;q<500;q++){
        if(arr[q]==dp){
           APP_IPRINT("Deleting %d bytes on address %p, line %d file %s\n", bytes[q],dp, line, s);
           free(dp);
           dp = NULL;
           lines[q]=0;
           strcpy(file[q],"");
           break;
        }            
     }    
     if(500==q)
         APP_IPRINT("\n\nPointer not found. Line:%d    File%s!!\n\n",line, s);
}
#else
#define newmalloc(x) malloc(x)
#define newfree(z) free(z)
#endif


typedef struct NBAMRENC_BUFDATA {
   OMX_U8 nFrames;     
}NBAMRENC_BUFDATA;

/* ======================================================================= */
/**
 *  M A C R O S FOR MALLOC and MEMORY FREE and CLOSING PIPES
 */
/* ======================================================================= */

#define OMX_NBAPP_CONF_INIT_STRUCT(_s_, _name_)	\
    memset((_s_), 0x0, sizeof(_name_));	\
    (_s_)->nSize = sizeof(_name_);		\
    (_s_)->nVersion.s.nVersionMajor = 0x1;	\
    (_s_)->nVersion.s.nVersionMinor = 0x0;	\
    (_s_)->nVersion.s.nRevision = 0x0;		\
    (_s_)->nVersion.s.nStep = 0x0

#define OMX_NBAPP_INIT_STRUCT(_s_, _name_)	\
    memset((_s_), 0x0, sizeof(_name_));	\

#define OMX_NBAPP_MALLOC_STRUCT(_pStruct_, _sName_)   \
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
/** NBAPP_COMP_PORT_TYPE  Port types
 *
 *  @param  NBAPP_INPUT_PORT			Input port
 *
 *  @param  NBAPP_OUTPUT_PORT			Output port
 */
/*  ====================================================================== */
/*This enum must not be changed. */
typedef enum NBAPP_COMP_PORT_TYPE {
    NBAPP_INPUT_PORT = 0,
    NBAPP_OUTPUT_PORT
}NBAPP_COMP_PORT_TYPE;

/* ======================================================================= */
/**
 * @def NBAPP_MAX_NUM_OF_BUFS   	Maximum number of buffers
 * @def	NBAPP_NUM_OF_CHANNELS 		Number of Channels
 * @def NBAPP_SAMPLING_FREQUENCY    Sampling frequency
 */
/* ======================================================================= */
#define NBAPP_MAX_NUM_OF_BUFS 10
#define NBAPP_NUM_OF_CHANNELS 1
#define NBAPP_SAMPLING_FREQUENCY 8000


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
OMX_U8 NextBuffer[NBAPP_INPUT_BUFFER_SIZE*3];
int FirstTime = 1;
int nRead;
NBAMRENC_BUFDATA* OutputFrames;

#ifdef DSP_RENDERING_ON 
AM_COMMANDDATATYPE cmd_data;
#endif	
OMX_STRING strAmrEncoder = "OMX.TI.AMR.encode";

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
int DasfMode;
int TeeMode=0;
int mframe=0;

int preempted = 0;

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
     /* OMX_S16 nCnt = 0; */
     OMX_COMPONENTTYPE *pComponent = (OMX_COMPONENTTYPE *)pHandle;

     eError = pComponent->GetState(pHandle, &CurState);
     if (CurState == OMX_StateInvalid && bInvalidState == OMX_TRUE)
	 {
		 	eError = OMX_ErrorInvalidState;
	 }

eError = OMX_GetState(pHandle, &CurState);
    if (CurState == OMX_StateInvalid) {
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
   OMX_STATETYPE state;

   OMX_U8 writeValue;  

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
			if (nData2 == NBAPP_INPUT_PORT) {
				inputPortDisabled = 1;
			}
			if (nData2 == NBAPP_OUTPUT_PORT) {
				outputPortDisabled = 1;
			}
		}
        if ((nData1 == OMX_CommandStateSet) && (TargetedState == nData2) && 
            (WaitForState_flag)){
            WaitForState_flag = 0;
            pthread_mutex_lock(&WaitForState_mutex);
            pthread_cond_signal(&WaitForState_threshold);
            pthread_mutex_unlock(&WaitForState_mutex);
        }
           break;
       case OMX_EventError:
		   if (nData1 == OMX_ErrorInvalidState) {
		   		bInvalidState =OMX_TRUE;
		   		APP_IPRINT("EventHandler: Invalid State!!!!\n");
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
		   writeValue = 2;  
	       write(Event_Pipe[1], &writeValue, sizeof(OMX_U8));
           break;
       case OMX_EventResourcesAcquired:
       	   APP_DPRINT( "%d :: App: Component OMX_EventResourcesAcquired = %d\n", __LINE__,eEvent);
           writeValue = 1;
           write(Event_Pipe[1], &writeValue, sizeof(OMX_U8));
           preempted=0;

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
/*    OutputFrames = pBuffer->pOutputPortPrivate;
    printf("Receiving %d Frames\n",OutputFrames->nFrames);*/
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
    OMX_CALLBACKTYPE AmrCaBa = {(void *)EventHandler,
				(void*)EmptyBufferDone,
                                (void*)FillBufferDone};
    OMX_HANDLETYPE pHandle;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 AppData = NBAMRENC_APP_ID;
    OMX_PARAM_PORTDEFINITIONTYPE* pCompPrivateStruct;
    OMX_AUDIO_PARAM_AMRTYPE *pAmrParam;
    OMX_COMPONENTTYPE *pComponent;
    OMX_STATETYPE state;
    OMX_BUFFERHEADERTYPE* pInputBufferHeader[NBAPP_MAX_NUM_OF_BUFS];
    OMX_BUFFERHEADERTYPE* pOutputBufferHeader[NBAPP_MAX_NUM_OF_BUFS];
    bInvalidState=OMX_FALSE;
#ifdef USE_BUFFER
    OMX_U8* pInputBuffer[NBAPP_MAX_NUM_OF_BUFS];
	OMX_U8* pOutputBuffer[NBAPP_MAX_NUM_OF_BUFS];
#endif
	TI_OMX_DSP_DEFINITION* audioinfo;
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
    int nOutBuff = 1;
    int NoDataRead=0;
	OMX_INDEXTYPE index;
   	OMX_U32	streamId;
   	int numInputBuffers=0,numOutputBuffers=0;
    TI_OMX_DATAPATH dataPath;
    int FrameMode=0;
int nbamrencdfwrite;
int nbamrencfdread;
    
    OMX_AUDIO_CONFIG_VOLUMETYPE* pCompPrivateStructGain = NULL;
    srand ( time(NULL) );

    pthread_mutex_init(&WaitForState_mutex, NULL);
    pthread_cond_init (&WaitForState_threshold, NULL);
    WaitForState_flag = 0;
	
	APP_IPRINT("------------------------------------------------------\n");
    APP_IPRINT("This is Main Thread In NBAMR ENCODER Test Application:\n");
    APP_IPRINT("Test Core 1.5 - " __DATE__ ":" __TIME__ "\n");
    APP_IPRINT("------------------------------------------------------\n");

#ifdef OMX_GETTIME
    APP_IPRINT("Line %d\n",__LINE__);
      GTeError = OMX_ListCreate(&pListHead);
        APP_IPRINT("Line %d\n",__LINE__);
      APP_IPRINT("eError = %d\n",GTeError);
      GT_START();
  APP_IPRINT("Line %d\n",__LINE__);
#endif

    /* check the input parameters */
    if((argc < 14) || (argc > 15)) {
        APP_IPRINT("%d :: Usage: [TestApp] [O/P] [FUNC_ID_X] [FM/DM] [NBAMR/EFR] [BITRATE] [DTXON/OFF] [NONMIME/MIME/IF2] [ACDNON/OFF] [FRAMES] [1 to N] [1 to N] [MFON] [1 to N (optional)]\n",__LINE__);
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
    
    if(!strcmp(argv[3],"FUNC_ID_1")) {
        APP_IPRINT("%d :: ### Testing TESTCASE 1 PLAY TILL END ###\n",__LINE__);
        testcnt = 1;
        testcnt1 = 1;
        tcID = 1;
    } else if(!strcmp(argv[3],"FUNC_ID_2")) {
        APP_IPRINT("%d :: ### Testing TESTCASE 2 STOP IN THE END ###\n",__LINE__);
        testcnt = 1;
        testcnt1 = 1;
        tcID = 2;
    } else if(!strcmp(argv[3],"FUNC_ID_3")) {
        APP_IPRINT("%d :: ### Testing TESTCASE 3 PAUSE - RESUME IN BETWEEN ###\n",__LINE__);
        testcnt = 1;
        testcnt1 = 1;
        tcID = 3;
    } else if(!strcmp(argv[3],"FUNC_ID_4")) {
        APP_IPRINT("%d :: ### Testing TESTCASE 4 STOP IN BETWEEN ###\n",__LINE__);
        testcnt = 2;
        testcnt1 = 1;
        tcID = 4;
        APP_IPRINT("######## testcnt = %d #########\n",testcnt);
    }
    if(!strcmp(argv[3],"FUNC_ID_5")){
        APP_IPRINT("%d :: ### Testing TESTCASE 5 ENCODE without Deleting component Here ###\n",__LINE__);
        if (argc == 15)
        {
            testcnt = atoi(argv[14]);
        }
        else
        {
            testcnt = STRESS_TEST_ITERATIONS;  /*20 cycles by default*/
        }
        testcnt1 = 1;
        tcID = 5;
    }
    if(!strcmp(argv[3],"FUNC_ID_6")) {
        APP_IPRINT("%d :: ### Testing TESTCASE 6 ENCODE with Deleting component Here ###\n",__LINE__);
        if (argc == 15)
        {
            testcnt1 = atoi(argv[14]);
        }
        else
        {
            testcnt1 = STRESS_TEST_ITERATIONS;  /*20 cycles by default*/
        }
        testcnt = 1;
        tcID = 6;
    }
    if(!strcmp(argv[3],"FUNC_ID_7")) {
        APP_IPRINT("%d :: ### Testing TESTCASE 7 ENCODE with Volume Control ###\n",__LINE__);
        testcnt = 1;
        testcnt1 = 1;
        tcID = 7;
    }
    if(!strcmp(argv[3],"FUNC_ID_8")) {
        APP_IPRINT("%d :: ### Testing PLAY TILL END  WITH TWO FRAMES BY BUFFER###\n",__LINE__);
            testcnt = 1;
        testcnt1 = 1;
        tcID = 1;
        mframe = 1;
    }
    for(j = 0; j < testcnt1; j++) {
          
#ifdef DSP_RENDERING_ON 
		if((nbamrencdfwrite=open(FIFO1,O_WRONLY))<0) {
            APP_IPRINT("[AMRTEST] - failure to open WRITE pipe\n");
        }
        else {
            APP_IPRINT("[AMRTEST] - opened WRITE pipe\n");
        }

        if((nbamrencfdread=open(FIFO2,O_RDONLY))<0) {
            APP_IPRINT("[AMRTEST] - failure to open READ pipe\n");
            goto EXIT;
        }
        else {
            APP_IPRINT("[AMRTEST] - opened READ pipe\n");
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

	TI_OMX_STREAM_INFO *streaminfo;
    OMX_NBAPP_MALLOC_STRUCT(streaminfo, TI_OMX_STREAM_INFO);
    OMX_NBAPP_MALLOC_STRUCT(audioinfo, TI_OMX_DSP_DEFINITION);
	OMX_NBAPP_INIT_STRUCT(audioinfo, TI_OMX_DSP_DEFINITION);

    ArrayOfPointers[0]=(TI_OMX_STREAM_INFO*)streaminfo;
    ArrayOfPointers[1]=(TI_OMX_DSP_DEFINITION*)audioinfo;
	if(j > 0) {
		APP_IPRINT ("%d :: Encoding the file for %d Time in TESTCASE 6\n",__LINE__,j+1);
		fIn = fopen(argv[1], "r");
		if( fIn == NULL ) {
			fprintf(stderr, "Error:  failed to open the file %s for read only access\n",argv[1]);
			goto EXIT;
		}

		fOut = fopen("TC6_Amr1.amr", "w");
		if( fOut == NULL ) {
			fprintf(stderr, "Error:  failed to create the output file %s\n",argv[2]);
			goto EXIT;
		}
	 }

    /* Load the NBAMR Encoder Component */
 

#ifdef OMX_GETTIME
	GT_START();
    eError = OMX_GetHandle(&pHandle, strAmrEncoder, &AppData, &AmrCaBa);
	GT_END("Call to GetHandle");
#else 
    eError = TIOMX_GetHandle(&pHandle, strAmrEncoder, &AppData, &AmrCaBa);
#endif
    if((eError != OMX_ErrorNone) || (pHandle == NULL)) {
        APP_DPRINT("Error in Get Handle function\n");
        goto EXIT;
    }
	

	/* Setting No.Of Input and Output Buffers for the Component */
	numInputBuffers = atoi(argv[11]);
	APP_DPRINT("\n%d :: App: numInputBuffers = %d \n",__LINE__,numInputBuffers);

	numOutputBuffers = atoi(argv[12]);
	APP_DPRINT("\n%d :: App: numOutputBuffers = %d \n",__LINE__,numOutputBuffers);


    OMX_NBAPP_MALLOC_STRUCT(pCompPrivateStruct, OMX_PARAM_PORTDEFINITIONTYPE);
	OMX_NBAPP_CONF_INIT_STRUCT(pCompPrivateStruct, OMX_PARAM_PORTDEFINITIONTYPE);
    OMX_NBAPP_MALLOC_STRUCT(pAmrParam, OMX_AUDIO_PARAM_AMRTYPE);
	OMX_NBAPP_CONF_INIT_STRUCT(pAmrParam, OMX_AUDIO_PARAM_AMRTYPE);
    
    ArrayOfPointers[2]=(OMX_PARAM_PORTDEFINITIONTYPE*)pCompPrivateStruct;
    ArrayOfPointers[3] = (OMX_AUDIO_PARAM_AMRTYPE *)pAmrParam;

    APP_DPRINT("%d :: Setting input port config\n",__LINE__);
    pCompPrivateStruct->nSize 							   = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
    pCompPrivateStruct->nVersion.s.nVersionMajor           = 0xF1;
    pCompPrivateStruct->nVersion.s.nVersionMinor 		   = 0xF2;
    pCompPrivateStruct->nPortIndex                         = NBAPP_INPUT_PORT;
    pCompPrivateStruct->eDir                               = OMX_DirInput;
    pCompPrivateStruct->nBufferCountActual                 = numInputBuffers;
    pCompPrivateStruct->nBufferCountMin                    = numInputBuffers;
    pCompPrivateStruct->nBufferSize                        = NBAPP_INPUT_BUFFER_SIZE;
    pCompPrivateStruct->bEnabled                           = OMX_TRUE;
    pCompPrivateStruct->bPopulated                         = OMX_FALSE;
    pCompPrivateStruct->eDomain                            = OMX_PortDomainAudio;
	pCompPrivateStruct->format.audio.eEncoding             = OMX_AUDIO_CodingAMR;
	pCompPrivateStruct->format.audio.cMIMEType             = NULL;
	pCompPrivateStruct->format.audio.pNativeRender         = NULL;
	pCompPrivateStruct->format.audio.bFlagErrorConcealment = OMX_FALSE;    /*Send input port config*/
	APP_DPRINT("%d :: Setting input port config\n",__LINE__);
	if(!(strcmp(argv[8],"NONMIME"))) {
		pCompPrivateStruct->format.audio.cMIMEType = "NONMIME";
		FrameMode = 0;
		pAmrParam->eAMRFrameFormat = OMX_AUDIO_AMRFrameFormatConformance;
    	APP_DPRINT("\n%d :: App: pCompPrivateStruct->format.audio.cMIMEType --> %s \n",
    											__LINE__,pCompPrivateStruct->format.audio.cMIMEType);
	}else if(!(strcmp(argv[8],"MIME"))) {
    	pCompPrivateStruct->format.audio.cMIMEType = "MIME";
		FrameMode = 1;
		pAmrParam->eAMRFrameFormat = OMX_AUDIO_AMRFrameFormatFSF;
    	APP_DPRINT("\n%d :: App: pCompPrivateStruct->format.audio.cMIMEType --> %s \n",
    											__LINE__,pCompPrivateStruct->format.audio.cMIMEType);
	} 
	else if(!(strcmp(argv[8],"IF2"))) {
    	pCompPrivateStruct->format.audio.cMIMEType = "IF2";
		FrameMode = 2;
		pAmrParam->eAMRFrameFormat = OMX_AUDIO_AMRFrameFormatIF2;
    	APP_DPRINT("\n%d :: App: pCompPrivateStruct->format.audio.cMIMEType --> %s \n",
    											__LINE__,pCompPrivateStruct->format.audio.cMIMEType);
	}    
    else {
		eError = OMX_ErrorBadParameter;
		APP_IPRINT("%d :: App: Should Be One of these Modes MIME, NONMIME\n",__LINE__);
		goto EXIT;
	}
    
	APP_DPRINT("%d :: Setting input port config\n",__LINE__);
/*	if(!(strcmp(argv[5],"EFR"))) {
		pAmrParam->eAMRDTXMode = OMX_AUDIO_AMRDTXasEFR;
		
    	APP_DPRINT("\n%d :: App: pCompPrivateStruct->EFR = %s \n",__LINE__,argv[5]);
	} else if(!(strcmp(argv[5],"NBAMR"))) {
		pAmrParam->eAMRDTXMode = OMX_AUDIO_AMRDTXModeOff;
		
    	APP_DPRINT("\n%d :: App: pCompPrivateStruct->NBAMR = %s \n",__LINE__,argv[5]);
	} else {
		eError = OMX_ErrorBadParameter;
		printf("%d :: App: Should Be One of these Modes EFR, NBAMR\n",__LINE__);
		goto EXIT;
	}
*/

	if(!(strcmp(argv[4],"FM"))) {
		audioinfo->dasfMode = 0;
		DasfMode = 0;
    	APP_DPRINT("\n%d :: App: audioinfo->dasfMode = %x \n",__LINE__,audioinfo->dasfMode);
	} else if(!(strcmp(argv[4],"DM"))){
		 audioinfo->dasfMode =  1;
		 DasfMode = 1;
    	 APP_DPRINT("\n%d :: App: audioinfo->dasfMode = %x \n",__LINE__,audioinfo->dasfMode);
		 APP_DPRINT("%d :: NBAMR ENCODER RUNNING UNDER DASF MODE \n",__LINE__);
		 pCompPrivateStruct->nBufferCountActual = 0;
	}  
	else if(!(strcmp(argv[4],"TMP"))){
		 audioinfo->dasfMode =  1;
		 DasfMode = 1;
		 audioinfo->teeMode = TEEMODE_PLAYBACK;
		 TeeMode = TEEMODE_PLAYBACK;
    	 APP_DPRINT("\n%d :: App: audioinfo->dasfMode = %x \n",__LINE__,audioinfo->dasfMode);
		 APP_DPRINT("%d :: NBAMR ENCODER RUNNING UNDER DASF MODE \n",__LINE__);
		 pCompPrivateStruct->nBufferCountActual = 0;
	}	
	else if(!(strcmp(argv[4],"TML"))){
		 audioinfo->dasfMode =  1;
		 DasfMode = 1;
 		 audioinfo->teeMode = TEEMODE_LOOPBACK;
		 TeeMode = TEEMODE_LOOPBACK;
    	 APP_DPRINT("\n%d :: App: audioinfo->dasfMode = %x \n",__LINE__,audioinfo->dasfMode);
		 APP_DPRINT("%d :: NBAMR ENCODER RUNNING UNDER DASF MODE \n",__LINE__);
		 pCompPrivateStruct->nBufferCountActual = 0;
	}	
	else if(!(strcmp(argv[4],"TMLP"))){
		 audioinfo->dasfMode =  1;
 		 audioinfo->teeMode = TEEMODE_PLAYLOOPBACK;
		 DasfMode = 1;
		 TeeMode = TEEMODE_PLAYLOOPBACK;
    	 APP_DPRINT("\n%d :: App: audioinfo->dasfMode = %x \n",__LINE__,audioinfo->dasfMode);
		 APP_DPRINT("%d :: NBAMR ENCODER RUNNING UNDER DASF MODE \n",__LINE__);
		 pCompPrivateStruct->nBufferCountActual = 0;
	}	
	else {
		eError = OMX_ErrorBadParameter;
		APP_IPRINT("\n%d :: App: audioinfo->dasfMode Sending Bad Parameter\n",__LINE__);
		APP_IPRINT("%d :: App: Should Be One of these Modes FM, DM\n",__LINE__);
		goto EXIT;
	}

	if(audioinfo->dasfMode == 0) {
		if((atoi(argv[10])) != 0) {
			eError = OMX_ErrorBadParameter;
			APP_IPRINT("\n%d :: App: No. of Frames Sending Bad Parameter\n",__LINE__);
			APP_IPRINT("%d :: App: For FILE mode argv[10] Should Be --> 0\n",__LINE__);
			APP_IPRINT("%d :: App: For DASF mode argv[10] Should be greater than zero depends on number of frames user want to encode\n",__LINE__);
			goto EXIT;
		}
	} else {
		if((atoi(argv[10])) == 0) {
			eError = OMX_ErrorBadParameter;
			APP_IPRINT("\n%d :: App: No. of Frames Sending Bad Parameter\n",__LINE__);
			APP_IPRINT("%d :: App: For DASF mode argv[10] Should be greater than zero depends on number of frames user want to encode\n",__LINE__);
			APP_IPRINT("%d :: App: For FILE mode argv[10] Should Be --> 0\n",__LINE__);
			goto EXIT;
		}
	}

	if(!(strcmp(argv[9],"ACDNOFF"))) {
		audioinfo->acousticMode = 0;
    	APP_DPRINT("\n%d :: App: audioinfo->acousticMode = %x \n",__LINE__,audioinfo->acousticMode);
	} else if(!(strcmp(argv[9],"ACDNON"))) {
		audioinfo->acousticMode = 1;		
		APP_DPRINT("\n%d :: App: audioinfo->acdnacousticModeMode = %x \n",__LINE__,audioinfo->acousticMode);
	} else {
		eError = OMX_ErrorBadParameter;
		APP_IPRINT("\n%d :: App: audioinfo->acdnMode Sending Bad Parameter\n",__LINE__);
		APP_IPRINT("%d :: App: Should Be One of these Modes ACDNON, ACDNOFF\n",__LINE__);
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
    pCompPrivateStruct->nSize 							   = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
    pCompPrivateStruct->nVersion.s.nVersionMajor           = 0xF1;
    pCompPrivateStruct->nVersion.s.nVersionMinor 		   = 0xF2;
    pCompPrivateStruct->nPortIndex                         = NBAPP_OUTPUT_PORT;
    pCompPrivateStruct->eDir                               = OMX_DirOutput;
    pCompPrivateStruct->nBufferCountActual                 = numOutputBuffers;
    pCompPrivateStruct->nBufferCountMin                    = numOutputBuffers;
    pCompPrivateStruct->nBufferSize                        = NBAPP_OUTPUT_BUFFER_SIZE;
    pCompPrivateStruct->bEnabled                           = OMX_TRUE;
    pCompPrivateStruct->bPopulated                         = OMX_FALSE;
    pCompPrivateStruct->eDomain                            = OMX_PortDomainAudio;
	pCompPrivateStruct->format.audio.eEncoding             = OMX_AUDIO_CodingAMR;
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
        goto EXIT;
    }

    pAmrParam->nSize                    = sizeof(OMX_AUDIO_PARAM_AMRTYPE);
    pAmrParam->nVersion.s.nVersionMajor = 0xF1;
    pAmrParam->nVersion.s.nVersionMinor = 0xF2;
    pAmrParam->nPortIndex               = NBAPP_OUTPUT_PORT;
    pAmrParam->nChannels                = NBAPP_NUM_OF_CHANNELS;
	pAmrParam->eAMRBandMode 			= 0;
	if(!(strcmp(argv[6],"BR122"))) {
		pAmrParam->eAMRBandMode = OMX_AUDIO_AMRBandModeNB7;
    	APP_DPRINT("\n%d :: App: pAmrParam->eAMRBandMode = %d \n",__LINE__,pAmrParam->eAMRBandMode);
	} else if(!(strcmp(argv[6],"BR102"))) {
		pAmrParam->eAMRBandMode = OMX_AUDIO_AMRBandModeNB6;
    	APP_DPRINT("\n%d :: App: pAmrParam->eAMRBandMode = %d \n",__LINE__,pAmrParam->eAMRBandMode);
	} else if(!(strcmp(argv[6],"BR795"))) {
		pAmrParam->eAMRBandMode = OMX_AUDIO_AMRBandModeNB5;
    	APP_DPRINT("\n%d :: App: pAmrParam->eAMRBandMode = %d \n",__LINE__,pAmrParam->eAMRBandMode);
	} else if(!(strcmp(argv[6],"BR74"))) {
		pAmrParam->eAMRBandMode = OMX_AUDIO_AMRBandModeNB4;
    	APP_DPRINT("\n%d :: App: pAmrParam->eAMRBandMode = %d \n",__LINE__,pAmrParam->eAMRBandMode);
	} else if(!(strcmp(argv[6],"BR67"))) {
		pAmrParam->eAMRBandMode = OMX_AUDIO_AMRBandModeNB3;
    	APP_DPRINT("\n%d :: App: pAmrParam->eAMRBandMode = %d \n",__LINE__,pAmrParam->eAMRBandMode);
	} else if(!(strcmp(argv[6],"BR59"))) {
		pAmrParam->eAMRBandMode = OMX_AUDIO_AMRBandModeNB2;
    	APP_DPRINT("\n%d :: App: pAmrParam->eAMRBandMode = %d \n",__LINE__,pAmrParam->eAMRBandMode);
	} else if(!(strcmp(argv[6],"BR515"))) {
		pAmrParam->eAMRBandMode = OMX_AUDIO_AMRBandModeNB1;
    	APP_DPRINT("\n%d :: App: pAmrParam->eAMRBandMode = %d \n",__LINE__,pAmrParam->eAMRBandMode);
	} else if(!(strcmp(argv[6],"BR475"))) {
		pAmrParam->eAMRBandMode = OMX_AUDIO_AMRBandModeNB0;
    	APP_DPRINT("\n%d :: App: pAmrParam->eAMRBandMode = %d \n",__LINE__,pAmrParam->eAMRBandMode);
	} else {
		eError = OMX_ErrorBadParameter;
		APP_IPRINT("\n%d :: App: pAmrParam->eAMRBandMode Sending Bad Parameter\n",__LINE__);
		APP_IPRINT("%d :: App: Should Be One of these BitRates BR122, BR102, BR795, BR74, BR67, BR59, BR515, BR475\n",__LINE__);
		goto EXIT;
	}

    APP_DPRINT("\n%d :: App: pAmrParam->eAMRBandMode --> %d \n",__LINE__,pAmrParam->eAMRBandMode);

    if(!(strcmp(argv[7],"DTXON"))) {
    	/**< AMR Discontinuous Transmission Mode is enabled  */
    	pAmrParam->eAMRDTXMode = OMX_AUDIO_AMRDTXModeOnAuto;
    	APP_DPRINT("\n%d :: App: pAmrParam->eAMRDTXMode --> %s \n",__LINE__,argv[7]);
	}else if(!(strcmp(argv[7],"DTXOFF"))) {
		/**< AMR Discontinuous Transmission Mode is disabled */
		pAmrParam->eAMRDTXMode = OMX_AUDIO_AMRDTXModeOff;
    	APP_DPRINT("\n%d :: App: pAmrParam->eAMRDTXMode --> %s \n",__LINE__,argv[7]);
	} else {
		eError = OMX_ErrorBadParameter;
		APP_IPRINT("\n%d :: App: pAmrParam->eAMRDTXMode Sending Bad Parameter\n",__LINE__);
		APP_IPRINT("%d :: App: Should Be One of these Modes DTXON, DTXOFF\n",__LINE__);
		goto EXIT;
	}

	APP_DPRINT("%d :: Setting input port config\n",__LINE__);
	if(!(strcmp(argv[5],"EFR"))) {
		pAmrParam->eAMRDTXMode = OMX_AUDIO_AMRDTXasEFR;		
    	APP_DPRINT("\n%d :: App: pCompPrivateStruct->EFR = %s \n",__LINE__,argv[5]);
	} else if(!(strcmp(argv[5],"NBAMR"))) {
        /*Do nothing, leave eAMRDTXMode as specified before*/   		
    	APP_DPRINT("\n%d :: App: pCompPrivateStruct->NBAMR = %s \n",__LINE__,argv[5]);
	} 
	else {
		eError = OMX_ErrorBadParameter;
		APP_IPRINT("\n%d :: App: pAmrParam->eAMRDTXMode Sending Bad Parameter\n",__LINE__);
		APP_IPRINT("%d :: App: Should Be One of these Modes NBAMR, EFR\n",__LINE__);
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
        APP_DPRINT("%d :: OMX_ErrorBadParameter\n",__LINE__);
        goto EXIT;
    }

#ifndef USE_BUFFER
	APP_DPRINT("%d :: About to call OMX_AllocateBuffer\n",__LINE__);
	for(i = 0; i < numInputBuffers; i++) {
		/* allocate input buffer */
		APP_DPRINT("%d :: About to call OMX_AllocateBuffer for pInputBufferHeader[%d]\n",__LINE__, i);
		eError = OMX_AllocateBuffer(pHandle, &pInputBufferHeader[i], 0, NULL, NBAPP_INPUT_BUFFER_SIZE*3);/*To allow two frames by buffer*/
		if(eError != OMX_ErrorNone) {
			APP_DPRINT("%d :: Error returned by OMX_AllocateBuffer for pInputBufferHeader[%d]\n",__LINE__, i);
			goto EXIT;
		}
	}
	APP_DPRINT("\n%d :: App: pCompPrivateStruct->nBufferSize --> %ld \n",__LINE__,pCompPrivateStruct->nBufferSize);
	for(i = 0; i < numOutputBuffers; i++) {
		/* allocate output buffer */
		APP_DPRINT("%d :: About to call OMX_AllocateBuffer for pOutputBufferHeader[%d]\n",__LINE__, i);
		eError = OMX_AllocateBuffer(pHandle, &pOutputBufferHeader[i], 1, NULL, NBAPP_OUTPUT_BUFFER_SIZE*3);
		if(eError != OMX_ErrorNone) {
			APP_DPRINT("%d :: Error returned by OMX_AllocateBuffer for pOutputBufferHeader[%d]\n",__LINE__, i);
			goto EXIT;
		}
	}
#else
	for(i = 0; i < numInputBuffers; i++) {
		pInputBuffer[i] = (OMX_U8*)newmalloc(NBAPP_INPUT_BUFFER_SIZE*3 + 256);
		APP_MEMPRINT("%d :: [TESTAPP ALLOC] pInputBuffer[%d] = %p\n",__LINE__,i,pInputBuffer[i]);
		if(NULL == pInputBuffer[i]) {
			APP_DPRINT("%d :: Malloc Failed\n",__LINE__);
			eError = OMX_ErrorInsufficientResources;
			goto EXIT;
		}
		pInputBuffer[i] = pInputBuffer[i] + 128;
		
		/*	allocate input buffer */
		APP_DPRINT("%d :: About to call OMX_UseBuffer\n",__LINE__);
		eError = OMX_UseBuffer(pHandle, &pInputBufferHeader[i], 0, NULL, NBAPP_INPUT_BUFFER_SIZE*13, pInputBuffer[i]);
		if(eError != OMX_ErrorNone) {
			APP_DPRINT("%d :: Error returned by OMX_UseBuffer()\n",__LINE__);
			goto EXIT;
		}
	}

	for(i = 0; i < numOutputBuffers; i++) {
		pOutputBuffer[i] = newmalloc (NBAPP_OUTPUT_BUFFER_SIZE*3 + 256);
		APP_MEMPRINT("%d :: [TESTAPP ALLOC] pOutputBuffer[%d] = %p\n",__LINE__,i,pOutputBuffer[i]);
		if(NULL == pOutputBuffer[i]) {
			APP_DPRINT("%d :: Malloc Failed\n",__LINE__);
			eError = OMX_ErrorInsufficientResources;
			goto EXIT;
		}
		pOutputBuffer[i] = pOutputBuffer[i] + 128;

		/* allocate output buffer */
		APP_DPRINT("%d :: About to call OMX_UseBuffer\n",__LINE__);
		eError = OMX_UseBuffer(pHandle, &pOutputBufferHeader[i], 1, NULL, NBAPP_OUTPUT_BUFFER_SIZE*13, pOutputBuffer[i]);
		if(eError != OMX_ErrorNone) {
			APP_DPRINT("%d :: Error returned by OMX_UseBuffer()\n",__LINE__);
			goto EXIT;
		}
	}
#endif


		pCompPrivateStructGain = newmalloc (sizeof(OMX_AUDIO_CONFIG_VOLUMETYPE));
		if(pCompPrivateStructGain == NULL) 
		{
			APP_DPRINT("%d :: App: Malloc Failed\n",__LINE__);
			goto EXIT;
		}
        ArrayOfPointers[4] = (OMX_AUDIO_CONFIG_VOLUMETYPE*) pCompPrivateStructGain;

		/* default setting for gain */
		pCompPrivateStructGain->nSize = sizeof(OMX_AUDIO_CONFIG_VOLUMETYPE);
		pCompPrivateStructGain->nVersion.s.nVersionMajor	= 0xF1;
		pCompPrivateStructGain->nVersion.s.nVersionMinor	= 0xF2;
		pCompPrivateStructGain->nPortIndex					= OMX_DirOutput;
		pCompPrivateStructGain->bLinear						= OMX_FALSE;
		pCompPrivateStructGain->sVolume.nValue				= 50;				/* actual volume */
		pCompPrivateStructGain->sVolume.nMin				= 0;				/* min volume */
		pCompPrivateStructGain->sVolume.nMax				= 100;				/* max volume */

    if (audioinfo->acousticMode == OMX_TRUE) {
        APP_IPRINT("Using Acoustic Device Node Path\n");
        dataPath = DATAPATH_ACDN;
    }
    else if (audioinfo->dasfMode) {
#ifdef RTM_PATH  
        APP_IPRINT("Using Real Time Mixer Path\n");
        dataPath = DATAPATH_APPLICATION_RTMIXER;
#endif

#ifdef ETEEDN_PATH
        APP_IPRINT("Using Eteeden Path\n");
        if (TeeMode == TEEMODE_NONE) {
            dataPath = DATAPATH_APPLICATION;
        }
        else {
            dataPath = DATAPATH_APPLICATION_TEE;
        }
#endif        
    }

	eError = OMX_GetExtensionIndex(pHandle, "OMX.TI.index.config.tispecific",&index);
	if (eError != OMX_ErrorNone) {
		APP_DPRINT("Error returned from OMX_GetExtensionIndex\n");
		goto EXIT;
	}

#ifdef DSP_RENDERING_ON 	
    cmd_data.hComponent = pHandle;
    cmd_data.AM_Cmd = AM_CommandIsInputStreamAvailable;
    
    cmd_data.param1 = 0;
    if((write(nbamrencdfwrite, &cmd_data, sizeof(cmd_data)))<0) {
        APP_IPRINT("%d ::OMX_AmrDecoder.c ::[NBAMR Dec Component] - send command to audio manager\n", __LINE__);
    }
    if((read(nbamrencfdread, &cmd_data, sizeof(cmd_data)))<0) {
        APP_IPRINT("%d ::OMX_AmrDecoder.c ::[NBAMR Dec Component] - failure to get data from the audio manager\n", __LINE__);
		goto EXIT;
    }
    audioinfo->streamId = cmd_data.streamID;
    streamId = audioinfo->streamId;
#endif

	eError = OMX_SetConfig (pHandle, index, audioinfo);
    if(eError != OMX_ErrorNone) {
        eError = OMX_ErrorBadParameter;
        APP_DPRINT("%d :: Error from OMX_SetConfig() function\n",__LINE__);
        goto EXIT;
    }

    eError = OMX_GetExtensionIndex(pHandle, "OMX.TI.index.config.nbamr.datapath",&index);
	if (eError != OMX_ErrorNone) {
		APP_IPRINT("Error getting extension index\n");
		goto EXIT;
	}

	eError = OMX_SetConfig (pHandle, index, &dataPath);
    if(eError != OMX_ErrorNone) {
        eError = OMX_ErrorBadParameter;
        APP_DPRINT("%d :: AmrDecTest.c :: Error from OMX_SetConfig() function\n",__LINE__);
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

    /* Wait for startup to complete */

    eError = WaitForState(pHandle, OMX_StateIdle);
#ifdef OMX_GETTIME
	GT_END("Call to SendCommand <OMX_StateIdle>");
#endif
    if(eError != OMX_ErrorNone) {
        APP_DPRINT( "Error:  hAmrEncoder->WaitForState reports an eError %X\n", eError);
        goto EXIT;
    }

   if (audioinfo->dasfMode ==  1) { 
        /* get streamID back to application */
	    eError = OMX_GetExtensionIndex(pHandle, "OMX.TI.index.config.nbamrstreamIDinfo",&index);
     	if (eError != OMX_ErrorNone) {
      		APP_IPRINT("Error getting extension index\n");
        		goto EXIT;
          	}
           
       	eError = OMX_GetConfig (pHandle, index, streaminfo);
        if(eError != OMX_ErrorNone) {
            eError = OMX_ErrorBadParameter;
            APP_DPRINT("%d :: AmrEncTest.c :: Error from OMX_GetConfig() function\n",__LINE__);
            goto EXIT;
        }

        streamId = streaminfo->streamId;
        APP_IPRINT("***************StreamId=%ld******************\n", streamId);
    }   

    for(i = 0; i < testcnt; i++) {
	frmCnt = 1;
	nFrameCount = 1;
	nOutBuff = 1;
	if(i > 0) {
		APP_IPRINT("%d :: Encoding the file for %d Time in TESTCASE 5 OR TESTCSE 4\n",__LINE__,i+1);
		fIn = fopen(argv[1], "r");
		if(fIn == NULL) {
			fprintf(stderr, "Error:  failed to open the file %s for readonly access\n", argv[1]);
			goto EXIT;
		}
		fOut = fopen("TC5_Amr1.amr", "w");
		if(fOut == NULL) {
			fprintf(stderr, "Error:  failed to create the output file %s\n", argv[2]);
			goto EXIT;
		}
	}

	APP_IPRINT("%d :: App: Sending OMX_StateExecuting Command\n",__LINE__);
#ifdef OMX_GETTIME
	GT_START();
#endif
    eError = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
    if(eError != OMX_ErrorNone) {
        APP_IPRINT("Error from SendCommand-Executing State function\n");
        goto EXIT;
    }
    eError = WaitForState(pHandle, OMX_StateExecuting);
#ifdef OMX_GETTIME
	GT_END("Call to SendCommand <OMX_StateExecuting>");
#endif
    if(eError != OMX_ErrorNone) {
        APP_DPRINT( "Error:  hAmrEncoder->WaitForState reports an eError %X\n", eError);
        goto EXIT;
    }

    if (audioinfo->dasfMode ==  1) 
    { 
		APP_IPRINT("%d :: App: No.of Frames Encoding = %d\n",__LINE__, atoi(argv[10]));
	}

    pComponent = (OMX_COMPONENTTYPE *)pHandle;

	if(audioinfo->dasfMode == 0) {
		for (k=0; k < numInputBuffers; k++) {
			OMX_BUFFERHEADERTYPE* pBuffer = pInputBufferHeader[k];			
			pBuffer->nFlags=0;
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
	#ifdef OMX_GETTIME
         if (kk==0)
         { 
            GT_FlagF=1;  /* 1 = First Buffer,  0 = Not First Buffer  */
            GT_START(); /* Fill Buffer */
         }
	#endif
		pComponent->FillThisBuffer(pHandle, pOutputBufferHeader[kk]);
	}

	eError = pComponent->GetState(pHandle, &state);
	if(eError != OMX_ErrorNone) {
		APP_DPRINT("%d :: pComponent->GetState has returned status %X\n",__LINE__, eError);
		goto EXIT;
	}
    retval = 1;

#ifndef WAITFORRESOURCES
    while((eError == OMX_ErrorNone) && (state != OMX_StateIdle) && (state != OMX_StateInvalid) ){
	if(1){
#else
    while(1) {
        if((eError == OMX_ErrorNone) && (state != OMX_StateIdle) && (state != OMX_StateInvalid) ){  
#endif
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
        
        if(!retval){
              NoDataRead++;
              if(NoDataRead==2){
                      APP_IPRINT("Stoping component since No data is read from the pipes\n");          
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
					OMX_BUFFERHEADERTYPE* pBuffer;
					read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
					if (frmCount==15 && tcID ==3){ /*Pause the component*/
						APP_IPRINT("App: Pausing Component for 5 Seconds\n");
						PauseComponent(pHandle);
						sleep(2);
						APP_IPRINT("App: Resume Component\n");
						PlayComponent(pHandle);
					}
					if (frmCount==20 && tcID ==4){ /*Stop the component*/
                        tcID = 1;
						StopComponent(pHandle);
						break;
                    }
					eError =  send_input_buffer(pHandle, pBuffer, fIn);
				}
			} else {
                   if (frmCount==15 && tcID ==3){  /*Pause the component*/
                        tcID = 1;
						APP_IPRINT("App: Pausing Component for 5 Seconds\n");
						PauseComponent(pHandle);
						sleep(2);
						APP_IPRINT("App: Resume Component\n");
						PlayComponent(pHandle);						
					}
					if (nFrameCount==50 && tcID ==4){ /*Stop the component*/
					    APP_IPRINT("Stoping the Component And Starting Again\n");
						StopComponent(pHandle);
						nFrameCount = 0;
						break;
                    }
					if(nFrameCount == 10 && tcID == 7)								
					{
							/* set high gain for record stream */
							APP_IPRINT("[NBAMR encoder] --- will set stream gain to high\n");
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
							APP_IPRINT("[NBAMR encoder] --- will set stream gain to low\n");
            				pCompPrivateStructGain->sVolume.nValue = 0x2000;
							eError = OMX_SetConfig(pHandle, OMX_IndexConfigAudioVolume, pCompPrivateStructGain);
							if (eError != OMX_ErrorNone) 
							{
								eError = OMX_ErrorBadParameter;
								goto EXIT;
							}							
					}                    
					APP_DPRINT("%d :: NBAMR ENCODER RUNNING UNDER DASF MODE \n",__LINE__);
					if(nFrameCount == atoi(argv[10])) {
						StopComponent(pHandle);  
					}
					APP_DPRINT("%d :: NBAMR ENCODER READING DATA FROM DASF  \n",__LINE__);
				}
		break;		
		default:
				APP_DPRINT("%d :: ### Simple DEFAULT Case Here ###\n",__LINE__);
		}

        if( FD_ISSET(OpBuf_Pipe[0], &rfds) ) {
            OMX_BUFFERHEADERTYPE* pBuf;
            read(OpBuf_Pipe[0], &pBuf, sizeof(pBuf));
            APP_DPRINT("%d :: App: pBuf->nFilledLen = %ld\n",__LINE__, pBuf->nFilledLen);
            nFrameLen = pBuf->nFilledLen;
            if(FrameMode==1) { /* Mime Mode */
	            if(1 == nFrameCount) {
                   char MimeHeader[] = {0x23, 0x21, 0x41, 0x4d, 0x52, 0x0a};
                   fwrite(MimeHeader, 1, NBAMRENC_MIME_HEADER_LEN, fOut);
	               fflush(fOut);
                   APP_IPRINT("%d :: App: MIME Supported:: FrameLen = %d\n",__LINE__, nFrameLen);
                }
            }
			APP_DPRINT("%d :: App: nFrameLen = %d \n",__LINE__, nFrameLen);
            if (nFrameLen != 0) {
				APP_DPRINT("%d :: Writing OutputBuffer No: %d to the file nWrite = %d \n",__LINE__, nOutBuff, nFrameLen);

				fwrite(pBuf->pBuffer, 1, nFrameLen, fOut);
				fflush(fOut);
			}
			if(pBuf->nFlags == OMX_BUFFERFLAG_EOS) {
				   APP_IPRINT("%d :: App: OMX_BUFFERFLAG_EOS is received\n",__LINE__);
				   APP_IPRINT("%d :: App: Shutting down ---------- \n",__LINE__);				   
				   StopComponent(pHandle);
				   pBuf->nFlags = 0;
			}
			else{
				nFrameCount++;
				nOutBuff++;
                pComponent->FillThisBuffer(pHandle, pBuf);
				APP_DPRINT("%d :: App: pBuf->nFlags = %ld\n",__LINE__, pBuf->nFlags);
			}
        }


if( FD_ISSET(Event_Pipe[0], &rfds) ) {
                OMX_U8 pipeContents;
                read(Event_Pipe[0], &pipeContents, sizeof(OMX_U8));

                if (pipeContents == 0) {
                    APP_IPRINT("Test app received OMX_ErrorResourcesPreempted\n");
                    WaitForState(pHandle,OMX_StateIdle);

				for(i=0; i < numInputBuffers; i++) {
					APP_DPRINT("%d :: App: About to newfree pInputBufferHeader[%d]\n",__LINE__, i);
					eError = OMX_FreeBuffer(pHandle, NBAPP_INPUT_PORT, pInputBufferHeader[i]);
					if((eError != OMX_ErrorNone)) {
						APP_DPRINT("%d:: Error in FreeBuffer function\n",__LINE__);
						goto EXIT;
					}
					
				}

				for(i=0; i < numOutputBuffers; i++) {
					APP_DPRINT("%d :: App: About to newfree pOutputBufferHeader[%d]\n",__LINE__, i);
					eError = OMX_FreeBuffer(pHandle, NBAPP_OUTPUT_PORT, pOutputBufferHeader[i]);
					if((eError != OMX_ErrorNone)) {
						APP_DPRINT("%d :: Error in Free Buffer function\n",__LINE__);
						goto EXIT;
					}
					
			    }

#ifdef USE_BUFFER

		for(i=0; i < numInputBuffers; i++) {
			if(pInputBuffer[i] != NULL){
	       		APP_MEMPRINT("%d :: App: [TESTAPPFREE] pInputBuffer[%d] = %p\n",__LINE__,i,pInputBuffer[i]);
	            pInputBuffer[i] = pInputBuffer[i] - 128;       		
				newfree(pInputBuffer[i]);
				pInputBuffer[i] = NULL;
			}
		}
#endif                        

                	OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateLoaded, NULL);
                    WaitForState(pHandle, OMX_StateLoaded); 

                    OMX_SendCommand(pHandle,OMX_CommandStateSet,OMX_StateWaitForResources,NULL);
                    WaitForState(pHandle,OMX_StateWaitForResources);
                }
                else if (pipeContents == 1) {
                    APP_IPRINT("Test app received OMX_ErrorResourcesAcquired\n");

                    OMX_SendCommand(pHandle,OMX_CommandStateSet,OMX_StateIdle,NULL);
                    	for(i = 0; i < numOutputBuffers; i++) {
							/* allocate output buffer */
							APP_DPRINT("%d :: About to call OMX_AllocateBuffer for pOutputBufferHeader[%d]\n",__LINE__, i);
							eError = OMX_AllocateBuffer(pHandle, &pOutputBufferHeader[i], 1, NULL, NBAPP_OUTPUT_BUFFER_SIZE*3);
							if(eError != OMX_ErrorNone) {
								APP_DPRINT("%d :: Error returned by OMX_AllocateBuffer for pOutputBufferHeader[%d]\n",__LINE__, i);
								goto EXIT;
							}
						}

                    WaitForState(pHandle,OMX_StateIdle);

                    OMX_SendCommand(pHandle,OMX_CommandStateSet,OMX_StateExecuting,NULL);
                    WaitForState(pHandle,OMX_StateExecuting);

                    rewind(fIn);

                    for(i = 0; i < numOutputBuffers; i++) {
						send_input_buffer (pHandle, pOutputBufferHeader[i], fIn);
                    }
                }                

                if (pipeContents == 2) {

#ifdef OMX_GETTIME
                    GT_START();
#endif
                                
                    OMX_SendCommand(pHandle,OMX_CommandStateSet,OMX_StateIdle,NULL);
                    WaitForState(pHandle,OMX_StateIdle);

#ifdef OMX_GETTIME
                    GT_END("Call to SendCommand <OMX_StateIdle>");
#endif

#ifdef WAITFORRESOURCES
					for(i=0; i < numInputBuffers; i++) {
						APP_DPRINT("%d :: App: About to newfree pInputBufferHeader[%d]\n",__LINE__, i);
						eError = OMX_FreeBuffer(pHandle, NBAPP_INPUT_PORT, pInputBufferHeader[i]);
						if((eError != OMX_ErrorNone)) {
							APP_DPRINT("%d:: Error in FreeBuffer function\n",__LINE__);
							goto EXIT;
						}
						
					}

					for(i=0; i < numOutputBuffers; i++) {
						APP_DPRINT("%d :: App: About to newfree pOutputBufferHeader[%d]\n",__LINE__, i);
						eError = OMX_FreeBuffer(pHandle, NBAPP_OUTPUT_PORT, pOutputBufferHeader[i]);
						if((eError != OMX_ErrorNone)) {
							APP_DPRINT("%d :: Error in Free Buffer function\n",__LINE__);
							goto EXIT;
						}
						
				    }
                                
                	OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateLoaded, NULL);
                    WaitForState(pHandle, OMX_StateLoaded); 

                    goto SHUTDOWN;
#endif
                }
            }



		
            eError = pComponent->GetState(pHandle, &state);
            if(eError != OMX_ErrorNone) {
                APP_DPRINT("%d :: pComponent->GetState has returned status %X\n",__LINE__, eError);
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
    	APP_IPRINT("%d :: App: The current state of the component = %d \n",__LINE__,state);
    	fclose(fOut);
    	fclose(fIn);
        FirstTime = 1;
        NoDataRead = 0;
		if(tcID == 4)
		    tcID =1;
		APP_IPRINT("%d :: App: NBAMR Encoded = %d Frames \n",__LINE__,(nOutBuff));
    } /*Test Case 4 & 5 Inner for loop ends here  */

	/* newfree the Allocate and Use Buffers */	
	APP_IPRINT("%d :: App: Freeing the Allocate OR Use Buffers in TestApp\n",__LINE__);	
	for(i=0; i < numInputBuffers; i++) {
		APP_DPRINT("%d :: App: About to newfree pInputBufferHeader[%d]\n",__LINE__, i);
		eError = OMX_FreeBuffer(pHandle, NBAPP_INPUT_PORT, pInputBufferHeader[i]);
		if((eError != OMX_ErrorNone)) {
			APP_DPRINT("%d:: Error in FreeBuffer function\n",__LINE__);
			goto EXIT;
		}
		pInputBufferHeader[i] = NULL;
	}

	for(i=0; i < numOutputBuffers; i++) {
		APP_DPRINT("%d :: App: About to newfree pOutputBufferHeader[%d]\n",__LINE__, i);
		eError = OMX_FreeBuffer(pHandle, NBAPP_OUTPUT_PORT, pOutputBufferHeader[i]);
		if((eError != OMX_ErrorNone)) {
			APP_DPRINT("%d :: Error in Free Buffer function\n",__LINE__);
			goto EXIT;
		}
		pOutputBufferHeader[i] = NULL;
    }
   
#ifdef USE_BUFFER
	/* newfree the App Allocated Buffers */
	APP_IPRINT("%d :: App: Freeing the App Allocated Buffers in TestApp\n",__LINE__);

	for(i=0; i < numInputBuffers; i++) {		
		if(pInputBuffer[i] != NULL){
	        APP_MEMPRINT("%d :: App: [TESTAPPFREE] pInputBuffer[%d] = %p\n",__LINE__,i,pInputBuffer[i]);
	        pInputBuffer[i] = pInputBuffer[i] - 128;
			newfree(pInputBuffer[i]);
			pInputBuffer[i] = NULL;
		}
	}

	for(i=0; i < numOutputBuffers; i++) {
		if(pOutputBuffer[i] != NULL){
    		APP_MEMPRINT("%d :: App: [TESTAPPFREE] pOutputBuffer[%d] = %p\n",__LINE__,i, pOutputBuffer[i]);
	    	pOutputBuffer[i] = pOutputBuffer[i] - 128;
			newfree(pOutputBuffer[i]);
			pOutputBuffer[i] = NULL;
		}
	}
#endif

	APP_IPRINT ("%d :: App: Sending the OMX_StateLoaded Command\n",__LINE__);
#ifdef OMX_GETTIME
	GT_START();
#endif
    eError = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateLoaded, NULL);
    if(eError != OMX_ErrorNone) {
        APP_DPRINT("%d:: Error from SendCommand-Idle State function\n",__LINE__);
        goto EXIT;
    }
    eError = WaitForState(pHandle, OMX_StateLoaded);
#ifdef OMX_GETTIME
	GT_END("Call to SendCommand <OMX_StateLoaded>");
#endif
    if ( eError != OMX_ErrorNone ){
        APP_IPRINT("Error: WaitForState has timed out %d", eError);
		goto EXIT;
    }
					   		  
	APP_IPRINT ("%d :: App: Sending the OMX_CommandPortDisable Command\n",__LINE__);
	eError = OMX_SendCommand(pHandle, OMX_CommandPortDisable, -1, NULL);
    if(eError != OMX_ErrorNone) {
        APP_DPRINT("%d:: Error from SendCommand OMX_CommandPortDisable\n",__LINE__);
        goto EXIT;
    }
    

#ifdef WAITFORRESOURCES
    eError = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateWaitForResources, NULL);
    if(eError != OMX_ErrorNone) {
        APP_DPRINT ("%d Error from SendCommand-Idle State function\n",__LINE__);
        goto EXIT;
    }
    eError = WaitForState(pHandle, OMX_StateWaitForResources);

    /* temporarily put this here until I figure out what should really happen here */
    sleep(10);
    /* temporarily put this here until I figure out what should really happen here */
#endif    
SHUTDOWN:

	APP_IPRINT("%d :: App: Freeing the Memory Allocated in TestApp\n",__LINE__);

    APP_MEMPRINT("%d :: App: [TESTAPPFREE] %p\n",__LINE__,pAmrParam);
    if(pAmrParam != NULL){
	    newfree(pAmrParam);
	    pAmrParam = NULL;
	}
    APP_MEMPRINT("%d :: App: [TESTAPPFREE] %p\n",__LINE__,pCompPrivateStruct);
	if(pCompPrivateStruct != NULL){
		newfree(pCompPrivateStruct);
		pCompPrivateStruct = NULL;
	}
    APP_MEMPRINT("%d :: App: [TESTAPPFREE] %p\n",__LINE__,audioinfo);
	if(audioinfo != NULL){
		newfree(audioinfo);
		audioinfo = NULL;
	}
    APP_MEMPRINT("%d :: App: [TESTAPPFREE] %p\n",__LINE__,streaminfo);
    if(streaminfo != NULL){
		newfree(streaminfo);
		streaminfo = NULL;
	}
	
	APP_IPRINT("%d :: App: Closing the Input and Output Pipes\n",__LINE__);
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

	eError = close(Event_Pipe[0]);
	if (0 != eError && OMX_ErrorNone == eError) {
		eError = OMX_ErrorHardware;
		APP_DPRINT("%d :: Error while closing Event_Pipe[0]\n",__LINE__);
		goto EXIT;
	}
		
	eError = close(Event_Pipe[1]);
	if (0 != eError && OMX_ErrorNone == eError) {
		eError = OMX_ErrorHardware;
		APP_DPRINT("%d :: Error while closing Event_Pipe[1]\n",__LINE__);
		goto EXIT;
	}
	
    APP_IPRINT("%d :: App: Free the Component handle\n",__LINE__);
    /* Unload the NBAMR Encoder Component */
    eError = TIOMX_FreeHandle(pHandle);
	if((eError != OMX_ErrorNone)) {
		APP_DPRINT("%d :: Error in Free Handle function\n",__LINE__);
		goto EXIT;
	}
	APP_IPRINT("%d :: App: Free Handle returned Successfully\n",__LINE__);

#ifdef DSP_RENDERING_ON 
    cmd_data.hComponent = pHandle;
    cmd_data.AM_Cmd = AM_Exit;

    if((write(nbamrencdfwrite, &cmd_data, sizeof(cmd_data)))<0)
        APP_IPRINT("%d ::- send command to audio manager\n",__LINE__);

    close(nbamrencdfwrite);
    close(nbamrencfdread);
#endif


	
	newfree(pCompPrivateStructGain);

	} /*Outer for loop ends here */

	pthread_mutex_destroy(&WaitForState_mutex);
    pthread_cond_destroy(&WaitForState_threshold);

    APP_IPRINT("%d :: *********************************************************************\n",__LINE__);
    APP_IPRINT("%d :: NOTE: An output file %s has been created in file system\n",__LINE__,argv[2]);
    APP_IPRINT("%d :: *********************************************************************\n",__LINE__);
EXIT:
	if(bInvalidState==OMX_TRUE)
	{
#ifndef USE_BUFFER
		eError = FreeAllResources(pHandle,
								pInputBufferHeader[0],
								pOutputBufferHeader[0],
								numInputBuffers,
								numOutputBuffers,
								fIn,
                                fOut);
#else
		eError = FreeAllResources(pHandle,
									pInputBuffer,
									pOutputBuffer,
									numInputBuffers,
									numOutputBuffers,
									fIn,
                                    fOut);
#endif
   }
#ifdef APP_DEBUGMEM    
    APP_IPRINT("\n-Printing memory not deleted-\n");
    for(i=0;i<500;i++){
        if (lines[i]!=0){
             APP_IPRINT(" --->%d Bytes allocated on File:%s Line: %d\n",bytes[i],file[i],lines[i]); 
             }
    }
#endif   
#ifdef OMX_GETTIME
	GT_END("AMR_Enc test <End>");
	OMX_ListDestroy(pListHead);	
#endif 	
    return eError;
}

OMX_ERRORTYPE send_input_buffer(OMX_HANDLETYPE pHandle, OMX_BUFFERHEADERTYPE* pBuffer, FILE *fIn)
{
	OMX_ERRORTYPE error = OMX_ErrorNone;
	OMX_COMPONENTTYPE *pComponent = (OMX_COMPONENTTYPE *)pHandle; 

	if(FirstTime){
		if(mframe){
			nRead = fread(pBuffer->pBuffer, 1, NBAPP_INPUT_BUFFER_SIZE*2, fIn);
		}
		else{
			nRead = fread(pBuffer->pBuffer, 1, NBAPP_INPUT_BUFFER_SIZE, fIn);
		}		
		pBuffer->nFilledLen = nRead;
	}
	else{
		memcpy(pBuffer->pBuffer, NextBuffer,nRead);
		pBuffer->nFilledLen = nRead;
	}
	
	if(mframe){
			nRead = fread(NextBuffer, 1, NBAPP_INPUT_BUFFER_SIZE*2, fIn);
	}
	else{
			nRead = fread(NextBuffer, 1, NBAPP_INPUT_BUFFER_SIZE, fIn);
	}	
	
	if(nRead < NBAPP_INPUT_BUFFER_SIZE && !DasfMode){


	#ifdef OMX_GETTIME
		GT_START();
	#endif
		error = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateIdle, NULL);
    	error = WaitForState(pHandle, OMX_StateIdle);
	#ifdef OMX_GETTIME
		GT_END("Call to SendCommand <OMX_StateIdle>");
	#endif
		if(error != OMX_ErrorNone) {
			APP_DPRINT ("%d :: Error from SendCommand-Idle(Stop) State function\n",__LINE__);
			goto EXIT;
		}


		pBuffer->nFlags = OMX_BUFFERFLAG_EOS;
	}else{
          pBuffer->nFlags = 0;
          }
		
	if(pBuffer->nFilledLen!=0){
    	/*APP_DPRINT("pBuffer->nFilledLen %d \n",pBuffer->nFilledLen);*/        
    	if(pBuffer->nFlags == OMX_BUFFERFLAG_EOS){
                           APP_IPRINT("Sending Last Input Buffer from App\n");
        }
/*        APP_DPRINT("Sending %d bytes to Comp\n", pBuffer->nFilledLen);*/
        pBuffer->nTimeStamp = rand() % 100;

	if (!preempted) {
	    error = pComponent->EmptyThisBuffer(pHandle, pBuffer);
        if (error == OMX_ErrorIncorrectStateOperation) 
				error = 0;		
		}
    }
   	FirstTime=0;
EXIT:
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
					fprintf(stderr, "\nError:  hAmrEncoder->WaitForState reports an error %X!!!!!!!\n", error);
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
					fprintf(stderr, "\nError:  hAmrEncoder->WaitForState reports an error %X!!!!!!!\n", error);
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
					fprintf(stderr, "\nError:  hAmrEncoder->WaitForState reports an error %X!!!!!!!\n", error);
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
			                FILE* fileIn, FILE* fileOut)
{
	APP_IPRINT("%d::Freeing all resources by state invalid \n",__LINE__);
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_U16 i; 

	for(i=0; i < NIB; i++) {		   
		   if(pBufferIn+i!=NULL){
                APP_IPRINT("%d :: APP: About to newfree pInputBufferHeader[%d]\n",__LINE__, i);               
		        eError = OMX_FreeBuffer(pHandle, OMX_DirInput, pBufferIn+i);
         }

	}


	for(i=0; i < NOB; i++) {
          if(pBufferOut+i!=NULL){
		   APP_IPRINT("%d :: APP: About to newfree pOutputBufferHeader[%d]\n",__LINE__, i);
		   eError = OMX_FreeBuffer(pHandle, OMX_DirOutput, pBufferOut+i);
         }
	}

	/*i value is fixed by the number calls to newmalloc in the App */
	for(i=0; i<5;i++)  
	{
		if (ArrayOfPointers[i] != NULL)
			newfree(ArrayOfPointers[i]);
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
							int NIB,int NOB,
							FILE* fileIn, FILE* fileOut)
{

		OMX_ERRORTYPE eError = OMX_ErrorNone;
		OMX_U16 i; 
		APP_IPRINT("%d::Freeing all resources by state invalid \n",__LINE__);
    	/* newfree the UseBuffers */
	    for(i=0; i < NIB; i++) {
		   UseInpBuf[i] = UseInpBuf[i] - 128;
		   APP_IPRINT("%d :: [TESTAPPFREE] pInputBuffer[%d] = %p\n",__LINE__,i,(UseInpBuf[i]));
		   if(UseInpBuf[i] != NULL){
			  newfree(UseInpBuf[i]);
			  UseInpBuf[i] = NULL;
		   }
		}

	    for(i=0; i < NOB; i++) {
		   UseOutBuf[i] = UseOutBuf[i] - 128;
		   APP_IPRINT("%d :: [TESTAPPFREE] pOutputBuffer[%d] = %p\n",__LINE__,i, UseOutBuf[i]);
		   if(UseOutBuf[i] != NULL){
			  newfree(UseOutBuf[i]);
			  UseOutBuf[i] = NULL;
		   }
		}

	/*i value is fixed by the number calls to newmalloc in the App */
		for(i=0; i<4;i++)  
		{
			if (ArrayOfPointers[i] != NULL)
				newfree(ArrayOfPointers[i]);
		}
	
		OMX_FreeHandle(pHandle);
	
		return eError;
}

#endif

