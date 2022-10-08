ifneq ($(BUILD_TINY_ANDROID),true)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

OMXCORE_CFLAGS := -g -O3 -DVERBOSE
OMXCORE_CFLAGS += -O0 -fno-inline -fno-short-enums
OMXCORE_CFLAGS += -D_ANDROID_
OMXCORE_CFLAGS += -U_ENABLE_QC_MSG_LOG_

#===============================================================================
#             Figure out the targets
#===============================================================================

ifeq ($(TARGET_BOARD_PLATFORM),msm7627a)
MM_CORE_TARGET = 7627A
else ifeq ($(TARGET_BOARD_PLATFORM),msm7630_surf)
MM_CORE_TARGET = 7630
else ifeq ($(TARGET_BOARD_PLATFORM),msm8660)
MM_CORE_TARGET = 8660
#Comment out following line to disable drm.play component
OMXCORE_CFLAGS += -DENABLE_DRMPLAY
else ifeq ($(TARGET_BOARD_PLATFORM),msm8960)
MM_CORE_TARGET = 8960
else ifeq ($(TARGET_BOARD_PLATFORM),msm8974)
MM_CORE_TARGET = 8974
else ifeq ($(TARGET_BOARD_PLATFORM),msm8610)
MM_CORE_TARGET = 8610
else ifeq ($(TARGET_BOARD_PLATFORM),msm8226)
MM_CORE_TARGET = 8226
else ifeq ($(TARGET_BOARD_PLATFORM),msm8916)
MM_CORE_TARGET = 8916
else ifeq ($(TARGET_BOARD_PLATFORM),msm8084)
MM_CORE_TARGET = 8084
else ifeq ($(TARGET_BOARD_PLATFORM),mpq8092)
MM_CORE_TARGET = 8092
else ifeq ($(TARGET_BOARD_PLATFORM),plutonium)
MM_CORE_TARGET = plutonium
else
MM_CORE_TARGET = default
endif

#===============================================================================
#             Deploy the headers that can be exposed
#===============================================================================

LOCAL_COPY_HEADERS_TO   := mm-core/omxcore
LOCAL_COPY_HEADERS      := inc/OMX_Audio.h
LOCAL_COPY_HEADERS      += inc/OMX_Component.h
LOCAL_COPY_HEADERS      += inc/OMX_ContentPipe.h
LOCAL_COPY_HEADERS      += inc/OMX_Core.h
LOCAL_COPY_HEADERS      += inc/OMX_Image.h
LOCAL_COPY_HEADERS      += inc/OMX_Index.h
LOCAL_COPY_HEADERS      += inc/OMX_IVCommon.h
LOCAL_COPY_HEADERS      += inc/OMX_Other.h
LOCAL_COPY_HEADERS      += inc/OMX_QCOMExtns.h
LOCAL_COPY_HEADERS      += inc/OMX_Types.h
LOCAL_COPY_HEADERS      += inc/OMX_Video.h
LOCAL_COPY_HEADERS      += inc/qc_omx_common.h
LOCAL_COPY_HEADERS      += inc/qc_omx_component.h
LOCAL_COPY_HEADERS      += inc/qc_omx_msg.h
LOCAL_COPY_HEADERS      += inc/QOMX_AudioExtensions.h
LOCAL_COPY_HEADERS      += inc/QOMX_AudioIndexExtensions.h
LOCAL_COPY_HEADERS      += inc/OMX_CoreExt.h
LOCAL_COPY_HEADERS      += inc/QOMX_CoreExtensions.h
LOCAL_COPY_HEADERS      += inc/QOMX_FileFormatExtensions.h
LOCAL_COPY_HEADERS      += inc/QOMX_IVCommonExtensions.h
LOCAL_COPY_HEADERS      += inc/QOMX_SourceExtensions.h
LOCAL_COPY_HEADERS      += inc/QOMX_VideoExtensions.h
LOCAL_COPY_HEADERS      += inc/OMX_IndexExt.h
LOCAL_COPY_HEADERS      += inc/OMX_VideoExt.h
LOCAL_COPY_HEADERS      += inc/QOMX_StreamingExtensions.h
LOCAL_COPY_HEADERS      += inc/QCMediaDefs.h
LOCAL_COPY_HEADERS      += inc/QCMetaData.h

#===============================================================================
#             LIBRARY for Android apps
#===============================================================================

LOCAL_C_INCLUDES        := $(LOCAL_PATH)/src/common
LOCAL_C_INCLUDES        += $(LOCAL_PATH)/inc
LOCAL_PRELINK_MODULE    := false
LOCAL_MODULE            := libOmxCore
LOCAL_MODULE_TAGS       := optional
LOCAL_SHARED_LIBRARIES  := liblog libdl
LOCAL_CFLAGS            := $(OMXCORE_CFLAGS)

LOCAL_SRC_FILES         := src/common/omx_core_cmp.cpp
LOCAL_SRC_FILES         += src/common/qc_omx_core.c
ifneq (,$(filter msm8916 plutonium,$(TARGET_BOARD_PLATFORM)))
LOCAL_SRC_FILES         += src/$(MM_CORE_TARGET)/registry_table_android.c
else
LOCAL_SRC_FILES         += src/$(MM_CORE_TARGET)/qc_registry_table_android.c
endif

include $(BUILD_SHARED_LIBRARY)

#===============================================================================
#             LIBRARY for command line test apps
#===============================================================================

include $(CLEAR_VARS)

LOCAL_C_INCLUDES        := $(LOCAL_PATH)/src/common
LOCAL_C_INCLUDES        += $(LOCAL_PATH)/inc
LOCAL_PRELINK_MODULE    := false
LOCAL_MODULE            := libmm-omxcore
LOCAL_MODULE_TAGS       := optional
LOCAL_SHARED_LIBRARIES  := liblog libdl
LOCAL_CFLAGS            := $(OMXCORE_CFLAGS)

LOCAL_SRC_FILES         := src/common/omx_core_cmp.cpp
LOCAL_SRC_FILES         += src/common/qc_omx_core.c
ifneq (,$(filter msm8916 plutonium,$(TARGET_BOARD_PLATFORM)))
LOCAL_SRC_FILES         += src/$(MM_CORE_TARGET)/registry_table.c
else
LOCAL_SRC_FILES         += src/$(MM_CORE_TARGET)/qc_registry_table.c
endif

include $(BUILD_SHARED_LIBRARY)

endif #BUILD_TINY_ANDROID
