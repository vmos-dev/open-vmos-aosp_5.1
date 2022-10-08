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
	libdomx \
	libOMX_Core \
	libOMX.TI.DUCATI1.VIDEO.H264E \
	libOMX.TI.DUCATI1.VIDEO.MPEG4E \
	libOMX.TI.DUCATI1.VIDEO.DECODER \
	libOMX.TI.DUCATI1.VIDEO.DECODER.secure \
	libOMX.TI.DUCATI1.VIDEO.CAMERA \
	libOMX.TI.DUCATI1.MISC.SAMPLE \
	libstagefrighthw \
        libI420colorconvert \
	libtiutils \
	libcamera \
	libion_ti \
	camera.omap4 \
	libomxcameraadapter \
	hwcomposer.omap4 \
	smc_pa_ctrl \
	tf_daemon \
	libtf_crypto_sst

PRODUCT_VENDOR_KERNEL_HEADERS := hardware/ti/omap4xxx/kernel-headers
