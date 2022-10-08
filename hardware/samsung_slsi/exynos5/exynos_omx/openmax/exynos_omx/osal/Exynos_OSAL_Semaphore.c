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
 * @file        Exynos_OSAL_Semaphore.c
 * @brief
 * @author      SeungBeom Kim (sbcrux.kim@samsung.com)
 * @version     2.0.0
 * @history
 *   2012.02.20 : Create
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#include "Exynos_OSAL_Memory.h"
#include "Exynos_OSAL_Semaphore.h"

#undef EXYNOS_LOG_TAG
#define EXYNOS_LOG_TAG    "EXYNOS_LOG_SEMA"
#define EXYNOS_LOG_OFF
#include "Exynos_OSAL_Log.h"


OMX_ERRORTYPE Exynos_OSAL_SemaphoreCreate(OMX_HANDLETYPE *semaphoreHandle)
{
    sem_t *sema;

    sema = (sem_t *)Exynos_OSAL_Malloc(sizeof(sem_t));
    if (!sema)
        return OMX_ErrorInsufficientResources;

    if (sem_init(sema, 0, 0) != 0) {
        Exynos_OSAL_Free(sema);
        return OMX_ErrorUndefined;
    }

    *semaphoreHandle = (OMX_HANDLETYPE)sema;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE Exynos_OSAL_SemaphoreTerminate(OMX_HANDLETYPE semaphoreHandle)
{
    sem_t *sema = (sem_t *)semaphoreHandle;

    if (sema == NULL)
        return OMX_ErrorBadParameter;

    if (sem_destroy(sema) != 0)
        return OMX_ErrorUndefined;

    Exynos_OSAL_Free(sema);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE Exynos_OSAL_SemaphoreWait(OMX_HANDLETYPE semaphoreHandle)
{
    sem_t *sema = (sem_t *)semaphoreHandle;

    FunctionIn();

    if (sema == NULL)
        return OMX_ErrorBadParameter;

    if (sem_wait(sema) != 0)
        return OMX_ErrorUndefined;

    FunctionOut();

    return OMX_ErrorNone;
}

OMX_ERRORTYPE Exynos_OSAL_SemaphorePost(OMX_HANDLETYPE semaphoreHandle)
{
    sem_t *sema = (sem_t *)semaphoreHandle;

    FunctionIn();

    if (sema == NULL)
        return OMX_ErrorBadParameter;

    if (sem_post(sema) != 0)
        return OMX_ErrorUndefined;

    FunctionOut();

    return OMX_ErrorNone;
}

OMX_ERRORTYPE Exynos_OSAL_Set_SemaphoreCount(OMX_HANDLETYPE semaphoreHandle, OMX_S32 val)
{
    sem_t *sema = (sem_t *)semaphoreHandle;

    if (sema == NULL)
        return OMX_ErrorBadParameter;

    if (sem_init(sema, 0, val) != 0)
        return OMX_ErrorUndefined;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE Exynos_OSAL_Get_SemaphoreCount(OMX_HANDLETYPE semaphoreHandle, OMX_S32 *val)
{
    sem_t *sema = (sem_t *)semaphoreHandle;
    int semaVal = 0;

    if (sema == NULL)
        return OMX_ErrorBadParameter;

    if (sem_getvalue(sema, &semaVal) != 0)
        return OMX_ErrorUndefined;

    *val = (OMX_S32)semaVal;

    return OMX_ErrorNone;
}
