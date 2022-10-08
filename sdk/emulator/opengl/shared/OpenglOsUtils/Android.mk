# This build script corresponds to a small library containing
# OS-specific support functions for:
#   - thread-local storage
#   - dynamic library loading
#   - child process creation and wait  (probably not needed in guest)
#
LOCAL_PATH := $(call my-dir)

### Host library ##############################################

host_common_SRC_FILES := osDynLibrary.cpp
host_common_LDLIBS :=
host_common_INCLUDES := $(LOCAL_PATH)

ifeq ($(HOST_OS),windows)
    host_common_SRC_FILES += \
        osProcessWin.cpp \
        osThreadWin.cpp
    host_common_LDLIBS += -lws2_32 -lpsapi
else
    host_common_SRC_FILES += \
        osProcessUnix.cpp \
        osThreadUnix.cpp
    host_common_LDLIBS += -ldl
endif

ifeq ($(HOST_OS),linux)
    host_common_LDLIBS += -lpthread -lrt -lX11
endif

### 32-bit host library ####
$(call emugl-begin-host-static-library,libOpenglOsUtils)
    $(call emugl-export,C_INCLUDES,$(host_common_INCLUDES))
    LOCAL_SRC_FILES = $(host_common_SRC_FILES)
    $(call emugl-export,LDLIBS,$(host_common_LDLIBS))
    $(call emugl-import,libemugl_common)
$(call emugl-end-module)

### 64-bit host library ####
ifdef EMUGL_BUILD_64BITS
    $(call emugl-begin-host64-static-library,lib64OpenglOsUtils)
        $(call emugl-export,C_INCLUDES,$(host_common_INCLUDES))
        LOCAL_SRC_FILES = $(host_common_SRC_FILES)
        $(call emugl-export,LDLIBS,$(host_common_LDLIBS))
        $(call emugl-import,lib64emugl_common)
        $(call emugl-export,CFLAGS,-m64 -fPIC)
    $(call emugl-end-module)
endif
