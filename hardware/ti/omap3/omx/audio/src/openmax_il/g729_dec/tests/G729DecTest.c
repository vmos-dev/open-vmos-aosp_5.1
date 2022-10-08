
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
/* ================================================================================
 *             Texas Instruments OMAP(TM) Platform Software
 *  (c) Copyright Texas Instruments, Incorporated.  All Rights Reserved.
 *
 *  Use of this software is controlled by the terms and conditions found 
 *  in the license agreement under which this software has been supplied.
 * ================================================================================ */
/**
 * @file G729DecTest.c
 *
 * This File contains the G729 DECODER OMX tests
 *
 * @path  $(OMAPSW_MPU)\linux\audio\src\openmax_il\g729_dec\tests
 *
 * @rev  0.5
 */
/* ----------------------------------------------------------------------------- 
 *! 
 *! Revision History 
 *! ===================================
 *! Date         Author(s)            Version  Description
 *! ---------    -------------------  -------  ---------------------------------
 *! 03-Jan-2007  A.Donjon                         0.1      Code update for G729 DECODER
 *! 19-Feb-2007  A.Donjon                         0.2      Update for SN change for last frame
 *! 06-Apr-2007  A.Donjon                         0.3      USE_BUFFER
 *! 08-Jun-2007  A.Donjon                         0.4      Variable input buffer size
 *! 04-Jul-2007  A.Donjon                         0.5      Improved test app.                            
 *! 
 *!
 * ================================================================================= */
/****************************************************************
 *  INCLUDE FILES                                                 
 ****************************************************************/

/* ----- system and platform files ----------------------------*/
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
#include <linux/soundcard.h>
#include <time.h>

/*-------program files ----------------------------------------*/
#include <OMX_Index.h>
#include <OMX_Types.h>
#include <TIDspOmx.h>
#include <OMX_Core.h>
#include <OMX_Audio.h>
#include <G729DecTest.h>
#include <OMX_G729Decoder.h>

#ifdef OMX_GETTIME
#include <OMX_Common_Utils.h>
#include <OMX_GetTime.h>     /*Headers for Performance & measuremet    */
#endif

/* ------compilation control switches -------------------------*/
#undef APP_DEBUG
#undef APP_MEMCHECK
#undef APP_INFO
#undef USE_BUFFER

/* ======================================================================= */
/**
 * @def  DASF                           Define a Value for DASF mode 
 */
/* ======================================================================= */
#define DASF 1
/* ======================================================================= */
/**
 * @def  GAIN                      Define a GAIN value for Configure Audio
 */
/* ======================================================================= */
#define GAIN 95
/* ======================================================================= */
/**
 * @def    G729DEC_SAMPLING_FREQUENCY          Sampling Frequency   
 */
/* ======================================================================= */
#define G729DEC_SAMPLING_FREQUENCY 8000
/* ======================================================================= */
/**
 * @def    EXTRA_BUFFBYTES                Num of Extra Bytes to be allocated
 */
/* ======================================================================= */
#define EXTRA_BUFFBYTES (256)

/* ======================================================================= */
/**
 * @def  CACHE_ALIGNMENT                           Buffer Cache Alignment
 */
/* ======================================================================= */
#define CACHE_ALIGNMENT 128




#define FIFO1 "/dev/fifo.1"
#define FIFO2 "/dev/fifo.2"

/****************************************************************
 * EXTERNAL REFERENCES NOTE : only use if not found in header file
 ****************************************************************/
/*--------data declarations -----------------------------------*/
/*--------function prototypes ---------------------------------*/

/****************************************************************
 * PUBLIC DECLARATIONS Defined here, used elsewhere
 ****************************************************************/
/*--------data declarations -----------------------------------*/

/*--------function prototypes ---------------------------------*/

/****************************************************************
 * PRIVATE DECLARATIONS Defined here, used only here
 ****************************************************************/
/*--------data declarations -----------------------------------*/

#ifdef OMX_GETTIME
OMX_ERRORTYPE eError = OMX_ErrorNone;
int GT_FlagE = 0;  /* Fill Buffer 1 = First Buffer,  0 = Not First Buffer  */
int GT_FlagF = 0;  /*Empty Buffer  1 = First Buffer,  0 = Not First Buffer  */
static OMX_NODE* pListHead = NULL;
#endif


OMX_S16 inputPortDisabled = 0;
OMX_S16 outputPortDisabled = 0;
OMX_S8 InputCallbacksPending = 0;
OMX_S8 OutputLastPending = 0;

typedef enum COMPONENTS {
    COMP_1,
    COMP_2
}COMPONENTS;

OMX_STRING strG729Decoder = "OMX.TI.G729.decode";
int IpBuf_Pipe[2] = {0};
int OpBuf_Pipe[2] = {0};
fd_set rfds;
OMX_S16 dasfMode = 0;
OMX_S16 packetsPerBuffer = 0;
OMX_S16 EOFevent = 0;
OMX_BOOL bExitOnError = OMX_FALSE;
int command = 0;

#ifdef DSP_RENDERING_ON
AM_COMMANDDATATYPE cmd_data;
#endif

/*--------function prototypes ---------------------------------*/
OMX_S16 maxint(OMX_S16 a, OMX_S16 b);
OMX_ERRORTYPE StopComponent(OMX_HANDLETYPE *pHandle);
OMX_ERRORTYPE PauseComponent(OMX_HANDLETYPE *pHandle);
OMX_ERRORTYPE PlayComponent(OMX_HANDLETYPE *pHandle);
OMX_S16 fill_data_fromFile (OMX_BUFFERHEADERTYPE *pBuf, FILE *fIn, OMX_HANDLETYPE  pHandle);
void ConfigureAudio();
OMX_ERRORTYPE send_input_buffer (OMX_HANDLETYPE pHandle, OMX_BUFFERHEADERTYPE* pBuffer, FILE *fIn);

#ifdef USE_BUFFER
OMX_ERRORTYPE FreeResources(OMX_AUDIO_PARAM_G729TYPE* pG729Param, 
                            OMX_AUDIO_PARAM_PCMMODETYPE* pPcmParam,
                            OMX_PARAM_PORTDEFINITIONTYPE* pCompPrivateStruct, 
                            OMX_AUDIO_CONFIG_MUTETYPE* pCompPrivateStructMute, 
                            OMX_AUDIO_CONFIG_VOLUMETYPE* pCompPrivateStructVolume,
                            TI_OMX_DSP_DEFINITION* audioinfo, 
                            OMX_U8* pInputBuffer[10],
                            OMX_U8* pOutputBuffer[10],
                            G729DEC_BufParamStruct* pInBufferParam[10],
                            OMX_HANDLETYPE* pHandle);
#else
OMX_ERRORTYPE FreeResources(OMX_AUDIO_PARAM_G729TYPE* pG729Param, 
                            OMX_AUDIO_PARAM_PCMMODETYPE* pPcmParam,
                            OMX_PARAM_PORTDEFINITIONTYPE* pCompPrivateStruct, 
                            OMX_AUDIO_CONFIG_MUTETYPE* pCompPrivateStructMute, 
                            OMX_AUDIO_CONFIG_VOLUMETYPE* pCompPrivateStructVolume,
                            TI_OMX_DSP_DEFINITION* audioinfo, 
                            OMX_BUFFERHEADERTYPE* pInputBufferHeader[10],
                            OMX_BUFFERHEADERTYPE* pOutputBufferHeader[10],
                            G729DEC_BufParamStruct* pInBufferParam[10],
                            OMX_HANDLETYPE* pHandle);
#endif                                      


/*--------macros ----------------------------------------------*/
#ifdef APP_DEBUG
#define APP_DPRINT(...)    fprintf(stderr,__VA_ARGS__)
#else
#define APP_DPRINT(...)
#endif

#ifdef APP_INFO
#define DPRINT(...)    fprintf(stderr,__VA_ARGS__)
#else
#define DPRINT(...)
#endif

#ifdef APP_MEMCHECK
#define APP_MEMPRINT(...)    fprintf(stderr,__VA_ARGS__)
#else
#define APP_MEMPRINT(...)
#endif


/* safe routine to get the maximum of 2 integers */
OMX_S16 maxint(OMX_S16 a, OMX_S16 b)
{
    return (a>b) ? a : b;
}

#define OMX_G729APP_INIT_STRUCT(_s_, _name_)    \
    memset((_s_), 0x0, sizeof(_name_)); \

#define OMX_G729APP_MALLOC_STRUCT(_pStruct_, _sName_)                   \
    _pStruct_ = (_sName_*)malloc(sizeof(_sName_));                      \
    if(_pStruct_ == NULL){                                              \
        printf("***********************************\n");                \
        printf("%d :: Malloc Failed\n",__LINE__);                       \
        printf("***********************************\n");                \
        error = OMX_ErrorInsufficientResources;                         \
        goto EXIT;                                                      \
    }                                                                   \
    APP_MEMPRINT("%d :: ALLOCATING MEMORY = %p\n",__LINE__,_pStruct_);

/* This method will wait for the component to get to the state
 * specified by the DesiredState input. */
static OMX_ERRORTYPE WaitForState(OMX_HANDLETYPE* pHandle,
                                  OMX_STATETYPE DesiredState)
{
    OMX_STATETYPE CurState = OMX_StateInvalid;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_S16 nCnt = 0;
    OMX_COMPONENTTYPE *pComponent = (OMX_COMPONENTTYPE *)pHandle;
    eError = pComponent->GetState(pHandle, &CurState);

    while((eError == OMX_ErrorNone) && (CurState != DesiredState) && (eError == OMX_ErrorNone) ) {
        sleep(1);
        if(nCnt++ == 10) {
            APP_DPRINT( "Still Waiting, press CTL-C to continue\n");
        }
        eError = pComponent->GetState(pHandle, &CurState);

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

    OMX_COMPONENTTYPE *pComponent = (OMX_COMPONENTTYPE *)hComponent;
    OMX_STATETYPE state = OMX_StateInvalid;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
        
#ifdef APP_DEBUG
    int iComp = *((int *)(pAppData));
#endif

    eError = pComponent->GetState (hComponent, &state);
    if(eError != OMX_ErrorNone) {
        APP_DPRINT("%d :: App: Error returned from GetState\n",__LINE__);
    }
    switch (eEvent) {
    case OMX_EventCmdComplete:

        if(nData1 == OMX_CommandPortDisable){
            if (nData2 == OMX_DirInput) {
                inputPortDisabled = 1;
            }
            if (nData2 == OMX_DirOutput) {
                outputPortDisabled = 1;
            }
        }
        else if(nData1 == OMX_CommandStateSet){


        }
        break;

    case OMX_EventError:
        /* Error notification */
        if(nData1==OMX_ErrorOverflow){
            APP_DPRINT("EventHandler: WARNING: Overflow ERROR\n");
            /* Output buffer with sufficient allocated size must be sent to SN */
        }
        if(nData1==OMX_ErrorStreamCorrupt){
            APP_DPRINT("EventHandler: ERROR: Data corrupt ERROR\n");
            /* Corrupted input buffer parameters, component must be reseted or stopped */
        }
        break;
        break;
    case OMX_EventMax:
        break;
    case OMX_EventMark:
        break;

    default:
        break;
    }
    return eError;
}

void FillBufferDone (OMX_HANDLETYPE hComponent, OMX_PTR ptr, OMX_BUFFERHEADERTYPE* pBufferO)
{
    APP_DPRINT ("APP:::: OUTPUT BUFFER = %p && %p\n",pBufferO, pBufferO->pBuffer);
    APP_DPRINT ("APP:::: pBuffer->nFilledLen = %d\n",pBufferO->nFilledLen);
    write(OpBuf_Pipe[1], &pBufferO, sizeof(pBufferO));
    
#ifdef OMX_GETTIME
    if (GT_FlagF == 1 ) /* First Buffer Reply*/  /* 1 = First Buffer,  0 = Not First Buffer  */
    { 
        GT_END("Call to FillBufferDone  <First: FillBufferDone>");
        GT_FlagF = 0 ;   /* 1 = First Buffer,  0 = Not First Buffer  */
    }
#endif

}


void EmptyBufferDone(OMX_HANDLETYPE hComponent, OMX_PTR ptr, OMX_BUFFERHEADERTYPE* pBufferI)
{
    OMX_S16 ret = 0;
    APP_DPRINT ("APP:::: INPUT BUFFER = %p && %p\n",pBufferI, pBufferI->pBuffer);
    if (command == 0){
        APP_DPRINT("output: pBuffer->nTimeStamp = %d\n", (int)pBufferI->nTimeStamp);
        APP_DPRINT("output: pBuffer->nTickCount = %ld\n", pBufferI->nTickCount);
    }
    ret = write(IpBuf_Pipe[1], &pBufferI, sizeof(pBufferI));
    
#ifdef OMX_GETTIME
    if (GT_FlagE == 1 ) /* First Buffer Reply*/  /* 1 = First Buffer,  0 = Not First Buffer  */
    {
        GT_END("Call to EmptyBufferDone <First: EmptyBufferDone>");
        GT_FlagE = 0;   /* 1 = First Buffer,  0 = Not First Buffer  */
    }
#endif
}



OMX_S16 SendInputBuffer = 0;
OMX_S16 numInputBuffers = 0;
OMX_S16 numOutputBuffers = 0;
FILE *fp;

int main(int argc, char* argv[])
{
    OMX_CALLBACKTYPE G729CaBa = {(void *)EventHandler,
                                 (void*)EmptyBufferDone,
                                 (void*)FillBufferDone};                               
    OMX_HANDLETYPE pHandle;
    OMX_ERRORTYPE error = OMX_ErrorNone;
    OMX_U32 AppData = 100;
    OMX_PARAM_PORTDEFINITIONTYPE* pCompPrivateStruct = NULL; 
    OMX_AUDIO_PARAM_G729TYPE *pG729Param = NULL;
    OMX_AUDIO_PARAM_PCMMODETYPE *pPcmParam = NULL;
    OMX_COMPONENTTYPE *pComponent = NULL; 
    OMX_STATETYPE state = OMX_StateInvalid;
    OMX_BUFFERHEADERTYPE* pInputBufferHeader[10] = {NULL};
    OMX_BUFFERHEADERTYPE* pOutputBufferHeader[10] = {NULL};
    TI_OMX_DSP_DEFINITION *audioinfo = NULL;
    G729DEC_BufParamStruct* pInBufferParam[10] = {NULL};

#ifdef USE_BUFFER
    OMX_U8* pInputBuffer[10] = {NULL};
    OMX_U8* pOutputBuffer[10]= {NULL};
#endif
    
    struct timeval tv;
    OMX_S16 retval = 0, i = 0, j = 0,k = 0;
    OMX_S16 frmCount = 0;
    OMX_S16 OutBufCount = 0;
    OMX_S16 InBufCount = 0;
    OMX_S16 testcnt = 1;
    OMX_S16 testcnt1 = 1;
    OMX_BUFFERHEADERTYPE* pBuffer = NULL;
    OMX_BUFFERHEADERTYPE* pBuf = NULL;
    OMX_INDEXTYPE index = 0;  
    TI_OMX_DATAPATH dataPath;
    int g729decfdwrite = 0;
    int g729decfdread = 0;
    OMX_AUDIO_CONFIG_MUTETYPE* pCompPrivateStructMute = NULL; 
    OMX_AUDIO_CONFIG_VOLUMETYPE* pCompPrivateStructVolume = NULL; 

#ifdef MTRACE
    mtrace();
#endif

    bExitOnError = OMX_FALSE; 
    OMX_G729APP_MALLOC_STRUCT(audioinfo, TI_OMX_DSP_DEFINITION);
    OMX_G729APP_INIT_STRUCT(audioinfo, TI_OMX_DSP_DEFINITION);
    APP_DPRINT("------------------------------------------------------\n");
    APP_DPRINT("This is Main Thread In G729 DECODER Test Application:\n");
    APP_DPRINT("Test Core 1.5 - " __DATE__ ":" __TIME__ "\n");
    APP_DPRINT("------------------------------------------------------\n");

#ifdef OMX_GETTIME
    GTeError = OMX_ListCreate(&pListHead);
    printf("eError = %d\n",GTeError);
    GT_START();
#endif

#ifdef DSP_RENDERING_ON
    if((g729decfdwrite=open(FIFO1,O_WRONLY))<0) {
        printf("[G729TEST] - failure to open WRITE pipe\n");
    }
    else {
        printf("[G729TEST] - opened WRITE pipe\n");
    }

    if((g729decfdread=open(FIFO2,O_RDONLY))<0) {
        printf("[G729TEST] - failure to open READ pipe\n");
        bExitOnError = OMX_TRUE;
        goto EXIT;
    }
    else {
        printf("[G729TEST] - opened READ pipe\n");
    }
#endif

    /* check the input parameters */
    if(argc != 8) {
        printf( "Usage:  testApp infile outfile TestCaseNo DASFmode nbinbuf nboutbuf nbPacketsPerBuffer\n");
        printf("        DASFmode: FM or DM for File Mode or DASF Mode\n");
        bExitOnError = OMX_TRUE;
        goto EXIT;
    }

    numInputBuffers = atoi(argv[5]);
    numOutputBuffers = atoi(argv[6]);

    /* validate number of buffers input from command */
    if(numInputBuffers < 1 || numInputBuffers > 4)
    {
        printf("Please use between at least 1 but no more than 4 input buffers\n");
        bExitOnError = OMX_TRUE;
        goto EXIT;
    }
    if(numOutputBuffers < 1 || numOutputBuffers > 4)
    {
        printf("Please use between at least 1 but no more than 4 output buffers\n");
        bExitOnError = OMX_TRUE;
        goto EXIT;
    }
    APP_DPRINT( "Nb input buffers: %d, Nb output buffers: %d\n", numInputBuffers, numOutputBuffers);
    packetsPerBuffer = atoi(argv[7]);
    if((packetsPerBuffer>0)&&(packetsPerBuffer<7)){
        APP_DPRINT( "Nb packets per buffer: %d\n", packetsPerBuffer);
    }
    else{
        printf("Number of packets per buffer should be between 1 and 6\n");
        bExitOnError = OMX_TRUE;
        goto EXIT;
    }

    /* check to see that the input file exists */
    struct stat sb = {0};
    OMX_S16 status = stat(argv[1], &sb);
    if( status != 0 ) {
        APP_DPRINT( "Cannot find file %s. (%u)\n", argv[1], errno);
        bExitOnError = OMX_TRUE;
        goto EXIT;
    }

    /* Open the file of data to be decoded */
    FILE* fIn = fopen(argv[1], "r");
    fp =fopen(argv[1], "r");
    if( fIn == NULL ) {
        APP_DPRINT( "Error:  failed to open the file %s for readonly\access\n", argv[1]);
        bExitOnError = OMX_TRUE;
        goto EXIT;
    }

    FILE* fOut = NULL;
    /* Open the output file only in no DASF mode*/
    if(!(strcmp(argv[4],"FM"))){ 
        fOut = fopen(argv[2], "w");
        APP_DPRINT( "NO DASF MODE, created output file \n");
        if( fOut == NULL ) {
            APP_DPRINT( "Error:  failed to create the output file %s\n", argv[2]);
            bExitOnError = OMX_TRUE;
            goto EXIT;
        }
    }

    /* Create a pipe used to queue data from the callback. */
    retval = pipe(IpBuf_Pipe);
    if( retval != 0) {
        APP_DPRINT( "Error:Fill Data Pipe failed to open\n");
        bExitOnError = OMX_TRUE;
        goto EXIT;
    }

    retval = pipe(OpBuf_Pipe);
    if( retval != 0) {
        APP_DPRINT( "Error:Empty Data Pipe failed to open\n");
        bExitOnError = OMX_TRUE;
        goto EXIT;
    }

    /* save off the "max" of the handles for the selct statement */
    OMX_S16 fdmax = maxint(IpBuf_Pipe[0], OpBuf_Pipe[0]);

    APP_DPRINT("%d :: G729Test\n",__LINE__);
    error = TIOMX_Init();
    APP_DPRINT("%d :: G729Test\n",__LINE__);
    if(error != OMX_ErrorNone) {
        APP_DPRINT("%d :: Error returned by OMX_Init()\n",__LINE__);
        bExitOnError = OMX_TRUE;
        goto EXIT;
    }

    /* Test case number */
    command = atoi(argv[3]);
    APP_DPRINT("%d :: G729Test\n",__LINE__);
    switch (command ) {
    case 0:
        printf ("-------------------------------------\n");
        printf ("Testing Time stamp and Tick count \n");
        printf ("-------------------------------------\n");
        break;
    case 1:
        printf ("-------------------------------------\n");
        printf ("Testing Simple PLAY till EOF \n");
        printf ("-------------------------------------\n");
        break;
    case 2:
        printf ("-------------------------------------\n");
        printf ("Testing Stop and Play \n");
        printf ("-------------------------------------\n");
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
        testcnt = 15;
        break;
    case 6:
        printf ("------------------------------------------------\n");
        printf ("Testing Repeated PLAY with Deleting Component\n");
        printf ("------------------------------------------------\n");
        testcnt1 = 15;
        break;
    case 7:
        printf ("------------------------------------------------------------\n");
        printf ("Testing Mute/Unmute for Playback Stream\n");
        printf ("------------------------------------------------------------\n");
        break;
    case 8:
        printf ("------------------------------------------------------------\n");
        printf ("Testing Set Volume for Playback Stream\n");
        printf ("------------------------------------------------------------\n");
        break;
    default:
        printf("------------------------------------------------------------\n");
        printf("Wrong test case number. Valid test number from 1 to 8\n");
        bExitOnError = OMX_TRUE;
        goto EXIT;
    }

    if(!(strcmp(argv[4],"FM"))) {
        audioinfo->dasfMode = 0;
        dasfMode = 0;
        printf("NON DASF MODE\n");
    } 
    else if(!(strcmp(argv[4],"DM"))){
        audioinfo->dasfMode = 1;
        dasfMode = 1;
        printf("DASF MODE\n");
        
#if STATE_TRANSITION_STATE
        ConfigureAudio();
#endif

    } 
    else {
        printf("Enter proper DASF mode: \n");
        printf("Should be one of these modes: FM or DM for File Mode or DASF Mode\n");
        bExitOnError = OMX_TRUE;
        goto EXIT;
    }


    APP_DPRINT("%d :: G729Test\n",__LINE__);
    for(j = 0; j < testcnt1; j++) {
        if(j >= 1) {
            printf ("Decoding the file for %d Time\n",j+1);
            close(IpBuf_Pipe[0]);
            close(IpBuf_Pipe[1]);
            close(OpBuf_Pipe[0]);
            close(OpBuf_Pipe[1]);
                        

            /* Create a pipe used to queue data from the callback. */
            retval = pipe( IpBuf_Pipe);
            if( retval != 0) {
                APP_DPRINT( "Error:Fill Data Pipe failed to open\n");
                bExitOnError = OMX_TRUE;
                goto EXIT;
            }

            retval = pipe( OpBuf_Pipe);
            if( retval != 0) {
                APP_DPRINT( "Error:Empty Data Pipe failed to open\n");
                bExitOnError = OMX_TRUE;
                goto EXIT;
            }

            /* Open the input file to be decoded */
            fIn = fopen(argv[1], "r");
            fp= fopen(argv[1], "r");
            if( fIn == NULL ) {
                fprintf(stderr, "Error:  failed to open the file %s for readonly\
                                                                   access\n", argv[1]);
                bExitOnError = OMX_TRUE;
                goto EXIT;
            }

            /* Open the output file only in non DASF mode */
            if(audioinfo->dasfMode == 0){
                fOut = fopen(argv[2], "w");
                if( fOut == NULL ) {
                    fprintf(stderr, "Error:  failed to create the output file \n");
                    bExitOnError = OMX_TRUE;
                    goto EXIT;
                }
                error = TIOMX_Init(); 
            }

        }
 
        /* Load the G729 Decoder Component */
        APP_DPRINT("%d :: G729Test\n",__LINE__);
        
#ifdef OMX_GETTIME
        GT_START();
        error = OMX_GetHandle(&pHandle, strG729Decoder, &AppData, &G729CaBa);
        GT_END("Call to GetHandle");
#else 
        error = TIOMX_GetHandle(&pHandle, strG729Decoder, &AppData, &G729CaBa);
#endif

        APP_DPRINT("%d :: G729Test\n",__LINE__);
        if((error != OMX_ErrorNone) || (pHandle == NULL)) {
            APP_DPRINT ("Error in Get Handle function\n");
            bExitOnError = OMX_TRUE;
            goto EXIT;
        }

        APP_DPRINT("%d :: G729Test\n",__LINE__);
        OMX_G729APP_MALLOC_STRUCT(pCompPrivateStruct, OMX_PARAM_PORTDEFINITIONTYPE);
        OMX_G729APP_INIT_STRUCT(pCompPrivateStruct, OMX_PARAM_PORTDEFINITIONTYPE);
        /* set playback stream mute/unmute */ 
        OMX_G729APP_MALLOC_STRUCT(pCompPrivateStructMute, OMX_AUDIO_CONFIG_MUTETYPE); 
        OMX_G729APP_INIT_STRUCT(pCompPrivateStructMute, OMX_AUDIO_CONFIG_MUTETYPE); 
        OMX_G729APP_MALLOC_STRUCT(pCompPrivateStructVolume, OMX_AUDIO_CONFIG_VOLUMETYPE); 
        OMX_G729APP_INIT_STRUCT(pCompPrivateStructVolume, OMX_AUDIO_CONFIG_VOLUMETYPE); 
        


        pCompPrivateStruct->nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
        pCompPrivateStruct->nVersion.s.nVersionMajor = 0x1; 
        pCompPrivateStruct->nVersion.s.nVersionMinor = 0x2; 
        APP_DPRINT("%d :: G729Test\n",__LINE__);

        /* Send input port config */
        pCompPrivateStruct->eDir = OMX_DirInput; 
        pCompPrivateStruct->nPortIndex = OMX_DirInput; 
        pCompPrivateStruct->nBufferCountActual = numInputBuffers; 
        pCompPrivateStruct->nBufferSize = INPUT_G729DEC_BUFFER_SIZE*packetsPerBuffer; 
        pCompPrivateStruct->format.audio.eEncoding = OMX_AUDIO_CodingG729; 
        pCompPrivateStruct->bEnabled = 1;
        pCompPrivateStruct->bPopulated = 0; 
#ifdef OMX_GETTIME
        GT_START();
        error = OMX_SetParameter (pHandle,OMX_IndexParamPortDefinition,
                                  pCompPrivateStruct);
        GT_END("Set Parameter Test-SetParameter");
#else
        error = OMX_SetParameter (pHandle,OMX_IndexParamPortDefinition,
                                  pCompPrivateStruct);
#endif
        if (error != OMX_ErrorNone) {
            error = OMX_ErrorBadParameter;
            printf ("%d:: OMX_ErrorBadParameter\n",__LINE__);
            bExitOnError = OMX_TRUE;
            goto EXIT;
        }

        /* Send output port config */
        pCompPrivateStruct->nPortIndex = OMX_DirOutput; 
        pCompPrivateStruct->eDir = OMX_DirOutput; 
        pCompPrivateStruct->format.audio.eEncoding = OMX_AUDIO_CodingPCM; 
        pCompPrivateStruct->nBufferCountActual = numOutputBuffers; 
        pCompPrivateStruct->nBufferSize = (OUTPUT_G729DEC_BUFFER_SIZE)*packetsPerBuffer; 
        if(audioinfo->dasfMode == 1) {
            pCompPrivateStruct->nBufferCountActual = 0;
        }
    
#ifdef OMX_GETTIME
        GT_START();
        error = OMX_SetParameter (pHandle,OMX_IndexParamPortDefinition,
                                  pCompPrivateStruct);
        GT_END("Set Parameter Test-SetParameter");
#else
        error = OMX_SetParameter (pHandle,OMX_IndexParamPortDefinition,
                                  pCompPrivateStruct);
#endif

        if (error != OMX_ErrorNone) {
            error = OMX_ErrorBadParameter;
            printf ("%d:: OMX_ErrorBadParameter\n",__LINE__);
            bExitOnError = OMX_TRUE;
            goto EXIT;
        }
    
        /* default setting for Mute/Unmute */ 
        pCompPrivateStructMute->nSize = sizeof (OMX_AUDIO_CONFIG_MUTETYPE); 
        pCompPrivateStructMute->nVersion.s.nVersionMajor    = 0x1; 
        pCompPrivateStructMute->nVersion.s.nVersionMinor    = 0x1; 
        pCompPrivateStructMute->nPortIndex                  = OMX_DirInput; 
        pCompPrivateStructMute->bMute                       = OMX_FALSE; 
   
        /* default setting for volume */ 
        pCompPrivateStructVolume->nSize = sizeof(OMX_AUDIO_CONFIG_VOLUMETYPE); 
        pCompPrivateStructVolume->nVersion.s.nVersionMajor  = 0x1; 
        pCompPrivateStructVolume->nVersion.s.nVersionMinor  = 0x1; 
        pCompPrivateStructVolume->nPortIndex                = OMX_DirInput; 
        pCompPrivateStructVolume->bLinear                   = OMX_FALSE; 
        pCompPrivateStructVolume->sVolume.nValue            = 0x4000;             /*actual volume */ 
        pCompPrivateStructVolume->sVolume.nMin              = 0;                /* min volume */ 
        pCompPrivateStructVolume->sVolume.nMax              = 100;              /* max volume */ 

#ifndef USE_BUFFER      

        for (i=0; i < numInputBuffers; i++) {
            OMX_G729APP_MALLOC_STRUCT(pInBufferParam[i], G729DEC_BufParamStruct);
            OMX_G729APP_INIT_STRUCT(pInBufferParam[i], G729DEC_BufParamStruct);
            /* allocate input buffer */
            APP_DPRINT("%d :: About to call OMX_AllocateBuffer\n",__LINE__);
            error = OMX_AllocateBuffer(pHandle,
                                       &pInputBufferHeader[i], 0,
                                       pInBufferParam[i],
                                       INPUT_G729DEC_BUFFER_SIZE*packetsPerBuffer);
            APP_DPRINT("%d :: called OMX_AllocateBuffer\n",__LINE__);
            if(error != OMX_ErrorNone) {
                APP_DPRINT("%d :: Error returned by OMX_AllocateBuffer()\n",__LINE__);
                bExitOnError = OMX_TRUE;
                goto EXIT;
            }
        }
        for (i=0; i < numOutputBuffers; i++) {
            /* allocate output buffer */
            APP_DPRINT("%d :: About to call OMX_AllocateBuffer\n",__LINE__);
            error = OMX_AllocateBuffer(pHandle,&pOutputBufferHeader[i],1,NULL,(OUTPUT_G729DEC_BUFFER_SIZE)*packetsPerBuffer);
            APP_DPRINT("%d :: called OMX_AllocateBuffer\n",__LINE__);
            if(error != OMX_ErrorNone) {
                APP_DPRINT("%d :: Error returned by OMX_AllocateBuffer()\n",__LINE__);
                bExitOnError = OMX_TRUE;
                goto EXIT;
            }
        }

#else

        APP_DPRINT("%d :: About to call OMX_UseBuffer\n",__LINE__);
        /* numInputBuffers validated above to resolve Klockworks error */
        for (i=0; i < numInputBuffers; i++){
            OMX_G729APP_MALLOC_STRUCT(pInBufferParam[i], G729DEC_BufParamStruct);
            OMX_G729APP_INIT_STRUCT(pInBufferParam[i], G729DEC_BufParamStruct);
            pInputBuffer[i] = (OMX_U8*)malloc((INPUT_G729DEC_BUFFER_SIZE*packetsPerBuffer) + EXTRA_BUFFBYTES);
            memset(pInputBuffer[i] , 0x0, (INPUT_G729DEC_BUFFER_SIZE*packetsPerBuffer) + EXTRA_BUFFBYTES);
            APP_MEMPRINT("%d:::[TESTAPPALLOC] %p\n",__LINE__,pInputBuffer[i]);
            pInputBuffer[i] = pInputBuffer[i] + CACHE_ALIGNMENT;

            /* allocate input buffer */
            APP_DPRINT("%d :: About to call OMX_UseBuffer\n",__LINE__);
            error = OMX_UseBuffer(pHandle,&pInputBufferHeader[i],0,pInBufferParam[i],INPUT_G729DEC_BUFFER_SIZE*packetsPerBuffer,pInputBuffer[i]);
            APP_DPRINT("%d :: called OMX_UseBuffer\n",__LINE__);
            if(error != OMX_ErrorNone){
                APP_DPRINT("%d :: Error returned by OMX_UseBuffer()\n",__LINE__);
                bExitOnError = OMX_TRUE;
                goto EXIT;
            }
        }
        /* numInputBuffers validated above to resolve Klockworks error */
        for ( i = 0 ; i < numOutputBuffers ; i++ ){
            pOutputBuffer[i] = (OMX_U8*)malloc(((OUTPUT_G729DEC_BUFFER_SIZE)*packetsPerBuffer)+EXTRA_BUFFBYTES);
            memset(pOutputBuffer[i] , 0x0, (OUTPUT_G729DEC_BUFFER_SIZE*packetsPerBuffer) + EXTRA_BUFFBYTES);
            APP_MEMPRINT("%d:::[TESTAPPALLOC] %p\n",__LINE__,pOutputBuffer);
            pOutputBuffer[i] = pOutputBuffer[i] + CACHE_ALIGNMENT;
            /* allocate output buffer */
            APP_DPRINT("%d :: About to call OMX_UseBuffer\n",__LINE__);
            error = OMX_UseBuffer(pHandle,&pOutputBufferHeader[i],1,NULL,(OUTPUT_G729DEC_BUFFER_SIZE)*packetsPerBuffer,pOutputBuffer[i]);
            APP_DPRINT("%d :: called OMX_UseBuffer\n",__LINE__);
            if(error != OMX_ErrorNone){
                APP_DPRINT("%d :: Error returned by OMX_UseBuffer()\n",__LINE__);
                bExitOnError = OMX_TRUE;
                goto EXIT;
            }
        }
#endif

        OMX_G729APP_MALLOC_STRUCT(pG729Param, OMX_AUDIO_PARAM_G729TYPE);
        OMX_G729APP_INIT_STRUCT(pG729Param, OMX_AUDIO_PARAM_G729TYPE);
        pG729Param->nSize = sizeof (OMX_AUDIO_PARAM_G729TYPE);
        pG729Param->nVersion.s.nVersionMajor = 0xF1;
        pG729Param->nVersion.s.nVersionMinor = 0xF2;
        pG729Param->nPortIndex = OMX_DirInput;
        pG729Param->nChannels = 1;
        pG729Param->eBitType = OMX_AUDIO_G729AB;

#ifdef OMX_GETTIME
        GT_START();
        error = OMX_SetParameter (pHandle,OMX_IndexParamAudioG729,
                                  pG729Param);
        GT_END("Set Parameter Test-SetParameter");
#else
        error = OMX_SetParameter (pHandle,OMX_IndexParamAudioG729,
                                  pG729Param);
#endif

        if (error != OMX_ErrorNone) {
            error = OMX_ErrorBadParameter;
            printf ("%d:: OMX_ErrorBadParameter\n",__LINE__);
            bExitOnError = OMX_TRUE;
            goto EXIT;
        }
        OMX_G729APP_MALLOC_STRUCT(pPcmParam, OMX_AUDIO_PARAM_PCMMODETYPE);
        OMX_G729APP_INIT_STRUCT(pPcmParam, OMX_AUDIO_PARAM_PCMMODETYPE);
        pPcmParam->nPortIndex = OMX_DirOutput;
        pPcmParam->nChannels = 1;
#ifdef OMX_GETTIME
        GT_START();
        error = OMX_SetParameter (pHandle,OMX_IndexParamAudioPcm,
                                  pPcmParam);
        GT_END("Set Parameter Test-SetParameter");
#else
        error = OMX_SetParameter (pHandle,OMX_IndexParamAudioPcm,
                                  pPcmParam);
#endif
        if (error != OMX_ErrorNone) {
            error = OMX_ErrorBadParameter;
            printf ("%d:: OMX_ErrorBadParameter\n",__LINE__);
            bExitOnError = OMX_TRUE;
            goto EXIT;
        }

        /* get TeeDN or ACDN mode */
        audioinfo->acousticMode = OMX_FALSE;

        if (dasfMode) {
#ifdef RTM_PATH    
            dataPath = DATAPATH_APPLICATION_RTMIXER;
#endif

#ifdef ETEEDN_PATH
            dataPath = DATAPATH_APPLICATION;
#endif        
        }
 
        error = OMX_GetExtensionIndex(pHandle, "OMX.TI.index.config.g729headerinfo",&index);
        if (error != OMX_ErrorNone) {
            printf("Error getting extension index\n");
            bExitOnError = OMX_TRUE;
            goto EXIT;
        }

#ifdef DSP_RENDERING_ON
        cmd_data.hComponent = pHandle;
        cmd_data.AM_Cmd = AM_CommandIsOutputStreamAvailable;
        cmd_data.param1 = 0;
        if((write(g729decfdwrite, &cmd_data, sizeof(cmd_data)))<0) {
            printf("%d ::OMX_G729Decoder.c ::[G729 Dec Component] - send command to audio manager\n", __LINE__);
        }
        if((read(g729decfdread, &cmd_data, sizeof(cmd_data)))<0) {
            printf("%d ::OMX_G729Decoder.c ::[G729 Dec Component] - failure to get data from the audio manager\n", __LINE__);
            bExitOnError = OMX_TRUE;
            goto EXIT;
        }
        audioinfo->streamId = cmd_data.streamID;
#endif

        error = OMX_SetConfig (pHandle, index, audioinfo);
        if(error != OMX_ErrorNone) {
            error = OMX_ErrorBadParameter;
            APP_DPRINT("%d :: Error from OMX_SetConfig() function\n",__LINE__);
            bExitOnError = OMX_TRUE;
            goto EXIT;
            /* TODO: should be sure resources are cleaned up at any of the goto EXIT statements */
        }

        error = OMX_GetExtensionIndex(pHandle, "OMX.TI.index.config.g729.datapath",&index);
        if (error != OMX_ErrorNone) {
            printf("Error getting extension index\n");
            bExitOnError = OMX_TRUE;
            goto EXIT;
        }
    

        error = OMX_SetConfig (pHandle, index, &dataPath);
        if(error != OMX_ErrorNone) {
            error = OMX_ErrorBadParameter;
            APP_DPRINT("%d :: G729DecTest.c :: Error from OMX_SetConfig() function\n",__LINE__);
            bExitOnError = OMX_TRUE;
            goto EXIT;
        }

#ifdef OMX_GETTIME
        GT_START();
#endif

        error = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
        if(error != OMX_ErrorNone) {
            APP_DPRINT ("Error from SendCommand-Idle(Init) State function\n");
            bExitOnError = OMX_TRUE;
            goto EXIT;
        }
        /* Wait for startup to complete */
        error = WaitForState(pHandle, OMX_StateIdle);
#ifdef OMX_GETTIME
        GT_END("Call to SendCommand <OMX_StateIdle>");
#endif
        if(error != OMX_ErrorNone) {
            APP_DPRINT( "Error:  hG729Decoder->WaitForState reports an error %X\n", error);
            bExitOnError = OMX_TRUE;
            goto EXIT;
        }
        for(i = 0; i < testcnt; i++) { /* PROCESS LOOP */
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
                    bExitOnError = OMX_TRUE;
                    goto EXIT;
                }

                retval = pipe(OpBuf_Pipe);
                if( retval != 0) {
                    APP_DPRINT( "Error:Empty Data Pipe failed to open\n");
                    bExitOnError = OMX_TRUE;
                    goto EXIT;
                }

                /* Open the input file for decoding */
                fIn = fopen(argv[1], "r");
                fp= fopen(argv[1], "r");
                if(fIn == NULL) {
                    fprintf(stderr, "Error:  failed to open the file %s for readonly access\n", argv[1]);
                    bExitOnError = OMX_TRUE;
                    goto EXIT;
                }

                /* Open the output file only in non DASF mode */
                if(audioinfo->dasfMode == 0){
                    fOut = fopen(argv[2], "w");
                    if(fOut == NULL) {
                        fprintf(stderr, "Error:  failed to create the output file \n");
                        bExitOnError = OMX_TRUE;
                        goto EXIT;
                    }
                }
            }
            printf ("Basic Function:: Sending OMX_StateExecuting Command\n");
#ifdef OMX_GETTIME
            GT_START();
#endif
            error = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
            if(error != OMX_ErrorNone) {
                APP_DPRINT ("Error from SendCommand-Executing State function\n");
                bExitOnError = OMX_TRUE;
                goto EXIT;
            }
            pComponent = (OMX_COMPONENTTYPE *)pHandle;
            error = pComponent->GetState(pHandle, &state);
            error = WaitForState(pHandle, OMX_StateExecuting);
#ifdef OMX_GETTIME
            GT_END("Call to SendCommand <OMX_StateExecuting>");
#endif
            if(error != OMX_ErrorNone) {
                APP_DPRINT( "Error:  hG729Decoder->WaitForState reports an error %X\n", error);
                bExitOnError = OMX_TRUE;
                goto EXIT;
            }

            InputCallbacksPending = 0;
            for (k=0; k < numInputBuffers; k++) {            
                pInputBufferHeader[k]->nFlags = 0; 
                
#ifdef OMX_GETTIME
                if (k==0)
                { 
                    GT_FlagE=1;  /* 1 = First Buffer,  0 = Not First Buffer  */
                    GT_START(); /* Empty Bufffer */
                }
#endif

                error = send_input_buffer (pHandle, pInputBufferHeader[k], fIn);
            }   

            if (audioinfo->dasfMode == 0) {
                for (k=0; k < numOutputBuffers; k++) {
                    pOutputBufferHeader[k]->nFlags = 0;
                    
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
            error = pComponent->GetState(pHandle, &state);
            retval = 1;
            SendInputBuffer = 0;
            EOFevent = 0;
            frmCount = 0;
            OutBufCount = 0;
            InBufCount = 0;
            while( (error == OMX_ErrorNone) && ((state != OMX_StateIdle) || (retval>0))  ) {
                FD_ZERO(&rfds);
                FD_SET(IpBuf_Pipe[0], &rfds);
                FD_SET(OpBuf_Pipe[0], &rfds);
                tv.tv_sec = 1;
                tv.tv_usec = 0;
        
                retval = select(fdmax+1, &rfds, NULL, NULL, &tv);
                if(retval == -1) {
                    perror("select()");
                    printf ( " : Error \n");
                    break;
                }
                APP_DPRINT("Input Callbacks pending = %d, Output Last Pending = %d\n", InputCallbacksPending, OutputLastPending);
                if( (retval == 0)  &&  (InputCallbacksPending < 1) && (!OutputLastPending) ) {
                    APP_DPRINT ("%d :: BasicFn App Timeout !!!!!!!!!!! \n",__LINE__);       
                    fprintf(stderr, "Shutting down Since there is nothing else to send nor read---------- \n");
                    StopComponent(pHandle);
                }
                
                /* FREE input buffer */
                if(FD_ISSET(IpBuf_Pipe[0], &rfds)) {        
                    read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer)); 
                    InputCallbacksPending--;
                    InBufCount++;                
                    frmCount+=packetsPerBuffer;                                
                    APP_DPRINT("frame count = %d\n", frmCount);
                    if(pBuffer->nFlags==1){      /* Last input buffer received by App    */
                        InputCallbacksPending = 0;
                    }                  
                    if( ((2==command) || (4==command)) && (600 == frmCount)){ /*Stop Tests*/
                        fprintf(stderr, "Send STOP Command to component ---------- \n");
                        StopComponent(pHandle);
                    }  
                                        
                    pBuffer->nFlags = 0;

                    if (state == OMX_StateExecuting){
                        if(!EOFevent){
                            error = send_input_buffer (pHandle, pBuffer, fIn); 
                        }
                        else{
                            printf("EOF, not sending input\n");
                        }
                        if (error != OMX_ErrorNone) { 
                            bExitOnError = OMX_TRUE;
                            goto EXIT; 
                        } 
                    }
                           
                    if(3 == command){ /*Pause Test*/
                        if(frmCount == 100) {   /*100 Frames processed */
                            printf (" Sending Pause command to Codec \n");
                            PauseComponent(pHandle);
                            printf("5 secs sleep...\n");
                            sleep(5);
                            printf (" Sending Resume command to Codec \n");
                            PlayComponent(pHandle);
                        }                               
                    }
                    else if ( 7 == command ){       /*Mute and UnMuteTest*/
                        if(frmCount == 100){ 
                            printf("************Mute the playback stream*****************\n"); 
                            pCompPrivateStructMute->bMute = OMX_TRUE; 
                            error = OMX_SetConfig(pHandle, OMX_IndexConfigAudioMute, pCompPrivateStructMute); 
                            if (error != OMX_ErrorNone) 
                            { 
                                error = OMX_ErrorBadParameter; 
                                bExitOnError = OMX_TRUE;
                                goto EXIT; 
                            } 
                        } 
                        else if(frmCount == 400) { 
                            printf("************Unmute the playback stream*****************\n"); 
                            pCompPrivateStructMute->bMute = OMX_FALSE; 
                            error = OMX_SetConfig(pHandle, OMX_IndexConfigAudioMute, pCompPrivateStructMute); 
                            if (error != OMX_ErrorNone) { 
                                error = OMX_ErrorBadParameter; 
                                bExitOnError = OMX_TRUE;
                                goto EXIT; 
                            } 
                        }
                    }
                    else if ( 8 == command ) { /*Set Volume Test*/
                        if(frmCount == 600){
                            printf("************Set stream volume to high*****************\n"); 
                            pCompPrivateStructVolume->sVolume.nValue = 0x7500; 
                            error = OMX_SetConfig(pHandle, OMX_IndexConfigAudioVolume, pCompPrivateStructVolume); 
                            if (error != OMX_ErrorNone) { 
                                error = OMX_ErrorBadParameter; 
                                bExitOnError = OMX_TRUE;
                                goto EXIT; 
                            } 
                        }
                        else if(frmCount == 1200) { 
                            printf("************Set stream volume to low*****************\n"); 
                            pCompPrivateStructVolume->sVolume.nValue = 0x2500; 
                            error = OMX_SetConfig(pHandle, OMX_IndexConfigAudioVolume, pCompPrivateStructVolume); 
                            if (error != OMX_ErrorNone) { 
                                error = OMX_ErrorBadParameter; 
                                bExitOnError = OMX_TRUE;
                                goto EXIT; 
                            } 
                        }                      
                    }
                }

                if( FD_ISSET(OpBuf_Pipe[0], &rfds)) {     
                    read(OpBuf_Pipe[0], &pBuf, sizeof(pBuf));
                    APP_DPRINT("reading from output buffer pipe\n");
                    OutBufCount++;
                    if ((state != OMX_StateExecuting) && (pBuf->nFilledLen > 0)){
                        printf("Writing remaining output buffer\n");
                    }
                    APP_DPRINT ("FWRITE output buffer of size %d\n",pBuf->nFilledLen);
                    if(pBuf->nFlags!=1){
                        fwrite(pBuf->pBuffer, 1, pBuf->nFilledLen, fOut);
                        fflush(fOut);
                        if (state == OMX_StateExecuting ) {
                            pComponent->FillThisBuffer(pHandle, pBuf);                                         
                        }
                    }
                    else{/* Last output frame = dummy frame from DSP */
                        OutputLastPending = 0;
                        InputCallbacksPending = 0;
                        pBuf->nFlags = 0;
                    }
                }            
                error = pComponent->GetState(pHandle, &state);
                if(error != OMX_ErrorNone) {
                    printf("%d:: Warning:  hG729Decoder->GetState has returned status %X\n", __LINE__, error);
                    bExitOnError = OMX_TRUE;
                    goto EXIT;
                }
            } /* While Loop Ending Here */
            if(audioinfo->dasfMode == 0){
                fclose(fOut);
            }
            fclose(fIn);    
            fclose(fp);
            printf("Number of free input buffers received by test app. : %d\n",InBufCount);
            printf("Number of free output buffers received by test app. : %d\n",OutBufCount); 
            if((command == 2) || (( command == 5)&&(audioinfo->dasfMode == 0)) || (( command == 6)&&(audioinfo->dasfMode == 0))) {
                sleep (2);
            }
        } /*Inner for loop ends here */

                
        printf("Free buffers\n");
        /* free buffers */
        for (i=0; i < numInputBuffers; i++) {
            error = OMX_FreeBuffer(pHandle,OMX_DirInput,pInputBufferHeader[i]);
            if( (error != OMX_ErrorNone)) {
                APP_DPRINT ("%d:: Error in Free Handle function\n",__LINE__);
                bExitOnError = OMX_TRUE;
                goto EXIT;
            }
            if(pInBufferParam[i] != NULL){
                free(pInBufferParam[i]);
                pInBufferParam[i] = NULL;
            }
        }
        for (i=0; i < numOutputBuffers; i++) {
            error = OMX_FreeBuffer(pHandle,OMX_DirOutput,pOutputBufferHeader[i]);
            if( (error != OMX_ErrorNone)) {
                APP_DPRINT ("%d:: Error in Free Handle function\n",__LINE__);
                bExitOnError = OMX_TRUE;
                goto EXIT;
            }
        }

#ifdef USE_BUFFER
        /* free the App Allocated Buffers */
        printf("%d :: App: Freeing the App Allocated Buffers in TestApp\n",__LINE__);
        for(i=0; i < numInputBuffers; i++) {    
            pInputBuffer[i] = pInputBuffer[i] - CACHE_ALIGNMENT;
            APP_MEMPRINT("%d :: App: [TESTAPPFREE] pInputBuffer[%d] = %p\n",__LINE__,i,pInputBuffer[i]);
            if(pInputBuffer[i] != NULL){
                free(pInputBuffer[i]);
                pInputBuffer[i] = NULL;;
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
        printf ("Sending the StateLoaded Command\n");
                
#ifdef OMX_GETTIME
        GT_START();
#endif

        error = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateLoaded, NULL);
        error = WaitForState(pHandle, OMX_StateLoaded);
#ifdef OMX_GETTIME
        GT_END("Call to SendCommand <OMX_StateLoaded>");
#endif

        if(error != OMX_ErrorNone) {
            APP_DPRINT ("%d:: Error from SendCommand-Idle State function\n",__LINE__);
            bExitOnError = OMX_TRUE;
            goto EXIT;
        }
                
        error = OMX_SendCommand(pHandle, OMX_CommandPortDisable, -1, NULL);

        printf ("Free the Component handle\n");
        /* Unload the G729 Decoder Component */
        error = TIOMX_FreeHandle(pHandle);
        if( (error != OMX_ErrorNone)) {
            APP_DPRINT ("%d:: Error in Free Handle function\n",__LINE__);
            goto EXIT;
        }
        APP_DPRINT ("%d:: Free Handle returned Successfully \n\n\n\n",__LINE__);
        free(pG729Param);
        free(pPcmParam);
        free(pCompPrivateStruct);
        free(pCompPrivateStructMute);
        free(pCompPrivateStructVolume);
        close(IpBuf_Pipe[0]);
        close(IpBuf_Pipe[1]);
        close(OpBuf_Pipe[0]);
        close(OpBuf_Pipe[1]);
        APP_DPRINT("Freed resources successfully\n");
    } /*Outer for loop ends here */
    free(audioinfo);
            
    /* De-Initialize OMX Core */
    error = TIOMX_Deinit();
    if (error != OMX_ErrorNone) {
        printf("APP::Failed to de-init OMX Core!\n");
    }
            
#ifdef DSP_RENDERING_ON
    cmd_data.hComponent = pHandle;
    cmd_data.AM_Cmd = AM_Exit;
    if((write(g729decfdwrite, &cmd_data, sizeof(cmd_data)))<0)
    {
        printf("%d ::OMX_G729Decoder.c :: [G729 Dec Component] - send command to audio manager\n",__LINE__);
    }
    close(g729decfdwrite);
    close(g729decfdread);
#endif    

 EXIT:

    if (bExitOnError){
#ifdef USE_BUFFER    
        FreeResources(pG729Param, pPcmParam, pCompPrivateStruct, 
                      pCompPrivateStructMute, pCompPrivateStructVolume, 
                      audioinfo, pInputBuffer, pOutputBuffer, pInBufferParam, pHandle); 
#else

        FreeResources(pG729Param, pPcmParam, pCompPrivateStruct, 
                      pCompPrivateStructMute, pCompPrivateStructVolume, 
                      audioinfo, pInputBufferHeader, pOutputBufferHeader, pInBufferParam, pHandle); 
#endif
        error = TIOMX_FreeHandle(pHandle);
        if( (error != OMX_ErrorNone)) {
            APP_DPRINT ("%d:: Error in Free Handle function\n",__LINE__);
            goto EXIT;
        }
    }

#ifdef OMX_GETTIME
    GT_END("G729_DEC test <End>");
    OMX_ListDestroy(pListHead);     
#endif 

#ifdef MTRACE
    muntrace();
#endif

    return error;
}

OMX_ERRORTYPE send_input_buffer(OMX_HANDLETYPE pHandle, OMX_BUFFERHEADERTYPE* pBuffer, FILE *fIn)
{
    OMX_ERRORTYPE error = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pComponent = (OMX_COMPONENTTYPE *)pHandle;
    OMX_S16 status = 0;

    status = fill_data_fromFile (pBuffer, fIn, pHandle);
    pBuffer->nTimeStamp = (OMX_S64) rand() % 70;
    pBuffer->nTickCount = (OMX_S64) rand() % 70;
    if (command == 0){
        APP_DPRINT("SENDING TIMESTAMP = %d\n", (int) pBuffer->nTimeStamp);
        APP_DPRINT("SENDING TICK COUNT = %ld\n", pBuffer->nTickCount);
    }
    if(status>=0){
        SendInputBuffer++;
        InputCallbacksPending++;  
        pComponent->EmptyThisBuffer(pHandle, pBuffer);
    }
    else{
        error = OMX_ErrorStreamCorrupt;  
    } 
    return error;
}

 
/* ===========================================================================*/
/**
 * @fn fill_data_fromFile fills input buffer with 1 G729 frame from input test file
 * Conversion from ITU format to frame type header + G729 packet
 *
 */
/* ===========================================================================*/
OMX_S16 fill_data_fromFile (OMX_BUFFERHEADERTYPE *pBuf, FILE *fIn, OMX_HANDLETYPE  pHandle)
{
    OMX_S16 nRead = 0;
    OMX_S16 nRead2 = 0;
    OMX_S16 dRead = 0;
    OMX_S16 j = 0, n = 0, k = 0, m = 0;
    /* BFI + number of bit in frame + serial bitstream */
    OMX_S16 serial[ITU_INPUT_SIZE];
    /* G729 frame type */
    OMX_S16 frame_type = 0;
    /* Number of data bytes in packet */
    OMX_U32 nBytes = 0;
    /* G729 packet */
    OMX_U8 *packet = NULL;
    /* Offset in bytes in input buffer */
    OMX_U8 offset = 0;
    G729DEC_BufParamStruct* pBufStructTemp = (G729DEC_BufParamStruct*)pBuf->pInputPortPrivate; 


    pBufStructTemp->frameLost = 0;
    pBufStructTemp->numPackets = packetsPerBuffer;
    pBufStructTemp->bNoUseDefaults = OMX_TRUE;
    pBuf->nFilledLen = 0;
    pBuf->nFlags = 0;
    for(j = 0; j < packetsPerBuffer; j++){      /* nb packets in input buffer */

        nRead2=fread(serial, sizeof(OMX_S16), 2 , fp); //this is temporary
        /* read BFI and number of bits in frame */
        nRead = fread(serial, sizeof(OMX_S16), 2 , fIn);
        if(nRead != 0){
            /* Number of data bytes in packet */
            nBytes = serial[1]>>3;
            pBufStructTemp->packetLength[j] = nBytes + 1;
            pBuf->nFilledLen += pBufStructTemp->packetLength[j]; 
            /* read ITU serial bitstream  */
            dRead = fread(&serial[2], sizeof(OMX_S16), serial[1], fIn);
            if(dRead != serial[1]){
                printf("WARN: Error in input file\n");
                dRead = -1; /*error flag */
            }
            /* set frame type */
            switch(nBytes){
            case G729SPEECHPACKETSIZE:
                frame_type = SPEECH_FRAME_TYPE;
                break;
            case G729SIDPACKETSIZE:
                frame_type = SID_FRAME_TYPE;
                break;
            case NO_TX_FRAME_TYPE:
                frame_type = NO_TX_FRAME_TYPE;
                break;
            default:
                frame_type = ERASURE_FRAME;
            }
            if(serial[0]!= SYNC_WORD){  /* Untransmitted frame => Frame erasure flag */
                frame_type = ERASURE_FRAME;
            }       
            /* Add G729 frame type header to G729 input packet */
            *((OMX_U8 *)(&pBuf->pBuffer[0]+offset)) = frame_type;
             
            /* Convert ITU format to bitstream */
            packet = (OMX_U8 *)(&pBuf->pBuffer[0]+offset+1);
            if(frame_type == SPEECH_FRAME_TYPE){
                n = 2;
                k = 0;
                while(n<SPEECH_FRAME_SIZE){
                    packet[k] = 0;
                    for(m=7;m>=0;m--){
                        serial[n] = (~(serial[n]) & 0x2)>>1;
                        packet[k] = packet[k] + (serial[n]<<m);
                        n++;
                    }
                    k++;
                }
            }
            if(frame_type == SID_FRAME_TYPE){
                n = 2;
                k = 0;
                while(n<SID_OCTET_FRAME_SIZE){
                    packet[k] = 0;
                    for(m=7;m>=0;m--){
                        serial[n] = (~(serial[n]) & 0x2)>>1;
                        packet[k] = packet[k] + (serial[n]<<m);
                        n++;
                    }
                    k++;
                }
            }                       
            offset = offset + nBytes + 1;
        }
        else{
            if(offset == 0){/* End of file on a dummy frame */
                /* Set flag on input buffer to indicate Last Frame */
                pBuf->nFlags=OMX_BUFFERFLAG_EOS;
                /* Dummy buffer (no data) */
                pBuf->nFilledLen = 0;
                EOFevent = 1;
                printf("End of file on a dummy frame \n");
            }
            else{/* End of file on valid frame */
                pBuf->nFlags=OMX_BUFFERFLAG_EOS;
                EOFevent = 1;
                printf("End of file on a valid frame \n");
            }
            if(dasfMode==0){
                OutputLastPending = 1;
            } 
            j = packetsPerBuffer;  /* break */
        }
    }
    return dRead;
}

void ConfigureAudio()
{
    int Mixer = 0, arg = 0, status = 0;

    Mixer = open("/dev/sound/mixer", O_WRONLY);
    if (Mixer < 0) {
        perror("open of /dev/sound/mixer failed");
        exit(1);
    }
    arg = G729DEC_SAMPLING_FREQUENCY;          /* sampling rate */
    printf("Sampling freq set to:%d\n",arg);
    status = ioctl(Mixer, SOUND_PCM_WRITE_RATE, &arg);
    if (status == -1) {
        perror("SOUND_PCM_WRITE_RATE ioctl failed");
        printf("sample rate set to %u\n", arg);
    }
    arg = AFMT_S16_LE;                  /* AFMT_S16_LE or AFMT_S32_LE */
    status = ioctl(Mixer, SOUND_PCM_SETFMT, &arg);
    if (status == -1) {
        perror("SOUND_PCM_SETFMT ioctl failed");
        printf("Bitsize set to %u\n", arg);
    }
    arg = 2;                    /* Channels mono 1 stereo 2 */
    status = ioctl(Mixer, SOUND_PCM_WRITE_CHANNELS, &arg);
    if (status == -1) {
        perror("SOUND_PCM_WRITE_CHANNELS ioctl failed");
        printf("Channels set to %u\n", arg);
    }
    arg = GAIN<<8|GAIN;
    status = ioctl(Mixer, SOUND_MIXER_WRITE_VOLUME, &arg);
    if (status == -1) {
        perror("SOUND_MIXER_WRITE_VOLUME ioctl failed");
        printf("Volume set to %u\n", arg);
    }
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
    error = WaitForState(pHandle, OMX_StateIdle);
        
#ifdef OMX_GETTIME
    GT_END("Call to SendCommand <OMX_StateIdle>");
#endif

    if(error != OMX_ErrorNone) {
        fprintf(stderr, "\nError:  hG729Decoder->WaitForState reports an error %X!!!!!!!\n", error);
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
    error = WaitForState(pHandle, OMX_StatePause);
    
#ifdef OMX_GETTIME
    GT_END("Call to SendCommand <OMX_StatePause>");
#endif

    if(error != OMX_ErrorNone) {
        fprintf(stderr, "\nError:  hG729Decoder->WaitForState reports an error %X!!!!!!!\n", error);
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
    error = WaitForState(pHandle, OMX_StateExecuting);
#ifdef OMX_GETTIME
    GT_END("Call to SendCommand <OMX_StateExecuting>");
#endif
    if(error != OMX_ErrorNone) {
        fprintf(stderr, "\nError:  hG729Decoder->WaitForState reports an error %X!!!!!!!\n", error);
        goto EXIT;
    }
 EXIT:
    return error;
}

#ifdef USE_BUFFER
OMX_ERRORTYPE FreeResources(OMX_AUDIO_PARAM_G729TYPE* pG729Param, 
                            OMX_AUDIO_PARAM_PCMMODETYPE* pPcmParam,
                            OMX_PARAM_PORTDEFINITIONTYPE* pCompPrivateStruct, 
                            OMX_AUDIO_CONFIG_MUTETYPE* pCompPrivateStructMute, 
                            OMX_AUDIO_CONFIG_VOLUMETYPE* pCompPrivateStructVolume,
                            TI_OMX_DSP_DEFINITION* audioinfo, 
                            OMX_U8* pInputBuffer[10],
                            OMX_U8* pOutputBuffer[10],
                            G729DEC_BufParamStruct* pInBufferParam[10],
                            OMX_HANDLETYPE* pHandle)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    int i = 0;
    
    printf("Free buffers\n");
    /* free buffers */
    for (i=0; i < numInputBuffers; i++) {
        eError = OMX_FreeBuffer(pHandle,OMX_DirInput,pInputBufferHeader[i]);
        if( (eError != OMX_ErrorNone)) {
            APP_DPRINT ("%d:: Error in Free Handle function\n",__LINE__);
            goto EXIT;
        }
        if(pInBufferParam[i] != NULL){
            free(pInBufferParam[i]);
            pInBufferParam[i] = NULL;
        }
    }

    for (i=0; i < numOutputBuffers; i++) {
        eError = OMX_FreeBuffer(pHandle,OMX_DirOutput,pOutputBufferHeader[i]);
        if( (eError != OMX_ErrorNone)) {
            APP_DPRINT ("%d:: Error in Free Handle function\n",__LINE__);
            printf("%d:: Error in Free Handle function\n",__LINE__);
            goto EXIT;
        }
    }
    free(pG729Param);
    free(pPcmParam);
    free(pCompPrivateStruct);
    free(pCompPrivateStructMute);
    free(pCompPrivateStructVolume);
     
    close(IpBuf_Pipe[0]);
    close(IpBuf_Pipe[1]);
    close(OpBuf_Pipe[0]);
    close(OpBuf_Pipe[1]);
    free(audioinfo);
    
 EXIT:
    return eError;
}

#else

OMX_ERRORTYPE FreeResources(OMX_AUDIO_PARAM_G729TYPE* pG729Param, 
                            OMX_AUDIO_PARAM_PCMMODETYPE* pPcmParam,
                            OMX_PARAM_PORTDEFINITIONTYPE* pCompPrivateStruct, 
                            OMX_AUDIO_CONFIG_MUTETYPE* pCompPrivateStructMute, 
                            OMX_AUDIO_CONFIG_VOLUMETYPE* pCompPrivateStructVolume,
                            TI_OMX_DSP_DEFINITION* audioinfo, 
                            OMX_BUFFERHEADERTYPE* pInputBufferHeader[10],
                            OMX_BUFFERHEADERTYPE* pOutputBufferHeader[10],
                            G729DEC_BufParamStruct* pInBufferParam[10],
                            OMX_HANDLETYPE* pHandle)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    int i = 0;
    
    printf("Free buffers\n");
    /* free buffers */
    for (i=0; i < numInputBuffers; i++) {
        eError = OMX_FreeBuffer(pHandle,OMX_DirInput,pInputBufferHeader[i]);
        if( (eError != OMX_ErrorNone)) {
            APP_DPRINT ("%d:: Error in Free Handle function\n",__LINE__);
            goto EXIT;
        }
        if(pInBufferParam[i] != NULL){
            free(pInBufferParam[i]);
            pInBufferParam[i] = NULL;
        }
    }
    for (i=0; i < numOutputBuffers; i++) {
        eError = OMX_FreeBuffer(pHandle,OMX_DirOutput,pOutputBufferHeader[i]);
        if( (eError != OMX_ErrorNone)) {
            APP_DPRINT ("%d:: Error in Free Handle function\n",__LINE__);
            printf("%d:: Error in Free Handle function\n",__LINE__);
            goto EXIT;
        }
    }
    free(pG729Param);
    free(pPcmParam);
    free(pCompPrivateStruct);
    free(pCompPrivateStructMute);
    free(pCompPrivateStructVolume); 
    close(IpBuf_Pipe[0]);
    close(IpBuf_Pipe[1]);
    close(OpBuf_Pipe[0]);
    close(OpBuf_Pipe[1]);
    free(audioinfo);

 EXIT:
    return eError;
}
#endif                                      
