# Copyright (c) 2011 Intel Corporation. All Rights Reserved.
#
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sub license, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice (including the
# next paragraph) shall be included in all copies or substantial portions
# of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
# IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
# ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS := \
    -DLINUX -DANDROID -g -Wall -Wno-unused \
    -DPSBVIDEO_LOG_ENABLE -DPSBVIDEO_VXD392 \
    -DPSBVIDEO_MSVDX_DEC_TILING -DPSBVIDEO_MSVDX_EC

LOCAL_C_INCLUDES := \
    $(call include-path-for, libhardware)/hardware \
    $(TARGET_OUT_HEADERS)/libva \
    $(TARGET_OUT_HEADERS)/libttm \
    $(TARGET_OUT_HEADERS)/libwsbm \
    $(TARGET_OUT_HEADERS)/libdrm \
    $(TARGET_OUT_HEADERS)/opengles \
    $(TARGET_OUT_HEADERS)/pvr \
    $(LOCAL_PATH)/hwdefs

LOCAL_SHARED_LIBRARIES += libdl libdrm libwsbm libcutils \
    libutils libbinder libhardware liblog

LOCAL_SRC_FILES := \
    object_heap.c \
    psb_buffer.c \
    psb_buffer_dm.c \
    psb_cmdbuf.c \
    psb_drv_video.c \
    psb_drv_debug.c \
    psb_surface_attrib.c \
    psb_output.c \
    android/psb_output_android.c \
    android/psb_android_glue.cpp \
    android/psb_surface_gralloc.c \
    android/psb_gralloc.cpp \
    psb_surface.c \
    psb_overlay.c \
    vc1_idx.c \
    vc1_vlc.c \
    pnw_H264.c \
    pnw_MPEG4.c \
    pnw_MPEG2.c \
    pnw_VC1.c \
    pnw_rotate.c \
    tng_vld_dec.c \
    tng_yuv_processor.c \
    tng_VP8.c \
    tng_jpegdec.c

ifneq ($(filter $(TARGET_BOARD_PLATFORM),merrifield moorefield morganfield),)
LOCAL_SRC_FILES += \
    pnw_H263ES.c \
    pnw_H264ES.c \
    pnw_MPEG4ES.c \
    pnw_cmdbuf.c \
    pnw_hostcode.c \
    pnw_hostheader.c \
    pnw_hostjpeg.c \
    pnw_jpeg.c \
    tng_ved_scaling.c \
    tng_cmdbuf.c \
    tng_hostheader.c \
    tng_hostcode.c \
    tng_picmgmt.c \
    tng_hostbias.c \
    tng_H264ES.c \
    tng_H263ES.c \
    tng_MPEG4ES.c \
    tng_jpegES.c \
    tng_slotorder.c \
    tng_hostair.c \
    tng_trace.c

ifeq ($(TARGET_HAS_ISV),true)
LOCAL_SRC_FILES += \
    vsp_VPP.c \
    vsp_cmdbuf.c \
    vsp_vp8.c \
    vsp_compose.c

LOCAL_CFLAGS += \
    -DPSBVIDEO_MRFL_VPP \
    -DPSBVIDEO_VPP_TILING
endif

ifeq ($(TARGET_HAS_VPP),true)
LOCAL_SRC_FILES += \
    vsp_VPP.c \
    vsp_vp8.c \
    vsp_cmdbuf.c \
    vsp_compose.c
endif

ifeq ($(TARGET_HAS_VPP),true)
LOCAL_C_INCLUDES += \
    $(TARGET_OUT_HEADERS)/libmedia_utils_vpp
endif


ifeq ($(TARGET_HAS_VPP),true)
LOCAL_SHARED_LIBRARIES += libvpp_setting
LOCAL_CFLAGS += DPSBVIDEO_MRFL_VPP_SETTING
endif

ifeq ($(TARGET_HAS_VPP),true)
LOCAL_CFLAGS += \
    -DPSBVIDEO_MRFL_VPP -DPSBVIDEO_MRFL \
    -DPSBVIDEO_VPP_TILING -DSLICE_HEADER_PARSING
else
LOCAL_CFLAGS += \
    -DPSBVIDEO_MRFL_VPP \
    -DPSBVIDEO_MRFL \
    -DSLICE_HEADER_PARSING
endif

ifeq ($(TARGET_BOARD_PLATFORM),merrifield)
ifeq ($(TARGET_HAS_VPP),true)
LOCAL_CFLAGS += -DPSBVIDEO_MRFL_VPP_ROTATE
endif
endif

else
LOCAL_CFLAGS += \
    -DPSBVIDEO_VXD392 -DBAYTRAIL \
    -DPSBVIDEO_MSVDX_DEC_TILING -DPSBVIDEO_MSVDX_EC
endif

ifeq ($(TARGET_HAS_MULTIPLE_DISPLAY),true)
LOCAL_SRC_FILES += android/psb_mds.cpp
LOCAL_CFLAGS += -DTARGET_HAS_MULTIPLE_DISPLAY
LOCAL_SHARED_LIBRARIES += libmultidisplay
endif
LOCAL_CFLAGS += -Werror

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := pvr_drv_video

include $(BUILD_SHARED_LIBRARY)
