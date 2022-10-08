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


#ifndef ISV_OMXCOMPONENT_H_

#define ISV_OMXCOMPONENT_H_

#include <utils/Mutex.h>
#include <utils/Vector.h>
#include <utils/RefBase.h>
#include "isv_omxcore.h"
#include "isv_processor.h"
#include "isv_profile.h"
#include "isv_worker.h"
#include "isv_bufmanager.h"

#define ISV_COMPONENT_DEBUG 0

#ifdef TARGET_VPP_USE_GEN
#define MIN_INPUT_NUM           (3)
#define MIN_OUTPUT_NUM          (3)
#else
#define MIN_INPUT_NUM           (4)    // forward reference frame number is 3 for merrifield/moorefield
#define MIN_OUTPUT_NUM          (10)   // 2.5FRC need hold 2 + 3 + 2 + 3= 10 buffers, without FRC we set to 6
#endif
#define MIN_ISV_BUFFER_NUM      ((MIN_OUTPUT_NUM) + (MIN_INPUT_NUM))
#define UNDEQUEUED_NUM          (4)   // display system hold 4 buffers

using namespace android;
class ISVComponent;

class ISVProcThreadObserver: public ISVProcessorObserver
{
public:
    ISVProcThreadObserver(OMX_COMPONENTTYPE *pBaseComponent, OMX_COMPONENTTYPE *pComponent, OMX_CALLBACKTYPE *pCallBacks, sp<ISVBufferManager> bufferManager);
    ~ISVProcThreadObserver();

    virtual OMX_ERRORTYPE releaseBuffer(PORT_INDEX index, OMX_BUFFERHEADERTYPE* pBuffer, bool flush);
    virtual OMX_ERRORTYPE reportOutputCrop();
private:
    OMX_COMPONENTTYPE *mBaseComponent;
    OMX_COMPONENTTYPE *mComponent;
    OMX_CALLBACKTYPE *mpCallBacks;
    sp<ISVBufferManager> mISVBufferManager;
};

class ISVComponent //: public RefBase
{
public:
    /*
     * construct & destruct
     */
    ISVComponent(OMX_PTR);
    ~ISVComponent();

    // replace component callbacks
    OMX_CALLBACKTYPE *getCallBacks(OMX_CALLBACKTYPE*);
    // pass down the real component&core
    void setComponent(OMX_COMPONENTTYPE *pComp, ISVOMXCore *pCore){mComponent = pComp; mCore = pCore;return;}
    // free the real component
    OMX_ERRORTYPE freeComponent(){return (*(mCore->mFreeHandle))(static_cast<OMX_HANDLETYPE>(mComponent));}
    // return ISV component handle
    OMX_COMPONENTTYPE *getBaseComponent(){return &mBaseComponent;}

    static Vector<ISVComponent*> g_isv_components;
private:
    /*
     * component methods & helpers
     */

    static OMX_ERRORTYPE SendCommand(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_COMMANDTYPE Cmd,
        OMX_IN  OMX_U32 nParam1,
        OMX_IN  OMX_PTR pCmdData);
    OMX_ERRORTYPE ISV_SendCommand(
        OMX_IN  OMX_COMMANDTYPE Cmd,
        OMX_IN  OMX_U32 nParam1,
        OMX_IN  OMX_PTR pCmdData);

    static OMX_ERRORTYPE GetParameter(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_INDEXTYPE nParamIndex,
        OMX_INOUT OMX_PTR pComponentParameterStructure);
    OMX_ERRORTYPE ISV_GetParameter(
        OMX_IN  OMX_INDEXTYPE nParamIndex,
        OMX_INOUT OMX_PTR pComponentParameterStructure);

    static OMX_ERRORTYPE SetParameter(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_INDEXTYPE nIndex,
        OMX_IN  OMX_PTR pComponentParameterStructure);
    OMX_ERRORTYPE ISV_SetParameter(
        OMX_IN  OMX_INDEXTYPE nIndex,
        OMX_IN  OMX_PTR pComponentParameterStructure);

    static OMX_ERRORTYPE GetConfig(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_INDEXTYPE nIndex,
        OMX_INOUT OMX_PTR pComponentConfigStructure);
    OMX_ERRORTYPE ISV_GetConfig(
        OMX_IN  OMX_INDEXTYPE nIndex,
        OMX_INOUT OMX_PTR pComponentConfigStructure);

    static OMX_ERRORTYPE SetConfig(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_INDEXTYPE nIndex,
        OMX_IN  OMX_PTR pComponentConfigStructure);
    OMX_ERRORTYPE ISV_SetConfig(
        OMX_IN  OMX_INDEXTYPE nIndex,
        OMX_IN  OMX_PTR pComponentConfigStructure);

    static OMX_ERRORTYPE GetExtensionIndex(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_STRING cParameterName,
        OMX_OUT OMX_INDEXTYPE* pIndexType);
    OMX_ERRORTYPE ISV_GetExtensionIndex(
        OMX_IN  OMX_STRING cParameterName,
        OMX_OUT OMX_INDEXTYPE* pIndexType);

    static OMX_ERRORTYPE GetState(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_OUT OMX_STATETYPE* pState);
    OMX_ERRORTYPE ISV_GetState(
        OMX_OUT OMX_STATETYPE* pState);

    static OMX_ERRORTYPE UseBuffer(
        OMX_IN OMX_HANDLETYPE hComponent,
        OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
        OMX_IN OMX_U32 nPortIndex,
        OMX_IN OMX_PTR pAppPrivate,
        OMX_IN OMX_U32 nSizeBytes,
        OMX_IN OMX_U8* pBuffer);
    OMX_ERRORTYPE ISV_UseBuffer(
        OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
        OMX_IN OMX_U32 nPortIndex,
        OMX_IN OMX_PTR pAppPrivate,
        OMX_IN OMX_U32 nSizeBytes,
        OMX_IN OMX_U8* pBuffer);

    static OMX_ERRORTYPE AllocateBuffer(
        OMX_IN OMX_HANDLETYPE hComponent,
        OMX_INOUT OMX_BUFFERHEADERTYPE** ppBuffer,
        OMX_IN OMX_U32 nPortIndex,
        OMX_IN OMX_PTR pAppPrivate,
        OMX_IN OMX_U32 nSizeBytes);
    OMX_ERRORTYPE ISV_AllocateBuffer(
        OMX_INOUT OMX_BUFFERHEADERTYPE** ppBuffer,
        OMX_IN OMX_U32 nPortIndex,
        OMX_IN OMX_PTR pAppPrivate,
        OMX_IN OMX_U32 nSizeBytes);

    static OMX_ERRORTYPE FreeBuffer(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_U32 nPortIndex,
        OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);
    OMX_ERRORTYPE ISV_FreeBuffer(
        OMX_IN  OMX_U32 nPortIndex,
        OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);

    static OMX_ERRORTYPE EmptyThisBuffer(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);
    OMX_ERRORTYPE ISV_EmptyThisBuffer(
        OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);

    static OMX_ERRORTYPE FillThisBuffer(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);
    OMX_ERRORTYPE ISV_FillThisBuffer(
        OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);

    static OMX_ERRORTYPE SetCallbacks(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_CALLBACKTYPE* pCallbacks,
        OMX_IN  OMX_PTR pAppData);
    OMX_ERRORTYPE ISV_SetCallbacks(
        OMX_IN  OMX_CALLBACKTYPE* pCallbacks,
        OMX_IN  OMX_PTR pAppData);

    static OMX_ERRORTYPE ComponentRoleEnum(
        OMX_IN OMX_HANDLETYPE hComponent,
        OMX_OUT OMX_U8 *cRole,
        OMX_IN OMX_U32 nIndex);
    OMX_ERRORTYPE ISV_ComponentRoleEnum(
        OMX_OUT OMX_U8 *cRole,
        OMX_IN OMX_U32 nIndex);

    static OMX_ERRORTYPE FillBufferDone(
        OMX_OUT OMX_HANDLETYPE hComponent,
        OMX_OUT OMX_PTR pAppData,
        OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer);
    OMX_ERRORTYPE ISV_FillBufferDone(
        OMX_OUT OMX_HANDLETYPE hComponent,
        OMX_OUT OMX_PTR pAppData,
        OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer);

    static OMX_ERRORTYPE EventHandler(
            OMX_IN OMX_HANDLETYPE hComponent,
            OMX_IN OMX_PTR pAppData,
            OMX_IN OMX_EVENTTYPE eEvent,
            OMX_IN OMX_U32 nData1,
            OMX_IN OMX_U32 nData2,
            OMX_IN OMX_PTR pEventData);
    OMX_ERRORTYPE ISV_EventHandler(
            OMX_IN OMX_HANDLETYPE hComponent,
            OMX_IN OMX_PTR pAppData,
            OMX_IN OMX_EVENTTYPE eEvent,
            OMX_IN OMX_U32 nData1,
            OMX_IN OMX_U32 nData2,
            OMX_IN OMX_PTR pEventData);
    /* end of component methods & helpers */

    void SetTypeHeader(OMX_PTR type, OMX_U32 size);

    // init & deinit functions
    status_t init(int32_t width, int32_t height);
    void deinit();

    const static OMX_U8 OMX_SPEC_VERSION_MAJOR = 1;
    const static OMX_U8 OMX_SPEC_VERSION_MINOR = 0;
    const static OMX_U8 OMX_SPEC_VERSION_REVISION = 0;
    const static OMX_U8 OMX_SPEC_VERSION_STEP = 0;

    const static OMX_U32 OMX_SPEC_VERSION = 0
        | (OMX_SPEC_VERSION_MAJOR << 0)
        | (OMX_SPEC_VERSION_MINOR << 8)
        | (OMX_SPEC_VERSION_REVISION << 16)
        | (OMX_SPEC_VERSION_STEP << 24);

    typedef enum OMX_ISVINDEXEXTTYPE {
        OMX_IndexISVStartUsed = OMX_IndexVendorStartUnused + 0x0000F000,
        OMX_IndexExtSetISVMode,                  /**< reference: OMX_U32 */
    } OMX_ISVINDEXEXTTYPE;

    typedef enum {
        ISV_DISABLE = 0,
        ISV_AUTO,
    } ISV_MODE;

private:
    OMX_COMPONENTTYPE mBaseComponent;  //returned by GetComponetHandle()
    OMX_COMPONENTTYPE *mComponent;      // passed from the real OMX core
    OMX_CALLBACKTYPE *mpCallBacks;
    ISVOMXCore *mCore;                  // owend by mComponent
    OMX_CALLBACKTYPE *mpISVCallBacks;

    // buffer manager
    sp<ISVBufferManager> mISVBufferManager;

    bool mThreadRunning;

    // vpp thread observer
    sp<ISVProcThreadObserver> mProcThreadObserver;

    // vpp input buffer count + output buffer count
    int32_t mNumISVBuffers;
    int32_t mNumDecoderBuffers;
    int32_t mNumDecoderBuffersBak;
    uint32_t mWidth;
    uint32_t mHeight;
    uint32_t mUseAndroidNativeBufferIndex;
    uint32_t mStoreMetaDataInBuffersIndex;
    uint32_t mHackFormat;

    bool mUseAndroidNativeBuffer;
    bool mUseAndroidNativeBuffer2;

    bool mVPPEnabled;
    bool mVPPOn;
    bool mVPPFlushing;
    bool mOutputCropChanged;
    bool mInitialized;
#ifdef TARGET_VPP_USE_GEN
    // vpp thread
    sp<ISVProcessor> mProcThread;
#else
    static sp<ISVProcessor> mProcThread;
#endif
    // protect create mProcThread instance
    bool mOwnProcessor;
    static pthread_mutex_t ProcThreadInstanceLock;
};

#endif  // #define ISV_OMXCOMPONENT_H_
