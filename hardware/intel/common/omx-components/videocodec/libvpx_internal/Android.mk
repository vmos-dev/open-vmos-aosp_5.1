LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

# libvpx
# if ARMv7 + NEON etc blah blah
include $(LOCAL_PATH)/libvpx.mk

# libwebm
#include $(LOCAL_PATH)/libwebm.mk
