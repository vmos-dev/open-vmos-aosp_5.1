LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    src/timm_osal.c \
    src/timm_osal_events.c \
    src/timm_osal_memory.c \
    src/timm_osal_mutex.c \
    src/timm_osal_pipes.c \
    src/timm_osal_semaphores.c \
    src/timm_osal_task.c \
    src/timm_osal_trace.c

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/inc

LOCAL_SHARED_LIBRARIES := \
    libdl \
    liblog \
    libc \
    libcutils

LOCAL_CFLAGS += -DOMAP_2430 -DOMX_DEBUG -D_Android -D_POSIX_VERSION_1_
LOCAL_CFLAGS += -DTIMM_OSAL_DEBUG_TRACE_DETAIL=1 # quiet

LOCAL_MODULE:= libmm_osal
LOCAL_MODULE_TAGS:= optional

include $(BUILD_HEAPTRACKED_SHARED_LIBRARY)
