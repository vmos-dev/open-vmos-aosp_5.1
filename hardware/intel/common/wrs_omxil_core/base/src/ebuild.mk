LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	cmodule.cpp \
	componentbase.cpp \
	portbase.cpp \
	portaudio.cpp \
	portvideo.cpp \
	portimage.cpp \
	portother.cpp

LOCAL_MODULE := libwrs_omxil_base

LOCAL_CPPFLAGS :=

LOCAL_LDFLAGS :=

LOCAL_C_INCLUDES := \
	$(WRS_OMXIL_CORE_ROOT)/utils/inc \
	$(WRS_OMXIL_CORE_ROOT)/base/inc \
	$(WRS_OMXIL_CORE_ROOT)/core/inc/khronos/openmax/include

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := libwrs_omxil_common

LOCAL_LDFLAGS := -ldl -lpthread

LOCAL_WHOLE_STATIC_LIBRARIES := \
	libwrs_omxil_utils \
	libwrs_omxil_base

include $(BUILD_SHARED_LIBRARY)
