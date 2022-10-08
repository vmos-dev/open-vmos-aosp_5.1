# only include if running on an omap4 platform
ifeq ($(TARGET_BOARD_PLATFORM),omap4)

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_C_INCLUDES += $(HARDWARE_TI_OMAP4_BASE)/kernel-headers-ti
LOCAL_C_INCLUDES += $(HARDWARE_TI_OMAP4_BASE)/system-core-headers-ti
LOCAL_SRC_FILES := ion.c
LOCAL_MODULE := libion_ti
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := liblog
include $(BUILD_HEAPTRACKED_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_C_INCLUDES += $(HARDWARE_TI_OMAP4_BASE)/kernel-headers-ti
LOCAL_C_INCLUDES += $(HARDWARE_TI_OMAP4_BASE)/system-core-headers-ti
LOCAL_SRC_FILES := ion.c ion_test.c
LOCAL_MODULE := iontest_ti
LOCAL_MODULE_TAGS := optional tests
LOCAL_SHARED_LIBRARIES := liblog
include $(BUILD_HEAPTRACKED_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_C_INCLUDES += $(HARDWARE_TI_OMAP4_BASE)/kernel-headers-ti
LOCAL_C_INCLUDES += $(HARDWARE_TI_OMAP4_BASE)/system-core-headers-ti
LOCAL_SRC_FILES := ion.c ion_test_2.c
LOCAL_MODULE := iontest2_ti
LOCAL_MODULE_TAGS := optional tests
LOCAL_SHARED_LIBRARIES := liblog
include $(BUILD_HEAPTRACKED_SHARED_LIBRARY)

endif
