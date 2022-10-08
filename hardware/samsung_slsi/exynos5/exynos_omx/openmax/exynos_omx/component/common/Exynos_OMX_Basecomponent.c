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
 * @file       Exynos_OMX_Basecomponent.c
 * @brief
 * @author     SeungBeom Kim (sbcrux.kim@samsung.com)
 *             Yunji Kim (yunji.kim@samsung.com)
 * @version    2.0.0
 * @history
 *    2012.02.20 : Create
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "Exynos_OSAL_Event.h"
#include "Exynos_OSAL_Thread.h"
#include "Exynos_OSAL_ETC.h"
#include "Exynos_OSAL_Semaphore.h"
#include "Exynos_OSAL_Mutex.h"
#include "Exynos_OMX_Baseport.h"
#include "Exynos_OMX_Basecomponent.h"
#include "Exynos_OMX_Resourcemanager.h"
#include "Exynos_OMX_Macros.h"

#undef  EXYNOS_LOG_TAG
#define EXYNOS_LOG_TAG    "EXYNOS_BASE_COMP"
#define EXYNOS_LOG_OFF
//#define EXYNOS_TRACE_ON
#include "Exynos_OSAL_Log.h"


/* Change CHECK_SIZE_VERSION Macro */
OMX_ERRORTYPE Exynos_OMX_Check_SizeVersion(OMX_PTR header, OMX_U32 size)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    OMX_VERSIONTYPE* version = NULL;
    if (header == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    version = (OMX_VERSIONTYPE*)((char*)header + sizeof(OMX_U32));
    if (*((OMX_U32*)header) != size) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (version->s.nVersionMajor != VERSIONMAJOR_NUMBER ||
        version->s.nVersionMinor != VERSIONMINOR_NUMBER) {
        ret = OMX_ErrorVersionMismatch;
        goto EXIT;
    }
    ret = OMX_ErrorNone;
EXIT:
    return ret;
}

OMX_ERRORTYPE Exynos_OMX_GetComponentVersion(
    OMX_IN  OMX_HANDLETYPE   hComponent,
    OMX_OUT OMX_STRING       pComponentName,
    OMX_OUT OMX_VERSIONTYPE *pComponentVersion,
    OMX_OUT OMX_VERSIONTYPE *pSpecVersion,
    OMX_OUT OMX_UUIDTYPE    *pComponentUUID)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;
    OMX_U32                   compUUID[3];

    FunctionIn();

    /* check parameters */
    if (hComponent     == NULL ||
        pComponentName == NULL || pComponentVersion == NULL ||
        pSpecVersion   == NULL || pComponentUUID    == NULL) {
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

    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    Exynos_OSAL_Strcpy(pComponentName, pExynosComponent->componentName);
    Exynos_OSAL_Memcpy(pComponentVersion, &(pExynosComponent->componentVersion), sizeof(OMX_VERSIONTYPE));
    Exynos_OSAL_Memcpy(pSpecVersion, &(pExynosComponent->specVersion), sizeof(OMX_VERSIONTYPE));

    /* Fill UUID with handle address, PID and UID.
     * This should guarantee uiniqness */
    compUUID[0] = (OMX_U32)pOMXComponent;
    compUUID[1] = getpid();
    compUUID[2] = getuid();
    Exynos_OSAL_Memcpy(*pComponentUUID, compUUID, 3 * sizeof(*compUUID));

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_GetState (
    OMX_IN OMX_HANDLETYPE  hComponent,
    OMX_OUT OMX_STATETYPE *pState)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;

    FunctionIn();

    if (hComponent == NULL || pState == NULL) {
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

    *pState = pExynosComponent->currentState;
    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_ComponentStateSet(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 messageParam)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_MESSAGE       *message;
    OMX_STATETYPE             destState = messageParam;
    OMX_STATETYPE             currentState = pExynosComponent->currentState;
    EXYNOS_OMX_BASEPORT      *pExynosPort = NULL;
    OMX_S32                   countValue = 0;
    unsigned int              i = 0, j = 0;
    int                       k = 0;

    FunctionIn();

    /* check parameters */
    if (currentState == destState) {
         ret = OMX_ErrorSameState;
            goto EXIT;
    }
    if (currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    if ((currentState == OMX_StateLoaded) && (destState == OMX_StateIdle)) {
        ret = Exynos_OMX_Get_Resource(pOMXComponent);
        if (ret != OMX_ErrorNone) {
            Exynos_OSAL_SignalSet(pExynosComponent->abendStateEvent);
            goto EXIT;
        }
    }
    if (((currentState == OMX_StateIdle) && (destState == OMX_StateLoaded))       ||
        ((currentState == OMX_StateIdle) && (destState == OMX_StateInvalid))      ||
        ((currentState == OMX_StateExecuting) && (destState == OMX_StateInvalid)) ||
        ((currentState == OMX_StatePause) && (destState == OMX_StateInvalid))) {
        Exynos_OMX_Release_Resource(pOMXComponent);
    }

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "destState: %d currentState: %d", destState, currentState);
    switch (destState) {
    case OMX_StateInvalid:
        switch (currentState) {
        case OMX_StateWaitForResources:
            Exynos_OMX_Out_WaitForResource(pOMXComponent);
        case OMX_StateIdle:
        case OMX_StateExecuting:
        case OMX_StatePause:
        case OMX_StateLoaded:
            pExynosComponent->currentState = OMX_StateInvalid;
            ret = pExynosComponent->exynos_BufferProcessTerminate(pOMXComponent);

            for (i = 0; i < ALL_PORT_NUM; i++) {
                if (pExynosComponent->pExynosPort[i].portWayType == WAY1_PORT) {
                    Exynos_OSAL_MutexTerminate(pExynosComponent->pExynosPort[i].way.port1WayDataBuffer.dataBuffer.bufferMutex);
                    pExynosComponent->pExynosPort[i].way.port1WayDataBuffer.dataBuffer.bufferMutex = NULL;
                } else if (pExynosComponent->pExynosPort[i].portWayType == WAY2_PORT) {
                    Exynos_OSAL_MutexTerminate(pExynosComponent->pExynosPort[i].way.port2WayDataBuffer.inputDataBuffer.bufferMutex);
                    pExynosComponent->pExynosPort[i].way.port2WayDataBuffer.inputDataBuffer.bufferMutex = NULL;
                    Exynos_OSAL_MutexTerminate(pExynosComponent->pExynosPort[i].way.port2WayDataBuffer.outputDataBuffer.bufferMutex);
                    pExynosComponent->pExynosPort[i].way.port2WayDataBuffer.outputDataBuffer.bufferMutex = NULL;
                }
                Exynos_OSAL_MutexTerminate(pExynosComponent->pExynosPort[i].hPortMutex);
                pExynosComponent->pExynosPort[i].hPortMutex = NULL;
            }

            if (pExynosComponent->bMultiThreadProcess == OMX_FALSE) {
                Exynos_OSAL_SignalTerminate(pExynosComponent->pauseEvent);
                pExynosComponent->pauseEvent = NULL;
            } else {
                for (i = 0; i < ALL_PORT_NUM; i++) {
                    Exynos_OSAL_SignalTerminate(pExynosComponent->pExynosPort[i].pauseEvent);
                    pExynosComponent->pExynosPort[i].pauseEvent = NULL;
                    if (pExynosComponent->pExynosPort[i].bufferProcessType == BUFFER_SHARE) {
                        Exynos_OSAL_SignalTerminate(pExynosComponent->pExynosPort[i].hAllCodecBufferReturnEvent);
                        pExynosComponent->pExynosPort[i].hAllCodecBufferReturnEvent = NULL;
                    }
                }
            }
            for (i = 0; i < ALL_PORT_NUM; i++) {
                Exynos_OSAL_SemaphoreTerminate(pExynosComponent->pExynosPort[i].bufferSemID);
                pExynosComponent->pExynosPort[i].bufferSemID = NULL;
            }
            if (pExynosComponent->exynos_codec_componentTerminate != NULL)
                pExynosComponent->exynos_codec_componentTerminate(pOMXComponent);

            ret = OMX_ErrorInvalidState;
            break;
        default:
            ret = OMX_ErrorInvalidState;
            break;
        }
        break;
    case OMX_StateLoaded:
        switch (currentState) {
        case OMX_StateIdle:
            ret = pExynosComponent->exynos_BufferProcessTerminate(pOMXComponent);

            for (i = 0; i < ALL_PORT_NUM; i++) {
                if (pExynosComponent->pExynosPort[i].portWayType == WAY1_PORT) {
                    Exynos_OSAL_MutexTerminate(pExynosComponent->pExynosPort[i].way.port1WayDataBuffer.dataBuffer.bufferMutex);
                    pExynosComponent->pExynosPort[i].way.port1WayDataBuffer.dataBuffer.bufferMutex = NULL;
                } else if (pExynosComponent->pExynosPort[i].portWayType == WAY2_PORT) {
                    Exynos_OSAL_MutexTerminate(pExynosComponent->pExynosPort[i].way.port2WayDataBuffer.inputDataBuffer.bufferMutex);
                    pExynosComponent->pExynosPort[i].way.port2WayDataBuffer.inputDataBuffer.bufferMutex = NULL;
                    Exynos_OSAL_MutexTerminate(pExynosComponent->pExynosPort[i].way.port2WayDataBuffer.outputDataBuffer.bufferMutex);
                    pExynosComponent->pExynosPort[i].way.port2WayDataBuffer.outputDataBuffer.bufferMutex = NULL;
                }
                Exynos_OSAL_MutexTerminate(pExynosComponent->pExynosPort[i].hPortMutex);
                pExynosComponent->pExynosPort[i].hPortMutex = NULL;
            }
            if (pExynosComponent->bMultiThreadProcess == OMX_FALSE) {
                Exynos_OSAL_SignalTerminate(pExynosComponent->pauseEvent);
                pExynosComponent->pauseEvent = NULL;
            } else {
                for (i = 0; i < ALL_PORT_NUM; i++) {
                    Exynos_OSAL_SignalTerminate(pExynosComponent->pExynosPort[i].pauseEvent);
                    pExynosComponent->pExynosPort[i].pauseEvent = NULL;
                    if (pExynosComponent->pExynosPort[i].bufferProcessType == BUFFER_SHARE) {
                        Exynos_OSAL_SignalTerminate(pExynosComponent->pExynosPort[i].hAllCodecBufferReturnEvent);
                        pExynosComponent->pExynosPort[i].hAllCodecBufferReturnEvent = NULL;
                    }
                }
            }
            for (i = 0; i < ALL_PORT_NUM; i++) {
                Exynos_OSAL_SemaphoreTerminate(pExynosComponent->pExynosPort[i].bufferSemID);
                pExynosComponent->pExynosPort[i].bufferSemID = NULL;
            }

            pExynosComponent->exynos_codec_componentTerminate(pOMXComponent);

            for (i = 0; i < (pExynosComponent->portParam.nPorts); i++) {
                pExynosPort = (pExynosComponent->pExynosPort + i);
                if (CHECK_PORT_TUNNELED(pExynosPort) && CHECK_PORT_BUFFER_SUPPLIER(pExynosPort)) {
                    while (Exynos_OSAL_GetElemNum(&pExynosPort->bufferQ) > 0) {
                        message = (EXYNOS_OMX_MESSAGE*)Exynos_OSAL_Dequeue(&pExynosPort->bufferQ);
                        if (message != NULL)
                            Exynos_OSAL_Free(message);
                    }
                    ret = pExynosComponent->exynos_FreeTunnelBuffer(pExynosPort, i);
                    if (OMX_ErrorNone != ret) {
                        goto EXIT;
                    }
                } else {
                    if (CHECK_PORT_ENABLED(pExynosPort)) {
                        Exynos_OSAL_SemaphoreWait(pExynosPort->unloadedResource);
                        pExynosPort->portDefinition.bPopulated = OMX_FALSE;
                    }
                }
            }
            pExynosComponent->currentState = OMX_StateLoaded;
            break;
        case OMX_StateWaitForResources:
            ret = Exynos_OMX_Out_WaitForResource(pOMXComponent);
            pExynosComponent->currentState = OMX_StateLoaded;
            break;
        case OMX_StateExecuting:
        case OMX_StatePause:
        default:
            ret = OMX_ErrorIncorrectStateTransition;
            break;
        }
        break;
    case OMX_StateIdle:
        switch (currentState) {
        case OMX_StateLoaded:
            for (i = 0; i < pExynosComponent->portParam.nPorts; i++) {
                pExynosPort = (pExynosComponent->pExynosPort + i);
                if (pExynosPort == NULL) {
                    ret = OMX_ErrorBadParameter;
                    goto EXIT;
                }
                if (CHECK_PORT_TUNNELED(pExynosPort) && CHECK_PORT_BUFFER_SUPPLIER(pExynosPort)) {
                    if (CHECK_PORT_ENABLED(pExynosPort)) {
                        ret = pExynosComponent->exynos_AllocateTunnelBuffer(pExynosPort, i);
                        if (ret!=OMX_ErrorNone)
                            goto EXIT;
                    }
                } else {
                    if (CHECK_PORT_ENABLED(pExynosPort)) {
                        Exynos_OSAL_SemaphoreWait(pExynosComponent->pExynosPort[i].loadedResource);
                        if (pExynosComponent->abendState == OMX_TRUE) {
                            Exynos_OSAL_SignalSet(pExynosComponent->abendStateEvent);
                            ret = Exynos_OMX_Release_Resource(pOMXComponent);
                            goto EXIT;
                        }
                        pExynosPort->portDefinition.bPopulated = OMX_TRUE;
                    }
                }
            }
            ret = pExynosComponent->exynos_codec_componentInit(pOMXComponent);
            if (ret != OMX_ErrorNone) {
                /*
                 * if (CHECK_PORT_TUNNELED == OMX_TRUE) thenTunnel Buffer Free
                 */
                Exynos_OSAL_SignalSet(pExynosComponent->abendStateEvent);
                Exynos_OMX_Release_Resource(pOMXComponent);
                goto EXIT;
            }
            if (pExynosComponent->bMultiThreadProcess == OMX_FALSE) {
                Exynos_OSAL_SignalCreate(&pExynosComponent->pauseEvent);
            } else {
                for (i = 0; i < ALL_PORT_NUM; i++) {
                    Exynos_OSAL_SignalCreate(&pExynosComponent->pExynosPort[i].pauseEvent);
                    if (pExynosComponent->pExynosPort[i].bufferProcessType == BUFFER_SHARE)
                        Exynos_OSAL_SignalCreate(&pExynosComponent->pExynosPort[i].hAllCodecBufferReturnEvent);
                }
            }
            for (i = 0; i < ALL_PORT_NUM; i++) {
                ret = Exynos_OSAL_SemaphoreCreate(&pExynosComponent->pExynosPort[i].bufferSemID);
                if (ret != OMX_ErrorNone) {
                    ret = OMX_ErrorInsufficientResources;
                    Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
                    goto EXIT;
                }
            }
            for (i = 0; i < ALL_PORT_NUM; i++) {
                if (pExynosComponent->pExynosPort[i].portWayType == WAY1_PORT) {
                    ret = Exynos_OSAL_MutexCreate(&pExynosComponent->pExynosPort[i].way.port1WayDataBuffer.dataBuffer.bufferMutex);
                    if (ret != OMX_ErrorNone) {
                        ret = OMX_ErrorInsufficientResources;
                        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
                        goto EXIT;
                    }
                } else if (pExynosComponent->pExynosPort[i].portWayType == WAY2_PORT) {
                    ret = Exynos_OSAL_MutexCreate(&pExynosComponent->pExynosPort[i].way.port2WayDataBuffer.inputDataBuffer.bufferMutex);
                    if (ret != OMX_ErrorNone) {
                        ret = OMX_ErrorInsufficientResources;
                        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
                        goto EXIT;
                    }
                    ret = Exynos_OSAL_MutexCreate(&pExynosComponent->pExynosPort[i].way.port2WayDataBuffer.outputDataBuffer.bufferMutex);
                    if (ret != OMX_ErrorNone) {
                        ret = OMX_ErrorInsufficientResources;
                        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
                        goto EXIT;
                    }
                }
                ret = Exynos_OSAL_MutexCreate(&pExynosComponent->pExynosPort[i].hPortMutex);
                if (ret != OMX_ErrorNone) {
                    ret = OMX_ErrorInsufficientResources;
                    goto EXIT;
                }
            }

            ret = pExynosComponent->exynos_BufferProcessCreate(pOMXComponent);
            if (ret != OMX_ErrorNone) {
                /*
                 * if (CHECK_PORT_TUNNELED == OMX_TRUE) thenTunnel Buffer Free
                 */
                if (pExynosComponent->bMultiThreadProcess == OMX_FALSE) {
                    Exynos_OSAL_SignalTerminate(pExynosComponent->pauseEvent);
                    pExynosComponent->pauseEvent = NULL;
                } else {
                    for (i = 0; i < ALL_PORT_NUM; i++) {
                        Exynos_OSAL_SignalTerminate(pExynosComponent->pExynosPort[i].pauseEvent);
                        pExynosComponent->pExynosPort[i].pauseEvent = NULL;
                        if (pExynosComponent->pExynosPort[i].bufferProcessType == BUFFER_SHARE) {
                            Exynos_OSAL_SignalTerminate(pExynosComponent->pExynosPort[i].hAllCodecBufferReturnEvent);
                            pExynosComponent->pExynosPort[i].hAllCodecBufferReturnEvent = NULL;
                        }
                    }
                }
                for (i = 0; i < ALL_PORT_NUM; i++) {
                    if (pExynosComponent->pExynosPort[i].portWayType == WAY1_PORT) {
                        Exynos_OSAL_MutexTerminate(pExynosComponent->pExynosPort[i].way.port1WayDataBuffer.dataBuffer.bufferMutex);
                        pExynosComponent->pExynosPort[i].way.port1WayDataBuffer.dataBuffer.bufferMutex = NULL;
                    } else if (pExynosComponent->pExynosPort[i].portWayType == WAY2_PORT) {
                        Exynos_OSAL_MutexTerminate(pExynosComponent->pExynosPort[i].way.port2WayDataBuffer.inputDataBuffer.bufferMutex);
                        pExynosComponent->pExynosPort[i].way.port2WayDataBuffer.inputDataBuffer.bufferMutex = NULL;
                        Exynos_OSAL_MutexTerminate(pExynosComponent->pExynosPort[i].way.port2WayDataBuffer.outputDataBuffer.bufferMutex);
                        pExynosComponent->pExynosPort[i].way.port2WayDataBuffer.outputDataBuffer.bufferMutex = NULL;
                    }
                    Exynos_OSAL_MutexTerminate(pExynosComponent->pExynosPort[i].hPortMutex);
                    pExynosComponent->pExynosPort[i].hPortMutex = NULL;
                }
                for (i = 0; i < ALL_PORT_NUM; i++) {
                    Exynos_OSAL_SemaphoreTerminate(pExynosComponent->pExynosPort[i].bufferSemID);
                    pExynosComponent->pExynosPort[i].bufferSemID = NULL;
                }

                ret = OMX_ErrorInsufficientResources;
                goto EXIT;
            }
            pExynosComponent->currentState = OMX_StateIdle;
            break;
        case OMX_StateExecuting:
        case OMX_StatePause:
            Exynos_OMX_BufferFlushProcess(pOMXComponent, ALL_PORT_INDEX, OMX_FALSE);
            pExynosComponent->currentState = OMX_StateIdle;
            break;
        case OMX_StateWaitForResources:
            pExynosComponent->currentState = OMX_StateIdle;
            break;
        default:
            ret = OMX_ErrorIncorrectStateTransition;
            break;
        }
        break;
    case OMX_StateExecuting:
        switch (currentState) {
        case OMX_StateLoaded:
            ret = OMX_ErrorIncorrectStateTransition;
            break;
        case OMX_StateIdle:
            for (i = 0; i < pExynosComponent->portParam.nPorts; i++) {
                pExynosPort = &pExynosComponent->pExynosPort[i];
                if (CHECK_PORT_TUNNELED(pExynosPort) && CHECK_PORT_BUFFER_SUPPLIER(pExynosPort) && CHECK_PORT_ENABLED(pExynosPort)) {
                    for (j = 0; j < pExynosPort->tunnelBufferNum; j++) {
                        Exynos_OSAL_SemaphorePost(pExynosComponent->pExynosPort[i].bufferSemID);
                    }
                }
            }

            pExynosComponent->transientState = EXYNOS_OMX_TransStateMax;
            pExynosComponent->currentState = OMX_StateExecuting;
            if (pExynosComponent->bMultiThreadProcess == OMX_FALSE) {
                Exynos_OSAL_SignalSet(pExynosComponent->pauseEvent);
            } else {
                for (i = 0; i < ALL_PORT_NUM; i++) {
                    Exynos_OSAL_SignalSet(pExynosComponent->pExynosPort[i].pauseEvent);
                }
            }
            break;
        case OMX_StatePause:
            for (i = 0; i < pExynosComponent->portParam.nPorts; i++) {
                pExynosPort = &pExynosComponent->pExynosPort[i];
                if (CHECK_PORT_TUNNELED(pExynosPort) && CHECK_PORT_BUFFER_SUPPLIER(pExynosPort) && CHECK_PORT_ENABLED(pExynosPort)) {
                    OMX_S32 semaValue = 0, cnt = 0;
                    Exynos_OSAL_Get_SemaphoreCount(pExynosComponent->pExynosPort[i].bufferSemID, &semaValue);
                    if (Exynos_OSAL_GetElemNum(&pExynosPort->bufferQ) > semaValue) {
                        cnt = Exynos_OSAL_GetElemNum(&pExynosPort->bufferQ) - semaValue;
                        for (k = 0; k < cnt; k++) {
                            Exynos_OSAL_SemaphorePost(pExynosComponent->pExynosPort[i].bufferSemID);
                        }
                    }
                }
            }

            pExynosComponent->currentState = OMX_StateExecuting;
            if (pExynosComponent->bMultiThreadProcess == OMX_FALSE) {
                Exynos_OSAL_SignalSet(pExynosComponent->pauseEvent);
            } else {
                for (i = 0; i < ALL_PORT_NUM; i++) {
                    Exynos_OSAL_SignalSet(pExynosComponent->pExynosPort[i].pauseEvent);
                }
            }
            break;
        case OMX_StateWaitForResources:
            ret = OMX_ErrorIncorrectStateTransition;
            break;
        default:
            ret = OMX_ErrorIncorrectStateTransition;
            break;
        }
        break;
    case OMX_StatePause:
        switch (currentState) {
        case OMX_StateLoaded:
            ret = OMX_ErrorIncorrectStateTransition;
            break;
        case OMX_StateIdle:
            pExynosComponent->currentState = OMX_StatePause;
            break;
        case OMX_StateExecuting:
            pExynosComponent->currentState = OMX_StatePause;
            break;
        case OMX_StateWaitForResources:
            ret = OMX_ErrorIncorrectStateTransition;
            break;
        default:
            ret = OMX_ErrorIncorrectStateTransition;
            break;
        }
        break;
    case OMX_StateWaitForResources:
        switch (currentState) {
        case OMX_StateLoaded:
            ret = Exynos_OMX_In_WaitForResource(pOMXComponent);
            pExynosComponent->currentState = OMX_StateWaitForResources;
            break;
        case OMX_StateIdle:
        case OMX_StateExecuting:
        case OMX_StatePause:
            ret = OMX_ErrorIncorrectStateTransition;
            break;
        default:
            ret = OMX_ErrorIncorrectStateTransition;
            break;
        }
        break;
    default:
        ret = OMX_ErrorIncorrectStateTransition;
        break;
    }

EXIT:
    if (ret == OMX_ErrorNone) {
        if (pExynosComponent->pCallbacks != NULL) {
            pExynosComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
            pExynosComponent->callbackData,
            OMX_EventCmdComplete, OMX_CommandStateSet,
            destState, NULL);
        }
    } else {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s:%d", __FUNCTION__, __LINE__);
        if (pExynosComponent->pCallbacks != NULL) {
            pExynosComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
            pExynosComponent->callbackData,
            OMX_EventError, ret, 0, NULL);
        }
    }
    FunctionOut();

    return ret;
}

static OMX_ERRORTYPE Exynos_OMX_MessageHandlerThread(OMX_PTR threadData)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;
    EXYNOS_OMX_MESSAGE       *message = NULL;
    OMX_U32                   messageType = 0, portIndex = 0;

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

    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    while (pExynosComponent->bExitMessageHandlerThread == OMX_FALSE) {
        Exynos_OSAL_SemaphoreWait(pExynosComponent->msgSemaphoreHandle);
        message = (EXYNOS_OMX_MESSAGE *)Exynos_OSAL_Dequeue(&pExynosComponent->messageQ);
        if (message != NULL) {
            messageType = message->messageType;
            switch (messageType) {
            case OMX_CommandStateSet:
                ret = Exynos_OMX_ComponentStateSet(pOMXComponent, message->messageParam);
                break;
            case OMX_CommandFlush:
                ret = Exynos_OMX_BufferFlushProcess(pOMXComponent, message->messageParam, OMX_TRUE);
                break;
            case OMX_CommandPortDisable:
                ret = Exynos_OMX_PortDisableProcess(pOMXComponent, message->messageParam);
                break;
            case OMX_CommandPortEnable:
                ret = Exynos_OMX_PortEnableProcess(pOMXComponent, message->messageParam);
                break;
            case OMX_CommandMarkBuffer:
                portIndex = message->messageParam;
                pExynosComponent->pExynosPort[portIndex].markType.hMarkTargetComponent = ((OMX_MARKTYPE *)message->pCmdData)->hMarkTargetComponent;
                pExynosComponent->pExynosPort[portIndex].markType.pMarkData            = ((OMX_MARKTYPE *)message->pCmdData)->pMarkData;
                break;
            case (OMX_COMMANDTYPE)EXYNOS_OMX_CommandComponentDeInit:
                pExynosComponent->bExitMessageHandlerThread = OMX_TRUE;
                break;
            default:
                break;
            }
            Exynos_OSAL_Free(message);
            message = NULL;
        }
    }

    Exynos_OSAL_ThreadExit(NULL);

EXIT:
    FunctionOut();

    return ret;
}

static OMX_ERRORTYPE Exynos_StateSet(EXYNOS_OMX_BASECOMPONENT *pExynosComponent, OMX_U32 nParam)
{
    OMX_U32 destState = nParam;
    OMX_U32 i = 0;

    if ((destState == OMX_StateIdle) && (pExynosComponent->currentState == OMX_StateLoaded)) {
        pExynosComponent->transientState = EXYNOS_OMX_TransStateLoadedToIdle;
        for(i = 0; i < pExynosComponent->portParam.nPorts; i++) {
            pExynosComponent->pExynosPort[i].portState = OMX_StateIdle;
        }
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "to OMX_StateIdle");
    } else if ((destState == OMX_StateLoaded) && (pExynosComponent->currentState == OMX_StateIdle)) {
        pExynosComponent->transientState = EXYNOS_OMX_TransStateIdleToLoaded;
        for (i = 0; i < pExynosComponent->portParam.nPorts; i++) {
            pExynosComponent->pExynosPort[i].portState = OMX_StateLoaded;
        }
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "to OMX_StateLoaded");
    } else if ((destState == OMX_StateIdle) && (pExynosComponent->currentState == OMX_StateExecuting)) {
        EXYNOS_OMX_BASEPORT *pExynosPort = NULL;

        pExynosPort = &(pExynosComponent->pExynosPort[INPUT_PORT_INDEX]);
        if ((pExynosPort->portDefinition.bEnabled == OMX_FALSE) &&
            (pExynosPort->portState == OMX_StateIdle)) {
            pExynosPort->exceptionFlag = INVALID_STATE;
            Exynos_OSAL_SemaphorePost(pExynosPort->loadedResource);
        }

        pExynosPort = &(pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX]);
        if ((pExynosPort->portDefinition.bEnabled == OMX_FALSE) &&
            (pExynosPort->portState == OMX_StateIdle)) {
            pExynosPort->exceptionFlag = INVALID_STATE;
            Exynos_OSAL_SemaphorePost(pExynosPort->loadedResource);
        }

        pExynosComponent->transientState = EXYNOS_OMX_TransStateExecutingToIdle;
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "to OMX_StateIdle");
    } else if ((destState == OMX_StateExecuting) && (pExynosComponent->currentState == OMX_StateIdle)) {
        pExynosComponent->transientState = EXYNOS_OMX_TransStateIdleToExecuting;
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "to OMX_StateExecuting");
    } else if (destState == OMX_StateInvalid) {
        for (i = 0; i < pExynosComponent->portParam.nPorts; i++) {
            pExynosComponent->pExynosPort[i].portState = OMX_StateInvalid;
        }
    }

    return OMX_ErrorNone;
}

static OMX_ERRORTYPE Exynos_SetPortFlush(EXYNOS_OMX_BASECOMPONENT *pExynosComponent, OMX_U32 nParam)
{
    OMX_ERRORTYPE        ret = OMX_ErrorNone;
    EXYNOS_OMX_BASEPORT *pExynosPort = NULL;
    OMX_S32              portIndex = nParam;
    OMX_U16              i = 0, cnt = 0, index = 0;


    if ((pExynosComponent->currentState == OMX_StateExecuting) ||
        (pExynosComponent->currentState == OMX_StatePause)) {
        if ((portIndex != ALL_PORT_INDEX) &&
           ((OMX_S32)portIndex >= (OMX_S32)pExynosComponent->portParam.nPorts)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        /*********************
        *    need flush event set ?????
        **********************/
        cnt = (portIndex == ALL_PORT_INDEX ) ? ALL_PORT_NUM : 1;
        for (i = 0; i < cnt; i++) {
            if (portIndex == ALL_PORT_INDEX)
                index = i;
            else
                index = portIndex;
            pExynosComponent->pExynosPort[index].bIsPortFlushed = OMX_TRUE;
        }
    } else {
        ret = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }
    ret = OMX_ErrorNone;

EXIT:
    return ret;
}

static OMX_ERRORTYPE Exynos_SetPortEnable(EXYNOS_OMX_BASECOMPONENT *pExynosComponent, OMX_U32 nParam)
{
    OMX_ERRORTYPE        ret = OMX_ErrorNone;
    EXYNOS_OMX_BASEPORT *pExynosPort = NULL;
    OMX_S32              portIndex = nParam;
    OMX_U16              i = 0, cnt = 0;

    FunctionIn();

    if ((portIndex != ALL_PORT_INDEX) &&
        ((OMX_S32)portIndex >= (OMX_S32)pExynosComponent->portParam.nPorts)) {
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    if (portIndex == ALL_PORT_INDEX) {
        for (i = 0; i < pExynosComponent->portParam.nPorts; i++) {
            pExynosPort = &pExynosComponent->pExynosPort[i];
            if (CHECK_PORT_ENABLED(pExynosPort)) {
                ret = OMX_ErrorIncorrectStateOperation;
                goto EXIT;
            } else {
                pExynosPort->portState = OMX_StateIdle;
            }
        }
    } else {
        pExynosPort = &pExynosComponent->pExynosPort[portIndex];
        if (CHECK_PORT_ENABLED(pExynosPort)) {
            ret = OMX_ErrorIncorrectStateOperation;
            goto EXIT;
        } else {
            pExynosPort->portState = OMX_StateIdle;
        }
    }
    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;

}

static OMX_ERRORTYPE Exynos_SetPortDisable(EXYNOS_OMX_BASECOMPONENT *pExynosComponent, OMX_U32 nParam)
{
    OMX_ERRORTYPE        ret = OMX_ErrorNone;
    EXYNOS_OMX_BASEPORT *pExynosPort = NULL;
    OMX_S32              portIndex = nParam;
    OMX_U16              i = 0, cnt = 0;

    FunctionIn();

    if ((portIndex != ALL_PORT_INDEX) &&
        ((OMX_S32)portIndex >= (OMX_S32)pExynosComponent->portParam.nPorts)) {
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    if (portIndex == ALL_PORT_INDEX) {
        for (i = 0; i < pExynosComponent->portParam.nPorts; i++) {
            pExynosPort = &pExynosComponent->pExynosPort[i];
            if (!CHECK_PORT_ENABLED(pExynosPort)) {
                ret = OMX_ErrorIncorrectStateOperation;
                goto EXIT;
            }
            pExynosPort->portState = OMX_StateLoaded;
            pExynosPort->bIsPortDisabled = OMX_TRUE;
        }
    } else {
        pExynosPort = &pExynosComponent->pExynosPort[portIndex];
        pExynosPort->portState = OMX_StateLoaded;
        pExynosPort->bIsPortDisabled = OMX_TRUE;
    }
    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

static OMX_ERRORTYPE Exynos_SetMarkBuffer(EXYNOS_OMX_BASECOMPONENT *pExynosComponent, OMX_U32 nParam)
{
    OMX_ERRORTYPE        ret = OMX_ErrorNone;
    EXYNOS_OMX_BASEPORT *pExynosPort = NULL;
    OMX_U32              portIndex = nParam;
    OMX_U16              i = 0, cnt = 0;


    if (nParam >= pExynosComponent->portParam.nPorts) {
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    if ((pExynosComponent->currentState == OMX_StateExecuting) ||
        (pExynosComponent->currentState == OMX_StatePause)) {
        ret = OMX_ErrorNone;
    } else {
        ret = OMX_ErrorIncorrectStateOperation;
    }

EXIT:
    return ret;
}

static OMX_ERRORTYPE Exynos_OMX_CommandQueue(
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent,
    OMX_COMMANDTYPE        Cmd,
    OMX_U32                nParam,
    OMX_PTR                pCmdData)
{
    OMX_ERRORTYPE    ret = OMX_ErrorNone;
    EXYNOS_OMX_MESSAGE *command = (EXYNOS_OMX_MESSAGE *)Exynos_OSAL_Malloc(sizeof(EXYNOS_OMX_MESSAGE));

    if (command == NULL) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    command->messageType  = (OMX_U32)Cmd;
    command->messageParam = nParam;
    command->pCmdData     = pCmdData;

    ret = Exynos_OSAL_Queue(&pExynosComponent->messageQ, (void *)command);
    if (ret != 0) {
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }
    ret = Exynos_OSAL_SemaphorePost(pExynosComponent->msgSemaphoreHandle);

EXIT:
    return ret;
}

OMX_ERRORTYPE Exynos_OMX_SendCommand(
    OMX_IN OMX_HANDLETYPE  hComponent,
    OMX_IN OMX_COMMANDTYPE Cmd,
    OMX_IN OMX_U32         nParam,
    OMX_IN OMX_PTR         pCmdData)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;
    EXYNOS_OMX_MESSAGE       *message = NULL;

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

    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    switch (Cmd) {
    case OMX_CommandStateSet :
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Command: OMX_CommandStateSet");
        Exynos_StateSet(pExynosComponent, nParam);
        break;
    case OMX_CommandFlush :
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Command: OMX_CommandFlush");
        ret = Exynos_SetPortFlush(pExynosComponent, nParam);
        if (ret != OMX_ErrorNone)
            goto EXIT;
        break;
    case OMX_CommandPortDisable :
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Command: OMX_CommandPortDisable");
        ret = Exynos_SetPortDisable(pExynosComponent, nParam);
        if (ret != OMX_ErrorNone)
            goto EXIT;
        break;
    case OMX_CommandPortEnable :
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Command: OMX_CommandPortEnable");
        ret = Exynos_SetPortEnable(pExynosComponent, nParam);
        if (ret != OMX_ErrorNone)
            goto EXIT;
        break;
    case OMX_CommandMarkBuffer :
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Command: OMX_CommandMarkBuffer");
        ret = Exynos_SetMarkBuffer(pExynosComponent, nParam);
        if (ret != OMX_ErrorNone)
            goto EXIT;
        break;
    default:
        break;
    }

    ret = Exynos_OMX_CommandQueue(pExynosComponent, Cmd, nParam, pCmdData);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_GetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nParamIndex,
    OMX_INOUT OMX_PTR     ComponentParameterStructure)
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

    if (ComponentParameterStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    switch (nParamIndex) {
    case OMX_IndexParamAudioInit:
    case OMX_IndexParamVideoInit:
    case OMX_IndexParamImageInit:
    case OMX_IndexParamOtherInit:
    {
        OMX_PORT_PARAM_TYPE *portParam = (OMX_PORT_PARAM_TYPE *)ComponentParameterStructure;
        ret = Exynos_OMX_Check_SizeVersion(portParam, sizeof(OMX_PORT_PARAM_TYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }
        portParam->nPorts         = 0;
        portParam->nStartPortNumber     = 0;
    }
        break;
    case OMX_IndexParamPortDefinition:
    {
        OMX_PARAM_PORTDEFINITIONTYPE *portDefinition = (OMX_PARAM_PORTDEFINITIONTYPE *)ComponentParameterStructure;
        OMX_U32                       portIndex = portDefinition->nPortIndex;
        EXYNOS_OMX_BASEPORT          *pExynosPort;

        if (portIndex >= pExynosComponent->portParam.nPorts) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }
        ret = Exynos_OMX_Check_SizeVersion(portDefinition, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        pExynosPort = &pExynosComponent->pExynosPort[portIndex];
        Exynos_OSAL_Memcpy(portDefinition, &pExynosPort->portDefinition, portDefinition->nSize);
    }
        break;
    case OMX_IndexParamPriorityMgmt:
    {
        OMX_PRIORITYMGMTTYPE *compPriority = (OMX_PRIORITYMGMTTYPE *)ComponentParameterStructure;

        ret = Exynos_OMX_Check_SizeVersion(compPriority, sizeof(OMX_PRIORITYMGMTTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        compPriority->nGroupID       = pExynosComponent->compPriority.nGroupID;
        compPriority->nGroupPriority = pExynosComponent->compPriority.nGroupPriority;
    }
        break;

    case OMX_IndexParamCompBufferSupplier:
    {
        OMX_PARAM_BUFFERSUPPLIERTYPE *bufferSupplier = (OMX_PARAM_BUFFERSUPPLIERTYPE *)ComponentParameterStructure;
        OMX_U32                       portIndex = bufferSupplier->nPortIndex;
        EXYNOS_OMX_BASEPORT          *pExynosPort;

        if ((pExynosComponent->currentState == OMX_StateLoaded) ||
            (pExynosComponent->currentState == OMX_StateWaitForResources)) {
            if (portIndex >= pExynosComponent->portParam.nPorts) {
                ret = OMX_ErrorBadPortIndex;
                goto EXIT;
            }
            ret = Exynos_OMX_Check_SizeVersion(bufferSupplier, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE));
            if (ret != OMX_ErrorNone) {
                goto EXIT;
            }

            pExynosPort = &pExynosComponent->pExynosPort[portIndex];


            if (pExynosPort->portDefinition.eDir == OMX_DirInput) {
                if (CHECK_PORT_BUFFER_SUPPLIER(pExynosPort)) {
                    bufferSupplier->eBufferSupplier = OMX_BufferSupplyInput;
                } else if (CHECK_PORT_TUNNELED(pExynosPort)) {
                    bufferSupplier->eBufferSupplier = OMX_BufferSupplyOutput;
                } else {
                    bufferSupplier->eBufferSupplier = OMX_BufferSupplyUnspecified;
                }
            } else {
                if (CHECK_PORT_BUFFER_SUPPLIER(pExynosPort)) {
                    bufferSupplier->eBufferSupplier = OMX_BufferSupplyOutput;
                } else if (CHECK_PORT_TUNNELED(pExynosPort)) {
                    bufferSupplier->eBufferSupplier = OMX_BufferSupplyInput;
                } else {
                    bufferSupplier->eBufferSupplier = OMX_BufferSupplyUnspecified;
                }
            }
        }
        else
        {
            ret = OMX_ErrorIncorrectStateOperation;
            goto EXIT;
        }
    }
        break;
    default:
    {
        ret = OMX_ErrorUnsupportedIndex;
        goto EXIT;
    }
        break;
    }

    ret = OMX_ErrorNone;

EXIT:

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_SetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        ComponentParameterStructure)
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

    if (ComponentParameterStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    switch (nIndex) {
    case OMX_IndexParamAudioInit:
    case OMX_IndexParamVideoInit:
    case OMX_IndexParamImageInit:
    case OMX_IndexParamOtherInit:
    {
        OMX_PORT_PARAM_TYPE *portParam = (OMX_PORT_PARAM_TYPE *)ComponentParameterStructure;
        ret = Exynos_OMX_Check_SizeVersion(portParam, sizeof(OMX_PORT_PARAM_TYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if ((pExynosComponent->currentState != OMX_StateLoaded) &&
            (pExynosComponent->currentState != OMX_StateWaitForResources)) {
            ret = OMX_ErrorIncorrectStateOperation;
            goto EXIT;
        }
        ret = OMX_ErrorUndefined;
        /* Exynos_OSAL_Memcpy(&pExynosComponent->portParam, portParam, sizeof(OMX_PORT_PARAM_TYPE)); */
    }
        break;
    case OMX_IndexParamPortDefinition:
    {
        OMX_PARAM_PORTDEFINITIONTYPE *portDefinition = (OMX_PARAM_PORTDEFINITIONTYPE *)ComponentParameterStructure;
        OMX_U32                       portIndex = portDefinition->nPortIndex;
        EXYNOS_OMX_BASEPORT          *pExynosPort;

        if (portIndex >= pExynosComponent->portParam.nPorts) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }
        ret = Exynos_OMX_Check_SizeVersion(portDefinition, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
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
        if (portDefinition->nBufferCountActual < pExynosPort->portDefinition.nBufferCountMin) {
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }

        Exynos_OSAL_Memcpy(&pExynosPort->portDefinition, portDefinition, portDefinition->nSize);
    }
        break;
    case OMX_IndexParamPriorityMgmt:
    {
        OMX_PRIORITYMGMTTYPE *compPriority = (OMX_PRIORITYMGMTTYPE *)ComponentParameterStructure;

        if ((pExynosComponent->currentState != OMX_StateLoaded) &&
            (pExynosComponent->currentState != OMX_StateWaitForResources)) {
            ret = OMX_ErrorIncorrectStateOperation;
            goto EXIT;
        }

        ret = Exynos_OMX_Check_SizeVersion(compPriority, sizeof(OMX_PRIORITYMGMTTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        pExynosComponent->compPriority.nGroupID = compPriority->nGroupID;
        pExynosComponent->compPriority.nGroupPriority = compPriority->nGroupPriority;
    }
        break;
    case OMX_IndexParamCompBufferSupplier:
    {
        OMX_PARAM_BUFFERSUPPLIERTYPE *bufferSupplier = (OMX_PARAM_BUFFERSUPPLIERTYPE *)ComponentParameterStructure;
        OMX_U32           portIndex = bufferSupplier->nPortIndex;
        EXYNOS_OMX_BASEPORT *pExynosPort = NULL;


        if (portIndex >= pExynosComponent->portParam.nPorts) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }
        ret = Exynos_OMX_Check_SizeVersion(bufferSupplier, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE));
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

        if (bufferSupplier->eBufferSupplier == OMX_BufferSupplyUnspecified) {
            ret = OMX_ErrorNone;
            goto EXIT;
        }
        if (CHECK_PORT_TUNNELED(pExynosPort) == 0) {
            ret = OMX_ErrorNone; /*OMX_ErrorNone ?????*/
            goto EXIT;
        }

        if (pExynosPort->portDefinition.eDir == OMX_DirInput) {
            if (bufferSupplier->eBufferSupplier == OMX_BufferSupplyInput) {
                /*
                if (CHECK_PORT_BUFFER_SUPPLIER(pExynosPort)) {
                    ret = OMX_ErrorNone;
                }
                */
                pExynosPort->tunnelFlags |= EXYNOS_TUNNEL_IS_SUPPLIER;
                bufferSupplier->nPortIndex = pExynosPort->tunneledPort;
                ret = OMX_SetParameter(pExynosPort->tunneledComponent, OMX_IndexParamCompBufferSupplier, bufferSupplier);
                goto EXIT;
            } else if (bufferSupplier->eBufferSupplier == OMX_BufferSupplyOutput) {
                ret = OMX_ErrorNone;
                if (CHECK_PORT_BUFFER_SUPPLIER(pExynosPort)) {
                    pExynosPort->tunnelFlags &= ~EXYNOS_TUNNEL_IS_SUPPLIER;
                    bufferSupplier->nPortIndex = pExynosPort->tunneledPort;
                    ret = OMX_SetParameter(pExynosPort->tunneledComponent, OMX_IndexParamCompBufferSupplier, bufferSupplier);
                }
                goto EXIT;
            }
        } else if (pExynosPort->portDefinition.eDir == OMX_DirOutput) {
            if (bufferSupplier->eBufferSupplier == OMX_BufferSupplyInput) {
                ret = OMX_ErrorNone;
                if (CHECK_PORT_BUFFER_SUPPLIER(pExynosPort)) {
                    pExynosPort->tunnelFlags &= ~EXYNOS_TUNNEL_IS_SUPPLIER;
                    ret = OMX_ErrorNone;
                }
                goto EXIT;
            } else if (bufferSupplier->eBufferSupplier == OMX_BufferSupplyOutput) {
                /*
                if (CHECK_PORT_BUFFER_SUPPLIER(pExynosPort)) {
                    ret = OMX_ErrorNone;
                }
                */
                pExynosPort->tunnelFlags |= EXYNOS_TUNNEL_IS_SUPPLIER;
                ret = OMX_ErrorNone;
                goto EXIT;
            }
        }
    }
        break;
    default:
    {
        ret = OMX_ErrorUnsupportedIndex;
        goto EXIT;
    }
        break;
    }

    ret = OMX_ErrorNone;

EXIT:

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_GetConfig(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_INOUT OMX_PTR     pComponentConfigStructure)
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
        ret = OMX_ErrorUnsupportedIndex;
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_SetConfig(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        pComponentConfigStructure)
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
        ret = OMX_ErrorUnsupportedIndex;
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_GetExtensionIndex(
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

    ret = OMX_ErrorBadParameter;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_SetCallbacks (
    OMX_IN OMX_HANDLETYPE    hComponent,
    OMX_IN OMX_CALLBACKTYPE* pCallbacks,
    OMX_IN OMX_PTR           pAppData)
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

    if (pCallbacks == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }
    if (pExynosComponent->currentState != OMX_StateLoaded) {
        ret = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    pExynosComponent->pCallbacks = pCallbacks;
    pExynosComponent->callbackData = pAppData;

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_UseEGLImage(
    OMX_IN OMX_HANDLETYPE            hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE **ppBufferHdr,
    OMX_IN OMX_U32                   nPortIndex,
    OMX_IN OMX_PTR                   pAppPrivate,
    OMX_IN void                     *eglImage)
{
    return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE Exynos_OMX_BaseComponent_Constructor(
    OMX_IN OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorBadParameter, Line:%d", __LINE__);
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    pExynosComponent = Exynos_OSAL_Malloc(sizeof(EXYNOS_OMX_BASECOMPONENT));
    if (pExynosComponent == NULL) {
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    Exynos_OSAL_Memset(pExynosComponent, 0, sizeof(EXYNOS_OMX_BASECOMPONENT));
    pOMXComponent->pComponentPrivate = (OMX_PTR)pExynosComponent;

    ret = Exynos_OSAL_SemaphoreCreate(&pExynosComponent->msgSemaphoreHandle);
    if (ret != OMX_ErrorNone) {
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    ret = Exynos_OSAL_MutexCreate(&pExynosComponent->compMutex);
    if (ret != OMX_ErrorNone) {
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    ret = Exynos_OSAL_SignalCreate(&pExynosComponent->abendStateEvent);
    if (ret != OMX_ErrorNone) {
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }

    pExynosComponent->bExitMessageHandlerThread = OMX_FALSE;
    Exynos_OSAL_QueueCreate(&pExynosComponent->messageQ, MAX_QUEUE_ELEMENTS);
    ret = Exynos_OSAL_ThreadCreate(&pExynosComponent->hMessageHandler, Exynos_OMX_MessageHandlerThread, pOMXComponent);
    if (ret != OMX_ErrorNone) {
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }

    pExynosComponent->bMultiThreadProcess = OMX_FALSE;

    pOMXComponent->GetComponentVersion = &Exynos_OMX_GetComponentVersion;
    pOMXComponent->SendCommand         = &Exynos_OMX_SendCommand;
    pOMXComponent->GetState            = &Exynos_OMX_GetState;
    pOMXComponent->SetCallbacks        = &Exynos_OMX_SetCallbacks;
    pOMXComponent->UseEGLImage         = &Exynos_OMX_UseEGLImage;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_BaseComponent_Destructor(
    OMX_IN OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;
    OMX_S32                   semaValue = 0;

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

    Exynos_OMX_CommandQueue(pExynosComponent, EXYNOS_OMX_CommandComponentDeInit, 0, NULL);
    Exynos_OSAL_SleepMillisec(0);
    Exynos_OSAL_Get_SemaphoreCount(pExynosComponent->msgSemaphoreHandle, &semaValue);
    if (semaValue == 0)
        Exynos_OSAL_SemaphorePost(pExynosComponent->msgSemaphoreHandle);
    Exynos_OSAL_SemaphorePost(pExynosComponent->msgSemaphoreHandle);

    Exynos_OSAL_ThreadTerminate(pExynosComponent->hMessageHandler);
    pExynosComponent->hMessageHandler = NULL;

    Exynos_OSAL_SignalTerminate(pExynosComponent->abendStateEvent);
    pExynosComponent->abendStateEvent = NULL;
    Exynos_OSAL_MutexTerminate(pExynosComponent->compMutex);
    pExynosComponent->compMutex = NULL;
    Exynos_OSAL_SemaphoreTerminate(pExynosComponent->msgSemaphoreHandle);
    pExynosComponent->msgSemaphoreHandle = NULL;
    Exynos_OSAL_QueueTerminate(&pExynosComponent->messageQ);

    Exynos_OSAL_Free(pExynosComponent);
    pExynosComponent = NULL;

    ret = OMX_ErrorNone;
EXIT:
    FunctionOut();

    return ret;
}


