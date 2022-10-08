# Copyright (C) 2011 The Android Open Source Project
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

LOCAL_PATH := $(call my-dir)

# Make the HAL library
# ============================================================
include $(CLEAR_VARS)

LOCAL_REQUIRED_MODULES :=

LOCAL_CFLAGS += -Wno-unused-parameter -Wno-int-to-pointer-cast
LOCAL_CFLAGS += -Wno-maybe-uninitialized -Wno-parentheses
LOCAL_CPPFLAGS += -Wno-conversion-null

LOCAL_C_INCLUDES += \
	external/libnl/include \
	$(call include-path-for, libhardware_legacy)/hardware_legacy \
	external/wpa_supplicant_8/src/drivers \
	$(TARGET_OUT_HEADERS)/libwpa_client \
	$(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include

LOCAL_SRC_FILES := \
	wifi_hal.cpp \
	common.cpp \
	cpp_bindings.cpp \
	llstats.cpp \
	gscan.cpp \
	gscan_event_handler.cpp \
	rtt.cpp

LOCAL_MODULE := libwifi-hal-qcom
LOCAL_SHARED_LIBRARIES += libnetutils liblog

ifneq ($(wildcard external/libnl),)
LOCAL_SHARED_LIBRARIES += libnl
LOCAL_C_INCLUDES += external/libnl/include
else
LOCAL_SHARED_LIBRARIES += libnl_2
LOCAL_C_INCLUDES += external/libnl-headers
endif

include $(BUILD_STATIC_LIBRARY)

