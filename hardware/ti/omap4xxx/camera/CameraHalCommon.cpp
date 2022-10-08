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

namespace android {

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

    ALOGD("PPM: %s :%ld.%ld ms", str, ( ppm.tv_sec /1000 ), ( ppm.tv_sec % 1000 ));
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

    ALOGD("PPM: %s :%llu.%llu ms : %llu ms", str, ( elapsed /1000 ), ( elapsed % 1000 ), absolute);
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

    ALOGD("PPM: %s :%ld.%ld ms :  %llu ms", temp_str, ( ppm.tv_sec /1000 ), ( ppm.tv_sec % 1000 ), absolute);

    va_end(args);
}

#endif

};


