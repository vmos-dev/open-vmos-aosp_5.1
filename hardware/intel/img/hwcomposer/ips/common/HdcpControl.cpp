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
#include <common/utils/HwcTrace.h>
#include <DrmConfig.h>
#include <Hwcomposer.h>
#include <DisplayQuery.h>
#include <ips/common/DrmControl.h>
#include <ips/common/HdcpControl.h>
#include <cutils/properties.h>


namespace android {
namespace intel {

HdcpControl::HdcpControl()
    : mCallback(NULL),
      mUserData(NULL),
      mCallbackState(CALLBACK_PENDING),
      mMutex(),
      mStoppedCondition(),
      mCompletedCondition(),
      mWaitForCompletion(false),
      mStopped(true),
      mAuthenticated(false),
      mActionDelay(0),
      mAuthRetryCount(0),
      mEnableAuthenticationLog(true)
{
}

HdcpControl::~HdcpControl()
{
}

bool HdcpControl::startHdcp()
{
    // this is a blocking and synchronous call
    Mutex::Autolock lock(mMutex);

    char prop[PROPERTY_VALUE_MAX];
    if (property_get("debug.hwc.hdcp.enable", prop, "1") > 0) {
        if (atoi(prop) == 0) {
            WLOGTRACE("HDCP is disabled");
            return false;
        }
    }

    if (!mStopped) {
        WLOGTRACE("HDCP has been started");
        return true;
    }

    mStopped = false;
    mAuthenticated = false;
    mWaitForCompletion = false;

    mThread = new HdcpControlThread(this);
    if (!mThread.get()) {
        ELOGTRACE("failed to create hdcp control thread");
        return false;
    }

    if (!runHdcp()) {
        ELOGTRACE("failed to run HDCP");
        mStopped = true;
        mThread = NULL;
        return false;
    }

    mAuthRetryCount = 0;
    mWaitForCompletion = !mAuthenticated;
    if (mAuthenticated) {
        mActionDelay = HDCP_VERIFICATION_DELAY_MS;
    } else {
        mActionDelay = HDCP_AUTHENTICATION_SHORT_DELAY_MS;
    }

    mThread->run("HdcpControl", PRIORITY_NORMAL);

    if (!mWaitForCompletion) {
        // HDCP is authenticated.
        return true;
    }
    status_t err = mCompletedCondition.waitRelative(mMutex, milliseconds(HDCP_AUTHENTICATION_TIMEOUT_MS));
    if (err == -ETIMEDOUT) {
        WLOGTRACE("timeout waiting for completion");
    }
    mWaitForCompletion = false;
    return mAuthenticated;
}

bool HdcpControl::startHdcpAsync(HdcpStatusCallback cb, void *userData)
{
    char prop[PROPERTY_VALUE_MAX];
    if (property_get("debug.hwc.hdcp.enable", prop, "1") > 0) {
        if (atoi(prop) == 0) {
            WLOGTRACE("HDCP is disabled");
            return false;
        }
    }

    if (cb == NULL || userData == NULL) {
        ELOGTRACE("invalid callback or user data");
        return false;
    }

    Mutex::Autolock lock(mMutex);

    if (!mStopped) {
        WLOGTRACE("HDCP has been started");
        return true;
    }

    mThread = new HdcpControlThread(this);
    if (!mThread.get()) {
        ELOGTRACE("failed to create hdcp control thread");
        return false;
    }

    mAuthRetryCount = 0;
    mCallback = cb;
    mUserData = userData;
    mCallbackState = CALLBACK_PENDING;
    mWaitForCompletion = false;
    mAuthenticated = false;
    mStopped = false;
    mActionDelay = HDCP_ASYNC_START_DELAY_MS;
    mThread->run("HdcpControl", PRIORITY_NORMAL);

    return true;
}

bool HdcpControl::stopHdcp()
{
    do {
        Mutex::Autolock lock(mMutex);
        if (mStopped) {
            return true;
        }

        mStopped = true;
        mStoppedCondition.signal();

        mAuthenticated = false;
        mWaitForCompletion = false;
        mCallback = NULL;
        mUserData = NULL;
        disableAuthentication();
    } while (0);

    if (mThread.get()) {
        mThread->requestExitAndWait();
        mThread = NULL;
    }

    return true;
}

bool HdcpControl::enableAuthentication()
{
    int fd = Hwcomposer::getInstance().getDrm()->getDrmFd();
    int ret = drmCommandNone(fd, DRM_PSB_ENABLE_HDCP);
    if (ret != 0) {
        if (mEnableAuthenticationLog) {
            ELOGTRACE("failed to enable HDCP authentication");
        } else {
            VLOGTRACE("failed to enable HDCP authentication");
        }

        mEnableAuthenticationLog = false;
        return false;
    }

    mEnableAuthenticationLog = true;
    return true;
}

bool HdcpControl::disableAuthentication()
{
    int fd = Hwcomposer::getInstance().getDrm()->getDrmFd();
    int ret = drmCommandNone(fd, DRM_PSB_DISABLE_HDCP);
    if (ret != 0) {
        ELOGTRACE("failed to stop disable authentication");
        return false;
    }
    return true;
}

bool HdcpControl::enableOverlay()
{
    return true;
}

bool HdcpControl::disableOverlay()
{
    return true;
}

bool HdcpControl::enableDisplayIED()
{
    int fd = Hwcomposer::getInstance().getDrm()->getDrmFd();
    int ret = drmCommandNone(fd, DRM_PSB_HDCP_DISPLAY_IED_ON);
    if (ret != 0) {
        ELOGTRACE("failed to enable overlay IED");
        return false;
    }
    return true;
}

bool HdcpControl::disableDisplayIED()
{
    int fd = Hwcomposer::getInstance().getDrm()->getDrmFd();
    int ret = drmCommandNone(fd, DRM_PSB_HDCP_DISPLAY_IED_OFF);
    if (ret != 0) {
        ELOGTRACE("failed to disable overlay IED");
        return false;
    }
    return true;
}

bool HdcpControl::isHdcpSupported()
{
    int fd = Hwcomposer::getInstance().getDrm()->getDrmFd();
    unsigned int caps = 0;
    int ret = drmCommandRead(fd, DRM_PSB_QUERY_HDCP, &caps, sizeof(caps));
    if (ret != 0) {
        ELOGTRACE("failed to query HDCP capability");
        return false;
    }
    if (caps == 0) {
        WLOGTRACE("HDCP is not supported");
        return false;
    } else {
        ILOGTRACE("HDCP is supported");
        return true;
    }
}

bool HdcpControl::checkAuthenticated()
{
    int fd = Hwcomposer::getInstance().getDrm()->getDrmFd();
    unsigned int match = 0;
    int ret = drmCommandRead(fd, DRM_PSB_GET_HDCP_LINK_STATUS, &match, sizeof(match));
    if (ret != 0) {
        ELOGTRACE("failed to get hdcp link status");
        return false;
    }
    if (match) {
        VLOGTRACE("HDCP is authenticated");
        mAuthenticated = true;
    } else {
        ELOGTRACE("HDCP is not authenticated");
        mAuthenticated = false;
    }
    return mAuthenticated;
}

bool HdcpControl::runHdcp()
{
    // Default return value is true so HDCP can be re-authenticated in the working thread
    bool ret = true;

    preRunHdcp();

    for (int i = 0; i < HDCP_INLOOP_RETRY_NUMBER; i++) {
        VLOGTRACE("enable and verify HDCP, iteration# %d", i);
        if (mStopped) {
            WLOGTRACE("HDCP authentication has been stopped");
            ret = false;
            break;
        }

        if (!enableAuthentication()) {
            if (mAuthenticated)
                ELOGTRACE("HDCP authentication failed. Retry");
            else
                VLOGTRACE("HDCP authentication failed. Retry");

            mAuthenticated = false;
            ret = true;
        } else {
            ILOGTRACE("HDCP is authenticated");
            mAuthenticated = true;
            ret = true;
            break;
        }

        if (mStopped) {
            WLOGTRACE("HDCP authentication has been stopped");
            ret = false;
            break;
        }

        if (i < HDCP_INLOOP_RETRY_NUMBER - 1) {
            // Adding delay to make sure panel receives video signal so it can start HDCP authentication.
            // (HDCP spec 1.3, section 2.3)
            usleep(HDCP_INLOOP_RETRY_DELAY_US);
        }
    }

    postRunHdcp();

    return ret;
}

bool HdcpControl::preRunHdcp()
{
    // TODO: for CTP platform, IED needs to be disabled during HDCP authentication.
    return true;
}

bool HdcpControl::postRunHdcp()
{
    // TODO: for CTP platform, IED needs to be disabled during HDCP authentication.
    return true;
}


void HdcpControl::signalCompletion()
{
    if (mWaitForCompletion) {
        ILOGTRACE("signal HDCP authentication completed, status = %d", mAuthenticated);
        mCompletedCondition.signal();
        mWaitForCompletion = false;
    }
}

bool HdcpControl::threadLoop()
{
    Mutex::Autolock lock(mMutex);
    status_t err = mStoppedCondition.waitRelative(mMutex, milliseconds(mActionDelay));
    if (err != -ETIMEDOUT) {
        ILOGTRACE("Hdcp is stopped.");
        signalCompletion();
        return false;
    }

    // default is to keep thread active
    bool ret = true;
    if (!mAuthenticated) {
        ret = runHdcp();
        mAuthRetryCount++;
    } else {
        mAuthRetryCount = 0;
        checkAuthenticated();
    }

    // set next action delay
    if (mAuthenticated) {
        mActionDelay = HDCP_VERIFICATION_DELAY_MS;
    } else {
        // If HDCP can not authenticate after "HDCP_RETRY_LIMIT" attempts
        // reduce HDCP retry frequency to 2 sec
        if (mAuthRetryCount >= HDCP_RETRY_LIMIT) {
            mActionDelay = HDCP_AUTHENTICATION_LONG_DELAY_MS;
        } else {
            mActionDelay = HDCP_AUTHENTICATION_SHORT_DELAY_MS;
        }
    }

    // TODO: move out of lock?
    if (!ret || mAuthenticated) {
        signalCompletion();
    }

    if (mCallback) {
         if ((mAuthenticated && mCallbackState == CALLBACK_AUTHENTICATED) ||
            (!mAuthenticated && mCallbackState == CALLBACK_NOT_AUTHENTICATED)) {
            // ignore callback as state is not changed
        } else {
            mCallbackState =
                mAuthenticated ? CALLBACK_AUTHENTICATED : CALLBACK_NOT_AUTHENTICATED;
            (*mCallback)(mAuthenticated, mUserData);
        }
    }
    return ret;
}

} // namespace intel
} // namespace android
