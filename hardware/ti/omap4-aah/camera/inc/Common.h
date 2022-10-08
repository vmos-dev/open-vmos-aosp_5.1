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

#ifndef CAMERAHAL_COMMON_H
#define CAMERAHAL_COMMON_H

#include "UtilsCommon.h"
#include "DebugUtils.h"
#include "Status.h"




// logging functions
#ifdef CAMERAHAL_DEBUG
#   define CAMHAL_LOGD  DBGUTILS_LOGD
#   define CAMHAL_LOGDA DBGUTILS_LOGDA
#   define CAMHAL_LOGDB DBGUTILS_LOGDB
#   ifdef CAMERAHAL_DEBUG_VERBOSE
#       define CAMHAL_LOGV  DBGUTILS_LOGV
#       define CAMHAL_LOGVA DBGUTILS_LOGVA
#       define CAMHAL_LOGVB DBGUTILS_LOGVB
#   else
#       define CAMHAL_LOGV(...)
#       define CAMHAL_LOGVA(str)
#       define CAMHAL_LOGVB(str, ...)
#   endif
#else
#   define CAMHAL_LOGD(...)
#   define CAMHAL_LOGDA(str)
#   define CAMHAL_LOGDB(str, ...)
#   define CAMHAL_LOGV(...)
#   define CAMHAL_LOGVA(str)
#   define CAMHAL_LOGVB(str, ...)
#endif

#define CAMHAL_LOGI  DBGUTILS_LOGI
#define CAMHAL_LOGW  DBGUTILS_LOGW
#define CAMHAL_LOGE  DBGUTILS_LOGE
#define CAMHAL_LOGEA DBGUTILS_LOGEA
#define CAMHAL_LOGEB DBGUTILS_LOGEB
#define CAMHAL_LOGF  DBGUTILS_LOGF

#define CAMHAL_ASSERT DBGUTILS_ASSERT
#define CAMHAL_ASSERT_X DBGUTILS_ASSERT_X

#define CAMHAL_UNUSED(x) (void)x




#endif // CAMERAHAL_COMMON_H
