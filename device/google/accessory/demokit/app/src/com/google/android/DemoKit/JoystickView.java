package com.google.android.DemoKit;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.view.View;

public class JoystickView extends View {

	private Drawable mJoystickBackground;
	private Drawable mJoystickPressed;
	private Drawable mJoystickNormal;

	private int fX;
	private int fY;
	Boolean fPressed;
	private Paint mLabelPaint;
	private String mLabelText;

	public JoystickView(Context context) {
		super(context);
		initJoystickView(context);
	}

	public JoystickView(Context context, AttributeSet attrs) {
		super(context, attrs);
		initJoystickView(context);
	}

	public void setPosition(int x, int y) {
		fX = x;
		fY = y;
		mLabelText = String.format("%d,%d", fX, fY);
		invalidate();
	}

	public void setPressed(boolean pressed) {
		fPressed = pressed;
		invalidate();
	}

	private void initJoystickView(Context context) {
		fX = fY = 0;
		fPressed = false;
		Resources r = context.getResources();
		mJoystickBackground = r.getDrawable(R.drawable.joystick_background);
		int w = mJoystickBackground.getIntrinsicWidth();
		int h = mJoystickBackground.getIntrinsicHeight();
		mJoystickBackground.setBounds(0, 0, w, h);
		mJoystickPressed = r.getDrawable(R.drawable.joystick_pressed_holo_dark);
		mJoystickNormal = r.getDrawable(R.drawable.joystick_normal_holo_dark);
		Utilities.centerAround(w / 2 - 4, h / 2 + 4, mJoystickNormal);
		Utilities.centerAround(w / 2 - 4, h / 2 + 4, mJoystickPressed);
		mLabelPaint = new Paint();
		mLabelPaint.setColor(Color.WHITE);
		mLabelPaint.setTextSize(24);
		mLabelPaint.setAntiAlias(true);
		mLabelPaint.setShadowLayer(1, 2, 2, Color.BLACK);
		setPosition(0, 0);
	}

	@Override
	protected void onDraw(Canvas canvas) {
		mJoystickBackground.draw(canvas);
		Drawable indicator = fPressed ? mJoystickPressed : mJoystickNormal;
		int w = mJoystickBackground.getIntrinsicWidth();
		int h = mJoystickBackground.getIntrinsicHeight();
		int x = w / 2 - 4 + fX;
		int y = h / 2 + 4 + fY;
		Utilities.centerAround(x, y, indicator);
		indicator.draw(canvas);
		canvas.drawText(mLabelText, x + 12, y + 8, mLabelPaint);
	}

	@Override
	protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
		setMeasuredDimension(mJoystickBackground.getIntrinsicWidth(),
				mJoystickBackground.getIntrinsicHeight());
	}

}
