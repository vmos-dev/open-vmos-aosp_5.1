#
# Copyright (C) 2014 The Android Open Source Project
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

# This file contains the definitions needed for a _really_ minimal system
# image to be run under emulation under upstream QEMU (www.qemu.org), once
# it supports a few Android virtual devices. Note that this is _not_ the
# same as running under the Android emulator.

# This should only contain what's necessary to boot the system, support
# ADB, and allow running command-line executable compiled against the
# following NDK libraries: libc, libm, libstdc++, libdl, liblog

# Host modules
PRODUCT_PACKAGES += \
    adb \

# Device modules
PRODUCT_PACKAGES += \
    adbd \
    bootanimation \
    debuggerd \
    debuggerd64 \
    dumpstate \
    dumpsys \
    e2fsck \
    gzip \
    healthd \
    init \
    init.environ.rc \
    init.rc \
    libbinder \
    libc \
    libctest \
    libcutils \
    libdl \
    libhardware \
    libhardware_legacy \
    liblog \
    libm \
    libstdc++ \
    libstlport \
    libsysutils \
    libutils \
    linker \
    linker64 \
    logcat \
    logd \
    logwrapper \
    mkshrc \
    netd \
    qemu-props \
    reboot \
    service \
    servicemanager \
    sh \
    toolbox \
    vold \

# SELinux packages
PRODUCT_PACKAGES += \
    sepolicy \
    file_contexts \
    seapp_contexts \
    property_contexts \
    mac_permissions.xml \

PRODUCT_COPY_FILES += \
    system/core/rootdir/init.usb.rc:root/init.usb.rc \
    system/core/rootdir/init.trace.rc:root/init.trace.rc \
    system/core/rootdir/ueventd.rc:root/ueventd.rc \
    system/core/rootdir/etc/hosts:system/etc/hosts \

PRODUCT_COPY_FILES += \
    device/generic/goldfish/fstab.goldfish:root/fstab.goldfish \
    device/generic/goldfish/init.goldfish.rc:root/init.goldfish.rc \
    device/generic/goldfish/init.goldfish.sh:system/etc/init.goldfish.sh \
    device/generic/goldfish/ueventd.goldfish.rc:root/ueventd.goldfish.rc \
