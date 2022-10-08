/*
 * Copyright 2012 Samsung Electronics S.LSI Co. LTD
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

/*
 * @file        Exynos_OSAL_Android.cpp
 * @brief
 * @author      Seungbeom Kim (sbcrux.kim@samsung.com)
 * @author      Hyeyeon Chung (hyeon.chung@samsung.com)
 * @author      Yunji Kim (yunji.kim@samsung.com)
 * @author      Jinsung Yang (jsgood.yang@samsung.com)
 * @version     2.0.0
 * @history
 *   2012.02.20 : Create
 */

#include <stdio.h>
#include <stdlib.h>

#include <system/window.h>
#include <ui/GraphicBuffer.h>
#include <ui/GraphicBufferMapper.h>
#include <ui/Rect.h>
#include <media/hardware/HardwareAPI.h>
#include <hardware/hardware.h>
#include <media/hardware/OMXPluginBase.h>
#include <media/hardware/MetadataBufferType.h>
#include <gralloc_priv.h>

#include "Exynos_OSAL_Mutex.h"
#include "Exynos_OSAL_Semaphore.h"
#include "Exynos_OMX_Baseport.h"
#include "Exynos_OMX_Basecomponent.h"
#include "Exynos_OMX_Macros.h"
#include "Exynos_OMX_Vdec.h"
#include "Exynos_OMX_Venc.h"
#include "Exynos_OSAL_Android.h"
#include "exynos_format.h"
#include "ion.h"

#undef  EXYNOS_LOG_TAG
#define EXYNOS_LOG_TAG    "Exynos_OSAL_Android"
#define EXYNOS_LOG_OFF
#include "Exynos_OSAL_Log.h"

using namespace android;

#ifdef __cplusplus
extern "C" {
#endif

int getIonFd(gralloc_module_t const *module)
{
    private_module_t* m = const_cast<private_module_t*>(reinterpret_cast<const private_module_t*>(module));
    return m->ionfd;
}

OMX_ERRORTYPE Exynos_OSAL_LockANBHandle(
    OMX_IN OMX_U32 handle,
    OMX_IN OMX_U32 width,
    OMX_IN OMX_U32 height,
    OMX_IN OMX_COLOR_FORMATTYPE format,
    OMX_OUT OMX_U32 *pStride,
    OMX_OUT OMX_PTR planes)
{
    FunctionIn();

    OMX_ERRORTYPE ret = OMX_ErrorNone;
    GraphicBufferMapper &mapper = GraphicBufferMapper::get();
    buffer_handle_t bufferHandle = (buffer_handle_t) handle;
    private_handle_t *priv_hnd = (private_handle_t *) bufferHandle;
    Rect bounds(width, height);
    ExynosVideoPlane *vplanes = (ExynosVideoPlane *) planes;
    void *vaddr[MAX_BUFFER_PLANE];

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "%s: handle: 0x%x", __func__, handle);

    int usage = 0;

    switch (format) {
    case OMX_COLOR_FormatYUV420Planar:
    case OMX_COLOR_FormatYUV420SemiPlanar:
    case OMX_SEC_COLOR_FormatNV12Tiled:
        usage = GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN;
        break;
    default:
        usage = GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN;
        break;
    }

    if (mapper.lock(bufferHandle, usage, bounds, vaddr) != 0) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: mapper.lock() fail", __func__);
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }

    vplanes[0].fd = priv_hnd->fd;
    vplanes[0].offset = 0;
    vplanes[0].addr = vaddr[0];
    vplanes[1].fd = priv_hnd->fd1;
    vplanes[1].offset = 0;
    vplanes[1].addr = vaddr[1];
    vplanes[2].fd = priv_hnd->fd2;
    vplanes[2].offset = 0;
    vplanes[2].addr = vaddr[2];

    *pStride = priv_hnd->stride;

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "%s: buffer locked: 0x%x", __func__, *vaddr);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OSAL_UnlockANBHandle(OMX_IN OMX_U32 handle)
{
    FunctionIn();

    OMX_ERRORTYPE ret = OMX_ErrorNone;
    GraphicBufferMapper &mapper = GraphicBufferMapper::get();
    buffer_handle_t bufferHandle = (buffer_handle_t) handle;

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "%s: handle: 0x%x", __func__, handle);

    if (mapper.unlock(bufferHandle) != 0) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: mapper.unlock() fail", __func__);
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "%s: buffer unlocked: 0x%x", __func__, handle);

EXIT:
    FunctionOut();

    return ret;
}

OMX_COLOR_FORMATTYPE Exynos_OSAL_GetANBColorFormat(OMX_IN OMX_U32 handle)
{
    FunctionIn();

    OMX_COLOR_FORMATTYPE ret = OMX_COLOR_FormatUnused;
    private_handle_t *priv_hnd = (private_handle_t *) handle;

    ret = Exynos_OSAL_Hal2OMXPixelFormat(priv_hnd->format);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "ColorFormat: 0x%x", ret);

EXIT:
    FunctionOut();

    return ret;
}

OMX_U32 Exynos_OSAL_GetANBStride(OMX_IN OMX_U32 handle)
{
    FunctionIn();

    OMX_U32 nStride = 0;
    private_handle_t *priv_hnd = (private_handle_t *) handle;

    nStride = priv_hnd->stride;

EXIT:
    FunctionOut();

    return nStride;
}

OMX_ERRORTYPE Exynos_OSAL_LockMetaData(
    OMX_IN OMX_PTR pBuffer,
    OMX_IN OMX_U32 width,
    OMX_IN OMX_U32 height,
    OMX_IN OMX_COLOR_FORMATTYPE format,
    OMX_OUT OMX_U32 *pStride,
    OMX_OUT OMX_PTR planes)
{
    FunctionIn();

    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_PTR pBuf;

    ret = Exynos_OSAL_GetInfoFromMetaData((OMX_BYTE)pBuffer, &pBuf);
    if (ret == OMX_ErrorNone) {
        ret = Exynos_OSAL_LockANBHandle((OMX_U32)pBuf, width, height, format, pStride, planes);
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OSAL_UnlockMetaData(OMX_IN OMX_PTR pBuffer)
{
    FunctionIn();

    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_PTR pBuf;

    ret = Exynos_OSAL_GetInfoFromMetaData((OMX_BYTE)pBuffer, &pBuf);
    if (ret == OMX_ErrorNone)
        ret = Exynos_OSAL_UnlockANBHandle((OMX_U32)pBuf);

EXIT:
    FunctionOut();

    return ret;
}

OMX_HANDLETYPE Exynos_OSAL_RefANB_Create()
{
    int i = 0;
    EXYNOS_OMX_REF_HANDLE *phREF = NULL;
    gralloc_module_t      *module = NULL;

    OMX_ERRORTYPE err = OMX_ErrorNone;

    FunctionIn();

    phREF = (EXYNOS_OMX_REF_HANDLE *) Exynos_OSAL_Malloc(sizeof(EXYNOS_OMX_REF_HANDLE));
    if (phREF == NULL)
        goto EXIT;

    Exynos_OSAL_Memset(phREF, 0, sizeof(EXYNOS_OMX_REF_HANDLE));
    for (i = 0; i < MAX_BUFFER_REF; i++) {
        phREF->SharedBuffer[i].BufferFd  = -1;
        phREF->SharedBuffer[i].BufferFd1 = -1;
        phREF->SharedBuffer[i].BufferFd2 = -1;
    }

    hw_get_module(GRALLOC_HARDWARE_MODULE_ID, (const hw_module_t **)&module);
    phREF->pGrallocModule = (OMX_PTR)module;

    err = Exynos_OSAL_MutexCreate(&phREF->hMutex);
    if (err != OMX_ErrorNone) {
        Exynos_OSAL_Free(phREF);
        phREF = NULL;
    }

EXIT:
    FunctionOut();

    return ((OMX_HANDLETYPE)phREF);
}

OMX_ERRORTYPE Exynos_OSAL_RefANB_Reset(OMX_HANDLETYPE hREF)
{
    int i = 0;
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    EXYNOS_OMX_REF_HANDLE *phREF = (EXYNOS_OMX_REF_HANDLE *)hREF;
    gralloc_module_t* module = NULL;

    FunctionIn();

    if (phREF == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    module = (gralloc_module_t *)phREF->pGrallocModule;

    Exynos_OSAL_MutexLock(phREF->hMutex);
    for (i = 0; i < MAX_BUFFER_REF; i++) {
        if (phREF->SharedBuffer[i].BufferFd > -1) {
            while(phREF->SharedBuffer[i].cnt > 0) {
                if (phREF->SharedBuffer[i].BufferFd > -1)
                    ion_decRef(getIonFd(module), phREF->SharedBuffer[i].pIonHandle);
                if (phREF->SharedBuffer[i].BufferFd1 > -1)
                    ion_decRef(getIonFd(module), phREF->SharedBuffer[i].pIonHandle1);
                if (phREF->SharedBuffer[i].BufferFd2 > -1)
                    ion_decRef(getIonFd(module), phREF->SharedBuffer[i].pIonHandle2);
                phREF->SharedBuffer[i].cnt--;
            }
            phREF->SharedBuffer[i].BufferFd    = -1;
            phREF->SharedBuffer[i].BufferFd1   = -1;
            phREF->SharedBuffer[i].BufferFd2   = -1;
            phREF->SharedBuffer[i].pIonHandle  = NULL;
            phREF->SharedBuffer[i].pIonHandle1 = NULL;
            phREF->SharedBuffer[i].pIonHandle2 = NULL;
        }
    }
    Exynos_OSAL_MutexUnlock(phREF->hMutex);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OSAL_RefANB_Terminate(OMX_HANDLETYPE hREF)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    EXYNOS_OMX_REF_HANDLE *phREF = (EXYNOS_OMX_REF_HANDLE *)hREF;
    FunctionIn();

    if (phREF == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    Exynos_OSAL_RefANB_Reset(phREF);

    phREF->pGrallocModule = NULL;

    ret = Exynos_OSAL_MutexTerminate(phREF->hMutex);
    if (ret != OMX_ErrorNone)
        goto EXIT;

    Exynos_OSAL_Free(phREF);
    phREF = NULL;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OSAL_RefANB_Increase(OMX_HANDLETYPE hREF, OMX_PTR pBuffer)
{
    int i;
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    buffer_handle_t bufferHandle = (buffer_handle_t) pBuffer; //pANB->handle
    private_handle_t *priv_hnd = (private_handle_t *) bufferHandle;
    EXYNOS_OMX_REF_HANDLE *phREF = (EXYNOS_OMX_REF_HANDLE *)hREF;
    gralloc_module_t* module = NULL;

    unsigned long *pIonHandle;
    unsigned long *pIonHandle1;
    unsigned long *pIonHandle2;

    FunctionIn();

    if (phREF == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    module = (gralloc_module_t *)phREF->pGrallocModule;

    Exynos_OSAL_MutexLock(phREF->hMutex);

    if (priv_hnd->fd >= 0) {
        ion_incRef(getIonFd(module), priv_hnd->fd, &pIonHandle);
    }
    if (priv_hnd->fd1 >= 0) {
        ion_incRef(getIonFd(module), priv_hnd->fd1, &pIonHandle1);
    }
    if (priv_hnd->fd2 >= 0) {
        ion_incRef(getIonFd(module), priv_hnd->fd2, &pIonHandle2);
    }

    for (i = 0; i < MAX_BUFFER_REF; i++) {
        if (phREF->SharedBuffer[i].BufferFd == priv_hnd->fd) {
            phREF->SharedBuffer[i].cnt++;
            break;
        }
    }

    if (i >=  MAX_BUFFER_REF) {
        for (i = 0; i < MAX_BUFFER_REF; i++) {
            if (phREF->SharedBuffer[i].BufferFd == -1) {
                phREF->SharedBuffer[i].BufferFd    = priv_hnd->fd;
                phREF->SharedBuffer[i].BufferFd1   = priv_hnd->fd1;
                phREF->SharedBuffer[i].BufferFd2   = priv_hnd->fd2;
                phREF->SharedBuffer[i].pIonHandle  = pIonHandle;
                phREF->SharedBuffer[i].pIonHandle1 = pIonHandle1;
                phREF->SharedBuffer[i].pIonHandle2 = pIonHandle2;
                phREF->SharedBuffer[i].cnt++;
                break;
            }
        }
    }

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "inc fd:%d cnt:%d", phREF->SharedBuffer[i].BufferFd, phREF->SharedBuffer[i].cnt);

    Exynos_OSAL_MutexUnlock(phREF->hMutex);

    if (i >=  MAX_BUFFER_REF) {
        ret = OMX_ErrorUndefined;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OSAL_RefANB_Decrease(OMX_HANDLETYPE hREF, OMX_U32 BufferFd)
{
    int i;
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    EXYNOS_OMX_REF_HANDLE *phREF = (EXYNOS_OMX_REF_HANDLE *)hREF;
    gralloc_module_t* module = NULL;

    FunctionIn();

    if ((phREF == NULL) || (BufferFd < 0)) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    module = (gralloc_module_t *)phREF->pGrallocModule;

    Exynos_OSAL_MutexLock(phREF->hMutex);

    for (i = 0; i < MAX_BUFFER_REF; i++) {
        if (phREF->SharedBuffer[i].BufferFd == BufferFd) {
            if (phREF->SharedBuffer[i].BufferFd > -1)
                ion_decRef(getIonFd(module), phREF->SharedBuffer[i].pIonHandle);
            if (phREF->SharedBuffer[i].BufferFd1 > -1)
                ion_decRef(getIonFd(module), phREF->SharedBuffer[i].pIonHandle1);
            if (phREF->SharedBuffer[i].BufferFd2 > -1)
                ion_decRef(getIonFd(module), phREF->SharedBuffer[i].pIonHandle2);
            phREF->SharedBuffer[i].cnt--;
            if (phREF->SharedBuffer[i].cnt == 0) {
                phREF->SharedBuffer[i].BufferFd    = -1;
                phREF->SharedBuffer[i].BufferFd1   = -1;
                phREF->SharedBuffer[i].BufferFd2   = -1;
                phREF->SharedBuffer[i].pIonHandle  = NULL;
                phREF->SharedBuffer[i].pIonHandle1 = NULL;
                phREF->SharedBuffer[i].pIonHandle2 = NULL;
            }
            break;
        }
    }
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "dec fd:%d cnt:%d", phREF->SharedBuffer[i].BufferFd, phREF->SharedBuffer[i].cnt);

    Exynos_OSAL_MutexUnlock(phREF->hMutex);

    if (i >=  MAX_BUFFER_REF) {
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE useAndroidNativeBuffer(
    EXYNOS_OMX_BASEPORT      *pExynosPort,
    OMX_BUFFERHEADERTYPE **ppBufferHdr,
    OMX_U32                nPortIndex,
    OMX_PTR                pAppPrivate,
    OMX_U32                nSizeBytes,
    OMX_U8                *pBuffer)
{
    OMX_ERRORTYPE         ret = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE *temp_bufferHeader = NULL;
    unsigned int          i = 0;
    OMX_U32               width, height;
    OMX_U32               stride;
    ExynosVideoPlane      planes[MAX_BUFFER_PLANE];

    FunctionIn();

    if (pExynosPort == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pExynosPort->portState != OMX_StateIdle) {
        ret = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }
    if (CHECK_PORT_TUNNELED(pExynosPort) && CHECK_PORT_BUFFER_SUPPLIER(pExynosPort)) {
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    temp_bufferHeader = (OMX_BUFFERHEADERTYPE *)Exynos_OSAL_Malloc(sizeof(OMX_BUFFERHEADERTYPE));
    if (temp_bufferHeader == NULL) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    Exynos_OSAL_Memset(temp_bufferHeader, 0, sizeof(OMX_BUFFERHEADERTYPE));

    for (i = 0; i < pExynosPort->portDefinition.nBufferCountActual; i++) {
        if (pExynosPort->bufferStateAllocate[i] == BUFFER_STATE_FREE) {
            pExynosPort->extendBufferHeader[i].OMXBufferHeader = temp_bufferHeader;
            pExynosPort->bufferStateAllocate[i] = (BUFFER_STATE_ASSIGNED | HEADER_STATE_ALLOCATED);
            INIT_SET_SIZE_VERSION(temp_bufferHeader, OMX_BUFFERHEADERTYPE);
            android_native_buffer_t *pANB = (android_native_buffer_t *) pBuffer;
            temp_bufferHeader->pBuffer = (OMX_U8 *)pANB->handle;
            temp_bufferHeader->nAllocLen      = nSizeBytes;
            temp_bufferHeader->pAppPrivate    = pAppPrivate;
            if (nPortIndex == INPUT_PORT_INDEX)
                temp_bufferHeader->nInputPortIndex = INPUT_PORT_INDEX;
            else
                temp_bufferHeader->nOutputPortIndex = OUTPUT_PORT_INDEX;

            width = pExynosPort->portDefinition.format.video.nFrameWidth;
            height = pExynosPort->portDefinition.format.video.nFrameHeight;
            Exynos_OSAL_LockANBHandle((OMX_U32)temp_bufferHeader->pBuffer, width, height,
                                pExynosPort->portDefinition.format.video.eColorFormat,
                                &stride, planes);
            pExynosPort->extendBufferHeader[i].buf_fd[0] = planes[0].fd;
            pExynosPort->extendBufferHeader[i].pYUVBuf[0] = planes[0].addr;
            pExynosPort->extendBufferHeader[i].buf_fd[1] = planes[1].fd;
            pExynosPort->extendBufferHeader[i].pYUVBuf[1] = planes[1].addr;
            pExynosPort->extendBufferHeader[i].buf_fd[2] = planes[2].fd;
            pExynosPort->extendBufferHeader[i].pYUVBuf[2] = planes[2].addr;
            Exynos_OSAL_UnlockANBHandle((OMX_U32)temp_bufferHeader->pBuffer);
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "useAndroidNativeBuffer: buf %d pYUVBuf[0]:0x%x (fd:%d), pYUVBuf[1]:0x%x (fd:%d)",
                            i, pExynosPort->extendBufferHeader[i].pYUVBuf[0], planes[0].fd,
                            pExynosPort->extendBufferHeader[i].pYUVBuf[1], planes[1].fd);

            pExynosPort->assignedBufferNum++;
            if (pExynosPort->assignedBufferNum == pExynosPort->portDefinition.nBufferCountActual) {
                pExynosPort->portDefinition.bPopulated = OMX_TRUE;
                /* Exynos_OSAL_MutexLock(pExynosComponent->compMutex); */
                Exynos_OSAL_SemaphorePost(pExynosPort->loadedResource);
                /* Exynos_OSAL_MutexUnlock(pExynosComponent->compMutex); */
            }
            *ppBufferHdr = temp_bufferHeader;
            ret = OMX_ErrorNone;

            goto EXIT;
        }
    }

    Exynos_OSAL_Free(temp_bufferHeader);
    ret = OMX_ErrorInsufficientResources;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OSAL_GetANBParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_INOUT OMX_PTR     ComponentParameterStructure)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pExynosComponent->currentState == OMX_StateInvalid ) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    if (ComponentParameterStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    switch (nIndex) {
    case OMX_IndexParamGetAndroidNativeBuffer:
    {
        GetAndroidNativeBufferUsageParams *pANBParams = (GetAndroidNativeBufferUsageParams *) ComponentParameterStructure;
        OMX_U32 portIndex = pANBParams->nPortIndex;

        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "%s: OMX_IndexParamGetAndroidNativeBuffer", __func__);

        ret = Exynos_OMX_Check_SizeVersion(pANBParams, sizeof(GetAndroidNativeBufferUsageParams));
        if (ret != OMX_ErrorNone) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: Exynos_OMX_Check_SizeVersion(GetAndroidNativeBufferUsageParams) is failed", __func__);
            goto EXIT;
        }

        if (portIndex >= pExynosComponent->portParam.nPorts) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        /* NOTE: OMX_IndexParamGetAndroidNativeBuffer returns original 'nUsage' without any
         * modifications since currently not defined what the 'nUsage' is for.
         */
        pANBParams->nUsage |= (GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_EXTERNAL_DISP);
    }
        break;

    default:
    {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: Unsupported index (%d)", __func__, nIndex);
        ret = OMX_ErrorUnsupportedIndex;
        goto EXIT;
    }
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OSAL_SetANBParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        ComponentParameterStructure)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pExynosComponent->currentState == OMX_StateInvalid ) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    if (ComponentParameterStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    switch (nIndex) {
    case OMX_IndexParamEnableAndroidBuffers:
    {
        EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
        EnableAndroidNativeBuffersParams *pANBParams = (EnableAndroidNativeBuffersParams *) ComponentParameterStructure;
        OMX_U32 portIndex = pANBParams->nPortIndex;
        EXYNOS_OMX_BASEPORT *pExynosPort = NULL;

        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "%s: OMX_IndexParamEnableAndroidNativeBuffers", __func__);

        ret = Exynos_OMX_Check_SizeVersion(pANBParams, sizeof(EnableAndroidNativeBuffersParams));
        if (ret != OMX_ErrorNone) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: Exynos_OMX_Check_SizeVersion(EnableAndroidNativeBuffersParams) is failed", __func__);
            goto EXIT;
        }

        if (portIndex >= pExynosComponent->portParam.nPorts) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pExynosPort = &pExynosComponent->pExynosPort[portIndex];
        if (CHECK_PORT_TUNNELED(pExynosPort) && CHECK_PORT_BUFFER_SUPPLIER(pExynosPort)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        /* ANB and DPB Buffer Sharing */
        if (pExynosPort->bStoreMetaData != OMX_TRUE)
            pExynosPort->bIsANBEnabled = pANBParams->enable;
        if ((portIndex == OUTPUT_PORT_INDEX) &&
            (pExynosPort->bIsANBEnabled == OMX_TRUE) &&
            ((pExynosPort->bufferProcessType & BUFFER_ANBSHARE) == BUFFER_ANBSHARE)) {
            pExynosPort->bufferProcessType = BUFFER_SHARE;
            pExynosPort->portDefinition.format.video.eColorFormat = (OMX_COLOR_FORMATTYPE)OMX_SEC_COLOR_FormatNV12Tiled;
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "OMX_IndexParamEnableAndroidBuffers & bufferProcessType change to BUFFER_SHARE");
        } else if ((portIndex == OUTPUT_PORT_INDEX) &&
            (pExynosPort->bStoreMetaData == OMX_FALSE && pExynosPort->bIsANBEnabled == OMX_FALSE) &&
            pExynosPort->bufferProcessType == BUFFER_SHARE) {
            pExynosPort->bufferProcessType = (EXYNOS_OMX_BUFFERPROCESS_TYPE)(BUFFER_COPY | BUFFER_ANBSHARE);
            pExynosPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatYUV420Planar;
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "No OMX_IndexParamEnableAndroidBuffers => reset bufferProcessType");
        }
    }
        break;

    case OMX_IndexParamUseAndroidNativeBuffer:
    {
        EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
        UseAndroidNativeBufferParams *pANBParams = (UseAndroidNativeBufferParams *) ComponentParameterStructure;
        OMX_U32 portIndex = pANBParams->nPortIndex;
        EXYNOS_OMX_BASEPORT *pExynosPort = NULL;
        android_native_buffer_t *pANB;
        OMX_U32 nSizeBytes;

        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "%s: OMX_IndexParamUseAndroidNativeBuffer, portIndex: %d", __func__, portIndex);

        ret = Exynos_OMX_Check_SizeVersion(pANBParams, sizeof(UseAndroidNativeBufferParams));
        if (ret != OMX_ErrorNone) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: Exynos_OMX_Check_SizeVersion(UseAndroidNativeBufferParams) is failed", __func__);
            goto EXIT;
        }

        if (portIndex >= pExynosComponent->portParam.nPorts) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pExynosPort = &pExynosComponent->pExynosPort[portIndex];
        if (CHECK_PORT_TUNNELED(pExynosPort) && CHECK_PORT_BUFFER_SUPPLIER(pExynosPort)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        if (pExynosPort->portState != OMX_StateIdle) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: Port state should be IDLE", __func__);
            ret = OMX_ErrorIncorrectStateOperation;
            goto EXIT;
        }

        pANB = pANBParams->nativeBuffer.get();

        /* MALI alignment restriction */
        nSizeBytes = ALIGN(pANB->width, 16) * ALIGN(pANB->height, 16);
        nSizeBytes += ALIGN(pANB->width / 2, 16) * ALIGN(pANB->height / 2, 16) * 2;

        ret = useAndroidNativeBuffer(pExynosPort,
                                     pANBParams->bufferHeader,
                                     pANBParams->nPortIndex,
                                     pANBParams->pAppPrivate,
                                     nSizeBytes,
                                     (OMX_U8 *) pANB);
        if (ret != OMX_ErrorNone) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: useAndroidNativeBuffer is failed: err=0x%x", __func__,ret);
            goto EXIT;
        }
    }
        break;

    case OMX_IndexParamStoreMetaDataBuffer:
    {
        StoreMetaDataInBuffersParams *pANBParams = (StoreMetaDataInBuffersParams *) ComponentParameterStructure;
        OMX_U32 portIndex = pANBParams->nPortIndex;
        EXYNOS_OMX_BASEPORT *pExynosPort = NULL;

        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "%s: OMX_IndexParamStoreMetaDataBuffer", __func__);

        ret = Exynos_OMX_Check_SizeVersion(pANBParams, sizeof(StoreMetaDataInBuffersParams));
        if (ret != OMX_ErrorNone) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: Exynos_OMX_Check_SizeVersion(StoreMetaDataInBuffersParams) is failed", __func__);
            goto EXIT;
        }

        if (portIndex >= pExynosComponent->portParam.nPorts) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pExynosPort = &pExynosComponent->pExynosPort[portIndex];
        if (CHECK_PORT_TUNNELED(pExynosPort) && CHECK_PORT_BUFFER_SUPPLIER(pExynosPort)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pExynosPort->bStoreMetaData = pANBParams->bStoreMetaData;
        if (pExynosComponent->codecType == HW_VIDEO_ENC_CODEC) {
            EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;;
        } else if (pExynosComponent->codecType == HW_VIDEO_DEC_CODEC) {
            EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;;
            if ((portIndex == OUTPUT_PORT_INDEX) &&
                (pExynosPort->bStoreMetaData == OMX_TRUE) &&
                ((pExynosPort->bufferProcessType & BUFFER_ANBSHARE) == BUFFER_ANBSHARE)) {
                pExynosPort->bufferProcessType = BUFFER_SHARE;
                pExynosPort->portDefinition.format.video.eColorFormat = (OMX_COLOR_FORMATTYPE)OMX_SEC_COLOR_FormatNV12Tiled;
                Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "OMX_IndexParamStoreMetaDataBuffer & bufferProcessType change to BUFFER_SHARE");
            } else if ((portIndex == OUTPUT_PORT_INDEX) &&
                (pExynosPort->bStoreMetaData == OMX_FALSE && pExynosPort->bIsANBEnabled == OMX_FALSE) &&
                pExynosPort->bufferProcessType == BUFFER_SHARE) {
                pExynosPort->bufferProcessType = (EXYNOS_OMX_BUFFERPROCESS_TYPE)(BUFFER_COPY | BUFFER_ANBSHARE);
                pExynosPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatYUV420Planar;
                Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "No OMX_IndexParamStoreMetaDataBuffer => reset bufferProcessType");
            }

        }
    }
        break;

    default:
    {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: Unsupported index (%d)", __func__, nIndex);
        ret = OMX_ErrorUnsupportedIndex;
        goto EXIT;
    }
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OSAL_GetInfoFromMetaData(OMX_IN OMX_BYTE pBuffer,
                                           OMX_OUT OMX_PTR *ppBuf)
{
    OMX_ERRORTYPE      ret = OMX_ErrorNone;
    MetadataBufferType type;

    FunctionIn();

/*
 * meta data contains the following data format.
 * payload depends on the MetadataBufferType
 * --------------------------------------------------------------
 * | MetadataBufferType                         |          payload                           |
 * --------------------------------------------------------------
 *
 * If MetadataBufferType is kMetadataBufferTypeCameraSource, then
 * --------------------------------------------------------------
 * | kMetadataBufferTypeCameraSource  | physical addr. of Y |physical addr. of CbCr |
 * --------------------------------------------------------------
 *
 * If MetadataBufferType is kMetadataBufferTypeGrallocSource, then
 * --------------------------------------------------------------
 * | kMetadataBufferTypeGrallocSource    | buffer_handle_t |
 * --------------------------------------------------------------
 */

    /* MetadataBufferType */
    Exynos_OSAL_Memcpy(&type, (MetadataBufferType *)pBuffer, sizeof(MetadataBufferType));

    if (type == kMetadataBufferTypeCameraSource) {
        void *pAddress = NULL;

        /* Address. of Y */
        Exynos_OSAL_Memcpy(&pAddress, pBuffer + sizeof(MetadataBufferType), sizeof(void *));
        ppBuf[0] = (void *)pAddress;

        /* Address. of CbCr */
        Exynos_OSAL_Memcpy(&pAddress, pBuffer + sizeof(MetadataBufferType) + sizeof(void *), sizeof(void *));
        ppBuf[1] = (void *)pAddress;

    } else if (type == kMetadataBufferTypeGrallocSource) {
        buffer_handle_t    pBufHandle;

        /* buffer_handle_t */
        Exynos_OSAL_Memcpy(&pBufHandle, pBuffer + sizeof(MetadataBufferType), sizeof(buffer_handle_t));
        ppBuf[0] = (OMX_PTR)pBufHandle;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OSAL_SetPrependSPSPPSToIDR(
    OMX_PTR pComponentParameterStructure,
    OMX_PTR pbPrependSpsPpsToIdr)
{
    OMX_ERRORTYPE                    ret        = OMX_ErrorNone;
    PrependSPSPPSToIDRFramesParams  *pANBParams = (PrependSPSPPSToIDRFramesParams *)pComponentParameterStructure;

    ret = Exynos_OMX_Check_SizeVersion(pANBParams, sizeof(PrependSPSPPSToIDRFramesParams));
    if (ret != OMX_ErrorNone) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: Exynos_OMX_Check_SizeVersion(PrependSPSPPSToIDRFrames) is failed", __func__);
        goto EXIT;
    }

    (*((OMX_BOOL *)pbPrependSpsPpsToIdr)) = pANBParams->bEnable;

EXIT:
    return ret;
}

OMX_COLOR_FORMATTYPE Exynos_OSAL_Hal2OMXPixelFormat(
    unsigned int hal_format)
{
    OMX_COLOR_FORMATTYPE omx_format;
    switch (hal_format) {
    case HAL_PIXEL_FORMAT_YCbCr_422_I:
        omx_format = OMX_COLOR_FormatYCbYCr;
        break;
    case HAL_PIXEL_FORMAT_YCbCr_420_P:
        omx_format = OMX_COLOR_FormatYUV420Planar;
        break;
    case HAL_PIXEL_FORMAT_YCbCr_420_SP:
        omx_format = OMX_COLOR_FormatYUV420SemiPlanar;
        break;
    case HAL_PIXEL_FORMAT_CUSTOM_YCbCr_420_SP_TILED:
        omx_format = (OMX_COLOR_FORMATTYPE)OMX_SEC_COLOR_FormatNV12TPhysicalAddress;
        break;
    case HAL_PIXEL_FORMAT_YCbCr_420_SP_TILED:
        omx_format = (OMX_COLOR_FORMATTYPE)OMX_SEC_COLOR_FormatNV12Tiled;
        break;
    case HAL_PIXEL_FORMAT_BGRA_8888:
    case HAL_PIXEL_FORMAT_CUSTOM_ARGB_8888:
        omx_format = OMX_COLOR_Format32bitARGB8888;
        break;
    default:
        omx_format = OMX_COLOR_FormatYUV420Planar;
        break;
    }
    return omx_format;
}

unsigned int Exynos_OSAL_OMX2HalPixelFormat(
    OMX_COLOR_FORMATTYPE omx_format)
{
    unsigned int hal_format;
    switch (omx_format) {
    case OMX_COLOR_FormatYCbYCr:
        hal_format = HAL_PIXEL_FORMAT_YCbCr_422_I;
        break;
    case OMX_COLOR_FormatYUV420Planar:
        hal_format = HAL_PIXEL_FORMAT_YCbCr_420_P;
        break;
    case OMX_COLOR_FormatYUV420SemiPlanar:
        hal_format = HAL_PIXEL_FORMAT_YCbCr_420_SP;
        break;
    case OMX_SEC_COLOR_FormatNV12TPhysicalAddress:
        hal_format = HAL_PIXEL_FORMAT_CUSTOM_YCbCr_420_SP_TILED;
        break;
    case OMX_SEC_COLOR_FormatNV12Tiled:
        hal_format = HAL_PIXEL_FORMAT_YCbCr_420_SP_TILED;
        break;
    case OMX_COLOR_Format32bitARGB8888:
        hal_format = HAL_PIXEL_FORMAT_CUSTOM_ARGB_8888;
        break;
    default:
        hal_format = HAL_PIXEL_FORMAT_YCbCr_420_P;
        break;
    }
    return hal_format;
}


#ifdef __cplusplus
}
#endif
