
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
/*
 *    LIBRARY INCLUDE
 */

#ifdef UNDER_CE
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <sys/select.h>
#include <pthread.h>
#include <linux/vt.h>
#include <signal.h>
#include <sys/stat.h>
#include <linux/soundcard.h>
#endif

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#include <OMX_Index.h> 
#include <OMX_Types.h>
#include <OMX_Core.h>
#include <OMX_Component.h>
#include <OMX_Audio.h>
#include <TIDspOmx.h>
#include <stdio.h>
#ifdef DSP_RENDERING_ON
#include <AudioManagerAPI.h>
#endif
#ifdef OMX_GETTIME
#include <OMX_Common_Utils.h>
#include <OMX_GetTime.h>
#endif
/*
 *     M A C R O S
 */

#undef APP_DEBUG  
#define APP_INFO
#define APP_ERROR
#define DASF
#define USE_BUFFER
#undef AACENC_DEBUGMEM 

/*#define GT_PERFM  *//*Defines the Performance and measurements mode*/
/*#undef GT_PERFM Defines the Performance and measurements mode*/

#ifdef  APP_INFO
        #define APP_IPRINT(...)    fprintf(stderr,__VA_ARGS__)			/* Information prints */
#else
        #define APP_IPRINT(...)
#endif


#ifdef  APP_ERROR
        #define APP_EPRINT(...)    fprintf(stderr,__VA_ARGS__)			/* errors & warnings prints */
#else
        #define APP_EPRINT(...)
#endif


#ifdef  APP_DEBUG
        #define APP_DPRINT(...)    fprintf(stderr,__VA_ARGS__)			/* Debug prints */
#else
        #define APP_DPRINT(...)
#endif

#ifdef OMX_GETTIME
  OMX_ERRORTYPE eError = OMX_ErrorNone;
  int GT_FlagE = 0;  /* Fill Buffer 1 = First Buffer,  0 = Not First Buffer  */
  int GT_FlagF = 0;  /*Empty Buffer  1 = First Buffer,  0 = Not First Buffer  */
  static OMX_NODE* pListHead = NULL;
#endif


#ifdef APP_MEMCHECK
    #define APP_MEMPRINT(...)    fprintf(stderr,__VA_ARGS__)
#else
    #define APP_MEMPRINT(...)
#endif

#define MONO 			 1
#define STEREO 			 2
#define SLEEP_TIME  	 10
#define INPUT_PORT  	 0
#define OUTPUT_PORT 	 1
#define MAX_NUM_OF_BUFS  5
#define INPUT_AACENC_BUFFER_SIZE 8192  
#define FIFO1 "/dev/fifo.1"
#define FIFO2 "/dev/fifo.2"

#define Min_8Kbps 		 8000
#define Max_576Kbps 	 576000
#define Min_volume 		 0										/* Minimum volume 					*/
#define Act_volume 		 50										/* Current volume 					*/
#define Max_volume 		 100

#define Max_48Kbps		 48000
#define Max_64Kbps		 64000

#define ObjectTypeLC	 2
#define ObjectTypeHE	 5
#define ObjectTypeHE2	 29	

#define BITS16			 16										/* unmasking  command line parameter */
#define Upto48kHz		 48000

#define newmalloc(x) mymalloc((x),(&ListHeader))				/* new prototype of malloc function 	*/		
#define newfree(z) myfree((z),(&ListHeader))					/* new prototype of free function 		*/


#undef  WAITFORRESOURCES

#ifdef AACENC_DEBUGMEM
void *arr[500]; 
int lines[500]; 
int bytes[500]; 
char file[500][50];
int r=0;
#endif

/*
 *    TYPE DEFINITIONS
 */

/* Structure for Linked List */
typedef struct DataList ListMember;
struct DataList
{
	int ListCounter;											/* Instance Counter 					*/
	void* Struct_Ptr;											/* Pointer to new alocate data 		*/
	ListMember* NextListMember;									/* Pointer to next instance	 		*/
};

typedef enum COMPONENTS 
{
    COMP_1,
    COMP_2
}COMPONENTS;

/* Ouput file format */
typedef enum FILE_FORMAT 
{
    RAW = 0,
    ADIF,
	ADTS
}FILE_FORMAT;

/*Structure for Wait for state sync */
typedef struct
{
	OMX_BOOL WaitForStateFlag;									/* flag  				 */
	pthread_cond_t  cond; 										/* conditional mutex 	*/
	pthread_mutex_t Mymutex;									/* Mutex 			*/
}Mutex;


/*
 *    FUNTIONS DECLARATION
 */

OMX_ERRORTYPE AddMemberToList(void* ptr, ListMember** ListHeader);

OMX_ERRORTYPE FreeListMember(void* ptr, ListMember** ListHeader);

OMX_ERRORTYPE CleanList(ListMember** ListHeader);

void * mymalloc(int size, ListMember** ListHeader );

int myfree(void *dp, ListMember** ListHeader);

static OMX_ERRORTYPE FreeAllResources( OMX_HANDLETYPE pHandle,
			                OMX_BUFFERHEADERTYPE* pBufferIn,
			                OMX_BUFFERHEADERTYPE* pBufferOut,
			                int NIB, int NOB,
			                FILE* fIn, FILE* fOut, 
			                ListMember* ListHeader);

#ifdef USE_BUFFER
static OMX_ERRORTYPE  freeAllUseResources(OMX_HANDLETYPE pHandle,
						  OMX_U8* UseInpBuf[],
						  OMX_U8* UseOutBuf[], 			
						  int NIB, int NOB,
						  FILE* fIn, FILE* fOut,
						  ListMember* ListHeader );

#endif		

OMX_BOOL ValidateParameters(OMX_U32 SampleRate, OMX_U32 numChannels, OMX_U32 BitRate, OMX_U32 ObjectType);

OMX_U32 CalculateOutputBufferSize (OMX_U32 SampleFrec, OMX_U32 BitRate, OMX_U16 FramesPerOutBuf );



/*
 *   GLOBAL VARIBLES
 */

int IpBuf_Pipe[2];
int OpBuf_Pipe[2];
int Event_Pipe[2];

int channel = 0;
int ObjectType=0;

int preempted = 0;
int firstbuffer = 1;

#ifdef DSP_RENDERING_ON
AM_COMMANDDATATYPE cmd_data;
#endif
OMX_STRING strAacEncoder = "OMX.TI.AAC.encode";
	

/* flag for Invalid State condition*/
static OMX_BOOL bInvalidState;

/* New instance of Mutex Structure */
Mutex WaitForStateMutex={OMX_TRUE,PTHREAD_COND_INITIALIZER,PTHREAD_MUTEX_INITIALIZER}; 	 

/* Flags for mutex control */
OMX_BOOL bWaiting = OMX_FALSE;
OMX_STATETYPE waiting_state=OMX_StateInvalid;

/* Flag to stop component */
OMX_BOOL bPlayCompleted = OMX_FALSE;


/*
 *   FUNCTIONS DEFINITION
 */


/*-------------------------------------------------------------------*/
/**
  * maxint()  Returns the biggest from two number
  *
  *  @param a					First Integer number
  *  @param  b					Second Integer number
  * 
  * @retval 						The biggest number
  *              
  **/
/*-------------------------------------------------------------------*/
int maxint(int a, int b)
{
   return (a>b) ? a : b;
}


/*-------------------------------------------------------------------*/
/**
  * WaitForState()  Waits for signal state transition if  the transition has not ocurred
  *
  *  @param pHandle					Component pointer
  *		      DesiredState				State to wait
  * 
  * @retval OMX_ErrorNone   			Success on Transition
  *              OMX_StateInvalid		 	Wrong transition
  **/
/*-------------------------------------------------------------------*/

static OMX_ERRORTYPE WaitForState(OMX_HANDLETYPE pHandle,
                                OMX_STATETYPE DesiredState)
{
     OMX_STATETYPE CurState = OMX_StateInvalid; 
     OMX_ERRORTYPE eError   = OMX_ErrorNone;

	 APP_DPRINT("%d: APP: waiting for %d \n",__LINE__,DesiredState);
     eError = OMX_GetState(pHandle, &CurState);
	 if(eError !=OMX_ErrorNone)
	 {
		APP_EPRINT("App: Error in GetState from WaitForState() \n" );
		goto EXIT;
	 }

   	 if (CurState != DesiredState)
   	 {
   		 bWaiting	   = OMX_TRUE;													/*	flag is enable since now we have to wait to the event */
   		 waiting_state = DesiredState;
		 APP_DPRINT("Now is waiting.... \n");
   		
   		 pthread_mutex_lock(&WaitForStateMutex.Mymutex);
   		 pthread_cond_wait(&WaitForStateMutex.cond,&WaitForStateMutex.Mymutex);		/*  Block on a Condition Variable"  */
   		 pthread_mutex_unlock( &WaitForStateMutex.Mymutex);
   	 }
   	 else if(CurState == DesiredState)
   	 {
		APP_DPRINT("...No need to wait \n");
   		 eError = OMX_ErrorNone;
   	 }


EXIT:
	return eError;
}


/*-------------------------------------------------------------------*/
/**
  * EventHandler()  Event Callback from Component. Method to notify when an event of
  *				   interest occurs within the component.
  *
  *  @param hComponent				The handle of the component that is calling this function.
  *			pAppData				additional event-specific data.
  *			eEvent					The event that the component is communicating
  *			nData1					The first integer event-specific parameter.
  *			nData2					The second integer event-specific parameter.
  * 
  * @retval OMX_ErrorNone   				Success, Event Gotten
  *              OMX_ErrorBadParameter		Error on parameters
  **/
/*-------------------------------------------------------------------*/

OMX_ERRORTYPE EventHandler(OMX_HANDLETYPE hComponent,
				  OMX_PTR pAppData,
                  OMX_EVENTTYPE eEvent, 
                  OMX_U32 nData1,
                  OMX_U32 nData2,
                  OMX_PTR pEventData)
{

   APP_DPRINT( "%d :: App: Entering EventHandler \n", __LINE__);
   OMX_STATETYPE state;
   OMX_ERRORTYPE eError = OMX_ErrorNone;

   OMX_U8 writeValue;  

#ifdef  APP_DEBUG
   int iComp = *((int *)(pAppData));
#endif

   eError = OMX_GetState (hComponent, &state);
   if(eError != OMX_ErrorNone) 
   {
       APP_DPRINT("%d :: App: Error returned from GetState\n",__LINE__);
	   goto EXIT;
   }

   APP_DPRINT( "%d :: App: Component eEvent = %d\n", __LINE__,eEvent);
   switch (eEvent) {
       case OMX_EventCmdComplete:

	   if( (waiting_state== nData2) && (nData1 == OMX_CommandStateSet) && (bWaiting) )		/* ensure that came from transition */
		   {
					bWaiting = OMX_FALSE;
					pthread_mutex_lock(&WaitForStateMutex.Mymutex);
					pthread_cond_signal(&WaitForStateMutex.cond);								/* Unblock a Specific Thread" */
					APP_DPRINT("App: Mutex signal sent \n"); 
					pthread_mutex_unlock( &WaitForStateMutex.Mymutex);							/*  unlock.  */
				
					APP_DPRINT ("%d :: App: Component State Changed To %d\n", __LINE__,state);
		   } 
   
		   APP_DPRINT(	"%d :: App: OMX_EventCmdComplete %d\n", __LINE__,eEvent);
           break;
       case OMX_EventError:
           if (nData1 != OMX_ErrorNone) 
		   {
               APP_DPRINT ("%d:: App: ErrorNotofication Came: \
                           Component Name : %d : Error Num %x \n",
                           __LINE__,iComp, (unsigned int)nData1);
           }
		   if (nData1 == OMX_ErrorInvalidState) {
		   		bInvalidState =OMX_TRUE;
		   }
		   else if(nData1 == OMX_ErrorResourcesPreempted) {
            preempted=1;
            writeValue = 0;  
            write(Event_Pipe[1], &writeValue, sizeof(OMX_U8));
	       }

	       else if (nData1 == OMX_ErrorResourcesLost) {  
	            bWaiting = 0;
	            pthread_mutex_lock(&WaitForStateMutex.Mymutex);
	            pthread_cond_signal(&WaitForStateMutex.cond);/*Sending Waking Up Signal*/
	            pthread_mutex_unlock(&WaitForStateMutex.Mymutex);
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
		   APP_DPRINT( "%d :: App: OMX_EventBufferFlag on port = %d\n", __LINE__, (int)nData1);
		   /* event for the input port to stop component  , since there is one for output port */
		   if(nData1 == INPUT_PORT && nData2 == OMX_BUFFERFLAG_EOS)  
		   { 	
		   		bPlayCompleted = OMX_TRUE;
			    APP_DPRINT( "%d :: App: Setting flag for playcompleted \n", __LINE__);
		   }

		   #ifdef WAITFORRESOURCES          
		   writeValue = 2;  
	       write(Event_Pipe[1], &writeValue, sizeof(OMX_U8));
		   #endif
		   
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


/*-------------------------------------------------------------------*/
/**
  * FillBufferDone()  Callback from component which returns an ouput buffer 
  *
  *  @param hComponent				The handle of the component that is calling this function.
  *			pBuffer					Returned output buffer
  *			ptr						A pointer to IL client-defined data
  *			
  * 
  * @retval  None  
  *
  **/
/*-------------------------------------------------------------------*/

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


/*-------------------------------------------------------------------*/
/**
  * EmptyBufferDone()  Callback from component which returns an input buffer 
  *
  *  @param hComponent				The handle of the component that is calling this function.
  *			pBuffer					Returned input buffer
  *			ptr						A pointer to IL client-defined data
  *			
  * 
  * @retval  None  
  *
  **/
/*-------------------------------------------------------------------*/

void EmptyBufferDone(OMX_HANDLETYPE hComponent, OMX_PTR ptr, OMX_BUFFERHEADERTYPE* pBuffer)
{
   APP_DPRINT("%d:: APP: Inside empty buffer done \n",__LINE__);

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



/*-------------------------------------------------------------------*/
/**
  * main()  Test App main function. Function called from the runtime startup routine after
  		   the runtime environment has been initialized
  *
  *  @param argv				Argument vector. Pointer to an array of string pointers passed to  main function
  *		      argc				Holds the number of arguments that are passed. 
  *									
  *			
  * 
  * @retval   error 			Return Value to OS 
  *
  **/
/*-------------------------------------------------------------------*/

int main(int argc, char* argv[])
{


    struct timeval tv;
    int retval,i =0;
    int frmCount = 0;
	int testcnt  = 1;
	int testcnt1 = 1;
    char fname[20] = "output";
	int nRead = 0;
	int done  = 0;
	int nIpBuffs = 0;
	int nOpBuffs = 0;
	int numofinbuff  = 0;
	int numofoutbuff = 0;
	int frmCnt = 1;
    int status,fdmax,jj,kk,k;
    int nFrameCount = 0;

    OMX_CALLBACKTYPE AacCaBa = {(void *)EventHandler,(void*) EmptyBufferDone,
                                                     (void*)FillBufferDone};
    OMX_HANDLETYPE *pHandle = NULL;
    OMX_ERRORTYPE  error = OMX_ErrorNone;
    OMX_U32 AppData = 100;
    OMX_PARAM_PORTDEFINITIONTYPE* pCompPrivateStruct =NULL;
    OMX_AUDIO_PARAM_AACPROFILETYPE *pAacParam =NULL;
	OMX_AUDIO_PARAM_PCMMODETYPE *iAacParam =NULL;
    OMX_COMPONENTTYPE *pComponent=NULL;
    OMX_STATETYPE state;
	TI_OMX_DSP_DEFINITION audioinfo;
	OMX_AUDIO_CONFIG_VOLUMETYPE* pCompPrivateStructGain = NULL;
    OMX_BUFFERHEADERTYPE* pInputBufferHeader[MAX_NUM_OF_BUFS];
    OMX_BUFFERHEADERTYPE* pOutputBufferHeader[MAX_NUM_OF_BUFS];
	OMX_INDEXTYPE index;
	TI_OMX_STREAM_INFO *streaminfo =NULL;
	OMX_U32 ArrValidFrecLC[9]={8000,11025,16000,22050,32000,44100,48000,88200,96000};
	OMX_U16 NumOfFrecsLC=9;
	OMX_U32 ArrValidFrecHE[6]={16000,22050,24000,32000,44100,48000};
	OMX_U16 NumOfFrecsHE=6;
	OMX_U16 FramesPerOutBuf  =12;							/* desired number of frames                                 */
	OMX_U32 OutputBufferSize =0;							/* Calculated size per Ouput buffer                      */
	bInvalidState=OMX_FALSE;								/* flag for invalid state transition                          */
	OMX_BOOL isValidCombination ;							/* flag for  bit rate and sample rate combination  */
    TI_OMX_DATAPATH dataPath;
	ListMember* ListHeader = NULL;							/* Linked list Header							 */
	FILE* fIn = NULL;										/* input File pointer 							 */
    FILE* fOut= NULL;										/* ouput File pointer 							 */
#ifdef DSP_RENDERING_ON        
int Aacenc_fdwrite;
int Aacenc_fdread;
#endif

	/*----------------------------------------------
 	First allocated structure
 	----------------------------------------------*/
	
	streaminfo = newmalloc(sizeof(TI_OMX_STREAM_INFO));
	if(NULL == streaminfo) 
	{
        APP_EPRINT("%d :: App: Malloc Failed\n",__LINE__);
        goto EXIT;
	}

	OMX_U32	streamId;
#ifdef USE_BUFFER
    OMX_U8* pInputBuffer[5];
    OMX_U8* pOutputBuffer[5];
#endif

    fd_set rfds;
	
	APP_IPRINT("------------------------------------------------------------\n");
    APP_IPRINT("This is Main Thread In MPEG AAC ENCODER Test Application:\n");
    APP_IPRINT("Test Core 1.5 - " __DATE__ ":" __TIME__ "\n");
    APP_IPRINT("------------------------------------------------------------\n");
   
#ifdef OMX_GETTIME
    APP_IPRINT("Line %d\n",__LINE__);
      GTeError = OMX_ListCreate(&pListHead);
        APP_IPRINT("Line %d\n",__LINE__);
      APP_IPRINT("eError = %d\n",GTeError);
      GT_START();
  APP_IPRINT("Line %d\n",__LINE__);
#endif

#ifdef USE_BUFFER
	APP_IPRINT("                     Use Buffer Enabled\n");
	APP_IPRINT("------------------------------------------------------------\n");
#endif

	/* check to see the number of parameters from the command line */
    if(  (argc < 14) || (argc > 15)  ) 
	{
        APP_EPRINT("Usage:   test: [INFILE] [OUTFILE] [MONO/STEREO] [TESTCASE] [DASF/FILE] [SFREQ] [BITRATE] [OBJECTTYPE] [FRAMES] [IP_BUF] [OUT_BUF] [BITRATE] [FILEFORMAT] [ROBUSTNESS]\n");
        APP_EPRINT("Example: AacEncTest_common ip.pcm op.aac MONO 5 0 44100 128000 2 0 1 1 0 1 <ROBUST>\n");
        goto EXIT;
    }

    /* check to see that the input file exists */
    struct stat sb = {0};
    status = stat(argv[1], &sb);
    if( status != 0 ) 
	{
        APP_EPRINT("%d :: App: Cannot find file %s. (%u)\n",__LINE__, argv[1], errno);
        goto EXIT;
    }

	/* check to see the test case number */
    switch (atoi(argv[4])) 
	{
		case 1:
			APP_IPRINT ("---------------------------------------------\n");
			APP_IPRINT ("Testing Simple Record till Predefined frames \n");
			APP_IPRINT ("---------------------------------------------\n");
			break;
		case 2:
			APP_IPRINT ("---------------------------------------------\n");
			APP_IPRINT ("Testing Stop After Record \n");
			APP_IPRINT ("---------------------------------------------\n");
			break;
		case 3:
			APP_IPRINT ("---------------------------------------------\n");
			APP_IPRINT ("Testing PAUSE & RESUME Command\n");
			APP_IPRINT ("---------------------------------------------\n");
			break;
		case 4:
			APP_IPRINT ("-------------------------------------------------\n");
			APP_IPRINT ("Testing Repeated PLAY without Deleting Component\n");
			APP_IPRINT ("-------------------------------------------------\n");
			strcat (fname,"_tc5.aac");
			if( (argc == 15) ) 
			{
				if(!strcmp("ROBUST",argv[14])) 
				{
					APP_IPRINT("%d :: APP: AAC: 100 Iterations - ROBUSTNESS Test mode\n",__LINE__);
					testcnt = 100;
				}
			}
			else
				testcnt = 20;
			break;
		case 5:
			APP_IPRINT ("------------------------------------------------\n");
			APP_IPRINT ("Testing Repeated PLAY with Deleting Component\n");
			APP_IPRINT ("------------------------------------------------\n");
			strcat (fname,"_tc6.aac");
			if( (argc == 15)) 
			{
				if(!strcmp("ROBUST",argv[14])) 
				{
					APP_IPRINT("%d :: APP: AAC: 100 Iterations - ROBUSTNESS Test mode\n",__LINE__);
					testcnt1 = 100;
				}
			}
			else
				testcnt1 = 20;
			break;
		case 6:
			APP_IPRINT ("-------------------------------------\n");
			APP_IPRINT ("Testing Stop and Play \n");
			APP_IPRINT ("-------------------------------------\n");
			strcat(fname,"_tc7.aac");
			testcnt = 2;
			break;
		case 7:
			APP_IPRINT ("-------------------------------------\n");
			APP_IPRINT ("VOLUME \n");
			APP_IPRINT ("-------------------------------------\n");
			break;
    }

	/*Opening INPUT FILE */
	fIn = fopen(argv[1], "r");
    if(fIn == NULL) 
	{
        APP_EPRINT("APP: Error:  failed to open the file %s for readonly access\n", argv[1]);
        goto EXIT;
    }

	/*Opening  OUTPUT FILE */
    fOut = fopen(argv[2], "w");
    if(fOut == NULL) 
	{
        APP_EPRINT("APP: Error:  failed to create the output file %s\n", argv[2]);
        goto EXIT;
    }

	APP_DPRINT("%d :: APP: AAC Encoding Test --- first create file handle\n",__LINE__);
	APP_DPRINT("%d :: APP: AAC Encoding Test --- fIn = [%p]\n", __LINE__, fIn);
	APP_DPRINT("%d :: APP: AAC Encoding Test --- fOut = [%p]\n",__LINE__, fOut);

	/* check to see the STEREO/MONO mode */
    if(!strcmp("MONO",argv[3])) 
	{
        APP_IPRINT("%d :: APP: AAC: Encoding in Mono Mode\n",__LINE__);
        channel = MONO;
    } 
	else if(!strcmp("STEREO",argv[3])) 
	{
        APP_IPRINT("%d :: APP: AAC: Encoding in Stereo Mode\n",__LINE__);
        channel = STEREO;
    } 
	else 
	{
        APP_DPRINT("%d :: APP: Error: Invalid Mode Specifier, Check argument 3\n",__LINE__);
    }


/*----------------------------------------------
 Main Loop for Deleting component test
 ----------------------------------------------*/
    jj=0;
    APP_IPRINT("%d :: APP: AAC ENC Test --- will call [%d] time for encoder\n",__LINE__, jj+1);
    for(jj=0; jj<testcnt1; jj++) 
	{

		if ( atoi(argv[4])== 5)
		{
			APP_IPRINT ("***************************************\n");
			APP_IPRINT ("%d :: TC-5 counter = %d\n",__LINE__,jj);
			APP_IPRINT ("***************************************\n");
		}

#ifdef DSP_RENDERING_ON        
		if((Aacenc_fdwrite=open(FIFO1,O_WRONLY))<0) {
	        APP_EPRINT("%d :: APP: - failure to open WRITE pipe\n",__LINE__);
	    }
	    else {
	        APP_DPRINT("%d :: APP: - opened WRITE pipe\n",__LINE__);
	    }

	    if((Aacenc_fdread=open(FIFO2,O_RDONLY))<0) {
	        APP_EPRINT("%d :: APP: - failure to open READ pipe\n",__LINE__);
	        goto EXIT;
	    }
	    else {
	        APP_DPRINT("%d :: APP: - opened READ pipe\n",__LINE__);
	    }
#endif
	    /* Create a pipe used to queue data from the callback. */
        retval = pipe(IpBuf_Pipe);
        if( retval != 0) 
		{
            APP_EPRINT("App: Error: Fill Data Pipe failed to open\n");
            goto EXIT;
		}

        retval = pipe(OpBuf_Pipe);
        if( retval != 0) 
		{
            APP_EPRINT( "App: Error: Empty Data Pipe failed to open\n");
            goto EXIT;
		}

		retval = pipe(Event_Pipe);
    	if( retval != 0) {
		    APP_DPRINT( "Error:Empty Data Pipe failed to open\n");
		    goto EXIT;
	    }


		/* save off the "max" of the handles for the selct statement */
        fdmax = maxint(IpBuf_Pipe[0], OpBuf_Pipe[0]);
		fdmax = maxint(fdmax,Event_Pipe[0]);
	
        error = TIOMX_Init();
        if(error != OMX_ErrorNone) 
		{
            APP_EPRINT("%d :: APP: Error returned by OMX_Init()\n",__LINE__);
            goto EXIT;
		}

		if(fIn == NULL) 
		{
			fIn = fopen(argv[1], "r");
			if( fIn == NULL ) 
			{
				APP_EPRINT("App: Error:  failed to open the file %s for readonly access\n", argv[1]);
				goto EXIT;
			}
		}
		if(fOut == NULL) 
		{
			fOut = fopen(fname, "w");
			if( fOut == NULL ) 
			{
				APP_EPRINT("App: Error:  failed to create the output file %s\n", argv[2]);
				goto EXIT;
			}
		}

		/*Component handler */
		pHandle = newmalloc(sizeof(OMX_HANDLETYPE));
        if(NULL == pHandle) 
		{
            APP_EPRINT("%d :: App: Malloc Failed\n",__LINE__);
            goto EXIT;
		}
        APP_IPRINT("%d :: App: pHandle = %p\n",__LINE__,pHandle);
#ifdef OMX_GETTIME
	    GT_START();  
	    error = OMX_GetHandle(pHandle,strAacEncoder,&AppData, &AacCaBa);		
	    GT_END("Call to GetHandle");
#else
	    error = TIOMX_GetHandle(pHandle,strAacEncoder,&AppData, &AacCaBa);
#endif
		if( (error != OMX_ErrorNone) || (*pHandle == NULL) ) 
		{
            APP_EPRINT("%d :: App: Error in Get Handle function %d \n",__LINE__,error);
			goto EXIT;
		}
		APP_DPRINT("%d :: APP: GetHandle Done..........\n",__LINE__);


		/* Setting input parameters */
        audioinfo.aacencHeaderInfo = newmalloc(sizeof(AACENC_HeadInfo));
        if (audioinfo.aacencHeaderInfo == NULL) 
		{
            APP_EPRINT("%d :: APP: Could not allocate audioinfo.aacencHeaderInfo\n",__LINE__);
            goto EXIT;
		}

		/* Check to see the F2F or DASF mode */
	    audioinfo.dasfMode = atoi(argv[5]);
	    if(audioinfo.dasfMode == 1)
		{
		    APP_IPRINT("%d :: APP: AAC Encoding in DASF MODE\n",__LINE__);
		}
	    else if(audioinfo.dasfMode == 0)
		{
		    APP_IPRINT("%d :: APP: AAC Encoding in FILE MODE\n",__LINE__);
		}
	    else 
		{
		    APP_EPRINT("APP: Error: Enter proper DASF mode\n");
		    APP_EPRINT("DASF:1\n");
		    APP_EPRINT("NON DASF:0\n");
		    goto EXIT;
		}

		/* Setting No. of Input and Output Buffers for the Component */
	    numofinbuff = atoi(argv[10]);
		numofoutbuff = atoi(argv[11]);

		/* Ensuring the propper value of input buffers for DASF mode : Should be 0 */
		if (audioinfo.dasfMode == 1)  /* DASF MODE */
		{
			if (numofinbuff != 0) 
			{
				APP_EPRINT ("%d :: App: WARNING: DASF-Mode should not use input buffers \n",__LINE__);
				APP_EPRINT ("%d :: App: WARNING: Changing the number of input buffers to 0 \n",__LINE__);
				numofinbuff = 0;
			}
			if(  numofoutbuff != 2)
			{
				APP_EPRINT ("%d :: App: WARNING: DASF-Mode should  use 2 Output buffers \n",__LINE__);
				APP_EPRINT ("%d :: App: WARNING: Changing the number of Output buffers to 2 \n",__LINE__);
				numofoutbuff = 2;
			}
		}
	    else if (audioinfo.dasfMode == 0) /* F2F MODE */
		{

			if( (numofinbuff < 0) || (numofinbuff > 4) ){
				APP_EPRINT ("%d :: App: ERROR: Input buffers value incorrect (0-4) \n",__LINE__);
				goto EXIT;
			}

			if( (numofoutbuff< 0) || (numofoutbuff > 4) ){
				APP_EPRINT ("%d :: App: ERROR: Output buffers value incorrect (0-4) \n",__LINE__);
				goto EXIT;
			}

			/* Ensuring the propper value of buffers for STEREO mode */
			if((channel == STEREO) && (numofoutbuff != 4))
			{
				APP_EPRINT ("%d :: App: WARNING: STEREO-Mode should use 4 output buffers \n",__LINE__);
				APP_EPRINT ("%d :: App: WARNING: Changing the number of output buffers to 4 \n",__LINE__);
				numofoutbuff = 4;
			}
	    }
		APP_DPRINT("\n%d :: App: numofinbuff = %ld \n",__LINE__, (long int)numofinbuff);
	    APP_DPRINT("\n%d :: App: numofoutbuff = %ld \n",__LINE__, (long int)numofoutbuff);

		pCompPrivateStruct = newmalloc (sizeof (OMX_PARAM_PORTDEFINITIONTYPE));
        if(NULL == pCompPrivateStruct) 
		{
           APP_DPRINT("%d :: APP: Malloc Failed\n",__LINE__);
           error = OMX_ErrorInsufficientResources;
           goto EXIT;
		}
 
	    /*Calculating an optimun size of Ouput  buffer according to number of frames*/
		OutputBufferSize = CalculateOutputBufferSize(atoi(argv[6]),atoi(argv[7]),FramesPerOutBuf);   /*Sample frec,  Bit rate , frames*/

		/* getting index for  framesPerOutBuf */
		error = OMX_GetExtensionIndex(*pHandle, "OMX.TI.index.config.aacencframesPerOutBuf",&index);
	    if (error != OMX_ErrorNone) 
		{
		    APP_DPRINT("%d :: APP: Error getting extension index\n",__LINE__);
		    goto EXIT;
		}
		/* Setting the Number of Frames per ouput buffer to component */
		error = OMX_SetConfig (*pHandle, index, &FramesPerOutBuf);  	
        if(error != OMX_ErrorNone)
		{
            error = OMX_ErrorBadParameter;
            APP_DPRINT("%d :: APP: Error from OMX_SetConfig() function\n",__LINE__);
            goto EXIT;
		}		
		
		/* Setting INPUT port */
		APP_DPRINT("%d :: APP: Setting input port config\n",__LINE__);
		pCompPrivateStruct->nSize                             = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
		pCompPrivateStruct->nVersion.s.nVersionMajor          = 0xF1;
		pCompPrivateStruct->nVersion.s.nVersionMinor          = 0xF2;
		pCompPrivateStruct->nPortIndex                        = INPUT_PORT;
		pCompPrivateStruct->eDir                              = OMX_DirInput;
		pCompPrivateStruct->nBufferCountActual                = numofinbuff;
		pCompPrivateStruct->nBufferCountMin                   = numofinbuff;
		pCompPrivateStruct->nBufferSize                       = INPUT_AACENC_BUFFER_SIZE;
		pCompPrivateStruct->bEnabled                          = OMX_TRUE;
        pCompPrivateStruct->bPopulated                        = OMX_FALSE;
		pCompPrivateStruct->format.audio.eEncoding            = OMX_AUDIO_CodingAAC;
#ifdef OMX_GETTIME
	GT_START();
		error = OMX_SetParameter (*pHandle,OMX_IndexParamPortDefinition, pCompPrivateStruct);
	GT_END("Set Parameter Test-SetParameter");
#else
		error = OMX_SetParameter (*pHandle,OMX_IndexParamPortDefinition, pCompPrivateStruct);
#endif    
		if(error != OMX_ErrorNone) 
		{
			error = OMX_ErrorBadParameter;
			APP_DPRINT("%d :: APP: OMX_ErrorBadParameter\n",__LINE__);
			goto EXIT;
		}

		/* Setting OUPUT port */
		APP_DPRINT("%d :: APP: Setting output port config\n",__LINE__);
		pCompPrivateStruct->nSize                             = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
		pCompPrivateStruct->nVersion.s.nVersionMajor          = 0xF1;
		pCompPrivateStruct->nVersion.s.nVersionMinor          = 0xF2;
		pCompPrivateStruct->nPortIndex                        = OUTPUT_PORT;
		pCompPrivateStruct->eDir                              = OMX_DirOutput;
		pCompPrivateStruct->nBufferCountActual                = numofoutbuff;
		pCompPrivateStruct->nBufferCountMin                   = numofoutbuff;
		pCompPrivateStruct->nBufferSize                       = OutputBufferSize;
		pCompPrivateStruct->bEnabled                          = OMX_TRUE;
        pCompPrivateStruct->bPopulated                        = OMX_FALSE;
		pCompPrivateStruct->format.audio.eEncoding            = OMX_AUDIO_CodingAAC;
#ifdef OMX_GETTIME
	GT_START();
	    error = OMX_SetParameter (*pHandle,OMX_IndexParamPortDefinition, pCompPrivateStruct);
	GT_END("Set Parameter Test-SetParameter");
#else
	    error = OMX_SetParameter (*pHandle,OMX_IndexParamPortDefinition, pCompPrivateStruct);
#endif  	
		if(error != OMX_ErrorNone) 
		{
			error = OMX_ErrorBadParameter;
			APP_DPRINT("%d :: APP: OMX_ErrorBadParameter\n",__LINE__);
			goto EXIT;
		}

		/*Ensuring  Valid Bits Per sample value  */
		ObjectType = atoi(argv[8]);

		if( ( ObjectType != ObjectTypeLC) && (ObjectType != ObjectTypeHE) && (ObjectType != ObjectTypeHE2) )
		{
			APP_EPRINT("%d :: APP: Incorrect Value for Object Type \n",__LINE__);
			goto EXIT;
		}

		iAacParam = newmalloc (sizeof (OMX_AUDIO_PARAM_PCMMODETYPE));
		if(NULL == iAacParam) 
		{
           APP_DPRINT("%d :: APP: Malloc Failed\n",__LINE__);
           error = OMX_ErrorInsufficientResources;
           goto EXIT;
		}
		/* Setting PCM params */
		iAacParam->nSize 					= sizeof (OMX_AUDIO_PARAM_PCMMODETYPE);
		iAacParam->nVersion.s.nVersionMajor = 0xF1;
		iAacParam->nVersion.s.nVersionMinor = 0xF2;
		iAacParam->nPortIndex 				= INPUT_PORT;
		iAacParam->nBitPerSample 			= BITS16;   /* BitsPerSample; */
#ifdef OMX_GETTIME
	GT_START();
		error = OMX_SetParameter (*pHandle, OMX_IndexParamAudioPcm, iAacParam);
	GT_END("Set Parameter Test-SetParameter");
#else
		error = OMX_SetParameter (*pHandle, OMX_IndexParamAudioPcm, iAacParam);
#endif    
		if(error != OMX_ErrorNone) 
		{
			error = OMX_ErrorBadParameter;
			APP_DPRINT("%d :: APP: OMX_ErrorBadParameter\n",__LINE__);
			goto EXIT;
		}

		/*Ensuring a valid Sample rate */

		if((atoi(argv[8]) == ObjectTypeHE) || (atoi(argv[8]) == ObjectTypeHE2)){
			for (i=0; i<NumOfFrecsHE; i++)
			{
				if(  atoi(argv[6]) == ArrValidFrecHE[i] )
					break;
				else if (i == (NumOfFrecsHE-1) )
				{
					APP_EPRINT("%d :: APP: Incorrect Value for Sample Rate for AAC-HE -- AAC-HEv2 \n",__LINE__);
					goto EXIT;	
				}
			}
		}
		else{			/* atoi(argv[8]) == ObjectTypeLC */
			for (i=0; i<NumOfFrecsLC; i++)
			{
				if(  atoi(argv[6]) == ArrValidFrecLC[i] )
					break;
				else if (i == (NumOfFrecsLC-1) )
				{
					APP_EPRINT("%d :: APP: Incorrect Value for Sample Rate for AAC-LC\n",__LINE__);
					goto EXIT;	
				}
			}
		}

		/*Ensuring a valid Bit Rate */
		if(  (atoi(argv[7]) < Min_8Kbps) || (atoi(argv[7]) > Max_576Kbps) )
		{
			APP_EPRINT("%d :: APP: Incorrect Value for Bit Rate \n",__LINE__);
			goto EXIT;
		}
		
		/*Ensuring a valid parameters combination */		/*NOTE: block should be moved up and function improved */
		isValidCombination = ValidateParameters( atoi(argv[6]), channel, atoi(argv[7]), atoi(argv[8]) );
		APP_IPRINT("%d :: APP: Parameters Combination %d \n",__LINE__,isValidCombination);
		if (!isValidCombination)
		{
			APP_EPRINT("%d :: App: Error: Invalid Samplerate, Bitrate or Channels parameters Combination \n",__LINE__);
			goto EXIT;
		}
		else if ((isValidCombination) &&( atoi(argv[6]) > Upto48kHz))			/*parameters out of combination table */
		{
			APP_EPRINT("%d :: App: Warning: Combination of Samplerate and Bitrate parameters has not been validated\n",__LINE__);
		}

		/*Ensuring Valid use of HE and HEv2  */

		if ((atoi(argv[8]) == ObjectTypeHE) && ((atoi(argv[7])) > Max_48Kbps))
		{
			APP_EPRINT("%d :: App: Error: HE AAC Type supports a Maximum of 48 Kbps BitRate \n",__LINE__);
			goto EXIT;	
		}
		
		if ((atoi(argv[8]) == ObjectTypeHE2) && ((atoi(argv[7])) > Max_64Kbps))
		{
			APP_EPRINT("%d :: App: Error: HEv2 AAC Type supports a Maximum of 128 Kbps BitRate \n",__LINE__);
			goto EXIT;	
		}
	
		pAacParam = newmalloc (sizeof (OMX_AUDIO_PARAM_AACPROFILETYPE));
		if(NULL == pAacParam) 
		{
           APP_EPRINT("%d :: APP: Malloc Failed\n",__LINE__);
           error = OMX_ErrorInsufficientResources;
           goto EXIT;
		}
		/* Setting AAC params */
	    pAacParam->nSize 					= sizeof (OMX_AUDIO_PARAM_AACPROFILETYPE);
		pAacParam->nVersion.s.nVersionMajor = 0xF1;
		pAacParam->nVersion.s.nVersionMinor = 0xF2;
		pAacParam->nPortIndex 				= OUTPUT_PORT;
		pAacParam->nChannels 				= channel;
		pAacParam->nBitRate					= atoi(argv[7]);
		pAacParam->nSampleRate 				= atoi(argv[6]);
		pAacParam->nAudioBandWidth 			= 0;
		pAacParam->nFrameLength 			= 0;
		pAacParam->nAACtools 				= 0x0000000C;
		pAacParam->nAACERtools 				= 0x00000000;

		if (atoi(argv[8]) == ObjectTypeLC)
			pAacParam->eAACProfile = OMX_AUDIO_AACObjectLC;
		else if (atoi(argv[8]) == ObjectTypeHE)
			pAacParam->eAACProfile = OMX_AUDIO_AACObjectHE;
		else if (atoi(argv[8]) == ObjectTypeHE2)
			pAacParam->eAACProfile = OMX_AUDIO_AACObjectHE_PS;

		if (0 == atoi(argv[13])){
            pAacParam->eAACStreamFormat = OMX_AUDIO_AACStreamFormatRAW;
		}
        else if (1 == atoi(argv[13])) {
			pAacParam->eAACStreamFormat = OMX_AUDIO_AACStreamFormatADIF;
		}
		else if (2 == atoi(argv[13])) {
			pAacParam->eAACStreamFormat = OMX_AUDIO_AACStreamFormatMP4ADTS;
		}
		if(channel == STEREO) {
			pAacParam->eChannelMode 	= OMX_AUDIO_ChannelModeStereo;
		}
		else if(channel == MONO) {
			pAacParam->eChannelMode 	= OMX_AUDIO_ChannelModeMono;
		}
#ifdef OMX_GETTIME
	GT_START();
		error = OMX_SetParameter (*pHandle, OMX_IndexParamAudioAac, pAacParam);
	GT_END("Set Parameter Test-SetParameter");
#else
		error = OMX_SetParameter (*pHandle, OMX_IndexParamAudioAac, pAacParam);
#endif    
		if(error != OMX_ErrorNone) 
		{
			error = OMX_ErrorBadParameter;
			APP_DPRINT("%d :: APP: OMX_ErrorBadParameter\n",__LINE__);
			goto EXIT;
		}

		/*Setting the Bit rate mode parameter */
		audioinfo.aacencHeaderInfo->bitratemode = atoi(argv[12]);

		/* setting for stream gain */
		pCompPrivateStructGain = newmalloc (sizeof(OMX_AUDIO_CONFIG_VOLUMETYPE));
		if(pCompPrivateStructGain == NULL) 
		{
			APP_EPRINT("%d :: App: Malloc Failed\n",__LINE__);
			goto EXIT;
		}
		/* default setting for gain */
		pCompPrivateStructGain->nSize 						= sizeof(OMX_AUDIO_CONFIG_VOLUMETYPE);
		pCompPrivateStructGain->nVersion.s.nVersionMajor	= 0xF1;
		pCompPrivateStructGain->nVersion.s.nVersionMinor	= 0xF2;
		pCompPrivateStructGain->nPortIndex					= OMX_DirOutput;
		pCompPrivateStructGain->bLinear						= OMX_FALSE;
		pCompPrivateStructGain->sVolume.nValue				= Act_volume;		/* actual volume */
		pCompPrivateStructGain->sVolume.nMin				= Min_volume;		/* min volume */
		pCompPrivateStructGain->sVolume.nMax				= Max_volume;		/* max volume */


		error = OMX_GetExtensionIndex(*pHandle, "OMX.TI.index.config.aacencHeaderInfo",&index);
	    if (error != OMX_ErrorNone) 
		{
		    APP_EPRINT("%d :: APP: Error getting extension index\n",__LINE__);
		    goto EXIT;
		}
#ifdef DSP_RENDERING_ON        
		cmd_data.hComponent = *pHandle;
	    cmd_data.AM_Cmd = AM_CommandIsInputStreamAvailable;
	    cmd_data.param1 = 0;

	    if((write(Aacenc_fdwrite, &cmd_data, sizeof(cmd_data)))<0) 
		{
	        APP_EPRINT("%d :: APP: failure to Send command to audio manager\n", __LINE__);
	    }
	    if((read(Aacenc_fdread, &cmd_data, sizeof(cmd_data)))<0) 
		{
	        APP_EPRINT("%d :: APP: failure to get data from the audio manager\n", __LINE__);
			goto EXIT;
	    }

	    audioinfo.streamId = cmd_data.streamID;
	    streamId = audioinfo.streamId;
#endif
		error = OMX_SetConfig (*pHandle, index, &audioinfo);
        if(error != OMX_ErrorNone) 
		{
            error = OMX_ErrorBadParameter;
            APP_DPRINT("%d :: APP: Error from OMX_SetConfig() function\n",__LINE__);
            goto EXIT;
		}

	    if (audioinfo.dasfMode) 
		{
#ifdef RTM_PATH    
	        dataPath = DATAPATH_APPLICATION_RTMIXER;
			APP_DPRINT("APP: datapath: %d \n",dataPath);
#endif

#ifdef ETEEDN_PATH
        	dataPath = DATAPATH_APPLICATION;
#endif        
    	}

	    error = OMX_GetExtensionIndex(*pHandle, "OMX.TI.index.config.aac.datapath",&index);
		if (error != OMX_ErrorNone) 
		{
			APP_EPRINT("%d :: APP: Error getting extension index\n",__LINE__);
			goto EXIT;
		}
		error = OMX_SetConfig (*pHandle, index, &dataPath);
	    if(error != OMX_ErrorNone) 
		{
	        error = OMX_ErrorBadParameter;
	        APP_EPRINT("%d :: APP: Error from OMX_SetConfig() function\n",__LINE__);
	        goto EXIT;
	    }


#ifndef USE_BUFFER
	    APP_DPRINT("%d :: APP: About to call OMX_AllocateBuffer\n",__LINE__);
	    for(i = 0; i < numofinbuff; i++) 
		{
		   /* allocate input buffer */
		   APP_DPRINT("%d :: APP: About to call OMX_AllocateBuffer for pInputBufferHeader[%d]\n",__LINE__, i);
		   error = OMX_AllocateBuffer(*pHandle, &pInputBufferHeader[i], 0, NULL, INPUT_AACENC_BUFFER_SIZE);
		   if(error != OMX_ErrorNone) 
		   {
			  APP_EPRINT("%d :: APP: Error returned by OMX_AllocateBuffer for pInputBufferHeader[%d]\n",__LINE__, i);
			  goto EXIT;
		   }
		}
        APP_DPRINT("\n%d :: APP: pCompPrivateStruct->nBufferSize --> %ld \n",__LINE__,
    												pCompPrivateStruct->nBufferSize);
	    for(i = 0; i < numofoutbuff; i++) 
		{
		   /* allocate output buffer */
		   APP_DPRINT("%d :: APP: About to call OMX_AllocateBuffer for pOutputBufferHeader[%d]\n",__LINE__, i);
		   error = OMX_AllocateBuffer(*pHandle, &pOutputBufferHeader[i], 1, NULL, OutputBufferSize);
		   if(error != OMX_ErrorNone) 
		   {
			  APP_EPRINT("%d :: APP: Error returned by OMX_AllocateBuffer for pOutputBufferHeader[%d]\n",__LINE__, i);
			  goto EXIT;
		   }
		}
#else
	    for(i = 0; i < numofinbuff; i++) 
		{
		   pInputBuffer[i] = (OMX_U8*)newmalloc(INPUT_AACENC_BUFFER_SIZE + 256);  
		   APP_DPRINT("%d :: APP: pInputBuffer[%d] = %p\n",__LINE__,i,pInputBuffer[i]);
		   if(NULL == pInputBuffer[i]) 
		   {
			  APP_EPRINT("%d :: APP: Malloc Failed\n",__LINE__);
			  error = OMX_ErrorInsufficientResources;
			  goto EXIT;
		   }
		   pInputBuffer[i] = pInputBuffer[i] + 128;
		   /* pass input buffer */
		   APP_DPRINT("%d :: APP: About to call OMX_UseBuffer\n",__LINE__);
		   APP_DPRINT("%d :: APP: pInputBufferHeader[%d] = %p\n",__LINE__,i,pInputBufferHeader[i]);
		   error = OMX_UseBuffer(*pHandle, &pInputBufferHeader[i], 0, NULL, INPUT_AACENC_BUFFER_SIZE, pInputBuffer[i]);
		   if(error != OMX_ErrorNone) 
		   {
			  APP_EPRINT("%d :: APP: Error returned by OMX_UseBuffer()\n",__LINE__);
			  goto EXIT;
		   }
		}

	    for(i = 0; i < numofoutbuff; i++) 
		{
		   pOutputBuffer[i] = (OMX_U8*) newmalloc (OutputBufferSize + 256); 
		   APP_DPRINT("%d :: APP: pOutputBuffer[%d] = %p\n",__LINE__,i,pOutputBuffer[i]);
		   if(NULL == pOutputBuffer[i]) 
		   {
			  APP_EPRINT("%d :: APP: Malloc Failed\n",__LINE__);
			  error = OMX_ErrorInsufficientResources;
			  goto EXIT;
		   }
		   pOutputBuffer[i] = pOutputBuffer[i] + 128;

		   /* allocate output buffer */
		   APP_DPRINT("%d :: APP: About to call OMX_UseBuffer\n",__LINE__);
		   APP_DPRINT("%d :: APP: pOutputBufferHeader[%d] = %p\n",__LINE__,i,pOutputBufferHeader[i]);
		   error = OMX_UseBuffer(*pHandle, &pOutputBufferHeader[i], 1, NULL, OutputBufferSize, pOutputBuffer[i]);
		   if(error != OMX_ErrorNone) 
		   {
			  APP_EPRINT("%d :: APP: Error returned by OMX_UseBuffer()\n",__LINE__);
			  goto EXIT;
		   }
		}
#endif

		/* --------Change to Idle  ---------*/
		APP_DPRINT ("%d:: APP: Sending OMX_StateIdle Command\n",__LINE__);
	#ifdef OMX_GETTIME
		GT_START();
	#endif
		error = OMX_SendCommand(*pHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
		if(error != OMX_ErrorNone) 
		{
			APP_EPRINT("APP: Error from SendCommand-Idle(Init) State function\n");
			goto EXIT;
		}
		/* Wait for startup to complete */
	error = WaitForState(*pHandle, OMX_StateIdle);
#ifdef OMX_GETTIME
	GT_END("Call to SendCommand <OMX_StateIdle>");
#endif  
		if(error != OMX_ErrorNone) 
		{
			APP_EPRINT("APP: Error: WaitForState reports an error %X\n", error);
			goto EXIT;
		}
		
		
		if (audioinfo.dasfMode == 1)
		{
			/* get streamID back to application */
		    error = OMX_GetExtensionIndex(*pHandle, "OMX.TI.index.config.aacencstreamIDinfo",&index);
		    if (error != OMX_ErrorNone) 
			{
			    APP_EPRINT("APP: Error getting extension index\n");
			    goto EXIT;
			}

		    error = OMX_GetConfig (*pHandle, index, streaminfo);
	        if(error != OMX_ErrorNone) 
			{
	            error = OMX_ErrorBadParameter;
	            APP_EPRINT("%d :: APP: Error from OMX_GetConfig() function\n",__LINE__);
	            goto EXIT;
			}

		    streamId = ((TI_OMX_STREAM_INFO*)streaminfo)->streamId;
		    APP_IPRINT(" ***************StreamId=%d******************\n", (int)streamId);		
		}
		

/*----------------------------------------------
 Main Loop for Non Deleting component test
 ----------------------------------------------*/
		kk = 0;
		for(kk=0; kk<testcnt; kk++) 
		{
			APP_DPRINT ("%d :: APP: Test counter = %d \n",__LINE__,kk);
			if(kk > 0) 
			{
	            APP_IPRINT ("Encoding the file one more Time\n");

	            close(IpBuf_Pipe[0]);
	            close(IpBuf_Pipe[1]);
	            close(OpBuf_Pipe[0]);
	            close(OpBuf_Pipe[1]);

	            /* Create a pipe used to queue data from the callback. */
	            retval = pipe(IpBuf_Pipe);
	            if( retval != 0) 
				{
	                APP_EPRINT( "%d :: APP: Error: Fill Data Pipe failed to open\n",__LINE__);
	                goto EXIT;
	            }

	            retval = pipe(OpBuf_Pipe);
	            if( retval != 0) 
				{
	                APP_EPRINT( "%d :: APP: Error: Empty Data Pipe failed to open\n",__LINE__);
	                goto EXIT;
	            }
				if (audioinfo.dasfMode == 0) /*No need for dasf mode */
				{
		            fIn = fopen(argv[1], "r");
		            if(fIn == NULL) 
					{
		                APP_EPRINT("Error:  failed to open the file %s for readonly access\n", argv[1]);
		                goto EXIT;
		            }
				}
	            fOut = fopen(fname, "w");
	            if(fOut == NULL) 
				{
	               APP_EPRINT("Error:  failed to create the output file \n");
	                goto EXIT;
	            }
        	}
			nFrameCount = 0;
			APP_IPRINT("------------------------------------------------------------\n");
			APP_IPRINT ("%d :: APP: Encoding the file [%d] Time\n",__LINE__, kk+1);
			APP_IPRINT("------------------------------------------------------------\n");
			if ( atoi(argv[4])== 4)
			{
				APP_IPRINT("------------------------------------------------------------\n");
				APP_IPRINT ("Testing Repeated RECORD without Deleting Component\n");
				APP_IPRINT("------------------------------------------------------------\n");
			}
			if(fIn == NULL) 
			{
				fIn = fopen(argv[1], "r");
				if(fIn == NULL) 
				{
					APP_EPRINT("APP: Error:  failed to open the file %s for readonly access\n", argv[1]);
					goto EXIT;
				}
			}
			
			if(fOut == NULL) 
			{
				fOut = fopen(fname, "w");
				if(fOut == NULL) 
				{
					APP_EPRINT("APP: Error:  failed to create the output file %s\n", argv[2]);
					goto EXIT;
				}
			}

			/* -------- Change to Executing ------------ */
			done = 0;
			APP_DPRINT ("%d :: APP: Sending OMX_StateExecuting Command\n",__LINE__);
		#ifdef OMX_GETTIME
			GT_START();
		#endif
			error = OMX_SendCommand(*pHandle,OMX_CommandStateSet, OMX_StateExecuting, NULL);
			if(error != OMX_ErrorNone) 
			{
				APP_EPRINT ("APP: Error from SendCommand-Executing State function \n");
				goto EXIT;
			}
			error = WaitForState(*pHandle, OMX_StateExecuting); 
		#ifdef OMX_GETTIME
			GT_END("Call to SendCommand <OMX_StateExecuting>");
		#endif
			if(error != OMX_ErrorNone) 
			{
				APP_EPRINT ( "APP: WaitForState reports an error \n");
				goto EXIT;
			}

			pComponent = (OMX_COMPONENTTYPE *)*pHandle;
		    error = OMX_GetState(*pHandle, &state);
	        if(error != OMX_ErrorNone)
			{
		        APP_EPRINT ("%d :: APP: OMX_GetState has returned status %X\n",__LINE__, error);
		        goto EXIT;
			}

			if (audioinfo.dasfMode == 0)
			{
				for(i = 0; i < numofinbuff; i++) 
				{
		            nRead = fread(pInputBufferHeader[i]->pBuffer, 1, pInputBufferHeader[i]->nAllocLen, fIn);
		            APP_DPRINT("%d :: APP: Reading InputBuffer = %d from the input file nRead = %d\n",__LINE__, nIpBuffs, nRead);
				    if((nRead < pInputBufferHeader[i]->nAllocLen) && (done == 0)) 
					{
			            APP_DPRINT("%d :: APP: Sending Last Input Buffer from TestApp(which can be zero or less than Buffer length) ---------- \n",__LINE__);
					    pInputBufferHeader[i]->nFlags = OMX_BUFFERFLAG_EOS;
					}
				    APP_DPRINT("%d :: APP :: Input Buffer: Calling EmptyThisBuffer: %p\n",__LINE__,pInputBufferHeader[i]);
				    pInputBufferHeader[i]->nFilledLen = nRead;
					pInputBufferHeader[i]->nTimeStamp= rand() % 100;
                    pInputBufferHeader[i]->nTickCount = rand() % 100;
				#ifdef OMX_GETTIME
					if (k==0)
					{ 
						GT_FlagE=1;  /* 1 = First Buffer,  0 = Not First Buffer  */
						GT_START(); /* Empty Bufffer */
					}
				#endif

				if (!preempted)
				    OMX_EmptyThisBuffer(*pHandle, pInputBufferHeader[i]);


					APP_DPRINT("APP: pInputBufferHeader[%d]->nTimeStamp = %lli\n",i,pInputBufferHeader[i]->nTimeStamp);
				    nIpBuffs++;
				}
			}
			
	        for (k=0; k < numofoutbuff; k++) 
			{
		        APP_DPRINT("%d :: APP: Before Fill this buffer is called = %x\n",__LINE__, (unsigned int)pOutputBufferHeader[k]);
			#ifdef OMX_GETTIME
				if (k==0)
					{ 
						GT_FlagF=1;  /* 1 = First Buffer,  0 = Not First Buffer  */
						GT_START(); /* Fill Buffer */
					}
			#endif    
                OMX_FillThisBuffer(*pHandle,  pOutputBufferHeader[k]);
			}

/*----------------------------------------------
 Main while for the buffers process
 ----------------------------------------------*/

 			 /* Component is stopping now by procesing the playcomplete event  (bPlayCompleted Flag) */

#ifndef WAITFORRESOURCES
			while(( (error == OMX_ErrorNone) && (state != OMX_StateIdle)) && (state != OMX_StateInvalid) && (!bPlayCompleted))
			{
			if(1){
#else
    		while(1) {
       		if((error == OMX_ErrorNone) && (state != OMX_StateIdle) && (state != OMX_StateInvalid) && (!bPlayCompleted)){ 
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
					fprintf (stderr, " : Error \n");
					break;
				}

				if(retval == 0) 
				{
					APP_DPRINT("%d :: APP: The current state of the component = %d \n",__LINE__,state);
					APP_DPRINT("\n\n\n%d ::!!!!!!!     App Timeout !!!!!!!!!!! \n",__LINE__);
					APP_DPRINT("%d :: ---------------------------------------\n\n\n",__LINE__);
				}

				switch (atoi(argv[4])) 
				{
					case 1:
					case 4:
					case 5:
						if(audioinfo.dasfMode == 0) 
						{
						    APP_DPRINT("%d :: APP: AAC ENCODER RUNNING UNDER FILE 2 FILE MODE \n",__LINE__);
		    		        if(FD_ISSET(IpBuf_Pipe[0], &rfds)) 
							{
            		            OMX_BUFFERHEADERTYPE* pBuffer;
            		            read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
            		            if(done == 0) 
								{
						            nRead = fread(pBuffer->pBuffer, 1, pBuffer->nAllocLen, fIn);
						            APP_DPRINT("%d :: APP: Reading InputBuffer = %d from the input file nRead = %d\n",__LINE__, nIpBuffs, nRead);
						            if((nRead < pBuffer->nAllocLen) && (done == 0)) 
									{
							            APP_IPRINT("%d :: APP: Sending Last Input Buffer from TestApp \n",__LINE__);
							            done 			= 1;
							            pBuffer->nFlags = OMX_BUFFERFLAG_EOS;
									}
						            APP_DPRINT("%d :: APP :: Input Buffer: Calling EmptyThisBuffer: %p\n",__LINE__,pBuffer);
						            pBuffer->nFilledLen = nRead;
                                    pBuffer->nTimeStamp= rand() % 100;
                                    pBuffer->nTickCount = rand() % 100;
						            OMX_EmptyThisBuffer(*pHandle, pBuffer);
						            nIpBuffs++;
								}
						   }
						}
						else {
						    if(done == 0) 
							{
							    APP_DPRINT("%d :: APP: AAC ENCODER RUNNING UNDER DASF MODE \n",__LINE__);
								if(nFrameCount == atoi(argv[9])) 
								{
									APP_DPRINT("%d :: APP: Sending Stop.........From APP \n",__LINE__);
									APP_DPRINT("%d :: APP: Shutting down ---------- \n",__LINE__);
								#ifdef OMX_GETTIME
									GT_START();
								#endif	
									error = OMX_SendCommand(*pHandle,OMX_CommandStateSet, OMX_StateIdle, NULL);
									if(error != OMX_ErrorNone) 
									{
										APP_EPRINT("APP: Error from SendCommand-Idle(Stop) State function\n");
										goto EXIT;
									}
									done = 1;
									error = WaitForState(*pHandle, OMX_StateIdle);
								#ifdef OMX_GETTIME
									GT_END("Call to SendCommand <OMX_StateIdle>");
								#endif
									if(error != OMX_ErrorNone) 
									{
										APP_DPRINT ( "APP: WaitForState reports an error \n");
										goto EXIT;
									}
									
								}
								APP_DPRINT("%d :: APP: AAC ENCODER READING DATA FROM DASF  \n",__LINE__);
							}
						}
						break;

					case 2:
						if(audioinfo.dasfMode == 0) 
						{
					  	    APP_DPRINT("%d :: APP: AAC ENCODER RUNNING UNDER FILE 2 FILE MODE \n",__LINE__);
					        if( FD_ISSET(IpBuf_Pipe[0], &rfds) ) 
							{
					            OMX_BUFFERHEADERTYPE* pBuffer;
					            read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
					            if(done == 0) 
								{
						            APP_DPRINT("%d :: APP: Reading InputBuffer = %d from the input file nRead = %d\n",__LINE__, nIpBuffs, nRead);
						            nRead = fread(pBuffer->pBuffer, 1, pBuffer->nAllocLen, fIn);
						            if(frmCnt == 20) 
									{
						                APP_DPRINT("%d :: APP: Sending Stop.........From APP \n",__LINE__);
						                APP_DPRINT("%d :: APP: Shutting down ---------- \n",__LINE__);
									#ifdef OMX_GETTIME
										GT_START(); 
									#endif
										error = OMX_SendCommand(*pHandle,OMX_CommandStateSet, OMX_StateIdle, NULL);
						                if(error != OMX_ErrorNone) 
										{
							               APP_EPRINT("APP: Error from SendCommand-Idle(Stop) State function\n");
							                goto EXIT;
										}
										error = WaitForState(*pHandle, OMX_StateIdle);
									#ifdef OMX_GETTIME
										GT_END("Call to SendCommand <OMX_StateIdle>");
									#endif
										if(error != OMX_ErrorNone) 
										{
											APP_EPRINT("APP: WaitForState reports an error \n");
											goto EXIT;
										}
						                done 				 = 1;
						                pBuffer->nFlags 	 = OMX_BUFFERFLAG_EOS;
						                pBuffer->nFilledLen = 0;
									}
									else if((nRead < pBuffer->nAllocLen) && (done == 0)) 
									{
							             APP_DPRINT("%d :: APP: Sending Last Input Buffer from TestApp(which can be zero or less than Buffer length)\n",__LINE__);
							             done = 1;
							             pBuffer->nFlags = OMX_BUFFERFLAG_EOS;
									}
							        APP_DPRINT("%d :: APP :: Input buffer: Calling EmptyThisBuffer: %p\n",__LINE__,pBuffer);
                					pBuffer->nTimeStamp= rand() % 100;				/* random value for time stamp */
                                    pBuffer->nTickCount = rand() % 100;
							        OMX_EmptyThisBuffer(*pHandle, pBuffer);
									nIpBuffs++;
							        frmCnt++;
								}
							}
						} 
						else 
						{
							APP_DPRINT("%d :: APP: AAC ENCODER RUNNING UNDER DASF MODE \n",__LINE__);
							if(nFrameCount == atoi(argv[9])) 
							{
								APP_DPRINT("%d :: APP: Sending Stop.........From APP \n",__LINE__);
								APP_DPRINT("%d :: APP: Shutting down ---------- \n",__LINE__);
							#ifdef OMX_GETTIME
								GT_START();
							#endif
								error = OMX_SendCommand(*pHandle,OMX_CommandStateSet, OMX_StateIdle, NULL);
								if(error != OMX_ErrorNone) 
								{
									APP_EPRINT("APP: Error from SendCommand-Idle(Stop) State function\n");
									goto EXIT;
								}
								done = 1;
								error = WaitForState(*pHandle, OMX_StateIdle);
							#ifdef OMX_GETTIME
								GT_END("Call to SendCommand <OMX_StateIdle>");
							#endif
								if(error != OMX_ErrorNone) 
								{
									APP_EPRINT ( "APP: WaitForState reports an error \n");
									goto EXIT;
								}
							}
							APP_DPRINT("%d :: APP: AAC ENCODER READING DATA FROM DASF  \n",__LINE__);
						}	
						break;

					case 3:
						if(audioinfo.dasfMode == 0) 
						{
							APP_DPRINT("%d :: APP: AAC ENCODER RUNNING UNDER FILE 2 FILE MODE \n",__LINE__);
		    		        if(FD_ISSET(IpBuf_Pipe[0], &rfds)) 
							{
            		            OMX_BUFFERHEADERTYPE* pBuffer;
            		            read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
            		            if(done == 0) 
								{
						            nRead = fread(pBuffer->pBuffer, 1, pBuffer->nAllocLen, fIn);
						            APP_DPRINT("%d :: APP: Reading InputBuffer = %d from the input file nRead = %d\n",__LINE__, nIpBuffs, nRead);
						            if((nRead < pBuffer->nAllocLen) && (done == 0)) 
									{
							            APP_DPRINT("%d :: APP: Sending Last Input Buffer from TestApp(which can be zero or less than Buffer length) ---------- \n",__LINE__);
							            done 			= 1;
							            pBuffer->nFlags = OMX_BUFFERFLAG_EOS;
									}
						            APP_DPRINT("%d :: APP : Input Buffer- Calling EmptyThisBuffer: %p\n",__LINE__,pBuffer);
						            pBuffer->nFilledLen = nRead;
                					pBuffer->nTimeStamp= rand() % 100;				/* random value for time stamp */
                                    pBuffer->nTickCount = rand() % 100;
						            OMX_EmptyThisBuffer(*pHandle, pBuffer);
						            nIpBuffs++;
								}
							}
							if(nIpBuffs == 9) 
							{
							#ifdef OMX_GETTIME
								GT_START();
							#endif
								error = OMX_SendCommand(*pHandle,OMX_CommandStateSet, OMX_StatePause, NULL);
								if(error != OMX_ErrorNone) 
								{
									APP_EPRINT("APP: Error from SendCommand-Idle(Stop) State function\n");
									goto EXIT;
								}
								APP_DPRINT("%d :: APP: Pause: OpBuffs received = %d\n",__LINE__,nOpBuffs);
								error = WaitForState(*pHandle, OMX_StatePause);
							#ifdef OMX_GETTIME
								GT_END("Call to SendCommand <OMX_StatePause>");
							#endif
								if(error != OMX_ErrorNone) 
								{
									APP_EPRINT("APP: Error: WaitForState reports an error %X\n", error);
									goto EXIT;
								}
								APP_DPRINT("%d :: APP: Pause: State paused = %d\n",__LINE__,nOpBuffs);
								APP_IPRINT("%d :: APP: Pausing component...\n",__LINE__);
								APP_DPRINT("%d :: APP: Is Sleeping here for %d seconds\n",__LINE__, SLEEP_TIME);
								sleep(SLEEP_TIME);
							#ifdef OMX_GETTIME
								GT_START();
							#endif
								error = OMX_SendCommand(*pHandle,OMX_CommandStateSet, OMX_StateExecuting, NULL);
								if(error != OMX_ErrorNone) 
								{
									APP_EPRINT("APP: Error from SendCommand-Executing State function\n");
									goto EXIT;
								}
								APP_DPRINT("%d :: APP: Resumed: OpBuffs received = %d\n",__LINE__,nOpBuffs);
								error = WaitForState(*pHandle, OMX_StateExecuting);
							#ifdef OMX_GETTIME
								GT_END("Call to SendCommand <OMX_StateIdle>");
							#endif
								if(error != OMX_ErrorNone) 
								{
									APP_DPRINT ( "APP: WaitForState reports an error \n");
									goto EXIT;
								}
							}
						} 
						else 
						{
							APP_DPRINT("%d :: APP: AAC ENCODER RUNNING UNDER DASF MODE \n",__LINE__);
							if(nFrameCount == atoi(argv[9])) 
							{
								APP_DPRINT("%d :: APP: Sending Stop.........From APP \n",__LINE__);
								APP_IPRINT("%d :: APP: Shutting down ---------- \n",__LINE__);
							#ifdef OMX_GETTIME
								GT_START();
							#endif
								error = OMX_SendCommand(*pHandle,OMX_CommandStateSet, OMX_StateIdle, NULL);
								if(error != OMX_ErrorNone) 
								{
									APP_EPRINT("APP: Error from SendCommand-Idle(Stop) State function\n");
									goto EXIT;
								}
								done = 1;
								error = WaitForState(*pHandle, OMX_StateIdle);
							#ifdef OMX_GETTIME
								GT_END("Call to SendCommand <OMX_StateIdle>");
							#endif	
								if(error != OMX_ErrorNone) 
								{
									APP_DPRINT ( "APP: WaitForState reports an error \n");
									goto EXIT;
								}
								
							}
							if(nFrameCount == 9) 
							{
							#ifdef OMX_GETTIME
								GT_START();
							#endif
								error = OMX_SendCommand(*pHandle,OMX_CommandStateSet, OMX_StatePause, NULL);
								if(error != OMX_ErrorNone) 
								{
									APP_EPRINT("APP: Error from SendCommand-Idle(Stop) State function\n");
									goto EXIT;
								}
								APP_DPRINT("%d :: APP: Pause: OpBuffs received = %d\n",__LINE__,nOpBuffs);
								error = WaitForState(*pHandle, OMX_StatePause);
							#ifdef OMX_GETTIME
								GT_END("Call to SendCommand <OMX_StatePause>");
							#endif
								if(error != OMX_ErrorNone) 
								{
									APP_EPRINT("APP: Error: WaitForState reports an error %X\n", error);
									goto EXIT;
								}
								APP_DPRINT("%d :: APP: Pause: State paused = %d\n",__LINE__,nOpBuffs);
								APP_DPRINT("%d :: APP: Is Sleeping here for %d seconds\n",__LINE__, SLEEP_TIME);
								sleep(SLEEP_TIME);		/* Pause time */
							#ifdef OMX_GETTIME
								GT_START();
							#endif
								error = OMX_SendCommand(*pHandle,OMX_CommandStateSet, OMX_StateExecuting, NULL);
								if(error != OMX_ErrorNone) 
								{
									APP_EPRINT("APP: Error from SendCommand-Executing State function\n");
									goto EXIT;
								}
								APP_DPRINT("%d :: APP: Resumed: OpBuffs received = %d\n",__LINE__,nOpBuffs);
								error = WaitForState(*pHandle, OMX_StateExecuting);
							#ifdef OMX_GETTIME
								GT_END("Call to SendCommand <OMX_StateExecuting>");
							#endif
						
								if(error != OMX_ErrorNone) 
								{
									APP_EPRINT("APP: Error: WaitForState reports an error %X\n", error);
									goto EXIT;
								}
								
							}
							APP_DPRINT("%d :: APP: AAC ENCODER READING DATA FROM DASF  \n",__LINE__);
						}
						break;
					case 6:
						if(audioinfo.dasfMode == 0) {
						    APP_DPRINT("%d :: APP: AAC ENCODER RUNNING UNDER FILE 2 FILE MODE \n",__LINE__);
							if(nIpBuffs == 20) 
							{
									APP_DPRINT("APP: Sending Stop Command after sending 20 frames \n");
								#ifdef OMX_GETTIME
									GT_START();
								#endif									
									error = OMX_SendCommand(*pHandle,OMX_CommandStateSet, OMX_StateIdle, NULL);
									if(error != OMX_ErrorNone) 
									{
										APP_EPRINT("APP: Error from SendCommand-Idle(Stop) State function\n");
										goto EXIT;
									}
									error = WaitForState(*pHandle, OMX_StateIdle);
								#ifdef OMX_GETTIME
									GT_END("Call to SendCommand <OMX_StateIdle>");
								#endif
									if(error != OMX_ErrorNone) 
									{
										APP_EPRINT("APP: Error: WaitForState reports an error %X\n", error);
										goto EXIT;
									}
									APP_DPRINT("%d :: APP: About to call GetState() \n",__LINE__);
					                error = OMX_GetState(*pHandle, &state);
					                if(error != OMX_ErrorNone) 
									{
						                APP_EPRINT("APP: Warning:  hAacEncoder->GetState has returned status %X\n", error);
						                goto EXIT;
									}
							}
						    else if(FD_ISSET(IpBuf_Pipe[0], &rfds)) 
							{
            		            OMX_BUFFERHEADERTYPE* pBuffer;
            		            read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
            		            if(done == 0) 
								{
						            nRead = fread(pBuffer->pBuffer, 1, pBuffer->nAllocLen, fIn);
						            APP_DPRINT("%d :: APP: Reading InputBuffer = %d from the input file nRead = %d\n",__LINE__, nIpBuffs, nRead);
						            if((nRead < pBuffer->nAllocLen) && (done == 0)) 
									{
							            APP_DPRINT("%d :: APP: Sending Last Input Buffer from TestApp(which can be zero or less than Buffer length) \n",__LINE__);
							            done 			= 1;
							            pBuffer->nFlags = OMX_BUFFERFLAG_EOS;
									}
									APP_DPRINT("%d :: APP: Input Buffer: Calling EmptyThisBuffer: %p\n",__LINE__,pBuffer);
						            pBuffer->nFilledLen = nRead;
                					pBuffer->nTimeStamp= rand() % 100;				/* random value for time stamp */
                                    pBuffer->nTickCount = rand() % 100;
						            OMX_EmptyThisBuffer(*pHandle, pBuffer);
						            nIpBuffs++;
								}
							}
						} 
						else {
						    APP_DPRINT("%d :: APP: AAC ENCODER RUNNING UNDER DASF MODE \n",__LINE__);
							if(nFrameCount == 5) 
							{
								APP_DPRINT("APP: Sending Stop Command after sending 4 frames \n");
							#ifdef OMX_GETTIME
								GT_START();
							#endif
								error = OMX_SendCommand(*pHandle,OMX_CommandStateSet, OMX_StateIdle, NULL);
								if(error != OMX_ErrorNone) 
								{
									APP_EPRINT("APP: Error from SendCommand-Idle(Stop) State function\n");
									goto EXIT;
								}
								error = WaitForState(*pHandle, OMX_StateIdle);
							#ifdef OMX_GETTIME
								GT_END("Call to SendCommand <OMX_StateIdle>");
							#endif
								if(error != OMX_ErrorNone) 
								{
									APP_EPRINT("APP: Error: WaitForState reports an error %X\n", error);
									goto EXIT;
								}
								done = 1;
							}
							else if(nFrameCount == atoi(argv[9])) 
							{
								APP_DPRINT("%d :: APP: Sending Stop.........From APP \n",__LINE__);
								APP_DPRINT("%d :: APP: Shutting down ---------- \n",__LINE__);
							#ifdef OMX_GETTIME
								GT_START();
							#endif	
								error = OMX_SendCommand(*pHandle,OMX_CommandStateSet, OMX_StateIdle, NULL);
								if(error != OMX_ErrorNone) 
								{
									APP_EPRINT("APP: Error from SendCommand-Idle(Stop) State function\n");
									goto EXIT;
								}
								done = 1;
								error = WaitForState(*pHandle, OMX_StateIdle);
							#ifdef OMX_GETTIME
								GT_END("Call to SendCommand <OMX_StateIdle>");
							#endif
								if(error != OMX_ErrorNone) 
								{
									APP_EPRINT("APP: Error: WaitForState reports an error %X\n", error);
									goto EXIT;
								}
							}
							APP_DPRINT("%d :: APP: AAC ENCODER READING DATA FROM DASF  \n",__LINE__);
						}
						break;

					case 7:
					    if(audioinfo.dasfMode == 0) 
						{
						    APP_IPRINT("%d :: APP: This test is not applied to file mode\n",__LINE__);
						    goto EXIT;
						}
					    else
						{
							APP_DPRINT("%d :: APP: AAC ENCODER RUNNING UNDER DASF MODE \n",__LINE__);
						    if(nFrameCount == 5)
							{
							    /* set high gain for record stream */
							    APP_DPRINT("APP: [AAC encoder] --- will set stream gain to high\n");
            				    pCompPrivateStructGain->sVolume.nValue = 0x8000;
							    error = OMX_SetConfig(*pHandle, OMX_IndexConfigAudioVolume, pCompPrivateStructGain);
							    if (error != OMX_ErrorNone) 
								{
								    error = OMX_ErrorBadParameter;
								    goto EXIT;
								}
							}
						    if(nFrameCount == 10)
							{
							    /* set low gain for record stream */
							    APP_DPRINT("APP: [AAC encoder] --- will set stream gain to low\n");
            				    pCompPrivateStructGain->sVolume.nValue = 0x2000;
							    error = OMX_SetConfig(*pHandle, OMX_IndexConfigAudioVolume, pCompPrivateStructGain);
							    if (error != OMX_ErrorNone) 
								{
								    error = OMX_ErrorBadParameter;
								    goto EXIT;
								}
							}
						    if(nFrameCount == 15) 
							{
							#ifdef OMX_GETTIME
								GT_START();
							#endif							
						        error = OMX_SendCommand(*pHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
						        if(error != OMX_ErrorNone) 
								{
							        APP_DPRINT ("%d :: APP: Error from SendCommand-Idle(Stop) State function\n",__LINE__);
							        goto EXIT;
								}
						        done = 1;
								error = WaitForState(*pHandle, OMX_StateIdle);
							#ifdef OMX_GETTIME
								GT_END("Call to SendCommand <OMX_StateIdle>");
							#endif								
								if(error != OMX_ErrorNone) 
								{
									APP_EPRINT("APP: Error: WaitForState reports an error %X\n", error);
									goto EXIT;
								}
						        APP_DPRINT("%d :: APP: Shutting down ---------- \n",__LINE__);
							} 
						    APP_DPRINT("%d :: APP: AAC ENCODER READING DATA FROM DASF  \n",__LINE__);
					}
					break;
					
					default:
						APP_DPRINT("%d :: APP: ### Running Simple DEFAULT Case Here ###\n",__LINE__);
				} /* end of switch loop */


				if(FD_ISSET(OpBuf_Pipe[0], &rfds)) 
				{
					OMX_BUFFERHEADERTYPE* pBuf;
					read(OpBuf_Pipe[0], &pBuf, sizeof(pBuf));

					if (firstbuffer){   /* Discard first buffer - Config audio (PV) */
						memset(pBuf->pBuffer, 0x0, pBuf->nAllocLen);
	                    pBuf->nFilledLen=0;
						firstbuffer = 0;
					}
					APP_DPRINT("%d :: App: Buffer to write to the file: %p \n",__LINE__,pBuf);
					APP_DPRINT("%d :: ------------- App File Write --------------\n",__LINE__);
					APP_DPRINT("%d :: App: %ld bytes are being written\n",__LINE__,(pBuf->nFilledLen));
					APP_DPRINT("%d :: ------------- App File Write --------------\n\n",__LINE__);
					nOpBuffs++;
					fwrite (pBuf->pBuffer, 1, (pBuf->nFilledLen), fOut);
					OMX_FillThisBuffer(*pHandle, pBuf);
					APP_DPRINT("%d :: APP: Sent %p Emptied Output Buffer = %d to Comp\n",__LINE__,pBuf,nFrameCount+1);
					nFrameCount++;
				}

				if( FD_ISSET(Event_Pipe[0], &rfds) ) {

				                OMX_U8 pipeContents;
				                read(Event_Pipe[0], &pipeContents, sizeof(OMX_U8));

				                if (pipeContents == 0) {
				                    APP_IPRINT("Test app received OMX_ErrorResourcesPreempted\n");
				                    WaitForState(*pHandle,OMX_StateIdle);

				                    error = OMX_FreeBuffer(pHandle,OMX_DirInput,pInputBufferHeader[i]);
				                    if( (error != OMX_ErrorNone)) {
				                        APP_DPRINT ("%d :: Error in Free Handle function\n",__LINE__);
				                    }

				                    error = OMX_FreeBuffer(pHandle,OMX_DirOutput,pOutputBufferHeader[i]);
				                    if( (error != OMX_ErrorNone)) {
				                        APP_DPRINT ("%d:: Error in Free Handle function\n",__LINE__);
				                    }
#ifdef USE_BUFFER
				             
						for(i=0; i < numofinbuff; i++) 
						{
							if (pInputBuffer[i] != NULL)
							{
							   pInputBuffer[i] = pInputBuffer[i] - 128;
							   APP_DPRINT("%d :: [TESTAPPFREE] pInputBuffer[%d] = %p\n",__LINE__,i,pInputBuffer[i]);
							   if(pInputBuffer[i] != NULL)
							   {
									newfree(pInputBuffer[i]);
									pInputBuffer[i] = NULL;
							   }
							}
						}

						for(i=0; i < numofoutbuff; i++) 
						{
							if (pOutputBuffer[i] != NULL)
							{
								pOutputBuffer[i] = pOutputBuffer[i] - 128;
								APP_DPRINT("%d :: [TESTAPPFREE] pOutputBuffer[%d] = %p\n",__LINE__,i, pOutputBuffer[i]);
								if(pOutputBuffer[i] != NULL)
								{
									newfree(pOutputBuffer[i]);
									pOutputBuffer[i] = NULL;
							   }
							}
						}
#endif                        

				                	OMX_SendCommand(*pHandle,OMX_CommandStateSet, OMX_StateLoaded, NULL);
				                    WaitForState(*pHandle, OMX_StateLoaded); 

				                    OMX_SendCommand(*pHandle,OMX_CommandStateSet,OMX_StateWaitForResources,NULL);
				                    WaitForState(*pHandle,OMX_StateWaitForResources);
	
				                }
				                else if (pipeContents == 1) {
	
				                    APP_IPRINT("Test app received OMX_ErrorResourcesAcquired\n");

				                    OMX_SendCommand(*pHandle,OMX_CommandStateSet,OMX_StateIdle,NULL);
				                    error = OMX_AllocateBuffer(pHandle,
				                                            &pOutputBufferHeader[i],
				                                            1,
				                                            NULL,
				                                            OutputBufferSize);

				            		APP_DPRINT("%d :: called OMX_AllocateBuffer\n",__LINE__);
				            		if(error != OMX_ErrorNone) {
				            			APP_DPRINT("%d :: Error returned by OMX_AllocateBuffer()\n",__LINE__);
				            			goto EXIT;
				            		}

				                    WaitForState(*pHandle,OMX_StateIdle);

				                    OMX_SendCommand(*pHandle,OMX_CommandStateSet,OMX_StateExecuting,NULL);
				                    WaitForState(*pHandle,OMX_StateExecuting);

				                    rewind(fIn);

									if (!preempted)
									    OMX_EmptyThisBuffer(*pHandle, pInputBufferHeader[i]);

				                   /* send_input_buffer (pHandle, pOutputBufferHeader, fIn); */
				                }                

				                if (pipeContents == 2) {

#ifdef OMX_GETTIME
                    GT_START();
#endif
				                    OMX_SendCommand(*pHandle,OMX_CommandStateSet,OMX_StateIdle,NULL);
				                    WaitForState(*pHandle,OMX_StateIdle);
#ifdef OMX_GETTIME
                    GT_END("Call to SendCommand <OMX_StateIdle>");
#endif

#ifdef WAITFORRESOURCES

					for(i=0; i<numofinbuff; i++) {

				                    error = OMX_FreeBuffer(pHandle,OMX_DirInput,pInputBufferHeader[i]);
				                    if( (error != OMX_ErrorNone)) {
				                        APP_DPRINT ("%d :: Error in Free Handle function\n",__LINE__);
				                    }
					}
					for(i=0; i<numofoutbuff; i++) {

				                    error = OMX_FreeBuffer(pHandle,OMX_DirOutput,pOutputBufferHeader[i]);
				                    if( (error != OMX_ErrorNone)) {
				                        APP_DPRINT ("%d:: Error in Free Handle function\n",__LINE__);
				                    }

					}
				                                
				                	OMX_SendCommand(*pHandle,OMX_CommandStateSet, OMX_StateLoaded, NULL);
				                    WaitForState(*pHandle, OMX_StateLoaded); 
					
				                    goto SHUTDOWN;
									
#endif

				                }
				            }
		
				if(done == 1) 
				{
                    APP_DPRINT("%d :: APP: About to call GetState() \n",__LINE__);
					error = OMX_GetState(*pHandle, &state);
					if(error != OMX_ErrorNone) 
					{
						APP_EPRINT("APP: Warning:  hAacEncoder->GetState has returned status %X\n", error);
						goto EXIT;
					}
				}

				}
	            else if (preempted) {
	                sched_yield();
	            }
	            else {
	                goto SHUTDOWN; 
	            }      
			} /* end of while loop */

			if (bPlayCompleted == OMX_TRUE )	/* Stop componet - just for F2F  mode*/
			{
			#ifdef OMX_GETTIME
				GT_START();
			#endif			
				error = OMX_SendCommand(*pHandle,OMX_CommandStateSet, OMX_StateIdle, NULL);
				if(error != OMX_ErrorNone) 
				{
					APP_EPRINT("APP: Error from SendCommand-Idle(Stop) State function\n");
					goto EXIT;
				}
				error = WaitForState(*pHandle, OMX_StateIdle); 

			#ifdef OMX_GETTIME
				GT_END("Call to SendCommand <OMX_StateIdle>");
			#endif
				
				if(error != OMX_ErrorNone) 
				{
					APP_EPRINT("APP: Error: WaitForState reports an error %X\n", error);
					goto EXIT;
				}
				bPlayCompleted = OMX_FALSE;
			}


/*----------------------------------------------
	 Final stage : cleaning and closing
 ----------------------------------------------*/
			APP_DPRINT("%d :: APP: The current state of the component = %d \n",__LINE__,state);
			fclose(fOut);
			fclose(fIn);
			fOut=NULL;
			fIn=NULL;
			frmCount = 0;
		} /* End of internal loop*/

	    error = OMX_SendCommand(*pHandle, OMX_CommandPortDisable, -1, NULL);
        if(error != OMX_ErrorNone) 
		{
           APP_DPRINT("%d:: APP: Error from SendCommand OMX_CommandPortDisable\n",__LINE__);
        	goto EXIT;
		}

	    /* free the Allocate Buffers */
	    for(i=0; i < numofinbuff; i++) 
		{
		   APP_DPRINT("%d :: APP: About to free pInputBufferHeader[%d]\n",__LINE__, i);
		   error = OMX_FreeBuffer(*pHandle, INPUT_PORT, pInputBufferHeader[i]);
		   if((error != OMX_ErrorNone)) 
		   {
			  APP_DPRINT("%d:: APP: Error in FreeBuffer function\n",__LINE__);
			  goto EXIT;
		   }
		}
	    for(i=0; i < numofoutbuff; i++) 
		{
		   APP_DPRINT("%d :: APP: About to free pOutputBufferHeader[%d]\n",__LINE__, i);
		   error = OMX_FreeBuffer(*pHandle, OUTPUT_PORT, pOutputBufferHeader[i]);
		   if((error != OMX_ErrorNone)) 
		   {
			  APP_DPRINT("%d :: APP: Error in Free Buffer function\n",__LINE__);
			  goto EXIT;
		   }
		}
#ifdef USE_BUFFER
	    /* free the UseBuffers */
	    for(i=0; i < numofinbuff; i++) 
		{
			if (pInputBuffer[i] != NULL)
			{
			   pInputBuffer[i] = pInputBuffer[i] - 128;
			   APP_DPRINT("%d :: [TESTAPPFREE] pInputBuffer[%d] = %p\n",__LINE__,i,pInputBuffer[i]);
			   if(pInputBuffer[i] != NULL)
			   {
				  newfree(pInputBuffer[i]);
				  pInputBuffer[i] = NULL;
			   }
			}
		}

	    for(i=0; i < numofoutbuff; i++) 
		{
			if (pOutputBuffer[i] != NULL)
			{
			   pOutputBuffer[i] = pOutputBuffer[i] - 128;
			   APP_DPRINT("%d :: [TESTAPPFREE] pOutputBuffer[%d] = %p\n",__LINE__,i, pOutputBuffer[i]);
			   if(pOutputBuffer[i] != NULL)
			   {
				  newfree(pOutputBuffer[i]);
				  pOutputBuffer[i] = NULL;
			   }
			}
		}
#endif

		/* --------Change to Loaded  ---------*/	
		APP_DPRINT("%d :: APP: Sending the StateLoaded Command\n",__LINE__);
	#ifdef OMX_GETTIME
		GT_START();
	#endif
		error = OMX_SendCommand(*pHandle,OMX_CommandStateSet, OMX_StateLoaded, NULL);
		if(error != OMX_ErrorNone) 
		{
			APP_EPRINT("APP: Error from SendCommand-Idle State function\n");
			goto EXIT;
		}
		/* Wait for new state */
		error = WaitForState(*pHandle, OMX_StateLoaded);	
	#ifdef OMX_GETTIME
		GT_END("Call to SendCommand <OMX_StateLoaded>");
	#endif
		if(error != OMX_ErrorNone) 
		{
			APP_EPRINT("APP: Error:  hAacEncoder->WaitForState reports an error %X\n", error);
			goto EXIT;
		 
		}
		APP_DPRINT("%d :: APP: State Of Component Is Loaded Now\n",__LINE__);


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

		APP_MEMPRINT("%d :: [TESTAPPFREE] %p\n",__LINE__,pAacParam);
        if(pAacParam != NULL)
		{
	        newfree(pAacParam);
	        pAacParam = NULL; 
		}
		APP_MEMPRINT("%d :: [TESTAPPFREE] %p\n",__LINE__,iAacParam);
        if(iAacParam != NULL)
		{
			APP_DPRINT("iAacParam %p \n",iAacParam);
	        newfree(iAacParam); 
	        iAacParam = NULL; 
			APP_DPRINT("iAacParam %p \n",iAacParam);
		}
        APP_MEMPRINT("%d :: [TESTAPPFREE] %p\n",__LINE__,pCompPrivateStruct);
	    if(pCompPrivateStruct != NULL)
		{
		    newfree(pCompPrivateStruct);
		    pCompPrivateStruct = NULL; 
		}
		APP_MEMPRINT("%d :: [TESTAPPFREE] %p\n",__LINE__,pCompPrivateStructGain);
	    if(pCompPrivateStructGain != NULL)
		{
		   newfree(pCompPrivateStructGain);
		    pCompPrivateStructGain = NULL; 
		}
		APP_MEMPRINT("%d :: [TESTAPPFREE] %p\n",__LINE__,audioinfo.aacencHeaderInfo);
	    if(audioinfo.aacencHeaderInfo != NULL)
		{
		    newfree(audioinfo.aacencHeaderInfo);
		    audioinfo.aacencHeaderInfo = NULL; 
		} 
	    error = close (IpBuf_Pipe[0]);
	    if (0 != error && OMX_ErrorNone == error) 
		{
		    error = OMX_ErrorHardware;
		    APP_DPRINT("%d :: APP: Error while closing IpBuf_Pipe[0]\n",__LINE__);
		    goto EXIT;
		}
	    error = close (IpBuf_Pipe[1]);
	    if (0 != error && OMX_ErrorNone == error) 
		{
		    error = OMX_ErrorHardware;
		    APP_DPRINT("%d :: APP: Error while closing IpBuf_Pipe[1]\n",__LINE__);
		    goto EXIT;
		}
	    error = close (OpBuf_Pipe[0]);
	    if (0 != error && OMX_ErrorNone == error) 
		{
		    error = OMX_ErrorHardware;
		    APP_DPRINT("%d :: APP: Error while closing OpBuf_Pipe[0]\n",__LINE__);
		    goto EXIT;
		}
	    error = close (OpBuf_Pipe[1]);
	    if (0 != error && OMX_ErrorNone == error) 
		{
		    error = OMX_ErrorHardware;
		    APP_DPRINT("%d :: APP: Error while closing OpBuf_Pipe[1]\n",__LINE__);
		    goto EXIT;
		}

		error = close(Event_Pipe[0]);
	if (0 != error && OMX_ErrorNone == error) {
		error = OMX_ErrorHardware;
		APP_DPRINT("%d :: Error while closing Event_Pipe[0]\n",__LINE__);
		goto EXIT;
	}
		
	error = close(Event_Pipe[1]);
	if (0 != error && OMX_ErrorNone == error) {
		error = OMX_ErrorHardware;
		APP_DPRINT("%d :: Error while closing Event_Pipe[1]\n",__LINE__);
		goto EXIT;
	}
	
#ifdef DSP_RENDERING_ON        		
	    cmd_data.hComponent = *pHandle;
	    cmd_data.AM_Cmd = AM_Exit;

	    if((write(Aacenc_fdwrite, &cmd_data, sizeof(cmd_data)))<0)
	        APP_EPRINT("%d :: APP: ---send command to audio manager\n",__LINE__);

	    close(Aacenc_fdwrite);
	    close(Aacenc_fdread);
#endif

	    /* Free the AacEncoder Component */
		error = TIOMX_FreeHandle(*pHandle);
		if( (error != OMX_ErrorNone)) {
			APP_EPRINT("APP: Error in Free Handle function\n");
			goto EXIT;
		}
		APP_DPRINT("%d :: App: pHandle = %p\n",__LINE__,pHandle);
		APP_IPRINT("%d :: APP: Free Handle returned Successfully \n",__LINE__);

		error = TIOMX_Deinit();
		if( (error != OMX_ErrorNone)) {
			APP_EPRINT("APP: Error in Deinit Core function\n");
			goto EXIT;
		}

		error= newfree(pHandle);
		if( (error != OMX_ErrorNone)) {
			APP_EPRINT("APP: Error in free PHandle\n");
			goto EXIT;
		}


    } /*--------- end of for loop--------- */

	error= CleanList(&ListHeader);			/* it frees streaminfo */					
	if( (error != OMX_ErrorNone)) 
	{
		APP_DPRINT("APP: Error in CleanList function\n");
		goto EXIT;
	}

	pthread_cond_destroy(&WaitForStateMutex.cond);
	pthread_mutex_destroy(&WaitForStateMutex.Mymutex);

#ifdef AACENC_DEBUGMEM	
	APP_IPRINT("\n-Printing memory not delete-\n");
    for(r=0;r<500;r++)
	{
        if (lines[r]!=0){
             APP_IPRINT(" --->%d Bytes allocated on %p File:%s Line: %d\n",bytes[r],arr[r],file[r],lines[r]);                  
        }

    }
#endif
	

EXIT:
	if(bInvalidState==OMX_TRUE)
	{
#ifndef USE_BUFFER

		error = FreeAllResources(*pHandle,
								pInputBufferHeader[0],
								pOutputBufferHeader[0],
								numofinbuff,
								numofoutbuff,
								fIn,fOut,ListHeader);
#else
		error = freeAllUseResources(*pHandle,
									pInputBuffer,
									pOutputBuffer,
									numofinbuff,
									numofoutbuff,
									fIn,fOut,ListHeader);
#endif
	}	
#ifdef OMX_GETTIME
  GT_END("AAC_ENC test <End>");
  OMX_ListDestroy(pListHead);	
#endif	

    return error;
}


/*-------------------------------------------------------------------*/
/**
  *  mymalloc() function to perform dynamic memory allocation. 
  *
  * @param size         			Size of memory requested
  * @param ListHeader		Top pointer of the linked List
  *   
  * @retval p   				Pointer to the allocated memory
  * 
  **/
/*-------------------------------------------------------------------*/

void * mymalloc(int size,ListMember** ListHeader)
{
   int error=0;
   void *p;     
   p = malloc(size);

   if(p==NULL)
   	{
       APP_EPRINT("APP: Memory not available\n");
       exit(1);
    }
   else
   	{
		error = AddMemberToList(p,ListHeader); 
		if(error)
			exit(1);
	    return p;
    }
}
 
 /*-------------------------------------------------------------------*/
 /**
   *  myfree() function to free dynamic memory allocated. 
   *
   * @param dp				 Dinamic memory pointer to be freed
   * @param ListHeader		 Top pointer of the linked List
   *   
   * @retval OMX_ErrorNone	 Success on freeing memory
   * 
   **/
 /*-------------------------------------------------------------------*/

int myfree(void *dp, ListMember** ListHeader)
{
	  int error=0;
	  error = FreeListMember(dp, ListHeader); 
     /* free(dp);  */
	  if (error)
	  	APP_EPRINT("APP: Error freeing \n");

	  return error;
}


/*-------------------------------------------------------------------*/
/**
  *  FreeAllResources() function that release all allocated resources when an important error is produced.
 * 					 Buffers were allocated by component
  *
  * @parameters  pointers  from most of allocated resources
  * 
  *   
  * @retval OMX_ErrorNone		Success on freeing resources
  * 
  **/
/*-------------------------------------------------------------------*/

OMX_ERRORTYPE FreeAllResources( OMX_HANDLETYPE pHandle,
			                OMX_BUFFERHEADERTYPE* pBufferIn,
			                OMX_BUFFERHEADERTYPE* pBufferOut,
			                int NIB, int NOB,
			                FILE* fIn, FILE* fOut,
			                ListMember* ListHeader)
{
	APP_DPRINT("%d:: APP: Freeing all resources by state invalid \n",__LINE__);
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_U16 i; 
	for(i=0; i < NIB; i++) 
	{
		   APP_DPRINT("%d :: APP: About to free pInputBufferHeader[%d]\n",__LINE__, i);
		   eError = OMX_FreeBuffer(pHandle, INPUT_PORT, pBufferIn+i);
		 	if(eError != OMX_ErrorNone) 
			{
				APP_DPRINT("APP: Error:  Freebuffer by MMU_Fault %X\n", eError);
				goto EXIT_ERROR;
			}
		   
	}
	for(i=0; i < NOB; i++) 
	{
		   APP_DPRINT("%d :: APP: About to free pOutputBufferHeader[%d]\n",__LINE__, i);
		   eError = OMX_FreeBuffer(pHandle, OUTPUT_PORT, pBufferOut+i);
		   if(eError != OMX_ErrorNone) 
		   {
			   APP_DPRINT("APP: Error:	Freebuffer by MMU_Fault %X\n", eError);
			   goto EXIT_ERROR;
		   }
	}

	/* Freeing Linked list */
	eError= CleanList(&ListHeader);
	if( (eError != OMX_ErrorNone)) 
	{
		APP_DPRINT("APP: Error in CleanList function\n");
		goto EXIT_ERROR;
	}

	pthread_cond_destroy(&WaitForStateMutex.cond);
	pthread_mutex_destroy(&WaitForStateMutex.Mymutex);
	
    eError = close (IpBuf_Pipe[0]);
    eError = close (IpBuf_Pipe[1]);
    eError = close (OpBuf_Pipe[0]);
    eError = close (OpBuf_Pipe[1]);
	if(fOut != NULL)	
	{
		fclose(fOut);
		fOut=NULL;
	}

	if(fIn != NULL)
	{	fclose(fIn);
		fIn=NULL;
	}

	eError = TIOMX_FreeHandle(pHandle);
	if( (eError != OMX_ErrorNone)) 
	{
		APP_EPRINT("APP: Error in Free Handle function\n");
		goto EXIT_ERROR;
	}
	eError = TIOMX_Deinit();

EXIT_ERROR:
	
	return eError;
}


/*-------------------------------------------------------------------*/
/**
  *  freeAllUseResources() function that release all allocated resources from APP 
  * 						when an important error is produced. Buffers were allocated by App.
  *
  * @parameters  pointers  from most of allocated resources
  * 
  *   
  * @retval OMX_ErrorNone		Success on freeing resources
  * 
  **/
/*-------------------------------------------------------------------*/

#ifdef USE_BUFFER

OMX_ERRORTYPE freeAllUseResources(OMX_HANDLETYPE pHandle,
							OMX_U8* UseInpBuf[],
							OMX_U8* UseOutBuf[], 			
							int NIB,int NOB,
							FILE* fIn, FILE* fOut,
							ListMember* ListHeader)
{

		OMX_ERRORTYPE eError = OMX_ErrorNone;
		OMX_U16 i; 
		APP_DPRINT("%d ::APP: Freeing all resources by state invalid \n",__LINE__);
    	/* free the UseBuffers */
	    for(i=0; i < NIB; i++) 
		{

			if( UseInpBuf[i] != NULL )	  
			{
			   UseInpBuf[i] = UseInpBuf[i] - 128;
			   APP_DPRINT("%d :: [TESTAPPFREE] pInputBuffer[%d] = %p\n",__LINE__,i,(UseInpBuf[i]));
			   if(UseInpBuf[i] != NULL)
			   {
				  newfree(UseInpBuf[i]);
				  UseInpBuf[i] = NULL;
			   }
			}
		}

	    for(i=0; i < NOB; i++) 
		{
			if (UseOutBuf[i] != NULL)
			{
			   UseOutBuf[i] = UseOutBuf[i] - 128;
			   APP_DPRINT("%d :: [TESTAPPFREE] pOutputBuffer[%d] = %p\n",__LINE__,i, UseOutBuf[i]);
			   if(UseOutBuf[i] != NULL)
			   {
				  newfree(UseOutBuf[i]);
				  UseOutBuf[i] = NULL;
			   }
			}
		}
		
		/* Freeing Linked list */
		eError= CleanList(&ListHeader);
		if( (eError != OMX_ErrorNone)) 
		{
			APP_DPRINT("APP: Error in CleanList function\n");
			goto EXIT_ERROR;
		}

		pthread_cond_destroy(&WaitForStateMutex.cond);
		pthread_mutex_destroy(&WaitForStateMutex.Mymutex);
	
		eError = close (IpBuf_Pipe[0]);
		eError = close (IpBuf_Pipe[1]);
		eError = close (OpBuf_Pipe[0]);
		eError = close (OpBuf_Pipe[1]);

		if (fOut != NULL)	/* Could have been closed  previously */ 
		{
			fclose(fOut);
			fOut=NULL;
		}
		
		if (fIn != NULL)
		{	fclose(fIn);
			fIn=NULL;
		}
	
		eError = TIOMX_FreeHandle(pHandle);
		if( (eError != OMX_ErrorNone)) 
		{
			APP_EPRINT("APP: Error in Free Handle function\n");
			goto EXIT_ERROR;
		}
		eError = TIOMX_Deinit();
	
EXIT_ERROR:

		return eError;

}

#endif



/*-------------------------------------------------------------------*/
/**
  *  ValidateParameters()  Function that validates the sample rate and Bitarete combination according to a
 * 						defined values table
  *
  * @parameters  SampleRate		  Sample rate value
  *				numChannels	  Number of channels 
  *				BitRate			  Bit rate value
  * 
  *   
  * @retval ValidParameter		  Boolean value for validation
  * 
  **/
/*-------------------------------------------------------------------*/

OMX_BOOL ValidateParameters(OMX_U32 SampleRate, OMX_U32 numChannels, OMX_U32 BitRate, OMX_U32 ObjectType)
{
	OMX_U32 LimitsLC [12][4]={{8000  ,MONO   ,8000  ,42000 },
				   			{8000  ,STEREO ,16000 ,84000 },
				   			{16000 ,MONO   ,8000  ,84000 },
				   			{16000 ,STEREO ,16000 ,168000},
				   			{22050 ,STEREO ,16000 ,232000},
							{22050 ,STEREO ,16000 ,232000},
				   			{32000 ,STEREO ,16000 ,320000},
				   			{32000 ,STEREO ,16000 ,320000},
				   			{44100 ,MONO   ,8000  ,160000},
				  			{44100 ,STEREO ,16000 ,320000},
				   			{48000 ,MONO   ,8000  ,160000},
				   			{48000 ,STEREO ,16000 ,320000} };
				   
	OMX_U32 LimitsHE [12][4]={{16000,MONO  ,8000  ,48000 },
				   			{16000 ,STEREO ,16000 ,128000},
				   			{22050 ,MONO   ,8000  ,64000 },
				   			{22050 ,STEREO ,16000 ,128000},
				   			{24000 ,MONO   ,8000  ,64000 },
							{24000 ,STEREO ,16000 ,128000},
				   			{32000 ,MONO   ,8000  ,64000 },
				   			{32000 ,STEREO ,16000 ,128000},
				   			{44100 ,MONO   ,12000 ,64000 },
				  			{44100 ,STEREO ,16000 ,128000},
				   			{48000 ,MONO   ,12000 ,64000 },
				   			{48000 ,STEREO ,16000 ,128000} };

	OMX_U32 LimitsHE2 [6][4]={{16000,STEREO,8000  ,48000},
				   			{22050 ,STEREO ,8000  ,64000},
				   			{24000 ,STEREO ,8000  ,64000},
				   			{32000 ,STEREO ,8000  ,64000},
				   			{44100 ,STEREO ,12000 ,64000},
							{48000 ,STEREO ,12000 ,64000}, };
		
	OMX_BOOL ValidParameter = OMX_TRUE;
	OMX_BOOL isDone = OMX_FALSE;
	OMX_U16 i,j;
	

	APP_DPRINT("Inside validateparameters \n");
	APP_DPRINT("sample rate %d \n",(int)SampleRate);
	APP_DPRINT("Bit rate %d \n", (int)BitRate);
		

	if(ObjectType == ObjectTypeHE) {
		for (i=0;i<12;i+=2)
		{
			APP_DPRINT("APP: sample compared %d : %d \n",(int)SampleRate, (int)LimitsHE[i][0]);
			if( SampleRate == LimitsHE[i][0] )  							/*check for  sample rate */
			{		
				for ( j=0; j<2; j++)
				{
					APP_DPRINT("APP: channels compared %d : %d \n", (int)numChannels, (int)LimitsHE[i+j][1]);
					if ( numChannels== LimitsHE[i+j][1] )					/* check for mono/stereo */
					{
						if ( (BitRate >= LimitsHE[i+j][2] ) && (BitRate <= LimitsHE[i+j][3]) )
						{
							ValidParameter = OMX_TRUE;					/* check for value is within range */
							isDone		   = OMX_TRUE;
							break;
						}
						else
						{
							ValidParameter = OMX_FALSE;					/* Value is out is out of range */
							break;
						}
					}
					else
					{
						ValidParameter		=OMX_FALSE;
					}
				}	
				if( (!ValidParameter) || (isDone) )						/* No need to keep searching */
					break;
			}
		}
	}


	else if(ObjectType == ObjectTypeHE2) {
		for (i=0;i<6;i++)
		{
			APP_DPRINT("APP: sample compared %d : %d \n",(int)SampleRate, (int)LimitsHE2[i][0]);
			if( SampleRate == LimitsHE2[i][0] )  							/*check for  sample rate */
			{		
					APP_DPRINT("APP: channels compared %d : %d \n", (int)numChannels, (int)LimitsHE2[i+j][1]);
					if ( numChannels== STEREO )					/* check for stereo only*/
					{
						if ( (BitRate >= LimitsHE2[i][2] ) && (BitRate <= LimitsHE2[i][3]) )
						{
							ValidParameter = OMX_TRUE;					/* check for value is within range */
							isDone		   = OMX_TRUE;
							break;
						}
						else
						{
							ValidParameter = OMX_FALSE;					/* Value is out is out of range */
							break;
						}
					}
					else
					{
						ValidParameter		=OMX_FALSE;
					}
				if( (!ValidParameter) || (isDone) )						/* No need to keep searching */
					break;
			}
		}

	}

	else{   						/* ObjectType == ObjectTypeLC */
		for (i=0;i<12;i+=2)
		{
			APP_DPRINT("APP: sample compared %d : %d \n",(int)SampleRate, (int)LimitsLC[i][0]);
			if( SampleRate == LimitsLC[i][0] )  							/*check for  sample rate */
			{		
				for ( j=0; j<2; j++)
				{
					APP_DPRINT("APP: channels compared %d : %d \n", (int)numChannels, (int)LimitsLC[i+j][1]);
					if ( numChannels== LimitsLC[i+j][1] )					/* check for mono/stereo */
					{
						if ( (BitRate >= LimitsLC[i+j][2] ) && (BitRate <= LimitsLC[i+j][3]) )
						{
							ValidParameter = OMX_TRUE;					/* check for value is within range */
							isDone		   = OMX_TRUE;
							break;
						}
						else
						{
							ValidParameter = OMX_FALSE;					/* Value is out is out of range */
							break;
						}
					}
					else
					{
						ValidParameter		=OMX_FALSE;
					}
				}	
				if( (!ValidParameter) || (isDone) )						/* No need to keep searching */
					break;
			}
		}
	}


return ValidParameter;
								
/*NOTE: 11025 and 22050 values for Sample rates are missing in the table. */

}


/*-------------------------------------------------------------------*/
/**
  *  CalculateOutputBufferSize() 	Calculates an optimun size per output 
  * 								buffer according to the number of frames
  *
  * @param   SampleFrec			Sample frecuency test parameter
  * @param   BitRate				Bit rate test parameter
  * @param   FramesPerOutBuf		Required number of frames
  *
  * @retval 	OutputBufferSize		Calculated buffer size			
  **/
/*-------------------------------------------------------------------*/

OMX_U32 CalculateOutputBufferSize (OMX_U32 SampleFrec, OMX_U32 BitRate, OMX_U16 FramesPerOutBuf  )
{
	float AvgFrameSize		=0;
	float FramePerSec		=0;
	float fOutBufSize		=0;
	float mantissa			=0;
	OMX_U32 OutputBufferSize=0;

	FramePerSec= (float)SampleFrec / 1024;								/*	 Sample frec /1024	*/
		APP_DPRINT("FramePerSec %f \n",FramePerSec);
	AvgFrameSize= ((BitRate/FramePerSec)/8)* 1.3; 						/*	 ( (BitRate/framesPerSec)/8 ) * 1.3  */
		APP_DPRINT("AvgFrameSize %f \n",AvgFrameSize);
	fOutBufSize= (AvgFrameSize * (float)FramesPerOutBuf)+1200;			/*	(AverageFrameSize * NumFrames)+1200   */
		APP_DPRINT("float- size output buffers %f \n",fOutBufSize);
	OutputBufferSize = (OMX_U32)fOutBufSize;
		APP_DPRINT("U32- size output buffers %d \n", (int)OutputBufferSize);
	mantissa = fOutBufSize - OutputBufferSize;
	if(mantissa > 0)		/*rounding-up*/
	{	
		OutputBufferSize++; 											/* Caluculated Ouput Buffer size */
	}											
	APP_DPRINT("(Rounded) size output buffers %d \n",(int)OutputBufferSize);

	return OutputBufferSize;


}


/*-------------------------------------------------------------------*/
/**
  *  AddMemberToList() Adds a memeber to the list for allocated memory pointers
  *
  * @param ptr         			memory pointer to add to the member list
  * @param ListHeader		Top pointer of the List
  *   *
  * @retval OMX_ErrorNone   					 Success, member added
 *               OMX_ErrorInsufficientResources		 Memory  failure
  **/
/*-------------------------------------------------------------------*/

OMX_ERRORTYPE AddMemberToList(void* ptr, ListMember** ListHeader)
{	
	int Error = OMX_ErrorNone;										/* No Error  */
	static int InstanceCounter = 0;
	ListMember* temp;
	if(*ListHeader == NULL)
	{
		InstanceCounter =0;											/* reset counter */
	}

	temp = (ListMember*)malloc(sizeof(ListMember));					/* New Member */
	if(NULL == temp)
	{
		APP_EPRINT("%d :: App: Malloc Failed\n",__LINE__);
		Error = OMX_ErrorInsufficientResources;						/* propper Error */
		goto EXIT_ERROR;
	} 
	APP_DPRINT("\nNew Instance created pointer : %p \n",temp);

	APP_DPRINT("Header parameter pointer : %p \n",*ListHeader);
	temp->NextListMember 		= *ListHeader;						/* Adding the new member */
	APP_DPRINT("Next linked pointer  : %p \n",temp->NextListMember);
	temp->ListCounter			= ++InstanceCounter;				/* Pre-increment */
	APP_DPRINT("Instance counter %d \n",temp->ListCounter);
	temp->Struct_Ptr 			= ptr;								/* Saving passed pointer (allocated memory)*/ 
	APP_DPRINT("Parameter pointer to save : %p \n",ptr);
	*ListHeader				 	= temp;								/* saving the Header */
	APP_DPRINT("New Header pointer : %p \n",*ListHeader);
	
	
EXIT_ERROR:
	return Error;
}


/*-------------------------------------------------------------------*/
/**
  * CleanList() Frees the complete Linked list from memory
  *
  *  @param ListHeader				Root List  pointer 
  * 
  * @retval OMX_ErrorNone   			 Success, memory freed
 *               OMX_ErrorUndefined		 Memory  failure
  **/
/*-------------------------------------------------------------------*/

OMX_ERRORTYPE CleanList(ListMember** ListHeader)
{
	int Error = OMX_ErrorNone;										/* No Error  */
	int ListCounter=0;
	ListMember* Temp;												/* Temporal pointer */
	ListCounter = (*ListHeader)->ListCounter;

	while (*ListHeader != NULL)
	{	
		APP_DPRINT("\nNum Instance to free %d \n",(*ListHeader)->ListCounter);
		Temp=(*ListHeader)->NextListMember;							
		if( (*ListHeader)->Struct_Ptr != NULL)						/* Ensure there is something to free */
		{
			APP_DPRINT(" Struct to free %p \n",(*ListHeader)->Struct_Ptr);
			free((*ListHeader)->Struct_Ptr);						/* Free memory */
			(*ListHeader)->Struct_Ptr = NULL;						/* Point to NULL */
		}
		else
		{
			APP_DPRINT("APP:  Warning: Content has alreadey been freed \n");
		}
		
		APP_DPRINT("freeing List Member %p \n",*ListHeader);
		free(*ListHeader);											/* Free member (first) */
		*ListHeader=Temp;											/* Asign Next member to header  */
	}
	if(*ListHeader== NULL)
	{
		APP_DPRINT("%d :: App: Freed the complete list: Header = %p \n",__LINE__,*ListHeader);
	}
	else
	{
		APP_EPRINT("%d :: App: Error Freeing List \n",__LINE__);
		Error =OMX_ErrorUndefined;	
	}
	return Error;
}


/*-------------------------------------------------------------------*/
/**
  * FreeListMember() Frees  a member from the Linked list with its member allocated memory
  *
  *  @param ListHeader				 Root List  pointer 
  * 
  * @retval OMX_ErrorNone   			 Success, member freed
 *              OMX_ErrorUndefined		 Memory  failure
  **/
/*-------------------------------------------------------------------*/

OMX_ERRORTYPE FreeListMember(void* ptr, ListMember** ListHeader)
{
	
	int Error = OMX_ErrorNone;													/* No Error  */
	int ListCounter=0;
	ListMember* Temp= NULL;															/* Temporal pointer */
	ListMember*	Backtemp=NULL;														/* follower of temporal pointer */
	ListCounter = (*ListHeader)->ListCounter;
	int count=0;

	Temp =(*ListHeader);							
	if (Temp != NULL)
	{
		while(Temp != NULL && Temp->Struct_Ptr != ptr)
		{	Backtemp = Temp;													/* One member back  */
			Temp = Temp->NextListMember;										/* next member */
			count++;
		}
		APP_DPRINT("Search ends: %p \n",Temp);
		if (Temp != NULL)														/* found it */
		{
			if (count > 0)														/* inside the List */
			{
				Backtemp->NextListMember = Temp->NextListMember;
				APP_DPRINT("About to free Content of List member: %p \n",Temp->Struct_Ptr);
				free(Temp->Struct_Ptr); 										/* free content */
				APP_DPRINT("About to free List member: %p Number: %d \n",Temp,Temp->ListCounter);
				free(Temp); 													/* free element */
			}
			else if (count == 0)												/* it was the first */
			{
					APP_DPRINT("About to free FIRST List member: %p \n",*ListHeader);
					Temp = Temp->NextListMember;								/* next member */
					free((*ListHeader)->Struct_Ptr);							/* free content */
					free(*ListHeader);											/* Free member (first) */
					*ListHeader=Temp;											/* Asign Next member to header  */
			}
		}
		else{																	/* Not found it */
			APP_DPRINT("Nothing to free \n");			
		}
		
	}
	else{
		APP_DPRINT("List is empty \n");
	}

	return Error;

}	


	

