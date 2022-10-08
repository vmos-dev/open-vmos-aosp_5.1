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

######################################################
# Library used by both policy manager and the audioHAL
######################################################
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    alsa_utils.cpp \
    AudioHardwareOutput.cpp \
    AudioOutput.cpp \
    AudioStreamOut.cpp \
    HDMIAudioOutput.cpp \
    AudioHardwareInput.cpp \
    AudioStreamIn.cpp \
    AudioHotplugThread.cpp

LOCAL_C_INCLUDES := \
    external/tinyalsa/include \
    $(call include-path-for, audio-utils)

LOCAL_SHARED_LIBRARIES := \
    libcommon_time_client \
    libcutils \
    liblog \
    libutils \
    libmedia \
    libbinder \
    libtinyalsa \
    libaudiospdif \
    libaudioutils

# until remotecontrolservice is added to PDK, don't include
# this in aosp_fugu builds.  only use in regular fugu builds.
ifeq ($(TARGET_PRODUCT),fugu)
LOCAL_C_INCLUDES += \
    vendor/google_athome/services/RemoteControlService/include

LOCAL_SHARED_LIBRARIES += \
    libremotecontrolservice

LOCAL_CFLAGS += -DREMOTE_CONTROL_INTERFACE
endif

LOCAL_STATIC_LIBRARIES += libmedia_helper

LOCAL_MODULE := libatv_audio
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)


##################################
# audioHAL
##################################
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    audio_hal_hooks.c \
    audio_hal_thunks.cpp \

LOCAL_C_INCLUDES := \
    external/tinyalsa/include \
    $(call include-path-for, audio-utils)

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    liblog \
    libutils \
    libmedia \
    libatv_audio

LOCAL_MODULE := audio.primary.fugu
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

##################################
# Audio Policy Manager
##################################
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    ATVAudioPolicyManager.cpp

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    liblog \
    libutils \
    libmedia \
    libatv_audio \
    libbinder \
    libaudiopolicymanagerdefault

LOCAL_C_INCLUDES := \
    external/tinyalsa/include \
    $(TOPDIR)frameworks/av/services/audiopolicy

ifeq ($(TARGET_PRODUCT),fugu)
LOCAL_C_INCLUDES += \
    vendor/google_athome/services/RemoteControlService/include

LOCAL_SHARED_LIBRARIES += \
    libremotecontrolservice

LOCAL_CFLAGS += -DREMOTE_CONTROL_INTERFACE
endif

LOCAL_MODULE := libaudiopolicymanager
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
