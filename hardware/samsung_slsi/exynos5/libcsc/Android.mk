LOCAL_PATH := $(call my-dir)

OMX_NAME := exynos

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    csc_helper.c

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)

LOCAL_C_INCLUDES := \
    system/core/include \
    $(LOCAL_PATH)/../exynos_omx/openmax/$(OMX_NAME)_omx/include/khronos \
    $(LOCAL_PATH)/../exynos_omx/openmax/$(OMX_NAME)_omx/include/$(OMX_NAME) \
    hardware/samsung_slsi/exynos5/include

LOCAL_CFLAGS := \
    -DUSE_SAMSUNG_COLORFORMAT \
    -DEXYNOS_OMX

LOCAL_MODULE := libcsc_helper
LOCAL_MODULE_TAGS := optional
LOCAL_STATIC_LIBRARIES := liblog

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	csc.c

LOCAL_C_INCLUDES := \
    hardware/samsung_slsi/exynos5/include \
	$(LOCAL_PATH)/../exynos_omx/openmax/$(OMX_NAME)_omx/include/khronos \
	$(LOCAL_PATH)/../exynos_omx/openmax/$(OMX_NAME)_omx/include/$(OMX_NAME) \
	$(LOCAL_PATH)/../libexynosutils

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)

LOCAL_CFLAGS :=

LOCAL_MODULE := libcsc

LOCAL_PRELINK_MODULE := false

LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES := libswconverter
LOCAL_WHOLE_STATIC_LIBRARIES := libcsc_helper
LOCAL_SHARED_LIBRARIES := liblog libexynosutils

LOCAL_CFLAGS += -DUSE_SAMSUNG_COLORFORMAT

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../include
LOCAL_CFLAGS += -DENABLE_GSCALER -DENABLE_G2D
LOCAL_SHARED_LIBRARIES += libexynosgscaler

LOCAL_CFLAGS += -DUSE_ION
LOCAL_SHARED_LIBRARIES += libion_exynos

LOCAL_CFLAGS += -DEXYNOS_OMX

include $(BUILD_SHARED_LIBRARY)
