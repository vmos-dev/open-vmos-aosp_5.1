LOCAL_PATH:= $(call my-dir)

$(call emugl-begin-host-executable,triangleV2)
$(call emugl-import,libEGL_translator libGLES_V2_translator)

LOCAL_SRC_FILES:= \
        triangleV2.cpp

LOCAL_CFLAGS += $(EMUGL_SDL_CFLAGS) -g -O0
LOCAL_LDLIBS += $(EMUGL_SDL_LDLIBS) -lstdc++

LOCAL_STATIC_LIBRARIES += $(EMUGL_SDL_STATIC_LIBRARIES)

ifeq ($(HOST_OS),darwin)
  # SDK 10.6+ deprecates __dyld_func_lookup required by dlcompat_init_func
  # in SDL_dlcompat.o this module depends.  Instruct linker to resolved it at runtime.
  OSX_VERSION_MAJOR := $(shell echo $(mac_sdk_version) | cut -d . -f 2)
  OSX_VERSION_MAJOR_GREATER_THAN_OR_EQUAL_TO_6 := $(shell [ $(OSX_VERSION_MAJOR) -ge 6 ] && echo true)
  ifeq ($(OSX_VERSION_MAJOR_GREATER_THAN_OR_EQUAL_TO_6),true)
    LOCAL_LDLIBS += -Wl,-undefined,dynamic_lookup
  endif
  $(call emugl-import,libMac_view)
endif

$(call emugl-end-module)
