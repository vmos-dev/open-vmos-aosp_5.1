ifneq ($(filter flo deb,$(TARGET_DEVICE)),)

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	voice_processing_descriptors.c

LOCAL_C_INCLUDES += \
	$(call include-path-for, audio-effects)

LOCAL_MODULE := libqcomvoiceprocessingdescriptors

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/soundfx

LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

endif