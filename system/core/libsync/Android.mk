LOCAL_PATH:= $(call my-dir)

ifeq ($(PRODUCT_OPENSRC_USE_PREBUILT),yes) # prebuilt compile

include $(CLEAR_VARS)

LOCAL_MODULE := libsync

LOCAL_MULTILIB := both
LOCAL_SRC_FILES_arm := prebuilt/arm/libsync.so
LOCAL_SRC_FILES_arm64 := prebuilt/arm64/libsync.so

LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES

LOCAL_UNINSTALLABLE_MODULE := true

$(shell cp -rf $(LOCAL_PATH)/prebuilt/arm/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)_result $(TARGET_OUT)/lib/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX))
ifeq ($(TARGET_IS_64_BIT),true)
$(shell cp -rf $(LOCAL_PATH)/prebuilt/arm64/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)_result $(TARGET_OUT)/lib64/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX))
endif

include $(BUILD_PREBUILT)

else

include $(CLEAR_VARS)
LOCAL_SRC_FILES := sync.c
LOCAL_MODULE := libsync
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := liblog libdl
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include
LOCAL_CFLAGS := -Werror
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := sync.c sync_test.c
LOCAL_MODULE := sync_test
LOCAL_MODULE_TAGS := optional tests
LOCAL_SHARED_LIBRARIES := liblog libdl
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_CFLAGS := -Werror
include $(BUILD_EXECUTABLE)

endif
