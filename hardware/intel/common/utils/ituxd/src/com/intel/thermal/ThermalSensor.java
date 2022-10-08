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

import com.intel.thermal.ThermalManager;

import android.util.Log;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;

/**
 * The ThermalSensor class describes the attributes of a Thermal Sensor. This
 * class implements methods that retrieve temperature sensor information from
 * the kernel through the native interface.
 *
 * @hide
 */
public class ThermalSensor {

    private static final String TAG = "ThermalSensor";
    private String mSensorPath;     /* sys path to read temp from */
    private String mSensorName;     /* name of the sensor */
    private String mInputTempPath;  /* sys path to read the current temp */
    private String mHighTempPath;   /* sys path to set the intermediate upper threshold */
    private String mLowTempPath;    /* sys path to set the intermediate lower threshold */
    private String mUEventDevPath;  /* sys path for uevent listener */
    private int mErrorCorrectionTemp; /* Temperature difference in mC */
    private int mSensorID = -1;
    private int mSensorState;       /* Thermal state of the sensor */
    private int mCurrTemp;          /* Holds the latest temperature of the sensor */
    private int mSensorSysfsIndx; /* Index of this sensor in the sysfs */
    private boolean mIsSensorActive = false; /* Whether this sensor is active */

    public void printAttrs() {
        Log.i(TAG, "mSensorName: " + mSensorName);
        Log.i(TAG, "mSensorPath: " + mSensorPath);
        Log.i(TAG, "mInputTempPath: " + mInputTempPath);
        Log.i(TAG, "mHighTempPath: " + mHighTempPath);
        Log.i(TAG, "mLowTempPath: " + mLowTempPath);
        Log.i(TAG, "mUEventDevPath: " + mUEventDevPath);
        Log.i(TAG, "mErrorCorrection: " + mErrorCorrectionTemp);
    }

    private void setSensorSysfsPath() {
        int indx = ThermalUtils.getThermalZoneIndex(mSensorName);
        // The battery subsystem exposes sensors under different names.
        // The only commonality among them is that all of them contain
        // the string "battery".
        if (indx == -1 && mSensorName.contains("battery")) {
            indx = ThermalUtils.getThermalZoneIndexContains("battery");
        }

        if (indx != -1) {
            mSensorPath = ThermalManager.sSysfsSensorBasePath + indx + "/";
        }

        mSensorSysfsIndx = indx;
    }

    public boolean getSensorActiveStatus() {
        return mIsSensorActive;
    }

    public String getSensorSysfsPath() {
        return mSensorPath;
    }

    public ThermalSensor() {
        mSensorState = ThermalManager.THERMAL_STATE_OFF;
        mCurrTemp = ThermalManager.INVALID_TEMP;
        mSensorPath = "auto";
        mInputTempPath = "auto";
        mHighTempPath = "auto";
        mLowTempPath = "auto";
        mUEventDevPath = "auto";

        // Set default value of 'correction temperature' to 1000mC
        mErrorCorrectionTemp = 1000;
    }

    public int getSensorID() {
        return mSensorID;
    }

    public int getSensorSysfsIndx() {
        return mSensorSysfsIndx;
    }

    public String getSensorPath() {
        return mSensorPath;
    }

    /**
     * This function sets the sensor path to the given String value. If the
     * String is "auto" it loops through the standard sysfs path, to obtain the
     * 'mSensorPath'. The standard sysfs path is /sys/class/
     * thermal/thermal_zoneX and the look up is based on 'mSensorName'.
     * If sensor path is "none", sensor temp is not read via any sysfs
     */
    public void setSensorPath(String path) {
        if (path.equalsIgnoreCase("auto")) {
            setSensorSysfsPath();
        } else {
            mSensorPath = path;
        }
    }

    private boolean isSensorSysfsValid(String path) {
        return ThermalUtils.isFileExists(path);
    }

    public String getSensorName() {
        return mSensorName;
    }

    public void setSensorName(String name) {
        mSensorName = name;
    }

    public void setUEventDevPath(String devPath) {
        mUEventDevPath = devPath;
    }

    public String getUEventDevPath() {
        return mUEventDevPath;
    }

    public void setErrorCorrectionTemp(int temp) {
        mErrorCorrectionTemp = temp;
    }

    public int getErrorCorrectionTemp() {
        return mErrorCorrectionTemp;
    }


    public void setInputTempPath(String name) {
        // sensor path is none, it means sensor temperature reporting is
        // not sysfs based. So turn sensor active by default.
        // If the sensor path does not exist, deactivate the sensor.
        if (mSensorPath != null && mSensorPath.equalsIgnoreCase("none")) {
            mIsSensorActive = true;
        } else {
            if (name != null && name.equalsIgnoreCase("auto")) {
                name = "temp";
            }
            mInputTempPath = mSensorPath + name;
            if (!isSensorSysfsValid(mInputTempPath)) {
                mIsSensorActive = false;
                Log.i(TAG, "Sensor:" + mSensorName + " path:" + mInputTempPath
                        + " is invalid...deactivaing Sensor");
            } else {
                mIsSensorActive = true;
            }
        }
    }

    public String getSensorInputTempPath() {
        return mInputTempPath;
    }

    public void setHighTempPath(String name) {
        if (name != null && name.equalsIgnoreCase("auto")) {
            mHighTempPath = mSensorPath + ThermalManager.sSysfsSensorHighTempPath;
        } else {
            if (mSensorPath != null && mSensorPath.equalsIgnoreCase("none")) {
                mHighTempPath = "invalid";
            } else {
                mHighTempPath = mSensorPath + name;
            }
        }
    }

    public String getSensorHighTempPath() {
        return mHighTempPath;
    }

    public void setLowTempPath(String name) {
        if (name != null && name.equalsIgnoreCase("auto")) {
            mLowTempPath = mSensorPath + ThermalManager.sSysfsSensorLowTempPath;
        } else {
            if (mSensorPath != null && mSensorPath.equalsIgnoreCase("none")) {
                mLowTempPath = "invalid";
            } else {
                mLowTempPath = mSensorPath + name;
            }
        }
    }

    public String getSensorLowTempPath() {
        return mLowTempPath;
    }

    public void setCurrTemp(int temp) {
        mCurrTemp = temp;
    }

    public int getCurrTemp() {
        return mCurrTemp;
    }

    /**
     * Method that actually does a Sysfs read.
     */
    public int readSensorTemp() {
        int val = ThermalUtils.readSysfsTemp(mInputTempPath);
        if (val <= ThermalManager.ABS_ZERO) {
            // Error will be returned as negative offset from absolute zero in milli degree C
            Log.e(TAG, "readSensorTemp failed with error:" + (val - ThermalManager.ABS_ZERO));
            val = ThermalManager.INVALID_TEMP;
        }
        return val;
    }

    /**
     * Method to read the current temperature from sensor. This method should be
     * used only when we want to obtain the latest temperature from sensors.
     * Otherwise, the getCurrTemp method should be used, which returns the
     * previously read value.
     */
    public void updateSensorTemp() {
        int val = readSensorTemp();
        if (val != ThermalManager.INVALID_TEMP) {
            setCurrTemp(val);
        }
    }

    public int getSensorThermalState() {
        return mSensorState;
    }

    public void setSensorThermalState(int state) {
        mSensorState = state;
    }

    public void setAutoValues() {
        if (mSensorPath.equalsIgnoreCase("auto")) {
            setSensorPath(mSensorPath);
        }
        if (mInputTempPath.equalsIgnoreCase("auto")) {
            setInputTempPath(mInputTempPath);
        }
        if (mHighTempPath.equalsIgnoreCase("auto")) {
            setHighTempPath(mHighTempPath);
        }
        if (mLowTempPath.equalsIgnoreCase("auto")) {
            setLowTempPath(mLowTempPath);
        }
        if (mUEventDevPath.equalsIgnoreCase("auto")) {
            // build the sensor UEvent listener path
            if (mSensorSysfsIndx == -1) {
                mUEventDevPath = "invalid";
                Log.i(TAG, "Cannot build UEvent path for sensor:" + mSensorName);
                return;
            } else {
                mUEventDevPath = ThermalManager.sUEventDevPath + mSensorSysfsIndx;
            }
        } else if (!mUEventDevPath.contains("DEVPATH=")) {
            mUEventDevPath = "DEVPATH=" + mUEventDevPath;
        }
    }

}
