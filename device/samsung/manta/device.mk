#
# Copyright (C) 2011 The Android Open-Source Project
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

ifeq ($(TARGET_PREBUILT_KERNEL),)
LOCAL_KERNEL := device/samsung/manta/kernel
else
LOCAL_KERNEL := $(TARGET_PREBUILT_KERNEL)
endif

PRODUCT_COPY_FILES := \
    $(LOCAL_KERNEL):kernel \
    device/samsung/manta/init.manta.rc:root/init.manta.rc \
    device/samsung/manta/init.manta.usb.rc:root/init.manta.usb.rc \
    device/samsung/manta/init.recovery.manta.rc:root/init.recovery.manta.rc \
    device/samsung/manta/fstab.manta:root/fstab.manta \
    device/samsung/manta/ueventd.manta.rc:root/ueventd.manta.rc

# Input device files for manta
PRODUCT_COPY_FILES += \
    device/samsung/manta/Atmel_maXTouch_Touchscreen.idc:system/usr/idc/Atmel_maXTouch_Touchscreen.idc \
    device/samsung/manta/manta-keypad.kl:system/usr/keylayout/manta-keypad.kl \
    device/samsung/manta/manta-keypad.kcm:system/usr/keychars/manta-keypad.kcm


# Init files for booting smdk5250 with a manta image
PRODUCT_COPY_FILES += \
    device/samsung/manta/init.smdk5250.rc:root/init.smdk5250.rc \
    device/samsung/manta/init.smdk5250.usb.rc:root/init.smdk5250.usb.rc \
    device/samsung/manta/fstab.smdk5250:root/fstab.smdk5250 \
    device/samsung/manta/ueventd.smdk5250.rc:root/ueventd.smdk5250.rc

# Input device files for smdk5250
PRODUCT_COPY_FILES += \
    device/samsung/manta/egalax_i2c.idc:system/usr/idc/egalax_i2c.idc \
    device/samsung/manta/smdk5250-keypad.kl:system/usr/keylayout/smdk5250-keypad.kl \
    device/samsung/manta/smdk5250-keypad.kcm:system/usr/keychars/smdk5250-keypad.kcm

PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/tablet_core_hardware.xml:system/etc/permissions/tablet_core_hardware.xml \
    frameworks/native/data/etc/android.hardware.touchscreen.multitouch.jazzhand.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.jazzhand.xml \
    frameworks/native/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml \
    frameworks/native/data/etc/android.hardware.wifi.direct.xml:system/etc/permissions/android.hardware.wifi.direct.xml \
    frameworks/av/media/libstagefright/data/media_codecs_google_audio.xml:system/etc/media_codecs_google_audio.xml \
    frameworks/av/media/libstagefright/data/media_codecs_google_telephony.xml:system/etc/media_codecs_google_telephony.xml \
    frameworks/av/media/libstagefright/data/media_codecs_google_video.xml:system/etc/media_codecs_google_video.xml \
    device/samsung/manta/media_codecs.xml:system/etc/media_codecs.xml \
    device/samsung/manta/media_profiles.xml:system/etc/media_profiles.xml \
    frameworks/native/data/etc/android.hardware.camera.flash-autofocus.xml:system/etc/permissions/android.hardware.camera.flash-autofocus.xml \
    frameworks/native/data/etc/android.hardware.camera.front.xml:system/etc/permissions/android.hardware.camera.front.xml \
    frameworks/native/data/etc/android.hardware.usb.accessory.xml:system/etc/permissions/android.hardware.usb.accessory.xml \
    frameworks/native/data/etc/android.hardware.usb.host.xml:system/etc/permissions/android.hardware.usb.host.xml \
    frameworks/native/data/etc/android.hardware.location.gps.xml:system/etc/permissions/android.hardware.location.gps.xml \
    frameworks/native/data/etc/android.hardware.sensor.accelerometer.xml:system/etc/permissions/android.hardware.sensor.accelerometer.xml \
    frameworks/native/data/etc/android.hardware.sensor.barometer.xml:system/etc/permissions/android.hardware.sensor.barometer.xml \
    frameworks/native/data/etc/android.hardware.sensor.compass.xml:system/etc/permissions/android.hardware.sensor.compass.xml \
    frameworks/native/data/etc/android.hardware.sensor.gyroscope.xml:system/etc/permissions/android.hardware.sensor.gyroscope.xml \
    frameworks/native/data/etc/android.hardware.sensor.light.xml:system/etc/permissions/android.hardware.sensor.light.xml \
    frameworks/native/data/etc/android.hardware.audio.low_latency.xml:system/etc/permissions/android.hardware.audio.low_latency.xml \
    frameworks/native/data/etc/android.hardware.ethernet.xml:system/etc/permissions/android.hardware.ethernet.xml

PRODUCT_COPY_FILES += \
    device/samsung/manta/bcmdhd.cal:system/etc/wifi/bcmdhd.cal

# audio mixer paths
PRODUCT_COPY_FILES += \
    device/samsung/manta/mixer_paths.xml:system/etc/mixer_paths.xml

# audio policy configuration
PRODUCT_COPY_FILES += \
    device/samsung/manta/audio_policy.conf:system/etc/audio_policy.conf

PRODUCT_PACKAGES := \
    libwpa_client \
    hostapd \
    dhcpcd.conf \
    wpa_supplicant \
    wpa_supplicant.conf

# audio effects
PRODUCT_PACKAGES += libaudience_voicefx
PRODUCT_COPY_FILES += \
    device/samsung/manta/audio_effects.conf:system/etc/audio_effects.conf

# BCM47511 GPS
PRODUCT_COPY_FILES += \
    device/samsung/manta/gps/gps.conf:system/etc/gps.conf \
    device/samsung/manta/gps/gpsd:system/vendor/bin/gpsd \
    device/samsung/manta/gps/gps.xml:system/vendor/etc/gps.xml \
    device/samsung/manta/gps/gps.manta.so:system/lib/hw/gps.manta.so

# NFC packages
PRODUCT_PACKAGES += \
    nfc_nci.bcm2079x.default \
    NfcNci \
    Tag

# NFC access control + feature files + configuration
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.nfc.xml:system/etc/permissions/android.hardware.nfc.xml \
    frameworks/native/data/etc/android.hardware.nfc.hce.xml:system/etc/permissions/android.hardware.nfc.hce.xml \
    device/samsung/manta/nfc/libnfc-brcm.conf:system/etc/libnfc-brcm.conf

PRODUCT_PACKAGES += \
    lights.manta \
    sensors.manta

PRODUCT_AAPT_CONFIG := xlarge hdpi xhdpi
PRODUCT_AAPT_PREF_CONFIG := xhdpi

PRODUCT_CHARACTERISTICS := tablet,nosdcard

DEVICE_PACKAGE_OVERLAYS := \
    device/samsung/manta/overlay

# for now include gralloc here. should come from hardware/samsung_slsi/exynos5
PRODUCT_PACKAGES += \
    gralloc.exynos5

PRODUCT_PACKAGES += \
    libion

PRODUCT_TAGS += dalvik.gc.type-precise

PRODUCT_PACKAGES += \
    librs_jni \
    com.android.future.usb.accessory

PRODUCT_PACKAGES += \
    audio.primary.manta \
    audio.a2dp.default \
    audio.usb.default \
    libbubblelevel \
    audio.r_submix.default

PRODUCT_PACKAGES += \
    power.manta

PRODUCT_PACKAGES += \
    camera.exynos5

# Filesystem management tools
PRODUCT_PACKAGES += \
    e2fsck

PRODUCT_PROPERTY_OVERRIDES := \
    wifi.interface=wlan0 \
    ro.opengles.version=196609 \
    ro.sf.lcd_density=320 \
    ro.hwui.texture_cache_size=72 \
    ro.hwui.layer_cache_size=48 \
    ro.hwui.r_buffer_cache_size=8 \
    ro.hwui.path_cache_size=32 \
    ro.hwui.gradient_cache_size=1 \
    ro.hwui.drop_shadow_cache_size=6 \
    ro.hwui.texture_cache_flushrate=0.4 \
    ro.hwui.text_small_cache_width=1024 \
    ro.hwui.text_small_cache_height=1024 \
    ro.hwui.text_large_cache_width=2048 \
    ro.hwui.text_large_cache_height=1024 \
    ro.hwui.disable_scissor_opt=true \
    af.fast_track_multiplier=1

# setup dalvik vm configs.
$(call inherit-product, frameworks/native/build/tablet-10in-xhdpi-2048-dalvik-heap.mk)

# Enable AAC 5.1 output
PRODUCT_PROPERTY_OVERRIDES += \
    media.aac_51_output_enabled=true

# set default USB configuration
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
    persist.sys.usb.config=mtp

# for off charging mode
PRODUCT_PACKAGES += \
    charger_res_images

$(call inherit-product-if-exists, hardware/samsung_slsi/exynos5/exynos5.mk)
$(call inherit-product-if-exists, vendor/samsung_slsi/exynos5/exynos5-vendor.mk)
$(call inherit-product-if-exists, vendor/samsung/manta/device-vendor.mk)

$(call inherit-product-if-exists, hardware/broadcom/wlan/bcmdhd/firmware/bcm4324/device-bcm.mk)
