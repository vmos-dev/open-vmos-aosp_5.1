# This contains common definitions used to define a host module to
# link SDL with the EmuGL test programs.

ifeq ($(BUILD_STANDALONE_EMULATOR),true)

# When using the emulator standalone build, inherit the values from the
# Makefile that included us.
EMUGL_SDL_CFLAGS := $(SDL_CFLAGS)
EMUGL_SDL_LDLIBS := $(SDL_LDLIBS)
EMUGL_SDL_STATIC_LIBRARIES := emulator_libSDL emulator_libSDLmain

else  # BUILD_STANDALONE_EMULATOR != true

# Otherwise, use the prebuilt libraries that come with the platform.

EMUGL_SDL_CONFIG ?= prebuilts/tools/$(HOST_PREBUILT_TAG)/sdl/bin/sdl-config
EMUGL_SDL_CFLAGS := $(shell $(EMUGL_SDL_CONFIG) --cflags)
EMUGL_SDL_LDLIBS := $(filter-out %.a %.lib,$(shell $(EMUGL_SDL_CONFIG) --static-libs))
EMUGL_SDL_STATIC_LIBRARIES := libSDL libSDLmain

ifeq ($(HOST_OS),windows)
EMUGL_SDL_LDLIBS += -lws2_32
endif

endif  # BUILD_STANDALONE_EMULATOR != true
