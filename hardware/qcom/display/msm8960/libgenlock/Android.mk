LOCAL_PATH := $(call my-dir)
include $(LOCAL_PATH)/../common.mk
include $(CLEAR_VARS)

LOCAL_MODULE                  := libgenlock
LOCAL_MODULE_TAGS             := optional
LOCAL_C_INCLUDES              := $(common_includes)
LOCAL_SHARED_LIBRARIES        := liblog libcutils
LOCAL_CFLAGS                  := $(common_flags) -DLOG_TAG=\"qdgenlock\"
LOCAL_ADDITIONAL_DEPENDENCIES := $(common_deps)
LOCAL_SRC_FILES               := genlock.cpp

include $(BUILD_SHARED_LIBRARY)
