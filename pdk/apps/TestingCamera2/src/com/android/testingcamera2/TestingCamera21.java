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

import android.app.Activity;
import android.content.res.Configuration;
import android.os.Bundle;
import android.util.TypedValue;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup.LayoutParams;
import android.view.WindowManager;
import android.widget.LinearLayout;
import android.widget.ScrollView;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

public class TestingCamera21 extends Activity implements CameraControlPane.InfoDisplayer {

    private LinearLayout mMainList;
    private ScrollView mOutputViewScroller;
    private ScrollView mControlScroller;
    private View mMainDivider;

    private PaneLayout<CameraControlPane> mCameraControlList;
    private PaneLayout<TargetControlPane> mTargetControlList;
    private PaneLayout<RequestControlPane> mRequestControlList;
    private PaneLayout<BurstControlPane> mBurstControlList;
    private PaneLayout<UtilControlPane> mUtilControlList;
    private List<PaneLayout<?>> mPaneLayouts = new ArrayList<PaneLayout<?>>();

    private CameraOps2 mCameraOps;
    private PaneTracker mPaneTracker;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main3);

        mCameraOps = new CameraOps2(this);
        mPaneTracker = new PaneTracker();

        TLog.Logger logger;
        logger = setUpUIAndLog();
        TLog.setLogger(logger);
    }

    private TLog.Logger setUpUIAndLog() {

        mMainList = (LinearLayout) findViewById(R.id.main_list);
        mOutputViewScroller = (ScrollView) findViewById(R.id.output_view_scroller);
        mControlScroller = (ScrollView) findViewById(R.id.control_scroller);
        mMainDivider = (View) findViewById(R.id.main_section_divider);

        onConfigurationChanged(getResources().getConfiguration());

        LogPane logPane = (LogPane) findViewById(R.id.log_pane);

        mTargetControlList = (PaneLayout<TargetControlPane>) findViewById(R.id.target_list);
        mPaneLayouts.add(mTargetControlList);

        mCameraControlList = (PaneLayout<CameraControlPane>) findViewById(R.id.camera_list);
        mPaneLayouts.add(mCameraControlList);

        mRequestControlList = (PaneLayout<RequestControlPane>) findViewById(R.id.request_list);
        mPaneLayouts.add(mRequestControlList);

        mBurstControlList = (PaneLayout<BurstControlPane>) findViewById(R.id.burst_list);
        mPaneLayouts.add(mBurstControlList);

        mUtilControlList = (PaneLayout<UtilControlPane>) findViewById(R.id.util_list);
        mPaneLayouts.add(mUtilControlList);

        return logPane;
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        /**
         * In landscape, place target view list left of the control pane list In
         * portrait, place target view list above the control pane list.
         */
        int width = 0;
        int height = 0;
        if (newConfig.orientation == Configuration.ORIENTATION_LANDSCAPE) {
            mMainList.setOrientation(LinearLayout.HORIZONTAL);
            height = LayoutParams.MATCH_PARENT;
        } else if (newConfig.orientation == Configuration.ORIENTATION_PORTRAIT) {
            mMainList.setOrientation(LinearLayout.VERTICAL);
            width = LayoutParams.MATCH_PARENT;
        }

        int thickness =
                getResources().getDimensionPixelSize(R.dimen.main_section_divider_thickness);
        LinearLayout.LayoutParams mainDividerLayout =
                new LinearLayout.LayoutParams(
                        (width == 0) ? thickness : width,
                        (height == 0) ? thickness : height);
        mMainDivider.setLayoutParams(mainDividerLayout);

        TypedValue outputViewListWeight = new TypedValue();
        getResources().getValue(R.dimen.output_view_list_weight, outputViewListWeight, false);
        LinearLayout.LayoutParams outputScrollerLayout =
                new LinearLayout.LayoutParams(width, height, outputViewListWeight.getFloat());
        mOutputViewScroller.setLayoutParams(outputScrollerLayout);

        TypedValue controlListWeight = new TypedValue();
        getResources().getValue(R.dimen.control_list_weight, controlListWeight, false);
        LinearLayout.LayoutParams controlScrollerLayout =
                new LinearLayout.LayoutParams(width, height, controlListWeight.getFloat());
        mControlScroller.setLayoutParams(controlScrollerLayout);

        WindowManager windowManager =  (WindowManager) getSystemService(WINDOW_SERVICE);
        int rot = windowManager.getDefaultDisplay().getRotation();

        mPaneTracker.notifyOrientationChange(windowManager.getDefaultDisplay().getRotation());

    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu to the action bar
        getMenuInflater().inflate(R.menu.testing_camera21, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here.
        int id = item.getItemId();
        boolean done = true;

        switch (id) {
        case R.id.action_add_camera:
            mCameraControlList.addPane(this);
            break;
        case R.id.action_add_target:
            mTargetControlList.addPane(this);
            break;
        case R.id.action_add_request:
            mRequestControlList.addPane(this);
            break;
        case R.id.action_add_burst:
            mBurstControlList.addPane(this);
            break;
        case R.id.action_add_utility:
            mUtilControlList.addPane(this);
            break;
        case R.id.action_load_config:
            selectConfig();
            break;
        case R.id.action_save_config:
            TLog.e("Saving a configuration is not yet implemented");
            break;
        default:
            done = false;
            break;
        }

        if (done)
            return true;

        return super.onOptionsItemSelected(item);
    }

    /**
     * Get shared camera controls
     */
    public CameraOps2 getCameraOps() {
        return mCameraOps;
    }

    /**
     * Get shared pane tracker
     */
    public PaneTracker getPaneTracker() {
        return mPaneTracker;
    }

    /**
     * Select which layout configuration to load, and then load it TODO:
     * Implement selecting something besides the default
     */
    private void selectConfig() {
        XmlPullParser config = getResources().getXml(R.xml.still_camera);
        readConfig(config);
    }

    /**
     * Parse a selected XML configuration
     *
     * @param configParser
     *            the XML parser to read from
     */
    private void readConfig(XmlPullParser configParser) {
        boolean inConfig = false;
        boolean gotConfig = false;
        try {
            while (configParser.getEventType() != XmlPullParser.END_DOCUMENT) {
                int eventType = configParser.next();
                switch (eventType) {
                case XmlPullParser.START_TAG:
                    if (!inConfig) {
                        configParser.require(XmlPullParser.START_TAG, XmlPullParser.NO_NAMESPACE,
                                "testingcamera2_config");
                        inConfig = true;
                    } else {
                        for (PaneLayout<?> paneLayout : mPaneLayouts) {
                            if (configParser.getName().equals(paneLayout.getXmlName())) {
                                paneLayout.readConfig(this, configParser);
                                break;
                            }
                        }
                    }
                    break;
                case XmlPullParser.END_TAG:
                    configParser.require(XmlPullParser.END_TAG, XmlPullParser.NO_NAMESPACE,
                            "testingcamera2_config");
                    inConfig = false;
                    gotConfig = true;
                    break;
                }
            }
        } catch (XmlPullParserException e) {
            TLog.e("Unable to parse new config.", e);
        } catch (IOException e) {
            TLog.e("Unable to read new config.", e);
        }

        if (gotConfig) {
            for (PaneLayout<?> paneLayout : mPaneLayouts) {
                paneLayout.activateConfig();
            }
        } else {
            for (PaneLayout<?> paneLayout : mPaneLayouts) {
                paneLayout.clearConfig();
            }
        }
    }

    @Override
    public void showCameraInfo(String cameraId) {
        final String infoTag = "camera_info_dialog";

        CameraInfoDialogFragment infoDialog = new CameraInfoDialogFragment();
        infoDialog.setCameraId(cameraId);
        infoDialog.show(getFragmentManager(), infoTag);
    }
}
