ifeq ($(strip $(BOARD_USES_ALSA_AUDIO)),true)

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

AUDIO_PLATFORM := $(TARGET_BOARD_PLATFORM)
ifneq ($(filter msm8974 msm8226 msm8084,$(TARGET_BOARD_PLATFORM)),)
  # B-family platform uses msm8974 code base
  AUDIO_PLATFORM = msm8974
ifneq ($(filter msm8226,$(TARGET_BOARD_PLATFORM)),)
  LOCAL_CFLAGS := -DPLATFORM_MSM8x26
endif
ifneq ($(filter msm8084,$(TARGET_BOARD_PLATFORM)),)
  LOCAL_CFLAGS := -DPLATFORM_MSM8084
endif
endif

LOCAL_SRC_FILES := \
	audio_hw.c \
	voice.c \
	platform_info.c \
	audio_extn/ext_speaker.c \
	$(AUDIO_PLATFORM)/platform.c

LOCAL_SHARED_LIBRARIES := \
	liblog \
	libcutils \
	libtinyalsa \
	libtinycompress \
	libaudioroute \
	libdl \
	libexpat

LOCAL_C_INCLUDES += \
	external/tinyalsa/include \
	external/tinycompress/include \
	$(call include-path-for, audio-route) \
	$(call include-path-for, audio-effects) \
	$(LOCAL_PATH)/$(AUDIO_PLATFORM) \
	$(LOCAL_PATH)/audio_extn \
	$(LOCAL_PATH)/voice_extn \
	external/expat/lib

ifeq ($(strip $(AUDIO_FEATURE_ENABLED_MULTI_VOICE_SESSIONS)),true)
    LOCAL_CFLAGS += -DMULTI_VOICE_SESSION_ENABLED
    LOCAL_SRC_FILES += voice_extn/voice_extn.c
endif

ifeq ($(strip $(AUDIO_FEATURE_ENABLED_HFP)),true)
    LOCAL_CFLAGS += -DHFP_ENABLED
    LOCAL_SRC_FILES += audio_extn/hfp.c
endif

LOCAL_MODULE := audio.primary.$(TARGET_BOARD_PLATFORM)

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw

LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

endif
