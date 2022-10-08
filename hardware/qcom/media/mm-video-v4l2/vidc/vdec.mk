ifneq ($(BUILD_TINY_ANDROID),true)

ROOT_DIR := $(call my-dir)
OMX_VIDEO_PATH := $(ROOT_DIR)/..

include $(CLEAR_VARS)
LOCAL_PATH:= $(ROOT_DIR)

# ---------------------------------------------------------------------------------
# 				Common definitons
# ---------------------------------------------------------------------------------

libOmxVdec-def := -D__alignx\(x\)=__attribute__\(\(__aligned__\(x\)\)\)
libOmxVdec-def += -D__align=__alignx
libOmxVdec-def += -Dinline=__inline
libOmxVdec-def += -g -O3
libOmxVdec-def += -DIMAGE_APPS_PROC
libOmxVdec-def += -D_ANDROID_
libOmxVdec-def += -DCDECL
libOmxVdec-def += -DT_ARM
libOmxVdec-def += -DNO_ARM_CLZ
libOmxVdec-def += -UENABLE_DEBUG_LOW
libOmxVdec-def += -DENABLE_DEBUG_HIGH
libOmxVdec-def += -DENABLE_DEBUG_ERROR
libOmxVdec-def += -UINPUT_BUFFER_LOG
libOmxVdec-def += -UOUTPUT_BUFFER_LOG
libOmxVdec-def += -Wno-parentheses
ifeq ($(TARGET_BOARD_PLATFORM),msm8660)
libOmxVdec-def += -DMAX_RES_1080P
libOmxVdec-def += -DPROCESS_EXTRADATA_IN_OUTPUT_PORT
libOmxVdec-def += -DTEST_TS_FROM_SEI
endif
ifeq ($(TARGET_BOARD_PLATFORM),msm8960)
libOmxVdec-def += -DMAX_RES_1080P
libOmxVdec-def += -DMAX_RES_1080P_EBI
libOmxVdec-def += -DPROCESS_EXTRADATA_IN_OUTPUT_PORT
libOmxVdec-def += -D_MSM8960_
endif
ifeq ($(TARGET_BOARD_PLATFORM),msm8974)
libOmxVdec-def += -DMAX_RES_1080P
libOmxVdec-def += -DMAX_RES_1080P_EBI
libOmxVdec-def += -DPROCESS_EXTRADATA_IN_OUTPUT_PORT
libOmxVdec-def += -D_MSM8974_
libOmxVdec-def += -D_HEVC_USE_ADSP_HEAP_
endif
ifeq ($(TARGET_BOARD_PLATFORM),msm7627a)
libOmxVdec-def += -DMAX_RES_720P
endif
ifeq ($(TARGET_BOARD_PLATFORM),msm7630_surf)
libOmxVdec-def += -DMAX_RES_720P
endif
ifeq ($(TARGET_BOARD_PLATFORM),msm8610)
libOmxVdec-def += -DMAX_RES_1080P
libOmxVdec-def += -DMAX_RES_1080P_EBI
libOmxVdec-def += -DPROCESS_EXTRADATA_IN_OUTPUT_PORT
libOmxVdec-def += -DSMOOTH_STREAMING_DISABLED
libOmxVdec-def += -DH264_PROFILE_LEVEL_CHECK
libOmxVdec-def += -D_MSM8974_
endif
ifeq ($(TARGET_BOARD_PLATFORM),msm8226)
libOmxVdec-def += -DMAX_RES_1080P
libOmxVdec-def += -DMAX_RES_1080P_EBI
libOmxVdec-def += -DPROCESS_EXTRADATA_IN_OUTPUT_PORT
libOmxVdec-def += -D_MSM8974_
endif
ifeq ($(TARGET_BOARD_PLATFORM),msm8084)
libOmxVdec-def += -DMAX_RES_1080P
libOmxVdec-def += -DMAX_RES_1080P_EBI
libOmxVdec-def += -DPROCESS_EXTRADATA_IN_OUTPUT_PORT
libOmxVdec-def += -D_MSM8974_
libOmxVdec-def += -D_ION_HEAP_MASK_COMPATIBILITY_WA
endif
ifeq ($(TARGET_BOARD_PLATFORM),mpq8092)
libOmxVdec-def += -DMAX_RES_1080P
libOmxVdec-def += -DMAX_RES_1080P_EBI
libOmxVdec-def += -DPROCESS_EXTRADATA_IN_OUTPUT_PORT
libOmxVdec-def += -D_MSM8974_
endif
libOmxVdec-def += -D_ANDROID_ICS_

ifeq ($(TARGET_USES_ION),true)
libOmxVdec-def += -DUSE_ION
endif

# ---------------------------------------------------------------------------------
# 			Make the Shared library (libOmxVdec)
# ---------------------------------------------------------------------------------

include $(CLEAR_VARS)
LOCAL_PATH:= $(ROOT_DIR)

libmm-vdec-inc          := bionic/libc/include
libmm-vdec-inc          += bionic/libstdc++/include
libmm-vdec-inc          += $(LOCAL_PATH)/vdec/inc
libmm-vdec-inc          += $(OMX_VIDEO_PATH)/vidc/common/inc
libmm-vdec-inc          += hardware/qcom/media/mm-core/inc
#DRM include - Interface which loads the DRM library
libmm-vdec-inc	        += $(OMX_VIDEO_PATH)/DivxDrmDecrypt/inc
libmm-vdec-inc          += $(TARGET_OUT_HEADERS)/qcom/display
libmm-vdec-inc          += $(TARGET_OUT_HEADERS)/adreno
libmm-vdec-inc          += frameworks/native/include/media/openmax
libmm-vdec-inc          += frameworks/native/include/media/hardware
libmm-vdec-inc          += $(vdec-inc)
libmm-vdec-inc      += hardware/qcom/media/libc2dcolorconvert
libmm-vdec-inc      += frameworks/av/include/media/stagefright


LOCAL_MODULE                    := libOmxVdec
LOCAL_MODULE_TAGS               := optional
LOCAL_CFLAGS                    := $(libOmxVdec-def)
LOCAL_C_INCLUDES                += $(libmm-vdec-inc)

LOCAL_PRELINK_MODULE    := false
LOCAL_SHARED_LIBRARIES  := liblog libutils libbinder libcutils libdl

LOCAL_SHARED_LIBRARIES  += libdivxdrmdecrypt
LOCAL_SHARED_LIBRARIES  += libqdMetaData

LOCAL_SRC_FILES         := vdec/src/frameparser.cpp
LOCAL_SRC_FILES         += vdec/src/h264_utils.cpp
LOCAL_SRC_FILES         += vdec/src/ts_parser.cpp
LOCAL_SRC_FILES         += vdec/src/mp4_utils.cpp
LOCAL_SRC_FILES         += vdec/src/hevc_utils.cpp
ifneq ($(filter msm8974 msm8610 msm8226 msm8084 mpq8092,$(TARGET_BOARD_PLATFORM)),)
LOCAL_SRC_FILES         += vdec/src/omx_vdec_msm8974.cpp
else
LOCAL_SHARED_LIBRARIES  += libhardware
libmm-vdec-inc          += $(TARGET_OUT_HEADERS)/qcom/display
LOCAL_SRC_FILES         += vdec/src/power_module.cpp
LOCAL_SRC_FILES         += vdec/src/omx_vdec.cpp
endif

LOCAL_SRC_FILES         += common/src/extra_data_handler.cpp
LOCAL_SRC_FILES         += common/src/vidc_color_converter.cpp

include $(BUILD_SHARED_LIBRARY)


# ---------------------------------------------------------------------------------
# 			Make the Shared library (libOmxVdecHevc)
# ---------------------------------------------------------------------------------

include $(CLEAR_VARS)
LOCAL_PATH:= $(ROOT_DIR)

ifneq ($(filter msm8974 msm8610 msm8084 mpq8092,$(TARGET_BOARD_PLATFORM)),)

LOCAL_MODULE                    := libOmxVdecHevc
LOCAL_MODULE_TAGS               := optional
LOCAL_CFLAGS                    := $(libOmxVdec-def)
LOCAL_C_INCLUDES                += $(libmm-vdec-inc)

LOCAL_PRELINK_MODULE    := false
LOCAL_SHARED_LIBRARIES  := liblog libutils libbinder libcutils libdl

LOCAL_SHARED_LIBRARIES  += libdivxdrmdecrypt
LOCAL_SHARED_LIBRARIES  += libqdMetaData

LOCAL_SRC_FILES         := vdec/src/frameparser.cpp
LOCAL_SRC_FILES         += vdec/src/h264_utils.cpp
LOCAL_SRC_FILES         += vdec/src/ts_parser.cpp
LOCAL_SRC_FILES         += vdec/src/mp4_utils.cpp

LOCAL_SRC_FILES         += vdec/src/omx_vdec_hevc.cpp
LOCAL_SRC_FILES         += vdec/src/hevc_utils.cpp

LOCAL_SRC_FILES         += common/src/extra_data_handler.cpp
LOCAL_SRC_FILES         += common/src/vidc_color_converter.cpp

#include $(BUILD_SHARED_LIBRARY)

endif

# ---------------------------------------------------------------------------------
# 			Make the apps-test (mm-vdec-omx-test)
# ---------------------------------------------------------------------------------
include $(CLEAR_VARS)

mm-vdec-test-inc    := hardware/qcom/media/mm-core/inc
mm-vdec-test-inc    += $(LOCAL_PATH)/vdec/inc
mm-vdec-test-inc    += $(vdec-inc)

LOCAL_MODULE                    := mm-vdec-omx-test
LOCAL_MODULE_TAGS               := optional
LOCAL_CFLAGS                    := $(libOmxVdec-def)
LOCAL_C_INCLUDES                := $(mm-vdec-test-inc)

LOCAL_PRELINK_MODULE      := false
LOCAL_SHARED_LIBRARIES    := libutils libOmxCore libOmxVdec libbinder libcutils

LOCAL_SRC_FILES           := vdec/src/queue.c
LOCAL_SRC_FILES           += vdec/test/omx_vdec_test.cpp

#include $(BUILD_EXECUTABLE)

# ---------------------------------------------------------------------------------
# 			Make the driver-test (mm-video-driver-test)
# ---------------------------------------------------------------------------------
include $(CLEAR_VARS)

mm-vdec-drv-test-inc    := hardware/qcom/media/mm-core/inc
mm-vdec-drv-test-inc    += $(LOCAL_PATH)/vdec/inc
mm-vdec-drv-test-inc    += $(vdec-inc)

LOCAL_MODULE                    := mm-video-driver-test
LOCAL_MODULE_TAGS               := optional
LOCAL_CFLAGS                    := $(libOmxVdec-def)
LOCAL_C_INCLUDES                := $(mm-vdec-drv-test-inc)
LOCAL_PRELINK_MODULE            := false

LOCAL_SRC_FILES                 := vdec/src/message_queue.c
LOCAL_SRC_FILES                 += vdec/test/decoder_driver_test.c

#include $(BUILD_EXECUTABLE)

endif #BUILD_TINY_ANDROID

# ---------------------------------------------------------------------------------
#                END
# ---------------------------------------------------------------------------------
