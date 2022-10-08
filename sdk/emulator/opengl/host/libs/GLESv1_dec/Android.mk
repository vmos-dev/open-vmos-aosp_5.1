LOCAL_PATH := $(call my-dir)

host_common_debug_CFLAGS :=

#For gl debbuging
#host_common_debug_CFLAGS += -DCHECK_GL_ERROR
#host_common_debug_CFLAGS += -DDEBUG_PRINTOUT


### host library #########################################
$(call emugl-begin-host-static-library,libGLESv1_dec)

$(call emugl-import, libOpenglCodecCommon libOpenglOsUtils)
$(call emugl-export,C_INCLUDES,$(LOCAL_PATH))

$(call emugl-gen-decoder,$(LOCAL_PATH),gl)

LOCAL_SRC_FILES := GLDecoder.cpp

$(call emugl-export,CFLAGS,$(host_common_debug_CFLAGS))
$(call emugl-export,LDLIBS,-lstdc++)

$(call emugl-end-module)


### host library, 64-bit ####################################
ifdef EMUGL_BUILD_64BITS
    $(call emugl-begin-host64-static-library,lib64GLESv1_dec)

    $(call emugl-import, lib64OpenglCodecCommon lib64OpenglOsUtils)
    $(call emugl-export,C_INCLUDES,$(LOCAL_PATH))

    $(call emugl-gen-decoder,$(LOCAL_PATH),gl)

    LOCAL_SRC_FILES := GLDecoder.cpp

    $(call emugl-export,CFLAGS,$(host_common_debug_CFLAGS) -m64)
    $(call emugl-export,LDLIBS,-lstdc++)

    $(call emugl-end-module)
endif