LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	Exynos_OMX_Component_Register.c \
	Exynos_OMX_Core.c

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := libExynosOMX_Core

LOCAL_CFLAGS :=

LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES := libExynosOMX_OSAL libExynosOMX_Basecomponent
LOCAL_SHARED_LIBRARIES := libc libdl libcutils libutils liblog \
	libExynosOMX_Resourcemanager

LOCAL_C_INCLUDES := $(EXYNOS_OMX_INC)/khronos \
	$(EXYNOS_OMX_INC)/exynos \
	$(EXYNOS_OMX_TOP)/osal \
	$(EXYNOS_OMX_TOP)/component/common

include $(BUILD_SHARED_LIBRARY)
