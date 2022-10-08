LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

#===============================================================================
#            Deploy the headers that can be exposed
#===============================================================================

LOCAL_COPY_HEADERS_TO   := mm-video-v4l2/DivxDrmDecrypt
LOCAL_COPY_HEADERS      := inc/DivXDrmDecrypt.h

LOCAL_CFLAGS := \
    -D_ANDROID_

LOCAL_SRC_FILES:=       \
    src/DivXDrmDecrypt.cpp

LOCAL_C_INCLUDES:= \
    $(LOCAL_PATH)/inc \
    $(TARGET_OUT_HEADERS)/mm-core/omxcore

LOCAL_PRELINK_MODULE:= false

LOCAL_MODULE:= libdivxdrmdecrypt
LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES	:= liblog libdl

LOCAL_LDLIBS +=
include $(BUILD_SHARED_LIBRARY)
