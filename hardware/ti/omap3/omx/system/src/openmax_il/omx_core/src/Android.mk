LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)



LOCAL_SRC_FILES:= \
	OMX_Core.c

LOCAL_C_INCLUDES += \
	$(TI_OMX_INCLUDES) \
	$(PV_INCLUDES)

LOCAL_SHARED_LIBRARIES := \
	libdl \
	liblog
	
LOCAL_CFLAGS := $(TI_OMX_CFLAGS)

ifneq ($(BUILD_WITHOUT_PV),true)
LOCAL_SHARED_LIBRARIES += \
	libVendor_ti_omx_config_parser
else
LOCAL_CFLAGS += -DNO_OPENCORE
endif
LOCAL_MODULE:= libOMX_Core

include $(BUILD_SHARED_LIBRARY)
