LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libaudioutils
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES:= \
	channels.c \
	fixedfft.cpp.arm \
	format.c \
	minifloat.c \
	primitives.c \
	resampler.c \
	echo_reference.c

LOCAL_C_INCLUDES += $(call include-path-for, speex)
LOCAL_C_INCLUDES += \
	$(call include-path-for, speex) \
	$(call include-path-for, audio-utils)

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	liblog \
	libspeexresampler

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libaudioutils
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := \
	channels.c \
	format.c \
	minifloat.c \
	primitives.c
LOCAL_C_INCLUDES += \
	$(call include-path-for, audio-utils)
include $(BUILD_HOST_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := libsndfile
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	tinysndfile.c

LOCAL_C_INCLUDES += \
	$(call include-path-for, audio-utils)

#LOCAL_SHARED_LIBRARIES := libaudioutils

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := libsndfile
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	tinysndfile.c

LOCAL_C_INCLUDES += \
	$(call include-path-for, audio-utils)

#LOCAL_SHARED_LIBRARIES := libaudioutils

include $(BUILD_HOST_STATIC_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))

