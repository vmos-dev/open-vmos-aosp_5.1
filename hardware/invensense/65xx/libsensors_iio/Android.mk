# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# Modified 2011 by InvenSense, Inc

LOCAL_PATH := $(call my-dir)

#ifneq ($(TARGET_SIMULATOR),true)

# InvenSense fragment of the HAL
include $(CLEAR_VARS)

LOCAL_MODULE := libinvensense_hal
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_OWNER := invensense

LOCAL_CFLAGS := -DLOG_TAG=\"Sensors\"

# ANDROID version check
MAJOR_VERSION :=$(shell echo $(PLATFORM_VERSION) | cut -f1 -d.)
MINOR_VERSION :=$(shell echo $(PLATFORM_VERSION) | cut -f2 -d.)
VERSION_JB :=$(shell test $(MAJOR_VERSION) -gt 4 -o $(MAJOR_VERSION) -eq 4 -a $(MINOR_VERSION) -gt 0 && echo true)
#ANDROID version check END
VERSION_JB:=true
ifeq ($(VERSION_JB),true)
LOCAL_CFLAGS += -DANDROID_JELLYBEAN
endif

ifneq (,$(filter $(TARGET_BUILD_VARIANT),eng userdebug))
ifneq ($(COMPILE_INVENSENSE_COMPASS_CAL),0)
LOCAL_CFLAGS += -DINVENSENSE_COMPASS_CAL
endif
ifeq ($(COMPILE_THIRD_PARTY_ACCEL),1)
LOCAL_CFLAGS += -DTHIRD_PARTY_ACCEL
endif
else # release builds, default
LOCAL_CFLAGS += -DINVENSENSE_COMPASS_CAL
endif

LOCAL_SRC_FILES += SensorBase.cpp
LOCAL_SRC_FILES += MPLSensor.cpp
LOCAL_SRC_FILES += MPLSupport.cpp
LOCAL_SRC_FILES += InputEventReader.cpp
LOCAL_SRC_FILES += PressureSensor.IIO.secondary.cpp

ifneq (,$(filter $(TARGET_BUILD_VARIANT),eng userdebug))
ifeq ($(COMPILE_INVENSENSE_COMPASS_CAL),0)
LOCAL_SRC_FILES += AkmSensor.cpp
LOCAL_SRC_FILES += CompassSensor.AKM.cpp
else ifeq ($(COMPILE_INVENSENSE_SENSOR_ON_PRIMARY_BUS), 1)
LOCAL_SRC_FILES += CompassSensor.IIO.primary.cpp
LOCAL_CFLAGS += -DSENSOR_ON_PRIMARY_BUS
else
LOCAL_SRC_FILES += CompassSensor.IIO.9150.cpp
endif
else # release builds, default
LOCAL_SRC_FILES += CompassSensor.IIO.9150.cpp
endif #userdebug

LOCAL_C_INCLUDES += $(LOCAL_PATH)/software/core/mllite
LOCAL_C_INCLUDES += $(LOCAL_PATH)/software/core/mllite/linux
LOCAL_C_INCLUDES += $(LOCAL_PATH)/software/core/driver/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/software/core/driver/include/linux

LOCAL_SHARED_LIBRARIES := liblog
LOCAL_SHARED_LIBRARIES += libcutils
LOCAL_SHARED_LIBRARIES += libutils
LOCAL_SHARED_LIBRARIES += libdl
LOCAL_SHARED_LIBRARIES += libmllite

# Additions for SysPed
LOCAL_SHARED_LIBRARIES += libmplmpu
LOCAL_C_INCLUDES += $(LOCAL_PATH)/software/core/mpl
LOCAL_CPPFLAGS += -DLINUX=1
LOCAL_PRELINK_MODULE := false

LOCAL_SHARED_LIBRARIES += libmllite
LOCAL_C_INCLUDES += $(LOCAL_PATH)/software/core/mllite
LOCAL_CPPFLAGS += -DLINUX=1
LOCAL_PRELINK_MODULE := false

include $(BUILD_SHARED_LIBRARY)

#endif # !TARGET_SIMULATOR

# Build a temporary HAL that links the InvenSense .so
include $(CLEAR_VARS)
ifneq ($(filter dory guppy guppypdk, $(TARGET_DEVICE)),)
LOCAL_MODULE := sensors.invensense
else
ifeq (,$(filter $(TARGET_BUILD_VARIANT),eng userdebug))
ifneq ($(filter manta grouper tilapia, $(TARGET_DEVICE)),)
#LOCAL_MODULE := sensors.invensense
else
LOCAL_MODULE := sensors.${TARGET_PRODUCT}
endif
else    # eng & userdebug builds
LOCAL_MODULE := sensors.${TARGET_PRODUCT}
endif   # eng & userdebug builds
endif	# !guppy
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw

LOCAL_SHARED_LIBRARIES += libmplmpu
LOCAL_C_INCLUDES += $(LOCAL_PATH)/software/core/mllite
LOCAL_C_INCLUDES += $(LOCAL_PATH)/software/core/mllite/linux
LOCAL_C_INCLUDES += $(LOCAL_PATH)/software/core/mpl
LOCAL_C_INCLUDES += $(LOCAL_PATH)/software/core/driver/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/software/core/driver/include/linux

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -DLOG_TAG=\"Sensors\"

ifeq ($(VERSION_JB),true)
LOCAL_CFLAGS += -DANDROID_JELLYBEAN
endif

ifneq (,$(filter $(TARGET_BUILD_VARIANT),eng userdebug))
ifneq ($(COMPILE_INVENSENSE_COMPASS_CAL),0)
LOCAL_CFLAGS += -DINVENSENSE_COMPASS_CAL
endif
ifeq ($(COMPILE_THIRD_PARTY_ACCEL),1)
LOCAL_CFLAGS += -DTHIRD_PARTY_ACCEL
endif
ifeq ($(COMPILE_INVENSENSE_SENSOR_ON_PRIMARY_BUS), 1)
LOCAL_SRC_FILES += CompassSensor.IIO.primary.cpp
LOCAL_CFLAGS += -DSENSOR_ON_PRIMARY_BUS
else
LOCAL_SRC_FILES += CompassSensor.IIO.9150.cpp
endif
else # release builds, default
LOCAL_SRC_FILES += CompassSensor.IIO.9150.cpp
endif # userdebug

ifeq (,$(filter $(TARGET_BUILD_VARIANT),eng userdebug))
ifneq ($(filter manta grouper tilapia, $(TARGET_DEVICE)),)
# it's already defined in some other Makefile for production builds
#LOCAL_SRC_FILES := sensors_mpl.cpp
else
LOCAL_SRC_FILES := sensors_mpl.cpp
endif
else    # eng & userdebug builds
LOCAL_SRC_FILES := sensors_mpl.cpp
endif   # eng & userdebug builds

#LOCAL_STRIP_MODULE := false

LOCAL_SHARED_LIBRARIES := libinvensense_hal
LOCAL_SHARED_LIBRARIES += libcutils
LOCAL_SHARED_LIBRARIES += libutils
LOCAL_SHARED_LIBRARIES += libdl
LOCAL_SHARED_LIBRARIES += liblog
LOCAL_SHARED_LIBRARIES += libmllite
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libmplmpu
LOCAL_SRC_FILES := libmplmpu.so
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_OWNER := invensense
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
OVERRIDE_BUILT_MODULE_PATH := $(TARGET_OUT_INTERMEDIATE_LIBRARIES)
LOCAL_STRIP_MODULE := true
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := libmllite
LOCAL_SRC_FILES := libmllite.so
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_OWNER := invensense
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
OVERRIDE_BUILT_MODULE_PATH := $(TARGET_OUT_INTERMEDIATE_LIBRARIES)
LOCAL_STRIP_MODULE := true
include $(BUILD_PREBUILT)

