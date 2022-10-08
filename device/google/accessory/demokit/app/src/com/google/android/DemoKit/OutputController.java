package com.google.android.DemoKit;

import android.view.ViewGroup;

public class OutputController extends AccessoryController {

	private boolean mVertical;

	OutputController(DemoKitActivity hostActivity, boolean vertical) {
		super(hostActivity);
		mVertical = vertical;
	}

	protected void onAccesssoryAttached() {
		setupServoController(1, R.id.servo1);
		setupServoController(2, R.id.servo2);
		setupServoController(3, R.id.servo3);

		setupLedController(1, R.id.leds1);
		setupLedController(2, R.id.leds2);
		setupLedController(3, R.id.leds3);

		setupRelayController(1, R.id.relay1);
		setupRelayController(2, R.id.relay2);
	}

	private void setupServoController(int servoIndex, int viewId) {
		ServoController sc = new ServoController(mHostActivity, servoIndex);
		sc.attachToView((ViewGroup) findViewById(viewId));
	}

	private void setupLedController(int index, int viewId) {
		ColorLEDController ledC = new ColorLEDController(mHostActivity, index,
				getResources(), mVertical);
		ledC.attachToView((ViewGroup) findViewById(viewId));
	}

	private void setupRelayController(int index, int viewId) {
		RelayController r = new RelayController(mHostActivity, index,
				getResources());
		r.attachToView((ViewGroup) findViewById(viewId));
	}

}
