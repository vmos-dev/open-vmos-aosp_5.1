/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */

package com.android.tests.testsensors;

import java.util.ArrayList;
import java.util.List;

import android.app.Activity;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.widget.TableLayout;
import android.widget.TableRow;
import android.widget.TextView;

public class TestSensorsActivity extends Activity {
    private static final String TAG = "testSensors";
    /** Array containing monitored sensors. */
    private List<MonitoredSensor> mSensors;
    /** Controls displayed list of sensors. */
    private TableLayout mTableLayout;

    /**
     * Encapsulates a sensor.
     */
    private class MonitoredSensor implements SensorEventListener {
        /** Sensor to monitor. */
        private final Sensor mSensor;
        /** Check box representing the sensor on the screen. */
        private final TextView mName;
        /** Text view displaying the value of the sensor. */
        private final TextView mVal;
        /** Formats string to show in the TextView. */
        private String mTextFmt;

        /**
         * Constructs MonitoredSensor instance, and register the listeners.
         *
         * @param sensor Sensor to monitor.
         */
        MonitoredSensor(Sensor sensor) {
            mSensor = sensor;

            // Add a row representing this sensor on the display
            final LayoutInflater inflater = getLayoutInflater();
            final TableRow row = (TableRow) inflater.inflate(R.layout.one_row, mTableLayout, false);
            mTableLayout.addView(row);

            // Initialize displayed checkbox for this sensor, and register
            // checked state listener for it.
            mName = (TextView) row.findViewById(R.id.row_textview_name);

            // Initialize displayed text box for this sensor.
            mVal = (TextView) row.findViewById(R.id.row_textview_value);
            mVal.setText("");

            // Set appropriate sensor name depending on the type. Unfortunately,
            // we can't really use sensor.getName() here, since the value it
            // returns (although resembles the purpose) is a bit vaguer than it
            // should be. Also choose an appropriate format for the strings that
            // display sensor's value, and strings that are sent to the
            // emulator.
            switch (sensor.getType()) {
                case Sensor.TYPE_ACCELEROMETER:
                    mName.setText("Accelerometer");
                    // 3 floats.
                    mTextFmt = "%+.2f %+.2f %+.2f";
                    break;
                case 9: // Sensor.TYPE_GRAVITY is missing in API 7
                    // 3 floats.
                    mName.setText("Gravity");
                    mTextFmt = "%+.2f %+.2f %+.2f";
                    break;
                case Sensor.TYPE_GYROSCOPE:
                    mName.setText("Gyroscope");
                    // 3 floats.
                    mTextFmt = "%+.2f %+.2f %+.2f";
                    break;
                case Sensor.TYPE_LIGHT:
                    mName.setText("Light");
                    // 1 integer.
                    mTextFmt = "%.0f";
                    break;
                case 10: // Sensor.TYPE_LINEAR_ACCELERATION is missing in API 7
                    mName.setText("Linear acceleration");
                    // 3 floats.
                    mTextFmt = "%+.2f %+.2f %+.2f";
                    break;
                case Sensor.TYPE_MAGNETIC_FIELD:
                    mName.setText("Magnetic field");
                    // 3 floats.
                    mTextFmt = "%+.2f %+.2f %+.2f";
                    break;
                case Sensor.TYPE_ORIENTATION:
                    mName.setText("Orientation");
                    // 3 integers.
                    mTextFmt = "%+03.0f %+03.0f %+03.0f";
                    break;
                case Sensor.TYPE_PRESSURE:
                    mName.setText("Pressure");
                    // 1 integer.
                    mTextFmt = "%.0f";
                    break;
                case Sensor.TYPE_PROXIMITY:
                    mName.setText("Proximity");
                    // 1 integer.
                    mTextFmt = "%.0f";
                    break;
                case 11: // Sensor.TYPE_ROTATION_VECTOR is missing in API 7
                    mName.setText("Rotation");
                    // 3 floats.
                    mTextFmt = "%+.2f %+.2f %+.2f";
                    break;
                case Sensor.TYPE_TEMPERATURE:
                    mName.setText("Temperature");
                    // 1 integer.
                    mTextFmt = "%.0f";
                    break;
                default:
                    mName.setText("<Unknown>");
                    mTextFmt = "N/A";
                    break;
            }
        }

        /**
         * Gets sensor type.
         *
         * @return Sensor type as one of the Sensor.TYPE_XXX constants.
         */
        private int getType() {
            return mSensor.getType();
        }

        /**
         * Starts monitoring the sensor. NOTE: This method is called from
         * outside of the UI thread.
         */
        private void startListening() {
            SensorManager sm = (SensorManager) getSystemService(SENSOR_SERVICE);
            sm.registerListener(this, mSensor, SensorManager.SENSOR_DELAY_UI);
        }

        /**
         * Stops monitoring the sensor. NOTE: This method is called from outside
         * of the UI thread.
         */
        private void stopListening() {
            SensorManager sm = (SensorManager) getSystemService(SENSOR_SERVICE);
            sm.unregisterListener(this);
        }

        /**
         * Handles "sensor changed" event. This is an implementation of the
         * SensorEventListener interface.
         */
        @Override
        public void onSensorChanged(SensorEvent event) {
            // Display current sensor value, and format message that will be
            // sent to the emulator.
            final int nArgs = event.values.length;
            String val;
            if (nArgs == 3) {
                val = String.format(mTextFmt, event.values[0], event.values[1], event.values[2]);
            } else if (nArgs == 2) {
                val = String.format(mTextFmt, event.values[0], event.values[1]);
            } else if (nArgs == 1) {
                val = String.format(mTextFmt, event.values[0]);
            } else {
                return;
            }
            mVal.setText(val);
        }

        /**
         * Handles "sensor accuracy changed" event. This is an implementation of
         * the SensorEventListener interface.
         */
        @Override
        public void onAccuracyChanged(Sensor sensor, int accuracy) {
        }
    } // MonitoredSensor

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        mTableLayout = (TableLayout) findViewById(R.id.tableLayout);

        // Iterate through the available sensors, adding them to the array.
        mSensors = new ArrayList<MonitoredSensor>();
        SensorManager sm = (SensorManager) getSystemService(SENSOR_SERVICE);
        List<Sensor> sensors = sm.getSensorList(Sensor.TYPE_ALL);
        int cur_index = 0;
        for (int n = 0; n < sensors.size(); n++) {
            Sensor avail_sensor = sensors.get(n);

            // There can be multiple sensors of the same type. We need only one.
            if (!isSensorTypeAlreadyMonitored(avail_sensor.getType())) {
                // The first sensor we've got for the given type is not
                // necessarily the right one. So, use the default sensor
                // for the given type.
                Sensor def_sens = sm.getDefaultSensor(avail_sensor.getType());
                MonitoredSensor to_add = new MonitoredSensor(def_sens);
                cur_index++;
                mSensors.add(to_add);
                to_add.startListening();
            }
        }
    }

    /**
     * Checks if a sensor for the given type is already monitored.
     *
     * @param type Sensor type (one of the Sensor.TYPE_XXX constants)
     * @return true if a sensor for the given type is already monitored, or
     *         false if the sensor is not monitored.
     */
    private boolean isSensorTypeAlreadyMonitored(int type) {
        for (int n = 0; n < mSensors.size(); n++) {
            if (mSensors.get(n).getType() == type) {
                return true;
            }
        }
        return false;
    }
}
