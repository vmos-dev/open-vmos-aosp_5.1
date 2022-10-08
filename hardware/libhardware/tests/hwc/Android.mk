LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libcnativewindow 
LOCAL_SRC_FILES := cnativewindow.c util.c
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := hwc-test-arrows
LOCAL_SRC_FILES := test-arrows.c
LOCAL_STATIC_LIBRARIES := libcnativewindow
LOCAL_SHARED_LIBRARIES := libEGL libGLESv2 libdl libhardware
LOCAL_CFLAGS := -DGL_GLEXT_PROTOTYPES
include $(BUILD_EXECUTABLE)
