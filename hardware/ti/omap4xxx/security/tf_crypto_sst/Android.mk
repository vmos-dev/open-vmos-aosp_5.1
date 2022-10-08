ifeq ($(TARGET_BOARD_PLATFORM),omap4)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false
LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := \
	lib_object.c \
	lib_mutex_linux.c \
	sst_stub.c \
	mtc.c \
	pkcs11_global.c \
	pkcs11_object.c \
	pkcs11_session.c

LOCAL_CFLAGS += -DLINUX
LOCAL_CFLAGS += -D__ANDROID32__

ifdef S_VERSION_BUILD
LOCAL_CFLAGS += -DS_VERSION_BUILD=$(S_VERSION_BUILD)
endif

LOCAL_CFLAGS += -I $(LOCAL_PATH)/../tf_sdk/include/

LOCAL_MODULE:= libtf_crypto_sst
LOCAL_STATIC_LIBRARIES := libtee_client_api_driver
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
endif
