# Determine if the emugen build needs to be builts from
# sources.
EMUGL_BUILD_EMUGEN :=
ifeq (true,$(BUILD_STANDALONE_EMULATOR))
  # The emulator's standalone build system can build host Linux
  # binaries even when it targets Windows by setting
  # LOCAL_HOST_BUILD to true, so rebuild from sources.
  EMUGL_BUILD_EMUGEN := true
else
  ifneq ($(HOST_OS),windows)
    # The platform build can only build emugen when targetting
    # the same host sytem.
    EMUGL_BUILD_EMUGEN := true
  endif
endif

LOCAL_PATH:=$(call my-dir)

ifeq (true,$(EMUGL_BUILD_EMUGEN))

$(call emugl-begin-host-executable,emugen)

LOCAL_SRC_FILES := \
    ApiGen.cpp \
    EntryPoint.cpp \
    main.cpp \
    strUtils.cpp \
    TypeFactory.cpp \

ifeq (true,$(BUILD_STANDALONE_EMULATOR))
LOCAL_HOST_BUILD := true
endif

$(call emugl-end-module)

# The location of the emugen host tool that is used to generate wire
# protocol encoders/ decoders. This variable is used by other emugl modules.
EMUGL_EMUGEN := $(LOCAL_BUILT_MODULE)

else # windows platform build

# on windows use the build host emugen executable
# (that will be the linux exeutable when using mingw build)
EMUGL_EMUGEN := $(BUILD_OUT_EXECUTABLES)/emugen

endif
