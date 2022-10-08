ifeq ($(strip $(BOARD_USES_WRS_OMXIL_CORE)),true)

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_COPY_HEADERS_TO := khronos/openmax

LOCAL_COPY_HEADERS := \
    core/inc/khronos/openmax/include/OMX_IntelErrorTypes.h \
    core/inc/khronos/openmax/include/OMX_IntelIndexExt.h \
    core/inc/khronos/openmax/include/OMX_IntelVideoExt.h

include $(BUILD_COPY_HEADERS)

include $(CLEAR_VARS)
LOCAL_COPY_HEADERS_TO := wrs_omxil_core
LOCAL_COPY_HEADERS := \
    base/inc/cmodule.h \
    base/inc/componentbase.h \
    base/inc/portaudio.h \
    base/inc/portbase.h \
    base/inc/portimage.h \
    base/inc/portother.h \
    base/inc/portvideo.h \
    utils/inc/audio_parser.h \
    utils/inc/list.h \
    utils/inc/log.h \
    utils/inc/module.h \
    utils/inc/queue.h \
    utils/inc/thread.h \
    utils/inc/workqueue.h
include $(BUILD_COPY_HEADERS)

include $(CLEAR_VARS)

WRS_OMXIL_CORE_ROOT := $(LOCAL_PATH)

COMPONENT_SUPPORT_BUFFER_SHARING := false
COMPONENT_SUPPORT_OPENCORE := false

# core
-include $(WRS_OMXIL_CORE_ROOT)/core/src/Android.mk

# base class
-include $(WRS_OMXIL_CORE_ROOT)/base/src/Android.mk

# utility
-include $(WRS_OMXIL_CORE_ROOT)/utils/src/Android.mk

LOCAL_CFLAGS += -Werror
endif
