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
 * @file        Exynos_OSAL_Queue.c
 * @brief
 * @author      SeungBeom Kim (sbcrux.kim@samsung.com)
 * @version     2.0.0
 * @history
 *   2012.02.20 : Create
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Exynos_OSAL_Memory.h"
#include "Exynos_OSAL_Mutex.h"
#include "Exynos_OSAL_Queue.h"


OMX_ERRORTYPE Exynos_OSAL_QueueCreate(EXYNOS_QUEUE *queueHandle, int maxNumElem)
{
    int i = 0;
    EXYNOS_QElem *newqelem = NULL;
    EXYNOS_QElem *currentqelem = NULL;
    EXYNOS_QUEUE *queue = (EXYNOS_QUEUE *)queueHandle;

    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if (!queue)
        return OMX_ErrorBadParameter;

    ret = Exynos_OSAL_MutexCreate(&queue->qMutex);
    if (ret != OMX_ErrorNone)
        return ret;

    queue->first = (EXYNOS_QElem *)Exynos_OSAL_Malloc(sizeof(EXYNOS_QElem));
    if (queue->first == NULL)
        return OMX_ErrorInsufficientResources;

    Exynos_OSAL_Memset(queue->first, 0, sizeof(EXYNOS_QElem));
    currentqelem = queue->last = queue->first;
    queue->numElem = 0;
    queue->maxNumElem = maxNumElem;
    for (i = 0; i < (queue->maxNumElem - 2); i++) {
        newqelem = (EXYNOS_QElem *)Exynos_OSAL_Malloc(sizeof(EXYNOS_QElem));
        if (newqelem == NULL) {
            while (queue->first != NULL) {
                currentqelem = queue->first->qNext;
                Exynos_OSAL_Free((OMX_PTR)queue->first);
                queue->first = currentqelem;
            }
            return OMX_ErrorInsufficientResources;
        } else {
            Exynos_OSAL_Memset(newqelem, 0, sizeof(EXYNOS_QElem));
            currentqelem->qNext = newqelem;
            currentqelem = newqelem;
        }
    }

    currentqelem->qNext = queue->first;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE Exynos_OSAL_QueueTerminate(EXYNOS_QUEUE *queueHandle)
{
    int i = 0;
    EXYNOS_QElem *currentqelem = NULL;
    EXYNOS_QUEUE *queue = (EXYNOS_QUEUE *)queueHandle;
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if (!queue)
        return OMX_ErrorBadParameter;

    for ( i = 0; i < (queue->maxNumElem - 2); i++) {
        currentqelem = queue->first->qNext;
        Exynos_OSAL_Free(queue->first);
        queue->first = currentqelem;
    }

    if(queue->first) {
        Exynos_OSAL_Free(queue->first);
        queue->first = NULL;
    }

    ret = Exynos_OSAL_MutexTerminate(queue->qMutex);

    return ret;
}

int Exynos_OSAL_Queue(EXYNOS_QUEUE *queueHandle, void *data)
{
    EXYNOS_QUEUE *queue = (EXYNOS_QUEUE *)queueHandle;
    if (queue == NULL)
        return -1;

    Exynos_OSAL_MutexLock(queue->qMutex);

    if ((queue->last->data != NULL) || (queue->numElem >= queue->maxNumElem)) {
        Exynos_OSAL_MutexUnlock(queue->qMutex);
        return -1;
    }
    queue->last->data = data;
    queue->last = queue->last->qNext;
    queue->numElem++;

    Exynos_OSAL_MutexUnlock(queue->qMutex);
    return 0;
}

void *Exynos_OSAL_Dequeue(EXYNOS_QUEUE *queueHandle)
{
    void *data = NULL;
    EXYNOS_QUEUE *queue = (EXYNOS_QUEUE *)queueHandle;
    if (queue == NULL)
        return NULL;

    Exynos_OSAL_MutexLock(queue->qMutex);

    if ((queue->first->data == NULL) || (queue->numElem <= 0)) {
        Exynos_OSAL_MutexUnlock(queue->qMutex);
        return NULL;
    }
    data = queue->first->data;
    queue->first->data = NULL;
    queue->first = queue->first->qNext;
    queue->numElem--;

    Exynos_OSAL_MutexUnlock(queue->qMutex);
    return data;
}

int Exynos_OSAL_GetElemNum(EXYNOS_QUEUE *queueHandle)
{
    int ElemNum = 0;
    EXYNOS_QUEUE *queue = (EXYNOS_QUEUE *)queueHandle;
    if (queue == NULL)
        return -1;

    Exynos_OSAL_MutexLock(queue->qMutex);
    ElemNum = queue->numElem;
    Exynos_OSAL_MutexUnlock(queue->qMutex);
    return ElemNum;
}

int Exynos_OSAL_SetElemNum(EXYNOS_QUEUE *queueHandle, int ElemNum)
{
    EXYNOS_QUEUE *queue = (EXYNOS_QUEUE *)queueHandle;
    if (queue == NULL)
        return -1;

    Exynos_OSAL_MutexLock(queue->qMutex);
    queue->numElem = ElemNum;
    Exynos_OSAL_MutexUnlock(queue->qMutex);
    return ElemNum;
}

int Exynos_OSAL_ResetQueue(EXYNOS_QUEUE *queueHandle)
{
    EXYNOS_QUEUE *queue = (EXYNOS_QUEUE *)queueHandle;
    EXYNOS_QElem *currentqelem = NULL;

    if (queue == NULL)
        return -1;

    Exynos_OSAL_MutexLock(queue->qMutex);
    queue->first->data = NULL;
    currentqelem = queue->first->qNext;
    while (currentqelem != queue->first) {
        currentqelem->data = NULL;
        currentqelem = currentqelem->qNext;
    }
    queue->last = queue->first;
    queue->numElem = 0x00;
    Exynos_OSAL_MutexUnlock(queue->qMutex);

    return 0;
}
