# hardware/libaudio-alsa/Android.mk
#
# Copyright 2008 Wind River Systems
#

ifeq ($(strip $(BOARD_USES_ALSA_AUDIO)),true)

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm
LOCAL_CFLAGS := -D_POSIX_SOURCE
LOCAL_CFLAGS += -DQCOM_CSDCLIENT_ENABLED
LOCAL_CFLAGS += -DQCOM_ACDB_ENABLED

ifeq ($(strip $(BOARD_USES_FLUENCE_INCALL)),true)
LOCAL_CFLAGS += -DUSES_FLUENCE_INCALL
endif

ifeq ($(strip $(BOARD_USES_SEPERATED_AUDIO_INPUT)),true)
LOCAL_CFLAGS += -DSEPERATED_AUDIO_INPUT
endif

LOCAL_SRC_FILES := \
  AudioHardwareALSA.cpp 	\
  AudioStreamOutALSA.cpp 	\
  AudioStreamInALSA.cpp 	\
  ALSAStreamOps.cpp		\
  audio_hw_hal.cpp \
  AudioUsbALSA.cpp \
  AudioUtil.cpp

LOCAL_STATIC_LIBRARIES := \
    libmedia_helper \
    libaudiohw_legacy \
    libaudiopolicy_legacy \

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libutils \
    libmedia \
    libhardware \
    libc        \
    libpower    \
    libalsa-intf

ifeq ($(TARGET_SIMULATOR),true)
 LOCAL_LDLIBS += -ldl
else
 LOCAL_SHARED_LIBRARIES += libdl
endif

LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/mm-audio/audio-alsa
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/mm-audio/libalsa-intf
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/mm-audio/surround_sound/
LOCAL_C_INCLUDES += hardware/libhardware/include
LOCAL_C_INCLUDES += hardware/libhardware_legacy/include
LOCAL_C_INCLUDES += frameworks/base/include
LOCAL_C_INCLUDES += system/core/include


LOCAL_MODULE := audio.primary.msm8960
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

# This is the ALSA audio policy manager

include $(CLEAR_VARS)

LOCAL_CFLAGS := -D_POSIX_SOURCE
LOCAL_CFLAGS += -DQCOM_ACDB_ENABLED

LOCAL_SRC_FILES := \
    audio_policy_hal.cpp \
    AudioPolicyManagerALSA.cpp

LOCAL_MODULE := audio_policy.msm8960
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE_TAGS := optional

LOCAL_STATIC_LIBRARIES := \
    libmedia_helper \
    libaudiopolicy_legacy

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libutils

LOCAL_C_INCLUDES += hardware/libhardware_legacy/audio

include $(BUILD_SHARED_LIBRARY)

# This is the ALSA module which behaves closely like the original

include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw

LOCAL_CFLAGS := -D_POSIX_SOURCE -Wno-multichar
LOCAL_CFLAGS += -DQCOM_ACDB_ENABLED

ifeq ($(strip $(BOARD_USES_FLUENCE_INCALL)),true)
LOCAL_CFLAGS += -DUSES_FLUENCE_INCALL
endif

ifeq ($(strip $(BOARD_USES_SEPERATED_AUDIO_INPUT)),true)
LOCAL_CFLAGS += -DSEPERATED_AUDIO_INPUT
endif

ifneq ($(ALSA_DEFAULT_SAMPLE_RATE),)
    LOCAL_CFLAGS += -DALSA_DEFAULT_SAMPLE_RATE=$(ALSA_DEFAULT_SAMPLE_RATE)
endif

LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/mm-audio/libalsa-intf

LOCAL_SRC_FILES:= \
    alsa_default.cpp \
    ALSAControl.cpp \
    AudioUtil.cpp

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    liblog    \
    libalsa-intf

ifeq ($(TARGET_SIMULATOR),true)
 LOCAL_LDLIBS += -ldl
else
 LOCAL_SHARED_LIBRARIES += libdl
endif

LOCAL_MODULE:= alsa.msm8960
LOCAL_MODULE_TAGS := optional

  include $(BUILD_SHARED_LIBRARY)
endif
