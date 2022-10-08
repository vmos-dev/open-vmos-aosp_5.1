LOCAL_PATH:=$(call my-dir)

$(call emugl-begin-host-executable,emulator_test_renderer)
$(call emugl-import,libOpenglRender event_injector)

LOCAL_SRC_FILES := main.cpp

LOCAL_CFLAGS += $(EMUGL_SDL_CFLAGS) -g -O0
LOCAL_LDLIBS += $(EMUGL_SDL_LDLIBS)

LOCAL_STATIC_LIBRARIES += libSDL libSDLmain

$(call emugl-end-module)
