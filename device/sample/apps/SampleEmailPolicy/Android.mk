# Copyright 2010, The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

ifneq ($(TARGET_BUILD_JAVA_SUPPORT_LEVEL),)

# This is the makefile for the Email Policy package contained elsewhere in this sample.
# When deploying to an actual device, you must change LOCAL_PACKAGE_NAME to the name desired for
# your local version, e.g. LOCAL_PACKAGE_NAME := MyDeviceEmailPolicy  This will cause the build
# system to create "MyDeviceEmailPolicy.apk".  You must then add this to the appropriate product.mk
# file for your device, to include the APK file in your system image.

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_PACKAGE_NAME := SampleEmailPolicy
LOCAL_CERTIFICATE := platform

LOCAL_PROGUARD_FLAG_FILES := proguard.flags

include $(BUILD_PACKAGE)

endif # JAVA_SUPPORT
