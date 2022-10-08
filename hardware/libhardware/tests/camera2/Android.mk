LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	camera2.cpp \
	camera2_utils.cpp \
	main.cpp \
	CameraMetadataTests.cpp \
	CameraModuleTests.cpp \
	CameraStreamTests.cpp \
	CameraFrameTests.cpp \
	CameraBurstTests.cpp \
	CameraMultiStreamTests.cpp\
	ForkedTests.cpp \
	TestForkerEventListener.cpp \
	TestSettings.cpp \

LOCAL_SHARED_LIBRARIES := \
	liblog \
	libutils \
	libcutils \
	libstlport \
	libhardware \
	libcamera_metadata \
	libcameraservice \
	libcamera_client \
	libgui \
	libsync \
	libui \
	libdl

LOCAL_STATIC_LIBRARIES := \
	libgtest

LOCAL_C_INCLUDES += \
	bionic \
	bionic/libstdc++/include \
	external/gtest/include \
	external/stlport/stlport \
	system/media/camera/include \
	frameworks/av/include/ \
	frameworks/av/services/camera/libcameraservice \
	frameworks/native/include \

LOCAL_CFLAGS += -Wall -Wextra

LOCAL_MODULE:= camera2_test
LOCAL_MODULE_STEM_32 := camera2_test
LOCAL_MODULE_STEM_64 := camera2_test64
LOCAL_MULTILIB := both
LOCAL_MODULE_TAGS := tests

include $(BUILD_NATIVE_TEST)
