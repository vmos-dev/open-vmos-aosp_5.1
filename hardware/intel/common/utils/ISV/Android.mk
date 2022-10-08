ifeq ($(TARGET_HAS_ISV),true)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    omx/isv_omxcore.cpp \
    omx/isv_omxcomponent.cpp \
    base/isv_bufmanager.cpp \
    base/isv_processor.cpp \
    base/isv_worker.cpp \
    profile/isv_profile.cpp

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libisv_omx_core
LOCAL_32_BIT_ONLY := true

LOCAL_SHARED_LIBRARIES := \
    libutils \
    libcutils \
    libdl \
    libhardware \
    libexpat \
    libva \
    libva-android

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/include \
    $(call include-path-for, frameworks-openmax) \
    $(TARGET_OUT_HEADERS)/libmedia_utils_vpp \
    $(TARGET_OUT_HEADERS)/display \
    $(TARGET_OUT_HEADERS)/khronos/openmax \
    $(TARGET_OUT_HEADERS)/libva \
    $(TARGET_OUT_HEADERS)/pvr/hal \
    $(call include-path-for, frameworks-native)/media/openmax

ifeq ($(USE_MEDIASDK),true)
    LOCAL_CFLAGS += -DUSE_MEDIASDK
endif

ifeq ($(TARGET_VPP_USE_GEN),true)
    LOCAL_CFLAGS += -DTARGET_VPP_USE_GEN
endif

LOCAL_CFLAGS += -Werror

include $(BUILD_SHARED_LIBRARY)

endif
