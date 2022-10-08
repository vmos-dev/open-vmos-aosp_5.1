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

#
# This file includes overrides to make a Hammerhead act like a low memory device
# to test low memory situations
#

PRODUCT_RUNTIMES := runtime_libart_default

$(call inherit-product, $(SRC_TARGET_DIR)/product/core_64_bit.mk)
$(call inherit-product, device/htc/flounder/device.mk)

PRODUCT_COPY_FILES += \
	device/htc/flounder/init.flounder_svelte.rc:root/init.flounder_svelte.rc:google \

# Copy flounder files as flounder64 so ${ro.hardware} can find them
PRODUCT_COPY_FILES += \
	device/htc/flounder/init.flounder_svelte.rc:root/init.flounder64_svelte.rc:google \

#TODO: figure out if we need an overlay
#DEVICE_PACKAGE_OVERLAYS := vendor/lge/hammerhead/svelte-overlay

PRODUCT_PROPERTY_OVERRIDES += \
	ro.config.low_ram=true \
	ro.logd.size=64K \
	ro.sf.lcd_density=144

PRODUCT_PROPERTY_OVERRIDES += dalvik.vm.jit.codecachesize=0

