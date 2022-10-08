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

import android.util.AttributeSet;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

import java.io.IOException;

public class BurstControlPane extends ControlPane {

    // XML attributes

    /** Name of pane tag */
    private static final String PANE_NAME = "burst_pane";

    // End XML attributes

    public BurstControlPane(TestingCamera21 tc, AttributeSet attrs, StatusListener listener) {
        super(tc, attrs, listener, tc.getPaneTracker());

        this.setName(tc.getResources().getString(R.string.burst_pane_title));
    }

    public BurstControlPane(TestingCamera21 tc, XmlPullParser configParser, StatusListener listener)
            throws XmlPullParserException, IOException {
        super(tc, null, listener, tc.getPaneTracker());

        this.setName(tc.getResources().getString(R.string.request_pane_title));

        configParser.require(XmlPullParser.START_TAG,
                XmlPullParser.NO_NAMESPACE, PANE_NAME);
        // Parse attributes here
        configParser.next();
        configParser.require(XmlPullParser.END_TAG,
                XmlPullParser.NO_NAMESPACE, PANE_NAME);

    }

}
