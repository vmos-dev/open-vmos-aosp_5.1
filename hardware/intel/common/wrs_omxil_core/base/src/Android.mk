LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(TARGET_HAS_ISV),true)
LOCAL_CFLAGS += -DTARGET_HAS_ISV
endif

LOCAL_SRC_FILES := \
	cmodule.cpp \
	componentbase.cpp \
	portbase.cpp \
	portaudio.cpp \
	portvideo.cpp \
	portimage.cpp \
	portother.cpp

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libwrs_omxil_base

LOCAL_CPPFLAGS :=

LOCAL_LDFLAGS :=

LOCAL_CFLAGS += -Werror
ifeq ($(strip $(COMPONENT_SUPPORT_BUFFER_SHARING)), true)
LOCAL_CFLAGS += -DCOMPONENT_SUPPORT_BUFFER_SHARING
endif
ifeq ($(strip $(COMPONENT_SUPPORT_OPENCORE)), true)
LOCAL_CFLAGS += -DCOMPONENT_SUPPORT_OPENCORE
endif

LOCAL_C_INCLUDES := \
	$(WRS_OMXIL_CORE_ROOT)/utils/inc \
	$(WRS_OMXIL_CORE_ROOT)/base/inc \
	$(WRS_OMXIL_CORE_ROOT)/core/inc/khronos/openmax/include \
        $(call include-path-for, frameworks-native)/media/hardware \
        $(TOP)/frameworks/native/include/media/openmax

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libwrs_omxil_common

LOCAL_WHOLE_STATIC_LIBRARIES := \
	libwrs_omxil_utils \
	libwrs_omxil_base

LOCAL_SHARED_LIBRARIES := \
	libdl \
	liblog
LOCAL_CFLAGS += -Werror
include $(BUILD_SHARED_LIBRARY)
