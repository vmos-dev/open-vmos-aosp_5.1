ifneq ($(BUILD_TINY_ANDROID),true)

ifneq ($(BOARD_VENDOR_QCOM_GPS_LOC_API_AMSS_VERSION),50001)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

AMSS_VERSION:=$(BOARD_VENDOR_QCOM_GPS_LOC_API_AMSS_VERSION)
RPC_INC:=inc-$(AMSS_VERSION)

generated_files:= \
	gen-$(AMSS_VERSION)/loc_api_clnt.c	\
	gen-$(AMSS_VERSION)/loc_api_cb_xdr.c	\
	gen-$(AMSS_VERSION)/loc_api_common_xdr.c \
	gen-$(AMSS_VERSION)/loc_api_cb_svc.c \
	gen-$(AMSS_VERSION)/loc_api_xdr.c \
	gen-$(AMSS_VERSION)/loc_api_fixup.c \
	gen-$(AMSS_VERSION)/loc_api_rpc_glue.c \
	src/loc_apicb_appinit.c

LOCAL_SRC_FILES:= $(generated_files)

# removed from library build since the client should implement this code.
#	src/loc_api_cb_server.c

LOCAL_CFLAGS:=-fno-short-enums
LOCAL_CFLAGS+=-include $(RPC_INC)/loc_api_common.h
LOCAL_CFLAGS+=-DDEBUG
# LOCAL_CFLAGS+=-DDEBUG -DVERBOSE

LOCAL_CFLAGS+=-DADD_XDR_FLOAT -DADD_XDR_BOOL

LOCAL_SHARED_LIBRARIES:= librpc
LOCAL_STATIC_LIBRARIES:= libcommondefs-rpc



LOCAL_COPY_HEADERS_TO:= libloc_api-rpc/inc
LOCAL_COPY_HEADERS:= \
	$(RPC_INC)/loc_api_cb.h \
	$(RPC_INC)/loc_api_common.h \
	$(RPC_INC)/loc_api.h \
	$(RPC_INC)/loc_api_fixup.h \
	$(RPC_INC)/loc_apicb_appinit.h \
	inc/debug.h \
	inc/loc_api_rpc_glue.h

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH) \
	$(LOCAL_PATH)/inc \
	$(LOCAL_PATH)/$(RPC_INC) \
	$(TARGET_OUT_HEADERS)/libcommondefs-rpc \
	$(TARGET_OUT_HEADERS)/librpc

LOCAL_MODULE:= libloc_api-rpc
LOCAL_MODULE_OWNER := qcom

include $(BUILD_STATIC_LIBRARY)

endif
endif
