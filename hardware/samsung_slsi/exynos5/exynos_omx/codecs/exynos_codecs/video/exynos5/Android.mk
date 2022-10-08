LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_VIDEO_PATH :=$(LOCAL_PATH)

include    $(LOCAL_VIDEO_PATH)/mfc_v4l2/Android.mk
