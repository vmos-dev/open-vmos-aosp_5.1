LOCAL_PATH:=$(call my-dir)
USE_BIONIC_HEADER:=true
include $(CLEAR_VARS)

ifeq ($(call is-board-platform,msm8960),true)
LOCAL_CFLAGS:= \
        -DAMSS_VERSION=$(AMSS_VERSION) \
        $(mmcamera_debug_defines) \
        $(mmcamera_debug_cflags) \
        $(USE_SERVER_TREE) \

ifneq ($(strip $(USE_BIONIC_HEADER)),true)
LOCAL_CFLAGS += -include $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include/linux/ion.h
endif

ifeq ($(strip $(TARGET_USES_ION)),true)
LOCAL_CFLAGS += -DUSE_ION
endif

LOCAL_CFLAGS += -D_ANDROID_

LOCAL_SRC_FILES:= \
        src/mm_qcamera_main_menu.c \
        src/mm_qcamera_display.c \
        src/mm_qcamera_app.c \
        src/mm_qcamera_snapshot.c \
        src/mm_qcamera_video.c \
        src/mm_qcamera_preview.c \
        src/mm_qcamera_rdi.c \
        src/mm_qcamera_unit_test.c \
        src/mm_qcamera_dual_test.c

LOCAL_C_INCLUDES:=$(LOCAL_PATH)/inc
LOCAL_C_INCLUDES+= \
        $(TARGET_OUT_INTERMEDIATES)/include/mm-camera-interface_badger \
        $(LOCAL_PATH)/../mm-camera-interface/inc \
        $(LOCAL_PATH)/../common \
        $(LOCAL_PATH)/../../../ \
        $(LOCAL_PATH)/../../../inc

ifneq ($(strip $(USE_BIONIC_HEADER)),true)
LOCAL_C_INCLUDES+= $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include/media
LOCAL_C_INCLUDES+= $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr
endif

ifeq ($(call is-board-platform,msm8960),true)
LOCAL_CFLAGS += -DCAMERA_ION_HEAP_ID=ION_CP_MM_HEAP_ID
else
LOCAL_CFLAGS += -DCAMERA_ION_HEAP_ID=ION_CAMERA_HEAP_ID
endif

LOCAL_SHARED_LIBRARIES:= \
         libcutils liblog libdl

LOCAL_MODULE:= mm-qcamera-app-badger

LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
endif
