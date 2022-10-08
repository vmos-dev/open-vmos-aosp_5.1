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
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CaptureRequest;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.Surface;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ListView;
import android.widget.Spinner;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

public class RequestControlPane extends ControlPane {
    // XML attributes

    /** Name of pane tag */
    private static final String PANE_NAME = "request_pane";

    /** Attribute: ID for pane (integer) */
    private static final String PANE_ID = "id";

    // End XML attributes

    private enum TemplateType {
        MANUAL(CameraDevice.TEMPLATE_MANUAL),
        PREVIEW(CameraDevice.TEMPLATE_PREVIEW),
        RECORD(CameraDevice.TEMPLATE_RECORD),
        STILL_CAPTURE(CameraDevice.TEMPLATE_STILL_CAPTURE),
        VIDEO_SNAPSHOT(CameraDevice.TEMPLATE_VIDEO_SNAPSHOT),
        ZERO_SHUTTER_LAG(CameraDevice.TEMPLATE_ZERO_SHUTTER_LAG);

        private int mTemplateVal;

        TemplateType(int templateVal) {
            mTemplateVal = templateVal;
        }

        int getTemplateValue() {
            return mTemplateVal;
        }
    }

    private static int mRequestPaneIdCounter = 0;

    private final int mPaneId;

    private List<CameraControlPane> mCameraPanes;
    private final List<TargetControlPane> mTargetPanes = new ArrayList<TargetControlPane>();

    private Spinner mCameraSpinner;
    private Spinner mTemplateSpinner;
    private ListView mOutputListView;

    private CheckableListAdapter mOutputAdapter;

    /**
     * Constructor for tooling only
     */
    public RequestControlPane(Context context, AttributeSet attrs) {
        super(context, attrs, null, null);

        mPaneId = 0;
        setUpUI(context);
    }

    public RequestControlPane(TestingCamera21 tc, AttributeSet attrs, StatusListener listener) {
        super(tc, attrs, listener, tc.getPaneTracker());

        mPaneId = mRequestPaneIdCounter++;
        setUpUI(tc);
    }

    public RequestControlPane(TestingCamera21 tc, XmlPullParser configParser,
            StatusListener listener) throws XmlPullParserException, IOException {
        super(tc, null, listener, tc.getPaneTracker());

        this.setName(tc.getResources().getString(R.string.request_pane_title));

        configParser.require(XmlPullParser.START_TAG, XmlPullParser.NO_NAMESPACE, PANE_NAME);

        int paneId = getAttributeInt(configParser, PANE_ID, -1);
        if (paneId == -1) {
            mPaneId = mRequestPaneIdCounter++;
        } else {
            mPaneId = paneId;
            if (mPaneId >= mRequestPaneIdCounter) {
                mRequestPaneIdCounter = mPaneId + 1;
            }
        }

        configParser.next();
        configParser.require(XmlPullParser.END_TAG, XmlPullParser.NO_NAMESPACE, PANE_NAME);

        setUpUI(tc);
    }

    public void notifyPaneEvent(ControlPane sourcePane, PaneTracker.PaneEvent event) {
        switch (event) {
            case NEW_CAMERA_SELECTED:
                if (mCameraPanes.size() > 0
                        && sourcePane == mCameraPanes.get(mCameraSpinner.getSelectedItemPosition())) {
                    updateOutputList();
                }
                break;
            case CAMERA_CONFIGURED:
                if (mCameraPanes.size() > 0
                        && sourcePane == mCameraPanes.get(mCameraSpinner.getSelectedItemPosition())) {
                    updateOutputList();
                }
                break;
            default:
                super.notifyPaneEvent(sourcePane, event);
        }
    }

    private void setUpUI(Context context) {
        String paneName =
                String.format(Locale.US, "%s %d",
                        context.getResources().getString(R.string.request_pane_title), mPaneId);
        this.setName(paneName);

        LayoutInflater inflater =
                (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);

        inflater.inflate(R.layout.request_pane, this);

        Button captureButton = (Button) findViewById(R.id.request_pane_capture_button);
        captureButton.setOnClickListener(mCaptureButtonListener);
        Button repeatButton = (Button) findViewById(R.id.request_pane_repeat_button);
        repeatButton.setOnClickListener(mRepeatButtonListener);

        mCameraSpinner = (Spinner) findViewById(R.id.request_pane_camera_spinner);
        mTemplateSpinner = (Spinner) findViewById(R.id.request_pane_template_spinner);
        mOutputListView = (ListView) findViewById(R.id.request_pane_output_listview);

        mOutputAdapter = new CheckableListAdapter(context, R.layout.checkable_list_item,
                new ArrayList<CheckableListAdapter.CheckableItem>());
        mOutputListView.setAdapter(mOutputAdapter);

        String[] templateNames = new String[TemplateType.values().length];
        for (int i = 0; i < templateNames.length; i++) {
            templateNames[i] = TemplateType.values()[i].toString();
        }
        mTemplateSpinner.setAdapter(new ArrayAdapter<String>(getContext(), R.layout.spinner_item,
                templateNames));

        mPaneTracker.addPaneListener(new CameraPanesListener());
        mCameraPanes = mPaneTracker.getPanes(CameraControlPane.class);
        updateCameraPaneList();
    }

    private class CameraPanesListener extends PaneTracker.PaneSetChangedListener<CameraControlPane> {
        public CameraPanesListener() {
            super(CameraControlPane.class);
        }

        @Override
        public void onPaneAdded(ControlPane pane) {
            mCameraPanes.add((CameraControlPane) pane);
            updateCameraPaneList();
        }

        @Override
        public void onPaneRemoved(ControlPane pane) {
            mCameraPanes.remove((CameraControlPane) pane);
            updateCameraPaneList();
        }
    }

    private OnClickListener mCaptureButtonListener = new OnClickListener() {
        @Override
        public void onClick(View v) {
            if (mCameraPanes.size() == 0) {
                TLog.e("No camera selected for request");
                return;
            }
            CameraControlPane camera = mCameraPanes.get(mCameraSpinner.getSelectedItemPosition());

            CaptureRequest request = createRequest(camera);
            if (request != null) {
                camera.capture(request);
            }
        }
    };

    private OnClickListener mRepeatButtonListener = new OnClickListener() {
        @Override
        public void onClick(View v) {
            if (mCameraPanes.size() == 0) {
                TLog.e("No camera selected for request");
                return;
            }
            CameraControlPane camera = mCameraPanes.get(mCameraSpinner.getSelectedItemPosition());

            CaptureRequest request = createRequest(camera);
            if (request != null) {
                camera.repeat(request);
            }
        }
    };

    private CaptureRequest createRequest(CameraControlPane camera) {
        if (mTargetPanes.size() == 0) {
            TLog.e("No target(s) selected for request");
            return null;
        }


        TemplateType template = TemplateType.valueOf((String) mTemplateSpinner.getSelectedItem());
        CaptureRequest.Builder builder = camera.getRequestBuilder(template.getTemplateValue());
        // TODO: Add setting overrides

        List<Integer> targetPostions = mOutputAdapter.getCheckedPositions();
        for (int i : targetPostions) {
            TargetControlPane target = mTargetPanes.get(i);
            Surface targetSurface = target.getTargetSurfaceForCameraPane(camera.getPaneName());
            if (targetSurface == null) {
                TLog.e("Target not configured for camera");
                return null;
            }
            builder.addTarget(targetSurface);
        }

        CaptureRequest request = builder.build();
        return request;
    }

    private void updateCameraPaneList() {
        String currentSelection = (String) mCameraSpinner.getSelectedItem();
        int newSelectionIndex = 0;
        String[] cameraSpinnerItems = new String[mCameraPanes.size()];
        for (int i = 0; i < cameraSpinnerItems.length; i++) {
            cameraSpinnerItems[i] = mCameraPanes.get(i).getPaneName();
            if (cameraSpinnerItems[i].equals(currentSelection)) {
                newSelectionIndex = i;
            }
        }
        mCameraSpinner.setAdapter(new ArrayAdapter<String>(getContext(), R.layout.spinner_item,
                cameraSpinnerItems));
        mCameraSpinner.setSelection(newSelectionIndex);

        updateOutputList();
    }

    private void updateOutputList() {
        if (mCameraPanes.size() > 0) {
            CameraControlPane currentCamera =
                    mCameraPanes.get(mCameraSpinner.getSelectedItemPosition());
            mTargetPanes.clear();
            List<TargetControlPane> newPanes = currentCamera.getCurrentConfiguredTargets();
            if (newPanes != null) {
                mTargetPanes.addAll(currentCamera.getCurrentConfiguredTargets());
            }

            String[] outputSpinnerItems = new String[mTargetPanes.size()];
            for (int i = 0; i < outputSpinnerItems.length; i++) {
                outputSpinnerItems[i] = mTargetPanes.get(i).getPaneName();
            }

            mOutputAdapter.updateItems(outputSpinnerItems);

        }
    }

}
