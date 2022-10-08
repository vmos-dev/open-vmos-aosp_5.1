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

import android.os.UEventObserver;
import android.util.Log;

import java.lang.Math;
import java.util.ArrayList;
import java.util.Arrays;

/**
 * The VirtualThermalZone class extends the ThermalZone class, with a default
 * implementation of the isZoneStateChanged() method. This computes the
 * zone state by computing the equation, which can be linear / higher order implementation
 * @hide
 */
public class VirtualThermalZone extends ThermalZone {

    private static final String TAG = "VirtualThermalZone";
    private String mEmulTempPath;
    private ThermalZoneMonitor mTzm = null;

    public void setEmulTempPath(String path) {
        mEmulTempPath = path;
    }

    public String getEmulTempPath() {
        return mEmulTempPath;
    }

    public VirtualThermalZone() {
        super();
    }

    // overridden to start UEvent observer only for Virtual zone
    public void startEmulTempObserver() {
        if (!getEmulTempFlag()) {
            return;
        }
        int indx = ThermalUtils.getThermalZoneIndex(getZoneName());
        if (indx == -1) {
            Log.i(TAG, "Could not obtain emul_temp sysfs node for " + getZoneName());
            return;
        }
        String uEventDevPath = ThermalManager.sUEventDevPath + indx;
        setEmulTempPath(ThermalManager.sSysfsSensorBasePath + indx + "/emul_temp");
        mEmulTempObserver.startObserving(uEventDevPath);
    }

    public void unregisterReceiver() {
        super.unregisterReceiver();
        if (getEmulTempFlag()) {
            mEmulTempObserver.stopObserving();
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

    // override fucntion
    public void calibrateThresholds() {
        ThermalSensor ts = getThermalSensorList().get(0);
        ThermalSensorAttrib sa = mThermalSensorsAttribMap.get(ts.getSensorName());
        if (sa == null) {
            return;
        }
        Integer weights[] = sa.getWeights();
        int m = weights[0];
        int c = getOffset();

        if (m == 0) return;
        for (int i = 0; i < mZoneTempThresholdsRaw.length; i++) {
            // We do not want to convert '0'. Let it represent 0 C.
            if (mZoneTempThresholdsRaw[i] == 0) continue;
            // Get raw systherm temperature: y=mx+c <--> x=(y-c)/m
            mZoneTempThresholds[i] = ((mZoneTempThresholdsRaw[i] - c) * 1000) / m;
        }
        Log.i(TAG, "calibrateThresholds[]: " + Arrays.toString(mZoneTempThresholds));
    }

    private int calculateZoneTemp() {
        int curZoneTemp = 0;
        int weightedTemp;
        ArrayList<ThermalSensor> list = getThermalSensorList();

        // Check if the SensorList is sane and usable
        if (list == null || list.get(0) == null) {
            return ThermalManager.INVALID_TEMP;
        }

        if (isUEventSupported()) {
            // for uevent based monitoring only first sensor used
            ThermalSensor ts = list.get(0);
            weightedTemp = getWeightedTemp(ts, ts.readSensorTemp());
            return weightedTemp == ThermalManager.INVALID_TEMP
                    ? ThermalManager.INVALID_TEMP : weightedTemp + getOffset();
        }

        // Polling mode
        for (ThermalSensor ts : list) {
            if (ts != null && ts.getSensorActiveStatus()) {
                weightedTemp = getWeightedTemp(ts, ts.readSensorTemp());
                if (weightedTemp == ThermalManager.INVALID_TEMP) {
                    return ThermalManager.INVALID_TEMP;
                }
                curZoneTemp += weightedTemp;
            }
        }
        return curZoneTemp + getOffset();
    }

    private UEventObserver mEmulTempObserver = new UEventObserver() {
        @Override
        public void onUEvent(UEventObserver.UEvent event) {
            String type = event.get("EVENT");
            if (type == null || Integer.parseInt(type) != ThermalManager.THERMAL_EMUL_TEMP_EVENT) {
                Log.i(TAG, "EventType does not match");
                return;
            }

            if (!getZoneName().equals(event.get("NAME"))) {
                Log.i(TAG, "ZoneName does not match");
                return;
            }

            int temp = calculateZoneTemp();
            if (temp == ThermalManager.INVALID_TEMP) {
                Log.i(TAG, "Obtained INVALID_TEMP[0xDEADBEEF]");
                return;
            }

            String path = getEmulTempPath();
            if (path == null) {
                Log.i(TAG, "EmulTempPath is null");
                return;
            }

            int ret = ThermalUtils.writeSysfs(path, temp);
            if (ret == -1) {
                Log.i(TAG, "Writing into emul_temp sysfs failed");
            }
        }
    };

    // override function
    public boolean updateZoneTemp() {
        int curZoneTemp = ThermalManager.INVALID_TEMP;
        int rawSensorTemp, sensorTemp;
        int weightedTemp;
        boolean flag = false;

        if (isUEventSupported()) {
            // In UEvent mode, the obtained temperature is the zone temperature
            return true;
        } else {
            for (int i = 0; i < mThermalSensors.size(); i++) {
                if (mThermalSensors.get(i) != null
                        && mThermalSensors.get(i).getSensorActiveStatus()) {
                    if (flag == false) {
                        // one time initialization of zone temp
                        curZoneTemp = 0;
                        flag = true;
                    }
                    weightedTemp = getWeightedTemp(mThermalSensors.get(i));
                    if (weightedTemp != ThermalManager.INVALID_TEMP) {
                        curZoneTemp += weightedTemp;
                    }
                }
            }
        }

        if (curZoneTemp != ThermalManager.INVALID_TEMP) {
            curZoneTemp += getOffset();
            setZoneTemp(curZoneTemp);
            if (getMovingAverageFlag() && !isUEventSupported()) {
                // only for polling mode apply moving average on predicted zone temp
                setZoneTemp(movingAverageTemp());
            }
            return true;
        }

        return false;
    }

    private int getWeightedTemp(ThermalSensor ts) {
        return getWeightedTemp(ts, ts.getCurrTemp());
    }

    private int getWeightedTemp(ThermalSensor ts, int rawSensorTemp) {
        int curZoneTemp = 0;
        Integer weights[], order[];
        ThermalSensorAttrib sa = mThermalSensorsAttribMap.get(ts.getSensorName());

        // No point in calculating 'WeightedTemp' on an 'anyway invalid' temperature
        if (rawSensorTemp == ThermalManager.INVALID_TEMP || sa == null) {
            return ThermalManager.INVALID_TEMP;
        }

        weights = sa.getWeights();
        order = sa.getOrder();
        if (weights == null && order == null) return rawSensorTemp;
        if (weights != null) {
            if (order == null) {
                // only first weight will be considered
                return (weights[0] * rawSensorTemp) / 1000;
            } else if (order != null && weights.length == order.length) {
                // if order array is provided in xml,
                // it should be of same size as weights array
                int sensorTemp = 0;
                for (int i = 0; i < weights.length; i++) {
                    // Divide by 1000 to convert to mC
                    sensorTemp += (weights[i] * (int) Math.pow(rawSensorTemp, order[i])) / 1000;
                }
                return sensorTemp;
            }
        }
        // for every other mismatch return the raw sensor temp
        return rawSensorTemp;
    }
}
