LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	wrs_omxcore.cpp

LOCAL_MODULE := libwrs_omxil_core

LOCAL_CPPFLAGS :=

LOCAL_LDFLAGS :=

LOCAL_SHARED_LIBRARIES := \
	libwrs_omxil_common

LOCAL_C_INCLUDES := \
	$(WRS_OMXIL_CORE_ROOT)/utils/inc \
	$(WRS_OMXIL_CORE_ROOT)/base/inc \
	$(WRS_OMXIL_CORE_ROOT)/core/inc/khronos/openmax/include

include $(BUILD_SHARED_LIBRARY)
