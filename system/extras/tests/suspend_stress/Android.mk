LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := suspend_stress.cpp
LOCAL_MODULE := suspend_stress
LOCAL_CFLAGS := -Wall -Werror
LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_STATIC_LIBRARIES := libc libcutils
include $(BUILD_EXECUTABLE)
