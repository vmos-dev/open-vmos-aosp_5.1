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

public class CameraManualControls {
    private int mSensitivity;
    private long mExposure;
    private long mFrameDuration;
    private boolean mManualControlEnabled;

    public CameraManualControls() {
        mSensitivity = 100;
        mExposure = 10000000L;
        mFrameDuration = 50000000L;
        mManualControlEnabled = false;
    }

    public int getSensitivity() {
        return mSensitivity;
    }

    public void setSensitivity(int sensitivity) {
        this.mSensitivity = sensitivity;
    }

    public long getExposure() {
        return mExposure;
    }

    public void setExposure(long exposure) {
        mExposure = exposure;
    }

    public long getFrameDuration() {
        return mFrameDuration;
    }

    public void setFrameDuration(long frameDuration) {
        mFrameDuration = frameDuration;
    }

    public boolean isManualControlEnabled() {
        return mManualControlEnabled;
    }

    public void setManualControlEnabled(boolean manualControlEnabled) {
        mManualControlEnabled = manualControlEnabled;
    }
}