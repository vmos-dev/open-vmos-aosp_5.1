ifeq ($(strip $(BOARD_USES_WRS_OMXIL_CORE)),true)
LOCAL_PATH := $(call my-dir)

ifeq ($(strip $(USE_VIDEO_EFFECT)),true)
LOCAL_C_FLAGS := -DUSE_VIDEO_EFFECT
endif

################################################################################

include $(CLEAR_VARS)

ifeq ($(TARGET_HAS_ISV),true)
LOCAL_CFLAGS += -DTARGET_HAS_ISV
endif

LOCAL_SHARED_LIBRARIES := \
    libwrs_omxil_common \
    libva_videodecoder \
    liblog \
    libva \
    libva-android

LOCAL_C_INCLUDES := \
    $(TARGET_OUT_HEADERS)/wrs_omxil_core \
    $(TARGET_OUT_HEADERS)/khronos/openmax \
    $(TARGET_OUT_HEADERS)/libmix_videodecoder \
    $(TARGET_OUT_HEADERS)/libva \
    $(call include-path-for, frameworks-native)/media/hardware \
    $(call include-path-for, frameworks-native)/media/openmax \
    $(call include-path-for, libhardware)

PLATFORM_USE_GEN_HW := \
    baytrail \
    cherrytrail

ifneq ($(filter $(TARGET_BOARD_PLATFORM),$(PLATFORM_USE_GEN_HW)),)
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/ufo
endif

LOCAL_SRC_FILES := \
    OMXComponentCodecBase.cpp \
    OMXVideoDecoderBase.cpp \
    OMXVideoDecoderAVC.cpp

LOCAL_CFLAGS += -Werror
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libOMXVideoDecoderAVC
ifeq ($(TARGET_BOARD_PLATFORM),clovertrail)
LOCAL_CFLAGS += -DVED_TILING
endif

ifeq ($(TARGET_BOARD_PLATFORM),merrifield)
LOCAL_CFLAGS += -DVED_TILING
endif

ifeq ($(TARGET_BOARD_PLATFORM),moorefield)
LOCAL_CFLAGS += -DVED_TILING
endif

ifeq ($(TARGET_VPP_USE_GEN),true)
LOCAL_CFLAGS += -DDEINTERLACE_EXT
endif

ifeq ($(TARGET_BOARD_PLATFORM),baytrail)
LOCAL_CFLAGS += -DUSE_GEN_HW
endif

include $(BUILD_SHARED_LIBRARY)

################################################################################

PLATFORM_SUPPORT_VP8 := \
    merrifield \
    morganfield \
    moorefield \
    baytrail \
    cherrytrail

ifneq ($(filter $(TARGET_BOARD_PLATFORM),$(PLATFORM_SUPPORT_VP8)),)
include $(CLEAR_VARS)

ifeq ($(TARGET_HAS_ISV),true)
LOCAL_CFLAGS += -DTARGET_HAS_ISV
endif

LOCAL_SHARED_LIBRARIES := \
    libwrs_omxil_common \
    libva_videodecoder \
    liblog \
    libva \
    libva-android

LOCAL_C_INCLUDES := \
    $(TARGET_OUT_HEADERS)/wrs_omxil_core \
    $(TARGET_OUT_HEADERS)/khronos/openmax \
    $(TARGET_OUT_HEADERS)/libmix_videodecoder \
    $(TARGET_OUT_HEADERS)/libva \
    $(call include-path-for, frameworks-native)/media/hardware \
    $(call include-path-for, frameworks-native)/media/openmax \
    $(call include-path-for, libhardware)

LOCAL_SRC_FILES := \
    OMXComponentCodecBase.cpp \
    OMXVideoDecoderBase.cpp \
    OMXVideoDecoderVP8.cpp

LOCAL_CFLAGS += -Werror
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libOMXVideoDecoderVP8

ifeq ($(TARGET_BOARD_PLATFORM),merrifield)
LOCAL_CFLAGS += -DVED_TILING
endif

ifeq ($(TARGET_BOARD_PLATFORM),moorefield)
LOCAL_CFLAGS += -DVED_TILING
endif

PLATFORM_USE_GEN_HW := \
    baytrail \
    cherrytrail

ifneq ($(filter $(TARGET_BOARD_PLATFORM),$(PLATFORM_USE_GEN_HW)),)
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/ufo
endif

ifneq ($(filter $(TARGET_BOARD_PLATFORM),$(PLATFORM_USE_GEN_HW)),)
LOCAL_CFLAGS += -DUSE_GEN_HW
endif

ifeq ($(TARGET_BOARD_PLATFORM),baytrail)
LOCAL_CFLAGS += -DUSE_X_TILE
endif

include $(BUILD_SHARED_LIBRARY)
endif

################################################################################

# VP9 with SW decode and HW Render
include $(CLEAR_VARS)

ifeq ($(TARGET_HAS_ISV),true)
LOCAL_CFLAGS += -DTARGET_HAS_ISV
endif

LOCAL_SHARED_LIBRARIES := \
    libwrs_omxil_common \
    libva_videodecoder \
    liblog \
    libva \
    libva-android \
    libva-tpi

LOCAL_STATIC_LIBRARIES := \
    libvpx_internal

LOCAL_C_INCLUDES := \
    $(TARGET_OUT_HEADERS)/wrs_omxil_core \
    $(TARGET_OUT_HEADERS)/khronos/openmax \
    $(TARGET_OUT_HEADERS)/libmix_videodecoder \
    $(TARGET_OUT_HEADERS)/libva \
    $(LOCAL_PATH)/libvpx_internal/libvpx \
    $(LOCAL_PATH)/libvpx_internal/libvpx/vpx_codec \
    $(LOCAL_PATH)/libvpx_internal/libvpx/vpx_ports \
    $(call include-path-for, frameworks-native)/media/hardware \
    $(call include-path-for, frameworks-native)/media/openmax \
    $(call include-path-for, libhardware)

ifeq ($(TARGET_BOARD_PLATFORM),baytrail)
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/ufo
endif

LOCAL_SRC_FILES := \
    OMXComponentCodecBase.cpp \
    OMXVideoDecoderBase.cpp \
    OMXVideoDecoderVP9HWR.cpp

LOCAL_CFLAGS += -Werror
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libOMXVideoDecoderVP9HWR

ifeq ($(TARGET_BOARD_PLATFORM),merrifield)
LOCAL_CFLAGS += -DVED_TILING
endif

ifeq ($(TARGET_BOARD_PLATFORM),moorefield)
LOCAL_CFLAGS += -DVED_TILING
endif

ifeq ($(TARGET_BOARD_PLATFORM),baytrail)
LOCAL_CFLAGS += -DUSE_GEN_HW
endif
include $(BUILD_SHARED_LIBRARY)

# VP9 hybrid decoder and HW Render
ifeq ($(TARGET_BOARD_PLATFORM),moorefield)
include $(CLEAR_VARS)
ifeq ($(TARGET_HAS_VPP),true)
LOCAL_CFLAGS += -DTARGET_HAS_VPP
endif
LOCAL_SHARED_LIBRARIES := \
    libwrs_omxil_common \
    liblog \
    libva_videodecoder \
    libdl \

LOCAL_C_INCLUDES := \
    $(TARGET_OUT_HEADERS)/wrs_omxil_core \
    $(TARGET_OUT_HEADERS)/khronos/openmax \
    $(TARGET_OUT_HEADERS)/libmix_videodecoder \
    $(TARGET_OUT_HEADERS)/libva \
    $(call include-path-for, frameworks-native)/media/hardware \
    $(call include-path-for, frameworks-native)/media/openmax \
    $(call include-path-for, libhardware)

ifeq ($(TARGET_BOARD_PLATFORM),baytrail)
    LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/ufo
endif

LOCAL_SRC_FILES := \
    OMXComponentCodecBase.cpp \
    OMXVideoDecoderBase.cpp \
    OMXVideoDecoderVP9Hybrid.cpp

LOCAL_CFLAGS += -Werror
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libOMXVideoDecoderVP9Hybrid

ifeq ($(TARGET_BOARD_PLATFORM),merrifield)
LOCAL_CFLAGS += -DVED_TILING
endif

ifeq ($(TARGET_BOARD_PLATFORM),moorefield)
LOCAL_CFLAGS += -DVED_TILING
endif

ifeq ($(TARGET_BOARD_PLATFORM),baytrail)
LOCAL_CFLAGS += -DUSE_GEN_HW
endif
include $(BUILD_SHARED_LIBRARY)
endif

include $(CLEAR_VARS)
ifeq ($(TARGET_HAS_ISV),true)
LOCAL_CFLAGS += -DTARGET_HAS_ISV
endif

LOCAL_SHARED_LIBRARIES := \
    libwrs_omxil_common \
    libva_videodecoder \
    liblog \
    libva \
    libva-android

LOCAL_C_INCLUDES := \
    $(TARGET_OUT_HEADERS)/wrs_omxil_core \
    $(TARGET_OUT_HEADERS)/khronos/openmax \
    $(TARGET_OUT_HEADERS)/libmix_videodecoder \
    $(TARGET_OUT_HEADERS)/libva \
    $(call include-path-for, frameworks-native)/media/hardware \
    $(call include-path-for, frameworks-native)/media/openmax \
    $(call include-path-for, libhardware)

PLATFORM_USE_GEN_HW := \
    baytrail \
    cherrytrail

ifneq ($(filter $(TARGET_BOARD_PLATFORM),$(PLATFORM_USE_GEN_HW)),)
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/ufo
endif

LOCAL_SRC_FILES := \
    OMXComponentCodecBase.cpp \
    OMXVideoDecoderBase.cpp \
    OMXVideoDecoderMPEG4.cpp

LOCAL_CFLAGS += -Werror
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libOMXVideoDecoderMPEG4
ifeq ($(TARGET_BOARD_PLATFORM),clovertrail)
LOCAL_CFLAGS += -DVED_TILING
endif

ifeq ($(TARGET_BOARD_PLATFORM),merrifield)
LOCAL_CFLAGS += -DVED_TILING
endif

ifeq ($(TARGET_BOARD_PLATFORM),moorefield)
LOCAL_CFLAGS += -DVED_TILING
endif

ifeq ($(TARGET_BOARD_PLATFORM),baytrail)
LOCAL_CFLAGS += -DUSE_GEN_HW
endif

include $(BUILD_SHARED_LIBRARY)

################################################################################

include $(CLEAR_VARS)
ifeq ($(TARGET_HAS_ISV),true)
LOCAL_CFLAGS += -DTARGET_HAS_ISV
endif

LOCAL_SHARED_LIBRARIES := \
    libwrs_omxil_common \
    libva_videodecoder \
    liblog \
    libva \
    libva-android

LOCAL_C_INCLUDES := \
    $(TARGET_OUT_HEADERS)/wrs_omxil_core \
    $(TARGET_OUT_HEADERS)/khronos/openmax \
    $(TARGET_OUT_HEADERS)/libmix_videodecoder \
    $(TARGET_OUT_HEADERS)/libva \
    $(call include-path-for, frameworks-native)/media/hardware \
    $(call include-path-for, frameworks-native)/media/openmax \
    $(call include-path-for, libhardware)

PLATFORM_USE_GEN_HW := \
    baytrail \
    cherrytrail

ifneq ($(filter $(TARGET_BOARD_PLATFORM),$(PLATFORM_USE_GEN_HW)),)
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/ufo
endif

LOCAL_SRC_FILES := \
    OMXComponentCodecBase.cpp \
    OMXVideoDecoderBase.cpp \
    OMXVideoDecoderH263.cpp

LOCAL_CFLAGS += -Werror
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libOMXVideoDecoderH263
ifeq ($(TARGET_BOARD_PLATFORM),clovertrail)
LOCAL_CFLAGS += -DVED_TILING
endif

ifeq ($(TARGET_BOARD_PLATFORM),merrifield)
LOCAL_CFLAGS += -DVED_TILING
endif

ifeq ($(TARGET_BOARD_PLATFORM),moorefield)
LOCAL_CFLAGS += -DVED_TILING
endif

ifeq ($(TARGET_BOARD_PLATFORM),baytrail)
LOCAL_CFLAGS += -DUSE_GEN_HW
endif

include $(BUILD_SHARED_LIBRARY)

################################################################################

include $(CLEAR_VARS)
ifeq ($(TARGET_HAS_ISV),true)
LOCAL_CFLAGS += -DTARGET_HAS_ISV
endif

LOCAL_SHARED_LIBRARIES := \
    libwrs_omxil_common \
    libva_videodecoder \
    liblog \
    libva \
    libva-android

LOCAL_C_INCLUDES := \
    $(TARGET_OUT_HEADERS)/wrs_omxil_core \
    $(TARGET_OUT_HEADERS)/khronos/openmax \
    $(TARGET_OUT_HEADERS)/libmix_videodecoder \
    $(TARGET_OUT_HEADERS)/libva \
    $(call include-path-for, frameworks-native)/media/hardware \
    $(call include-path-for, frameworks-native)/media/openmax \
    $(call include-path-for, libhardware)

PLATFORM_USE_GEN_HW := \
    baytrail \
    cherrytrail

ifneq ($(filter $(TARGET_BOARD_PLATFORM),$(PLATFORM_USE_GEN_HW)),)
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/ufo
endif

LOCAL_SRC_FILES := \
    OMXComponentCodecBase.cpp \
    OMXVideoDecoderBase.cpp \
    OMXVideoDecoderWMV.cpp

LOCAL_CFLAGS += -Werror
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libOMXVideoDecoderWMV
ifeq ($(TARGET_BOARD_PLATFORM),clovertrail)
LOCAL_CFLAGS += -DVED_TILING
endif

ifeq ($(TARGET_BOARD_PLATFORM),merrifield)
LOCAL_CFLAGS += -DVED_TILING
endif

ifeq ($(TARGET_BOARD_PLATFORM),moorefield)
LOCAL_CFLAGS += -DVED_TILING
endif

ifeq ($(TARGET_VPP_USE_GEN),true)
LOCAL_CFLAGS += -DDEINTERLACE_EXT
endif

ifeq ($(TARGET_BOARD_PLATFORM),baytrail)
LOCAL_CFLAGS += -DUSE_GEN_HW
endif

include $(BUILD_SHARED_LIBRARY)

################################################################################

#Build secure AVC video decoder only on supported platforms
ifeq ($(USE_INTEL_SECURE_AVC),true)

include $(CLEAR_VARS)
ifeq ($(TARGET_HAS_ISV),true)
LOCAL_CFLAGS += -DTARGET_HAS_ISV
endif

LOCAL_SHARED_LIBRARIES := \
    libwrs_omxil_common \
    libdrm \
    libva_videodecoder \
    liblog \
    libva \
    libva-android

LOCAL_C_INCLUDES := \
    $(TARGET_OUT_HEADERS)/wrs_omxil_core \
    $(TARGET_OUT_HEADERS)/khronos/openmax \
    $(TARGET_OUT_HEADERS)/libmix_videodecoder \
    $(TARGET_OUT_HEADERS)/libva \
    $(TARGET_OUT_HEADERS)/libdrm \
    $(call include-path-for, frameworks-native)/media/hardware \
    $(call include-path-for, frameworks-native)/media/openmax \
    $(call include-path-for, libhardware)

LOCAL_SRC_FILES := \
    OMXComponentCodecBase.cpp \
    OMXVideoDecoderBase.cpp

ifeq ($(TARGET_BOARD_PLATFORM),moorefield)
LOCAL_SRC_FILES += \
    securevideo/moorefield/OMXVideoDecoderAVCSecure.cpp \
    securevideo/moorefield/drm_vendor_api.c
LOCAL_CFLAGS += -DVED_TILING
LOCAL_SHARED_LIBRARIES += libdl
endif

LOCAL_CFLAGS += -Werror
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libOMXVideoDecoderAVCSecure

include $(BUILD_SHARED_LIBRARY)

endif #USE_INTEL_SECURE_AVC

################################################################################

include $(CLEAR_VARS)
ifeq ($(TARGET_HAS_ISV),true)
LOCAL_CFLAGS += -DTARGET_HAS_ISV
endif

LOCAL_SHARED_LIBRARIES := \
    libwrs_omxil_common \
    liblog \
    libva_videoencoder \
    libva \
    libva-android \
    libva-tpi \
    libutils \
    libcutils \
    libhardware \
    libintelmetadatabuffer

LOCAL_C_INCLUDES := \
    $(TARGET_OUT_HEADERS)/wrs_omxil_core \
    $(TARGET_OUT_HEADERS)/khronos/openmax \
    $(TARGET_OUT_HEADERS)/libmix_videoencoder \
    $(TARGET_OUT_HEADERS)/libva \
    $(call include-path-for, frameworks-native)/media/hardware \
    $(call include-path-for, frameworks-native)/media/openmax \

LOCAL_SRC_FILES := \
    OMXComponentCodecBase.cpp \
    OMXVideoEncoderBase.cpp \
    OMXVideoEncoderAVC.cpp

LOCAL_CFLAGS += $(LOCAL_C_FLAGS)
LOCAL_CFLAGS += -Werror
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libOMXVideoEncoderAVC
include $(BUILD_SHARED_LIBRARY)

################################################################################

include $(CLEAR_VARS)
ifeq ($(TARGET_HAS_ISV),true)
LOCAL_CFLAGS += -DTARGET_HAS_ISV
endif

LOCAL_SHARED_LIBRARIES := \
    libwrs_omxil_common \
    liblog \
    libva_videoencoder \
    libva \
    libva-android \
    libva-tpi \
    libutils \
    libcutils \
    libhardware \
    libintelmetadatabuffer

LOCAL_C_INCLUDES := \
    $(TARGET_OUT_HEADERS)/wrs_omxil_core \
    $(TARGET_OUT_HEADERS)/khronos/openmax \
    $(TARGET_OUT_HEADERS)/libmix_videoencoder \
    $(TARGET_OUT_HEADERS)/libva \
    $(call include-path-for, frameworks-native)/media/hardware \
    $(call include-path-for, frameworks-native)/media/openmax \

LOCAL_SRC_FILES := \
    OMXComponentCodecBase.cpp \
    OMXVideoEncoderBase.cpp \
    OMXVideoEncoderH263.cpp

LOCAL_CFLAGS += $(LOCAL_C_FLAGS)

ifeq ($(SW_MPEG4_ENCODER),true)
    LOCAL_CFLAGS += -DSYNC_MODE
endif
LOCAL_CFLAGS += -Werror
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libOMXVideoEncoderH263
include $(BUILD_SHARED_LIBRARY)

################################################################################

include $(CLEAR_VARS)
ifeq ($(TARGET_HAS_ISV),true)
LOCAL_CFLAGS += -DTARGET_HAS_ISV
endif

LOCAL_SHARED_LIBRARIES := \
    libwrs_omxil_common \
    liblog \
    libva_videoencoder \
    libva \
    libva-android \
    libva-tpi \
    libutils \
    libcutils \
    libhardware \
    libintelmetadatabuffer

LOCAL_C_INCLUDES := \
    $(TARGET_OUT_HEADERS)/wrs_omxil_core \
    $(TARGET_OUT_HEADERS)/khronos/openmax \
    $(TARGET_OUT_HEADERS)/libmix_videoencoder \
    $(TARGET_OUT_HEADERS)/libva \
    $(call include-path-for, frameworks-native)/media/hardware \
    $(call include-path-for, frameworks-native)/media/openmax \

LOCAL_SRC_FILES := \
    OMXComponentCodecBase.cpp \
    OMXVideoEncoderBase.cpp \
    OMXVideoEncoderMPEG4.cpp

LOCAL_CFLAGS += $(LOCAL_C_FLAGS)

LOCAL_CFLAGS += -Werror
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libOMXVideoEncoderMPEG4
include $(BUILD_SHARED_LIBRARY)

################################################################################

include $(CLEAR_VARS)
ifeq ($(TARGET_HAS_ISV),true)
LOCAL_CFLAGS += -DTARGET_HAS_ISV
endif

LOCAL_SHARED_LIBRARIES := \
    libwrs_omxil_common \
    libva_videodecoder \
    liblog \
    libva \
    libva-android

LOCAL_C_INCLUDES := \
    $(TARGET_OUT_HEADERS)/wrs_omxil_core \
    $(TARGET_OUT_HEADERS)/khronos/openmax \
    $(TARGET_OUT_HEADERS)/libmix_videodecoder \
    $(TARGET_OUT_HEADERS)/libva \
    $(call include-path-for, frameworks-native)/media/hardware \
    $(call include-path-for, frameworks-native)/media/openmax \
    $(call include-path-for, libhardware)

LOCAL_SRC_FILES := \
    OMXComponentCodecBase.cpp \
    OMXVideoDecoderBase.cpp \
    OMXVideoDecoderPAVC.cpp

LOCAL_CFLAGS += -Werror
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libOMXVideoDecoderPAVC
ifeq ($(TARGET_BOARD_PLATFORM),clovertrail)
LOCAL_CFLAGS += -DVED_TILING
endif

ifeq ($(TARGET_BOARD_PLATFORM),merrifield)
LOCAL_CFLAGS += -DVED_TILING
endif

ifeq ($(TARGET_BOARD_PLATFORM),moorefield)
LOCAL_CFLAGS += -DVED_TILING
endif

include $(BUILD_SHARED_LIBRARY)

################################################################################

include $(CLEAR_VARS)
ifeq ($(TARGET_HAS_ISV),true)
LOCAL_CFLAGS += -DTARGET_HAS_ISV
endif

LOCAL_SHARED_LIBRARIES := \
    libwrs_omxil_common \
    liblog \
    libva_videoencoder \
    libva \
    libva-android \
    libva-tpi \
    libutils \
    libcutils \
    libhardware \
    libintelmetadatabuffer

LOCAL_C_INCLUDES := \
    $(TARGET_OUT_HEADERS)/wrs_omxil_core \
    $(TARGET_OUT_HEADERS)/khronos/openmax \
    $(TARGET_OUT_HEADERS)/libmix_videoencoder \
    $(TARGET_OUT_HEADERS)/libva \
    $(call include-path-for, frameworks-native)/media/hardware \
    $(call include-path-for, frameworks-native)/media/openmax \

LOCAL_SRC_FILES := \
    OMXComponentCodecBase.cpp \
    OMXVideoEncoderBase.cpp \
    OMXVideoEncoderVP8.cpp

LOCAL_CFLAGS += $(LOCAL_C_FLAGS)
LOCAL_CFLAGS += -Werror
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libOMXVideoEncoderVP8
include $(BUILD_SHARED_LIBRARY)

endif # ifeq ($(strip $(BOARD_USES_WRS_OMXIL_CORE)),true)
