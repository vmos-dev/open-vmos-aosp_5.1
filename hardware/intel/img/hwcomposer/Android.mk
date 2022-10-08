# Copyright (C) 2008 The Android Open Source Project
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

ifeq ($(INTEL_HWC_MOOREFIELD),true)

LOCAL_PATH := $(call my-dir)

# HAL module implemenation, not prelinked and stored in
# hw/<OVERLAY_HARDWARE_MODULE_ID>.<ro.product.board>.so
include $(CLEAR_VARS)

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := hwcomposer.$(TARGET_BOARD_PLATFORM)
LOCAL_CFLAGS := -Werror

LOCAL_SHARED_LIBRARIES := liblog libcutils libdrm \
                          libwsbm libutils libhardware \
                          libva libva-tpi libva-android libsync

LOCAL_SRC_FILES := \
    common/base/Drm.cpp \
    common/base/HwcLayer.cpp \
    common/base/HwcLayerList.cpp \
    common/base/Hwcomposer.cpp \
    common/base/HwcModule.cpp \
    common/base/DisplayAnalyzer.cpp \
    common/buffers/BufferCache.cpp \
    common/buffers/GraphicBuffer.cpp \
    common/buffers/BufferManager.cpp \
    common/devices/DummyDevice.cpp \
    common/devices/PhysicalDevice.cpp \
    common/devices/PrimaryDevice.cpp \
    common/devices/ExternalDevice.cpp \
    common/observers/UeventObserver.cpp \
    common/observers/VsyncEventObserver.cpp \
    common/observers/SoftVsyncObserver.cpp \
    common/planes/DisplayPlane.cpp \
    common/planes/DisplayPlaneManager.cpp \
    common/utils/Dump.cpp

LOCAL_SRC_FILES += \
    ips/common/BlankControl.cpp \
    ips/common/HdcpControl.cpp \
    ips/common/DrmControl.cpp \
    ips/common/VsyncControl.cpp \
    ips/common/OverlayPlaneBase.cpp \
    ips/common/SpritePlaneBase.cpp \
    ips/common/PixelFormat.cpp \
    ips/common/GrallocBufferBase.cpp \
    ips/common/GrallocBufferMapperBase.cpp \
    ips/common/TTMBufferMapper.cpp \
    ips/common/DrmConfig.cpp \
    ips/common/Wsbm.cpp \
    ips/common/WsbmWrapper.c \
    ips/common/RotationBufferProvider.cpp

LOCAL_SRC_FILES += \
    ips/tangier/TngGrallocBuffer.cpp \
    ips/tangier/TngGrallocBufferMapper.cpp \
    ips/tangier/TngDisplayQuery.cpp \
    ips/tangier/TngDisplayContext.cpp

LOCAL_SRC_FILES += \
    ips/anniedale/AnnPlaneManager.cpp \
    ips/anniedale/AnnOverlayPlane.cpp \
    ips/anniedale/AnnRGBPlane.cpp \
    ips/anniedale/AnnCursorPlane.cpp \
    ips/anniedale/PlaneCapabilities.cpp

LOCAL_SRC_FILES += \
    platforms/merrifield_plus/PlatfBufferManager.cpp \
    platforms/merrifield_plus/PlatfPrimaryDevice.cpp \
    platforms/merrifield_plus/PlatfExternalDevice.cpp \
    platforms/merrifield_plus/PlatfHwcomposer.cpp

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/include \
    $(LOCAL_PATH)/include/pvr/hal \
    $(TARGET_OUT_HEADERS)/libdrm \
    $(TARGET_OUT_HEADERS)/libwsbm/wsbm \
    $(TARGET_OUT_HEADERS)/libttm \
    frameworks/native/include/media/openmax

ifeq ($(TARGET_SUPPORT_HDMI_PRIMARY),true)
   LOCAL_CFLAGS += -DINTEL_SUPPORT_HDMI_PRIMARY
endif

LOCAL_COPY_HEADERS:=include/pvr/hal/hal_public.h
LOCAL_COPY_HEADERS_TO:=pvr/hal

include $(BUILD_SHARED_LIBRARY)

endif
