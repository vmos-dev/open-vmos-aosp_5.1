LOCAL_PATH := $(call my-dir)

ifneq ($(BOARD_USES_GENERIC_INVENSENSE),false)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional

LOCAL_MODULE := libmlplatform
#modify these to point to the mpl source installation
MLSDK_ROOT = .
MLPLATFORM_DIR = $(MLSDK_ROOT)/platform/linux

LOCAL_CFLAGS += -D_REENTRANT -DLINUX -DANDROID
LOCAL_CFLAGS += -DCONFIG_MPU_SENSORS_MPU3050
LOCAL_CFLAGS += -I$(LOCAL_PATH)/$(MLSDK_ROOT)/platform/include
LOCAL_CFLAGS += -I$(LOCAL_PATH)/$(MLSDK_ROOT)/platform/include/linux
LOCAL_CFLAGS += -I$(LOCAL_PATH)/$(MLPLATFORM_DIR)
LOCAL_CFLAGS += -I$(LOCAL_PATH)/$(MLPLATFORM_DIR)/kernel
LOCAL_CFLAGS += -I$(LOCAL_PATH)/$(MLSDK_ROOT)/mllite

ML_SOURCES := \
    $(MLPLATFORM_DIR)/mlos_linux.c \
    $(MLPLATFORM_DIR)/mlsl_linux_mpu.c

LOCAL_SRC_FILES := $(ML_SOURCES)

LOCAL_SHARED_LIBRARIES := liblog libm libutils libcutils
LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libmllite
LOCAL_MODULE_TAGS := optional
#modify these to point to the mpl source installation
MLSDK_ROOT = .
MLPLATFORM_DIR = $(MLSDK_ROOT)/platform
MLLITE_DIR = $(MLSDK_ROOT)/mllite
MPL_DIR = $(MLSDK_ROOT)/mldmp

LOCAL_CFLAGS += -DNDEBUG
LOCAL_CFLAGS += -D_REENTRANT -DLINUX -DANDROID
LOCAL_CFLAGS += -DCONFIG_MPU_SENSORS_MPU3050
LOCAL_CFLAGS += -DUNICODE -D_UNICODE -DSK_RELEASE
LOCAL_CFLAGS += -DI2CDEV=\"/dev/mpu\"
LOCAL_CFLAGS += -I$(LOCAL_PATH)/$(MPL_DIR) -I$(LOCAL_PATH)/$(MLLITE_DIR) -I$(LOCAL_PATH)/$(MLPLATFORM_DIR)/include
LOCAL_CFLAGS += -I$(LOCAL_PATH)/$(MLSDK_ROOT)/mlutils -I$(LOCAL_PATH)/$(MLSDK_ROOT)/mlapps/common
LOCAL_CFLAGS += -I$(LOCAL_PATH)/$(MLSDK_ROOT)/platform/include/linux
LOCAL_CFLAGS += -I$(LOCAL_PATH)/$(MLSDK_ROOT)/mllite/akmd
LOCAL_CFLAGS += -I$(LOCAL_PATH)/$(MLPLATFORM_DIR)/linux

# optionally apply the compass filter. this is set in
# BoardConfig.mk
ifeq ($(BOARD_INVENSENSE_APPLY_COMPASS_NOISE_FILTER),true)
LOCAL_CFLAGS += -DAPPLY_COMPASS_FILTER
endif

ML_SOURCES = \
        $(MLLITE_DIR)/accel.c \
        $(MLLITE_DIR)/compass.c \
        $(MLLITE_DIR)/pressure.c \
        $(MLLITE_DIR)/mldl_cfg_mpu.c \
        $(MLLITE_DIR)/dmpDefault.c \
        $(MLLITE_DIR)/ml.c \
	$(MLLITE_DIR)/mlarray.c \
	$(MLLITE_DIR)/mlarray_legacy.c \
        $(MLLITE_DIR)/mlFIFO.c \
        $(MLLITE_DIR)/mlFIFOHW.c \
        $(MLLITE_DIR)/mlMathFunc.c \
        $(MLLITE_DIR)/ml_stored_data.c \
        $(MLLITE_DIR)/mlcontrol.c \
        $(MLLITE_DIR)/mldl.c \
        $(MLLITE_DIR)/mldmp.c \
        $(MLLITE_DIR)/mlstates.c \
        $(MLLITE_DIR)/mlsupervisor.c \
        $(MLLITE_DIR)/mlBiasNoMotion.c \
        $(MLLITE_DIR)/mlSetGyroBias.c \
        \
        $(MLLITE_DIR)/ml_mputest.c \
        $(MLSDK_ROOT)/mlutils/mputest.c \
        $(MLSDK_ROOT)/mlutils/checksum.c


ifeq ($(HARDWARE),M_HW)
    ML_SOURCES += $(MLLITE_DIR)/accel/mantis.c
endif

LOCAL_SRC_FILES := $(ML_SOURCES)
LOCAL_SHARED_LIBRARIES := libm libutils libcutils liblog libmlplatform
LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)

#This makes an .so from our .a
#include $(CLEAR_VARS)
#LOCAL_MODULE := libmpl
#LOCAL_MODULE_TAGS := optional
#LOCAL_SRC_FILES := mlsdk/mldmp/mpl/android/libmpl.a
#LOCAL_SHARED_LIBRARIES := libm libutils libcutils liblog libmlplatform libmllite
#LOCAL_WHOLE_STATIC_LIBRARIES := libmpl
#LOCAL_PREBUILT_LIBS := mlsdk/mldmp/mpl/android/libmpl.a
#LOCAL_PRELINK_MODULE := false
#include $(BUILD_SHARED_LIBRARY)
#include $(BUILD_MULTI_PREBUILT)

endif
