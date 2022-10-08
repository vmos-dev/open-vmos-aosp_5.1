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

package com.intel.thermal;

import android.app.Application;
import android.content.Intent;
import android.os.SystemProperties;
import android.util.Log;

public class ituxdApp extends Application {

    static final String TAG = "ituxdApp";

    public ituxdApp() {

    }

    @Override
    public void onCreate() {
        super.onCreate();

        if ("1".equals(SystemProperties.get("persist.service.thermal", "0"))) {
            Log.i(TAG, "Thermal Service enabled");
            startService(new Intent(this, ThermalService.class));
        } else {
            Log.i(TAG, "Thermal Service disabled");
        }
    }
}
