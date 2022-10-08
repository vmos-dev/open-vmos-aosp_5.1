/* Copyright (c) 2011-2013, The Linux Foundation. All rights reserved.
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
#define LOG_NDDEBUG 0
#define LOG_TAG "LocSvc_CtxBase"

#include <dlfcn.h>
#include <cutils/sched_policy.h>
#include <unistd.h>
#include <ContextBase.h>
#include <msg_q.h>
#include <loc_target.h>
#include <log_util.h>
#include <loc_log.h>

namespace loc_core {


IzatProxyBase* ContextBase::getIzatProxy(const char* libName)
{
    IzatProxyBase* proxy = NULL;
    void* lib = dlopen(libName, RTLD_NOW);

    if ((void*)NULL != lib) {
        getIzatProxy_t* getter = (getIzatProxy_t*)dlsym(lib, "getIzatProxy");
        if (NULL != getter) {
            proxy = (*getter)();
        }
    }
    if (NULL == proxy) {
        proxy = new IzatProxyBase();
    }
    return proxy;
}

LocApiBase* ContextBase::createLocApi(LOC_API_ADAPTER_EVENT_MASK_T exMask)
{
    LocApiBase* locApi = NULL;

    // first if can not be MPQ
    if (TARGET_MPQ != get_target()) {
        if (NULL == (locApi = mIzatProxy->getLocApi(mMsgTask, exMask))) {
            void *handle = NULL;
            //try to see if LocApiV02 is present
            if((handle = dlopen("libloc_api_v02.so", RTLD_NOW)) != NULL) {
                LOC_LOGD("%s:%d]: libloc_api_v02.so is present", __func__, __LINE__);
                getLocApi_t* getter = (getLocApi_t*)dlsym(handle, "getLocApi");
                if(getter != NULL) {
                    LOC_LOGD("%s:%d]: getter is not NULL for LocApiV02", __func__, __LINE__);
                    locApi = (*getter)(mMsgTask,exMask);
                }
            }
            // only RPC is the option now
            else {
                LOC_LOGD("%s:%d]: libloc_api_v02.so is NOT present. Trying RPC",
                         __func__, __LINE__);
                handle = dlopen("libloc_api-rpc-qc.so", RTLD_NOW);
                if (NULL != handle) {
                    getLocApi_t* getter = (getLocApi_t*)dlsym(handle, "getLocApi");
                    if (NULL != getter) {
                        LOC_LOGD("%s:%d]: getter is not NULL in RPC", __func__, __LINE__);
                        locApi = (*getter)(mMsgTask, exMask);
                    }
                }
            }
        }
    }

    // locApi could still be NULL at this time
    // we would then create a dummy one
    if (NULL == locApi) {
        locApi = new LocApiBase(mMsgTask, exMask);
    }

    return locApi;
}

ContextBase::ContextBase(const MsgTask* msgTask,
                         LOC_API_ADAPTER_EVENT_MASK_T exMask,
                         const char* libName) :
    mIzatProxy(getIzatProxy(libName)),
    mMsgTask(msgTask),
    mLocApi(createLocApi(exMask))
{
}

}
