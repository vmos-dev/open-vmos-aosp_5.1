LOCAL_PATH := $(call my-dir)


### host library ############################################
$(call emugl-begin-host-static-library,lib_renderControl_dec)
$(call emugl-import,libOpenglCodecCommon)
$(call emugl-gen-decoder,$(LOCAL_PATH),renderControl)
# For renderControl_types.h
$(call emugl-export,C_INCLUDES,$(LOCAL_PATH))
$(call emugl-end-module)

### host library, 64-bit ####################################
ifdef EMUGL_BUILD_64BITS
    $(call emugl-begin-host64-static-library,lib64_renderControl_dec)
    $(call emugl-import,lib64OpenglCodecCommon)
    $(call emugl-gen-decoder,$(LOCAL_PATH),renderControl)
    # For renderControl_types.h
    $(call emugl-export,C_INCLUDES,$(LOCAL_PATH))
    $(call emugl-export,CFLAGS,-m64)
    $(call emugl-end-module)
endif