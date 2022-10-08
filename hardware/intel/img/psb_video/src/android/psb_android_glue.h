/*
 * Copyright (c) 2011 Intel Corporation. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Zhaohan Ren  <zhaohan.ren@intel.com>
 *
 */
#ifndef __PSB_ANDROID_GLUE_H__
#define __PSB_ANDROID_GLUE_H__

#ifdef __cplusplus
extern "C"
{
#endif
    unsigned char* psb_android_registerBuffers(void** surface, int pid, int width, int height);

    void psb_android_postBuffer(int offset);

    void psb_android_clearHeap();

    int psb_android_surfaceflinger_status(void** surface, int *sf_compostion, int *rotation, int *widi);

    void psb_android_get_destbox(short* destx, short* desty, unsigned short* destw, unsigned short* desth);
    void psb_android_dynamic_source_display(int buffer_index, int hdmi_mode);
    void psb_android_dynamic_source_destroy();
    int psb_android_surfaceflinger_rotate(void* window, int *rotation);

    enum {
        INIT_VALUE = 0,
        HDMI_VIDEO_ISPLAYING,
        WIDI_VIDEO_ISPLAYING,
    };

#ifdef TARGET_HAS_MULTIPLE_DISPLAY
    void init_mds_listener(void*);
    void deinit_mds_listener(void*);
    int  psb_android_get_mds_mode(void*);
    int  psb_android_get_mds_decoder_output_resolution(void*, int*, int*, int*, int*, int*, int*);
    int  psb_android_get_mds_vpp_state(void*);
#else
#ifdef PSBVIDEO_MRFL_VPP
    int  psb_android_get_vpp_state();
#endif
#endif

#ifdef __cplusplus
}
#endif


#endif
