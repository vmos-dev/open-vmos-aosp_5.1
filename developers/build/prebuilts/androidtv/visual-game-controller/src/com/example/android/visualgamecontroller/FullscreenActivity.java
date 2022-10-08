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

package com.example.android.visualgamecontroller;

import android.annotation.TargetApi;
import android.app.Activity;
import android.content.Context;
import android.hardware.input.InputManager;
import android.hardware.input.InputManager.InputDeviceListener;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;

import com.example.android.visualgamecontroller.util.SystemUiHider;

import java.util.ArrayList;

/**
 * An example full-screen activity that shows and hides the system UI (i.e.
 * status bar and navigation/system bar) with user interaction.
 * 
 * @see SystemUiHider
 */
public class FullscreenActivity extends Activity implements InputDeviceListener {
    private static final String TAG = "FullscreenActivity";

    /**
     * Whether or not the system UI should be auto-hidden after
     * {@link #AUTO_HIDE_DELAY_MILLIS} milliseconds.
     */
    private static final boolean AUTO_HIDE = true;

    /**
     * If {@link #AUTO_HIDE} is set, the number of milliseconds to wait after
     * user interaction before hiding the system UI.
     */
    private static final int AUTO_HIDE_DELAY_MILLIS = 3000;

    /**
     * If set, will toggle the system UI visibility upon interaction. Otherwise,
     * will show the system UI visibility upon interaction.
     */
    private static final boolean TOGGLE_ON_CLICK = true;

    /**
     * The flags to pass to {@link SystemUiHider#getInstance}.
     */
    private static final int HIDER_FLAGS = SystemUiHider.FLAG_HIDE_NAVIGATION;

    /**
     * The instance of the {@link SystemUiHider} for this activity.
     */
    private SystemUiHider mSystemUiHider;

    private ControllerView mControllerView;

    public enum ButtonMapping {
        BUTTON_A(KeyEvent.KEYCODE_BUTTON_A),
        BUTTON_B(KeyEvent.KEYCODE_BUTTON_B),
        BUTTON_X(KeyEvent.KEYCODE_BUTTON_X),
        BUTTON_Y(KeyEvent.KEYCODE_BUTTON_Y),
        BUTTON_L1(KeyEvent.KEYCODE_BUTTON_L1),
        BUTTON_R1(KeyEvent.KEYCODE_BUTTON_R1),
        BUTTON_L2(KeyEvent.KEYCODE_BUTTON_L2),
        BUTTON_R2(KeyEvent.KEYCODE_BUTTON_R2),
        BUTTON_SELECT(KeyEvent.KEYCODE_BUTTON_SELECT),
        BUTTON_START(KeyEvent.KEYCODE_BUTTON_START),
        BUTTON_THUMBL(KeyEvent.KEYCODE_BUTTON_THUMBL),
        BUTTON_THUMBR(KeyEvent.KEYCODE_BUTTON_THUMBR),
        BACK(KeyEvent.KEYCODE_BACK),
        POWER(KeyEvent.KEYCODE_BUTTON_MODE);

        private final int mKeyCode;

        ButtonMapping(int keyCode) {
            mKeyCode = keyCode;
        }

        private int getKeycode() {
            return mKeyCode;
        }
    }

    public enum AxesMapping {
        AXIS_X(MotionEvent.AXIS_X),
        AXIS_Y(MotionEvent.AXIS_Y),
        AXIS_Z(MotionEvent.AXIS_Z),
        AXIS_RZ(MotionEvent.AXIS_RZ),
        AXIS_HAT_X(MotionEvent.AXIS_HAT_X),
        AXIS_HAT_Y(MotionEvent.AXIS_HAT_Y),
        AXIS_LTRIGGER(MotionEvent.AXIS_LTRIGGER),
        AXIS_RTRIGGER(MotionEvent.AXIS_RTRIGGER),
        AXIS_BRAKE(MotionEvent.AXIS_BRAKE),
        AXIS_GAS(MotionEvent.AXIS_GAS);

        private final int mMotionEvent;

        AxesMapping(int motionEvent) {
            mMotionEvent = motionEvent;
        }

        private int getMotionEvent() {
            return mMotionEvent;
        }
    }

    private int[] mButtons = new int[ButtonMapping.values().length];
    private float[] mAxes = new float[AxesMapping.values().length];
    private InputManager mInputManager;
    private ArrayList<Integer> mConnectedDevices = new ArrayList<Integer>();
    private int mCurrentDeviceId = -1;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        setContentView(R.layout.activity_fullscreen);

        final View controlsView = findViewById(R.id.fullscreen_content_controls);
        final View contentView = findViewById(R.id.fullscreen_content);

        mControllerView = (ControllerView) findViewById(R.id.controller);
        for (int i = 0; i < mButtons.length; i++) {
            mButtons[i] = 0;
        }
        for (int i = 0; i < mAxes.length; i++) {
            mAxes[i] = 0.0f;
        }
        mControllerView.setButtonsAxes(mButtons, mAxes);

        // Set up an instance of SystemUiHider to control the system UI for
        // this activity.
        mSystemUiHider = SystemUiHider.getInstance(this, contentView, HIDER_FLAGS);
        mSystemUiHider.setup();
        mSystemUiHider
                .setOnVisibilityChangeListener(new SystemUiHider.OnVisibilityChangeListener() {
                    // Cached values.
                    int mControlsHeight;
                    int mShortAnimTime;

                    @Override
                    @TargetApi(Build.VERSION_CODES.HONEYCOMB_MR2)
                    public void onVisibilityChange(boolean visible) {
                        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB_MR2) {
                            // If the ViewPropertyAnimator API is available
                            // (Honeycomb MR2 and later), use it to animate the
                            // in-layout UI controls at the bottom of the
                            // screen.
                            if (mControlsHeight == 0) {
                                mControlsHeight = controlsView.getHeight();
                            }
                            if (mShortAnimTime == 0) {
                                mShortAnimTime = getResources().getInteger(
                                        android.R.integer.config_shortAnimTime);
                            }
                            controlsView.animate()
                                    .translationY(visible ? 0 : mControlsHeight)
                                    .setDuration(mShortAnimTime);
                        } else {
                            // If the ViewPropertyAnimator APIs aren't
                            // available, simply show or hide the in-layout UI
                            // controls.
                            controlsView.setVisibility(visible ? View.VISIBLE : View.GONE);
                        }

                        if (visible && AUTO_HIDE) {
                            // Schedule a hide().
                            delayedHide(AUTO_HIDE_DELAY_MILLIS);
                        }
                    }
                });

        // Set up the user interaction to manually show or hide the system UI.
        contentView.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (TOGGLE_ON_CLICK) {
                    mSystemUiHider.toggle();
                } else {
                    mSystemUiHider.show();
                }
            }
        });

        mInputManager = (InputManager) getSystemService(Context.INPUT_SERVICE);
        checkGameControllers();
    }

    /**
     * Check for any game controllers that are connected already.
     */
    private void checkGameControllers() {
        Log.d(TAG, "checkGameControllers");
        int[] deviceIds = mInputManager.getInputDeviceIds();
        for (int deviceId : deviceIds) {
            InputDevice dev = InputDevice.getDevice(deviceId);
            int sources = dev.getSources();

            // Verify that the device has gamepad buttons, control sticks, or
            // both.
            if (((sources & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD)
                    || ((sources & InputDevice.SOURCE_JOYSTICK)
                        == InputDevice.SOURCE_JOYSTICK)) {
                // This device is a game controller. Store its device ID.
                if (!mConnectedDevices.contains(deviceId)) {
                    mConnectedDevices.add(deviceId);
                    if (mCurrentDeviceId == -1) {
                        mCurrentDeviceId = deviceId;
                        mControllerView.setCurrentControllerNumber(dev.getControllerNumber());
                        mControllerView.invalidate();
                    }
                }
            }
        }
    }

    @Override
    protected void onPostCreate(Bundle savedInstanceState) {
        super.onPostCreate(savedInstanceState);

        // Trigger the initial hide() shortly after the activity has been
        // created, to briefly hint to the user that UI controls
        // are available.
        delayedHide(100);
    }

    @Override
    protected void onResume() {
        super.onResume();
        mInputManager.registerInputDeviceListener(this, null);
    }

    @Override
    protected void onPause() {
        super.onPause();
        mInputManager.unregisterInputDeviceListener(this);
    }

    /**
     * Touch listener to use for in-layout UI controls to delay hiding the
     * system UI. This is to prevent the jarring behavior of controls going away
     * while interacting with activity UI.
     */
    View.OnTouchListener mDelayHideTouchListener = new View.OnTouchListener() {
        @Override
        public boolean onTouch(View view, MotionEvent motionEvent) {
            if (AUTO_HIDE) {
                delayedHide(AUTO_HIDE_DELAY_MILLIS);
            }
            return false;
        }
    };

    Handler mHideHandler = new Handler();
    Runnable mHideRunnable = new Runnable() {
        @Override
        public void run() {
            mSystemUiHider.hide();
        }
    };

    /**
     * Schedules a call to hide() in [delay] milliseconds, canceling any
     * previously scheduled calls.
     */
    private void delayedHide(int delayMillis) {
        mHideHandler.removeCallbacks(mHideRunnable);
        mHideHandler.postDelayed(mHideRunnable, delayMillis);
    }

    /*
     * (non-Javadoc)
     * @see android.app.Activity#onGenericMotionEvent(android.view.MotionEvent)
     */
    @Override
    public boolean onGenericMotionEvent(final MotionEvent ev) {
        // Log.d(TAG, "onGenericMotionEvent: " + ev);
        InputDevice device = ev.getDevice();
        // Only care about game controllers.
        if (device != null && device.getId() == mCurrentDeviceId) {
            if (isGamepad(device)) {
                for (AxesMapping axesMapping : AxesMapping.values()) {
                    mAxes[axesMapping.ordinal()] = getCenteredAxis(ev, device,
                            axesMapping.getMotionEvent());
                }
                mControllerView.invalidate();
                return true;
            }
        }
        return super.onGenericMotionEvent(ev);
    }

    /**
     * Get centered position for axis input by considering flat area.
     * 
     * @param event
     * @param device
     * @param axis
     * @return
     */
    private float getCenteredAxis(MotionEvent event, InputDevice device, int axis) {
        InputDevice.MotionRange range = device.getMotionRange(axis, event.getSource());

        // A joystick at rest does not always report an absolute position of
        // (0,0). Use the getFlat() method to determine the range of values
        // bounding the joystick axis center.
        if (range != null) {
            float flat = range.getFlat();
            float value = event.getAxisValue(axis);

            // Ignore axis values that are within the 'flat' region of the
            // joystick axis center.
            if (Math.abs(value) > flat) {
                return value;
            }
        }
        return 0;
    }

    /*
     * (non-Javadoc)
     * @see android.support.v4.app.FragmentActivity#onKeyDown(int,
     * android.view.KeyEvent)
     */
    @Override
    public boolean onKeyDown(final int keyCode, KeyEvent ev) {
        // Log.d(TAG, "onKeyDown: " + ev);
        InputDevice device = ev.getDevice();
        // Only care about game controllers.
        if (device != null && device.getId() == mCurrentDeviceId) {
            if (isGamepad(device)) {
                int index = getButtonMappingIndex(keyCode);
                if (index >= 0) {
                    mButtons[index] = 1;
                    mControllerView.invalidate();
                }
                return true;
            }
        }
        return super.onKeyDown(keyCode, ev);
    }

    /*
     * (non-Javadoc)
     * @see android.app.Activity#onKeyUp(int, android.view.KeyEvent)
     */
    @Override
    public boolean onKeyUp(final int keyCode, KeyEvent ev) {
        // Log.d(TAG, "onKeyUp: " + ev);
        InputDevice device = ev.getDevice();
        // Only care about game controllers.
        if (device != null && device.getId() == mCurrentDeviceId) {
            if (isGamepad(device)) {
                int index = getButtonMappingIndex(keyCode);
                if (index >= 0) {
                    mButtons[index] = 0;
                    mControllerView.invalidate();
                }
                return true;
            }
        }
        return super.onKeyUp(keyCode, ev);
    }

    /**
     * Utility method to determine if input device is a gamepad.
     * 
     * @param device
     * @return
     */
    private boolean isGamepad(InputDevice device) {
        if ((device.getSources() &
                InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD
                || (device.getSources() &
                InputDevice.SOURCE_CLASS_JOYSTICK) == InputDevice.SOURCE_JOYSTICK) {
            return true;
        }
        return false;
    }

    /**
     * Get the array index for the key code.
     * 
     * @param keyCode
     * @return
     */
    private int getButtonMappingIndex(int keyCode) {
        for (ButtonMapping buttonMapping : ButtonMapping.values()) {
            if (buttonMapping.getKeycode() == keyCode) {
                return buttonMapping.ordinal();
            }
        }
        return -1;
    }

    /*
     * (non-Javadoc)
     * @see
     * android.hardware.input.InputManager.InputDeviceListener#onInputDeviceAdded
     * (int)
     */
    @Override
    public void onInputDeviceAdded(int deviceId) {
        Log.d(TAG, "onInputDeviceAdded: " + deviceId);
        if (!mConnectedDevices.contains(deviceId)) {
            mConnectedDevices.add(new Integer(deviceId));
        }
        if (mCurrentDeviceId == -1) {
            mCurrentDeviceId = deviceId;
            InputDevice dev = InputDevice.getDevice(mCurrentDeviceId);
            if (dev != null) {
                mControllerView.setCurrentControllerNumber(dev.getControllerNumber());
                mControllerView.invalidate();
            }
        }
    }

    /*
     * (non-Javadoc)
     * @see
     * android.hardware.input.InputManager.InputDeviceListener#onInputDeviceRemoved
     * (int)
     */
    @Override
    public void onInputDeviceRemoved(int deviceId) {
        Log.d(TAG, "onInputDeviceRemoved: " + deviceId);
        mConnectedDevices.remove(new Integer(deviceId));
        if (mCurrentDeviceId == deviceId) {
            mCurrentDeviceId = -1;
        }
        if (mConnectedDevices.size() == 0) {
            mControllerView.setCurrentControllerNumber(-1);
            mControllerView.invalidate();
        } else {
            mCurrentDeviceId = mConnectedDevices.get(0);
            InputDevice dev = InputDevice.getDevice(mCurrentDeviceId);
            if (dev != null) {
                mControllerView.setCurrentControllerNumber(dev.getControllerNumber());
                mControllerView.invalidate();
            }
        }
    }

    /*
     * (non-Javadoc)
     * @see
     * android.hardware.input.InputManager.InputDeviceListener#onInputDeviceChanged
     * (int)
     */
    @Override
    public void onInputDeviceChanged(int deviceId) {
        Log.d(TAG, "onInputDeviceChanged: " + deviceId);
        mControllerView.invalidate();
    }

}
