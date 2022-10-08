LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	camera_metadata.c

LOCAL_C_INCLUDES:= \
	system/media/camera/include \
	system/media/private/camera/include

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	liblog

LOCAL_MODULE := libcamera_metadata
LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS += \
	-Wall \
	-fvisibility=hidden \
	-std=c99

ifneq ($(filter userdebug eng,$(TARGET_BUILD_VARIANT)),)
    # Enable assert()
    LOCAL_CFLAGS += -UNDEBUG -DLOG_NDEBUG=1
endif

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/../include

include $(BUILD_SHARED_LIBRARY)
