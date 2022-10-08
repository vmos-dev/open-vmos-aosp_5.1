package com.google.android.DemoKit;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;

public class Slider extends View {

	interface SliderPositionListener {
		void onPositionChange(double value);
	}

	private Drawable mIndicator;
	private Drawable mBackground;
	private double mPosition;
	private SliderPositionListener mListener;
	private boolean mVertical;

	public Slider(Context context) {
		super(context);
		initSliderView(context, false);
	}

	public Slider(Context context, AttributeSet attrs) {
		super(context, attrs);
		initSliderView(context, false);
	}

	public void setSliderBackground(Drawable background) {
		mBackground = background;
		invalidate();
	}

	public void setPositionListener(SliderPositionListener listener) {
		mListener = listener;
	}

	public void setPosition(double position) {
		if (mPosition != position) {
			invalidate();
			mPosition = position;
			if (mListener != null) {
				mListener.onPositionChange(mPosition);
			}
		}
	}

	private OnTouchListener mClickListener = new OnTouchListener() {
		public boolean onTouch(View v, MotionEvent m) {
			Rect r = new Rect();
			getDrawingRect(r);

			double position;
			if (mVertical) {
				double y = m.getY();
				position = Math.max(0, (r.bottom - y) / r.height());
			} else {
				double x = m.getX();
				position = Math.max(0, (x - r.left) / r.width());
			}
			position = Math.min(1, position);
			setPosition(position);
			return true;
		}
	};

	protected void initSliderView(Context context, boolean vertical) {
		mPosition = 0;
		mVertical = vertical;
		Resources res = context.getResources();
		if (mVertical) {
			mBackground = res
					.getDrawable(R.drawable.scrubber_vertical_blue_holo_dark);
		} else {
			mBackground = res
					.getDrawable(R.drawable.scrubber_horizontal_holo_dark);
		}
		mIndicator = res.getDrawable(R.drawable.scrubber_control_holo_dark);
		this.setOnTouchListener(mClickListener);
	}

	protected void onDraw(Canvas canvas) {
		Rect r = new Rect();
		getDrawingRect(r);
		if (mVertical) {
			int lineX = r.centerX();
			int bgW = mBackground.getIntrinsicWidth() / 2;
			if (bgW == 0) {
				bgW = 5;
			}
			mBackground.setBounds(lineX - bgW, r.top + 10, lineX + bgW,
					r.bottom - 10);
			mBackground.draw(canvas);
			final int kMargin = 48;
			int indicatorY = (int) (r.bottom - (r.height() - kMargin)
					* mPosition)
					- kMargin / 2;
			Utilities.centerAround(lineX, indicatorY, mIndicator);
			mIndicator.draw(canvas);
		} else {
			int lineY = r.centerY();
			int bgH = mBackground.getIntrinsicHeight() / 2;
			if (bgH == 0) {
				bgH = 5;
			}
			mBackground.setBounds(r.left + 10, lineY - bgH, r.right - 10, lineY
					+ bgH);
			mBackground.draw(canvas);
			final int kMargin = 48;
			int indicatorX = (int) ((r.width() - kMargin) * mPosition) + r.left
					+ kMargin / 2;
			Utilities.centerAround(indicatorX, lineY, mIndicator);
			mIndicator.draw(canvas);
		}
	}

	protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
		super.onMeasure(widthMeasureSpec, heightMeasureSpec);
		if (mVertical) {
			setMeasuredDimension(mIndicator.getIntrinsicWidth(),
					getMeasuredHeight());
		} else {
			setMeasuredDimension(getMeasuredWidth(),
					mIndicator.getIntrinsicHeight());
		}
	}

}
