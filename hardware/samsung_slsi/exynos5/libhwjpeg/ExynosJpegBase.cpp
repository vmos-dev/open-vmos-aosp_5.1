/*
 * Copyright Samsung Electronics Co.,LTD.
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <math.h>
#include <sys/poll.h>

#include <cutils/log.h>

#include <utils/Log.h>

#include "ExynosJpegApi.h"

#define JPEG_DEC_NODE        "/dev/video11"
#define JPEG_ENC_NODE        "/dev/video12"

#define MAX_JPG_WIDTH (8192)
#define MAX_JPG_HEIGHT (8192)

#define JPEG_ERROR_LOG(fmt,...)

ExynosJpegBase::ExynosJpegBase()
{
}

ExynosJpegBase::~ExynosJpegBase()
{
}

int ExynosJpegBase::t_v4l2Querycap(int iFd)
{
    struct v4l2_capability cap;
    int iRet = ERROR_NONE;

    iRet = ioctl(iFd, VIDIOC_QUERYCAP, &cap);
    if (iRet < 0) {
        JPEG_ERROR_LOG("[%s:%d]: VIDIOC_QUERYCAP failed\n", __func__, iRet);
        return iRet;
    }

    return iRet;
}

int ExynosJpegBase::t_v4l2SetJpegcomp(int iFd, int iQuality)
{
    struct v4l2_jpegcompression arg;
    int iRet = ERROR_NONE;

    arg.quality = iQuality;

    iRet = ioctl(iFd, VIDIOC_S_JPEGCOMP, &arg);
    if (iRet < 0) {
        JPEG_ERROR_LOG("[%s:%d]: VIDIOC_S_JPEGCOMP failed\n", __func__, iRet);
        return iRet;
    }

    return iRet;
}

int ExynosJpegBase::t_v4l2SetFmt(int iFd, enum v4l2_buf_type eType, struct CONFIG *pstConfig)
{
    struct v4l2_format fmt;
    int iRet = ERROR_NONE;

    fmt.type = eType;
    fmt.fmt.pix_mp.width = pstConfig->width;
    fmt.fmt.pix_mp.height = pstConfig->height;
    fmt.fmt.pix_mp.field = V4L2_FIELD_ANY;
    fmt.fmt.pix_mp.num_planes = pstConfig->numOfPlanes;

    if (pstConfig->mode == MODE_ENCODE)
        fmt.fmt.pix_mp.colorspace = V4L2_COLORSPACE_JPEG;

    switch (fmt.type) {
    case V4L2_BUF_TYPE_VIDEO_OUTPUT:    // fall through
    case V4L2_BUF_TYPE_VIDEO_CAPTURE:
        break;
    case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
        if (pstConfig->mode == MODE_ENCODE) {
            fmt.fmt.pix_mp.pixelformat = pstConfig->pix.enc_fmt.in_fmt;
        } else {
            fmt.fmt.pix_mp.pixelformat = pstConfig->pix.dec_fmt.in_fmt;
            fmt.fmt.pix_mp.plane_fmt[0].sizeimage = pstConfig->sizeJpeg;
        }
        break;
    case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
        if (pstConfig->mode == MODE_ENCODE) {
            fmt.fmt.pix_mp.pixelformat = pstConfig->pix.enc_fmt.out_fmt;
        } else {
            fmt.fmt.pix_mp.pixelformat = pstConfig->pix.dec_fmt.out_fmt;
            fmt.fmt.pix_mp.width = pstConfig->scaled_width;
            fmt.fmt.pix_mp.height = pstConfig->scaled_height;
        }
        break;
    default:
            return ERROR_INVALID_V4l2_BUF_TYPE;
            break;
    }

    iRet = ioctl(iFd, VIDIOC_S_FMT, &fmt);
    if (iRet < 0) {
        JPEG_ERROR_LOG("[%s:%d]: VIDIOC_S_FMT failed\n", __func__, iRet);
        return iRet;
    }

    return iRet;
}

int ExynosJpegBase::t_v4l2GetFmt(int iFd, enum v4l2_buf_type eType, struct CONFIG *pstConfig)
{
    struct v4l2_format fmt;
    int iRet = ERROR_NONE;

    fmt.type = eType;
    iRet = ioctl(iFd, VIDIOC_G_FMT, &fmt);
    if (iRet < 0) {
        JPEG_ERROR_LOG("[%s:%d]: VIDIOC_G_FMT failed\n", __func__, iRet);
        return iRet;
    }

    switch (fmt.type) {
    case V4L2_BUF_TYPE_VIDEO_OUTPUT:    // fall through
    case V4L2_BUF_TYPE_VIDEO_CAPTURE:
        pstConfig->width = fmt.fmt.pix.width;
        pstConfig->height = fmt.fmt.pix.height;
        break;
    case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
        pstConfig->width = fmt.fmt.pix_mp.width;
        pstConfig->height = fmt.fmt.pix_mp.height;
        if (pstConfig->mode == MODE_ENCODE)
            pstConfig->pix.enc_fmt.in_fmt = fmt.fmt.pix_mp.pixelformat;
        else
            pstConfig->pix.dec_fmt.in_fmt = fmt.fmt.pix_mp.pixelformat;
        break;
    case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
        pstConfig->width = fmt.fmt.pix_mp.width;
        pstConfig->height = fmt.fmt.pix_mp.height;
        if (pstConfig->mode == MODE_ENCODE)
            pstConfig->pix.enc_fmt.out_fmt = fmt.fmt.pix_mp.pixelformat;
        else
            pstConfig->pix.dec_fmt.out_fmt = fmt.fmt.pix_mp.pixelformat;
        break;
    default:
        return ERROR_INVALID_V4l2_BUF_TYPE;
    }

    return iRet;
}

int ExynosJpegBase::t_v4l2Reqbufs(int iFd, int iBufCount, struct BUF_INFO *pstBufInfo)
{
    struct v4l2_requestbuffers req;
    int iRet = ERROR_NONE;

    memset(&req, 0, sizeof(v4l2_requestbuffers));

    req.type = pstBufInfo->buf_type;
    req.memory = pstBufInfo->memory;

    //if (pstBufInfo->memory == V4L2_MEMORY_MMAP)
        req.count = iBufCount;

    iRet = ioctl(iFd, VIDIOC_REQBUFS, &req);
    if (iRet < 0) {
        JPEG_ERROR_LOG("[%s:%d]: VIDIOC_REQBUFS failed\n", __func__, iRet);
        return iRet;
    }

    return iRet;
}

int ExynosJpegBase::t_v4l2Qbuf(int iFd, struct BUF_INFO *pstBufInfo, struct BUFFER *pstBuf)
{
    struct v4l2_buffer v4l2_buf;
    struct v4l2_plane plane[JPEG_MAX_PLANE_CNT];
    int i;
    int iRet = ERROR_NONE;

    memset(&v4l2_buf, 0, sizeof(struct v4l2_buffer));
    memset(plane, 0, (int)JPEG_MAX_PLANE_CNT * sizeof(struct v4l2_plane));

    v4l2_buf.index = 0;
    v4l2_buf.type = pstBufInfo->buf_type;
    v4l2_buf.memory = pstBufInfo->memory;
    v4l2_buf.field = V4L2_FIELD_ANY;
    v4l2_buf.length = pstBufInfo->numOfPlanes;
    v4l2_buf.m.planes = plane;

    if (pstBufInfo->memory == V4L2_MEMORY_DMABUF) {
        for (i = 0; i < pstBufInfo->numOfPlanes; i++) {
            v4l2_buf.m.planes[i].m.fd = (unsigned long)pstBuf->addr[i];
            v4l2_buf.m.planes[i].length = pstBuf->size[i];
        }
    }

    iRet = ioctl(iFd, VIDIOC_QBUF, &v4l2_buf);
    if (iRet < 0) {
        JPEG_ERROR_LOG("[%s:%d] VIDIOC_QBUF failed\n", __func__, iRet);
        pstBuf->numOfPlanes = 0;
        return iRet;
    }

    return iRet;
}

int ExynosJpegBase::t_v4l2Dqbuf(int iFd, enum v4l2_buf_type eType, enum v4l2_memory eMemory, int iNumPlanes)
{
    struct v4l2_buffer buf;
    struct v4l2_plane planes[3];
    int iRet = ERROR_NONE;

    memset(&buf, 0, sizeof(struct v4l2_buffer));
    memset(planes, 0, sizeof(struct v4l2_plane)*3);

    buf.type = eType;
    buf.memory = eMemory;
    buf.length = iNumPlanes;
    buf.m.planes = planes;

    iRet = ioctl(iFd, VIDIOC_DQBUF, &buf);
    if (iRet < 0) {
        JPEG_ERROR_LOG("[%s:%d] VIDIOC_DQBUF failed\n", __func__, iRet);
        return iRet;
    }

#ifdef KERNEL_33_JPEG_API
    if ((eType == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) && \
        (t_stJpegConfig.mode == MODE_ENCODE)) {
        t_stJpegConfig.sizeJpeg = buf.m.planes[0].bytesused;
    }
#endif

    return iRet;
}

int ExynosJpegBase::t_v4l2StreamOn(int iFd, enum v4l2_buf_type eType)
{
    int iRet = ERROR_NONE;

    iRet = ioctl(iFd, VIDIOC_STREAMON, &eType);
    if (iRet < 0) {
        JPEG_ERROR_LOG("[%s:%d] VIDIOC_STREAMON failed\n", __func__, iRet);
        return iRet;
    }

    return iRet;
}

int ExynosJpegBase::t_v4l2StreamOff(int iFd, enum v4l2_buf_type eType)
{
    int iRet = ERROR_NONE;

    iRet = ioctl(iFd, VIDIOC_STREAMOFF, &eType);
    if (iRet < 0) {
        JPEG_ERROR_LOG("[%s:%d] VIDIOC_STREAMOFF failed\n", __func__, iRet);
        return iRet;
    }

    return iRet;
}

int ExynosJpegBase::t_v4l2SetCtrl(int iFd, int iCid, int iValue)
{
    struct v4l2_control vc;
    int iRet = ERROR_NONE;

    vc.id = iCid;
    vc.value = iValue;

    iRet = ioctl(iFd, VIDIOC_S_CTRL, &vc);
    if (iRet < 0) {
        JPEG_ERROR_LOG("[%s] VIDIOC_S_CTRL failed : cid(%d), value(%d)\n", __func__, iCid, iValue);
        return iRet;
    }

    return iRet;
}

int ExynosJpegBase::t_v4l2GetCtrl(int iFd, int iCid)
{
    struct v4l2_control ctrl;
    int iRet = ERROR_NONE;

    ctrl.id = iCid;

    iRet = ioctl(iFd, VIDIOC_G_CTRL, &ctrl);
    if (iRet < 0) {
        JPEG_ERROR_LOG("[%s] VIDIOC_G_CTRL failed : cid(%d)\n", __func__, ctrl.id);
        return iRet;
    }

    return ctrl.value;
}

int ExynosJpegBase::create(enum MODE eMode)
{
    if (t_bFlagCreate == true) {
        return ERROR_JPEG_DEVICE_ALREADY_CREATE;
    }

    int iRet = ERROR_NONE;

    switch (eMode) {
    case MODE_ENCODE:
        t_iJpegFd = open(JPEG_ENC_NODE, O_RDWR, 0);
        break;
    case MODE_DECODE:
        t_iJpegFd = open(JPEG_DEC_NODE, O_RDWR, 0);
        break;
    default:
        t_iJpegFd = -1;
        return ERROR_INVALID_JPEG_MODE;
        break;
    }

    if (t_iJpegFd < 0) {
        t_iJpegFd = -1;
        JPEG_ERROR_LOG("[%s]: JPEG_NODE open failed\n", __func__);
        return ERROR_CANNOT_OPEN_JPEG_DEVICE;
    }

    if (t_iJpegFd <= 0) {
        t_iJpegFd = -1;
        JPEG_ERROR_LOG("ERR(%s):JPEG device was closed\n", __func__);
        return ERROR_JPEG_DEVICE_ALREADY_CLOSED;
    }

    iRet = t_v4l2Querycap(t_iJpegFd);
    if (iRet < 0) {
        JPEG_ERROR_LOG("[%s]: QUERYCAP failed\n", __func__);
        close(t_iJpegFd);
        return ERROR_CANNOT_OPEN_JPEG_DEVICE;
    }

    memset(&t_stJpegConfig, 0, sizeof(struct CONFIG));
    memset(&t_stJpegInbuf, 0, sizeof(struct BUFFER));
    memset(&t_stJpegOutbuf, 0, sizeof(struct BUFFER));

    t_stJpegConfig.mode = eMode;

    t_bFlagCreate = true;
    t_bFlagCreateInBuf = false;
    t_bFlagCreateOutBuf = false;
    t_bFlagExcute = false;

    t_iPlaneNum = 0;

    return ERROR_NONE;
}

int ExynosJpegBase::destroy(int iInBufs, int iOutBufs)
{
    if (t_bFlagCreate == false) {
        return ERROR_JPEG_DEVICE_ALREADY_DESTROY;
    }

    if (t_iJpegFd > 0) {
        struct BUF_INFO stBufInfo;

        if (t_bFlagExcute) {
            t_v4l2StreamOff(t_iJpegFd, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);
            t_v4l2StreamOff(t_iJpegFd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
        }

        if (t_bFlagExcute) {
            stBufInfo.numOfPlanes = iInBufs;
            stBufInfo.memory = V4L2_MEMORY_MMAP;

            stBufInfo.buf_type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
            t_v4l2Reqbufs(t_iJpegFd, 0, &stBufInfo);

            stBufInfo.numOfPlanes = iOutBufs;
            stBufInfo.buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
            t_v4l2Reqbufs(t_iJpegFd, 0, &stBufInfo);
        }

        close(t_iJpegFd);
    }

    t_iJpegFd = -1;
    t_bFlagCreate = false;
    return ERROR_NONE;
}

int ExynosJpegBase::setSize(int iW, int iH)
{
    if (t_bFlagCreate == false) {
        return ERROR_JPEG_DEVICE_NOT_CREATE_YET;
    }

    if (iW < 0 || MAX_JPG_WIDTH < iW) {
        return ERROR_INVALID_IMAGE_SIZE;
    }

    if (iH < 0 || MAX_JPG_HEIGHT < iH) {
        return ERROR_INVALID_IMAGE_SIZE;
    }

    t_stJpegConfig.width = iW;
    t_stJpegConfig.height = iH;

    return ERROR_NONE;
}

int ExynosJpegBase::setJpegConfig(enum MODE eMode, void *pConfig)
{
    if (t_bFlagCreate == false) {
        return ERROR_JPEG_DEVICE_NOT_CREATE_YET;
    }

    if (pConfig == NULL) {
        return ERROR_JPEG_CONFIG_POINTER_NULL;
    }

    memcpy(&t_stJpegConfig, pConfig, sizeof(struct CONFIG));

    switch (eMode) {
    case MODE_ENCODE:
        switch (t_stJpegConfig.pix.enc_fmt.in_fmt) {
        case V4L2_PIX_FMT_YUV420:
        case V4L2_PIX_FMT_NV16:
        case V4L2_PIX_FMT_YUYV:
        case V4L2_PIX_FMT_RGB565X:
        case V4L2_PIX_FMT_BGR32:
        case V4L2_PIX_FMT_RGB32:
            t_iPlaneNum = 1;
            break;
        default:
            JPEG_ERROR_LOG("%s::Invalid input color format(%d) fail\n", __func__, t_stJpegConfig.pix.enc_fmt.in_fmt);
            t_iPlaneNum = 0;
            return ERROR_INVALID_COLOR_FORMAT;
        }
        break;
    case MODE_DECODE:
        switch (t_stJpegConfig.pix.dec_fmt.out_fmt) {
        case V4L2_PIX_FMT_YUV420:
        case V4L2_PIX_FMT_NV16:
        case V4L2_PIX_FMT_YUYV:
        case V4L2_PIX_FMT_RGB565X:
        case V4L2_PIX_FMT_BGR32:
        case V4L2_PIX_FMT_RGB32:
            t_iPlaneNum = 1;
            break;
        default:
            JPEG_ERROR_LOG("%s::Invalid input color format(%d) fail\n", __func__, t_stJpegConfig.pix.dec_fmt.out_fmt);
            t_iPlaneNum = 0;
            return ERROR_INVALID_COLOR_FORMAT;
        }
        break;
    default:
        t_iPlaneNum = 0;
        return ERROR_INVALID_JPEG_MODE;
        break;
    }

    return ERROR_NONE;
}

void *ExynosJpegBase::getJpegConfig(void)
{
    if (t_bFlagCreate == false) {
        return NULL;
    }

    return &t_stJpegConfig;
}

int ExynosJpegBase::getBuf(bool bCreateBuf, struct BUFFER *pstBuf, int *piBuf, int *iBufSize, int iSize, int iPlaneNum)
{
    if (t_bFlagCreate == false) {
        return ERROR_JPEG_DEVICE_NOT_CREATE_YET;
    }

     if (bCreateBuf == false) {
        return ERROR_BUF_NOT_SET_YET;
    }

     if ((piBuf == NULL) || (iSize == 0)) {
        return ERROR_BUFFR_IS_NULL;
     }

     if (iSize < iPlaneNum) {
        return ERROR_BUFFER_TOO_SMALL;
     }

    for (int i=0;i<iPlaneNum;i++) {
        piBuf[i] = pstBuf->addr[i];
    }

    for (int i=0;i<iPlaneNum;i++) {
        iBufSize[i] = pstBuf->size[i];
    }

    return ERROR_NONE;
}

int ExynosJpegBase::setBuf(struct BUFFER *pstBuf, int *piBuf, int *iSize, int iPlaneNum)
{
    if (t_bFlagCreate == false) {
        return ERROR_JPEG_DEVICE_NOT_CREATE_YET;
    }

    if (iPlaneNum <= 0) {
        return ERROR_BUFFER_TOO_SMALL;
    }

    for(int i=0;i<iPlaneNum;i++) {
        if (piBuf[i] == NULL) {
            memset(pstBuf, 0, sizeof(struct BUFFER));
            return ERROR_BUFFR_IS_NULL;
        }
        if (iSize[i] <= 0) {
            memset(pstBuf, 0, sizeof(struct BUFFER));
            return ERROR_BUFFER_TOO_SMALL;
        }
        pstBuf->addr[i] = piBuf[i];
        pstBuf->size[i] = iSize[i];
    }

    pstBuf->numOfPlanes = iPlaneNum;

    return ERROR_NONE;
}

int ExynosJpegBase::setCache(int iValue)
{
    if (t_bFlagCreate == false) {
        return ERROR_JPEG_DEVICE_NOT_CREATE_YET;
    }

    if (t_v4l2SetCtrl(t_iJpegFd, V4L2_CID_CACHEABLE, iValue)<0) {
        JPEG_ERROR_LOG("%s::cache setting failed\n", __func__);
        return ERROR_CANNOT_CHANGE_CACHE_SETTING;
    }

    return ERROR_NONE;
}

int ExynosJpegBase::setColorFormat(enum MODE eMode, int iV4l2ColorFormat)
{
    if (t_bFlagCreate == false) {
        return ERROR_JPEG_DEVICE_NOT_CREATE_YET;
    }

    switch(iV4l2ColorFormat) {
    case V4L2_PIX_FMT_YUYV:
    case V4L2_PIX_FMT_YUV420:
    case V4L2_PIX_FMT_NV16:
    case V4L2_PIX_FMT_RGB565X:
    case V4L2_PIX_FMT_BGR32:
    case V4L2_PIX_FMT_RGB32:
        switch (eMode) {
        case MODE_ENCODE:
            t_stJpegConfig.pix.enc_fmt.in_fmt = iV4l2ColorFormat;
            break;
        case MODE_DECODE:
            t_stJpegConfig.pix.dec_fmt.out_fmt = iV4l2ColorFormat;
            break;
        default:
            return ERROR_INVALID_JPEG_MODE;
            break;
        }
        break;
    default:
        JPEG_ERROR_LOG("%s::Invalid input color format(%d) fail\n", __func__, iV4l2ColorFormat);
        t_iPlaneNum = 0;
        return ERROR_INVALID_COLOR_FORMAT;
        break;
    }

    switch (iV4l2ColorFormat) {
    case V4L2_PIX_FMT_YUV420:
    case V4L2_PIX_FMT_NV16:
    case V4L2_PIX_FMT_YUYV:
    case V4L2_PIX_FMT_RGB565X:
    case V4L2_PIX_FMT_BGR32:
    case V4L2_PIX_FMT_RGB32:
        t_iPlaneNum = 1;
        break;
    default:
        JPEG_ERROR_LOG("%s::Invalid input color format(%d) fail\n", __func__, iV4l2ColorFormat);
        t_iPlaneNum = 0;
        return ERROR_INVALID_COLOR_FORMAT;
    }

    return ERROR_NONE;
}

int ExynosJpegBase::setJpegFormat(enum MODE eMode, int iV4l2JpegFormat)
{
    if (t_bFlagCreate == false) {
        return ERROR_JPEG_DEVICE_NOT_CREATE_YET;
    }

    switch(iV4l2JpegFormat) {
    case V4L2_PIX_FMT_JPEG_444:
    case V4L2_PIX_FMT_JPEG_422:
    case V4L2_PIX_FMT_JPEG_420:
    case V4L2_PIX_FMT_JPEG_GRAY:
        switch (eMode) {
        case MODE_ENCODE:
            t_stJpegConfig.pix.enc_fmt.out_fmt = iV4l2JpegFormat;
            break;
        case MODE_DECODE:
        t_stJpegConfig.pix.dec_fmt.in_fmt = iV4l2JpegFormat;
            break;
        default:
            return ERROR_INVALID_JPEG_MODE;
            break;
        }
        break;
    default:
        return ERROR_INVALID_JPEG_FORMAT;
        break;
    }

    return ERROR_NONE;
}

int ExynosJpegBase::setColorBufSize(enum MODE eMode, int *piBufSize, int iSize)
{
    int iFormat;

    switch (eMode) {
    case MODE_ENCODE:
        iFormat = t_stJpegConfig.pix.enc_fmt.in_fmt;
        break;
    case MODE_DECODE:
        iFormat = t_stJpegConfig.pix.dec_fmt.out_fmt;
        break;
    default:
        return ERROR_INVALID_JPEG_MODE;
        break;
    }

    return setColorBufSize(iFormat, piBufSize, iSize, t_stJpegConfig.width, t_stJpegConfig.height);
}

int ExynosJpegBase::setColorBufSize(int iFormat, int *piBufSize, int iSize, int width, int height)
{
    int pBufSize[3];

    if(iSize>3) {
        return ERROR_INVALID_IMAGE_SIZE;
    }

    switch (iFormat) {
    case V4L2_PIX_FMT_YUYV:
    case V4L2_PIX_FMT_RGB565X:
    case V4L2_PIX_FMT_NV16:
        pBufSize[0] = width*height*2;
        pBufSize[1] = 0;
        pBufSize[2] = 0;
        break;
    case V4L2_PIX_FMT_RGB32:
    case V4L2_PIX_FMT_BGR32:
        pBufSize[0] = width*height*4;
        pBufSize[1] = 0;
        pBufSize[2] = 0;
        break;
    case V4L2_PIX_FMT_YUV420:
        pBufSize[0] = (width*height*3)/2;
        pBufSize[1] = 0;
        pBufSize[2] = 0;
        break;
    default:
        pBufSize[0] = width*height*4;
        pBufSize[1] = width*height*4;
        pBufSize[2] = width*height*4;
        break;
    }

    memcpy(piBufSize, pBufSize, iSize*sizeof(int));

    return ERROR_NONE;
}

int ExynosJpegBase::updateConfig(enum MODE eMode, int iInBufs, int iOutBufs, int iInBufPlanes, int iOutBufPlanes)
{
    if (t_bFlagCreate == false) {
        return ERROR_JPEG_DEVICE_NOT_CREATE_YET;
    }

    int iRet = ERROR_NONE;

    if (eMode == MODE_ENCODE) {
        iRet = t_v4l2SetJpegcomp(t_iJpegFd, t_stJpegConfig.enc_qual);
        if (iRet < 0) {
            JPEG_ERROR_LOG("[%s,%d]: S_JPEGCOMP failed\n", __func__,iRet);
            return ERROR_INVALID_JPEG_CONFIG;
        }
    }

    t_stJpegConfig.numOfPlanes = iInBufPlanes;
    t_stJpegConfig.mode = eMode;

    iRet = t_v4l2SetFmt(t_iJpegFd, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, &t_stJpegConfig);
    if (iRet < 0) {
        JPEG_ERROR_LOG("[%s,%d]: jpeg input S_FMT failed\n", __func__,iRet);
        return ERROR_INVALID_JPEG_CONFIG;
    }

    struct BUF_INFO stBufInfo;

    stBufInfo.numOfPlanes = iInBufs;
    stBufInfo.buf_type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    stBufInfo.memory = V4L2_MEMORY_DMABUF;

    iRet = t_v4l2Reqbufs(t_iJpegFd, iInBufs, &stBufInfo);
    if (iRet < 0) {
        JPEG_ERROR_LOG("[%s:%d]: Input REQBUFS failed\n", __func__, iRet);
        return ERROR_EXCUTE_FAIL;
    }

    t_stJpegConfig.numOfPlanes = iOutBufPlanes;
    iRet = t_v4l2SetFmt(t_iJpegFd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, &t_stJpegConfig);
    if (iRet < 0) {
        JPEG_ERROR_LOG("[%s,%d]: jpeg output S_FMT failed\n", __func__,iRet);
        return ERROR_INVALID_JPEG_CONFIG;
    }

    stBufInfo.numOfPlanes = iOutBufs;
    stBufInfo.buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

    iRet = t_v4l2Reqbufs(t_iJpegFd, iOutBufs, &stBufInfo);
    if (iRet < 0) {
        JPEG_ERROR_LOG("[%s:%d]: Output REQBUFS failed\n", __func__, iRet);
        return ERROR_REQBUF_FAIL;
    }

    return ERROR_NONE;
}

int ExynosJpegBase::execute(int iInBufPlanes, int iOutBufPlanes)
{
    if (t_bFlagCreate == false) {
        return ERROR_JPEG_DEVICE_NOT_CREATE_YET;
    }

    struct BUF_INFO stBufInfo;
    int iRet = ERROR_NONE;

    t_bFlagExcute = true;

    stBufInfo.numOfPlanes = iInBufPlanes;
    stBufInfo.buf_type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;

    stBufInfo.memory = V4L2_MEMORY_DMABUF;

    iRet = t_v4l2Qbuf(t_iJpegFd, &stBufInfo, &t_stJpegInbuf);
    if (iRet < 0) {
        JPEG_ERROR_LOG("[%s:%d]: Input QBUF failed\n", __func__, iRet);
        return ERROR_EXCUTE_FAIL;
    }

    stBufInfo.numOfPlanes = iOutBufPlanes;
    stBufInfo.buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

    iRet = t_v4l2Qbuf(t_iJpegFd, &stBufInfo, &t_stJpegOutbuf);
    if (iRet < 0) {
        JPEG_ERROR_LOG("[%s:%d]: Output QBUF failed\n", __func__, iRet);
        return ERROR_EXCUTE_FAIL;
    }

    iRet = t_v4l2StreamOn(t_iJpegFd, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);
    if (iRet < 0) {
        JPEG_ERROR_LOG("[%s:%d]: input stream on failed\n", __func__, iRet);
        return ERROR_EXCUTE_FAIL;
    }
    iRet = t_v4l2StreamOn(t_iJpegFd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
    if (iRet < 0) {
        JPEG_ERROR_LOG("[%s:%d]: output stream on failed\n", __func__, iRet);
        return ERROR_EXCUTE_FAIL;
    }

    iRet = t_v4l2Dqbuf(t_iJpegFd, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, V4L2_MEMORY_MMAP, iInBufPlanes);
    if (iRet < 0) {
        JPEG_ERROR_LOG("[%s:%d]: Intput DQBUF failed\n", __func__, iRet);
        return ERROR_EXCUTE_FAIL;
    }
    iRet = t_v4l2Dqbuf(t_iJpegFd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, V4L2_MEMORY_MMAP, iOutBufPlanes);
    if (iRet < 0) {
        JPEG_ERROR_LOG("[%s:%d]: Output DQBUF failed\n", __func__, iRet);
        return ERROR_EXCUTE_FAIL;
    }

    return ERROR_NONE;
}

