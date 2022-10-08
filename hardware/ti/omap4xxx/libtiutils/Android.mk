################################################

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES:= \
    MessageQueue.cpp \
    Semaphore.cpp \
    ErrorUtils.cpp
    
LOCAL_SHARED_LIBRARIES:= \
    libdl \
    libui \
    libbinder \
    libutils \
    libcutils \
    liblog

LOCAL_C_INCLUDES += \
	bionic/libc/include \
	hardware/ti/omap4xxx/domx/omx_core/inc \
	hardware/ti/omap4xxx/domx/mm_osal/inc
	
LOCAL_CFLAGS += -fno-short-enums 

# LOCAL_CFLAGS +=

LOCAL_MODULE:= libtiutils
LOCAL_MODULE_TAGS:= optional

include $(BUILD_HEAPTRACKED_SHARED_LIBRARY)
