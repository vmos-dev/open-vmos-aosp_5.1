/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.testingcamera2.v1;

import android.hardware.camera2.CaptureRequest;

/**
 * A camera AF control class wraps the AF control parameters.
 */
public class CameraAutoFocusControls {
    private boolean mAfControlEnabled = true;
    private int mAfMode = CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_PICTURE;
    private Integer mPendingTrigger = null;

    public CameraAutoFocusControls() {
    }

    public synchronized void setAfMode(int afMode) {
        mAfMode = afMode;
    }

    public synchronized void enableAfControl(boolean enable) {
        mAfControlEnabled = enable;
    }

    public synchronized boolean isAfControlEnabled() {
        return mAfControlEnabled;
    }

    public synchronized int getAfMode() {
        return mAfMode;
    }

    public synchronized void setPendingTriggerStart() {
        mPendingTrigger = CaptureRequest.CONTROL_AF_TRIGGER_START;
    }

    public synchronized void setPendingTriggerCancel() {
        mPendingTrigger = CaptureRequest.CONTROL_AF_TRIGGER_CANCEL;
    }

    public synchronized Integer consumePendingTrigger() {
        Integer pendingTrigger = mPendingTrigger;
        mPendingTrigger = null;
        return pendingTrigger;
    }
}
