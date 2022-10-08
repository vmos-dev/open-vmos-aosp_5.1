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

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := perm_checker.c

LOCAL_SHARED_LIBRARIES := libc

LOCAL_MODULE := perm_checker

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/local

include $(BUILD_EXECUTABLE)

####

include $(CLEAR_VARS)

LOCAL_MODULE := perm_checker.conf

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_CLASS := DATA

LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/local

LOCAL_SRC_FILES := $(LOCAL_MODULE)

include $(BUILD_PREBUILT)

####

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := tests

LOCAL_MODULE := recovery_test

LOCAL_SRC_FILES := recovery_test.cpp

LOCAL_SHARED_LIBRARIES += libcutils libutils liblog liblogwrap

LOCAL_STATIC_LIBRARIES += libtestUtil libfs_mgr

LOCAL_C_INCLUDES += system/extras/tests/include \
                    system/core/fs_mgr/include \
                    system/extras/ext4_utils \
                    system/core/logwrapper/include

include $(BUILD_NATIVE_TEST)
