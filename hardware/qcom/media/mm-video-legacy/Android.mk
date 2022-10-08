ifneq ($(filter msm8960,$(TARGET_BOARD_PLATFORM)),)
OMX_VIDEO_PATH := $(call my-dir)
include $(CLEAR_VARS)

include $(OMX_VIDEO_PATH)/vidc/Android.mk
include $(OMX_VIDEO_PATH)/DivxDrmDecrypt/Android.mk
endif
