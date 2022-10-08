LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libaudiospdif
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES:= \
	FrameScanner.cpp \
	SPDIFEncoder.cpp

LOCAL_C_INCLUDES += $(call include-path-for, audio-utils)

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	liblog

include $(BUILD_SHARED_LIBRARY)
