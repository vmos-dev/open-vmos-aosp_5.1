
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
/* ==============================================================================
*             Texas Instruments OMAP (TM) Platform Software
*  (c) Copyright Texas Instruments, Incorporated.  All Rights Reserved.
*
*  Use of this software is controlled by the terms and conditions found
*  in the license agreement under which this software has been supplied.
* ============================================================================ */
/**
* @file PcmEncTest.c
*
* This file implements Test for PCM encoder OMX Component, which is fully
* compliant with the Khronos 1.0 specification.
*
* @path  $(CSLPATH)\
*
* @rev  1.0
*/
/* ----------------------------------------------------------------------------
*!
*! Revision History
*! ===================================
*! 24-Jan-2006 rg:  Initial Version. Change required per OMAPSWxxxxxxxxx
*! to provide _________________.
*!
* ============================================================================= */

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

#include <OMX_Index.h>
#include <OMX_Types.h>
#include <OMX_Component.h>
#include <OMX_Core.h>
#include <OMX_Audio.h>
/*#include <TIDspOmx.h>*/

#include <pthread.h>
#include <stdio.h>
#include <linux/soundcard.h>
#ifdef DSP_RENDERING_ON		
#include <AudioManagerAPI.h>
#endif
#include <time.h>
#ifdef OMX_GETTIME
#include <OMX_Common_Utils.h>
#include <OMX_GetTime.h>     /*Headers for Performance & measuremet    */
#endif

FILE *fpRes;
/**flush*/
int num_flush = 0;
int nNextFlushFrame = 100;
/***/

#define OMX_AMRDEC_NonMIME 0
#define MIME_HEADER_LEN 6

#undef APP_DEBUG
#undef APP_MEMCHECK
#define DASF 1
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

/* Borrowed from http://www.dtek.chalmers.se/groups/dvd/dist/oss_audio.c */
/* AFMT_AC3 is really IEC61937 / IEC60958, mpeg/ac3/dts over spdif */
#ifndef AFMT_AC3
#define AFMT_AC3        0x00000400  /* Dolby Digital AC3 */
#endif
#ifndef AFMT_S32_LE
#define AFMT_S32_LE     0x00001000  /* 32/24-bits, in 24bit use the msbs */
#endif
#ifndef AFMT_S32_BE
#define AFMT_S32_BE     0x00002000  /* 32/24-bits, in 24bit use the msbs */
#endif


#define FIFO1 "/dev/fifo.1"
#define FIFO2 "/dev/fifo.2"


/* ======================================================================= */
/**
 * @def    OUTPUT_WBAMRDEC_BUFFER_SIZE   Default output buffer size
 *
 */
/* ======================================================================= */
#define OUTPUT_WBAMRDEC_BUFFER_SIZE 640

/* ======================================================================= */
/**
 * @def    INPUT_WBAMRDEC_BUFFER_SIZE   Default input buffer size
 *
 */
/* ======================================================================= */
#define INPUT_WBAMRDEC_BUFFER_SIZE 116

/* ======================================================================= */
/**
 * @def    APP_DEBUGMEM    This Macro turns On the logic to detec memory
 *                         leaks on the App. To debug the component, 
 *                         WBAMRDEC_DEBUGMEM must be defined.
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

#define newmalloc(x) mymalloc(__LINE__,__FILE__,x)
#define newfree(z) myfree(z,__LINE__,__FILE__)

void * mymalloc(int line, char *s, int size);
int myfree(void *dp, int line, char *s);

#else

#define newmalloc(x) malloc(x)
#define newfree(z) free(z)

#endif
    
#ifdef OMX_GETTIME
  OMX_ERRORTYPE eError = OMX_ErrorNone;
  int GT_FlagE = 0;  /* Fill Buffer 1 = First Buffer,  0 = Not First Buffer  */
  int GT_FlagF = 0;  /*Empty Buffer  1 = First Buffer,  0 = Not First Buffer  */
  static OMX_NODE* pListHead = NULL;
#endif

enum
{
    NEXT_BITRATE_UNCHANGED = 0,
    NEXT_BITRATE_CHANGED
};

typedef enum
{
    MIME_NO_SUPPORT = 0,
    MIME_SUPPORTED
}MIME_Settings;


/* ======================================================================= */
/** WBAMRDEC_BUFDATA       
*
*  @param  nFrames           # of Frames processed on the Output Buffer.
*
*/
/*  ==================================================================== */
typedef struct WBAMRDEC_BUFDATA {
   OMX_U8 nFrames;     
}WBAMRDEC_BUFDATA;

int GetInfoFromBufferHeader(unsigned char **pBufPtr, int *pCurBitRate,
                                                int *pNextBitRateFlag);

void ResetBufferPointers(unsigned char **pBuffer);
int maxint(int a, int b);

/*int gMimeFlag = 0;*/
OMX_S16 WBAmrFrameFormat = 0;
int gStateNotifi = 0;
int gState;
int inputPortDisabled = 0;
int outputPortDisabled = 0;
int alternate = 0;
int numRead;
int cnn=1;
int cnnn=1;
int testCaseNo = 0;
OMX_U8 NextBuffer[INPUT_WBAMRDEC_BUFFER_SIZE*3];
OMX_U8 TempBuffer[INPUT_WBAMRDEC_BUFFER_SIZE];

#undef  WAITFORRESOURCES
pthread_mutex_t WaitForState_mutex;
pthread_cond_t  WaitForState_threshold;
OMX_U8          WaitForState_flag;
OMX_U8      TargetedState;
/**flush*/
pthread_mutex_t WaitForOUTFlush_mutex;
pthread_cond_t  WaitForOUTFlush_threshold;
pthread_mutex_t WaitForINFlush_mutex;
pthread_cond_t  WaitForINFlush_threshold;
/*****/
int FirstTime = 1;
int nRead=0;
static OMX_BOOL bInvalidState;
void* ArrayOfPointers[6];
WBAMRDEC_BUFDATA* OutputFrames;

#ifdef DSP_RENDERING_ON		
AM_COMMANDDATATYPE cmd_data;
#endif        
int fill_data (OMX_U8 *pBuf, int mode, FILE *fIn);

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
typedef enum COMPONENTS {
    COMP_1,
    COMP_2
}COMPONENTS;
void ConfigureAudio();

OMX_STRING strAmrEncoder = "OMX.TI.WBAMR.decode";
int IpBuf_Pipe[2];
int OpBuf_Pipe[2];
int Event_Pipe[2];

int preempted = 0;

OMX_ERRORTYPE send_input_buffer (OMX_HANDLETYPE pHandle, OMX_BUFFERHEADERTYPE* pBuffer, FILE *fIn);
OMX_ERRORTYPE StopComponent(OMX_HANDLETYPE *pHandle);
OMX_ERRORTYPE PauseComponent(OMX_HANDLETYPE *pHandle);
OMX_ERRORTYPE PlayComponent(OMX_HANDLETYPE *pHandle);

fd_set rfds;

OMX_S16 dasfmode = 0;

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
     eError = pComponent->GetState(pHandle, &CurState);
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

/* ================================================================================= */
/**
* @fn EventHandler() description for SendCommand
EventHandler().
App event handler
*
*  @see         OMX_Core.h
*/
/* ================================================================================ */

OMX_ERRORTYPE EventHandler(
        OMX_HANDLETYPE hComponent,
        OMX_PTR pAppData,
        OMX_EVENTTYPE eEvent,
        OMX_U32 nData1,
        OMX_U32 nData2,
        OMX_PTR pEventData)
{

   OMX_COMPONENTTYPE *pComponent = (OMX_COMPONENTTYPE *)hComponent;
   OMX_STATETYPE state;
   OMX_ERRORTYPE eError;

   OMX_U8 writeValue;  
   
#ifdef APP_DEBUG
   int iComp = *((int *)(pAppData));
#endif
   eError = pComponent->GetState (hComponent, &state);
   if(eError != OMX_ErrorNone) {
       APP_DPRINT("%d :: App: Error returned from GetState\n",__LINE__);
   }

   switch (eEvent) {
       case OMX_EventCmdComplete:
	   	  /**flush*/
			if (nData1 == OMX_CommandFlush){    
	            if(nData2 == 0){
	                pthread_mutex_lock(&WaitForINFlush_mutex);
	                pthread_cond_signal(&WaitForINFlush_threshold);
	                pthread_mutex_unlock(&WaitForINFlush_mutex);
	            }
	            if(nData2 == 1){
	                pthread_mutex_lock(&WaitForOUTFlush_mutex);
	                pthread_cond_signal(&WaitForOUTFlush_threshold);
	                pthread_mutex_unlock(&WaitForOUTFlush_mutex);
	            }
	        }
		/****/
           APP_DPRINT ( "%d :: App: Component State Changed To %d\n", __LINE__,state);

		if (nData1 == OMX_CommandPortDisable) {
			if (nData2 == OMX_DirInput) {
				inputPortDisabled = 1;
			}
			if (nData2 == OMX_DirOutput) {
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
           break;
       case OMX_EventMark:
           break;
#ifdef WAITFORRESOURCES
	   case OMX_EventBufferFlag:
       	   APP_DPRINT( "%d :: App: Component OMX_EventBufferFlag = %d\n", __LINE__,eEvent);
		   writeValue = 2;  
	       write(Event_Pipe[1], &writeValue, sizeof(OMX_U8));
           break;
#endif		   
       case OMX_EventResourcesAcquired:
       	   APP_DPRINT( "%d :: App: Component OMX_EventResourcesAcquired = %d\n", __LINE__,eEvent);
		   writeValue = 1;
           write(Event_Pipe[1], &writeValue, sizeof(OMX_U8));
           preempted=0;

       	   break;

       default:
           break;
   }
	return eError;
}
/* ================================================================================= */
/**
* @fn FillBufferDone() description for FillBufferDone
FillBufferDone().
Called by the component when an output buffer has been filled
*
*  @see         OMX_Core.h
*/
/* ================================================================================ */

void FillBufferDone (OMX_HANDLETYPE hComponent, OMX_PTR ptr, OMX_BUFFERHEADERTYPE* pBuffer)
{
    APP_DPRINT ("APP:::: OUTPUT BUFFER = %p && %p\n",pBuffer, pBuffer->pBuffer);
    APP_DPRINT ("APP:::: pBuffer->nFilledLen = %ld\n",pBuffer->nFilledLen);
/*    OutputFrames = pBuffer->pOutputPortPrivate;
    printf("Receiving output %d Frames\n",OutputFrames->nFrames);*/

    write(OpBuf_Pipe[1], &pBuffer, sizeof(pBuffer));
	#ifdef OMX_GETTIME
      if (GT_FlagF == 1 ) /* First Buffer Reply*/  /* 1 = First Buffer,  0 = Not First Buffer  */
      {
        GT_END("Call to FillBufferDone  <First: FillBufferDone>");
        GT_FlagF = 0 ;   /* 1 = First Buffer,  0 = Not First Buffer  */
      }
    #endif
}

/* ================================================================================= */
/**
* @fn EmptyBufferDone() description for EmptyBufferDone
EmptyBufferDone().
Called by the component when an input buffer has been emptied
*
*  @see         OMX_Core.h
*/
/* ================================================================================ */

void EmptyBufferDone(OMX_HANDLETYPE hComponent, OMX_PTR ptr, OMX_BUFFERHEADERTYPE* pBuffer)
{
	OMX_S16 ret;
    APP_DPRINT ("APP:::: INPUT BUFFER = %p && %p\n",pBuffer, pBuffer->pBuffer);

	if (!preempted) 
		ret = write(IpBuf_Pipe[1], &pBuffer, sizeof(pBuffer));
#ifdef OMX_GETTIME
    if (GT_FlagE == 1 ) /* First Buffer Reply*/  /* 1 = First Buffer,  0 = Not First Buffer  */
    {
      GT_END("Call to EmptyBufferDone <First: EmptyBufferDone>");
	  GT_FlagE = 0;   /* 1 = First Buffer,  0 = Not First Buffer  */
    }
#endif
}


FILE *inputToSN;

/* ================================================================================= */
/**
* @fn main() description for main
main().
Test app main function
*
*  @see         OMX_Core.h
*/
/* ================================================================================ */

int main(int argc, char* argv[])
      {


    OMX_CALLBACKTYPE AmrCaBa = {(void *)EventHandler,
				(void*)EmptyBufferDone,
                                (void*)FillBufferDone};
    OMX_HANDLETYPE pHandle;
    OMX_ERRORTYPE error = OMX_ErrorNone;
    OMX_U32 AppData = 100;
    OMX_PARAM_PORTDEFINITIONTYPE* pCompPrivateStruct;
    OMX_AUDIO_PARAM_AMRTYPE *pAmrParam;
    OMX_COMPONENTTYPE *pComponent = (OMX_COMPONENTTYPE *)pHandle;
    OMX_STATETYPE state;
	/* TODO: Set a max number of buffers */
    OMX_BUFFERHEADERTYPE* pInputBufferHeader[10];
	/* TODO: Set a max number of buffers */
    OMX_BUFFERHEADERTYPE* pOutputBufferHeader[10];
    OMX_BUFFERHEADERTYPE* pBuf;
    bInvalidState=OMX_FALSE;
    /* TI_OMX_DATAPATH dataPath;*/
    FILE* fOut = NULL;
    FILE* fIn = fopen(argv[1], "r");

   
#ifdef USE_BUFFER
    OMX_U8* pInputBuffer[10];
    OMX_U8* pOutputBuffer[10];
#endif



	int numInputBuffers = 0;
	int numOutputBuffers = 0;
    struct timeval tv;
    int retval, i, j,k;
    int frmCount = 0;
	int testcnt = 1;
	int testcnt1 = 1;
    OMX_INDEXTYPE index;
    OMX_U32	streamId;
    int count=0;
#ifdef DSP_RENDERING_ON		
    int wbamrdecfdwrite;
    int wbamrdecfdread;
#endif

    /*TI_OMX_DSP_DEFINITION* audioinfo;*/
    OMX_AUDIO_CONFIG_MUTETYPE* pCompPrivateStructMute = NULL; 
    OMX_AUDIO_CONFIG_VOLUMETYPE* pCompPrivateStructVolume = NULL; 

	/*TI_OMX_STREAM_INFO *streaminfo;
    streaminfo = newmalloc(sizeof(TI_OMX_STREAM_INFO));
	audioinfo = newmalloc(sizeof(TI_OMX_DSP_DEFINITION));
	
    APP_MEMPRINT("%d :: [TESTAPPALLOC] audioinfo = %p\n",__LINE__,audioinfo);
    if(NULL == audioinfo) {
        APP_DPRINT("%d :: Malloc Failed\n",__LINE__);
        error = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
	
    ArrayOfPointers[0]=(TI_OMX_STREAM_INFO*)streaminfo;
    ArrayOfPointers[1]=(TI_OMX_DSP_DEFINITION*)audioinfo;*/

    pthread_mutex_init(&WaitForState_mutex, NULL);
    pthread_cond_init (&WaitForState_threshold, NULL);
    WaitForState_flag = 0;
	/**flush*/
	pthread_mutex_init(&WaitForOUTFlush_mutex, NULL);
    pthread_cond_init (&WaitForOUTFlush_threshold, NULL);
    pthread_mutex_init(&WaitForINFlush_mutex, NULL);
    pthread_cond_init (&WaitForINFlush_threshold, NULL);
	/*****/
    
	APP_DPRINT("------------------------------------------------------\n");
    APP_DPRINT("This is Main Thread In WBAMR DECODER Test Application:\n");
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
    if((argc < 11) || (argc > 12))
    {
        printf( "Usage:  test infile [outfile]\n");
        goto EXIT;
    }

	numInputBuffers = atoi(argv[9]);
	numOutputBuffers = atoi(argv[10]);

	if ( (numInputBuffers < 1) || (numInputBuffers >4)){
        APP_DPRINT ("%d :: App: ERROR: No. of input buffers not valid (0-4) \n",__LINE__);
		goto EXIT;
	}

	if ( (numOutputBuffers< 1) || (numOutputBuffers>4)){
        APP_DPRINT ("%d :: App: ERROR: No. of output buffers not valid (0-4) \n",__LINE__);
		goto EXIT;
	}

	/* check to see that the input file exists */
    struct stat sb = {0};
    OMX_S16 status = stat(argv[1], &sb);
    if( status != 0 ) {
        APP_DPRINT( "Cannot find file %s. (%u)\n", argv[1], errno);
        goto EXIT;
    }

    /* Open the file of data to be rendered. */
    if( fIn == NULL ) {
        APP_DPRINT( "Error:  failed to open the file %s for readonly\access\n", argv[1]);
        goto EXIT;
    }
    else{
         printf("------------\n");
         }
    
	APP_DPRINT("file is %x\n",fIn);

    /*FILE* fOut = NULL;*/
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
		    APP_DPRINT( "Error:Empty Event Pipe failed to open\n");
		    goto EXIT;
	}

    /* save off the "max" of the handles for the selct statement */
    OMX_S16 fdmax = maxint(IpBuf_Pipe[0], OpBuf_Pipe[0]);
	fdmax = maxint(fdmax,Event_Pipe[0]);

    APP_DPRINT("%d :: AmrTest\n",__LINE__);
    error = TIOMX_Init();
    APP_DPRINT("%d :: AmrTest\n",__LINE__);
    if(error != OMX_ErrorNone) {
        APP_DPRINT("%d :: Error returned by OMX_Init()\n",__LINE__);
        goto EXIT;
    }
    int command;
    command = atoi(argv[6]);
    APP_DPRINT("%d :: AmrTest\n",__LINE__);
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
            if (argc == 12)
            {
                testcnt = atoi(argv[11]);
            }
            else
            {
                testcnt = 20;  /*20 cycles by default*/
            }
            break;
        case 6:
            printf ("------------------------------------------------\n");
            printf ("Testing Repeated PLAY with Deleting Component\n");
            printf ("------------------------------------------------\n");
            if (argc == 12)
            {
                testcnt1 = atoi(argv[11]);
            }
            else
            {
                testcnt1 = 20;  /*20 cycles by default*/
            }
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
  	  	  	printf ("Testing Frame Lost Suport\n");
  	  	  	printf ("------------------------------------------------------------\n");
  	  	  	break; 
		case 13:
		   printf ("------------------------------------------------------------\n");
		   printf ("Testing Ringtone test\n");
		   printf ("------------------------------------------------------------\n");
		   testcnt = 10;
		   break;			
    }

    APP_DPRINT("%d :: AmrTest\n",__LINE__);
		dasfmode = atoi(argv[7]);

    APP_DPRINT("%d :: AmrTest\n",__LINE__);
	for(j = 0; j < testcnt1; j++) {

    #ifdef DSP_RENDERING_ON

        if((wbamrdecfdwrite=open(FIFO1,O_WRONLY))<0) {
            printf("[AMRTEST] - failure to open WRITE pipe\n");
        }
        else {
            printf("[AMRTEST] - opened WRITE pipe\n");
        }

        if((wbamrdecfdread=open(FIFO2,O_RDONLY))<0) {
            printf("[AMRTEST] - failure to open READ pipe\n");
            goto EXIT;
        }
        else {
            printf("[AMRTEST] - opened READ pipe\n");
        }  

	#endif

        if(j >= 1) {
            printf ("Decoding the file for %d Time\n",j+1);
			close(IpBuf_Pipe[0]);
			close(IpBuf_Pipe[1]);
			close(OpBuf_Pipe[0]);
			close(OpBuf_Pipe[1]);
	APP_DPRINT("file is %x\n",fIn);
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
            fIn = fopen(argv[1], "r");
            if( fIn == NULL ) {
                fprintf(stderr, "Error:  failed to open the file %s for readonly\
                                                                   access\n", argv[1]);
                goto EXIT;
            }

            fOut = fopen(argv[2], "a");
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

    /* Load the WBAMR Encoder Component */
    APP_DPRINT("%d :: AmrTest\n",__LINE__);

#ifdef OMX_GETTIME
	GT_START();
    error = OMX_GetHandle(&pHandle, strAmrEncoder, &AppData, &AmrCaBa);
	GT_END("Call to GetHandle");
#else 
	error = TIOMX_GetHandle(&pHandle, strAmrEncoder, &AppData, &AmrCaBa);
#endif
    APP_DPRINT("%d :: AmrTest\n",__LINE__);
    if((error != OMX_ErrorNone) || (pHandle == NULL)) {
        APP_DPRINT ("Error in Get Handle function\n");
        goto EXIT;
    }

    APP_DPRINT("%d :: AmrTest\n",__LINE__);
    pCompPrivateStruct = newmalloc (sizeof (OMX_PARAM_PORTDEFINITIONTYPE));
	if (pCompPrivateStruct == 0) {
		printf("Malloc failed\n");
		error = -1;
		goto EXIT;
	}
    ArrayOfPointers[2]=(OMX_PARAM_PORTDEFINITIONTYPE*)pCompPrivateStruct;

    pCompPrivateStructMute = newmalloc (sizeof(OMX_AUDIO_CONFIG_MUTETYPE)); 
    if(pCompPrivateStructMute == NULL) { 
         printf("%d :: App: Malloc Failed\n",__LINE__); 
         goto EXIT; 
     } 
     ArrayOfPointers[3] = (OMX_AUDIO_CONFIG_MUTETYPE*)pCompPrivateStructMute;
   
     pCompPrivateStructVolume = newmalloc (sizeof(OMX_AUDIO_CONFIG_VOLUMETYPE)); 
     if(pCompPrivateStructVolume == NULL) { 
         printf("%d :: App: Malloc Failed\n",__LINE__); 
         goto EXIT; 
     } 
     ArrayOfPointers[4] = (OMX_AUDIO_CONFIG_VOLUMETYPE*)pCompPrivateStructVolume;

    APP_MEMPRINT("%d:::[TESTAPPALLOC] %p\n",__LINE__,pCompPrivateStruct);

    APP_DPRINT("%d :: AmrTest\n",__LINE__);
    pCompPrivateStruct->nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
    pCompPrivateStruct->nVersion.s.nVersionMajor = 0xF1;
    pCompPrivateStruct->nVersion.s.nVersionMinor = 0xF2;
    pCompPrivateStruct->nPortIndex = OMX_DirInput;
    error = OMX_GetParameter (pHandle,OMX_IndexParamPortDefinition,pCompPrivateStruct);    

    /* Send input port config */

    if(!(strcmp(argv[3],"MIME"))) 
    {
        WBAmrFrameFormat = OMX_AUDIO_AMRFrameFormatFSF;
	}
    else if(!(strcmp(argv[3],"NONMIME"))) 
    {
	    WBAmrFrameFormat = OMX_AUDIO_AMRFrameFormatConformance;
	}
    else if(!(strcmp(argv[3],"IF2"))) 
    {
		WBAmrFrameFormat = OMX_AUDIO_AMRFrameFormatIF2;
	}
	else 
    {
		printf("Enter proper Frame Format Type MIME, NON MIME or IF2\nExit\n");
		goto EXIT;
	}


    pCompPrivateStruct->eDir = OMX_DirInput;
    pCompPrivateStruct->nBufferCountActual = numInputBuffers;
    pCompPrivateStruct->nBufferCountMin                    = numInputBuffers;
    pCompPrivateStruct->nBufferSize = INPUT_WBAMRDEC_BUFFER_SIZE;
    pCompPrivateStruct->eDomain                            = OMX_PortDomainAudio;
    pCompPrivateStruct->format.audio.eEncoding = OMX_AUDIO_CodingAMR;
#ifdef OMX_GETTIME
	GT_START();
    error = OMX_SetParameter (pHandle,OMX_IndexParamPortDefinition,pCompPrivateStruct);
	GT_END("Set Parameter Test-SetParameter");
#else
    error = OMX_SetParameter (pHandle,OMX_IndexParamPortDefinition,pCompPrivateStruct);
#endif
    if (error != OMX_ErrorNone) {
        error = OMX_ErrorBadParameter;
        printf ("%d:: OMX_ErrorBadParameter\n",__LINE__);
        goto EXIT;
    }

    pCompPrivateStruct->nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
    pCompPrivateStruct->nVersion.s.nVersionMajor = 0xF1; 
    pCompPrivateStruct->nVersion.s.nVersionMinor = 0xF2; 
    pCompPrivateStruct->nPortIndex = OMX_DirOutput; 
    error = OMX_GetParameter (pHandle,OMX_IndexParamPortDefinition,pCompPrivateStruct);

    /* Send output port config */
    pCompPrivateStruct->eDir = OMX_DirOutput;
    pCompPrivateStruct->nBufferCountActual = numOutputBuffers;
    pCompPrivateStruct->nBufferCountMin                    = numOutputBuffers;
    pCompPrivateStruct->nBufferSize = OUTPUT_WBAMRDEC_BUFFER_SIZE;
    pCompPrivateStruct->eDomain                            = OMX_PortDomainAudio;

    if(dasfmode ) 
    {
           pCompPrivateStruct->nBufferCountActual = 0;
	}
#ifdef OMX_GETTIME
	GT_START();
    error = OMX_SetParameter (pHandle,OMX_IndexParamPortDefinition,pCompPrivateStruct);
	GT_END("Set Parameter Test-SetParameter");
#else
    error = OMX_SetParameter (pHandle,OMX_IndexParamPortDefinition,pCompPrivateStruct);
#endif
    if (error != OMX_ErrorNone) {
        error = OMX_ErrorBadParameter;
        printf ("%d:: OMX_ErrorBadParameter\n",__LINE__);
        goto EXIT;
    }

	pAmrParam = newmalloc (sizeof (OMX_AUDIO_PARAM_AMRTYPE));
    APP_MEMPRINT("%d:::[TESTAPPALLOC] %p\n",__LINE__,pAmrParam);
    pAmrParam->nSize = sizeof (OMX_AUDIO_PARAM_AMRTYPE);
    pAmrParam->nVersion.s.nVersionMajor = 0xF1;
    pAmrParam->nVersion.s.nVersionMinor = 0xF2;
    pAmrParam->nPortIndex = OMX_DirInput;
    pAmrParam->nChannels = 1;
    pAmrParam->nBitRate = 8000;
	pAmrParam->eAMRBandMode = atoi(argv[5]);
    APP_DPRINT("\n%d :: App: pAmrParam->eAMRBandMode --> %d \n",__LINE__,
    												pAmrParam->eAMRBandMode);

      ArrayOfPointers[5] = (OMX_AUDIO_PARAM_AMRTYPE *)pAmrParam;

#ifndef USE_BUFFER

	for (i=0; i < numInputBuffers; i++) {
		/* allocate input buffer */
		APP_DPRINT("%d :: About to call OMX_AllocateBuffer\n",__LINE__);
		error = OMX_AllocateBuffer(pHandle,&pInputBufferHeader[i],0,NULL,INPUT_WBAMRDEC_BUFFER_SIZE*3);
		APP_DPRINT("%d :: called OMX_AllocateBuffer\n",__LINE__);
		if(error != OMX_ErrorNone) {
	        APP_DPRINT("%d :: Error returned by OMX_AllocateBuffer()\n",__LINE__);
			goto EXIT;
		}

	}
        for (i=0; i < numOutputBuffers; i++) {
	          /* allocate output buffer */
	          APP_DPRINT("%d :: About to call OMX_AllocateBuffer\n",__LINE__);
	          printf("About to call OMX_AllocateBuffer\n");
	          if ((testCaseNo != 7)&&(testCaseNo != 9))
    	          error = OMX_AllocateBuffer(pHandle,&pOutputBufferHeader[i],1,NULL,OUTPUT_WBAMRDEC_BUFFER_SIZE*1);
              else     	          
                  error = OMX_AllocateBuffer(pHandle,&pOutputBufferHeader[i],1,NULL,OUTPUT_WBAMRDEC_BUFFER_SIZE*2);
	          APP_DPRINT("%d :: called OMX_AllocateBuffer\n",__LINE__);
	          if(error != OMX_ErrorNone) {
			      APP_DPRINT("%d :: Error returned by OMX_AllocateBuffer()\n",__LINE__);
			      goto EXIT;
		      }
	    }

#else

	printf("%d::USE BUFFER\n",__LINE__);
    APP_DPRINT("%d :: About to call OMX_UseBuffer\n",__LINE__);

	for (i=0; i < numInputBuffers; i++) {
        pInputBuffer[i] = (OMX_U8*)newmalloc(INPUT_WBAMRDEC_BUFFER_SIZE*3 + 256);
        APP_MEMPRINT("%d:::[TESTAPPALLOC] %p\n",__LINE__,pInputBuffer[i]);
        pInputBuffer[i] = pInputBuffer[i] + 128;
        /* allocate input buffer */
        APP_DPRINT("%d :: About to call OMX_UseBuffer\n",__LINE__);
        error = OMX_UseBuffer(pHandle,&pInputBufferHeader[i],0,NULL,INPUT_WBAMRDEC_BUFFER_SIZE*3,pInputBuffer[i]);
        APP_DPRINT("%d :: called OMX_UseBuffer\n",__LINE__);
        if(error != OMX_ErrorNone) {
            APP_DPRINT("%d :: Error returned by OMX_UseBuffer()\n",__LINE__);
            goto EXIT;
            }
    }

	for (i=0; i < numOutputBuffers; i++) {
        if ((testCaseNo != 7)&&(testCaseNo != 9)){
            pOutputBuffer[i]= newmalloc (OUTPUT_WBAMRDEC_BUFFER_SIZE*1 + 256);
            APP_MEMPRINT("%d:::[TESTAPPALLOC] %p\n",__LINE__,pOutputBuffer[i]);
            pOutputBuffer[i] = pOutputBuffer[i] + 128;
            error = OMX_UseBuffer(pHandle,&pOutputBufferHeader[i],1,NULL,OUTPUT_WBAMRDEC_BUFFER_SIZE*1,pOutputBuffer[i]);
            APP_DPRINT("%d :: called OMX_UseBuffer\n",__LINE__);
            if(error != OMX_ErrorNone) {
                APP_DPRINT("%d :: Error returned by OMX_UseBuffer()\n",__LINE__);
                goto EXIT;
            }
        }
        else{             
            pOutputBuffer[i]= newmalloc (OUTPUT_WBAMRDEC_BUFFER_SIZE*2 + 256);
            APP_MEMPRINT("%d:::[TESTAPPALLOC] %p\n",__LINE__,pOutputBuffer[i]);
            pOutputBuffer[i] = pOutputBuffer[i] + 128;
            error = OMX_UseBuffer(pHandle,&pOutputBufferHeader[i],1,NULL,OUTPUT_WBAMRDEC_BUFFER_SIZE*2,pOutputBuffer[i]);
            APP_DPRINT("%d :: called OMX_UseBuffer\n",__LINE__);
            if(error != OMX_ErrorNone) {
                APP_DPRINT("%d :: Error returned by OMX_UseBuffer()\n",__LINE__);
                goto EXIT;
            }
        
        }
	}
#endif


    if(!(strcmp(argv[4],"DTXON"))) {
    	/**< AMR Discontinuous Transmission Mode is enabled  */
    	pAmrParam->eAMRDTXMode = OMX_AUDIO_AMRDTXModeOnVAD1;
    	APP_DPRINT("\n%d :: App: pAmrParam->eAMRDTXMode --> %s \n",__LINE__,
    												argv[4]);
	}
	if(!(strcmp(argv[4],"DTXOFF"))) {
		/**< AMR Discontinuous Transmission Mode is disabled */
		pAmrParam->eAMRDTXMode = OMX_AUDIO_AMRDTXModeOff;
    	APP_DPRINT("\n%d :: App: pAmrParam->eAMRDTXMode --> %s \n",__LINE__,
    												argv[4]);
	}

    if(!(strcmp(argv[4],"WBAMR_EFR"))) {
    	/**< DTX as WBAMR_EFR instead of AMR standard  */
    	pAmrParam->eAMRDTXMode = OMX_AUDIO_AMRDTXasEFR;
    	APP_DPRINT("\n%d :: App: pAmrParam->eAMRDTXMode --> %s \n",__LINE__,
    												argv[4]);
	}

    pAmrParam->eAMRFrameFormat = WBAmrFrameFormat;
    
	/*if(gMimeFlag == 1){
			pAmrParam->eAMRFrameFormat = OMX_AUDIO_AMRFrameFormatFSF;	
	}
	else if(gMimeFlag == 0){
			 pAmrParam->eAMRFrameFormat = OMX_AUDIO_AMRFrameFormatConformance;
	}
	else {
			printf("Enter proper MIME mode\n");
			printf("MIME:0\n");
			printf("NON MIME:1\n");
			goto EXIT;
	}*/
    
    
#ifdef OMX_GETTIME
	GT_START();
    error = OMX_SetParameter (pHandle,OMX_IndexParamAudioAmr,pAmrParam);
	GT_END("Set Parameter Test-SetParameter");
#else
    error = OMX_SetParameter (pHandle,OMX_IndexParamAudioAmr,pAmrParam);
#endif
	if (error != OMX_ErrorNone) {
        error = OMX_ErrorBadParameter;
        printf ("%d:: OMX_ErrorBadParameter\n",__LINE__);
        goto EXIT;
    }
    pAmrParam->nPortIndex = OMX_DirOutput;
    pAmrParam->nChannels = 1;
    pAmrParam->nBitRate = 8000;
#ifdef OMX_GETTIME
	GT_START();
    error = OMX_SetParameter (pHandle,OMX_IndexParamAudioAmr,
                                           pAmrParam);
	GT_END("Set Parameter Test-SetParameter");
#else
    error = OMX_SetParameter (pHandle,OMX_IndexParamAudioAmr,
                                           pAmrParam);
#endif
    if (error != OMX_ErrorNone) {
        error = OMX_ErrorBadParameter;
        printf ("%d:: OMX_ErrorBadParameter\n",__LINE__);
        goto EXIT;
    }

	/* get TeeDN or ACDN mode */
	/*audioinfo->acousticMode = atoi(argv[8]);*/
	/*
    if (audioinfo->acousticMode == OMX_TRUE) {
        printf("Using Acoustic Device Node Path\n");
	dataPath = DATAPATH_ACDN;
    }*/
    else if (dasfmode) {
#ifdef RTM_PATH    
        printf("Using Real Time Mixer Path\n");
        dataPath = DATAPATH_APPLICATION_RTMIXER;
#endif

#ifdef ETEEDN_PATH
        printf("Using Eteedn Path\n");
        dataPath = DATAPATH_APPLICATION;
#endif        
    }

    /*audioinfo->dasfMode = dasfmode;*/
    error = OMX_GetExtensionIndex(pHandle, "OMX.TI.index.config.wbamrheaderinfo",&index);
	if (error != OMX_ErrorNone) {
		printf("Error getting extension index\n");
		goto EXIT;
	}

#ifdef DSP_RENDERING_ON		
	cmd_data.hComponent = pHandle;
    cmd_data.AM_Cmd = AM_CommandIsOutputStreamAvailable;
    
    /* for encoder, using AM_CommandIsInputStreamAvailable */
    cmd_data.param1 = 0;

    if((write(wbamrdecfdwrite, &cmd_data, sizeof(cmd_data)))<0) {
        printf("%d ::OMX_WbAmrDecoder.c ::[WBAMR Dec Component] - send command to audio manager\n", __LINE__);
    }
    if((read(wbamrdecfdread, &cmd_data, sizeof(cmd_data)))<0) {
        printf("%d ::OMX_WbAmrDecoder.c ::[WBAMR Dec Component] - failure to get data from the audio manager\n", __LINE__);
		goto EXIT;
    }

    audioinfo->streamId = cmd_data.streamID;
    streamId = audioinfo->streamId;
#endif
	
	/*error = OMX_SetConfig (pHandle, index, audioinfo);
    if(error != OMX_ErrorNone) {
        error = OMX_ErrorBadParameter;
        APP_DPRINT("%d :: Error from OMX_SetConfig() function\n",__LINE__);
        goto EXIT;
    }
	*/
    /*error = OMX_GetExtensionIndex(pHandle, "OMX.TI.index.config.wbamr.datapath",&index);
	if (error != OMX_ErrorNone) {
		printf("Error getting extension index\n");
		goto EXIT;
	}*/
    

	/*error = OMX_SetConfig (pHandle, index, &dataPath);
    if(error != OMX_ErrorNone) {
        error = OMX_ErrorBadParameter;
        APP_DPRINT("%d :: AmrDecTest.c :: Error from OMX_SetConfig() function\n",__LINE__);
        goto EXIT;
    }*/


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
        APP_DPRINT( "Error:  WaitForState reports an error %X\n", error);
        goto EXIT;
    }
  if (dasfmode) {
    	/* get streamID back to application */
     	error = OMX_GetExtensionIndex(pHandle, "OMX.TI.index.config.wbamrstreamIDinfo",&index);
      	if (error != OMX_ErrorNone) {
       		printf("Error getting extension index\n");
         		goto EXIT;
           	}
	    printf("***************StreamId=%ld******************\n", streamId);
    }
    for(i = 0; i < testcnt; i++) {
        if(i > 0) {
            printf ("Decoding the file for %d Time\n",i+1);

			close(IpBuf_Pipe[0]);
			close(IpBuf_Pipe[1]);
			close(OpBuf_Pipe[0]);
			close(OpBuf_Pipe[1]);

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

            fIn = fopen(argv[1], "r");
            if(fIn == NULL) {
                fprintf(stderr, "Error:  failed to open the file %s for readonly access\n", argv[1]);
                goto EXIT;
            }
            fOut = fopen(argv[2], "a");
            if(fOut == NULL) {
                fprintf(stderr, "Error:  failed to create the output file \n");
                goto EXIT;
            }
        }
		
  
    if((command == 13)){
		if (i == 0){
			printf ("Basic Function:: Sending OMX_StateExecuting Command\n");
#ifdef OMX_GETTIME
			GT_START();
#endif
		    error = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
		    if(error != OMX_ErrorNone) {
		        APP_DPRINT ("Error from SendCommand-Executing State function\n");
		        goto EXIT;
		    }
		    pComponent = (OMX_COMPONENTTYPE *)pHandle;

		    error = WaitForState(pHandle, OMX_StateExecuting);
#ifdef OMX_GETTIME
			GT_END("Call to SendCommand <OMX_StateExecuting>");
#endif
		    if(error != OMX_ErrorNone) {
		        APP_DPRINT( "Error:  WaitForState reports an error %X\n", error);
		        goto EXIT;
		    }
		}
	}else{
		printf ("Basic Function(else):: Sending OMX_StateExecuting Command\n");
#ifdef OMX_GETTIME
		GT_START();
#endif
		error = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
		if(error != OMX_ErrorNone) {
			APP_DPRINT ("Error from SendCommand-Executing State function\n");
			goto EXIT;
		}
		pComponent = (OMX_COMPONENTTYPE *)pHandle;

		error = WaitForState(pHandle, OMX_StateExecuting);
#ifdef OMX_GETTIME
		GT_END("Call to SendCommand <OMX_StateExecuting>");
#endif
		if(error != OMX_ErrorNone) {
			APP_DPRINT( "Error:  WaitForState reports an error %X\n", error);
			goto EXIT;
		}
	}
	
	
	
	for (k=0; k < numInputBuffers; k++) {
	#ifdef OMX_GETTIME
        if (k==0)
        { 
			GT_FlagE=1;  /* 1 = First Buffer,  0 = Not First Buffer  */
            GT_START(); /* Empty Bufffer */
        }
	#endif
        error = send_input_buffer (pHandle, pInputBufferHeader[k], fIn);

	}

	if (dasfmode == 0) {
		for (k=0; k < numOutputBuffers; k++) {
		#ifdef OMX_GETTIME
			if (k==0)
			{ 
				GT_FlagF=1;  /* 1 = First Buffer,  0 = Not First Buffer  */
				GT_START(); /* Fill Buffer */
			}
		#endif
			pComponent->FillThisBuffer(pHandle,  pOutputBufferHeader[k]);
		}
	}

    pComponent->GetState(pHandle, &state);
    retval = 1;
#ifndef WAITFORRESOURCES
   while( (error == OMX_ErrorNone) && ((state != OMX_StateIdle) || (count < 2))  ) 
    {
	if(1)
    {
#else
   while(1) 
   {
        if((eError == OMX_ErrorNone) && (state != OMX_StateIdle) || (count < 2))  ) 

        { 
#endif

        FD_ZERO(&rfds);
        FD_SET(IpBuf_Pipe[0], &rfds);
        FD_SET(OpBuf_Pipe[0], &rfds);
		FD_SET(Event_Pipe[0], &rfds);
        tv.tv_sec = 1;
        tv.tv_usec = 0;
						
        retval = select(fdmax+1, &rfds, NULL, NULL, &tv);
        if(retval == -1) 
        {
            perror("select()");
            APP_DPRINT ( " : Error \n");
            break;
        }
		
    if (retval == 0) 
           count++;
    if (count ==3){
		if(command != 13){
			fprintf(stderr, "Shutting down Since there is nothing else to send nor read---------- \n");
			StopComponent(pHandle);
		}else{
			if(i != 9){
				printf("**RING_TONE: Lets play again!\n");
				printf("counter= %d \n",i);
				goto my_exit;

			}else{
				StopComponent(pHandle);
				printf("stopcomponent\n");
			}		
		}
    }

	if(FD_ISSET(IpBuf_Pipe[0], &rfds)){
		OMX_BUFFERHEADERTYPE* pBuffer;
		read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
		frmCount++;
		pBuffer->nFlags = 0; 
			
		if( ((2==command) || (4==command) ) && (50 == frmCount)){ /*Stop Tests*/
				fprintf(stderr, "Send Stop Command to component ---------- \n");
				StopComponent(pHandle);
		}
		
		/**flush*/
		/*if ((frmCount >= nNextFlushFrame) && (num_flush < 5000)){*/
		/*
		if (frmCount >= nNextFlushFrame){
			printf("Flush #%d\n", num_flush++);
            error = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StatePause, NULL);
            if(error != OMX_ErrorNone) {
				fprintf (stderr,"Error from SendCommand-Pasue State function\n");
                goto EXIT;
            }
            error = WaitForState(pHandle, OMX_StatePause);
            if(error != OMX_ErrorNone) {
				fprintf(stderr, "Error:  WBAmrDecoder->WaitForState reports an error %X\n", error);
				goto EXIT;
            }
            error = OMX_SendCommand(pHandle, OMX_CommandFlush, 0, NULL);
            pthread_mutex_lock(&WaitForINFlush_mutex); 
            pthread_cond_wait(&WaitForINFlush_threshold, 
							&WaitForINFlush_mutex);
            pthread_mutex_unlock(&WaitForINFlush_mutex);

            error = OMX_SendCommand(pHandle, OMX_CommandFlush, 1, NULL);
            pthread_mutex_lock(&WaitForOUTFlush_mutex); 
            pthread_cond_wait(&WaitForOUTFlush_threshold, 
							&WaitForOUTFlush_mutex);
            pthread_mutex_unlock(&WaitForOUTFlush_mutex);    

            error = OMX_SendCommand(pHandle, OMX_CommandStateSet,OMX_StateExecuting, NULL);
            if(error != OMX_ErrorNone) {
				fprintf (stderr,"Error from SendCommand-Executing State function\n");
				goto EXIT;
            }
            error = WaitForState(pHandle, OMX_StateExecuting);
            if(error != OMX_ErrorNone) {
                fprintf(stderr, "Error:  AmrDecoder->WaitForState reports an error %X\n", error);
                goto EXIT;
            }
                                    
            fseek(fIn, 0, SEEK_SET);
            frmCount = 0;
            nNextFlushFrame = 5 + 50 * ((rand() * 1.0) / RAND_MAX);
            printf("nNextFlushFrame d= %d\n", nNextFlushFrame);
        }*/
		if (state == OMX_StateExecuting){/**/
			error = send_input_buffer (pHandle, pBuffer, fIn); 
				if (error != OMX_ErrorNone) { 
					printf("goto EXIT %d\n",__LINE__);
					goto EXIT; 
				} 
	     }/**/
	
		if(3 == command){ /*Pause Test*/
			if(frmCount == 100) {   /*100 Frames processed */
				printf ("Sending Pause command to Codec \n");
				PauseComponent(pHandle);
				printf("5 secs sleep...\n");
				sleep(5);
				printf ("Sending Resume command to Codec \n");
				PlayComponent(pHandle);
        		}				
		}
		else if ( 10 == command ){	/*Mute and UnMuteTest*/
				if(frmCount == 50){ 
					printf("************Mute the playback stream*****************\n"); 
					pCompPrivateStructMute->bMute = OMX_TRUE; 
					error = OMX_SetConfig(pHandle, OMX_IndexConfigAudioMute, pCompPrivateStructMute); 
					if (error != OMX_ErrorNone) 
					{ 
						error = OMX_ErrorBadParameter; 
						goto EXIT; 
					} 
				} 
				else if(frmCount == 120) { 
						printf("************Unmute the playback stream*****************\n"); 
						pCompPrivateStructMute->bMute = OMX_FALSE; 
						error = OMX_SetConfig(pHandle, OMX_IndexConfigAudioMute, pCompPrivateStructMute); 
						if (error != OMX_ErrorNone) { 
							error = OMX_ErrorBadParameter; 
							goto EXIT; 
						} 
					}
			
			}
		else if ( 11 == command ) { /*Set Volume Test*/
					if(frmCount == 35) { 
						printf("************Set stream volume to high*****************\n"); 
						pCompPrivateStructVolume->sVolume.nValue = 0x8000; 
						error = OMX_SetConfig(pHandle, OMX_IndexConfigAudioVolume, pCompPrivateStructVolume); 
						if (error != OMX_ErrorNone) { 
							error = OMX_ErrorBadParameter; 
							goto EXIT; 
						} 
					}
					if(frmCount == 120) { 
						printf("************Set stream volume to low*****************\n"); 
						pCompPrivateStructVolume->sVolume.nValue = 0x1000; 
						error = OMX_SetConfig(pHandle, OMX_IndexConfigAudioVolume, pCompPrivateStructVolume); 
						if (error != OMX_ErrorNone) { 
							error = OMX_ErrorBadParameter; 
							goto EXIT; 
						} 
					} 			
			}
       else if (command == 12) { /* frame lost test */
  	  	  	          if(frmCount == 35 || frmCount == 120) {
  	  	  	             printf("************Sending lost frame*****************\n");
  	  	  	             error = OMX_GetExtensionIndex(pHandle, "OMX.TI.index.config.wbamr.framelost",&index);	                       
  	  	                 error = OMX_SetConfig(pHandle, index, NULL);
  	  	  	    	     if (error != OMX_ErrorNone) {
  	  	  		             error = OMX_ErrorBadParameter;
  	  	  		             goto EXIT;
  	  	  		         }
  	  	  	           }
  	  	  	}
		}
	
        if( FD_ISSET(OpBuf_Pipe[0], &rfds) ) {            
            read(OpBuf_Pipe[0], &pBuf, sizeof(pBuf));
            if(pBuf->nFlags == OMX_BUFFERFLAG_EOS){
                    /*printf("EOS received by App, Stopping the component\n");*/
                    pBuf->nFlags = 0;
                   /* StopComponent(pHandle);*/
            }
			/**/fwrite(pBuf->pBuffer, 1, pBuf->nFilledLen, fOut);
			fflush(fOut);
			if (state == OMX_StateExecuting ) {
				pComponent->FillThisBuffer(pHandle, pBuf);
			}
        }

		if( FD_ISSET(Event_Pipe[0], &rfds) ) {
                OMX_U8 pipeContents;
                read(Event_Pipe[0], &pipeContents, sizeof(OMX_U8));
	            if (pipeContents == 0) {
                    printf("Test app received OMX_ErrorResourcesPreempted\n");
                    WaitForState(pHandle,OMX_StateIdle);

					for(i=0; i < numInputBuffers; i++) {
						APP_DPRINT("%d :: App: About to newfree pInputBufferHeader[%d]\n",__LINE__, i);
						error = OMX_FreeBuffer(pHandle, OMX_DirInput, pInputBufferHeader[i]);
						if((error != OMX_ErrorNone)) {
							APP_DPRINT("%d:: Error in FreeBuffer function\n",__LINE__);
							goto EXIT;
						}
						
					}

					for(i=0; i < numOutputBuffers; i++) {
						APP_DPRINT("%d :: App: About to newfree pOutputBufferHeader[%d]\n",__LINE__, i);
						error = OMX_FreeBuffer(pHandle, OMX_DirOutput, pOutputBufferHeader[i]);
						if((error != OMX_ErrorNone)) {
							APP_DPRINT("%d :: Error in Free Buffer function\n",__LINE__);
							goto EXIT;
						}
						
				    }
#ifdef USE_BUFFER

		for(i=0; i < numOutputBuffers; i++) {
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
                    printf("Test app received OMX_ErrorResourcesAcquired\n");

                    OMX_SendCommand(pHandle,OMX_CommandStateSet,OMX_StateIdle,NULL);
			 		for (i=0; i < numOutputBuffers; i++) {
				          /* allocate output buffer */
				          APP_DPRINT("%d :: About to call OMX_AllocateBuffer\n",__LINE__);
			                  error = OMX_AllocateBuffer(pHandle,&pOutputBufferHeader[i],1,NULL,OUTPUT_WBAMRDEC_BUFFER_SIZE*2);
				          APP_DPRINT("%d :: called OMX_AllocateBuffer\n",__LINE__);
				          if(error != OMX_ErrorNone) {
						      APP_DPRINT("%d :: Error returned by OMX_AllocateBuffer()\n",__LINE__);
						      goto EXIT;
					      }
				    }

                    WaitForState(pHandle,OMX_StateIdle);

                    OMX_SendCommand(pHandle,OMX_CommandStateSet,OMX_StateExecuting,NULL);
                    WaitForState(pHandle,OMX_StateExecuting);

                    rewind(fIn);

                    send_input_buffer (pHandle, (OMX_BUFFERHEADERTYPE*)pOutputBufferHeader, fIn);
                }                

                if (pipeContents == 2) {
                           
				if(command !=13){
					APP_DPRINT("%d :: AmrDecTest.c :: StopComponent\n",__LINE__);
                     StopComponent(pHandle);
#ifdef WAITFORRESOURCES
					for(i=0; i < numInputBuffers; i++) {
						APP_DPRINT("%d :: App: About to newfree pInputBufferHeader[%d]\n",__LINE__, i);
						error = OMX_FreeBuffer(pHandle, OMX_DirInput, pInputBufferHeader[i]);
						if((error != OMX_ErrorNone)) {
							APP_DPRINT("%d:: Error in FreeBuffer function\n",__LINE__);
							goto EXIT;
						}
						
					}

					for(i=0; i < numOutputBuffers; i++) {
						APP_DPRINT("%d :: App: About to newfree pOutputBufferHeader[%d]\n",__LINE__, i);
						error = OMX_FreeBuffer(pHandle, OMX_DirOutput, pOutputBufferHeader[i]);
						if((error != OMX_ErrorNone)) {
							APP_DPRINT("%d :: Error in Free Buffer function\n",__LINE__);
							goto EXIT;
						}
						
				    }
                                
                	OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateLoaded, NULL);
                    WaitForState(pHandle, OMX_StateLoaded); 

                    goto SHUTDOWN;
#endif
				}else{
				printf("**RING_TONE: !!!!\n");
					if(i != 9){
						printf("**RING_TONE: Lets play again!\n");
						printf("counter= %d \n",i);
		                goto my_exit;

		            }else{
						StopComponent(pHandle);
						printf("stopcomponent\n");
		            }
				}
              }
            }
        error = pComponent->GetState(pHandle, &state);
        if(error != OMX_ErrorNone) {
              APP_DPRINT("%d:: Warning:  hAmrEncoder->GetState has returned status %X\n",
              																  __LINE__, error);
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
/*-----------------------------------------*/
    printf ("The current state of the component = %d \n",state);
my_exit:
    fclose(fOut);
    fclose(fIn);
    FirstTime = 1;
    count=0;

    } /*Inner for loop ends here */
	/* newfree buffers */
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

	/* newfree the App Allocated Buffers */
	printf("%d :: App: Freeing the App Allocated Buffers in TestApp\n",__LINE__);
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
	printf ("Sending the StateLoaded Command\n");
	
#ifdef OMX_GETTIME
	GT_START();
#endif
    error = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateLoaded, NULL);
    if(error != OMX_ErrorNone) {
        APP_DPRINT ("%d:: Error from SendCommand-Idle State function\n",__LINE__);
        goto EXIT;
    }
    error = WaitForState(pHandle, OMX_StateLoaded);
#ifdef OMX_GETTIME
	GT_END("Call to SendCommand <OMX_StateLoaded>");
#endif
    if(error != OMX_ErrorNone) {
        APP_DPRINT( "Error:  WaitForState reports an error %X\n", error);
        goto EXIT;
    }
    	
	error = OMX_SendCommand(pHandle, OMX_CommandPortDisable, -1, NULL);



#ifdef WAITFORRESOURCES
    error = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateWaitForResources, NULL);
    if(error != OMX_ErrorNone) {
        APP_DPRINT ("%d Error from SendCommand-Idle State function\n",__LINE__);
        goto EXIT;
    }
    error = WaitForState(pHandle, OMX_StateWaitForResources);

    /* temporarily put this here until I figure out what should really happen here */
    sleep(10);
    /* temporarily put this here until I figure out what should really happen here */
#endif    
SHUTDOWN:
#ifdef DSP_RENDERING_ON
	cmd_data.hComponent = pHandle;
    cmd_data.AM_Cmd = AM_Exit;

    if((write(wbamrdecfdwrite, &cmd_data, sizeof(cmd_data)))<0)
        printf("%d ::OMX_AmrDecoder.c :: [NBAMR Dec Component] - send command to audio manager\n",__LINE__);
	close(wbamrdecfdwrite);
    close(wbamrdecfdread);
#endif
    printf ("Free the Component handle\n");

    /* Unload the WBAMR Encoder Component */

    APP_MEMPRINT("%d:::[TESTAPPFREE] %p\n",__LINE__,pAmrParam);
    newfree(pAmrParam);
    
    APP_MEMPRINT("%d:::[TESTAPPFREE] %p\n",__LINE__,pCompPrivateStruct);
    newfree(pCompPrivateStruct);
    APP_MEMPRINT("%d:::[TESTAPPFREE] %p\n",__LINE__,pCompPrivateStructMute);
    newfree(pCompPrivateStructMute);
    APP_MEMPRINT("%d:::[TESTAPPFREE] %p\n",__LINE__,pCompPrivateStructVolume);
    newfree(pCompPrivateStructVolume);
    
    error = TIOMX_FreeHandle(pHandle);

		if( (error != OMX_ErrorNone)) {
        APP_DPRINT ("%d:: Error in Free Handle function\n",__LINE__);
        goto EXIT;
    }

	close(IpBuf_Pipe[0]);
	close(IpBuf_Pipe[1]);
	close(OpBuf_Pipe[0]);
	close(OpBuf_Pipe[1]);

	fclose(inputToSN);
    APP_DPRINT ("%d:: Free Handle returned Successfully \n\n\n\n",__LINE__);

	} /*Outer for loop ends here */
	
	pthread_mutex_destroy(&WaitForState_mutex);
    pthread_cond_destroy(&WaitForState_threshold);
	
	/**flush	*/
	pthread_mutex_destroy(&WaitForOUTFlush_mutex);
	pthread_cond_destroy(&WaitForOUTFlush_threshold);  
	pthread_mutex_destroy(&WaitForINFlush_mutex);
	pthread_cond_destroy(&WaitForINFlush_threshold);  	
	/***/

  	error = TIOMX_Deinit();
    if( (error != OMX_ErrorNone)) 
    {
			APP_DPRINT("APP: Error in Deinit Core function\n");
			goto EXIT;
    }
   
    /*newfree(audioinfo);
    
    newfree(streaminfo);*/


	
EXIT:
	if(bInvalidState==OMX_TRUE)
	{
#ifndef USE_BUFFER
		error = FreeAllResources(pHandle,
								pInputBufferHeader[0],
								pOutputBufferHeader[0],
								numInputBuffers,
								numOutputBuffers,
								fIn,
                                fOut);
#else
		error = FreeAllResources(pHandle,
									pInputBuffer,
									pOutputBuffer,
									numInputBuffers,
									numOutputBuffers,
									fIn,
                                    fOut);
#endif
   }
#ifdef APP_DEBUGMEM	
    printf("\n-Printing memory not deleted-\n");
    for(i=0;i<500;i++){
        if (lines[i]!=0){
             printf(" --->%d Bytes allocated on %p File:%s Line: %d\n",bytes[i], arr[i],file[i],lines[i]); 
             }
    }
#endif

#ifdef OMX_GETTIME
	GT_END("WBAMR_DEC test <End>");
	OMX_ListDestroy(pListHead);	
#endif    	
    return error;

 }

/* ================================================================================= */
/**
* @fn send_input_buffer() description for send_input_buffer
send_input_buffer().
Sends input buffer to component
*
*/
/* ================================================================================ */
OMX_ERRORTYPE send_input_buffer(OMX_HANDLETYPE pHandle, OMX_BUFFERHEADERTYPE* pBuffer, FILE *fIn)
{
        OMX_ERRORTYPE error = OMX_ErrorNone;
        OMX_COMPONENTTYPE *pComponent = (OMX_COMPONENTTYPE *)pHandle;
        APP_DPRINT ("*************************\n");
        APP_DPRINT ("%d :: pBuffer = %p nRead = %d\n",__LINE__,pBuffer,nRead);
        APP_DPRINT ("*************************\n");        
    
    if(FirstTime)
    {
        nRead = fill_data (pBuffer->pBuffer, WBAmrFrameFormat, fIn);
        pBuffer->nFilledLen = nRead;
    }
        else
        {
            memcpy(pBuffer->pBuffer, NextBuffer,nRead);
            pBuffer->nFilledLen = nRead;
        }
        
         nRead = fill_data (NextBuffer, WBAmrFrameFormat, fIn);

         if (WBAmrFrameFormat == OMX_AUDIO_AMRFrameFormatConformance)
         {
                if( nRead<numRead )
                      pBuffer->nFlags = OMX_BUFFERFLAG_EOS;         
                else
                      pBuffer->nFlags = 0;
         }
        
        else
        {
                if( 0 == nRead )
                      pBuffer->nFlags = OMX_BUFFERFLAG_EOS;         
                else
                   pBuffer->nFlags = 0; 
        }

        if(pBuffer->nFilledLen!=0)
        {
                if (pBuffer->nFlags == OMX_BUFFERFLAG_EOS)
                    printf("EOS send on last data buffer\n");
                
                pBuffer->nTimeStamp = rand() % 100;
                error = pComponent->EmptyThisBuffer(pHandle, pBuffer);
                if (error == OMX_ErrorIncorrectStateOperation) 
                        error = 0;



        }
        FirstTime=0;
        /*printf("Fin de send_input buffer...");*/

        return error;
}

/* ================================================================================= */
/**
* @fn fill_data() description for fill_data
fill_data().
Fills input buffer
*
*/
/* ================================================================================ */

int fill_data (OMX_U8 *pBuf, int mode, FILE *fIn)
{
    int nBytesRead = 0;
    static unsigned long totalRead = 0;
    static int fileHdrReadFlag = 0;

    if (OMX_AUDIO_AMRFrameFormatConformance == mode) 
    {
         if (!fileHdrReadFlag) 
         {
            fprintf (stderr, "Reading the file in Frame Format Conformance Mode\n");            
            fileHdrReadFlag = 1;
         }

		if (testCaseNo == 7) {
			numRead = 2*INPUT_WBAMRDEC_BUFFER_SIZE;
		}
		else if (testCaseNo == 8) {
			numRead = INPUT_WBAMRDEC_BUFFER_SIZE/2;
		}
		else if (testCaseNo == 9) {
			if (alternate == 0) {
				numRead = 2*INPUT_WBAMRDEC_BUFFER_SIZE;
				alternate = 1;
			}
			else {
				numRead = INPUT_WBAMRDEC_BUFFER_SIZE/2;
				alternate = 0;
			}
		}
		else {
			numRead = INPUT_WBAMRDEC_BUFFER_SIZE;
		}		
        nBytesRead = fread(pBuf, 1, numRead , fIn);		

    } 
    
    else if(OMX_AUDIO_AMRFrameFormatFSF == mode)
    { /*Mime Mode*/
        static const unsigned char FrameLength[] = {18, 24, 33, 37, 41, 47, 51, 59, 61,6};
        char size = 0;
        int index = 45; /* some invalid value*/
        signed char first_char;
        if (!fileHdrReadFlag) {
           fprintf (stderr, "Reading the file in MIME Mode\n");
           /* Read the file header */
           if((fgetc(fIn) != 0x23) || (fgetc(fIn) != 0x21) ||
              (fgetc(fIn) != 0x41) || (fgetc(fIn) != 0x4d) ||
              (fgetc(fIn) != 0x52) || (fgetc(fIn) != 0x2d) ||
              (fgetc(fIn) != 0x57) || (fgetc(fIn) != 0x42) ||
              (fgetc(fIn) != 0x0a)) {
                   fprintf(stderr, "Error:  File does not have correct header\n");
           }
           fileHdrReadFlag = 1;
        }
        first_char = fgetc(fIn);
        if (EOF == first_char) {
            fprintf(stderr, "Got EOF character\n");
            nBytesRead = 0;
            goto EXIT;
        }
        index = (first_char>>3) & 0xf;

        if((index>=0) && (index<10))
        {
            size = FrameLength[index];
            *((OMX_S8*)(pBuf)) = first_char;
            nBytesRead = fread(((char*)(pBuf))+1, 1, size - 1, fIn);
            nBytesRead += 1;
        } 
        else if (index == 14) {
            *((char*)(pBuf)) = first_char;
            nBytesRead = 1;
        } 
        else if (index == 15) {
            *((char*)(pBuf)) = first_char;
            nBytesRead = 1;
        } 
        else {
            nBytesRead = 0;
            fprintf(stderr, "Invalid Index in the frame index1 = %d \n", index);
            goto EXIT;
        }
    }
    
    else if(OMX_AUDIO_AMRFrameFormatIF2 == mode)
    {/*IF2*/
        static const OMX_U8 FrameLength[] = {18, 23, 33, 37, 41, 47, 51, 59, 61, 6};/*List of valid IF2 frame length*/
        OMX_S8 size = 0;
        OMX_S16 index = 45; /* some invalid value*/
        OMX_S8 first_char;
         
        if (!fileHdrReadFlag) {
           fprintf (stderr, "Reading the file in IF2 Mode\n");
           fileHdrReadFlag = 1;
        }

        first_char = fgetc(fIn);
        if (EOF == first_char) {
            fprintf(stderr, "Got EOF character\n");
            nBytesRead = 0;
            goto EXIT;
        }		
        index = (first_char>>4) & 0xf;
        if((index>=0) && (index<=9))
        {
            size = FrameLength[index];
            *((OMX_S8*)(pBuf)) = first_char;
            nBytesRead = fread(((OMX_S8*)(pBuf))+1, 1, size - 1, fIn);
            nBytesRead += 1;
		} else if (index == 14) {
            *((char*)(pBuf)) = first_char;
            nBytesRead = 1;
        } else if (index == 15) {
            *((OMX_S8*)(pBuf)) = first_char;
            nBytesRead = 1;
        } else {
            nRead = 0;
            printf("Invalid Index in the frame index2 = %d \n", index);
            goto EXIT;
        }   




    }

    totalRead += nBytesRead;
EXIT:
    return nBytesRead;
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
					fprintf(stderr, "\nError:  WaitForState reports an error %X!!!!!!!\n", error);
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
                    fprintf (stderr,"\nError from SendCommand-Pasue State function!!!!!!\n");
                    goto EXIT;
		}
	error =	WaitForState(pHandle, OMX_StatePause);
#ifdef OMX_GETTIME
	GT_END("Call to SendCommand <OMX_StatePause>");
#endif
    if(error != OMX_ErrorNone) {
					fprintf(stderr, "\nError:  WaitForState reports an error %X!!!!!!!\n", error);
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
                    fprintf (stderr,"\nError from SendCommand-Executing State function!!!!!!!\n");
                    goto EXIT;
		}
	error =	WaitForState(pHandle, OMX_StateExecuting);
#ifdef OMX_GETTIME
	GT_END("Call to SendCommand <OMX_StateExecuting>");
#endif
    if(error != OMX_ErrorNone) {
					fprintf(stderr, "\nError:  WaitForState reports an error %X!!!!!!!\n", error);
					goto EXIT;
	}
EXIT:
    return error;
}

#ifdef APP_DEBUGMEM
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
         printf("Allocating %d bytes on address %p, line %d file %s\n", size, p, line, s);
         return p;
   }
}

int myfree(void *dp, int line, char *s){
    int q;
    if (dp==NULL){
       printf("Null Memory can not be deleted line: %d  file: %s\n", line, s);
       return 0;
    }
        
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
     return 1;
}
#endif
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
	printf("%d::Freeing all resources by state invalid \n",__LINE__);
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_U16 i; 
	for(i=0; i < NIB; i++) {
		   printf("%d :: APP: About to free pInputBufferHeader[%d]\n",__LINE__, i);
		   eError = OMX_FreeBuffer(pHandle, OMX_DirInput, pBufferIn+i);

	}


	for(i=0; i < NOB; i++) {
		   printf("%d :: APP: About to free pOutputBufferHeader[%d]\n",__LINE__, i);
		   eError = OMX_FreeBuffer(pHandle, OMX_DirOutput, pBufferOut+i);
	}

	/*i value is fixed by the number calls to malloc in the App */
	for(i=0; i<6;i++)  
	{
		if (ArrayOfPointers[i] != NULL)
			free(ArrayOfPointers[i]);
	}
	

	    eError = close (IpBuf_Pipe[0]);
	    eError = close (IpBuf_Pipe[1]);
	    eError = close (OpBuf_Pipe[0]);
	    eError = close (OpBuf_Pipe[1]);
		eError = close (Event_Pipe[0]);
		eError = close (Event_Pipe[1]);
	if(fileOut != NULL)	/* Could have been closed  previously */
	{
		fclose(fileOut);
		fileOut=NULL;
	}

	if(fileIn != NULL)
	{	fclose(fileIn);
		fileIn=NULL;
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
		printf("%d::Freeing all resources by state invalid \n",__LINE__);
    	/* free the UseBuffers */
	    for(i=0; i < NIB; i++) {
		   UseInpBuf[i] = UseInpBuf[i] - 128;
		   printf("%d :: [TESTAPPFREE] pInputBuffer[%d] = %p\n",__LINE__,i,(UseInpBuf[i]));
		   if(UseInpBuf[i] != NULL){
			  newfree(UseInpBuf[i]);
			  UseInpBuf[i] = NULL;
		   }
		}

	    for(i=0; i < NOB; i++) {
		   UseOutBuf[i] = UseOutBuf[i] - 128;
		   printf("%d :: [TESTAPPFREE] pOutputBuffer[%d] = %p\n",__LINE__,i, UseOutBuf[i]);
		   if(UseOutBuf[i] != NULL){
			  newfree(UseOutBuf[i]);
			  UseOutBuf[i] = NULL;
		   }
		}

	/*i value is fixed by the number calls to malloc in the App */
		for(i=0; i<6;i++)  
		{
			if (ArrayOfPointers[i] != NULL)
				free(ArrayOfPointers[i]);
		}
	
			eError = close (IpBuf_Pipe[0]);
			eError = close (IpBuf_Pipe[1]);
			eError = close (OpBuf_Pipe[0]);
			eError = close (OpBuf_Pipe[1]);

		if (fileOut != NULL)	/* Could have been closed  previously */
		{
			fclose(fileOut);
			fileOut=NULL;
		}
		
		if (fileIn != NULL)
		{	fclose(fileIn);
			fileIn=NULL;
		}
	
		OMX_FreeHandle(pHandle);
	
		return eError;
}

#endif
