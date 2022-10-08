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
 * @file       Exynos_OMX_Resourcemanager.c
 * @brief
 * @author     SeungBeom Kim (sbcrux.kim@samsung.com)
 * @version    2.0.0
 * @history
 *    2012.02.20 : Create
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Exynos_OMX_Resourcemanager.h"
#include "Exynos_OMX_Basecomponent.h"
#include "Exynos_OSAL_Memory.h"
#include "Exynos_OSAL_Mutex.h"

#undef  EXYNOS_LOG_TAG
#define EXYNOS_LOG_TAG    "EXYNOS_RM"
#define EXYNOS_LOG_OFF
#include "Exynos_OSAL_Log.h"


#define MAX_RESOURCE_VIDEO_DEC 3 /* for Android */
#define MAX_RESOURCE_VIDEO_ENC 2 /* for Android */

/* Max allowable video scheduler component instance */
static EXYNOS_OMX_RM_COMPONENT_LIST *gpVideoDecRMComponentList = NULL;
static EXYNOS_OMX_RM_COMPONENT_LIST *gpVideoDecRMWaitingList = NULL;
static EXYNOS_OMX_RM_COMPONENT_LIST *gpVideoEncRMComponentList = NULL;
static EXYNOS_OMX_RM_COMPONENT_LIST *gpVideoEncRMWaitingList = NULL;
static OMX_HANDLETYPE ghVideoRMComponentListMutex = NULL;


OMX_ERRORTYPE addElementList(EXYNOS_OMX_RM_COMPONENT_LIST **ppList, OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE                 ret = OMX_ErrorNone;
    EXYNOS_OMX_RM_COMPONENT_LIST *pTempComp = NULL;
    EXYNOS_OMX_BASECOMPONENT     *pExynosComponent = NULL;

    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (*ppList != NULL) {
        pTempComp = *ppList;
        while (pTempComp->pNext != NULL) {
            pTempComp = pTempComp->pNext;
        }
        pTempComp->pNext = (EXYNOS_OMX_RM_COMPONENT_LIST *)Exynos_OSAL_Malloc(sizeof(EXYNOS_OMX_RM_COMPONENT_LIST));
        if (pTempComp->pNext == NULL) {
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }
        ((EXYNOS_OMX_RM_COMPONENT_LIST *)(pTempComp->pNext))->pNext = NULL;
        ((EXYNOS_OMX_RM_COMPONENT_LIST *)(pTempComp->pNext))->pOMXStandComp = pOMXComponent;
        ((EXYNOS_OMX_RM_COMPONENT_LIST *)(pTempComp->pNext))->groupPriority = pExynosComponent->compPriority.nGroupPriority;
        goto EXIT;
    } else {
        *ppList = (EXYNOS_OMX_RM_COMPONENT_LIST *)Exynos_OSAL_Malloc(sizeof(EXYNOS_OMX_RM_COMPONENT_LIST));
        if (*ppList == NULL) {
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }
        pTempComp = *ppList;
        pTempComp->pNext = NULL;
        pTempComp->pOMXStandComp = pOMXComponent;
        pTempComp->groupPriority = pExynosComponent->compPriority.nGroupPriority;
    }

EXIT:
    return ret;
}

OMX_ERRORTYPE removeElementList(EXYNOS_OMX_RM_COMPONENT_LIST **ppList, OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE                 ret = OMX_ErrorNone;
    EXYNOS_OMX_RM_COMPONENT_LIST *pCurrComp = NULL;
    EXYNOS_OMX_RM_COMPONENT_LIST *pPrevComp = NULL;
    OMX_BOOL                      bDetectComp = OMX_FALSE;

    if (*ppList == NULL) {
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }

    pCurrComp = *ppList;
    while (pCurrComp != NULL) {
        if (pCurrComp->pOMXStandComp == pOMXComponent) {
            if (*ppList == pCurrComp) {
                *ppList = pCurrComp->pNext;
                Exynos_OSAL_Free(pCurrComp);
            } else {
                if (pPrevComp != NULL)
                    pPrevComp->pNext = pCurrComp->pNext;

                Exynos_OSAL_Free(pCurrComp);
            }
            bDetectComp = OMX_TRUE;
            break;
        } else {
            pPrevComp = pCurrComp;
            pCurrComp = pCurrComp->pNext;
        }
    }

    if (bDetectComp == OMX_FALSE)
        ret = OMX_ErrorComponentNotFound;
    else
        ret = OMX_ErrorNone;

EXIT:
    return ret;
}

int searchLowPriority(EXYNOS_OMX_RM_COMPONENT_LIST *RMComp_list, OMX_U32 inComp_priority, EXYNOS_OMX_RM_COMPONENT_LIST **outLowComp)
{
    int ret = 0;
    EXYNOS_OMX_RM_COMPONENT_LIST *pTempComp = NULL;
    EXYNOS_OMX_RM_COMPONENT_LIST *pCandidateComp = NULL;

    if (RMComp_list == NULL)
        ret = -1;

    pTempComp = RMComp_list;
    *outLowComp = 0;

    while (pTempComp != NULL) {
        if (pTempComp->groupPriority > inComp_priority) {
            if (pCandidateComp != NULL) {
                if (pCandidateComp->groupPriority < pTempComp->groupPriority)
                    pCandidateComp = pTempComp;
            } else {
                pCandidateComp = pTempComp;
            }
        }

        pTempComp = pTempComp->pNext;
    }

    *outLowComp = pCandidateComp;
    if (pCandidateComp == NULL)
        ret = 0;
    else
        ret = 1;

EXIT:
    return ret;
}

OMX_ERRORTYPE removeComponent(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;

    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pExynosComponent->currentState == OMX_StateIdle) {
        (*(pExynosComponent->pCallbacks->EventHandler))
            (pOMXComponent, pExynosComponent->callbackData,
            OMX_EventError, OMX_ErrorResourcesLost, 0, NULL);
        ret = OMX_SendCommand(pOMXComponent, OMX_CommandStateSet, OMX_StateLoaded, NULL);
        if (ret != OMX_ErrorNone) {
            ret = OMX_ErrorUndefined;
            goto EXIT;
        }
    } else if ((pExynosComponent->currentState == OMX_StateExecuting) || (pExynosComponent->currentState == OMX_StatePause)) {
        /* Todo */
    }

    ret = OMX_ErrorNone;

EXIT:
    return ret;
}


OMX_ERRORTYPE Exynos_OMX_ResourceManager_Init()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    FunctionIn();
    ret = Exynos_OSAL_MutexCreate(&ghVideoRMComponentListMutex);
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_ResourceManager_Deinit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    EXYNOS_OMX_RM_COMPONENT_LIST *pCurrComponent;
    EXYNOS_OMX_RM_COMPONENT_LIST *pNextComponent;

    FunctionIn();

    Exynos_OSAL_MutexLock(ghVideoRMComponentListMutex);

    if (gpVideoDecRMComponentList) {
        pCurrComponent = gpVideoDecRMComponentList;
        while (pCurrComponent != NULL) {
            pNextComponent = pCurrComponent->pNext;
            Exynos_OSAL_Free(pCurrComponent);
            pCurrComponent = pNextComponent;
        }
        gpVideoDecRMComponentList = NULL;
    }
    if (gpVideoDecRMWaitingList) {
        pCurrComponent = gpVideoDecRMWaitingList;
        while (pCurrComponent != NULL) {
            pNextComponent = pCurrComponent->pNext;
            Exynos_OSAL_Free(pCurrComponent);
            pCurrComponent = pNextComponent;
        }
        gpVideoDecRMWaitingList = NULL;
    }

    if (gpVideoEncRMComponentList) {
        pCurrComponent = gpVideoEncRMComponentList;
        while (pCurrComponent != NULL) {
            pNextComponent = pCurrComponent->pNext;
            Exynos_OSAL_Free(pCurrComponent);
            pCurrComponent = pNextComponent;
        }
        gpVideoEncRMComponentList = NULL;
    }
    if (gpVideoEncRMWaitingList) {
        pCurrComponent = gpVideoEncRMWaitingList;
        while (pCurrComponent != NULL) {
            pNextComponent = pCurrComponent->pNext;
            Exynos_OSAL_Free(pCurrComponent);
            pCurrComponent = pNextComponent;
        }
        gpVideoEncRMWaitingList = NULL;
    }

    Exynos_OSAL_MutexUnlock(ghVideoRMComponentListMutex);

    Exynos_OSAL_MutexTerminate(ghVideoRMComponentListMutex);
    ghVideoRMComponentListMutex = NULL;

    ret = OMX_ErrorNone;
EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_Get_Resource(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE                 ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT     *pExynosComponent = NULL;
    EXYNOS_OMX_RM_COMPONENT_LIST *pComponentTemp = NULL;
    EXYNOS_OMX_RM_COMPONENT_LIST *pComponentCandidate = NULL;
    int numElem = 0;
    int lowCompDetect = 0;

    FunctionIn();

    Exynos_OSAL_MutexLock(ghVideoRMComponentListMutex);

    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    if (pExynosComponent->codecType == HW_VIDEO_DEC_CODEC) {
        pComponentTemp = gpVideoDecRMComponentList;
        if (pComponentTemp != NULL) {
            while (pComponentTemp) {
                numElem++;
                pComponentTemp = pComponentTemp->pNext;
            }
        } else {
            numElem = 0;
        }
        if (numElem >= MAX_RESOURCE_VIDEO_DEC) {
            lowCompDetect = searchLowPriority(gpVideoDecRMComponentList, pExynosComponent->compPriority.nGroupPriority, &pComponentCandidate);
            if (lowCompDetect <= 0) {
                ret = OMX_ErrorInsufficientResources;
                goto EXIT;
            } else {
                ret = removeComponent(pComponentCandidate->pOMXStandComp);
                if (ret != OMX_ErrorNone) {
                    ret = OMX_ErrorInsufficientResources;
                    goto EXIT;
                } else {
                    ret = removeElementList(&gpVideoDecRMComponentList, pComponentCandidate->pOMXStandComp);
                    ret = addElementList(&gpVideoDecRMComponentList, pOMXComponent);
                    if (ret != OMX_ErrorNone) {
                        ret = OMX_ErrorInsufficientResources;
                        goto EXIT;
                    }
                }
            }
        } else {
            ret = addElementList(&gpVideoDecRMComponentList, pOMXComponent);
            if (ret != OMX_ErrorNone) {
                ret = OMX_ErrorInsufficientResources;
                goto EXIT;
            }
        }
    } else if (pExynosComponent->codecType == HW_VIDEO_ENC_CODEC) {
        pComponentTemp = gpVideoEncRMComponentList;
        if (pComponentTemp != NULL) {
            while (pComponentTemp) {
                numElem++;
                pComponentTemp = pComponentTemp->pNext;
            }
        } else {
            numElem = 0;
        }
        if (numElem >= MAX_RESOURCE_VIDEO_ENC) {
            lowCompDetect = searchLowPriority(gpVideoEncRMComponentList, pExynosComponent->compPriority.nGroupPriority, &pComponentCandidate);
            if (lowCompDetect <= 0) {
                ret = OMX_ErrorInsufficientResources;
                goto EXIT;
            } else {
                ret = removeComponent(pComponentCandidate->pOMXStandComp);
                if (ret != OMX_ErrorNone) {
                    ret = OMX_ErrorInsufficientResources;
                    goto EXIT;
                } else {
                    ret = removeElementList(&gpVideoEncRMComponentList, pComponentCandidate->pOMXStandComp);
                    ret = addElementList(&gpVideoEncRMComponentList, pOMXComponent);
                    if (ret != OMX_ErrorNone) {
                        ret = OMX_ErrorInsufficientResources;
                        goto EXIT;
                    }
                }
            }
        } else {
            ret = addElementList(&gpVideoEncRMComponentList, pOMXComponent);
            if (ret != OMX_ErrorNone) {
                ret = OMX_ErrorInsufficientResources;
                goto EXIT;
            }
        }
    }
    ret = OMX_ErrorNone;

EXIT:

    Exynos_OSAL_MutexUnlock(ghVideoRMComponentListMutex);

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_Release_Resource(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE                 ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT     *pExynosComponent = NULL;
    EXYNOS_OMX_RM_COMPONENT_LIST *pComponentTemp = NULL;
    OMX_COMPONENTTYPE            *pOMXWaitComponent = NULL;
    int numElem = 0;

    FunctionIn();

    Exynos_OSAL_MutexLock(ghVideoRMComponentListMutex);

    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    if (pExynosComponent->codecType == HW_VIDEO_DEC_CODEC) {
        pComponentTemp = gpVideoDecRMWaitingList;
        if (gpVideoDecRMComponentList == NULL) {
            ret = OMX_ErrorUndefined;
            goto EXIT;
        }

        ret = removeElementList(&gpVideoDecRMComponentList, pOMXComponent);
        if (ret != OMX_ErrorNone) {
            ret = OMX_ErrorUndefined;
            goto EXIT;
        }
        while (pComponentTemp) {
            numElem++;
            pComponentTemp = pComponentTemp->pNext;
        }
        if (numElem > 0) {
            pOMXWaitComponent = gpVideoDecRMWaitingList->pOMXStandComp;
            removeElementList(&gpVideoDecRMWaitingList, pOMXWaitComponent);
            ret = OMX_SendCommand(pOMXWaitComponent, OMX_CommandStateSet, OMX_StateIdle, NULL);
            if (ret != OMX_ErrorNone) {
                goto EXIT;
            }
        }
    } else if (pExynosComponent->codecType == HW_VIDEO_ENC_CODEC) {
        pComponentTemp = gpVideoEncRMWaitingList;
        if (gpVideoEncRMComponentList == NULL) {
            ret = OMX_ErrorUndefined;
            goto EXIT;
        }

        ret = removeElementList(&gpVideoEncRMComponentList, pOMXComponent);
        if (ret != OMX_ErrorNone) {
            ret = OMX_ErrorUndefined;
            goto EXIT;
        }
        while (pComponentTemp) {
            numElem++;
            pComponentTemp = pComponentTemp->pNext;
        }
        if (numElem > 0) {
            pOMXWaitComponent = gpVideoEncRMWaitingList->pOMXStandComp;
            removeElementList(&gpVideoEncRMWaitingList, pOMXWaitComponent);
            ret = OMX_SendCommand(pOMXWaitComponent, OMX_CommandStateSet, OMX_StateIdle, NULL);
            if (ret != OMX_ErrorNone) {
                goto EXIT;
            }
        }
    }

EXIT:

    Exynos_OSAL_MutexUnlock(ghVideoRMComponentListMutex);

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_In_WaitForResource(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;

    FunctionIn();

    Exynos_OSAL_MutexLock(ghVideoRMComponentListMutex);

    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pExynosComponent->codecType == HW_VIDEO_DEC_CODEC)
        ret = addElementList(&gpVideoDecRMWaitingList, pOMXComponent);
    else if (pExynosComponent->codecType == HW_VIDEO_ENC_CODEC)
        ret = addElementList(&gpVideoEncRMWaitingList, pOMXComponent);

    Exynos_OSAL_MutexUnlock(ghVideoRMComponentListMutex);

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_Out_WaitForResource(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;

    FunctionIn();

    Exynos_OSAL_MutexLock(ghVideoRMComponentListMutex);

    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pExynosComponent->codecType == HW_VIDEO_DEC_CODEC)
        ret = removeElementList(&gpVideoDecRMWaitingList, pOMXComponent);
    else if (pExynosComponent->codecType == HW_VIDEO_ENC_CODEC)
        ret = removeElementList(&gpVideoEncRMWaitingList, pOMXComponent);

    Exynos_OSAL_MutexUnlock(ghVideoRMComponentListMutex);

    FunctionOut();

    return ret;
}

