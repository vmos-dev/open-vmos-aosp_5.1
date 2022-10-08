LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    fingerprint_tests.cpp \

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libhardware \

#LOCAL_C_INCLUDES += \
#    system/media/camera/include \

LOCAL_CFLAGS += -Wall -Wextra

LOCAL_MODULE:= fingerprint_tests
LOCAL_MODULE_TAGS := tests

include $(BUILD_NATIVE_TEST)
