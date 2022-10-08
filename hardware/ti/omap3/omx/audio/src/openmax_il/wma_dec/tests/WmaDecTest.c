
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
#include <TIDspOmx.h>
#include <pthread.h>
#include <stdio.h>
#include <linux/soundcard.h>


#ifdef OMX_GETTIME 
    #include "OMX_Common_Utils.h"
    #include "OMX_GetTime.h"     /*Headers for Performance & measuremet    */
#endif
  
#define INPUT_WMADEC_BUFFER_SIZE 4096 * 4
#define OUTPUT_WMADEC_BUFFER_SIZE 4096 * 10
#define NUM_WMADEC_INPUT_BUFFERS 1
#define NUM_WMADEC_OUTPUT_BUFFERS 1
#define STRESS_TEST_INTERATIONS 20

#define FIFO1 "/dev/fifo.1"
#define FIFO2 "/dev/fifo.2"

#define OMX_WMADEC_NonMIME 1
#define MIME_HEADER_LEN 6
#define WINDOW_PLAY_OFFSET 2
#undef APP_DEBUG
#undef APP_MEMCHECK
#undef TWOINPUTBUFFERS
#undef USE_BUFFER 
/*For timestamp and tickcount*/
#undef APP_TIME_TIC_DEBUG
/* This Macro turns On the logic to detec memory
   leaks on the App. To debug the component, 
   WMADEC_MEMDEBUG must be defined.*/
#undef APP_MEMDEBUG

#ifdef APP_DEBUG
    #define APP_DPRINT(...)    fprintf(stderr,__VA_ARGS__)
#else
    #define APP_DPRINT(...)
#endif

#ifdef APP_TIME_TIC_DEBUG
	#define TIME_PRINT(...)		fprintf(stderr,__VA_ARGS__)
	#define TICK_PRINT(...)		fprintf(stderr,__VA_ARGS__)
#else
	#define TIME_PRINT(...)     if(frameMode)fprintf(stderr,__VA_ARGS__)
	#define TICK_PRINT(...)     if(frameMode)fprintf(stderr,__VA_ARGS__)
#endif

#ifdef APP_MEMDEBUG
    void *arr[500];
    int lines[500];
    int bytes[500];
    char file[500][50];
    int r;
    #define newmalloc(x) mymalloc(__LINE__,__FILE__,x)
    #define newfree(z) myfree(z,__LINE__,__FILE__)
    void * mymalloc(int line, char *s, int size);
    int myfree(void *dp, int line, char *s);
#else
    #define newmalloc(x) malloc(x)
    #define newfree(z) free(z)
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

/* ======================================================================= */
/**
 *      Global variales declaration
 */
/* ======================================================================= */
#ifdef OMX_GETTIME
  OMX_ERRORTYPE eError = OMX_ErrorNone;
  int GT_FlagE = 0;  /* Fill Buffer 1 = First Buffer,  0 = Not First Buffer  */
  int GT_FlagF = 0;  /*Empty Buffer  1 = First Buffer,  0 = Not First Buffer  */
  static OMX_NODE* pListHead = NULL;
#endif

FILE *fpRes;
static OMX_BOOL bInvalidState;
void* ArrayOfPointers[6];

pthread_mutex_t WaitForState_mutex;
pthread_cond_t  WaitForState_threshold;
OMX_U8          WaitForState_flag;
OMX_U8		TargetedState;

int wmadecfdwrite;
int wmadecfdread;

/*AM_COMMANDDATATYPE cmd_data;*/

int gMimeFlag = 0;
int gStateNotifi = 0;
int gState;
int lastBufferSent = 0;
static int  playCompleted = 0;
int sampleRateChange = 0;

fd_set rfds;
int done = 0;
int whileloopdone = 0;
int frameMode = 0;
/******************************************************************************/
OMX_S16 numInputBuffers = 0;
OMX_S16 numOutputBuffers = 0;
#ifdef USE_BUFFER
    OMX_U8* pInputBuffer[10];       
    OMX_U8* pOutputBuffer[10];
#endif

OMX_BUFFERHEADERTYPE* pInputBufferHeader[10];
OMX_BUFFERHEADERTYPE* pOutputBufferHeader[10];

/*FILE *fIn=NULL;
int timeToExit = 0;*/
/* RM control */
int preempted = 0;
int Event_Pipe[2];
/******************************************************************************/

/* ======================================================================= */
/**
 *      Enumered Types
 */
/* ======================================================================= */
typedef enum COMPONENTS {
    COMP_1,
    COMP_2
}COMPONENTS;

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
/**
 *      Function Declaration
 */
/* ======================================================================= */

int GetInfoFromBufferHeader(unsigned char **pBufPtr, int *pCurBitRate,
                                                int *pNextBitRateFlag);

void ResetBufferPointers(unsigned char **pBuffer);
int maxint(int a, int b);

int fill_data (OMX_BUFFERHEADERTYPE *pBuf, FILE *fIn);
int fill_data_tc7 (OMX_BUFFERHEADERTYPE *pBuf, FILE *fIn);
int unParse_Header (OMX_U8* pBuffer, FILE *fIn, int * payload);
void ConfigureAudio();

OMX_STRING strWmaEncoder = "OMX.TI.WMA.decode";
int IpBuf_Pipe[2];
int OpBuf_Pipe[2];
OMX_ERRORTYPE StopComponent(OMX_HANDLETYPE *pHandle);
OMX_ERRORTYPE send_input_buffer (OMX_HANDLETYPE pHandle, OMX_BUFFERHEADERTYPE* pBuffer, FILE *fIn);
OMX_ERRORTYPE send_input_buffer_tc7 (OMX_HANDLETYPE pHandle, OMX_BUFFERHEADERTYPE* pBuffer, FILE *fIn);
void fill_init_params(OMX_HANDLETYPE pHandle,const char * filename, int dasfmode, TI_OMX_DATAPATH * dataPath);
float calc_buff_size(FILE *fIn);
int FreeAllResources( OMX_HANDLETYPE pHandle,
			                OMX_BUFFERHEADERTYPE* pBufferIn,
			                OMX_BUFFERHEADERTYPE* pBufferOut,
			                int NIB, int NOB,
			                FILE* fIn, FILE* fOut);

#ifdef USE_BUFFER
int  freeAllUseResources(OMX_HANDLETYPE pHandle,
						  OMX_U8* UseInpBuf[],
						  OMX_U8* UseOutBuf[], 			
						  int NIB, int NOB,
						  FILE* fIn, FILE* fOut);

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
     OMX_ERRORTYPE eError1 = OMX_ErrorNone;
     /*int nCnt = 0;*/
     OMX_COMPONENTTYPE *pComponent = (OMX_COMPONENTTYPE *)pHandle;
     eError1 = pComponent->GetState(pHandle, &CurState);
	 if (CurState == OMX_StateInvalid && bInvalidState == OMX_TRUE)
	 {
		 	eError1 = OMX_ErrorInvalidState;
	 }
 	if( (eError1 == OMX_ErrorNone) &&
            (CurState != DesiredState) )
    {
        APP_DPRINT( "%d :: App: WaitForState\n", __LINE__);
        WaitForState_flag = 1;
        TargetedState = DesiredState;
        pthread_mutex_lock(&WaitForState_mutex); 
        pthread_cond_wait(&WaitForState_threshold, &WaitForState_mutex);/*Going to sleep till signal arrives*/
        pthread_mutex_unlock(&WaitForState_mutex); 
        APP_DPRINT( "%d :: App: WaitForState\n", __LINE__);

     }
     if( eError1 != OMX_ErrorNone ) return eError1;
     return OMX_ErrorNone;
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
OMX_ERRORTYPE EventHandler(OMX_HANDLETYPE hComponent, OMX_PTR pAppData,
							OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2,
							OMX_PTR pEventData)
{
   OMX_U8 writeValue;  

   OMX_COMPONENTTYPE *pComponent = (OMX_COMPONENTTYPE *)hComponent;
   OMX_STATETYPE state;
   OMX_ERRORTYPE eError1;
   
#ifdef APP_DEBUG
   int iComp = *((int *)(pAppData));
#endif
   eError1 = pComponent->GetState (hComponent, &state);
   
   if(eError1 != OMX_ErrorNone) {
       APP_DPRINT("%d :: App: Error returned from GetState\n",__LINE__);
   }

   switch (eEvent) {
       case OMX_EventCmdComplete:
			if (nData1 == OMX_CommandPortDisable) {
				if (nData2 == OMX_DirInput) {
					APP_DPRINT ( "%d Component State Changed To %d\n", __LINE__,state);
				}
				if (nData2 == OMX_DirOutput) {
					APP_DPRINT ( "%d Component State Changed To %d\n", __LINE__,state);
				}
			}

            if ((nData1 == OMX_CommandStateSet) && 
				(TargetedState == nData2) && (WaitForState_flag))
            {
                APP_DPRINT( "%d :: App: Component State Changed To %d\n", __LINE__,state);
                WaitForState_flag = 0;
                pthread_mutex_lock(&WaitForState_mutex);
                /*Sending Waking Up Signal*/
                pthread_cond_signal(&WaitForState_threshold);
                pthread_mutex_unlock(&WaitForState_mutex);
            }        
            APP_DPRINT( "%d :: App: Component State Changed To %d\n", __LINE__,state);
            break;
		case OMX_EventResourcesAcquired:
	        writeValue = 1;
	        preempted = 0;
	        write(Event_Pipe[1], &writeValue, sizeof(OMX_U8));
	        break;

       case OMX_EventError:
           if (nData1 != OMX_ErrorNone) {
		   
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
	   if (nData1 == OMX_ErrorInvalidState) {
	   		bInvalidState =OMX_TRUE;
	   }
           break;
       case OMX_EventMax:
           break;
       case OMX_EventMark:
           break;
		case OMX_EventBufferFlag:
			APP_DPRINT( "%d :: App: EOS Event Received\n", __LINE__);
            if((int)pEventData == OMX_BUFFERFLAG_EOS)
            {
                playCompleted = 1;
				writeValue = 2;  
				write(Event_Pipe[1], &writeValue, sizeof(OMX_U8));
            }
			if(nData2 == (OMX_U32)OMX_BUFFERFLAG_EOS) { 
				if(nData1 == (OMX_U32)NULL)
					puts("IN Buffer flag received!");
				else if(nData1 == (OMX_U32)NULL)
					puts("OUT Buffer flag received!");
			}
			break;
       default:
           break;
   }
    return eError1;
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
    APP_DPRINT("APP:::: OUTPUT BUFFER = %p && %p, pBuffer->nFilledLen = %d\n",
            pBuffer, pBuffer->pBuffer, pBuffer->nFilledLen);
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
    APP_DPRINT ("APP:::: INPUT BUFFER = %p && %p\n",pBuffer, pBuffer->pBuffer);
    APP_DPRINT("EmptyBufferDone\n");
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

printf("\n*************************************************************************    \n*********** Entering to the WMA DEC TEST APP\n **************************************************************************\n");
    OMX_CALLBACKTYPE WmaCaBa = {(void *)EventHandler,
                (void*)EmptyBufferDone,
                                (void*)FillBufferDone};
    OMX_HANDLETYPE pHandle;
    OMX_ERRORTYPE error = OMX_ErrorNone;
    OMX_U32 AppData = 100;
    OMX_PARAM_PORTDEFINITIONTYPE* pCompPrivateStruct;
    OMX_AUDIO_PARAM_WMATYPE *pWmaParam;
    OMX_COMPONENTTYPE *pComponent_dasf;
	OMX_COMPONENTTYPE *pComponent = (OMX_COMPONENTTYPE *)pHandle;
    OMX_STATETYPE state;
    /* TODO: Set a max number of buffers */
    OMX_BUFFERHEADERTYPE* pInputBufferHeader[10];
    /* TODO: Set a max number of buffers */
    OMX_BUFFERHEADERTYPE* pOutputBufferHeader[10];
	/*OMX_U32 count = 0;*/
    /*OMX_U32 fdmax = 0;*/

	FILE *fOut = NULL, *fIn = NULL;
	
	bInvalidState=OMX_FALSE;
    TI_OMX_DATAPATH dataPath;    
	OMX_INDEXTYPE index;
	
    int k;
    OMX_STATETYPE curState;

    int numInputBuffers = 0;
    int numOutputBuffers = 0;
    struct timeval tv;
    int retval, i, j;
    int frmCount = 0;
    int frmCnt = 1;
    int testcnt = 1;
    int testcnt1 = 1;
    int dasfmode = 0;
    int stress = 0;
    OMX_U64 outBuffSize;
    char fname[15] = "output";
	
    pthread_mutex_init(&WaitForState_mutex, NULL);
    pthread_cond_init (&WaitForState_threshold, NULL);
    WaitForState_flag = 0;    
    
    APP_DPRINT("------------------------------------------------------\n");
    APP_DPRINT("This is Main Thread In WMA DECODER Test Application:\n");
    APP_DPRINT("Test Core 1.5 - " __DATE__ ":" __TIME__ "\n");
    APP_DPRINT("------------------------------------------------------\n");

#ifdef OMX_GETTIME
    printf("Line %d\n",__LINE__);
      eError = OMX_ListCreate(&pListHead);
        printf("Line %d\n",__LINE__);
      printf("eError = %d\n",eError);
      GT_START();
  printf("Line %d\n",__LINE__);
#endif

    /* check the input parameters */
    if(!(argc == 7 || argc == 8))  {
        printf( "Usage: test infile [outfile] args. Wrong Arguments: See Example Below\n");
        printf( "./WmaDecTest_common test1_wma_v8_5kbps_8khz_1.rca test1_wma_v8_5kbps_8khz_1.WMA 1 0 1 1\n");
        goto EXIT;
    }
    numInputBuffers = atoi(argv[5]);
    numOutputBuffers = atoi(argv[6]);
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
    
	if( argv[7] != NULL  )
	{
        if(!strcmp(argv[7], "FRAME"))
        {
            frameMode = 1;
            printf("******** Frame mode selected *********\n");
        }
        else
        {
            stress = atoi(argv[7]);
        }
	}

    /* check to see that the input file exists */
    struct stat sb = {0};
    int status = stat(argv[1], &sb);
    if( status != 0 ) {
        APP_DPRINT( "Cannot find file %s. (%u)\n", argv[1], errno);
        goto EXIT;
    }
    APP_DPRINT("%d :: WmaTest\n",__LINE__);
    dasfmode = atoi(argv[4]);
    if(dasfmode == 1){
        printf("DASF MODE\n");
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
    /* Open the file of data to be rendered. */
    fIn = fopen(argv[1], "r");
    if( fIn == NULL ) {
        APP_DPRINT( "Error:  failed to open the file %s for readonly\access\n", argv[1]);
        goto EXIT;
    }

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
        APP_DPRINT( "%d [WMADECTEST] Error: Empty Data Pipe failed to open\n",__LINE__);
        goto EXIT;
    }

    /* save off the "max" of the handles for the selct statement */
    int fdmax = maxint(IpBuf_Pipe[0], OpBuf_Pipe[0]);

    APP_DPRINT("%d :: WmaTest\n",__LINE__);
    error = TIOMX_Init();
    APP_DPRINT("%d :: WmaTest\n",__LINE__);
    if(error != OMX_ErrorNone) {
        APP_DPRINT("%d :: Error returned by TIOMX_Init()\n",__LINE__);
        goto EXIT;
    }

    APP_DPRINT("%d :: WmaTest\n",__LINE__);
    switch (atoi(argv[3])) {
        case 1:
               printf ("-------------------------------------\n");
               printf ("Testing Simple PLAY till EOF \n");
               printf ("-------------------------------------\n");
               break;
        case 2:
               printf ("-------------------------------------\n");
               printf ("Testing Stop and Play \n");
               printf ("-------------------------------------\n");
               strcat(fname,"_tc2.pcm");
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
               strcat(fname,"_tc5.pcm");
				if (stress)
				{
					testcnt = 100;
					printf("******   Stress testing selected, running 100 iterations   ******\n");
				}
				else
				{
					testcnt = STRESS_TEST_INTERATIONS;
				}
               break;
        case 6:
               printf ("------------------------------------------------\n");
               printf ("Testing Repeated PLAY with Deleting Component\n");
               printf ("------------------------------------------------\n");
               strcat(fname,"_tc6.pcm");
			   	if (stress)
				{
					testcnt1 = 100;
					printf("******   Stress testing selected, running 100 iterations   ******\n");
				}
				else
				{
					testcnt1 = STRESS_TEST_INTERATIONS;
				}
               break;
        case 7:
               printf ("------------------------------------------------\n");
               printf ("Testing Window Play\n");
               printf ("------------------------------------------------\n");
               strcat(fname,"_tc6.pcm");
               break;
        case 8:
            printf ("------------------------------------------------\n");
            printf ("Testing Power Management\n");
            printf ("------------------------------------------------\n");
            sampleRateChange = 1;
            break;
		case 9:
		   printf ("------------------------------------------------------------\n");
		   printf ("Testing Ringtone test\n");
		   printf ("------------------------------------------------------------\n");
		   strcat(fname,"_tc9.pcm");
		   testcnt = 10;
		   break;	
    }

    APP_DPRINT("%d :: WmaTest\n",__LINE__);
        dasfmode = atoi(argv[4]);
        if(dasfmode == 1){
            printf("DASF MODE\n");
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

    APP_DPRINT("%d :: WmaTest\n",__LINE__);
    for(j = 0; j < testcnt1; j++) {
        whileloopdone = 0;
        if(j > 0) {
            printf ("=Decoding the file %d Time\n",j+1);
            close(IpBuf_Pipe[0]);
            close(IpBuf_Pipe[1]);
            close(OpBuf_Pipe[0]);
            close(OpBuf_Pipe[1]);
			close(Event_Pipe[0]);
            close(Event_Pipe[1]);

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
                APP_DPRINT( "%d [GSMFRTEST] Error:Empty Data Pipe failed to open\n",__LINE__);
                goto EXIT;
            }
            fIn = fopen(argv[1], "r");
            if( fIn == NULL ) {
                fprintf(stderr, "Error:  failed to open the file %s for readonly\
                                                                   access\n", argv[1]);
                goto EXIT;
            }
            fOut = fopen(fname, "w");
            if( fOut == NULL ) {
                fprintf(stderr, "Error:  failed to create the output file \n");
                goto EXIT;
            }
            error = TIOMX_Init();
			if(error != OMX_ErrorNone) {
			APP_DPRINT("%d [GSMFRTEST] Error returned by OMX_Init()\n",__LINE__);
            goto EXIT;
            }
         }

#ifdef DSP_RENDERING_ON  
        if((wmadecfdwrite=open(FIFO1,O_WRONLY))<0)
        {
            printf("[WmaTEST] - failure to open WRITE pipe\n");
        }
        else
        {
            printf("[WmaTEST] - opened WRITE pipe\n");
        }
        if((wmadecfdread=open(FIFO2,O_RDONLY))<0)
        {
            printf("[WmaTEST] - failure to open READ pipe\n");
            goto EXIT;
        }
        else
        {
            printf("[WmaTEST] - opened READ pipe\n");
        }
#endif
    /* Load the WMA Encoder Component */
    APP_DPRINT("%d :: WmaTest\n",__LINE__);
#ifdef OMX_GETTIME
  GT_START();
    error = TIOMX_GetHandle(&pHandle, strWmaEncoder, &AppData, &WmaCaBa);
  GT_END("Call to GetHandle");
#else
    error = TIOMX_GetHandle(&pHandle, strWmaEncoder, &AppData, &WmaCaBa);
#endif   

    APP_DPRINT("%d :: WmaTest\n",__LINE__);
    if((error != OMX_ErrorNone) || (pHandle == NULL)) {
        APP_DPRINT ("Error in Get Handle function\n");
        goto EXIT;
    }

    APP_DPRINT("%d :: WmaTest\n",__LINE__);
    pCompPrivateStruct = newmalloc (sizeof (OMX_PARAM_PORTDEFINITIONTYPE));
    ArrayOfPointers[0]=(OMX_PARAM_PORTDEFINITIONTYPE*)pCompPrivateStruct;
    APP_MEMPRINT("%d:::[TESTAPPALLOC] %p\n",__LINE__,pCompPrivateStruct);

    APP_DPRINT("%d :: WmaTest\n",__LINE__);
    pCompPrivateStruct->nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
    pCompPrivateStruct->nVersion.s.nVersionMajor = 0xF1;
    pCompPrivateStruct->nVersion.s.nVersionMinor = 0xF2;
    APP_DPRINT("%d :: WmaTest\n",__LINE__);

    /* Send input port config */
    pCompPrivateStruct->nPortIndex = OMX_DirInput;
    pCompPrivateStruct->format.audio.cMIMEType = newmalloc(20);
    ArrayOfPointers[1]=pCompPrivateStruct->format.audio.cMIMEType;
    APP_MEMPRINT("%d:::[TESTAPPALLOC] %p\n",__LINE__,pCompPrivateStruct->format.audio.cMIMEType);

#ifndef USE_BUFFER
    APP_DPRINT("%d :: About to call OMX_AllocateBuffer\n",__LINE__);
    for (i=0; i < numInputBuffers; i++) {
    /* allocate input buffer */
    APP_DPRINT("%d :: About to call OMX_AllocateBuffer\n",__LINE__);
    error = OMX_AllocateBuffer(pHandle,&pInputBufferHeader[i],0,NULL,INPUT_WMADEC_BUFFER_SIZE);
    APP_DPRINT("%d :: called OMX_AllocateBuffer\n",__LINE__);
		if(error != OMX_ErrorNone) {
			APP_DPRINT("%d :: Error returned by OMX_AllocateBuffer()\n",__LINE__);
        goto EXIT;
		}

    }

    if (frameMode)
    {
        outBuffSize = calc_buff_size(fIn);
        APP_DPRINT("%d :: outBuffSize %lld\n", outBuffSize, __LINE__);
        fclose(fIn);
        fIn = fopen(argv[1], "r");
        if(fIn == NULL) {
            fprintf(stderr, "Error:  failed to open the file %s for readonly access\n", argv[1]);
            goto EXIT;
        }        
    } 
    else if (dasfmode == 1)
    {
        outBuffSize = 4096 * 2 * 2;
    }
    else
    {
        outBuffSize = OUTPUT_WMADEC_BUFFER_SIZE;
    }
    for (i=0; i < numOutputBuffers; i++) {
        /* allocate output buffer */
        APP_DPRINT("%d :: About to call OMX_AllocateBuffer\n",__LINE__);
        error = OMX_AllocateBuffer(pHandle,&pOutputBufferHeader[i],1,NULL,outBuffSize);
        APP_DPRINT("%d :: called OMX_AllocateBuffer\n",__LINE__);
        if(error != OMX_ErrorNone) {
            APP_DPRINT("%d :: Error returned by OMX_AllocateBuffer()\n",__LINE__);
            goto EXIT;
        }
    }
#else
    OMX_U8* pInputBuffer;
    OMX_U8* pOutputBuffer;
    

    APP_DPRINT("%d :: About to call OMX_UseBuffer\n",__LINE__);


    pOutputBuffer= newmalloc (OUTPUT_WMADEC_BUFFER_SIZE + 256);
    APP_MEMPRINT("%d:::[TESTAPPALLOC] %p\n",__LINE__,pOutputBuffer);
    pOutputBuffer = pOutputBuffer + 128;
    pInputBuffer = (OMX_U8*)newmalloc(INPUT_WMADEC_BUFFER_SIZE);
    APP_MEMPRINT("%d:::[TESTAPPALLOC] %p\n",__LINE__,pInputBuffer);

    /* allocate input buffer */
    APP_DPRINT("%d :: About to call OMX_UseBuffer\n",__LINE__);
    error = OMX_UseBuffer(pHandle,&pInputBufferHeader,0,NULL,INPUT_WMADEC_BUFFER_SIZE,pInputBuffer);
    APP_DPRINT("%d :: called OMX_UseBuffer\n",__LINE__);
    if(error != OMX_ErrorNone) {
        APP_DPRINT("%d :: Error returned by OMX_UseBuffer()\n",__LINE__);
        goto EXIT;
    }

    if (frameMode)
    {
        outBuffSize = calc_buff_size(fIn);
        printf("%d :: outBuffSize %d\n", outBuffSize, __LINE__);
        fclose(fIn);
        fIn = fopen(argv[1], "r");
        if(fIn == NULL) {
            fprintf(stderr, "Error:  failed to open the file %s for readonly access\n", argv[1]);
            goto EXIT;
        }        
    } 
    else
    {
        outBuffSize = OUTPUT_WMADEC_BUFFER_SIZE;
    }      
    
    /* allocate output buffer */
    APP_DPRINT("%d :: About to call OMX_UseBuffer\n",__LINE__);
    error = OMX_UseBuffer(pHandle,&pOutputBufferHeader,1,NULL,outBuffSize,pOutputBuffer);
    APP_DPRINT("%d :: called OMX_UseBuffer\n",__LINE__);
    if(error != OMX_ErrorNone) {
        APP_DPRINT("%d :: Error returned by OMX_UseBuffer()\n",__LINE__);
        goto EXIT;
    }

#endif
    pCompPrivateStruct->nPortIndex = OMX_DirInput;
    pCompPrivateStruct->eDir = OMX_DirInput;
    pCompPrivateStruct->nBufferCountActual = NUM_WMADEC_INPUT_BUFFERS;
    pCompPrivateStruct->nBufferSize = INPUT_WMADEC_BUFFER_SIZE;
    pCompPrivateStruct->format.audio.eEncoding = OMX_AUDIO_CodingWMA;



#ifdef OMX_GETTIME
  GT_START();
    error = OMX_SetParameter (pHandle,OMX_IndexParamAudioPortFormat, pCompPrivateStruct);
  GT_END("Set Parameter Test-SetParameter");
#else
    error = OMX_SetParameter (pHandle,OMX_IndexParamAudioPortFormat, pCompPrivateStruct);
#endif

    if (error != OMX_ErrorNone) {
        error = OMX_ErrorBadParameter;
        printf ("%d:: OMX_ErrorBadParameter\n",__LINE__);
        goto EXIT;
    }

    /* Send output port config */
    pCompPrivateStruct->nPortIndex = OMX_DirOutput;
    pCompPrivateStruct->eDir = OMX_DirOutput;
    pCompPrivateStruct->nBufferCountActual = NUM_WMADEC_OUTPUT_BUFFERS;
    pCompPrivateStruct->nBufferSize = outBuffSize;
    pCompPrivateStruct->format.audio.eEncoding = OMX_AUDIO_CodingPCM;

    if(dasfmode) 
    {
		pCompPrivateStruct->nBufferCountActual = 0;
    }
    
#ifdef OMX_GETTIME
  GT_START();
    error = OMX_SetParameter (pHandle,OMX_IndexParamAudioPortFormat, pCompPrivateStruct);
  GT_END("Set Parameter Test-SetParameter");
#else
    error = OMX_SetParameter (pHandle,OMX_IndexParamAudioPortFormat, pCompPrivateStruct);
#endif

    if (error != OMX_ErrorNone) {
        error = OMX_ErrorBadParameter;
        printf ("%d:: OMX_ErrorBadParameter\n",__LINE__);
        goto EXIT;
    }

    pWmaParam = newmalloc (sizeof (OMX_AUDIO_PARAM_WMATYPE));
    ArrayOfPointers[2]=(OMX_AUDIO_PARAM_WMATYPE*)pWmaParam;
    APP_MEMPRINT("%d:::[TESTAPPALLOC] %p\n",__LINE__,pWmaParam);
    pWmaParam->nSize = sizeof (OMX_AUDIO_PARAM_WMATYPE);
    pWmaParam->nVersion.s.nVersionMajor = 0xF1;
    pWmaParam->nVersion.s.nVersionMinor = 0xF2;
    pWmaParam->nPortIndex = OMX_DirInput;
    pWmaParam->nChannels = 1;
    pWmaParam->nBitRate = 8000;
    pWmaParam->eFormat = OMX_AUDIO_WMAFormat9;

#ifdef OMX_GETTIME
  GT_START();
    error = OMX_SetParameter (pHandle,OMX_IndexParamAudioWma,pWmaParam);
  GT_END("Set Parameter Test-SetParameter");
#else
    error = OMX_SetParameter (pHandle,OMX_IndexParamAudioWma,pWmaParam);
#endif

    if (error != OMX_ErrorNone) {
        error = OMX_ErrorBadParameter;
        printf ("%d:: OMX_ErrorBadParameter\n",__LINE__);
        goto EXIT;
    }

    pWmaParam->nPortIndex = OMX_DirOutput;
    pWmaParam->nChannels = 1;
    pWmaParam->nBitRate = 8000;
	
#ifdef OMX_GETTIME
  GT_START();
    error = OMX_SetParameter (pHandle,OMX_IndexParamAudioWma,pWmaParam);
  GT_END("Set Parameter Test--SetParameter");
#else
    error = OMX_SetParameter (pHandle,OMX_IndexParamAudioWma,pWmaParam);
#endif

    if (error != OMX_ErrorNone) {
        error = OMX_ErrorBadParameter;
        printf ("%d:: OMX_ErrorBadParameter\n",__LINE__);
        goto EXIT;
    }

    pComponent_dasf = (OMX_COMPONENTTYPE *)pHandle;

    if (dasfmode) 
    {
#ifdef RTM_PATH    
        dataPath = DATAPATH_APPLICATION_RTMIXER;
#endif

#ifdef ETEEDN_PATH
        dataPath = DATAPATH_APPLICATION;
#endif        
    }
    
    fill_init_params(pHandle,argv[1],dasfmode,&dataPath);
	
#ifdef OMX_GETTIME
  GT_START();
    error = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
  GT_END("Call to SendCommand <OMX_StateIdle>");
#else
    error = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
#endif
    
    if(error != OMX_ErrorNone) {
        APP_DPRINT ("Error from SendCommand-Idle(Init) State function\n");
        goto EXIT;
    }
	
    /* Wait for startup to complete */
    error = WaitForState(pHandle, OMX_StateIdle);
    if(error != OMX_ErrorNone) {
        APP_DPRINT( "Error:  hWmaEncoder->WaitForState reports an error %X\n", error);
        goto EXIT;
    }
    
    if(dasfmode) 
    {    
	    /* get streamID back to application */
	    error = OMX_GetExtensionIndex(pHandle, "OMX.TI.index.config.wmastreamIDinfo",&index);
	    if (error != OMX_ErrorNone) {
		    printf("Error getting extension index\n");
		    goto EXIT;
	    }
    }
    
    for(i = 0; i < testcnt; i++) {
            whileloopdone = 0;
        if(i > 0) {
            printf ("Decoding the file %d Time\n",i+1);

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
            fOut = fopen(fname, "w");
            if(fOut == NULL) {
                fprintf(stderr, "Error:  failed to create the output file \n");
                goto EXIT;
            }
        }

        done = 0;

		if (atoi(argv[3]) == 9){
			if(i==0){
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
		            APP_DPRINT( "Error:  hWmaEncoder->WaitForState reports an error %X\n", error);
		            goto EXIT;
		        }
			}
		}else{
	        printf ("Basic Function (else):: Sending OMX_StateExecuting Command\n");
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
	            APP_DPRINT( "Error:  hWmaEncoder->WaitForState reports an error %X\n", error);
	            goto EXIT;
	        }
		}
		
        if (atoi(argv[3]) == 7) {
	        for (k=0; k < numInputBuffers; k++) {
#ifdef OMX_GETTIME
	             if (k==0)
	              { 
	                GT_FlagE=1;  /* 1 = First Buffer,  0 = Not First Buffer  */
	                GT_START(); /* Empty Bufffer */
	              }
#endif        
	            error = send_input_buffer_tc7 (pHandle, pInputBufferHeader[k], fIn);
	        }
        }
        else {
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
        }
        if (dasfmode == 0) 
        {
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

        if(sampleRateChange) {
        /*
            cmd_data.hComponent = pHandle;
            cmd_data.AM_Cmd = AM_CommandWarnSampleFreqChange;  
            cmd_data.param1 = 44100;*/
#ifdef DSP_RENDERING_ON
            if((write(wmadecfdwrite, &cmd_data, sizeof(cmd_data)))<0) {
                printf("send command to audio manager\n");
            }
#endif    
        }
        OMX_GetState(pHandle,&curState);

        while((error == OMX_ErrorNone && curState != OMX_StateIdle ) 
				&& curState != OMX_StateInvalid) {
				
            FD_ZERO(&rfds);
            FD_SET(IpBuf_Pipe[0], &rfds);
            FD_SET(OpBuf_Pipe[0], &rfds);
            tv.tv_sec = 1;
            tv.tv_usec = 0;
            frmCount++;
            retval = select(fdmax+1, &rfds, NULL, NULL, &tv);
			
            if(retval == -1) {
                perror("select()");
                APP_DPRINT ( " : Error \n");
                break;
            }
			
            if(retval == 0) {
                APP_DPRINT ("%d :: BasicFn App Timeout !!!!!!!!!!! \n",__LINE__);
            }
			
            switch (atoi(argv[3])) {
            case 1:
            case 5:
            case 6:
            case 8:
                    if(FD_ISSET(IpBuf_Pipe[0], &rfds)) {
                       OMX_BUFFERHEADERTYPE* pBuffer;
                       read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
                       pBuffer->nTimeStamp = rand() % 100;
                       pBuffer->nTickCount = rand() % 70; 
                       TIME_PRINT("TimeStamp Input: %lld\n",pBuffer->nTimeStamp);
                       TICK_PRINT("TickCount Input: %ld\n",pBuffer->nTickCount);
                       if(!playCompleted)
                        {
                            APP_DPRINT("APP:: %d send_input_buffer (pHandle, pBuffer, fIn)\n", __LINE__);
                            error = send_input_buffer (pHandle, pBuffer, fIn);
                        }                 
                       if (error != OMX_ErrorNone) {
                           printf ("Error While reading input pipe\n");
                           goto EXIT;
                       }
                       frmCnt++;
                    }
                    break;
					
            case 7:
                    if(FD_ISSET(IpBuf_Pipe[0], &rfds)) {
                       OMX_BUFFERHEADERTYPE* pBuffer;
                       read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
                       error = send_input_buffer_tc7 (pHandle, pBuffer, fIn);
                       if (error != OMX_ErrorNone) {
                           printf ("Error While reading input pipe\n");
                           goto EXIT;
                       }
                       frmCnt++;
                    }
                    break;
					
            case 2:
                    if(frmCount == 10) {
                        printf (" Sending Stop command to Codec \n");
                        
#ifdef OMX_GETTIME
                        GT_START();
                        error = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
                        GT_END("Call to SendCommand <OMX_StateIdle> ");
#else
                        error = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
#endif
                        if(error != OMX_ErrorNone) {
                            fprintf (stderr,"Error from SendCommand-Pasue State function\n");
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
					
            case 4:
                if(FD_ISSET(IpBuf_Pipe[0], &rfds)) {

                    OMX_BUFFERHEADERTYPE* pBuffer;
                    read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
                    
                    if(frmCount >= 5) {
                        fprintf(stderr, "Shutting down ---------- \n");
#ifdef OMX_GETTIME
                        GT_START();
                        error = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateIdle, NULL);
                        GT_END("Call to SendCommand <OMX_StateIdle>");
#else
                        error = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateIdle, NULL);;
#endif
                        
                        if(error != OMX_ErrorNone) {
                            fprintf (stderr,"Error from SendCommand-Idle(Stop) State function\n");
                            goto EXIT;
                        }
                        done = 1;
                    }
                    else {
                        error = send_input_buffer (pHandle, pBuffer, fIn);
                        if (error != OMX_ErrorNone) {
                            printf ("Error While reading input pipe\n");
                            goto EXIT;
                        }
                    }
                    frmCnt++;
                }
                 break;
				 
            case 3:
                    if (frmCount == 8) {
                        printf (" Sending Resume command to Codec \n");
#ifdef OMX_GETTIME
                        GT_START();
                        error = OMX_SendCommand(pHandle, OMX_CommandStateSet,OMX_StateExecuting, NULL);
                        GT_END("Call to SendCommand <OMX_StateExecuting>");
#else
                        error = OMX_SendCommand(pHandle, OMX_CommandStateSet,OMX_StateExecuting, NULL);
#endif 
                        if(error != OMX_ErrorNone) {
                            fprintf (stderr,"Error from SendCommand-Executing State function\n");
                            goto EXIT;
                        }
                        /* Wait for startup to complete */
                        error = WaitForState(pHandle, OMX_StateExecuting);
                        
                        if(error != OMX_ErrorNone) {
                            fprintf(stderr, "Error:  hPcmDecoder->WaitForState reports an error %X\n", error);
                            goto EXIT;
                        }
                    }
                    if(frmCount == 6) {
                        printf (" Sending Pause command to Codec \n");

#ifdef OMX_GETTIME
                        GT_START();
                        error = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StatePause, NULL);
                        GT_END("Call to SendCommand <OMX_StatePause> ");
#else
                        error = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StatePause, NULL);
#endif 
                        if(error != OMX_ErrorNone) {
                            fprintf (stderr,"Error from SendCommand-Pasue State function\n");
                            goto EXIT;
                        }
                        /* Wait for startup to complete */
                        error = WaitForState(pHandle, OMX_StatePause);
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
			case 9:
					if(FD_ISSET(IpBuf_Pipe[0], &rfds)) {
						OMX_BUFFERHEADERTYPE* pBuffer;
						read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
						if(!playCompleted && pBuffer->nFlags!=OMX_BUFFERFLAG_EOS){
							pBuffer->nFlags = 0;
                            error = send_input_buffer (pHandle, pBuffer, fIn);
                        }
						
						if (error != OMX_ErrorNone) {
							goto EXIT;
						}
						frmCnt++;
					}
					break;
					
            default:
                    APP_DPRINT("%d :: ### Running Simple DEFAULT Case Here ###\n",__LINE__);
            }
            if( FD_ISSET(OpBuf_Pipe[0], &rfds) ) {
                OMX_BUFFERHEADERTYPE* pBuf;
                read(OpBuf_Pipe[0], &pBuf, sizeof(pBuf));
                fwrite(pBuf->pBuffer, 1, pBuf->nFilledLen, fOut);
                fflush(fOut);
                APP_DPRINT("Writing %d bytes to file\n",pBuf->nFilledLen);
                if (curState == OMX_StateExecuting && !playCompleted)
                {
                    pComponent->FillThisBuffer(pHandle, pBuf);
                }
				if (atoi(argv[3]) == 7) {
					pComponent->FillThisBuffer(pHandle, pBuf);
				}
				
    			if(pBuf->nFlags == OMX_BUFFERFLAG_EOS){
    				APP_DPRINT("State to Idle :: pBuf->nFlags == OMX_BUFFERFLAG_EOS\n");
					pBuf->nFlags = 0;
    			}
            }
			
			
            if(done == 1) {
                error = pComponent->GetState(pHandle, &state);
    			printf("done\n");
                if(error != OMX_ErrorNone) {
                    APP_DPRINT("%d:: Warning:  hWmaEncoder->GetState has returned status %X\n",
                                                                                      __LINE__, error);
                    goto EXIT;
                }
            }
            if (playCompleted){
				if((atoi(argv[3]) == 9) && (i != 9)){
					puts("*RING_TONE: Lets play again!");
					playCompleted = 0;
					goto my_exit;				
				}else{
					printf ("APP :: Play Completed. Stop component \n");
	                StopComponent(pHandle);
	                playCompleted = 0;
				}
            } 			
            OMX_GetState(pHandle,&curState);
        } /* While Loop Ending Here */
        
        printf ("The current state of the component = %d \n",curState);
		my_exit:
	        fclose(fOut);
	        fclose(fIn);

    } /*Inner for loop ends here */
	
    if(sampleRateChange) {
        /*cmd_data.hComponent = pHandle;
        cmd_data.AM_Cmd = AM_CommandWarnSampleFreqChange;  
        cmd_data.param1 = 48000;*/
		
#ifdef DSP_RENDERING_ON
        if((write(wmadecfdwrite, &cmd_data, sizeof(cmd_data)))<0) {
            printf("send command to audio manager\n");
        }
#endif
    }
/*
    cmd_data.hComponent = pHandle;
    cmd_data.AM_Cmd = AM_Exit;
    */
	
#ifdef DSP_RENDERING_ON    
    if((write(wmadecfdwrite, &cmd_data, sizeof(cmd_data)))<0)
        printf("%d ::OMX_WMADecoder.c :: [WMA Dec Component] - send command to audio manager\n",__LINE__);
    close(wmadecfdwrite);
    close(wmadecfdread);  
#endif    

    printf ("Sending the StateLoaded Command\n");
    
#ifdef OMX_GETTIME
    GT_START();
    error = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateLoaded, NULL);
    GT_END("Call to SendCommand <OMX_StateLoaded>");
#else
    error = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateLoaded, NULL);
#endif 
    if(error != OMX_ErrorNone) {
        APP_DPRINT ("%d:: Error from SendCommand-Idle State function\n",__LINE__);
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

#ifdef OMX_GETTIME
    GT_START();
    error = OMX_SendCommand(pHandle, OMX_CommandPortDisable, -1, NULL);
    GT_END("Call to SendCommand <OMX_CommandPortDisable>");
#else
    error = OMX_SendCommand(pHandle, OMX_CommandPortDisable, -1, NULL);
#endif 
	/* Wait for startup to complete */
    error = WaitForState(pHandle, OMX_StateLoaded);
    if(error != OMX_ErrorNone) {
        APP_DPRINT( "Error:  hWmaEncoder->WaitForState reports an error %X\n", error);
        goto EXIT;
    }	

#ifdef USE_BUFFER
    APP_MEMPRINT("%d:::[TESTAPPFREE] %p\n",__LINE__,pInputBuffer);
    newfree(pInputBuffer);
    pOutputBuffer -= 128;
    APP_MEMPRINT("%d:::[TESTAPPFREE] %p\n",__LINE__,pOutputBuffer);
    newfree(pOutputBuffer);
#endif
    printf ("Free the Component handle\n");
    /* Unload the WMA Encoder Component */
    error = TIOMX_FreeHandle(pHandle);
    APP_MEMPRINT("%d:::[TESTAPPFREE] %p\n",__LINE__,pWmaParam);
    newfree(pWmaParam);
    APP_MEMPRINT("%d:::[TESTAPPFREE] %p\n",__LINE__,pCompPrivateStruct->format.audio.cMIMEType);
    newfree(pCompPrivateStruct->format.audio.cMIMEType);
    APP_MEMPRINT("%d:::[TESTAPPFREE] %p\n",__LINE__,pCompPrivateStruct);
    newfree(pCompPrivateStruct);
    if( (error != OMX_ErrorNone)) {
        APP_DPRINT ("%d:: Error in Free Handle function\n",__LINE__);
        goto EXIT;
    }
    close(IpBuf_Pipe[0]);
    close(IpBuf_Pipe[1]);
    close(OpBuf_Pipe[0]);
    close(OpBuf_Pipe[1]);
    APP_DPRINT ("%d:: Free Handle returned Successfully \n\n\n\n",__LINE__);

    } /*Outer for loop ends here */
    
    pthread_mutex_destroy(&WaitForState_mutex);
    pthread_cond_destroy(&WaitForState_threshold);
	error = TIOMX_Deinit();
    if( (error != OMX_ErrorNone)) {
			APP_DPRINT("APP: Error in Deinit Core function\n");
			goto EXIT;
    }
#ifdef APP_MEMDEBUG
    printf("\n-Printing memory not deleted-\n");

    for(r=0;r<500;r++){

        if (lines[r]!=0){

            printf(" --->%d Bytes allocated on %p File:%s Line: %d\n",bytes[r],arr[r],file[r],lines[r]);                  

        }

    }
#endif
    
EXIT:
	if(bInvalidState==OMX_TRUE)
	{

#ifndef USE_BUFFER
		error = FreeAllResources(pHandle,
								pInputBufferHeader[0],
								pOutputBufferHeader[0],
								numInputBuffers, 
								numOutputBuffers,
								fIn,fOut);
#else
		error = freeAllUseResources(pHandle,
									pInputBuffer,
									pOutputBuffer,
									numInputBuffers,,
									numOutputBuffers,
									fIn,fOut);
#endif
	}else{	
	}
#ifdef OMX_GETTIME
  GT_END("WMA_DEC test <End>");
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
    int nRead = fill_data (pBuffer, fIn);

		
    if(nRead <= 0) {
        APP_DPRINT("Sending EOS\n");
        pBuffer->nFlags = OMX_BUFFERFLAG_EOS;
        pBuffer->nFilledLen = nRead;
        APP_DPRINT("nRead = %d\n",nRead);
        APP_DPRINT("Last EmptyThisBuffer\n");
        pComponent->EmptyThisBuffer(pHandle, pBuffer);
        lastBufferSent = 1;
    }
    else {
        APP_DPRINT("Send input buffer nRead = %d\n",nRead);
        pBuffer->nFilledLen = nRead;
        if(!(pBuffer->nFlags & OMX_BUFFERFLAG_CODECCONFIG)){
          pBuffer->nFlags = 0;
        }
        pComponent->EmptyThisBuffer(pHandle, pBuffer);
    }
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
OMX_ERRORTYPE send_input_buffer_tc7(OMX_HANDLETYPE pHandle, OMX_BUFFERHEADERTYPE* pBuffer, FILE *fIn)
{
    OMX_ERRORTYPE error = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pComponent = (OMX_COMPONENTTYPE *)pHandle;
    int nRead = fill_data_tc7 (pBuffer, fIn);
    if(nRead < pBuffer->nAllocLen) {
       pBuffer->nFlags = OMX_BUFFERFLAG_EOS;
#ifdef OMX_GETTIME
    GT_START();
       error = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateIdle, NULL);
    GT_END("Call to SendCommand <OMX_StateIdle> ");
#else
       error = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateIdle, NULL);
#endif 

    }
    else {
        pBuffer->nFilledLen = nRead;
        pComponent->EmptyThisBuffer(pHandle, pBuffer);
    }
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
int fill_data (OMX_BUFFERHEADERTYPE *pBuf,FILE *fIn)
{
    int nRead;
    OMX_U32 packetSize;
    OMX_U8 byteOffset;
    static OMX_U8 first_cap = 0;
    static OMX_U8 first_buff = 0;
    static int totalRead = 0;
    static int fileHdrReadFlag = 0;
    static int ccnt = 1;
    static int payload=0;
    OMX_U8 temp = 0;
    nRead = 0;
    byteOffset = 0;
    if(frameMode)
    {
      /* TODO: Update framemode TC to match component */
        if (!fileHdrReadFlag) {
            /*The first input buffer readed have the .rca header information*/
            nRead = fread(pBuf->pBuffer, 1, 70, fIn);
            byteOffset = 70;
            fileHdrReadFlag = 1;
        }
        /*Read packet header, buffer begins at the offset*/
        nRead += fread(pBuf->pBuffer + byteOffset, 1, 5, fIn);
        /*Extract Packet size*/
        packetSize = *((OMX_U32 *)(pBuf->pBuffer + byteOffset + 1));
        /*Read the rest of the packet, buffer begins at the end of the packet header*/
        byteOffset += 5;
        nRead += fread(pBuf->pBuffer + byteOffset, 1, packetSize, fIn);
    }
    else
    {
      if(first_buff){
        if (first_cap){
          fread(&temp,5,1,fIn); // moving file 5 bytes
        }
        first_cap =1;
        nRead = fread(pBuf->pBuffer, 1, payload, fIn);
        if(pBuf->nFlags & OMX_BUFFERFLAG_CODECCONFIG)
        {
          pBuf->nFlags = 0;
        }
      }
      else{
        nRead = unParse_Header(pBuf->pBuffer,fIn, &payload);
        pBuf->nFlags = OMX_BUFFERFLAG_CODECCONFIG;
        first_buff=1;
      }
    }
    totalRead += nRead;
    pBuf->nFilledLen = nRead;
    APP_DPRINT("\n*****************************************************\n");
    APP_DPRINT("%d :: App:: Read IpBuff = %p pBuf->nAllocLen = * %ld, nRead = %ld\n",
                   __LINE__, pBuf->pBuffer, pBuf->nAllocLen, nRead);
    APP_DPRINT("\n*****************************************************\n");    
    ccnt++;
    return nRead;
}

/* ================================================================================= */
/**
* @fn fill_data() description for fill_data  
fill_data().  
Fills input buffer
*
*/
/* ================================================================================ */
int fill_data_tc7 (OMX_BUFFERHEADERTYPE *pBuf,FILE *fIn)
{
    int nRead;
    static int totalRead = 0;
    static int fileHdrReadFlag = 0;
    static int ccnt = 1;
    OMX_U8* tempBuffer; 
    OMX_U8* pBufferOffset;

    if (!fileHdrReadFlag) {
        nRead = fread(pBuf->pBuffer, 1,75 , fIn);
        tempBuffer = newmalloc(19500*sizeof(OMX_U8));
        fread(tempBuffer, 1, 19500, fIn);
        newfree(tempBuffer);
        pBufferOffset = pBuf->pBuffer;
        pBufferOffset += 75;
        nRead += fread(pBufferOffset, 1, pBuf->nAllocLen-75, fIn);
        fileHdrReadFlag = 1;
    }
    else {
        nRead = fread(pBuf->pBuffer, 1, pBuf->nAllocLen , fIn);
    }

    APP_DPRINT("\n*****************************************************\n");
    APP_DPRINT ("%d :: App:: pBuf->pBuffer = %p pBuf->nAllocLen = * %ld, nRead = %ld\n",__LINE__, pBuf->pBuffer, pBuf->nAllocLen, nRead); 
    APP_DPRINT("\n*****************************************************\n");

    totalRead += nRead;
    pBuf->nFilledLen = nRead;
    ccnt++;
    return nRead;
}

float calc_buff_size(FILE *fIn)
{
    int nRead;
    OMX_U64 playDuration;
    OMX_U64 numPackets;
    OMX_U16 sampleRate;
    OMX_U16 channelNum;
    long double pcmBytesPerPacket;
    float outBuffSize;
    OMX_U8* pBuffer;
    pBuffer = newmalloc((OMX_U8) 100); 
        
    /*Read first 70 bytes header + 5 bytes first packet header*/
    nRead = fread(pBuffer, 1, 70, fIn);
    /*for(i = 0; i < 70; i++)
    {
        printf("pBuffer[%d] = 0x%x\n", i, pBuffer[i]);
    }*/
    numPackets = *((OMX_U64 *)pBuffer);
    playDuration = *((OMX_U64 *)(pBuffer + 8));
    /*playDuration = 906880000;*/
    sampleRate = *((OMX_U16 *)(pBuffer + 46));
    channelNum = *((OMX_U16 *)(pBuffer + 44));
    APP_DPRINT("numPackets %lld\n", numPackets);
    APP_DPRINT("playDuration %lld\n", playDuration);
    APP_DPRINT("sampleRate %d\n", sampleRate);
    APP_DPRINT("channelNum %d\n", channelNum);
    pcmBytesPerPacket = (long double)playDuration * (long double) sampleRate / (10000000 * (long double) numPackets);
    APP_DPRINT("pcmBytesPerPacket %f\n", pcmBytesPerPacket);
    outBuffSize = OUTPUT_WMADEC_BUFFER_SIZE + pcmBytesPerPacket * channelNum * 2;
    APP_DPRINT("outBuffSize %f\n", outBuffSize);
    newfree(pBuffer);
    return outBuffSize;
}

void fill_init_params(OMX_HANDLETYPE pHandle,const char * filename,int dasfmode, TI_OMX_DATAPATH * dataPath)
{
    OMX_ERRORTYPE error = OMX_ErrorNone;
    WMA_HeadInfo* pHeaderInfo;
	OMX_INDEXTYPE index;
	OMX_ERRORTYPE eError1 = OMX_ErrorNone;
	TI_OMX_DSP_DEFINITION dspDefinition;
    OMX_U32 samplerate;
    OMX_U8 i;
    OMX_U8 temp;
    OMX_U16 arr[50];
    FILE *parameterFile;
    pHeaderInfo = newmalloc(sizeof(WMA_HeadInfo));
    if (pHeaderInfo == NULL)
    {
        printf("Could not allocate pHeaderInfo\n");
		goto EXIT;
    }
    
    parameterFile = fopen(filename,"r");
    if (parameterFile == NULL) 
    {
        printf("Could not open file\n");
        goto EXIT;
    }
    memset(arr,(int) NULL, 50);
    /*Read first 50 bytes of input file*/
    for (i = 0 ; i< 50 ; i++)
    {
        fscanf(parameterFile, "%c", &temp);
        arr[i] = temp;
    }
    fclose(parameterFile);
    /*Obtain sample rate from 46th and 48th bytes*/
    samplerate = arr[47] << 8 | arr[46];

	dspDefinition.wmaHeaderInfo = pHeaderInfo;
    pHeaderInfo->iSamplePerSec = samplerate;
    pHeaderInfo->iChannel = arr[44];

	dspDefinition.dasfMode = dasfmode;
 
	eError1 = OMX_GetExtensionIndex(pHandle, "OMX.TI.index.config.wmaheaderinfo",&index);
	if (eError1 != OMX_ErrorNone) {
		printf("Error getting extension index\n");
		goto EXIT;
	}

    /*OMX_SetConfig(pHandle,index,&dspDefinition);*/
    /*cmd_data.hComponent = pHandle;
    cmd_data.AM_Cmd = AM_CommandIsOutputStreamAvailable;
    */
    /* for decoder, using AM_CommandIsInputStreamAvailable */
    /*cmd_data.param1 = 0;*/
#ifdef DSP_RENDERING_ON    
    if((write(wmadecfdwrite, &cmd_data, sizeof(cmd_data)))<0) {
        printf("%d [WMA Dec Component] - send command to audio manager\n", __LINE__);
        goto EXIT;
    }
    if((read(wmadecfdread, &cmd_data, sizeof(cmd_data)))<0) {
        printf("%d [WMA Dec Component] - failure to get data from the audio manager\n", __LINE__);
		goto EXIT;
    }
#endif    
    /*dspDefinition.streamId = cmd_data.streamID; */
    if(dspDefinition.dasfMode)
        printf("***************StreamId=%d******************\n", (int)dspDefinition.streamId);

    error = OMX_SetConfig(pHandle,index,&dspDefinition);
	if (error != OMX_ErrorNone) {
		printf("Error in SetConfig\n");
		goto EXIT;
	}

    error = OMX_GetExtensionIndex(pHandle, "OMX.TI.index.config.wmadec.datapath",&index);
	if (error != OMX_ErrorNone) {
		printf("Error getting extension index\n");
		goto EXIT;
	}

	error = OMX_SetConfig (pHandle, index, dataPath);
    if(error != OMX_ErrorNone) {
        error = OMX_ErrorBadParameter;
        APP_DPRINT("%d :: WmaDecTest.c :: Error from OMX_SetConfig() function\n",__LINE__);
        goto EXIT;
    }    

EXIT:
    if(pHeaderInfo != NULL)
    {
        newfree(pHeaderInfo);
    }

    printf("Exiting fill_init_params()\n");
}

OMX_ERRORTYPE StopComponent(OMX_HANDLETYPE *pHandle)
{
    OMX_ERRORTYPE error = OMX_ErrorNone;

#ifdef OMX_GETTIME
    GT_START();    
    error = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateIdle, NULL);
    GT_END("Call to SendCommand <OMX_StateIdle>");
#else
    error = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateIdle, NULL);
#endif
	if(error != OMX_ErrorNone) {
                    fprintf (stderr,"\nError from SendCommand-Idle(Stop) State function!!!!!!!!\n");
                    goto EXIT;
		}
    error =	WaitForState(pHandle, OMX_StateIdle);
    if(error != OMX_ErrorNone) {
    	fprintf(stderr, "\nError:  WaitForState reports an error %X!!!!!!!\n", error);
    	goto EXIT;
	}
EXIT:
    return error;
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
    int q;
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


/*=================================================================

							Freeing All allocated resources
							
==================================================================*/

int FreeAllResources( OMX_HANDLETYPE pHandle,
			                OMX_BUFFERHEADERTYPE* pBufferIn,
			                OMX_BUFFERHEADERTYPE* pBufferOut,
			                int NIB, int NOB,
			                FILE* fIn, FILE* fOut)
{
/*	show_meminfo(&mem1); */
	printf("%d::Freeing all resources by state invalid \n",__LINE__);
	OMX_ERRORTYPE eError1 = OMX_ErrorNone;
	OMX_U16 i; 
	for(i=0; i < NIB; i++) {
		   printf("%d :: APP: About to free pInputBufferHeader[%d]\n",__LINE__, i);
		   eError1 = OMX_FreeBuffer(pHandle, OMX_DirInput, pBufferIn+i);

	}


	for(i=0; i < NOB; i++) {
		   printf("%d :: APP: About to free pOutputBufferHeader[%d]\n",__LINE__, i);
		   eError1 = OMX_FreeBuffer(pHandle, OMX_DirOutput, pBufferOut+i);
	}

	/*i value is fixed by the number calls to malloc in the App */
	for(i=0; i<6;i++)  
	{
		if (ArrayOfPointers[i] != NULL){
			 printf("%d :: APP: About to free ArrayOfPointers[%d]\n",__LINE__, i);
			newfree(ArrayOfPointers[i]);
		}
	}
	

	    eError1 = close (IpBuf_Pipe[0]);
	    eError1 = close (IpBuf_Pipe[1]);
	    eError1 = close (OpBuf_Pipe[0]);
	    eError1 = close (OpBuf_Pipe[1]);
	if(fOut != NULL)	/* Could have been closed  previously */
	{
		fclose(fOut);
		fOut=NULL;
	}

	if(fIn != NULL)
	{	fclose(fIn);
		fIn=NULL;
	}
/*	show_meminfo(&mem2); */

	TIOMX_FreeHandle(pHandle);

/*	show_meminfo(&mem2); */
/*	show_memcomp(&mem1,&mem2);*/

	return eError1;
}

/*=================================================================
							Freeing the resources with USE_BUFFER define				
==================================================================*/
#ifdef USE_BUFFER

int freeAllUseResources(OMX_HANDLETYPE pHandle,
							OMX_U8* UseInpBuf[],
							OMX_U8* UseOutBuf[], 			
							int NIB,int NOB,
							FILE* fIn, FILE* fOut)
{
		OMX_ERRORTYPE eError1 = OMX_ErrorNone;
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
				newfree(ArrayOfPointers[i]);
		}
			eError1 = close (IpBuf_Pipe[0]);
			eError1 = close (IpBuf_Pipe[1]);
			eError1 = close (OpBuf_Pipe[0]);
			eError1 = close (OpBuf_Pipe[1]);

		if (fOut != NULL)	/* Could have been closed  previously */ */
		{
			fclose(fOut);
			fOut=NULL;
		}
		if (fIn != NULL)
		{	fclose(fIn);
			fIn=NULL;
		}
		OMX_FreeHandle(pHandle);

		return eError1;
}

#endif
/* ================================================================================= */
/**
* @fn unParse_Header
* To match Android OMX Component, wee need to extract the info from the rca pattern
* to build the config buffer.
*/
/* ================================================================================ */
int unParse_Header (OMX_U8* pBuffer, FILE *fIn, int * payload){

  OMX_U8* tempBuffer= malloc(75);
  memset(pBuffer,0x00,75);
  memset(tempBuffer,0x00,75);
  fread(tempBuffer,75,1,fIn);
  tempBuffer+=42;
  memcpy(pBuffer,tempBuffer,sizeof(OMX_U16));
  tempBuffer+=2;
  memcpy(pBuffer+2,tempBuffer,sizeof(OMX_U16));
  tempBuffer+=2;
  memcpy(pBuffer+4,tempBuffer,sizeof(OMX_U32));
  tempBuffer+=4;
  memcpy(pBuffer+8,tempBuffer,sizeof(OMX_U32));
  tempBuffer+=4;
  memcpy(pBuffer+12,tempBuffer,sizeof(OMX_U16));
  tempBuffer+=2;
  memcpy(pBuffer+14,tempBuffer,sizeof(OMX_U16));
  tempBuffer+=8;
  memcpy(pBuffer+22,tempBuffer,sizeof(OMX_U16));
  tempBuffer += 7;
  *payload = *((OMX_U16*)tempBuffer);
  return 28;
}
