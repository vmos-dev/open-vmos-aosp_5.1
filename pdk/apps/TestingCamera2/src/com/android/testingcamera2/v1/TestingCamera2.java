/*
 * Copyright (C) 2013 The Android Open Source Project
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

import android.app.Activity;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.ImageFormat;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CaptureFailure;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.CaptureResult;
import android.hardware.camera2.TotalCaptureResult;
import android.media.Image;
import android.media.MediaMuxer;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.util.Range;
import android.view.OrientationEventListener;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup.LayoutParams;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.CheckBox;
import android.widget.ImageView;
import android.widget.RadioGroup;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.ToggleButton;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import com.android.testingcamera2.R;

public class TestingCamera2 extends Activity implements SurfaceHolder.Callback {

    private static final String TAG = "TestingCamera2";
    private static final boolean VERBOSE = Log.isLoggable(TAG, Log.VERBOSE);
    private CameraOps mCameraOps;
    private static final int mSeekBarMax = 100;
    private static final long MAX_EXPOSURE = 200000000L; // 200ms
    private static final long MIN_EXPOSURE = 100000L; // 100us
    private static final long MAX_FRAME_DURATION = 1000000000L; // 1s
    // Manual control change step size
    private static final int STEP_SIZE = 100;
    // Min and max sensitivity ISO values
    private static final int MIN_SENSITIVITY = 100;
    private static final int MAX_SENSITIVITY = 1600;
    private static final int ORIENTATION_UNINITIALIZED = -1;

    private int mLastOrientation = ORIENTATION_UNINITIALIZED;
    private OrientationEventListener mOrientationEventListener;
    private SurfaceView mPreviewView;
    private ImageView mStillView;

    private SurfaceHolder mCurrentPreviewHolder = null;

    private Button mInfoButton;
    private Button mFlushButton;
    private ToggleButton mFocusLockToggle;
    private Spinner mFocusModeSpinner;
    private CheckBox mUseMediaCodecCheckBox;

    private SeekBar mSensitivityBar;
    private SeekBar mExposureBar;
    private SeekBar mFrameDurationBar;

    private TextView mSensitivityInfoView;
    private TextView mExposureInfoView;
    private TextView mFrameDurationInfoView;
    private TextView mCaptureResultView;
    private ToggleButton mRecordingToggle;
    private ToggleButton mManualCtrlToggle;

    private CameraControls mCameraControl = null;
    private final Set<View> mManualControls = new HashSet<View>();
    private final Set<View> mAutoControls = new HashSet<View>();



    Handler mMainHandler;
    boolean mUseMediaCodec;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                WindowManager.LayoutParams.FLAG_FULLSCREEN);

        setContentView(R.layout.main);

        mPreviewView = (SurfaceView) findViewById(R.id.preview_view);
        mPreviewView.getHolder().addCallback(this);

        mStillView = (ImageView) findViewById(R.id.still_view);

        mInfoButton  = (Button) findViewById(R.id.info_button);
        mInfoButton.setOnClickListener(mInfoButtonListener);
        mFlushButton  = (Button) findViewById(R.id.flush_button);
        mFlushButton.setOnClickListener(mFlushButtonListener);

        mFocusLockToggle = (ToggleButton) findViewById(R.id.focus_button);
        mFocusLockToggle.setOnClickListener(mFocusLockToggleListener);
        mFocusModeSpinner = (Spinner) findViewById(R.id.focus_mode_spinner);
        mAutoControls.add(mFocusLockToggle);

        mRecordingToggle = (ToggleButton) findViewById(R.id.start_recording);
        mRecordingToggle.setOnClickListener(mRecordingToggleListener);
        mUseMediaCodecCheckBox = (CheckBox) findViewById(R.id.use_media_codec);
        mUseMediaCodecCheckBox.setOnCheckedChangeListener(mUseMediaCodecListener);
        mUseMediaCodecCheckBox.setChecked(mUseMediaCodec);

        mManualCtrlToggle = (ToggleButton) findViewById(R.id.manual_control);
        mManualCtrlToggle.setOnClickListener(mControlToggleListener);

        mSensitivityBar = (SeekBar) findViewById(R.id.sensitivity_seekbar);
        mSensitivityBar.setOnSeekBarChangeListener(mSensitivitySeekBarListener);
        mSensitivityBar.setMax(mSeekBarMax);
        mManualControls.add(mSensitivityBar);

        mExposureBar = (SeekBar) findViewById(R.id.exposure_time_seekbar);
        mExposureBar.setOnSeekBarChangeListener(mExposureSeekBarListener);
        mExposureBar.setMax(mSeekBarMax);
        mManualControls.add(mExposureBar);

        mFrameDurationBar = (SeekBar) findViewById(R.id.frame_duration_seekbar);
        mFrameDurationBar.setOnSeekBarChangeListener(mFrameDurationSeekBarListener);
        mFrameDurationBar.setMax(mSeekBarMax);
        mManualControls.add(mFrameDurationBar);

        mSensitivityInfoView = (TextView) findViewById(R.id.sensitivity_bar_label);
        mExposureInfoView = (TextView) findViewById(R.id.exposure_time_bar_label);
        mFrameDurationInfoView = (TextView) findViewById(R.id.frame_duration_bar_label);
        mCaptureResultView = (TextView) findViewById(R.id.capture_result_info_label);

        enableManualControls(false);
        mCameraControl = new CameraControls();

        // Get UI handler
        mMainHandler = new Handler();

        try {
            mCameraOps = CameraOps.create(this, mCameraOpsListener, mMainHandler);
        } catch(ApiFailureException e) {
            logException("Cannot create camera ops!",e);
        }

        mOrientationEventListener = new OrientationEventListener(this) {
            @Override
            public void onOrientationChanged(int orientation) {
                if (orientation == ORIENTATION_UNKNOWN) {
                    orientation = 0;
                }
                mCameraOps.updateOrientation(orientation);
            }
        };
        mOrientationEventListener.enable();
        // Process the initial configuration (for i.e. initial orientation)
        // We need this because #onConfigurationChanged doesn't get called when the app launches
        maybeUpdateConfiguration(getResources().getConfiguration());
    }

    @Override
    public void onResume() {
        super.onResume();
        try {
            if (VERBOSE) Log.v(TAG, String.format("onResume"));

            mCameraOps.minimalPreviewConfig(mPreviewView.getHolder());
            mCurrentPreviewHolder = mPreviewView.getHolder();
        } catch (ApiFailureException e) {
            logException("Can't configure preview surface: ",e);
        }
    }

    @Override
    public void onPause() {
        super.onPause();
        try {
            if (VERBOSE) Log.v(TAG, String.format("onPause"));

            mCameraOps.closeDevice();
        } catch (ApiFailureException e) {
            logException("Can't close device: ",e);
        }
        mCurrentPreviewHolder = null;
    }

    @Override
    protected void onDestroy() {
        mOrientationEventListener.disable();
        super.onDestroy();
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);

        if (VERBOSE) {
            Log.v(TAG, String.format("onConfiguredChanged: orientation %x",
                    newConfig.orientation));
        }

        maybeUpdateConfiguration(newConfig);
    }

    private void maybeUpdateConfiguration(Configuration newConfig) {
        if (VERBOSE) {
            Log.v(TAG, String.format("handleConfiguration: orientation %x",
                    newConfig.orientation));
        }

        if (mLastOrientation != newConfig.orientation) {
            mLastOrientation = newConfig.orientation;
            updatePreviewOrientation();
        }
    }

    private void updatePreviewOrientation() {
        LayoutParams params = mPreviewView.getLayoutParams();
        int width = params.width;
        int height = params.height;

        if (VERBOSE) {
            Log.v(TAG, String.format(
                    "onConfiguredChanged: current layout is %dx%d", width,
                    height));
        }
        /**
         * Force wide aspect ratios for landscape
         * Force narrow aspect ratios for portrait
         */
        if (mLastOrientation == Configuration.ORIENTATION_LANDSCAPE) {
            if (height > width) {
                int tmp = width;
                width = height;
                height = tmp;
            }
        } else if (mLastOrientation == Configuration.ORIENTATION_PORTRAIT) {
            if (width > height) {
                int tmp = width;
                width = height;
                height = tmp;
            }
        }

        if (width != params.width && height != params.height) {
            if (VERBOSE) {
                Log.v(TAG, String.format(
                        "onConfiguredChanged: updating preview size to %dx%d", width,
                        height));
            }
            params.width = width;
            params.height = height;

            mPreviewView.setLayoutParams(params);
        }
    }

    /** SurfaceHolder.Callback methods */
    @Override
    public void surfaceChanged(SurfaceHolder holder,
            int format,
            int width,
            int height) {
        if (VERBOSE) {
            Log.v(TAG, String.format("surfaceChanged: format %x, width %d, height %d", format,
                    width, height));
        }
        if (mCurrentPreviewHolder != null && holder == mCurrentPreviewHolder) {
            try {
                mCameraOps.minimalPreview(holder, mCameraControl);
            } catch (ApiFailureException e) {
                logException("Can't start minimal preview: ", e);
            }
        }
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
    }

    private final Button.OnClickListener mInfoButtonListener = new Button.OnClickListener() {
        @Override
        public void onClick(View v) {
            final Handler uiHandler = new Handler();
            AsyncTask.execute(new Runnable() {
                @Override
                public void run() {
                    try {
                        mCameraOps.minimalJpegCapture(mCaptureCallback, mCaptureResultListener,
                                uiHandler, mCameraControl);
                        if (mCurrentPreviewHolder != null) {
                            mCameraOps.minimalPreview(mCurrentPreviewHolder, mCameraControl);
                        }
                    } catch (ApiFailureException e) {
                        logException("Can't take a JPEG! ", e);
                    }
                }
            });
        }
    };

    private final Button.OnClickListener mFlushButtonListener = new Button.OnClickListener() {
        @Override
        public void onClick(View v) {
            AsyncTask.execute(new Runnable() {
                @Override
                public void run() {
                    try {
                        mCameraOps.flush();
                    } catch (ApiFailureException e) {
                        logException("Can't flush!", e);
                    }
                }
            });
        }
    };

    /**
     * UI controls enable/disable for all manual controls
     */
    private void enableManualControls(boolean enabled) {
        for (View v : mManualControls) {
            v.setEnabled(enabled);
        }

        for (View v : mAutoControls) {
            v.setEnabled(!enabled);
        }
    }

    private final CameraOps.CaptureCallback mCaptureCallback = new CameraOps.CaptureCallback() {
        @Override
        public void onCaptureAvailable(Image capture) {
            if (capture.getFormat() != ImageFormat.JPEG) {
                Log.e(TAG, "Unexpected format: " + capture.getFormat());
                return;
            }
            ByteBuffer jpegBuffer = capture.getPlanes()[0].getBuffer();
            byte[] jpegData = new byte[jpegBuffer.capacity()];
            jpegBuffer.get(jpegData);

            Bitmap b = BitmapFactory.decodeByteArray(jpegData, 0, jpegData.length);
            mStillView.setImageBitmap(b);
        }
    };

    // TODO: this callback is not called for each capture, need figure out why.
    private final CameraOps.CaptureResultListener mCaptureResultListener =
            new CameraOps.CaptureResultListener() {

                @Override
                public void onCaptureStarted(CameraCaptureSession session, CaptureRequest request,
                        long timestamp, long frameNumber) {
                }

                @Override
                public void onCaptureCompleted(
                        CameraCaptureSession session, CaptureRequest request,
                        TotalCaptureResult result) {
                    Log.i(TAG, "Capture result is available");
                    Integer reqCtrlMode;
                    Integer resCtrlMode;
                    if (request == null || result ==null) {
                        Log.e(TAG, "request/result is invalid");
                        return;
                    }
                    Log.i(TAG, "Capture complete");
                    final StringBuffer info = new StringBuffer("Capture Result:\n");

                    reqCtrlMode = request.get(CaptureRequest.CONTROL_MODE);
                    resCtrlMode = result.get(CaptureResult.CONTROL_MODE);
                    info.append("Control mode: request " + reqCtrlMode + ". result " + resCtrlMode);
                    info.append("\n");

                    Integer reqSen = request.get(CaptureRequest.SENSOR_SENSITIVITY);
                    Integer resSen = result.get(CaptureResult.SENSOR_SENSITIVITY);
                    info.append("Sensitivity: request " + reqSen + ". result " + resSen);
                    info.append("\n");

                    Long reqExp = request.get(CaptureRequest.SENSOR_EXPOSURE_TIME);
                    Long resExp = result.get(CaptureResult.SENSOR_EXPOSURE_TIME);
                    info.append("Exposure: request " + reqExp + ". result " + resExp);
                    info.append("\n");

                    Long reqFD = request.get(CaptureRequest.SENSOR_FRAME_DURATION);
                    Long resFD = result.get(CaptureResult.SENSOR_FRAME_DURATION);
                    info.append("Frame duration: request " + reqFD + ". result " + resFD);
                    info.append("\n");

                    List<CaptureResult.Key<?>> resultKeys = result.getKeys();
                    if (VERBOSE) {
                        CaptureResult.Key<?>[] arrayKeys =
                                resultKeys.toArray(new CaptureResult.Key<?>[0]);
                        Log.v(TAG, "onCaptureCompleted - dumping keys: " +
                                Arrays.toString(arrayKeys));
                    }
                    info.append("Total keys: " + resultKeys.size());
                    info.append("\n");

                    if (mMainHandler != null) {
                        mMainHandler.post (new Runnable() {
                            @Override
                            public void run() {
                                // Update UI for capture result
                                mCaptureResultView.setText(info);
                            }
                        });
                    }
                }

                @Override
                public void onCaptureFailed(CameraCaptureSession session, CaptureRequest request,
                        CaptureFailure failure) {
                    Log.e(TAG, "Capture failed");
                }
    };

    private void logException(String msg, Throwable e) {
        Log.e(TAG, msg + Log.getStackTraceString(e));
    }

    private RadioGroup getRadioFmt() {
      return (RadioGroup)findViewById(R.id.radio_fmt);
    }

    private int getOutputFormat() {
        RadioGroup fmt = getRadioFmt();
        switch (fmt.getCheckedRadioButtonId()) {
            case R.id.radio_mp4:
                return MediaMuxer.OutputFormat.MUXER_OUTPUT_MPEG_4;

            case R.id.radio_webm:
                return MediaMuxer.OutputFormat.MUXER_OUTPUT_WEBM;

            default:
                throw new IllegalStateException("Checked button unrecognized.");
        }
    }

    private final OnSeekBarChangeListener mSensitivitySeekBarListener =
            new OnSeekBarChangeListener() {

              @Override
              public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                  Range<Integer> defaultRange = new Range<Integer>(MIN_SENSITIVITY,
                          MAX_SENSITIVITY);
                  CameraCharacteristics properties = mCameraOps.getCameraCharacteristics();
                  Range<Integer> sensitivityRange = properties.get(
                          CameraCharacteristics.SENSOR_INFO_SENSITIVITY_RANGE);
                  if (sensitivityRange == null || sensitivityRange.getLower() > MIN_SENSITIVITY ||
                          sensitivityRange.getUpper() < MAX_SENSITIVITY) {
                      Log.e(TAG, "unable to get sensitivity range, use default range");
                      sensitivityRange = defaultRange;
                  }
                  int min = sensitivityRange.getLower();
                  int max = sensitivityRange.getUpper();
                  float progressFactor = progress / (float)mSeekBarMax;
                  int curSensitivity = (int) (min + (max - min) * progressFactor);
                  curSensitivity = (curSensitivity / STEP_SIZE ) * STEP_SIZE;
                  mCameraControl.getManualControls().setSensitivity(curSensitivity);
                  // Update the sensitivity info
                  StringBuffer info = new StringBuffer("Sensitivity(ISO):");
                  info.append("" + curSensitivity);
                  mSensitivityInfoView.setText(info);
                  mCameraOps.updatePreview(mCameraControl);
              }

              @Override
              public void onStartTrackingTouch(SeekBar seekBar) {
              }

              @Override
              public void onStopTrackingTouch(SeekBar seekBar) {
              }
    };

    private final OnSeekBarChangeListener mExposureSeekBarListener =
            new OnSeekBarChangeListener() {

              @Override
              public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                  Range<Long> defaultRange = new Range<Long>(MIN_EXPOSURE, MAX_EXPOSURE);
                  CameraCharacteristics properties = mCameraOps.getCameraCharacteristics();
                  Range<Long> exposureRange = properties.get(
                          CameraCharacteristics.SENSOR_INFO_EXPOSURE_TIME_RANGE);
                  // Not enforce the max value check here, most of the devices don't support
                  // larger than 30s exposure time
                  if (exposureRange == null || exposureRange.getLower() > MIN_EXPOSURE ||
                          exposureRange.getUpper() < 0) {
                      exposureRange = defaultRange;
                      Log.e(TAG, "exposure time range is invalid, use default range");
                  }
                  long min = exposureRange.getLower();
                  long max = exposureRange.getUpper();
                  float progressFactor = progress / (float)mSeekBarMax;
                  long curExposureTime = (long) (min + (max - min) * progressFactor);
                  mCameraControl.getManualControls().setExposure(curExposureTime);
                  // Update the sensitivity info
                  StringBuffer info = new StringBuffer("Exposure Time:");
                  info.append("" + curExposureTime / 1000000.0 + "ms");
                  mExposureInfoView.setText(info);
                  mCameraOps.updatePreview(mCameraControl);
              }

              @Override
              public void onStartTrackingTouch(SeekBar seekBar) {
              }

              @Override
              public void onStopTrackingTouch(SeekBar seekBar) {
              }
    };

    private final OnSeekBarChangeListener mFrameDurationSeekBarListener =
            new OnSeekBarChangeListener() {

              @Override
              public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                  CameraCharacteristics properties = mCameraOps.getCameraCharacteristics();
                  Long frameDurationMax = properties.get(
                          CameraCharacteristics.SENSOR_INFO_MAX_FRAME_DURATION);
                  if (frameDurationMax == null || frameDurationMax <= 0) {
                      frameDurationMax = MAX_FRAME_DURATION;
                      Log.e(TAG, "max frame duration is invalid, set to " + frameDurationMax);
                  }
                  // Need calculate from different resolution, hard code to 10ms for now.
                  long min = 10000000L;
                  long max = frameDurationMax;
                  float progressFactor = progress / (float)mSeekBarMax;
                  long curFrameDuration = (long) (min + (max - min) * progressFactor);
                  mCameraControl.getManualControls().setFrameDuration(curFrameDuration);
                  // Update the sensitivity info
                  StringBuffer info = new StringBuffer("Frame Duration:");
                  info.append("" + curFrameDuration / 1000000.0 + "ms");
                  mFrameDurationInfoView.setText(info);
                  mCameraOps.updatePreview(mCameraControl);
              }

              @Override
              public void onStartTrackingTouch(SeekBar seekBar) {
              }

              @Override
              public void onStopTrackingTouch(SeekBar seekBar) {
              }
    };

    private final View.OnClickListener mControlToggleListener =
            new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            boolean enableManual;
            if (mManualCtrlToggle.isChecked()) {
                enableManual = true;
            } else {
                enableManual = false;
            }
            mCameraControl.getManualControls().setManualControlEnabled(enableManual);
            enableManualControls(enableManual);
            mCameraOps.updatePreview(mCameraControl);
        }
    };

    private final View.OnClickListener mRecordingToggleListener =
            new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            if (mRecordingToggle.isChecked()) {
                try {
                    Log.i(TAG, "start recording, useMediaCodec = " + mUseMediaCodec);
                    RadioGroup fmt = getRadioFmt();
                    fmt.setActivated(false);
                    mCameraOps.startRecording(
                            /* applicationContext */ TestingCamera2.this,
                            /* useMediaCodec */ mUseMediaCodec,
                            /* outputFormat */ getOutputFormat());
                } catch (ApiFailureException e) {
                    logException("Failed to start recording", e);
                }
            } else {
                try {
                    mCameraOps.stopRecording(TestingCamera2.this);
                    getRadioFmt().setActivated(true);
                } catch (ApiFailureException e) {
                    logException("Failed to stop recording", e);
                }
            }
        }
    };

    private final View.OnClickListener mFocusLockToggleListener =
            new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            if (VERBOSE) {
                Log.v(TAG, "focus_lock#onClick - start");
            }

            CameraAutoFocusControls afControls = mCameraControl.getAfControls();

            if (mFocusLockToggle.isChecked()) {
                Log.i(TAG, "lock focus");

                afControls.setPendingTriggerStart();
            } else {
                Log.i(TAG, "unlock focus");

                afControls.setPendingTriggerCancel();
            }

            mCameraOps.updatePreview(mCameraControl);

            if (VERBOSE) {
                Log.v(TAG, "focus_lock#onClick - end");
            }
        }
    };

    private final CompoundButton.OnCheckedChangeListener mUseMediaCodecListener =
            new CompoundButton.OnCheckedChangeListener() {
        @Override
        public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
            mUseMediaCodec = isChecked;
        }
    };

    private final CameraOps.Listener mCameraOpsListener = new CameraOps.Listener() {
        @Override
        public void onCameraOpened(String cameraId, CameraCharacteristics characteristics) {
            /*
             * Populate dynamic per-camera settings
             */

            // Map available AF Modes -> AF mode spinner dropdown list of strings
            int[] availableAfModes =
                    characteristics.get(CameraCharacteristics.CONTROL_AF_AVAILABLE_MODES);

            String[] allAfModes = getResources().getStringArray(R.array.focus_mode_spinner_arrays);

            final List<String> afModeList = new ArrayList<>();
            final int[] afModePositions = new int[availableAfModes.length];

            int i = 0;
            for (int mode : availableAfModes) {
                afModeList.add(allAfModes[mode]);
                afModePositions[i++] = mode;
            }

            ArrayAdapter<String> dataAdapter = new ArrayAdapter<>(TestingCamera2.this,
                    android.R.layout.simple_spinner_item, afModeList);
            dataAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
            mFocusModeSpinner.setAdapter(dataAdapter);

            /*
             * Change the AF mode and update preview when AF spinner's selected item changes
             */
            mFocusModeSpinner.setOnItemSelectedListener(new OnItemSelectedListener() {

                @Override
                public void onItemSelected(AdapterView<?> parent, View view, int position,
                        long id) {
                    int afMode = afModePositions[position];

                    Log.i(TAG, "Change auto-focus mode to " + afModeList.get(position)
                            + " " + afMode);

                    mCameraControl.getAfControls().setAfMode(afMode);

                    mCameraOps.updatePreview(mCameraControl);
                }

                @Override
                public void onNothingSelected(AdapterView<?> parent) {
                    // Do nothing
                }
            });
        }
    };
}
