LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	Exynos_OMX_Basecomponent.c \
	Exynos_OMX_Baseport.c

LOCAL_MODULE := libExynosOMX_Basecomponent

LOCAL_CFLAGS :=

LOCAL_STATIC_LIBRARIES := libExynosOMX_OSAL
LOCAL_SHARED_LIBRARIES := libcutils libutils

LOCAL_C_INCLUDES := $(EXYNOS_OMX_INC)/khronos \
	$(EXYNOS_OMX_INC)/exynos \
	$(EXYNOS_OMX_TOP)/osal

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	Exynos_OMX_Resourcemanager.c

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := libExynosOMX_Resourcemanager

LOCAL_CFLAGS :=

LOCAL_STATIC_LIBRARIES := libExynosOMX_OSAL
LOCAL_SHARED_LIBRARIES := libcutils libutils

LOCAL_C_INCLUDES := $(EXYNOS_OMX_INC)/khronos \
	$(EXYNOS_OMX_INC)/exynos \
	$(EXYNOS_OMX_TOP)/osal

include $(BUILD_SHARED_LIBRARY)
