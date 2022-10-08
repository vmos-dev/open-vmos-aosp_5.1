# Copyright 2011 The Android Open Source Project
#
# Android.mk for avdlauncher
#
# The "AVD Launcher" is for Windows only.
# This simple .exe will sit at the root of the Windows SDK
# and currently simply executes tools\android.bat.
# Eventually it should simply replace the batch file.


#----- The current C++ avdlauncher -----

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(HOST_OS),windows)

LOCAL_SRC_FILES := \
	src/source/avdlauncher.c

LOCAL_CFLAGS += -Wall -Wno-unused-parameter
LOCAL_CFLAGS += -D_XOPEN_SOURCE -D_GNU_SOURCE -DSH_HISTORY
LOCAL_MODULE := avdlauncher

LOCAL_MODULE_TAGS := optional

# Locate windres executable
WINDRES := windres
ifneq ($(USE_MINGW),)
  # When building the resources under Linux, use the MinGW one
  WINDRES := i586-mingw32msvc-windres
endif

# Link the Windows icon file as well into the executable, based on the technique
# used in external/qemu/Makefile.android.  The variables need to have different
# names to not interfere with the ones from qemu/Makefile.android.
#
INTERMEDIATE     := $(call intermediates-dir-for,EXECUTABLES,$(LOCAL_MODULE),true)
AVDLAUNCHER_ICON_OBJ := avdlauncher_icon.o
AVDLAUNCHER_ICON_PATH := $(LOCAL_PATH)/images
$(AVDLAUNCHER_ICON_PATH)/$(AVDLAUNCHER_ICON_OBJ): $(AVDLAUNCHER_ICON_PATH)/android_icon.rc
	$(WINDRES) $< -I $(AVDLAUNCHER_ICON_PATH) -o $@

# seems to be the only way to add an object file that was not generated from
# a C/C++/Java source file to our build system. and very unfortunately,
# $(TOPDIR)/$(LOCALPATH) will always be prepended to this value, which forces
# us to put the object file in the source directory...
#
LOCAL_PREBUILT_OBJ_FILES += images/$(AVDLAUNCHER_ICON_OBJ)

include $(BUILD_HOST_EXECUTABLE)

$(call dist-for-goals,droid,$(LOCAL_BUILT_MODULE))

endif

