ifeq ($(ENABLE_IMG_GRAPHICS),true)

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:=          \
   wsbm_driver.c           \
   wsbm_fencemgr.c         \
   wsbm_mallocpool.c       \
   wsbm_manager.c          \
   wsbm_mm.c           \
   wsbm_slabpool.c         \
   wsbm_ttmpool.c          \
   wsbm_userpool.c

LOCAL_CFLAGS += -DHAVE_CONFIG_H
LOCAL_C_INCLUDES :=            \
   $(LOCAL_PATH)/../       \
   $(TARGET_OUT_HEADERS)/drm \
   $(TARGET_OUT_HEADERS)/ipp \
   $(TARGET_OUT_HEADERS)/libdrm \
   $(TARGET_OUT_HEADERS)/libdrm/shared-core \
   $(TARGET_OUT_HEADERS)/libttm

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE:= libwsbm
LOCAL_SHARED_LIBRARIES:= libdrm
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_COPY_HEADERS_TO := libwsbm/wsbm
LOCAL_COPY_HEADERS :=          \
   wsbm_atomic.h           \
   wsbm_driver.h           \
   wsbm_fencemgr.h         \
   wsbm_manager.h          \
   wsbm_mm.h           \
   wsbm_pool.h         \
   wsbm_priv.h         \
   wsbm_util.h
include $(BUILD_COPY_HEADERS)

endif # ($(ENABLE_IMG_GRAPHICS),true)

