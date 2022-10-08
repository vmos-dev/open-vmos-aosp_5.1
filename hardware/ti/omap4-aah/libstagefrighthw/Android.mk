ifeq ($(TARGET_BOARD_PLATFORM),omap4)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    TIOMXPlugin.cpp

LOCAL_C_INCLUDES:= \
        $(TOP)/frameworks/native/include/media/openmax \
        $(TOP)/frameworks/native/include/media/hardware

LOCAL_SHARED_LIBRARIES :=       \
        libbinder               \
        libutils                \
        libcutils               \
        liblog                  \
        libui                   \
        libdl                   \

LOCAL_MODULE := libstagefrighthw

include $(BUILD_HEAPTRACKED_SHARED_LIBRARY)

endif
