LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_PREBUILT_LIBS := lib/libIMCdownload.a
LOCAL_MODULE_TAGS := eng

include $(BUILD_MULTI_PREBUILT)

############################################################
include $(CLEAR_VARS)

LOCAL_PREBUILT_LIBS := lib/libPrgHandler.a
LOCAL_MODULE_TAGS := eng

include $(BUILD_MULTI_PREBUILT)

############################################################
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := eng
LOCAL_C_INCLUDES += bootable/recovery \
                    device/asus/tilapia/recovery/lib

LOCAL_SRC_FILES := recovery_updater.c

LOCAL_STATIC_LIBRARIES := libIMCdownload libPrgHandler

# should match TARGET_RECOVERY_UPDATER_LIBS set in BoardConfig.mk
LOCAL_MODULE := librecovery_updater_tilapia

include $(BUILD_STATIC_LIBRARY)
