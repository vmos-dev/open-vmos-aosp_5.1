LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

WRS_OMXIL_CORE_ROOT := $(LOCAL_PATH)

# core
-include $(WRS_OMXIL_CORE_ROOT)/core/src/ebuild.mk

# base class
-include $(WRS_OMXIL_CORE_ROOT)/base/src/ebuild.mk

# utility
-include $(WRS_OMXIL_CORE_ROOT)/utils/src/ebuild.mk
