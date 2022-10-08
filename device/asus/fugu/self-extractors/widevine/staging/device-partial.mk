# Copyright 2014 The Android Open Source Project
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

# Widevine blob(s) necessary for Fugu hardware
PRODUCT_COPY_FILES := \
    vendor/widevine/fugu/proprietary/com.google.widevine.software.drm.xml:system/etc/permissions/com.google.widevine.software.drm.xml:widevine \
    vendor/widevine/fugu/proprietary/com.google.widevine.software.drm.jar:system/framework/com.google.widevine.software.drm.jar:widevine \
    vendor/widevine/fugu/proprietary/libdrmframework.so:system/lib/arm/libdrmframework.so:widevine \
    vendor/widevine/fugu/proprietary/libdrmwvmplugin.so:system/vendor/lib/drm/libdrmwvmplugin.so:widevine \
    vendor/widevine/fugu/proprietary/libdrmdecrypt.so:system/vendor/lib/libdrmdecrypt.so:widevine \
    vendor/widevine/fugu/proprietary/liboemcrypto.so:system/vendor/lib/liboemcrypto.so:widevine \
    vendor/widevine/fugu/proprietary/libwvdrm_L1.so:system/vendor/lib/libwvdrm_L1.so:widevine \
    vendor/widevine/fugu/proprietary/libwvm.so:system/vendor/lib/libwvm.so:widevine \
    vendor/widevine/fugu/proprietary/libWVStreamControlAPI_L1.so:system/vendor/lib/libWVStreamControlAPI_L1.so:widevine \
    vendor/widevine/fugu/proprietary/libwvdrmengine.so:system/vendor/lib/mediadrm/libwvdrmengine.so:widevine \

