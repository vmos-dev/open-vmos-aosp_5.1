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


#include "ErrorUtils.h"

namespace Ti {
namespace Utils {

/**
   @brief Method to convert from POSIX to Android errors

   @param error Any of the standard POSIX error codes (defined in bionic/libc/kernel/common/asm-generic/errno.h)
   @return Any of the standard Android error code (defined in frameworks/base/include/utils/Errors.h)
 */
status_t ErrorUtils::posixToAndroidError(int error)
{
    switch(error)
        {
        case 0:
            return NO_ERROR;
        case EINVAL:
        case EFBIG:
        case EMSGSIZE:
        case E2BIG:
        case EFAULT:
        case EILSEQ:
            return BAD_VALUE;
        case ENOSYS:
            return INVALID_OPERATION;
        case EACCES:
        case EPERM:
            return PERMISSION_DENIED;
        case EADDRINUSE:
        case EAGAIN:
        case EALREADY:
        case EBUSY:
        case EEXIST:
        case EINPROGRESS:
            return ALREADY_EXISTS;
        case ENOMEM:
            return NO_MEMORY;
        default:
            return UNKNOWN_ERROR;
        };

    return NO_ERROR;
}


/**
   @brief Method to convert from TI OSAL to Android errors

   @param error Any of the standard TI OSAL error codes (defined in
                                                                                            hardware/ti/omx/ducati/domx/system/mm_osal/inc/timm_osal_error.h)
   @return Any of the standard Android error code  (defined in frameworks/base/include/utils/Errors.h)
 */
status_t ErrorUtils::osalToAndroidError(TIMM_OSAL_ERRORTYPE error)
{
    switch(error)
    {
    case TIMM_OSAL_ERR_NONE:
        return NO_ERROR;
    case TIMM_OSAL_ERR_ALLOC:
        return NO_MEMORY;
    default:
        return UNKNOWN_ERROR;
    }

    return NO_ERROR;
}

/**
   @brief Method to convert from OMX to Android errors

   @param error Any of the standard OMX error codes (defined in hardware/ti/omx/ducati/domx/system/omx_core/inc/OMX_Core.h)
   @return Any of the standard Android error code (defined in frameworks/base/include/utils/Errors.h)
 */
status_t ErrorUtils::omxToAndroidError(OMX_ERRORTYPE error)
{
    switch(error)
        {
        case OMX_ErrorNone:
            return NO_ERROR;
        case OMX_ErrorBadParameter:
        case OMX_ErrorInvalidComponentName:
        case OMX_ErrorUndefined:
        case OMX_ErrorInvalidState:
        case OMX_ErrorStreamCorrupt:
        case OMX_ErrorPortsNotCompatible:
        case OMX_ErrorVersionMismatch:
        case OMX_ErrorMbErrorsInFrame:
            return BAD_VALUE;
        case OMX_ErrorInsufficientResources:
            return NO_MEMORY;
        case OMX_ErrorComponentNotFound:
        case OMX_ErrorNotImplemented:
        case OMX_ErrorFormatNotDetected:
        case OMX_ErrorUnsupportedSetting:
            return NAME_NOT_FOUND;
        case OMX_ErrorUnderflow:
        case OMX_ErrorOverflow:
        case OMX_ErrorUnsupportedIndex:
        case OMX_ErrorBadPortIndex:
            return BAD_INDEX;
        case OMX_ErrorHardware:
        case OMX_ErrorContentPipeCreationFailed:
        case OMX_ErrorContentPipeOpenFailed:
            return FAILED_TRANSACTION;
        case OMX_ErrorTimeout:
           return TIMED_OUT;
        case OMX_ErrorSameState:
        case OMX_ErrorIncorrectStateTransition:
        case OMX_ErrorIncorrectStateOperation:
           return PERMISSION_DENIED;
        case OMX_ErrorTunnelingUnsupported:
           return INVALID_OPERATION;
        default:
            return UNKNOWN_ERROR;
        }

    return NO_ERROR;
}


} // namespace Utils
} // namespace Ti
