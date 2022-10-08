
LOCAL_PATH := $(call my-dir)

ifeq ($(TARGET_BOARD_PLATFORM),moorefield)

include $(CLEAR_VARS)
LOCAL_COPY_HEADERS := \
    libmediaparser/mixvbp/vbp_manager/include/vbp_loader.h
LOCAL_COPY_HEADERS_TO := libmixvbp
include $(BUILD_COPY_HEADERS)

include $(CLEAR_VARS)
LOCAL_COPY_HEADERS := \
    DRM/cc54/inc/sepdrm.h \
    DRM/cc54/inc/drm_common_api.h \
    DRM/cc54/inc/drm_error.h \
    DRM/cc54/inc/pr_drm_api.h \
    DRM/cc54/inc/wv_drm_api.h \
    DRM/cc54/inc/wv_fkp.h \
    DRM/cc54/inc/wv_mod_drm_api.h \
    DRM/cc54/inc/wv_mod_drm_error.h \
    DRM/cc54/inc/wv_mod_oem_crypto.h
include $(BUILD_COPY_HEADERS)

endif # ifeq ($(TARGET_BOARD_PLATFORM),moorefield)