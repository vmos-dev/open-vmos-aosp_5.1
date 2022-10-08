LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	Exynos_OMX_Mpeg4dec.c \
	library_register.c

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := libOMX.Exynos.MPEG4.Decoder
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/omx

LOCAL_CFLAGS :=

ifeq ($(BOARD_NONBLOCK_MODE_PROCESS), true)
LOCAL_CFLAGS += -DNONBLOCK_MODE_PROCESS
endif

ifeq ($(BOARD_USE_ANB), true)
LOCAL_CFLAGS += -DUSE_ANB
endif

LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES := libExynosOMX_Vdec libExynosOMX_OSAL libExynosOMX_Basecomponent \
	libswconverter libExynosVideoApi
LOCAL_SHARED_LIBRARIES := libc libdl libcutils libutils liblog libui \
	libExynosOMX_Resourcemanager libcsc libexynosv4l2 libion_exynos libexynosgscaler \
	libhardware

ifeq ($(BOARD_USES_MFC_FPS),true)
LOCAL_CFLAGS += -DCONFIG_MFC_FPS
endif

LOCAL_C_INCLUDES := $(EXYNOS_OMX_INC)/khronos \
	$(EXYNOS_OMX_INC)/exynos \
	$(EXYNOS_OMX_TOP)/osal \
	$(EXYNOS_OMX_TOP)/core \
	$(EXYNOS_OMX_COMPONENT)/common \
	$(EXYNOS_OMX_COMPONENT)/video/dec \
	hardware/samsung_slsi/exynos5/include \
	hardware/samsung_slsi/exynos5/libcsc \
	hardware/samsung_slsi/exynos5/exynos_omx/codecs/exynos_codecs/video/exynos5/mfc_v4l2/include

include $(BUILD_SHARED_LIBRARY)
