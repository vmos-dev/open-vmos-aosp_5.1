package com.google.android.DemoKit;

import android.graphics.Color;
import android.text.SpannableStringBuilder;
import android.text.style.RelativeSizeSpan;
import android.text.style.SubscriptSpan;
import android.view.ViewGroup;
import android.widget.TextView;

public class ColorWheelLEDController implements ColorWheel.OnColorChangedListener {
	private DemoKitActivity mActivity;

	public ColorWheelLEDController(DemoKitActivity activity) {
		mActivity = activity;
	}

  public void colorChanged(int color) {
    int red = Color.red(color) / 8;
    int green = Color.green(color) / 8;
    int blue = Color.blue(color) / 8;

    mActivity.sendCommand(DemoKitActivity.LED_SERVO_COMMAND,(byte)0,(byte)red);
    mActivity.sendCommand(DemoKitActivity.LED_SERVO_COMMAND,(byte)1,(byte)green);
    mActivity.sendCommand(DemoKitActivity.LED_SERVO_COMMAND,(byte)2,(byte)blue);

    mActivity.sendCommand(DemoKitActivity.LED_SERVO_COMMAND,(byte)3,(byte)red);
    mActivity.sendCommand(DemoKitActivity.LED_SERVO_COMMAND,(byte)4,(byte)green);
    mActivity.sendCommand(DemoKitActivity.LED_SERVO_COMMAND,(byte)5,(byte)blue);

    mActivity.sendCommand(DemoKitActivity.LED_SERVO_COMMAND,(byte)6,(byte)red);
    mActivity.sendCommand(DemoKitActivity.LED_SERVO_COMMAND,(byte)7,(byte)green);
    mActivity.sendCommand(DemoKitActivity.LED_SERVO_COMMAND,(byte)8,(byte)blue);
  }
}
