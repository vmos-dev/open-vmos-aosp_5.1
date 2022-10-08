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
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.ToggleButton;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

/**
 * A base control pane, with standard abilities to collapse or remove itself.
 */
public class ControlPane extends LinearLayout {

    private final StatusListener mStatusListener;
    private String mPaneName = "unnamed";

    protected final PaneTracker mPaneTracker;

    private View[] mHeaderViews;

    private OnClickListener mRemoveButtonListener = new OnClickListener() {
        @Override
        public void onClick(View v) {
            remove();
        }
    };

    private OnClickListener mCollapseButtonListener = new OnClickListener() {
        private boolean mCollapsed = false;

        @Override
        public void onClick(View v) {
            if (mCollapsed) {
                mCollapsed = false;
                // Unhide all pane items
                for (int i = 0; i < ControlPane.this.getChildCount(); i++) {
                    ControlPane.this.getChildAt(i).setVisibility(VISIBLE);
                }
            } else {
                mCollapsed = true;
                // Hide all pane items
                for (int i = 0; i < ControlPane.this.getChildCount(); i++) {
                    ControlPane.this.getChildAt(i).setVisibility(GONE);
                }
                // Except for the header
                for (int i = 0; i < mHeaderViews.length; i++) {
                    mHeaderViews[i].setVisibility(VISIBLE);
                }
            }
        }
    };

    public ControlPane(Context context, AttributeSet attrs, StatusListener listener,
            PaneTracker paneTracker) {
        super(context, attrs);
        mStatusListener = listener;
        mPaneTracker = paneTracker;  // Parent takes care of adding pane to tracking

        this.setOrientation(VERTICAL);

        LayoutInflater inflater = (LayoutInflater)context.getSystemService
                (Context.LAYOUT_INFLATER_SERVICE);

        inflater.inflate(R.layout.control_pane_header, this);

        // Add all header views into list to manage collapsing pane correctly
        mHeaderViews = new View[getChildCount()];
        for (int i = 0; i < getChildCount(); i++) {
            mHeaderViews[i] = getChildAt(i);
        }

        // Set up the header controls
        ToggleButton collapseButton = (ToggleButton)
                findViewById(R.id.control_pane_collapse_button);
        collapseButton.setOnClickListener(mCollapseButtonListener);

        Button removeButton = (Button) findViewById(R.id.control_pane_remove_button);
        removeButton.setOnClickListener(mRemoveButtonListener);
    }

    /**
     * Add to pane tracking when the pane becomes part of the UI
     */
    @Override
    public void onAttachedToWindow() {
        super.onAttachedToWindow();
        if (mPaneTracker != null) {
            mPaneTracker.addPane(this);
        }
    }

    /**
     * Remove this pane from its list and clean up its state
     */
    public void remove() {
        if (mStatusListener != null) {
            mStatusListener.onRemoveRequested(ControlPane.this);
        }
        if (mPaneTracker != null ) {
            mPaneTracker.removePane(this);
        }
    }

    /**
     * Get a nice name for this pane.
     */
    public String getPaneName() {
        return mPaneName;
    }

    /**
     * Listener to be implemented by an application service that handles removing this
     * pane from the UI. Called when the pane is ready to be destroyed.
     */
    public interface StatusListener {
        public void onRemoveRequested(ControlPane p);
    }

    /**
     * Set the name for this pane, also used as the header title.
     */
    protected void setName(String name) {
        mPaneName = name;
        ((TextView) findViewById(R.id.control_pane_title_text)).setText(name);
    }

    /**
     * Get an XML attribute as a integer; if the attribute does not exist, return the default value.
     *
     * @throws XmlPullParserException if parser not at a START_TAG event, or the attribute is not
     *   formatted as a string.
     */
    protected static int getAttributeInt(XmlPullParser configParser,
            String attributeName, int defaultValue) throws XmlPullParserException {
        String value = configParser.getAttributeValue(null, attributeName);
        if (value == null ) return defaultValue;

        try {
            int v = Integer.parseInt(value);
            return v;
        } catch (NumberFormatException e) {
            throw new XmlPullParserException("Expected integer attribute",
                    configParser, e);
        }
    }

    /**
     * Get an XML attribute as a String; if the attribute does not exist, return the default value.
     *
     * @throws XmlPullParserException if parser not at a START_TAG event.
     */
    protected static String getAttributeString(
            XmlPullParser configParser,
            String attributeName, String defaultValue) throws XmlPullParserException {
        String value = configParser.getAttributeValue(null, attributeName);
        return (value == null) ? defaultValue : value;
    }

    /**
     * Called when other panes want to inform the rest of the app of interesting events
     *
     * @param sourcePane the source pane of the event
     * @param event the type of event
     */
    public void notifyPaneEvent(ControlPane sourcePane, PaneTracker.PaneEvent event) {
        // Default empty implementation
    }

    /**
     * Called when the app's UI orientation changes.
     *
     * @param orientation one of the Surface.ROTATION_* constants
     */
    public void onOrientationChange(int orientation) {
        // Default empty implementation
    }

}
