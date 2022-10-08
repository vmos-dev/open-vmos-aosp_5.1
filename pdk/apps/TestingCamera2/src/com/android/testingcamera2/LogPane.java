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

import java.util.Date;
import java.text.SimpleDateFormat;

import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.Color;
import android.os.Handler;
import android.os.Looper;
import android.text.Spannable;
import android.text.SpannableString;
import android.text.method.ScrollingMovementMethod;
import android.text.style.ForegroundColorSpan;
import android.util.AttributeSet;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.MotionEvent;
import android.widget.LinearLayout;
import android.widget.TextView;

public class LogPane extends LinearLayout implements TLog.Logger {

    private final TextView mLogTextView;
    private final Handler mHandler;

    @SuppressLint("SimpleDateFormat")
    private SimpleDateFormat mDateFormatter = new SimpleDateFormat("HH:mm:ss.SSS : ");

    public LogPane(Context context, AttributeSet attrs) {
        super(context, attrs);

        mHandler = new Handler(Looper.getMainLooper());

        this.setOrientation(VERTICAL);

        TextView titleText = new TextView(context);
        titleText.setText(R.string.log_pane_label);
        titleText.setTextSize(TypedValue.COMPLEX_UNIT_PX,
                context.getResources().getDimension(R.dimen.pane_title_text));
        LinearLayout.LayoutParams titleTextLayoutParams =
                new LinearLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);

        this.addView(titleText, titleTextLayoutParams);

        mLogTextView = new TextView(context);
        mLogTextView.setFreezesText(true);
        LinearLayout.LayoutParams logTextLayoutParams =
                new LinearLayout.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT);
        logTextLayoutParams.leftMargin = 40;
        mLogTextView.setLayoutParams(logTextLayoutParams);
        mLogTextView.setLines(10);
        mLogTextView.setMovementMethod(new ScrollingMovementMethod());
        mLogTextView.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View view, MotionEvent event) {
                // Allow scrolling of text while inside a scroll view
                view.getParent().requestDisallowInterceptTouchEvent(true);
                return false;
            }
        });
        mLogTextView.setGravity(Gravity.BOTTOM);

        this.addView(mLogTextView);
    }

    @Override
    public void addToLog(final String text, boolean error) {
        StringBuffer logEntry = new StringBuffer(32);
        logEntry.append("\n").append(mDateFormatter.format(new Date()));
        logEntry.append(text);
        final Spannable logLine = new SpannableString(logEntry.toString());

        int lineColor = error ? Color.RED : Color.WHITE;
        logLine.setSpan(new ForegroundColorSpan(lineColor), 0, logLine.length(),
                Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);

        mHandler.post(new Runnable() {
            public void run() {
                mLogTextView.append(logLine);
            }
        });
    }
}
