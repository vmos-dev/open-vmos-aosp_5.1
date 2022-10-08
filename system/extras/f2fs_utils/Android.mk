# Copyright 2014 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)

ifeq ($(HOST_OS),linux)

include $(CLEAR_VARS)
LOCAL_MODULE := libf2fs_utils_host
LOCAL_SRC_FILES := f2fs_utils.c
LOCAL_STATIC_LIBRARIES := \
    libsparse_host \
    libz
LOCAL_C_INCLUDES := external/f2fs-tools/include external/f2fs-tools/mkfs
include $(BUILD_HOST_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := f2fs_ioutils.c
LOCAL_C_INCLUDES := external/f2fs-tools/include external/f2fs-tools/mkfs
LOCAL_STATIC_LIBRARIES := \
    libsparse_host \
    libext2_uuid_host \
    libz
LOCAL_MODULE := libf2fs_ioutils_host
include $(BUILD_HOST_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := f2fs_dlutils.c
LOCAL_C_INCLUDES := external/f2fs-tools/include external/f2fs-tools/mkfs
# Will attempt to dlopen("libf2fs_fmt_host_dyn")
LOCAL_LDLIBS := -ldl
LOCAL_MODULE := libf2fs_dlutils_host
include $(BUILD_HOST_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := make_f2fs_main.c
LOCAL_MODULE := make_f2fs
# libf2fs_dlutils_host will dlopen("libf2fs_fmt_host_dyn")
LOCAL_LDFLAGS := -ldl -rdynamic
# The following libf2fs_* are from system/extras/f2fs_utils,
# and do not use code in external/f2fs-tools.
LOCAL_STATIC_LIBRARIES := libf2fs_utils_host libf2fs_ioutils_host libf2fs_dlutils_host
LOCAL_REQUIRED_MODULES := libf2fs_fmt_host_dyn
LOCAL_STATIC_LIBRARIES += \
    libsparse_host \
    libz
include $(BUILD_HOST_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := libf2fs_dlutils
LOCAL_SRC_FILES := f2fs_dlutils.c
LOCAL_C_INCLUDES := external/f2fs-tools/include external/f2fs-tools/mkfs
LOCAL_SHARED_LIBRARIES := libdl
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libf2fs_dlutils_static
LOCAL_SRC_FILES := f2fs_dlutils.c
LOCAL_C_INCLUDES := external/f2fs-tools/include external/f2fs-tools/mkfs
LOCAL_SHARED_LIBRARIES := libdl
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libf2fs_utils_static
LOCAL_SRC_FILES := f2fs_utils.c
LOCAL_C_INCLUDES := external/f2fs-tools/include external/f2fs-tools/mkfs
LOCAL_STATIC_LIBRARIES := \
    libsparse_static
include $(BUILD_STATIC_LIBRARY)

endif

include $(CLEAR_VARS)
LOCAL_MODULE := libf2fs_sparseblock
LOCAL_SRC_FILES := f2fs_sparseblock.c
LOCAL_SHARED_LIBRARIES := libcutils
LOCAL_C_INCLUDES := external/f2fs-tools/include \
		system/core/include/log
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := f2fs_sparseblock
LOCAL_SRC_FILES := f2fs_sparseblock.c
LOCAL_SHARED_LIBRARIES := libcutils
LOCAL_C_INCLUDES := external/f2fs-tools/include \
		system/core/include/log
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := mkf2fsuserimg.sh
LOCAL_SRC_FILES := mkf2fsuserimg.sh
LOCAL_MODULE_CLASS := EXECUTABLES
# We don't need any additional suffix.
LOCAL_MODULE_SUFFIX :=
LOCAL_BUILT_MODULE_STEM := $(notdir $(LOCAL_SRC_FILES))
LOCAL_IS_HOST_MODULE := true
include $(BUILD_PREBUILT)


