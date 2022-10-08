LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	list.c \
	queue.c \
	module.c \
	thread.cpp \
	workqueue.cpp

LOCAL_MODULE := libwrs_omxil_utils

LOCAL_CFLAGS :=

LOCAL_CPPFLAGS :=

LOCAL_LDFLAGS :=

LOCAL_SHARED_LIBRARIES :=

LOCAL_C_INCLUDES := \
	$(WRS_OMXIL_CORE_ROOT)/utils/inc

include $(BUILD_STATIC_LIBRARY)
