# Copyright 2011 The Android Open Source Project
#
# Android.mk for find_lock.exe & static library


LOCAL_PATH := $(call my-dir)

# "find_lock.exe"
# ===============

include $(CLEAR_VARS)

ifeq ($(HOST_OS),windows)

LOCAL_SRC_FILES := \
	find_lock.cpp \
	find_lock_exe.cpp

LOCAL_MODULE := find_lock
LOCAL_STATIC_LIBRARIES := libfindjava

LOCAL_CFLAGS += -Wall -Wno-unused-parameter
LOCAL_CFLAGS += -D_XOPEN_SOURCE -D_GNU_SOURCE -DSH_HISTORY -DUSE_MINGW
LOCAL_CFLAGS += -I$(TOPDIR)sdk/find_java

LOCAL_MODULE_TAGS := optional

# Locate windres executable
WINDRES := windres
ifneq ($(USE_MINGW),)
  # When building the Windows resources under Linux, use the MinGW one
  WINDRES := i586-mingw32msvc-windres
endif

# Link the Windows icon file as well into the executable, based on the technique
# used in external/qemu/Makefile.android.  The variables need to have different
# names to not interfere with the ones from qemu/Makefile.android.
#
INTERMEDIATE         := $(call intermediates-dir-for,EXECUTABLES,$(LOCAL_MODULE),true)
FIND_LOCK_ICON_OBJ  := find_lock_icon.o
FIND_LOCK_ICON_PATH := $(LOCAL_PATH)/images
$(FIND_LOCK_ICON_PATH)/$(FIND_LOCK_ICON_OBJ): $(FIND_LOCK_ICON_PATH)/android_icon.rc
	$(WINDRES) $< -I $(FIND_LOCK_ICON_PATH) -o $@

# seems to be the only way to add an object file that was not generated from
# a C/C++/Java source file to our build system. and very unfortunately,
# $(TOPDIR)/$(LOCALPATH) will always be prepended to this value, which forces
# us to put the object file in the source directory...
#
LOCAL_PREBUILT_OBJ_FILES += images/$(FIND_LOCK_ICON_OBJ)

include $(BUILD_HOST_EXECUTABLE)

$(call dist-for-goals,droid,$(LOCAL_BUILT_MODULE))

endif


