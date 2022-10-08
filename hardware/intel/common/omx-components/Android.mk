ifeq ($(BOARD_USES_MRST_OMX),true)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

INTEL_OMX_COMPONENT_ROOT := $(LOCAL_PATH)

#intel video codecs
include $(INTEL_OMX_COMPONENT_ROOT)/videocodec/Android.mk
include $(INTEL_OMX_COMPONENT_ROOT)/videocodec/libvpx_internal/Android.mk
endif #BOARD_USES_MRST_OMX
