
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
/* ====================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
* ==================================================================== */
#ifdef UNDER_CE
#include <windows.h>
#else
#include <sys/stat.h>
#include <sys/time.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h> 
#include <string.h> 
#include <sched.h> 
#include <unistd.h> 
#include <sys/types.h> 
#include <sys/select.h>
#include <time.h> 
#include <OMX_Component.h>
#include "JPEGTest.h"

/* DSP recovery includes */
#include <qosregistry.h>
#include <qosti.h>
#include <dbapi.h>
#include <DSPManager.h>
#include <DSPProcessor.h>
#include <DSPProcessor_OEM.h>

#ifdef UNDER_CE
#define PRINT printf
#else
/*#define OMX_DEB*/
#ifdef OMX_DEB
#define PRINT(str,args...) fprintf(stdout,"[%s] %s():%d: *** "str"",__FILE__,__FUNCTION__,__LINE__,##args)
#else
#define PRINT(str, args...)
#endif
#endif  

typedef unsigned char uchar;
/**
 * Pipe used to communicate back to the main thread from the component thread;
**/

int IpBuf_Pipe[2];
int OpBuf_Pipe[2];
int Event_Pipe[2];
struct timeval tim;
int DEINIT_FLAG = 0;
int bPreempted=0;


/* safe routine to get the maximum of 2 integers */

inline int maxint(int a, int b)
{
    return(a>b) ? a : b;
}


 /*Define prototypes*/
 
#ifdef DSP_MMU_FAULT_HANDLING
int LoadBaseImage();
#endif

 static OMX_ERRORTYPE WaitForEvent_JPEG(OMX_HANDLETYPE* pHandle,
                                  OMX_EVENTTYPE DesiredEvent,
                                  OMX_U32 data,
                                  OMX_STATETYPE DesiredState);


static int Get16m(const void * Short)
{
    return(((uchar *)Short)[0] << 8) | ((uchar *)Short)[1];
}

void  FixFrameSize ( IMAGE_INFO* imageinfo)
{

    int nWidth=imageinfo->nWidth, nHeight=imageinfo->nHeight;

    /*round up if nWidth is not multiple of 32*/
    ( (nWidth%32 ) !=0 ) ?  nWidth=32 * (  (  nWidth/32 ) + 1 )  : nWidth;
    PRINT("411 file new nWidth %d \n", nWidth);
    
    /*round up if nHeight is not multiple of 16*/
    ( (nHeight%16) !=0 ) ?  nHeight=16 * (  (  nHeight/16 ) + 1 )  : nHeight;
    PRINT("new nHeight %d \n", nHeight);
    
    imageinfo->nWidth = nWidth;
    imageinfo->nHeight = nHeight;
}


int GetYUVformat(uchar * Data)
{
    unsigned char Nf;
    int j;
     int temp_index;
     int temp;
     int image_format;
    short           H[4],V[4]; 
        
     Nf = Data[7];


    for (j = 0; j < Nf; j++)
    {
       temp_index = j * 3 + 7 + 2;
        /*---------------------------------------------------------*/
       /* H[j]: upper 4 bits of a byte, horizontal sampling fator.                  */
       /* V[j]: lower 4 bits of a byte, vertical sampling factor.                    */
       /*---------------------------------------------------------*/
         H[j] = (0x0f & (Data[temp_index] >> 4));
         V[j] = (0x0f & Data[temp_index]);
    }

    /*------------------------------------------------------------------*/
    /* Set grayscale flag, namely if image is gray then set it to 1,    */
    /* else set it to 0.                                                */
    /*------------------------------------------------------------------*/
    image_format = -1;


    if (Nf == 1){
      image_format = OMX_COLOR_FormatL8;
    }


    if (Nf == 3) 
    {
       temp = (V[0]*H[0])/(V[1]*H[1]) ;

      if (temp == 4 && H[0] == 2)
        image_format = OMX_COLOR_FormatYUV420PackedPlanar;
      
      if (temp == 4 && H[0] == 4)
        image_format = OMX_COLOR_FormatYUV411Planar;
      
      if (temp == 2)
        image_format = OMX_COLOR_FormatCbYCrY; /* YUV422 interleaved, little endian */
      
      if (temp == 1)
        image_format = OMX_COLOR_FormatYUV444Interleaved;
    }

    return (image_format);

}

/*--------------------------------------------------------------------------
* Parse the marker stream until SOS or EOI is seen;
* ------------------------------------------------------------------------*/

#ifdef UNDER_CE
int ReadJpegSections (HANDLE infile, 
                      IMAGE_INFO* imageinfo)
#else
int ReadJpegSections (FILE * infile, 
                      IMAGE_INFO* imageinfo)
#endif
{
    int a = 0;
    long lSize = 0;
#ifdef UNDER_CE
    int got = 0;
    int b = 0;
    lSize = GetFileSize(infile, NULL );
    SetFilePointer(infile, 0, NULL, FILE_BEGIN);
#else   
    fseek (infile , 0 , SEEK_END);
    lSize = ftell (infile);
    rewind (infile);
#endif

    PRINT ("Size is %d \n", (int)lSize);
    imageinfo->nProgressive = 0; /*Default value is non progressive*/

     
#ifdef UNDER_CE
    ReadFile(infile, &a, 1, &got, NULL);
#else   
    a = fgetc(infile);
#endif

#ifdef UNDER_CE
    ReadFile(infile, &b, 1, &got, NULL);
    if ( a != 0xff || b != M_SOI )  {
#else
    if ( a != 0xff || fgetc(infile) != M_SOI )  {
#endif  
        return 0;
    }
    for ( ;; )  {
        int itemlen = 0;
        int marker = 0;
        int ll = 0,lh = 0, got = 0;
        uchar * Data = NULL;

        for ( a=0;a<7;a++ ) {
#ifdef UNDER_CE
            ReadFile(infile, &marker, 1, &got, NULL);
#else
            marker = fgetc(infile);
#endif          
            PRINT("MARKER IS %x\n",marker);

            if ( marker != 0xff )   {
                break;
            }
            if ( a >= 6 )   {
                PRINT("too many padding bytes\n");
                if ( Data != NULL ) {
                    free(Data);
                    Data=NULL;
                }
                return 0;
            }
        }
        if ( marker == 0xff )   {
            /* 0xff is legal padding, but if we get that many, something's wrong.*/
            PRINT("too many padding bytes!");
        }

        /* Read the length of the section.*/
#ifdef UNDER_CE
        ReadFile(infile, &lh, 1, &got, NULL);
        ReadFile(infile, &ll, 1, &got, NULL);
#else       
        lh = fgetc(infile);
        ll = fgetc(infile);
#endif      

        itemlen = (lh << 8) | ll;

        if ( itemlen < 2 )  {
            PRINT("invalid marker");
        }

        Data = (uchar *)malloc(itemlen);
        if ( Data == NULL ) {
            PRINT("Could not allocate memory");
        }

        /* Store first two pre-read bytes. */
        Data[0] = (uchar)lh;
        Data[1] = (uchar)ll;

#ifdef UNDER_CE
        ReadFile(infile, Data+2, itemlen-2, &got, NULL);
#else
        got = fread(Data+2, 1, itemlen-2, infile); /* Read the whole section.*/
#endif      
        if ( got != itemlen-2 ) {
            PRINT("Premature end of file?");
        }

        PRINT("Jpeg section marker 0x%02x size %d\n",marker, itemlen);
        switch ( marker )   {
        
        case M_SOS:  
            if ( Data != NULL ) {
                free(Data);
                Data=NULL;
            }

            return lSize;

        case M_EOI:   
            PRINT("No image in jpeg!\n");
            if ( Data != NULL ) {
                free(Data);
                Data=NULL;
            }
            return 0;

        case M_COM: /* Comment section  */

            break;

        case M_JFIF:

            break;

        case M_EXIF:

            break;

        case M_SOF2:
            PRINT("nProgressive IMAGE!\n");
            imageinfo->nProgressive=1;

        case M_SOF0: 
        case M_SOF1: 
        case M_SOF3: 
        case M_SOF5: 
        case M_SOF6: 
        case M_SOF7: 
        case M_SOF9: 
        case M_SOF10:
        case M_SOF11:
        case M_SOF13:
        case M_SOF14:
        case M_SOF15:

            imageinfo->nHeight = Get16m(Data+3);
            imageinfo->nWidth = Get16m(Data+5);
            imageinfo->format = GetYUVformat(Data);
            switch (imageinfo->format) {
            case OMX_COLOR_FormatYUV420PackedPlanar:
                printf("APP:: Image chroma format is OMX_COLOR_FormatYUV420PackedPlanar\n");
                break;
            case OMX_COLOR_FormatYUV411Planar:
                printf("APP:: Image chroma format is OMX_COLOR_FormatYUV411Planar\n");
                break;
            case OMX_COLOR_FormatCbYCrY:
                printf("APP:: Image chroma format is OMX_COLOR_FormatYUV422Interleaved\n");
                break;
            case OMX_COLOR_FormatYUV444Interleaved:
                 printf("APP:: Image chroma format is OMX_COLOR_FormatYUV444Interleaved\n");
                 break;
            case OMX_COLOR_FormatL8:
                printf("APP:: Image chroma format is OMX_COLOR_FormatL8 \n");
                break;
            default:
                 printf("APP:: Cannot find Image chroma format \n");
                 imageinfo->format = OMX_COLOR_FormatUnused;
                 break;
            }
            printf("APP:: Image Width x Height = %u * %u\n", Get16m(Data+5), Get16m(Data+3)  );
            /*
            PRINT("JPEG image is %uw * %uh,\n", Get16m(Data+3), Get16m(Data+5)  );

            if ( *(Data+9)==0x41 )  {
                PRINT("THIS IS A YUV 411 ENCODED IMAGE \n" );
                imageinfo->format= 1;
            }
            */

            if ( Data != NULL ) {
                free(Data);
                Data=NULL;
            }
            break;
        default:
            /* Skip any other sections.*/
            break;
        }

        if ( Data != NULL ) {
            free(Data);
            Data=NULL;
        }
    }

    
    return 0;
}



OMX_ERRORTYPE EventHandler(OMX_HANDLETYPE hComponent,
                                               OMX_PTR pAppData,
                                               OMX_EVENTTYPE eEvent, 
                                    OMX_U32 nData1,
                                               OMX_U32 data2,
                                               OMX_PTR pEventData)
{
    JPEGD_EVENTPRIVATE MyEvent;

    MyEvent.eEvent = eEvent;
    MyEvent.nData1 = nData1;
    MyEvent.nData2 = data2;
    MyEvent.pAppData = pAppData;
    MyEvent.pEventInfo = pEventData;

    PRINT("eEvent = %x, nData1 = %x\n\n", eEvent, (unsigned int)nData1);

    switch ( eEvent ) {
        case OMX_EventCmdComplete:
            PRINT ("Component State Changed To %ld\n", data2);
            break;
        
        case OMX_EventError:
            if ( nData1 != OMX_ErrorNone ){
                printf ("APP:: ErrorNotification received: Error Num %x,\tSeverity = %ld\n", (unsigned int)nData1, data2);
                if ((nData1 == OMX_ErrorHardware) || (nData1 == OMX_ErrorInsufficientResources)){
                    printf("\nAPP:: OMX_ErrorHardware. Deinitialization of the component....\n\n");
                    break;
                }
                else if(nData1 == OMX_ErrorResourcesPreempted) {
                    bPreempted = 1;
                    PRINT("APP:: OMX_ErrorResourcesPreempted !\n\n");
                }
                else if(nData1 == OMX_ErrorInvalidState){
                    return OMX_ErrorNone;
                }
                else if(nData1 == OMX_ErrorTimeout){
                    return OMX_ErrorNone;
                }
                else {
                    DEINIT_FLAG = 1;
                }
            }
            break;     

        case OMX_EventResourcesAcquired:
            bPreempted = 0;
            break;  
        case OMX_EventBufferFlag:
            printf("APP:: OMX_EventBufferFlag received\n");
            break;
        case OMX_EventPortSettingsChanged:
        case OMX_EventMax:
        case OMX_EventMark:
            break;  
            
        default:
            break;
    }

    write(Event_Pipe[1], &MyEvent, sizeof(JPEGD_EVENTPRIVATE));

    return OMX_ErrorNone;
}



void FillBufferDone (OMX_HANDLETYPE hComponent, 
                     OMX_PTR ptr, 
                     OMX_BUFFERHEADERTYPE* pBuffHead)
{
    PRINT("OutBufHeader %p, pBuffer = %p\n", pBuffHead, pBuffHead->pBuffer);
    write(OpBuf_Pipe[1], &pBuffHead, sizeof(pBuffHead));
    /*	if (eError == -1) {	   		   
       PRINT ("Error while writing in OpBuf_pipe from app\n");

    }*/

}




void EmptyBufferDone(OMX_HANDLETYPE hComponent, 
                     OMX_PTR ptr, 
                     OMX_BUFFERHEADERTYPE* pBuffer)
{
    write(IpBuf_Pipe[1], &pBuffer, sizeof(pBuffer));
    /*	if (eError == -1) {	   		   
    	PRINT ("Error while writing in IpBuf_pipe from EmptyBufferDone\n");		
    }*/
}



#ifdef UNDER_CE
int fill_data (OMX_BUFFERHEADERTYPE *pBuf,
               HANDLE fIn, long lBuffUsed)
#else
int fill_data (OMX_BUFFERHEADERTYPE *pBuf, 
               FILE *fIn, int lBuffUsed)
#endif
{
    int nRead;

    PRINT(" checking buf address %x\n", (unsigned int)pBuf->pBuffer);

#ifdef UNDER_CE
    lSize = GetFileSize(fIn, NULL );
    SetFilePointer(fIn, 0, NULL, FILE_BEGIN);
#else    
    //fseek (fIn , 0 , SEEK_END);
    //lSize = ftell (fIn);
    //rewind (fIn);
#endif  

#ifdef UNDER_CE
    ReadFile(fIn, pBuf->pBuffer, lBuffUsed, &nRead, NULL);
#else
    nRead = fread(pBuf->pBuffer,1, lBuffUsed, fIn);
#endif  

    PRINT ("printing lsize %d \n", (int) lBuffUsed);
    PRINT( "Read %d bytes from file\n", nRead);

    pBuf->nFilledLen = nRead;
    return nRead;
}
/* This method will wait for the component to get to the event
 * specified by the DesiredEvent input. */
static OMX_ERRORTYPE WaitForEvent_JPEG(OMX_HANDLETYPE* pHandle,
                                  OMX_EVENTTYPE DesiredEvent,
                                  OMX_U32 nFDmax,
                                  OMX_STATETYPE DesiredState)
{
    OMX_ERRORTYPE error = OMX_ErrorNone;
    OMX_STATETYPE CurrentState;
    OMX_COMPONENTTYPE *pComponent = (OMX_COMPONENTTYPE *)pHandle;
    int nRetval;
    sigset_t set;

    while(1) {

        error = pComponent->GetState(pHandle, &CurrentState);
        if(error || CurrentState == DesiredState) { 
            break;
        }

        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(Event_Pipe[0], &rfds);
        sigemptyset(&set);
        sigaddset(&set,SIGALRM);
        
        nRetval = pselect(nFDmax+1, &rfds, NULL, NULL, NULL, &set);
        
        if ( nRetval == -1 ) {
#ifndef UNDER_CE    		
            perror("select()");
#endif			
            fprintf (stderr, " : Error \n");
            break;
        }
        
        if ( nRetval == 0 ) {
            PRINT("Waiting: Desired Event = %x, Desired State = %x \n",DesiredEvent, DesiredState);
        }
        
        if ( FD_ISSET(Event_Pipe[0], &rfds)) {
            
            JPEGD_EVENTPRIVATE EventPrivate;
            read(Event_Pipe[0], &EventPrivate, sizeof(JPEGD_EVENTPRIVATE));

            if (EventPrivate.eEvent == DesiredEvent && 
                EventPrivate.nData1 == OMX_CommandStateSet && 
                EventPrivate.nData2 == DesiredState) {
                PRINT("OMX_EventCmdComplete :: nData2 = %x\n", (unsigned int)EventPrivate.nData2);
                break;
            }
            else if (EventPrivate.eEvent == OMX_EventError && 
                EventPrivate.nData1 == OMX_ErrorInsufficientResources && 
                DesiredState == OMX_StateIdle) {
                printf("\n\n Received OMX_EventError: Error = 0x%x,\tSeverity = %ld\n\n", (unsigned int)EventPrivate.nData1, EventPrivate.nData2);
                error = OMX_ErrorInsufficientResources;   
                break;
            }
            else if (EventPrivate.eEvent == OMX_EventError && 
                EventPrivate.nData1 == OMX_ErrorResourcesLost && 
                DesiredState == OMX_StateLoaded) {
                printf("\n\n Received OMX_EventError: Error = 0x%x,\tSeverity = %ld\n\n", (unsigned int)EventPrivate.nData1, EventPrivate.nData2);
                break;
            }
        }
        
    }

    PRINT("Exit from Wait For Event function JPEGD\n");
    return error;
}



#ifdef UNDER_CE
int _tmain(int argc, TCHAR **argv) 
#else
int main(int argc, char** argv)
#endif
{

    OMX_HANDLETYPE pHandle = NULL;
    OMX_U32 AppData = 100;      
    OMX_CALLBACKTYPE JPEGCaBa = {   (void *)EventHandler,
                                                            (void*) EmptyBufferDone,
                                                            (void*)FillBufferDone};

    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_BOOL bError = OMX_FALSE;
    int nRetval;
    int nWidth;
    int nHeight;
    /*int nFramesent= 0;*/
    long lBuffused;
    int nOutformat;
    int nResizeMode;
    int nIndex1;
    int nIndex2;
    OMX_U32 nMCURow = 0;
    OMX_U32 nXOrg = 0;         /*Sectional decoding: X origin*/
    OMX_U32 nYOrg = 0;         /*Sectional decoding: Y origin*/
    OMX_U32 nXLength = 0;      /*Sectional decoding: X lenght*/
    OMX_U32 nYLength = 0;    /*Sectional decoding: Y lenght*/
    OMX_U32 i = 0;      /*Temporary counter*/

    IMAGE_INFO* imageinfo = NULL;
    OMX_PORT_PARAM_TYPE* pPortParamType = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pParamPortDef = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pInPortDef = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pOutPortDef = NULL;
    OMX_CONFIG_SCALEFACTORTYPE* pScaleFactor = NULL;
    OMX_PORT_PARAM_TYPE* pPortType = NULL;
    OMX_CUSTOM_IMAGE_DECODE_SECTION* pSectionDecode = NULL;
    OMX_CUSTOM_IMAGE_DECODE_SUBREGION* pSubRegionDecode = NULL;
	OMX_CUSTOM_RESOLUTION *pMaxResolution = NULL;

#ifdef UNDER_CE
    TCHAR* szInFile = NULL;
    TCHAR* szOutFile = NULL; 
    HANDLE fIn = NULL;
    HANDLE fOut = NULL;
    DWORD dwWritten;
#else
    char* szInFile = NULL;
    char* szOutFile = NULL; 
    FILE* fIn = NULL;
    FILE* fOut = NULL;
#endif

    int nFramesDecoded = 0;
    double t1 = 0;
    double t2 = 0;
    /*int nCountstarted = 0;*/
    OMX_S32 nPostProcCompId = 200; 
    /*int nDone = 0;*/
    int nExternal= 0;
    OMX_U8 * pTemp;
    OMX_BUFFERHEADERTYPE* pInBuffHead[NUM_OF_BUFFERS];
    OMX_BUFFERHEADERTYPE* pOutBuffHead[NUM_OF_BUFFERS];
    OMX_U8* pInBuffer = NULL;
    OMX_U8* pOutBuffer = NULL;
    int nFdmax;
    int nRead;
    OMX_INDEXTYPE nCustomIndex = OMX_IndexMax;
    OMX_U16 nBufferHdrSend = 0;
    OMX_U16 nSampleFactor = 32;
    OMX_U8 nNUM_OF_DECODING_BUFFERS = 1;    
    sigset_t set;    
    OMX_BOOL bWaitForLoaded = OMX_FALSE;
    int  nMaxFrames = 1;
    OMX_BUFFERHEADERTYPE* pInBufferHdr;    
    struct timeval tv1, tv2;

    /* validate command line args */
    if ( (argc < 5) || ((argc > 7) && (argc < 11)) ||(argc > 11)) {
#ifdef UNDER_CE
        printf("usage: %S <input.jpg> <output.yuv> <nOutformat-1:420 4:422 9:RGB16 10: RGB24 11:RGB32 12:BGR32> <nResizeMode> <nRepeat> <nSections> <nXOrigin> <nYOrigin> <nXLenght> <nYLenght>\n",
        argv[0]);
#else    	
        printf("usage: %s <input.jpg> <output.yuv> <nOutformat-1:Original(default)  4:422 9:RGB16 10: RGB24 11:RGB32 12:BGR32> <nResizeMode> <nRepeat> <nSections> <nXOrigin> <nYOrigin> <nXLenght> <nYLenght>\n",
        argv[0]);
#endif
        printf("nResizeMode 100:No Rescaling\nnResizeMode 50:Rescale to 50 percent of original size \n");
        printf("nResizeMode 25:Rescale to 25 percent of original size\nnResizeMode 12:Rescale to 12.5 percent of original size\n\n");
        printf("Example ./JpegTestCommon patterns/shrek.jpg shrek.yuv 1 50 \n");
        printf("Input: 720x480. Output: YUV420, 360x240\n\n");
        printf("nRepeat: It is an optional parameter. Range is 0 to 100. Repeatedly encode the same frame 'nRepeat+1' times");
        printf("Example ./JpegTestCommon patterns/shrek.jpg shrek.yuv 1 50 0\n");
        printf("Example ./JpegTestCommon patterns/shrek.jpg shrek.yuv 1 50 9\n");
        printf("Output: YUV420, 360x240\n\n");
        printf("<nSections> It's an optional parameter. Values: 0, 1, 2, ...\n");
        printf("Example ./JpegTestCommon patterns/shrek.jpg shrek.yuv 4 50 0 1\n");
        printf("Output: YUV422, 360x240\n\n");
        printf("SubRegion decode:\n");
        printf("<nXOrigin> <nYOrigin> <nXLenght> <nYLenght>\n");
        printf("Example ./JpegTestCommon patterns/shrek.jpg shrek.yuv 1 100 0 0 192 48 288 256\n");
        printf("Output: YUV420, 288x256\n\n");
        return -1;
    }


    szInFile = argv[1];
    szOutFile = argv[2];
#ifdef UNDER_CE
    nOutformat = _wtoi(argv[3]);
    nResizeMode = _wtoi(argv[4]);
#else	    
    nOutformat = atoi(argv[3]);
    nResizeMode = atoi(argv[4]);
#endif

    if (argc >= 6){
        nMaxFrames = atoi(argv[5]) + 1;
    }
    
    if(argc >= 7){
        nMCURow = atoi(argv[6]);
        if(nOutformat >= 9){
            printf("\nAPP: WARNING Sectional decoding is not supported for RGB color formats\n\n");
            nMCURow = 0;
        }
        if(nMCURow){
            nNUM_OF_DECODING_BUFFERS = NUM_OF_BUFFERS;
        }
        else{
            nNUM_OF_DECODING_BUFFERS = 1;
        }        
    }

    if(argc > 7){
        nXOrg = atoi(argv[7]);
        nYOrg = atoi(argv[8]);
        nXLength = atoi(argv[9]);
        nYLength = atoi(argv[10]);
    }

    imageinfo = (IMAGE_INFO *)malloc (sizeof (IMAGE_INFO ) );
    pPortParamType = (OMX_PORT_PARAM_TYPE*)malloc(sizeof(OMX_PORT_PARAM_TYPE));
    pParamPortDef = (OMX_PARAM_PORTDEFINITIONTYPE*)malloc(sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
    pInPortDef = (OMX_PARAM_PORTDEFINITIONTYPE*)malloc(sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
    pOutPortDef = (OMX_PARAM_PORTDEFINITIONTYPE*)malloc(sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
    pScaleFactor = (OMX_CONFIG_SCALEFACTORTYPE*)malloc(sizeof(OMX_CONFIG_SCALEFACTORTYPE));
    pPortType = (OMX_PORT_PARAM_TYPE*)malloc(sizeof(OMX_PORT_PARAM_TYPE));
    pSectionDecode = (OMX_CUSTOM_IMAGE_DECODE_SECTION*)malloc(sizeof(OMX_CUSTOM_IMAGE_DECODE_SECTION));
    pSubRegionDecode = (OMX_CUSTOM_IMAGE_DECODE_SUBREGION*)malloc(sizeof(OMX_CUSTOM_IMAGE_DECODE_SUBREGION));
	pMaxResolution = (OMX_CUSTOM_RESOLUTION *)malloc(sizeof(OMX_CUSTOM_RESOLUTION ));


    printf("\n------------------------------------------------\n");
    printf("OMX JPEG Decoder Test App built on " __DATE__ ":" __TIME__ "\n");
    printf("------------------------------------------------\n");
    printf("\nAPP:: Output File Name is %s \n", szOutFile);
    
    /* Create a pipe used to queue data from the callback. */
    nRetval = pipe(IpBuf_Pipe);
    PRINT("Pipe InBuf_Pipe just created\n");
    if ( nRetval != 0 )	{
        fprintf(stderr, "Error:Fill Data Pipe failed to open\n");
        goto EXIT;
    }

    PRINT("Pipe OpBuf_Pipe just created\n");
    nRetval = pipe(OpBuf_Pipe);
    if ( nRetval != 0 ) {
        fprintf(stderr, "Error:Empty Data Pipe failed to open\n");
        goto EXIT;
    }

    PRINT("Pipe Event_Pipe just created\n");
    nRetval = pipe(Event_Pipe);
    if ( nRetval != 0 ) {
        fprintf(stderr, "Error:Empty Data Pipe failed to open\n");
        goto EXIT;
    }

    /* save off the "max" of the handles for the selct statement */
    nFdmax = maxint(IpBuf_Pipe[0], OpBuf_Pipe[0]);
    nFdmax = maxint(Event_Pipe[0], nFdmax);

#ifdef DSP_MMU_FAULT_HANDLING
    /* LOAD BASE IMAGE FIRST TIME */
    LoadBaseImage();
#endif

    eError = TIOMX_Init();
    if ( eError != OMX_ErrorNone ) {
        PRINT("%d :: Error returned by TIOMX_Init()\n",__LINE__);
        goto EXIT;
    }

    /*--------------------------------------------------------------------------------
    *
    * Open the file of data to be rendered.  Since this is a just sample
    * application, the data is "rendered" to a test mixer.  So, the test
    * file better contain data that can be printed to the terminal w/o
    * problems or you will not be a happy [JPEGTest.c] fill_data():473: *** Read 997386 bytes from file
    **/
    PRINT("Opening input & output file\n");
#ifdef UNDER_CE
    fOut = CreateFile(szOutFile, GENERIC_WRITE, 0,
    NULL,CREATE_ALWAYS, 0, NULL);
    if (INVALID_HANDLE_VALUE == fOut)
    {
        PRINT("Error:  failed to create the output file %S\n",
        szOutFile);
        goto EXIT;
    }

    fIn = CreateFile(szInFile, GENERIC_READ, 0,
    NULL,OPEN_EXISTING, 0, NULL);
    if (INVALID_HANDLE_VALUE == fIn) {
        PRINT("Error:  failed to open the file %s for readonly\n" \
        "access\n", szInFile);
        goto EXIT;
    }
    
#else	

    fIn = fopen(szInFile, "r");
    if ( fIn == NULL ) {
        printf("\nError: failed to open the file <%s> for reading\n", 
        szInFile);
        goto EXIT;
    }
    PRINT("APP:: File %s opened \n" , szInFile);    
#endif

    lBuffused = ReadJpegSections(fIn , imageinfo);

    if (lBuffused == 0) {
        printf("The file size is 0. Maybe the format of the file is not correct\n");
        goto EXIT;
    }

    /* Load the JPEGDecoder Component */

    PRINT("Calling TIOMX_GetHandle\n");
    eError = TIOMX_GetHandle(&pHandle,StrJpegDecoder, (void *)&AppData, &JPEGCaBa);
    if ( (eError != OMX_ErrorNone) ||  (pHandle == NULL) ) {
        fprintf (stderr,"Error in Get Handle function\n");
        goto EXIT;
    }

    eError = OMX_GetParameter(pHandle, OMX_IndexParamImageInit, pPortType);
    if ( eError != OMX_ErrorNone ) {
        goto EXIT;
    }


    nIndex1 = pPortType->nStartPortNumber;
    nIndex2 = nIndex1 + 1;
    nHeight = imageinfo->nHeight;
    nWidth = imageinfo->nWidth;

    FixFrameSize(imageinfo);

    pScaleFactor->xWidth = (int)nResizeMode;
    pScaleFactor->xHeight = (int)nResizeMode;

    eError = OMX_SetConfig (pHandle, OMX_IndexConfigCommonScale, pScaleFactor); 
    if ( eError != OMX_ErrorNone ) {
        goto EXIT;
    }

    eError = OMX_GetParameter (pHandle, OMX_IndexParamPortDefinition, pInPortDef);
    if ( eError != OMX_ErrorNone ) {
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    if (pInPortDef->eDir == nIndex1 ) {
        pInPortDef->nPortIndex = nIndex1;
    }
    else {
        pInPortDef->nPortIndex = nIndex2;
    }

    /* Set the component's OMX_PARAM_PORTDEFINITIONTYPE structure (input) */
    /**********************************************************************/

    pInPortDef->nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);       
    pInPortDef->nVersion.s.nVersionMajor = 0x1; 
    pInPortDef->nVersion.s.nVersionMinor = 0x0; 
    pInPortDef->nVersion.s.nRevision = 0x0;     
    pInPortDef->nVersion.s.nStep = 0x0;
    pInPortDef->nPortIndex = 0x0;
    pInPortDef->eDir = OMX_DirInput;
    pInPortDef->nBufferCountActual =1;
    pInPortDef->nBufferCountMin = 1;
    pInPortDef->bEnabled = OMX_TRUE;
    pInPortDef->bPopulated = OMX_FALSE;
    pInPortDef->eDomain = OMX_PortDomainImage;
    pInPortDef->format.image.cMIMEType = "JPEGDEC";
    pInPortDef->format.image.pNativeRender = NULL; 
    pInPortDef->format.image.nFrameWidth = imageinfo->nWidth;
    pInPortDef->format.image.nFrameHeight = imageinfo->nHeight;
    pInPortDef->format.image.nStride = -1; 
    pInPortDef->format.image.nSliceHeight = -1;
    pInPortDef->format.image.bFlagErrorConcealment = OMX_FALSE;
    pInPortDef->format.image.eColorFormat =OMX_COLOR_FormatCbYCrY ;
    pInPortDef->format.image.eCompressionFormat = OMX_IMAGE_CodingJPEG;
    pInPortDef->nBufferSize = lBuffused;


    if (imageinfo->format == OMX_COLOR_FormatYCbYCr ||
        imageinfo->format == OMX_COLOR_FormatYUV444Interleaved ||
        imageinfo->format == OMX_COLOR_FormatUnused) {
        pInPortDef->format.image.eColorFormat = OMX_COLOR_FormatCbYCrY;
    }
    else {
        pInPortDef->format.image.eColorFormat = OMX_COLOR_FormatYUV420PackedPlanar;
    }

    PRINT("Calling OMX_SetParameter\n");

	/* Set max width & height value*/
    eError = OMX_GetExtensionIndex(pHandle, "OMX.TI.JPEG.decoder.Param.SetMaxResolution", (OMX_INDEXTYPE*)&nCustomIndex);
    if ( eError != OMX_ErrorNone ) {
        printf ("JPEGDec test:: %d:error= %x\n", __LINE__, eError);
        goto EXIT;
    }

	pMaxResolution->nWidth = imageinfo->nWidth;
	pMaxResolution->nHeight = imageinfo->nHeight;

	eError = OMX_SetParameter (pHandle, nCustomIndex, pMaxResolution);
    if ( eError != OMX_ErrorNone ) {
        printf ("JPEGDec test:: %d:error= %x\n", __LINE__, eError);
        goto EXIT;
    }

    eError = OMX_SetParameter (pHandle, OMX_IndexParamPortDefinition, pInPortDef);
    if ( eError != OMX_ErrorNone ) {
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }
    
    memset(pOutPortDef, 0x0, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));    
    eError = OMX_GetParameter (pHandle, OMX_IndexParamPortDefinition, pOutPortDef);
    if ( eError != OMX_ErrorNone ) {
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    if (pOutPortDef->eDir == nIndex1 ) {
        pOutPortDef->nPortIndex = nIndex1;
    }
    else {
        pOutPortDef->nPortIndex = nIndex2;
    }
    
    /*****************************************************************/
    /* Set the component's OMX_PARAM_PORTDEFINITIONTYPE structure (Output) */
    /**********************************************************************/
    pOutPortDef->nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);       
    pOutPortDef->nVersion.s.nVersionMajor = 0x1; 
    pOutPortDef->nVersion.s.nVersionMinor = 0x0; 
    pOutPortDef->nVersion.s.nRevision = 0x0;     
    pOutPortDef->nVersion.s.nStep = 0x0;
    pOutPortDef->nPortIndex = 0x1;
    pOutPortDef->eDir = OMX_DirOutput;
    pOutPortDef->nBufferCountActual = nNUM_OF_DECODING_BUFFERS;
    pOutPortDef->nBufferCountMin = 1;
    pOutPortDef->bEnabled = OMX_TRUE;
    pOutPortDef->bPopulated = OMX_FALSE;
    pOutPortDef->eDomain = OMX_PortDomainImage;

    /* OMX_IMAGE_PORTDEFINITION values for Output port */
    pOutPortDef->format.image.cMIMEType = "JPEGDEC";
    pOutPortDef->format.image.pNativeRender = NULL;	
    pOutPortDef->format.image.nFrameWidth = imageinfo->nWidth;
    pOutPortDef->format.image.nFrameHeight = imageinfo->nHeight;
    pOutPortDef->format.image.nStride = -1;	
    pOutPortDef->format.image.nSliceHeight = -1;
    pOutPortDef->format.image.bFlagErrorConcealment = OMX_FALSE;
    pOutPortDef->format.image.eCompressionFormat = OMX_IMAGE_CodingUnused;

    PRINT("nWidth and nHeight = %d and %d\n",nWidth,nHeight);
    if ( nOutformat == 4 ) {
        pOutPortDef->format.image.eColorFormat = OMX_COLOR_FormatCbYCrY;	
    }
    else if (nOutformat == 9) {
        pOutPortDef->format.image.eColorFormat = OMX_COLOR_Format16bitRGB565;
        PRINT("color format is %d\n", pOutPortDef->format.image.eColorFormat);
    }
    else if (nOutformat == 10) {
        pOutPortDef->format.image.eColorFormat = OMX_COLOR_Format24bitRGB888;
    }
    else if (nOutformat == 11) {
        pOutPortDef->format.image.eColorFormat = OMX_COLOR_Format32bitARGB8888;
    }
    else if (nOutformat == 12) {
        pOutPortDef->format.image.eColorFormat = OMX_COLOR_Format32bitBGRA8888;
    }
    else { /*Set DEFAULT (original) color format*/
        pOutPortDef->format.image.eColorFormat = imageinfo->format; /*Setting input original format */

        if(imageinfo->format == OMX_COLOR_Format16bitRGB565 || 
                imageinfo->format == OMX_COLOR_Format24bitRGB888 || 
                imageinfo->format == OMX_COLOR_Format32bitARGB8888 || 
                imageinfo->format == OMX_COLOR_Format32bitBGRA8888 || 
                imageinfo->format == OMX_COLOR_FormatL8){
            for(i = 0; i < strlen(szOutFile); i++){                       
                if(szOutFile[i]=='.'){              
                    if(szOutFile[i+1]=='y'){
                        szOutFile[i+1]='r';
                        szOutFile[i+2]='a';
                        szOutFile[i+3]='w';
                        szOutFile[i+4]='\0';
                        printf("\n\nAPP::--WARNING:\nIncorrect output file extension. Changing output file name extension--\n");
                        printf("APP:: New file name: %s\n\n\n", szOutFile);
                        break;
                    }
                    break; 
                }               
            }
        }
    }

    fOut = fopen(szOutFile, "w");
    if ( fOut == NULL ) {
        printf("\nError: failed to open the file <%s> for writing", szOutFile);
        goto EXIT;
    }

    if(nResizeMode == 800){
        nWidth *= 8;
        nHeight *= 8;
    }
    else if(nResizeMode == 400){
        nWidth *= 4;
        nHeight *= 4;
    }
    else if(nResizeMode == 200){
        nWidth *= 2;
        nHeight *= 2;
    }
    else if (nResizeMode == 50) {
        nWidth /= 2;
        nHeight /= 2;
    } else if (nResizeMode == 25) {
        nWidth /= 4;
        nHeight /= 4;
    } else if (nResizeMode == 12) {
        nWidth /= 4;
        nHeight /= 4;
    }

    if(nMCURow == 0){ /*Full frame decoding*/
        if ( nOutformat == 1 ) { /* Buffer size depends on the Original color format*/
            if (imageinfo->format == OMX_COLOR_FormatYUV420PackedPlanar ||
                imageinfo->format == OMX_COLOR_FormatYUV411Planar) {    
                pOutPortDef->nBufferSize = ( int )((( nWidth * nHeight) *3 ) /2);
                } 
            else if (imageinfo->format == OMX_COLOR_FormatCbYCrY) {
                pOutPortDef->nBufferSize =  ( int ) ((nWidth * nHeight) *2);
            }
            else if (imageinfo->format == OMX_COLOR_FormatYUV444Interleaved) {
                    pOutPortDef->nBufferSize =  ( int ) ((nWidth * nHeight) *3);
            }
            else {
                pOutPortDef->nBufferSize = ( int ) (nWidth * nHeight) * 2;
            }
        }
        else if (nOutformat == 4 || nOutformat == 9) {        /* OMX_COLOR_FormatCbYCrY & OMX_COLOR_Format16bitRGB565*/
            pOutPortDef->nBufferSize =  (int)((nWidth * nHeight) * 2);
        }
        else if (nOutformat == 10) { /* OMX_COLOR_Format24bitRGB888 */
           pOutPortDef->nBufferSize = (int) ((nWidth * nHeight) * 3);
        } 
        else if (nOutformat == 11 || nOutformat == 12) { /* OMX_COLOR_Format32bitARGB8888 & OMX_COLOR_Format32bitBGRA8888*/
           pOutPortDef->nBufferSize = (int) ((nWidth * nHeight) * 4);
        }
        else { 
            eError = OMX_ErrorBadParameter;
            goto EXIT;
        }
    }
    else{ /*Slice Decoding*/
        switch(imageinfo->format){
            case OMX_COLOR_FormatYUV420PackedPlanar:
                nSampleFactor = 16;
                break;
            case OMX_COLOR_FormatYUV411Planar:
            case OMX_COLOR_FormatCbYCrY:
            case OMX_COLOR_FormatYUV444Interleaved:
                nSampleFactor = 8;
                break;
            default:
                nSampleFactor = 8;
                break;
        }


        if(nResizeMode == 12){ /* setting to 13 instead of 12.5 */
            nResizeMode = 13;
        }

        if ( nOutformat == 1 ) { /* Buffer size depends on the Original color format*/
            if (imageinfo->format == OMX_COLOR_FormatYUV420PackedPlanar ||
                    imageinfo->format == OMX_COLOR_FormatYUV411Planar) {   
                pOutPortDef->nBufferSize = (OMX_U32)(((nWidth*3)/2)*(nSampleFactor*nMCURow*nResizeMode)/100);

            }
            else if (imageinfo->format == OMX_COLOR_FormatCbYCrY){
                    pOutPortDef->nBufferSize =  (OMX_U32) ( (nWidth * 2) * (nSampleFactor * nMCURow*nResizeMode)/100);
            }
            else if (imageinfo->format == OMX_COLOR_FormatYUV444Interleaved) {
                pOutPortDef->nBufferSize =  (OMX_U32) ( (nWidth * 3) * (nSampleFactor * nMCURow*nResizeMode)/100);
            }
            else{
                pOutPortDef->nBufferSize =  (OMX_U32) ( (nWidth) * (nSampleFactor * nMCURow*nResizeMode)/100);
            }
        }
        else if(nOutformat == 4){ /*YUV422 ILE */
            pOutPortDef->nBufferSize = (OMX_U32)((nWidth * 2) * (nSampleFactor * nMCURow * nResizeMode)/100);
        }
    }

    printf("APP:: Output buffer size is %ld\n", pOutPortDef->nBufferSize);
    printf("APP:: Output color format is ");
    if (nOutformat == 4){
        printf("OMX_COLOR_FormatCbYCrY (YUV422ILE)\n");
    }
    else if (nOutformat == 9){
        printf("OMX_COLOR_Format16bitRGB565 (RGB16)\n");
    }
    else if (nOutformat == 10){
        printf("OMX_COLOR_Format24bitRGB888 (BGR24)\n");
    }
    else if (nOutformat == 11){
        printf("OMX_COLOR_Format32bitARGB8888 (ARGB32)\n");
	}
    else if (nOutformat == 12){
        printf("OMX_COLOR_Format32bitBGRA8888 (BGRA32)\n");
    }
    else{
         if (imageinfo->format == OMX_COLOR_FormatYUV420PackedPlanar){
            printf("OMX_COLOR_FormatYUV420PackedPlanar\n");
         }
         else if (imageinfo->format == OMX_COLOR_FormatYUV411Planar) {
            printf("OMX_COLOR_FormatYUV411Planar\n");
        }
        else if (imageinfo->format == OMX_COLOR_FormatCbYCrY){
                printf("OMX_COLOR_FormatCbYCrY (YUV422ILE)\n");
        }
        else if (imageinfo->format == OMX_COLOR_FormatYUV444Interleaved) {
            printf("OMX_COLOR_FormatYUV444Interleaved\n");
        }
        else if (imageinfo->format == OMX_COLOR_FormatL8){
            printf("OMX_COLOR_FormatL8 (Gray 8)\n");
        }
        else{
            printf("Unknow format\n");
        }
    }

    eError = OMX_SetParameter (pHandle, OMX_IndexParamPortDefinition, pOutPortDef);
    if ( eError != OMX_ErrorNone ) {
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    eError = OMX_GetExtensionIndex(pHandle, "OMX.TI.JPEG.decoder.Config.ProgressiveFactor", (OMX_INDEXTYPE*)&nCustomIndex);
    if ( eError != OMX_ErrorNone ) {
        printf ("JPEGDec test:: %d:error= %x\n", __LINE__, eError);
        goto EXIT;
    }
    eError = OMX_SetConfig (pHandle, nCustomIndex, &(imageinfo->nProgressive));
    if ( eError != OMX_ErrorNone ) {
        printf ("JPEGDec test:: %d:error= %x\n", __LINE__, eError);
        goto EXIT;
    }

    eError = OMX_GetExtensionIndex(pHandle, "OMX.TI.JPEG.decoder.Config.OutputColorFormat", (OMX_INDEXTYPE*)&nCustomIndex);
    if ( eError != OMX_ErrorNone ) {
        printf ("JPEGDec test:: %d:error= %x\n", __LINE__, eError);
        goto EXIT;
    }

    eError = OMX_SetConfig (pHandle, nCustomIndex, &(pOutPortDef->format.image.eColorFormat));
    if ( eError != OMX_ErrorNone ) {
        printf ("JPEGDec test:: %d:error= %x\n", __LINE__, eError);
        goto EXIT;
    }

    if(nMCURow){
        eError = OMX_GetExtensionIndex(pHandle, "OMX.TI.JPEG.decoder.Param.SectionDecode", (OMX_INDEXTYPE*)&nCustomIndex);
        if ( eError != OMX_ErrorNone ) {
            printf ("JPEGDec test:: %d:error= %x\n", __LINE__, eError);
            goto EXIT;
        }
        eError = OMX_GetParameter(pHandle, nCustomIndex, pSectionDecode);
        if ( eError != OMX_ErrorNone ) {
            printf ("JPEGDec test:: %d:error= %x\n", __LINE__, eError);
            goto EXIT;
        }
        pSectionDecode->nMCURow = nMCURow; /*number of slices*/
        pSectionDecode->bSectionsInput = OMX_FALSE; /*Should be false at input port. Unsupported slice dec at input port at the moment*/
        pSectionDecode->bSectionsOutput = OMX_TRUE; /*Should be true if slice at output port*/
        eError = OMX_SetParameter(pHandle, nCustomIndex, pSectionDecode);
        if ( eError != OMX_ErrorNone ) {
            printf ("JPEGDec test:: %d:error= %x\n", __LINE__, eError);
            goto EXIT;
        }
    }

    if(nXOrg || nYOrg || nXLength || nYLength){
        eError = OMX_GetExtensionIndex(pHandle, "OMX.TI.JPEG.decoder.Param.SubRegionDecode", (OMX_INDEXTYPE*)&nCustomIndex);
        if ( eError != OMX_ErrorNone ) {
            printf ("JPEGDec test:: %d:error= %x\n", __LINE__, eError);
            goto EXIT;
        }

        eError = OMX_GetParameter(pHandle, nCustomIndex, pSubRegionDecode);
        if ( eError != OMX_ErrorNone ) {
            printf ("JPEGDec test:: %d:error= %x\n", __LINE__, eError);
            goto EXIT;
        }
        
        pSubRegionDecode->nXOrg = nXOrg;
        pSubRegionDecode->nYOrg = nYOrg;
        pSubRegionDecode->nXLength = nXLength;
        pSubRegionDecode->nYLength = nYLength;
        eError = OMX_SetParameter(pHandle, nCustomIndex, pSubRegionDecode);
        if ( eError != OMX_ErrorNone ) {
            printf ("JPEGDec test:: %d:error= %x\n", __LINE__, eError);
            goto EXIT;
        }
        PRINT("pSubRegionDecode set\n");
    }

    /********* EXTERNAL BUFFER ****************/

    if ( nExternal == 1 ) {
        pTemp=(OMX_U8*)malloc(pInPortDef->nBufferSize+256);
        pTemp+=128;
        pInBuffer = pTemp;
        pTemp=(OMX_U8*)malloc(pOutPortDef->nBufferSize+256);
        pTemp+=128;
        pOutBuffer = pTemp;
 
        eError = OMX_UseBuffer(pHandle, &pInBuffHead[0], nIndex1, pInBuffHead, pInPortDef->nBufferSize, pInBuffer);
        for(nBufferHdrSend = 0; (nBufferHdrSend < nNUM_OF_DECODING_BUFFERS); nBufferHdrSend++){
            eError = OMX_UseBuffer(pHandle, &pOutBuffHead[nBufferHdrSend], nIndex2 , pOutBuffHead, pOutPortDef->nBufferSize, pOutBuffer); 
        }
    }
    else {

        OMX_AllocateBuffer(pHandle, &pInBuffHead[0], nIndex1, (void *)&nPostProcCompId, pInPortDef->nBufferSize); 

        for(nBufferHdrSend = 0; (nBufferHdrSend < nNUM_OF_DECODING_BUFFERS); nBufferHdrSend++){
            OMX_AllocateBuffer(pHandle, &pOutBuffHead[nBufferHdrSend], nIndex2, (void *)&nPostProcCompId, pOutPortDef->nBufferSize);  
            PRINT("APP:: AllocateBuff Hdr = %p ; pBuffer = %p\n", pOutBuffHead[nBufferHdrSend], pOutBuffHead[nBufferHdrSend]->pBuffer);
       }

    }

    gettimeofday(&tv1, NULL);
    eError = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle ,NULL);
    if ( eError != OMX_ErrorNone ) {
        fprintf (stderr,"Error from SendCommand-Idle(Init) State function\n");
        goto EXIT;
    }

    WaitForEvent_JPEG(pHandle, OMX_EventCmdComplete, nFdmax, OMX_StateIdle);

    PRINT("Transitioned to IDLE State\n");
    /*PRINT("from loaded to idle: %ld %ld %ld %ld\n", tv1.tv_sec, tv1.tv_usec, tv2.tv_sec, tv2.tv_usec);*/

    gettimeofday(&tv1, NULL);
    eError = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateExecuting, NULL);
    if ( eError != OMX_ErrorNone ) {
        fprintf (stderr,"eError from SendCommand-Executing State function\n");
        goto EXIT;
    }

    WaitForEvent_JPEG(pHandle, OMX_EventCmdComplete, nFdmax, OMX_StateExecuting);
    gettimeofday(&tv2, NULL);

    PRINT("Transitioned to EXECUTE State\n");
    /*PRINT("from idle to exec: %ld %ld %ld %ld\n", tv1.tv_sec, tv1.tv_usec, tv2.tv_sec, tv2.tv_usec);*/

    rewind(fIn);
    nRead = fill_data(pInBuffHead[0], fIn, lBuffused);

#ifndef UNDER_CE				

    gettimeofday(&tim, NULL);
    t1=tim.tv_sec+(tim.tv_usec/1000000.0);
#endif

    pInBuffHead[0]->nFilledLen = nRead;
    pInBuffHead[0]->nFlags = OMX_BUFFERFLAG_EOS;

    OMX_EmptyThisBuffer(pHandle, pInBuffHead[0]);
    DEINIT_FLAG = 0;

    if(nMCURow){
        for(nBufferHdrSend = 0; nBufferHdrSend < nNUM_OF_DECODING_BUFFERS; nBufferHdrSend++){
            OMX_FillThisBuffer(pHandle, pOutBuffHead[nBufferHdrSend]);
        }
    }
    else{
        OMX_FillThisBuffer(pHandle, pOutBuffHead[0]);
    }

    /** Handle the component's requests for data until we run out of data.  Do this 
    * in a way that will allow the UI to continue to run (if there is a UI, which
    * this sample application does NOT have)
    **/

    PRINT("\n\n\n*******************************************\n\n\n");
    
    while (DEINIT_FLAG == 0){

        if (bPreempted){
            goto DEINIT2;
        }

        fd_set rfds;
        FD_ZERO(&rfds);

        FD_SET(IpBuf_Pipe[0], &rfds);
        FD_SET(OpBuf_Pipe[0], &rfds);
        FD_SET(Event_Pipe[0], &rfds);

        sigemptyset(&set);
        sigaddset(&set,SIGALRM);
        nRetval = pselect(nFdmax+1, &rfds, NULL, NULL, NULL, &set);
        if ( nRetval == -1 ) {
#ifndef UNDER_CE    		
            perror("select()");
#endif			
            fprintf (stderr, " : Error \n");
            break;
        }

        /**
        * If FD_ISSET then there is data available in the pipe.  
        * Read it and get the buffer data out.  
        * Then re-fill the buffer and send it back.
        **/
        if ( FD_ISSET(Event_Pipe[0], &rfds)) {
            
            JPEGD_EVENTPRIVATE EventPrivate;
            read(Event_Pipe[0], &EventPrivate, sizeof(JPEGD_EVENTPRIVATE));
            switch(EventPrivate.eEvent) {

                case OMX_EventError:
                    DEINIT_FLAG = OMX_TRUE;
                    bError = OMX_TRUE;
                    printf("APP:: Waiting for OMX_StateInvalid... \n");
                    WaitForEvent_JPEG(pHandle, OMX_EventCmdComplete, nFdmax, OMX_StateInvalid);
                    printf("APP:: At Invalid state.\n");
                    goto EXIT;
                    break;

                case OMX_EventBufferFlag:
                    printf("APP:: Unloading component...\n");
                    break;

                default:
                    printf("APP:: Non-error event rise. Event -> 0x%x\n", EventPrivate.eEvent);
                    break;
            }
        }

        if ( FD_ISSET(IpBuf_Pipe[0], &rfds)){
            read(IpBuf_Pipe[0], &pInBufferHdr, sizeof(pInBufferHdr));

            if ( (!nMCURow) &&  (nFramesDecoded < nMaxFrames)){
                pInBuffHead[0]->nFilledLen = nRead;
                pInBuffHead[0]->nFlags = OMX_BUFFERFLAG_EOS;
                OMX_EmptyThisBuffer(pHandle, pInBufferHdr);
            }
        }

        if ( FD_ISSET(OpBuf_Pipe[0], &rfds) ) {
            OMX_BUFFERHEADERTYPE* pBuf;
#ifndef UNDER_CE
            gettimeofday(&tim, NULL);
            t2=tim.tv_sec+(tim.tv_usec/1000000.0);
            /*	printf("\n%.6lf seconds elapsed\n", t2-t1);  */
#endif
            read(OpBuf_Pipe[0], &pBuf, sizeof(pBuf));

            PRINT("%d ::App: Read from OpBuf_Pipe OutBufHeader %p, nFilledLen = %d\n", __LINE__, pBuf, (int)pBuf->nFilledLen);

#ifdef UNDER_CE
            WriteFile(fOut, pBuf->pBuffer, pBuf->nFilledLen, &dwWritten, NULL);
#else
            fwrite(pBuf->pBuffer, 1, (int ) (int)pBuf->nFilledLen, fOut);
            fflush(fOut);
#endif

            nFramesDecoded++;
            PRINT("\n\n\n*******************************************\n\n\n");

            if (( (nMCURow) &&  (pBuf->nFlags && OMX_BUFFERFLAG_EOS)) ||
               ( (!nMCURow) &&  (nFramesDecoded >= nMaxFrames))){
                break;
            }
            PRINT("---------------------------- Output Buff FRAME %d ----------------------------\n", nFramesDecoded);

            eError = OMX_FillThisBuffer(pHandle, pBuf);
            if ( eError != OMX_ErrorNone ) {
                printf ("Error from OMX_FillThisBuffer\n");
                goto EXIT;
            }

            if (!nMCURow){
                rewind(fOut);
            }
        }
    }/***** End of While Loop *****/

    if (bError == OMX_FALSE){
        PRINT("APP:: Sending back to Idle\n");

        eError = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateIdle, NULL);
        if ( eError != OMX_ErrorNone ) {
            fprintf (stderr,"Error from SendCommand-Idle(nStop) State function\n");
            goto EXIT;
        }
    }
DEINIT2:

    if (bError == OMX_FALSE){
        WaitForEvent_JPEG(pHandle, OMX_EventCmdComplete, nFdmax, OMX_StateIdle);

        eError = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateLoaded, NULL);
        if ( eError != OMX_ErrorNone ) {
            fprintf (stderr,"Error from SendCommand-Idle State function\n");
            goto EXIT;
        }

        bWaitForLoaded = OMX_TRUE;
    }
//DEINIT1:    

    eError = OMX_SendCommand(pHandle, OMX_CommandPortDisable, 0x0, NULL);
    if ( eError != OMX_ErrorNone ) {
        fprintf (stderr,"APP:: Error from SendCommand-PortDisable function. Input port.\n");
        goto EXIT;
    }

    eError = OMX_SendCommand(pHandle, OMX_CommandPortDisable, 0x1, NULL);
    if ( eError != OMX_ErrorNone ) {
        fprintf (stderr,"APP:: Error from SendCommand-PortDisable function. Output port.\n");
        goto EXIT;
    }

    /* Free buffers */
    if ( nExternal==1 )
    {
        pOutBuffer-=128;
        pInBuffer-=128;
        eError = OMX_FreeBuffer(pHandle, nIndex1, pInBuffHead[0]); 
        for(nBufferHdrSend = 0; nBufferHdrSend < nNUM_OF_DECODING_BUFFERS; nBufferHdrSend++){
            eError = OMX_FreeBuffer(pHandle, nIndex2, pOutBuffHead[nBufferHdrSend]); 
        }

        free(pOutBuffer);
        free(pInBuffer);

    } 
    else {
        eError = OMX_FreeBuffer(pHandle, nIndex1, pInBuffHead[0]); 
        for (nBufferHdrSend = 0; nBufferHdrSend < nNUM_OF_DECODING_BUFFERS; nBufferHdrSend ++) {
            eError = OMX_FreeBuffer(pHandle, nIndex2, pOutBuffHead[nBufferHdrSend]);  
        }
    }

    if (bWaitForLoaded && (bError == OMX_FALSE)){
        WaitForEvent_JPEG(pHandle, OMX_EventCmdComplete, nFdmax, OMX_StateLoaded);
    }

EXIT:
    if (pPortParamType) {
        free(pPortParamType);
        pPortParamType = NULL;
    }
    if (pParamPortDef) {
        free(pParamPortDef);
        pParamPortDef = NULL;
    }
    if (pInPortDef) {
        free(pInPortDef);
        pInPortDef = NULL;
    }
    if (pOutPortDef) {
        free(pOutPortDef);
        pOutPortDef = NULL;
    }
    if (imageinfo) {
        free(imageinfo);
        imageinfo = NULL;
    }
    if (pScaleFactor) {
        free(pScaleFactor);
        pScaleFactor = NULL;
    }
    if (pPortType) {
        free(pPortType);
        pPortType = NULL;
    }
    if(pSectionDecode){
        free(pSectionDecode);
        pSectionDecode = NULL;
    }
    if(pSubRegionDecode){
        free(pSubRegionDecode);
        pSubRegionDecode = NULL;
    }
	if(pMaxResolution) {
		free(pMaxResolution);
		pMaxResolution = NULL;
	}

    if ( fOut != NULL ) {
        PRINT("Closing Output File\n");
#ifdef UNDER_CE
        CloseHandle(fOut);
#else            
        fclose(fOut);
#endif
    }

    if ( fIn != NULL ) {
        PRINT("Closing Input File\n");
#ifdef UNDER_CE
        CloseHandle(fIn);
#else
        fclose(fIn);
#endif
    }

    if (pHandle) {
        eError = TIOMX_FreeHandle(pHandle);
        if ( (eError != OMX_ErrorNone) )	{
            fprintf (stderr,"Error in Free Handle function\n");
        }
    }

#ifdef DSP_MMU_FAULT_HANDLING
    if(bError) {
        LoadBaseImage();
    }
#endif

    eError = TIOMX_Deinit();
    if ( eError != OMX_ErrorNone ) {
        PRINT("Error returned by OMX_Init()\n");
        goto EXIT;
    }
    
    return eError;
}


#ifdef DSP_MMU_FAULT_HANDLING

int LoadBaseImage() {
    unsigned int uProcId = 0;	/* default proc ID is 0. */
    unsigned int index = 0;
    
    struct DSP_PROCESSORINFO dspInfo;
    DSP_HPROCESSOR hProc;
    DSP_STATUS status = DSP_SOK;
    unsigned int numProcs;
    char* argv[2];
   
    argv[0] = "/lib/dsp/baseimage.dof";
    
    status = (DBAPI)DspManager_Open(0, NULL);
    if (DSP_FAILED(status)) {
        printf("DSPManager_Open failed \n");
        return -1;
    } 
    while (DSP_SUCCEEDED(DSPManager_EnumProcessorInfo(index,&dspInfo,
        (unsigned int)sizeof(struct DSP_PROCESSORINFO),&numProcs))) {
        if ((dspInfo.uProcessorType == DSPTYPE_55) || 
            (dspInfo.uProcessorType == DSPTYPE_64)) {
            uProcId = index;
            status = DSP_SOK;
            break;
        }
        index++;
    }
    status = DSPProcessor_Attach(uProcId, NULL, &hProc);
    if (DSP_SUCCEEDED(status)) {
        status = DSPProcessor_Stop(hProc);
        if (DSP_SUCCEEDED(status)) {
            status = DSPProcessor_Load(hProc,1,(const char **)argv,NULL);
            if (DSP_SUCCEEDED(status)) {
                status = DSPProcessor_Start(hProc);
                if (DSP_SUCCEEDED(status)) {
                } 
                else {
                }
            } 
			else {
            }
            DSPProcessor_Detach(hProc);
        }
        else {
        }
    }
    else {
    }
    fprintf(stderr,"Baseimage Loaded\n");

    return 0;		
}
#endif

