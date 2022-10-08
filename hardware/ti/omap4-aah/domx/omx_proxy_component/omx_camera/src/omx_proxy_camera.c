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

/**
 *  @file  omx_proxy_camera.c
 *         This file contains methods that provides the functionality for
 *         the OpenMAX1.1 DOMX Framework Tunnel Proxy component.
 ******************************************************************************
 This is the proxy specific wrapper that passes the component name to the
 generic proxy init() The proxy wrapper also does some runtime/static time
 config on per proxy basis This is a thin wrapper that is called when
 componentiit() of the proxy is called  static OMX_ERRORTYPE PROXY_Wrapper_init
 (OMX_HANDLETYPE hComponent, OMX_PTR pAppData);
 this layer gets called first whenever a proxy s get handle is called
 ******************************************************************************
 *  @path WTSD_DucatiMMSW\omx\omx_il_1_x\omx_proxy_component\src
 *
 *  @rev 1.0
 */

/*==============================================================
 *! Revision History
 *! ============================
 *! 19-August-2009 B Ravi Kiran ravi.kiran@ti.com: Initial Version
 *================================================================*/

/******************************************************************
 *   INCLUDE FILES
 ******************************************************************/
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <stdlib.h>
#include <errno.h>

#include <timm_osal_interfaces.h>
#include <OMX_TI_IVCommon.h>
#include <OMX_TI_Index.h>
#include "omx_proxy_common.h"
#include "timm_osal_mutex.h"

#ifdef USE_ION
#include <unistd.h>
#include <ion/ion.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/eventfd.h>
#include <fcntl.h>
#include <errno.h>
#endif

#define COMPONENT_NAME "OMX.TI.DUCATI1.VIDEO.CAMERA"
/*Needs to be specific for every configuration wrapper*/

#undef LOG_TAG
#define LOG_TAG "CameraHAL"

#define DEFAULT_DCC 1

#define LINUX_PAGE_SIZE (4 * 1024)

#define _PROXY_OMX_INIT_PARAM(param,type) do {		\
	TIMM_OSAL_Memset((param), 0, sizeof (type));	\
	(param)->nSize = sizeof (type);			\
	(param)->nVersion.s.nVersionMajor = 1;		\
	(param)->nVersion.s.nVersionMinor = 1;		\
	} while(0)

/* VTC specific changes */
#define MAX_NUM_INTERNAL_BUFFERS 4
#define MAX_VTC_WIDTH 1920
#define MAX_VTC_HEIGHT 1080
#define BORDER_WIDTH 32
#define BORDER_HEIGHT 32
#define MAX_VTC_WIDTH_WITH_VNF (MAX_VTC_WIDTH + BORDER_WIDTH)
#define MAX_VTC_HEIGHT_WITH_VNF (MAX_VTC_HEIGHT + BORDER_HEIGHT)
OMX_PTR gCamIonHdl[MAX_NUM_INTERNAL_BUFFERS][2];

/* Tiler heap resservation specific */
#define OMAP_ION_HEAP_TILER_ALLOCATION_MASK (1<<4)
/* store handles for tracking and freeing */
OMX_PTR gComponentBufferAllocation[PROXY_MAXNUMOFPORTS][MAX_NUM_INTERNAL_BUFFERS];

/* Incase of multiple instance, making sure DCC is initialized only for
   first instance */
static OMX_S16 numofInstance = 0;
int dcc_flag = 0;
TIMM_OSAL_PTR cam_mutex = NULL;

/* To store DCC buffer size */
OMX_S32 dccbuf_size = 0;

/* Ducati Mapped Addr  */
OMX_PTR DCC_Buff = NULL;

#ifdef USE_ION
OMX_PTR DCC_Buff_ptr = NULL;
int ion_fd;
int mmap_fd;
#endif

OMX_S32 read_DCCdir(OMX_PTR, OMX_STRING *, OMX_U16);
OMX_ERRORTYPE DCC_Init(OMX_HANDLETYPE);
OMX_ERRORTYPE send_DCCBufPtr(OMX_HANDLETYPE hComponent);
void DCC_DeInit();
OMX_ERRORTYPE PROXY_ComponentDeInit(OMX_HANDLETYPE);
OMX_ERRORTYPE __PROXY_SetConfig(OMX_HANDLETYPE, OMX_INDEXTYPE,
								OMX_PTR, OMX_PTR);
OMX_ERRORTYPE __PROXY_GetConfig(OMX_HANDLETYPE, OMX_INDEXTYPE,
								OMX_PTR, OMX_PTR);
OMX_ERRORTYPE __PROXY_SetParameter(OMX_IN OMX_HANDLETYPE, OMX_INDEXTYPE,
									OMX_PTR, OMX_PTR, OMX_U32);
OMX_ERRORTYPE __PROXY_GetParameter(OMX_IN OMX_HANDLETYPE, OMX_INDEXTYPE,
									OMX_PTR, OMX_PTR);
OMX_ERRORTYPE PROXY_SendCommand(OMX_HANDLETYPE, OMX_COMMANDTYPE,
 								        OMX_U32,OMX_PTR);
OMX_ERRORTYPE CameraMaptoTilerDuc(OMX_TI_CONFIG_SHAREDBUFFER *, OMX_PTR *);
//COREID TARGET_CORE_ID = CORE_APPM3;

static OMX_ERRORTYPE ComponentPrivateDeInit(OMX_IN OMX_HANDLETYPE hComponent)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	TIMM_OSAL_ERRORTYPE eOsalError = TIMM_OSAL_ERR_NONE;
	PROXY_COMPONENT_PRIVATE *pCompPrv;
	OMX_COMPONENTTYPE *hComp = (OMX_COMPONENTTYPE *) hComponent;
	OMX_U32 i, j;

	pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;

	if (dcc_flag)
	{
		eOsalError =
		    TIMM_OSAL_MutexObtain(cam_mutex, TIMM_OSAL_SUSPEND);
		if (eOsalError != TIMM_OSAL_ERR_NONE)
		{
			TIMM_OSAL_Error("Mutex Obtain failed");
		}

		numofInstance = numofInstance - 1;

		eOsalError = TIMM_OSAL_MutexRelease(cam_mutex);
		PROXY_assert(eOsalError == TIMM_OSAL_ERR_NONE,
		    OMX_ErrorInsufficientResources, "Mutex release failed");
	}
        for(i=0; i < MAX_NUM_INTERNAL_BUFFERS; i++) {
            if (gCamIonHdl[i][0] != NULL) {
                ion_free(pCompPrv->ion_fd, gCamIonHdl[i][0]);
                gCamIonHdl[i][0] = NULL;
            }
            if (gCamIonHdl[i][1] != NULL) {
                ion_free(pCompPrv->ion_fd, gCamIonHdl[i][1]);
                gCamIonHdl[i][1] = NULL;
            }

        }

        for (i = 0; i < PROXY_MAXNUMOFPORTS; i++) {
            for (j = 0; j < MAX_NUM_INTERNAL_BUFFERS; j++) {
                if (gComponentBufferAllocation[i][j]) {
                    ion_free(pCompPrv->ion_fd, gComponentBufferAllocation[i][j]);
                }
                gComponentBufferAllocation[i][j] = NULL;
            }
        }

	eError = PROXY_ComponentDeInit(hComponent);

      EXIT:
	return eError;
}

static OMX_ERRORTYPE Camera_SendCommand(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_COMMANDTYPE eCmd,
    OMX_IN OMX_U32 nParam, OMX_IN OMX_PTR pCmdData)

{
	OMX_ERRORTYPE eError = OMX_ErrorNone, eCompReturn;
	RPC_OMX_ERRORTYPE eRPCError = RPC_OMX_ErrorNone;
	PROXY_COMPONENT_PRIVATE *pCompPrv;
	OMX_COMPONENTTYPE *hComp = (OMX_COMPONENTTYPE *) hComponent;
	static OMX_BOOL dcc_loaded = OMX_FALSE;

	OMX_ERRORTYPE dcc_eError = OMX_ErrorNone;
	TIMM_OSAL_ERRORTYPE eOsalError = TIMM_OSAL_ERR_NONE;
	OMX_U32 i;

	pCompPrv = (PROXY_COMPONENT_PRIVATE *) hComp->pComponentPrivate;

	if ((eCmd == OMX_CommandStateSet) &&
	(nParam == (OMX_STATETYPE) OMX_StateIdle))
	{
		if (!dcc_loaded)
		{
			dcc_eError = DCC_Init(hComponent);
			if (dcc_eError != OMX_ErrorNone)
			{
				DOMX_ERROR(" Error in DCC Init");
			}
			/* Configure Ducati to use DCC buffer from A9 side
			 *ONLY* if DCC_Init is successful. */
			if (dcc_eError == OMX_ErrorNone)
			{
				dcc_eError = send_DCCBufPtr(hComponent);
				if (dcc_eError != OMX_ErrorNone)
				{
					DOMX_ERROR(" Error in Sending DCC Buf ptr");
				}
				DCC_DeInit();
			}
			dcc_loaded = OMX_TRUE;
		}
	} else if (eCmd == OMX_CommandPortDisable) {
            int i, j;
            for (i = 0; i < MAX_NUM_INTERNAL_BUFFERS; i++) {
                for (j = 0; j < PROXY_MAXNUMOFPORTS; j++) {
                    if (((j == nParam) || (nParam == OMX_ALL)) &&
                         gComponentBufferAllocation[i][j])
                    {
                        ion_free(pCompPrv->ion_fd, gComponentBufferAllocation[i][j]);
                        gComponentBufferAllocation[i][j] = NULL;
                    }
                }
            }

        }


	eError =
	PROXY_SendCommand(hComponent,eCmd,nParam,pCmdData);


EXIT:

   DOMX_EXIT("eError: %d", eError);
   return eError;

}

/* ===========================================================================*/
/**
 * @name CameraGetConfig()
 * @brief For some specific indices, buffer allocated on A9 side
 *        needs to be mapped and sent to Ducati.
 * @param
 * @return OMX_ErrorNone = Successful
 */
/* ===========================================================================*/

static OMX_ERRORTYPE CameraGetConfig(OMX_IN OMX_HANDLETYPE
    hComponent, OMX_IN OMX_INDEXTYPE nParamIndex,
    OMX_INOUT OMX_PTR pComponentParameterStructure)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_TI_CONFIG_SHAREDBUFFER *pConfigSharedBuffer = NULL;
	OMX_PTR pTempSharedBuff = NULL;
	OMX_U32 status = 0;

	switch (nParamIndex)
	{
	case OMX_TI_IndexConfigAAAskipBuffer:
	case OMX_TI_IndexConfigCamCapabilities:
	case OMX_TI_IndexConfigExifTags:
	case OMX_TI_IndexConfigAlgoAreas:
		pConfigSharedBuffer =
			(OMX_TI_CONFIG_SHAREDBUFFER *) pComponentParameterStructure;

		pTempSharedBuff = pConfigSharedBuffer->pSharedBuff;

		// TODO(XXX): Cache API is not yet available. Client needs to
		// allocate tiler buffer directly and assign to pSharedBuff.
		// Ptr allocated by MemMgr_Alloc in uncacheable so there
		// would be no need to cache API

		eError = __PROXY_GetConfig(hComponent,
								nParamIndex,
								pConfigSharedBuffer,
								&(pConfigSharedBuffer->pSharedBuff));

		PROXY_assert((eError == OMX_ErrorNone), eError,
		    "Error in GetConfig");

		pConfigSharedBuffer->pSharedBuff = pTempSharedBuff;

		goto EXIT;
		break;
	default:
		break;
	}

	return __PROXY_GetConfig(hComponent,
							nParamIndex,
							pComponentParameterStructure,
							NULL);

 EXIT:
	return eError;
}

/* ===========================================================================*/
/**
 * @name CameraSetConfig()
 * @brief For some specific indices, buffer allocated on A9 side needs to
 *        be mapped and sent to Ducati.
 * @param
 * @return OMX_ErrorNone = Successful
 */
/* ===========================================================================*/


static OMX_ERRORTYPE CameraSetConfig(OMX_IN OMX_HANDLETYPE
    hComponent, OMX_IN OMX_INDEXTYPE nParamIndex,
    OMX_INOUT OMX_PTR pComponentParameterStructure)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_TI_CONFIG_SHAREDBUFFER *pConfigSharedBuffer = NULL;
	OMX_PTR pTempSharedBuff = NULL;
	OMX_U32 status = 0;

	switch (nParamIndex)
	{
	case OMX_TI_IndexConfigAAAskipBuffer:
	case OMX_TI_IndexConfigCamCapabilities:
	case OMX_TI_IndexConfigExifTags:
	case OMX_TI_IndexConfigAlgoAreas:
		pConfigSharedBuffer =
			(OMX_TI_CONFIG_SHAREDBUFFER *)
			pComponentParameterStructure;

		pTempSharedBuff = pConfigSharedBuffer->pSharedBuff;

		// TODO(XXX): Cache API is not yet available. Client needs to
		// allocate tiler buffer directly and assign to pSharedBuff.
		// Ptr allocated by MemMgr_Alloc in uncacheable so there
		// would be no need to cache API

		eError = __PROXY_SetConfig(hComponent,
								nParamIndex,
								pConfigSharedBuffer,
								&(pConfigSharedBuffer->pSharedBuff));

		PROXY_assert((eError == OMX_ErrorNone), eError,
		    "Error in GetConfig");

		pConfigSharedBuffer->pSharedBuff = pTempSharedBuff;

		goto EXIT;
		break;
	default:
		break;
	}

	return __PROXY_SetConfig(hComponent,
							nParamIndex,
							pComponentParameterStructure,
							NULL);

 EXIT:
	return eError;
}

static OMX_ERRORTYPE CameraSetParam(OMX_IN OMX_HANDLETYPE
    hComponent, OMX_IN OMX_INDEXTYPE nParamIndex,
    OMX_INOUT OMX_PTR pComponentParameterStructure)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    struct ion_handle *handle;
    OMX_U32 i =0;
    OMX_S32 ret = 0;
    PROXY_COMPONENT_PRIVATE *pCompPrv;
    OMX_COMPONENTTYPE *hComp = (OMX_COMPONENTTYPE *)hComponent;
    OMX_U32 stride_Y = 0, stride_UV = 0;
    OMX_TI_PARAM_VTCSLICE *pVtcConfig;// = (OMX_TI_PARAM_VTCSLICE *)pComponentParameterStructure;
    OMX_TI_PARAM_COMPONENTBUFALLOCTYPE *bufferalloc = NULL;
    int size = 0;
    int fd1 = -1, fd2 = -1;

    pCompPrv = (PROXY_COMPONENT_PRIVATE *)hComp->pComponentPrivate;
    //fprintf(stdout, "DOMX: CameraSetParam: called!!!\n");
    switch (nParamIndex)
    {
        case OMX_TI_IndexParamVtcSlice:
            pVtcConfig = (OMX_TI_PARAM_VTCSLICE *)pComponentParameterStructure;
            fprintf(stdout, "DOMX: CameraSetParam: OMX_TI_IndexParamVtcSlice is called!!!\n");
            DOMX_ERROR("CameraSetParam Called for Vtc Slice index\n");

            //fprintf(stdout, "CameraSetParam Called for Vtc Slice height = %d\n", ((OMX_TI_PARAM_VTCSLICE *)pComponentParameterStructure)->nSliceHeight);
		    // MAX_NUM_INTERNAL_BUFFERS;

    		for(i=0; i < MAX_NUM_INTERNAL_BUFFERS; i++) {
                    pVtcConfig->nInternalBuffers = i;
		    ret = ion_alloc_tiler(pCompPrv->ion_fd, MAX_VTC_WIDTH_WITH_VNF, MAX_VTC_HEIGHT_WITH_VNF, TILER_PIXEL_FMT_8BIT, OMAP_ION_HEAP_TILER_MASK, &handle, (size_t *)&stride_Y);
			if (ret < 0) {
				DOMX_ERROR ("ION allocation failed - %s", strerror(errno));
				goto EXIT;
			}

			ret = ion_share(pCompPrv->ion_fd, handle, &fd1);
			if (ret < 0) {
				DOMX_ERROR("ION share failed");
				ion_free(pCompPrv->ion_fd, handle);
				goto EXIT;
			}

			pVtcConfig->IonBufhdl[0] = (OMX_PTR)(fd1);

		    //fprintf(stdout, "DOMX: ION Buffer#%d: Y: 0x%x\n", i, pVtcConfig->IonBufhdl[0]);

			ret = ion_alloc_tiler(pCompPrv->ion_fd, MAX_VTC_WIDTH_WITH_VNF/2, MAX_VTC_HEIGHT_WITH_VNF/2, TILER_PIXEL_FMT_16BIT, OMAP_ION_HEAP_TILER_MASK, &handle, (size_t *)&stride_UV);
			if (ret < 0) {
				DOMX_ERROR ("ION allocation failed - %s", strerror(errno));
				goto EXIT;
			}

			ret = ion_share(pCompPrv->ion_fd, handle, &fd2);
			if (ret < 0) {
				DOMX_ERROR("ION share failed");
				ion_free(pCompPrv->ion_fd, handle);
				goto EXIT;
			}

		    pVtcConfig->IonBufhdl[1] = (OMX_PTR)(fd2);
                    gCamIonHdl[i][0] = pVtcConfig->IonBufhdl[0];
                    gCamIonHdl[i][1] = pVtcConfig->IonBufhdl[1];
		    //fprintf(stdout, "DOMX: ION Buffer#%d: UV: 0x%x\n", i, pVtcConfig->IonBufhdl[1]);
		    eError = __PROXY_SetParameter(hComponent,
                                              OMX_TI_IndexParamVtcSlice,
					      pVtcConfig,
					pVtcConfig->IonBufhdl, 2);
		    close(fd1);
		    close(fd2);
               }
		goto EXIT;
	case OMX_TI_IndexParamComponentBufferAllocation: {
                OMX_U32 port = 0, index = 0;
		int fd;
		bufferalloc = (OMX_TI_PARAM_COMPONENTBUFALLOCTYPE *)
			pComponentParameterStructure;

                port = bufferalloc->nPortIndex;
                index = bufferalloc->nIndex;

		size = bufferalloc->nAllocWidth * bufferalloc->nAllocLines;
		ret = ion_alloc_tiler (pCompPrv->ion_fd, size, 1,
				       TILER_PIXEL_FMT_PAGE,
				       OMAP_ION_HEAP_TILER_ALLOCATION_MASK,
				       &handle, &stride_Y);
		if (ret < 0) {
			DOMX_ERROR ("ION allocation failed - %s", strerror(errno));
			goto EXIT;
		}

		ret = ion_share(pCompPrv->ion_fd, handle, &fd);
		if (ret < 0) {
			DOMX_ERROR("ION share failed");
			ion_free(pCompPrv->ion_fd, handle);
			goto EXIT;
		}

		bufferalloc->pBuf[0] = fd;
		eError = __PROXY_SetParameter(hComponent,
					      OMX_TI_IndexParamComponentBufferAllocation,
					      bufferalloc, &bufferalloc->pBuf[0], 1);
                if (eError != OMX_ErrorNone) {
                   ion_free(pCompPrv->ion_fd, handle);
                } else {
                   if (gComponentBufferAllocation[port][index]) {
                       ion_free(pCompPrv->ion_fd, gComponentBufferAllocation[port][index]);
                   }
                   gComponentBufferAllocation[port][index] = handle;
                }
		close (fd);
        }
		goto EXIT;
		break;
	default:
		 break;
	}
	eError = __PROXY_SetParameter(hComponent,
								nParamIndex,
								pComponentParameterStructure,
							NULL, 0);

	if (eError != OMX_ErrorNone) {
		DOMX_ERROR(" CameraSetParam: Error in SetParam 0x%x", eError);
	}
EXIT:
    return eError;
}
OMX_ERRORTYPE OMX_ComponentInit(OMX_HANDLETYPE hComponent)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_ERRORTYPE dcc_eError = OMX_ErrorNone;
	OMX_COMPONENTTYPE *pHandle = NULL;
	PROXY_COMPONENT_PRIVATE *pComponentPrivate;
        OMX_U32 i = 0, j = 0;
	pHandle = (OMX_COMPONENTTYPE *) hComponent;
	TIMM_OSAL_ERRORTYPE eOsalError = TIMM_OSAL_ERR_NONE;
	DOMX_ENTER("_____________________INSIDE CAMERA PROXY"
	    "WRAPPER__________________________\n");
	pHandle->pComponentPrivate = (PROXY_COMPONENT_PRIVATE *)
	    TIMM_OSAL_Malloc(sizeof(PROXY_COMPONENT_PRIVATE),
	    TIMM_OSAL_TRUE, 0, TIMMOSAL_MEM_SEGMENT_INT);

	pComponentPrivate =
	    (PROXY_COMPONENT_PRIVATE *) pHandle->pComponentPrivate;
	if (pHandle->pComponentPrivate == NULL)
	{
		DOMX_ERROR(" ERROR IN ALLOCATING PROXY COMPONENT"
		    "PRIVATE STRUCTURE");
		eError = OMX_ErrorInsufficientResources;
		goto EXIT;
	}
	TIMM_OSAL_Memset(pComponentPrivate, 0,
		sizeof(PROXY_COMPONENT_PRIVATE));

	pComponentPrivate->cCompName =
	    TIMM_OSAL_Malloc(MAX_COMPONENT_NAME_LENGTH * sizeof(OMX_U8),
	    TIMM_OSAL_TRUE, 0, TIMMOSAL_MEM_SEGMENT_INT);
	/*Copying component Name - this will be picked up in the proxy common */
	assert(strlen(COMPONENT_NAME) + 1 < MAX_COMPONENT_NAME_LENGTH);
	TIMM_OSAL_Memcpy(pComponentPrivate->cCompName, COMPONENT_NAME,
	    strlen(COMPONENT_NAME) + 1);

	/*Calling Proxy Common Init() */
	eError = OMX_ProxyCommonInit(hComponent);
	if (eError != OMX_ErrorNone)
	{
		DOMX_ERROR("\Error in Initializing Proxy");
		TIMM_OSAL_Free(pComponentPrivate->cCompName);
		TIMM_OSAL_Free(pComponentPrivate);
		goto EXIT;
	}
        for(i=0; i < MAX_NUM_INTERNAL_BUFFERS; i++) {
            gCamIonHdl[i][0] = NULL;
            gCamIonHdl[i][1] = NULL;
        }

        for (i = 0; i < PROXY_MAXNUMOFPORTS; i++) {
            for (j = 0; j < MAX_NUM_INTERNAL_BUFFERS; j++) {
                gComponentBufferAllocation[i][j] = NULL;
            }
        }

	pHandle->ComponentDeInit = ComponentPrivateDeInit;
	pHandle->GetConfig = CameraGetConfig;
	pHandle->SetConfig = CameraSetConfig;
	pHandle->SendCommand = Camera_SendCommand;
	pHandle->SetParameter = CameraSetParam;

      EXIT:
	return eError;
}

/* ===========================================================================*/
/**
 * @name DCC_Init()
 * @brief
 * @param void
 * @return OMX_ErrorNone = Successful
 * @sa TBD
 *
 */
/* ===========================================================================*/
OMX_ERRORTYPE DCC_Init(OMX_HANDLETYPE hComponent)
{
	OMX_TI_PARAM_DCCURIINFO param;
	OMX_PTR ptempbuf;
	OMX_U16 nIndex = 0;
	OMX_ERRORTYPE eError = OMX_ErrorNone;
#ifdef USE_ION
	int ret;
	size_t stride;
#endif

	OMX_S32 status = 0;
	OMX_STRING dcc_dir[200];
	OMX_U16 i;
	_PROXY_OMX_INIT_PARAM(&param, OMX_TI_PARAM_DCCURIINFO);

	DOMX_ENTER("ENTER");
	/* Read the the DCC URI info */
	for (nIndex = 0; eError != OMX_ErrorNoMore; nIndex++)
	{
		param.nIndex = nIndex;
		eError =
			OMX_GetParameter(hComponent,
			OMX_TI_IndexParamDccUriInfo, &param);

		PROXY_assert((eError == OMX_ErrorNone) ||
			(eError == OMX_ErrorNoMore), eError,
			"Error in GetParam for Dcc URI info");

		if (eError == OMX_ErrorNone)
		{
			DOMX_DEBUG("DCC URI's %s ", param.sDCCURI);
			dcc_dir[nIndex] =
				TIMM_OSAL_Malloc(sizeof(OMX_U8) *
				(strlen(DCC_PATH) + MAX_URI_LENGTH + 1),
				TIMM_OSAL_TRUE, 0, TIMMOSAL_MEM_SEGMENT_INT);
			PROXY_assert(dcc_dir[nIndex] != NULL,
				OMX_ErrorInsufficientResources, "Malloc failed");
			strcpy(dcc_dir[nIndex], DCC_PATH);
			strncat(dcc_dir[nIndex], (OMX_STRING) param.sDCCURI, MAX_URI_LENGTH);
			strcat(dcc_dir[nIndex], "/");
		}
	}

	/* setting  back errortype OMX_ErrorNone */
	if (eError == OMX_ErrorNoMore)
	{
		eError = OMX_ErrorNone;
	}

	dccbuf_size = read_DCCdir(NULL, dcc_dir, nIndex);

    if(dccbuf_size <= 0)
    {
	    DOMX_DEBUG("No DCC files found, switching back to default DCC");
        return OMX_ErrorInsufficientResources;
    }

#ifdef USE_ION
	ion_fd = ion_open();
	if(ion_fd == 0)
	{
		DOMX_ERROR("ion_open failed!!!");
		return OMX_ErrorInsufficientResources;
	}
	dccbuf_size = (dccbuf_size + LINUX_PAGE_SIZE -1) & ~(LINUX_PAGE_SIZE - 1);
	ret = ion_alloc(ion_fd, dccbuf_size, 0x1000, 1 << ION_HEAP_TYPE_CARVEOUT,
		(struct ion_handle **)&DCC_Buff);

        if (ret || ((int)DCC_Buff == -ENOMEM)) {
                ret = ion_alloc_tiler(ion_fd, dccbuf_size, 1, TILER_PIXEL_FMT_PAGE,
                                OMAP_ION_HEAP_TILER_MASK, &DCC_Buff, &stride);
        }

        if (ret || ((int)DCC_Buff == -ENOMEM)) {
                DOMX_ERROR("FAILED to allocate DCC buffer of size=%d. ret=0x%x",
                                        dccbuf_size, ret);
                return OMX_ErrorInsufficientResources;
        }

	if (ion_map(ion_fd, DCC_Buff, dccbuf_size, PROT_READ | PROT_WRITE, MAP_SHARED, 0,
                   (unsigned char **)&DCC_Buff_ptr, &mmap_fd) < 0)
	{
		DOMX_ERROR("userspace mapping of ION buffers returned error");
		return OMX_ErrorInsufficientResources;
	}
	ptempbuf = DCC_Buff_ptr;
#endif
	dccbuf_size = read_DCCdir(ptempbuf, dcc_dir, nIndex);

	PROXY_assert(dccbuf_size > 0, OMX_ErrorInsufficientResources,
		"ERROR in copy DCC files into buffer");

 EXIT:
	for (i = 0; i < nIndex - 1; i++)
	{
			TIMM_OSAL_Free(dcc_dir[i]);
	}

	return eError;

}

/* ===========================================================================*/
/**
 * @name send_DCCBufPtr()
 * @brief : Sending the DCC uri buff addr to ducati
 * @param void
 * @return return = 0 is successful
 * @sa TBD
 *
 */
/* ===========================================================================*/

OMX_ERRORTYPE send_DCCBufPtr(OMX_HANDLETYPE hComponent)
{
	OMX_TI_CONFIG_SHAREDBUFFER uribufparam;
	OMX_ERRORTYPE eError = OMX_ErrorNone;

	_PROXY_OMX_INIT_PARAM(&uribufparam, OMX_TI_CONFIG_SHAREDBUFFER);
	uribufparam.nPortIndex = OMX_ALL;

	DOMX_ENTER("ENTER");

	uribufparam.nSharedBuffSize = dccbuf_size;
#ifdef USE_ION
	uribufparam.pSharedBuff = (OMX_PTR) mmap_fd;
#else
	uribufparam.pSharedBuff = (OMX_PTR) DCC_Buff;
#endif

	DOMX_DEBUG("SYSLINK MAPPED ADDR:  0x%x sizeof buffer %d",
		uribufparam.pSharedBuff, uribufparam.nSharedBuffSize);

	eError = __PROXY_SetParameter(hComponent,
								OMX_TI_IndexParamDccUriBuffer,
								&uribufparam,
								&(uribufparam.pSharedBuff), 1);

	if (eError != OMX_ErrorNone) {
		DOMX_ERROR(" Error in SetParam for DCC Uri Buffer 0x%x", eError);
	}

	DOMX_EXIT("EXIT");
	return eError;
}

/* ===========================================================================*/
/**
 * @name read_DCCdir()
 * @brief : copies all the dcc profiles into the allocated 1D-Tiler buffer
 *          and returns the size of the buffer.
 * @param void : OMX_PTR is null then returns the size of the DCC directory
 * @return return = size of the DCC directory or error in case of any failures
 *		    in file read or open
 * @sa TBD
 *
 */
/* ===========================================================================*/
OMX_S32 read_DCCdir(OMX_PTR buffer, OMX_STRING * dir_path, OMX_U16 numofURI)
{
	FILE *pFile;
	OMX_S32 lSize;
	OMX_S32 dcc_buf_size = 0;
	size_t result;
	OMX_STRING filename;
	char temp[200];
	OMX_STRING dotdot = "..";
	DIR *d;
	struct dirent *dir;
	OMX_U16 i = 0;
	OMX_S32 ret = 0;

	DOMX_ENTER("ENTER");
	for (i = 0; i < numofURI - 1; i++)
	{
		d = opendir(dir_path[i]);
		if (d)
		{
			/* read each filename */
			while ((dir = readdir(d)) != NULL)
			{
				filename = dir->d_name;
				strcpy(temp, dir_path[i]);
				strcat(temp, filename);
				if ((*filename != *dotdot))
				{
					DOMX_DEBUG
					    ("\n\t DCC Profiles copying into buffer => %s mpu_addr: %p",
					    temp, buffer);
					pFile = fopen(temp, "rb");
					if (pFile == NULL)
					{
						DOMX_ERROR("File open error");
						ret = -1;
					} else
					{
						fseek(pFile, 0, SEEK_END);
						lSize = ftell(pFile);
						rewind(pFile);
						/* buffer is not NULL then copy all the DCC profiles into buffer
						   else return the size of the DCC directory */
						if (buffer)
						{
							// copy file into the buffer:
							result =
							    fread(buffer, 1,
							    lSize, pFile);
							if (result != (size_t) lSize)
							{
								DOMX_ERROR
								    ("fread: Reading error");
								ret = -1;
							}
							buffer =
							    buffer + lSize;
						}
						/* getting the size of the total dcc files available in FS */
						dcc_buf_size =
						    dcc_buf_size + lSize;
						// terminate
						fclose(pFile);
					}
				}
			}
			closedir(d);
		}
	}
	if (ret == 0)
		ret = dcc_buf_size;

	DOMX_EXIT("return %d", ret);
	return ret;
}

/* ===========================================================================*/
/**
 * @name DCC_Deinit()
 * @brief
 * @param void
 * @return void
 * @sa TBD
 *
 */
/* ===========================================================================*/
void DCC_DeInit()
{
	DOMX_ENTER("ENTER");

	if (DCC_Buff)
	{
#ifdef USE_ION
		munmap(DCC_Buff_ptr, dccbuf_size);
		close(mmap_fd);
		ion_free(ion_fd, DCC_Buff);
		ion_close(ion_fd);
		DCC_Buff = NULL;
#endif
	}

	DOMX_EXIT("EXIT");
}



/*===============================================================*/
/** @fn Cam_Setup : This function is called when the the OMX Camera library is
 *                  loaded. It creates a mutex, which is used during DCC_Init()
 */
/*===============================================================*/
void __attribute__ ((constructor)) Cam_Setup(void)
{
	TIMM_OSAL_ERRORTYPE eError = TIMM_OSAL_ERR_NONE;

	eError = TIMM_OSAL_MutexCreate(&cam_mutex);
	if (eError != TIMM_OSAL_ERR_NONE)
	{
		TIMM_OSAL_Error("Creation of default mutex failed");
	}
}


/*===============================================================*/
/** @fn Cam_Destroy : This function is called when the the OMX Camera library is
 *                    unloaded. It destroys the mutex which was created by
 *                    Core_Setup().
 *
 */
/*===============================================================*/
void __attribute__ ((destructor)) Cam_Destroy(void)
{
	TIMM_OSAL_ERRORTYPE eError = TIMM_OSAL_ERR_NONE;

	eError = TIMM_OSAL_MutexDelete(cam_mutex);
	if (eError != TIMM_OSAL_ERR_NONE)
	{
		TIMM_OSAL_Error("Destruction of default mutex failed");
	}
}
