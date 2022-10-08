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
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Locale;
import java.util.Set;

import android.content.Context;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.Surface;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.ToggleButton;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraCaptureSession.CaptureCallback;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.CaptureResult;
import android.hardware.camera2.TotalCaptureResult;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

import com.android.testingcamera2.PaneTracker.PaneEvent;

import java.io.IOException;

/**
 *
 * Basic control pane block for the control list
 *
 */
public class CameraControlPane extends ControlPane {

    // XML attributes

    /** Name of pane tag */
    private static final String PANE_NAME = "camera_pane";

    /** Attribute: ID for pane (integer) */
    private static final String PANE_ID = "id";
    /** Attribute: ID for camera to select (String) */
    private static final String CAMERA_ID = "camera_id";

    // End XML attributes

    private static final int MAX_CACHED_RESULTS = 100;

    private static int mCameraPaneIdCounter = 0;

    /**
     * These correspond to the callbacks from
     * android.hardware.camera2.CameraDevice.StateCallback, plus UNAVAILABLE for
     * when there's not a valid camera selected.
     */
    private enum CameraState {
        UNAVAILABLE,
        CLOSED,
        OPENED,
        DISCONNECTED,
        ERROR
    }

    /**
     * These correspond to the callbacks from {@link CameraCaptureSession.StateCallback}, plus
     * {@code CONFIGURING} for before a session is returned and {@code NONE} for when there
     * is no session created.
     */
    private enum SessionState {
        NONE,
        CONFIGURED,
        CONFIGURE_FAILED,
        READY,
        ACTIVE,
        CLOSED
    }

    private enum CameraCall {
        NONE,
        CONFIGURE
    }

    private final int mPaneId;

    private CameraOps2 mCameraOps;
    private InfoDisplayer mInfoDisplayer;

    private Spinner mCameraSpinner;
    private ToggleButton mOpenButton;
    private Button mInfoButton;
    private TextView mStatusText;
    private Button mConfigureButton;
    private Button mStopButton;
    private Button mFlushButton;

    /**
     * All controls that should be enabled when there's a valid camera ID
     * selected
     */
    private final Set<View> mBaseControls = new HashSet<View>();
    /**
     * All controls that should be enabled when camera is at least in the OPEN
     * state
     */
    private final Set<View> mOpenControls = new HashSet<View>();
    /**
     * All controls that should be enabled when camera is at least in the IDLE
     * state
     */
    private final Set<View> mConfiguredControls = new HashSet<View>();

    private String[] mCameraIds;
    private String mCurrentCameraId;

    private CameraState mCameraState;
    private CameraDevice mCurrentCamera;
    private CameraCaptureSession mCurrentCaptureSession;
    private SessionState mSessionState = SessionState.NONE;
    private CameraCall mActiveCameraCall;
    private LinkedList<TotalCaptureResult> mRecentResults = new LinkedList<>();

    private List<Surface> mConfiguredSurfaces;
    private List<TargetControlPane> mConfiguredTargetPanes;

    /**
     * Constructor for tooling only
     */
    public CameraControlPane(Context context, AttributeSet attrs) {
        super(context, attrs, null, null);

        mPaneId = 0;
        setUpUI(context);
    }

    public CameraControlPane(TestingCamera21 tc, AttributeSet attrs, StatusListener listener) {

        super(tc, attrs, listener, tc.getPaneTracker());

        mPaneId = mCameraPaneIdCounter++;
        setUpUI(tc);
        initializeCameras(tc);

        if (mCameraIds != null) {
            switchToCamera(mCameraIds[0]);
        }
    }

    public CameraControlPane(TestingCamera21 tc, XmlPullParser configParser, StatusListener listener)
            throws XmlPullParserException, IOException {
        super(tc, null, listener, tc.getPaneTracker());

        configParser.require(XmlPullParser.START_TAG, XmlPullParser.NO_NAMESPACE, PANE_NAME);

        int paneId = getAttributeInt(configParser, PANE_ID, -1);
        if (paneId == -1) {
            mPaneId = mCameraPaneIdCounter++;
        } else {
            mPaneId = paneId;
            if (mPaneId >= mCameraPaneIdCounter) {
                mCameraPaneIdCounter = mPaneId + 1;
            }
        }

        String cameraId = getAttributeString(configParser, CAMERA_ID, null);

        configParser.next();
        configParser.require(XmlPullParser.END_TAG, XmlPullParser.NO_NAMESPACE, PANE_NAME);

        setUpUI(tc);
        initializeCameras(tc);

        boolean gotCamera = false;
        if (mCameraIds != null && cameraId != null) {
            for (int i = 0; i < mCameraIds.length; i++) {
                if (cameraId.equals(mCameraIds[i])) {
                    switchToCamera(mCameraIds[i]);
                    mCameraSpinner.setSelection(i);
                    gotCamera = true;
                }
            }
        }

        if (!gotCamera && mCameraIds != null) {
            switchToCamera(mCameraIds[0]);
        }
    }

    @Override
    public void remove() {
        closeCurrentCamera();
        super.remove();
    }

    /**
     * Get list of target panes that are currently actively configured for this
     * camera
     */
    public List<TargetControlPane> getCurrentConfiguredTargets() {
        return mConfiguredTargetPanes;
    }

    /**
     * Interface to be implemented by an application service for displaying a
     * camera's information.
     */
    public interface InfoDisplayer {
        public void showCameraInfo(String cameraId);
    }

    public CameraCharacteristics getCharacteristics() {
        if (mCurrentCameraId != null) {
            return mCameraOps.getCameraInfo(mCurrentCameraId);
        }
        return null;
    }

    public CaptureRequest.Builder getRequestBuilder(int template) {
        CaptureRequest.Builder request = null;
        if (mCurrentCamera != null) {
            try {
                request = mCurrentCamera.createCaptureRequest(template);
                // Workaround for b/15748139
                request.set(CaptureRequest.STATISTICS_LENS_SHADING_MAP_MODE,
                        CaptureRequest.STATISTICS_LENS_SHADING_MAP_MODE_ON);
            } catch (CameraAccessException e) {
                TLog.e("Unable to build request for camera %s with template %d.", e,
                        mCurrentCameraId, template);
            }
        }
        return request;
    }

    /**
     * Send single capture to camera device.
     *
     * @param request
     * @return true if capture sent successfully
     */
    public boolean capture(CaptureRequest request) {
        if (mCurrentCaptureSession != null) {
            try {
                mCurrentCaptureSession.capture(request, mResultListener, null);
                return true;
            } catch (CameraAccessException e) {
                TLog.e("Unable to capture for camera %s.", e, mCurrentCameraId);
            }
        }
        return false;
    }

    public boolean repeat(CaptureRequest request) {
        if (mCurrentCaptureSession != null) {
            try {
                mCurrentCaptureSession.setRepeatingRequest(request, mResultListener, null);
                return true;
            } catch (CameraAccessException e) {
                TLog.e("Unable to set repeating request for camera %s.", e, mCurrentCameraId);
            }
        }
        return false;
    }

    public TotalCaptureResult getResultAt(long timestamp) {
        for (TotalCaptureResult result : mRecentResults) {
            long resultTimestamp = result.get(CaptureResult.SENSOR_TIMESTAMP);
            if (resultTimestamp == timestamp) return result;
            if (resultTimestamp > timestamp) return null;
        }
        return null;
    }

    private CaptureCallback mResultListener = new CaptureCallback() {
        public void onCaptureCompleted(
                CameraCaptureSession session,
                CaptureRequest request,
                TotalCaptureResult result) {
            mRecentResults.add(result);
            if (mRecentResults.size() > MAX_CACHED_RESULTS) {
                mRecentResults.remove();
            }
        }
    };

    private void setUpUI(Context context) {
        String paneName =
                String.format(Locale.US, "%s %c",
                        context.getResources().getString(R.string.camera_pane_title),
                        (char) ('A' + mPaneId));
        this.setName(paneName);

        LayoutInflater inflater =
                (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);

        inflater.inflate(R.layout.camera_pane, this);

        mCameraSpinner = (Spinner) findViewById(R.id.camera_pane_camera_spinner);
        mCameraSpinner.setOnItemSelectedListener(mCameraSpinnerListener);

        mOpenButton = (ToggleButton) findViewById(R.id.camera_pane_open_button);
        mOpenButton.setOnCheckedChangeListener(mOpenButtonListener);
        mBaseControls.add(mOpenButton);

        mInfoButton = (Button) findViewById(R.id.camera_pane_info_button);
        mInfoButton.setOnClickListener(mInfoButtonListener);
        mBaseControls.add(mInfoButton);

        mStatusText = (TextView) findViewById(R.id.camera_pane_status_text);

        mConfigureButton = (Button) findViewById(R.id.camera_pane_configure_button);
        mConfigureButton.setOnClickListener(mConfigureButtonListener);
        mOpenControls.add(mConfigureButton);

        mStopButton = (Button) findViewById(R.id.camera_pane_stop_button);
        mStopButton.setOnClickListener(mStopButtonListener);
        mConfiguredControls.add(mStopButton);
        mFlushButton = (Button) findViewById(R.id.camera_pane_flush_button);
        mFlushButton.setOnClickListener(mFlushButtonListener);
        mConfiguredControls.add(mFlushButton);
    }

    private void initializeCameras(TestingCamera21 tc) {
        mCameraOps = tc.getCameraOps();
        mInfoDisplayer = tc;

        updateCameraList();
    }

    private void updateCameraList() {
        mCameraIds = null;
        try {
            mCameraIds = mCameraOps.getCamerasAndListen(mCameraAvailabilityCallback);
            String[] cameraSpinnerItems = new String[mCameraIds.length];
            for (int i = 0; i < mCameraIds.length; i++) {
                cameraSpinnerItems[i] = String.format("Camera %s", mCameraIds[i]);
            }
            mCameraSpinner.setAdapter(new ArrayAdapter<String>(getContext(), R.layout.spinner_item,
                    cameraSpinnerItems));

        } catch (CameraAccessException e) {
            TLog.e("Exception trying to get list of cameras: " + e);
        }
    }

    private final CompoundButton.OnCheckedChangeListener mOpenButtonListener =
            new CompoundButton.OnCheckedChangeListener() {
                @Override
                public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                    if (isChecked) {
                        // Open camera
                        mCurrentCamera = null;
                        boolean success = mCameraOps.openCamera(mCurrentCameraId, mCameraListener);
                        buttonView.setChecked(success);
                    } else {
                        // Close camera
                        closeCurrentCamera();
                    }
                }
            };

    private final OnClickListener mInfoButtonListener = new OnClickListener() {
        @Override
        public void onClick(View v) {
            mInfoDisplayer.showCameraInfo(mCurrentCameraId);
        }
    };

    private final OnClickListener mStopButtonListener = new OnClickListener() {
        @Override
        public void onClick(View v) {
            if (mCurrentCaptureSession != null) {
                try {
                    mCurrentCaptureSession.stopRepeating();
                } catch (CameraAccessException e) {
                    TLog.e("Unable to stop repeating request for camera %s.", e, mCurrentCameraId);
                }
            }
        }
    };

    private final OnClickListener mFlushButtonListener = new OnClickListener() {
        @Override
        public void onClick(View v) {
            if (mCurrentCaptureSession != null) {
                try {
                    mCurrentCaptureSession.abortCaptures();
                } catch (CameraAccessException e) {
                    TLog.e("Unable to flush camera %s.", e, mCurrentCameraId);
                }
            }
        }
    };

    private final OnClickListener mConfigureButtonListener = new OnClickListener() {
        @Override
        public void onClick(View v) {
            List<Surface> targetSurfaces = new ArrayList<Surface>();
            List<TargetControlPane> targetPanes = new ArrayList<TargetControlPane>();
            for (TargetControlPane targetPane : mPaneTracker.getPanes(TargetControlPane.class)) {
                Surface target = targetPane.getTargetSurfaceForCameraPane(getPaneName());
                if (target != null) {
                    targetSurfaces.add(target);
                    targetPanes.add(targetPane);
                }
            }
            try {
                TLog.i("Configuring camera %s with %d surfaces", mCurrentCamera.getId(),
                        targetSurfaces.size());
                mActiveCameraCall = CameraCall.CONFIGURE;
                if (targetSurfaces.size() > 0) {
                    mCurrentCamera.createCaptureSession(targetSurfaces, mSessionListener, /*handler*/null);
                } else if (mCurrentCaptureSession != null) {
                    mCurrentCaptureSession.close();
                    mCurrentCaptureSession = null;
                }
                mConfiguredSurfaces = targetSurfaces;
                mConfiguredTargetPanes = targetPanes;
            } catch (CameraAccessException e) {
                mActiveCameraCall = CameraCall.NONE;
                TLog.e("Unable to configure camera %s.", e, mCurrentCamera.getId());
            } catch (IllegalArgumentException e) {
                mActiveCameraCall = CameraCall.NONE;
                TLog.e("Unable to configure camera %s.", e, mCurrentCamera.getId());
            } catch (IllegalStateException e) {
                mActiveCameraCall = CameraCall.NONE;
                TLog.e("Unable to configure camera %s.", e, mCurrentCamera.getId());
            }
        }
    };

    private final CameraCaptureSession.StateCallback mSessionListener =
            new CameraCaptureSession.StateCallback() {

        @Override
        public void onConfigured(CameraCaptureSession session) {
            mCurrentCaptureSession = session;
            TLog.i("Configuration completed for camera %s.", mCurrentCamera.getId());

            setSessionState(SessionState.CONFIGURED);
        }

        @Override
        public void onConfigureFailed(CameraCaptureSession session) {
            mActiveCameraCall = CameraCall.NONE;
            TLog.e("Configuration failed for camera %s.", mCurrentCamera.getId());

            setSessionState(SessionState.CONFIGURE_FAILED);
        }

        @Override
        public void onReady(CameraCaptureSession session) {
            setSessionState(SessionState.READY);
        }

        /**
         * This method is called when the session starts actively processing capture requests.
         *
         * <p>If capture requests are submitted prior to {@link #onConfigured} being called,
         * then the session will start processing those requests immediately after the callback,
         * and this method will be immediately called after {@link #onConfigured}.
         *
         * <p>If the session runs out of capture requests to process and calls {@link #onReady},
         * then this callback will be invoked again once new requests are submitted for capture.</p>
         */
        @Override
        public void onActive(CameraCaptureSession session) {
            setSessionState(SessionState.ACTIVE);
        }

        /**
         * This method is called when the session is closed.
         *
         * <p>A session is closed when a new session is created by the parent camera device,
         * or when the parent camera device is closed (either by the user closing the device,
         * or due to a camera device disconnection or fatal error).</p>
         *
         * <p>Once a session is closed, all methods on it will throw an IllegalStateException, and
         * any repeating requests or bursts are stopped (as if {@link #stopRepeating()} was called).
         * However, any in-progress capture requests submitted to the session will be completed
         * as normal.</p>
         */
        @Override
        public void onClosed(CameraCaptureSession session) {
            // Ignore closes if the session has been replaced
            if (mCurrentCaptureSession != null && session != mCurrentCaptureSession) {
                return;
            }
            setSessionState(SessionState.CLOSED);
        }
    };

    private final CameraDevice.StateCallback mCameraListener = new CameraDevice.StateCallback() {
        @Override
        public void onClosed(CameraDevice camera) {
            // Don't change state on close, tracked by callers of close()
        }

        @Override
        public void onDisconnected(CameraDevice camera) {
            setCameraState(CameraState.DISCONNECTED);
        }

        @Override
        public void onError(CameraDevice camera, int error) {
            setCameraState(CameraState.ERROR);
        }

        @Override
        public void onOpened(CameraDevice camera) {
            mCurrentCamera = camera;
            setCameraState(CameraState.OPENED);
        }
    };

    private void switchToCamera(String newCameraId) {
        closeCurrentCamera();

        mCurrentCameraId = newCameraId;

        if (mCurrentCameraId == null) {
            setCameraState(CameraState.UNAVAILABLE);
        } else {
            setCameraState(CameraState.CLOSED);
        }

        mPaneTracker.notifyOtherPanes(this, PaneTracker.PaneEvent.NEW_CAMERA_SELECTED);
    }

    private void closeCurrentCamera() {
        if (mCurrentCamera != null) {
            mCurrentCamera.close();
            mCurrentCamera = null;
            setCameraState(CameraState.CLOSED);
            mOpenButton.setChecked(false);
        }
    }

    private void setSessionState(SessionState newState) {
        mSessionState = newState;
        mStatusText.setText("S." + mSessionState.toString());

        switch (mSessionState) {
            case CONFIGURE_FAILED:
                mActiveCameraCall = CameraCall.NONE;
                // fall-through
            case CLOSED:
                enableBaseControls(true);
                enableOpenControls(true);
                enableConfiguredControls(false);
                mConfiguredTargetPanes = null;
                break;
            case NONE:
                enableBaseControls(true);
                enableOpenControls(true);
                enableConfiguredControls(false);
                mConfiguredTargetPanes = null;
                break;
            case CONFIGURED:
                if (mActiveCameraCall != CameraCall.CONFIGURE) {
                    throw new AssertionError();
                }
                mPaneTracker.notifyOtherPanes(this, PaneEvent.CAMERA_CONFIGURED);
                mActiveCameraCall = CameraCall.NONE;
                // fall-through
            case READY:
            case ACTIVE:
                enableBaseControls(true);
                enableOpenControls(true);
                enableConfiguredControls(true);
                break;
            default:
                throw new AssertionError("Unhandled case " + mSessionState);
        }
    }

    private void setCameraState(CameraState newState) {
        mCameraState = newState;
        mStatusText.setText("C." + mCameraState.toString());
        switch (mCameraState) {
            case UNAVAILABLE:
                enableBaseControls(false);
                enableOpenControls(false);
                enableConfiguredControls(false);
                mConfiguredTargetPanes = null;
                break;
            case CLOSED:
            case DISCONNECTED:
            case ERROR:
                enableBaseControls(true);
                enableOpenControls(false);
                enableConfiguredControls(false);
                mConfiguredTargetPanes = null;
                break;
            case OPENED:
                enableBaseControls(true);
                enableOpenControls(true);
                enableConfiguredControls(false);
                mConfiguredTargetPanes = null;
                break;
        }
    }

    private void enableBaseControls(boolean enabled) {
        for (View v : mBaseControls) {
            v.setEnabled(enabled);
        }
        if (!enabled) {
            mOpenButton.setChecked(false);
        }
    }

    private void enableOpenControls(boolean enabled) {
        for (View v : mOpenControls) {
            v.setEnabled(enabled);
        }
    }

    private void enableConfiguredControls(boolean enabled) {
        for (View v : mConfiguredControls) {
            v.setEnabled(enabled);
        }
    }

    private final CameraManager.AvailabilityCallback mCameraAvailabilityCallback =
            new CameraManager.AvailabilityCallback() {
        @Override
        public void onCameraAvailable(String cameraId) {
            // TODO: Update camera list in an intelligent fashion
            // (can't just call updateCameraList or the selected camera may change)
        }

        @Override
        public void onCameraUnavailable(String cameraId) {
            // TODO: Update camera list in an intelligent fashion
            // (can't just call updateCameraList or the selected camera may change)
        }
    };

    private final OnItemSelectedListener mCameraSpinnerListener = new OnItemSelectedListener() {
        @Override
        public void onItemSelected(AdapterView<?> parent, View view, int pos, long id) {
            String newCameraId = mCameraIds[pos];
            if (newCameraId != mCurrentCameraId) {
                switchToCamera(newCameraId);
            }
        }

        @Override
        public void onNothingSelected(AdapterView<?> parent) {
            switchToCamera(null);
        }
    };
}
