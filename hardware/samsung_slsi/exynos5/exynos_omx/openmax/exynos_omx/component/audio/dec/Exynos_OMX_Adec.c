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
 * @file        Exynos_OMX_Adec.c
 * @brief
 * @author      Yunji Kim (yunji.kim@samsung.com)
 *
 * @version     1.1.0
 * @history
 *   2012.02.28 : Create
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Exynos_OMX_Macros.h"
#include "Exynos_OSAL_Event.h"
#include "Exynos_OMX_Adec.h"
#include "Exynos_OMX_Basecomponent.h"
#include "Exynos_OSAL_Thread.h"
#include "Exynos_OSAL_Semaphore.h"
#include "Exynos_OSAL_Mutex.h"
#include "Exynos_OSAL_ETC.h"
#include "srp_api.h"

#undef  EXYNOS_LOG_TAG
#define EXYNOS_LOG_TAG    "EXYNOS_AUDIO_DEC"
#define EXYNOS_LOG_OFF
#include "Exynos_OSAL_Log.h"

OMX_ERRORTYPE Exynos_OMX_UseBuffer(
    OMX_IN OMX_HANDLETYPE            hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE **ppBufferHdr,
    OMX_IN OMX_U32                   nPortIndex,
    OMX_IN OMX_PTR                   pAppPrivate,
    OMX_IN OMX_U32                   nSizeBytes,
    OMX_IN OMX_U8                   *pBuffer)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;
    EXYNOS_OMX_BASEPORT      *pExynosPort = NULL;
    OMX_BUFFERHEADERTYPE     *temp_bufferHeader = NULL;
    OMX_U32                   i = 0;

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
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;
    EXYNOS_OMX_BASEPORT      *pExynosPort = NULL;
    OMX_BUFFERHEADERTYPE     *temp_bufferHeader = NULL;
    OMX_U8                   *temp_buffer = NULL;
    OMX_U32                   i = 0;

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

    temp_buffer = Exynos_OSAL_Malloc(sizeof(OMX_U8) * nSizeBytes);
    if (temp_buffer == NULL) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    temp_bufferHeader = (OMX_BUFFERHEADERTYPE *)Exynos_OSAL_Malloc(sizeof(OMX_BUFFERHEADERTYPE));
    if (temp_bufferHeader == NULL) {
        Exynos_OSAL_Free(temp_buffer);
        temp_buffer = NULL;
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    Exynos_OSAL_Memset(temp_bufferHeader, 0, sizeof(OMX_BUFFERHEADERTYPE));

    for (i = 0; i < pExynosPort->portDefinition.nBufferCountActual; i++) {
        if (pExynosPort->bufferStateAllocate[i] == BUFFER_STATE_FREE) {
            pExynosPort->extendBufferHeader[i].OMXBufferHeader = temp_bufferHeader;
            pExynosPort->bufferStateAllocate[i] = (BUFFER_STATE_ALLOCATED | HEADER_STATE_ALLOCATED);
            INIT_SET_SIZE_VERSION(temp_bufferHeader, OMX_BUFFERHEADERTYPE);
            temp_bufferHeader->pBuffer        = temp_buffer;
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
    Exynos_OSAL_Free(temp_buffer);
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
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;
    EXYNOS_OMX_BASEPORT      *pExynosPort = NULL;
    OMX_BUFFERHEADERTYPE     *temp_bufferHeader = NULL;
    OMX_U8                   *temp_buffer = NULL;
    OMX_U32                   i = 0;

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

    for (i = 0; i < pExynosPort->portDefinition.nBufferCountActual; i++) {
        if (((pExynosPort->bufferStateAllocate[i] | BUFFER_STATE_FREE) != 0) && (pExynosPort->extendBufferHeader[i].OMXBufferHeader != NULL)) {
            if (pExynosPort->extendBufferHeader[i].OMXBufferHeader->pBuffer == pBufferHdr->pBuffer) {
                if (pExynosPort->bufferStateAllocate[i] & BUFFER_STATE_ALLOCATED) {
                    Exynos_OSAL_Free(pExynosPort->extendBufferHeader[i].OMXBufferHeader->pBuffer);
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
    EXYNOS_OMX_BASEPORT          *pExynosPort = NULL;
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

OMX_BOOL Exynos_Check_BufferProcess_State(EXYNOS_OMX_BASECOMPONENT *pExynosComponent)
{
    if ((pExynosComponent->currentState == OMX_StateExecuting) &&
        (pExynosComponent->pExynosPort[INPUT_PORT_INDEX].portState == OMX_StateIdle) &&
        (pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX].portState == OMX_StateIdle) &&
        (pExynosComponent->transientState != EXYNOS_OMX_TransStateExecutingToIdle) &&
        (pExynosComponent->transientState != EXYNOS_OMX_TransStateIdleToExecuting)) {
        return OMX_TRUE;
    } else {
        return OMX_FALSE;
    }
}

OMX_ERRORTYPE Exynos_InputBufferReturn(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_BASEPORT      *exynosOMXInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT      *exynosOMXOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    EXYNOS_OMX_DATABUFFER    *dataBuffer = &exynosOMXInputPort->way.port1WayDataBuffer.dataBuffer;
    OMX_BUFFERHEADERTYPE     *bufferHeader = dataBuffer->bufferHeader;

    FunctionIn();

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
        pExynosComponent->pCallbacks->EmptyBufferDone(pOMXComponent, pExynosComponent->callbackData, bufferHeader);
    }

    if ((pExynosComponent->currentState == OMX_StatePause) &&
        ((!CHECK_PORT_BEING_FLUSHED(exynosOMXInputPort) && !CHECK_PORT_BEING_FLUSHED(exynosOMXOutputPort)))) {
        Exynos_OSAL_SignalWait(pExynosComponent->pauseEvent, DEF_MAX_WAIT_TIME);
        Exynos_OSAL_SignalReset(pExynosComponent->pauseEvent);
    }

    dataBuffer->dataValid     = OMX_FALSE;
    dataBuffer->dataLen       = 0;
    dataBuffer->remainDataLen = 0;
    dataBuffer->usedDataLen   = 0;
    dataBuffer->bufferHeader  = NULL;
    dataBuffer->nFlags        = 0;
    dataBuffer->timeStamp     = 0;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_InputBufferGetQueue(EXYNOS_OMX_BASECOMPONENT *pExynosComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    EXYNOS_OMX_BASEPORT   *pExynosPort = NULL;
    EXYNOS_OMX_DATABUFFER *dataBuffer = NULL;
    EXYNOS_OMX_MESSAGE    *message = NULL;

    FunctionIn();

    pExynosPort= &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    dataBuffer = &pExynosPort->way.port1WayDataBuffer.dataBuffer;

    if (pExynosComponent->currentState != OMX_StateExecuting) {
        ret = OMX_ErrorUndefined;
        goto EXIT;
    } else {
        Exynos_OSAL_SemaphoreWait(pExynosPort->bufferSemID);
        Exynos_OSAL_MutexLock(dataBuffer->bufferMutex);
        if (dataBuffer->dataValid != OMX_TRUE) {
            message = (EXYNOS_OMX_MESSAGE *)Exynos_OSAL_Dequeue(&pExynosPort->bufferQ);
            if (message == NULL) {
                ret = OMX_ErrorUndefined;
                Exynos_OSAL_MutexUnlock(dataBuffer->bufferMutex);
                goto EXIT;
            }

            dataBuffer->bufferHeader = (OMX_BUFFERHEADERTYPE *)(message->pCmdData);
            dataBuffer->allocSize = dataBuffer->bufferHeader->nAllocLen;
            dataBuffer->dataLen = dataBuffer->bufferHeader->nFilledLen;
            dataBuffer->remainDataLen = dataBuffer->dataLen;
            dataBuffer->usedDataLen = 0;
            dataBuffer->dataValid = OMX_TRUE;
            dataBuffer->nFlags = dataBuffer->bufferHeader->nFlags;
            dataBuffer->timeStamp = dataBuffer->bufferHeader->nTimeStamp;

            Exynos_OSAL_Free(message);

            if (dataBuffer->allocSize <= dataBuffer->dataLen)
                Exynos_OSAL_Log(EXYNOS_LOG_WARNING, "Input Buffer Full, Check input buffer size! allocSize:%d, dataLen:%d", dataBuffer->allocSize, dataBuffer->dataLen);
        }
        Exynos_OSAL_MutexUnlock(dataBuffer->bufferMutex);
        ret = OMX_ErrorNone;
    }
EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OutputBufferReturn(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_BASEPORT      *exynosOMXInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT      *exynosOMXOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    EXYNOS_OMX_DATABUFFER    *dataBuffer = &exynosOMXOutputPort->way.port1WayDataBuffer.dataBuffer;
    OMX_BUFFERHEADERTYPE     *bufferHeader = dataBuffer->bufferHeader;

    FunctionIn();

    if (bufferHeader != NULL) {
        bufferHeader->nFilledLen = dataBuffer->remainDataLen;
        bufferHeader->nOffset    = 0;
        bufferHeader->nFlags     = dataBuffer->nFlags;
        bufferHeader->nTimeStamp = dataBuffer->timeStamp;

        if (pExynosComponent->propagateMarkType.hMarkTargetComponent != NULL) {
            bufferHeader->hMarkTargetComponent = pExynosComponent->propagateMarkType.hMarkTargetComponent;
            bufferHeader->pMarkData = pExynosComponent->propagateMarkType.pMarkData;
            pExynosComponent->propagateMarkType.hMarkTargetComponent = NULL;
            pExynosComponent->propagateMarkType.pMarkData = NULL;
        }

        if (bufferHeader->nFlags & OMX_BUFFERFLAG_EOS) {
            pExynosComponent->pCallbacks->EventHandler(pOMXComponent,
                            pExynosComponent->callbackData,
                            OMX_EventBufferFlag,
                            OUTPUT_PORT_INDEX,
                            bufferHeader->nFlags, NULL);
        }

        pExynosComponent->pCallbacks->FillBufferDone(pOMXComponent, pExynosComponent->callbackData, bufferHeader);
    }

    if ((pExynosComponent->currentState == OMX_StatePause) &&
        ((!CHECK_PORT_BEING_FLUSHED(exynosOMXInputPort) && !CHECK_PORT_BEING_FLUSHED(exynosOMXOutputPort)))) {
        Exynos_OSAL_SignalWait(pExynosComponent->pauseEvent, DEF_MAX_WAIT_TIME);
        Exynos_OSAL_SignalReset(pExynosComponent->pauseEvent);
    }

    /* reset dataBuffer */
    dataBuffer->dataValid     = OMX_FALSE;
    dataBuffer->dataLen       = 0;
    dataBuffer->remainDataLen = 0;
    dataBuffer->usedDataLen   = 0;
    dataBuffer->bufferHeader  = NULL;
    dataBuffer->nFlags        = 0;
    dataBuffer->timeStamp     = 0;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OutputBufferGetQueue(EXYNOS_OMX_BASECOMPONENT *pExynosComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    EXYNOS_OMX_BASEPORT   *pExynosPort = NULL;
    EXYNOS_OMX_DATABUFFER *dataBuffer = NULL;
    EXYNOS_OMX_MESSAGE    *message = NULL;

    FunctionIn();

    pExynosPort= &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    dataBuffer = &pExynosPort->way.port1WayDataBuffer.dataBuffer;

    if (pExynosComponent->currentState != OMX_StateExecuting) {
        ret = OMX_ErrorUndefined;
        goto EXIT;
    } else {
        Exynos_OSAL_SemaphoreWait(pExynosPort->bufferSemID);
        Exynos_OSAL_MutexLock(dataBuffer->bufferMutex);
        if (dataBuffer->dataValid != OMX_TRUE) {
            message = (EXYNOS_OMX_MESSAGE *)Exynos_OSAL_Dequeue(&pExynosPort->bufferQ);
            if (message == NULL) {
                ret = OMX_ErrorUndefined;
                Exynos_OSAL_MutexUnlock(dataBuffer->bufferMutex);
                goto EXIT;
            }

            dataBuffer->bufferHeader = (OMX_BUFFERHEADERTYPE *)(message->pCmdData);
            dataBuffer->allocSize = dataBuffer->bufferHeader->nAllocLen;
            dataBuffer->dataLen = 0; //dataBuffer->bufferHeader->nFilledLen;
            dataBuffer->remainDataLen = dataBuffer->dataLen;
            dataBuffer->usedDataLen = 0; //dataBuffer->bufferHeader->nOffset;
            dataBuffer->dataValid = OMX_TRUE;
            /* dataBuffer->nFlags = dataBuffer->bufferHeader->nFlags; */
            /* dataBuffer->nTimeStamp = dataBuffer->bufferHeader->nTimeStamp; */
            pExynosPort->processData.buffer.singlePlaneBuffer.dataBuffer = dataBuffer->bufferHeader->pBuffer;
            pExynosPort->processData.allocSize = dataBuffer->bufferHeader->nAllocLen;

            Exynos_OSAL_Free(message);
        }
        Exynos_OSAL_MutexUnlock(dataBuffer->bufferMutex);
        ret = OMX_ErrorNone;
    }
EXIT:
    FunctionOut();

    return ret;

}

OMX_BOOL Exynos_Preprocessor_InputData(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_BOOL                  ret = OMX_FALSE;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_BASEPORT      *exynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_DATABUFFER    *inputUseBuffer = &exynosInputPort->way.port1WayDataBuffer.dataBuffer;
    EXYNOS_OMX_DATA          *inputData = &exynosInputPort->processData;
    OMX_U32                   copySize = 0;
    OMX_BYTE                  checkInputStream = NULL;
    OMX_U32                   checkInputStreamLen = 0;
    OMX_U32                   checkedSize = 0;
    OMX_BOOL                  flagEOF = OMX_FALSE;
    OMX_BOOL                  previousFrameEOF = OMX_FALSE;

    FunctionIn();

    if (inputUseBuffer->dataValid == OMX_TRUE) {
        checkInputStream = inputUseBuffer->bufferHeader->pBuffer + inputUseBuffer->usedDataLen;
        checkInputStreamLen = inputUseBuffer->remainDataLen;

        if (inputData->dataLen == 0) {
            previousFrameEOF = OMX_TRUE;
        } else {
            previousFrameEOF = OMX_FALSE;
        }

        /* Audio extractor should parse into frame units. */
        flagEOF = OMX_TRUE;
        checkedSize = checkInputStreamLen;
        copySize = checkedSize;

        if (inputUseBuffer->nFlags & OMX_BUFFERFLAG_EOS)
            pExynosComponent->bSaveFlagEOS = OMX_TRUE;

        if (((inputData->allocSize) - (inputData->dataLen)) >= copySize) {
            if (copySize > 0)
                Exynos_OSAL_Memcpy(inputData->buffer.singlePlaneBuffer.dataBuffer + inputData->dataLen, checkInputStream, copySize);

            inputUseBuffer->dataLen -= copySize;
            inputUseBuffer->remainDataLen -= copySize;
            inputUseBuffer->usedDataLen += copySize;

            inputData->dataLen += copySize;
            inputData->remainDataLen += copySize;

            if (previousFrameEOF == OMX_TRUE) {
                inputData->timeStamp = inputUseBuffer->timeStamp;
                inputData->nFlags = inputUseBuffer->nFlags;
            }

            if (pExynosComponent->bUseFlagEOF == OMX_TRUE) {
                if (pExynosComponent->bSaveFlagEOS == OMX_TRUE) {
                    inputData->nFlags |= OMX_BUFFERFLAG_EOS;
                    flagEOF = OMX_TRUE;
                    pExynosComponent->bSaveFlagEOS = OMX_FALSE;
                } else {
                    inputData->nFlags = (inputData->nFlags & (~OMX_BUFFERFLAG_EOS));
                }
            } else {
                if ((checkedSize == checkInputStreamLen) && (pExynosComponent->bSaveFlagEOS == OMX_TRUE)) {
                    if ((inputUseBuffer->nFlags & OMX_BUFFERFLAG_EOS) &&
                        ((inputData->nFlags & OMX_BUFFERFLAG_CODECCONFIG) ||
                        (inputData->dataLen == 0))) {
                        inputData->nFlags |= OMX_BUFFERFLAG_EOS;
                        flagEOF = OMX_TRUE;
                        pExynosComponent->bSaveFlagEOS = OMX_FALSE;
                    } else if ((inputUseBuffer->nFlags & OMX_BUFFERFLAG_EOS) &&
                               (!(inputData->nFlags & OMX_BUFFERFLAG_CODECCONFIG)) &&
                               (inputData->dataLen != 0)) {
                        inputData->nFlags = (inputData->nFlags & (~OMX_BUFFERFLAG_EOS));
                        flagEOF = OMX_TRUE;
                        pExynosComponent->bSaveFlagEOS = OMX_TRUE;
                    }
                } else {
                    inputData->nFlags = (inputUseBuffer->nFlags & (~OMX_BUFFERFLAG_EOS));
                }
            }
        } else {
            /*????????????????????????????????? Error ?????????????????????????????????*/
            Exynos_ResetCodecData(inputData);
            flagEOF = OMX_FALSE;
        }

        if ((inputUseBuffer->remainDataLen == 0) ||
            (CHECK_PORT_BEING_FLUSHED(exynosInputPort)))
            Exynos_InputBufferReturn(pOMXComponent);
        else
            inputUseBuffer->dataValid = OMX_TRUE;
    }

    if (flagEOF == OMX_TRUE) {
        if (pExynosComponent->checkTimeStamp.needSetStartTimeStamp == OMX_TRUE) {
            /* Flush SRP buffers */
            SRP_Flush();

            pExynosComponent->checkTimeStamp.needCheckStartTimeStamp = OMX_TRUE;
            pExynosComponent->checkTimeStamp.startTimeStamp = inputData->timeStamp;
            pExynosComponent->checkTimeStamp.nStartFlags = inputData->nFlags;
            pExynosComponent->checkTimeStamp.needSetStartTimeStamp = OMX_FALSE;
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "first frame timestamp after seeking %lld us (%.2f secs)",
                inputData->timeStamp, inputData->timeStamp / 1E6);
        }

        ret = OMX_TRUE;
    } else {
        ret = OMX_FALSE;
    }

    FunctionOut();

    return ret;
}

OMX_BOOL Exynos_Postprocess_OutputData(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_BOOL                  ret = OMX_FALSE;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_BASEPORT      *exynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    EXYNOS_OMX_DATABUFFER    *outputUseBuffer = &exynosOutputPort->way.port1WayDataBuffer.dataBuffer;
    EXYNOS_OMX_DATA          *outputData = &exynosOutputPort->processData;
    OMX_U32                   copySize = 0;

    FunctionIn();

    if (outputUseBuffer->dataValid == OMX_TRUE) {
        if (pExynosComponent->checkTimeStamp.needCheckStartTimeStamp == OMX_TRUE) {
            if (pExynosComponent->checkTimeStamp.startTimeStamp == outputData->timeStamp) {
                pExynosComponent->checkTimeStamp.startTimeStamp = -19761123;
                pExynosComponent->checkTimeStamp.nStartFlags = 0x0;
                pExynosComponent->checkTimeStamp.needSetStartTimeStamp = OMX_FALSE;
                pExynosComponent->checkTimeStamp.needCheckStartTimeStamp = OMX_FALSE;
            } else {
                Exynos_ResetCodecData(outputData);
                ret = OMX_TRUE;
                goto EXIT;
            }
        } else if (pExynosComponent->checkTimeStamp.needSetStartTimeStamp == OMX_TRUE) {
            Exynos_ResetCodecData(outputData);
            ret = OMX_TRUE;
            goto EXIT;
        }

        if (outputData->remainDataLen <= (outputUseBuffer->allocSize - outputUseBuffer->dataLen)) {
            copySize = outputData->remainDataLen;

            outputUseBuffer->dataLen += copySize;
            outputUseBuffer->remainDataLen += copySize;
            outputUseBuffer->nFlags = outputData->nFlags;
            outputUseBuffer->timeStamp = outputData->timeStamp;

            ret = OMX_TRUE;

            /* reset outputData */
            Exynos_ResetCodecData(outputData);

            if ((outputUseBuffer->remainDataLen > 0) ||
                (outputUseBuffer->nFlags & OMX_BUFFERFLAG_EOS) ||
                (CHECK_PORT_BEING_FLUSHED(exynosOutputPort)))
                Exynos_OutputBufferReturn(pOMXComponent);
        } else {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "output buffer is smaller than decoded data size Out Length");

            ret = OMX_FALSE;

            /* reset outputData */
            Exynos_ResetCodecData(outputData);
        }
    } else {
        ret = OMX_FALSE;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_BufferProcess(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_AUDIODEC_COMPONENT *pAudioDec = (EXYNOS_OMX_AUDIODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_OMX_BASEPORT      *exynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT      *exynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    EXYNOS_OMX_DATABUFFER    *inputUseBuffer = &exynosInputPort->way.port1WayDataBuffer.dataBuffer;
    EXYNOS_OMX_DATABUFFER    *outputUseBuffer = &exynosOutputPort->way.port1WayDataBuffer.dataBuffer;
    EXYNOS_OMX_DATA          *inputData = &exynosInputPort->processData;
    EXYNOS_OMX_DATA          *outputData = &exynosOutputPort->processData;
    OMX_U32                   copySize = 0;

    pExynosComponent->reInputData = OMX_FALSE;

    FunctionIn();

    while (!pAudioDec->bExitBufferProcessThread) {
        Exynos_OSAL_SleepMillisec(0);

        if (((pExynosComponent->currentState == OMX_StatePause) ||
            (pExynosComponent->currentState == OMX_StateIdle) ||
            (pExynosComponent->transientState == EXYNOS_OMX_TransStateLoadedToIdle) ||
            (pExynosComponent->transientState == EXYNOS_OMX_TransStateExecutingToIdle)) &&
            (pExynosComponent->transientState != EXYNOS_OMX_TransStateIdleToLoaded)&&
            ((!CHECK_PORT_BEING_FLUSHED(exynosInputPort) && !CHECK_PORT_BEING_FLUSHED(exynosOutputPort)))) {
            Exynos_OSAL_SignalWait(pExynosComponent->pauseEvent, DEF_MAX_WAIT_TIME);
            Exynos_OSAL_SignalReset(pExynosComponent->pauseEvent);
        }

        while ((Exynos_Check_BufferProcess_State(pExynosComponent)) && (!pAudioDec->bExitBufferProcessThread)) {
            Exynos_OSAL_SleepMillisec(0);

            Exynos_OSAL_MutexLock(outputUseBuffer->bufferMutex);
            if ((outputUseBuffer->dataValid != OMX_TRUE) &&
                (!CHECK_PORT_BEING_FLUSHED(exynosOutputPort))) {
                Exynos_OSAL_MutexUnlock(outputUseBuffer->bufferMutex);
                ret = Exynos_OutputBufferGetQueue(pExynosComponent);
                if ((ret == OMX_ErrorUndefined) ||
                    (exynosInputPort->portState != OMX_StateIdle) ||
                    (exynosOutputPort->portState != OMX_StateIdle)) {
                    break;
                }
            } else {
                Exynos_OSAL_MutexUnlock(outputUseBuffer->bufferMutex);
            }

            if (pExynosComponent->reInputData == OMX_FALSE) {
                Exynos_OSAL_MutexLock(inputUseBuffer->bufferMutex);
                if ((Exynos_Preprocessor_InputData(pOMXComponent) == OMX_FALSE) &&
                    (!CHECK_PORT_BEING_FLUSHED(exynosInputPort))) {
                        Exynos_OSAL_MutexUnlock(inputUseBuffer->bufferMutex);
                        ret = Exynos_InputBufferGetQueue(pExynosComponent);
                        break;
                }

                Exynos_OSAL_MutexUnlock(inputUseBuffer->bufferMutex);
            }

            Exynos_OSAL_MutexLock(inputUseBuffer->bufferMutex);
            Exynos_OSAL_MutexLock(outputUseBuffer->bufferMutex);
            ret = pAudioDec->exynos_codec_bufferProcess(pOMXComponent, inputData, outputData);
            Exynos_OSAL_MutexUnlock(outputUseBuffer->bufferMutex);
            Exynos_OSAL_MutexUnlock(inputUseBuffer->bufferMutex);

            if (ret == OMX_ErrorInputDataDecodeYet)
                pExynosComponent->reInputData = OMX_TRUE;
            else
                pExynosComponent->reInputData = OMX_FALSE;

            Exynos_OSAL_MutexLock(outputUseBuffer->bufferMutex);
            Exynos_Postprocess_OutputData(pOMXComponent);
            Exynos_OSAL_MutexUnlock(outputUseBuffer->bufferMutex);
        }
    }

EXIT:

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_GetFlushBuffer(EXYNOS_OMX_BASEPORT *pExynosPort, EXYNOS_OMX_DATABUFFER **pDataBuffer)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    FunctionIn();

    if (pExynosPort->portWayType == WAY2_PORT) {
        *pDataBuffer = NULL;
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    *pDataBuffer = &pExynosPort->way.port1WayDataBuffer.dataBuffer;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_FlushPort(OMX_COMPONENTTYPE *pOMXComponent, OMX_S32 portIndex)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_BASEPORT      *pExynosPort = NULL;
    EXYNOS_OMX_DATABUFFER    *flushPortBuffer = NULL;
    OMX_BUFFERHEADERTYPE     *bufferHeader = NULL;
    EXYNOS_OMX_MESSAGE       *message = NULL;
    OMX_U32                   flushNum = 0;
    OMX_S32                   semValue = 0;

    FunctionIn();

    pExynosPort = &pExynosComponent->pExynosPort[portIndex];
    while (Exynos_OSAL_GetElemNum(&pExynosPort->bufferQ) > 0) {
        Exynos_OSAL_Get_SemaphoreCount(pExynosComponent->pExynosPort[portIndex].bufferSemID, &semValue);
        if (semValue == 0)
            Exynos_OSAL_SemaphorePost(pExynosComponent->pExynosPort[portIndex].bufferSemID);
        Exynos_OSAL_SemaphoreWait(pExynosComponent->pExynosPort[portIndex].bufferSemID);

        message = (EXYNOS_OMX_MESSAGE *)Exynos_OSAL_Dequeue(&pExynosPort->bufferQ);
        if (message != NULL) {
            bufferHeader = (OMX_BUFFERHEADERTYPE *)message->pCmdData;
            bufferHeader->nFilledLen = 0;

            if (portIndex == OUTPUT_PORT_INDEX) {
                pExynosComponent->pCallbacks->FillBufferDone(pOMXComponent, pExynosComponent->callbackData, bufferHeader);
            } else {
                pExynosComponent->pCallbacks->EmptyBufferDone(pOMXComponent, pExynosComponent->callbackData, bufferHeader);
            }

            Exynos_OSAL_Free(message);
            message = NULL;
        }
    }

    Exynos_OMX_GetFlushBuffer(pExynosPort, &flushPortBuffer);
    if (flushPortBuffer != NULL) {
        if (flushPortBuffer->dataValid == OMX_TRUE) {
            if (portIndex == INPUT_PORT_INDEX)
                Exynos_InputBufferReturn(pOMXComponent);
            else if (portIndex == OUTPUT_PORT_INDEX)
                Exynos_OutputBufferReturn(pOMXComponent);
        }
    }

    while(1) {
        OMX_S32 cnt = 0;
        Exynos_OSAL_Get_SemaphoreCount(pExynosComponent->pExynosPort[portIndex].bufferSemID, &cnt);
        if (cnt <= 0)
            break;
        Exynos_OSAL_SemaphoreWait(pExynosComponent->pExynosPort[portIndex].bufferSemID);
    }
    Exynos_OSAL_SetElemNum(&pExynosPort->bufferQ, 0);

    pExynosPort->processData.dataLen       = 0;
    pExynosPort->processData.nFlags        = 0;
    pExynosPort->processData.remainDataLen = 0;
    pExynosPort->processData.timeStamp     = 0;
    pExynosPort->processData.usedDataLen   = 0;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_BufferFlush(OMX_COMPONENTTYPE *pOMXComponent, OMX_S32 nPortIndex, OMX_BOOL bEvent)
{
    OMX_ERRORTYPE                  ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = NULL;
    EXYNOS_OMX_AUDIODEC_COMPONENT *pAudioDec = NULL;
    EXYNOS_OMX_BASEPORT           *pExynosPort = NULL;
    EXYNOS_OMX_DATABUFFER         *flushPortBuffer = NULL;
    OMX_U32                        i = 0, cnt = 0;

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
    pAudioDec = (EXYNOS_OMX_AUDIODEC_COMPONENT *)pExynosComponent->hComponentHandle;

    Exynos_OSAL_SignalSet(pExynosComponent->pauseEvent);

    pExynosPort = &pExynosComponent->pExynosPort[nPortIndex];
    Exynos_OMX_GetFlushBuffer(pExynosPort, &flushPortBuffer);

    Exynos_OSAL_MutexLock(flushPortBuffer->bufferMutex);
    ret = Exynos_OMX_FlushPort(pOMXComponent, nPortIndex);
    Exynos_OSAL_MutexUnlock(flushPortBuffer->bufferMutex);

    pExynosComponent->pExynosPort[nPortIndex].bIsPortFlushed = OMX_FALSE;

    if (bEvent == OMX_TRUE && ret == OMX_ErrorNone) {
        pExynosComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
                            pExynosComponent->callbackData,
                            OMX_EventCmdComplete,
                            OMX_CommandFlush, nPortIndex, NULL);
    }

    if (nPortIndex == INPUT_PORT_INDEX) {
        pExynosComponent->checkTimeStamp.needSetStartTimeStamp = OMX_TRUE;
        pExynosComponent->checkTimeStamp.needCheckStartTimeStamp = OMX_FALSE;
        Exynos_OSAL_Memset(pExynosComponent->timeStamp, -19771003, sizeof(OMX_TICKS) * MAX_TIMESTAMP);
        Exynos_OSAL_Memset(pExynosComponent->nFlags, 0, sizeof(OMX_U32) * MAX_FLAGS);
        pExynosComponent->getAllDelayBuffer = OMX_FALSE;
        pExynosComponent->bSaveFlagEOS = OMX_FALSE;
        pExynosComponent->reInputData = OMX_FALSE;
    }

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

OMX_ERRORTYPE Exynos_OMX_AudioDecodeGetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nParamIndex,
    OMX_INOUT OMX_PTR     ComponentParameterStructure)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent = NULL;
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
    case OMX_IndexParamAudioInit:
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
    case OMX_IndexParamAudioPortFormat:
    {
        OMX_AUDIO_PARAM_PORTFORMATTYPE *portFormat = (OMX_AUDIO_PARAM_PORTFORMATTYPE *)ComponentParameterStructure;
        OMX_U32                         portIndex = portFormat->nPortIndex;
        OMX_U32                         index    = portFormat->nIndex;
        EXYNOS_OMX_BASEPORT            *pExynosPort = NULL;
        OMX_PARAM_PORTDEFINITIONTYPE   *portDefinition = NULL;
        OMX_U32                         supportFormatNum = 0; /* supportFormatNum = N-1 */

        ret = Exynos_OMX_Check_SizeVersion(portFormat, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
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

            portFormat->eEncoding = portDefinition->format.audio.eEncoding;
        } else if (portIndex == OUTPUT_PORT_INDEX) {
            supportFormatNum = OUTPUT_PORT_SUPPORTFORMAT_NUM_MAX - 1;
            if (index > supportFormatNum) {
                ret = OMX_ErrorNoMore;
                goto EXIT;
            }

            pExynosPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
            portDefinition = &pExynosPort->portDefinition;

            portFormat->eEncoding = portDefinition->format.audio.eEncoding;
        }
        ret = OMX_ErrorNone;
    }
        break;
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
OMX_ERRORTYPE Exynos_OMX_AudioDecodeSetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        ComponentParameterStructure)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent = NULL;
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
    case OMX_IndexParamAudioPortFormat:
    {
        OMX_AUDIO_PARAM_PORTFORMATTYPE *portFormat = (OMX_AUDIO_PARAM_PORTFORMATTYPE *)ComponentParameterStructure;
        OMX_U32                         portIndex = portFormat->nPortIndex;
        OMX_U32                         index    = portFormat->nIndex;
        EXYNOS_OMX_BASEPORT            *pExynosPort = NULL;
        OMX_PARAM_PORTDEFINITIONTYPE   *portDefinition = NULL;
        OMX_U32                         supportFormatNum = 0; /* supportFormatNum = N-1 */

        ret = Exynos_OMX_Check_SizeVersion(portFormat, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if ((portIndex >= pExynosComponent->portParam.nPorts)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pExynosPort = &pExynosComponent->pExynosPort[portIndex];
        portDefinition = &pExynosPort->portDefinition;

        portDefinition->format.audio.eEncoding = portFormat->eEncoding;
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

OMX_ERRORTYPE Exynos_OMX_AudioDecodeGetConfig(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE nIndex,
    OMX_PTR pComponentConfigStructure)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent = NULL;
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

OMX_ERRORTYPE Exynos_OMX_AudioDecodeSetConfig(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE nIndex,
    OMX_PTR pComponentConfigStructure)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent = NULL;
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
    case OMX_IndexConfigAudioMute:
    {
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "OMX_IndexConfigAudioMute");
        ret = OMX_ErrorUnsupportedIndex;
    }
        break;
    case OMX_IndexConfigAudioVolume:
    {
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "OMX_IndexConfigAudioVolume");
        ret = OMX_ErrorUnsupportedIndex;
    }
        break;
    default:
        ret = Exynos_OMX_SetConfig(hComponent, nIndex, pComponentConfigStructure);
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_AudioDecodeGetExtensionIndex(
    OMX_IN OMX_HANDLETYPE  hComponent,
    OMX_IN OMX_STRING      cParameterName,
    OMX_OUT OMX_INDEXTYPE *pIndexType)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent = NULL;
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

    ret = Exynos_OMX_GetExtensionIndex(hComponent, cParameterName, pIndexType);

EXIT:
    FunctionOut();

    return ret;
}

static OMX_ERRORTYPE Exynos_OMX_BufferProcessThread(OMX_PTR threadData)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pSECComponent = NULL;
    EXYNOS_OMX_MESSAGE       *message = NULL;

    FunctionIn();

    if (threadData == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)threadData;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }
    pSECComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    Exynos_OMX_BufferProcess(pOMXComponent);

    Exynos_OSAL_ThreadExit(NULL);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_BufferProcess_Create(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_AUDIODEC_COMPONENT *pAudioDec = (EXYNOS_OMX_AUDIODEC_COMPONENT *)pExynosComponent->hComponentHandle;

    FunctionIn();

    pAudioDec->bExitBufferProcessThread = OMX_FALSE;

    ret = Exynos_OSAL_ThreadCreate(&pAudioDec->hBufferProcessThread,
                 Exynos_OMX_BufferProcessThread,
                 pOMXComponent);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_BufferProcess_Terminate(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_AUDIODEC_COMPONENT *pAudioDec = (EXYNOS_OMX_AUDIODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    OMX_S32                countValue = 0;
    unsigned int           i = 0;

    FunctionIn();

    pAudioDec->bExitBufferProcessThread = OMX_TRUE;

    for (i = 0; i < ALL_PORT_NUM; i++) {
        Exynos_OSAL_Get_SemaphoreCount(pExynosComponent->pExynosPort[i].bufferSemID, &countValue);
        if (countValue == 0)
            Exynos_OSAL_SemaphorePost(pExynosComponent->pExynosPort[i].bufferSemID);
    }

    Exynos_OSAL_SignalSet(pExynosComponent->pauseEvent);
    Exynos_OSAL_ThreadTerminate(pAudioDec->hBufferProcessThread);
    pAudioDec->hBufferProcessThread = NULL;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_AudioDecodeComponentInit(OMX_IN OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE                  ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE             *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = NULL;
    EXYNOS_OMX_BASEPORT           *pExynosPort = NULL;
    EXYNOS_OMX_AUDIODEC_COMPONENT *pAudioDec = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_Error, Line:%d", __LINE__);
        goto EXIT;
    }

    ret = Exynos_OMX_BaseComponent_Constructor(pOMXComponent);
    if (ret != OMX_ErrorNone) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_Error, Line:%d", __LINE__);
        goto EXIT;
    }

    ret = Exynos_OMX_Port_Constructor(pOMXComponent);
    if (ret != OMX_ErrorNone) {
        Exynos_OMX_BaseComponent_Destructor(pOMXComponent);
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_Error, Line:%d", __LINE__);
        goto EXIT;
    }

    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    pAudioDec = Exynos_OSAL_Malloc(sizeof(EXYNOS_OMX_AUDIODEC_COMPONENT));
    if (pAudioDec == NULL) {
        Exynos_OMX_BaseComponent_Destructor(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }

    Exynos_OSAL_Memset(pAudioDec, 0, sizeof(EXYNOS_OMX_AUDIODEC_COMPONENT));
    pExynosComponent->hComponentHandle = (OMX_HANDLETYPE)pAudioDec;
    pExynosComponent->bSaveFlagEOS = OMX_FALSE;
    pExynosComponent->bMultiThreadProcess = OMX_FALSE;

    /* Input port */
    pExynosPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    pExynosPort->portDefinition.nBufferCountActual = MAX_AUDIO_INPUTBUFFER_NUM;
    pExynosPort->portDefinition.nBufferCountMin = MAX_AUDIO_INPUTBUFFER_NUM;
    pExynosPort->portDefinition.nBufferSize = DEFAULT_AUDIO_INPUT_BUFFER_SIZE;
    pExynosPort->portDefinition.eDomain = OMX_PortDomainAudio;

    pExynosPort->portDefinition.format.audio.cMIMEType = Exynos_OSAL_Malloc(MAX_OMX_MIMETYPE_SIZE);
    Exynos_OSAL_Strcpy(pExynosPort->portDefinition.format.audio.cMIMEType, "audio/raw");
    pExynosPort->portDefinition.format.audio.pNativeRender = 0;
    pExynosPort->portDefinition.format.audio.bFlagErrorConcealment = OMX_FALSE;
    pExynosPort->portDefinition.format.audio.eEncoding = OMX_AUDIO_CodingUnused;

    /* Output port */
    pExynosPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    pExynosPort->portDefinition.nBufferCountActual = MAX_AUDIO_OUTPUTBUFFER_NUM;
    pExynosPort->portDefinition.nBufferCountMin = MAX_AUDIO_OUTPUTBUFFER_NUM;
    pExynosPort->portDefinition.nBufferSize = DEFAULT_AUDIO_OUTPUT_BUFFER_SIZE;
    pExynosPort->portDefinition.eDomain = OMX_PortDomainAudio;

    pExynosPort->portDefinition.format.audio.cMIMEType = Exynos_OSAL_Malloc(MAX_OMX_MIMETYPE_SIZE);
    Exynos_OSAL_Strcpy(pExynosPort->portDefinition.format.audio.cMIMEType, "audio/raw");
    pExynosPort->portDefinition.format.audio.pNativeRender = 0;
    pExynosPort->portDefinition.format.audio.bFlagErrorConcealment = OMX_FALSE;
    pExynosPort->portDefinition.format.audio.eEncoding = OMX_AUDIO_CodingUnused;


    pOMXComponent->UseBuffer              = &Exynos_OMX_UseBuffer;
    pOMXComponent->AllocateBuffer         = &Exynos_OMX_AllocateBuffer;
    pOMXComponent->FreeBuffer             = &Exynos_OMX_FreeBuffer;
    pOMXComponent->ComponentTunnelRequest = &Exynos_OMX_ComponentTunnelRequest;

    pExynosComponent->exynos_AllocateTunnelBuffer = &Exynos_OMX_AllocateTunnelBuffer;
    pExynosComponent->exynos_FreeTunnelBuffer     = &Exynos_OMX_FreeTunnelBuffer;
    pExynosComponent->exynos_BufferProcessCreate    = &Exynos_OMX_BufferProcess_Create;
    pExynosComponent->exynos_BufferProcessTerminate = &Exynos_OMX_BufferProcess_Terminate;
    pExynosComponent->exynos_BufferFlush          = &Exynos_OMX_BufferFlush;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_AudioDecodeComponentDeinit(OMX_IN OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE                  ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE             *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = NULL;
    EXYNOS_OMX_BASEPORT           *pExynosPort = NULL;
    EXYNOS_OMX_AUDIODEC_COMPONENT *pAudioDec = NULL;
    int                            i = 0;

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

    pAudioDec = (EXYNOS_OMX_AUDIODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    Exynos_OSAL_Free(pAudioDec);
    pExynosComponent->hComponentHandle = pAudioDec = NULL;

    for(i = 0; i < ALL_PORT_NUM; i++) {
        pExynosPort = &pExynosComponent->pExynosPort[i];
        Exynos_OSAL_Free(pExynosPort->portDefinition.format.audio.cMIMEType);
        pExynosPort->portDefinition.format.audio.cMIMEType = NULL;
    }

    ret = Exynos_OMX_Port_Destructor(pOMXComponent);

    ret = Exynos_OMX_BaseComponent_Destructor(hComponent);

EXIT:
    FunctionOut();

    return ret;
}
