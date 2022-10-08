LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	swconvertor.c \
	csc_tiled_to_linear_y_neon.s \
	csc_tiled_to_linear_uv_neon.s \
	csc_tiled_to_linear_uv_deinterleave_neon.s \
	csc_interleave_memcpy_neon.s \
	csc_ARGB8888_to_YUV420SP_NEON.s

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../exynos_omx/openmax/include/khronos \
	$(LOCAL_PATH)/../include

LOCAL_MODULE := libswconverter

LOCAL_PRELINK_MODULE := false

LOCAL_CFLAGS :=

LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES :=
LOCAL_SHARED_LIBRARIES := liblog

include $(BUILD_STATIC_LIBRARY)
