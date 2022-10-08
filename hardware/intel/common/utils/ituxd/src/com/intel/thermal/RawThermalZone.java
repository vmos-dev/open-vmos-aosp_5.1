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

import java.util.ArrayList;
import java.util.Arrays;

/**
 * The RawThermalZone class extends the ThermalZone class, with a default
 * implementation of the isZoneStateChanged() method. This computes the
 * zone state by first computing max of all sensor temperature (in polling mode)
 * and comparing this temperature against zone thresholds. For uevent based
 * monitoring only the temperature of first sensor is used to compute zone state.
 *
 * @hide
 */
public class RawThermalZone extends ThermalZone {

    private static final String TAG = "RawThermalZone";
    private ThermalZoneMonitor mTzm = null;

    public RawThermalZone() {
        super();
        // for raw zone emul temp flag is always false
        mSupportsEmulTemp = false;
    }

    // irrespective of what flag is set in XML, emul temp flag is false for raw thermal zone
    // over ride function. so that even if flag is 1 in XML, 0 will be written
    public void setEmulTempFlag(int flag) {
        mSupportsEmulTemp = false;
        if (flag != 0) {
            Log.i(TAG, "zone:" + getZoneName()
                    + " is a raw zone, doesnt support emulated temperature");
        }
    }

    public void startMonitoring() {
        mTzm = new ThermalZoneMonitor(this);
    }

    public void stopMonitoring() {
        if (mTzm != null) {
            mTzm.stopMonitor();
        }
    }

    // override updateZoneTemp
    public boolean updateZoneTemp() {
        int curTemp = ThermalManager.INVALID_TEMP, maxCurTemp = ThermalManager.INVALID_TEMP;
        if (isUEventSupported()) {
            ArrayList<ThermalSensor> list = getThermalSensorList();
            if (list != null) {
                // for uevent based monitoring only first sensor used
                ThermalSensor s = list.get(0);
                if (s != null) {
                    maxCurTemp = s.getCurrTemp();
                }
            }
        } else {
            //zone temp is max of all sensor temp
            for (int i = 0; i < mThermalSensors.size(); i++) {
                if (mThermalSensors.get(i) != null &&
                        mThermalSensors.get(i).getSensorActiveStatus()) {
                    curTemp = mThermalSensors.get(i).getCurrTemp();
                    if (curTemp > maxCurTemp) {
                        maxCurTemp = curTemp;
                    }
                }
            }
        }

        if (maxCurTemp != ThermalManager.INVALID_TEMP) {
            setZoneTemp(maxCurTemp);
            // it is assumed only one sensor will be provided for moving average
            if (getMovingAverageFlag() && !isUEventSupported()) {
                // only for polling mode apply moving average on predicted zone temp
                setZoneTemp(movingAverageTemp());
            }
            return true;
        }

        return false;
    }
}
