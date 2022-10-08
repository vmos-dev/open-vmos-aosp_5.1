ifeq ($(BUILD_WITH_FULL_STAGEFRIGHT),true)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    WrsOMXPlugin.cpp


LOCAL_CFLAGS := $(PV_CFLAGS_MINUS_VISIBILITY)

#enable log
#LOCAL_CFLAGS += -DLOG_NDEBUG=0

ifeq ($(USE_MEDIASDK),true)
    LOCAL_CFLAGS += -DUSE_MEDIASDK
endif

ifeq ($(TARGET_HAS_ISV), true)
LOCAL_CFLAGS +=-DTARGET_HAS_ISV
endif

LOCAL_C_INCLUDES:= \
        $(call include-path-for, frameworks-native)/media/hardware \
        $(call include-path-for, frameworks-native)/media/openmax

LOCAL_SHARED_LIBRARIES :=       \
        libbinder               \
        libutils                \
        libcutils               \
        libdl                   \
        libstagefright_foundation

LOCAL_MODULE := libstagefrighthw

include $(BUILD_SHARED_LIBRARY)
endif

