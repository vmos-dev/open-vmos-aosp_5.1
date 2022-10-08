# Copyright 2007 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= showslab.c
LOCAL_SHARED_LIBRARIES :=
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)

LOCAL_MODULE_TAGS := debug

LOCAL_MODULE:= showslab

include $(BUILD_EXECUTABLE)

