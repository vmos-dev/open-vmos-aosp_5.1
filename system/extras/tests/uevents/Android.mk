LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := uevents.c

LOCAL_SHARED_LIBRARIES += libcutils
LOCAL_MODULE:= uevents

LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
