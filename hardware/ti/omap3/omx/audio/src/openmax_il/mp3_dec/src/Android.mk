ifeq ($(BUILD_MP3_DECODER),1)
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)



LOCAL_SRC_FILES:= \
	OMX_Mp3Dec_CompThread.c		\
	OMX_Mp3Dec_Utils.c		\
	OMX_Mp3Decoder.c

LOCAL_C_INCLUDES := $(TI_OMX_COMP_C_INCLUDES) \
	$(TI_OMX_SYSTEM)/common/inc		\
	$(TI_OMX_AUDIO)/mp3_dec/inc
	
LOCAL_SHARED_LIBRARIES := $(TI_OMX_COMP_SHARED_LIBRARIES) \
        liblog

LOCAL_LDLIBS += \
	-lpthread \
	-ldl \
	-lsdl
	
LOCAL_CFLAGS := $(TI_OMX_CFLAGS) -DOMAP_2430

LOCAL_MODULE:= libOMX.TI.MP3.decode

include $(BUILD_SHARED_LIBRARY)
endif
