LOCAL_PATH := $(call my-dir)

$(call emugl-begin-host-shared-library,libut_rendercontrol_dec)
$(call emugl-import, libOpenglCodecCommon)
$(call emugl-gen-decoder,$(LOCAL_PATH),ut_rendercontrol)
$(call emugl-end-module)
