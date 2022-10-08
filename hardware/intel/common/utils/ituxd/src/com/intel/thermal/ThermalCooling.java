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

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;
import org.xmlpull.v1.XmlPullParserFactory;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.SystemProperties;
import android.util.Log;

import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Hashtable;

/**
 * The ThermalCooling class parses the thermal_throttle_config.xml. This class
 * receives Thermal Intents and takes appropriate actions based on the policies
 * configured in the xml file.
 *
 * @hide
 */
public class ThermalCooling {
    private static final String TAG = "ThermalCooling";
    private static final String THERMAL_SHUTDOWN_NOTIFY_PATH =
            "/sys/module/intel_mid_osip/parameters/force_shutdown_occured";

    private Context mContext;

    // count to keep track of zones in critical state, waiting for shutdown
    private int mCriticalZonesCount = 0;
    private static final Object sCriticalZonesCountLock = new Object();

    private ThermalZoneReceiver mThermalIntentReceiver = new ThermalZoneReceiver();
    private ProfileChangeReceiver mProfChangeReceiver = new ProfileChangeReceiver();
    private boolean mProfChangeListenerInitialized = false;
    /**
     * This is the parser class which parses the thermal_throttle_config.xml
     * file.
     */
    protected enum MetaTag {
        ENUM_THROTTLEVALUES,
        ENUM_THROTTLEMASK,
        ENUM_DETHROTTLEMASK,
        ENUM_UNKNOWN
    }

    public class ThermalParser {
        private static final String THERMAL_THROTTLE_CONFIG = "thermalthrottleconfig";

        private static final String CDEVINFO = "ContributingDeviceInfo";

        private static final String ZONETHROTINFO = "ZoneThrottleInfo";

        private static final String COOLINGDEVICEINFO = "CoolingDeviceInfo";

        private static final String THROTTLEMASK = "ThrottleDeviceMask";

        private static final String DETHROTTLEMASK = "DethrottleDeviceMask";

        private static final String THROTTLEVALUES = "ThrottleValues";

        private static final String COOLINGDEVICESTATES = "CoolingDeviceStates";

        private static final String PROFILE = "Profile";

        private ArrayList<Integer> mTempMaskList;

        private ArrayList<Integer> mTempThrottleValuesList;;

        private boolean done = false;

        XmlPullParserFactory mFactory;

        XmlPullParser mParser;

        ThermalCoolingDevice mDevice = null;

        /* Hashtable of (ZoneID and ZoneCoolerBindingInfo object) */
        Hashtable<Integer, ThermalManager.ZoneCoolerBindingInfo> mZoneCoolerBindMap = null;
        String mCurProfileName = ThermalManager.DEFAULT_PROFILE_NAME;
        int mNumProfiles = 0;

        ThermalManager.ZoneCoolerBindingInfo mZone = null;

        FileReader mInputStream = null;

        ThermalParser(String fname) {
            try {
                mFactory = XmlPullParserFactory.newInstance(
                        System.getProperty(XmlPullParserFactory.PROPERTY_NAME), null);
                mFactory.setNamespaceAware(true);
                mParser = mFactory.newPullParser();
            } catch (XmlPullParserException xppe) {
                Log.e(TAG, "mParser NewInstance Exception");
            }

            try {
                mInputStream = new FileReader(fname);
                if (mInputStream == null)
                    return;
                if (mParser != null) {
                    mParser.setInput(mInputStream);
                }
                mDevice = null;
                mZone = null;
            } catch (XmlPullParserException xppe) {
                Log.e(TAG, "mParser setInput XmlPullParserException");
            } catch (FileNotFoundException e) {
                Log.e(TAG, "mParser setInput FileNotFoundException");
            }

        }

        ThermalParser() {
            mParser = mContext.getResources().
                    getXml(ThermalManager.sThrottleFileXmlId);
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
                while (mEventType != XmlPullParser.END_DOCUMENT && !done) {
                    switch (mEventType) {
                        case XmlPullParser.START_DOCUMENT:
                            Log.i(TAG, "StartDocument");
                            break;
                        case XmlPullParser.START_TAG:
                            String tagName = mParser.getName();
                            boolean isMetaTag = false;
                            if (tagName != null && tagName.equalsIgnoreCase(THROTTLEVALUES)) {
                                tag = MetaTag.ENUM_THROTTLEVALUES;
                                isMetaTag = true;
                            } else if (tagName != null && tagName.equalsIgnoreCase(THROTTLEMASK)) {
                                tag = MetaTag.ENUM_THROTTLEMASK;
                                isMetaTag = true;
                            } else if (tagName != null
                                    && tagName.equalsIgnoreCase(DETHROTTLEMASK)) {
                                tag = MetaTag.ENUM_DETHROTTLEMASK;
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
                    if (mInputStream != null) {
                        mInputStream.close();
                    }
                } catch (IOException e) {
                    Log.i(TAG, "IOException caught in parse() function");
                    ret = false;
                }
                return ret;
            }
        }

        public boolean processMetaTag(String tagName, MetaTag tagId) {
            if (mParser == null || tagName == null)  return false;
            ArrayList<Integer> tempList = new ArrayList<Integer>();
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

            switch(tagId) {
                case ENUM_THROTTLEVALUES:
                    if (mDevice == null) {
                        return false;
                    } else {
                        // add throttle value for TCRITICAL (same as last value)
                        tempList.add(tempList.get(tempList.size() - 1));
                        mDevice.setThrottleValuesList(tempList);
                    }
                    break;
                case ENUM_THROTTLEMASK:
                    if (mZone == null || mZone.getLastCoolingDeviceInstance() ==  null) {
                        return false;
                    } else {
                        // Always throttle at CRITICAL state (last state)
                        tempList.add(1);
                        mZone.getLastCoolingDeviceInstance().setThrottleMaskList(tempList);
                    }
                    break;
                case ENUM_DETHROTTLEMASK:
                    if (mZone == null || mZone.getLastCoolingDeviceInstance() ==  null) {
                        return false;
                    } else {
                        // Dethrottling at CRITICAL state (last state) is dontcare condition
                        tempList.add(0);
                        mZone.getLastCoolingDeviceInstance().setDeThrottleMaskList(tempList);
                    }
                    break;
                default:
                    return false;
            }
            return true;
        }
        boolean processStartElement(String name) {
            if (name == null)
                return false;
            boolean ret = true;
            try {
                if (name.equalsIgnoreCase(CDEVINFO)) {
                    if (mDevice == null)
                        mDevice = new ThermalCoolingDevice();
                } else if (name.equalsIgnoreCase(ZONETHROTINFO)) {
                    if (mZone == null) {
                        mZone = new ThermalManager.ZoneCoolerBindingInfo();
                    }
                    if (mZoneCoolerBindMap == null) {
                        mZoneCoolerBindMap = new Hashtable<Integer,
                                ThermalManager.ZoneCoolerBindingInfo>();
                    }
                } else if (name.equalsIgnoreCase(PROFILE)) {
                    mNumProfiles++;
                    if (mZoneCoolerBindMap == null) {
                        mZoneCoolerBindMap = new Hashtable<Integer,
                                ThermalManager.ZoneCoolerBindingInfo>();
                    }
                } else if (name.equalsIgnoreCase(COOLINGDEVICEINFO) && mZone != null) {
                    if (mZone.getCoolingDeviceInfoList() == null) {
                        mZone.initializeCoolingDeviceInfoList();
                    }
                    mZone.createNewCoolingDeviceInstance();
                } else {
                    // Retrieve zone and cooling device mapping
                    if (name.equalsIgnoreCase("ZoneID") && mZone != null) {
                        mZone.setZoneID(Integer.parseInt(mParser.nextText()));
                    } else if (name.equalsIgnoreCase("CriticalShutDown") && mZone != null) {
                        mZone.setCriticalActionShutdown(Integer.parseInt(mParser.nextText()));
                    } else if (name.equalsIgnoreCase(THROTTLEMASK) && mZone != null) {
                        mTempMaskList = new ArrayList<Integer>();
                    } else if (name.equalsIgnoreCase(DETHROTTLEMASK) && mZone != null) {
                        mTempMaskList = new ArrayList<Integer>();
                    } else if (name.equalsIgnoreCase("CoolingDevId") && mZone != null) {
                        mZone.getLastCoolingDeviceInstance().setCoolingDeviceId(
                                Integer.parseInt(mParser.nextText()));
                    } else if (name.equalsIgnoreCase(COOLINGDEVICESTATES) && mZone != null) {
                        // Increase cooling device states by 1, required for CRITICAL state
                        mZone.getLastCoolingDeviceInstance().setCoolingDeviceStates(
                                Integer.parseInt(mParser.nextText()) + 1);
                    }
                    // Retrieve cooling device information
                    if (name.equalsIgnoreCase("CDeviceName") && mDevice != null) {
                        mDevice.setDeviceName(mParser.nextText());
                    } else if (name.equalsIgnoreCase("CDeviceID") && mDevice != null) {
                        mDevice.setDeviceId(Integer.parseInt(mParser.nextText()));
                    } else if (name.equalsIgnoreCase("CDeviceClassPath") && mDevice != null) {
                        mDevice.setClassPath(mParser.nextText());
                    } else if (name.equalsIgnoreCase("CDeviceThrottlePath") && mDevice != null) {
                        mDevice.setThrottlePath(mParser.nextText());
                    } else if (name.equalsIgnoreCase("Name")) {
                        mCurProfileName = mParser.nextText();
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
            if (name == null)
                return;
            if (name.equalsIgnoreCase(CDEVINFO) && mDevice != null) {
                // if cooling dev suports less then DEFAULT throttle values donot add to map.
                if (mDevice.getNumThrottleValues() < ThermalManager.DEFAULT_NUM_THROTTLE_VALUES) {
                    Log.i(TAG, "cooling dev:" + mDevice.getDeviceName()
                            + " deactivated! throttle values < "
                            + ThermalManager.DEFAULT_NUM_THROTTLE_VALUES);
                    mDevice = null;
                    return;
                }
                if (mDevice.getThrottlePath().equals("auto")) {
                    mDevice.setThrottlePath("auto");
                }
                if (loadCoolingDevice(mDevice)) {
                    ThermalManager.sCDevMap.put(mDevice.getDeviceId(), mDevice);
                }
                mDevice = null;
            } else if (name.equalsIgnoreCase(ZONETHROTINFO) && mZone != null) {
                mZone.printAttributes();
                if (mZoneCoolerBindMap != null) {
                    mZoneCoolerBindMap.put(mZone.getZoneID(), mZone);
                }
                mZone = null;
            } else if (name.equalsIgnoreCase(PROFILE)) {
                if (mZoneCoolerBindMap != null) {
                    ThermalManager.sProfileBindMap.put(mCurProfileName, mZoneCoolerBindMap);
                    mZoneCoolerBindMap = new Hashtable<Integer,
                            ThermalManager.ZoneCoolerBindingInfo>();
                }
            } else if (name.equalsIgnoreCase(THERMAL_THROTTLE_CONFIG)) {
                Log.i(TAG, "Parsing Finished..");
                // This indicates we have not seen any <Profile> tag.
                // Consider it as if we have only one 'Default' Profile.
                if (mNumProfiles == 0 && mZoneCoolerBindMap != null) {
                    ThermalManager.sProfileBindMap.put(mCurProfileName, mZoneCoolerBindMap);
                }
                done = true;
            } else if (name.equalsIgnoreCase(COOLINGDEVICEINFO) && mZone != null) {
                ThermalManager.ZoneCoolerBindingInfo.CoolingDeviceInfo cDevInfo;
                cDevInfo = mZone.getLastCoolingDeviceInstance();
                if (cDevInfo != null) {
                    ThermalCoolingDevice cDev = ThermalManager.sCDevMap
                            .get(cDevInfo.getCoolingDeviceId());
                    if (cDev == null) return;
                    int cds = cDevInfo.getCoolingDeviceStates();
                    // check the CDS against the number of throttle values exposed.
                    // If exceeds, cap it.
                    if (cds > cDev.getNumThrottleValues()) {
                        cDevInfo.setCoolingDeviceStates(cDev.getNumThrottleValues());
                        Log.i(TAG, "capping cdevid: " + cDevInfo.getCoolingDeviceId()
                                + " to " + cDev.getNumThrottleValues() + " states");
                    }
                    if (cDevInfo.checkMaskList(cDev.getNumThrottleValues())) {
                        // add only active cooling devices to list
                        mZone.addCoolingDeviceToList(cDevInfo);
                    }
                }
            }
        }
    }

    private void configureDynamicTurbo() {
        // Disable Dynamic Turbo based on the system property
        int indx = ThermalUtils.getCoolingDeviceIndexContains("SoC");
        if (indx != -1 && !ThermalManager.sIsDynamicTurboEnabled) {
            String path = ThermalManager.sCoolingDeviceBasePath + indx
                    + ThermalManager.sCoolingDeviceState;
            ThermalUtils.writeSysfs(path, ThermalManager.DISABLE_DYNAMIC_TURBO);
        }
    }

    public boolean init(Context context) {
        Log.i(TAG, "Thermal Cooling manager init() called");

        mContext = context;
        ThermalParser parser;
        if (!ThermalManager.sIsOverlays) {
            parser = new ThermalParser(ThermalManager.sThrottleFilePath);
        } else {
            parser = new ThermalParser();
        }

        if (parser == null || !parser.parse()) {
            Log.i(TAG, "thermal_throttle_config.xml parsing failed");
            return false;
        }

        // Set this sZoneCoolerBindMap to the DefaultProfile Map
        ThermalManager.setCurBindMap(ThermalManager.DEFAULT_PROFILE_NAME);

        // Register for thermal zone state changed notifications
        IntentFilter filter = new IntentFilter();
        filter.addAction(ThermalManager.ACTION_THERMAL_ZONE_STATE_CHANGED);
        mContext.registerReceiver(mThermalIntentReceiver, filter);

        configureDynamicTurbo();
        return true;
    }

    private final class ProfileChangeReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (action.equals(ThermalManager.ACTION_CHANGE_THERMAL_PROFILE)) {
                String profName = intent.getStringExtra(ThermalManager.EXTRA_PROFILE);
                if (profName != null) {
                    ThermalManager.changeThermalProfile(profName);
                }
            }
        }
    }

    private void incrementCrticalZoneCount() {
        synchronized(sCriticalZonesCountLock) {
            mCriticalZonesCount++;
        }
    }

    private final class ThermalZoneReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            String zoneName = intent.getStringExtra(ThermalManager.EXTRA_NAME);
            String profName = intent.getStringExtra(ThermalManager.EXTRA_PROFILE);
            int thermZone = intent.getIntExtra(ThermalManager.EXTRA_ZONE, -1);
            int thermState = intent.getIntExtra(ThermalManager.EXTRA_STATE, 0);
            int thermEvent = intent.getIntExtra(ThermalManager.EXTRA_EVENT, 0);
            int zoneTemp = intent.getIntExtra(ThermalManager.EXTRA_TEMP, 0);

            // Assume 'Default' profile if there is no profile parameter
            // as part of the intent.
            if (profName == null) {
                profName = ThermalManager.DEFAULT_PROFILE_NAME;
            }

            Log.i(TAG, "Received THERMAL INTENT:(ProfileName, ZoneName, State, EventType, Temp):"
                    + "(" + profName + ", " + zoneName + ", " + thermState + ", "
                    + ThermalZone.getEventTypeAsString(thermEvent) + ", " + zoneTemp + ")");

            Hashtable<Integer, ThermalManager.ZoneCoolerBindingInfo> mBindMap =
                    ThermalManager.getBindMap(profName);
            if (mBindMap == null) {
                Log.i(TAG, "mBindMap null inside ThermalZoneReceiver");
                return;
            }

            ThermalManager.ZoneCoolerBindingInfo zoneCoolerBindInfo = mBindMap.get(thermZone);
            if (zoneCoolerBindInfo == null) {
                Log.i(TAG, "zoneCoolerBindInfo null for zoneID" + thermZone);
                return;
            }

            boolean flag = zoneCoolerBindInfo.getCriticalActionShutdown() == 1;
            int lastState = zoneCoolerBindInfo.getLastState();
            if (thermState < lastState) {
                ThermalManager.updateZoneCriticalPendingMap(thermZone,
                        ThermalManager.CRITICAL_FALSE);
            } else if (thermState == lastState && flag) {
                /* no telephony support, so (!isEmergencyCallOnGoing) is true */
                if (true) {
                    doShutdown();
                } else {
                    // increment the count of zones in critical state pending on shutdown
                    ThermalManager.updateZoneCriticalPendingMap(thermZone,
                            ThermalManager.CRITICAL_TRUE);
                }
            }

            /* if THERMALOFF is the zone state, it is guaranteed that the zone has transitioned
            from a higher state, due to a low event, to THERMALOFF.Hence take de-throttling action
            corresponding to NORMAL */
            if (thermState == ThermalManager.THERMAL_STATE_OFF) {
                thermState = ThermalManager.THERMAL_STATE_NORMAL;
            }
            handleThermalEvent(thermZone, thermEvent, thermState, zoneCoolerBindInfo);
        }
    }

    private boolean loadCoolingDevice(ThermalCoolingDevice device) {
        Class cls;
        Method throttleMethod;
        String classPath = device.getClassPath();

        if (classPath == null) {
            Log.i(TAG, "ClassPath not found");
            return false;
        }

        if (classPath.equalsIgnoreCase("none") || classPath.equalsIgnoreCase("auto")
                || classPath.equalsIgnoreCase("AppAgent")) {
            Log.i(TAG, "ClassPath: none/auto/AppAgent");
            return true;
        }

        /* Load the cooling device class */
        try {
            cls = Class.forName(classPath);
            device.setDeviceClass(cls);
        } catch (Throwable e) {
            Log.i(TAG, "Unable to load class " + classPath);
            return false;
        }

        /* Initialize the cooling device class */
        try {
            Class partypes[] = new Class[3];
            partypes[0] = Context.class;
            partypes[1] = String.class;
            partypes[2] = ArrayList.class;
            Method init = cls.getMethod("init", partypes);
            Object arglist[] = new Object[3];
            arglist[0] = mContext;
            arglist[1] = device.getThrottlePath();
            arglist[2] = device.getThrottleValuesList();
            init.invoke(cls, arglist);
        } catch (NoSuchMethodException e) {
            Log.i(TAG, "NoSuchMethodException caught in device class init: " + classPath);
        } catch (SecurityException e) {
            Log.i(TAG, "SecurityException caught in device class init: " + classPath);
        } catch (IllegalAccessException e) {
            Log.i(TAG, "IllegalAccessException caught in device class init: " + classPath);
        } catch (IllegalArgumentException e) {
            Log.i(TAG, "IllegalArgumentException caught in device class init: " + classPath);
        } catch (ExceptionInInitializerError e) {
            Log.i(TAG, "ExceptionInInitializerError caught in device class init: " + classPath);
        } catch (InvocationTargetException e) {
            Log.i(TAG, "InvocationTargetException caught in device class init: " + classPath);
        }

        /* Get the throttleDevice method from cooling device class */
        try {
            Class partypes[] = new Class[1];
            partypes[0] = Integer.TYPE;
            throttleMethod = cls.getMethod("throttleDevice", partypes);
            device.setThrottleMethod(throttleMethod);
        } catch (NoSuchMethodException e) {
            Log.i(TAG, "NoSuchMethodException caught initializing throttle function");
        } catch (SecurityException e) {
            Log.i(TAG, "SecurityException caught initializing throttle function");
        }

        return true;
    }


    public void doShutdown() {
        ThermalUtils.writeSysfs(THERMAL_SHUTDOWN_NOTIFY_PATH, 1);
        /* We must avoid reboot after shutdown. */
        SystemProperties.set("sys.property_forcedshutdown", "1");
        Intent criticalIntent = new Intent(Intent.ACTION_REQUEST_SHUTDOWN);
        criticalIntent.putExtra(Intent.EXTRA_KEY_CONFIRM, false);
        criticalIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        Log.i(TAG, "Thermal Service initiating shutdown");
        mContext.startActivity(criticalIntent);
    }

    public void registerProfChangeListener() {
        IntentFilter profChangeIntentFilter = new IntentFilter();
        profChangeIntentFilter.addAction(ThermalManager.ACTION_CHANGE_THERMAL_PROFILE);
        // TODO: add some permission (BRICK ??) to protect it from third party apps
        mContext.registerReceiver(mProfChangeReceiver, profChangeIntentFilter);
        mProfChangeListenerInitialized = true;
    }

    /* Method to handle the thermal event based on HIGH or LOW event */
    private void handleThermalEvent(int zoneId, int eventType, int thermalState,
            ThermalManager.ZoneCoolerBindingInfo zoneCoolerBindInfo) {
        ThermalCoolingDevice tDevice;
        int deviceId;
        int existingState, targetState;
        int currThrottleMask, currDethrottleMask;
        int index = 0;

        if (zoneCoolerBindInfo.getCoolingDeviceInfoList() == null)
            return;

        for (ThermalManager.ZoneCoolerBindingInfo.CoolingDeviceInfo CdeviceInfo :
                zoneCoolerBindInfo.getCoolingDeviceInfoList()) {
            int coolingDeviceState =  thermalState /
                    zoneCoolerBindInfo.getZoneToCoolDevBucketSizeIndex(index);
            // cap it
            coolingDeviceState = (coolingDeviceState > (CdeviceInfo.getCoolingDeviceStates() - 1))
                    ? CdeviceInfo.getCoolingDeviceStates() - 1 : coolingDeviceState;
            int finalThrottleState = coolingDeviceState *
                    zoneCoolerBindInfo.getCoolDevToThrottBucketSizeIndex(index);
            // cap it
            finalThrottleState = (finalThrottleState > (CdeviceInfo.getMaxThrottleStates() - 1))
                    ? CdeviceInfo.getMaxThrottleStates() - 1 : finalThrottleState;
            index++;
            if (ThermalManager.THERMAL_HIGH_EVENT == eventType) {
                ArrayList<Integer> throttleMaskList = CdeviceInfo.getThrottleMaskList();
                if (throttleMaskList == null) continue;
                // cap to avoid out of bound exception
                coolingDeviceState = (coolingDeviceState > throttleMaskList.size() - 1)
                        ? throttleMaskList.size() - 1 : coolingDeviceState;
                currThrottleMask = throttleMaskList.get(coolingDeviceState);
                deviceId = CdeviceInfo.getCoolingDeviceId();

                tDevice = ThermalManager.sCDevMap.get(deviceId);
                if (tDevice == null)
                    continue;

                if (currThrottleMask == ThermalManager.THROTTLE_MASK_ENABLE) {
                    existingState = tDevice.getThermalState();
                    tDevice.updateZoneState(zoneId, finalThrottleState);
                    targetState = tDevice.getThermalState();

                    /* Do not throttle if device is already in desired state.
                     * (We can save Sysfs write)
                     * */
                    if (existingState != targetState) throttleDevice(deviceId, targetState);

                } else {
                     // If throttle mask is not enabled, don't do anything here.
                }
            }

            if (ThermalManager.THERMAL_LOW_EVENT == eventType) {
                ArrayList<Integer> dethrottleMaskList = CdeviceInfo.getDeThrottleMaskList();
                if (dethrottleMaskList == null) continue;
                // cap to avoid out of bound exception
                coolingDeviceState = (coolingDeviceState > dethrottleMaskList.size() - 1)
                        ? dethrottleMaskList.size() - 1 : coolingDeviceState;
                currDethrottleMask = dethrottleMaskList.get(coolingDeviceState);
                deviceId = CdeviceInfo.getCoolingDeviceId();

                tDevice = ThermalManager.sCDevMap.get(deviceId);
                if (tDevice == null)
                    continue;

                existingState = tDevice.getThermalState();
                tDevice.updateZoneState(zoneId, finalThrottleState);
                targetState = tDevice.getThermalState();

                /* Do not dethrottle if device is already in desired state.
                 * (We can save Sysfs write) */
                if ((existingState != targetState) &&
                        (currDethrottleMask == ThermalManager.DETHROTTLE_MASK_ENABLE)) {
                    throttleDevice(deviceId, targetState);
                }
            }
        }

    }

    /*
     * defaultThrottleMethod is called for cooling devices for which an additional
     * plugin file is not provided. Since the throttle path and the throttle values
     * are known, we dont need an additional plugin to implement the policy. This info
     * is provided via thermal_throttle_config file. If for a cooling device,
     * Assumptions -
     * 1. If CDeviceClassPath is 'auto' this triggers a call to defaultThrottleMethod().
     * if a false throttle path is provided, the write fails and function exits gracefully
     * with a warning message.
     * 2. If 'auto' mode is used for CDeviceClassPath, and no throttle values are provided,
     * thermal state will be written.
     * 3. If CDeviceThrottlePath is 'auto', then throttle path will be constrcuted.
     * The Cooling device name should contain a subset string that matches the type for
     * /sys/class/thermal/cooling_deviceX/type inorder to find the right index X
     * 4. CDeviceThrottlePath is null no write operation will be done
     **/
    private void defaultThrottleMethod(ThermalCoolingDevice cdev, int level) {
        int finalValue;
        String throttlePath = null;

        if (cdev == null) return;

        if (level < cdev.getNumThrottleValues() - 1) {
            try {
                ArrayList<Integer> values = cdev.getThrottleValuesList();
                if (values == null || values.size() == 0) {
                    finalValue = level;
                } else {
                    finalValue =  values.get(level);
                }

                throttlePath = cdev.getThrottlePath();
                if (throttlePath == null) {
                    Log.w(TAG, "throttle path is null");
                    return;
                }

                if (!ThermalUtils.isFileExists(throttlePath)) {
                    Log.w(TAG, "invalid throttle path for cooling device:" + cdev.getDeviceName());
                    return;
                }

                if (ThermalUtils.writeSysfs(throttlePath, finalValue) == -1) {
                    Log.w(TAG, "write to sysfs failed");
                }
            } catch (IndexOutOfBoundsException e) {
                Log.w(TAG, "IndexOutOfBoundsException caught in defaultThrottleMethod()");
            }
        }
    }

    /* Method to throttle cooling device */
    private void throttleDevice(int coolingDevId, int throttleLevel) {
        /* Retrieve the cooling device based on ID */
        ThermalCoolingDevice dev = ThermalManager.sCDevMap.get(coolingDevId);
        if (dev != null) {
            if (dev.getClassPath() != null && dev.getClassPath().equalsIgnoreCase("auto")) {
                defaultThrottleMethod(dev, throttleLevel);
            } else {
                Class c = dev.getDeviceClass();
                Method throt = dev.getThrottleMethod();
                if (throt == null)
                    return;
                Object arglist[] = new Object[1];
                arglist[0] = new Integer(throttleLevel);

                // Invoke the throttle method passing the throttle level as parameter
                try {
                    throt.invoke(c, arglist);
                } catch (IllegalAccessException e) {
                    Log.i(TAG, "IllegalAccessException caught throttleDevice() ");
                } catch (IllegalArgumentException e) {
                    Log.i(TAG, "IllegalArgumentException caught throttleDevice() ");
                } catch (ExceptionInInitializerError e) {
                    Log.i(TAG, "ExceptionInInitializerError caught throttleDevice() ");
                } catch (SecurityException e) {
                    Log.i(TAG, "SecurityException caught throttleDevice() ");
                } catch (InvocationTargetException e) {
                    Log.i(TAG, "InvocationTargetException caught throttleDevice() ");
                }
            }
        } else {
            Log.i(TAG, "throttleDevice: Unable to retrieve cooling device " + coolingDevId);
        }
    }

    public void unregisterReceivers() {
        if (mContext != null) {
            mContext.unregisterReceiver(mThermalIntentReceiver);
            // During Thermal Service init, when parsing fails, we
            // unregister all receivers here. mProfChangeReceiver
            // might not have been initialized at that time because
            // we initialize this only after starting the Default profile.
            if (mProfChangeListenerInitialized) {
                mContext.unregisterReceiver(mProfChangeReceiver);
            }
        }
    }
}
