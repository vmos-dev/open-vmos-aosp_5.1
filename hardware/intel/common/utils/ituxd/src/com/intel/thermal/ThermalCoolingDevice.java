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

import java.lang.reflect.Method;
import java.util.ArrayList;

/**
 * This class contains the cooling device specific information. It also contains
 * a reference to the actual throttle function.
 *
 * @hide
 */
public class ThermalCoolingDevice {

    private static final String TAG = "ThermalCoolingDevice";

    private String mDeviceName;

    private String mClassPath;

    private String mThrottlePath;

    private int mCurrentThermalState;

    private int mDeviceId;

    private Class mDeviceClass;

    private Method mThrottleMethod;

    /* Maintains list of zoneid's under which this cooling device falls. */
    private ArrayList<Integer> mZoneIdList = new ArrayList<Integer>();

    /* Maintains corresponding state of zone present in mZoneidList */
    private ArrayList<Integer> mZoneStateList = new ArrayList<Integer>();

    /* List of values used to throttle this cooling device */
    private ArrayList<Integer> mThrottleValues = null;

    private int mNumThrottleValues = ThermalManager.DEFAULT_NUM_THROTTLE_VALUES;

    public ThermalCoolingDevice() {
        mCurrentThermalState = -1;
        mThrottlePath = "auto";
        mClassPath = "auto";
    }

    public void setNumThrottleValues(int num) {
        mNumThrottleValues = num;
    }

    public int getNumThrottleValues() {
        return mNumThrottleValues;
    }

    public void setDeviceName(String Name) {
        mDeviceName = Name;
    }

    public String getDeviceName() {
        return mDeviceName;
    }

    public void setDeviceId(int deviceId) {
        mDeviceId = deviceId;
    }

    public int getDeviceId() {
        return mDeviceId;
    }

    public String getClassPath() {
        return mClassPath;
    }

    public void setClassPath(String Path) {
        mClassPath = Path;
    }

    public Class getDeviceClass() {
        return mDeviceClass;
    }

    public void setDeviceClass(Class cls) {
        mDeviceClass = cls;
    }

    public Method getThrottleMethod() {
        return mThrottleMethod;
    }

    public void setThrottleMethod(Method method) {
        mThrottleMethod = method;
    }

    public String getThrottlePath() {
        return mThrottlePath;
    }

    public void setThrottlePath(String path) {
        if (path.equalsIgnoreCase("auto") && !mDeviceName.equalsIgnoreCase("battery")) {
            //construct the throttle path
            int indx = ThermalUtils.getCoolingDeviceIndexContains(mDeviceName);
            if (indx != -1) {
                mThrottlePath = ThermalManager.sCoolingDeviceBasePath + indx +
                        ThermalManager.sCoolingDeviceState;
            } else {
                mThrottlePath = "invalid";
            }
        } else {
            mThrottlePath = path;
        }
    }

    public ArrayList<Integer> getZoneIdList() {
        return mZoneIdList;
    }

    public ArrayList<Integer> getZoneStateList() {
        return mZoneStateList;
    }

    public ArrayList<Integer> getThrottleValuesList() {
        return mThrottleValues;
    }

    public void setThrottleValuesList(ArrayList<Integer> list) {
        mThrottleValues = list;
        mNumThrottleValues = list.size();
    }
    /**
     * Sets the current thermal state of cooling device which will be maximum of
     * all states of zones under which this cooling device falls.
     */
    private void updateCurrentThermalState() {
        int state = 0;
        for (Integer coolingDevState : mZoneStateList) {
            state = Math.max(state, coolingDevState);
        }
        mCurrentThermalState = state;
    }

    /**
     * Adds zoneID and its thermal state to mListOfZoneIDs and
     * mListOfTStatesOfZones array. If zoneId exists then its thermal state is
     * updated else zoneId and its state will be added to array.
     */
    public void updateZoneState(int zoneId, int state) {
        int index = -1;

        if (!mZoneIdList.isEmpty()) {
            index = mZoneIdList.indexOf(zoneId);
        }

        // Entry does not exist
        if (index == -1) {
            mZoneIdList.add(zoneId);
            mZoneStateList.add(state);
        } else {
            mZoneStateList.set(index, state);
        }

        updateCurrentThermalState();
    }

    public int getThermalState() {
        return mCurrentThermalState;
    }

}
