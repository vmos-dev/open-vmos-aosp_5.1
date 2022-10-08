/*
 * Copyright (C) 2010 The Android Open Source Project
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

#ifndef ANDROID_CAMERA_HARDWARE_DECODER_LIBJPEG_H
#define ANDROID_CAMERA_HARDWARE_DECODER_LIBJPEG_H

#include "CameraHal.h"

extern "C" {
#include "jhead.h"

#undef TRUE
#undef FALSE

}


namespace Ti {
namespace Camera {

class Decoder_libjpeg
{

public:
    Decoder_libjpeg();
    ~Decoder_libjpeg();
    int readDHTSize();
    int appendDHT(unsigned char *jpeg_src, int filled_len, unsigned char *jpeg_with_dht_buffer, int buff_size);
    bool decode(unsigned char *jpeg_src, int filled_len, unsigned char *nv12_buffer, int stride);

private:
    void release();
    unsigned char **Y_Plane;
    unsigned char **U_Plane;
    unsigned char **V_Plane;
    unsigned char *UV_Plane;
    unsigned int mWidth, mHeight;
};

} // namespace Camera
} // namespace Ti

#endif

