# -*- mode: makefile -*-
# Copyright (C) 2007 The Android Open Source Project
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
# Definitions for building the native code needed for the core library.
#

#
# Common definitions for host and target.
#

# These two definitions are used to help sanity check what's put in
# sub.mk. See, the "error" directives immediately below.
core_magic_local_target := ...//::default:://...
core_local_path := $(LOCAL_PATH)

# Include a submakefile, resolve its source file locations,
# and stick them on core_src_files.  The submakefiles are
# free to append to LOCAL_SRC_FILES, LOCAL_C_INCLUDES,
# LOCAL_SHARED_LIBRARIES, or LOCAL_STATIC_LIBRARIES, but nothing
# else. All other LOCAL_* variables will be ignored.
#
# $(1): directory containing the makefile to include
define include-core-native-dir
    LOCAL_SRC_FILES :=
    include $(LOCAL_PATH)/$(1)/sub.mk
    ifneq ($$(LOCAL_MODULE),$(core_magic_local_target))
        $$(error $(LOCAL_PATH)/$(1)/sub.mk should not include CLEAR_VARS \
            or define LOCAL_MODULE)
    endif
    ifneq ($$(LOCAL_PATH),$(core_local_path))
        $$(error $(LOCAL_PATH)/$(1)/sub.mk should not define LOCAL_PATH)
    endif
    core_src_files += $$(addprefix $(1)/,$$(LOCAL_SRC_FILES))
    LOCAL_SRC_FILES :=
endef

# Set up the default state. Note: We use CLEAR_VARS here, even though
# we aren't quite defining a new rule yet, to make sure that the
# sub.mk files don't see anything stray from the last rule that was
# set up.

include $(CLEAR_VARS)
LOCAL_MODULE := $(core_magic_local_target)
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/NativeCode.mk
core_src_files :=

# Include the sub.mk files.
$(foreach dir, \
    dalvik/src/main/native luni/src/main/native, \
    $(eval $(call include-core-native-dir,$(dir))))

# Extract out the allowed LOCAL_* variables.
core_c_includes := libcore/include $(LOCAL_C_INCLUDES)
core_shared_libraries := $(LOCAL_SHARED_LIBRARIES)
core_static_libraries := $(LOCAL_STATIC_LIBRARIES)
core_cflags := $(LOCAL_CFLAGS) -Wall -Wextra -Werror
core_cppflags += -std=gnu++11

core_test_files := \
  luni/src/test/native/dalvik_system_JniTest.cpp \
  luni/src/test/native/test_openssl_engine.cpp \

#
# Build for the target (device).
#

include $(CLEAR_VARS)
LOCAL_CFLAGS += $(core_cflags)
LOCAL_CPPFLAGS += $(core_cppflags)
LOCAL_SRC_FILES += $(core_src_files)
LOCAL_C_INCLUDES += $(core_c_includes)
LOCAL_SHARED_LIBRARIES += $(core_shared_libraries) libcrypto libdl libexpat libicuuc libicui18n libnativehelper libz libutils
LOCAL_STATIC_LIBRARIES += $(core_static_libraries) libziparchive
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libjavacore
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/NativeCode.mk
include external/stlport/libstlport.mk
include $(BUILD_SHARED_LIBRARY)

# Test JNI library.
ifeq ($(LIBCORE_SKIP_TESTS),)

include $(CLEAR_VARS)
LOCAL_CFLAGS += $(core_cflags)
LOCAL_CPPFLAGS += $(core_cppflags)
LOCAL_SRC_FILES += $(core_test_files)
LOCAL_C_INCLUDES += libcore/include external/openssl/include
LOCAL_SHARED_LIBRARIES += libcrypto
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libjavacoretests
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/NativeCode.mk
include external/stlport/libstlport.mk
include $(BUILD_SHARED_LIBRARY)

endif # LIBCORE_SKIP_TESTS


#
# Build for the host.
#

include $(CLEAR_VARS)
LOCAL_CLANG := true
LOCAL_SRC_FILES += $(core_src_files)
LOCAL_CFLAGS += $(core_cflags)
LOCAL_C_INCLUDES += $(core_c_includes)
LOCAL_CPPFLAGS += $(core_cppflags)
LOCAL_LDLIBS += -ldl -lpthread
ifeq ($(HOST_OS),linux)
LOCAL_LDLIBS += -lrt
endif
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libjavacore
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/NativeCode.mk
LOCAL_SHARED_LIBRARIES += $(core_shared_libraries) libexpat-host libicuuc-host libicui18n-host libcrypto-host libz-host
LOCAL_STATIC_LIBRARIES += $(core_static_libraries) libziparchive-host libutils
LOCAL_MULTILIB := both
include $(BUILD_HOST_SHARED_LIBRARY)

ifeq ($(LIBCORE_SKIP_TESTS),)
    include $(CLEAR_VARS)
    LOCAL_CLANG := true
    LOCAL_SRC_FILES += $(core_test_files)
    LOCAL_CFLAGS += $(core_cflags)
    LOCAL_C_INCLUDES += libcore/include external/openssl/include
    LOCAL_CPPFLAGS += $(core_cppflags)
    LOCAL_LDLIBS += -ldl -lpthread
    LOCAL_MODULE_TAGS := optional
    LOCAL_MODULE := libjavacoretests
    LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/NativeCode.mk
    LOCAL_SHARED_LIBRARIES := libcrypto-host
    include $(BUILD_HOST_SHARED_LIBRARY)
endif # LIBCORE_SKIP_TESTS
