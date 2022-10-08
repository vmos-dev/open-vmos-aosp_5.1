# Copyright (C) 2012 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH := $(call my-dir)

# Build only when BOARD_USES_ALSA_AUDIO is set
ifeq ($(BOARD_USES_ALSA_AUDIO), true)
ifeq ($(BOARD_USES_TINY_ALSA_AUDIO), true)

include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_CFLAGS := -D_POSIX_C_SOURCE=200809

LOCAL_SRC_FILES := \
    tinyaudio_hw.c

LOCAL_C_INCLUDES += \
    external/tinyalsa/include

LOCAL_SHARED_LIBRARIES := liblog libcutils libtinyalsa

LOCAL_MODULE := audio.hdmi.$(TARGET_BOARD_PLATFORM)
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS += -Werror

include $(BUILD_SHARED_LIBRARY)

endif #BOARD_USES_TINY_ALSA_AUDIO
endif #BOARD_USES_ALSA_AUDIO
