LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)



LOCAL_SRC_FILES:= \
	src/OMX_VideoDec_Thread.c		\
	src/OMX_VideoDec_Utils.c		\
	src/OMX_VideoDecoder.c

LOCAL_C_INCLUDES := $(TI_OMX_COMP_C_INCLUDES) \
	$(TI_OMX_VIDEO)/video_decode/inc \
	

ifeq ($(PERF_INSTRUMENTATION),1)
LOCAL_C_INCLUDES += \
	$(TI_OMX_SYSTEM)/perf/inc 
endif

LOCAL_SHARED_LIBRARIES := $(TI_OMX_COMP_SHARED_LIBRARIES)

ifeq ($(PERF_INSTRUMENTATION),1)
LOCAL_SHARED_LIBRARIES += \
	libPERF
endif

LOCAL_LDLIBS += \
	-lpthread \

LOCAL_CFLAGS := $(TI_OMX_CFLAGS) -DANDROID -DOMAP_2430

LOCAL_MODULE:= libOMX.TI.Video.Decoder

include $(BUILD_SHARED_LIBRARY)
