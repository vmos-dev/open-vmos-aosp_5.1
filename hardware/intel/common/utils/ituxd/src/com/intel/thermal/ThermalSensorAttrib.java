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
 * The ThermalSensorAttrib class describes the attributes of a Thermal Sensor that are
 * zone specific.
 * @hide
 */
public class ThermalSensorAttrib {

    private static final String TAG = "ThermalSensorAttrib";
    private String mSensorName;
    private Integer mWeights[];
    private Integer mOrder[];

    public String getSensorName() {
        return mSensorName;
    }

    public void setSensorName(String name) {
        mSensorName = name;
    }

    public Integer[] getWeights() {
        return mWeights;
    }

    public void setWeights(ArrayList<Integer> list) {
        if (list != null) {
            mWeights = new Integer[list.size()];
            if (mWeights != null) {
                mWeights = list.toArray(mWeights);
            }
        }
    }

    public Integer[] getOrder() {
        return mOrder;
    }

    public void setOrder(ArrayList<Integer> list) {
        if (list != null) {
            mOrder = new Integer[list.size()];
            if (mOrder != null) {
                mOrder = list.toArray(mOrder);
            }
        }
    }

    public void printAttrs() {
        Log.i(TAG, "SensorAttrib: " + mSensorName);
        Log.i(TAG, "mWeights[]: " + Arrays.toString(mWeights));
        Log.i(TAG, "mOrder[]: " + Arrays.toString(mOrder));
        if (mWeights != null && mOrder != null && mWeights.length != mOrder.length) {
            Log.i(TAG, "mismatch in weights and order array, raw sensor temp will be used");
        }
    }
}
