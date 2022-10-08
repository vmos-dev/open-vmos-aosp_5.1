# Build the unit tests.
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SHARED_LIBRARIES := \
	libutils \
	libstlport \
	libcamera_metadata

LOCAL_STATIC_LIBRARIES := \
	libgtest \
	libgtest_main

LOCAL_C_INCLUDES := \
	bionic \
	bionic/libstdc++/include \
	external/gtest/include \
	external/stlport/stlport \
	system/media/camera/include \
	system/media/private/camera/include

LOCAL_SRC_FILES := \
	camera_metadata_tests.cpp

LOCAL_MODULE := camera_metadata_tests
LOCAL_MODULE_TAGS := tests
LOCAL_MODULE_STEM_32 := camera_metadata_tests
LOCAL_MODULE_STEM_64 := camera_metadata_tests64
LOCAL_MULTILIB := both

include $(BUILD_NATIVE_TEST)
