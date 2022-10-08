LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    omx_rpc/src/omx_rpc.c \
    omx_rpc/src/omx_rpc_skel.c \
    omx_rpc/src/omx_rpc_stub.c \
    omx_rpc/src/omx_rpc_config.c \
    omx_rpc/src/omx_rpc_platform.c \
    omx_proxy_common/src/omx_proxy_common.c \
    profiling/src/profile.c

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/omx_rpc/inc \
    $(LOCAL_PATH)/../omx_core/inc \
    $(LOCAL_PATH)/../mm_osal/inc \
    $(LOCAL_PATH)/profiling/inc \
    $(HARDWARE_TI_OMAP4_BASE)/hwc/ \
    $(HARDWARE_TI_OMAP4_BASE)/ion/ \
    $(HARDWARE_TI_OMAP4_BASE)/kernel-headers-ti \
    $(HARDWARE_TI_OMAP4_BASE)/system-core-headers-ti \
    system/core/include/cutils \
    $(HARDWARE_TI_OMAP4_BASE)/../../libhardware/include

LOCAL_CFLAGS += -D_Android -DENABLE_GRALLOC_BUFFERS -DUSE_ENHANCED_PORTRECONFIG -DANDROID_QUIRK_LOCK_BUFFER -DUSE_ION


LOCAL_SHARED_LIBRARIES := \
    libmm_osal \
    libc \
    liblog \
    libion_ti \
    libcutils

LOCAL_MODULE:= libdomx
LOCAL_MODULE_TAGS:= optional

include $(BUILD_HEAPTRACKED_SHARED_LIBRARY)
