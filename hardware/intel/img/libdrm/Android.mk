LOCAL_PATH:= $(call my-dir)

ifeq ($(ENABLE_IMG_GRAPHICS),true)

common_CFLAGS := -W -g -DPLATFORM_ANDROID
common_C_INCLUDES +=  \
   $(TARGET_OUT_HEADERS)/drm \
   $(TARGET_OUT_HEADERS)/libdrm \
   $(TARGET_OUT_HEADERS)/libdrm/shared-core

include $(CLEAR_VARS)

common_SRC_FILES := \
	libdrm/libdrm_lists.h \
	libdrm/xf86drm.c \
	libdrm/xf86drmHash.c \
	libdrm/xf86drmRandom.c \
	libdrm/xf86drmMode.c \
	libdrm/xf86drmSL.c \


ifeq ($(TARGET_ARCH),arm)
	LOCAL_CFLAGS += -fstrict-aliasing -fomit-frame-pointer
endif

LOCAL_CFLAGS += $(common_CFLAGS)
ifeq ($(TARGET_OS)-$(TARGET_ARCH),linux-x86)
LOCAL_CFLAGS += -DUSTL_ANDROID_X86
endif

LOCAL_SRC_FILES := $(common_SRC_FILES)
LOCAL_C_INCLUDES += $(common_C_INCLUDES)
LOCAL_MODULE := libdrm
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_COPY_HEADERS := \
	libdrm/xf86drm.h libdrm/xf86drmMode.h \
	shared-core/drm.h shared-core/drm_mode.h shared-core/drm_sarea.h
LOCAL_COPY_HEADERS_TO := libdrm
include $(BUILD_COPY_HEADERS)

include $(CLEAR_VARS)
LOCAL_COPY_HEADERS := \
    shared-core/i915_drm.h \
    shared-core/mach64_drm.h \
    shared-core/mga_drm.h \
    shared-core/nouveau_drm.h \
    shared-core/r128_drm.h \
    shared-core/r300_reg.h \
    shared-core/radeon_drm.h \
    shared-core/savage_drm.h \
    shared-core/sis_drm.h \
    shared-core/via_3d_reg.h \
    shared-core/via_drm.h \
    shared-core/xgi_drm.h
LOCAL_COPY_HEADERS_TO := libdrm/shared-core
include $(BUILD_COPY_HEADERS)

endif
