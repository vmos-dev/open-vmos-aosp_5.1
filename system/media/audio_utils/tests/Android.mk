# Build the unit tests for audio_utils

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SHARED_LIBRARIES := \
	liblog \
	libcutils \
	libstlport \
	libaudioutils

LOCAL_STATIC_LIBRARIES := \
	libgtest \
	libgtest_main

LOCAL_C_INCLUDES := \
	bionic \
	bionic/libstdc++/include \
	external/gtest/include \
	external/stlport/stlport \
	$(call include-path-for, audio-utils)

LOCAL_SRC_FILES := \
	primitives_tests.cpp

LOCAL_MODULE := primitives_tests
LOCAL_MODULE_TAGS := tests

include $(BUILD_EXECUTABLE)
