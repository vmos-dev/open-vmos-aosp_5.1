ifeq ($(TARGET_BOARD_PLATFORM),omap4)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    ColorConvert.cpp

LOCAL_C_INCLUDES:= \
        $(TOP)/frameworks/native/include/media/openmax \
        $(TOP)/frameworks/native/include/media/editor

LOCAL_SHARED_LIBRARIES :=       \

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE := libI420colorconvert

include $(BUILD_HEAPTRACKED_SHARED_LIBRARY)

endif
