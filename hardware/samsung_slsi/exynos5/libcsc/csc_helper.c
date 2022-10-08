/*
 *
 * Copyright 2012 Samsung Electronics S.LSI Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "libcsc_helper"
#include <cutils/log.h>

#include <system/graphics.h>

#include "Exynos_OMX_Def.h"

#include "csc.h"
#include "exynos_format.h"
#include "fimg2d.h"

OMX_COLOR_FORMATTYPE hal_2_omx_pixel_format(
    unsigned int hal_format)
{
    OMX_COLOR_FORMATTYPE omx_format;
    switch (hal_format) {
    case HAL_PIXEL_FORMAT_YCbCr_422_I:
        omx_format = OMX_COLOR_FormatYCbYCr;
        break;
    case HAL_PIXEL_FORMAT_YCbCr_420_P:
        omx_format = OMX_COLOR_FormatYUV420Planar;
        break;
    case HAL_PIXEL_FORMAT_YCbCr_420_SP:
        omx_format = OMX_COLOR_FormatYUV420SemiPlanar;
        break;
    case HAL_PIXEL_FORMAT_CUSTOM_YCbCr_420_SP_TILED:
        omx_format = OMX_SEC_COLOR_FormatNV12TPhysicalAddress;
        break;
    case HAL_PIXEL_FORMAT_YCbCr_420_SP_TILED:
        omx_format = OMX_SEC_COLOR_FormatNV12Tiled;
        break;
    case HAL_PIXEL_FORMAT_BGRA_8888:
    case HAL_PIXEL_FORMAT_CUSTOM_ARGB_8888:
        omx_format = OMX_COLOR_Format32bitARGB8888;
        break;
    default:
        omx_format = OMX_COLOR_FormatYUV420Planar;
        break;
    }
    return omx_format;
}

unsigned int omx_2_hal_pixel_format(
    OMX_COLOR_FORMATTYPE omx_format)
{
    unsigned int hal_format;
    switch (omx_format) {
    case OMX_COLOR_FormatYCbYCr:
        hal_format = HAL_PIXEL_FORMAT_YCbCr_422_I;
        break;
    case OMX_COLOR_FormatYUV420Planar:
        hal_format = HAL_PIXEL_FORMAT_YCbCr_420_P;
        break;
    case OMX_COLOR_FormatYUV420SemiPlanar:
        hal_format = HAL_PIXEL_FORMAT_YCbCr_420_SP;
        break;
    case OMX_SEC_COLOR_FormatNV12TPhysicalAddress:
        hal_format = HAL_PIXEL_FORMAT_CUSTOM_YCbCr_420_SP_TILED;
        break;
    case OMX_SEC_COLOR_FormatNV12Tiled:
        hal_format = HAL_PIXEL_FORMAT_YCbCr_420_SP_TILED;
        break;
    case OMX_COLOR_Format32bitARGB8888:
        hal_format = HAL_PIXEL_FORMAT_CUSTOM_ARGB_8888;
        break;
    default:
        hal_format = HAL_PIXEL_FORMAT_YCbCr_420_P;
        break;
    }
    return hal_format;
}

unsigned int hal_2_g2d_color_format(unsigned int hal_format)
{
    switch (hal_format) {
    case HAL_PIXEL_FORMAT_RGBA_8888:
    case HAL_PIXEL_FORMAT_BGRA_8888:
    case HAL_PIXEL_FORMAT_CUSTOM_ARGB_8888:
        return CF_ARGB_8888;

    case HAL_PIXEL_FORMAT_RGBX_8888:
        return CF_XRGB_8888;

    case HAL_PIXEL_FORMAT_RGB_888:
        return CF_RGB_888;

    case HAL_PIXEL_FORMAT_RGB_565:
        return CF_RGB_565;

    case HAL_PIXEL_FORMAT_YCbCr_422_I:
        return CF_YCBCR_422;

    case HAL_PIXEL_FORMAT_YCbCr_420_SP:
        return CF_YCBCR_420;

    default:
        return SRC_DST_FORMAT_END;
    }
}

unsigned int hal_2_g2d_pixel_order(unsigned int hal_format)
{
    switch (hal_format) {
    case HAL_PIXEL_FORMAT_BGRA_8888:
    case HAL_PIXEL_FORMAT_CUSTOM_ARGB_8888:
        return AX_RGB;

    case HAL_PIXEL_FORMAT_RGBA_8888:
    case HAL_PIXEL_FORMAT_RGBX_8888:
    case HAL_PIXEL_FORMAT_RGB_888:
    case HAL_PIXEL_FORMAT_RGB_565:
        return RGB_AX;

    case HAL_PIXEL_FORMAT_YCbCr_422_I:
        return P1_Y1CBY0CR;

    case HAL_PIXEL_FORMAT_YCbCr_420_SP:
        return P2_CRCB;

    default:
        return ARGB_ORDER_END;
    }
}

size_t hal_2_g2d_bpp(unsigned int hal_format)
{
    switch (hal_format) {
        case HAL_PIXEL_FORMAT_BGRA_8888:
        case HAL_PIXEL_FORMAT_CUSTOM_ARGB_8888:
        case HAL_PIXEL_FORMAT_RGBA_8888:
        case HAL_PIXEL_FORMAT_RGBX_8888:
            return 32;

        case HAL_PIXEL_FORMAT_RGB_888:
            return 24;

        case HAL_PIXEL_FORMAT_RGB_565:
            return 16;

        case HAL_PIXEL_FORMAT_YCbCr_422_I:
        case HAL_PIXEL_FORMAT_YCbCr_420_SP:
            return 8;

        default:
            return 0;
        }
}
