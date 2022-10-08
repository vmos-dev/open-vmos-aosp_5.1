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

import android.app.ActivityManagerNative;
import android.app.IntentService;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.Process;
import android.os.SystemProperties;
import android.os.UserHandle;
import android.util.Log;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.lang.ClassLoader;
import java.lang.NullPointerException;
import java.lang.reflect.Array;
import java.lang.SecurityException;
import java.util.ArrayList;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.Iterator;
import java.util.Map;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;
import org.xmlpull.v1.XmlPullParserFactory;

/**
 * The ThermalService monitors the Thermal zones on the platform.
 * The number of thermal zones and sensors associated with the zones are
 * obtained from the thermal_sensor_config.xml file. When any thermal zone
 * crosses the thresholds configured in the xml, a Thermal Intent is sent.
 * ACTION_THERMAL_ZONE_STATE_CHANGED
 * The Thermal Cooling Manager acts upon this intent and throttles
 * the corresponding cooling device.
 *
 * @hide
 */
public class ThermalService extends Service {
    private static final String TAG = ThermalService.class.getSimpleName();
    private static Context mContext;
    private Handler mHandler = new Handler();
    static {
        System.loadLibrary("thermalJNI");
    }
    protected enum MetaTag {
            ENUM_UNKNOWN,
            ENUM_ZONETHRESHOLD,
            ENUM_POLLDELAY,
            ENUM_MOVINGAVGWINDOW
    }

    public class ThermalParser {
        // Names of the XML Tags
        private static final String PINFO = "PlatformInfo";
        private static final String SENSOR_ATTRIB = "SensorAttrib";
        private static final String SENSOR = "Sensor";
        private static final String ZONE = "Zone";
        private static final String THERMAL_CONFIG = "thermalconfig";
        private static final String THRESHOLD = "Threshold";
        private static final String POLLDELAY = "PollDelay";
        private static final String MOVINGAVGWINDOW = "MovingAverageWindow";
        private static final String ZONELOGIC = "ZoneLogic";
        private static final String WEIGHT = "Weight";
        private static final String ORDER = "Order";
        private static final String OFFSET = "Offset";
        private static final String ZONETHRESHOLD = "ZoneThreshold";
        private static final String PROFILE = "Profile";

        private boolean mDone = false;
        private ThermalManager.PlatformInfo mPlatformInfo = null;
        private ThermalSensor mCurrSensor = null;
        private ThermalZone mCurrZone = null;
        private ArrayList<ThermalSensorAttrib> mCurrSensorAttribList = null;
        private ThermalSensorAttrib mCurrSensorAttrib = null;
        private ArrayList<ThermalZone> mThermalZones = null;
        private ArrayList<Integer> mPollDelayList = null;
        private ArrayList<Integer> mMovingAvgWindowList = null;
        private ArrayList<Integer> mWeightList = null;
        private ArrayList<Integer> mOrderList = null;
        private ArrayList<Integer> mZoneThresholdList = null;
        private String mSensorName = null;
        XmlPullParserFactory mFactory = null;
        XmlPullParser mParser = null;
        int mTempZoneId = -1;
        int mNumProfiles = 0;
        String mTempZoneName = null;
        String mCurProfileName = ThermalManager.DEFAULT_PROFILE_NAME;
        FileReader mInputStream = null;

        ThermalParser(String fname) {
            try {
                mFactory = XmlPullParserFactory.newInstance(System.
                        getProperty(XmlPullParserFactory.PROPERTY_NAME), null);
                mFactory.setNamespaceAware(true);
                mParser = mFactory.newPullParser();
            } catch (SecurityException e) {
                Log.e(TAG, "SecurityException caught in ThermalParser");
            } catch (IllegalArgumentException e) {
                Log.e(TAG, "IllegalArgumentException caught in ThermalParser");
            } catch (XmlPullParserException xppe) {
                Log.e(TAG, "XmlPullParserException caught in ThermalParser");
            }

            try {
                mInputStream = new FileReader(fname);
                mPlatformInfo = null;
                mCurrSensor = null;
                mCurrZone = null;
                mThermalZones = null;
                if (mInputStream == null) return;
                if (mParser != null) {
                    mParser.setInput(mInputStream);
                }
            } catch (FileNotFoundException e) {
                Log.e(TAG, "FileNotFoundException Exception in ThermalParser()");
            } catch (XmlPullParserException e) {
                Log.e(TAG, "XmlPullParserException Exception in ThermalParser()");
            }
        }

        ThermalParser() {
            mParser = mContext.getResources().
                    getXml(ThermalManager.sSensorFileXmlId);
        }

        public ThermalManager.PlatformInfo getPlatformInfo() {
            return mPlatformInfo;
        }

        public boolean parse() {
            if (ThermalManager.sIsOverlays == false && mInputStream == null) return false;
            /* if mParser is null, close any open stream before exiting */
            if (mParser == null) {
                try {
                    if (mInputStream != null) {
                        mInputStream.close();
                    }
                } catch (IOException e) {
                    Log.i(TAG, "IOException caught in parse() function");
                }
                return false;
            }

            boolean ret = true;
            MetaTag tag = MetaTag.ENUM_UNKNOWN;
            try {
                int mEventType = mParser.getEventType();
                while (mEventType != XmlPullParser.END_DOCUMENT && !mDone) {
                    switch (mEventType) {
                        case XmlPullParser.START_DOCUMENT:
                            Log.i(TAG, "StartDocument");
                            break;
                        case XmlPullParser.START_TAG:
                            String tagName = mParser.getName();
                            boolean isMetaTag = false;
                            if (tagName != null && tagName.equalsIgnoreCase(ZONETHRESHOLD)) {
                                tag = MetaTag.ENUM_ZONETHRESHOLD;
                                isMetaTag = true;
                            } else if (tagName != null && tagName.equalsIgnoreCase(POLLDELAY)) {
                                tag = MetaTag.ENUM_POLLDELAY;
                                isMetaTag = true;
                            } else if (tagName != null
                                    && tagName.equalsIgnoreCase(MOVINGAVGWINDOW)) {
                                tag = MetaTag.ENUM_MOVINGAVGWINDOW;
                                isMetaTag = true;
                            }
                            if (isMetaTag) {
                                ret = processMetaTag(tagName, tag);
                            } else {
                                ret = processStartElement(tagName);
                            }
                            if (!ret) {
                                if (mInputStream != null) mInputStream.close();
                                return false;
                            }
                            break;
                        case XmlPullParser.END_TAG:
                            processEndElement(mParser.getName());
                            break;
                    }
                    mEventType = mParser.next();
                }
            } catch (XmlPullParserException xppe) {
                Log.i(TAG, "XmlPullParserException caught in parse():" + xppe.getMessage());
                ret = false;
            } catch (IOException e) {
                Log.i(TAG, "IOException caught in parse():" + e.getMessage());
                ret = false;
            } finally {
                try {
                    // end of parsing, close the stream
                    // close is moved here, since if there is an exception
                    // while parsing doc, input stream needs to be closed
                    if (mInputStream != null) mInputStream.close();
                } catch (IOException e) {
                    Log.i(TAG, "IOException caught in parse() function");
                    ret = false;
                }
                return ret;
            }
        }

        boolean processMetaTag(String tagName, MetaTag tagId) {
            if (mParser == null || tagName == null || mCurrZone == null)  return false;
            ArrayList<Integer> tempList;
            tempList = new ArrayList<Integer>();
            // add the dummy value for TOFF now. update it once meta tag parsed
            tempList.add(0);
            try {
                int eventType = mParser.next();
                while (true) {
                    if (eventType == XmlPullParser.START_TAG) {
                        tempList.add(Integer.parseInt(mParser.nextText()));
                    } else if (eventType == XmlPullParser.END_TAG &&
                            mParser.getName().equalsIgnoreCase(tagName)) {
                        break;
                    }
                    eventType = mParser.next();
                }
            } catch (XmlPullParserException xppe) {
                Log.e(TAG, "XmlPullParserException:" + xppe.getMessage());
                return false;
            } catch (IOException ioe) {
                Log.e(TAG, "IOException:" + ioe.getMessage());
                return false;
            }
            // now that all state values are parse, copy the value corresponding to <normal>
            // state to TOFF and last state to CRITICAL state.
            // now we have reached end of meta tag add this temp list to appropriate list
            switch(tagId) {
                case ENUM_POLLDELAY:
                    // add TOFF
                    tempList.set(0, tempList.get(1));
                    // add TCRITICAL
                    tempList.add(tempList.get(tempList.size() - 1));
                    mCurrZone.setPollDelay(tempList);
                    break;
                case ENUM_ZONETHRESHOLD:
                    // add TCRITICAL
                    tempList.add(tempList.get(tempList.size() - 1));
                    mCurrZone.updateMaxStates(tempList.size());
                    mCurrZone.setZoneTempThreshold(tempList);
                    break;
                case ENUM_MOVINGAVGWINDOW:
                    // add TOFF
                    tempList.set(0, tempList.get(1));
                    // add TCRITICAL
                    tempList.add(tempList.get(tempList.size() - 1));
                    mCurrZone.setMovingAvgWindow(tempList);
                    break;
                case ENUM_UNKNOWN:
                default:
                    break;
            }
            tempList = null;
            return true;
        }

        boolean processStartElement(String name) {
            if (name == null)
                return false;
            String zoneName;
            boolean ret = true;
            try {
                if (name.equalsIgnoreCase(PINFO)) {
                    mPlatformInfo = new ThermalManager.PlatformInfo();
                    // Default Thermal States
                    mPlatformInfo.mMaxThermalStates = 5;
                } else if (name.equalsIgnoreCase(PROFILE)) {
                    mNumProfiles++;
                } else if (name.equalsIgnoreCase(SENSOR)) {
                    if (mCurrSensor == null) {
                        mCurrSensor = new ThermalSensor();
                    }
                } else if (name.equalsIgnoreCase(SENSOR_ATTRIB)) {
                    if (mCurrSensorAttribList == null) {
                        mCurrSensorAttribList = new ArrayList<ThermalSensorAttrib>();
                    }
                    mCurrSensorAttrib = new ThermalSensorAttrib();
                } else if (name.equalsIgnoreCase(ZONE)) {
                    if (mThermalZones == null)
                        mThermalZones = new ArrayList<ThermalZone>();
                } else {
                    // Retrieve Platform Information
                    if (mPlatformInfo != null && name.equalsIgnoreCase("PlatformThermalStates")) {
                        mPlatformInfo.mMaxThermalStates = Integer.parseInt(mParser.nextText());
                        // Retrieve Zone Information
                    } else if (name.equalsIgnoreCase("ZoneName") && mTempZoneId != -1) {
                        mTempZoneName = mParser.nextText();
                    } else if (name.equalsIgnoreCase("Name")) {
                        mCurProfileName = mParser.nextText();
                    } else if (name.equalsIgnoreCase(ZONELOGIC) && mTempZoneId != -1
                            && mTempZoneName != null) {
                        String zoneLogic = mParser.nextText();
                        if (zoneLogic.equalsIgnoreCase("VirtualSkin")) {
                            mCurrZone = new VirtualThermalZone();
                        } else {
                            // default zone raw
                            mCurrZone = new RawThermalZone();
                        }
                        if (mCurrZone != null) {
                            mCurrZone.setZoneName(mTempZoneName);
                            mCurrZone.setZoneId(mTempZoneId);
                            mCurrZone.setZoneLogic(zoneLogic);
                        }
                    } else if (name.equalsIgnoreCase("ZoneID")) {
                        mTempZoneId = Integer.parseInt(mParser.nextText());
                    } else if (name.equalsIgnoreCase("SupportsUEvent") && mCurrZone != null)
                        mCurrZone.setSupportsUEvent(Integer.parseInt(mParser.nextText()));
                    else if (name.equalsIgnoreCase("SupportsEmulTemp") && mCurrZone != null)
                        mCurrZone.setEmulTempFlag(Integer.parseInt(mParser.nextText()));
                    else if (name.equalsIgnoreCase("DebounceInterval") && mCurrZone != null)
                        mCurrZone.setDBInterval(Integer.parseInt(mParser.nextText()));
                    else if (name.equalsIgnoreCase(POLLDELAY) && mCurrZone != null) {
                        mPollDelayList = new ArrayList<Integer>();
                    } else if (name.equalsIgnoreCase(OFFSET) && mCurrZone != null) {
                        mCurrZone.setOffset(Integer.parseInt(mParser.nextText()));
                    }

                    // Retrieve Sensor Information
                    else if (name.equalsIgnoreCase("SensorName")) {
                        if (mCurrSensorAttrib != null) {
                            mCurrSensorAttrib.setSensorName(mParser.nextText());
                        } else if (mCurrSensor != null) {
                            mCurrSensor.setSensorName(mParser.nextText());
                        }
                    } else if (name.equalsIgnoreCase("SensorPath") && mCurrSensor != null)
                        mCurrSensor.setSensorPath(mParser.nextText());
                    else if (name.equalsIgnoreCase("InputTemp") && mCurrSensor != null)
                        mCurrSensor.setInputTempPath(mParser.nextText());
                    else if (name.equalsIgnoreCase("HighTemp") && mCurrSensor != null)
                        mCurrSensor.setHighTempPath(mParser.nextText());
                    else if (name.equalsIgnoreCase("LowTemp") && mCurrSensor != null)
                        mCurrSensor.setLowTempPath(mParser.nextText());
                    else if (name.equalsIgnoreCase("UEventDevPath") && mCurrSensor != null)
                        mCurrSensor.setUEventDevPath(mParser.nextText());
                    else if (name.equalsIgnoreCase("ErrorCorrection") && mCurrSensor != null)
                        mCurrSensor.setErrorCorrectionTemp(Integer.parseInt(mParser.nextText()));
                    else if (name.equalsIgnoreCase(WEIGHT) && mCurrSensorAttrib != null) {
                        if (mWeightList == null) {
                            mWeightList = new ArrayList<Integer>();
                        }
                        if (mWeightList != null) {
                            mWeightList.add(Integer.parseInt(mParser.nextText()));
                        }
                    } else if (name.equalsIgnoreCase(ORDER) && mCurrSensorAttrib != null) {
                        if (mOrderList == null) {
                            mOrderList = new ArrayList<Integer>();
                        }
                        if (mOrderList != null) {
                            mOrderList.add(Integer.parseInt(mParser.nextText()));
                        }
                    }
                }
            } catch (XmlPullParserException e) {
                Log.i(TAG, "XmlPullParserException caught in processStartElement()");
                ret = false;
            } catch (IOException e) {
                Log.i(TAG, "IOException caught in processStartElement()");
                ret = false;
            } finally {
                return ret;
            }
        }

        void processEndElement(String name) {
            if (name.equalsIgnoreCase(SENSOR)) {
                // insert in map, only if no sensor with same name already in map
                if (mCurrSensor == null) return;
                mCurrSensor.setAutoValues();
                if (ThermalManager.getSensor(mCurrSensor.getSensorName()) == null) {
                    ThermalManager.sSensorMap.put(mCurrSensor.getSensorName(), mCurrSensor);
                } else {
                    Log.i(TAG, "sensor:" + mCurrSensor.getSensorName() + " already present");
                }
                mCurrSensor = null;
            } else if (name.equalsIgnoreCase(SENSOR_ATTRIB) && mCurrSensorAttribList != null) {
                if (mCurrSensorAttrib != null) {
                    mCurrSensorAttrib.setWeights(mWeightList);
                    mCurrSensorAttrib.setOrder(mOrderList);
                }
                mWeightList = null;
                mOrderList = null;
                if (mCurrSensorAttrib != null
                        && ThermalManager.getSensor(mCurrSensorAttrib.getSensorName()) != null) {
                    // this is valid sensor, so now update the zone sensorattrib list
                    // and sensor list.This check is needed to avoid a scenario where
                    // a invalid sensor name might be included in sensorattrib list.
                    // This check filters out all invalid sensor attrib.
                    mCurrSensorAttribList.add(mCurrSensorAttrib);
                }
            } else if (name.equalsIgnoreCase(ZONE) && mCurrZone != null
                    && mThermalZones != null) {
                mCurrZone.setSensorList(mCurrSensorAttribList);
                mThermalZones.add(mCurrZone);
                mCurrZone = null;
                mTempZoneId = -1;
                mTempZoneName = null;
                mCurrSensorAttribList = null;
            } else if (name.equalsIgnoreCase(POLLDELAY) && mCurrZone != null) {
                mCurrZone.setPollDelay(mPollDelayList);
                mPollDelayList = null;
            } else if (name.equalsIgnoreCase(MOVINGAVGWINDOW) && mCurrZone != null) {
                mCurrZone.setMovingAvgWindow(mMovingAvgWindowList);
                mMovingAvgWindowList = null;
            } else if (name.equalsIgnoreCase(THERMAL_CONFIG)) {
                // This indicates we have not seen any <Profile> tag.
                // Consider it as if we have only one 'Default' Profile.
                if (mNumProfiles == 0) {
                    ThermalManager.sProfileZoneMap.put(mCurProfileName, mThermalZones);
                }
                mDone = true;
            } else if (name.equalsIgnoreCase(PROFILE)) {
                ThermalManager.sProfileZoneMap.put(mCurProfileName, mThermalZones);
                mThermalZones = null;
            } else if (name.equalsIgnoreCase(ZONETHRESHOLD) && mCurrZone != null) {
                mCurrZone.setZoneTempThreshold(mZoneThresholdList);
                mZoneThresholdList = null;
            }
        }
    }

    /* Class to notifying thermal events */
    public class Notify implements Runnable {
        private final BlockingQueue cQueue;
        Notify (BlockingQueue q) {
            cQueue = q;
        }

        public void run () {
            try {
                while (true) { consume((ThermalEvent) cQueue.take()); }
            } catch (InterruptedException ex) {
                Log.i(TAG, "caught InterruptedException in run()");
            }
        }

        /* Method to consume thermal event */
        public void consume (ThermalEvent event) {
            Intent statusIntent = new Intent();
            statusIntent.setAction(ThermalManager.ACTION_THERMAL_ZONE_STATE_CHANGED);

            statusIntent.putExtra(ThermalManager.EXTRA_NAME, event.mZoneName);
            statusIntent.putExtra(ThermalManager.EXTRA_PROFILE, event.mProfName);
            statusIntent.putExtra(ThermalManager.EXTRA_ZONE, event.mZoneId);
            statusIntent.putExtra(ThermalManager.EXTRA_EVENT, event.mEventType);
            statusIntent.putExtra(ThermalManager.EXTRA_STATE, event.mThermalLevel);
            statusIntent.putExtra(ThermalManager.EXTRA_TEMP, event.mZoneTemp);

            /* Send the Thermal Intent */
            mContext.sendBroadcastAsUser(statusIntent, UserHandle.ALL);
        }
    }

    /* Register for boot complete Intent */
    public ThermalService() {
        super();
    }

    private void configureTurboProperties() {
        String prop = SystemProperties.get("persist.thermal.turbo.dynamic");

        if (prop.equals("0")) {
            ThermalManager.sIsDynamicTurboEnabled = false;
            Log.i(TAG, "Dynamic Turbo disabled through persist.thermal.turbo.dynamic");
        } else if (prop.equals("1")) {
            ThermalManager.sIsDynamicTurboEnabled = true;
            Log.i(TAG, "Dynamic Turbo enabled through persist.thermal.turbo.dynamic");
        } else {
            // Set it to true so that we don't write ThermalManager.DISABLE_DYNAMIC_TURBO
            // into any cooling device based on this.
            ThermalManager.sIsDynamicTurboEnabled = true;
            Log.i(TAG, "property persist.thermal.turbo.dynamic not present");
        }
    }

    @Override
    public void onDestroy() {
        // stop all thread
        ThermalManager.stopCurrentProfile();
        ThermalManager.sCoolingManager.unregisterReceivers();
        // clear all static data
        ThermalManager.clearData();
        Log.w(TAG, "ituxd destroyed");
    }

    @Override
    public void onCreate() {
        mContext = getApplicationContext();
        ThermalManager.setContext(mContext);
    }

    @Override
    public IBinder onBind(Intent intent) {
        return(null);
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startid)
    {
        boolean ret;
        ThermalManager.loadiTUXVersion();
        /* Check for exitence of config files */
        ThermalUtils.initialiseConfigFiles(mContext);
        if (!ThermalManager.sIsConfigFiles && !ThermalManager.sIsOverlays) {
            Log.i(TAG, "Thermal config files do not exist. Exiting ThermalService");
            return START_NOT_STICKY;
        }

        /* Set Dynamic Turbo status based on the property */
        configureTurboProperties();

        /* Intiliaze DTS TjMax temperature */
        ThermalUtils.getTjMax();

        /* Initialize the Thermal Cooling Manager */
        ThermalManager.sCoolingManager = new ThermalCooling();
        if (ThermalManager.sCoolingManager != null) {
            ret = ThermalManager.sCoolingManager.init(mContext);
            if (!ret) {
                Log.i(TAG, "CoolingManager is null. Exiting ThermalService");
                return START_NOT_STICKY;
            }
        }

        /* Parse the thermal configuration file to determine zone/sensor information */
        ThermalParser mThermalParser;
        if (ThermalManager.sIsConfigFiles) {
            mThermalParser = new ThermalParser(ThermalManager.sSensorFilePath);
        } else {
            mThermalParser = new ThermalParser();
        }

        if (mThermalParser != null) {
            ret = mThermalParser.parse();
            if (!ret) {
                ThermalManager.sCoolingManager.unregisterReceivers();
                Log.i(TAG, "thermal_sensor_config.xml parsing Failed. Exiting ThermalService");
                return START_NOT_STICKY;
            }
        }

        /* Retrieve the platform information after parsing */
        ThermalManager.sPlatformInfo = mThermalParser.getPlatformInfo();

        /* Print thermal_sensor_config.xml information */
        Iterator it = ThermalManager.sProfileZoneMap.entrySet().iterator();
        while (it.hasNext()) {
            Map.Entry entry = (Map.Entry) it.next();
            String key = (String) entry.getKey();
            ArrayList<ThermalZone> tzList = (ArrayList<ThermalZone>) entry.getValue();
            Log.i(TAG, "Zones under Profile: " + key);
            for (ThermalZone tz : tzList) tz.printAttrs();
        }

        /* read persistent system properties for shutdown notification */
        ThermalManager.readShutdownNotiferProperties();
        /* initialize the thermal notifier thread */
        Notify notifier = new Notify(ThermalManager.sEventQueue);
        new Thread(notifier, "ThermalNotifier").start();

        ThermalManager.buildProfileNameList();
        ThermalManager.initializeStickyIntent();

        /* Building bucket size for all profiles */
        ThermalManager.setBucketSizeForProfiles();

        /* Start monitoring the zones in Default Thermal Profile */
        ThermalManager.startDefaultProfile();

        return START_STICKY;
    }
}
