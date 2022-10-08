# Copyright 2013 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    librilutils.c \
    record_stream.c

LOCAL_CFLAGS :=

LOCAL_MODULE:= librilutils

include $(BUILD_SHARED_LIBRARY)


# Create static library for those that want it
# =========================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    librilutils.c \
    record_stream.c

LOCAL_STATIC_LIBRARIES :=

LOCAL_CFLAGS :=

LOCAL_MODULE:= librilutils_static

include $(BUILD_STATIC_LIBRARY)
