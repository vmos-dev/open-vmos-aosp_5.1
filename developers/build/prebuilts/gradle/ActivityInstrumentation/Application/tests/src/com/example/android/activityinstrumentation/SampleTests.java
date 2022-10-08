/*
 * Copyright 2013 The Android Open Source Project
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

package com.example.android.activityinstrumentation;

import android.app.Activity;
import android.test.ActivityInstrumentationTestCase2;
import android.widget.Spinner;

import com.example.android.activityinstrumentation.MainActivity;
import com.example.android.activityinstrumentation.R;

/**
 * This is a simple framework for a test of an Application.  See
 * {@link android.test.ApplicationTestCase ApplicationTestCase} for more information on
 * how to write and extend Application tests.
 *
 * <p>To run this test, you can type:
 * adb shell am instrument -w \
 * -e class com.example.android.activityinstrumentation.MainActivityTest \
 * quux.tests/android.test.InstrumentationTestRunner
 *
 * <p>Individual tests are defined as any method beginning with 'test'.
 *
 * <p>ActivityInstrumentationTestCase2 allows these tests to run alongside a running
 * copy of the application under inspection. Calling getActivity() will return a
 * handle to this activity (launching it if needed).
 */
public class SampleTests extends ActivityInstrumentationTestCase2<MainActivity> {

    public SampleTests() {
        super("com.example.android.activityinstrumentation", MainActivity.class);
    }

    /**
     * Test to make sure that spinner values are persisted across activity restarts.
     *
     * <p>Launches the main activity, sets a spinner value, closes the activity, then relaunches
     * that activity. Checks to make sure that the spinner values match what we set them to.
     */
    // BEGIN_INCLUDE (test_name)
    public void testSpinnerValuePersistedBetweenLaunches() {
        // END_INCLUDE (test_name)
        final int TEST_SPINNER_POSITION_1 = MainActivity.WEATHER_PARTLY_CLOUDY;

        // BEGIN_INCLUDE (launch_activity)
        // Launch the activity
        Activity activity = getActivity();
        // END_INCLUDE (launch_activity)

        // BEGIN_INCLUDE (write_to_ui)
        // Set spinner to test position 1
        final Spinner spinner1 = (Spinner) activity.findViewById(R.id.spinner);
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                // Attempts to manipulate the UI must be performed on a UI thread.
                // Calling this outside runOnUiThread() will cause an exception.
                //
                // You could also use @UiThreadTest, but activity lifecycle methods
                // cannot be called if this annotation is used.
                spinner1.requestFocus();
                spinner1.setSelection(TEST_SPINNER_POSITION_1);
            }
        });
        // END_INCLUDE (write_to_ui)

        // BEGIN_INCLUDE (relaunch_activity)
        // Close the activity
        activity.finish();
        setActivity(null);  // Required to force creation of a new activity

        // Relaunch the activity
        activity = this.getActivity();
        // END_INCLUDE (relaunch_activity)

        // BEGIN_INCLUDE (check_results)
        // Verify that the spinner was saved at position 1
        final Spinner spinner2 = (Spinner) activity.findViewById(R.id.spinner);
        int currentPosition = spinner2.getSelectedItemPosition();
        assertEquals(TEST_SPINNER_POSITION_1, currentPosition);
        // END_INCLUDE (check_results)

        // Since this is a stateful test, we need to make sure that the activity isn't simply
        // echoing a previously-stored value that (coincidently) matches position 1
        final int TEST_SPINNER_POSITION_2 = MainActivity.WEATHER_SNOW;

        // Set spinner to test position 2
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                spinner2.requestFocus();
                spinner2.setSelection(TEST_SPINNER_POSITION_2);
            }
        });

        // Close the activity
        activity.finish();
        setActivity(null);

        // Relaunch the activity
        activity = this.getActivity();

        // Verify that the spinner was saved at position 2
        final Spinner spinner3 = (Spinner) activity.findViewById(R.id.spinner);
        currentPosition = spinner3.getSelectedItemPosition();
        assertEquals(TEST_SPINNER_POSITION_2, currentPosition);
    }
}
