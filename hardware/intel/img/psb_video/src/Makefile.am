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

pvr_drv_video_la_LTLIBRARIES = pvr_drv_video.la
pvr_drv_video_ladir = /usr/lib/dri
pvr_drv_video_la_LDFLAGS = -lwsbm -pthread -module -avoid-version -Wl,--no-undefined
pvr_drv_video_la_LIBADD = -ldrm -lX11 -lXrandr -lva-x11 -lXv -lm -lXext -lpvr2d
AM_CFLAGS = -DDEBUG -DLINUX -I$(top_srcdir)/src/hwdefs $(DRM_CFLAGS) 


pvr_drv_video_la_SOURCES = psb_drv_video.c object_heap.c psb_buffer.c psb_buffer_dm.c psb_cmdbuf.c psb_surface.c \
		vc1_vlc.c vc1_idx.c psb_ws_driver.c \
		pnw_hostheader.c pnw_hostcode.c pnw_rotate.c\
		pnw_cmdbuf.c pnw_H264ES.c pnw_H263ES.c pnw_MPEG4ES.c \
		pnw_H264.c pnw_MPEG2.c pnw_MPEG4.c pnw_hostjpeg.c pnw_jpeg.c pnw_VC1.c tng_VP8.c \
		tng_cmdbuf.c tng_hostheader.c tng_hostcode.c \
		tng_picmgmt.c tng_hostbias.c tng_slotorder.c tng_hostair.c \
		tng_H264ES.c tng_H263ES.c  tng_jpegES.c tng_trace.c tng_MPEG4ES.c \
		psb_output.c  psb_overlay.c psb_texture.c \
		x11/psb_x11.c x11/psb_coverlay.c x11/psb_xrandr.c x11/psb_xvva.c x11/psb_ctexture.c \
		psb_surface_attrib.c psb_drv_debug.c tng_jpegdec.c tng_vld_dec.c tng_yuv_processor.c
#		vc1_ap_i.c vc1_ap_p.c vc1_ap_utils.c vc1_bitplane.c \
#		vc1_shiftreg.c vc1_spmp.c vc1_utils.c


CFLAGS = -O1 -Wall -ffloat-store -fvisibility=hidden -DPSBVIDEO_MRST -DPSBVIDEO_MFLD -DPSBVIDEO_MRFL -D_FOR_FPGA_ -DPSBVIDEO_MRFL_DEC

symbol_info:	pvr_drv_video.la
	objdump -T .libs/pvr_drv_video.so | grep UND | sort -k 5 > Linker_dependencies.txt
	objdump -T .libs/pvr_drv_video.so | grep -v UND | sort -k 5 > Linker_exports.txt

#install-exec-am:
#	-rm -f $(pvr_drv_video_ladir)/psb_drv_video.so
#	-$(LN_S) $(pvr_drv_video_ladir)/pvr_drv_video.so $(pvr_drv_video_ladir)/psb_drv_video.so
