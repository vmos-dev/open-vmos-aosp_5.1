#
# Copyright (C) 2012 The Android Open Source Project
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
#

# enable jni only if java build is supported, for PDK
ifneq ($(TARGET_BUILD_JAVA_SUPPORT_LEVEL),)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := libhellojni_jni

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
		HelloJniNative.cpp

LOCAL_C_INCLUDES := $(JNI_H_INCLUDE)

include $(BUILD_SHARED_LIBRARY)

endif # java supproted
