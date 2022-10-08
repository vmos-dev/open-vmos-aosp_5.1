#ifndef MORPHO_NR_EXT_H
#define MORPHO_NR_EXT_H

#include "morpho_noise_reduction.h"
/*
return == 0 : OK
return != 0 : NG (Please print the return value to check Error types)
*/
MORPHO_API(int)
LINK_mm_camera_morpho_noise_reduction(
    unsigned char* yuvImage,
    int width,
    int height,
    int y_level,
    int c_level);

#endif //MORPHO_NR_EXT_H
