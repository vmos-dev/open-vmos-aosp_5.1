LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES:= \
	dspexec.c

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../inc	

LOCAL_SHARED_LIBRARIES := \
	libbridge

LOCAL_CFLAGS += -Wall -g -O2 -finline-functions -DOMAP_3430

LOCAL_MODULE:= dspexec

include $(BUILD_EXECUTABLE)

