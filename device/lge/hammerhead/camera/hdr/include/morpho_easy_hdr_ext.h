#ifndef MORPHO_EASY_HDR_EXT_H
#define MORPHO_EASY_HDR_EXT_H

#include "morpho_easy_hdr.h"
/*
return == 0 : OK
return != 0 : NG (Please print the return value to check Error types)
*/
MORPHO_API(int)
LINK_mm_camera_HDR(
    unsigned char* yuvInput01,
    unsigned char* yuvInput02,
    unsigned char* yuvInput03,
    unsigned char* pHDROutImage,
    int width,
    int height,
    int indoor);

#endif //MORPHO_EASY_HDR_EXT_H
