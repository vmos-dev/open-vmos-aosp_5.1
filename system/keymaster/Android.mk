# Copyright (C) 2014 The Android Open Source Project
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

###
# libkeymaster_messages contains just the code necessary to communicate with a
# GoogleKeymaster implementation, e.g. one running in TrustZone.
#
# Note that this library is too large; it should not include ocb.c and not use
# openssl.  At present it must, because the code needs refactoring to separate
# concerns a bit better.
#
# TODO(swillden@google.com): Refactor and pare this down.
##
include $(CLEAR_VARS)
LOCAL_MODULE:= libkeymaster_messages
LOCAL_SRC_FILES:= \
		authorization_set.cpp \
		google_keymaster_messages.cpp \
		google_keymaster_utils.cpp \
		ocb.c \
		key_blob.cpp \
		serializable.cpp
LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	external/openssl/include
LOCAL_SHARED_LIBRARIES := libcrypto
LOCAL_CFLAGS = -Wall -Werror
LOCAL_MODULE_TAGS := optional
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE:= libkeymaster
LOCAL_SRC_FILES:= \
		authorization_set.cpp \
		google_keymaster_messages.cpp \
		google_keymaster.cpp \
		google_keymaster_utils.cpp \
		ocb.c \
		key_blob.cpp \
		serializable.cpp
LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	external/openssl/include
LOCAL_SHARED_LIBRARIES := libcrypto
LOCAL_CFLAGS = -Wall -Werror
LOCAL_MODULE_TAGS := optional
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk
include $(BUILD_STATIC_LIBRARY)
