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
import android.content.res.TypedArray;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.ToggleButton;

import java.util.List;
import java.util.ArrayList;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

import java.io.IOException;

/**
 * A simple linear layout to hold a set of control panes, with the ability to
 * add more panes, and to serialize itself in/out of configurations.
 */
public abstract class PaneLayout<T extends ControlPane> extends LinearLayout implements
        ControlPane.StatusListener {

    private static final String DEFAULT_XML_PANE_NAME = "pane_layout";

    private List<T> mPanes = new ArrayList<T>();

    private List<T> mNewPanes = new ArrayList<T>();

    private final String mPaneXmlName;
    private PaneTracker mPaneTracker;

    private OnClickListener mCollapseButtonListener = new OnClickListener() {
        private boolean mCollapsed = false;

        @Override
        public void onClick(View v) {
            if (mCollapsed) {
                mCollapsed = false;
                // Unhide all panes
                for (T pane : mPanes) {
                    pane.setVisibility(VISIBLE);
                }
            } else {
                mCollapsed = true;
                // Hide all panes
                for (T pane : mPanes) {
                    pane.setVisibility(GONE);
                }
            }
        }
    };

    public PaneLayout(Context context, AttributeSet attrs) {
        super(context, attrs);

        mPaneXmlName = DEFAULT_XML_PANE_NAME;
        mPaneTracker = null;

        setUpUI(context, attrs);
    }

    public PaneLayout(Context context, AttributeSet attrs, String paneXmlName) {
        super(context, attrs);

        mPaneXmlName = paneXmlName;

        setUpUI(context, attrs);
    }

    public void setPaneTracker(PaneTracker tracker) {
        mPaneTracker = tracker;
    }

    public String getXmlName() {
        return mPaneXmlName;
    }

    private void setUpUI(Context context, AttributeSet attrs) {

        LayoutInflater inflater =
                (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);

        inflater.inflate(R.layout.pane_layout_header, this);

        TextView title = (TextView) findViewById(R.id.pane_layout_title_text);

        TypedArray a =
                context.getTheme().obtainStyledAttributes(attrs, R.styleable.PaneLayout, 0, 0);

        try {
            title.setText(a.getString(R.styleable.PaneLayout_headerTitle));
        } finally {
            a.recycle();
        }

        ToggleButton collapseButton = (ToggleButton) findViewById(R.id.pane_layout_collapse_button);
        collapseButton.setOnClickListener(mCollapseButtonListener);
    }

    public void readConfig(TestingCamera21 tc, XmlPullParser configParser)
            throws XmlPullParserException, IOException {

        configParser.require(XmlPullParser.START_TAG, XmlPullParser.NO_NAMESPACE, mPaneXmlName);
        int eventType = configParser.next();
        while (eventType != XmlPullParser.END_TAG) {
            switch (eventType) {
            case XmlPullParser.START_TAG:
                T pane = readControlPane(tc, configParser);
                mNewPanes.add(pane);
                break;
            }
            eventType = configParser.next();
        }
        configParser.require(XmlPullParser.END_TAG, XmlPullParser.NO_NAMESPACE, mPaneXmlName);
    }

    public void clearConfig() {
        mNewPanes.clear();
    }

    public void activateConfig() {
        ArrayList<T> oldPanes = new ArrayList<T>(mPanes);
        for (T pane : oldPanes) {
            // This will call back to onRemoveRequested
            pane.remove();
        }

        for (T newPane : mNewPanes) {
            addPane(newPane);
        }
        clearConfig();
    }

    public void addPane(TestingCamera21 tc) {
        T newPane = createControlPane(tc, null);
        addPane(newPane);
    }

    private void addPane(T newPane) {
        mPanes.add(newPane);
        if (mPaneTracker != null) {
            mPaneTracker.addPane(newPane);
        }
        addView(newPane);
    }

    public void onRemoveRequested(ControlPane p) {
        removeView(p);
        if (mPaneTracker != null) {
            mPaneTracker.removePane(p);
        }
        mPanes.remove(p);
    }

    protected abstract T createControlPane(TestingCamera21 tc, AttributeSet attrs);

    protected abstract T readControlPane(TestingCamera21 tc, XmlPullParser configParser)
            throws XmlPullParserException, IOException;

    static class TargetPaneLayout extends PaneLayout<TargetControlPane> {
        private static final String PANE_XML_NAME = "target_panes";

        public TargetPaneLayout(Context context, AttributeSet attrs) {
            super(context, attrs, PANE_XML_NAME);
        }

        protected TargetControlPane createControlPane(TestingCamera21 tc, AttributeSet attrs) {
            return new TargetControlPane(tc, attrs, this);
        }

        protected TargetControlPane readControlPane(TestingCamera21 tc, XmlPullParser configParser)
                throws XmlPullParserException, IOException {
            return new TargetControlPane(tc, configParser, this);
        }
    }

    static class CameraPaneLayout extends PaneLayout<CameraControlPane> {
        private static final String PANE_XML_NAME = "camera_panes";

        public CameraPaneLayout(Context context, AttributeSet attrs) {
            super(context, attrs, PANE_XML_NAME);
        }

        protected CameraControlPane createControlPane(TestingCamera21 tc, AttributeSet attrs) {
            return new CameraControlPane(tc, attrs, this);
        }

        protected CameraControlPane readControlPane(TestingCamera21 tc, XmlPullParser configParser)
                throws XmlPullParserException, IOException {
            return new CameraControlPane(tc, configParser, this);
        }
    }

    static class RequestPaneLayout extends PaneLayout<RequestControlPane> {
        private static final String PANE_XML_NAME = "request_panes";

        public RequestPaneLayout(Context context, AttributeSet attrs) {
            super(context, attrs, PANE_XML_NAME);
        }

        protected RequestControlPane createControlPane(TestingCamera21 tc, AttributeSet attrs) {
            return new RequestControlPane(tc, attrs, this);
        }

        protected RequestControlPane readControlPane(TestingCamera21 tc, XmlPullParser configParser)
                throws XmlPullParserException, IOException {
            return new RequestControlPane(tc, configParser, this);
        }

    }

    static class BurstPaneLayout extends PaneLayout<BurstControlPane> {
        private static final String PANE_XML_NAME = "burst_panes";

        public BurstPaneLayout(Context context, AttributeSet attrs) {
            super(context, attrs, PANE_XML_NAME);
        }

        protected BurstControlPane createControlPane(TestingCamera21 tc, AttributeSet attrs) {
            return new BurstControlPane(tc, attrs, this);
        }

        protected BurstControlPane readControlPane(TestingCamera21 tc, XmlPullParser configParser)
                throws XmlPullParserException, IOException {
            return new BurstControlPane(tc, configParser, this);
        }
    }

    static class UtilPaneLayout extends PaneLayout<UtilControlPane> {
        private static final String PANE_XML_NAME = "util_panes";

        public UtilPaneLayout(Context context, AttributeSet attrs) {
            super(context, attrs, PANE_XML_NAME);
        }

        public UtilControlPane createControlPane(TestingCamera21 tc, AttributeSet attrs) {
            return new UtilControlPane(tc, attrs, this);
        }

        protected UtilControlPane readControlPane(TestingCamera21 tc, XmlPullParser configParser)
                throws XmlPullParserException, IOException {
            return new UtilControlPane(tc, configParser, this);
        }
    }

}