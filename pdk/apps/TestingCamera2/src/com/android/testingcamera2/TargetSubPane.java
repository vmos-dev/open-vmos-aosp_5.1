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

package com.android.testingcamera2;

import android.content.Context;
import android.util.AttributeSet;
import android.view.Surface;
import android.widget.LinearLayout;

public abstract class TargetSubPane extends LinearLayout {

    public TargetSubPane(Context context, AttributeSet attrs) {
        super(context, attrs);
        // TODO Auto-generated constructor stub
    }

    public abstract void setTargetCameraPane(CameraControlPane target);

    /**
     * Set the current orientation of the UI, relative to the native device orientation
     *
     * @param orientation current UI orientation in degrees, one of the Surface.ROTATION_* values
     */
    public abstract void setUiOrientation(int orientation);

    public abstract Surface getOutputSurface();
}
