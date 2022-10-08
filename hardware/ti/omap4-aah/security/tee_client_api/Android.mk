# Only applicable for OMAP4 and OMAP5 boards.
# First eliminate OMAP3 and then ensure that this is not used
# for customer boards
ifneq ($(TARGET_BOARD_PLATFORM),omap3)
ifeq ($(findstring omap, $(TARGET_BOARD_PLATFORM)),omap)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false
LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES:= \
	tee_client_api_linux_driver.c

ifdef S_VERSION_BUILD
LOCAL_CFLAGS += -DS_VERSION_BUILD=$(S_VERSION_BUILD)
endif

LOCAL_LDLIBS += -llog

LOCAL_CFLAGS += -DLINUX
LOCAL_CFLAGS += -DANDROID
LOCAL_CFLAGS += -I $(LOCAL_PATH)/../tf_sdk/include/

LOCAL_MODULE:= libtee_client_api_driver
LOCAL_MODULE_TAGS := optional

include $(BUILD_STATIC_LIBRARY)
endif
endif
