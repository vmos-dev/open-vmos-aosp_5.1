LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)



LOCAL_SRC_FILES:= \
	perf_reader.c \
	../src/perf.c \
	../src/perf_config.c \
	../src/perf_log.c 

TI_OMX_CFLAGS += -D__PERF_INSTRUMENTATION__ 
TI_OMX_CFLAGS += -D__PERF_LOG_LOCATION__

ifdef PERF_CUSTOMIZABLE
LOCAL_SRC_FILES+= \
	../src/perf_print.c \
	../src/perf_rt.c 

TI_OMX_CFLAGS += -D__PERF_CUSTOMIZABLE__
endif

LOCAL_C_INCLUDES := \
	$(TI_OMX_SYSTEM)/perf/inc \
	$(TI_OMX_INCLUDES) \
	$(PV_INCLUDES) \
	bionic/libc/include \

LOCAL_SHARED_LIBRARIES := \
	libdl \
	liblog
	
LOCAL_CFLAGS := $(TI_OMX_CFLAGS)

LOCAL_MODULE:= PERF_reader

include $(BUILD_STATIC_LIBRARY)
