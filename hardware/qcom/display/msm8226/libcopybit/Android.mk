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

LOCAL_PATH:= $(call my-dir)
include $(LOCAL_PATH)/../common.mk
include $(CLEAR_VARS)

LOCAL_COPY_HEADERS_TO         := $(common_header_export_path)
LOCAL_COPY_HEADERS            := copybit.h copybit_priv.h c2d2.h
#Copy the headers regardless of whether copybit is built
include $(BUILD_COPY_HEADERS)

LOCAL_MODULE                  := copybit.$(TARGET_BOARD_PLATFORM)
LOCAL_MODULE_RELATIVE_PATH    := hw
LOCAL_MODULE_TAGS             := optional
LOCAL_C_INCLUDES              := $(common_includes) $(kernel_includes)
LOCAL_SHARED_LIBRARIES        := $(common_libs) libdl libmemalloc
LOCAL_CFLAGS                  := $(common_flags) -DLOG_TAG=\"qdcopybit\"
LOCAL_ADDITIONAL_DEPENDENCIES := $(common_deps)

ifeq ($(TARGET_USES_C2D_COMPOSITION),true)
    LOCAL_CFLAGS += -DCOPYBIT_Z180=1 -DC2D_SUPPORT_DISPLAY=1
    LOCAL_SRC_FILES := copybit_c2d.cpp software_converter.cpp
    include $(BUILD_SHARED_LIBRARY)
else
    ifneq ($(call is-chipset-in-board-platform,msm7630),true)
        ifeq ($(call is-board-platform-in-list,$(MSM7K_BOARD_PLATFORMS)),true)
            LOCAL_CFLAGS += -DCOPYBIT_MSM7K=1
            LOCAL_SRC_FILES := software_converter.cpp copybit.cpp
            include $(BUILD_SHARED_LIBRARY)
        endif
        ifeq ($(call is-board-platform-in-list,msm8610),true)
            LOCAL_SRC_FILES := software_converter.cpp copybit.cpp
            include $(BUILD_SHARED_LIBRARY)
        endif
    endif
endif
