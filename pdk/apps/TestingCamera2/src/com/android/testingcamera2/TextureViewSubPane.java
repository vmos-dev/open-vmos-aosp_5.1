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
import android.graphics.SurfaceTexture;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.params.StreamConfigurationMap;
import android.util.Size;
import android.util.AttributeSet;
import android.util.Size;
import android.view.LayoutInflater;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.TextureView;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.LinearLayout;
import android.widget.Spinner;
import android.widget.AdapterView.OnItemSelectedListener;

public class TextureViewSubPane extends TargetSubPane implements TextureView.SurfaceTextureListener {

    private static final int NO_SIZE = -1;
    private final TextureView mTextureView;
    private SurfaceTexture mSurfaceTexture;

    private final Spinner mSizeSpinner;
    private Size[] mSizes;
    private int mCurrentSizeId = NO_SIZE;
    private CameraControlPane mCurrentCamera;

    public TextureViewSubPane(Context context, AttributeSet attrs) {
        super(context, attrs);

        LayoutInflater inflater =
                (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);

        inflater.inflate(R.layout.textureview_target_subpane, this);
        this.setOrientation(VERTICAL);

        mTextureView = (TextureView) this.findViewById(R.id.target_subpane_texture_view_view);
        mTextureView.setSurfaceTextureListener(this);
        mSizeSpinner = (Spinner) this.findViewById(R.id.target_subpane_texture_view_size_spinner);
        mSizeSpinner.setOnItemSelectedListener(mSizeSpinnerListener);
    }

    @Override
    public void setTargetCameraPane(CameraControlPane target) {
        if (target != null) {
            Size oldSize = null;
            if (mCurrentSizeId != NO_SIZE) {
                oldSize = mSizes[mCurrentSizeId];
            }

            {
                CameraCharacteristics info = target.getCharacteristics();
                StreamConfigurationMap streamConfigMap =
                        info.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
                mSizes = streamConfigMap.getOutputSizes(SurfaceTexture.class);
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
        } else {
            mSizeSpinner.setAdapter(null);
            mCurrentSizeId = NO_SIZE;
        }
    }

    @Override
    public void setUiOrientation(int orientation) {
        // TODO Auto-generated method stub

    }

    private void updateSizes() {
        if (mCurrentSizeId != NO_SIZE) {
            Size s = mSizes[mCurrentSizeId];
            if (mSurfaceTexture != null) {
                mSurfaceTexture.setDefaultBufferSize(s.getWidth(), s.getHeight());
            }
            int width = getWidth();
            int height = width * s.getHeight() / s.getWidth();
            mTextureView.setLayoutParams(new LinearLayout.LayoutParams(width, height));
        } else {
            // Make sure the view has some reasonable size even when there's no
            // target camera for aspect-ratio correct sizing
            int width = getWidth();
            int height = width / 2;
            mTextureView.setLayoutParams(new LinearLayout.LayoutParams(width, height));
        }
    }

    @Override
    public Surface getOutputSurface() {
        return (mSurfaceTexture != null) ? new Surface(mSurfaceTexture) : null;
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
    public void onSurfaceTextureAvailable(final SurfaceTexture surface, final int width,
            final int height) {
        mSurfaceTexture = surface;
        updateSizes();
    }

    @Override
    public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {
        return false;
    }

    @Override
    public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height) {
        // ignore
    }

    @Override
    public void onSurfaceTextureUpdated(SurfaceTexture surface) {
        // ignore
    }
}
