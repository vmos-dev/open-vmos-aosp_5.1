/*
 * Copyright (C) 2012 The Android Open Source Project
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

package com.android.testingcamera;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.FragmentManager;
import android.content.res.Resources;
import android.graphics.ImageFormat;
import android.hardware.Camera;
import android.hardware.Camera.Parameters;
import android.hardware.Camera.ErrorCallback;
import android.media.CamcorderProfile;
import android.media.MediaRecorder;
import android.media.MediaScannerConnection;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.SystemClock;
import android.view.View;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View.OnClickListener;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.LinearLayout;
import android.widget.LinearLayout.LayoutParams;
import android.widget.SeekBar;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.ToggleButton;
import android.renderscript.RenderScript;
import android.text.Layout;
import android.text.method.ScrollingMovementMethod;
import android.util.Log;
import android.util.SparseArray;

import java.io.File;
import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * A simple test application for the camera API.
 *
 * The goal of this application is to allow all camera API features to be
 * exercised, and all information provided by the API to be shown.
 */
public class TestingCamera extends Activity
    implements SurfaceHolder.Callback, Camera.PreviewCallback,
        Camera.ErrorCallback {

    /** UI elements */
    private SurfaceView mPreviewView;
    private SurfaceHolder mPreviewHolder;
    private LinearLayout mPreviewColumn;

    private SurfaceView mCallbackView;
    private SurfaceHolder mCallbackHolder;

    private Spinner mCameraSpinner;
    private CheckBox mKeepOpenCheckBox;
    private Button mInfoButton;
    private Spinner mPreviewSizeSpinner;
    private Spinner mPreviewFrameRateSpinner;
    private ToggleButton mPreviewToggle;
    private Spinner mAutofocusModeSpinner;
    private Button mAutofocusButton;
    private Button mCancelAutofocusButton;
    private TextView mFlashModeSpinnerLabel;
    private Spinner mFlashModeSpinner;
    private ToggleButton mExposureLockToggle;
    private Spinner mSnapshotSizeSpinner;
    private Button  mTakePictureButton;
    private Spinner mCamcorderProfileSpinner;
    private Spinner mVideoRecordSizeSpinner;
    private Spinner mVideoFrameRateSpinner;
    private ToggleButton mRecordToggle;
    private CheckBox mRecordHandoffCheckBox;
    private ToggleButton mRecordStabilizationToggle;
    private ToggleButton mRecordHintToggle;
    private ToggleButton mLockCameraToggle;
    private Spinner mCallbackFormatSpinner;
    private ToggleButton mCallbackToggle;
    private TextView mColorEffectSpinnerLabel;
    private Spinner mColorEffectSpinner;
    private SeekBar mZoomSeekBar;

    private TextView mLogView;

    private Set<View> mOpenOnlyControls = new HashSet<View>();
    private Set<View> mPreviewOnlyControls = new HashSet<View>();

    private SparseArray<String> mFormatNames;

    /** Camera state */
    private int mCameraId;
    private Camera mCamera;
    private Camera.Parameters mParams;
    private List<Camera.Size> mPreviewSizes;
    private int mPreviewSize = 0;
    private List<Integer> mPreviewFrameRates;
    private int mPreviewFrameRate = 0;
    private List<Integer> mPreviewFormats;
    private int mPreviewFormat = 0;
    private List<String> mAfModes;
    private int mAfMode = 0;
    private List<String> mFlashModes;
    private int mFlashMode = 0;
    private List<Camera.Size> mSnapshotSizes;
    private int mSnapshotSize = 0;
    private List<CamcorderProfile> mCamcorderProfiles;
    private int mCamcorderProfile = 0;
    private List<Camera.Size> mVideoRecordSizes;
    private int mVideoRecordSize = 0;
    private List<Integer> mVideoFrameRates;
    private int mVideoFrameRate = 0;
    private List<String> mColorEffects;
    private int mColorEffect = 0;
    private int mZoom = 0;

    private MediaRecorder mRecorder;
    private File mRecordingFile;

    private RenderScript mRS;

    private boolean mCallbacksEnabled = false;
    private CallbackProcessor mCallbackProcessor = null;
    long mLastCallbackTimestamp = -1;
    float mCallbackAvgFrameDuration = 30;
    int mCallbackFrameCount = 0;
    private static final float MEAN_FPS_HISTORY_COEFF = 0.9f;
    private static final float MEAN_FPS_MEASUREMENT_COEFF = 0.1f;
    private static final int   FPS_REPORTING_PERIOD = 200; // frames
    private static final int CALLBACK_BUFFER_COUNT = 3;

    private static final int CAMERA_UNINITIALIZED = 0;
    private static final int CAMERA_OPEN = 1;
    private static final int CAMERA_PREVIEW = 2;
    private static final int CAMERA_TAKE_PICTURE = 3;
    private static final int CAMERA_RECORD = 4;
    private int mState = CAMERA_UNINITIALIZED;

    private static final int NO_CAMERA_ID = -1;

    /** Misc variables */

    private static final String TAG = "TestingCamera";


    /** Activity lifecycle */

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.main);

        mPreviewColumn = (LinearLayout) findViewById(R.id.preview_column);

        mPreviewView = (SurfaceView) findViewById(R.id.preview);
        mPreviewView.getHolder().addCallback(this);

        mCallbackView = (SurfaceView)findViewById(R.id.callback_view);

        mCameraSpinner = (Spinner) findViewById(R.id.camera_spinner);
        mCameraSpinner.setOnItemSelectedListener(mCameraSpinnerListener);

        mKeepOpenCheckBox = (CheckBox) findViewById(R.id.keep_open_checkbox);

        mInfoButton = (Button) findViewById(R.id.info_button);
        mInfoButton.setOnClickListener(mInfoButtonListener);
        mOpenOnlyControls.add(mInfoButton);

        mPreviewSizeSpinner = (Spinner) findViewById(R.id.preview_size_spinner);
        mPreviewSizeSpinner.setOnItemSelectedListener(mPreviewSizeListener);
        mOpenOnlyControls.add(mPreviewSizeSpinner);

        mPreviewFrameRateSpinner = (Spinner) findViewById(R.id.preview_frame_rate_spinner);
        mPreviewFrameRateSpinner.setOnItemSelectedListener(mPreviewFrameRateListener);
        mOpenOnlyControls.add(mPreviewFrameRateSpinner);

        mPreviewToggle = (ToggleButton) findViewById(R.id.start_preview);
        mPreviewToggle.setOnClickListener(mPreviewToggleListener);
        mOpenOnlyControls.add(mPreviewToggle);

        mAutofocusModeSpinner = (Spinner) findViewById(R.id.af_mode_spinner);
        mAutofocusModeSpinner.setOnItemSelectedListener(mAutofocusModeListener);
        mOpenOnlyControls.add(mAutofocusModeSpinner);

        mAutofocusButton = (Button) findViewById(R.id.af_button);
        mAutofocusButton.setOnClickListener(mAutofocusButtonListener);
        mPreviewOnlyControls.add(mAutofocusButton);

        mCancelAutofocusButton = (Button) findViewById(R.id.af_cancel_button);
        mCancelAutofocusButton.setOnClickListener(mCancelAutofocusButtonListener);
        mPreviewOnlyControls.add(mCancelAutofocusButton);

        mFlashModeSpinnerLabel = (TextView) findViewById(R.id.flash_mode_spinner_label);

        mFlashModeSpinner = (Spinner) findViewById(R.id.flash_mode_spinner);
        mFlashModeSpinner.setOnItemSelectedListener(mFlashModeListener);
        mOpenOnlyControls.add(mFlashModeSpinner);

        mExposureLockToggle = (ToggleButton) findViewById(R.id.exposure_lock);
        mExposureLockToggle.setOnClickListener(mExposureLockToggleListener);
        mOpenOnlyControls.add(mExposureLockToggle);

        mZoomSeekBar = (SeekBar) findViewById(R.id.zoom_seekbar);
        mZoomSeekBar.setOnSeekBarChangeListener(mZoomSeekBarListener);

        mSnapshotSizeSpinner = (Spinner) findViewById(R.id.snapshot_size_spinner);
        mSnapshotSizeSpinner.setOnItemSelectedListener(mSnapshotSizeListener);
        mOpenOnlyControls.add(mSnapshotSizeSpinner);

        mTakePictureButton = (Button) findViewById(R.id.take_picture);
        mTakePictureButton.setOnClickListener(mTakePictureListener);
        mPreviewOnlyControls.add(mTakePictureButton);

        mCamcorderProfileSpinner = (Spinner) findViewById(R.id.camcorder_profile_spinner);
        mCamcorderProfileSpinner.setOnItemSelectedListener(mCamcorderProfileListener);
        mOpenOnlyControls.add(mCamcorderProfileSpinner);

        mVideoRecordSizeSpinner = (Spinner) findViewById(R.id.video_record_size_spinner);
        mVideoRecordSizeSpinner.setOnItemSelectedListener(mVideoRecordSizeListener);
        mOpenOnlyControls.add(mVideoRecordSizeSpinner);

        mVideoFrameRateSpinner = (Spinner) findViewById(R.id.video_frame_rate_spinner);
        mVideoFrameRateSpinner.setOnItemSelectedListener(mVideoFrameRateListener);
        mOpenOnlyControls.add(mVideoFrameRateSpinner);

        mRecordToggle = (ToggleButton) findViewById(R.id.start_record);
        mRecordToggle.setOnClickListener(mRecordToggleListener);
        mPreviewOnlyControls.add(mRecordToggle);

        mRecordHandoffCheckBox = (CheckBox) findViewById(R.id.record_handoff_checkbox);

        mRecordStabilizationToggle = (ToggleButton) findViewById(R.id.record_stabilization);
        mRecordStabilizationToggle.setOnClickListener(mRecordStabilizationToggleListener);
        mOpenOnlyControls.add(mRecordStabilizationToggle);

        mRecordHintToggle = (ToggleButton) findViewById(R.id.record_hint);
        mRecordHintToggle.setOnClickListener(mRecordHintToggleListener);
        mOpenOnlyControls.add(mRecordHintToggle);

        mLockCameraToggle = (ToggleButton) findViewById(R.id.lock_camera);
        mLockCameraToggle.setOnClickListener(mLockCameraToggleListener);
        mLockCameraToggle.setChecked(true); // ON by default
        mOpenOnlyControls.add(mLockCameraToggle);

        mCallbackFormatSpinner = (Spinner) findViewById(R.id.callback_format_spinner);
        mCallbackFormatSpinner.setOnItemSelectedListener(mCallbackFormatListener);
        mOpenOnlyControls.add(mCallbackFormatSpinner);

        mCallbackToggle = (ToggleButton) findViewById(R.id.enable_callbacks);
        mCallbackToggle.setOnClickListener(mCallbackToggleListener);
        mOpenOnlyControls.add(mCallbackToggle);

        mColorEffectSpinnerLabel = (TextView) findViewById(R.id.color_effect_spinner_label);

        mColorEffectSpinner = (Spinner) findViewById(R.id.color_effect_spinner);
        mColorEffectSpinner.setOnItemSelectedListener(mColorEffectListener);
        mOpenOnlyControls.add(mColorEffectSpinner);

        mLogView = (TextView) findViewById(R.id.log);
        mLogView.setMovementMethod(new ScrollingMovementMethod());

        mOpenOnlyControls.addAll(mPreviewOnlyControls);

        mFormatNames = new SparseArray<String>(7);
        mFormatNames.append(ImageFormat.JPEG, "JPEG");
        mFormatNames.append(ImageFormat.NV16, "NV16");
        mFormatNames.append(ImageFormat.NV21, "NV21");
        mFormatNames.append(ImageFormat.RGB_565, "RGB_565");
        mFormatNames.append(ImageFormat.UNKNOWN, "UNKNOWN");
        mFormatNames.append(ImageFormat.YUY2, "YUY2");
        mFormatNames.append(ImageFormat.YV12, "YV12");

        int numCameras = Camera.getNumberOfCameras();
        String[] cameraNames = new String[numCameras + 1];
        cameraNames[0] = "None";
        for (int i = 0; i < numCameras; i++) {
            cameraNames[i + 1] = "Camera " + i;
        }

        mCameraSpinner.setAdapter(
                new ArrayAdapter<String>(this,
                        R.layout.spinner_item, cameraNames));
        if (numCameras > 0) {
            mCameraId = 0;
            mCameraSpinner.setSelection(mCameraId + 1);
        } else {
            resetCamera();
            mCameraSpinner.setSelection(0);
        }

        mRS = RenderScript.create(this);
    }

    @Override
    public void onResume() {
        super.onResume();
        log("onResume: Setting up");
        mPreviewHolder = null;
        setUpCamera();
    }

    @Override
    public void onPause() {
        super.onPause();
        if (mState == CAMERA_RECORD) {
            stopRecording(false);
        }
        if (mKeepOpenCheckBox.isChecked()) {
            log("onPause: Not releasing camera");

            if (mState == CAMERA_PREVIEW) {
                mCamera.stopPreview();
                mState = CAMERA_OPEN;
            }
        } else {
            log("onPause: Releasing camera");

            if (mCamera != null) {
                mCamera.release();
            }
            mState = CAMERA_UNINITIALIZED;
        }
    }

    /** SurfaceHolder.Callback methods */
    @Override
    public void surfaceChanged(SurfaceHolder holder,
            int format,
            int width,
            int height) {
        if (holder == mPreviewView.getHolder()) {
            if (mState >= CAMERA_OPEN) {
                final int previewWidth =
                        mPreviewSizes.get(mPreviewSize).width;
                final int previewHeight =
                        mPreviewSizes.get(mPreviewSize).height;

                if ( Math.abs((float)previewWidth / previewHeight -
                        (float)width/height) > 0.01f) {
                    Handler h = new Handler();
                    h.post(new Runnable() {
                        @Override
                        public void run() {
                            layoutPreview();
                        }
                    });
                }
            }

            if (mPreviewHolder != null) {
                return;
            }
            log("Surface holder available: " + width + " x " + height);
            mPreviewHolder = holder;
            try {
                if (mCamera != null) {
                    mCamera.setPreviewDisplay(holder);
                }
            } catch (IOException e) {
                logE("Unable to set up preview!");
            }
        } else if (holder == mCallbackView.getHolder()) {
            mCallbackHolder = holder;
        }
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        mPreviewHolder = null;
    }

    public void setCameraDisplayOrientation() {
        android.hardware.Camera.CameraInfo info =
                new android.hardware.Camera.CameraInfo();
        android.hardware.Camera.getCameraInfo(mCameraId, info);
        int rotation = getWindowManager().getDefaultDisplay()
                .getRotation();
        int degrees = 0;
        switch (rotation) {
            case Surface.ROTATION_0: degrees = 0; break;
            case Surface.ROTATION_90: degrees = 90; break;
            case Surface.ROTATION_180: degrees = 180; break;
            case Surface.ROTATION_270: degrees = 270; break;
        }

        int result;
        if (info.facing == Camera.CameraInfo.CAMERA_FACING_FRONT) {
            result = (info.orientation + degrees) % 360;
            result = (360 - result) % 360;  // compensate the mirror
        } else {  // back-facing
            result = (info.orientation - degrees + 360) % 360;
        }
        log(String.format(
            "Camera sensor orientation %d, UI rotation %d, facing %s. Final orientation %d",
            info.orientation, rotation,
            info.facing == Camera.CameraInfo.CAMERA_FACING_FRONT ? "FRONT" : "BACK",
            result));
        mCamera.setDisplayOrientation(result);
    }

    /** UI controls enable/disable for all open-only controls */
    private void enableOpenOnlyControls(boolean enabled) {
        for (View v : mOpenOnlyControls) {
                v.setEnabled(enabled);
        }
    }

    /** UI controls enable/disable for all preview-only controls */
    private void enablePreviewOnlyControls(boolean enabled) {
        for (View v : mPreviewOnlyControls) {
                v.setEnabled(enabled);
        }
    }

    /** UI listeners */

    private AdapterView.OnItemSelectedListener mCameraSpinnerListener =
                new AdapterView.OnItemSelectedListener() {
        @Override
        public void onItemSelected(AdapterView<?> parent,
                        View view, int pos, long id) {
            int cameraId = pos - 1;
            if (mCameraId != cameraId) {
                resetCamera();
                mCameraId = cameraId;
                mPreviewToggle.setChecked(false);
                setUpCamera();
            }
        }

        @Override
        public void onNothingSelected(AdapterView<?> parent) {

        }
    };

    private OnClickListener mInfoButtonListener = new OnClickListener() {
        @Override
        public void onClick(View v) {
            if (mCameraId != NO_CAMERA_ID) {
                FragmentManager fm = getFragmentManager();
                InfoDialogFragment infoDialog = new InfoDialogFragment();
                infoDialog.updateInfo(mCameraId, mCamera);
                infoDialog.show(fm, "info_dialog_fragment");
            }
        }
    };

    private AdapterView.OnItemSelectedListener mPreviewSizeListener =
        new AdapterView.OnItemSelectedListener() {
        @Override
        public void onItemSelected(AdapterView<?> parent,
                View view, int pos, long id) {
            if (pos == mPreviewSize) return;
            if (mState == CAMERA_PREVIEW) {
                log("Stopping preview and callbacks to switch resolutions");
                stopCallbacks();
                mCamera.stopPreview();
            }

            mPreviewSize = pos;
            int width = mPreviewSizes.get(mPreviewSize).width;
            int height = mPreviewSizes.get(mPreviewSize).height;
            mParams.setPreviewSize(width, height);

            log("Setting preview size to " + width + "x" + height);

            mCamera.setParameters(mParams);
            resizePreview();

            if (mState == CAMERA_PREVIEW) {
                log("Restarting preview");
                mCamera.startPreview();
            }
        }

        @Override
        public void onNothingSelected(AdapterView<?> parent) {

        }
    };

    private AdapterView.OnItemSelectedListener mPreviewFrameRateListener =
                new AdapterView.OnItemSelectedListener() {
        @Override
        public void onItemSelected(AdapterView<?> parent,
                        View view, int pos, long id) {
            if (pos == mPreviewFrameRate) return;
            mPreviewFrameRate = pos;
            mParams.setPreviewFrameRate(mPreviewFrameRates.get(mPreviewFrameRate));

            log("Setting preview frame rate to " + ((TextView)view).getText());

            mCamera.setParameters(mParams);
        }

        @Override
        public void onNothingSelected(AdapterView<?> parent) {

        }
    };

    private View.OnClickListener mPreviewToggleListener =
            new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            if (mState == CAMERA_TAKE_PICTURE) {
                logE("Can't change preview state while taking picture!");
                return;
            }
            if (mPreviewToggle.isChecked()) {
                log("Starting preview");
                mCamera.startPreview();
                mState = CAMERA_PREVIEW;
                enablePreviewOnlyControls(true);
            } else {
                log("Stopping preview");
                mCamera.stopPreview();
                mState = CAMERA_OPEN;

                enablePreviewOnlyControls(false);
            }
        }
    };

    private OnItemSelectedListener mAutofocusModeListener =
                new OnItemSelectedListener() {
        @Override
        public void onItemSelected(AdapterView<?> parent,
                        View view, int pos, long id) {
            if (pos == mAfMode) return;

            mAfMode = pos;
            String focusMode = mAfModes.get(mAfMode);
            log("Setting focus mode to " + focusMode);
            if (focusMode == Camera.Parameters.FOCUS_MODE_CONTINUOUS_PICTURE ||
                        focusMode == Camera.Parameters.FOCUS_MODE_CONTINUOUS_VIDEO) {
                mCamera.setAutoFocusMoveCallback(mAutofocusMoveCallback);
            }
            mParams.setFocusMode(focusMode);

            mCamera.setParameters(mParams);
        }

        @Override
        public void onNothingSelected(AdapterView<?> arg0) {

        }
    };

    private OnClickListener mAutofocusButtonListener =
            new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            log("Triggering autofocus");
            mCamera.autoFocus(mAutofocusCallback);
        }
    };

    private OnClickListener mCancelAutofocusButtonListener =
            new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            log("Cancelling autofocus");
            mCamera.cancelAutoFocus();
        }
    };

    private Camera.AutoFocusCallback mAutofocusCallback =
            new Camera.AutoFocusCallback() {
        @Override
        public void onAutoFocus(boolean success, Camera camera) {
            log("Autofocus completed: " + (success ? "success" : "failure") );
        }
    };

    private Camera.AutoFocusMoveCallback mAutofocusMoveCallback =
            new Camera.AutoFocusMoveCallback() {
        @Override
        public void onAutoFocusMoving(boolean start, Camera camera) {
            log("Autofocus movement: " + (start ? "starting" : "stopped") );
        }
    };

    private OnItemSelectedListener mFlashModeListener =
                new OnItemSelectedListener() {
        @Override
        public void onItemSelected(AdapterView<?> parent,
                        View view, int pos, long id) {
            if (pos == mFlashMode) return;

            mFlashMode = pos;
            String flashMode = mFlashModes.get(mFlashMode);
            log("Setting flash mode to " + flashMode);
            mParams.setFlashMode(flashMode);
            mCamera.setParameters(mParams);
        }

        @Override
        public void onNothingSelected(AdapterView<?> arg0) {

        }
    };


    private AdapterView.OnItemSelectedListener mSnapshotSizeListener =
            new AdapterView.OnItemSelectedListener() {
        @Override
        public void onItemSelected(AdapterView<?> parent,
                View view, int pos, long id) {
            if (pos == mSnapshotSize) return;

            mSnapshotSize = pos;
            int width = mSnapshotSizes.get(mSnapshotSize).width;
            int height = mSnapshotSizes.get(mSnapshotSize).height;
            log("Setting snapshot size to " + width + " x " + height);

            mParams.setPictureSize(width, height);

            mCamera.setParameters(mParams);
        }

        @Override
        public void onNothingSelected(AdapterView<?> parent) {

        }
    };

    private View.OnClickListener mTakePictureListener =
            new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            log("Taking picture");
            if (mState == CAMERA_PREVIEW) {
                mState = CAMERA_TAKE_PICTURE;
                enablePreviewOnlyControls(false);
                mPreviewToggle.setChecked(false);

                mCamera.takePicture(mShutterCb, mRawCb, mPostviewCb, mJpegCb);
            } else {
                logE("Can't take picture while not running preview!");
            }
        }
    };

    private AdapterView.OnItemSelectedListener mCamcorderProfileListener =
                new AdapterView.OnItemSelectedListener() {
        @Override
        public void onItemSelected(AdapterView<?> parent,
                        View view, int pos, long id) {
            if (pos != mCamcorderProfile) {
                log("Setting camcorder profile to " + ((TextView)view).getText());
                mCamcorderProfile = pos;
            }

            // Additionally change video recording size to match
            mVideoRecordSize = 0; // "default", in case it's not found
            int width = mCamcorderProfiles.get(pos).videoFrameWidth;
            int height = mCamcorderProfiles.get(pos).videoFrameHeight;
            for (int i = 0; i < mVideoRecordSizes.size(); i++) {
                Camera.Size s = mVideoRecordSizes.get(i);
                if (width == s.width && height == s.height) {
                    mVideoRecordSize = i;
                    break;
                }
            }
            log("Setting video record size to " + mVideoRecordSize);
            mVideoRecordSizeSpinner.setSelection(mVideoRecordSize);
        }

        @Override
        public void onNothingSelected(AdapterView<?> parent) {

        }
    };

    private AdapterView.OnItemSelectedListener mVideoRecordSizeListener =
                new AdapterView.OnItemSelectedListener() {
        @Override
        public void onItemSelected(AdapterView<?> parent,
                        View view, int pos, long id) {
            if (pos == mVideoRecordSize) return;

            log("Setting video record size to " + ((TextView)view).getText());
            mVideoRecordSize = pos;
        }

        @Override
        public void onNothingSelected(AdapterView<?> parent) {

        }
    };

    private AdapterView.OnItemSelectedListener mVideoFrameRateListener =
                new AdapterView.OnItemSelectedListener() {
        @Override
        public void onItemSelected(AdapterView<?> parent,
                        View view, int pos, long id) {
            if (pos == mVideoFrameRate) return;

            log("Setting video frame rate to " + ((TextView)view).getText());
            mVideoFrameRate = pos;
        }

        @Override
        public void onNothingSelected(AdapterView<?> parent) {

        }
    };

    private View.OnClickListener mRecordToggleListener =
            new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            if (!mLockCameraToggle.isChecked()) {
                logE("Re-lock camera before recording");
                return;
            }

            mPreviewToggle.setEnabled(false);
            if (mState == CAMERA_PREVIEW) {
                startRecording();
            } else if (mState == CAMERA_RECORD) {
                stopRecording(false);
            } else {
                logE("Can't toggle recording in current state!");
            }
            mPreviewToggle.setEnabled(true);
        }
    };

    private View.OnClickListener mRecordStabilizationToggleListener =
            new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            boolean on = ((ToggleButton) v).isChecked();
            mParams.setVideoStabilization(on);

            mCamera.setParameters(mParams);
        }
    };

    private View.OnClickListener mRecordHintToggleListener =
            new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            boolean on = ((ToggleButton) v).isChecked();
            mParams.setRecordingHint(on);

            mCamera.setParameters(mParams);
        }
    };

    private View.OnClickListener mLockCameraToggleListener =
            new View.OnClickListener() {
        @Override
        public void onClick(View v) {

            if (mState == CAMERA_RECORD) {
                logE("Stop recording before toggling lock");
                return;
            }

            boolean on = ((ToggleButton) v).isChecked();

            if (on) {
                mCamera.lock();
                log("Locked camera");
            } else {
                mCamera.unlock();
                log("Unlocked camera");
            }
        }
    };

    private Camera.ShutterCallback mShutterCb = new Camera.ShutterCallback() {
        @Override
        public void onShutter() {
            log("Shutter callback received");
        }
    };

    private Camera.PictureCallback mRawCb = new Camera.PictureCallback() {
        @Override
        public void onPictureTaken(byte[] data, Camera camera) {
            log("Raw callback received");
        }
    };

    private Camera.PictureCallback mPostviewCb = new Camera.PictureCallback() {
        @Override
        public void onPictureTaken(byte[] data, Camera camera) {
            log("Postview callback received");
        }
    };

    private Camera.PictureCallback mJpegCb = new Camera.PictureCallback() {
        @Override
        public void onPictureTaken(byte[] data, Camera camera) {
            log("JPEG picture callback received");
            FragmentManager fm = getFragmentManager();
            SnapshotDialogFragment snapshotDialog = new SnapshotDialogFragment();

            snapshotDialog.updateImage(data);
            snapshotDialog.show(fm, "snapshot_dialog_fragment");

            mPreviewToggle.setEnabled(true);

            mState = CAMERA_OPEN;
        }
    };

    private AdapterView.OnItemSelectedListener mCallbackFormatListener =
            new AdapterView.OnItemSelectedListener() {
        public void onItemSelected(AdapterView<?> parent,
                        View view, int pos, long id) {
            mPreviewFormat = pos;

            log("Setting preview format to " +
                    mFormatNames.get(mPreviewFormats.get(mPreviewFormat)));

            switch (mState) {
            case CAMERA_UNINITIALIZED:
                return;
            case CAMERA_OPEN:
                break;
            case CAMERA_PREVIEW:
                if (mCallbacksEnabled) {
                    log("Stopping preview and callbacks to switch formats");
                    stopCallbacks();
                    mCamera.stopPreview();
                }
                break;
            case CAMERA_RECORD:
                logE("Can't update format while recording active");
                return;
            }

            mParams.setPreviewFormat(mPreviewFormats.get(mPreviewFormat));
            mCamera.setParameters(mParams);

            if (mCallbacksEnabled) {
                if (mState == CAMERA_PREVIEW) {
                    mCamera.startPreview();
                }
            }

            configureCallbacks(mCallbackView.getWidth(), mCallbackView.getHeight());
        }

        public void onNothingSelected(AdapterView<?> parent) {

        }
    };

    private View.OnClickListener mCallbackToggleListener =
                new View.OnClickListener() {
        public void onClick(View v) {
            if (mCallbacksEnabled) {
                log("Disabling preview callbacks");
                stopCallbacks();
                mCallbacksEnabled = false;
                resizePreview();
                mCallbackView.setVisibility(View.GONE);

            } else {
                log("Enabling preview callbacks");
                mCallbacksEnabled = true;
                resizePreview();
                mCallbackView.setVisibility(View.VISIBLE);
            }
        }
    };


    // Internal methods

    void setUpCamera() {
        if (mCameraId == NO_CAMERA_ID) return;

        log("Setting up camera " + mCameraId);
        logIndent(1);

        if (mState < CAMERA_OPEN) {
            log("Opening camera " + mCameraId);

            try {
                mCamera = Camera.open(mCameraId);
            } catch (RuntimeException e) {
                logE("Exception opening camera: " + e.getMessage());
                resetCamera();
                mCameraSpinner.setSelection(0);
                logIndent(-1);
                return;
            }
            mState = CAMERA_OPEN;
        }

        mCamera.setErrorCallback(this);

        setCameraDisplayOrientation();
        mParams = mCamera.getParameters();

        // Set up preview size selection

        log("Configuring camera");
        logIndent(1);

        updatePreviewSizes(mParams);
        updatePreviewFrameRate(mCameraId);
        updatePreviewFormats(mParams);
        updateAfModes(mParams);
        updateFlashModes(mParams);
        updateSnapshotSizes(mParams);
        updateCamcorderProfile(mCameraId);
        updateVideoRecordSize(mCameraId);
        updateVideoFrameRate(mCameraId);
        updateColorEffects(mParams);

        // Trigger updating video record size to match camcorder profile
        mCamcorderProfileSpinner.setSelection(mCamcorderProfile);

        if (mParams.isVideoStabilizationSupported()) {
            log("Video stabilization is supported");
            mRecordStabilizationToggle.setEnabled(true);
        } else {
            log("Video stabilization not supported");
            mRecordStabilizationToggle.setEnabled(false);
        }

        if (mParams.isAutoExposureLockSupported()) {
            log("Auto-Exposure locking is supported");
            mExposureLockToggle.setEnabled(true);
        } else {
            log("Auto-Exposure locking is not supported");
            mExposureLockToggle.setEnabled(false);
        }

        if (mParams.isZoomSupported()) {
            int maxZoom = mParams.getMaxZoom();
            mZoomSeekBar.setMax(maxZoom);
            log("Zoom is supported, set max to " + maxZoom);
            mZoomSeekBar.setEnabled(true);
        } else {
            log("Zoom is not supported");
            mZoomSeekBar.setEnabled(false);
        }

        // Update parameters based on above updates
        mCamera.setParameters(mParams);

        if (mPreviewHolder != null) {
            log("Setting preview display");
            try {
                mCamera.setPreviewDisplay(mPreviewHolder);
            } catch(IOException e) {
                Log.e(TAG, "Unable to set up preview!");
            }
        }

        logIndent(-1);

        enableOpenOnlyControls(true);

        resizePreview();
        if (mPreviewToggle.isChecked()) {
            log("Starting preview" );
            mCamera.startPreview();
            mState = CAMERA_PREVIEW;
        } else {
            mState = CAMERA_OPEN;
            enablePreviewOnlyControls(false);
        }
        logIndent(-1);
    }

    private void resetCamera() {
        if (mState >= CAMERA_OPEN) {
            log("Closing old camera");
            mCamera.release();
        }
        mCamera = null;
        mCameraId = NO_CAMERA_ID;
        mState = CAMERA_UNINITIALIZED;

        enableOpenOnlyControls(false);
    }

    private void updateAfModes(Parameters params) {
        mAfModes = params.getSupportedFocusModes();

        mAutofocusModeSpinner.setAdapter(
                new ArrayAdapter<String>(this, R.layout.spinner_item,
                        mAfModes.toArray(new String[0])));

        mAfMode = 0;

        params.setFocusMode(mAfModes.get(mAfMode));

        log("Setting AF mode to " + mAfModes.get(mAfMode));
    }

    private void updateFlashModes(Parameters params) {
        mFlashModes = params.getSupportedFlashModes();

        if (mFlashModes != null) {
            mFlashModeSpinnerLabel.setVisibility(View.VISIBLE);
            mFlashModeSpinner.setVisibility(View.VISIBLE);
            mFlashModeSpinner.setAdapter(
                    new ArrayAdapter<String>(this, R.layout.spinner_item,
                            mFlashModes.toArray(new String[0])));

            mFlashMode = 0;

            params.setFlashMode(mFlashModes.get(mFlashMode));

            log("Setting Flash mode to " + mFlashModes.get(mFlashMode));
        } else {
            // this camera has no flash
            mFlashModeSpinnerLabel.setVisibility(View.GONE);
            mFlashModeSpinner.setVisibility(View.GONE);
        }
    }

    private View.OnClickListener mExposureLockToggleListener =
            new View.OnClickListener() {
        public void onClick(View v) {
            boolean on = ((ToggleButton) v).isChecked();
            log("Auto-Exposure was " + mParams.getAutoExposureLock());
            mParams.setAutoExposureLock(on);
            log("Auto-Exposure is now " + mParams.getAutoExposureLock());
        }
    };

    private final SeekBar.OnSeekBarChangeListener mZoomSeekBarListener =
            new SeekBar.OnSeekBarChangeListener() {
        @Override
        public void onProgressChanged(SeekBar seekBar, int progress,
                boolean fromUser) {
            mZoom = progress;
            mParams.setZoom(mZoom);
            mCamera.setParameters(mParams);
        }
        @Override
        public void onStartTrackingTouch(SeekBar seekBar) { }
        @Override
        public void onStopTrackingTouch(SeekBar seekBar) {
            log("Zoom set to " + mZoom + " / " + mParams.getMaxZoom() + " (" +
                    ((float)(mParams.getZoomRatios().get(mZoom))/100) + "x)");
        }
    };

    private void updatePreviewSizes(Camera.Parameters params) {
        mPreviewSizes = params.getSupportedPreviewSizes();

        String[] availableSizeNames = new String[mPreviewSizes.size()];
        int i = 0;
        for (Camera.Size previewSize: mPreviewSizes) {
            availableSizeNames[i++] =
                Integer.toString(previewSize.width) + " x " +
                Integer.toString(previewSize.height);
        }
        mPreviewSizeSpinner.setAdapter(
                new ArrayAdapter<String>(
                        this, R.layout.spinner_item, availableSizeNames));

        mPreviewSize = 0;

        int width = mPreviewSizes.get(mPreviewSize).width;
        int height = mPreviewSizes.get(mPreviewSize).height;
        params.setPreviewSize(width, height);
        log("Setting preview size to " + width + " x " + height);
    }

    private void updatePreviewFrameRate(int cameraId) {
        List<Integer> frameRates = mParams.getSupportedPreviewFrameRates();
        int defaultPreviewFrameRate = mParams.getPreviewFrameRate();

        List<String> frameRateStrings = new ArrayList<String>();
        mPreviewFrameRates = new ArrayList<Integer>();

        int currentIndex = 0;
        for (Integer frameRate : frameRates) {
            mPreviewFrameRates.add(frameRate);
            if(frameRate == defaultPreviewFrameRate) {
                frameRateStrings.add(frameRate.toString() + " (Default)");
                mPreviewFrameRate = currentIndex;
            } else {
                frameRateStrings.add(frameRate.toString());
            }
            currentIndex++;
        }

        String[] nameArray = (String[])frameRateStrings.toArray(new String[0]);
        mPreviewFrameRateSpinner.setAdapter(
                new ArrayAdapter<String>(
                        this, R.layout.spinner_item, nameArray));

        mPreviewFrameRateSpinner.setSelection(mPreviewFrameRate);
        log("Setting preview frame rate to " + nameArray[mPreviewFrameRate]);
    }

    private void updatePreviewFormats(Camera.Parameters params) {
        mPreviewFormats = params.getSupportedPreviewFormats();

        String[] availableFormatNames = new String[mPreviewFormats.size()];
        int i = 0;
        for (Integer previewFormat: mPreviewFormats) {
            availableFormatNames[i++] = mFormatNames.get(previewFormat);
        }
        mCallbackFormatSpinner.setAdapter(
                new ArrayAdapter<String>(
                        this, R.layout.spinner_item, availableFormatNames));

        mPreviewFormat = 0;
        mCallbacksEnabled = false;
        mCallbackToggle.setChecked(false);
        mCallbackView.setVisibility(View.GONE);

        params.setPreviewFormat(mPreviewFormats.get(mPreviewFormat));
        log("Setting preview format to " +
                mFormatNames.get(mPreviewFormats.get(mPreviewFormat)));
    }

    private void updateSnapshotSizes(Camera.Parameters params) {
        String[] availableSizeNames;
        mSnapshotSizes = params.getSupportedPictureSizes();

        availableSizeNames = new String[mSnapshotSizes.size()];
        int i = 0;
        for (Camera.Size snapshotSize : mSnapshotSizes) {
            availableSizeNames[i++] =
                Integer.toString(snapshotSize.width) + " x " +
                Integer.toString(snapshotSize.height);
        }
        mSnapshotSizeSpinner.setAdapter(
                new ArrayAdapter<String>(
                        this, R.layout.spinner_item, availableSizeNames));

        mSnapshotSize = 0;

        int snapshotWidth = mSnapshotSizes.get(mSnapshotSize).width;
        int snapshotHeight = mSnapshotSizes.get(mSnapshotSize).height;
        params.setPictureSize(snapshotWidth, snapshotHeight);
        log("Setting snapshot size to " + snapshotWidth + " x " + snapshotHeight);
    }

    private void updateCamcorderProfile(int cameraId) {
        // Have to query all of these individually,
        final int PROFILES[] = new int[] {
            CamcorderProfile.QUALITY_2160P,
            CamcorderProfile.QUALITY_1080P,
            CamcorderProfile.QUALITY_480P,
            CamcorderProfile.QUALITY_720P,
            CamcorderProfile.QUALITY_CIF,
            CamcorderProfile.QUALITY_HIGH,
            CamcorderProfile.QUALITY_LOW,
            CamcorderProfile.QUALITY_QCIF,
            CamcorderProfile.QUALITY_QVGA,
            CamcorderProfile.QUALITY_TIME_LAPSE_2160P,
            CamcorderProfile.QUALITY_TIME_LAPSE_1080P,
            CamcorderProfile.QUALITY_TIME_LAPSE_480P,
            CamcorderProfile.QUALITY_TIME_LAPSE_720P,
            CamcorderProfile.QUALITY_TIME_LAPSE_CIF,
            CamcorderProfile.QUALITY_TIME_LAPSE_HIGH,
            CamcorderProfile.QUALITY_TIME_LAPSE_LOW,
            CamcorderProfile.QUALITY_TIME_LAPSE_QCIF,
            CamcorderProfile.QUALITY_TIME_LAPSE_QVGA
        };

        final String PROFILE_NAMES[] = new String[] {
            "2160P",
            "1080P",
            "480P",
            "720P",
            "CIF",
            "HIGH",
            "LOW",
            "QCIF",
            "QVGA",
            "TIME_LAPSE_2160P",
            "TIME_LAPSE_1080P",
            "TIME_LAPSE_480P",
            "TIME_LAPSE_720P",
            "TIME_LAPSE_CIF",
            "TIME_LAPSE_HIGH",
            "TIME_LAPSE_LOW",
            "TIME_LAPSE_QCIF",
            "TIME_LAPSE_QVGA"
        };

        List<String> availableCamcorderProfileNames = new ArrayList<String>();
        mCamcorderProfiles = new ArrayList<CamcorderProfile>();

        for (int i = 0; i < PROFILES.length; i++) {
            if (CamcorderProfile.hasProfile(cameraId, PROFILES[i])) {
                availableCamcorderProfileNames.add(PROFILE_NAMES[i]);
                mCamcorderProfiles.add(CamcorderProfile.get(cameraId, PROFILES[i]));
            }
        }
        String[] nameArray = (String[])availableCamcorderProfileNames.toArray(new String[0]);
        mCamcorderProfileSpinner.setAdapter(
                new ArrayAdapter<String>(
                        this, R.layout.spinner_item, nameArray));

        mCamcorderProfile = 0;
        log("Setting camcorder profile to " + nameArray[mCamcorderProfile]);

    }

    private void updateVideoRecordSize(int cameraId) {
        List<Camera.Size> videoSizes = mParams.getSupportedVideoSizes();
        if (videoSizes == null) { // TODO: surface this to the user
            log("Failed to get video size list, using preview sizes instead");
            videoSizes = mParams.getSupportedPreviewSizes();
        }

        List<String> availableVideoRecordSizes = new ArrayList<String>();
        mVideoRecordSizes = new ArrayList<Camera.Size>();

        availableVideoRecordSizes.add("Default");
        mVideoRecordSizes.add(mCamera.new Size(0,0));

        for (Camera.Size s : videoSizes) {
              availableVideoRecordSizes.add(s.width + "x" + s.height);
              mVideoRecordSizes.add(s);
        }
        String[] nameArray = (String[])availableVideoRecordSizes.toArray(new String[0]);
        mVideoRecordSizeSpinner.setAdapter(
                new ArrayAdapter<String>(
                        this, R.layout.spinner_item, nameArray));

        mVideoRecordSize = 0;
        log("Setting video record profile to " + nameArray[mVideoRecordSize]);
    }

    private void updateVideoFrameRate(int cameraId) {
        // Use preview framerates as video framerates
        List<Integer> frameRates = mParams.getSupportedPreviewFrameRates();

        List<String> frameRateStrings = new ArrayList<String>();
        mVideoFrameRates = new ArrayList<Integer>();

        frameRateStrings.add("Default");
        mVideoFrameRates.add(0);

        for (Integer frameRate : frameRates) {
            frameRateStrings.add(frameRate.toString());
            mVideoFrameRates.add(frameRate);
        }
        String[] nameArray = (String[])frameRateStrings.toArray(new String[0]);
        mVideoFrameRateSpinner.setAdapter(
                new ArrayAdapter<String>(
                        this, R.layout.spinner_item, nameArray));

        mVideoFrameRate = 0;
        log("Setting recording frame rate to " + nameArray[mVideoFrameRate]);
    }

    void resizePreview() {
        // Reset preview layout parameters, to trigger layout pass
        // This will eventually call layoutPreview below
        Resources res = getResources();
        mPreviewView.setLayoutParams(
                new LinearLayout.LayoutParams(LayoutParams.MATCH_PARENT, 0,
                        mCallbacksEnabled ?
                        res.getInteger(R.integer.preview_with_callback_weight):
                        res.getInteger(R.integer.preview_only_weight) ));
    }

    void layoutPreview() {
        int width = mPreviewSizes.get(mPreviewSize).width;
        int height = mPreviewSizes.get(mPreviewSize).height;
        float previewAspect = ((float) width) / height;

        int viewHeight = mPreviewView.getHeight();
        int viewWidth = mPreviewView.getWidth();
        float viewAspect = ((float) viewWidth) / viewHeight;
        if ( previewAspect > viewAspect) {
            viewHeight = (int) (viewWidth / previewAspect);
        } else {
            viewWidth = (int) (viewHeight * previewAspect);
        }
        mPreviewView.setLayoutParams(
                new LayoutParams(viewWidth, viewHeight));

        if (mCallbacksEnabled) {
            int callbackHeight = mCallbackView.getHeight();
            int callbackWidth = mCallbackView.getWidth();
            float callbackAspect = ((float) callbackWidth) / callbackHeight;
            if ( previewAspect > callbackAspect) {
                callbackHeight = (int) (callbackWidth / previewAspect);
            } else {
                callbackWidth = (int) (callbackHeight * previewAspect);
            }
            mCallbackView.setLayoutParams(
                    new LayoutParams(callbackWidth, callbackHeight));
            configureCallbacks(callbackWidth, callbackHeight);
        }
    }


    private void configureCallbacks(int callbackWidth, int callbackHeight) {
        if (mState >= CAMERA_OPEN && mCallbacksEnabled) {
            mCamera.setPreviewCallbackWithBuffer(null);
            int width = mPreviewSizes.get(mPreviewSize).width;
            int height = mPreviewSizes.get(mPreviewSize).height;
            int format = mPreviewFormats.get(mPreviewFormat);

            mCallbackProcessor = new CallbackProcessor(width, height, format,
                    getResources(), mCallbackView,
                    callbackWidth, callbackHeight, mRS);

            int size = getCallbackBufferSize(width, height, format);
            log("Configuring callbacks:" + width + " x " + height +
                    " , format " + format);
            for (int i = 0; i < CALLBACK_BUFFER_COUNT; i++) {
                mCamera.addCallbackBuffer(new byte[size]);
            }
            mCamera.setPreviewCallbackWithBuffer(this);
        }
        mLastCallbackTimestamp = -1;
        mCallbackFrameCount = 0;
        mCallbackAvgFrameDuration = 30;
    }

    private void stopCallbacks() {
        if (mState >= CAMERA_OPEN) {
            mCamera.setPreviewCallbackWithBuffer(null);
            if (mCallbackProcessor != null) {
                if (!mCallbackProcessor.stop()) {
                    logE("Can't stop preview callback processing!");
                }
            }
        }
    }

    @Override
    public void onPreviewFrame(byte[] data, Camera camera) {
        long timestamp = SystemClock.elapsedRealtime();
        if (mLastCallbackTimestamp != -1) {
            long frameDuration = timestamp - mLastCallbackTimestamp;
            mCallbackAvgFrameDuration =
                    mCallbackAvgFrameDuration * MEAN_FPS_HISTORY_COEFF +
                    frameDuration * MEAN_FPS_MEASUREMENT_COEFF;
        }
        mLastCallbackTimestamp = timestamp;
        if (mState < CAMERA_PREVIEW || !mCallbacksEnabled) {
            mCamera.addCallbackBuffer(data);
            return;
        }
        mCallbackFrameCount++;
        if (mCallbackFrameCount % FPS_REPORTING_PERIOD == 0) {
            log("Got " + FPS_REPORTING_PERIOD + " callback frames, fps "
                    + 1e3/mCallbackAvgFrameDuration);
        }
        mCallbackProcessor.displayCallback(data);

        mCamera.addCallbackBuffer(data);
    }

    @Override
    public void onError(int error, Camera camera) {
        String errorName;
        switch (error) {
        case Camera.CAMERA_ERROR_SERVER_DIED:
            errorName = "SERVER_DIED";
            break;
        case Camera.CAMERA_ERROR_UNKNOWN:
            errorName = "UNKNOWN";
            break;
        default:
            errorName = "?";
            break;
        }
        logE("Camera error received: " + errorName + " (" + error + ")" );
        logE("Shutting down camera");
        resetCamera();
        mCameraSpinner.setSelection(0);
    }

    static final int MEDIA_TYPE_IMAGE = 0;
    static final int MEDIA_TYPE_VIDEO = 1;
    @SuppressLint("SimpleDateFormat")
    File getOutputMediaFile(int type){
        // To be safe, you should check that the SDCard is mounted
        // using Environment.getExternalStorageState() before doing this.

        String state = Environment.getExternalStorageState();
        if (!Environment.MEDIA_MOUNTED.equals(state)) {
                return null;
        }

        File mediaStorageDir = new File(Environment.getExternalStoragePublicDirectory(
                  Environment.DIRECTORY_DCIM), "TestingCamera");
        // This location works best if you want the created images to be shared
        // between applications and persist after your app has been uninstalled.

        // Create the storage directory if it does not exist
        if (! mediaStorageDir.exists()){
            if (! mediaStorageDir.mkdirs()){
                logE("Failed to create directory for pictures/video");
                return null;
            }
        }

        // Create a media file name
        String timeStamp = new SimpleDateFormat("yyyyMMdd_HHmmss").format(new Date());
        File mediaFile;
        if (type == MEDIA_TYPE_IMAGE){
            mediaFile = new File(mediaStorageDir.getPath() + File.separator +
            "IMG_"+ timeStamp + ".jpg");
        } else if(type == MEDIA_TYPE_VIDEO) {
            mediaFile = new File(mediaStorageDir.getPath() + File.separator +
            "VID_"+ timeStamp + ".mp4");
        } else {
            return null;
        }

        return mediaFile;
    }

    void notifyMediaScannerOfFile(File newFile,
                final MediaScannerConnection.OnScanCompletedListener listener) {
        final Handler h = new Handler();
        MediaScannerConnection.scanFile(this,
                new String[] { newFile.toString() },
                null,
                new MediaScannerConnection.OnScanCompletedListener() {
                    @Override
                    public void onScanCompleted(final String path, final Uri uri) {
                        h.post(new Runnable() {
                            @Override
                            public void run() {
                                log("MediaScanner notified: " +
                                        path + " -> " + uri);
                                if (listener != null)
                                    listener.onScanCompleted(path, uri);
                            }
                        });
                    }
                });
    }

    private void deleteFile(File badFile) {
        if (badFile.exists()) {
            boolean success = badFile.delete();
            if (success) log("Deleted file " + badFile.toString());
            else log("Unable to delete file " + badFile.toString());
        }
    }

    private void startRecording() {
        log("Starting recording");
        logIndent(1);
        log("Configuring MediaRecoder");

        mRecordHandoffCheckBox.setEnabled(false);
        if (mRecordHandoffCheckBox.isChecked()) {
            mCamera.release();
        } else {
            mCamera.unlock();
        }

        if (mRecorder != null) {
            mRecorder.release();
        }

        mRecorder = new MediaRecorder();
        mRecorder.setOnErrorListener(mRecordingErrorListener);
        mRecorder.setOnInfoListener(mRecordingInfoListener);
        if (!mRecordHandoffCheckBox.isChecked()) {
            mRecorder.setCamera(mCamera);
        }
        mRecorder.setPreviewDisplay(mPreviewHolder.getSurface());

        mRecorder.setAudioSource(MediaRecorder.AudioSource.CAMCORDER);
        mRecorder.setVideoSource(MediaRecorder.VideoSource.CAMERA);
        mRecorder.setProfile(mCamcorderProfiles.get(mCamcorderProfile));
        Camera.Size videoRecordSize = mVideoRecordSizes.get(mVideoRecordSize);
        if (videoRecordSize.width > 0 && videoRecordSize.height > 0) {
            mRecorder.setVideoSize(videoRecordSize.width, videoRecordSize.height);
        }
        if (mVideoFrameRates.get(mVideoFrameRate) > 0) {
            mRecorder.setVideoFrameRate(mVideoFrameRates.get(mVideoFrameRate));
        }
        File outputFile = getOutputMediaFile(MEDIA_TYPE_VIDEO);
        log("File name:" + outputFile.toString());
        mRecorder.setOutputFile(outputFile.toString());

        boolean ready = false;
        log("Preparing MediaRecorder");
        try {
            mRecorder.prepare();
            ready = true;
        } catch (Exception e) {
            StringWriter writer = new StringWriter();
            e.printStackTrace(new PrintWriter(writer));
            logE("Exception preparing MediaRecorder:\n" + writer.toString());
        }

        if (ready) {
            try {
                log("Starting MediaRecorder");
                mRecorder.start();
                mState = CAMERA_RECORD;
                log("Recording active");
                mRecordingFile = outputFile;
            } catch (Exception e) {
                StringWriter writer = new StringWriter();
                e.printStackTrace(new PrintWriter(writer));
                logE("Exception starting MediaRecorder:\n" + writer.toString());
                ready = false;
            }
        }

        if (!ready) {
            mRecordToggle.setChecked(false);
            mRecordHandoffCheckBox.setEnabled(true);

            if (mRecordHandoffCheckBox.isChecked()) {
                mState = CAMERA_UNINITIALIZED;
                setUpCamera();
            }
        }
        logIndent(-1);
    }

    private MediaRecorder.OnErrorListener mRecordingErrorListener =
            new MediaRecorder.OnErrorListener() {
        @Override
        public void onError(MediaRecorder mr, int what, int extra) {
            logE("MediaRecorder reports error: " + what + ", extra "
                    + extra);
            if (mState == CAMERA_RECORD) {
                stopRecording(true);
            }
        }
    };

    private MediaRecorder.OnInfoListener mRecordingInfoListener =
            new MediaRecorder.OnInfoListener() {
        @Override
        public void onInfo(MediaRecorder mr, int what, int extra) {
            log("MediaRecorder reports info: " + what + ", extra "
                    + extra);
        }
    };

    private void stopRecording(boolean error) {
        log("Stopping recording");
        mRecordHandoffCheckBox.setEnabled(true);
        mRecordToggle.setChecked(false);
        if (mRecorder != null) {
            try {
                mRecorder.stop();
            } catch (RuntimeException e) {
                // this can happen if there were no frames received by recorder
                logE("Could not create output file");
                error = true;
            }

            if (mRecordHandoffCheckBox.isChecked()) {
                mState = CAMERA_UNINITIALIZED;
                setUpCamera();
            } else {
                mCamera.lock();
                mState = CAMERA_PREVIEW;
            }

            if (!error) {
                notifyMediaScannerOfFile(mRecordingFile, null);
            } else {
                deleteFile(mRecordingFile);
            }
            mRecordingFile = null;
        } else {
            logE("Recorder is unexpectedly null!");
        }
    }

    static int getCallbackBufferSize(int width, int height, int format) {
        int size = -1;
        switch (format) {
        case ImageFormat.NV21:
            size = width * height * 3 / 2;
            break;
        case ImageFormat.YV12:
            int y_stride = (int) (Math.ceil( width / 16.) * 16);
            int y_size = y_stride * height;
            int c_stride = (int) (Math.ceil(y_stride / 32.) * 16);
            int c_size = c_stride * height/2;
            size = y_size + c_size * 2;
            break;
        case ImageFormat.NV16:
        case ImageFormat.RGB_565:
        case ImageFormat.YUY2:
            size = 2 * width * height;
            break;
        case ImageFormat.JPEG:
            Log.e(TAG, "JPEG callback buffers not supported!");
            size = 0;
            break;
        case ImageFormat.UNKNOWN:
            Log.e(TAG, "Unknown-format callback buffers not supported!");
            size = 0;
            break;
        }
        return size;
    }

    private OnItemSelectedListener mColorEffectListener =
                new OnItemSelectedListener() {
        @Override
        public void onItemSelected(AdapterView<?> parent,
                        View view, int pos, long id) {
            if (pos == mColorEffect) return;

            mColorEffect = pos;
            String colorEffect = mColorEffects.get(mColorEffect);
            log("Setting color effect to " + colorEffect);
            mParams.setColorEffect(colorEffect);
            mCamera.setParameters(mParams);
        }

        @Override
        public void onNothingSelected(AdapterView<?> arg0) {
        }
    };

    private void updateColorEffects(Parameters params) {
        mColorEffects = params.getSupportedColorEffects();
        if (mColorEffects != null) {
            mColorEffectSpinnerLabel.setVisibility(View.VISIBLE);
            mColorEffectSpinner.setVisibility(View.VISIBLE);
            mColorEffectSpinner.setAdapter(
                    new ArrayAdapter<String>(this, R.layout.spinner_item,
                            mColorEffects.toArray(new String[0])));
            mColorEffect = 0;
            params.setColorEffect(mColorEffects.get(mColorEffect));
            log("Setting Color Effect to " + mColorEffects.get(mColorEffect));
        } else {
            mColorEffectSpinnerLabel.setVisibility(View.GONE);
            mColorEffectSpinner.setVisibility(View.GONE);
        }
    }

    private int mLogIndentLevel = 0;
    private String mLogIndent = "\t";
    /** Increment or decrement log indentation level */
    synchronized void logIndent(int delta) {
        mLogIndentLevel += delta;
        if (mLogIndentLevel < 0) mLogIndentLevel = 0;
        char[] mLogIndentArray = new char[mLogIndentLevel + 1];
        for (int i = -1; i < mLogIndentLevel; i++) {
            mLogIndentArray[i + 1] = '\t';
        }
        mLogIndent = new String(mLogIndentArray);
    }

    @SuppressLint("SimpleDateFormat")
    SimpleDateFormat mDateFormatter = new SimpleDateFormat("HH:mm:ss.SSS");
    /** Log both to log text view and to device logcat */
    void log(String logLine) {
        Log.d(TAG, logLine);
        logAndScrollToBottom(logLine, mLogIndent);
    }

    void logE(String logLine) {
        Log.e(TAG, logLine);
        logAndScrollToBottom(logLine, mLogIndent + "!!! ");
    }

    synchronized private void logAndScrollToBottom(String logLine, String logIndent) {
        StringBuffer logEntry = new StringBuffer(32);
        logEntry.append("\n").append(mDateFormatter.format(new Date())).append(logIndent);
        logEntry.append(logLine);
        mLogView.append(logEntry);
        final Layout layout = mLogView.getLayout();
        if (layout != null){
            int scrollDelta = layout.getLineBottom(mLogView.getLineCount() - 1)
                - mLogView.getScrollY() - mLogView.getHeight();
            if(scrollDelta > 0) {
                mLogView.scrollBy(0, scrollDelta);
            }
        }
    }
}
