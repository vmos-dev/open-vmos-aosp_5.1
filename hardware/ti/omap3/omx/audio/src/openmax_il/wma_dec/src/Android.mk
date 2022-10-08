ifeq ($(BUILD_WMA_DECODER),1)
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)



LOCAL_SRC_FILES:= \
	OMX_WmaDec_ComponentThread.c    \
	OMX_WmaDec_Utils.c	    \
	OMX_WmaDecoder.c

LOCAL_C_INCLUDES := $(TI_OMX_COMP_C_INCLUDES) \
	$(TI_OMX_SYSTEM)/common/inc		\
	$(TI_OMX_AUDIO)/wma_dec/inc
	
LOCAL_SHARED_LIBRARIES := $(TI_OMX_COMP_SHARED_LIBRARIES) \
        liblog

LOCAL_LDLIBS += \
	-lpthread \
	-ldl \
	-lsdl
	
LOCAL_CFLAGS := $(TI_OMX_CFLAGS) -DOMAP_2430
LOCAL_MODULE:= libOMX.TI.WMA.decode

include $(BUILD_SHARED_LIBRARY)
endif
