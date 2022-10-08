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
 * @file        Exynos_OSAL_Thread.c
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
#include <errno.h>
#include <time.h>
#include <unistd.h>

#include "Exynos_OSAL_Memory.h"
#include "Exynos_OSAL_Thread.h"

#undef EXYNOS_LOG_TAG
#define EXYNOS_LOG_TAG    "EXYNOS_LOG_THREAD"
#define EXYNOS_LOG_OFF
#include "Exynos_OSAL_Log.h"


typedef struct _EXYNOS_THREAD_HANDLE_TYPE
{
    pthread_t          pthread;
    pthread_attr_t     attr;
    struct sched_param schedparam;
    int                stack_size;
} EXYNOS_THREAD_HANDLE_TYPE;


OMX_ERRORTYPE Exynos_OSAL_ThreadCreate(OMX_HANDLETYPE *threadHandle, OMX_PTR function_name, OMX_PTR argument)
{
    FunctionIn();

    int result = 0;
    int detach_ret = 0;
    EXYNOS_THREAD_HANDLE_TYPE *thread;
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    thread = Exynos_OSAL_Malloc(sizeof(EXYNOS_THREAD_HANDLE_TYPE));
    Exynos_OSAL_Memset(thread, 0, sizeof(EXYNOS_THREAD_HANDLE_TYPE));

    pthread_attr_init(&thread->attr);
    if (thread->stack_size != 0)
        pthread_attr_setstacksize(&thread->attr, thread->stack_size);

    /* set priority */
    if (thread->schedparam.sched_priority != 0)
        pthread_attr_setschedparam(&thread->attr, &thread->schedparam);

    detach_ret = pthread_attr_setdetachstate(&thread->attr, PTHREAD_CREATE_JOINABLE);
    if (detach_ret != 0) {
        Exynos_OSAL_Free(thread);
        *threadHandle = NULL;
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }

    result = pthread_create(&thread->pthread, &thread->attr, function_name, (void *)argument);
    /* pthread_setschedparam(thread->pthread, SCHED_RR, &thread->schedparam); */

    switch (result) {
    case 0:
        *threadHandle = (OMX_HANDLETYPE)thread;
        ret = OMX_ErrorNone;
        break;
    case EAGAIN:
        Exynos_OSAL_Free(thread);
        *threadHandle = NULL;
        ret = OMX_ErrorInsufficientResources;
        break;
    default:
        Exynos_OSAL_Free(thread);
        *threadHandle = NULL;
        ret = OMX_ErrorUndefined;
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OSAL_ThreadTerminate(OMX_HANDLETYPE threadHandle)
{
    FunctionIn();

    OMX_ERRORTYPE ret = OMX_ErrorNone;
    EXYNOS_THREAD_HANDLE_TYPE *thread = (EXYNOS_THREAD_HANDLE_TYPE *)threadHandle;

    if (!thread) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pthread_join(thread->pthread, NULL) != 0) {
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }

    Exynos_OSAL_Free(thread);
    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OSAL_ThreadCancel(OMX_HANDLETYPE threadHandle)
{
    EXYNOS_THREAD_HANDLE_TYPE *thread = (EXYNOS_THREAD_HANDLE_TYPE *)threadHandle;

    if (!thread)
        return OMX_ErrorBadParameter;

    /* thread_cancel(thread->pthread); */
    pthread_exit(&thread->pthread);
    pthread_join(thread->pthread, NULL);

    Exynos_OSAL_Free(thread);
    return OMX_ErrorNone;
}

void Exynos_OSAL_ThreadExit(void *value_ptr)
{
    pthread_exit(value_ptr);
    return;
}

void Exynos_OSAL_SleepMillisec(OMX_U32 ms)
{
    usleep(ms * 1000);
    return;
}
