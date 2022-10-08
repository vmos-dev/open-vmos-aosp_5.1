ifeq ($(BUILD_AAC_DECODER),1)
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)



LOCAL_SRC_FILES:= \
	OMX_AacDec_CompThread.c		\
	OMX_AacDec_Utils.c		\
	OMX_AacDecoder.c

LOCAL_C_INCLUDES := $(TI_OMX_COMP_C_INCLUDES) \
	$(TI_OMX_SYSTEM)/common/inc		\
	$(TI_OMX_AUDIO)/aac_dec/inc
	
LOCAL_SHARED_LIBRARIES := $(TI_OMX_COMP_SHARED_LIBRARIES) \
        liblog


LOCAL_LDLIBS += \
	-lpthread \
	-ldl \
	-lsdl
	
LOCAL_CFLAGS := $(TI_OMX_CFLAGS) -DOMAP_2430

LOCAL_MODULE:= libOMX.TI.AAC.decode

include $(BUILD_SHARED_LIBRARY)
endif
