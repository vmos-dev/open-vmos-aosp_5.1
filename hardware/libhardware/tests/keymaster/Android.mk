# Build the keymaster unit tests

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    keymaster_test.cpp

# Note that "bionic" is needed because of stlport
LOCAL_C_INCLUDES := \
    bionic \
    external/gtest/include \
    external/openssl/include \
    external/stlport/stlport

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libutils \
    libcrypto \
    libstlport \
    libhardware

LOCAL_STATIC_LIBRARIES := \
    libgtest \
    libgtest_main

LOCAL_MODULE := keymaster_test

LOCAL_MODULE_TAGS := tests

include $(BUILD_EXECUTABLE)
