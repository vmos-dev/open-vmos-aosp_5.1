# Copyright 2013 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= get_dm_versions.c
LOCAL_MODULE:= get_dm_versions
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS :=
include $(BUILD_EXECUTABLE)
