/*
// Copyright (c) 2014 Intel Corporation 
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
#ifndef HDCP_CONTROL_H
#define HDCP_CONTROL_H

#include <IHdcpControl.h>
#include <common/base/SimpleThread.h>

namespace android {
namespace intel {

class HdcpControl : public IHdcpControl {
public:
    HdcpControl();
    virtual ~HdcpControl();

public:
    virtual bool startHdcp();
    virtual bool startHdcpAsync(HdcpStatusCallback cb, void *userData);
    virtual bool stopHdcp();

protected:
    bool enableAuthentication();
    bool disableAuthentication();
    bool enableOverlay();
    bool disableOverlay();
    bool enableDisplayIED();
    bool disableDisplayIED();
    bool isHdcpSupported();
    bool checkAuthenticated();
    virtual bool preRunHdcp();
    virtual bool postRunHdcp();
    bool runHdcp();
    inline void signalCompletion();

private:
    enum {
        HDCP_INLOOP_RETRY_NUMBER = 1,
        HDCP_INLOOP_RETRY_DELAY_US = 50000,
        HDCP_VERIFICATION_DELAY_MS = 2000,
        HDCP_ASYNC_START_DELAY_MS = 100,
        HDCP_AUTHENTICATION_SHORT_DELAY_MS = 200,
        HDCP_AUTHENTICATION_LONG_DELAY_MS = 2000,
        HDCP_AUTHENTICATION_TIMEOUT_MS = 5000,
        HDCP_RETRY_LIMIT = 10,
    };

    enum {
        CALLBACK_PENDING,
        CALLBACK_AUTHENTICATED,
        CALLBACK_NOT_AUTHENTICATED,
    };

protected:
    HdcpStatusCallback mCallback;
    void *mUserData;
    int mCallbackState;
    Mutex mMutex;
    Condition mStoppedCondition;
    Condition mCompletedCondition;
    bool mWaitForCompletion;
    bool mStopped;
    bool mAuthenticated;
    int mActionDelay;  // in milliseconds
    uint32_t mAuthRetryCount;
    bool mEnableAuthenticationLog;

private:
    DECLARE_THREAD(HdcpControlThread, HdcpControl);
};

} // namespace intel
} // namespace android

#endif /* HDCP_CONTROL_H */
