LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)



LOCAL_SRC_FILES:= \
	OMX_AmrEnc_ComponentThread.c		\
	OMX_AmrEnc_Utils.c		\
	OMX_AmrEncoder.c

LOCAL_C_INCLUDES := $(TI_OMX_COMP_C_INCLUDES) \
	$(TI_OMX_SYSTEM)/common/inc		\
	$(TI_OMX_AUDIO)/nbamr_enc/inc
	
LOCAL_SHARED_LIBRARIES := $(TI_OMX_COMP_SHARED_LIBRARIES) \
        liblog

LOCAL_LDLIBS += \
	-lpthread \
	-ldl \
	-lsdl
	
LOCAL_CFLAGS := $(TI_OMX_CFLAGS) -DOMAP_2430

LOCAL_MODULE:= libOMX.TI.AMR.encode

include $(BUILD_SHARED_LIBRARY)
