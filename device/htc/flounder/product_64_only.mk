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
# This file is the build configuration that is shared by all products
# based on the flounder device that want only a 64-bit zygote (no
# 32-bit app support)
#

# Make sure we have the 64b zygote service
PRODUCT_COPY_FILES += system/core/rootdir/init.zygote64.rc:root/init.zygote64.rc

# Use 64b single zygote
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += ro.zygote=zygote64

# Disable bluetooth because of continuous driver crashes
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += config.disable_bluetooth=true

$(call inherit-product, device/htc/flounder/product.mk)
