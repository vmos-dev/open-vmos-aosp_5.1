ifneq ($(filter tegra132,$(TARGET_BOARD_PLATFORM)),)
ifneq ($(filter flounder flounder64 flounder_lte,$(TARGET_DEVICE)),)

MY_LOCAL_PATH := $(call my-dir)

include $(MY_LOCAL_PATH)/hal/Android.mk
include $(MY_LOCAL_PATH)/soundtrigger/Android.mk
include $(MY_LOCAL_PATH)/visualizer/Android.mk

endif
endif
