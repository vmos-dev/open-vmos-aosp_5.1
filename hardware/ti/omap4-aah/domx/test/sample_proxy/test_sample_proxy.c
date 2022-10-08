/*
 * Copyright (c) 2010, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define COMPONENT_NAME "OMX.TI.DUCATI1.MISC.SAMPLE"	// needs to be specific for every configuration wrapper
#define OMX_SAMPLE_USEBUF OMX_TRUE

/****************************************************************
*  INCLUDE FILES
****************************************************************/
/* ----- system and platform files ----------------------------*/
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


/*-------program files ----------------------------------------*/
#include <OMX_Core.h>
#include <OMX_Component.h>
#include "timm_osal_interfaces.h"


#define OMX_SAMPLE_TILER_TEST

#ifdef OMX_SAMPLE_TILER_TEST

#include "memmgr.h"

#endif


#define OMX_SAMPLE_INPUT_PORT 0
#define OMX_SAMPLE_OUTPUT_PORT 1

#ifdef OMX_SAMPLE_TILER_TEST


#define OMX_SAMPLE_IN_2DYHEIGHT 4
#define OMX_SAMPLE_IN_2DYWIDTH 8
#define OMX_SAMPLE_IN_2DUVHEIGHT 2
#define OMX_SAMPLE_IN_2DUVWIDTH 4
/*
 #define STRIDE_8BIT (16 * 1024)
 #define STRIDE_16BIT (32 * 1024)
*/
#define OMX_SAMPLE_IN_HEIGHT 6
#define OMX_SAMPLE_IN_WIDTH 8

#define STRIDE_LINUX (4 * 1024)

#define OMX_SAMPLE_BUFFER_SIZE 48


void Test_Util_Memcpy_1Dto2D(TIMM_OSAL_PTR pDst2D, TIMM_OSAL_PTR pSrc1D,
    TIMM_OSAL_U32 nSize1D, TIMM_OSAL_U32 nHeight2D, TIMM_OSAL_U32 nWidth2D,
    TIMM_OSAL_U32 nStride2D)
{
	TIMM_OSAL_U8 *pInBuffer;
	TIMM_OSAL_U8 *pOutBuffer;
	TIMM_OSAL_U32 nSizeLeft, i;

	nSizeLeft = nSize1D;
	pInBuffer = (TIMM_OSAL_U8 *) pSrc1D;
	pOutBuffer = (TIMM_OSAL_U8 *) pDst2D;
	//The lower limit is copied. If nSize1D < H*W then 1Dsize is copied else H*W is copied
	for (i = 0; i < nHeight2D; i++)
	{
		if (nSizeLeft >= nWidth2D)
		{
			TIMM_OSAL_Memcpy(pOutBuffer, pInBuffer, nWidth2D);
		} else
		{
			TIMM_OSAL_Memcpy(pOutBuffer, pInBuffer, nSizeLeft);
			break;
		}
		nSizeLeft -= nWidth2D;
		pInBuffer =
		    (TIMM_OSAL_U8 *) ((TIMM_OSAL_U32) pInBuffer + nWidth2D);
		pOutBuffer =
		    (TIMM_OSAL_U8 *) ((TIMM_OSAL_U32) pOutBuffer + nStride2D);
	}
}

#endif


/****************************************************************
*  EXTERNAL REFERENCES NOTE : only use if not found in header file
****************************************************************/
/*--------data declarations -----------------------------------*/
/*--------function prototypes ---------------------------------*/

/****************************************************************
*  PRIVATE DECLARATIONS Defined and used only here
****************************************************************/
/*--------function declarations -------------------------------*/

/*--------data declarations -----------------------------------*/
#define NUM_DOMAINS 0x4
#define OMX_NOPORT 0xfffffffe

//#define INPUT_FILE "../../omx/omx_il_1_x/omx_base/test/patterns/chikita.mp3"
#define INPUT_FILE "sample_input.mp3"
#define NON_TUN_OUTPUT_FILE "nt_output.mp3"

static int gTest = 1;

#define TIMM_OSAL_MallocaBuffer(_size, bContigous, nBlockAlignment) \
        TIMM_OSAL_Malloc (_size, TIMM_OSAL_TRUE, 0, TIMMOSAL_MEM_SEGMENT_EXT)

#define OMX_TEST_BUFFERS_OF_TRAFFIC 20

#define OMX_TEST_BAIL_IF_ERROR(_eError)		\
    if(OMX_ErrorNone != (eError = _eError)){	\
        goto OMX_TEST_BAIL;		\
    }

#define OMX_TEST_SET_ERROR_BAIL(_eCode, _desc)	\
{						\
    eError = _eCode;				\
    printf(_desc);\
    goto OMX_TEST_BAIL;			\
}

#define OMX_TEST_INIT_STRUCT(_s_, _name_)	\
    memset(&(_s_), 0x0, sizeof(_name_));	\
    (_s_).nSize = sizeof(_name_);		\
    (_s_).nVersion.s.nVersionMajor = 0x1;	\
    (_s_).nVersion.s.nVersionMinor = 0x1;	\
    (_s_).nVersion.s.nRevision = 0x0;		\
    (_s_).nVersion.s.nStep = 0x0

#define BUFFER_LIST_CLEAR_ENTRY(_pL, _pB)\
    _pB = _pL->pBufHdr;        \
    _pL->pBufHdr = NULL;    \
    _pL = _pL->pNextBuf;

#define BUFFER_LIST_SET_ENTRY(_pL, _pB)    \
{                \
    BufferList *_pT = _pL;    \
    while(_pT && _pT->pBufHdr){    \
        _pT = _pT->pNextBuf;    \
    }                \
    if(_pT)            \
        _pT->pBufHdr = _pB;    \
}

#define BUFFERLIST_CLEAR_ENTRY(_pL, _pB)\
    _pB = _pL->pBufHdr;        \
    _pL->pBufHdr = NULL;    \
    _pL = _pL->pNextBuf;

typedef struct _BufferList BufferList;

struct _BufferList
{
	OMX_BUFFERHEADERTYPE *pBufHdr;
	OMX_BUFFERHEADERTYPE *pOrigBufHdr;
	BufferList *pNextBuf;
};

typedef struct SampleCompTestCtxt
{
	OMX_HANDLETYPE hComp;
	OMX_STATETYPE eState;
	OMX_U32 nPorts;
	OMX_HANDLETYPE hStateSetEvent;
	OMX_HANDLETYPE hPortDisableEvent;
	OMX_HANDLETYPE hPortEnableEvent;
	OMX_STRING inFilePath;
	OMX_STRING outFilePath;
	FILE *pOutputFile;
	FILE *pInputfile;
	OMX_BOOL bEOS;
	BufferList *pInBufferList;
	BufferList *pOutBufferList;
	OMX_BOOL bClientAllocBuf;
	OMX_U32 nBufDoneCalls;
	OMX_PORT_PARAM_TYPE sPortParam[NUM_DOMAINS];
} SampleCompTestCtxt;

//this test compiles only for ducati SYS
typedef struct TestCtxt
{
	OMX_HANDLETYPE hComp;
	OMX_STATETYPE eState;
	OMX_U32 nPorts;
	OMX_HANDLETYPE hStateSetEvent;
	OMX_HANDLETYPE hPortDisableEvent;
	OMX_HANDLETYPE hPortEnableEvent;
	OMX_STRING inFilePath;
	OMX_STRING outFilePath;
	FILE *pOutputFile;
	FILE *pInputfile;
	OMX_BOOL bEOS;
	BufferList *pInBufferList;
	BufferList *pOutBufferList;
	OMX_BOOL bClientAllocBuf;
	OMX_U32 nBufDoneCalls;
	//OMX_PORT_PARAM_TYPE sPortParam[NUM_DOMAINS];
} TestCtxt;


static OMX_U32 nInBufCount = 0;
static OMX_U32 nOutBufCount = 0;


//Semaphore_Handle EBDSem;
//Semaphore_Params EBDsemParams;

//Semaphore_Handle FBDSem;
//Semaphore_Params FBDsemParams;
/*========================================================*/
/* @ fn OMX_TEST_ErrorToString :: ERROR  to  STRING   */
/*========================================================*/
OMX_STRING OMX_TEST_ErrorToString(OMX_ERRORTYPE eError)
{

	OMX_STRING errorString;

	switch (eError)
	{

	case OMX_ErrorNone:
		errorString = "ErrorNone";
		break;
	case OMX_ErrorInsufficientResources:
		errorString = "ErrorInsufficientResources";
		break;
	case OMX_ErrorUndefined:
		errorString = "ErrorUndefined";
		break;
	case OMX_ErrorInvalidComponentName:
		errorString = "ErrorInvalidComponentName";
		break;
	case OMX_ErrorComponentNotFound:
		errorString = "ErrorComponentNotFound";
		break;
	case OMX_ErrorInvalidComponent:
		errorString = "ErrorInvalidComponent";
		break;
	case OMX_ErrorBadParameter:
		errorString = "ErrorBadParameter";
		break;
	case OMX_ErrorNotImplemented:
		errorString = "ErrorNotImplemented";
		break;
	case OMX_ErrorUnderflow:
		errorString = "ErrorUnderflow";
		break;
	case OMX_ErrorOverflow:
		errorString = "ErrorOverflow";
		break;
	case OMX_ErrorHardware:
		errorString = "ErrorHardware";
		break;
	case OMX_ErrorInvalidState:
		errorString = "ErrorInvalidState";
		break;
	case OMX_ErrorStreamCorrupt:
		errorString = "ErrorStreamCorrupt";
		break;
	case OMX_ErrorPortsNotCompatible:
		errorString = "ErrorPortsNotCompatible";
		break;
	case OMX_ErrorResourcesLost:
		errorString = "ErrorResourcesLost";
		break;
	case OMX_ErrorNoMore:
		errorString = "ErrorNoMore";
		break;
	case OMX_ErrorVersionMismatch:
		errorString = "ErrorVersionMismatch";
		break;
	case OMX_ErrorNotReady:
		errorString = "ErrorNotReady";
		break;
	case OMX_ErrorTimeout:
		errorString = "ErrorTimeout";
		break;
	case OMX_ErrorSameState:
		errorString = "ErrorSameState";
		break;
	case OMX_ErrorResourcesPreempted:
		errorString = "ErrorResourcesPreempted";
		break;
	case OMX_ErrorPortUnresponsiveDuringAllocation:
		errorString = "ErrorPortUnresponsiveDuringAllocation";
		break;
	case OMX_ErrorPortUnresponsiveDuringDeallocation:
		errorString = "ErrorPortUnresponsiveDuringDeallocation";
		break;
	case OMX_ErrorPortUnresponsiveDuringStop:
		errorString = "ErrorPortUnresponsiveDuringStop";
		break;
	case OMX_ErrorIncorrectStateTransition:
		errorString = "ErrorIncorrectStateTransition";
		break;
	case OMX_ErrorIncorrectStateOperation:
		errorString = "ErrorIncorrectStateOperation";
		break;
	case OMX_ErrorUnsupportedSetting:
		errorString = "ErrorUnsupportedSetting";
		break;
	case OMX_ErrorUnsupportedIndex:
		errorString = "ErrorUnsupportedIndex";
		break;
	case OMX_ErrorBadPortIndex:
		errorString = "ErrorBadPortIndex";
		break;
	case OMX_ErrorPortUnpopulated:
		errorString = "ErrorPortUnpopulated";
		break;
	case OMX_ErrorComponentSuspended:
		errorString = "ErrorComponentSuspended";
		break;
	case OMX_ErrorDynamicResourcesUnavailable:
		errorString = "ErrorDynamicResourcesUnavailable";
		break;
	case OMX_ErrorMbErrorsInFrame:
		errorString = "ErrorMbErrorsInFrame";
		break;
	case OMX_ErrorFormatNotDetected:
		errorString = "ErrorFormatNotDetected";
		break;
	case OMX_ErrorContentPipeOpenFailed:
		errorString = "ErrorContentPipeOpenFailed";
		break;
	case OMX_ErrorContentPipeCreationFailed:
		errorString = "ErrorContentPipeCreationFailed";
		break;
	case OMX_ErrorSeperateTablesUsed:
		errorString = "ErrorSeperateTablesUsed";
		break;
	case OMX_ErrorTunnelingUnsupported:
		errorString = "ErrorTunnelingUnsupported";
		break;
	default:
		errorString = "<unknown>";
		break;
	}
	return errorString;
}

/*========================================================*/
/* @ fn OMX_TEST_StateToString :: STATE  to  STRING   */
/*========================================================*/
OMX_STRING OMX_TEST_StateToString(OMX_STATETYPE eState)
{
	OMX_STRING StateString;

	switch (eState)
	{
	case OMX_StateInvalid:
		StateString = "Invalid";
		break;
	case OMX_StateLoaded:
		StateString = "Loaded";
		break;
	case OMX_StateIdle:
		StateString = "Idle";
		break;
	case OMX_StateExecuting:
		StateString = "Executing";
		break;
	case OMX_StatePause:
		StateString = "Pause";
		break;
	case OMX_StateWaitForResources:
		StateString = "WaitForResources ";
		break;
	default:
		StateString = "<unknown>";
		break;
	}

	return StateString;
}

/* Application callback Functions */
/*========================================================*/
/* @ fn SampleTest_EventHandler :: Application callback   */
/*========================================================*/
OMX_ERRORTYPE SampleTest_EventHandler(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_PTR pAppData,
    OMX_IN OMX_EVENTTYPE eEvent,
    OMX_IN OMX_U32 nData1, OMX_IN OMX_U32 nData2, OMX_IN OMX_PTR pEventData)
{
	SampleCompTestCtxt *pContext;

	printf("\n___________ ENTERED CLIENT CALLBACK:%s", __FUNCTION__);

	if (pAppData == NULL)
		return OMX_ErrorNone;

	pContext = (SampleCompTestCtxt *) pAppData;

	switch (eEvent)
	{
	case OMX_EventCmdComplete:
		printf("\n OMX_EventCmdComplete case: \n");
		if (OMX_CommandStateSet == nData1)
		{
			printf(" Component Transitioned to %s state \n",
			    OMX_TEST_StateToString((OMX_STATETYPE) nData2));
			pContext->eState = (OMX_STATETYPE) nData2;
			TIMM_OSAL_SemaphoreRelease(pContext->hStateSetEvent);
		} else if (OMX_CommandFlush == nData1)
		{
			/* Nothing to do over here */
		} else if (OMX_CommandPortDisable == nData1)
		{
			/* Nothing to do over here */
			TIMM_OSAL_SemaphoreRelease(pContext->
			    hPortDisableEvent);
		} else if (OMX_CommandPortEnable == nData1)
		{
			/* Nothing to do over here */
		} else if (OMX_CommandMarkBuffer == nData1)
		{
			/* Nothing to do over here */
		}
		break;

	case OMX_EventError:
		printf("\nOMX EVENT ERROR!!!!!! \n");
		break;

	case OMX_EventMark:
		break;

	case OMX_EventPortSettingsChanged:
		break;

	case OMX_EventBufferFlag:
		break;

	case OMX_EventResourcesAcquired:
		break;

	case OMX_EventComponentResumed:
		break;

	case OMX_EventDynamicResourcesAvailable:
		break;

	case OMX_EventPortFormatDetected:
		break;

	default:
		break;
	}

	goto OMX_TEST_BAIL;
      OMX_TEST_BAIL:
	return OMX_ErrorNone;
}

/*========================================================*/
/* @ fn SampleTest_EmptyBufferDone :: Application callback    */
/*========================================================*/
OMX_ERRORTYPE SampleTest_EmptyBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_PTR pAppData, OMX_IN OMX_BUFFERHEADERTYPE * pBuffer)
{
	SampleCompTestCtxt *pContext;

	printf("\n___________ ENTERED CLIENT CALLBACK:%s", __FUNCTION__);

	if (pAppData == NULL)
		return OMX_ErrorNone;

	pContext = (SampleCompTestCtxt *) pAppData;
	pContext->nBufDoneCalls++;

	BUFFER_LIST_SET_ENTRY(pContext->pInBufferList, pBuffer);
	//Semaphore_post(EBDSem);

	goto OMX_TEST_BAIL;
      OMX_TEST_BAIL:
	printf("\nEBD done\n");
	return OMX_ErrorNone;
}

/*========================================================*/
/* @ fn SampleTest_FillBufferDone ::   Application callback  */
/*========================================================*/
OMX_ERRORTYPE SampleTest_FillBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_PTR pAppData, OMX_IN OMX_BUFFERHEADERTYPE * pBuffHeader)
{
	SampleCompTestCtxt *pContext;

	printf("\n___________ ENTERED CLIENT CALLBACK:%s", __FUNCTION__);

	if (pAppData == NULL)
		return OMX_ErrorNone;

	pContext = (SampleCompTestCtxt *) pAppData;
	pContext->nBufDoneCalls++;

	if (pContext->pOutputFile && gTest)
	{
		printf(" writing to output file :: buffer cnt : %d\n",
		    nOutBufCount);
		nOutBufCount++;
		fwrite(pBuffHeader->pBuffer, 1, pBuffHeader->nFilledLen,
		    pContext->pOutputFile);
	}

	BUFFER_LIST_SET_ENTRY(pContext->pOutBufferList, pBuffHeader);
	//Semaphore_post(FBDSem);

	goto OMX_TEST_BAIL;
      OMX_TEST_BAIL:
	printf("\nFBD done\n");
	return OMX_ErrorNone;
}


static void SampleTest_ReadInputFile(SampleCompTestCtxt * pContext,
    OMX_PTR pData, OMX_U32 nBytes, FILE * fp)
{
	OMX_U32 nReadSize = 0;

	printf("\nIn read i/p file\n");
	if (pData == NULL)
		printf("\npData is NULL\n");
	else
		printf("\npData = %x\n", pData);
	printf("\nnBytes = %d\n", nBytes);
	if (fp == NULL)
		printf("\nfp is NULL\n");
	else
		printf("\nfp = %x\n", fp);
	nReadSize = fread(pData, 1, nBytes, fp);
	if (nReadSize != nBytes)
	{
		pContext->bEOS = OMX_TRUE;
	}

	printf(" Reading from file :: Buffer cont : %d \n", nInBufCount);
	nInBufCount++;
}

/*========================================================*/
/* @ fn SampleTest_WriteInBuffers ::   Reads a buffer from a file and send to Comp       */
/*========================================================*/
OMX_ERRORTYPE SampleTest_WriteInBuffers(SampleCompTestCtxt * pContext)
{

	OMX_ERRORTYPE eError = OMX_ErrorNone;
	BufferList *pList;
	OMX_BUFFERHEADERTYPE *pBufHeader;
#ifdef OMX_SAMPLE_TILER_TEST
	OMX_U8 *pTmpBuffer = NULL, *pOrigTmpBuffer = NULL;
#endif
	pList = pContext->pInBufferList;
	while (pList && pList->pBufHdr)
	{
		BUFFERLIST_CLEAR_ENTRY(pList, pBufHeader);
		printf("\nAbout to read from input file\n");
#if defined(OMX_SAMPLE_TILER_TEST)

		//OMX_SAMPLE_BUFFER_SIZE is the total amt of data to be sent in the buffer
		pTmpBuffer =
		    TIMM_OSAL_Malloc(OMX_SAMPLE_BUFFER_SIZE, 0, 0, 0);
		if (pTmpBuffer == NULL)
			OMX_TEST_SET_ERROR_BAIL
			    (OMX_ErrorInsufficientResources,
			    "malloc failed \n");
		pOrigTmpBuffer = pTmpBuffer;

		SampleTest_ReadInputFile(pContext, pTmpBuffer,
		    OMX_SAMPLE_BUFFER_SIZE, pContext->pInputfile);

		Test_Util_Memcpy_1Dto2D(pBufHeader->pBuffer, pTmpBuffer,
		    OMX_SAMPLE_BUFFER_SIZE, OMX_SAMPLE_IN_HEIGHT,
		    OMX_SAMPLE_IN_WIDTH, STRIDE_LINUX);
		pBufHeader->nFilledLen = OMX_SAMPLE_BUFFER_SIZE;

		printf("\nBefore ETB pBufHeader->nInputPortIndex = %d\n",
		    pBufHeader->nInputPortIndex);

		TIMM_OSAL_Free(pOrigTmpBuffer);

#else
		SampleTest_ReadInputFile(pContext, pBufHeader->pBuffer,
		    pBufHeader->nAllocLen, pContext->pInputfile);
		pBufHeader->nFilledLen = pBufHeader->nAllocLen;

#endif
		if (pContext->bEOS == OMX_TRUE)
		{
			pBufHeader->nFlags |= OMX_BUFFERFLAG_EOS;
		}

		eError = OMX_EmptyThisBuffer(pContext->hComp, pBufHeader);

		OMX_TEST_BAIL_IF_ERROR(eError);
	}

      OMX_TEST_BAIL:
	return eError;

}

/*========================================================*/
/* @ fn SampleTest_ReadOutBuffers ::   Send out buffers    */
/*========================================================*/

OMX_ERRORTYPE SampleTest_ReadOutBuffers(SampleCompTestCtxt * pContext)
{

	OMX_ERRORTYPE eError = OMX_ErrorNone;
	BufferList *pList;
	OMX_BUFFERHEADERTYPE *pBufHeader;

	pList = pContext->pOutBufferList;
	while (pList && pList->pBufHdr)
	{
		printf("\nAbout to do FTB\n");

		BUFFERLIST_CLEAR_ENTRY(pList, pBufHeader);

		eError = OMX_FillThisBuffer(pContext->hComp, pBufHeader);

		OMX_TEST_BAIL_IF_ERROR(eError);
		if (pBufHeader->nFlags == OMX_BUFFERFLAG_EOS)
		{
			pContext->nBufDoneCalls = OMX_TEST_BUFFERS_OF_TRAFFIC;
		}
	}

      OMX_TEST_BAIL:
	return eError;

}



/*========================================================*/
/* @ fn SampleTest_AllocateBuffers ::   Allocates the Resources on the available ports  */
/*========================================================*/
OMX_ERRORTYPE SampleTest_AllocateBuffers(SampleCompTestCtxt * pContext,
    OMX_PARAM_PORTDEFINITIONTYPE * pPortDef)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_U8 *pBuffer = NULL;
	BufferList *pBufferList;
	BufferList *pTemp;
	OMX_BUFFERHEADERTYPE *pBufferHdr;
	OMX_U32 i = 100;
	OMX_COMPONENTTYPE *pComp;

#ifdef OMX_SAMPLE_TILER_TEST
	MemAllocBlock *pBlock = NULL;
	OMX_U32 nNumBlocks = 1;

/*For i/p port allocate 2D packed buffer, for o/p port allocate 1D buffer.
Ideally client should get this from GetParams but this is just a sample test so
values are hardcoded*/

	if (pPortDef->nPortIndex == OMX_SAMPLE_INPUT_PORT)
	{
		nNumBlocks = 2;
		pBlock =
		    TIMM_OSAL_Malloc(sizeof(MemAllocBlock) * nNumBlocks, 0, 0,
		    0);
		TIMM_OSAL_Memset(pBlock, 0, sizeof(MemAllocBlock) * nNumBlocks);
		pBlock[0].dim.area.width = OMX_SAMPLE_IN_2DYWIDTH;
		pBlock[0].dim.area.height = OMX_SAMPLE_IN_2DYHEIGHT;
		pBlock[0].pixelFormat = PIXEL_FMT_8BIT;
		pBlock[1].dim.area.width = OMX_SAMPLE_IN_2DUVWIDTH;
		pBlock[1].dim.area.height = OMX_SAMPLE_IN_2DUVHEIGHT;
		pBlock[1].pixelFormat = PIXEL_FMT_16BIT;
	} else
	{
		nNumBlocks = 1;
		pBlock =
		    TIMM_OSAL_Malloc(sizeof(MemAllocBlock) * nNumBlocks, 0, 0,
		    0);
		TIMM_OSAL_Memset(pBlock, 0, sizeof(MemAllocBlock) * nNumBlocks);
		pBlock[0].dim.len = OMX_SAMPLE_BUFFER_SIZE;
		pBlock[0].pixelFormat = PIXEL_FMT_PAGE;
	}
#endif

	for (i = 0; i < pPortDef->nBufferCountActual; i++)
	{
		pBufferList =
		    (BufferList *) TIMM_OSAL_Malloc(sizeof(BufferList),
		    TIMM_OSAL_TRUE, 0, TIMMOSAL_MEM_SEGMENT_INT);
		if (!pBufferList)
		{
			OMX_TEST_SET_ERROR_BAIL
			    (OMX_ErrorInsufficientResources,
			    "malloc failed \n");
		}

		if (pContext->bClientAllocBuf)
		{

#ifdef OMX_SAMPLE_TILER_TEST
/*For i/p port allocate 2D packed buffer, for o/p port allocate 1D buffer.
Ideally client should get this from GetParams but this is just a sample test so
values are hardcoded*/
			pBuffer = MemMgr_Alloc(pBlock, nNumBlocks);
			printf("\nMemMgr allocated buffer = 0x%x\n", pBuffer);
#else
			pBuffer =
			    (OMX_U8 *) TIMM_OSAL_MallocaBuffer(pPortDef->
			    nBufferSize, pPortDef->bBuffersContiguous,
			    pPortDef->nBufferAlignment);

#endif

			if (!pBufferList)
			{
				OMX_TEST_SET_ERROR_BAIL
				    (OMX_ErrorInsufficientResources,
				    "malloc failed \n");
			}

			printf("\nCalling UseBuf on port %d\n",
			    pPortDef->nPortIndex);
			eError =
			    OMX_UseBuffer(pContext->hComp, &pBufferHdr,
			    pPortDef->nPortIndex, 0, pPortDef->nBufferSize,
			    pBuffer);

			OMX_TEST_BAIL_IF_ERROR(eError);

		} else
		{

			pComp = (OMX_COMPONENTTYPE *) pContext->hComp;
			printf("\nCalling allocate buffer\n");
			eError =
			    OMX_AllocateBuffer(pContext->hComp, &pBufferHdr,
			    pPortDef->nPortIndex, 0, pPortDef->nBufferSize);

			OMX_TEST_BAIL_IF_ERROR(eError);
		}
		printf("\npBufferHdr->nOutputPortIndex = %d\n",
		    pBufferHdr->nOutputPortIndex);
		printf("\npBufferHdr->nInputPortIndex = %d\n",
		    pBufferHdr->nInputPortIndex);
		pBufferList->pNextBuf = NULL;
		pBufferList->pBufHdr = pBufferHdr;
		pBufferList->pOrigBufHdr = pBufferHdr;

		if (pPortDef->eDir == OMX_DirInput)
		{
			printf("\npBufferHdr->nOutputPortIndex = %d\n",
			    pBufferHdr->nOutputPortIndex);
			printf("\npBufferHdr->nInputPortIndex = %d\n",
			    pBufferHdr->nInputPortIndex);
			pBufferHdr->nOutputPortIndex = OMX_NOPORT;
			if (pContext->pInBufferList == NULL)
			{
				pContext->pInBufferList = pBufferList;
			} else
			{
				pTemp = pContext->pInBufferList;
				while (pTemp->pNextBuf)
					pTemp = pTemp->pNextBuf;
				pTemp->pNextBuf = pBufferList;
			}
		} else
		{
			pBufferHdr->nInputPortIndex = OMX_NOPORT;
			printf("\npBufferHdr->nOutputPortIndex = %d\n",
			    pBufferHdr->nOutputPortIndex);
			printf("\npBufferHdr->nInputPortIndex = %d\n",
			    pBufferHdr->nInputPortIndex);
			if (pContext->pOutBufferList == NULL)
			{
				pContext->pOutBufferList = pBufferList;
			} else
			{
				pTemp = pContext->pOutBufferList;
				while (pTemp->pNextBuf)
					pTemp = pTemp->pNextBuf;
				pTemp->pNextBuf = pBufferList;
			}
		}
	}

      OMX_TEST_BAIL:
#ifdef OMX_SAMPLE_TILER_TEST
	if (pBlock != NULL)
		TIMM_OSAL_Free(pBlock);
#endif
	if (eError != OMX_ErrorNone)
	{
		if (pBufferList)
		{
			TIMM_OSAL_Free(pBufferList);
		}
	}

	return eError;
}

/*========================================================*/
/* @ fn SampleTest_DeInitBuffers ::   Destroy the resources  */
/*========================================================*/
OMX_ERRORTYPE SampleTest_DeInitBuffers(SampleCompTestCtxt * pContext)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_U8 *pBuffer;
	BufferList *pBufferList;
	BufferList *pTemp;

	OMX_U32 nRetVal = 0;

	pTemp = pContext->pInBufferList;

	while (pTemp)
	{
		pBufferList = (BufferList *) pTemp;
		pBuffer = (OMX_U8 *) pTemp->pOrigBufHdr->pBuffer;

		printf("\nCalling Free Buffer on port no. %d\n",
		    pTemp->pOrigBufHdr->nInputPortIndex);

		eError =
		    OMX_FreeBuffer(pContext->hComp,
		    pTemp->pOrigBufHdr->nInputPortIndex, pTemp->pOrigBufHdr);
		OMX_TEST_BAIL_IF_ERROR(eError);

                if (pContext->bClientAllocBuf)
                {

#ifdef OMX_SAMPLE_TILER_TEST
                        nRetVal = MemMgr_Free(pBuffer);
                        if (nRetVal)
                        {
                                printf("\nError in MemMgr free\n");
                        }
#else
                        TIMM_OSAL_Free(pBuffer);
#endif
                }

		pTemp = pTemp->pNextBuf;
		if (pBufferList)
			TIMM_OSAL_Free(pBufferList);
	}

	pContext->pInBufferList = NULL;

	pTemp = pContext->pOutBufferList;
	while (pTemp)
	{
		pBufferList = (BufferList *) pTemp;
		pBuffer = (OMX_U8 *) pTemp->pOrigBufHdr->pBuffer;
		printf("\nCalling Free Buffer on port no. %d\n",
		    pTemp->pOrigBufHdr->nOutputPortIndex);

		eError =
		    OMX_FreeBuffer(pContext->hComp,
		    pTemp->pOrigBufHdr->nOutputPortIndex, pTemp->pOrigBufHdr);
		OMX_TEST_BAIL_IF_ERROR(eError);
                if (pContext->bClientAllocBuf)
                {
#ifdef OMX_SAMPLE_TILER_TEST
                        nRetVal = MemMgr_Free(pBuffer);
                        if (nRetVal)
                        {
                                printf("\nError in MemMgr free\n");
                        }
#else
                        TIMM_OSAL_Free(pBuffer);
#endif
                }

		pTemp = pTemp->pNextBuf;
		if (pBufferList)
			TIMM_OSAL_Free(pBufferList);

	}

	pContext->pOutBufferList = NULL;

      OMX_TEST_BAIL:
	return eError;
}

/*========================================================*/
/* @ fn SampleTest_TransitionWait ::   Waits for the transition to be completed ,
 *  incase of loaded to idle Allocates the Resources and while idle to loaded
 *  destroys the resources */
/*========================================================*/
OMX_ERRORTYPE SampleTest_TransitionWait(OMX_STATETYPE eToState,
    SampleCompTestCtxt * pContext)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_PARAM_PORTDEFINITIONTYPE tPortDef;
	OMX_U32 i, j;

	eError = OMX_SendCommand(pContext->hComp, OMX_CommandStateSet,
	    eToState, NULL);
	OMX_TEST_BAIL_IF_ERROR(eError);

	if ((eToState == OMX_StateIdle) &&
	    (pContext->eState == OMX_StateLoaded))
	{
		for (i = 0; i < NUM_DOMAINS; i++)
		{
			for (j = pContext->sPortParam[i].nStartPortNumber;
			    j < pContext->sPortParam[i].nStartPortNumber
			    + pContext->sPortParam[i].nPorts; j++)
			{

				OMX_TEST_INIT_STRUCT(tPortDef,
				    OMX_PARAM_PORTDEFINITIONTYPE);
				tPortDef.nPortIndex = j;
//printf("\nCalling GetParam before UseBuf on port %d\n",j);
				eError = OMX_GetParameter(pContext->hComp,
				    OMX_IndexParamPortDefinition,
				    (OMX_PTR) & tPortDef);
				OMX_TEST_BAIL_IF_ERROR(eError);

//                    if(tPortDef.bEnabled)//AD
				eError =
				    SampleTest_AllocateBuffers(pContext,
				    &tPortDef);

				OMX_TEST_BAIL_IF_ERROR(eError);
			}
		}
	} else if ((eToState == OMX_StateLoaded) &&
	    (pContext->eState == OMX_StateIdle))
	{

		eError = SampleTest_DeInitBuffers(pContext);
		OMX_TEST_BAIL_IF_ERROR(eError);

	}
	printf("\nWaiting for state set event\n");
	TIMM_OSAL_SemaphoreObtain(pContext->hStateSetEvent,
	    TIMM_OSAL_SUSPEND);
	printf("\nState set event recd.\n");

	if (pContext->eState != eToState)
		OMX_TEST_SET_ERROR_BAIL(OMX_ErrorUndefined,
		    " InComplete Transition \n");

      OMX_TEST_BAIL:
	return eError;
}


/*========================================================*/
/* @ fn OMX_Sample_UT0001 ::  Initializes, move to Idle and then to executing, process
*    buffers and then destroy the component by moving back to idle, loaded, invalid */
/*========================================================*/
void main(void)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_HANDLETYPE hComp = NULL;
	OMX_CALLBACKTYPE oCallbacks;
	SampleCompTestCtxt *pContext;
	SampleCompTestCtxt oAppData;

	int ch1, ch2;
	int pass;
	int while_pass = 0, loc_diff = 0;


	pContext = &oAppData;
	printf(" Entering : %s \n", __FUNCTION__);
	memset(pContext, 0x0, sizeof(SampleCompTestCtxt));

	oCallbacks.EventHandler = SampleTest_EventHandler;
	oCallbacks.EmptyBufferDone = SampleTest_EmptyBufferDone;
	oCallbacks.FillBufferDone = SampleTest_FillBufferDone;
	printf("\nCalling sem create\n");
	/* Initialize Events to Track callbacks  */
	TIMM_OSAL_SemaphoreCreate(&pContext->hStateSetEvent, 0);
	TIMM_OSAL_SemaphoreCreate(&pContext->hPortDisableEvent, 0);
	//TIMM_OSAL_MutexObtain(pContext->hStateSetEvent, TIMM_OSAL_SUSPEND);
	printf("\nSem created\n");


	pContext->pInputfile = fopen(INPUT_FILE, "rb");
	if (NULL == pContext->pInputfile)
	{
		eError = OMX_ErrorInsufficientResources;
		//goto OMX_TEST_BAIL;
	}
	printf("\nInput file opened\n");
	pContext->pOutputFile = fopen(NON_TUN_OUTPUT_FILE, "wb");
	if (NULL == pContext->pOutputFile)
	{
		eError = OMX_ErrorInsufficientResources;
		//goto OMX_TEST_BAIL;
	}


	/* Initialize OpenMAX */
	printf("\nInitialize OpenMAX\n");
	eError = OMX_Init();
	OMX_TEST_BAIL_IF_ERROR(eError);
	/* Load a component  */
	printf("\nLoad a component\n");
	eError =
	    OMX_GetHandle(&hComp, (OMX_STRING) "OMX.TI.DUCATI1.MISC.SAMPLE",
	    pContext, &oCallbacks);
	OMX_TEST_BAIL_IF_ERROR(eError);
	pContext->hComp = hComp;
printf ("\neError = 0x%x\n", eError);
if(hComp)
	printf("\nhComp = 0x%x\n", hComp);
	/* Verify that the component is in Loaded state */
	printf("\nVerify that the component is in Loaded state\n");

	eError = OMX_GetState(pContext->hComp, &pContext->eState);
	OMX_TEST_BAIL_IF_ERROR(eError);
	if (OMX_StateLoaded != pContext->eState)
	{
		OMX_TEST_SET_ERROR_BAIL(OMX_ErrorUndefined,
		    "not in loaded state \n");
	}

	/* detect all Audio ports on the component */
	OMX_TEST_INIT_STRUCT(pContext->sPortParam[0], OMX_PORT_PARAM_TYPE);
	eError = OMX_GetParameter(hComp, OMX_IndexParamAudioInit,
	    (OMX_PTR) & pContext->sPortParam[0]);
	OMX_TEST_BAIL_IF_ERROR(eError);

	/* detect all video ports on the component */
	OMX_TEST_INIT_STRUCT(pContext->sPortParam[1], OMX_PORT_PARAM_TYPE);
	eError = OMX_GetParameter(hComp, OMX_IndexParamVideoInit,
	    (OMX_PTR) & pContext->sPortParam[1]);
	OMX_TEST_BAIL_IF_ERROR(eError);

	/* detect all image ports on the component */
	OMX_TEST_INIT_STRUCT(pContext->sPortParam[2], OMX_PORT_PARAM_TYPE);
	eError = OMX_GetParameter(hComp, OMX_IndexParamImageInit,
	    (OMX_PTR) & pContext->sPortParam[2]);
	OMX_TEST_BAIL_IF_ERROR(eError);

	/* detect all other ports on the component */
	OMX_TEST_INIT_STRUCT(pContext->sPortParam[3], OMX_PORT_PARAM_TYPE);
	eError = OMX_GetParameter(hComp, OMX_IndexParamOtherInit,
	    (OMX_PTR) & pContext->sPortParam[3]);
	OMX_TEST_BAIL_IF_ERROR(eError);

	pContext->nPorts = pContext->sPortParam[0].nPorts +
	    pContext->sPortParam[1].nPorts +
	    pContext->sPortParam[2].nPorts + pContext->sPortParam[3].nPorts;


	pContext->bClientAllocBuf = OMX_SAMPLE_USEBUF;
	if (pContext->bClientAllocBuf == OMX_TRUE)
	{
		printf(" Client does Allocation of buffers \n");
	} else
	{
		printf(" Component does Allocation of buffers \n");
	}

	/* Transition to Idle state */

	eError = SampleTest_TransitionWait(OMX_StateIdle, pContext);
	OMX_TEST_BAIL_IF_ERROR(eError);
	/* Transition to Executing state */
	eError = SampleTest_TransitionWait(OMX_StateExecuting, pContext);
	OMX_TEST_BAIL_IF_ERROR(eError);
	printf("\nAbout to start processing buffers\n");


	/*  process buffers   */

	while (pContext->nBufDoneCalls < OMX_TEST_BUFFERS_OF_TRAFFIC)
	{

		eError = SampleTest_WriteInBuffers(pContext);
		OMX_TEST_BAIL_IF_ERROR(eError);

		eError = SampleTest_ReadOutBuffers(pContext);
		OMX_TEST_BAIL_IF_ERROR(eError);
		while_pass++;
		//printf("\n WHILE PROCESS LOOP COUNT:  BufDoneCount = %d %d",
		//    while_pass, pContext->nBufDoneCalls);

		//printf("\n Waiting on Sample component to respond");

	}


	gTest = 0;

	/* Transition back to Idle state  */
	eError = SampleTest_TransitionWait(OMX_StateIdle, pContext);
	OMX_TEST_BAIL_IF_ERROR(eError);

	/* Trasnition back to Loaded state */
	eError = SampleTest_TransitionWait(OMX_StateLoaded, pContext);
	OMX_TEST_BAIL_IF_ERROR(eError);


      OMX_TEST_BAIL:

	fclose(pContext->pInputfile);
	fclose(pContext->pOutputFile);

	if (eError == OMX_ErrorNone)
	{
		eError = OMX_FreeHandle(pContext->hComp);
		eError = OMX_Deinit();
	} else
	{
		SampleTest_TransitionWait(OMX_StateInvalid, pContext);
		SampleTest_DeInitBuffers(pContext);

		OMX_FreeHandle(pContext->hComp);
		OMX_Deinit();
	}

	TIMM_OSAL_SemaphoreDelete(pContext->hStateSetEvent);
	TIMM_OSAL_SemaphoreDelete(pContext->hPortDisableEvent);

	/* emit the Test Result */
	if (OMX_ErrorNone != eError)
	{
		printf(" TestCase Failed and returned an error:: %s \n",
		    OMX_TEST_ErrorToString(eError));
	} else
	{
		printf
		    ("\nTest case has ended, now comparing input and output files\n");
		pContext->pInputfile = fopen(INPUT_FILE, "rb");
		if (NULL == pContext->pInputfile)
		{
			printf("\n Error in opening INPUT_FILE");
			eError = OMX_ErrorInsufficientResources;
		}

		pContext->pOutputFile = fopen(NON_TUN_OUTPUT_FILE, "rb");
		if (NULL == pContext->pOutputFile)
		{
			printf("\n Error in opening NON_TUN_OUTPUT_FILE");
			eError = OMX_ErrorInsufficientResources;
		}
		pass = 1;
		while (1)
		{
			if (eError != OMX_ErrorNone)
			{
				pass = 0;
				break;
			}
			ch1 = fgetc(pContext->pInputfile);
			ch2 = fgetc(pContext->pOutputFile);
			loc_diff++;
			if (ch1 == EOF || ch2 == EOF)
			{
				break;
			}
			if (ch1 != ch2)
			{
				pass = 0;
				printf("\n \n");
				printf
				    ("\n FILE SIZE = 320 KB - NUMBER OF ITERATIONS = 20 (10 for INPUT AND 10 FOR OUTPUT buffers\n");
				printf
				    ("\n ################### LOCATION OF DIFFERENCE: %d ################### \n",
				    loc_diff);
				break;
			}
		}
		fclose(pContext->pInputfile);
		fclose(pContext->pOutputFile);
		if (pass == 1)
			printf(" Test Case has Passed\n");
		else
		{
			printf("\nTest case has failed. (EOF not reached)\n");
			eError = OMX_ErrorUndefined;
		}
	}

	if (eError == OMX_ErrorNone)
	{
		printf(" Test Case has Passed :) \n");
	} else
	{
		printf("\nTest case has failed.(OMX Error)\n");
	}
}

