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

#ifndef ERROR_UTILS_H
#define ERROR_UTILS_H

///Header file where all the android error codes are defined
#include <utils/Errors.h>

///Header file where all the OMX error codes are defined
#include "OMX_Core.h"

#include "Status.h"


extern "C"
{
///Header file where all the TI OSAL error codes are defined
#include "timm_osal_error.h"
};

namespace Ti {
namespace Utils {

///Generic class with static methods to convert any standard error type to Android error type
class ErrorUtils
{
public:
    ///Method to convert from POSIX to Android errors
    static status_t posixToAndroidError(int error);

    ///Method to convert from TI OSAL to Android errors
    static status_t osalToAndroidError(TIMM_OSAL_ERRORTYPE error);

    ///Method to convert from OMX to Android errors
    static status_t omxToAndroidError(OMX_ERRORTYPE error);

};

} // namespace Utils
} // namespace Ti

#endif /// ERROR_UTILS_H
