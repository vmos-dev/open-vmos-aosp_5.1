LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
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
	libui \
	libgui \
	libcamera_client

LOCAL_C_INCLUDES += \
	frameworks/base/include/ui \
	frameworks/base/include/surfaceflinger \
	frameworks/base/include/camera \
	frameworks/base/include/media \
	$(PV_INCLUDES)

LOCAL_MODULE:= camera_test
LOCAL_MODULE_TAGS:= tests

LOCAL_CFLAGS += -Wall -fno-short-enums -O0 -g -D___ANDROID___

ifeq ($(TARGET_BOARD_PLATFORM),omap4)
    LOCAL_CFLAGS += -DTARGET_OMAP4
endif

include $(BUILD_HEAPTRACKED_EXECUTABLE)
