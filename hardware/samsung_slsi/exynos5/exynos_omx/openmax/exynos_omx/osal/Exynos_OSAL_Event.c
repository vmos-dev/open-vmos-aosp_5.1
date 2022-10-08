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
 * @file        Exynos_OSAL_Event.c
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
#include <errno.h>

#include "Exynos_OSAL_Memory.h"
#include "Exynos_OSAL_Mutex.h"
#include "Exynos_OSAL_Event.h"

#undef  EXYNOS_LOG_TAG
#define EXYNOS_LOG_TAG    "Exynos_OSAL_EVENT"
#define EXYNOS_LOG_OFF
#include "Exynos_OSAL_Log.h"


OMX_ERRORTYPE Exynos_OSAL_SignalCreate(OMX_HANDLETYPE *eventHandle)
{
    Exynos_OSAL_THREADEVENT *event;
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    event = (Exynos_OSAL_THREADEVENT *)Exynos_OSAL_Malloc(sizeof(Exynos_OSAL_THREADEVENT));
    if (!event) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    Exynos_OSAL_Memset(event, 0, sizeof(Exynos_OSAL_THREADEVENT));
    event->signal = OMX_FALSE;

    ret = Exynos_OSAL_MutexCreate(&event->mutex);
    if (ret != OMX_ErrorNone) {
        Exynos_OSAL_Free(event);
        goto EXIT;
    }

    if (pthread_cond_init(&event->condition, NULL)) {
        Exynos_OSAL_MutexTerminate(event->mutex);
        Exynos_OSAL_Free(event);
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }

    *eventHandle = (OMX_HANDLETYPE)event;
    ret = OMX_ErrorNone;

EXIT:
    return ret;
}

OMX_ERRORTYPE Exynos_OSAL_SignalTerminate(OMX_HANDLETYPE eventHandle)
{
    Exynos_OSAL_THREADEVENT *event = (Exynos_OSAL_THREADEVENT *)eventHandle;
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if (!event) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    ret = Exynos_OSAL_MutexLock(event->mutex);
    if (ret != OMX_ErrorNone) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    if (pthread_cond_destroy(&event->condition)) {
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }

    ret = Exynos_OSAL_MutexUnlock(event->mutex);
    if (ret != OMX_ErrorNone) {
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }

    ret = Exynos_OSAL_MutexTerminate(event->mutex);
    if (ret != OMX_ErrorNone) {
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }

    Exynos_OSAL_Free(event);

EXIT:
    return ret;
}

OMX_ERRORTYPE Exynos_OSAL_SignalReset(OMX_HANDLETYPE eventHandle)
{
    Exynos_OSAL_THREADEVENT *event = (Exynos_OSAL_THREADEVENT *)eventHandle;
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if (!event) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    ret = Exynos_OSAL_MutexLock(event->mutex);
    if (ret != OMX_ErrorNone) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    event->signal = OMX_FALSE;

    Exynos_OSAL_MutexUnlock(event->mutex);

EXIT:
    return ret;
}

OMX_ERRORTYPE Exynos_OSAL_SignalSet(OMX_HANDLETYPE eventHandle)
{
    Exynos_OSAL_THREADEVENT *event = (Exynos_OSAL_THREADEVENT *)eventHandle;
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if (!event) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    ret = Exynos_OSAL_MutexLock(event->mutex);
    if (ret != OMX_ErrorNone) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    event->signal = OMX_TRUE;
    pthread_cond_signal(&event->condition);

    Exynos_OSAL_MutexUnlock(event->mutex);

EXIT:
    return ret;
}

OMX_ERRORTYPE Exynos_OSAL_SignalWait(OMX_HANDLETYPE eventHandle, OMX_U32 ms)
{
    Exynos_OSAL_THREADEVENT *event = (Exynos_OSAL_THREADEVENT *)eventHandle;
    OMX_ERRORTYPE         ret = OMX_ErrorNone;
    struct timespec       timeout;
    struct timeval        now;
    int                   funcret = 0;
    OMX_U32               tv_us;

    FunctionIn();

    if (!event) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    gettimeofday(&now, NULL);

    tv_us = now.tv_usec + ms * 1000;
    timeout.tv_sec = now.tv_sec + tv_us / 1000000;
    timeout.tv_nsec = (tv_us % 1000000) * 1000;

    ret = Exynos_OSAL_MutexLock(event->mutex);
    if (ret != OMX_ErrorNone) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    if (ms == 0) {
        if (!event->signal)
            ret = OMX_ErrorTimeout;
    } else if (ms == DEF_MAX_WAIT_TIME) {
        while (!event->signal)
            pthread_cond_wait(&event->condition, (pthread_mutex_t *)(event->mutex));
        ret = OMX_ErrorNone;
    } else {
        while (!event->signal) {
            funcret = pthread_cond_timedwait(&event->condition, (pthread_mutex_t *)(event->mutex), &timeout);
            if ((!event->signal) && (funcret == ETIMEDOUT)) {
                ret = OMX_ErrorTimeout;
                break;
            }
        }
    }

    Exynos_OSAL_MutexUnlock(event->mutex);

EXIT:
    FunctionOut();

    return ret;
}
