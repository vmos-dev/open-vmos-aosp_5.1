/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
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

#include "CameraHal.h"

namespace Ti {
namespace Camera {

const char CameraHal::PARAMS_DELIMITER []= ",";

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

struct timeval CameraHal::ppm_start;

#endif

#if PPM_INSTRUMENTATION

/**
   @brief PPM instrumentation

   Dumps the current time offset. The time reference point
   lies within the CameraHAL constructor.

   @param str - log message
   @return none

 */
void CameraHal::PPM(const char* str){
    struct timeval ppm;

    gettimeofday(&ppm, NULL);
    ppm.tv_sec = ppm.tv_sec - ppm_start.tv_sec;
    ppm.tv_sec = ppm.tv_sec * 1000000;
    ppm.tv_sec = ppm.tv_sec + ppm.tv_usec - ppm_start.tv_usec;

    CAMHAL_LOGI("PPM: %s :%ld.%ld ms", str, ( ppm.tv_sec /1000 ), ( ppm.tv_sec % 1000 ));
}

#elif PPM_INSTRUMENTATION_ABS

/**
   @brief PPM instrumentation

   Dumps the current time offset. The time reference point
   lies within the CameraHAL constructor. This implemetation
   will also dump the abosolute timestamp, which is useful when
   post calculation is done with data coming from the upper
   layers (Camera application etc.)

   @param str - log message
   @return none

 */
void CameraHal::PPM(const char* str){
    struct timeval ppm;

    unsigned long long elapsed, absolute;
    gettimeofday(&ppm, NULL);
    elapsed = ppm.tv_sec - ppm_start.tv_sec;
    elapsed *= 1000000;
    elapsed += ppm.tv_usec - ppm_start.tv_usec;
    absolute = ppm.tv_sec;
    absolute *= 1000;
    absolute += ppm.tv_usec /1000;

    CAMHAL_LOGI("PPM: %s :%llu.%llu ms : %llu ms", str, ( elapsed /1000 ), ( elapsed % 1000 ), absolute);
}

#endif

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

/**
   @brief PPM instrumentation

   Calculates and dumps the elapsed time using 'ppm_first' as
   reference.

   @param str - log message
   @return none

 */
void CameraHal::PPM(const char* str, struct timeval* ppm_first, ...){
    char temp_str[256];
    struct timeval ppm;
    unsigned long long absolute;
    va_list args;

    va_start(args, ppm_first);
    vsprintf(temp_str, str, args);
    gettimeofday(&ppm, NULL);
    absolute = ppm.tv_sec;
    absolute *= 1000;
    absolute += ppm.tv_usec /1000;
    ppm.tv_sec = ppm.tv_sec - ppm_first->tv_sec;
    ppm.tv_sec = ppm.tv_sec * 1000000;
    ppm.tv_sec = ppm.tv_sec + ppm.tv_usec - ppm_first->tv_usec;

    CAMHAL_LOGI("PPM: %s :%ld.%ld ms :  %llu ms", temp_str, ( ppm.tv_sec /1000 ), ( ppm.tv_sec % 1000 ), absolute);

    va_end(args);
}

#endif


/** Common utility function definitions used all over the HAL */

unsigned int CameraHal::getBPP(const char* format) {
    unsigned int bytesPerPixel;

   // Calculate bytes per pixel based on the pixel format
   if (strcmp(format, android::CameraParameters::PIXEL_FORMAT_YUV422I) == 0) {
       bytesPerPixel = 2;
   } else if (strcmp(format, android::CameraParameters::PIXEL_FORMAT_RGB565) == 0 ||
              strcmp(format, android::CameraParameters::PIXEL_FORMAT_BAYER_RGGB) == 0) {
       bytesPerPixel = 2;
   } else if (strcmp(format, android::CameraParameters::PIXEL_FORMAT_YUV420SP) == 0) {
       bytesPerPixel = 1;
   } else {
       bytesPerPixel = 1;
   }

   return bytesPerPixel;
}

void CameraHal::getXYFromOffset(unsigned int *x, unsigned int *y,
                                unsigned int offset, unsigned int stride,
                                const char* format)
{
   CAMHAL_ASSERT( x && y && format && (0U < stride) );

   *x = (offset % stride) / getBPP(format);
   *y = (offset / stride);
}

const char* CameraHal::getPixelFormatConstant(const char* parametersFormat)
{
    const char *pixelFormat = NULL;

    if ( NULL != parametersFormat ) {
        if ( 0 == strcmp(parametersFormat, (const char *) android::CameraParameters::PIXEL_FORMAT_YUV422I) ) {
            CAMHAL_LOGVA("CbYCrY format selected");
            pixelFormat = (const char *) android::CameraParameters::PIXEL_FORMAT_YUV422I;
        } else if ( (0 == strcmp(parametersFormat, (const char *) android::CameraParameters::PIXEL_FORMAT_YUV420SP)) ||
                    (0 == strcmp(parametersFormat, (const char *) android::CameraParameters::PIXEL_FORMAT_YUV420P)) ) {
            // TODO(XXX): We are treating YV12 the same as YUV420SP
            CAMHAL_LOGVA("YUV420SP format selected");
            pixelFormat = (const char *) android::CameraParameters::PIXEL_FORMAT_YUV420SP;
        } else if ( 0 == strcmp(parametersFormat, (const char *) android::CameraParameters::PIXEL_FORMAT_RGB565) ) {
            CAMHAL_LOGVA("RGB565 format selected");
            pixelFormat = (const char *) android::CameraParameters::PIXEL_FORMAT_RGB565;
        } else if ( 0 == strcmp(parametersFormat, (const char *) android::CameraParameters::PIXEL_FORMAT_BAYER_RGGB) ) {
            CAMHAL_LOGVA("BAYER format selected");
            pixelFormat = (const char *) android::CameraParameters::PIXEL_FORMAT_BAYER_RGGB;
        } else if ( 0 == strcmp(parametersFormat, android::CameraParameters::PIXEL_FORMAT_JPEG) ) {
            CAMHAL_LOGVA("JPEG format selected");
            pixelFormat = (const char *) android::CameraParameters::PIXEL_FORMAT_JPEG;
        } else {
            CAMHAL_LOGEA("Invalid format, NV12 format selected as default");
            pixelFormat = (const char *) android::CameraParameters::PIXEL_FORMAT_YUV420SP;
        }
    } else {
        CAMHAL_LOGEA("Preview format is NULL, defaulting to NV12");
        pixelFormat = (const char *) android::CameraParameters::PIXEL_FORMAT_YUV420SP;
    }

    return pixelFormat;
}

size_t CameraHal::calculateBufferSize(const char* parametersFormat, int width, int height)
{
    int bufferSize = -1;

    if ( NULL != parametersFormat ) {
        if ( 0 == strcmp(parametersFormat, (const char *) android::CameraParameters::PIXEL_FORMAT_YUV422I) ) {
            bufferSize = width * height * 2;
        } else if ( (0 == strcmp(parametersFormat, android::CameraParameters::PIXEL_FORMAT_YUV420SP)) ||
                    (0 == strcmp(parametersFormat, android::CameraParameters::PIXEL_FORMAT_YUV420P)) ) {
            bufferSize = width * height * 3 / 2;
        } else if ( 0 == strcmp(parametersFormat, (const char *) android::CameraParameters::PIXEL_FORMAT_RGB565) ) {
            bufferSize = width * height * 2;
        } else if ( 0 == strcmp(parametersFormat, (const char *) android::CameraParameters::PIXEL_FORMAT_BAYER_RGGB) ) {
            bufferSize = width * height * 2;
        } else {
            CAMHAL_LOGEA("Invalid format");
            bufferSize = 0;
        }
    } else {
        CAMHAL_LOGEA("Preview format is NULL");
        bufferSize = 0;
    }

    return bufferSize;
}


} // namespace Camera
} // namespace Ti
