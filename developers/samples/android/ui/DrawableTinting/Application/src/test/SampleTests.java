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
package android.ui.DrawableTinting.Application.src.test;

import android.graphics.Color;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffColorFilter;
import android.test.ActivityInstrumentationTestCase2;
import android.test.UiThreadTest;
import android.ui.DrawableTinting.Application.src.main.java.com.example.android.drawabletinting.DrawableTintingFragment;
import android.view.View;
import android.widget.ImageView;
import android.widget.SeekBar;
import android.widget.Spinner;

import com.example.android.drawabletinting.MainActivity;
import com.example.android.drawabletinting.R;

/**
 * Tests for drawabletinting sample.
 */
public class SampleTests extends ActivityInstrumentationTestCase2<MainActivity> {

    private MainActivity mTestActivity;
    private DrawableTintingFragment mTestFragment;

    View mFragmentView;
    SeekBar mAlpha;
    SeekBar mRed;
    SeekBar mGreen;
    SeekBar mBlue;
    Spinner mBlendMode;
    ImageView mImage;

    public SampleTests() {
        super(MainActivity.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        // Starts the activity under test using the default Intent with:
        // action = {@link Intent#ACTION_MAIN}
        // flags = {@link Intent#FLAG_ACTIVITY_NEW_TASK}
        // All other fields are null or empty.
        mTestActivity = getActivity();
        mTestFragment = (DrawableTintingFragment)
                mTestActivity.getSupportFragmentManager().getFragments().get(1);

        final View fragmentView = mTestFragment.getView();
        mAlpha = (SeekBar) fragmentView.findViewById(R.id.alphaSeek);
        mRed = (SeekBar) fragmentView.findViewById(R.id.redSeek);
        mGreen = (SeekBar) fragmentView.findViewById(R.id.greenSeek);
        mBlue = (SeekBar) fragmentView.findViewById(R.id.blueSeek);
        mBlendMode = (Spinner) fragmentView.findViewById(R.id.blendSpinner);
        mImage = (ImageView) fragmentView.findViewById(R.id.image);
    }

    /**
     * Test if the test fixture has been set up correctly.
     */
    public void testPreconditions() {
        //Try to add a message to add context to your assertions. These messages will be shown if
        //a tests fails and make it easy to understand why a test failed
        assertNotNull("mTestActivity is null", mTestActivity);
        assertNotNull("mTestFragment is null", mTestFragment);
    }

    /**
     * Test the initial state of all UI elements, color and blend mode.
     */
    public void testInitialState() {
        assertEquals(255, mAlpha.getProgress());
        assertEquals(0, mRed.getProgress());
        assertEquals(0, mGreen.getProgress());
        assertEquals(0, mBlue.getProgress());
        assertEquals("Add", (String) mBlendMode.getSelectedItem());

        PorterDuffColorFilter colorFilter = (PorterDuffColorFilter) mImage.getColorFilter();
        assertNotNull(colorFilter);
        int initialColor = Color.BLACK;
        assertEquals(initialColor, colorFilter.getColor());
        assertEquals(PorterDuff.Mode.ADD, colorFilter.getMode());
    }

    /**
     * Test application of blend modes.
     */
    @UiThreadTest
    public void testModes() {
        final int testColor = Color.GREEN;
        // Test that each tint mode can be successfully applied and matches a valid PorterDuff mode
        final PorterDuff.Mode[] modes = PorterDuff.Mode.values();

        // Test that each blend mode can be applied
        for (PorterDuff.Mode m : modes) {
            mTestFragment.updateTint(testColor, m);
            final PorterDuffColorFilter filter = (PorterDuffColorFilter) mImage.getColorFilter();
            assertEquals(m, filter.getMode());
            assertEquals(testColor, filter.getColor());
        }
    }

    /**
     * Test the color computation from ARGB Seekbars.
     */
    public void testColor() {
        // Red
        mAlpha.setProgress(255);
        mRed.setProgress(255);
        mBlue.setProgress(0);
        mGreen.setProgress(0);
        assertEquals(Color.RED, mTestFragment.getColor());

        // Blue
        mAlpha.setProgress(255);
        mRed.setProgress(0);
        mBlue.setProgress(255);
        mGreen.setProgress(0);
        assertEquals(Color.BLUE, mTestFragment.getColor());

        // Green
        mAlpha.setProgress(255);
        mRed.setProgress(0);
        mBlue.setProgress(0);
        mGreen.setProgress(255);
        assertEquals(Color.GREEN, mTestFragment.getColor());

        // Black
        mAlpha.setProgress(255);
        mRed.setProgress(0);
        mBlue.setProgress(0);
        mGreen.setProgress(0);
        assertEquals(Color.BLACK, mTestFragment.getColor());

        // Transparent
        mAlpha.setProgress(0);
        mRed.setProgress(0);
        mBlue.setProgress(0);
        mGreen.setProgress(0);
        assertEquals(Color.TRANSPARENT, mTestFragment.getColor());
    }

}
