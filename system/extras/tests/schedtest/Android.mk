LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	schedtest.c

LOCAL_MODULE := schedtest

include $(BUILD_EXECUTABLE)
