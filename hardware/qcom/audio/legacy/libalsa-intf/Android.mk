LOCAL_PATH:= $(call my-dir)

ifeq ($(strip $(BOARD_USES_ALSA_AUDIO)),true)
# Any prebuilt files with default TAGS can use the below:

include $(CLEAR_VARS)
#LOCAL_SRC_FILES:= aplay.c alsa_pcm.c alsa_mixer.c
LOCAL_SRC_FILES:= aplay.c
LOCAL_MODULE:= aplay
LOCAL_SHARED_LIBRARIES:= libc libcutils libalsa-intf
LOCAL_MODULE_TAGS:= debug
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
#LOCAL_SRC_FILES:= arec.c alsa_pcm.c
LOCAL_SRC_FILES:= arec.c
LOCAL_MODULE:= arec
LOCAL_SHARED_LIBRARIES:= libc libcutils libalsa-intf
LOCAL_MODULE_TAGS:= debug
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= amix.c
LOCAL_MODULE:= amix
LOCAL_SHARED_LIBRARIES := libc libcutils libalsa-intf
LOCAL_MODULE_TAGS:= debug
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= alsaucm_test.c
LOCAL_MODULE:= alsaucm_test
LOCAL_SHARED_LIBRARIES:= libc libcutils libalsa-intf
LOCAL_MODULE_TAGS:= debug
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_COPY_HEADERS_TO   := mm-audio/libalsa-intf
LOCAL_COPY_HEADERS      := alsa_audio.h
LOCAL_COPY_HEADERS      += alsa_ucm.h
LOCAL_COPY_HEADERS      += msm8960_use_cases.h
LOCAL_SRC_FILES:= alsa_mixer.c alsa_pcm.c alsa_ucm.c
LOCAL_MODULE:= libalsa-intf
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES:= libc libcutils #libutils #libmedia libhardware_legacy
LOCAL_CFLAGS := -DQC_PROP -DCONFIG_DIR=\"/system/etc/snd_soc_msm/\"

ifeq ($(TARGET_SIMULATOR),true)
 LOCAL_LDLIBS += -ldl
else
 LOCAL_SHARED_LIBRARIES += libdl
endif
LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)
endif
