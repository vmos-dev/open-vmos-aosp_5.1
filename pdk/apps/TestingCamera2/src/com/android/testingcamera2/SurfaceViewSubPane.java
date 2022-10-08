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

import java.util.ArrayList;
import java.util.List;

import android.content.Context;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.params.StreamConfigurationMap;
import android.util.Size;
import android.util.AttributeSet;
import android.util.Size;
import android.view.LayoutInflater;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.LinearLayout;
import android.widget.Spinner;
import android.widget.AdapterView.OnItemSelectedListener;

public class SurfaceViewSubPane extends TargetSubPane implements SurfaceHolder.Callback {

    private static final int NO_SIZE = -1;
    private final FixedAspectSurfaceView mFixedSurfaceView;
    private Surface mSurface;

    private final Spinner mSizeSpinner;
    private Size[] mSizes;
    private int mCurrentCameraOrientation = 0;
    private int mCurrentUiOrientation = 0;

    private int mCurrentSizeId = NO_SIZE;
    private CameraControlPane mCurrentCamera;

    private float mAspectRatio = DEFAULT_ASPECT_RATIO;

    private static final float DEFAULT_ASPECT_RATIO = 1.5f;

    public SurfaceViewSubPane(Context context, AttributeSet attrs) {
        super(context, attrs);

        LayoutInflater inflater =
                (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);

        inflater.inflate(R.layout.surfaceview_target_subpane, this);
        this.setOrientation(VERTICAL);

        mFixedSurfaceView = (FixedAspectSurfaceView) this.findViewById(R.id.target_subpane_surface_view_view);
        mFixedSurfaceView.getHolder().addCallback(this);
        mSizeSpinner = (Spinner) this.findViewById(R.id.target_subpane_surface_view_size_spinner);
        mSizeSpinner.setOnItemSelectedListener(mSizeSpinnerListener);
    }

    @Override
    public void setTargetCameraPane(CameraControlPane target) {
        if (target != null) {
            Size oldSize = null;
            if (mCurrentSizeId != NO_SIZE) {
                oldSize = mSizes[mCurrentSizeId];
            }

            CameraCharacteristics info = target.getCharacteristics();
            {
                StreamConfigurationMap streamConfigMap =
                        info.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
                mSizes = streamConfigMap.getOutputSizes(SurfaceHolder.class);
            }

            int newSelectionId = 0;
            for (int i = 0; i < mSizes.length; i++) {
                if (mSizes[i].equals(oldSize)) {
                    newSelectionId = i;
                    break;
                }
            }
            String[] outputSizeItems = new String[mSizes.length];
            for (int i = 0; i < outputSizeItems.length; i++) {
                outputSizeItems[i] = mSizes[i].toString();
            }

            mSizeSpinner.setAdapter(new ArrayAdapter<String>(getContext(), R.layout.spinner_item,
                    outputSizeItems));
            mSizeSpinner.setSelection(newSelectionId);

            // Map sensor orientation to Surface.ROTATE_* constants
            final int SENSOR_ORIENTATION_TO_SURFACE_ROTATE = 90;
            mCurrentCameraOrientation = info.get(CameraCharacteristics.SENSOR_ORIENTATION) /
                    SENSOR_ORIENTATION_TO_SURFACE_ROTATE;

            updateAspectRatio();
        } else {
            mSizeSpinner.setAdapter(null);
            mCurrentSizeId = NO_SIZE;
        }
    }

    @Override
    public void setUiOrientation(int orientation) {
        mCurrentUiOrientation = orientation;
        updateAspectRatio();
    }

    private void updateSizes() {
        if (mCurrentSizeId != NO_SIZE) {
            Size s = mSizes[mCurrentSizeId];
            mFixedSurfaceView.getHolder().setFixedSize(s.getWidth(), s.getHeight());
            mAspectRatio = ((float) s.getWidth()) / s.getHeight();
        } else {
            // Make sure the view has some reasonable size even when there's no
            // target camera for aspect-ratio correct sizing
            mAspectRatio = DEFAULT_ASPECT_RATIO;
        }
        updateAspectRatio();
    }

    private void updateAspectRatio() {
        // Swap aspect ratios when the UI orientation and the camera orientation don't line up
        boolean swapAspect = Math.abs(mCurrentUiOrientation - mCurrentCameraOrientation) % 2 == 1;
        if (swapAspect) {
            mFixedSurfaceView.setAspectRatio(1/mAspectRatio);
        } else {
            mFixedSurfaceView.setAspectRatio(mAspectRatio);
        }
    }

    @Override
    public Surface getOutputSurface() {
        return mSurface;
    }

    private final OnItemSelectedListener mSizeSpinnerListener = new OnItemSelectedListener() {
        @Override
        public void onItemSelected(AdapterView<?> parent, View view, int pos, long id) {
            mCurrentSizeId = pos;
            updateSizes();
        };

        @Override
        public void onNothingSelected(AdapterView<?> parent) {
            mCurrentSizeId = NO_SIZE;
        };
    };

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        mSurface = holder.getSurface();
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }

}
