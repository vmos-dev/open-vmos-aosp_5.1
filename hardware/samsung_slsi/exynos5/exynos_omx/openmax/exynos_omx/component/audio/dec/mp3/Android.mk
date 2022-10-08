LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	Exynos_OMX_Mp3dec.c \
	library_register.c

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := libOMX.Exynos.MP3.Decoder
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/omx

LOCAL_CFLAGS :=

LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES := libExynosOMX_Adec libExynosOMX_OSAL libExynosOMX_Basecomponent \
	libsrpapi
LOCAL_SHARED_LIBRARIES := libc libdl libcutils libutils libui \
	libExynosOMX_Resourcemanager

LOCAL_C_INCLUDES := $(EXYNOS_OMX_INC)/khronos \
	$(EXYNOS_OMX_INC)/exynos \
	$(EXYNOS_OMX_TOP)/osal \
	$(EXYNOS_OMX_TOP)/core \
	$(EXYNOS_OMX_COMPONENT)/common \
	$(EXYNOS_OMX_COMPONENT)/audio/dec \
	hardware/samsung_slsi/exynos5/exynos_omx/codecs/exynos_codecs/audio/exynos5/srp/alp/include

include $(BUILD_SHARED_LIBRARY)
