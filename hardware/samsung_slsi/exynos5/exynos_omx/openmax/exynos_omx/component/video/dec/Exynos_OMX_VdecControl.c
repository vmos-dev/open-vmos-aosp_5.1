/*
 *
 * Copyright 2012 Samsung Electronics S.LSI Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * @file        Exynos_OMX_VdecControl.c
 * @brief
 * @author      SeungBeom Kim (sbcrux.kim@samsung.com)
 * @version     2.0.0
 * @history
 *   2012.02.20 : Create
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Exynos_OMX_Macros.h"
#include "Exynos_OSAL_Event.h"
#include "Exynos_OMX_Vdec.h"
#include "Exynos_OMX_VdecControl.h"
#include "Exynos_OMX_Basecomponent.h"
#include "Exynos_OSAL_Thread.h"
#include "Exynos_OSAL_Semaphore.h"
#include "Exynos_OSAL_Mutex.h"
#include "Exynos_OSAL_ETC.h"
#include "Exynos_OSAL_SharedMemory.h"
#include "Exynos_OSAL_Queue.h"
#include "csc.h"

#ifdef USE_ANB
#include "Exynos_OSAL_Android.h"
#endif

#undef  EXYNOS_LOG_TAG
#define EXYNOS_LOG_TAG    "EXYNOS_VIDEO_DECCONTROL"
#define EXYNOS_LOG_OFF
//#define EXYNOS_TRACE_ON
#include "Exynos_OSAL_Log.h"


OMX_ERRORTYPE Exynos_OMX_UseBuffer(
    OMX_IN OMX_HANDLETYPE            hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE **ppBufferHdr,
    OMX_IN OMX_U32                   nPortIndex,
    OMX_IN OMX_PTR                   pAppPrivate,
    OMX_IN OMX_U32                   nSizeBytes,
    OMX_IN OMX_U8                   *pBuffer)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;
    EXYNOS_OMX_BASEPORT      *pExynosPort = NULL;
    OMX_BUFFERHEADERTYPE  *temp_bufferHeader = NULL;
    OMX_U32                i = 0;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    pExynosPort = &pExynosComponent->pExynosPort[nPortIndex];
    if (nPortIndex >= pExynosComponent->portParam.nPorts) {
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }
    if (pExynosPort->portState != OMX_StateIdle) {
        ret = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    if (CHECK_PORT_TUNNELED(pExynosPort) && CHECK_PORT_BUFFER_SUPPLIER(pExynosPort)) {
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    temp_bufferHeader = (OMX_BUFFERHEADERTYPE *)Exynos_OSAL_Malloc(sizeof(OMX_BUFFERHEADERTYPE));
    if (temp_bufferHeader == NULL) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    Exynos_OSAL_Memset(temp_bufferHeader, 0, sizeof(OMX_BUFFERHEADERTYPE));

    for (i = 0; i < pExynosPort->portDefinition.nBufferCountActual; i++) {
        if (pExynosPort->bufferStateAllocate[i] == BUFFER_STATE_FREE) {
            pExynosPort->extendBufferHeader[i].OMXBufferHeader = temp_bufferHeader;
            pExynosPort->bufferStateAllocate[i] = (BUFFER_STATE_ASSIGNED | HEADER_STATE_ALLOCATED);
            INIT_SET_SIZE_VERSION(temp_bufferHeader, OMX_BUFFERHEADERTYPE);
            temp_bufferHeader->pBuffer        = pBuffer;
            temp_bufferHeader->nAllocLen      = nSizeBytes;
            temp_bufferHeader->pAppPrivate    = pAppPrivate;
            if (nPortIndex == INPUT_PORT_INDEX)
                temp_bufferHeader->nInputPortIndex = INPUT_PORT_INDEX;
            else
                temp_bufferHeader->nOutputPortIndex = OUTPUT_PORT_INDEX;

            pExynosPort->assignedBufferNum++;
            if (pExynosPort->assignedBufferNum == pExynosPort->portDefinition.nBufferCountActual) {
                pExynosPort->portDefinition.bPopulated = OMX_TRUE;
                /* Exynos_OSAL_MutexLock(pExynosComponent->compMutex); */
                Exynos_OSAL_SemaphorePost(pExynosPort->loadedResource);
                /* Exynos_OSAL_MutexUnlock(pExynosComponent->compMutex); */
            }
            *ppBufferHdr = temp_bufferHeader;
            ret = OMX_ErrorNone;
            goto EXIT;
        }
    }

    Exynos_OSAL_Free(temp_bufferHeader);
    ret = OMX_ErrorInsufficientResources;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_AllocateBuffer(
    OMX_IN OMX_HANDLETYPE            hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE **ppBuffer,
    OMX_IN OMX_U32                   nPortIndex,
    OMX_IN OMX_PTR                   pAppPrivate,
    OMX_IN OMX_U32                   nSizeBytes)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = NULL;
    EXYNOS_OMX_BASEPORT      *pExynosPort = NULL;
    OMX_BUFFERHEADERTYPE  *temp_bufferHeader = NULL;
    OMX_U8                *temp_buffer = NULL;
    int                    temp_buffer_fd = -1;
    OMX_U32                i = 0;
    MEMORY_TYPE            mem_type;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;

    pExynosPort = &pExynosComponent->pExynosPort[nPortIndex];
    if (nPortIndex >= pExynosComponent->portParam.nPorts) {
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }
/*
    if (pExynosPort->portState != OMX_StateIdle ) {
        ret = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }
*/
    if (CHECK_PORT_TUNNELED(pExynosPort) && CHECK_PORT_BUFFER_SUPPLIER(pExynosPort)) {
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    if ((pVideoDec->bDRMPlayerMode == OMX_TRUE) && (nPortIndex == INPUT_PORT_INDEX)) {
        mem_type = SECURE_MEMORY;
    } else if (pExynosPort->bufferProcessType == BUFFER_SHARE) {
        mem_type = NORMAL_MEMORY;
    } else {
        mem_type = SYSTEM_MEMORY;
    }
    temp_buffer = Exynos_OSAL_SharedMemory_Alloc(pVideoDec->hSharedMemory, nSizeBytes, mem_type);
    if (temp_buffer == NULL) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    temp_buffer_fd = Exynos_OSAL_SharedMemory_VirtToION(pVideoDec->hSharedMemory, temp_buffer);

    temp_bufferHeader = (OMX_BUFFERHEADERTYPE *)Exynos_OSAL_Malloc(sizeof(OMX_BUFFERHEADERTYPE));
    if (temp_bufferHeader == NULL) {
        Exynos_OSAL_SharedMemory_Free(pVideoDec->hSharedMemory, temp_buffer);
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    Exynos_OSAL_Memset(temp_bufferHeader, 0, sizeof(OMX_BUFFERHEADERTYPE));

    for (i = 0; i < pExynosPort->portDefinition.nBufferCountActual; i++) {
        if (pExynosPort->bufferStateAllocate[i] == BUFFER_STATE_FREE) {
            pExynosPort->extendBufferHeader[i].OMXBufferHeader = temp_bufferHeader;
            pExynosPort->extendBufferHeader[i].buf_fd[0] = temp_buffer_fd;
            pExynosPort->bufferStateAllocate[i] = (BUFFER_STATE_ALLOCATED | HEADER_STATE_ALLOCATED);
            INIT_SET_SIZE_VERSION(temp_bufferHeader, OMX_BUFFERHEADERTYPE);
            if (mem_type == SECURE_MEMORY)
                temp_bufferHeader->pBuffer = temp_buffer_fd;
            else
                temp_bufferHeader->pBuffer = temp_buffer;
            temp_bufferHeader->nAllocLen      = nSizeBytes;
            temp_bufferHeader->pAppPrivate    = pAppPrivate;
            if (nPortIndex == INPUT_PORT_INDEX)
                temp_bufferHeader->nInputPortIndex = INPUT_PORT_INDEX;
            else
                temp_bufferHeader->nOutputPortIndex = OUTPUT_PORT_INDEX;
            pExynosPort->assignedBufferNum++;
            if (pExynosPort->assignedBufferNum == pExynosPort->portDefinition.nBufferCountActual) {
                pExynosPort->portDefinition.bPopulated = OMX_TRUE;
                /* Exynos_OSAL_MutexLock(pExynosComponent->compMutex); */
                Exynos_OSAL_SemaphorePost(pExynosPort->loadedResource);
                /* Exynos_OSAL_MutexUnlock(pExynosComponent->compMutex); */
            }
            *ppBuffer = temp_bufferHeader;
            ret = OMX_ErrorNone;
            goto EXIT;
        }
    }

    Exynos_OSAL_Free(temp_bufferHeader);
    Exynos_OSAL_SharedMemory_Free(pVideoDec->hSharedMemory, temp_buffer);

    ret = OMX_ErrorInsufficientResources;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_FreeBuffer(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_U32        nPortIndex,
    OMX_IN OMX_BUFFERHEADERTYPE *pBufferHdr)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = NULL;
    EXYNOS_OMX_BASEPORT      *pExynosPort = NULL;
    OMX_BUFFERHEADERTYPE  *temp_bufferHeader = NULL;
    OMX_U8                *temp_buffer = NULL;
    OMX_U32                i = 0;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    pExynosPort = &pExynosComponent->pExynosPort[nPortIndex];

    if (CHECK_PORT_TUNNELED(pExynosPort) && CHECK_PORT_BUFFER_SUPPLIER(pExynosPort)) {
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    if ((pExynosPort->portState != OMX_StateLoaded) && (pExynosPort->portState != OMX_StateInvalid)) {
        (*(pExynosComponent->pCallbacks->EventHandler)) (pOMXComponent,
                        pExynosComponent->callbackData,
                        (OMX_U32)OMX_EventError,
                        (OMX_U32)OMX_ErrorPortUnpopulated,
                        nPortIndex, NULL);
    }

    for (i = 0; i < /*pExynosPort->portDefinition.nBufferCountActual*/MAX_BUFFER_NUM; i++) {
        if (((pExynosPort->bufferStateAllocate[i] | BUFFER_STATE_FREE) != 0) && (pExynosPort->extendBufferHeader[i].OMXBufferHeader != NULL)) {
            if (pExynosPort->extendBufferHeader[i].OMXBufferHeader->pBuffer == pBufferHdr->pBuffer) {
                if (pExynosPort->bufferStateAllocate[i] & BUFFER_STATE_ALLOCATED) {
                    if ((pVideoDec->bDRMPlayerMode == OMX_TRUE) && (nPortIndex == INPUT_PORT_INDEX)) {
                        OMX_PTR mapBuffer = Exynos_OSAL_SharedMemory_IONToVirt(pVideoDec->hSharedMemory, (int)pExynosPort->extendBufferHeader[i].OMXBufferHeader->pBuffer);
                        Exynos_OSAL_SharedMemory_Free(pVideoDec->hSharedMemory, mapBuffer);
                    } else {
                        Exynos_OSAL_SharedMemory_Free(pVideoDec->hSharedMemory, pExynosPort->extendBufferHeader[i].OMXBufferHeader->pBuffer);
                    }
                    pExynosPort->extendBufferHeader[i].OMXBufferHeader->pBuffer = NULL;
                    pBufferHdr->pBuffer = NULL;
                } else if (pExynosPort->bufferStateAllocate[i] & BUFFER_STATE_ASSIGNED) {
                    ; /* None*/
                }
                pExynosPort->assignedBufferNum--;
                if (pExynosPort->bufferStateAllocate[i] & HEADER_STATE_ALLOCATED) {
                    Exynos_OSAL_Free(pExynosPort->extendBufferHeader[i].OMXBufferHeader);
                    pExynosPort->extendBufferHeader[i].OMXBufferHeader = NULL;
                    pBufferHdr = NULL;
                }
                pExynosPort->bufferStateAllocate[i] = BUFFER_STATE_FREE;
                ret = OMX_ErrorNone;
                goto EXIT;
            }
        }
    }

EXIT:
    if (ret == OMX_ErrorNone) {
        if (pExynosPort->assignedBufferNum == 0) {
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pExynosPort->unloadedResource signal set");
            /* Exynos_OSAL_MutexLock(pExynosComponent->compMutex); */
            Exynos_OSAL_SemaphorePost(pExynosPort->unloadedResource);
            /* Exynos_OSAL_MutexUnlock(pExynosComponent->compMutex); */
            pExynosPort->portDefinition.bPopulated = OMX_FALSE;
        }
    }

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_AllocateTunnelBuffer(EXYNOS_OMX_BASEPORT *pOMXBasePort, OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE                 ret = OMX_ErrorNone;
    EXYNOS_OMX_BASEPORT             *pExynosPort = NULL;
    OMX_BUFFERHEADERTYPE         *temp_bufferHeader = NULL;
    OMX_U8                       *temp_buffer = NULL;
    OMX_U32                       bufferSize = 0;
    OMX_PARAM_PORTDEFINITIONTYPE  portDefinition;

    ret = OMX_ErrorTunnelingUnsupported;
EXIT:
    return ret;
}

OMX_ERRORTYPE Exynos_OMX_FreeTunnelBuffer(EXYNOS_OMX_BASEPORT *pOMXBasePort, OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    EXYNOS_OMX_BASEPORT* pExynosPort = NULL;
    OMX_BUFFERHEADERTYPE* temp_bufferHeader = NULL;
    OMX_U8 *temp_buffer = NULL;
    OMX_U32 bufferSize = 0;

    ret = OMX_ErrorTunnelingUnsupported;
EXIT:
    return ret;
}

OMX_ERRORTYPE Exynos_OMX_ComponentTunnelRequest(
    OMX_IN OMX_HANDLETYPE hComp,
    OMX_IN OMX_U32        nPort,
    OMX_IN OMX_HANDLETYPE hTunneledComp,
    OMX_IN OMX_U32        nTunneledPort,
    OMX_INOUT OMX_TUNNELSETUPTYPE *pTunnelSetup)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    ret = OMX_ErrorTunnelingUnsupported;
EXIT:
    return ret;
}

OMX_ERRORTYPE Exynos_OMX_GetFlushBuffer(EXYNOS_OMX_BASEPORT *pExynosPort, EXYNOS_OMX_DATABUFFER *pDataBuffer[])
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    FunctionIn();

    *pDataBuffer = NULL;

    if (pExynosPort->portWayType == WAY1_PORT) {
        *pDataBuffer = &pExynosPort->way.port1WayDataBuffer.dataBuffer;
    } else if (pExynosPort->portWayType == WAY2_PORT) {
            pDataBuffer[0] = &(pExynosPort->way.port2WayDataBuffer.inputDataBuffer);
            pDataBuffer[1] = &(pExynosPort->way.port2WayDataBuffer.outputDataBuffer);
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_FlushPort(OMX_COMPONENTTYPE *pOMXComponent, OMX_S32 portIndex)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_OMX_BASEPORT      *pExynosPort = NULL;
    OMX_BUFFERHEADERTYPE     *bufferHeader = NULL;
    EXYNOS_OMX_DATABUFFER    *pDataPortBuffer[2] = {NULL, NULL};
    EXYNOS_OMX_MESSAGE       *message = NULL;
    OMX_U32                flushNum = 0;
    OMX_S32                semValue = 0;
    int i = 0, maxBufferNum = 0;
    FunctionIn();

    pExynosPort = &pExynosComponent->pExynosPort[portIndex];

    while (Exynos_OSAL_GetElemNum(&pExynosPort->bufferQ) > 0) {
        Exynos_OSAL_Get_SemaphoreCount(pExynosComponent->pExynosPort[portIndex].bufferSemID, &semValue);
        if (semValue == 0)
            Exynos_OSAL_SemaphorePost(pExynosComponent->pExynosPort[portIndex].bufferSemID);

        Exynos_OSAL_SemaphoreWait(pExynosComponent->pExynosPort[portIndex].bufferSemID);
        message = (EXYNOS_OMX_MESSAGE *)Exynos_OSAL_Dequeue(&pExynosPort->bufferQ);
        if ((message != NULL) && (message->messageType != EXYNOS_OMX_CommandFakeBuffer)) {
            bufferHeader = (OMX_BUFFERHEADERTYPE *)message->pCmdData;
            bufferHeader->nFilledLen = 0;

            if (portIndex == OUTPUT_PORT_INDEX) {
                Exynos_OMX_OutputBufferReturn(pOMXComponent, bufferHeader);
            } else if (portIndex == INPUT_PORT_INDEX) {
                Exynos_OMX_InputBufferReturn(pOMXComponent, bufferHeader);
            }
        }
        Exynos_OSAL_Free(message);
        message = NULL;
    }

    Exynos_OMX_GetFlushBuffer(pExynosPort, pDataPortBuffer);
    if (portIndex == INPUT_PORT_INDEX) {
        if (pDataPortBuffer[0]->dataValid == OMX_TRUE)
            Exynos_InputBufferReturn(pOMXComponent, pDataPortBuffer[0]);
        if (pDataPortBuffer[1]->dataValid == OMX_TRUE)
            Exynos_InputBufferReturn(pOMXComponent, pDataPortBuffer[1]);
    } else if (portIndex == OUTPUT_PORT_INDEX) {
        if (pDataPortBuffer[0]->dataValid == OMX_TRUE)
            Exynos_OutputBufferReturn(pOMXComponent, pDataPortBuffer[0]);
        if (pDataPortBuffer[1]->dataValid == OMX_TRUE)
            Exynos_OutputBufferReturn(pOMXComponent, pDataPortBuffer[1]);
    }

    if (pExynosComponent->bMultiThreadProcess == OMX_TRUE) {
        if (pExynosPort->bufferProcessType == BUFFER_SHARE) {
            if (pExynosPort->processData.bufferHeader != NULL) {
                if (portIndex == INPUT_PORT_INDEX) {
                    Exynos_OMX_InputBufferReturn(pOMXComponent, pExynosPort->processData.bufferHeader);
                } else if (portIndex == OUTPUT_PORT_INDEX) {
                    Exynos_OMX_OutputBufferReturn(pOMXComponent, pExynosPort->processData.bufferHeader);
                }
            }
            Exynos_ResetCodecData(&pExynosPort->processData);

            maxBufferNum = pExynosPort->portDefinition.nBufferCountActual;
            for (i = 0; i < maxBufferNum; i++) {
                if (pExynosPort->extendBufferHeader[i].bBufferInOMX == OMX_TRUE) {
                    if (portIndex == OUTPUT_PORT_INDEX) {
                        Exynos_OMX_OutputBufferReturn(pOMXComponent, pExynosPort->extendBufferHeader[i].OMXBufferHeader);
                    } else if (portIndex == INPUT_PORT_INDEX) {
                        Exynos_OMX_InputBufferReturn(pOMXComponent, pExynosPort->extendBufferHeader[i].OMXBufferHeader);
                    }
                }
            }
        }
    } else {
        Exynos_ResetCodecData(&pExynosPort->processData);
    }

    if ((pExynosPort->bufferProcessType == BUFFER_SHARE) &&
        (portIndex == OUTPUT_PORT_INDEX)){
        EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = NULL;

        if (pOMXComponent->pComponentPrivate == NULL) {
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }
        pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
        pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;

        Exynos_OSAL_RefANB_Reset(pVideoDec->hRefHandle);
    }

    while(1) {
        OMX_S32 cnt = 0;
        Exynos_OSAL_Get_SemaphoreCount(pExynosComponent->pExynosPort[portIndex].bufferSemID, &cnt);
        if (cnt <= 0)
            break;
        Exynos_OSAL_SemaphoreWait(pExynosComponent->pExynosPort[portIndex].bufferSemID);
    }
    Exynos_OSAL_ResetQueue(&pExynosPort->bufferQ);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_BufferFlush(OMX_COMPONENTTYPE *pOMXComponent, OMX_S32 nPortIndex, OMX_BOOL bEvent)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = NULL;
    EXYNOS_OMX_BASEPORT      *pExynosPort = NULL;
    EXYNOS_OMX_DATABUFFER    *flushPortBuffer[2] = {NULL, NULL};
    OMX_U32                   i = 0, cnt = 0;

    FunctionIn();

    if (pOMXComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE,"OMX_CommandFlush start, port:%d", nPortIndex);

    pExynosComponent->pExynosPort[nPortIndex].bIsPortFlushed = OMX_TRUE;

    if (pExynosComponent->bMultiThreadProcess == OMX_FALSE) {
        Exynos_OSAL_SignalSet(pExynosComponent->pauseEvent);
    } else {
        Exynos_OSAL_SignalSet(pExynosComponent->pExynosPort[nPortIndex].pauseEvent);
    }

    pExynosPort = &pExynosComponent->pExynosPort[nPortIndex];
    Exynos_OMX_GetFlushBuffer(pExynosPort, flushPortBuffer);

    if ((pExynosComponent->pExynosPort[nPortIndex].bufferProcessType & BUFFER_COPY) == BUFFER_COPY)
        Exynos_OSAL_SemaphorePost(pExynosPort->codecSemID);
    Exynos_OSAL_SemaphorePost(pExynosPort->bufferSemID);

    pVideoDec->exynos_codec_bufferProcessRun(pOMXComponent, nPortIndex);
    Exynos_OSAL_MutexLock(flushPortBuffer[0]->bufferMutex);
    pVideoDec->exynos_codec_stop(pOMXComponent, nPortIndex);
    Exynos_OSAL_MutexLock(flushPortBuffer[1]->bufferMutex);
    ret = Exynos_OMX_FlushPort(pOMXComponent, nPortIndex);
    if (pVideoDec->bReconfigDPB == OMX_TRUE)
        pVideoDec->exynos_codec_reconfigAllBuffers(pOMXComponent, nPortIndex);
    else if ((pExynosComponent->pExynosPort[nPortIndex].bufferProcessType & BUFFER_COPY) == BUFFER_COPY)
        pVideoDec->exynos_codec_enqueueAllBuffer(pOMXComponent, nPortIndex);
    Exynos_ResetCodecData(&pExynosPort->processData);

    if (ret == OMX_ErrorNone) {
        if (nPortIndex == INPUT_PORT_INDEX) {
            pExynosComponent->checkTimeStamp.needSetStartTimeStamp = OMX_TRUE;
            pExynosComponent->checkTimeStamp.needCheckStartTimeStamp = OMX_FALSE;
            Exynos_OSAL_Memset(pExynosComponent->timeStamp, -19771003, sizeof(OMX_TICKS) * MAX_TIMESTAMP);
            Exynos_OSAL_Memset(pExynosComponent->nFlags, 0, sizeof(OMX_U32) * MAX_FLAGS);
            pExynosComponent->getAllDelayBuffer = OMX_FALSE;
            pExynosComponent->bSaveFlagEOS = OMX_FALSE;
            pExynosComponent->bBehaviorEOS = OMX_FALSE;
            pExynosComponent->reInputData = OMX_FALSE;
        }

        pExynosComponent->pExynosPort[nPortIndex].bIsPortFlushed = OMX_FALSE;
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE,"OMX_CommandFlush EventCmdComplete, port:%d", nPortIndex);
        if (bEvent == OMX_TRUE)
            pExynosComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
                            pExynosComponent->callbackData,
                            OMX_EventCmdComplete,
                            OMX_CommandFlush, nPortIndex, NULL);
    }
    Exynos_OSAL_MutexUnlock(flushPortBuffer[1]->bufferMutex);
    Exynos_OSAL_MutexUnlock(flushPortBuffer[0]->bufferMutex);

EXIT:
    if ((ret != OMX_ErrorNone) && (pOMXComponent != NULL) && (pExynosComponent != NULL)) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR,"%s : %d", __FUNCTION__, __LINE__);
        pExynosComponent->pCallbacks->EventHandler(pOMXComponent,
                        pExynosComponent->callbackData,
                        OMX_EventError,
                        ret, 0, NULL);
    }

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_ResolutionUpdate(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE                  ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec          = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_OMX_BASEPORT           *pInputPort         = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT           *pOutputPort        = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

    pOutputPort->cropRectangle.nTop     = pOutputPort->newCropRectangle.nTop;
    pOutputPort->cropRectangle.nLeft    = pOutputPort->newCropRectangle.nLeft;
    pOutputPort->cropRectangle.nWidth   = pOutputPort->newCropRectangle.nWidth;
    pOutputPort->cropRectangle.nHeight  = pOutputPort->newCropRectangle.nHeight;

    pInputPort->portDefinition.format.video.nFrameWidth     = pInputPort->newPortDefinition.format.video.nFrameWidth;
    pInputPort->portDefinition.format.video.nFrameHeight    = pInputPort->newPortDefinition.format.video.nFrameHeight;
    pInputPort->portDefinition.format.video.nStride         = pInputPort->newPortDefinition.format.video.nStride;
    pInputPort->portDefinition.format.video.nSliceHeight    = pInputPort->newPortDefinition.format.video.nSliceHeight;

    pOutputPort->portDefinition.nBufferCountActual  = pOutputPort->newPortDefinition.nBufferCountActual;
    pOutputPort->portDefinition.nBufferCountMin     = pOutputPort->newPortDefinition.nBufferCountMin;

    Exynos_UpdateFrameSize(pOMXComponent);

    return ret;
}

OMX_ERRORTYPE Exynos_InputBufferReturn(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATABUFFER *dataBuffer)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_BASEPORT      *exynosOMXInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    OMX_BUFFERHEADERTYPE     *bufferHeader = NULL;

    FunctionIn();

    bufferHeader = dataBuffer->bufferHeader;

    if (bufferHeader != NULL) {
        if (exynosOMXInputPort->markType.hMarkTargetComponent != NULL ) {
            bufferHeader->hMarkTargetComponent      = exynosOMXInputPort->markType.hMarkTargetComponent;
            bufferHeader->pMarkData                 = exynosOMXInputPort->markType.pMarkData;
            exynosOMXInputPort->markType.hMarkTargetComponent = NULL;
            exynosOMXInputPort->markType.pMarkData = NULL;
        }

        if (bufferHeader->hMarkTargetComponent != NULL) {
            if (bufferHeader->hMarkTargetComponent == pOMXComponent) {
                pExynosComponent->pCallbacks->EventHandler(pOMXComponent,
                                pExynosComponent->callbackData,
                                OMX_EventMark,
                                0, 0, bufferHeader->pMarkData);
            } else {
                pExynosComponent->propagateMarkType.hMarkTargetComponent = bufferHeader->hMarkTargetComponent;
                pExynosComponent->propagateMarkType.pMarkData = bufferHeader->pMarkData;
            }
        }

        bufferHeader->nFilledLen = 0;
        bufferHeader->nOffset = 0;
        Exynos_OMX_InputBufferReturn(pOMXComponent, bufferHeader);
    }

    /* reset dataBuffer */
    Exynos_ResetDataBuffer(dataBuffer);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_InputBufferGetQueue(EXYNOS_OMX_BASECOMPONENT *pExynosComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorUndefined;
    EXYNOS_OMX_BASEPORT   *pExynosPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_MESSAGE    *message = NULL;
    EXYNOS_OMX_DATABUFFER *inputUseBuffer = NULL;

    FunctionIn();

    inputUseBuffer = &(pExynosPort->way.port2WayDataBuffer.inputDataBuffer);

    if (pExynosComponent->currentState != OMX_StateExecuting) {
        ret = OMX_ErrorUndefined;
        goto EXIT;
    } else if ((pExynosComponent->transientState != EXYNOS_OMX_TransStateExecutingToIdle) &&
               (!CHECK_PORT_BEING_FLUSHED(pExynosPort))) {
        Exynos_OSAL_SemaphoreWait(pExynosPort->bufferSemID);
        if (inputUseBuffer->dataValid != OMX_TRUE) {
            message = (EXYNOS_OMX_MESSAGE *)Exynos_OSAL_Dequeue(&pExynosPort->bufferQ);
            if (message == NULL) {
                ret = OMX_ErrorUndefined;
                goto EXIT;
            }
            if (message->messageType == EXYNOS_OMX_CommandFakeBuffer) {
                Exynos_OSAL_Free(message);
                ret = OMX_ErrorCodecFlush;
                goto EXIT;
            }

            inputUseBuffer->bufferHeader  = (OMX_BUFFERHEADERTYPE *)(message->pCmdData);
            inputUseBuffer->allocSize     = inputUseBuffer->bufferHeader->nAllocLen;
            inputUseBuffer->dataLen       = inputUseBuffer->bufferHeader->nFilledLen;
            inputUseBuffer->remainDataLen = inputUseBuffer->dataLen;
            inputUseBuffer->usedDataLen   = 0;
            inputUseBuffer->dataValid     = OMX_TRUE;
            inputUseBuffer->nFlags        = inputUseBuffer->bufferHeader->nFlags;
            inputUseBuffer->timeStamp     = inputUseBuffer->bufferHeader->nTimeStamp;

            Exynos_OSAL_Free(message);

            if (inputUseBuffer->allocSize <= inputUseBuffer->dataLen)
                Exynos_OSAL_Log(EXYNOS_LOG_WARNING, "Input Buffer Full, Check input buffer size! allocSize:%d, dataLen:%d", inputUseBuffer->allocSize, inputUseBuffer->dataLen);
        }
        ret = OMX_ErrorNone;
    }
EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OutputBufferReturn(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATABUFFER *dataBuffer)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_BASEPORT      *exynosOMXOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    OMX_BUFFERHEADERTYPE     *bufferHeader = NULL;

    FunctionIn();

    bufferHeader = dataBuffer->bufferHeader;

    if (bufferHeader != NULL) {
        bufferHeader->nFilledLen = dataBuffer->remainDataLen;
        bufferHeader->nOffset    = 0;
        bufferHeader->nFlags     = dataBuffer->nFlags;
        bufferHeader->nTimeStamp = dataBuffer->timeStamp;

        if ((exynosOMXOutputPort->bStoreMetaData == OMX_TRUE) && (bufferHeader->nFilledLen > 0))
            bufferHeader->nFilledLen = bufferHeader->nAllocLen;

        if (pExynosComponent->propagateMarkType.hMarkTargetComponent != NULL) {
            bufferHeader->hMarkTargetComponent = pExynosComponent->propagateMarkType.hMarkTargetComponent;
            bufferHeader->pMarkData = pExynosComponent->propagateMarkType.pMarkData;
            pExynosComponent->propagateMarkType.hMarkTargetComponent = NULL;
            pExynosComponent->propagateMarkType.pMarkData = NULL;
        }

        if ((bufferHeader->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) {
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE,"event OMX_BUFFERFLAG_EOS!!!");
            pExynosComponent->pCallbacks->EventHandler(pOMXComponent,
                            pExynosComponent->callbackData,
                            OMX_EventBufferFlag,
                            OUTPUT_PORT_INDEX,
                            bufferHeader->nFlags, NULL);
        }

        Exynos_OMX_OutputBufferReturn(pOMXComponent, bufferHeader);
    }

    /* reset dataBuffer */
    Exynos_ResetDataBuffer(dataBuffer);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OutputBufferGetQueue(EXYNOS_OMX_BASECOMPONENT *pExynosComponent)
{
    OMX_ERRORTYPE       ret = OMX_ErrorUndefined;
    EXYNOS_OMX_BASEPORT   *pExynosPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    EXYNOS_OMX_MESSAGE    *message = NULL;
    EXYNOS_OMX_DATABUFFER *outputUseBuffer = NULL;

    FunctionIn();

    if ((pExynosPort->bufferProcessType & BUFFER_COPY) == BUFFER_COPY) {
        outputUseBuffer = &(pExynosPort->way.port2WayDataBuffer.outputDataBuffer);
    } else if (pExynosPort->bufferProcessType == BUFFER_SHARE) {
        outputUseBuffer = &(pExynosPort->way.port2WayDataBuffer.inputDataBuffer);
    }

    if (pExynosComponent->currentState != OMX_StateExecuting) {
        ret = OMX_ErrorUndefined;
        goto EXIT;
    } else if ((pExynosComponent->transientState != EXYNOS_OMX_TransStateExecutingToIdle) &&
               (!CHECK_PORT_BEING_FLUSHED(pExynosPort))){
        Exynos_OSAL_SemaphoreWait(pExynosPort->bufferSemID);
        if (outputUseBuffer->dataValid != OMX_TRUE) {
            message = (EXYNOS_OMX_MESSAGE *)Exynos_OSAL_Dequeue(&pExynosPort->bufferQ);
            if (message == NULL) {
                ret = OMX_ErrorUndefined;
                goto EXIT;
            }
            if (message->messageType == EXYNOS_OMX_CommandFakeBuffer) {
                Exynos_OSAL_Free(message);
                ret = OMX_ErrorCodecFlush;
                goto EXIT;
            }

            outputUseBuffer->bufferHeader  = (OMX_BUFFERHEADERTYPE *)(message->pCmdData);
            outputUseBuffer->allocSize     = outputUseBuffer->bufferHeader->nAllocLen;
            outputUseBuffer->dataLen       = 0; //dataBuffer->bufferHeader->nFilledLen;
            outputUseBuffer->remainDataLen = outputUseBuffer->dataLen;
            outputUseBuffer->usedDataLen   = 0; //dataBuffer->bufferHeader->nOffset;
            outputUseBuffer->dataValid     = OMX_TRUE;
            /* dataBuffer->nFlags             = dataBuffer->bufferHeader->nFlags; */
            /* dataBuffer->nTimeStamp         = dataBuffer->bufferHeader->nTimeStamp; */
/*
            if (pExynosPort->bufferProcessType == BUFFER_SHARE)
                outputUseBuffer->pPrivate      = outputUseBuffer->bufferHeader->pOutputPortPrivate;
            else if ((pExynosPort->bufferProcessType & BUFFER_COPY) == BUFFER_COPY) {
                pExynosPort->processData.dataBuffer = outputUseBuffer->bufferHeader->pBuffer;
                pExynosPort->processData.allocSize  = outputUseBuffer->bufferHeader->nAllocLen;
            }
*/

            Exynos_OSAL_Free(message);
        }
        ret = OMX_ErrorNone;
    }
EXIT:
    FunctionOut();

    return ret;

}

OMX_BUFFERHEADERTYPE *Exynos_OutputBufferGetQueue_Direct(EXYNOS_OMX_BASECOMPONENT *pExynosComponent)
{
    OMX_BUFFERHEADERTYPE  *retBuffer = NULL;
    EXYNOS_OMX_BASEPORT   *pExynosPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    EXYNOS_OMX_MESSAGE    *message = NULL;

    FunctionIn();

    if (pExynosComponent->currentState != OMX_StateExecuting) {
        retBuffer = NULL;
        goto EXIT;
    } else if ((pExynosComponent->transientState != EXYNOS_OMX_TransStateExecutingToIdle) &&
               (!CHECK_PORT_BEING_FLUSHED(pExynosPort))){
        Exynos_OSAL_SemaphoreWait(pExynosPort->bufferSemID);

        message = (EXYNOS_OMX_MESSAGE *)Exynos_OSAL_Dequeue(&pExynosPort->bufferQ);
        if (message == NULL) {
            retBuffer = NULL;
            goto EXIT;
        }
        if (message->messageType == EXYNOS_OMX_CommandFakeBuffer) {
            Exynos_OSAL_Free(message);
            retBuffer = NULL;
            goto EXIT;
        }

        retBuffer  = (OMX_BUFFERHEADERTYPE *)(message->pCmdData);
        Exynos_OSAL_Free(message);
    }

EXIT:
    FunctionOut();

    return retBuffer;
}

OMX_ERRORTYPE Exynos_CodecBufferEnQueue(EXYNOS_OMX_BASECOMPONENT *pExynosComponent, OMX_U32 PortIndex, OMX_PTR data)
{
    OMX_ERRORTYPE       ret = OMX_ErrorNone;
    EXYNOS_OMX_BASEPORT   *pExynosPort = NULL;

    FunctionIn();

    pExynosPort= &pExynosComponent->pExynosPort[PortIndex];

    if (data == NULL) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    ret = Exynos_OSAL_Queue(&pExynosPort->codecBufferQ, (void *)data);
    if (ret != 0) {
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }
    Exynos_OSAL_SemaphorePost(pExynosPort->codecSemID);

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_CodecBufferDeQueue(EXYNOS_OMX_BASECOMPONENT *pExynosComponent, OMX_U32 PortIndex, OMX_PTR *data)
{
    OMX_ERRORTYPE       ret = OMX_ErrorNone;
    EXYNOS_OMX_BASEPORT   *pExynosPort = NULL;
    OMX_U32 tempData;

    FunctionIn();

    pExynosPort = &pExynosComponent->pExynosPort[PortIndex];
    Exynos_OSAL_SemaphoreWait(pExynosPort->codecSemID);
    tempData = (OMX_U32)Exynos_OSAL_Dequeue(&pExynosPort->codecBufferQ);
    if (tempData == NULL) {
        *data = NULL;
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }
    *data = (OMX_PTR)tempData;

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_CodecBufferReset(EXYNOS_OMX_BASECOMPONENT *pExynosComponent, OMX_U32 PortIndex)
{
    OMX_ERRORTYPE       ret = OMX_ErrorNone;
    EXYNOS_OMX_BASEPORT   *pExynosPort = NULL;

    FunctionIn();

    pExynosPort= &pExynosComponent->pExynosPort[PortIndex];

    ret = Exynos_OSAL_ResetQueue(&pExynosPort->codecBufferQ);
    if (ret != 0) {
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }
    while (1) {
        int cnt = 0;
        Exynos_OSAL_Get_SemaphoreCount(pExynosPort->codecSemID, &cnt);
        if (cnt > 0)
            Exynos_OSAL_SemaphoreWait(pExynosPort->codecSemID);
        else
            break;
    }
    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_VideoDecodeGetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nParamIndex,
    OMX_INOUT OMX_PTR     ComponentParameterStructure)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;
    EXYNOS_OMX_BASEPORT      *pExynosPort = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    if (pExynosComponent->currentState == OMX_StateInvalid ) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    if (ComponentParameterStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    switch (nParamIndex) {
    case OMX_IndexParamVideoInit:
    {
        OMX_PORT_PARAM_TYPE *portParam = (OMX_PORT_PARAM_TYPE *)ComponentParameterStructure;
        ret = Exynos_OMX_Check_SizeVersion(portParam, sizeof(OMX_PORT_PARAM_TYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        portParam->nPorts           = pExynosComponent->portParam.nPorts;
        portParam->nStartPortNumber = pExynosComponent->portParam.nStartPortNumber;
        ret = OMX_ErrorNone;
    }
        break;
    case OMX_IndexParamVideoPortFormat:
    {
        OMX_VIDEO_PARAM_PORTFORMATTYPE *portFormat = (OMX_VIDEO_PARAM_PORTFORMATTYPE *)ComponentParameterStructure;
        OMX_U32                         portIndex = portFormat->nPortIndex;
        OMX_U32                         index    = portFormat->nIndex;
        EXYNOS_OMX_BASEPORT               *pExynosPort = NULL;
        OMX_PARAM_PORTDEFINITIONTYPE   *portDefinition = NULL;
        OMX_U32                         supportFormatNum = 0; /* supportFormatNum = N-1 */

        ret = Exynos_OMX_Check_SizeVersion(portFormat, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if ((portIndex >= pExynosComponent->portParam.nPorts)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }


        if (portIndex == INPUT_PORT_INDEX) {
            supportFormatNum = INPUT_PORT_SUPPORTFORMAT_NUM_MAX - 1;
            if (index > supportFormatNum) {
                ret = OMX_ErrorNoMore;
                goto EXIT;
            }

            pExynosPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
            portDefinition = &pExynosPort->portDefinition;

            portFormat->eCompressionFormat = portDefinition->format.video.eCompressionFormat;
            portFormat->eColorFormat       = portDefinition->format.video.eColorFormat;
            portFormat->xFramerate           = portDefinition->format.video.xFramerate;
        } else if (portIndex == OUTPUT_PORT_INDEX) {
            pExynosPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
            portDefinition = &pExynosPort->portDefinition;

            if ((pExynosPort->bIsANBEnabled == OMX_FALSE) &&
                (pExynosPort->bStoreMetaData == OMX_FALSE)) {
                switch (index) {
                case supportFormat_0:
                    portFormat->eCompressionFormat = OMX_VIDEO_CodingUnused;
                    portFormat->eColorFormat       = OMX_COLOR_FormatYUV420Planar;
                    portFormat->xFramerate         = portDefinition->format.video.xFramerate;
                    break;
                case supportFormat_1:
                    portFormat->eCompressionFormat = OMX_VIDEO_CodingUnused;
                    portFormat->eColorFormat       = OMX_COLOR_FormatYUV420SemiPlanar;
                    portFormat->xFramerate         = portDefinition->format.video.xFramerate;
                    break;
                case supportFormat_2:
                    portFormat->eCompressionFormat = OMX_VIDEO_CodingUnused;
                    portFormat->eColorFormat       = OMX_SEC_COLOR_FormatNV12TPhysicalAddress;
                    portFormat->xFramerate         = portDefinition->format.video.xFramerate;
                    break;
                case supportFormat_3:
                    portFormat->eCompressionFormat = OMX_VIDEO_CodingUnused;
                    portFormat->eColorFormat       = OMX_SEC_COLOR_FormatNV12Tiled;
                    portFormat->xFramerate         = portDefinition->format.video.xFramerate;
                    break;
                default:
                    if (index > supportFormat_0) {
                        ret = OMX_ErrorNoMore;
                        goto EXIT;
                    }
                    break;
                }
            } else {
                switch (index) {
                case supportFormat_0:
                    portFormat->eCompressionFormat = OMX_VIDEO_CodingUnused;
                    portFormat->eColorFormat       = OMX_SEC_COLOR_FormatNV12Tiled;
                    portFormat->xFramerate         = portDefinition->format.video.xFramerate;
                    break;
                default:
                    if (index > supportFormat_0) {
                        ret = OMX_ErrorNoMore;
                        goto EXIT;
                    }
                    break;
                }
            }
        }
        ret = OMX_ErrorNone;
    }
        break;
#ifdef USE_ANB
    case OMX_IndexParamGetAndroidNativeBuffer:
    {
        ret = Exynos_OSAL_GetANBParameter(hComponent, nParamIndex, ComponentParameterStructure);
    }
        break;
    case OMX_IndexParamPortDefinition:
    {
        OMX_PARAM_PORTDEFINITIONTYPE *portDefinition = (OMX_PARAM_PORTDEFINITIONTYPE *)ComponentParameterStructure;
        OMX_U32                       portIndex = portDefinition->nPortIndex;
        EXYNOS_OMX_BASEPORT             *pExynosPort;

        ret = Exynos_OMX_GetParameter(hComponent, nParamIndex, ComponentParameterStructure);
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        /* at this point, GetParameter has done all the verification, we
         * just dereference things directly here
         */
        pExynosPort = &pExynosComponent->pExynosPort[portIndex];
        if ((pExynosPort->bIsANBEnabled == OMX_TRUE) ||
            (pExynosPort->bStoreMetaData == OMX_TRUE)){
            portDefinition->format.video.eColorFormat =
                (OMX_COLOR_FORMATTYPE)Exynos_OSAL_OMX2HalPixelFormat(portDefinition->format.video.eColorFormat);
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "portDefinition->format.video.eColorFormat:0x%x", portDefinition->format.video.eColorFormat);
        }

        // WORKAROUND provided by Google engineer
        if (portIndex == OUTPUT_PORT_INDEX &&
                (pExynosPort->bufferProcessType & BUFFER_COPY) == BUFFER_COPY) {
            // Decoder actually uses width and height as stride and slice height for buffer copy.
            // Changing only at getParam as the internal value may be used elsewhere
            portDefinition->format.video.nStride = portDefinition->format.video.nFrameWidth;
            portDefinition->format.video.nSliceHeight = portDefinition->format.video.nFrameHeight;
        }
    }
        break;
#endif
    default:
    {
        ret = Exynos_OMX_GetParameter(hComponent, nParamIndex, ComponentParameterStructure);
    }
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}
OMX_ERRORTYPE Exynos_OMX_VideoDecodeSetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        ComponentParameterStructure)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;
    EXYNOS_OMX_BASEPORT      *pExynosPort = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    if (pExynosComponent->currentState == OMX_StateInvalid ) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    if (ComponentParameterStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    switch (nIndex) {
    case OMX_IndexParamVideoPortFormat:
    {
        OMX_VIDEO_PARAM_PORTFORMATTYPE *portFormat = (OMX_VIDEO_PARAM_PORTFORMATTYPE *)ComponentParameterStructure;
        OMX_U32                         portIndex = portFormat->nPortIndex;
        OMX_U32                         index    = portFormat->nIndex;
        EXYNOS_OMX_BASEPORT               *pExynosPort = NULL;
        OMX_PARAM_PORTDEFINITIONTYPE   *portDefinition = NULL;
        OMX_U32                         supportFormatNum = 0;

        ret = Exynos_OMX_Check_SizeVersion(portFormat, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if ((portIndex >= pExynosComponent->portParam.nPorts)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        } else {
            pExynosPort = &pExynosComponent->pExynosPort[portIndex];
            portDefinition = &pExynosPort->portDefinition;

            portDefinition->format.video.eColorFormat       = portFormat->eColorFormat;
            portDefinition->format.video.eCompressionFormat = portFormat->eCompressionFormat;
            portDefinition->format.video.xFramerate         = portFormat->xFramerate;

            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "portIndex:%d, portFormat->eColorFormat:0x%x", portIndex, portFormat->eColorFormat);
        }
    }
        break;
    case OMX_IndexParamPortDefinition:
    {
        OMX_PARAM_PORTDEFINITIONTYPE  *pPortDefinition = (OMX_PARAM_PORTDEFINITIONTYPE *)ComponentParameterStructure;
        EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
        OMX_U32                        portIndex = pPortDefinition->nPortIndex;
        EXYNOS_OMX_BASEPORT           *pExynosPort;
        OMX_U32 width, height, size;
        OMX_U32 realWidth, realHeight;
        OMX_U32 originalWidth, originalHeight;

        if (portIndex >= pExynosComponent->portParam.nPorts) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }
        ret = Exynos_OMX_Check_SizeVersion(pPortDefinition, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        pExynosPort = &pExynosComponent->pExynosPort[portIndex];

        if ((pExynosComponent->currentState != OMX_StateLoaded) && (pExynosComponent->currentState != OMX_StateWaitForResources)) {
            if (pExynosPort->portDefinition.bEnabled == OMX_TRUE) {
                ret = OMX_ErrorIncorrectStateOperation;
                goto EXIT;
            }
        }
        if ((pPortDefinition->nBufferCountActual < pExynosPort->portDefinition.nBufferCountMin) ||
            ((pVideoDec->bDRMPlayerMode == OMX_TRUE) &&
             (pPortDefinition->nBufferCountActual > (pExynosPort->portDefinition.nBufferCountMin + MAX_DISPLAY_EXTRA_BUFFER)))) {
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }

        originalWidth = pExynosPort->portDefinition.format.video.nFrameWidth;
        originalHeight = pExynosPort->portDefinition.format.video.nFrameHeight;
        Exynos_OSAL_Memcpy(&pExynosPort->portDefinition, pPortDefinition, pPortDefinition->nSize);

#ifdef USE_ANB // Modified by Google engineer
        /* should not affect the format since in ANB case, the caller
                * is providing us a HAL format */
        if ((pExynosPort->bIsANBEnabled == OMX_TRUE) ||
            (pExynosPort->bStoreMetaData == OMX_TRUE)) {
            pExynosPort->portDefinition.format.video.eColorFormat =
                Exynos_OSAL_Hal2OMXPixelFormat(pExynosPort->portDefinition.format.video.eColorFormat);
        }
#endif

        realWidth = pExynosPort->portDefinition.format.video.nFrameWidth;
        realHeight = pExynosPort->portDefinition.format.video.nFrameHeight;
        width = ((realWidth + 15) & (~15));
        height = ((realHeight + 15) & (~15));
        size = (width * height * 3) / 2;
        pExynosPort->portDefinition.format.video.nStride = width;
        pExynosPort->portDefinition.format.video.nSliceHeight = height;
        pExynosPort->portDefinition.nBufferSize = (size > pExynosPort->portDefinition.nBufferSize) ? size : pExynosPort->portDefinition.nBufferSize;

        if (realWidth != originalWidth || realHeight != originalHeight) {
            pExynosPort->cropRectangle.nTop = 0;
            pExynosPort->cropRectangle.nLeft = 0;
            pExynosPort->cropRectangle.nWidth = realWidth;
            pExynosPort->cropRectangle.nHeight = realHeight;
        }

        if (portIndex == INPUT_PORT_INDEX) {
            EXYNOS_OMX_BASEPORT *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
            pExynosOutputPort->portDefinition.format.video.nFrameWidth = pExynosPort->portDefinition.format.video.nFrameWidth;
            pExynosOutputPort->portDefinition.format.video.nFrameHeight = pExynosPort->portDefinition.format.video.nFrameHeight;
            pExynosOutputPort->portDefinition.format.video.nStride = width;
            pExynosOutputPort->portDefinition.format.video.nSliceHeight = height;
            // if resetting the output port size, also reset the crop size for the output port;
            // otherwise, output crop will not be updated when setting up the output port because
            // the output size would not have changed after we updated it here
            if (realWidth != originalWidth || realHeight != originalHeight) {
                pExynosOutputPort->cropRectangle.nTop = 0;
                pExynosOutputPort->cropRectangle.nLeft = 0;
                pExynosOutputPort->cropRectangle.nWidth = realWidth;
                pExynosOutputPort->cropRectangle.nHeight = realHeight;
            }

            switch (pExynosOutputPort->portDefinition.format.video.eColorFormat) {
            case OMX_COLOR_FormatYUV420Planar:
            case OMX_COLOR_FormatYUV420SemiPlanar:
            case OMX_SEC_COLOR_FormatNV12TPhysicalAddress:
                pExynosOutputPort->portDefinition.nBufferSize = (width * height * 3) / 2;
                break;
            case OMX_SEC_COLOR_FormatNV12Tiled:
                pExynosOutputPort->portDefinition.nBufferSize =
                    calc_plane(pExynosPort->portDefinition.format.video.nFrameWidth, pExynosOutputPort->portDefinition.format.video.nFrameHeight) +
                    calc_plane(pExynosPort->portDefinition.format.video.nFrameWidth, pExynosOutputPort->portDefinition.format.video.nFrameHeight >> 1);
                break;
            default:
                Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Color format is not support!! use default YUV size!!");
                ret = OMX_ErrorUnsupportedSetting;
                break;
            }

            if (pExynosOutputPort->bufferProcessType == BUFFER_SHARE) {
                pExynosOutputPort->portDefinition.nBufferSize =
                    calc_plane(pExynosPort->portDefinition.format.video.nFrameWidth, pExynosOutputPort->portDefinition.format.video.nFrameHeight) +
                    calc_plane(pExynosPort->portDefinition.format.video.nFrameWidth, pExynosOutputPort->portDefinition.format.video.nFrameHeight >> 1);
            }
        }
    }
        break;
#ifdef USE_ANB
    case OMX_IndexParamEnableAndroidBuffers:
    case OMX_IndexParamUseAndroidNativeBuffer:
    case OMX_IndexParamStoreMetaDataBuffer:
    {
        ret = Exynos_OSAL_SetANBParameter(hComponent, nIndex, ComponentParameterStructure);
    }
        break;
#endif
    case OMX_IndexParamEnableThumbnailMode:
    {
        EXYNOS_OMX_VIDEO_THUMBNAILMODE *pThumbnailMode = (EXYNOS_OMX_VIDEO_THUMBNAILMODE *)ComponentParameterStructure;
        EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;

        ret = Exynos_OMX_Check_SizeVersion(pThumbnailMode, sizeof(EXYNOS_OMX_VIDEO_THUMBNAILMODE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        pVideoDec->bThumbnailMode = pThumbnailMode->bEnable;
        if (pVideoDec->bThumbnailMode == OMX_TRUE) {
            EXYNOS_OMX_BASEPORT *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
            pExynosOutputPort->portDefinition.nBufferCountMin = 1;
            pExynosOutputPort->portDefinition.nBufferCountActual = 1;
        }

        ret = OMX_ErrorNone;
    }
        break;
    default:
    {
        ret = Exynos_OMX_SetParameter(hComponent, nIndex, ComponentParameterStructure);
    }
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_VideoDecodeGetConfig(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE nIndex,
    OMX_PTR pComponentConfigStructure)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }
    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pComponentConfigStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    switch (nIndex) {
    default:
        ret = Exynos_OMX_GetConfig(hComponent, nIndex, pComponentConfigStructure);
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_VideoDecodeSetConfig(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE nIndex,
    OMX_PTR pComponentConfigStructure)
{
    OMX_ERRORTYPE           ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }
    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pComponentConfigStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    switch (nIndex) {
    default:
        ret = Exynos_OMX_SetConfig(hComponent, nIndex, pComponentConfigStructure);
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_VideoDecodeGetExtensionIndex(
    OMX_IN OMX_HANDLETYPE  hComponent,
    OMX_IN OMX_STRING      cParameterName,
    OMX_OUT OMX_INDEXTYPE *pIndexType)
{
    OMX_ERRORTYPE           ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    if ((cParameterName == NULL) || (pIndexType == NULL)) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

#ifdef USE_ANB
    if (Exynos_OSAL_Strcmp(cParameterName, EXYNOS_INDEX_PARAM_ENABLE_ANB) == 0) {
        *pIndexType = (OMX_INDEXTYPE) OMX_IndexParamEnableAndroidBuffers;
        goto EXIT;
    }
    if (Exynos_OSAL_Strcmp(cParameterName, EXYNOS_INDEX_PARAM_GET_ANB) == 0) {
        *pIndexType = (OMX_INDEXTYPE) OMX_IndexParamGetAndroidNativeBuffer;
        goto EXIT;
    }
    if (Exynos_OSAL_Strcmp(cParameterName, EXYNOS_INDEX_PARAM_USE_ANB) == 0) {
        *pIndexType = (OMX_INDEXTYPE) OMX_IndexParamUseAndroidNativeBuffer;
        goto EXIT;
    }
#endif
    if (Exynos_OSAL_Strcmp(cParameterName, EXYNOS_INDEX_PARAM_ENABLE_THUMBNAIL) == 0) {
        *pIndexType = OMX_IndexParamEnableThumbnailMode;
        goto EXIT;
    }
#ifdef USE_STOREMETADATA
    if (Exynos_OSAL_Strcmp(cParameterName, EXYNOS_INDEX_PARAM_STORE_METADATA_BUFFER) == 0) {
        *pIndexType = (OMX_INDEXTYPE) OMX_IndexParamStoreMetaDataBuffer;
        goto EXIT;
    }
#endif

    ret = Exynos_OMX_GetExtensionIndex(hComponent, cParameterName, pIndexType);

EXIT:
    FunctionOut();

    return ret;
}

#ifdef USE_ANB
OMX_ERRORTYPE Exynos_Shared_ANBBufferToData(EXYNOS_OMX_DATABUFFER *pUseBuffer, EXYNOS_OMX_DATA *pData, EXYNOS_OMX_BASEPORT *pExynosPort, EXYNOS_OMX_PLANE nPlane)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_U32 width, height;
    ExynosVideoPlane planes[MAX_BUFFER_PLANE];

    memset(planes, 0, sizeof(planes));

    width = pExynosPort->portDefinition.format.video.nFrameWidth;
    height = pExynosPort->portDefinition.format.video.nFrameHeight;

    if ((pExynosPort->bIsANBEnabled == OMX_TRUE) ||
        (pExynosPort->bStoreMetaData == OMX_TRUE)) {
        OMX_U32 stride;
        if ((pUseBuffer->bufferHeader != NULL) && (pUseBuffer->bufferHeader->pBuffer != NULL)) {
            if (pExynosPort->bIsANBEnabled == OMX_TRUE) {
                Exynos_OSAL_LockANBHandle(pUseBuffer->bufferHeader->pBuffer, width, height, pExynosPort->portDefinition.format.video.eColorFormat, &stride, planes);
            } else if (pExynosPort->bStoreMetaData == OMX_TRUE) {
                Exynos_OSAL_LockMetaData(pUseBuffer->bufferHeader->pBuffer, width, height, pExynosPort->portDefinition.format.video.eColorFormat, &stride, planes);
            }
            pUseBuffer->dataLen = sizeof(void *);
        } else {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s : %d", __FUNCTION__, __LINE__);
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }
    } else {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s : %d", __FUNCTION__, __LINE__);
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    if (nPlane == TWO_PLANE) {
        /* Case of Shared Buffer, Only support two PlaneBuffer */
        pData->buffer.multiPlaneBuffer.dataBuffer[0] = planes[0].addr;
        pData->buffer.multiPlaneBuffer.fd[0] = planes[0].fd;
        pData->buffer.multiPlaneBuffer.dataBuffer[1] = planes[1].addr;
        pData->buffer.multiPlaneBuffer.fd[1] = planes[1].fd;
    } else {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Can not support plane");
        ret = OMX_ErrorNotImplemented;
        goto EXIT;
    }

    pData->allocSize     = pUseBuffer->allocSize;
    pData->dataLen       = pUseBuffer->dataLen;
    pData->usedDataLen   = pUseBuffer->usedDataLen;
    pData->remainDataLen = pUseBuffer->remainDataLen;
    pData->timeStamp     = pUseBuffer->timeStamp;
    pData->nFlags        = pUseBuffer->nFlags;
    pData->pPrivate      = pUseBuffer->pPrivate;
    pData->bufferHeader  = pUseBuffer->bufferHeader;

EXIT:
    return ret;
}

OMX_ERRORTYPE Exynos_Shared_DataToANBBuffer(EXYNOS_OMX_DATA *pData, EXYNOS_OMX_DATABUFFER *pUseBuffer, EXYNOS_OMX_BASEPORT *pExynosPort)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    pUseBuffer->bufferHeader          = pData->bufferHeader;
    pUseBuffer->allocSize             = pData->allocSize;
    pUseBuffer->dataLen               = pData->dataLen;
    pUseBuffer->usedDataLen           = pData->usedDataLen;
    pUseBuffer->remainDataLen         = pData->remainDataLen;
    pUseBuffer->timeStamp             = pData->timeStamp;
    pUseBuffer->nFlags                = pData->nFlags;
    pUseBuffer->pPrivate              = pData->pPrivate;

    if ((pUseBuffer->bufferHeader == NULL) ||
        (pUseBuffer->bufferHeader->pBuffer == NULL)) {
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }

    if (pExynosPort->bIsANBEnabled == OMX_TRUE) {
        Exynos_OSAL_UnlockANBHandle(pUseBuffer->bufferHeader->pBuffer);
    } else if (pExynosPort->bStoreMetaData == OMX_TRUE) {
        Exynos_OSAL_UnlockMetaData(pUseBuffer->bufferHeader->pBuffer);
    } else {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s : %d", __FUNCTION__, __LINE__);
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

EXIT:
    return ret;
}
#endif
