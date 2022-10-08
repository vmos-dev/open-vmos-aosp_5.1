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
import android.view.LayoutInflater;
import android.view.Surface;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.Spinner;
import android.widget.ToggleButton;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

import java.io.IOException;
import java.util.List;
import java.util.Locale;

public class TargetControlPane extends ControlPane {
    // XML attributes

    /** Name of pane tag */
    private static final String PANE_NAME = "target_pane";

    /** Attribute: ID for pane (integer) */
    private static final String PANE_ID = "id";

    /** Attribute: Type of output (string), value must be one of OutputViewType.getXmlName() */
    private static final String OUTPUT_TYPE = "type";

    // End XML attributes

    /**
     * Available output view types
     *
     * <p>Adding new entries to this list requires updating createOutputView() as
     * well.</p>
     */
    private enum OutputViewType {
        SURFACE_VIEW,
        TEXTURE_VIEW,
        IMAGE_READER,
        RENDERSCRIPT,
        MEDIA_RECORDER,
        MEDIA_CODEC;

        static String getXmlName(OutputViewType type) {
            return type.toString().toLowerCase(Locale.US);
        }
    }

    private static int mTargetPaneIdCounter = 0;

    private int mPaneId;

    private List<CameraControlPane> mCameraPanes;

    private Spinner mCameraSpinner;
    private ToggleButton mCameraConfigureToggle;

    private Spinner mOutputSpinner;

    private TargetSubPane mCurrentOutput;

    private int mOrientation = 0;

    /**
     * Constructor for tooling only
     */
    public TargetControlPane(Context context, AttributeSet attrs) {
        super(context, attrs, null, null);

        mPaneId = 0;
        setUpUI(context);
    }

    public TargetControlPane(TestingCamera21 tc, AttributeSet attrs, StatusListener listener) {
        super(tc, attrs, listener, tc.getPaneTracker());

        mPaneId = mTargetPaneIdCounter++;
        setUpUI(tc);

    }

    public TargetControlPane(TestingCamera21 tc, XmlPullParser configParser, StatusListener listener)
            throws XmlPullParserException, IOException {
        super(tc, null, listener, tc.getPaneTracker());

        configParser.require(XmlPullParser.START_TAG, XmlPullParser.NO_NAMESPACE, PANE_NAME);

        int paneId = getAttributeInt(configParser, PANE_ID, -1);
        if (paneId == -1) {
            mPaneId = mTargetPaneIdCounter++;
        } else {
            mPaneId = paneId;
            if (mPaneId >= mTargetPaneIdCounter) {
                mTargetPaneIdCounter = mPaneId + 1;
            }
        }

        configParser.next();
        configParser.require(XmlPullParser.END_TAG, XmlPullParser.NO_NAMESPACE, PANE_NAME);

        setUpUI(tc);
    }

    /**
     * Get this target's Surface aimed at the given camera pane. If no target
     * for that camera is defined by this pane, returns null.
     *
     * @param paneName ID of the camera pane to return a Surface for
     * @return a Surface to configure for the camera device, or null if none
     *         available
     */
    public Surface getTargetSurfaceForCameraPane(String paneName) {
        if (paneName == null || mCurrentOutput == null) return null;

        boolean isMyTarget =
                paneName.equals(mCameraSpinner.getSelectedItem()) &&
                mCameraConfigureToggle.isChecked();
        return isMyTarget ? mCurrentOutput.getOutputSurface() : null;
    }

    public void notifyPaneEvent(ControlPane sourcePane, PaneTracker.PaneEvent event) {
        switch (event) {
        case NEW_CAMERA_SELECTED:
            if (mCameraPanes.size() > 0
                    && sourcePane == mCameraPanes.get(mCameraSpinner.getSelectedItemPosition())) {
                if (mCurrentOutput != null) {
                    mCurrentOutput.setTargetCameraPane((CameraControlPane) sourcePane);
                }
            }
            break;
        default:
            super.notifyPaneEvent(sourcePane, event);
        }
    }

    public void onOrientationChange(int orientation) {
        mOrientation = orientation;
        if (mCurrentOutput != null) {
            mCurrentOutput.setUiOrientation(mOrientation);
        }
    }

    private void setUpUI(Context context) {

        String paneTitle =
                String.format(Locale.US, "%s %d",
                        context.getResources().getString(R.string.target_pane_title), mPaneId);
        this.setName(paneTitle);

        LayoutInflater inflater =
                (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);

        inflater.inflate(R.layout.target_pane, this);

        mCameraSpinner = (Spinner) findViewById(R.id.target_pane_camera_spinner);
        mCameraSpinner.setOnItemSelectedListener(mCameraSpinnerListener);
        mCameraConfigureToggle = (ToggleButton) findViewById(R.id.target_pane_configure_toggle);
        mCameraConfigureToggle.setChecked(true);

        mOutputSpinner = (Spinner) findViewById(R.id.target_pane_output_spinner);
        mOutputSpinner.setOnItemSelectedListener(mOutputSpinnerListener);

        OutputViewType[] outputTypes = OutputViewType.values();
        String[] outputSpinnerItems = new String[outputTypes.length];

        for (int i = 0; i < outputTypes.length; i++) {
            outputSpinnerItems[i] = outputTypes[i].toString();
        }
        mOutputSpinner.setAdapter(new ArrayAdapter<String>(getContext(), R.layout.spinner_item,
                outputSpinnerItems));

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

    private OnItemSelectedListener mCameraSpinnerListener = new OnItemSelectedListener() {
        @Override
        public void onItemSelected(AdapterView<?> parent, View view, int pos, long id) {
            updateSubPaneCamera();
        }

        @Override
        public void onNothingSelected(AdapterView<?> arg0) {

        }
    };

    private OnItemSelectedListener mOutputSpinnerListener = new OnItemSelectedListener() {
        @Override
        public void onItemSelected(AdapterView<?> parent, View view, int pos, long id) {
            if (mCurrentOutput != null) {
                TargetControlPane.this.removeView(mCurrentOutput);
            }
            OutputViewType outputType =
                    OutputViewType.valueOf((String) mOutputSpinner.getSelectedItem());
            mCurrentOutput = createOutputView(outputType);
            if (mCurrentOutput != null) {
                TargetControlPane.this.addView(mCurrentOutput);
                mCurrentOutput.setUiOrientation(mOrientation);
                updateSubPaneCamera();
            }
        }

        @Override
        public void onNothingSelected(AdapterView<?> arg0) {
            if (mCurrentOutput != null) {
                TargetControlPane.this.removeView(mCurrentOutput);
                mCurrentOutput = null;
            }
        }
    };

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
    }

    private void updateSubPaneCamera() {
        if (mCameraPanes.size() > 0 && mCurrentOutput != null) {
            mCurrentOutput.setTargetCameraPane(mCameraPanes.get(mCameraSpinner
                    .getSelectedItemPosition()));
        }
    }

    private TargetSubPane createOutputView(OutputViewType type) {
        TargetSubPane newPane = null;
        switch (type) {
            case IMAGE_READER:
                newPane = new ImageReaderSubPane(getContext(), null);
                break;
            case MEDIA_CODEC:
            case MEDIA_RECORDER:
            case RENDERSCRIPT:
                TLog.e("No implementation yet for view type %s", type);
                break;
            case SURFACE_VIEW:
                newPane = new SurfaceViewSubPane(getContext(), null);
                break;
            case TEXTURE_VIEW:
                newPane = new TextureViewSubPane(getContext(), null);
                break;
        }
        return newPane;
    }
}
