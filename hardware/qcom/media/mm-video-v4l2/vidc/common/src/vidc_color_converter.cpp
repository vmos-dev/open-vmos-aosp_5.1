/*--------------------------------------------------------------------------
Copyright (c) 2012-2014, The Linux Foundation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of The Linux Foundation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
--------------------------------------------------------------------------*/
#define LOG_TAG "OMX_C2D"

#include <utils/Log.h>
#include <gralloc_priv.h>
#include "vidc_color_converter.h"
#include "vidc_debug.h"

omx_c2d_conv::omx_c2d_conv()
{
    c2dcc = NULL;
    mLibHandle = NULL;
    mConvertOpen = NULL;
    mConvertClose = NULL;
    src_format = NV12_2K;
}

bool omx_c2d_conv::init()
{
    bool status = true;

    if (mLibHandle || mConvertOpen || mConvertClose) {
        DEBUG_PRINT_ERROR("omx_c2d_conv::init called twice");
        status = false;
    }

    if (status) {
        mLibHandle = dlopen("libc2dcolorconvert.so", RTLD_LAZY);

        if (mLibHandle) {
            mConvertOpen = (createC2DColorConverter_t *)
                dlsym(mLibHandle,"createC2DColorConverter");
            mConvertClose = (destroyC2DColorConverter_t *)
                dlsym(mLibHandle,"destroyC2DColorConverter");

            if (!mConvertOpen || !mConvertClose)
                status = false;
        } else
            status = false;
    }

    if (!status && mLibHandle) {
        dlclose(mLibHandle);
        mLibHandle = NULL;
        mConvertOpen = NULL;
        mConvertClose = NULL;
    }

    return status;
}

bool omx_c2d_conv::convert(int src_fd, void *src_base, void *src_viraddr,
        int dest_fd, void *dest_base, void *dest_viraddr)
{
    int result;

    if (!src_viraddr || !dest_viraddr || !c2dcc || !dest_base || !src_base) {
        DEBUG_PRINT_ERROR("Invalid arguments omx_c2d_conv::convert");
        return false;
    }

    result =  c2dcc->convertC2D(src_fd, src_base, src_viraddr,
            dest_fd, dest_base, dest_viraddr);
    DEBUG_PRINT_LOW("Color convert status %d",result);
    return ((result < 0)?false:true);
}

bool omx_c2d_conv::open(unsigned int height,unsigned int width,
        ColorConvertFormat src, ColorConvertFormat dest)
{
    bool status = false;

    if (!c2dcc) {
        c2dcc = mConvertOpen(width, height, width, height,
                src,dest,0,0);

        if (c2dcc) {
            src_format = src;
            status = true;
        } else
            DEBUG_PRINT_ERROR("mConvertOpen failed");
    }

    return status;
}
void omx_c2d_conv::close()
{
    if (mLibHandle) {
        if (mConvertClose && c2dcc)
            mConvertClose(c2dcc);

        c2dcc = NULL;
    }
}

void omx_c2d_conv::destroy()
{
    DEBUG_PRINT_HIGH("Destroy C2D instance");

    if (mLibHandle) {
        if (mConvertClose && c2dcc)
            mConvertClose(c2dcc);

        dlclose(mLibHandle);
    }

    c2dcc = NULL;
    mLibHandle = NULL;
    mConvertOpen = NULL;
    mConvertClose = NULL;
}
omx_c2d_conv::~omx_c2d_conv()
{
    destroy();
}
int omx_c2d_conv::get_src_format()
{
    int format = -1;

    if (src_format == NV12_2K) {
        format = HAL_PIXEL_FORMAT_NV12_ENCODEABLE;
    } else if (src_format == RGBA8888) {
        format = HAL_PIXEL_FORMAT_RGBA_8888;
    }

    return format;
}
bool omx_c2d_conv::get_buffer_size(int port,unsigned int &buf_size)
{
    int cret = 0;
    bool ret = false;
    C2DBuffReq bufferreq;

    if (c2dcc) {
        bufferreq.size = 0;
        cret = c2dcc->getBuffReq(port,&bufferreq);
        DEBUG_PRINT_LOW("Status of getbuffer is %d", cret);
        ret = (cret)?false:true;
        buf_size = bufferreq.size;
    }

    return ret;
}

bool omx_c2d_conv::get_output_filled_length(unsigned int &filled_length)
{
    bool ret = false;
    C2DBuffReq req;
    filled_length = 0;

    if (c2dcc) {
        int cret = c2dcc->getBuffReq(C2D_OUTPUT, &req);
        DEBUG_PRINT_LOW("Status of getBuffReq is %d", cret);
        if (!cret && (req.bpp.denominator > 0)) {
            filled_length = (req.stride * req.sliceHeight * req.bpp.numerator);
            filled_length /= req.bpp.denominator;
            ret = true;
        }
    }

    return ret;
}
