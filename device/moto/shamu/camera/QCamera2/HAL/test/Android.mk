ifneq (,$(findstring $(PLATFORM_VERSION), 4.4 4.4.1 4.4.2))
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    qcamera_test.cpp \

LOCAL_SHARED_LIBRARIES:= \
    libdl \
    libui \
    libutils \
    libcutils \
    libbinder \
    libmedia \
    libui \
    libgui \
    libcamera_client \
    libskia \
    libstagefright \
    libstagefright_foundation \

ifneq (1,$(filter 1,$(shell echo "$$(( $(PLATFORM_SDK_VERSION) >= 18 ))" )))

LOCAL_SHARED_LIBRARIES += \
    libmedia_native \

LOCAL_32_BIT_ONLY := true
LOCAL_CFLAGS += -DUSE_JB_MR1

endif

LOCAL_C_INCLUDES += \
    frameworks/base/include/ui \
    frameworks/base/include/surfaceflinger \
    frameworks/base/include/camera \
    frameworks/base/include/media \
    external/skia/include/core \
    external/skia/include/images \
    hardware/qcom/display/libgralloc \
    frameworks/av/include/media/stagefright \
    frameworks/native/include/media/openmax \

LOCAL_MODULE:= camera_test
LOCAL_MODULE_TAGS:= tests

LOCAL_CFLAGS += -Wall -fno-short-enums -O0

LOCAL_32_BIT_ONLY := true
include $(BUILD_EXECUTABLE)

endif
