/*
 * Copyright (C) 2012 Intel Corporation.  All rights reserved.
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
 *
 */


#include <OMX_Component.h>
#include "isv_omxcomponent.h"
#include <media/hardware/HardwareAPI.h>
#include "isv_profile.h"
#include <OMX_IndexExt.h>
#include <hal_public.h>

//#define LOG_NDEBUG 0
#undef LOG_TAG
#define LOG_TAG "isv-omxil"

using namespace android;

/**********************************************************************************
 * component methods & helpers
 */
#define GET_ISVOMX_COMPONENT(hComponent)                                    \
    ISVComponent *pComp = static_cast<ISVComponent*>                        \
        ((static_cast<OMX_COMPONENTTYPE*>(hComponent))->pComponentPrivate); \
    if (!pComp)                                                             \
        return OMX_ErrorBadParameter;

Vector<ISVComponent*> ISVComponent::g_isv_components;

#ifndef TARGET_VPP_USE_GEN
//global, static
sp<ISVProcessor> ISVComponent::mProcThread = NULL;
#endif

//global, static
pthread_mutex_t ISVComponent::ProcThreadInstanceLock = PTHREAD_MUTEX_INITIALIZER;

ISVComponent::ISVComponent(
        OMX_PTR pAppData)
    :   mComponent(NULL),
        mpCallBacks(NULL),
        mCore(NULL),
        mpISVCallBacks(NULL),
        mISVBufferManager(NULL),
        mThreadRunning(false),
        mProcThreadObserver(NULL),
        mNumISVBuffers(MIN_ISV_BUFFER_NUM),
        mNumDecoderBuffers(0),
        mNumDecoderBuffersBak(0),
        mWidth(0),
        mHeight(0),
        mUseAndroidNativeBufferIndex(0),
        mStoreMetaDataInBuffersIndex(0),
        mHackFormat(0),
        mUseAndroidNativeBuffer(false),
        mUseAndroidNativeBuffer2(false),
        mVPPEnabled(false),
        mVPPFlushing(false),
        mOutputCropChanged(false),
        mInitialized(false),
#ifdef TARGET_VPP_USE_GEN
        mProcThread(NULL),
#endif
        mOwnProcessor(false)
{
    memset(&mBaseComponent, 0, sizeof(OMX_COMPONENTTYPE));
    /* handle initialization */
    SetTypeHeader(&mBaseComponent, sizeof(mBaseComponent));
    mBaseComponent.pApplicationPrivate = pAppData;
    mBaseComponent.pComponentPrivate = static_cast<OMX_PTR>(this);

    /* connect handle's functions */
    mBaseComponent.GetComponentVersion = NULL;
    mBaseComponent.SendCommand = SendCommand;
    mBaseComponent.GetParameter = GetParameter;
    mBaseComponent.SetParameter = SetParameter;
    mBaseComponent.GetConfig = GetConfig;
    mBaseComponent.SetConfig = SetConfig;
    mBaseComponent.GetExtensionIndex = GetExtensionIndex;
    mBaseComponent.GetState = GetState;
    mBaseComponent.ComponentTunnelRequest = NULL;
    mBaseComponent.UseBuffer = UseBuffer;
    mBaseComponent.AllocateBuffer = AllocateBuffer;
    mBaseComponent.FreeBuffer = FreeBuffer;
    mBaseComponent.EmptyThisBuffer = EmptyThisBuffer;
    mBaseComponent.FillThisBuffer = FillThisBuffer;
    mBaseComponent.SetCallbacks = SetCallbacks;
    mBaseComponent.ComponentDeInit = NULL;
    mBaseComponent.UseEGLImage = NULL;
    mBaseComponent.ComponentRoleEnum = ComponentRoleEnum;
    g_isv_components.push_back(static_cast<ISVComponent*>(this));

    mVPPOn = ISVProfile::isFRCOn() || ISVProfile::isVPPOn();
    ALOGD_IF(ISV_COMPONENT_DEBUG, "%s: mVPPOn %d", __func__, mVPPOn);

    if (mISVBufferManager == NULL) {
        mISVBufferManager = new ISVBufferManager();
    }

}

ISVComponent::~ISVComponent()
{
    ALOGD_IF(ISV_COMPONENT_DEBUG, "%s", __func__);
    if (mpISVCallBacks) {
        free(mpISVCallBacks);
        mpISVCallBacks = NULL;
    }

    for (OMX_U32 i = 0; i < g_isv_components.size(); i++) {
        if (g_isv_components.itemAt(i) == static_cast<ISVComponent*>(this)) {
            g_isv_components.removeAt(i);
        }
    }

    memset(&mBaseComponent, 0, sizeof(OMX_COMPONENTTYPE));
    deinit();
    mVPPOn = false;
    mISVBufferManager = NULL;
}

status_t ISVComponent::init(int32_t width, int32_t height)
{
    if (mInitialized)
        return STATUS_OK;

    bool frcOn = false;
    if (mProcThreadObserver == NULL)
        mProcThreadObserver = new ISVProcThreadObserver(&mBaseComponent, mComponent, mpCallBacks, mISVBufferManager);

    pthread_mutex_lock(&ProcThreadInstanceLock);
    if (mProcThread == NULL) {
        mProcThread = new ISVProcessor(false, mISVBufferManager, mProcThreadObserver, width, height);
        mOwnProcessor = true;
        mProcThread->start();
    }
#ifndef TARGET_VPP_USE_GEN
    else {
        mVPPEnabled = false;
        mOwnProcessor = false;
        ALOGI("%s: failed to alloc isv processor", __func__);
        pthread_mutex_unlock(&ProcThreadInstanceLock);
        return STATUS_ERROR;
    }
#endif
    pthread_mutex_unlock(&ProcThreadInstanceLock);

    mInitialized = true;
    return STATUS_OK;
}

void ISVComponent::deinit()
{
    pthread_mutex_lock(&ProcThreadInstanceLock);
    if (mOwnProcessor) {
        if (mProcThread != NULL) {
            mProcThread->stop();
            mProcThread = NULL;
            ALOGD_IF(ISV_COMPONENT_DEBUG, "%s: delete ISV processor ", __func__);
        }
    }
    pthread_mutex_unlock(&ProcThreadInstanceLock);

    mProcThreadObserver = NULL;

    mInitialized = false;
}

OMX_CALLBACKTYPE* ISVComponent::getCallBacks(OMX_CALLBACKTYPE* pCallBacks)
{
    //reset component callback functions
    mpCallBacks = pCallBacks;
    if (mpISVCallBacks) {
        free(mpISVCallBacks);
        mpISVCallBacks = NULL;
    }

    mpISVCallBacks = (OMX_CALLBACKTYPE *)calloc(1, sizeof(OMX_CALLBACKTYPE));
    if (!mpISVCallBacks) {
        ALOGE("%s: failed to alloc isv callbacks", __func__);
        return NULL;
    }
    mpISVCallBacks->EventHandler = EventHandler;
    mpISVCallBacks->EmptyBufferDone = pCallBacks->EmptyBufferDone;
    mpISVCallBacks->FillBufferDone = FillBufferDone;
    return mpISVCallBacks;
}

OMX_ERRORTYPE ISVComponent::SendCommand(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_COMMANDTYPE Cmd,
    OMX_IN  OMX_U32 nParam1,
    OMX_IN  OMX_PTR pCmdData)
{
    GET_ISVOMX_COMPONENT(hComponent);

    return pComp->ISV_SendCommand(Cmd, nParam1, pCmdData);
}

OMX_ERRORTYPE ISVComponent::ISV_SendCommand(
    OMX_IN  OMX_COMMANDTYPE Cmd,
    OMX_IN  OMX_U32 nParam1,
    OMX_IN  OMX_PTR pCmdData)
{
    ALOGD_IF(ISV_COMPONENT_DEBUG, "%s: Cmd index 0x%08x, nParam1 %d", __func__, Cmd, nParam1);

    if (mVPPEnabled && mVPPOn) {
        if ((Cmd == OMX_CommandFlush && (nParam1 == kPortIndexOutput || nParam1 == OMX_ALL))
                || (Cmd == OMX_CommandStateSet && nParam1 == OMX_StateIdle)
                || (Cmd == OMX_CommandPortDisable && nParam1 == 1)) {
            ALOGD_IF(ISV_COMPONENT_DEBUG, "%s: receive flush command, notify vpp thread to flush(Seek begin)", __func__);
            mVPPFlushing = true;
            mProcThread->notifyFlush();
        }
    }

    return OMX_SendCommand(mComponent, Cmd, nParam1, pCmdData);
}

OMX_ERRORTYPE ISVComponent::GetParameter(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nParamIndex,
    OMX_INOUT OMX_PTR pComponentParameterStructure)
{
    GET_ISVOMX_COMPONENT(hComponent);

    return pComp->ISV_GetParameter(nParamIndex, pComponentParameterStructure);
}

OMX_ERRORTYPE ISVComponent::ISV_GetParameter(
    OMX_IN  OMX_INDEXTYPE nParamIndex,
    OMX_INOUT OMX_PTR pComponentParameterStructure)
{
    ALOGD_IF(ISV_COMPONENT_DEBUG, "%s: nIndex 0x%08x", __func__, nParamIndex);

    OMX_ERRORTYPE err = OMX_GetParameter(mComponent, nParamIndex, pComponentParameterStructure);

    if (err == OMX_ErrorNone && mVPPEnabled && mVPPOn) {
        OMX_PARAM_PORTDEFINITIONTYPE *def =
            static_cast<OMX_PARAM_PORTDEFINITIONTYPE*>(pComponentParameterStructure);

        if (nParamIndex == OMX_IndexParamPortDefinition
                && def->nPortIndex == kPortIndexOutput) {
            ALOGD_IF(ISV_COMPONENT_DEBUG, "%s: orignal bufferCountActual %d, bufferCountMin %d",  __func__, def->nBufferCountActual, def->nBufferCountMin);
#ifndef TARGET_VPP_USE_GEN
            //FIXME: THIS IS A HACK!! Request NV12 buffer for YV12 format
            //because VSP only support NV12 output
            OMX_VIDEO_PORTDEFINITIONTYPE *video_def = &def->format.video;
            if (video_def->eColorFormat == VA_FOURCC_YV12) {
                //FIXME workaround Disable ISV for YV12 input
                mVPPEnabled = false;
                ALOGI("%s: Disable ISV for YV12 input. mVPPEnabled %d", __func__, mVPPEnabled);
            } else {
                //FIXME workaround avc low resolution playback
                def->nBufferCountActual += mNumISVBuffers + 9;
                def->nBufferCountMin += mNumISVBuffers + 9;
            }
#endif
        }
    }

    return err;
}

OMX_ERRORTYPE ISVComponent::SetParameter(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nIndex,
    OMX_IN  OMX_PTR pComponentParameterStructure)
{
    GET_ISVOMX_COMPONENT(hComponent);
 
    return pComp->ISV_SetParameter(nIndex, pComponentParameterStructure);
}

OMX_ERRORTYPE ISVComponent::ISV_SetParameter(
    OMX_IN  OMX_INDEXTYPE nIndex,
    OMX_IN  OMX_PTR pComponentParameterStructure)
{
    ALOGD_IF(ISV_COMPONENT_DEBUG, "%s: nIndex 0x%08x", __func__, nIndex);

    if (nIndex == static_cast<OMX_INDEXTYPE>(OMX_IndexExtSetISVMode)) {
        ISV_MODE* def = static_cast<ISV_MODE*>(pComponentParameterStructure);

        if (*def == ISV_AUTO) {
            mVPPEnabled = true;
            ALOGD_IF(ISV_COMPONENT_DEBUG, "%s: mVPPEnabled -->true", __func__);
#ifndef TARGET_VPP_USE_GEN
            if (mVPPOn) {
                uint32_t number = MIN_INPUT_NUM + MIN_OUTPUT_NUM;
                OMX_INDEXTYPE index;
                status_t error =
                    OMX_GetExtensionIndex(
                            mComponent,
                            (OMX_STRING)"OMX.Intel.index.vppBufferNum",
                            &index);
                if (error == OK) {
                    error = OMX_SetParameter(mComponent, index, (OMX_PTR)&number);
                } else {
                    // ingore this error
                    ALOGW("Get vpp number index failed");
                }
            }
#endif
        } else if (*def == ISV_DISABLE)
            mVPPEnabled = false;
        return OMX_ErrorNone;
    }

    OMX_ERRORTYPE err = OMX_SetParameter(mComponent, nIndex, pComponentParameterStructure);
    if (err == OMX_ErrorNone && mVPPEnabled && mVPPOn) {
        if (nIndex == OMX_IndexParamPortDefinition) {
            OMX_PARAM_PORTDEFINITIONTYPE *def =
                static_cast<OMX_PARAM_PORTDEFINITIONTYPE*>(pComponentParameterStructure);

            if (def->nPortIndex == kPortIndexOutput) {
                //set the buffer count we should fill to decoder before feed buffer to VPP
                mNumDecoderBuffersBak = mNumDecoderBuffers = def->nBufferCountActual - MIN_OUTPUT_NUM - UNDEQUEUED_NUM;
                OMX_VIDEO_PORTDEFINITIONTYPE *video_def = &def->format.video;

                //FIXME: init itself here
                if (mWidth != video_def->nFrameWidth
                        || mHeight != video_def->nFrameHeight) {
                    deinit();
                    if (STATUS_OK == init(video_def->nFrameWidth, video_def->nFrameHeight)) {
                        mWidth = video_def->nFrameWidth;
                        mHeight = video_def->nFrameHeight;
                    }
                }
                ALOGD_IF(ISV_COMPONENT_DEBUG, "%s: def->nBufferCountActual %d, mNumDecoderBuffersBak %d", __func__,
                        def->nBufferCountActual, mNumDecoderBuffersBak);
                if (mISVBufferManager != NULL && OK != mISVBufferManager->setBufferCount(def->nBufferCountActual)) {
                    ALOGE("%s: failed to set ISV buffer count, set VPPEnabled -->false", __func__);
                    mVPPEnabled = false;
                }
                ALOGD_IF(ISV_COMPONENT_DEBUG, "%s: video frame width %d, height %d",  __func__, 
                        video_def->nFrameWidth, video_def->nFrameHeight);
            }

            if (def->nPortIndex == kPortIndexInput) {
                OMX_VIDEO_PORTDEFINITIONTYPE *video_def = &def->format.video;

                if (mProcThread != NULL)
                    mProcThread->configFRC(video_def->xFramerate);
            }
        }

        if (mUseAndroidNativeBuffer
                && nIndex == static_cast<OMX_INDEXTYPE>(mUseAndroidNativeBufferIndex)) {
            UseAndroidNativeBufferParams *def =
                static_cast<UseAndroidNativeBufferParams*>(pComponentParameterStructure);

            if (mISVBufferManager != NULL && OK != mISVBufferManager->useBuffer(def->nativeBuffer)) {
                    ALOGE("%s: failed to register graphic buffers to ISV, set mVPPEnabled -->false", __func__);
                    mVPPEnabled = false;
            }
        }

        if (nIndex == static_cast<OMX_INDEXTYPE>(mStoreMetaDataInBuffersIndex)) {
            StoreMetaDataInBuffersParams *params = static_cast<StoreMetaDataInBuffersParams*>(pComponentParameterStructure);
            if (params->nPortIndex == kPortIndexOutput) {
                if (mISVBufferManager != NULL) {
                    bool bMetaDataMode = params->bStoreMetaData == OMX_TRUE;
                    mISVBufferManager->setMetaDataMode(bMetaDataMode);
                } else {
                    ALOGE("%s: falied to set Meta Data Mode ", __func__);
                }
            }
            ALOGD_IF(ISV_COMPONENT_DEBUG, "%s: receive ISVStoreMetaDataInBuffers mISVWorkMode %d", __func__, (params->bStoreMetaData == OMX_TRUE));
        }
    }
    return err;
}

OMX_ERRORTYPE ISVComponent::GetConfig(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nIndex,
    OMX_INOUT OMX_PTR pComponentConfigStructure)
{
    GET_ISVOMX_COMPONENT(hComponent);

    return pComp->ISV_GetConfig(nIndex, pComponentConfigStructure);
}

OMX_ERRORTYPE ISVComponent::ISV_GetConfig(
    OMX_IN  OMX_INDEXTYPE nIndex,
    OMX_INOUT OMX_PTR pComponentConfigStructure)
{
    ALOGD_IF(ISV_COMPONENT_DEBUG, "%s: nIndex 0x%08x", __func__, nIndex);

    OMX_ERRORTYPE err = OMX_GetConfig(mComponent, nIndex, pComponentConfigStructure);
    if (err == OMX_ErrorNone && mVPPEnabled && mVPPOn) {
        if (nIndex == OMX_IndexConfigCommonOutputCrop) {
            OMX_CONFIG_RECTTYPE *rect = static_cast<OMX_CONFIG_RECTTYPE*>(pComponentConfigStructure);
            if (rect->nPortIndex == kPortIndexOutput &&
                    rect->nWidth < mWidth &&
                    rect->nHeight < mHeight) {
                mISVBufferManager->setBuffersFlag(ISVBuffer::ISV_BUFFER_NEED_CLEAR);
                ALOGD_IF(ISV_COMPONENT_DEBUG, "%s: mark all buffers need clear", __func__);
            }
        }
    }
    return err;
}

OMX_ERRORTYPE ISVComponent::SetConfig(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nIndex,
    OMX_IN  OMX_PTR pComponentConfigStructure)
{
    GET_ISVOMX_COMPONENT(hComponent);

    return pComp->ISV_SetConfig(nIndex, pComponentConfigStructure);
}

OMX_ERRORTYPE ISVComponent::ISV_SetConfig(
    OMX_IN  OMX_INDEXTYPE nIndex,
    OMX_IN  OMX_PTR pComponentConfigStructure)
{
    ALOGD_IF(ISV_COMPONENT_DEBUG, "%s: nIndex 0x%08x", __func__, nIndex);

    if (nIndex == static_cast<OMX_INDEXTYPE>(OMX_IndexConfigAutoFramerateConversion)) {
        OMX_CONFIG_BOOLEANTYPE *config = static_cast<OMX_CONFIG_BOOLEANTYPE*>(pComponentConfigStructure);
        if (config->bEnabled) {
            mVPPEnabled = true;
            ALOGI("%s: mVPPEnabled=true", __func__);
        } else {
            mVPPEnabled = false;
            ALOGI("%s: mVPPEnabled=false", __func__);
        }
        return OMX_ErrorNone;
    }

    return OMX_SetConfig(mComponent, nIndex, pComponentConfigStructure);
}

OMX_ERRORTYPE ISVComponent::GetExtensionIndex(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_STRING cParameterName,
    OMX_OUT OMX_INDEXTYPE* pIndexType)
{
    GET_ISVOMX_COMPONENT(hComponent);

    return pComp->ISV_GetExtensionIndex(cParameterName, pIndexType);
}

OMX_ERRORTYPE ISVComponent::ISV_GetExtensionIndex(
    OMX_IN  OMX_STRING cParameterName,
    OMX_OUT OMX_INDEXTYPE* pIndexType)
{
    ALOGD_IF(ISV_COMPONENT_DEBUG, "%s: cParameterName %s", __func__, cParameterName);
    if(!strncmp(cParameterName, "OMX.intel.index.SetISVMode", strlen(cParameterName))) {
        *pIndexType = static_cast<OMX_INDEXTYPE>(OMX_IndexExtSetISVMode);
        return OMX_ErrorNone;
    }

    OMX_ERRORTYPE err = OMX_GetExtensionIndex(mComponent, cParameterName, pIndexType);

    if(err == OMX_ErrorNone &&
            !strncmp(cParameterName, "OMX.google.android.index.useAndroidNativeBuffer2", strlen(cParameterName)))
        mUseAndroidNativeBuffer2 = true;

    if(err == OMX_ErrorNone &&
            !strncmp(cParameterName, "OMX.google.android.index.useAndroidNativeBuffer", strlen(cParameterName))) {
        mUseAndroidNativeBuffer = true;
        mUseAndroidNativeBufferIndex = static_cast<uint32_t>(*pIndexType);
    }

    if(err == OMX_ErrorNone &&
            !strncmp(cParameterName, "OMX.google.android.index.storeMetaDataInBuffers", strlen(cParameterName))) {
        mStoreMetaDataInBuffersIndex = static_cast<uint32_t>(*pIndexType);
        ALOGD_IF(ISV_COMPONENT_DEBUG, "%s: storeMetaDataInBuffersIndex 0x%08x return %d", __func__, mStoreMetaDataInBuffersIndex, err);
    }
    ALOGD_IF(ISV_COMPONENT_DEBUG, "%s: cParameterName %s, nIndex 0x%08x", __func__,
            cParameterName, *pIndexType);
    return err;
}

OMX_ERRORTYPE ISVComponent::GetState(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_STATETYPE* pState)
{
    GET_ISVOMX_COMPONENT(hComponent);

    return pComp->ISV_GetState(pState);
}

OMX_ERRORTYPE ISVComponent::ISV_GetState(
    OMX_OUT OMX_STATETYPE* pState)
{
    ALOGD_IF(ISV_COMPONENT_DEBUG, "%s", __func__);

    return OMX_GetState(mComponent, pState);
}

OMX_ERRORTYPE ISVComponent::UseBuffer(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE **ppBufferHdr,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate,
    OMX_IN OMX_U32 nSizeBytes,
    OMX_IN OMX_U8 *pBuffer)
{
    GET_ISVOMX_COMPONENT(hComponent);

    return pComp->ISV_UseBuffer(ppBufferHdr, nPortIndex,
                                 pAppPrivate, nSizeBytes, pBuffer);
}

OMX_ERRORTYPE ISVComponent::ISV_UseBuffer(
    OMX_INOUT OMX_BUFFERHEADERTYPE **ppBufferHdr,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate,
    OMX_IN OMX_U32 nSizeBytes,
    OMX_IN OMX_U8 *pBuffer)
{
    ALOGD_IF(ISV_COMPONENT_DEBUG, "%s", __func__);

    OMX_ERRORTYPE err = OMX_UseBuffer(mComponent, ppBufferHdr, nPortIndex,
            pAppPrivate, nSizeBytes, pBuffer);
#ifndef USE_IVP
    if(err == OMX_ErrorNone
            && mVPPEnabled
            && mVPPOn
            && nPortIndex == kPortIndexOutput
            /*&& mUseAndroidNativeBuffer2*/) {
        if (mISVBufferManager != NULL) {
            if (OK != mISVBufferManager->useBuffer(reinterpret_cast<unsigned long>(pBuffer))) {
                ALOGE("%s: failed to register graphic buffers to ISV, set mVPPEnabled -->false", __func__);
                mVPPEnabled = false;
            } else
                ALOGD_IF(ISV_COMPONENT_DEBUG, "%s: mVPP useBuffer success. buffer handle %p", __func__, pBuffer);
        }
    }
#endif
    return err;
}

OMX_ERRORTYPE ISVComponent::AllocateBuffer(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE **ppBuffer,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate,
    OMX_IN OMX_U32 nSizeBytes)
{
    GET_ISVOMX_COMPONENT(hComponent);

    return pComp->ISV_AllocateBuffer(ppBuffer, nPortIndex,
                                      pAppPrivate, nSizeBytes);
}

OMX_ERRORTYPE ISVComponent::ISV_AllocateBuffer(
    OMX_INOUT OMX_BUFFERHEADERTYPE **ppBuffer,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate,
    OMX_IN OMX_U32 nSizeBytes)
{
    ALOGD_IF(ISV_COMPONENT_DEBUG, "%s", __func__);

    return OMX_AllocateBuffer(mComponent, ppBuffer, nPortIndex,
                                      pAppPrivate, nSizeBytes);
}

OMX_ERRORTYPE ISVComponent::FreeBuffer(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_U32 nPortIndex,
    OMX_IN  OMX_BUFFERHEADERTYPE *pBuffer)
{
    GET_ISVOMX_COMPONENT(hComponent);

    return pComp->ISV_FreeBuffer(nPortIndex, pBuffer);
}

OMX_ERRORTYPE ISVComponent::ISV_FreeBuffer(
    OMX_IN  OMX_U32 nPortIndex,
    OMX_IN  OMX_BUFFERHEADERTYPE *pBuffer)
{
    ALOGD_IF(ISV_COMPONENT_DEBUG, "%s: pBuffer %p", __func__, pBuffer);

    if(mVPPEnabled && mVPPOn
            && nPortIndex == kPortIndexOutput) {
        if (mISVBufferManager != NULL && OK != mISVBufferManager->freeBuffer(reinterpret_cast<unsigned long>(pBuffer->pBuffer)))
            ALOGW("%s: pBuffer %p has not been registered into ISV", __func__, pBuffer);
    }
    return OMX_FreeBuffer(mComponent, nPortIndex, pBuffer);
}

OMX_ERRORTYPE ISVComponent::EmptyThisBuffer(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)
{
    GET_ISVOMX_COMPONENT(hComponent);

    return pComp->ISV_EmptyThisBuffer(pBuffer);
}

OMX_ERRORTYPE ISVComponent::ISV_EmptyThisBuffer(
    OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)
{
    ALOGD_IF(ISV_COMPONENT_DEBUG, "%s: pBuffer %p", __func__, pBuffer);

    return OMX_EmptyThisBuffer(mComponent, pBuffer);
}

OMX_ERRORTYPE ISVComponent::FillThisBuffer(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_BUFFERHEADERTYPE *pBuffer)
{
    ALOGD_IF(ISV_COMPONENT_DEBUG, "%s: API entry.", __func__);
    GET_ISVOMX_COMPONENT(hComponent);

    return pComp->ISV_FillThisBuffer(pBuffer);
}

OMX_ERRORTYPE ISVComponent::ISV_FillThisBuffer(
    OMX_IN  OMX_BUFFERHEADERTYPE *pBuffer)
{
    if(!mVPPEnabled || !mVPPOn)
        return OMX_FillThisBuffer(mComponent, pBuffer);

    ISVBuffer* isvBuffer = NULL;

    if (mISVBufferManager != NULL) {
        isvBuffer = mISVBufferManager->mapBuffer(reinterpret_cast<unsigned long>(pBuffer->pBuffer));
        if (isvBuffer == NULL) {
            ALOGE("%s: failed to map ISVBuffer, set mVPPEnabled -->false", __func__);
            mVPPEnabled = false;
            return OMX_FillThisBuffer(mComponent, pBuffer);
        }

        if (OK != isvBuffer->initBufferInfo(mHackFormat)) {
            ALOGD_IF(ISV_COMPONENT_DEBUG, "%s: isvBuffer %p failed to initBufferInfo", __func__, isvBuffer);
            mVPPEnabled = false;
            return OMX_FillThisBuffer(mComponent, pBuffer);
        }
    }

    if (mNumDecoderBuffers > 0) {
        mNumDecoderBuffers--;
        ALOGD_IF(ISV_COMPONENT_DEBUG, "%s: fill pBuffer %p to the decoder, decoder still need extra %d buffers", __func__,
                pBuffer, mNumDecoderBuffers);

        if (isvBuffer != NULL)
            isvBuffer->clearIfNeed();

        return OMX_FillThisBuffer(mComponent, pBuffer);
    }
    mProcThread->addOutput(pBuffer);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE ISVComponent::FillBufferDone(
        OMX_OUT OMX_HANDLETYPE hComponent,
        OMX_OUT OMX_PTR pAppData,
        OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer)
{
    ALOGD_IF(ISV_COMPONENT_DEBUG, "%s: API entry. ISV component num %d, component handle %p on index 0", __func__,
            g_isv_components.size(),
            g_isv_components.itemAt(0));
    for (OMX_U32 i = 0; i < g_isv_components.size(); i++) {
        if (static_cast<OMX_HANDLETYPE>(g_isv_components.itemAt(i)->mComponent) == hComponent)
            return g_isv_components.itemAt(i)->ISV_FillBufferDone(hComponent, pAppData, pBuffer);
    }
    return OMX_ErrorUndefined;
}

OMX_ERRORTYPE ISVComponent::ISV_FillBufferDone(
        OMX_OUT OMX_HANDLETYPE __maybe_unused hComponent,
        OMX_OUT OMX_PTR pAppData,
        OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer)
{
    ALOGD_IF(ISV_COMPONENT_DEBUG, "%s: %p <== buffer_handle_t %p. mVPPEnabled %d, mVPPOn %d", __func__,
            pBuffer, pBuffer->pBuffer, mVPPEnabled, mVPPOn);
    if (!mpCallBacks) {
        ALOGE("%s: no call back functions were registered.", __func__);
        return OMX_ErrorUndefined;
    }

    if(!mVPPEnabled || !mVPPOn || mVPPFlushing || pBuffer->nFilledLen == 0) {
        ALOGD_IF(ISV_COMPONENT_DEBUG, "%s: FillBufferDone pBuffer %p, timeStamp %.2f ms", __func__, pBuffer, pBuffer->nTimeStamp/1E3);
        return mpCallBacks->FillBufferDone(&mBaseComponent, pAppData, pBuffer);
    }

    if (mOutputCropChanged && mISVBufferManager != NULL) {
        ISVBuffer* isvBuffer = mISVBufferManager->mapBuffer(reinterpret_cast<unsigned long>(pBuffer->pBuffer));
        if (isvBuffer != NULL)
            isvBuffer->setFlag(ISVBuffer::ISV_BUFFER_CROP_CHANGED);
        mOutputCropChanged = false;
    }

    mProcThread->addInput(pBuffer);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE ISVComponent::EventHandler(
        OMX_IN OMX_HANDLETYPE hComponent,
        OMX_IN OMX_PTR pAppData,
        OMX_IN OMX_EVENTTYPE eEvent,
        OMX_IN OMX_U32 nData1,
        OMX_IN OMX_U32 nData2,
        OMX_IN OMX_PTR pEventData)
{
    ALOGD_IF(ISV_COMPONENT_DEBUG, "%s: API entry. ISV component num %d, component handle %p on index 0", __func__,
            g_isv_components.size(),
            g_isv_components.itemAt(0));
    for (OMX_U32 i = 0; i < g_isv_components.size(); i++) {
        if (static_cast<OMX_HANDLETYPE>(g_isv_components.itemAt(i)->mComponent) == hComponent)
            return g_isv_components.itemAt(i)->ISV_EventHandler(hComponent, pAppData, eEvent, nData1, nData2, pEventData);
    }
    return OMX_ErrorUndefined;
}

OMX_ERRORTYPE ISVComponent::ISV_EventHandler(
        OMX_IN OMX_HANDLETYPE __maybe_unused hComponent,
        OMX_IN OMX_PTR pAppData,
        OMX_IN OMX_EVENTTYPE eEvent,
        OMX_IN OMX_U32 nData1,
        OMX_IN OMX_U32 nData2,
        OMX_IN OMX_PTR pEventData)
{
    if (!mpCallBacks) {
        ALOGE("%s: no call back functions were registered.", __func__);
        return OMX_ErrorUndefined;
    }

    if(!mVPPEnabled || !mVPPOn)
        return mpCallBacks->EventHandler(&mBaseComponent, pAppData, eEvent, nData1, nData2, pEventData);

    switch (eEvent) {
        case OMX_EventCmdComplete:
        {
            ALOGD_IF(ISV_COMPONENT_DEBUG, "%s: OMX_EventCmdComplete Cmd type 0x%08x, data2 %d", __func__,
                    nData1, nData2);
            if (((OMX_COMMANDTYPE)nData1 == OMX_CommandFlush && (nData2 == kPortIndexOutput || nData2 == OMX_ALL))
                || ((OMX_COMMANDTYPE)nData1 == OMX_CommandStateSet && nData2 == OMX_StateIdle)
                || ((OMX_COMMANDTYPE)nData1 == OMX_CommandPortDisable && nData2 == 1)) {
                mProcThread->waitFlushFinished();
                mVPPFlushing = false;
                mNumDecoderBuffers = mNumDecoderBuffersBak;
            }
            break;
        }

        case OMX_EventError:
        {
            //do we need do anything here?
            ALOGE("%s: ERROR(0x%08x, %d)", __func__, nData1, nData2);
            //mProcThread->flush();
            break;
        }

        case OMX_EventPortSettingsChanged:
        {
            if (nData1 == kPortIndexOutput && nData2 == OMX_IndexConfigCommonOutputCrop) {
                ALOGD_IF(ISV_COMPONENT_DEBUG, "%s: output crop changed", __func__);
                mOutputCropChanged = true;
                return OMX_ErrorNone;
            }
            break;
        }

        default:
        {
            ALOGD_IF(ISV_COMPONENT_DEBUG, "%s: EVENT(%d, %ld, %ld)", __func__, eEvent, nData1, nData2);
            break;
        }
    }
    return mpCallBacks->EventHandler(&mBaseComponent, pAppData, eEvent, nData1, nData2, pEventData);
}

OMX_ERRORTYPE ISVComponent::SetCallbacks(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_CALLBACKTYPE* pCallbacks,
    OMX_IN  OMX_PTR pAppData)
{
    GET_ISVOMX_COMPONENT(hComponent);

    return pComp->ISV_SetCallbacks(pCallbacks, pAppData);
}

OMX_ERRORTYPE ISVComponent::ISV_SetCallbacks(
    OMX_IN  OMX_CALLBACKTYPE* pCallbacks,
    OMX_IN  OMX_PTR pAppData)
{
    ALOGD_IF(ISV_COMPONENT_DEBUG, "%s", __func__);

    if (mVPPEnabled && mVPPOn) {
        if (mpISVCallBacks == NULL) {
            mpISVCallBacks = (OMX_CALLBACKTYPE *)calloc(1, sizeof(OMX_CALLBACKTYPE));
            if (!mpISVCallBacks) {
                ALOGE("%s: failed to alloc isv callbacks", __func__);
                return OMX_ErrorUndefined;
            }
        }
        mpISVCallBacks->EventHandler = EventHandler;
        mpISVCallBacks->EmptyBufferDone = pCallbacks->EmptyBufferDone;
        mpISVCallBacks->FillBufferDone = FillBufferDone;
        mpCallBacks = pCallbacks;
        return mComponent->SetCallbacks(mComponent, mpISVCallBacks, pAppData);
    }
    return mComponent->SetCallbacks(mComponent, pCallbacks, pAppData);
}

OMX_ERRORTYPE ISVComponent::ComponentRoleEnum(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_U8 *cRole,
    OMX_IN OMX_U32 nIndex)
{
    GET_ISVOMX_COMPONENT(hComponent);

    return pComp->ISV_ComponentRoleEnum(cRole, nIndex);
}

OMX_ERRORTYPE ISVComponent::ISV_ComponentRoleEnum(
    OMX_OUT OMX_U8 *cRole,
    OMX_IN OMX_U32 nIndex)
{
    ALOGD_IF(ISV_COMPONENT_DEBUG, "%s", __func__);

    return mComponent->ComponentRoleEnum(mComponent, cRole, nIndex);
}


void ISVComponent::SetTypeHeader(OMX_PTR type, OMX_U32 size)
{
    OMX_U32 *nsize;
    OMX_VERSIONTYPE *nversion;

    if (!type)
        return;

    nsize = (OMX_U32 *)type;
    nversion = (OMX_VERSIONTYPE *)((OMX_U8 *)type + sizeof(OMX_U32));

    *nsize = size;
    nversion->nVersion = OMX_SPEC_VERSION;
}


ISVProcThreadObserver::ISVProcThreadObserver(
        OMX_COMPONENTTYPE *pBaseComponent,
        OMX_COMPONENTTYPE *pComponent,
        OMX_CALLBACKTYPE *pCallBacks,
        sp<ISVBufferManager> bufferManager)
    :   mBaseComponent(pBaseComponent),
        mComponent(pComponent),
        mpCallBacks(pCallBacks),
        mISVBufferManager(bufferManager)
{
    ALOGV("VPPProcThreadObserver!");
}

ISVProcThreadObserver::~ISVProcThreadObserver()
{
    ALOGV("~VPPProcThreadObserver!");
    mBaseComponent = NULL;
    mComponent = NULL;
    mpCallBacks = NULL;
}

OMX_ERRORTYPE ISVProcThreadObserver::releaseBuffer(PORT_INDEX index, OMX_BUFFERHEADERTYPE* pBuffer, bool bFLush)
{
    if (!mBaseComponent || !mComponent || !mpCallBacks)
        return OMX_ErrorUndefined;

    OMX_ERRORTYPE err = OMX_ErrorNone;
    if (bFLush) {
        pBuffer->nFilledLen = 0;
        pBuffer->nOffset = 0;
        err = mpCallBacks->FillBufferDone(&mBaseComponent, mBaseComponent->pApplicationPrivate, pBuffer);
        ALOGD_IF(ISV_COMPONENT_DEBUG, "%s: flush pBuffer %p", __func__, pBuffer);
        return err;
    }

    if (index == kPortIndexInput) {
        pBuffer->nFilledLen = 0;
        pBuffer->nOffset = 0;
        pBuffer->nFlags = 0;

        if (mISVBufferManager != NULL) {
            ISVBuffer* isvBuffer = mISVBufferManager->mapBuffer(reinterpret_cast<unsigned long>(pBuffer->pBuffer));
            if (isvBuffer != NULL)
                isvBuffer->clearIfNeed();
        }

        err = OMX_FillThisBuffer(mComponent, pBuffer);
        ALOGD_IF(ISV_COMPONENT_DEBUG, "%s: FillBuffer pBuffer %p", __func__, pBuffer);
    } else {
        err = mpCallBacks->FillBufferDone(&mBaseComponent, mBaseComponent->pApplicationPrivate, pBuffer);
        ALOGD_IF(ISV_COMPONENT_DEBUG, "%s: FillBufferDone pBuffer %p, timeStamp %.2f ms", __func__, pBuffer, pBuffer->nTimeStamp/1E3);
    }

    return err;
}

OMX_ERRORTYPE ISVProcThreadObserver::reportOutputCrop()
{
    if (!mBaseComponent || !mComponent || !mpCallBacks)
        return OMX_ErrorUndefined;

    OMX_ERRORTYPE err = OMX_ErrorNone;
    err = mpCallBacks->EventHandler(&mBaseComponent, mBaseComponent->pApplicationPrivate,
                                    OMX_EventPortSettingsChanged,
                                    kPortIndexOutput, OMX_IndexConfigCommonOutputCrop, NULL);
    return err;
}
