#
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
#

# Base modules (will move elsewhere, previously user tagged)
PRODUCT_PACKAGES += \
    20-dns.conf \
    95-configured \
    appwidget \
    appops \
    am \
    android.policy \
    android.test.runner \
    app_process \
    applypatch \
    blkid \
    bmgr \
    bugreport \
    content \
    dhcpcd \
    dhcpcd-run-hooks \
    dnsmasq \
    dpm \
    framework \
    fsck_msdos \
    ime \
    input \
    javax.obex \
    libandroid \
    libandroid_runtime \
    libandroid_servers \
    libaudioeffect_jni \
    libaudioflinger \
    libaudiopolicyservice \
    libaudiopolicymanager \
    libbundlewrapper \
    libcamera_client \
    libcameraservice \
    libdl \
    libdrmclearkeyplugin \
    libeffectproxy \
    libeffects \
    libinput \
    libinputflinger \
    libiprouteutil \
    libjnigraphics \
    libldnhncr \
    libmedia \
    libmedia_jni \
    libmediaplayerservice \
    libmtp \
    libnetd_client \
    libnetlink \
    libnetutils \
    libpdfium \
    libreference-ril \
    libreverbwrapper \
    libril \
    librtp_jni \
    libsensorservice \
    libskia \
    libsonivox \
    libsoundpool \
    libsoundtrigger \
    libsoundtriggerservice \
    libsqlite \
    libstagefright \
    libstagefright_amrnb_common \
    libstagefright_avc_common \
    libstagefright_enc_common \
    libstagefright_foundation \
    libstagefright_omx \
    libstagefright_yuv \
    libusbhost \
    libutils \
    libvisualizer \
    libvorbisidec \
    libmediandk \
    libwifi-service \
    media \
    media_cmd \
    mediaserver \
    monkey \
    mtpd \
    ndc \
    netcfg \
    netd \
    ping \
    ping6 \
    platform.xml \
    pppd \
    pm \
    racoon \
    run-as \
    schedtest \
    sdcard \
    services \
    settings \
    svc \
    tc \
    vdc \
    vold \
    wm

PRODUCT_OPENSRC_USE_PREBUILT := yes

PRODUCT_PREBUILT_WEBVIEWCHROMIUM := yes
## PREBUILD webview 
PRODUCT_PACKAGES += \
    webview \
    libwebviewchromium \

PRODUCT_COPY_FILES := $(call add-to-product-copy-files-if-exists,\
    frameworks/base/preloaded-classes:system/etc/preloaded-classes)

# Note: it is acceptable to not have a compiled-classes file. In that case, all boot classpath
#       classes will be compiled.
PRODUCT_COPY_FILES += $(call add-to-product-copy-files-if-exists,\
    frameworks/base/compiled-classes:system/etc/compiled-classes)

## add fix
PRODUCT_COPY_FILES += device/generic/goldfish/dir/proc/:root/
PRODUCT_COPY_FILES += device/generic/goldfish/dir/proc/cpuinfo:root/proc/cpuinfo
PRODUCT_COPY_FILES += device/generic/goldfish/dir/proc/version:root/proc/version
PRODUCT_COPY_FILES += device/generic/goldfish/dir/proc/cmdline:root/proc/cmdline

## wifi
PRODUCT_COPY_FILES += device/generic/goldfish/dir/wifi/:system/etc/
PRODUCT_COPY_FILES += device/generic/goldfish/dir/wifi/wpa_supplicant.conf:system/etc/wifi/wpa_supplicant.conf
PRODUCT_COPY_FILES += device/generic/goldfish/dir/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml
$(call inherit-product, $(SRC_TARGET_DIR)/product/embedded.mk)
