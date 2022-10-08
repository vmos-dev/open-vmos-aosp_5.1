/*
 * Copyright 2014 Intel Corporation All Rights Reserved.
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

import android.util.Log;

/**
 * The ThermalZoneMonitor class runs a thread for each zone
 * with which it is instantiated.
 *
 * @hide
 */
public class ThermalZoneMonitor implements Runnable {
    private static final String TAG = "ThermalZoneMonitor";
    private Thread t;
    private ThermalZone zone;
    private String mThreadName;
    private boolean stop = false;

    public ThermalZoneMonitor(ThermalZone tz) {
        zone = tz;
        mThreadName = "ThermalZone" + zone.getZoneId();
        t = new Thread(this, mThreadName);
        t.start();
    }

    public void stopMonitor() {
        stop = true;
        t.interrupt();
    }

    public void run() {
        try {
            while (!stop && !t.isInterrupted()) {
                if (zone.isZoneStateChanged()) {
                    zone.sendThermalEvent();
                }
                // stop value can be changed before going to sleep
                if (!stop) {
                    Thread.sleep(zone.getPollDelay(zone.getZoneState()));
                }
            }
        } catch (InterruptedException iex) {
            Log.i(TAG, "Stopping thread " + mThreadName + " [InterruptedException]");
        }
    }
}
