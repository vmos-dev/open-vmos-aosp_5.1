/* Copyright (c) 2012 - 2013, The Linux Foundation. All rights reserved.
 *
 * redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * this software is provided "as is" and any express or implied
 * warranties, including, but not limited to, the implied warranties of
 * merchantability, fitness for a particular purpose and non-infringement
 * are disclaimed.  in no event shall the copyright owner or contributors
 * be liable for any direct, indirect, incidental, special, exemplary, or
 * consequential damages (including, but not limited to, procurement of
 * substitute goods or services; loss of use, data, or profits; or
 * business interruption) however caused and on any theory of liability,
 * whether in contract, strict liability, or tort (including negligence
 * or otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 *
 */

#ifndef C2D_ColorConverter_H_
#define C2D_ColorConverter_H_

#include <c2d2.h>
#include <ColorConverter.h>
#include <sys/types.h>

typedef C2D_STATUS (*LINK_c2dCreateSurface)( uint32 *surface_id,
        uint32 surface_bits,
        C2D_SURFACE_TYPE surface_type,
        void *surface_definition );

typedef C2D_STATUS (*LINK_c2dUpdateSurface)( uint32 surface_id,
        uint32 surface_bits,
        C2D_SURFACE_TYPE surface_type,
        void *surface_definition );

typedef C2D_STATUS (*LINK_c2dReadSurface)( uint32 surface_id,
        C2D_SURFACE_TYPE surface_type,
        void *surface_definition,
        int32 x, int32 y );

typedef C2D_STATUS (*LINK_c2dDraw)( uint32 target_id,
        uint32 target_config, C2D_RECT *target_scissor,
        uint32 target_mask_id, uint32 target_color_key,
        C2D_OBJECT *objects_list, uint32 num_objects );

typedef C2D_STATUS (*LINK_c2dFlush)( uint32 target_id, c2d_ts_handle *timestamp);

typedef C2D_STATUS (*LINK_c2dFinish)( uint32 target_id);

typedef C2D_STATUS (*LINK_c2dWaitTimestamp)( c2d_ts_handle timestamp );

typedef C2D_STATUS (*LINK_c2dDestroySurface)( uint32 surface_id );

typedef C2D_STATUS (*LINK_c2dMapAddr)( int mem_fd, void * hostptr, uint32 len, uint32 offset, uint32 flags, void ** gpuaddr);

typedef C2D_STATUS (*LINK_c2dUnMapAddr)(void * gpuaddr);

namespace android {

/*TODO: THIS NEEDS TO ENABLED FOR JB PLUS*/
enum ColorConvertFormat {
    RGB565 = 1,
    YCbCr420Tile,
    YCbCr420SP,
    YCbCr420P,
    YCrCb420P,
    RGBA8888,
    NV12_2K,
    NV12_128m,
};

typedef struct {
    int32_t numerator;
    int32_t denominator;
} C2DBytesPerPixel;

typedef struct {
  int32_t width;
  int32_t height;
  int32_t stride;
  int32_t sliceHeight;
  int32_t lumaAlign;
  int32_t sizeAlign;
  int32_t size;
  C2DBytesPerPixel bpp;
} C2DBuffReq;

typedef enum {
  C2D_INPUT = 0,
  C2D_OUTPUT,
} C2D_PORT;

class C2DColorConverterBase {

public:
    virtual ~C2DColorConverterBase(){};
    virtual int convertC2D(int srcFd, void *srcBase, void * srcData, int dstFd, void *dstBase, void * dstData) = 0;
    virtual int32_t getBuffReq(int32_t port, C2DBuffReq *req) = 0;
    virtual int32_t dumpOutput(char * filename, char mode) = 0;
};

typedef C2DColorConverterBase* createC2DColorConverter_t(size_t srcWidth, size_t srcHeight, size_t dstWidth, size_t dstHeight, ColorConvertFormat srcFormat, ColorConvertFormat dstFormat, int32_t flags, size_t srcStride);
typedef void destroyC2DColorConverter_t(C2DColorConverterBase*);

}

#endif  // C2D_ColorConverter_H_
