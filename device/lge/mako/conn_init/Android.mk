#
# Copyright 2012 The Android Open Source Project
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
#

ifneq ($(filter mako occam,$(TARGET_DEVICE)),)

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libwfcu
LOCAL_SRC_FILES := wfc_util_fctrl.c \
                   wfc_util_common.c
LOCAL_CFLAGS := -Wall \
                -Werror
LOCAL_CFLAGS += -DCONFIG_LGE_WLAN_WIFI_PATCH
ifeq ($(BOARD_HAS_QCOM_WLAN), true)
LOCAL_SRC_FILES += wfc_util_qcom.c
LOCAL_CFLAGS += -DCONFIG_LGE_WLAN_QCOM_PATCH
LOCAL_CFLAGS += -DWLAN_CHIP_VERSION_WCNSS
endif
LOCAL_SHARED_LIBRARIES := libcutils liblog
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_OWNER := lge
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := conn_init.c
LOCAL_SHARED_LIBRARIES := libcutils liblog
LOCAL_SHARED_LIBRARIES += libwfcu
LOCAL_CFLAGS += -Wall -Werror
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/bin
LOCAL_MODULE := conn_init
LOCAL_MODULE_OWNER := lge

# Install symlinks with targets unavailable at build time
LOCAL_POST_INSTALL_CMD := \
    mkdir -p $(TARGET_OUT_VENDOR)/firmware/wlan/prima/; \
    ln -sf /data/misc/wifi/WCNSS_qcom_cfg.ini $(TARGET_OUT_VENDOR)/firmware/wlan/prima/WCNSS_qcom_cfg.ini; \
    ln -sf /data/misc/wifi/WCNSS_qcom_wlan_nv.bin $(TARGET_OUT_VENDOR)/firmware/wlan/prima/WCNSS_qcom_wlan_nv.bin

include $(BUILD_EXECUTABLE)

endif
