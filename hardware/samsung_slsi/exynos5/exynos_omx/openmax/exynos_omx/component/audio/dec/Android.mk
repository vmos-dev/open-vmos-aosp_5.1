LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	Exynos_OMX_Adec.c

LOCAL_MODULE := libExynosOMX_Adec
LOCAL_ARM_MODE := arm
LOCAL_MODULE_TAGS := optional

LOCAL_C_INCLUDES := $(EXYNOS_OMX_INC)/khronos \
	$(EXYNOS_OMX_INC)/exynos \
	$(EXYNOS_OMX_TOP)/osal \
	$(EXYNOS_OMX_TOP)/core \
	$(EXYNOS_OMX_COMPONENT)/common \
	$(EXYNOS_OMX_COMPONENT)/audio/dec \
	hardware/samsung_slsi/exynos5/exynos_omx/codecs/exynos_codecs/audio/exynos5/srp/alp/include

include $(BUILD_STATIC_LIBRARY)
