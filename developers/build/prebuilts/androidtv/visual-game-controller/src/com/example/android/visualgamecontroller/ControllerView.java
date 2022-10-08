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

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.BlurMaskFilter;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Point;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Display;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.WindowManager;

import com.example.android.visualgamecontroller.FullscreenActivity.AxesMapping;
import com.example.android.visualgamecontroller.FullscreenActivity.ButtonMapping;

/**
 * Custom view to display the game controller state visually.
 */
public class ControllerView extends SurfaceView implements
        SurfaceHolder.Callback {
    private static final String TAG = "ControllerView";
    private static final float IMAGE_RESOLUTION_HEIGHT = 1080.0F;
    private static final int MAX_CONTROLLERS = 4;

    private WindowManager mWindowManager;
    private Bitmap mControllerBitmap;
    private Bitmap mAxisBitmap;
    private Bitmap mBlueLedBitmap;
    private Bitmap mRightDirectionalBitmap;
    private Bitmap mTopDirectionalBitmap;
    private Bitmap mLeftDirectionalBitmap;
    private Bitmap mBottomDirectionalBitmap;
    private Bitmap mRightPaddleBitmap;
    private Bitmap mLeftPaddleBitmap;
    private Bitmap mGradientBitmap;
    private Paint mBackgroundPaint;
    private Paint mImagePaint;
    private Paint mCirclePaint;
    private Paint mLedPaint;
    private Paint mDirectionalPaint;
    private Paint mGradientPaint;
    private Point mSize = new Point();
    private float mDisplayRatio = 1.0f;
    private int[] mButtons;
    private float[] mAxes;
    private int mCurrentControllerNumber = -1;
    // Image asset locations
    private float[] mYButton = {
            823 / IMAGE_RESOLUTION_HEIGHT, 276 / IMAGE_RESOLUTION_HEIGHT,
            34 / IMAGE_RESOLUTION_HEIGHT
    };
    private float[] mXButton = {
            744 / IMAGE_RESOLUTION_HEIGHT, 355 / IMAGE_RESOLUTION_HEIGHT,
            34 / IMAGE_RESOLUTION_HEIGHT
    };
    private float[] mBButton = {
            903 / IMAGE_RESOLUTION_HEIGHT, 355 / IMAGE_RESOLUTION_HEIGHT,
            34 / IMAGE_RESOLUTION_HEIGHT
    };
    private float[] mAButton = {
            823 / IMAGE_RESOLUTION_HEIGHT, 434 / IMAGE_RESOLUTION_HEIGHT,
            34 / IMAGE_RESOLUTION_HEIGHT
    };
    private float[] mPowerButton = {
            533 / IMAGE_RESOLUTION_HEIGHT, 353 / IMAGE_RESOLUTION_HEIGHT,
            50 / IMAGE_RESOLUTION_HEIGHT
    };
    private float[] mHomeButton = {
            624 / IMAGE_RESOLUTION_HEIGHT, 353 / IMAGE_RESOLUTION_HEIGHT,
            30 / IMAGE_RESOLUTION_HEIGHT
    };
    private float[] mBackButton = {
            443 / IMAGE_RESOLUTION_HEIGHT, 353 / IMAGE_RESOLUTION_HEIGHT,
            30 / IMAGE_RESOLUTION_HEIGHT
    };
    private float[] mLedButtons = {
            463 / IMAGE_RESOLUTION_HEIGHT, 449 / IMAGE_RESOLUTION_HEIGHT,
            502 / IMAGE_RESOLUTION_HEIGHT, 449 / IMAGE_RESOLUTION_HEIGHT,
            539 / IMAGE_RESOLUTION_HEIGHT,
            449 / IMAGE_RESOLUTION_HEIGHT, 574 / IMAGE_RESOLUTION_HEIGHT,
            449 / IMAGE_RESOLUTION_HEIGHT
    };
    private float[] mRightDirectionalButton = {
            264 / IMAGE_RESOLUTION_HEIGHT, 336 / IMAGE_RESOLUTION_HEIGHT
    };
    private float[] mTopDirectionalButton = {
            218 / IMAGE_RESOLUTION_HEIGHT, 263 / IMAGE_RESOLUTION_HEIGHT
    };
    private float[] mLeftDirectionalButton = {
            144 / IMAGE_RESOLUTION_HEIGHT, 337 / IMAGE_RESOLUTION_HEIGHT
    };
    private float[] mBottomDirectionalButton = {
            217 / IMAGE_RESOLUTION_HEIGHT, 384 / IMAGE_RESOLUTION_HEIGHT
    };
    private float[] mLeftAxis = {
            305 / IMAGE_RESOLUTION_HEIGHT, 485 / IMAGE_RESOLUTION_HEIGHT,
            63 / IMAGE_RESOLUTION_HEIGHT, 50 / IMAGE_RESOLUTION_HEIGHT
    };
    private float[] mRightAxis = {
            637 / IMAGE_RESOLUTION_HEIGHT, 485 / IMAGE_RESOLUTION_HEIGHT,
            63 / IMAGE_RESOLUTION_HEIGHT, 50 / IMAGE_RESOLUTION_HEIGHT
    };
    private float[] mRightPaddle = {
            705 / IMAGE_RESOLUTION_HEIGHT, 166 / IMAGE_RESOLUTION_HEIGHT
    };
    private float[] mRightPaddlePressed = {
            705 / IMAGE_RESOLUTION_HEIGHT, 180 / IMAGE_RESOLUTION_HEIGHT
    };
    private float[] mLeftPaddle = {
            135 / IMAGE_RESOLUTION_HEIGHT, 166 / IMAGE_RESOLUTION_HEIGHT
    };
    private float[] mLeftPaddlePressed = {
            135 / IMAGE_RESOLUTION_HEIGHT, 180 / IMAGE_RESOLUTION_HEIGHT
    };
    private float[] mLeftAxisButton = {
            368 / IMAGE_RESOLUTION_HEIGHT, 548 / IMAGE_RESOLUTION_HEIGHT,
            64 / IMAGE_RESOLUTION_HEIGHT
    };
    private float[] mRightAxisButton = {
            700 / IMAGE_RESOLUTION_HEIGHT, 548 / IMAGE_RESOLUTION_HEIGHT,
            64 / IMAGE_RESOLUTION_HEIGHT
    };
    private float[] mRightGradient = {
            705 / IMAGE_RESOLUTION_HEIGHT, 125 / IMAGE_RESOLUTION_HEIGHT
    };
    private float[] mLeftGradient = {
            125 / IMAGE_RESOLUTION_HEIGHT, 125 / IMAGE_RESOLUTION_HEIGHT
    };
    private float mAxisLeftX, mAxisLeftY;
    private float mAxisRightX, mAxisRightY;

    /**
     * Class constructor taking only a context. Use this constructor to create
     * {@link ControllerView} objects from your own code.
     * 
     * @param context
     */
    public ControllerView(Context context) {
        super(context);
        init();
    }

    /**
     * Class constructor taking a context and an attribute set. This constructor
     * is used by the layout engine to construct a {@link ControllerView} from a
     * set of XML attributes.
     * 
     * @param context
     * @param attrs An attribute set which can contain attributes from
     *            {@link com.example.android.customviews.R.styleable.ControllerView}
     *            as well as attributes inherited from {@link android.view.View}
     */
    public ControllerView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    /**
     * Initialize the custom control.
     */
    private void init() {
        mWindowManager = (WindowManager) getContext().getSystemService(Context.WINDOW_SERVICE);
        mBackgroundPaint = new Paint();
        mBackgroundPaint.setStyle(Paint.Style.FILL);
        mBackgroundPaint.setDither(true);
        mBackgroundPaint.setAntiAlias(true);

        mImagePaint = new Paint();

        mCirclePaint = new Paint();
        mCirclePaint.setStyle(Paint.Style.FILL);
        mCirclePaint.setDither(true);
        mCirclePaint.setAntiAlias(true);

        mLedPaint = new Paint();
        mLedPaint.setStyle(Paint.Style.FILL);
        mLedPaint.setDither(true);
        mLedPaint.setAntiAlias(true);
        BlurMaskFilter blurMaskFilter = new BlurMaskFilter(20.0f, BlurMaskFilter.Blur.OUTER);
        mLedPaint.setMaskFilter(blurMaskFilter);

        mDirectionalPaint = new Paint();
        mDirectionalPaint.setDither(true);
        mDirectionalPaint.setAntiAlias(true);
        mDirectionalPaint.setAlpha(204);

        mGradientPaint = new Paint();
        mGradientPaint.setDither(true);
        mGradientPaint.setAntiAlias(true);
        mGradientPaint.setAlpha(204);
    }

    private void loadBitmaps(int displayWidth, int displayHeight) {
        // Load the image resources
        mControllerBitmap = BitmapFactory.decodeResource(getResources(),
                R.drawable.game_controller_paddles);
        int controllerBitmapWidth = mControllerBitmap.getWidth();
        int controllerBitmapHeight = mControllerBitmap.getHeight();
        mAxisBitmap = BitmapFactory.decodeResource(getResources(), R.drawable.axis);
        mBlueLedBitmap = BitmapFactory.decodeResource(getResources(), R.drawable.led_blue);
        mRightDirectionalBitmap = BitmapFactory.decodeResource(getResources(),
                R.drawable.directional_right);
        mTopDirectionalBitmap = BitmapFactory.decodeResource(getResources(),
                R.drawable.directional_top);
        mLeftDirectionalBitmap = BitmapFactory.decodeResource(getResources(),
                R.drawable.directional_left);
        mBottomDirectionalBitmap = BitmapFactory.decodeResource(getResources(),
                R.drawable.directional_bottom);
        mRightPaddleBitmap = BitmapFactory.decodeResource(getResources(),
                R.drawable.right_paddle);
        mLeftPaddleBitmap = BitmapFactory.decodeResource(getResources(),
                R.drawable.left_paddle);
        mGradientBitmap = BitmapFactory.decodeResource(getResources(),
                R.drawable.gradient);

        mControllerBitmap = Bitmap.createScaledBitmap(mControllerBitmap, displayHeight,
                displayHeight, true);

        mDisplayRatio = displayHeight * 1.0f / controllerBitmapHeight;
        // Scale the image bitmaps
        mAxisBitmap = Bitmap.createScaledBitmap(mAxisBitmap,
                (int) (mAxisBitmap.getWidth() * mDisplayRatio),
                (int) (mAxisBitmap.getHeight() * mDisplayRatio),
                true);
        mBlueLedBitmap = Bitmap.createScaledBitmap(mBlueLedBitmap,
                (int) (mBlueLedBitmap.getWidth() * mDisplayRatio),
                (int) (mBlueLedBitmap.getHeight() * mDisplayRatio),
                true);
        mRightDirectionalBitmap = Bitmap.createScaledBitmap(mRightDirectionalBitmap,
                (int) (mRightDirectionalBitmap.getWidth() * mDisplayRatio),
                (int) (mRightDirectionalBitmap.getHeight() * mDisplayRatio),
                true);
        mTopDirectionalBitmap = Bitmap.createScaledBitmap(mTopDirectionalBitmap,
                (int) (mTopDirectionalBitmap.getWidth() * mDisplayRatio),
                (int) (mTopDirectionalBitmap.getHeight() * mDisplayRatio),
                true);
        mLeftDirectionalBitmap = Bitmap.createScaledBitmap(mLeftDirectionalBitmap,
                (int) (mLeftDirectionalBitmap.getWidth() * mDisplayRatio),
                (int) (mLeftDirectionalBitmap.getHeight() * mDisplayRatio),
                true);
        mBottomDirectionalBitmap = Bitmap.createScaledBitmap(mBottomDirectionalBitmap,
                (int) (mBottomDirectionalBitmap.getWidth() * mDisplayRatio),
                (int) (mBottomDirectionalBitmap.getHeight() * mDisplayRatio),
                true);
        mRightPaddleBitmap = Bitmap.createScaledBitmap(mRightPaddleBitmap,
                (int) (mRightPaddleBitmap.getWidth() * mDisplayRatio),
                (int) (mRightPaddleBitmap.getHeight() * mDisplayRatio),
                true);
        mLeftPaddleBitmap = Bitmap.createScaledBitmap(mLeftPaddleBitmap,
                (int) (mLeftPaddleBitmap.getWidth() * mDisplayRatio),
                (int) (mLeftPaddleBitmap.getHeight() * mDisplayRatio),
                true);
        mGradientBitmap = Bitmap.createScaledBitmap(mGradientBitmap,
                (int) (mGradientBitmap.getWidth() * mDisplayRatio),
                (int) (mGradientBitmap.getHeight() * mDisplayRatio),
                true);
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width,
            int height) {
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
    }

    /*
     * (non-Javadoc)
     * @see android.view.SurfaceView#onMeasure(int, int)
     */
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        int width = 0;
        int height = 0;

        Display display = mWindowManager.getDefaultDisplay();
        display.getSize(mSize);
        int displayWidth = mSize.x;
        int displayHeight = mSize.y;
        displayWidth = getWidth();
        displayHeight = getHeight();

        int widthSpecMode = MeasureSpec.getMode(widthMeasureSpec);
        int widthSpecSize = MeasureSpec.getSize(widthMeasureSpec);
        int heightSpecMode = MeasureSpec.getMode(heightMeasureSpec);
        int heightSpecSize = MeasureSpec.getSize(heightMeasureSpec);
        Log.d(TAG, "widthSpecSize=" + widthSpecSize + ", heightSpecSize=" + heightSpecSize);

        if (widthSpecMode == MeasureSpec.EXACTLY) {
            width = widthSpecSize;
        } else if (widthSpecMode == MeasureSpec.AT_MOST) {
            width = Math.min(displayWidth, widthSpecSize);
        } else {
            width = displayWidth;
        }

        if (heightSpecMode == MeasureSpec.EXACTLY) {
            height = heightSpecSize;
        } else if (heightSpecMode == MeasureSpec.AT_MOST) {
            height = Math.min(displayHeight, heightSpecSize);
        } else {
            height = displayHeight;
        }

        setMeasuredDimension(width, height);

        if (width > 0 && height > 0) {
            loadBitmaps(width, height);
        }
    }

    /*
     * (non-Javadoc)
     * @see android.view.View#onDraw(android.graphics.Canvas)
     */
    @Override
    protected void onDraw(Canvas canvas) {
        int offset = getWidth() / 2 - getHeight() / 2;

        // Draw the background
        canvas.drawColor(Color.BLACK);
        canvas.drawRect(0, 0, getWidth(), getHeight(), mBackgroundPaint);

        // Draw the brake/gas indicators
        if (mAxes[AxesMapping.AXIS_BRAKE.ordinal()] > 0.0f) {
            mGradientPaint.setAlpha((int) (mAxes[AxesMapping.AXIS_BRAKE.ordinal()] * 100) + 155);
            canvas.drawBitmap(mGradientBitmap, offset + mLeftGradient[0]
                    * getHeight(), mLeftGradient[1] * getHeight(), mGradientPaint);
        }
        if (mAxes[AxesMapping.AXIS_GAS.ordinal()] > 0.0f) {
            mGradientPaint.setAlpha((int) (mAxes[AxesMapping.AXIS_GAS.ordinal()] * 100) + 155);
            canvas.drawBitmap(mGradientBitmap, offset + mRightGradient[0]
                    * getHeight(), mRightGradient[1] * getHeight(), mGradientPaint);
        }

        // Draw the paddles
        canvas.drawColor(Color.TRANSPARENT);
        if (mButtons[ButtonMapping.BUTTON_R1.ordinal()] == 0) {
            canvas.drawBitmap(mRightPaddleBitmap, offset + mRightPaddle[0]
                    * getHeight(), mRightPaddle[1] * getHeight(), mImagePaint);
        } else if (mButtons[ButtonMapping.BUTTON_R1.ordinal()] == 1) {
            canvas.drawBitmap(mRightPaddleBitmap, offset + mRightPaddlePressed[0]
                    * getHeight(), mRightPaddlePressed[1] * getHeight(), mImagePaint);
        }
        if (mButtons[ButtonMapping.BUTTON_L1.ordinal()] == 0) {
            canvas.drawBitmap(mLeftPaddleBitmap, offset + mLeftPaddle[0]
                    * getHeight(), mLeftPaddle[1] * getHeight(), mImagePaint);
        }
        else if (mButtons[ButtonMapping.BUTTON_L1.ordinal()] == 1) {
            canvas.drawBitmap(mLeftPaddleBitmap, offset + mLeftPaddlePressed[0]
                    * getHeight(), mLeftPaddlePressed[1] * getHeight(), mImagePaint);
        }

        // Draw the controller body
        canvas.drawBitmap(mControllerBitmap, offset, 0, mImagePaint);

        // Draw the axes
        mAxisLeftX = offset + mLeftAxis[0] * getHeight();
        mAxisLeftY = mLeftAxis[1] * getHeight();
        mAxisRightX = offset + mRightAxis[0] * getHeight();
        mAxisRightY = mRightAxis[1] * getHeight();
        if (mAxes[AxesMapping.AXIS_X.ordinal()] != 0.0f) {
            mAxisLeftX = mAxisLeftX + mLeftAxis[3] * getHeight()
                    * mAxes[AxesMapping.AXIS_X.ordinal()];
        }
        if (mAxes[AxesMapping.AXIS_Y.ordinal()] != 0.0f) {
            mAxisLeftY = mAxisLeftY + mLeftAxis[3]
                    * getHeight() * mAxes[AxesMapping.AXIS_Y.ordinal()];
        }
        canvas.drawBitmap(mAxisBitmap, mAxisLeftX, mAxisLeftY, mImagePaint);
        if (mAxes[AxesMapping.AXIS_Z.ordinal()] != 0.0f) {
            mAxisRightX = mAxisRightX + mRightAxis[3] * getHeight()
                    * mAxes[AxesMapping.AXIS_Z.ordinal()];
        }
        if (mAxes[AxesMapping.AXIS_RZ.ordinal()] != 0.0f) {
            mAxisRightY = mAxisRightY + mRightAxis[3]
                    * getHeight() * mAxes[AxesMapping.AXIS_RZ.ordinal()];
        }
        canvas.drawBitmap(mAxisBitmap, mAxisRightX, mAxisRightY, mImagePaint);

        // Draw the LED light
        if (mCurrentControllerNumber > 0 && mCurrentControllerNumber <= MAX_CONTROLLERS) {
            canvas.drawBitmap(mBlueLedBitmap, offset
                    + mLedButtons[2 * mCurrentControllerNumber - 2] * getHeight(),
                    mLedButtons[2 * mCurrentControllerNumber - 1] * getHeight(), mLedPaint);
        }

        // Draw the directional buttons
        if (mAxes[AxesMapping.AXIS_HAT_X.ordinal()] == 1.0f) {
            canvas.drawBitmap(mRightDirectionalBitmap, offset + mRightDirectionalButton[0]
                    * getHeight(),
                    mRightDirectionalButton[1] * getHeight(), mDirectionalPaint);
        }
        if (mAxes[AxesMapping.AXIS_HAT_Y.ordinal()] == -1.0f) {
            canvas.drawBitmap(mTopDirectionalBitmap, offset + mTopDirectionalButton[0]
                    * getHeight(),
                    mTopDirectionalButton[1] * getHeight(), mDirectionalPaint);
        }
        if (mAxes[AxesMapping.AXIS_HAT_X.ordinal()] == -1.0f) {
            canvas.drawBitmap(mLeftDirectionalBitmap, offset + mLeftDirectionalButton[0]
                    * getHeight(),
                    mLeftDirectionalButton[1] * getHeight(), mDirectionalPaint);
        }
        if (mAxes[AxesMapping.AXIS_HAT_Y.ordinal()] == 1.0f) {
            canvas.drawBitmap(mBottomDirectionalBitmap, offset + mBottomDirectionalButton[0]
                    * getHeight(), mBottomDirectionalButton[1] * getHeight(), mDirectionalPaint);
        }

        // Draw the A/B/X/Y buttons
        canvas.drawColor(Color.TRANSPARENT);
        mCirclePaint.setColor(getResources().getColor(R.color.transparent_black));
        if (mButtons[ButtonMapping.BUTTON_Y.ordinal()] == 1) {
            canvas.drawCircle(offset + mYButton[0] * getHeight(), mYButton[1] * getHeight(),
                    mYButton[2] * getHeight(), mCirclePaint);
        }
        if (mButtons[ButtonMapping.BUTTON_X.ordinal()] == 1) {
            canvas.drawCircle(offset + mXButton[0] * getHeight(), mXButton[1] * getHeight(),
                    mXButton[2] * getHeight(), mCirclePaint);
        }
        if (mButtons[ButtonMapping.BUTTON_B.ordinal()] == 1) {
            canvas.drawCircle(offset + mBButton[0] * getHeight(), mBButton[1] * getHeight(),
                    mBButton[2] * getHeight(), mCirclePaint);
        }
        if (mButtons[ButtonMapping.BUTTON_A.ordinal()] == 1) {
            canvas.drawCircle(offset + mAButton[0] * getHeight(), mAButton[1] * getHeight(),
                    mAButton[2] * getHeight(), mCirclePaint);
        }

        // Draw the center buttons
        if (mButtons[ButtonMapping.POWER.ordinal()] == 1) {
            canvas.drawCircle(offset + mPowerButton[0] * getHeight(),
                    mPowerButton[1] * getHeight(),
                    mPowerButton[2] * getHeight(), mCirclePaint);
        }
        if (mButtons[ButtonMapping.BUTTON_START.ordinal()] == 1) {
            canvas.drawCircle(offset + mHomeButton[0] * getHeight(), mHomeButton[1] * getHeight(),
                    mHomeButton[2] * getHeight(), mCirclePaint);
        }
        if (mButtons[ButtonMapping.BACK.ordinal()] == 1) {
            canvas.drawCircle(offset + mBackButton[0] * getHeight(), mBackButton[1] * getHeight(),
                    mBackButton[2] * getHeight(), mCirclePaint);
        }

        // Draw the axes
        if (mButtons[ButtonMapping.BUTTON_THUMBL.ordinal()] == 1) {
            canvas.drawCircle(mLeftAxisButton[2] * getHeight() + mAxisLeftX, mLeftAxisButton[2]
                    * getHeight() + mAxisLeftY,
                    mLeftAxisButton[2] * getHeight(), mCirclePaint);
        }
        if (mButtons[ButtonMapping.BUTTON_THUMBR.ordinal()] == 1) {
            canvas.drawCircle(mRightAxisButton[2] * getHeight() + mAxisRightX, mRightAxisButton[2]
                    * getHeight() + mAxisRightY,
                    mRightAxisButton[2] * getHeight(), mCirclePaint);
        }
    }

    /**
     * Set the button and axes mapping data structures.
     * 
     * @param buttons
     * @param axes
     */
    public void setButtonsAxes(int[] buttons, float[] axes) {
        mButtons = buttons;
        mAxes = axes;
    }

    public void setCurrentControllerNumber(int number) {
        Log.d(TAG, "setCurrentControllerNumber: " + number);
        mCurrentControllerNumber = number;
    }
}
