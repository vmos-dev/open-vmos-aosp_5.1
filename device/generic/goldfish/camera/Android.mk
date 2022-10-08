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


LOCAL_PATH := $(call my-dir)

ifeq ($(PRODUCT_OPENSRC_USE_PREBUILT),yes)

include $(CLEAR_VARS)

LOCAL_MODULE_RELATIVE_PATH := hw

LOCAL_MODULE := camera.goldfish

LOCAL_MULTILIB := both
LOCAL_SRC_FILES_arm := prebuilt/arm/camera.goldfish.so
LOCAL_SRC_FILES_arm64 := prebuilt/arm64/camera.goldfish.so

LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES

LOCAL_UNINSTALLABLE_MODULE := true

$(shell mkdir -p $(TARGET_OUT)/lib/$(LOCAL_MODULE_RELATIVE_PATH)/)
$(shell cp -rf $(LOCAL_PATH)/prebuilt/arm/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)_result $(TARGET_OUT)/lib/$(LOCAL_MODULE_RELATIVE_PATH)/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX))
ifeq ($(TARGET_IS_64_BIT),true)
$(shell mkdir -p $(TARGET_OUT)/lib64/$(LOCAL_MODULE_RELATIVE_PATH)/)
$(shell cp -rf $(LOCAL_PATH)/prebuilt/arm64/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)_result $(TARGET_OUT)/lib64/$(LOCAL_MODULE_RELATIVE_PATH)/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX))
endif

include $(BUILD_PREBUILT)

#################################################################
ifneq ($(TARGET_BUILD_PDK),true)

include $(CLEAR_VARS)

LOCAL_MODULE_RELATIVE_PATH := hw

LOCAL_MODULE := camera.goldfish.jpeg

LOCAL_MULTILIB := both
LOCAL_SRC_FILES_arm := prebuilt/arm/camera.goldfish.jpeg.so
LOCAL_SRC_FILES_arm64 := prebuilt/arm64/camera.goldfish.jpeg.so

LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES

LOCAL_UNINSTALLABLE_MODULE := true

$(shell mkdir -p $(TARGET_OUT)/lib/$(LOCAL_MODULE_RELATIVE_PATH)/)
$(shell mkdir -p $(TARGET_OUT)/lib64/$(LOCAL_MODULE_RELATIVE_PATH)/)
$(shell cp -rf $(LOCAL_PATH)/prebuilt/arm/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)_result $(TARGET_OUT)/lib/$(LOCAL_MODULE_RELATIVE_PATH)/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX))
ifeq ($(TARGET_IS_64_BIT),true)
$(shell cp -rf $(LOCAL_PATH)/prebuilt/arm64/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)_result $(TARGET_OUT)/lib64/$(LOCAL_MODULE_RELATIVE_PATH)/$(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX))
endif

include $(BUILD_PREBUILT)

endif # !PDK

else

include $(CLEAR_VARS)

LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_CFLAGS += -fno-short-enums -DQEMU_HARDWARE
LOCAL_CFLAGS += -Wno-unused-parameter -Wno-missing-field-initializers
LOCAL_SHARED_LIBRARIES:= \
    libbinder \
    liblog \
    libutils \
    libcutils \
    libcamera_client \
    libui \
    libdl

# JPEG conversion libraries and includes.
LOCAL_SHARED_LIBRARIES += \
	libjpeg \
	libcamera_metadata

LOCAL_C_INCLUDES += external/jpeg \
	frameworks/native/include/media/hardware \
	$(LOCAL_PATH)/../opengl/system/OpenglSystemCommon \
	$(call include-path-for, camera)

LOCAL_SRC_FILES := \
	EmulatedCameraHal.cpp \
	EmulatedCameraFactory.cpp \
	EmulatedCameraHotplugThread.cpp \
	EmulatedBaseCamera.cpp \
	EmulatedCamera.cpp \
		EmulatedCameraDevice.cpp \
		EmulatedQemuCamera.cpp \
		EmulatedQemuCameraDevice.cpp \
		EmulatedFakeCamera.cpp \
		EmulatedFakeCameraDevice.cpp \
		Converters.cpp \
		PreviewWindow.cpp \
		CallbackNotifier.cpp \
		QemuClient.cpp \
		JpegCompressor.cpp \
	EmulatedCamera2.cpp \
		EmulatedFakeCamera2.cpp \
		EmulatedQemuCamera2.cpp \
		fake-pipeline2/Scene.cpp \
		fake-pipeline2/Sensor.cpp \
		fake-pipeline2/JpegCompressor.cpp \
	EmulatedCamera3.cpp \
		EmulatedFakeCamera3.cpp


ifeq ($(TARGET_PRODUCT),vbox_x86)
LOCAL_MODULE := camera.vbox_x86
else
LOCAL_MODULE := camera.goldfish
endif

include $(BUILD_SHARED_LIBRARY)

#################################################################
ifneq ($(TARGET_BUILD_PDK),true)

include $(CLEAR_VARS)

LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_CFLAGS += -fno-short-enums -DQEMU_HARDWARE
LOCAL_CFLAGS += -Wno-unused-parameter
LOCAL_SHARED_LIBRARIES:= \
    libcutils \
    liblog \
    libskia \
    libandroid_runtime

LOCAL_C_INCLUDES += external/jpeg \
                    external/skia/include/core/ \
                    frameworks/base/core/jni/android/graphics \
                    frameworks/native/include

LOCAL_SRC_FILES := JpegStub.cpp

LOCAL_MODULE := camera.goldfish.jpeg

include $(BUILD_SHARED_LIBRARY)

endif # !PDK
endif # !PRODUCT_OPENSRC_USE_PREBUILT