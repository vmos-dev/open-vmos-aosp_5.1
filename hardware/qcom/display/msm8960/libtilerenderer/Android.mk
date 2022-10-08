LOCAL_PATH := $(call my-dir)
include $(LOCAL_PATH)/../common.mk
include $(CLEAR_VARS)

ifeq ($(USE_OPENGL_RENDERER),true)
LOCAL_MODULE           := libtilerenderer
LOCAL_MODULE_TAGS      := optional
LOCAL_CFLAGS           := -DLOG_TAG=\"qdtilerenderer\"
LOCAL_C_INCLUDES := \
        frameworks/base/include/utils \
        frameworks/base/libs/hwui \
        external/skia/include/core \
        external/skia/include/effects \
        external/skia/include/images \
        external/skia/src/ports \
        external/skia/include/utils \
        hardware/libhardware/include/hardware \
        frameworks/base/opengl/include/GLES2
LOCAL_SHARED_LIBRARIES := $(common_libs) libGLESv2 libhwui
LOCAL_SRC_FILES        := tilerenderer.cpp

include $(BUILD_SHARED_LIBRARY)
endif
