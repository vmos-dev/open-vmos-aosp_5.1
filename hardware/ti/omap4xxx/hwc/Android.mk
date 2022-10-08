# HWC under heavy development and should not be included in builds for now
LOCAL_PATH := $(call my-dir)

# HAL module implementation, not prelinked and stored in
# hw/<HWCOMPOSE_HARDWARE_MODULE_ID>.<ro.product.board>.so
include $(CLEAR_VARS)
LOCAL_PRELINK_MODULE := false
LOCAL_ARM_MODE := arm
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/../vendor/lib/hw
LOCAL_SHARED_LIBRARIES := liblog libEGL libcutils libutils libhardware libhardware_legacy libz
LOCAL_SRC_FILES := hwc.c
LOCAL_STATIC_LIBRARIES := libpng

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE := hwcomposer.omap4
LOCAL_CFLAGS := -DLOG_TAG=\"ti_hwc\"
LOCAL_C_INCLUDES += external/libpng external/zlib
# LOG_NDEBUG=0 means verbose logging enabled
# LOCAL_CFLAGS += -DLOG_NDEBUG=0
include $(BUILD_SHARED_LIBRARY)
