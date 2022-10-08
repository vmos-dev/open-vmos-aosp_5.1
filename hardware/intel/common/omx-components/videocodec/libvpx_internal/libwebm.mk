# libwebm
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := libwebm/mkvparser.cpp

LOCAL_MODULE := libwebm

include $(BUILD_STATIC_LIBRARY)
