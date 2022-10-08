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

import android.content.Context;
import android.content.Intent;
import android.os.SystemProperties;
import android.os.UserHandle;
import android.util.Log;

import java.io.File;
import java.io.IOException;
import java.lang.NumberFormatException;
import java.lang.StringBuilder;
import java.util.ArrayList;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Iterator;
import java.util.Map;
import java.util.NoSuchElementException;
/**
 * The ThermalManager class contains data structures that are common to both
 * Thermal Sensor/Zone and Cooling device parts.
 *
 * @hide
 */
public class ThermalManager {
    private static final String TAG = "ThermalManager";
    private static Context sContext;
    private static String sVersion;
    private static String sCurProfileName;
    private static String sProfileNameList;
    private static int sProfileCount;
    private static final String ITUX_VERSION_PROPERTY = "ro.thermal.ituxversion";
    /* Parameter needed for reading configuration files */
    public static final String SENSOR_FILE_NAME = "thermal_sensor_config.xml";
    public static final String THROTTLE_FILE_NAME = "thermal_throttle_config.xml";
    public static final String DEFAULT_DIR_PATH = "/system/etc/";
    public static final String DEBUG_DIR_PATH = "/data/";
    public static String sSensorFilePath;
    public static String sThrottleFilePath;
    /* *XmlId's are assigned if config files are choosen from overlays */
    public static int sSensorFileXmlId = -1;
    public static int sThrottleFileXmlId = -1;
    /* Set to true if config are available in DEFAULT or DEBUG path */
    public static boolean sIsConfigFiles = false;
    /* Whether we are using the config files from overlays directory or from /etc/ */
    public static boolean sIsOverlays = false;
    /* Parameters required for MaxTrip data */
    public static final String TJMAX_PATH = "/sys/devices/platform/coretemp.0/temp2_crit";
    public static final int sDefaultTjMax = 90000;
    public static int sTjMaxTemp;
    public static final int sMaxSkinTrip = 150000;

    public static String sUEventDevPath = "DEVPATH=/devices/virtual/thermal/thermal_zone";
    /**
     * Thermal Zone State Changed Action: This is broadcast when the state of a
     * thermal zone changes.
     */
    public static final String ACTION_THERMAL_ZONE_STATE_CHANGED =
            "com.intel.thermal.action.THERMAL_ZONE_STATE_CHANGED";

    public static PlatformInfo sPlatformInfo;
    public static ThermalCooling sCoolingManager;
    /* List of Thermal zones for current profile. Access protected by 'sProfileSwitchLock' */
    private static ArrayList<ThermalZone> sThermalZonesList;

    /* Hashtable of (ProfileName and ListOfZonesUnderThisProfile) */
    public static Hashtable<String, ArrayList<ThermalZone>> sProfileZoneMap =
            new Hashtable<String, ArrayList<ThermalZone>>();

    /**
     * This holds the map for the current profile. Access protected by 'sProfileSwitchLock'.
     * Should be initialized for every profile change.
     */
    private static Hashtable<Integer, ZoneCoolerBindingInfo> sZoneCoolerBindMap =
            new Hashtable<Integer, ZoneCoolerBindingInfo>();

    /* Hashtable of (ProfileName and Hashtable(zoneID, ZoneCoolerBindingInfo) object */
    public static Hashtable<String, Hashtable<Integer, ZoneCoolerBindingInfo>> sProfileBindMap =
            new Hashtable<String, Hashtable<Integer, ZoneCoolerBindingInfo>>();

    /* Hashtable of (Cooling Device ID and ThermalCoolingDevice object) */
    public static Hashtable<Integer, ThermalCoolingDevice> sCDevMap =
            new Hashtable<Integer, ThermalCoolingDevice>();

    /* Hashtable of sensor name and sensor object */
    public static Hashtable<String, ThermalSensor> sSensorMap =
            new Hashtable<String, ThermalSensor>();

    public static final int CRITICAL_TRUE = 1;
    public static final int CRITICAL_FALSE = 0;
    /* sZoneCriticalPendingMap stores info whether a zone is in critical state and platform
     * shutdown has not yet occured due to some scenario like ongoing emergency call
     **/
    public static Hashtable<Integer, Integer> sZoneCriticalPendingMap = null;
    /* this lock is to access sZoneCriticalPendingMap synchronously */
    private static final Object sCriticalPendingLock = new Object();
    /* this count keeps track of number of zones in pending critical state.When
     * sZoneCriticalPendingMap is updated, the count is either incremented or
     * decremented depending on whether criical pending flag for a zone is true/
     * false. By keeping a count we can avoid scanning through the entire map to
     * see if there is a pending critical shutdown
     **/
    private static int sCriticalZonesCount = 0;

    /* Blocking queue to hold thermal events from thermal zones */
    private static final int EVENT_QUEUE_SIZE = 10;

    public static BlockingQueue<ThermalEvent> sEventQueue = new ArrayBlockingQueue<ThermalEvent>(EVENT_QUEUE_SIZE);
    /* this lock is to handle uevent callbacks synchronously */
    private static final Object sLock = new Object();

    /**
     * Extra for {@link ACTION_THERMAL_ZONE_STATE_CHANGED}:
     * integer containing the thermal zone.
     */
    public static final String EXTRA_ZONE = "zone";

    /**
     * Extra for {@link ACTION_THERMAL_ZONE_STATE_CHANGED}:
     * integer containing the thermal state of the zone.
     */
    public static final String EXTRA_STATE = "state";

    /**
     * Extra for {@link ACTION_THERMAL_ZONE_STATE_CHANGED}:
     * integer containing the thermal event type for the zone.
     */
    public static final String EXTRA_EVENT = "event";

    /**
     * Extra for {@link ACTION_THERMAL_ZONE_STATE_CHANGED}:
     * integer containing the temperature of the zone.
     */
    public static final String EXTRA_TEMP = "temp";
    public static final String ACTION_CHANGE_THERMAL_PROFILE =
            "android.intent.action.CHANGE_THERMAL_PROFILE";
    /**
     * Extra for {@link ACTION_THERMAL_ZONE_STATE_CHANGED}:
     * String containing the name of the zone.
     */
    public static final String EXTRA_NAME = "name";
    public static final String EXTRA_PROFILE = "Profile";

    private static Intent sQueryProfileIntent;
    public static final String ACTION_QUERY_THERMAL_PROFILE =
            "com.intel.thermal.action.QUERY_THERMAL_PROFILE";
    public static final String ACTION_KILL = "kill";

    /**
     * Integer containing the number of thermal profiles.
     */
    public static final String EXTRA_NUM_PROFILE = "NumProfiles";
    /**
     * Space separated string containing list of thermal profile names.
     */
    public static final String EXTRA_PROFILE_LIST = "ProfileList";
    /**
     * String containing current thermal profile name.
     */
    public static final String EXTRA_CUR_PROFILE = "CurProfile";

    /* values for "STATE" field in the THERMAL_STATE_CHANGED Intent */
    public static final int THERMAL_STATE_OFF = -1;

    public static final int THERMAL_STATE_NORMAL = 0;

    public static final int THERMAL_STATE_WARNING = 1;

    public static final int THERMAL_STATE_ALERT = 2;

    public static final int THERMAL_STATE_CRITICAL = 3;

    public static final int DEFAULT_NUM_THROTTLE_VALUES = 4;

    // 5 including TOFF and TCRITICAL
    public static final int DEFAULT_NUM_ZONE_STATES = 5;

    public static final String STATE_NAMES[] = {
            "OFF", "NORMAL", "WARNING", "ALERT", "CRITICAL"
    };

    /* values of the "EVENT" field in the THERMAL_STATE_CHANGED intent */
    /* Indicates type of event */
    public static final int THERMAL_LOW_EVENT = 0;

    public static final int THERMAL_HIGH_EVENT = 1;

    public static final int THERMAL_EMUL_TEMP_EVENT = 2;

    public static final int INVALID_TEMP = 0xDEADBEEF;

    /* Absolute zero in millidegree C */
    public static final int ABS_ZERO = -273000;

    /* base sysfs path for sensors */
    public static final String sSysfsSensorBasePath = "/sys/class/thermal/thermal_zone";

    public static final String sSysfsSensorHighTempPath = "trip_point_1_temp";

    public static final String sSysfsSensorLowTempPath = "trip_point_0_temp";

    public static final String sCoolingDeviceBasePath = "/sys/class/thermal/cooling_device";

    public static final String sCoolingDeviceState = "/cur_state";

    public static final int THROTTLE_MASK_ENABLE = 1;

    public static final int DETHROTTLE_MASK_ENABLE = 1;

    /**
     * Magic number (agreed upon between the Thermal driver and the Thermal Service)
     * symbolising Dynamic Turbo OFF
     */
    public static final int DISABLE_DYNAMIC_TURBO = 0xB0FF;

    public static boolean sIsDynamicTurboEnabled = false;

    /* thermal notifier system properties for shutdown action */
    public static boolean sShutdownTone = false;

    public static boolean sShutdownToast = false;

    public static boolean sShutdownVibra = false;

    /* Name of default Thermal Profile */
    public static final String DEFAULT_PROFILE_NAME = "Default";

    /* Lock protecting profile-switch */
    private static final Object sProfileSwitchLock = new Object();

    /**
     * This class stores the zone throttle info. It contains the zoneID,
     * CriticalShutdown flag and CoolingDeviceInfo arraylist.
     */
    public static class ZoneCoolerBindingInfo {
        private int mZoneID;
        // max states includes TOFF also.
        // if user provides k threshold values in XML.
        // mMaxStates = k + 1(for critical) + 1(for TOFF)
        // this is same as the max states stored in corresponding zone object
        protected int mMaxStates;
        private int mIsCriticalActionShutdown;

        /* cooler ID mask, 1 - throttle device, 0- no action, -1- dont care */
        private ArrayList<CoolingDeviceInfo> mCoolingDeviceInfoList = null;

        // ManyToOneMapping: ZoneStates >= CoolingDeviceStates
        private ArrayList<Integer> mZoneToCoolDevBucketSize = null;

        // OneToOneMapping: CoolingDeviceStates >= ThrottleValues
        private ArrayList<Integer> mCoolDevToThrottBucketSize = null;

        private CoolingDeviceInfo lastCoolingDevInfoInstance = null;

        public ZoneCoolerBindingInfo() {
            mZoneToCoolDevBucketSize = new ArrayList<Integer>();
            mCoolDevToThrottBucketSize = new ArrayList<Integer>();
        }

        public int getLastState() {
            // mMaxStates = k + 1(for critical) + 1(for TOFF)
            return mMaxStates - 2;
        }

        public void setMaxStates(int state) {
            mMaxStates = state;
        }

        public int getMaxStates() {
            return mMaxStates;
        }

        public void setZoneToCoolDevBucketSize() {
            int size = 1;
            int zoneStates = getMaxStates();
            for (CoolingDeviceInfo coolDev : mCoolingDeviceInfoList) {
                size = (zoneStates - 1) / coolDev.getCoolingDeviceStates();
                mZoneToCoolDevBucketSize.add(size == 0 ? 1 : size);
            }
        }

        public int getZoneToCoolDevBucketSizeIndex(int index) {
            if (mZoneToCoolDevBucketSize.size() > index)
                return mZoneToCoolDevBucketSize.get(index);

            return 1;
        }

        public int getCoolDevToThrottBucketSizeIndex(int index) {
            if (mZoneToCoolDevBucketSize.size() > index)
                return mCoolDevToThrottBucketSize.get(index);

            return 1;
        }

        public void setCoolDevToThrottBucketSize() {
            int size = 1;
            for (CoolingDeviceInfo coolDev : mCoolingDeviceInfoList) {
                size = coolDev.getMaxThrottleStates() / coolDev.getCoolingDeviceStates();
                mCoolDevToThrottBucketSize.add(size == 0 ? 1 : size);
            }
        }

        public void printAttributes() {
            if (mCoolingDeviceInfoList == null) return;
            StringBuilder s = new StringBuilder();
            for (CoolingDeviceInfo c : mCoolingDeviceInfoList) {
                if (c != null) {
                    s.append(c.getCoolingDeviceId());
                    s.append(",");
                }
            }
            Log.i(TAG, "zone id:" + mZoneID + " coolingDevID  mapped:" + s.toString());
        }

        public void printMappedAttributes() {
            if (mZoneToCoolDevBucketSize == null || mCoolDevToThrottBucketSize == null) return;
            StringBuilder s = new StringBuilder();
            for (int bs : mZoneToCoolDevBucketSize) {
                s.append(bs);
                s.append(",");
            }
            Log.i(TAG, "zone id:" + mZoneID + " ZoneToCoolDevBucketSize:" + s.toString());
            // clear the string
            s.delete(0,s.length());
            for (int bs : mCoolDevToThrottBucketSize) {
                s.append(bs);
                s.append(",");
            }
            Log.i(TAG, "zone id:" + mZoneID + " CoolDevToThrottBucketSize:" + s.toString());
        }

        public class CoolingDeviceInfo {
            private int mCDeviceID;

            // mCoolingDeviceState is number of device states exposed under a zone.
            // this must be less than or equal to its total number of throttle values
            private int mCoolingDeviceStates = DEFAULT_NUM_THROTTLE_VALUES;

            // store a copy here for fast lookup during throttling/dethrottling
            private int mMaxThrottleStates = 0;
            private ArrayList<Integer> mDeviceThrottleMask = null;

            private ArrayList<Integer> mDeviceDethrottleMask = null;

            public CoolingDeviceInfo() {
            }

            public int getMaxThrottleStates() {
                return mMaxThrottleStates;
            }

            public boolean checkMaskList(int throttleStates) {
                boolean ret = true;
                // if the list is empty this mean, THROTTLE MASK and/or
                // DETHTOTTLE mask was not provided. Initialize default mask.
                if (mDeviceThrottleMask ==  null) {
                    mDeviceThrottleMask = new ArrayList<Integer>();
                    for (int i = 0; i < mCoolingDeviceStates; i++) {
                        mDeviceThrottleMask.add(THROTTLE_MASK_ENABLE);
                    }
                } else if (mDeviceThrottleMask.size() != mCoolingDeviceStates) {
                    Log.i(TAG, "cdevid:" + mCDeviceID
                            + " has mismatch in Cooling device state and mask array!deactivate!");
                    ret = false;
                }

                if (mDeviceDethrottleMask ==  null) {
                    mDeviceDethrottleMask = new ArrayList<Integer>();
                    for (int i = 0; i < mCoolingDeviceStates; i++) {
                        mDeviceDethrottleMask.add(DETHROTTLE_MASK_ENABLE);
                    }
                } else if (mDeviceDethrottleMask.size() != mCoolingDeviceStates) {
                    Log.i(TAG, "cdevid:" + mCDeviceID
                            + " has mismatch in Cooling device state and mask array!deactivate!");
                    ret = false;
                }
                if (ret) {
                    mMaxThrottleStates = throttleStates;
                }
                return ret;
            }

            public int getCoolingDeviceId() {
                return mCDeviceID;
            }

            public void setCoolingDeviceId(int deviceID) {
                mCDeviceID = deviceID;
            }

            public int getCoolingDeviceStates() {
                return mCoolingDeviceStates;
            }

            public void setCoolingDeviceStates(int num) {
                mCoolingDeviceStates = num;
            }

            public ArrayList<Integer> getThrottleMaskList() {
                return mDeviceThrottleMask;
            }

            public ArrayList<Integer> getDeThrottleMaskList() {
                return mDeviceDethrottleMask;
            }

            public void setThrottleMaskList(ArrayList<Integer> list) {
                this.mDeviceThrottleMask = list;
            }

            public void setDeThrottleMaskList(ArrayList<Integer> list) {
                this.mDeviceDethrottleMask = list;
            }

        }

        public ArrayList<CoolingDeviceInfo> getCoolingDeviceInfoList() {
            return mCoolingDeviceInfoList;
        }

        public void createNewCoolingDeviceInstance() {
            lastCoolingDevInfoInstance = new CoolingDeviceInfo();
        }

        public CoolingDeviceInfo getLastCoolingDeviceInstance() {
            return lastCoolingDevInfoInstance;
        }

        public void setZoneID(int zoneID) {
            mZoneID = zoneID;
        }

        public int getZoneID() {
            return mZoneID;
        }

        public void setCriticalActionShutdown(int val) {
            mIsCriticalActionShutdown = val;
        }

        public int getCriticalActionShutdown() {
            return mIsCriticalActionShutdown;
        }

        public void setCoolingDeviceInfoList(ArrayList<CoolingDeviceInfo> devinfoList) {
            mCoolingDeviceInfoList = devinfoList;
        }

        public void initializeCoolingDeviceInfoList() {
            mCoolingDeviceInfoList = new ArrayList<CoolingDeviceInfo>();
        }

        public void addCoolingDeviceToList(CoolingDeviceInfo CdeviceInfo) {
            mCoolingDeviceInfoList.add(CdeviceInfo);
        }
    }

    /* platform information */
    public static class PlatformInfo {
       public int mMaxThermalStates;

       public int getMaxThermalStates() {
            return mMaxThermalStates;
       }

       public void printAttrs() {
           Log.i(TAG, Integer.toString(mMaxThermalStates));
       }
       public PlatformInfo() {}
    }

    /* methods */
    public ThermalManager() {
        // empty constructor
    }

    public static void setContext(Context context) {
        sContext = context;
    }

    public static String getVersion() {
        return sVersion;
    }

    public static void loadiTUXVersion() {
        sVersion = SystemProperties.get(ITUX_VERSION_PROPERTY, "none");
        if (sVersion.equalsIgnoreCase("none")) {
            Log.i(TAG, "iTUX Version not found!");
        } else {
            Log.i(TAG, "iTUX Version:" + sVersion);
        }
    }

    public static void addThermalEvent(ThermalEvent event) {
        try {
            ThermalManager.sEventQueue.put(event);
        } catch (InterruptedException ex) {
            Log.i(TAG, "caught InterruptedException in posting to event queue");
        }
    }

    public static void setCurBindMap(String profName) {
        synchronized (sProfileSwitchLock) {
            sZoneCoolerBindMap = sProfileBindMap.get(profName);
        }
    }

    public static Hashtable<Integer, ZoneCoolerBindingInfo> getCurBindMap() {
        synchronized (sProfileSwitchLock) {
            return sZoneCoolerBindMap;
        }
    }

    public static Hashtable<Integer, ZoneCoolerBindingInfo> getBindMap(String profName) {
        return sProfileBindMap.get(profName);
    }

    private static void setCurProfileName(String profName) {
        sCurProfileName = profName;
    }

    public static String getCurProfileName() {
        return sCurProfileName;
    }

    private static boolean isProfileExists(String profName) {
        if (sProfileZoneMap.get(profName) == null || sProfileBindMap.get(profName) == null) {
            return false;
        }
        return true;
    }

    private static void startNewProfile(String profName) {
        sThermalZonesList = sProfileZoneMap.get(profName);
        sZoneCoolerBindMap = sProfileBindMap.get(profName);
        if (sThermalZonesList == null || sZoneCoolerBindMap == null) {
            Log.i(TAG, "Couldn't shift to profile:" + profName);
            return;
        }
        initializeZoneCriticalPendingMap();
        setCurProfileName(profName);
        int activeZones = startMonitoringZones();
        Log.i(TAG, activeZones + " zones found active in profile " + profName);
        // broadcast a sticky intent for the clients
        sendQueryProfileIntent();
    }

    public static void stopCurrentProfile() {
        for (ThermalZone zone : sThermalZonesList) {
            // Stop Polling threads
            zone.stopMonitoring();
            // Unregister UEvent/EmulTemp observers
            zone.unregisterReceiver();
            // Reset Parameters:
            // Zone State: Normal, Event Type: LOW, Temperature: Normal Threshold
            zone.setZoneState(0);
            zone.setEventType(ThermalManager.THERMAL_LOW_EVENT);
            zone.setZoneTemp(zone.getZoneTempThreshold(0));
            // Send ThermalIntent with above parameters
            // This will release all throttle controls this zone had.
            // Since we are in the middle of a profile switch(stop),
            // set the override parameter as true, so that this
            // event is actually queued for processing.
            // TODO: Find a way to take care of zones that are not
            // present in thermal_sensor_config.xml but present in
            // thermal_throttle_config.xml (usually from other components)
            zone.sendThermalEvent();
            // Reprogram the sensor thresholds if this zone supported interrupts
            // TODO: We are reprogramming the calibrated thresholds in case the
            // the sensor was using 'weights' and 'offset'. Hope this is fine.
            if (zone.isUEventSupported()) {
                zone.programThresholds((zone.getThermalSensorList()).get(0));
            }
        }
    }

    public static void startDefaultProfile() {
        if (isProfileExists(DEFAULT_PROFILE_NAME)) {
            startNewProfile(DEFAULT_PROFILE_NAME);
        }
        // register for Thermal Profile Change Intent only after
        // we have started the default profile
        sCoolingManager.registerProfChangeListener();
    }

    public static void changeThermalProfile(String newProfName) {
        synchronized (sProfileSwitchLock) {
            if (newProfName.equalsIgnoreCase(sCurProfileName)) {
                Log.i(TAG, "New Profile same as current profile. Profile change request Ignored");
                return;
            }
            if (!isProfileExists(newProfName)) {
                Log.i(TAG, "New Profile does not exist in xml. Profile change request Ignored");
                return;
            }
            Log.i(TAG, "ACTION_CHANGE_THERMAL_PROFILE received. New Profile: " + newProfName);

            stopCurrentProfile();
            startNewProfile(newProfName);
        }
    }

    public static void setBucketSizeForProfiles() {
        Iterator it = ThermalManager.sProfileZoneMap.entrySet().iterator();
        while (it.hasNext()) {
            Map.Entry entryProfZone = (Map.Entry) it.next();
            String keyProfile = (String) entryProfZone.getKey();
            sThermalZonesList = (ArrayList<ThermalZone>) entryProfZone.getValue();
            setCurBindMap(keyProfile);
            for (ThermalZone zone : sThermalZonesList) {
                if (sZoneCoolerBindMap == null) {
                    Log.e(TAG, "ZoneCoolerBindMap null while setBucketSizeForProfiles");
                    return;
                }
                ZoneCoolerBindingInfo bindInfo = sZoneCoolerBindMap.get(zone.getZoneId());
                if (bindInfo == null) {
                    Log.e(TAG, "CoolerBindingInfo for zoneid:" + zone.getZoneId() + "not mapped");
                    return;
                }
                bindInfo.setMaxStates(zone.getMaxStates());
                bindInfo.setZoneToCoolDevBucketSize();
                bindInfo.setCoolDevToThrottBucketSize();
                if (zone.isUEventSupported()) {
                    // calibration of thresholds based on weight, order
                    if (!zone.isMaxThreshExceed())
                        zone.calibrateThresholds();
                }
            }
        }
    }

    public static int startMonitoringZones() {
        int activeZonesCount = 0;
        for (ThermalZone zone : sThermalZonesList) {
            zone.computeZoneActiveStatus();
            if (zone.getZoneActiveStatus() == false) {
                Log.i(TAG, "deactivating inactive zone:" + zone.getZoneName());
                continue;
            }

            ZoneCoolerBindingInfo bindInfo = sZoneCoolerBindMap.get(zone.getZoneId());
            if (bindInfo != null) {
                // TODO: To be conditioned under debug
                bindInfo.printMappedAttributes();
            }
            if (zone.isUEventSupported()) {
                zone.registerUevent();
            } else {
                // start polling thread for each zone
                zone.startMonitoring();
            }
            zone.startEmulTempObserver();
            activeZonesCount++;
        }
        return activeZonesCount;
    }

    public static void readShutdownNotiferProperties() {
        try {
            if ("1".equals(SystemProperties.get("persist.thermal.shutdown.msg", "0"))) {
                sShutdownToast = true;
            }
            if ("1".equals(SystemProperties.get("persist.thermal.shutdown.tone", "0"))) {
                sShutdownTone = true;
            }
            if ("1".equals(SystemProperties.get("persist.thermal.shutdown.vibra", "0"))) {
                sShutdownVibra = true;
            }
        } catch (java.lang.IllegalArgumentException e) {
            Log.e(TAG, "exception caught in reading thermal system properties");
        }
    }

    private static void initializeZoneCriticalPendingMap() {
        sZoneCriticalPendingMap = new Hashtable<Integer, Integer>();
        if (sZoneCriticalPendingMap == null) return;
        Enumeration en;
        try {
            // look up for zone list is performed from sZoneCoolerBindMap instead of
            // sThermalZonesList since some non thermal zones may not have entry in
            // sThermalZonesList. This is because such zones only have entry in throttle
            // config file and not in sensor config files.
            // 'sZoneCoolerBindMap' is protected by caller here.
            en = sZoneCoolerBindMap.keys();
            while (en.hasMoreElements()) {
                int zone = (Integer) en.nextElement();
                sZoneCriticalPendingMap.put(zone, CRITICAL_FALSE);
            }
        } catch (NoSuchElementException e) {
            Log.i(TAG, "NoSuchElementException in InitializeZoneCriticalPendingMap()");
        }
    }

    /*
     * updateZoneCriticalPendingMap updates sZoneCriticalPendingMap synchronously.
     * sCriticalZonesCount is incremented iff old value in the map for the zone is
     * FALSE (ensures count is incremented only once for a zone) and decremented
     * iff oldval is TRUE (ensures no negative value for count)
     **/
    public static boolean updateZoneCriticalPendingMap(int zoneid, int flag) {
        synchronized (sCriticalPendingLock) {
            if (sZoneCriticalPendingMap == null) return false;
                Integer oldVal = sZoneCriticalPendingMap.get(zoneid);
                if (oldVal == null) return false;
                sZoneCriticalPendingMap.put(zoneid, flag);
                if (oldVal == CRITICAL_FALSE && flag == CRITICAL_TRUE) {
                   sCriticalZonesCount++;
                } else if (oldVal == CRITICAL_TRUE && flag == CRITICAL_FALSE) {
                   sCriticalZonesCount--;
                }
                return true;
        }
    }

    public static boolean checkShutdownCondition() {
        synchronized (sCriticalPendingLock) {
           return sCriticalZonesCount > 0;
        }
    }

    public static ThermalSensor getSensor(String sensorName) {
        if (sensorName == null || sSensorMap == null) return null;
        return sSensorMap.get(sensorName);
    }

    public static void buildProfileNameList() {
        int count = 0;
        StringBuilder s = new StringBuilder();
        Iterator it = sProfileZoneMap.entrySet().iterator();
        while (it.hasNext()) {
            Map.Entry entry = (Map.Entry) it.next();
            String key = (String) entry.getKey();
            // create list of only valid profiles
            if (isProfileExists(key)) {
                // build a space seperate list of string
                s.append(key);
                s.append(" ");
                count++;
            }
        }

        sProfileNameList = s.toString();
        sProfileCount = count;
        Log.i(TAG, "profile name list:" + sProfileNameList);
        Log.i(TAG, "profile count:" + sProfileCount);
    }

    public static void initializeStickyIntent() {
        sQueryProfileIntent = new Intent();
        sQueryProfileIntent.setAction(ACTION_QUERY_THERMAL_PROFILE);
    }

    private static void sendQueryProfileIntent() {
        if (sQueryProfileIntent != null && sContext != null) {
            sQueryProfileIntent.putExtra(ThermalManager.EXTRA_NUM_PROFILE, sProfileCount);
            sQueryProfileIntent.putExtra(ThermalManager.EXTRA_PROFILE_LIST, sProfileNameList);
            sQueryProfileIntent.putExtra(ThermalManager.EXTRA_CUR_PROFILE, sCurProfileName);
            sContext.sendStickyBroadcastAsUser(sQueryProfileIntent, UserHandle.ALL);
        }
    }

    public static void clearData() {
        sThermalZonesList.clear();
        // clearing hastables
        sProfileZoneMap.clear();
        sZoneCoolerBindMap.clear();
        sProfileBindMap.clear();
        sCDevMap.clear();
        sSensorMap.clear();
        sZoneCriticalPendingMap.clear();
    }
}
