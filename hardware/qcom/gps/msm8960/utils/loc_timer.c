/* Copyright (c) 2013, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation, nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include<stdio.h>
#include<stdlib.h>
#include<sys/time.h>
#include "loc_timer.h"
#include<time.h>
#include<errno.h>

#define MAX_DELAY_RETRIES 3

typedef struct {
    loc_timer_callback callback_func;
    void *user_data;
    unsigned int time_msec;
}timer_data;

static void *timer_thread(void *thread_data)
{
    int ret;
    unsigned char retries=0;
    struct timespec ts;
    struct timeval tv;
    timer_data t;
    t.callback_func = ((timer_data *)thread_data)->callback_func;
    t.user_data = ((timer_data *)thread_data)->user_data;
    t.time_msec = ((timer_data *)thread_data)->time_msec;
    pthread_cond_t timer_cond;
    pthread_mutex_t timer_mutex;

    LOC_LOGD("%s:%d]: Enter. Delay = %d\n", __func__, __LINE__, t.time_msec);
    //Copied over all info into local variable. Do not need allocated struct
    free(thread_data);

    if(pthread_cond_init(&timer_cond, NULL)) {
        LOC_LOGE("%s:%d]: Pthread cond init failed\n", __func__, __LINE__);
        ret = -1;
        goto err;
    }
    if(pthread_mutex_init(&timer_mutex, NULL)) {
        LOC_LOGE("%s:%d]: Pthread mutex init failed\n", __func__, __LINE__);
        ret = -1;
        goto mutex_err;
    }
    while(retries < MAX_DELAY_RETRIES) {
        gettimeofday(&tv, NULL);
        clock_gettime(CLOCK_REALTIME, &ts);
        if(t.time_msec >= 1000) {
            ts.tv_sec += t.time_msec/1000;
            t.time_msec = t.time_msec % 1000;
        }
        if(t.time_msec)
            ts.tv_nsec += t.time_msec * 1000000;
        if(ts.tv_nsec > 999999999) {
            LOC_LOGD("%s:%d]: Large nanosecs\n", __func__, __LINE__);
            ts.tv_sec += 1;
            ts.tv_nsec -= 1000000000;
        }
        LOC_LOGD("%s:%d]: ts.tv_sec:%d; ts.tv_nsec:%d\n",
                 __func__, __LINE__, (int)ts.tv_sec, (int)ts.tv_nsec);
        LOC_LOGD("%s:%d]: Current time: %d sec; %d nsec\n",
                 __func__, __LINE__, (int)tv.tv_sec, (int)tv.tv_usec*1000);
        pthread_mutex_lock(&(timer_mutex));
        ret = pthread_cond_timedwait(&timer_cond, &timer_mutex, &ts);
        pthread_mutex_unlock(&(timer_mutex));
        if(ret != ETIMEDOUT) {
            LOC_LOGE("%s:%d]: Call to pthread timedwait failed; ret=%d\n",
                     __func__, __LINE__,ret);
            ret = -1;
            retries++;
        }
        else {
            ret = 0;
            break;
        }
    }

    pthread_mutex_destroy(&timer_mutex);
mutex_err:
    pthread_cond_destroy(&timer_cond);
err:
    if(!ret)
        t.callback_func(t.user_data, ret);
    LOC_LOGD("%s:%d]: Exit\n", __func__, __LINE__);
    return NULL;
}

int loc_timer_start(unsigned int msec, loc_timer_callback cb_func,
                    void* caller_data)
{
    int ret=0;
    timer_data *t=NULL;
    pthread_attr_t tattr;
    pthread_t id;
    LOC_LOGD("%s:%d]: Enter\n", __func__, __LINE__);
    if(cb_func == NULL || msec == 0) {
        LOC_LOGE("%s:%d]: Error: Wrong parameters\n", __func__, __LINE__);
        ret = -1;
        goto err;
    }
    t = (timer_data *)calloc(1, sizeof(timer_data));
    if(t == NULL) {
        LOC_LOGE("%s:%d]: Could not allocate memory. Failing.\n",
                 __func__, __LINE__);
        ret = -1;
        goto err;
    }

    t->callback_func = cb_func;
    t->user_data = caller_data;
    t->time_msec = msec;

    pthread_attr_init(&tattr);
    pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
    if(pthread_create(&(id), &tattr, timer_thread, (void *)t)) {
        LOC_LOGE("%s:%d]: Could not create thread\n", __func__, __LINE__);
        ret = -1;
        goto attr_err;
    }
    else {
        LOC_LOGD("%s:%d]: Created thread with id: %d\n",
                 __func__, __LINE__, (int)id);
    }

attr_err:
    pthread_attr_destroy(&tattr);
err:
    LOC_LOGD("%s:%d]: Exit\n", __func__, __LINE__);
    return ret;
}
