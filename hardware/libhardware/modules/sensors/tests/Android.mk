LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	SensorEventQueue_test.cpp

#LOCAL_CFLAGS := -g
LOCAL_MODULE := sensorstests

LOCAL_STATIC_LIBRARIES := libcutils libutils

LOCAL_C_INCLUDES := $(LOCAL_PATH)/.. bionic

LOCAL_LDLIBS += -lpthread

include $(BUILD_HOST_EXECUTABLE)
