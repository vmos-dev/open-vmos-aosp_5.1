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


ifneq ($(filter manta, $(TARGET_DEVICE)),)
LOCAL_PATH := $(call my-dir)

# HAL module implemenation stored in
# hw/<SENSORS_HARDWARE_MODULE_ID>.<ro.product.board>.so
include $(CLEAR_VARS)

LOCAL_MODULE := sensors.manta

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw

LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -DLOG_TAG=\"Sensors\"
LOCAL_CFLAGS += -DINVENSENSE_COMPASS_CAL
LOCAL_C_INCLUDES += hardware/invensense/60xx/libsensors_iio
LOCAL_SRC_FILES := \
    sensors.cpp \
    IioSensorBase.cpp \
    InputEventReader.cpp \
    LightSensor.cpp \
    PressureSensor.cpp \
    SensorBase.cpp

LOCAL_SHARED_LIBRARIES := libinvensense_hal liblog libutils libdl

include $(BUILD_SHARED_LIBRARY)

endif # is a manta TARGET_DEVICE
