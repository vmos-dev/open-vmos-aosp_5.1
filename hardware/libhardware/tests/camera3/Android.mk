LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    camera3tests.cpp \

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libhardware \
    libcamera_metadata \

LOCAL_C_INCLUDES += \
    system/media/camera/include \

LOCAL_CFLAGS += -Wall -Wextra

LOCAL_MODULE:= camera3_tests
LOCAL_MODULE_TAGS := tests

include $(BUILD_NATIVE_TEST)
