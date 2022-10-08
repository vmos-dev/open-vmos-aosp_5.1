
LOCAL_PATH:= $(call my-dir)

# Temporary wrapper to disable the camera_test in non CPCAM mode.
# The camera_test source code should respect the lack of
# OMAP_ENHANCEMENT and OMAP_ENHANCEMENT_CPCAM macros in order to
# be returned back into build.
ifdef OMAP_ENHANCEMENT_CPCAM

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	camera_test_surfacetexture.cpp \
	camera_test_menu.cpp \
	camera_test_script.cpp

LOCAL_SHARED_LIBRARIES:= \
	libdl \
	libui \
	libutils \
	libcutils \
	liblog \
	libbinder \
	libmedia \
	libmedia_native \
	libui \
	libgui \
	libcamera_client \
	libEGL \
	libGLESv2 \

ifdef OMAP_ENHANCEMENT_CPCAM
LOCAL_STATIC_LIBRARIES += \
    libcpcamcamera_client
endif

LOCAL_C_INCLUDES += \
	frameworks/base/include/ui \
	frameworks/base/include/surfaceflinger \
	frameworks/base/include/camera \
	frameworks/base/include/media

LOCAL_MODULE:= camera_test
LOCAL_MODULE_TAGS:= tests

LOCAL_CFLAGS += -Wall -fno-short-enums -O0 -g -D___ANDROID___ $(ANDROID_API_CFLAGS)

# Add TARGET FLAG for OMAP4 and OMAP5 boards only
# First eliminate OMAP3 and then ensure that this is not used
# for customer boards.
ifneq ($(TARGET_BOARD_PLATFORM),omap3)
    ifeq ($(findstring omap, $(TARGET_BOARD_PLATFORM)),omap)
        LOCAL_CFLAGS += -DTARGET_OMAP4
    endif
endif

include $(BUILD_HEAPTRACKED_EXECUTABLE)


include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	surfacetexture_test.cpp

LOCAL_SHARED_LIBRARIES:= \
	libdl \
	libui \
	libutils \
	libcutils \
	liblog \
	libbinder \
	libmedia \
	libui \
	libgui \
	libcamera_client \
	libEGL \
	libGLESv2 \
	libion

LOCAL_C_INCLUDES += \
	frameworks/base/include/ui \
	frameworks/base/include/surfaceflinger \
	frameworks/base/include/camera \
	frameworks/base/include/media \
	hardware/ti/omap4xxx/ion

LOCAL_MODULE:= surfacetexture_test
LOCAL_MODULE_TAGS:= tests

LOCAL_CFLAGS += -Wall -fno-short-enums -O0 -g -D___ANDROID___ $(ANDROID_API_CFLAGS)

ifeq ($(TARGET_BOARD_PLATFORM),omap4)
    LOCAL_CFLAGS += -DTARGET_OMAP4
endif

include $(BUILD_HEAPTRACKED_EXECUTABLE)

endif
