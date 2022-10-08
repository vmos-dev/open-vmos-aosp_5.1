ifeq ($(BUILD_G729_DECODER),1)
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)



LOCAL_SRC_FILES:= \
	OMX_G729Dec_ComponentThread.c \
	OMX_G729Dec_Utils.c \
	OMX_G729Decoder.c

LOCAL_C_INCLUDES := $(TI_OMX_COMP_C_INCLUDES) \
	$(TI_OMX_SYSTEM)/common/inc \
	$(TI_OMX_AUDIO)/g729_dec/inc
	
LOCAL_SHARED_LIBRARIES := $(TI_OMX_COMP_SHARED_LIBRARIES) \
        liblog


LOCAL_LDLIBS += \
	-lpthread \
	-ldl \
	-lsdl
	
LOCAL_CFLAGS := $(TI_OMX_CFLAGS) -DOMAP_2430

LOCAL_MODULE:= libOMX.TI.G729.decode

include $(BUILD_SHARED_LIBRARY)
endif
