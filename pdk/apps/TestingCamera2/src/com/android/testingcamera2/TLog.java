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

import java.util.Locale;

import android.util.Log;

public class TLog {

    private static Logger mLogger;
    private static final String TAG = "TestingCamera2";

    synchronized static public void setLogger(Logger logger) {
        mLogger = logger;
    }

    /**
     * Log an informative message to the current log destination and to the system log.
     * Supports formatting in the style of String.format()
     *
     * @param text The text to print out, with optional formatting specifiers
     * @param args Arguments to fill in to the string
     */
    synchronized static public void i(String text, Object... args) {
        if (args.length > 0) {
            text = String.format(Locale.US, text, args);
        }
        android.util.Log.i(TAG, text);
        if (mLogger != null) {
            mLogger.addToLog(text, false);
        }
    }

    /**
     * Log an error message to the current log destination and to the system log.
     * Supports formatting in the style of String.format()
     *
     * @param text The text to print out, with optional formatting specifiers
     * @param args Arguments to fill in to the string
     */
    synchronized static public void e(String text, Object... args) {
        if (args.length > 0) {
            text = String.format(Locale.US, text, args);
        }
        android.util.Log.e(TAG, text);
        if (mLogger != null) {
            mLogger.addToLog(text, true);
        }
    }

    /**
     * Log an error message to the current log destination and to the system log,
     * including the exception stack trace.
     *
     * <p>The trace will be added after the main error message.  Supports formatting in
     * the style of String.format()</p>
     *
     * @param text The text to print out, with optional formatting specifiers
     * @param e The throwable for the error
     * @param args Arguments to fill in to the string
     */
    synchronized static public void e(String text, Throwable e, Object... args) {
        text = String.format("%s\n%s", text, Log.getStackTraceString(e));
        if (args.length > 0) {
            text = String.format(Locale.US, text, args);
        }
        android.util.Log.e(TAG, text);
        if (mLogger != null) {
            mLogger.addToLog(text, true);
        }
    }

    public interface Logger {
        public void addToLog(String text, boolean error);
    }
}