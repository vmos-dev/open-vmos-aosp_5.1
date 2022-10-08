ifeq ($(BUILD_VPP),1)
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)



LOCAL_SRC_FILES:= \
	src/OMX_VPP.c \
	src/OMX_VPP_Utils.c \
	src/OMX_VPP_CompThread.c \
	src/OMX_VPP_ImgConv.c \

LOCAL_C_INCLUDES := $(TI_OMX_COMP_C_INCLUDES) \
	$(TI_OMX_VIDEO)/prepost_processor/inc \

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

LOCAL_CFLAGS := $(TI_OMX_CFLAGS) -DANDROID -DOMAP_2430 -g

LOCAL_MODULE:= libOMX.TI.VPP

include $(BUILD_SHARED_LIBRARY)
endif

#########################################################
ifeq ($(BUILD_VPP_TEST),1)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= tests/VPPTest.c

LOCAL_C_INCLUDES := $(TI_OMX_COMP_C_INCLUDES) \
	$(TI_OMX_VIDEO)/prepost_processor/inc \

LOCAL_SHARED_LIBRARIES := $(TI_OMX_COMP_SHARED_LIBRARIES)

LOCAL_CFLAGS := $(TI_OMX_CFLAGS)

LOCAL_MODULE:= VPPTest_common

include $(BUILD_EXECUTABLE)
endif

