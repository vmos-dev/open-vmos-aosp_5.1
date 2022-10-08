LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	nusensors.cpp

LOCAL_SHARED_LIBRARIES := \
	libcutils libhardware

LOCAL_MODULE:= test-nusensors

LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
