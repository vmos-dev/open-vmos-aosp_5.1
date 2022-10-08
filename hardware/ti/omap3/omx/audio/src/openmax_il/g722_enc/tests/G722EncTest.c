
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
 * @file G722EncTest.c
 *
 * This File contains the G722 ENCODER OMX tests
 *
 * @path  $(OMAPSW_MPU)\linux\audio\src\openmax_il\g722_enc\tests
 *
 * @rev  0.1
 */
/* ----------------------------------------------------------------------------- 
 *! 
 *! Revision History 
 *! ===================================
 *! Date         Author(s)            Version  Description
 *! ---------    -------------------  -------  ---------------------------------
 *! 08-Mar-2007  A.Donjon             0.1      Code update for G722 ENCODER  
 *! 21-Mar-2007  A.Donjon             0.2      Test fwk change for pause and stop cmd
 *! 
 *!
 * ================================================================================= */
/*define OMX_GETTIME*/

#undef  WAITFORRESOURCES


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

FILE *fpRes = NULL;
FILE *outputDebugFile = NULL;
FILE *fOut= NULL, *fIn = NULL;
#undef APP_DEBUG
#undef APP_MEMCHECK

#define USE_BUFFER

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

#define G722ENC_NUM_INPUT_BUFFERS 1 /*Component default number of input buffers*/
#define G722ENC_NUM_OUTPUT_BUFFERS 1 /*Component default number of output buffers*/
#define G722ENC_MIN_INPUT_BUFFER_SIZE 16    /*G722ENC minimum input buffer size in bytes*/
#define G722ENC_MAX_INPUT_BUFFER_SIZE 320   /*G722ENC maximum input buffer size in bytes*/
#define G722ENC_MIN_OUTPUT_BUFFER_SIZE 4    /*G722ENC minimum output buffer size in bytes*/
#define G722ENC_MAX_OUTPUT_BUFFER_SIZE 80   /*G722ENC maximum output buffer size in bytes*/
#define SAMPLING_RATE   16000
#define SAMPLE_SIZE     16
#define G722ENC_CACHE_ALIGN_MALLOC 256
#define G722ENC_CACHE_ALIGN_OFFSET 128
#define strG722Encoder "OMX.TI.G722.encode"
#define FIFO1 "/dev/fifo.1"
#define FIFO2 "/dev/fifo.2"

#ifdef OMX_GETTIME
OMX_ERRORTYPE eError = OMX_ErrorNone;
int GT_FlagE = 0;  /* Fill Buffer 1 = First Buffer,  0 = Not First Buffer  */
int GT_FlagF = 0;  /*Empty Buffer  1 = First Buffer,  0 = Not First Buffer  */
static OMX_NODE* pListHead = NULL;
int Gt_k = 0 ;
#endif

int maxint(int a, int b);

int fill_data (OMX_BUFFERHEADERTYPE *pBuf, FILE *fIn);


int IpBuf_Pipe[2] = {0};
int OpBuf_Pipe[2] = {0};
int Event_Pipe[2] = {0};
int lastbuffer=0;
int nbInCbPending=0;

OMX_ERRORTYPE send_input_buffer (OMX_HANDLETYPE pHandle, OMX_BUFFERHEADERTYPE* pBuffer, FILE *fIn);

fd_set rfds;
int done = 0;
int whileloopdone = 0;

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
    int nCnt = 0;
    /*OMX_COMPONENTTYPE *pComponent = (OMX_COMPONENTTYPE *)pHandle;*/

    eError = OMX_GetState(pHandle, &CurState);
    while( (eError == OMX_ErrorNone) && (CurState != DesiredState)  ) {
        sched_yield();
        if(nCnt++ == 10) {
            APP_DPRINT( "Still Waiting, press CTL-C to continue\n");
        }

        eError = OMX_GetState(pHandle, &CurState);
    }

    if( eError != OMX_ErrorNone ) return eError;
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
OMX_ERRORTYPE EventHandler(OMX_HANDLETYPE hComponent,OMX_PTR pAppData,OMX_EVENTTYPE eEvent,
                           OMX_U32 nData1, OMX_U32 nData2, OMX_PTR pEventData)
{
    OMX_ERRORTYPE error = OMX_ErrorNone;
    OMX_U8 writeValue = 0;  

    switch (eEvent)
    {
    case OMX_EventCmdComplete:
        /* State change notification. Do Nothing */
        break;
    case OMX_EventError:
        /* Error notification */
        if(nData1==OMX_ErrorOverflow){
            printf("EventHandler: Overflow Error\n");
#ifdef OMX_GETTIME
            GT_START();
#endif
            error = OMX_SendCommand(hComponent,OMX_CommandStateSet, OMX_StateIdle, NULL);
            if(error != OMX_ErrorNone){ 
                APP_DPRINT ("%d :: Error from SendCommand-Idle(Stop) State function\n",__LINE__);
            }
            done = 1;
            error = WaitForState(hComponent, OMX_StateIdle);
#ifdef OMX_GETTIME
            GT_END("Call to SendCommand <OMX_StateIdle>");
#endif      
            if(error != OMX_ErrorNone) {
                APP_DPRINT( "Error:  G722Encoder->WaitForState reports an error %X\n", error);
                goto EXIT;
            }
        }else if(nData1 == OMX_ErrorResourcesPreempted) {
            writeValue = 0;  
            preempted = 1;
            write(Event_Pipe[1], &writeValue, sizeof(OMX_U8));
        }
        break;
    case OMX_EventMax:
    case OMX_EventMark:
    case OMX_EventPortSettingsChanged:
    case OMX_EventComponentResumed:
    case OMX_EventDynamicResourcesAvailable:
    case OMX_EventPortFormatDetected:
    case OMX_EventBufferFlag:
        writeValue = 2;  
        write(Event_Pipe[1], &writeValue, sizeof(OMX_U8));

    case OMX_EventResourcesAcquired:
        writeValue = 1;
        preempted = 0;
        write(Event_Pipe[1], &writeValue, sizeof(OMX_U8));

        break;
    } /* end of switch */
 EXIT:
    return OMX_ErrorNone;
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
    OMX_CALLBACKTYPE PcmCaBa = {(void *)EventHandler,
                                (void*)EmptyBufferDone,
                                (void*)FillBufferDone};
    OMX_HANDLETYPE pHandle = NULL;
    OMX_ERRORTYPE error = OMX_ErrorNone;
    OMX_U32 AppData = 100;
    OMX_PARAM_PORTDEFINITIONTYPE* pCompPrivateStruct = NULL;
    OMX_AUDIO_PARAM_ADPCMTYPE *pG722Param = NULL;
    OMX_COMPONENTTYPE *pComponent = NULL;
    OMX_AUDIO_CONFIG_VOLUMETYPE* pCompPrivateStructGain = NULL;
    /*OMX_STATETYPE state;*/
    OMX_BUFFERHEADERTYPE* pInputBufferHeader = NULL;
    OMX_BUFFERHEADERTYPE* pOutputBufferHeader = NULL;
    TI_OMX_DATAPATH dataPath;

#ifdef USE_BUFFER
    OMX_U8* pInputBuffer = NULL;
    OMX_U8* pOutputBuffer = NULL;
#endif
    TI_OMX_DSP_DEFINITION *pAppPrivate = NULL;
    int inBufSize = 0;
    int outBufSize = 0;
    OMX_S16 numOutputBuffers = 0;
    OMX_S16 numInputBuffers = 0;
    TI_OMX_STREAM_INFO *streaminfo = NULL;
    OMX_INDEXTYPE index = 0;
    OMX_STATETYPE testAppState = OMX_StateInvalid;

    struct timeval tv;
    int retval = 0, i = 0, j = 0;
    int frmCount = 0;
    int frmCount1 = 0;
    int frmCnt = 1;
    int testcnt = 1;
    int testcnt1 = 1;
    int nbDASFframes = 0;
    int fdmax = 0;
    int testCaseNo = 0;
#ifdef DSP_RENDERING_ON
    int g722encfdwrite = 0;
    int g722encfdread = 0;
#endif

    streaminfo = malloc(sizeof(TI_OMX_STREAM_INFO));
    pAppPrivate = malloc(sizeof(TI_OMX_DSP_DEFINITION));
    
    APP_DPRINT("------------------------------------------------------\n");
    APP_DPRINT("This is Main Thread In G722 ENCODER Test Application:\n");
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
#ifdef DSP_RENDERING_ON
    if((g722encfdwrite=open(FIFO1,O_WRONLY))<0) {
        APP_DPRINT("[G722TEST] - failure to open WRITE pipe\n");
    }
    else {
        APP_DPRINT("[G722TEST] - opened WRITE pipe\n");
    }

    if((g722encfdread=open(FIFO2,O_RDONLY))<0) {
        APP_DPRINT("[G722TEST] - failure to open READ pipe\n");
        goto EXIT;
    }
    else {
        APP_DPRINT("[G722TEST] - opened READ pipe\n");
    }
#endif

    APP_DPRINT("File = %s, Function = %s, Line = %d\n",__FILE__,__FUNCTION__,__LINE__);
    if((argc != 11)) {
        printf("    =========================================== \n");
        printf("Usage:  TestApp inFile outFile TestCaseNo nbInSamp nbOutSamp opMode nbInBuf nbOutBuf DASF nbFramesToEncode\n");
        printf("Example: G722EncTest speeche.pcm output.bit 1 160 40 0 1 1 1 1000\n");
        printf("opMode: 0=64kps 1=56kbps 2=48kbps\n");
        printf("DASF mode: 1=DASF 0=file-to-file mode\n");
        printf("nbFramesToEncode: number of frames to encode in DASF mode\n");
        goto EXIT;
    }

    /* check to see that the input file exists */
    struct stat sb = {0};
    int status = stat(argv[1], &sb);
    if( status != 0 ) {
        APP_DPRINT( "Cannot find file %s. (%u)\n", argv[1], errno);
        goto EXIT;
    }
    
    error = TIOMX_Init();
    if(error != OMX_ErrorNone) {
        APP_DPRINT("%d :: Error returned by TIOMX_Init()\n",__LINE__);
        goto EXIT;
    }   

    inBufSize = atoi(argv[4])<<1;  
    outBufSize = atoi(argv[5])<<1; 

    numInputBuffers = atoi(argv[7]);
    numOutputBuffers = atoi(argv[8]);
    
    nbDASFframes = atoi(argv[10]);
    if (atoi(argv[3]) > 0 && atoi(argv[3]) < 7) {
        switch (atoi(argv[3])) {
        case 1:
            printf ("--------------------------------------------------\n");
            printf ("Testing Simple RECORD till EOF or predefined frame number\n");
            printf ("--------------------------------------------------\n");
            break;
        case 2:
            printf ("--------------------------------------------------\n");
            printf ("Testing RECORD and STOP at predefined frame number\n");
            printf ("--------------------------------------------------\n");
            testCaseNo = 2;
            break;
        case 3:
            printf ("--------------------------------------------------\n");
            printf ("Testing PAUSE & RESUME Command                    \n");
            printf ("--------------------------------------------------\n");
            testCaseNo = 3;
            break;
        case 4:
            printf ("--------------------------------------------------\n");
            printf ("Testing Repeated RECORD without Deleting Component\n");
            printf ("--------------------------------------------------\n");
            testcnt = 15;
            break;
        case 5:
            printf ("--------------------------------------------------\n");
            printf ("Testing Repeated RECORD with Deleting Component   \n");
            printf ("--------------------------------------------------\n");
            testcnt1 = 15;
            break;
        case 6:
            printf ("--------------------------------------------------\n");
            printf ("Testing Set Volume for record stream \n");
            printf ("--------------------------------------------------\n");
            break;
        }
    }
    else {
        printf("Bad parameter for test case\n");
        goto EXIT;
    }
    
    for(j = 0; j < testcnt1; j++) {
        whileloopdone = 0;
        /* Create the handle of the G722 Encoder Component */           
        /*pHandle = malloc(sizeof(OMX_HANDLETYPE));*/
        APP_MEMPRINT("%d:::[TESTAPPALLOC] %p\n",__LINE__,pHandle);
#ifdef OMX_GETTIME
        GT_START();
        error = TIOMX_GetHandle(&pHandle, strG722Encoder, &AppData, &PcmCaBa);
        GT_END("Call to GetHandle");
#else 
        error = TIOMX_GetHandle(&pHandle, strG722Encoder, &AppData, &PcmCaBa);
#endif
        if((error != OMX_ErrorNone) || (pHandle == NULL)) {
            APP_DPRINT ("Error in Get Handle function\n");
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
            APP_DPRINT( "Error:Empty Data Pipe failed to open\n");
            goto EXIT;
        }

        /* save off the "max" of the handles for the selct statement */
        fdmax = maxint(IpBuf_Pipe[0], OpBuf_Pipe[0]);
        fdmax = maxint(fdmax,Event_Pipe[0]);

        if (atoi(argv[3]) == 5){
            printf ("%d :: Encoding the file [%d] Time\n",__LINE__, j+1);
        }

        
        if (atoi(argv[9]) == 0 || atoi(argv[9]) == 1) {
            pAppPrivate->dasfMode = atoi(argv[9]);
        }
        else {
            printf("Bad value for DASF mode\n");
            goto EXIT;
        }
        /* add to config TeeDN */
        pAppPrivate->teeMode = 0;
        pCompPrivateStruct = malloc (sizeof (OMX_PARAM_PORTDEFINITIONTYPE));
        APP_MEMPRINT("%d:::[TESTAPPALLOC] %p\n",__LINE__,pCompPrivateStruct);
        pCompPrivateStruct->nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
        pCompPrivateStruct->nVersion.s.nVersionMajor = 1;
        pCompPrivateStruct->nVersion.s.nVersionMinor = 1;
        pCompPrivateStruct->nPortIndex = OMX_DirInput;
        OMX_GetParameter (pHandle,OMX_IndexParamPortDefinition,pCompPrivateStruct);
              
        /* Send input port config */
        pCompPrivateStruct->eDir = OMX_DirInput;
        pCompPrivateStruct->bEnabled = OMX_TRUE;
        pCompPrivateStruct->nBufferCountMin = G722ENC_NUM_INPUT_BUFFERS;
        pCompPrivateStruct->nBufferSize = inBufSize;
        pCompPrivateStruct->format.audio.eEncoding = OMX_AUDIO_CodingADPCM;
        pCompPrivateStruct->nPortIndex = 0x0;
        pCompPrivateStruct->nBufferCountActual = G722ENC_NUM_INPUT_BUFFERS;
        pCompPrivateStruct->bEnabled = OMX_TRUE;    
#ifdef OMX_GETTIME
        GT_START();
        error = OMX_SetParameter (pHandle,OMX_IndexParamPortDefinition, pCompPrivateStruct);
        GT_END("Set Parameter Test-SetParameter");
#else
        error = OMX_SetParameter (pHandle,OMX_IndexParamPortDefinition, pCompPrivateStruct);
#endif
        if (error != OMX_ErrorNone) {
            error = OMX_ErrorBadParameter;
            APP_DPRINT ("%d :: OMX_ErrorBadParameter\n",__LINE__);
            goto EXIT;
        }

        /* Send output port config */
        pCompPrivateStruct->nPortIndex = OMX_DirOutput;
        pCompPrivateStruct->eDir = OMX_DirOutput;
        pCompPrivateStruct->bEnabled = OMX_TRUE;
        pCompPrivateStruct->nBufferCountMin = G722ENC_NUM_OUTPUT_BUFFERS;
        pCompPrivateStruct->nPortIndex = 0x1;
        pCompPrivateStruct->nBufferCountActual =  G722ENC_NUM_OUTPUT_BUFFERS;
        pCompPrivateStruct->bEnabled = OMX_TRUE;
        pCompPrivateStruct->nBufferSize = outBufSize;
#ifdef OMX_GETTIME
        GT_START();
        error = OMX_SetParameter (pHandle, OMX_IndexParamPortDefinition, pCompPrivateStruct);
        GT_END("Set Parameter Test-SetParameter");
#else
        error = OMX_SetParameter (pHandle, OMX_IndexParamPortDefinition, pCompPrivateStruct);
#endif
        if (error != OMX_ErrorNone) {
            error = OMX_ErrorBadParameter;
            APP_DPRINT ("%d :: OMX_ErrorBadParameter\n",__LINE__);
            goto EXIT;
        }

        pG722Param = malloc (sizeof (OMX_AUDIO_PARAM_ADPCMTYPE));
        APP_MEMPRINT("%d:::[TESTAPPALLOC] %p\n",__LINE__,pG722Param);
        pG722Param->nSize = sizeof (OMX_AUDIO_PARAM_ADPCMTYPE);
        pG722Param->nVersion.s.nVersionMajor = 1;
        pG722Param->nVersion.s.nVersionMinor = 1;
        pG722Param->nPortIndex = OMX_DirOutput;
        pG722Param->nChannels = 1;
        /* Codec rate selection */
        pG722Param->nSampleRate = atoi(argv[6]);
        if((atoi(argv[6])<0)||(atoi(argv[6])>2)){
            printf("Error in input arguments\n");
            printf("opMode: 0=64kps 1=56kbps 2=48kbps\n");
            printf("opMode set to 64kbps\n");
            pG722Param->nSampleRate = 0;
        }
        pG722Param->nBitsPerSample = SAMPLE_SIZE;
#ifdef OMX_GETTIME
        GT_START();
        error = OMX_SetParameter (pHandle, OMX_IndexParamAudioAdpcm, pG722Param);
        GT_END("Set Parameter Test-SetParameter");
#else
        error = OMX_SetParameter (pHandle, OMX_IndexParamAudioAdpcm, pG722Param);
#endif
        if (error != OMX_ErrorNone) {
            error = OMX_ErrorBadParameter;
            APP_DPRINT ("%d :: OMX_ErrorBadParameter\n",__LINE__);
            goto EXIT;
        }

        pG722Param->nPortIndex = OMX_DirInput;
        pG722Param->nSampleRate = SAMPLING_RATE;
#ifdef OMX_GETTIME
        GT_START();
        error = OMX_SetParameter (pHandle, OMX_IndexParamAudioAdpcm, pG722Param);
        GT_END("Set Parameter Test-SetParameter");
#else
        error = OMX_SetParameter (pHandle, OMX_IndexParamAudioAdpcm, pG722Param);
#endif
        if (error != OMX_ErrorNone) {
            error = OMX_ErrorBadParameter;
            APP_DPRINT ("%d :: OMX_ErrorBadParameter\n",__LINE__);
            goto EXIT;
        }     

        /* setting for stream gain */
        pCompPrivateStructGain = malloc (sizeof(OMX_AUDIO_CONFIG_VOLUMETYPE));
        if(pCompPrivateStructGain == NULL) 
        {
            APP_DPRINT("%d :: App: Malloc Failed\n",__LINE__);
            goto EXIT;
        }
        /* default setting for gain */
        pCompPrivateStructGain->nSize = sizeof(OMX_AUDIO_CONFIG_VOLUMETYPE);
        pCompPrivateStructGain->nVersion.s.nVersionMajor    = 0xF1;
        pCompPrivateStructGain->nVersion.s.nVersionMinor    = 0xF2;
        pCompPrivateStructGain->nPortIndex                  = OMX_DirOutput;
        pCompPrivateStructGain->bLinear                     = OMX_FALSE;
        pCompPrivateStructGain->sVolume.nValue              = 50;               /* actual volume */
        pCompPrivateStructGain->sVolume.nMin                = 0;                /* min volume */
        pCompPrivateStructGain->sVolume.nMax                = 100;              /* max volume */

        error = OMX_GetExtensionIndex(pHandle, "OMX.TI.index.config.g722headerinfo",&index);
        if (error != OMX_ErrorNone) {
            printf("Error getting extension index\n");
            goto EXIT;
        }    
        OMX_SetConfig(pHandle, index, pAppPrivate); 

#ifndef USE_BUFFER
        /* allocate input buffer */
        error = OMX_AllocateBuffer(pHandle,&pInputBufferHeader,0,NULL,inBufSize);
        if(error != OMX_ErrorNone) {
            APP_DPRINT("%d :: Error returned by OMX_AllocateBuffer()\n",__LINE__);
            goto EXIT;
        }
        /* allocate output buffer */
        error = OMX_AllocateBuffer(pHandle,&pOutputBufferHeader,1,NULL,outBufSize);
        if(error != OMX_ErrorNone) {
            APP_DPRINT("%d :: Error returned by OMX_AllocateBuffer()\n",__LINE__);
            goto EXIT;
        }
#else

        pInputBuffer = (OMX_U8*)malloc(inBufSize+G722ENC_CACHE_ALIGN_MALLOC);
        pInputBuffer = pInputBuffer+G722ENC_CACHE_ALIGN_OFFSET;
        APP_MEMPRINT("%d:::[TESTAPPALLOC] %p\n",__LINE__,pInputBuffer);
        /* allocate input buffer */
        error = OMX_UseBuffer(pHandle,&pInputBufferHeader,0,NULL,inBufSize,pInputBuffer);
        if(error != OMX_ErrorNone) {
            APP_DPRINT("%d :: Error returned by OMX_UseBuffer()\n",__LINE__);
            goto EXIT;
        }
        pOutputBuffer= (OMX_U8*)malloc (outBufSize+G722ENC_CACHE_ALIGN_MALLOC);
        pOutputBuffer = pOutputBuffer+G722ENC_CACHE_ALIGN_OFFSET;
        APP_MEMPRINT("%d:::[TESTAPPALLOC] %p\n",__LINE__,pOutputBuffer);
        /* allocate output buffer */
        error = OMX_UseBuffer(pHandle,&pOutputBufferHeader,1,NULL,outBufSize,pOutputBuffer);
        if(error != OMX_ErrorNone) {
            APP_DPRINT("%d :: Error returned by OMX_UseBuffer()\n",__LINE__);
            goto EXIT;
        }

#endif
        /* get streamID back to application */
        error = OMX_GetExtensionIndex(pHandle, "OMX.TI.index.config.g722streamIDinfo",&index);
        if (error != OMX_ErrorNone) {
            printf("Error getting extension index\n");
            goto EXIT;
        }

        error = OMX_GetConfig (pHandle, index, streaminfo);
        if(error != OMX_ErrorNone) {
            error = OMX_ErrorBadParameter;
            APP_DPRINT("%d :: G722EncTest.c :: Error from OMX_GetConfig() function\n",__LINE__);
            goto EXIT;
        }

#ifdef DSP_RENDERING_ON
        if((write(g722encfdwrite, &cmd_data, sizeof(cmd_data)))<0) {
        }
        if((read(g722encfdread, &cmd_data, sizeof(cmd_data)))<0) {
            goto EXIT;
        }
#endif

        error = OMX_SetConfig (pHandle, index, pAppPrivate);
        if(error != OMX_ErrorNone) {
            error = OMX_ErrorBadParameter;
            APP_DPRINT("%d :: Error from OMX_SetConfig() function\n",__LINE__);
            goto EXIT;
        }

        if (pAppPrivate->dasfMode) {
#ifdef RTM_PATH    
            dataPath = DATAPATH_APPLICATION_RTMIXER;
#endif

#ifdef ETEEDN_PATH
            dataPath = DATAPATH_APPLICATION;
#endif        
        }

        error = OMX_GetExtensionIndex(pHandle, "OMX.TI.index.config.g722.datapath",&index);
        if (error != OMX_ErrorNone) {
            printf("Error getting extension index\n");
            goto EXIT;
        }
        error = OMX_SetConfig (pHandle, index, &dataPath);
        if(error != OMX_ErrorNone) {
            error = OMX_ErrorBadParameter;
            goto EXIT;
        }
#ifdef OMX_GETTIME
        GT_START();
#endif
        error = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
        if(error != OMX_ErrorNone) {
            APP_DPRINT ("Error from SendCommand-Idle(Init) State function - error = %d\n",error);
            goto EXIT;
        }
        /* Wait for startup to complete */
        error = WaitForState(pHandle, OMX_StateIdle);
#ifdef OMX_GETTIME
        GT_END("Call to SendCommand <OMX_StateIdle>");
#endif      
        if(error != OMX_ErrorNone) {
            APP_DPRINT( "Error:  G722Encoder->WaitForState reports an error %X\n", error);
            goto EXIT;
        }
#ifdef OMX_GETTIME
        Gt_k = 0 ;
#endif
        for(i = 0; i < testcnt; i++) {
            whileloopdone = 0;
            frmCount1 = 0;
            frmCount = 0;
            nbInCbPending = 0;
 
            fIn = fopen(argv[1], "r");
            if(fIn == NULL) {
                fprintf(stderr, "Error:  failed to open the file %s for readonly access\n", argv[1]);
                goto EXIT;
            }

            fOut = fopen(argv[2], "w");
            if(fOut == NULL) {
                fprintf(stderr, "Error:  failed to create the output file \n");
                goto EXIT;
            }
        
            if(atoi(argv[3]) == 4) {
                printf ("%d :: Encoding the file [%d] Time\n",__LINE__, i+1);
            }

            done = 0;
            
            APP_DPRINT ("%d :: Sending OMX_StateExecuting Command\n",__LINE__);
#ifdef OMX_GETTIME
            GT_START();
#endif
            error = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
            if(error != OMX_ErrorNone) {
                APP_DPRINT ("Error from SendCommand-Executing State function\n");
                goto EXIT;
            }
            pComponent = (OMX_COMPONENTTYPE *)(pHandle);
            error = WaitForState(pHandle, OMX_StateExecuting);
#ifdef OMX_GETTIME
            GT_END("Call to SendCommand <OMX_StateExecuting>");
#endif          
            if(error != OMX_ErrorNone) {
                APP_DPRINT( "Error:  hAmrEncoder->WaitForState reports an error %X\n", error);
                goto EXIT;
            }

#ifdef OMX_GETTIME
            if (Gt_k==0)
            { 
                GT_FlagE=1;  /* 1 = First Buffer,  0 = Not First Buffer  */
                GT_START(); /* Fill Buffer */
            }
#endif  
            if (pAppPrivate->dasfMode == 0) {
                send_input_buffer(pHandle, pInputBufferHeader, fIn);
            }   
#ifdef OMX_GETTIME
            if (Gt_k==0)
            { 
                GT_FlagF=1;  /* 1 = First Buffer,  0 = Not First Buffer  */
                GT_START(); /* Fill Buffer */
            }
#endif     
            OMX_FillThisBuffer(pComponent,pOutputBufferHeader);
            OMX_GetState (pHandle, &testAppState);
#ifdef OMX_GETTIME
            Gt_k=1;
#endif      
    
#ifdef WAITFORRESOURCES
        if(1){
            while((error == OMX_ErrorNone) && (whileloopdone != 1)) {
#else
        while((error == OMX_ErrorNone) && (testAppState != OMX_StateIdle)) {
            if(1){
            
#endif
                if ((testAppState == OMX_StateIdle)&&(nbInCbPending==0)) {
                    whileloopdone = 1;
                }

                FD_ZERO(&rfds);
                FD_SET(IpBuf_Pipe[0], &rfds);
                FD_SET(OpBuf_Pipe[0], &rfds);
                FD_SET(Event_Pipe[0], &rfds);
                tv.tv_sec = 1;
                tv.tv_usec = 0;
                retval = select(fdmax+1, &rfds, NULL, NULL, &tv);
                if(retval == -1) {
                    perror("select()");
                    APP_DPRINT ("%d :: Error \n",__LINE__);
                    break;
                }

                if(retval == 0) {
                    APP_DPRINT ("%d :: BasicFn App Timeout !!!!!!!!!!! \n",__LINE__);
                }

                /* === input buffer === */
                switch (atoi(argv[3])){

                    case 1:     /* simple record till EOF or predefined frame number */
                    case 4:     /* repeated record without deleting component */
                    case 5:     /* repeated record with deleting component */
                        if(pAppPrivate->dasfMode == 0) { /* file-to-file mode */
                            if(FD_ISSET(IpBuf_Pipe[0], &rfds)){
                                OMX_BUFFERHEADERTYPE* pBuffer = NULL;
                                read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
                                nbInCbPending--;
                                frmCount++;
                                error = send_input_buffer (pHandle, pBuffer, fIn);
                                if (error != OMX_ErrorNone) {
                                    APP_DPRINT ("%d :: Error While reading input pipe\n",__LINE__);
                                    goto EXIT;
                                }
                            }
                        }
                        else{   /* DASF mode */
                            if((frmCount1 == nbDASFframes) && (done == 0)){
                                APP_DPRINT("Sending Idle Command - Line %d\n",__LINE__);
                            #ifdef OMX_GETTIME
                                GT_START();
                            #endif
                                error = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
                                if(error != OMX_ErrorNone){ 
                                    APP_DPRINT ("%d :: Error from SendCommand-Idle(Stop) State function\n",__LINE__);
                                    goto EXIT;
                                }
                                error = WaitForState(pHandle, OMX_StateIdle);
                            #ifdef OMX_GETTIME
                                GT_END("Call to SendCommand <OMX_StateIdle>");
                            #endif      
                                if(error != OMX_ErrorNone) {
                                    APP_DPRINT( "Error:  G722Encoder->WaitForState reports an error %X\n",error);
                                    goto EXIT;
                                    }           
                                done = 1;                           
                                APP_DPRINT("%d :: Shutting down ---------- \n",__LINE__);
                            }
                        
                        }
                        break;

                    case 2: /* Stop */
                        if(pAppPrivate->dasfMode == 0) {  /* file-to-file mode */
                            if(FD_ISSET(IpBuf_Pipe[0], &rfds)) {
                                if(frmCount1 > 149) {
                                    nbInCbPending--;    
                                    if(nbInCbPending == 0){
                                        APP_DPRINT("Sending Idle Command - Line %d\n",__LINE__);
                                    #ifdef OMX_GETTIME
                                        GT_START();
                                    #endif
                                        error = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateIdle, NULL);
                                        if(error != OMX_ErrorNone) {
                                            APP_DPRINT ("%d :: Error from SendCommand-Idle(Stop) State function\n",__LINE__);
                                            goto EXIT;
                                        }
                                        error = WaitForState(pHandle, OMX_StateIdle);
                                    #ifdef OMX_GETTIME
                                        GT_END("Call to SendCommand <OMX_StateIdle>");
                                    #endif
                                        if(error != OMX_ErrorNone) {
                                            APP_DPRINT( "Error:  G722Encoder->WaitForState reports an error %X\n", error);
                                            goto EXIT;
                                        }
                                        done = 1;
                                        APP_DPRINT("%d :: Shutting down ---------- \n",__LINE__);
                                    }
                                }
                                else{
                                    OMX_BUFFERHEADERTYPE* pBuffer = NULL;
                                    read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
                                    nbInCbPending--;    
                                    frmCount++;
                                    error = send_input_buffer (pHandle, pBuffer, fIn);
                                    if (error != OMX_ErrorNone) {
                                        APP_DPRINT ("%d :: Error While reading input pipe\n",__LINE__);
                                        goto EXIT;
                                    }
                                    frmCnt++;
                                }
                            }
                        }
                        else { /* DASF mode */
                            if(frmCount1 == 300) {
                                APP_DPRINT("Sending Idle Command - Line %d\n",__LINE__);
                            #ifdef OMX_GETTIME
                                GT_START();
                            #endif
                                error = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateIdle, NULL);
                                if(error != OMX_ErrorNone) {
                                    APP_DPRINT ("%d :: Error from SendCommand-Idle(Stop) State function\n",__LINE__);
                                    goto EXIT;
                                }
                                done = 1;
                                error = WaitForState(pHandle, OMX_StateIdle);
                            #ifdef OMX_GETTIME
                                GT_END("Call to SendCommand <OMX_StateIdle>");
                            #endif      
                                if(error != OMX_ErrorNone) {
                                    APP_DPRINT( "Error:  G722Encoder->WaitForState reports an error %X\n", error);
                                    goto EXIT;
                                }
                                APP_DPRINT("%d :: Shutting down ---------- \n",__LINE__);
                            }
                        }
                        break;

                    case 3: /* pause and resume */
                        /*if (frmCount == 320) {
                            APP_DPRINT ("%d :: Sending Resume command to Codec \n",__LINE__);
                        #ifdef OMX_GETTIME
                            GT_START();
                        #endif
                            error = OMX_SendCommand(pHandle, OMX_CommandStateSet,OMX_StateExecuting, NULL);
                            if(error != OMX_ErrorNone) {
                                APP_DPRINT ("%d :: Error from SendCommand-Executing State function\n",__LINE__);
                                goto EXIT;
                            }
                            APP_DPRINT("Calling Wait For State Line %d\n",__LINE__);
                            error = WaitForState(pHandle, OMX_StateExecuting);
                        #ifdef OMX_GETTIME
                            GT_END("Call to SendCommand <OMX_StateExecuting>");
                        #endif                      
                            APP_DPRINT("After Wait For State Line %d\n",__LINE__);
                            if(error != OMX_ErrorNone) {
                                APP_DPRINT("%d :: Error: hPcmEncoder->WaitForState reports an error\n",__LINE__);
                                goto EXIT;
                            }
                        }*/
                        if(frmCount == 300) {
                            APP_DPRINT ("%d :: Sending Pause command to Codec \n",__LINE__);
                        #ifdef OMX_GETTIME
                            GT_START();
                        #endif
                            error = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StatePause, NULL);
                            if(error != OMX_ErrorNone) {
                                APP_DPRINT ("%d :: Error from SendCommand-Pasue State function\n",__LINE__);
                                goto EXIT;
                            }
                            /* Wait for state to complete */
                            APP_DPRINT("Calling Wait For State Line %d\n",__LINE__);
                            error = WaitForState(pHandle, OMX_StatePause);
                        #ifdef OMX_GETTIME
                            GT_END("Call to SendCommand <OMX_StatePause>");
                        #endif
                            APP_DPRINT("After Wait For State Line %d\n",__LINE__);
                            if(error != OMX_ErrorNone) {
                                APP_DPRINT("%d :: Error: hPcmEncoder->WaitForState reports an error\n", __LINE__);
                                goto EXIT;
                            }
                            
                            puts("Sleeping for 3 secs...");
                            sleep(3);
                            puts("Resuming...");
                            error = OMX_SendCommand(pHandle, OMX_CommandStateSet,OMX_StateExecuting, NULL);
                            if(error != OMX_ErrorNone) {
                                APP_DPRINT ("%d :: Error from SendCommand-Executing State function\n",__LINE__);
                                goto EXIT;
                            }
                            APP_DPRINT("Calling Wait For State Line %d\n",__LINE__);
                            error = WaitForState(pHandle, OMX_StateExecuting);
                        #ifdef OMX_GETTIME
                            GT_END("Call to SendCommand <OMX_StateExecuting>");
                        #endif                      
                            APP_DPRINT("After Wait For State Line %d\n",__LINE__);
                            if(error != OMX_ErrorNone) {
                                APP_DPRINT("%d :: Error: hPcmEncoder->WaitForState reports an error\n",__LINE__);
                                goto EXIT;
                            }
                        }
                        if(pAppPrivate->dasfMode == 0) {   /* file-to-file mode */
                            if(FD_ISSET(IpBuf_Pipe[0], &rfds)) {
                                OMX_BUFFERHEADERTYPE* pBuffer = NULL;
                                read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
                                nbInCbPending--;
                                frmCount++;
                                error = send_input_buffer (pHandle, pBuffer, fIn);
                                if (error != OMX_ErrorNone) {
                                    APP_DPRINT ("%d :: Error While reading input pipe\n",__LINE__);
                                    goto EXIT;
                                }
                                frmCnt++;
                            }
                        }
                        else {  /* DASF mode */
                            if(frmCount1 == nbDASFframes) {
                                APP_DPRINT("Sending Idle Command - Line %d\n",__LINE__);
                            #ifdef OMX_GETTIME
                                GT_START();
                            #endif
                                error = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateIdle, NULL);
                                if(error != OMX_ErrorNone) {
                                    APP_DPRINT ("%d :: Error from SendCommand-Idle(Stop) State function\n",__LINE__);
                                    goto EXIT;
                                }
                                done = 1;
                                error = WaitForState(pHandle, OMX_StateIdle);
                            #ifdef OMX_GETTIME
                                GT_END("Call to SendCommand <OMX_StateIdle>");
                            #endif      
                                if(error != OMX_ErrorNone) {
                                    APP_DPRINT( "Error:  G722Encoder->WaitForState reports an error %X\n", error);
                                    goto EXIT;
                                    }
                                APP_DPRINT("%d :: Shutting down ---------- \n",__LINE__);
                            }
                        }
                        break;

                    case 6: /* Set Volume */
                        if(pAppPrivate->dasfMode == 0){ /* file-to-file mode */
                            APP_DPRINT("%d :: This test is not applied to file mode\n",__LINE__);
                            goto EXIT;
                        }
                        else{  /* DASF mode */
                            if(frmCount1 == 500){
                                /* set high gain for record stream */
                                APP_DPRINT("[G722 encoder] --- will set stream gain to high\n");
                                pCompPrivateStructGain->sVolume.nValue = 0x8000;
                                error = OMX_SetConfig(pHandle, OMX_IndexConfigAudioVolume, pCompPrivateStructGain);
                                if (error != OMX_ErrorNone) 
                                {
                                    error = OMX_ErrorBadParameter;
                                    goto EXIT;
                                }
                            }
                            if(frmCount1 == 800){
                                /* set low gain for record stream */
                                APP_DPRINT("[G722 encoder] --- will set stream gain to low\n");
                                pCompPrivateStructGain->sVolume.nValue = 0x1000;
                                error = OMX_SetConfig(pHandle, OMX_IndexConfigAudioVolume, pCompPrivateStructGain);
                                if (error != OMX_ErrorNone) 
                                {
                                    error = OMX_ErrorBadParameter;
                                    goto EXIT;
                                }
                                
                            }
                            
                            if(frmCount1 == nbDASFframes) {
                            printf("frmCount1 == nbDASFFrames\n\n");
                               APP_DPRINT("Sending Idle Command - Line %d\n",__LINE__);
                            #ifdef OMX_GETTIME
                                GT_START();
                            #endif
                               error = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
                               if(error != OMX_ErrorNone) {
                                   APP_DPRINT ("%d :: Error from SendCommand-Idle(Stop) State function\n",__LINE__);
                                   goto EXIT;
                               }
                                done = 1;
                                error = WaitForState(pHandle, OMX_StateIdle);
                            #ifdef OMX_GETTIME
                                GT_END("Call to SendCommand <OMX_StateIdle>");
                            #endif      
                                if(error != OMX_ErrorNone) {
                                    APP_DPRINT( "Error:  G722Encoder->WaitForState reports an error %X\n", error);
                                    goto EXIT;
                                    }               
                                APP_DPRINT("%d :: Shutting down ---------- \n",__LINE__);
                            }
                        }
                        break;


                        default:
                            APP_DPRINT("%d :: ### Running Simple DEFAULT Case Here ###\n",__LINE__);
                }

                /* === output buffer === */
                if(FD_ISSET(OpBuf_Pipe[0], &rfds)) {
                    OMX_BUFFERHEADERTYPE* pBuf = NULL;
                    read(OpBuf_Pipe[0], &pBuf, sizeof(pBuf));

                    if (pBuf->nAllocLen != pBuf->nFilledLen) {
                        APP_DPRINT("%d :: WARNING: Different Size, %ld\n", __LINE__, pBuf->nFilledLen);
                    }

                    fwrite(pBuf->pBuffer, 1, pBuf->nFilledLen, fOut);
                    fflush(fOut);

                    frmCount1++;
                    OMX_FillThisBuffer(pHandle, pBuf);
                   
                }
                
                if( FD_ISSET(Event_Pipe[0], &rfds) ) {
                    OMX_U8 pipeContents;
                    /*OMX_BUFFERHEADERTYPE* pBuffer;*/

                    read(Event_Pipe[0], &pipeContents, sizeof(OMX_U8));

                    if (pipeContents == 0) {
                        printf("Test app received OMX_ErrorResourcesPreempted\n");
                        WaitForState(pHandle,OMX_StateIdle);
                        error = OMX_FreeBuffer(pHandle,OMX_DirInput,pInputBufferHeader);
                        if( (error != OMX_ErrorNone)) {
                            APP_DPRINT ("%d:: Error in Free Handle function\n",__LINE__);
                            goto EXIT;
                        }
                        error = OMX_FreeBuffer(pHandle,OMX_DirOutput,pOutputBufferHeader);
                        if( (error != OMX_ErrorNone)) {
                            APP_DPRINT ("%d:: Error in Free Handle function\n",__LINE__);
                            goto EXIT;
                        }
#ifdef USE_BUFFER
                        /* free the App Allocated Buffers */
                        pInputBuffer = pInputBuffer-G722ENC_CACHE_ALIGN_OFFSET;
                        pOutputBuffer = pOutputBuffer-G722ENC_CACHE_ALIGN_OFFSET;
                        free(pOutputBuffer);
                        free(pInputBuffer);
                        pOutputBuffer = NULL;
                        pInputBuffer = NULL;
#endif
                        OMX_SendCommand(pHandle,OMX_CommandStateSet,OMX_StateLoaded,NULL);
                        WaitForState(pHandle,OMX_StateLoaded);
                        OMX_SendCommand(pHandle,OMX_CommandStateSet,OMX_StateWaitForResources,NULL);
                        WaitForState(pHandle,OMX_StateWaitForResources);
                    }
                    if (pipeContents == 1) {
#ifdef WAITFORRESOURCES
                        printf("Test app received OMX_ErrorResourcesAcquired\n");
                        OMX_SendCommand(pHandle,OMX_CommandStateSet,OMX_StateIdle,NULL);
                        /* allocate input buffer */
                        error = OMX_AllocateBuffer(pHandle,&pInputBufferHeader,0,NULL,inBufSize);
                        if(error != OMX_ErrorNone) {
                            APP_DPRINT("%d :: Error returned by OMX_AllocateBuffer()\n",__LINE__);
                            goto EXIT;
                        }

                        WaitForState(pHandle,OMX_StateIdle);
                        OMX_SendCommand(pHandle,OMX_CommandStateSet,OMX_StateExecuting,NULL);
                        WaitForState(pHandle,OMX_StateExecuting);
                        rewind(fIn);
                        send_input_buffer (pHandle, pBuffer, fIn);
#endif
                    }
                    if (pipeContents == 2) {

                        /* Send component to Idle */
                        OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateIdle, NULL);
                        WaitForState(pHandle, OMX_StateIdle); 

#ifdef WAITFORRESOURCES
                        error = OMX_FreeBuffer(pHandle,OMX_DirInput,pInputBufferHeader);
                        if( (error != OMX_ErrorNone)) {
                            APP_DPRINT ("%d:: Error in Free Handle function\n",__LINE__);
                            goto EXIT;
                        }
                        error = OMX_FreeBuffer(pHandle,OMX_DirOutput,pOutputBufferHeader);
                        if( (error != OMX_ErrorNone)) {
                            APP_DPRINT ("%d:: Error in Free Handle function\n",__LINE__);
                            goto EXIT;
                        }
#ifdef USE_BUFFER
                        /* free the App Allocated Buffers */
                        pInputBuffer = pInputBuffer-G722ENC_CACHE_ALIGN_OFFSET;
                        pOutputBuffer = pOutputBuffer-G722ENC_CACHE_ALIGN_OFFSET;
                        free(pOutputBuffer);
                        free(pInputBuffer);
                        pOutputBuffer = NULL;
                        pInputBuffer = NULL;
#endif

                        error = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateLoaded, NULL);
                        if(error != OMX_ErrorNone) {
                            APP_DPRINT ("%d Error from SendCommand-Idle State function\n",__LINE__);
                            printf("goto EXIT %d\n",__LINE__);
                            goto EXIT;
                        }
                        error = WaitForState(pHandle, OMX_StateLoaded);
#ifdef OMX_GETTIME
                        GT_END("Call to SendCommand <OMX_StateLoaded>");
#endif
                        if(error != OMX_ErrorNone) {
                            APP_DPRINT( "%d Error:  WaitForState reports an error %X\n",__LINE__, error);
                            printf("goto EXIT %d\n",__LINE__);
                            goto EXIT;
                        }
                        goto SHUTDOWN;
#endif
                    }                        
                }        
                
                /*if(done == 1) {
                    APP_DPRINT("Calling GetState Line %d\n",__LINE__);
                    error = OMX_GetState(pHandle, &state);
                    if(error != OMX_ErrorNone) {
                        APP_DPRINT("%d :: Warning:  OMX_GetState has returned status %X\n", __LINE__, error);
                        goto EXIT;
                    }
                }*/  

                OMX_GetState (pHandle, &testAppState);
                
            }
            else if (preempted){
                sched_yield();
            }
            else{
                goto SHUTDOWN;
            }            
        } /* While Loop Ending Here */ 
  
        APP_DPRINT ("%d :: The current state of the component = %d \n",__LINE__,state);
        fclose(fOut);
        fclose(fIn);
        fOut = NULL;
        fIn = NULL;
        frmCnt = 1;

        if((atoi(argv[3]) == 4) || (atoi(argv[3]) == 5)) {
            APP_DPRINT("Pause 6 seconds\n");
            /*sleep (6);*/
        } 
        else {
            sleep (0);
        }
    } /*Inner for loop ends here */

#ifndef WAITFORRESOURCES 
    /* free buffers */
    error = OMX_FreeBuffer(pHandle,OMX_DirInput,pInputBufferHeader);
    if( (error != OMX_ErrorNone)) {
        APP_DPRINT ("%d:: Error in Free Handle function\n",__LINE__);
        goto EXIT;
    }
    error = OMX_FreeBuffer(pHandle,OMX_DirOutput,pOutputBufferHeader);
    if( (error != OMX_ErrorNone)) {
        APP_DPRINT ("%d:: Error in Free Handle function\n",__LINE__);
        goto EXIT;
    }
#ifdef USE_BUFFER
    /* free the App Allocated Buffers */
    pInputBuffer = pInputBuffer-G722ENC_CACHE_ALIGN_OFFSET;
    pOutputBuffer = pOutputBuffer-G722ENC_CACHE_ALIGN_OFFSET;
    free(pOutputBuffer);
    free(pInputBuffer);
    pOutputBuffer = NULL;
    pInputBuffer = NULL;
#endif
#endif  
    APP_DPRINT ("%d :: Sending the OMX_StateLoaded Command\n",__LINE__);
#ifdef OMX_GETTIME
    GT_START();
#endif
    error = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateLoaded, NULL);
    APP_DPRINT ("%d :: Sent the OMX_StateLoaded Command\n",__LINE__);
    error = WaitForState(pHandle, OMX_StateLoaded); 
#ifdef OMX_GETTIME
    GT_END("Call to SendCommand <OMX_StateLoaded>");
#endif

#ifdef WAITFORRESOURCES
        error = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateWaitForResources, NULL);
        if(error != OMX_ErrorNone) {
            APP_DPRINT ("%d Error from SendCommand-Idle State function\n",__LINE__);
            printf("goto EXIT %d\n",__LINE__);
            goto EXIT;
        }
        error = WaitForState(pHandle, OMX_StateWaitForResources);

        /* temporarily put this here until I figure out what should really happen here */
        sleep(10);
        /* temporarily put this here until I figure out what should really happen here */
#endif    


    APP_DPRINT("%d :: Free the Component handle\n",__LINE__);
SHUTDOWN:
    /* Unload the G722 Encoder Component */
    error = TIOMX_FreeHandle(pHandle);
    if((error != OMX_ErrorNone)) {
        APP_DPRINT ("%d :: Error in Free Handle function\n",__LINE__);
        goto EXIT;
    }
    APP_DPRINT ("%d :: Free Handle returned Successfully \n",__LINE__);

    APP_MEMPRINT("%d:::[TESTAPPFREE] %p\n",__LINE__,pCompPrivateStruct);
    if(pCompPrivateStructGain != NULL) {
        free(pCompPrivateStructGain);
        pCompPrivateStructGain = NULL;
    }
    free(pCompPrivateStruct);
    pCompPrivateStruct = NULL;
    free(pG722Param);
    pG722Param = NULL;


    close(IpBuf_Pipe[0]);
    close(IpBuf_Pipe[1]);
    close(OpBuf_Pipe[0]);
    close(OpBuf_Pipe[1]);
    close(Event_Pipe[0]);
    close(Event_Pipe[1]);
    } /*Outer for loop ends here */

EXIT:

#ifdef DSP_RENDERING_ON
    if((write(g722encfdwrite, &cmd_data, sizeof(cmd_data)))<0)
        APP_DPRINT("%d ::- send command to audio manager\n",__LINE__);

    close(g722encfdwrite);
    close(g722encfdread);
#endif
    if(streaminfo){
        free(streaminfo);
        streaminfo = NULL;
    }
    free(pAppPrivate);
    pAppPrivate = NULL;

#ifdef OMX_GETTIME
    GT_END("G722_ENC test <End>");
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
/*    OMX_COMPONENTTYPE *pComponent = (OMX_COMPONENTTYPE *)pHandle;*/
    int nRead = fill_data (pBuffer, fIn);
    if((nRead == 0) && (done == 0)){
        done = 1;
        pBuffer->nFlags = OMX_BUFFERFLAG_EOS;
        pBuffer->nFilledLen = nRead;

        OMX_EmptyThisBuffer(pHandle, pBuffer);
        nbInCbPending++;
  
    }
    if(done==0){
        pBuffer->nFlags = 0;
        pBuffer->nFilledLen = nRead;
        if(!preempted){
            OMX_EmptyThisBuffer(pHandle, pBuffer);
            nbInCbPending++;
        }
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
    int nRead = 0;
    static int totalRead = 0;
    
    nRead = fread(pBuf->pBuffer, 1, pBuf->nAllocLen , fIn);
    totalRead += nRead;
    pBuf->nFilledLen = nRead;
    return nRead;
}
