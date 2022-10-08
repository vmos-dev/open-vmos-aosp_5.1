ifneq ($(BUILD_WITHOUT_PV),true)
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := src/ti_omx_interface.cpp

LOCAL_MODULE := libVendor_ti_omx

PV_TOP := external/opencore

PV_COPY_HEADERS_TO := libpv

PV_INCLUDES := \
	$(PV_TOP)/android \
	$(PV_TOP)/extern_libs_v2/khronos/openmax/include \
	$(PV_TOP)/engines/common/include \
	$(PV_TOP)/engines/player/config/android \
	$(PV_TOP)/engines/player/include \
	$(PV_TOP)/nodes/pvmediaoutputnode/include \
	$(PV_TOP)/nodes/pvdownloadmanagernode/config/opencore \
	$(PV_TOP)/pvmi/pvmf/include \
	$(PV_TOP)/fileformats/mp4/parser/config/opencore \
	$(PV_TOP)/oscl/oscl/config/android \
	$(PV_TOP)/oscl/oscl/config/shared \
	$(PV_TOP)/engines/author/include \
	$(PV_TOP)/android/drm/oma1/src \
	$(PV_TOP)/build_config/opencore_dynamic \
	$(TARGET_OUT_HEADERS)/$(PV_COPY_HEADERS_TO)

LOCAL_CFLAGS :=   $(PV_CFLAGS)

LOCAL_ARM_MODE := arm

LOCAL_C_INCLUDES := \
    $(PV_INCLUDES)

LOCAL_SHARED_LIBRARIES := libOMX_Core liblog

-include $(PV_TOP)/Android_platform_extras.mk

-include $(PV_TOP)/Android_system_extras.mk

include $(BUILD_SHARED_LIBRARY)
endif
