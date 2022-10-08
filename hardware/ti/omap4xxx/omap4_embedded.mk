# Copyright (C) 2011 The Android Open Source Project
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

# This file lists the modules that are specific to OMAP4 but are used by
# all OMAP4 devices.

PRODUCT_PACKAGES := \
	libion_ti \
	hwcomposer.omap4

PRODUCT_VENDOR_KERNEL_HEADERS := hardware/ti/omap4xxx/kernel-headers
