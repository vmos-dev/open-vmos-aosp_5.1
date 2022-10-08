# This build script corresponds to a library containing many definitions
# common to both the guest and the host. They relate to
#
LOCAL_PATH := $(call my-dir)

commonSources := \
        GLClientState.cpp \
        GLSharedGroup.cpp \
        glUtils.cpp \
        SocketStream.cpp \
        TcpStream.cpp \
        TimeUtils.cpp

host_commonSources := $(commonSources)

ifeq ($(HOST_OS),windows)
    host_commonSources += Win32PipeStream.cpp
else
    host_commonSources += UnixStream.cpp
endif


### OpenglCodecCommon  host ##############################################
$(call emugl-begin-host-static-library,libOpenglCodecCommon)

LOCAL_SRC_FILES := $(host_commonSources)
$(call emugl-import, libemugl_common)
$(call emugl-export,C_INCLUDES,$(EMUGL_PATH)/host/include/libOpenglRender $(LOCAL_PATH))
$(call emugl-export,LDLIBS,-lstdc++)
$(call emugl-end-module)


### OpenglCodecCommon  host, 64-bit #########################################
ifdef EMUGL_BUILD_64BITS
    $(call emugl-begin-host64-static-library,lib64OpenglCodecCommon)

    LOCAL_SRC_FILES := $(host_commonSources)

    $(call emugl-import, lib64emugl_common)
    $(call emugl-export,C_INCLUDES,$(EMUGL_PATH)/host/include/libOpenglRender $(LOCAL_PATH))
    $(call emugl-export,CFLAGS,-m64 -fPIC)
    $(call emugl-export,LDLIBS,-lstdc++)
    $(call emugl-end-module)
endif
