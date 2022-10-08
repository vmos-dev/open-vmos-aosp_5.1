LOCAL_PATH := $(call my-dir)
include $(LOCAL_PATH)/../common.mk
include $(CLEAR_VARS)

LOCAL_MODULE                  := libqdutils
LOCAL_MODULE_TAGS             := optional
LOCAL_SHARED_LIBRARIES        := $(common_libs)
LOCAL_C_INCLUDES              := $(common_includes) $(kernel_includes)
LOCAL_CFLAGS                  := $(common_flags) -DLOG_TAG=\"qdutils\"
LOCAL_ADDITIONAL_DEPENDENCIES := $(common_deps)
LOCAL_SRC_FILES               := profiler.cpp mdp_version.cpp \
                                 idle_invalidator.cpp \
                                 comptype.cpp
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_COPY_HEADERS_TO           := $(common_header_export_path)
LOCAL_COPY_HEADERS              := qdMetaData.h
LOCAL_MODULE_PATH               := $(TARGET_OUT_SHARED_LIBRARIES)
LOCAL_SHARED_LIBRARIES          := liblog libcutils
LOCAL_C_INCLUDES                := $(common_includes)
LOCAL_ADDITIONAL_DEPENDENCIES   := $(common_deps)
LOCAL_SRC_FILES                 := qdMetaData.cpp
LOCAL_CFLAGS                    := $(common_flags)
LOCAL_CFLAGS                    += -DLOG_TAG=\"DisplayMetaData\"
LOCAL_MODULE_TAGS               := optional
LOCAL_MODULE                    := libqdMetaData
include $(BUILD_SHARED_LIBRARY)

