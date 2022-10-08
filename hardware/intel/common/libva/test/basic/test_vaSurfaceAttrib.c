/*
 * Copyright (c) 2007 Intel Corporation. All Rights Reserved.
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
 */

#define TEST_DESCRIPTION	"vaCreateSurfaces with different VASurfaceAttrib"

#include "test_common.c"
#include <va/va_drmcommon.h>

void pre()
{
    test_init();
}

void test()
{
    VAEntrypoint entrypoints[5];
    int num_entrypoints,slice_entrypoint;
    VAConfigAttrib attrib[2];
    VAConfigID config_id;
    unsigned int fourcc, luma_stride, chroma_u_stride, chroma_v_stride, luma_offset, chroma_u_offset;
    unsigned int chroma_v_offset, buffer_name;
    unsigned int *p_buffer;
    int i, ret;
    char c;
    int frame_width=640,  frame_height=480;
    VASurfaceID surface_id;
    VAImage image_id;
    unsigned char *usrbuf;

    usrbuf = (unsigned char*)malloc(frame_width * frame_height * 2);
    ASSERT ( usrbuf != NULL);

    VASurfaceAttribExternalBuffers  vaSurfaceExternBuf;
    VASurfaceAttrib attrib_list[2];

    va_status = vaQueryConfigEntrypoints(va_dpy, VAProfileH264Baseline, entrypoints, &num_entrypoints);
    ASSERT( VA_STATUS_SUCCESS == va_status );

    for	(slice_entrypoint = 0; slice_entrypoint < num_entrypoints; slice_entrypoint++) {
        if (entrypoints[slice_entrypoint] == VAEntrypointEncSlice)
            break;
    }
    if (slice_entrypoint == num_entrypoints) {
        /* not find Slice entry point */
        status("VAEntrypointEncSlice doesn't support, exit.\n");
        ASSERT(0);
    }

    /* find out the format for the render target, and rate control mode */
    attrib[0].type = VAConfigAttribRTFormat;
    attrib[1].type = VAConfigAttribRateControl;
    va_status = vaGetConfigAttributes(va_dpy, VAProfileH264Baseline, VAEntrypointEncSlice, &attrib[0], 2);
    ASSERT( VA_STATUS_SUCCESS == va_status );

    if ((attrib[0].value & VA_RT_FORMAT_YUV420) == 0) {
        /* not find desired YUV420 RT format */
        status("VA_RT_FORMAT_YUV420 doesn't support, exit\n");
        ASSERT(0);
    }
    if ((attrib[1].value & VA_RC_VBR) == 0) {
        /* Can't find matched RC mode */
        status("VBR mode doesn't found, exit\n");
        ASSERT(0);
    }

    attrib[0].value = VA_RT_FORMAT_YUV420; /* set to desired RT format */
    attrib[1].value = VA_RC_VBR; /* set to desired RC mode */

    va_status = vaCreateConfig(va_dpy, VAProfileH264Baseline, VAEntrypointEncSlice, &attrib[0], 2, &config_id);
    ASSERT( VA_STATUS_SUCCESS == va_status );

    attrib_list[1].type = (VASurfaceAttribType)VASurfaceAttribExternalBufferDescriptor;
    attrib_list[0].type = (VASurfaceAttribType)VASurfaceAttribMemoryType;
    va_status = vaGetSurfaceAttributes(va_dpy, config_id, attrib_list, 2);
    ASSERT( VA_STATUS_SUCCESS == va_status );

    if (attrib_list[0].flags != VA_SURFACE_ATTRIB_NOT_SUPPORTED) {
        status("supported memory type:\n");
        if (attrib_list[0].value.value.i & VA_SURFACE_ATTRIB_MEM_TYPE_VA)
            status("\tVA_SURFACE_ATTRIB_MEM_TYPE_VA\n");
        if (attrib_list[0].value.value.i & VA_SURFACE_ATTRIB_MEM_TYPE_V4L2)
            status("\tVA_SURFACE_ATTRIB_MEM_TYPE_V4L2\n");
        if (attrib_list[0].value.value.i & VA_SURFACE_ATTRIB_MEM_TYPE_USER_PTR)
            status("\tVA_SURFACE_ATTRIB_MEM_TYPE_USER_PTR\n");
        if (attrib_list[0].value.value.i & VA_SURFACE_ATTRIB_MEM_TYPE_ANDROID_GRALLOC)
           status("\tVA_SURFACE_ATTRIB_MEM_TYPE_ANDROID_GRALLOC\n");
        if (attrib_list[0].value.value.i &  VA_SURFACE_ATTRIB_MEM_TYPE_ANDROID_ION)
            status("\tVA_SURFACE_ATTRIB_MEM_TYPE_ANDROID_ION\n");
        if (attrib_list[0].value.value.i & VA_SURFACE_ATTRIB_MEM_TYPE_KERNEL_DRM)
            status("\tVA_SURFACE_ATTRIB_MEM_TYPE_KERNEL_DRM\n");
    }

    if ((attrib_list[1].flags != VA_SURFACE_ATTRIB_NOT_SUPPORTED) &&
            (attrib_list[0].value.value.i & VA_SURFACE_ATTRIB_MEM_TYPE_USER_PTR)) {
        status("vaCreateSurfaces from external usr pointer\n");

        vaSurfaceExternBuf.buffers = (unsigned long*)malloc(sizeof(unsigned int));
        vaSurfaceExternBuf.buffers[0] = usrbuf;
        vaSurfaceExternBuf.num_buffers = 1;
        vaSurfaceExternBuf.width = frame_width;
        vaSurfaceExternBuf.height = frame_height;
        vaSurfaceExternBuf.pitches[0] = vaSurfaceExternBuf.pitches[1] = vaSurfaceExternBuf.pitches[2] = frame_width;
        //vaSurfaceExternBuf.flags = VA_SURFACE_ATTRIB_MEM_TYPE_ANDROID_GRALLOC;
        vaSurfaceExternBuf.pixel_format = VA_FOURCC_NV12;
        //vaSurfaceExternBuf.pitches[0] = attribute_tpi->luma_stride;

        attrib_list[1].flags = VA_SURFACE_ATTRIB_SETTABLE;
        attrib_list[1].value.type = VAGenericValueTypePointer;
        attrib_list[1].value.value.p = (void *)&vaSurfaceExternBuf;

        attrib_list[0].flags = VA_SURFACE_ATTRIB_SETTABLE;
        attrib_list[0].value.type = VAGenericValueTypeInteger;
        attrib_list[0].value.value.i = VA_SURFACE_ATTRIB_MEM_TYPE_USER_PTR;

        va_status = vaCreateSurfaces(va_dpy, VA_RT_FORMAT_YUV420, frame_width, frame_height, &surface_id, 1, attrib_list, 2);
        ASSERT( VA_STATUS_SUCCESS == va_status );

        va_status = vaDeriveImage(va_dpy, surface_id, &image_id);
        ASSERT( VA_STATUS_SUCCESS == va_status );

        va_status = vaMapBuffer(va_dpy, image_id.buf, (void**)&p_buffer);
        ASSERT( VA_STATUS_SUCCESS == va_status );

        memset(p_buffer, 0x80, image_id.width * image_id.height);

        va_status = vaUnmapBuffer(va_dpy, image_id.buf);
        ASSERT( VA_STATUS_SUCCESS == va_status );

        va_status = vaDestroyImage(va_dpy, image_id.image_id);
        ASSERT( VA_STATUS_SUCCESS == va_status );

        va_status = vaDestroySurfaces(va_dpy, &surface_id, 1);
        ASSERT( VA_STATUS_SUCCESS == va_status );

    }

    va_status = vaDestroyConfig(va_dpy, config_id);
    ASSERT( VA_STATUS_SUCCESS == va_status );

    free(usrbuf);

}

void post()
{
    test_terminate();
}
