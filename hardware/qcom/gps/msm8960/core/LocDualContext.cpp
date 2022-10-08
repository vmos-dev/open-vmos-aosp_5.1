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
#define LOG_TAG "LocSvc_DualCtx"

#include <cutils/sched_policy.h>
#include <unistd.h>
#include <LocDualContext.h>
#include <msg_q.h>
#include <log_util.h>
#include <loc_log.h>

namespace loc_core {

// nothing exclude for foreground
const LOC_API_ADAPTER_EVENT_MASK_T
LocDualContext::mFgExclMask = 0;
// excluded events for background clients
const LOC_API_ADAPTER_EVENT_MASK_T
LocDualContext::mBgExclMask =
    (LOC_API_ADAPTER_BIT_SATELLITE_REPORT |
     LOC_API_ADAPTER_BIT_NMEA_1HZ_REPORT |
     LOC_API_ADAPTER_BIT_NMEA_POSITION_REPORT |
     LOC_API_ADAPTER_BIT_IOCTL_REPORT |
     LOC_API_ADAPTER_BIT_STATUS_REPORT |
     LOC_API_ADAPTER_BIT_GEOFENCE_GEN_ALERT);

const MsgTask* LocDualContext::mMsgTask = NULL;
ContextBase* LocDualContext::mFgContext = NULL;
ContextBase* LocDualContext::mBgContext = NULL;

// the name must be shorter than 15 chars
const char* LocDualContext::mLocationHalName = "Loc_hal_worker";
const char* LocDualContext::mIzatLibName = "libizat_core.so";

const MsgTask* LocDualContext::getMsgTask(MsgTask::tCreate tCreator,
                                          const char* name)
{
    if (NULL == mMsgTask) {
        mMsgTask = new MsgTask(tCreator, name);
    }
    return mMsgTask;
}

const MsgTask* LocDualContext::getMsgTask(MsgTask::tAssociate tAssociate,
                                          const char* name)
{
    if (NULL == mMsgTask) {
        mMsgTask = new MsgTask(tAssociate, name);
    }
    return mMsgTask;
}

ContextBase* LocDualContext::getLocFgContext(MsgTask::tCreate tCreator,
                                             const char* name)
{
    if (NULL == mFgContext) {
        const MsgTask* msgTask = getMsgTask(tCreator, name);
        mFgContext = new LocDualContext(msgTask,
                                        mFgExclMask);
    }
    return mFgContext;
}

ContextBase* LocDualContext::getLocFgContext(MsgTask::tAssociate tAssociate,
                                        const char* name)
{
    if (NULL == mFgContext) {
        const MsgTask* msgTask = getMsgTask(tAssociate, name);
        mFgContext = new LocDualContext(msgTask,
                                        mFgExclMask);
    }
    return mFgContext;

}

ContextBase* LocDualContext::getLocBgContext(MsgTask::tCreate tCreator,
                                             const char* name)
{
    if (NULL == mBgContext) {
        const MsgTask* msgTask = getMsgTask(tCreator, name);
        mBgContext = new LocDualContext(msgTask,
                                        mBgExclMask);
    }
    return mBgContext;
}

ContextBase* LocDualContext::getLocBgContext(MsgTask::tAssociate tAssociate,
                                             const char* name)
{
    if (NULL == mBgContext) {
        const MsgTask* msgTask = getMsgTask(tAssociate, name);
        mBgContext = new LocDualContext(msgTask,
                                        mBgExclMask);
    }
    return mBgContext;
}

LocDualContext::LocDualContext(const MsgTask* msgTask,
                               LOC_API_ADAPTER_EVENT_MASK_T exMask) :
    ContextBase(msgTask, exMask, mIzatLibName)
{
}

}
