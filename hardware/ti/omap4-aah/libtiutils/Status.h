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

#ifndef TI_UTILS_STATUS_H
#define TI_UTILS_STATUS_H

#include <utils/Errors.h>

#include "UtilsCommon.h"




namespace Ti {




typedef int status_t;

#define TI_CAMERA_DEFINE_STATUS_CODE(x) x = android::x,
enum {
    TI_CAMERA_DEFINE_STATUS_CODE(OK)
    TI_CAMERA_DEFINE_STATUS_CODE(NO_ERROR)
    TI_CAMERA_DEFINE_STATUS_CODE(UNKNOWN_ERROR)
    TI_CAMERA_DEFINE_STATUS_CODE(NO_MEMORY)
    TI_CAMERA_DEFINE_STATUS_CODE(INVALID_OPERATION)
    TI_CAMERA_DEFINE_STATUS_CODE(BAD_VALUE)
    TI_CAMERA_DEFINE_STATUS_CODE(BAD_TYPE)
    TI_CAMERA_DEFINE_STATUS_CODE(NAME_NOT_FOUND)
    TI_CAMERA_DEFINE_STATUS_CODE(PERMISSION_DENIED)
    TI_CAMERA_DEFINE_STATUS_CODE(NO_INIT)
    TI_CAMERA_DEFINE_STATUS_CODE(ALREADY_EXISTS)
    TI_CAMERA_DEFINE_STATUS_CODE(DEAD_OBJECT)
    TI_CAMERA_DEFINE_STATUS_CODE(FAILED_TRANSACTION)
    TI_CAMERA_DEFINE_STATUS_CODE(JPARKS_BROKE_IT)
    TI_CAMERA_DEFINE_STATUS_CODE(BAD_INDEX)
    TI_CAMERA_DEFINE_STATUS_CODE(NOT_ENOUGH_DATA)
    TI_CAMERA_DEFINE_STATUS_CODE(WOULD_BLOCK)
    TI_CAMERA_DEFINE_STATUS_CODE(TIMED_OUT)
    TI_CAMERA_DEFINE_STATUS_CODE(UNKNOWN_TRANSACTION)
    TI_CAMERA_DEFINE_STATUS_CODE(FDS_NOT_ALLOWED)
};
#undef TI_CAMERA_DEFINE_STATUS_CODE




} // namespace Ti




#endif // TI_UTILS_STATUS_H
