
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
 * @file OMX_G722DecTest.c
 *
 * This file contains the test application code that invokes the component.
 *
 * @path  $(CSLPATH)\OMAPSW_MPU\linux\audio\src\openmax_il\g722_dec\tests
 *
 * @rev  1.0
 */
/* ----------------------------------------------------------------------------
 *!
 *! Revision History
 *! ===================================
 *! This is newest file
 * =========================================================================== */
/* ------compilation control switches -------------------------*/
/*#define APP_DEBUG **/
/*#undef APP_DEBUG **/
/*undef OMX_GETTIME  */

/*#define G722DEC_DEBUG*/
/*#define APP_DEBUG*/
/*#define APP_STATEDETAILS*/
/*#define WAITFORRESOURCES*/
#undef USE_BUFFER
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
#include <signal.h>
#include <sys/stat.h>
#include <OMX_Index.h>
#include <OMX_Types.h>
#include <OMX_Core.h>
#include <OMX_Audio.h>
#include <TIDspOmx.h>
#include <pthread.h>
#include <linux/soundcard.h>
typedef enum {FALSE, TRUE} boolean;
#include <OMX_G722Dec_Utils.h>


#ifdef OMX_GETTIME
#include <OMX_Common_Utils.h>
#include <OMX_GetTime.h>     /*Headers for Performance & measuremet    */
#endif

#ifdef APP_DEBUG
#define APP_DPRINT(...)    fprintf(stderr,__VA_ARGS__)
#else
#define APP_DPRINT(...)     /*0j0 printf (__VA_ARGS__)*/
#endif

#ifdef APP_STATEDETAILS
#define APP_STATEPRINT(...)    fprintf(stderr,__VA_ARGS__)
#else
#define APP_STATEPRINT(...)    /*0j0 printf (__VA_ARGS__)*/
#endif

/* Constants */
#define APP_OUTPUT_FILE        "output" /*.pcm"*/
#define SLEEP_TIME             2
#define G722_APP_ID            100 /* Defines G722 Dec App ID, App must use this value */
#define INVALID_SAMPLING_FREQ  51  /*Do not change; fixed for component!*/
#define MAX_NUM_OF_BUFS        10  /*Do not change; fixed for component!*/
#define EOS_MIN_SIZE           2
#define FIFO1                  "/dev/fifo.1"
#define FIFO2                  "/dev/fifo.2"
#define STR_G722DECODER        "OMX.TI.G722.decode"

#ifdef APP_DEBUG
#define N_REPETITIONS 5
#else
#define N_REPETITIONS 20
#endif

/* undef OMX_GETTIME  */
#ifdef OMX_GETTIME
/* OMX_ERRORTYPE eError = OMX_ErrorNone;*/
int GT_FlagE = 0;  /* Fill Buffer 1 = First Buffer,  0 = Not First Buffer  */
int GT_FlagF = 0;  /*Empty Buffer  1 = First Buffer,  0 = Not First Buffer  */
static OMX_NODE* pListHead = NULL;
#endif

/*typedef enum {FALSE, TRUE} boolean;*/

/* Prototypes */
boolean              validateArguments  (int, char *[], int *, char *, int *, int *, int *, int *, int *, int *);
int                  maxint             (int a, int b);
static OMX_ERRORTYPE WaitForState       (OMX_HANDLETYPE *,       OMX_STATETYPE);
OMX_ERRORTYPE        EventHandler       (OMX_HANDLETYPE,         OMX_PTR,                OMX_EVENTTYPE, OMX_U32, OMX_U32, OMX_PTR);
void                 FillBufferDone     (OMX_HANDLETYPE,         OMX_PTR,                OMX_BUFFERHEADERTYPE *);
void                 EmptyBufferDone    (OMX_HANDLETYPE,         OMX_PTR,                OMX_BUFFERHEADERTYPE *);
OMX_ERRORTYPE        send_input_buffer  (OMX_HANDLETYPE,         OMX_BUFFERHEADERTYPE *, FILE *);
int                  validateArg4       (char *);
int           getNumTestCase            (char *, char *);
boolean       checkInputParameters      (int);
boolean       validateSampRate          (int);
boolean       omxSetInputPortParameter  (OMX_HANDLETYPE *, int, int);
boolean       omxSetOutputPortParameter (OMX_HANDLETYPE *, int, int, int);
boolean       g722SetInputPort          (OMX_HANDLETYPE *, int);
boolean       g722SetOutputPort         (OMX_HANDLETYPE *);
boolean       omxSetConfigMute          (OMX_HANDLETYPE *, OMX_BOOL);
boolean       omxSetConfigVolume        (OMX_HANDLETYPE *, OMX_S32);
OMX_ERRORTYPE omxUseBuffers             (OMX_HANDLETYPE *, int, int, int, OMX_BUFFERHEADERTYPE *[], int, int, OMX_BUFFERHEADERTYPE *[]);
OMX_ERRORTYPE testCases                 (OMX_HANDLETYPE *, fd_set *,int, FILE *, FILE *, int *, int *, struct timeval *, int, int, OMX_BUFFERHEADERTYPE *[], int, OMX_BUFFERHEADERTYPE *[]);
OMX_ERRORTYPE testCase_2_4              (OMX_HANDLETYPE *, int, FILE *, FILE *, int, OMX_BUFFERHEADERTYPE *[]);
OMX_ERRORTYPE testCase_3                (OMX_HANDLETYPE *);

OMX_ERRORTYPE sendInBuffFillOutBuff     (OMX_HANDLETYPE *, int, int, int, OMX_BUFFERHEADERTYPE *[],  FILE *, OMX_BUFFERHEADERTYPE *[]);
OMX_ERRORTYPE omxFreeBuffers            (OMX_HANDLETYPE *pHandle, int nBuffs, OMX_BUFFERHEADERTYPE *pBufferHeader  [], char *sBuffTypeMsg);

void          printTestCaseInfo         (int, char *);
void          getString_OMX_State       (char *ptrString, OMX_STATETYPE state);

/* Global variables */
int sendlastbuffer   = 0;
OMX_STATETYPE gState = OMX_StateExecuting;

int IpBuf_Pipe[2] = {0};
int OpBuf_Pipe[2] = {0};
int Event_Pipe[2] = {0};

int preempted = 0;

/* ================================================================================= * */
/**
 * @fn main() This is the main function of application which gets called.
 *
 * @param argc This is the number of commandline arguments..
 *
 * @param argv[] This is an array of pointers to command line arguments..
 *
 * @pre          None
 *
 * @post         None
 *
 *  @return      An integer value.
 *
 *  @see         None
 */
/* ================================================================================ * */
int main(int argc, char* argv[])
{
    OMX_CALLBACKTYPE              G722CaBa = {(void *)EventHandler,
                                              (void *)EmptyBufferDone,
                                              (void *)FillBufferDone
    };
    int                           nIpBuffs   = 1;
    int                           nOpBuffs   = 1;
    int                           nIpBufSize = 4096; /*default value */
    int                           nOpBufSize = 320; /* default value */
    int                           gDasfMode  = 0;
    OMX_HANDLETYPE               *pHandle                  = NULL;
    OMX_ERRORTYPE                 error                    = OMX_ErrorNone;
    OMX_U32                       AppData                  = G722_APP_ID;
    TI_OMX_DSP_DEFINITION         appPrivate;
    OMX_INDEXTYPE                 index     = 0;
    OMX_BUFFERHEADERTYPE         *pInputBufferHeader  [MAX_NUM_OF_BUFS] = {NULL};
    OMX_BUFFERHEADERTYPE         *pOutputBufferHeader [MAX_NUM_OF_BUFS] = {NULL};
    TI_OMX_DATAPATH               dataPath;
    fd_set                        rfds;
    static int                    totalFilled = 0;
    struct timeval                tv;
    int                           retval       = 0;
    int                           frmCnt       = 1;
    char                          fname [100]  = APP_OUTPUT_FILE;
    FILE*                         fIn          = NULL;
    FILE*                         fOut         = NULL;
    int                           tcID         = 0;
#ifdef DSP_RENDERING_ON
    int                           g722decfdwrite = 0;
    int                           g722decfdread  = 0;
#endif
    
#ifdef OMX_GETTIME
    OMX_ListCreate(&pListHead);
#endif  
    int iSampRate = 0;
    boolean bFlag = validateArguments (argc, argv, &iSampRate, fname, &tcID, &gDasfMode, &nIpBuffs, &nIpBufSize, &nOpBuffs, &nOpBufSize);    
    if (bFlag == FALSE) {
        printf ("<<<<<<<<< Argument validate fault >>>>>>>>>");
        exit (1);
    }
    int nIdx = 0, 
        nJdx = 0;
    int nCntTest1 = (tcID != 6)? 1: N_REPETITIONS;
    int nCntTest  = (tcID != 5)? 1: N_REPETITIONS;
#ifdef APP_DEBUG    
    char strState [20] = {""};
#endif    
    
    error = TIOMX_Init();
    if(error != OMX_ErrorNone) {
        APP_DPRINT("%d :: Error returned by OMX_Init()\n",__LINE__);
        exit (1);
    }

    for (nIdx = 0; nIdx < nCntTest1; nIdx++) {
        pHandle = malloc(sizeof(OMX_HANDLETYPE));
        if(pHandle == NULL){
            printf("%d :: App: Malloc Failed\n",__LINE__);
            exit (1);
        }

#ifdef DSP_RENDERING_ON
        if((g722decfdwrite = open(FIFO1,O_WRONLY))<0) {
            printf("[G722TEST] - failure to open WRITE pipe\n");
            exit (1);
        }
        printf("[G722TEST] - opened WRITE pipe\n");

        if((g722decfdread = open(FIFO2,O_RDONLY))<0) {
            printf("[G722TEST] - failure to open READ pipe\n");
            exit (1);
        }
        printf("[G722TEST] - opened READ pipe\n");


        if((write(g722decfdwrite, &cmd_data, sizeof(cmd_data)))<0) {
            printf("%d ::G722DecTest.c ::[G722 Dec Component] - send command to audio manager\n", __LINE__);
        }
        if((read(g722decfdread, &cmd_data, sizeof(cmd_data)))<0) {
            printf("%d ::G722DecTest.c ::[G722 Dec Component] - failure to get data from the audio manager\n", __LINE__);
            exit (1);
        }
#endif

#ifdef OMX_GETTIME
        GT_START();
        error = OMX_GetHandle(pHandle, STR_G722DECODER, &AppData, &G722CaBa);
        GT_END("Call to GetHandle");
#else 
        error = TIOMX_GetHandle(pHandle, STR_G722DECODER, &AppData, &G722CaBa);
#endif

        if((error != OMX_ErrorNone) || (*pHandle == NULL)) {
            APP_DPRINT ("Error in Get Handle function\n");
            exit (1);
        }

        /* Create a pipe used to queue data from the callback. */
        retval = pipe(IpBuf_Pipe);
        if( retval != 0) {
            printf( "Error:Fill In Buff Pipe failed to open\n");
            exit (1);
        }

        retval = pipe(OpBuf_Pipe);
        if( retval != 0) {
            printf( "Error:Empty Out Buff Pipe failed to open\n");
            exit (1);
        }

        retval = pipe(Event_Pipe);
        if( retval != 0) {
            printf( "Error:Empty Event Pipe failed to open\n");
            exit (1);
        }
       
        /* Send input port config */
        bFlag = omxSetInputPortParameter (pHandle, nIpBuffs, nIpBufSize);
        if (bFlag == FALSE) {
            exit (1);
        }

        /* Send output port config */
        bFlag = omxSetOutputPortParameter (pHandle, nOpBuffs, nOpBufSize, gDasfMode);
        if (bFlag == FALSE) {
            exit (1);
        }

        error = OMX_GetExtensionIndex(*pHandle, "OMX.TI.index.config.g722headerinfo",&index);
        if (error != OMX_ErrorNone) {
            printf("Error getting extension index\n");
            exit (1);
        }

        appPrivate.dasfMode = gDasfMode;

        error = OMX_SetConfig(*pHandle, index, &appPrivate);
        if(error != OMX_ErrorNone) {
            error = OMX_ErrorBadParameter;
            APP_DPRINT("%d :: Error from OMX_SetConfig() function\n",__LINE__);
            exit (1);
        }
#ifdef OMX_GETTIME
        GT_START();
#endif
        
        error = omxUseBuffers (pHandle, gDasfMode, nIpBuffs, nIpBufSize, pInputBufferHeader, nOpBuffs, nOpBufSize, pOutputBufferHeader);
        if (error != OMX_ErrorNone) {
            exit (1);
        }

        /* Send  G722 config for input */
        bFlag = g722SetInputPort (pHandle, iSampRate);
        if (bFlag == FALSE) {
            exit (1);
        }

        /* Send  G722 config for output */
        bFlag = g722SetOutputPort (pHandle);
        if (bFlag == FALSE) {
            exit (1);
        }         

        if(gDasfMode == 1) {
            APP_DPRINT("%d :: G722 DECODER RUNNING UNDER DASF MODE\n",__LINE__);

#ifdef RTM_PATH    
            dataPath = DATAPATH_APPLICATION_RTMIXER;
#endif
#ifdef ETEEDN_PATH
            dataPath = DATAPATH_APPLICATION;
#endif        
            error = OMX_GetExtensionIndex(*pHandle, "OMX.TI.index.config.g722dec.datapath",&index);
            if (error != OMX_ErrorNone) {
                printf("Error getting extension index\n");
                exit (1);
            }
            error = OMX_SetConfig (*pHandle, index, &dataPath);
            if(error != OMX_ErrorNone) {
                error = OMX_ErrorBadParameter;
                APP_DPRINT("%d :: G722DecTest.c :: Error from OMX_SetConfig() function\n",__LINE__);
                exit (1);
            }        
        }
        error = OMX_SendCommand(*pHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
        if(error != OMX_ErrorNone) {
            APP_DPRINT ("Error from SendCommand-Idle(Init) State function - error = %d\n",error);
            exit (1);
        }


        APP_DPRINT("%d :: App :: About to call WaitForState to change to Idle\n",__LINE__);
        error = WaitForState(pHandle, OMX_StateIdle);
        APP_DPRINT("%d :: App :: state changed to Idle\n",__LINE__);
        if(error != OMX_ErrorNone) {
            APP_DPRINT( "Error:  hAmrEncoder->WaitForState reports an error %X\n", error);
            exit (1);
        }

        for (nJdx = 0; nJdx < nCntTest; nJdx++) {
            if(gDasfMode == 0) {
                APP_DPRINT("%d :: G722 DECODER RUNNING UNDER FILE MODE\n",__LINE__);
                fOut = fopen(fname, "w");
                if(fOut == NULL) {
                    APP_DPRINT ( "Error:  failed to create the output file \n");
                    exit (1);
                }
                APP_DPRINT("%d :: Op File has created\n",__LINE__);
            }
            fIn = fopen(argv[1], "r");
            if(fIn == NULL) {
                fprintf(stderr, "Error:  failed to open the file %s for readonly access\n", argv[1]);
                exit (1);
            }

            if (tcID == 5) {
                printf("*********************************************************\n");
                printf ("App: %d time Sending OMX_StateExecuting Command: TC 5\n", nJdx);
                printf("*********************************************************\n");
            }
            if (tcID == 6) {
                printf("*********************************************************\n");
                printf("%d :: App: Outer %d time Sending OMX_StateExecuting Command: TC6\n",__LINE__, nIdx);
                printf("*********************************************************\n");
            }
            /* EOS_sent = OMX_FALSE; */
#ifdef OMX_GETTIME
            GT_START();
#endif
            error = OMX_SendCommand(*pHandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
            if(error != OMX_ErrorNone) {
                APP_DPRINT ("Error from SendCommand-Executing State function\n");
                exit (1);
            }

            APP_DPRINT("%d :: App :: About to call WaitForState to change to Executing\n",__LINE__);
            error = WaitForState(pHandle, OMX_StateExecuting);
            gState = OMX_StateExecuting;            
#ifdef OMX_GETTIME
            GT_START();
#endif
            if(error != OMX_ErrorNone) {
                APP_DPRINT( "Error:  WaitForState reports an error %X\n", error);
                exit (1);
            }
            APP_DPRINT("%d :: App :: state changed to Executing\n",__LINE__);

            error = sendInBuffFillOutBuff (pHandle, nIpBuffs, nOpBuffs, gDasfMode, pInputBufferHeader,  fIn, pOutputBufferHeader);
            if (error != OMX_ErrorNone) {
                exit (1);
            }

            while (1) {
#ifdef APP_DEBUG                
                getString_OMX_State (strState, gState);
#endif               
                
                if ((error == OMX_ErrorNone) && (gState != OMX_StateIdle)) {
                    error = testCases (pHandle, &rfds, tcID, fIn, fOut, &frmCnt, &totalFilled, &tv, gDasfMode, nIpBuffs, pInputBufferHeader, nOpBuffs, pOutputBufferHeader);
                    if (error != OMX_ErrorNone) {
                        printf ("<<<<<<<<<<<<<< testCases fault exit >>>>>>>>>>>>>>\n");
                        exit (1);
                    }
                } else if (preempted) {
                    sched_yield ();
                } else {
                    break;
                    /* goto SHUTDOWN */
                }
            } /* While Loop Ending Here */
            fclose(fIn);
            if(0 == gDasfMode) {
                fclose(fOut);
            }

            if(nIdx != (nCntTest - 1)) {
                if((tcID == 5) || (tcID == 2)) {
                    printf("%d :: sleeping here for %d secs\n",__LINE__,SLEEP_TIME);
                    sleep (SLEEP_TIME);
                }
            }
        }
#ifdef DSP_RENDERING_ON
        cmd_data.AM_Cmd = AM_Exit;
        if((write (g722decfdwrite, &cmd_data, sizeof(cmd_data)))<0)
            printf("%d :: [G722 Dec Component] - send command to audio manager\n",__LINE__);
        close(g722decfdwrite);
        close(g722decfdread);
#endif
        

        APP_DPRINT ("Sending the StateLoaded Command\n");
#ifdef OMX_GETTIME
        GT_START();
#endif
        error = OMX_SendCommand(*pHandle,OMX_CommandStateSet, OMX_StateLoaded, NULL);
        
        error = omxFreeBuffers (pHandle, nIpBuffs, pInputBufferHeader, "Input");
        if((error != OMX_ErrorNone)) {
            APP_DPRINT ("%d:: Error in Free Input Buffers function\n",__LINE__);
            exit (1);
        }
        if(gDasfMode == 0) {
            error = omxFreeBuffers (pHandle, nOpBuffs, pOutputBufferHeader, "Output");
            if((error != OMX_ErrorNone)) {
                APP_DPRINT ("%d:: Error in Free Output Buffers function\n",__LINE__);
                exit (1);
            }
        }
        error = WaitForState(pHandle, OMX_StateLoaded);                             
#ifdef OMX_GETTIME 
        GT_END("Call to SendCommand <OMX_StateLoaded>");
#endif
        if(error != OMX_ErrorNone) {
            APP_DPRINT ("%d:: Error from SendCommand-Idle State function\n",__LINE__);
            exit (1);
        }

#ifdef WAITFORRESOURCES
        error = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateWaitForResources, NULL);
        if(error != OMX_ErrorNone) {
            APP_DPRINT ("%d :: AmrDecTest.c :: Error from SendCommand-Idle State function\n",__LINE__);
            printf("goto EXIT %d\n",__LINE__);

            goto EXIT;
        }
        error = WaitForState(pHandle, OMX_StateWaitForResources);

        /* temporarily put this here until I figure out what should really happen here */
        sleep(10);
#endif    

        error = OMX_SendCommand(*pHandle, OMX_CommandPortDisable, -1, NULL);


#ifdef DSP_RENDERING_ON
        if((write(g722decfdwrite, &cmd_data, sizeof(cmd_data)))<0)
            APP_DPRINT ("%d [G722Dec Test] - send command to audio manager\n",__LINE__);
#endif
        
        error = TIOMX_FreeHandle(*pHandle);
        if( (error != OMX_ErrorNone)) {
            APP_DPRINT ("%d:: Error in Free Handle function\n",__LINE__);
            exit (1);
        } 
        APP_DPRINT ("%d:: Free Handle returned Successfully \n\n\n\n",__LINE__);
        if (pHandle) {
            free(pHandle);
        }

        close (Event_Pipe[0]);
        close (Event_Pipe[1]);
        close (IpBuf_Pipe[0]);
        close (IpBuf_Pipe[1]);
        close (OpBuf_Pipe[0]);
        close (OpBuf_Pipe[1]);
        
        sendlastbuffer = 0;
    }

    error = TIOMX_Deinit();
    if( (error != OMX_ErrorNone)) {
        APP_DPRINT("APP: Error in Deinit Core function\n");
    }
    
    if (gDasfMode == 0) {
        printf("**********************************************************\n");
        printf("NOTE: An output file %s has been created in file system\n",APP_OUTPUT_FILE);
        printf("**********************************************************\n");
    }
#ifdef OMX_GETTIME
    GT_END("G722_DEC test <End>");
    OMX_ListDestroy(pListHead); 
#endif
    exit (0);
} /* end main () */

/* ================================================================================= * */
/**
 * @fn validateArguments() This is the main function of application which gets called.
 *
 */
/* ================================================================================ * */
boolean validateArguments (int nArgc, char *sArgv [], int *iSampRate, char *fname, int *tcID, int *gDasfMode, int *nIpBuffs, int *nIpBufSize, int *nOpBuffs, int *nOpBufSize)
{
    APP_DPRINT("------------------------------------------------------\n");
    APP_DPRINT("This is Main Thread In G722 DECODER Test Application:\n");
    APP_DPRINT("Test Core 1.5 - " __DATE__ ":" __TIME__ "\n");
    APP_DPRINT("------------------------------------------------------\n");

    /* check the input parameters */
    if (checkInputParameters (nArgc) == FALSE) {
        return (FALSE);
    }

    /* check to see that the input file exists */
    struct stat sb = {0};
    int status = stat(sArgv[1], &sb);
    if( status != 0 ) {
        printf( "Cannot find file %s. (%u)\n", sArgv[1], errno);
        printf ("Test not performed!!!!!!\n\n");
        return (FALSE);
    }

    int nTmp = atoi(sArgv[2]);
    if (validateSampRate (nTmp) == FALSE) {
        return (FALSE);
    }
    *iSampRate = nTmp;

    *tcID = getNumTestCase (sArgv [3], fname);
    if (*tcID == 0) {
        printf("Invalid Test Case ID: exiting...\n");
        return (FALSE);
    }
    
    *gDasfMode = validateArg4 (sArgv [4]);
    if (*gDasfMode == -1) {
        return (FALSE);
    }

    *nIpBuffs = atoi(sArgv[5]);
    APP_DPRINT("%d :: App: nIpBuf = %d\n",__LINE__, *nIpBuffs);
    *nIpBufSize = atoi(sArgv[6]);
    APP_DPRINT("%d :: App: nIpBufSize = %d\n",__LINE__, *nIpBufSize);
    *nOpBuffs = atoi(sArgv[7]);
    APP_DPRINT("%d :: App: nOpBuf = %d\n",__LINE__, *nOpBuffs);
    *nOpBufSize = atoi(sArgv[8]);
    APP_DPRINT("%d :: App: nOpBufSize = %d\n",__LINE__, *nOpBufSize);

    printf ("Sample rate:\t\t %d\n", *iSampRate);
    printf ("Test case ID:\t\t %d\n", *tcID);
    printf ("DasfMode:\t\t %d\n", *gDasfMode);
    printf ("# of Input Buffers:\t %d\n", *nIpBuffs);
    printf ("Size of Input Buffers:\t %d\n", *nIpBufSize);
    printf ("# of Output Buffers:\t %d\n", *nOpBuffs);
    printf ("Size of Output Buffers:\t %d\n", *nOpBufSize);
    return (TRUE);
}
/**/
/* ================================================================================= * */
/**
 * @fn maxint() gives the maximum of two integers.
 *
 * @param a intetger a
 *
 * @param b integer b
 *
 * @pre          None
 *
 * @post         None
 *
 *  @return      bigger number
 *
 *  @see         None
 */
/* ================================================================================ * */
int maxint(int a, int b)
{
    return (a>b) ? a : b;
}

/* ================================================================================= * */
/**
 * @fn WaitForState() Waits for the state to change.
 *
 * @param pHandle This is component handle allocated by the OMX core.
 *
 * @param DesiredState This is state which the app is waiting for.
 *
 * @pre          None
 *
 * @post         None
 *
 *  @return      Appropriate OMX Error.
 *
 *  @see         None
 */
/* ================================================================================ * */
static OMX_ERRORTYPE WaitForState(OMX_HANDLETYPE* pHandle, OMX_STATETYPE DesiredState)
{
    OMX_STATETYPE CurState = OMX_StateInvalid;
    OMX_ERRORTYPE error = OMX_ErrorNone;
    int nCnt = 0;

    error = OMX_GetState(*pHandle, &CurState);
    while( (error == OMX_ErrorNone) && (CurState != DesiredState)) {
        sched_yield();
        if(nCnt++ == 10) {
            APP_DPRINT( "Still Waiting, press CTL-C to continue\n");
        }
        error = OMX_GetState(*pHandle, &CurState);
    }

    return error;
}


/* ================================================================================= * */
/**
 * @fn EventHandler() This is event handle which is called by the component on
 * any event change.
 *
 * @param hComponent This is component handle allocated by the OMX core.
 *
 * @param pAppData This is application private data.
 *
 * @param eEvent  This is event generated from the component.
 *
 * @param nData1  Data1 associated with the event.
 *
 * @param nData2  Data2 associated with the event.
 *
 * @param pEventData Any other string event data from component.
 *
 * @pre          None
 *
 * @post         None
 *
 *  @return      Appropriate OMX error.
 *
 *  @see         None
 */
/* ================================================================================ * */
OMX_ERRORTYPE EventHandler(OMX_HANDLETYPE hComponent,OMX_PTR pAppData,OMX_EVENTTYPE eEvent,
                           OMX_U32 nData1, OMX_U32 nData2, OMX_PTR pEventData)
{
    OMX_U8        writeValue = 0;
#ifdef APP_DEBUG
    char strState [20] = {""};
    char strTargetState [20] = {""};
    getString_OMX_State (strState, nData2);
    getString_OMX_State (strTargetState, nData1);
#endif

    switch (eEvent) {
    case OMX_EventResourcesAcquired:
        APP_DPRINT ("------> OMX_EventResourcesAcquired <--------\n");
        writeValue = 1;
        write(Event_Pipe[1], &writeValue, sizeof(OMX_U8));
        preempted = 0;
        break;
    case OMX_EventCmdComplete:
        APP_DPRINT ("------> OMX_EventCmdComplete<--------\n");
        gState = (OMX_STATETYPE)nData2;
        break;
    case OMX_EventMax:
        APP_DPRINT ("------> OMX_EventMax<--------\n");
    case OMX_EventMark:
        APP_DPRINT ("------> OMX_EventMark<--------\n");
        break;
    case OMX_EventPortSettingsChanged:
        APP_DPRINT ("------> OMX_EventPortSettingsChanged <--------\n");
        break;
    case OMX_EventComponentResumed:
        APP_DPRINT ("------> OMX_EventComponentResumed <--------\n");
        break;
    case OMX_EventDynamicResourcesAvailable:
        APP_DPRINT ("------> OMX_EventDynamicResourcesAvailable <--------\n");
        break;
    case OMX_EventPortFormatDetected:
        break;
    case OMX_EventError:
        APP_DPRINT ("------> OMX_EventError <--------\n");
        APP_DPRINT ( "%d :: G722DecTest.c  Event: OMX_EventError. Component State Changed To %s. Target %s\n", __LINE__, strState, strTargetState);            
        if (nData1 != OMX_ErrorInvalidState && nData1 == OMX_ErrorResourcesPreempted) {
            APP_DPRINT ("------> \tprocess on  OMX_ErrorResourcesPreempted<--------\n");                
            preempted=1;

            writeValue = 0;  
            write(Event_Pipe[1], &writeValue, sizeof(OMX_U8));
        } else {
            APP_DPRINT ("------> \tnothing to process on OMX_EnentError <--------\n");                
        }
        break;
    case OMX_EventBufferFlag:
        APP_DPRINT ("------> OMX_EventBufferFlag <--------\n");
        APP_DPRINT ( "%d :: G722DecTest.c  Event: OMX_EventBufferFlag. Component State Changed To %s. Target %s\n", __LINE__, strState, strTargetState);
        /* <<<<< Process Manager <<<<< */
        writeValue = 2;  
        write(Event_Pipe[1], &writeValue, sizeof(OMX_U8));
        /* >>>>>>>>>> */
        break;
    default:
        APP_DPRINT ("------> Unknown event <--------\n");
        break;
    }

    return OMX_ErrorNone;
}

/* ================================================================================= * */
/**
 * @fn FillBufferDone() Component sens the output buffer to app using this
 * callback.
 *
 * @param hComponent This is component handle allocated by the OMX core.
 *
 * @param ptr This is another pointer.
 *
 * @param pBuffer This is output buffer.
 *
 * @pre          None
 *
 * @post         None
 *
 *  @return      None
 *
 *  @see         None
 */
/* ================================================================================ * */
void FillBufferDone (OMX_HANDLETYPE hComponent, OMX_PTR ptr, OMX_BUFFERHEADERTYPE* pBuffer)
{
    write(OpBuf_Pipe[1], &pBuffer, sizeof(pBuffer));
#ifdef OMX_GETTIME
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    int GT_FlagE = 0;  /* Fill Buffer 1 = First Buffer,  0 = Not First Buffer  */
    int GT_FlagF = 0;  /*Empty Buffer  1 = First Buffer,  0 = Not First Buffer  */
    static OMX_NODE* pListHead = NULL;
#endif
}

/* ================================================================================= * */
/**
 * @fn EmptyBufferDone() Component sends the input buffer to app using this
 * callback.
 *
 * @param hComponent This is component handle allocated by the OMX core.
 *
 * @param ptr This is another pointer.
 *
 * @param pBuffer This is input buffer.
 *
 * @pre          None
 *
 * @post         None
 *
 *  @return      None
 *
 *  @see         None
 */
/* ================================================================================ * */
void EmptyBufferDone(OMX_HANDLETYPE hComponent, OMX_PTR ptr, OMX_BUFFERHEADERTYPE* pBuffer)
{
    if (!preempted)
        write(IpBuf_Pipe[1], &pBuffer, sizeof(pBuffer));
#ifdef OMX_GETTIME
    if (GT_FlagF == 1 ) { /* First Buffer Reply*/  /* 1 = First Buffer,  0 = Not First Buffer  */
        GT_END("Call to FillBufferDone  <First: FillBufferDone>");
        GT_FlagF = 0 ;   /* 1 = First Buffer,  0 = Not First Buffer  */
    }
#endif
}

/* ================================================================================= * */
/**
 * @fn send_input_buffer() Sends the input buffer to the component.
 *
 * @param pHandle This is component handle allocated by the OMX core.
 *
 * @param pBuffer This is the buffer pointer.
 *
 * @fIn This is input file handle.
 *
 * @pre          None
 *
 * @post         None
 *
 *  @return      Appropriate OMX error.
 *
 *  @see         None
 */
/* ================================================================================ * */
OMX_ERRORTYPE send_input_buffer(OMX_HANDLETYPE pHandle, OMX_BUFFERHEADERTYPE* pBufferHeader, FILE *fIn)
{
    OMX_ERRORTYPE error = OMX_ErrorNone;
    int nRead = 0;
    static int totalRead = 0;
    static int fileHdrReadFlag = 0;
    static int ccnt = 1;
    if (!fileHdrReadFlag) {
        fileHdrReadFlag = 1;
    }
    nRead = fread(pBufferHeader->pBuffer, 1, pBufferHeader->nAllocLen , fIn);

    if (nRead != 0) {
        totalRead += nRead;
        pBufferHeader->nFilledLen = nRead;
        ccnt++;

        APP_DPRINT("\n*****************************************************\n");
        APP_DPRINT ("%d :: App:: pBuffer->pBuffer = %p pBuffer->nAllocLen = * %ld, nRead = %d, totalread = %d\n",
                    __LINE__, pBufferHeader->pBuffer, pBufferHeader->nAllocLen, nRead, totalRead);
        APP_DPRINT("\n*****************************************************\n");
    }
    
    if(sendlastbuffer){
        sendlastbuffer = 0;
        pBufferHeader->nFlags = 0;

        return error;
    }

    if(nRead < pBufferHeader->nAllocLen) {
        pBufferHeader->nFlags = OMX_BUFFERFLAG_EOS;
        sendlastbuffer = 1;
    } else {
        pBufferHeader->nFlags = 0;
    }
    pBufferHeader->nFilledLen = nRead;
    error = OMX_EmptyThisBuffer(pHandle, pBufferHeader);
    return error;
}

/* ================================================================================= * */
/**
 * @fn validateArg4() This is the main function of application which gets called.
 *
 */
/* ================================================================================ * */
int validateArg4 (char *sArg4)
{
    int gDasfMode = -1;
    
    if (!strcmp(sArg4, "DM")) {
        gDasfMode = 1;
    } else if (!strcmp(sArg4, "FM")) {
        gDasfMode = 0;
    } else {
        printf("\n%d :: App: Sending Bad Parameter for dasf mode\n",__LINE__);
        printf("%d :: App: Should Be One of these Modes FM or DM\n",__LINE__);
        gDasfMode = -1;
    }
    
    return (gDasfMode);
}

/* ================================================================================= * */
/**
 * @fn getNumTestCase() This is the main function of application which gets called.
 *
 */
/* ================================================================================ * */
int getNumTestCase (char *sTestCase, char *sFileName)
{
    int iTestCase = 0;
    
    if(!strcmp(sTestCase,"TC_1")) {
        iTestCase = 1;
    } else if (!strcmp(sTestCase,"TC_2")) {
        iTestCase = 2;
    } else if (!strcmp(sTestCase,"TC_3")) {
        iTestCase = 3;
    } else if (!strcmp(sTestCase,"TC_4")) {
        iTestCase = 4;
    } else if (!strcmp(sTestCase,"TC_5")) {
        iTestCase = 5;
    } else if (!strcmp(sTestCase,"TC_6")) {
        iTestCase = 6;
    } else if (!strcmp(sTestCase,"TC_7")) {
        iTestCase = 7;
    } else if (!strcmp(sTestCase,"TC_8")) {
        iTestCase = 8;
    } else if (!strcmp(sTestCase,"TC_9")) {
        iTestCase = 9;
    }
    printTestCaseInfo (iTestCase, sFileName);
    
    return (iTestCase);
}

/* ================================================================================= * */
/**
 * @fn printTestCaseInfo() This is the main function of application which gets called.
 *
 */
/* ================================================================================ * */
void printTestCaseInfo (int iTestCase, char *sFileName)
{
    switch (iTestCase) {
    case 1:
    case 7:
    case 8:
        printf ("-------------------------------------\n");
        printf ("Testing Simple PLAY till EOF \n");
        printf ("-------------------------------------\n");
        strcat(sFileName, ".pcm");
        break;
    case 2:
        printf ("-------------------------------------\n");
        printf ("Testing Basic Stop \n");
        printf ("-------------------------------------\n");
        strcat(sFileName, "_tc2.pcm");
        break;
    case 3:
        printf ("-------------------------------------\n");
        printf ("Testing PAUSE & RESUME Command\n");
        printf ("-------------------------------------\n");
        strcat(sFileName, ".pcm");
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
        strcat(sFileName,"_tc5.pcm");
        break;
    case 6:
        printf ("------------------------------------------------\n");
        printf ("Testing Repeated PLAY with Deleting Component\n");
        printf ("------------------------------------------------\n");
        strcat(sFileName,"_tc6.pcm");
        break;
    case 9:
        printf ("------------------------------------------------\n");
        printf ("TimeStamp, TickCounts\n");
        printf ("------------------------------------------------\n");
        strcat(sFileName,"_tc6.pcm");
        break;
    } 
    
    return;
}

/* ================================================================================= * */
/**
 * @fn checkInputParameters() This is the main function of application which gets called.
 *
 */
/* ================================================================================ * */
boolean checkInputParameters (iNumParams)
{
    if(iNumParams != 9) {
        printf("Wrong Arguments: See Below:\n\n");
        printf("%d :: Usage: [TestApp] [Input File] [Input File Bit Rate] [TC ID]\
[FM/DM] [NB INPUT BUF] [INPUT BUF SIZE] [NB OUTPUT BUF] [OUTPUT BUF SIZE]\n",__LINE__);
        return (FALSE);
    }
    return (TRUE);
}

/* ================================================================================= * */
/**
 * @fn validateSampRate() This is the main function of application which gets called.
 *
 */
/* ================================================================================ * */
boolean validateSampRate (int iSampRate)
{
    if (iSampRate == 64000) {
        return (TRUE);
    } else if(iSampRate == 56000) {
        return (TRUE);
    } else if(iSampRate == 48000) {
        return (TRUE);
    } else if (iSampRate == INVALID_SAMPLING_FREQ) {
        printf("***********************************************************\n");
        printf("%d :: App: Note: Invalid Frequency has been specified by App\n",__LINE__);
        printf("***********************************************************\n");
        return (FALSE);
    } else {
        printf ("%d :: Sample Frequency Not Supported: \n", __LINE__);
        printf ("Supported frequencies are: 64000 or 56000 or 48000.\n");
        printf ("If you want to specify invalid sampling frequency, specify %d and test the component for error.\n",
                INVALID_SAMPLING_FREQ);
        printf("%d :: Exiting...........\n",__LINE__);
        return (FALSE);
    }
    printf ("Unspecified error.\n");
    return (FALSE);
}

/* ================================================================================= * */
/**
 * @fn omxSetInputPortParameter() This is the main function of application which gets called.
 *
 */
/* ================================================================================ * */
boolean omxSetInputPortParameter (OMX_HANDLETYPE *pHandle, int nIpBuffs, int nIpBufSize)
{
    OMX_PARAM_PORTDEFINITIONTYPE pCompPrivateStruct;
    OMX_ERRORTYPE error = OMX_ErrorNone;
    
    pCompPrivateStruct.nSize                  = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
    pCompPrivateStruct.eDir                   = OMX_DirInput;
    pCompPrivateStruct.nBufferCountActual     = nIpBuffs;
    pCompPrivateStruct.nBufferSize            = nIpBufSize;
    pCompPrivateStruct.format.audio.eEncoding = OMX_AUDIO_CodingPCM;
    pCompPrivateStruct.bEnabled               = OMX_TRUE;
    pCompPrivateStruct.bPopulated             = OMX_FALSE;
    pCompPrivateStruct.nPortIndex             = OMX_DirInput;
#ifdef OMX_GETTIME
    GT_START();
    error = OMX_SetParameter(*pHandle,OMX_IndexParamPortDefinition, &pCompPrivateStruct);
    GT_END("Set Parameter Test-SetParameter");
#else
    error = OMX_SetParameter(*pHandle,OMX_IndexParamPortDefinition, &pCompPrivateStruct);
#endif
    if(error != OMX_ErrorNone) {
        error = OMX_ErrorBadParameter;
        printf ("%d :: OMX_ErrorBadParameter\n",__LINE__);
    }
  
    return (TRUE);
}

/* ================================================================================= * */
/**
 * @fn omxSetOutputPortParameter() This is the main function of application which gets called.
 *
 */
/* ================================================================================ * */
boolean omxSetOutputPortParameter (OMX_HANDLETYPE *pHandle, int nOpBuffs, int nOpBufSize, int gDasfMode)
{
    OMX_PARAM_PORTDEFINITIONTYPE pCompPrivateStruct;
    OMX_ERRORTYPE error = OMX_ErrorNone;
    
    pCompPrivateStruct.eDir                   = OMX_DirOutput;
    pCompPrivateStruct.nBufferCountActual     = (gDasfMode == 1)? 0 : nOpBuffs;
    pCompPrivateStruct.nBufferSize            = nOpBufSize;
    pCompPrivateStruct.format.audio.eEncoding = OMX_AUDIO_CodingPCM;
    pCompPrivateStruct.bEnabled               = OMX_TRUE;
    pCompPrivateStruct.bPopulated             = (gDasfMode == 1)? OMX_TRUE : OMX_FALSE;
    pCompPrivateStruct.nPortIndex             = OMX_DirOutput;

#ifdef OMX_GETTIME
    GT_START();
    error = OMX_SetParameter(*pHandle,OMX_IndexParamPortDefinition, &pCompPrivateStruct);
    GT_END("Set Parameter Test-SetParameter");
#else
    error = OMX_SetParameter(*pHandle,OMX_IndexParamPortDefinition, &pCompPrivateStruct);
#endif
    if(error != OMX_ErrorNone) {
        error = OMX_ErrorBadParameter;
        printf ("%d :: OMX_ErrorBadParameter\n",__LINE__);
    }
  
    return (TRUE);
}

/* ================================================================================= * */
/**
 * @fn g722SetInputPort() This is the main function of application which gets called.
 *
 */
/* ================================================================================ * */
boolean g722SetInputPort (OMX_HANDLETYPE *pHandle, int iSampRate)
{
    OMX_AUDIO_PARAM_PCMMODETYPE pG722Param;
    OMX_ERRORTYPE error = OMX_ErrorNone;
    
    pG722Param.nSize                    = sizeof(OMX_AUDIO_PARAM_PCMMODETYPE);
    pG722Param.nVersion.s.nVersionMajor = 0xF1;
    pG722Param.nVersion.s.nVersionMinor = 0xF2;
    pG722Param.nPortIndex               = OMX_DirInput;
    pG722Param.nChannels                = 1; /* mono */
    pG722Param.nBitPerSample            = 8;
    pG722Param.eNumData                 = OMX_NumericalDataUnsigned;
    pG722Param.eEndian                  = OMX_EndianLittle;
    pG722Param.bInterleaved             = OMX_FALSE;
    pG722Param.ePCMMode                 = OMX_AUDIO_PCMModeLinear;
    pG722Param.nSamplingRate            = iSampRate;

#ifdef OMX_GETTIME
    GT_START();
    error = OMX_SetParameter (*pHandle, OMX_IndexParamAudioPcm, &pG722Param);
    GT_END("Set Parameter Test-SetParameter");
#else
    error = OMX_SetParameter (*pHandle, OMX_IndexParamAudioPcm, &pG722Param);
#endif

    if (error != OMX_ErrorNone) {
        error = OMX_ErrorBadParameter;
        return (FALSE);
    }
    return (TRUE);
}

/* ================================================================================= * */
/**
 * @fn g722SetOutputPort() This is the main function of application which gets called.
 *
 */
/* ================================================================================ * */
boolean g722SetOutputPort (OMX_HANDLETYPE *pHandle)
{
    OMX_AUDIO_PARAM_PCMMODETYPE pG722Param;
    OMX_ERRORTYPE error = OMX_ErrorNone;
    
    pG722Param.nSize                    = sizeof(OMX_AUDIO_PARAM_PCMMODETYPE);
    pG722Param.nVersion.s.nVersionMajor = 0xF1;
    pG722Param.nVersion.s.nVersionMinor = 0xF2;
    pG722Param.nPortIndex               = OMX_DirOutput;
    pG722Param.nChannels                = 1; /* mono */
    pG722Param.nBitPerSample            = 16;
    pG722Param.nSamplingRate            = 16000;
    pG722Param.eNumData                 = OMX_NumericalDataUnsigned;
    pG722Param.eEndian                  = OMX_EndianLittle;
    pG722Param.bInterleaved             = OMX_FALSE;
    pG722Param.ePCMMode                 = OMX_AUDIO_PCMModeLinear;

#ifdef OMX_GETTIME
    GT_START();
    error = OMX_SetParameter (*pHandle, OMX_IndexParamAudioPcm, &pG722Param);
    GT_END("Set Parameter Test-SetParameter");
#else
    error = OMX_SetParameter (*pHandle, OMX_IndexParamAudioPcm, &pG722Param);
#endif

    if (error != OMX_ErrorNone) {
        error = OMX_ErrorBadParameter;
        return (FALSE);
    }
    return (TRUE);
}

/* ================================================================================= * */
/**
 * @fn omxSetConfigMute() This is the main function of application which gets called.
 *
 */
/* ================================================================================ * */
boolean omxSetConfigMute (OMX_HANDLETYPE *pHandle, OMX_BOOL bMute)
{
    OMX_AUDIO_CONFIG_MUTETYPE    pCompPrivateStructMute;
    OMX_ERRORTYPE error = OMX_ErrorNone;
    
    /* default setting for Mute/Unmute */
    pCompPrivateStructMute.nSize                    = sizeof (OMX_AUDIO_CONFIG_MUTETYPE);
    pCompPrivateStructMute.nVersion.s.nVersionMajor = 0xF1;
    pCompPrivateStructMute.nVersion.s.nVersionMinor = 0xF2;
    pCompPrivateStructMute.nPortIndex               = OMX_DirInput;
    pCompPrivateStructMute.bMute                    = bMute;

    error = OMX_SetConfig(*pHandle, OMX_IndexConfigAudioMute, &pCompPrivateStructMute);
    if (error != OMX_ErrorNone) {
        error = OMX_ErrorBadParameter;
        return (FALSE);
    }
    return (TRUE);
}

/* ================================================================================= * */
/**
 * @fn omxSetConfigVolume() This is the main function of application which gets called.
 *
 */
/* ================================================================================ * */
boolean omxSetConfigVolume (OMX_HANDLETYPE *pHandle, OMX_S32 nVolume)
{
    OMX_AUDIO_CONFIG_VOLUMETYPE  pCompPrivateStructVolume;
    OMX_ERRORTYPE error = OMX_ErrorNone;
    
    /* default setting for volume */
    pCompPrivateStructVolume.nSize                    = sizeof(OMX_AUDIO_CONFIG_VOLUMETYPE);
    pCompPrivateStructVolume.nVersion.s.nVersionMajor = 0xF1;
    pCompPrivateStructVolume.nVersion.s.nVersionMinor = 0xF2;
    pCompPrivateStructVolume.nPortIndex               = OMX_DirInput;
    pCompPrivateStructVolume.bLinear                  = OMX_FALSE;
    pCompPrivateStructVolume.sVolume.nValue           = nVolume;  /* actual volume */
    pCompPrivateStructVolume.sVolume.nMin             = 0;   /* min volume */
    pCompPrivateStructVolume.sVolume.nMax             = 100; /* max volume */

    error = OMX_SetConfig(*pHandle, OMX_IndexConfigAudioVolume, &pCompPrivateStructVolume);
    if (error != OMX_ErrorNone) {
        error = OMX_ErrorBadParameter;
        return (FALSE);
    }
    return (TRUE);
}

/* ================================================================================= * */
/**
 * @fn omxUseBuffers() This is the main function of application which gets called.
 *
 */
/* ================================================================================ * */
OMX_ERRORTYPE omxUseBuffers (OMX_HANDLETYPE *pHandle, int gDasfMode, int nIpBuffs, int nIpBufSize, OMX_BUFFERHEADERTYPE *pInputBufferHeader [], int nOpBuffs, int nOpBufSize, OMX_BUFFERHEADERTYPE *pOutputBufferHeader [])
{
    OMX_ERRORTYPE error = OMX_ErrorNone;
    int i=0;                                                                                    /* 0j0 <<<<<<<<<<< */
       
#ifndef USE_BUFFER
    for(i=0; i < nIpBuffs; i++) {
        APP_DPRINT("%d :: About to call OMX_AllocateBuffer On Input\n",__LINE__);
        error = OMX_AllocateBuffer(*pHandle, &pInputBufferHeader[i], 0, NULL, nIpBufSize);
        APP_DPRINT("%d :: called OMX_AllocateBuffer\n",__LINE__);
        if(error != OMX_ErrorNone) {
            APP_DPRINT("%d :: Error returned by OMX_AllocateBuffer()\n",__LINE__);
            return (error);
        }
    }

    if (gDasfMode == 0) {
        for(i=0; i < nOpBuffs; i++) {
            error = OMX_AllocateBuffer(*pHandle,&pOutputBufferHeader[i],1,NULL,nOpBufSize);
            if(error != OMX_ErrorNone) {
                APP_DPRINT("%d :: Error returned by OMX_AllocateBuffer()\n",__LINE__);
                return (error);
            }
        }
    }
#else
    OMX_U8* pInputBuffer  [MAX_NUM_OF_BUFS] = {NULL};
    OMX_U8* pOutputBuffer [MAX_NUM_OF_BUFS] = {NULL};

    for(i=0; i<nIpBuffs; i++) {
        if (pInputBuffer[i] == NULL) {
            pInputBuffer[i] = (OMX_U8*)malloc(nIpBufSize);
            APP_DPRINT("%d :: About to call OMX_UseBuffer On Input\n",__LINE__);
            printf("%d :: About to call OMX_UseBuffer On Input\n",__LINE__);
            error = OMX_UseBuffer(*pHandle,&pInputBufferHeader[i],0,NULL,nIpBufSize,pInputBuffer[i]);
            if(error != OMX_ErrorNone) {
                APP_DPRINT("%d :: Error returned by OMX_UseBuffer()\n",__LINE__);
                return (error);
            }
        }
    }

    if (gDasfMode == 0) {
        for(i=0; i<nOpBuffs; i++) {
            if (pOutputBuffer[i] == NULL) {
                pOutputBuffer[i] = malloc (nOpBufSize + 256);
                pOutputBuffer[i] = pOutputBuffer[i] + 128;
                /* allocate output buffer */
                APP_DPRINT("%d :: About to call OMX_UseBuffer On Output\n",__LINE__);
                printf("%d :: About to call OMX_UseBuffer On Output\n",__LINE__);
                error = OMX_UseBuffer(*pHandle,&pOutputBufferHeader[i],1,NULL,nOpBufSize,pOutputBuffer[i]);
                if(error != OMX_ErrorNone) {
                    APP_DPRINT("%d :: Error returned by OMX_UseBuffer()\n",__LINE__);
                    return (error);
                }
                pOutputBufferHeader[i]->nFilledLen = 0;
            }
        }
    }
#endif
    return (error);
}

/* ================================================================================= * */
/**
 * @fn testCases() This is the main function of application which gets called.
 *
 */
/* ================================================================================ * */
OMX_ERRORTYPE testCases (OMX_HANDLETYPE *pHandle, fd_set *rfds, int tcID, FILE *fIn, FILE *fOut, int *frmCnt, int *totalFilled, struct timeval *tv, int gDasfMode, int nIpBuffs, OMX_BUFFERHEADERTYPE *pInputBufferHeader [], int nOpBuffs, OMX_BUFFERHEADERTYPE *pOutputBufferHeader [])
{
    boolean bFlag = true;
    static int frmCount = 0;
    
    OMX_ERRORTYPE error = OMX_ErrorNone;
    int fdmax = maxint(IpBuf_Pipe [0], OpBuf_Pipe [0]);
    fdmax = maxint (fdmax, Event_Pipe [0]);
    
    FD_ZERO (rfds);
    FD_SET (IpBuf_Pipe[0], rfds);
    FD_SET (OpBuf_Pipe[0], rfds);
    FD_SET (Event_Pipe[0], rfds);
    tv->tv_sec  = 1;
    tv->tv_usec = 0;

    int retval = select(fdmax+1, rfds, NULL, NULL, tv);
    if(retval == -1) {
        perror("select()");
        APP_DPRINT ( " : Error \n");
        return (OMX_EventError);
    }

    if(retval == 0) {
        APP_DPRINT ("%d :: BasicFn App Timeout !!!!!!!!!!! \n",__LINE__);
    }
    /*****/
    frmCount++;

    switch (tcID) {
    case 1:
    case 5:
    case 6:
        if(FD_ISSET(IpBuf_Pipe[0], rfds)) {
            OMX_BUFFERHEADERTYPE* pBuffer = NULL;
            read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
            APP_DPRINT("%d :: APP: input buffer received, filled length = %d\n",__LINE__,(int)pBuffer->nFilledLen);
            error = send_input_buffer (*pHandle, pBuffer, fIn);
            if (error != OMX_ErrorNone) {
                return (error);
            }
        }
        break;
    case 2:
    case 4:
        if(FD_ISSET(IpBuf_Pipe[0], rfds)) {
            if(*frmCnt == 4) {
                APP_DPRINT("Shutting down ---------- \n");
#ifdef OMX_GETTIME
                GT_START();
#endif
                error = OMX_SendCommand(*pHandle,OMX_CommandStateSet, OMX_StateIdle, NULL);
                if(error != OMX_ErrorNone) {
                    fprintf (stderr,"Error from SendCommand-Idle(Stop) State function\n");
                    return (error);
                }
                if(tcID == 4) {
                    error = testCase_2_4 (pHandle, gDasfMode, fIn, fOut, nIpBuffs, pInputBufferHeader);
                    if(error != OMX_ErrorNone) {
                        fprintf (stderr,"Error from SendCommand-Idle(Stop) State function\n");
                        return (error);
                    }
                }
            } else {
                OMX_BUFFERHEADERTYPE* pBuffer = NULL;
                read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
                error = send_input_buffer (*pHandle, pBuffer, fIn);
                if (error != OMX_ErrorNone) {
                    return (error);
                }
            }
            (*frmCnt)++;
        }
        break;
    case 3:
        if(frmCount == 3 || frmCount == 6) {
            error = testCase_3 (pHandle);
            if(error != OMX_ErrorNone) {
                return (error);
            }
        }

        if(FD_ISSET(IpBuf_Pipe[0], rfds)) {

            OMX_BUFFERHEADERTYPE* pBuffer = NULL;
            read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
            error = send_input_buffer (*pHandle, pBuffer, fIn);
            if (error != OMX_ErrorNone) {
                return (error);
            }

            (*frmCnt)++;
        }
        break;
    case 7:
        /* test mute/unmute playback stream */
        if(FD_ISSET(IpBuf_Pipe[0], rfds)) {
            OMX_BUFFERHEADERTYPE* pBuffer = NULL;
            read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
            error = send_input_buffer (*pHandle, pBuffer, fIn);
            if (error != OMX_ErrorNone) {
                return (error);
            }
            (*frmCnt)++;
        }

        if(*frmCnt == 3) {
            printf("************Mute the playback stream*****************\n");
            bFlag = omxSetConfigMute (pHandle, OMX_TRUE);
            if (bFlag == FALSE) {
                return (error);
            }
        }

        if(*frmCnt == 6) {
            printf("************Unmute the playback stream*****************\n");
            bFlag = omxSetConfigMute (pHandle, OMX_FALSE);
            if (bFlag == FALSE) {
                return (error);
            }                            
        }
        break;

    case 8:
        /* test set volume for playback stream */
        if(FD_ISSET(IpBuf_Pipe[0], rfds)) {
            OMX_BUFFERHEADERTYPE* pBuffer = NULL;
            read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
            error = send_input_buffer (*pHandle, pBuffer, fIn);
            if (error != OMX_ErrorNone) {
                return (error);
            }

            (*frmCnt)++;
        }

        if(*frmCnt == 3) {
            printf("************Set stream volume to high*****************\n");
            bFlag = omxSetConfigVolume (pHandle, 0x8000);
            if (bFlag == FALSE) {
                return (error);
            }
        }

        if(*frmCnt == 6) {
            printf("************Set stream volume to low*****************\n");
            bFlag = omxSetConfigVolume (pHandle, 0x1000);
            if (bFlag == FALSE) {
                return (error);
            }
        }
        break;
    case 9:
        if(FD_ISSET(IpBuf_Pipe[0], rfds)) {
            OMX_BUFFERHEADERTYPE* pBuffer=  NULL;
            read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
            APP_DPRINT("%d :: APP: input buffer received, filled length = %d\n",__LINE__,(int)pBuffer->nFilledLen);
            error = send_input_buffer (*pHandle, pBuffer, fIn);
            if (error != OMX_ErrorNone) {
                return (error);
            }
            
            pBuffer->nTimeStamp = (OMX_S64) rand() % 70;
            pBuffer->nTickCount = (OMX_S64) rand() % 70;
            printf("SENDING TIMESTAMP = %d\n", (int) pBuffer->nTimeStamp);
            printf("SENDING TICK COUNT = %ld\n", pBuffer->nTickCount);
        }
        break;

    default:
        APP_DPRINT("%d :: ### Running Simple DEFAULT Case Here ###\n",__LINE__);
    } /* end switch */         
    /**********************/
    if( FD_ISSET(OpBuf_Pipe[0], rfds) ) {
        OMX_BUFFERHEADERTYPE* pBuf = NULL;
        read(OpBuf_Pipe[0], &pBuf, sizeof(pBuf));
        if(pBuf->nFlags == OMX_BUFFERFLAG_EOS){
            printf("EOS received by App, Stopping the component\n");
            pBuf->nFlags = 0;
            /*            StopComponent(pHandle);*/
            fseek(fIn, 0, SEEK_SET);
        }
        if(pBuf->nFilledLen == 0) {
            APP_DPRINT("%d :: APP: output buffer received, filled length = %d, totalfilled = %d\n",__LINE__,(int)pBuf->nFilledLen,totalFilled);
        } else {
            APP_DPRINT("%d :: APP: output buffer received, filled length = %d, totalfilled = %d\n",__LINE__,(int)pBuf->nFilledLen,totalFilled);
            fwrite(pBuf->pBuffer, 1, pBuf->nFilledLen, fOut);
        }
        totalFilled += pBuf->nFilledLen;
        fflush(fOut);
        OMX_FillThisBuffer(*pHandle, pBuf);
    }
    /*************/
    /*<<<<<<< Process Manager <<<<<<< */
    if ( FD_ISSET (Event_Pipe[0], rfds) ) {        
        OMX_U8 pipeContents =0;
        read(Event_Pipe[0], &pipeContents, sizeof(OMX_U8));       

        if (pipeContents == 0) {

            printf("Test app received OMX_ErrorResourcesPreempted\n");
            WaitForState(pHandle,OMX_StateIdle);
            int i;
            for (i=0; i < nIpBuffs; i++) {
                error = OMX_FreeBuffer (pHandle,OMX_DirInput,pInputBufferHeader[i]);
                if( (error != OMX_ErrorNone)) {
                    APP_DPRINT ("%d :: G722DecTest.c :: Error in Free Handle function\n",__LINE__);
                }
            }
            
            if (gDasfMode == 0) {
                for (i=0; i < nOpBuffs; i++) {
                    error = OMX_FreeBuffer (pHandle,OMX_DirOutput,pOutputBufferHeader[i]);
                    if( (error != OMX_ErrorNone)) {
                        APP_DPRINT ("%d:: Error in Free Handle function\n",__LINE__);
                    }
                }
            }

#ifdef USE_BUFFER
            /* newfree the App Allocated Buffers */
            APP_DPRINT("%d :: AmrDecTest.c :: Freeing the App Allocated Buffers in TestApp\n",__LINE__);
            for(i=0; i < nIpBuffs; i++) {
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

            OMX_SendCommand(*pHandle,OMX_CommandStateSet, OMX_StateIdle/*OMX_StateLoaded*/,NULL);
            WaitForState(pHandle, OMX_StateIdle/*OMX_StateLoaded*/);
#ifdef WAITFORRESOURCES            
            OMX_SendCommand(*pHandle,OMX_CommandStateSet,OMX_StateWaitForResources,NULL);
            WaitForState(pHandle,OMX_StateWaitForResources);
#endif            
        } else if (pipeContents == 1) {

            printf("Test app received OMX_ErrorResourcesAcquired\n");

            OMX_SendCommand(*pHandle,OMX_CommandStateSet,OMX_StateIdle,NULL);
            int i = 0;
            for (i=0; i < nIpBuffs; i++) {
                /* allocate input buffer */
                error = OMX_AllocateBuffer (*pHandle,&pInputBufferHeader[i],0,NULL, G722D_INPUT_BUFFER_SIZE *3); /*To have enought space for    */
                if(error != OMX_ErrorNone) {
                    APP_DPRINT("%d :: G722DecTest.c :: Error returned by OMX_AllocateBuffer()\n",__LINE__);
                }
            }
            WaitForState(pHandle,OMX_StateIdle);
            OMX_SendCommand(*pHandle,OMX_CommandStateSet,OMX_StateExecuting,NULL);
            WaitForState(pHandle,OMX_StateExecuting);
            rewind(fIn);
            
            for (i=0; i < nIpBuffs;i++) {    
                send_input_buffer(*pHandle, pInputBufferHeader[i], fIn); 
            }                
        } else if (pipeContents == 2) {
            OMX_STATETYPE CurState = OMX_StateInvalid;
            char strCurrState [20] = {""};

            error = OMX_GetState(*pHandle, &CurState);
            getString_OMX_State (strCurrState, CurState);
            error = OMX_SendCommand(*pHandle,OMX_CommandStateSet, OMX_StateIdle, NULL);
            error = WaitForState(pHandle, OMX_StateIdle);
#ifdef OMX_GETTIME
            GT_END("Call to SendCommand <OMX_StateIdle>");
#endif
            if(error != OMX_ErrorNone) {
                printf("\nError:  WaitForState reports an error %X!!!!!!!\n", error);
                return error;
            }
            /*            
                          #if 1
                          error = OMX_SendCommand(*pHandle,OMX_CommandStateSet, OMX_StateLoaded, NULL);
                          if(error != OMX_ErrorNone) {
                          APP_DPRINT ("%d :: G722DecTest.c :: Error from SendCommand-Idle State function\n",__LINE__);
                          printf("goto EXIT %d\n",__LINE__);

                          *goto EXIT;*
                          return (-1);
                          }
                          error = WaitForState(pHandle, OMX_StateLoaded);
                          if(error != OMX_ErrorNone) {
                          APP_DPRINT( "%d :: G722DecTest.c :: Error:  WaitForState reports an error %X\n",__LINE__, error);
                          printf("goto EXIT %d\n",__LINE__);
                          *goto EXIT;*
                          return (-1);
                          }
                          #   ifdef OMX_GETTIME
                          GT_END("Call to SendCommand <OMX_StateIdle>"); *<OMX_StateLoaded>");*
                          #   endif
            
                          #endif
            */            
            /*goto SHUTDOWN;*/
            return (OMX_ErrorNone); 
            /**/                               
        } else {
            printf ("--------> Pipe content = %d <----------\n", pipeContents);
        }
    }
    return (error);
}

/* ================================================================================= * */
/**
 * @fn testCase_2_4() This is the main function of application which gets called.
 *
 */
/* ================================================================================ * */
OMX_ERRORTYPE testCase_2_4 (OMX_HANDLETYPE *pHandle, int gDasfMode, FILE *fIn, FILE *fOut, int nIpBuffs, OMX_BUFFERHEADERTYPE *pInputBufferHeader  [])
{
    OMX_ERRORTYPE error = OMX_ErrorNone;
    
    APP_STATEPRINT ("*********** Waiting for state to change to Idle ************\n\n");

#ifdef OMX_GETTIME
    GT_END("Call to SendCommand <OMX_StateIdle>");
#endif

    error = WaitForState(pHandle, OMX_StateIdle);
    if(error != OMX_ErrorNone) {
        fprintf(stderr, "Error:  WaitForState reports an error %X\n", error);
        return (error);
    }
    APP_STATEPRINT("*********** State Changed to Idle ************\n\n");

    printf("Component Has Stopped here and waiting for %d seconds before it plays further\n",SLEEP_TIME);
    sleep(SLEEP_TIME);

    APP_STATEPRINT("*************** Execute command to Component *******************\n");

#ifdef OMX_GETTIME
    GT_START();
#endif

    error = OMX_SendCommand(*pHandle, OMX_CommandStateSet,OMX_StateExecuting, NULL);
    if(error != OMX_ErrorNone) {
        fprintf (stderr,"Error from SendCommand-Executing State function\n");
        return (error);
    }

    APP_STATEPRINT("*********** Waiting for state to change to Executing ************\n\n");
    error = WaitForState(pHandle, OMX_StateExecuting);
    
#ifdef OMX_GETTIME
    GT_START();
#endif
    if(error != OMX_ErrorNone) {
        fprintf(stderr, "Error:  WaitForState reports an error %X\n", error);
        return (error);
    }
    APP_STATEPRINT("*********** State Changed to Executing ************\n\n");

    if (gDasfMode == 0) {
        /*rewind input and output files*/
        fseek(fIn, 0L, SEEK_SET);
        fseek(fOut, 0L, SEEK_SET);
    }
    int k = 0;
    for (k=0; k < nIpBuffs; k++) {
        error = send_input_buffer (*pHandle, pInputBufferHeader[k], fIn);
    }
    return (error);
}
/* ================================================================================= * */
/**
 * @fn testCase_3() This is the main function of application which gets called.
 *
 */
/* ================================================================================ * */
OMX_ERRORTYPE testCase_3 (OMX_HANDLETYPE *pHandle)
{
    OMX_ERRORTYPE error = OMX_ErrorNone;
    
    APP_STATEPRINT("\n\n*************** Pause command to Component *******************\n");

#ifdef OMX_GETTIME
    GT_END("Call to SendCommand <OMX_StateExecuting>");
#endif

    error = OMX_SendCommand (*pHandle, OMX_CommandStateSet, OMX_StatePause, NULL);
    if(error != OMX_ErrorNone) {
        fprintf (stderr,"Error from SendCommand-Pasue State function\n");
        return (error);
    }
    APP_STATEPRINT("*********** Waiting for state to change to Pause ************\n");
#ifdef OMX_GETTIME
    GT_START();
#endif
    error = WaitForState (pHandle, OMX_StatePause);

    if(error != OMX_ErrorNone) {
        fprintf(stderr, "Error:  WaitForState reports an error %X\n", error);
        return (error);
    }
    APP_STATEPRINT("*********** State Changed to Pause ************\n\n");

    printf("Sleeping for %d secs....\n\n",SLEEP_TIME);
    sleep(SLEEP_TIME);

    APP_STATEPRINT("*************** Resume command to Component *******************\n");
#ifdef OMX_GETTIME
    GT_END("Call to SendCommand <OMX_StatePause>");
#endif
    error = OMX_SendCommand (*pHandle, OMX_CommandStateSet,OMX_StateExecuting, NULL);
    if(error != OMX_ErrorNone) {
        fprintf (stderr,"Error from SendCommand-Executing State function\n");
        return (error);
    }

    APP_STATEPRINT("******** Waiting for state to change to Resume ************\n");

#ifdef OMX_GETTIME
    GT_END("Call to SendCommand <OMX_StateExecuting>");
#endif

    error = WaitForState (pHandle, OMX_StateExecuting);
    if(error != OMX_ErrorNone) {
        fprintf(stderr, "Error:  WaitForState reports an error %X\n", error);
        return (error);
    }
    APP_STATEPRINT("*********** State Changed to Resume ************\n\n");
    
    return (error);
}
/* ================================================================================= * */
/**
 * @fn sendInBuffFillOutBuff() This is the main function of application which gets called.
 *
 */
/* ================================================================================ * */
OMX_ERRORTYPE sendInBuffFillOutBuff (OMX_HANDLETYPE *pHandle, int nIpBuffs, int nOpBuffs, int gDasfMode, OMX_BUFFERHEADERTYPE *pInputBufferHeader[],  FILE *fIn, OMX_BUFFERHEADERTYPE *pOutputBufferHeader[])
{
    int k = 0;
    OMX_ERRORTYPE error = OMX_ErrorNone;
    
    for (k=0; k < nIpBuffs; k++) {
#ifdef OMX_GETTIME
        if (k == 0) { 
            GT_FlagE=1;  /* 1 = First Buffer,  0 = Not First Buffer  */
            GT_START(); /* Empty Bufffer */
        }
#endif
        error = send_input_buffer (*pHandle, pInputBufferHeader[k], fIn);
        if(error != OMX_ErrorNone) {
            return (error);
        }
    }
    if (gDasfMode == 0) {
        for (k=0; k < nOpBuffs; k++) {
#ifdef OMX_GETTIME
            if (k == 0) { 
                GT_FlagF=1;  /* 1 = First Buffer,  0 = Not First Buffer  */
                GT_START(); /* Fill Buffer */
            }
#endif
            error = OMX_FillThisBuffer (*pHandle, pOutputBufferHeader[k]);
            if(error != OMX_ErrorNone) {
                return (error);
            }
        }
    }
    return (error);
}
/* ================================================================================= * */
/**
 * @fn omxFreeBuffers() This is the main function of application which gets called.
 *
 */
/* ================================================================================ * */
OMX_ERRORTYPE omxFreeBuffers (OMX_HANDLETYPE *pHandle, int nBuffs, OMX_BUFFERHEADERTYPE *pBufferHeader  [], char *sBuffTypeMsg)
{
    int i = 0;
    OMX_ERRORTYPE error = OMX_ErrorNone;
    
    for (i=0; i<nBuffs; i++) {
        APP_DPRINT("%d :: App: Freeing %p %s BufHeader\n",__LINE__,pBufferHeader[i], sBuffTypeMsg);
        error = OMX_FreeBuffer(*pHandle,OMX_DirInput,pBufferHeader[i]);
        if((error != OMX_ErrorNone)) {
            APP_DPRINT ("%d:: Error in Free Handle function\n",__LINE__);
            return (error);
        }
    }
    
    return (error);
}
/* ================================================================================= * */
/**
 * @fn getString_OMX_State () This is the main function of application which gets called.
 *
 */
/* ================================================================================ * */
void getString_OMX_State (char *ptrString, OMX_STATETYPE state)
{
    switch (state) {
    case OMX_StateInvalid:
        strcpy (ptrString, "OMX_StateInvalid\0");
        break;
    case OMX_StateLoaded:
        strcpy (ptrString, "OMX_StateLoaded\0");
        break;
    case OMX_StateIdle:
        strcpy (ptrString, "OMX_StateIdle\0");
        break;
    case OMX_StateExecuting:
        strcpy (ptrString, "OMX_StateExecuting\0");
        break;
    case OMX_StatePause:
        strcpy (ptrString, "OMX_StatePause\0");
        break;
    case OMX_StateWaitForResources:
        strcpy (ptrString, "OMX_StateWaitForResources\0");
        break;
    case OMX_StateMax:
        strcpy (ptrString, "OMX_StateMax\0");
        break;
    default:
        strcpy (ptrString, "Unknown state\0");
        break;
            
    }
    return;
}
