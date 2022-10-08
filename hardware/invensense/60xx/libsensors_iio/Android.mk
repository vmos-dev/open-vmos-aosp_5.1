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

ifneq ($(TARGET_SIMULATOR),true)

# InvenSense fragment of the HAL
include $(CLEAR_VARS)

LOCAL_MODULE := libinvensense_hal

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_OWNER := invensense

LOCAL_CFLAGS := -DLOG_TAG=\"Sensors\"
# Comment out for ICS. Affects Android LOG macros.
LOCAL_CFLAGS += -DANDROID_JELLYBEAN

ifeq ($(ENG_BUILD),1)
ifeq ($(COMPILE_INVENSENSE_COMPASS_CAL),1)
LOCAL_CFLAGS += -DINVENSENSE_COMPASS_CAL
endif
ifeq ($(COMPILE_THIRD_PARTY_ACCEL),1)
LOCAL_CFLAGS += -DTHIRD_PARTY_ACCEL
endif
ifeq ($(COMPILE_COMPASS_YAS53x),1)
LOCAL_CFLAGS += -DCOMPASS_YAS53x
endif
ifeq ($(COMPILE_COMPASS_AK8975),1)
LOCAL_CFLAGS += -DCOMPASS_AK8975
endif
ifeq ($(COMPILE_COMPASS_AMI306),1)
LOCAL_CFLAGS += -DCOMPASS_AMI306
endif
else # release builds, default
LOCAL_CFLAGS += -DINVENSENSE_COMPASS_CAL
endif

LOCAL_SRC_FILES := SensorBase.cpp
LOCAL_SRC_FILES += MPLSensor.cpp
LOCAL_SRC_FILES += MPLSupport.cpp
LOCAL_SRC_FILES += InputEventReader.cpp
LOCAL_SRC_FILES += CompassSensor.IIO.9150.cpp

LOCAL_C_INCLUDES += $(LOCAL_PATH)
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
# Experimental
ifeq ($(BOARD_INV_LIBMLLITE_FROM_SOURCE),true)
LOCAL_CPPFLAGS += -DLIBMLLITE_FROM_SOURCE
endif
LOCAL_PRELINK_MODULE := false

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

# Experimental
ifneq ($(BOARD_INV_LIBMLLITE_FROM_SOURCE),true)
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
else
include $(CLEAR_VARS)
LOCAL_CFLAGS += -DANDROID_JELLYBEAN
LOCAL_CFLAGS += -DLINUX=1
LOCAL_MODULE := libmllite
LOCAL_SRC_FILES := $(call all-c-files-under, software/core/mllite)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/software/core/mllite
LOCAL_C_INCLUDES += $(LOCAL_PATH)/software/core/mllite/linux
LOCAL_C_INCLUDES += $(LOCAL_PATH)/software/core/driver/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/software/core/driver/include/linux
LOCAL_MODULE_OWNER := invensense
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
LOCAL_SHARED_LIBRARIES := liblog
include $(BUILD_SHARED_LIBRARY)
endif

endif # !TARGET_SIMULATOR
