LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

$(call add-prebuilt-files, ETC, 01_Vendor_ti_omx.cfg)

include $(TI_OMX_TOP)/core_plugin/omx_core_plugin/Android.mk
