/*
 * componentbase.cpp, component base class
 *
 * Copyright (c) 2009-2010 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include <string.h>

#include <pthread.h>

#include <OMX_Core.h>
#include <OMX_Component.h>

#include <componentbase.h>

#include <queue.h>
#include <workqueue.h>
#include <OMX_IndexExt.h>
#include <HardwareAPI.h>

//#define LOG_NDEBUG 0
#undef LOG_TAG
#define LOG_TAG "componentbase"
#include <log.h>

static const OMX_U32 kMaxAdaptiveStreamingWidth = 1920;
static const OMX_U32 kMaxAdaptiveStreamingHeight = 1088;
/*
 * CmdProcessWork
 */
CmdProcessWork::CmdProcessWork(CmdHandlerInterface *ci)
{
    this->ci = ci;

    workq = new WorkQueue;

    __queue_init(&q);
    pthread_mutex_init(&lock, NULL);

    workq->StartWork(true);

    LOGV("command process workqueue started\n");
}

CmdProcessWork::~CmdProcessWork()
{
    struct cmd_s *temp;

    workq->StopWork();
    delete workq;

    while ((temp = PopCmdQueue()))
        free(temp);

    pthread_mutex_destroy(&lock);

    LOGV("command process workqueue stopped\n");
}

OMX_ERRORTYPE CmdProcessWork::PushCmdQueue(struct cmd_s *cmd)
{
    int ret;

    pthread_mutex_lock(&lock);
    ret = queue_push_tail(&q, cmd);
    if (ret) {
        pthread_mutex_unlock(&lock);
        return OMX_ErrorInsufficientResources;
    }

    workq->ScheduleWork(this);
    pthread_mutex_unlock(&lock);

    return OMX_ErrorNone;
}

struct cmd_s *CmdProcessWork::PopCmdQueue(void)
{
    struct cmd_s *cmd;

    pthread_mutex_lock(&lock);
    cmd = (struct cmd_s *)queue_pop_head(&q);
    pthread_mutex_unlock(&lock);

    return cmd;
}

void CmdProcessWork::Work(void)
{
    struct cmd_s *cmd;

    cmd = PopCmdQueue();
    if (cmd) {
        ci->CmdHandler(cmd);
        free(cmd);
    }
}

/* end of CmdProcessWork */

/*
 * ComponentBase
 */
/*
 * constructor & destructor
 */
void ComponentBase::__ComponentBase(void)
{
    memset(name, 0, OMX_MAX_STRINGNAME_SIZE);
    cmodule = NULL;
    handle = NULL;

    roles = NULL;
    nr_roles = 0;

    working_role = NULL;

    ports = NULL;
    nr_ports = 0;
    mEnableAdaptivePlayback = OMX_FALSE;
    memset(&portparam, 0, sizeof(portparam));

    state = OMX_StateUnloaded;

    cmdwork = NULL;

    bufferwork = NULL;

    pthread_mutex_init(&ports_block, NULL);
    pthread_mutex_init(&state_block, NULL);
}

ComponentBase::ComponentBase()
{
    __ComponentBase();
}

ComponentBase::ComponentBase(const OMX_STRING name)
{
    __ComponentBase();
    SetName(name);
}

ComponentBase::~ComponentBase()
{
    pthread_mutex_destroy(&ports_block);
    pthread_mutex_destroy(&state_block);

    if (roles) {
        if (roles[0])
            free(roles[0]);
        free(roles);
    }
}

/* end of constructor & destructor */

/*
 * accessor
 */
/* name */
void ComponentBase::SetName(const OMX_STRING name)
{
    strncpy(this->name, name, (strlen(name) < OMX_MAX_STRINGNAME_SIZE) ? strlen(name) : (OMX_MAX_STRINGNAME_SIZE-1));
   // strncpy(this->name, name, OMX_MAX_STRINGNAME_SIZE);
    this->name[OMX_MAX_STRINGNAME_SIZE-1] = '\0';
}

OMX_STRING ComponentBase::GetName(void)
{
    return name;
}

/* component module */
void ComponentBase::SetCModule(CModule *cmodule)
{
    this->cmodule = cmodule;
}

CModule *ComponentBase::GetCModule(void)
{
    return cmodule;
}

/* end of accessor */

/*
 * core methods & helpers
 */
/* roles */
OMX_ERRORTYPE ComponentBase::SetRolesOfComponent(OMX_U32 nr_roles,
                                                 const OMX_U8 **roles)
{
    OMX_U32 i;

    if (!roles || !nr_roles)
        return OMX_ErrorBadParameter;

    if (this->roles) {
        free(this->roles[0]);
        free(this->roles);
        this->roles = NULL;
    }

    this->roles = (OMX_U8 **)malloc(sizeof(OMX_STRING) * nr_roles);
    if (!this->roles)
        return OMX_ErrorInsufficientResources;

    this->roles[0] = (OMX_U8 *)malloc(OMX_MAX_STRINGNAME_SIZE * nr_roles);
    if (!this->roles[0]) {
        free(this->roles);
        this->roles = NULL;
        return OMX_ErrorInsufficientResources;
    }

    for (i = 0; i < nr_roles; i++) {
        if (i < nr_roles-1)
            this->roles[i+1] = this->roles[i] + OMX_MAX_STRINGNAME_SIZE;

        strncpy((OMX_STRING)&this->roles[i][0],
                (const OMX_STRING)&roles[i][0], OMX_MAX_STRINGNAME_SIZE);
    }

    this->nr_roles = nr_roles;
    return OMX_ErrorNone;
}

/* GetHandle & FreeHandle */
OMX_ERRORTYPE ComponentBase::GetHandle(OMX_HANDLETYPE *pHandle,
                                       OMX_PTR pAppData,
                                       OMX_CALLBACKTYPE *pCallBacks)
{
    OMX_U32 i;
    OMX_ERRORTYPE ret;

    if (!pHandle)
        return OMX_ErrorBadParameter;

    if (handle)
        return OMX_ErrorUndefined;

    cmdwork = new CmdProcessWork(this);
    if (!cmdwork)
        return OMX_ErrorInsufficientResources;

    bufferwork = new WorkQueue();
    if (!bufferwork) {
        ret = OMX_ErrorInsufficientResources;
        goto free_cmdwork;
    }

    handle = (OMX_COMPONENTTYPE *)calloc(1, sizeof(*handle));
    if (!handle) {
        ret = OMX_ErrorInsufficientResources;
        goto free_bufferwork;
    }

    /* handle initialization */
    SetTypeHeader(handle, sizeof(*handle));
    handle->pComponentPrivate = static_cast<OMX_PTR>(this);
    handle->pApplicationPrivate = pAppData;

    /* connect handle's functions */
    handle->GetComponentVersion = NULL;
    handle->SendCommand = SendCommand;
    handle->GetParameter = GetParameter;
    handle->SetParameter = SetParameter;
    handle->GetConfig = GetConfig;
    handle->SetConfig = SetConfig;
    handle->GetExtensionIndex = GetExtensionIndex;
    handle->GetState = GetState;
    handle->ComponentTunnelRequest = NULL;
    handle->UseBuffer = UseBuffer;
    handle->AllocateBuffer = AllocateBuffer;
    handle->FreeBuffer = FreeBuffer;
    handle->EmptyThisBuffer = EmptyThisBuffer;
    handle->FillThisBuffer = FillThisBuffer;
    handle->SetCallbacks = SetCallbacks;
    handle->ComponentDeInit = NULL;
    handle->UseEGLImage = NULL;
    handle->ComponentRoleEnum = ComponentRoleEnum;

    appdata = pAppData;
    callbacks = pCallBacks;

    if (nr_roles == 1) {
        SetWorkingRole((OMX_STRING)&roles[0][0]);
        ret = ApplyWorkingRole();
        if (ret != OMX_ErrorNone) {
            SetWorkingRole(NULL);
            goto free_handle;
        }
    }

    *pHandle = (OMX_HANDLETYPE *)handle;
    state = OMX_StateLoaded;
    return OMX_ErrorNone;

free_handle:
    free(handle);

    appdata = NULL;
    callbacks = NULL;
    *pHandle = NULL;

free_bufferwork:
    delete bufferwork;

free_cmdwork:
    delete cmdwork;

    return ret;
}

OMX_ERRORTYPE ComponentBase::FreeHandle(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE ret;

    if (hComponent != handle)
        return OMX_ErrorBadParameter;

    if (state != OMX_StateLoaded)
        return OMX_ErrorIncorrectStateOperation;

    FreePorts();

    free(handle);

    appdata = NULL;
    callbacks = NULL;

    delete cmdwork;
    delete bufferwork;

    state = OMX_StateUnloaded;
    return OMX_ErrorNone;
}

/* end of core methods & helpers */

/*
 * component methods & helpers
 */
OMX_ERRORTYPE ComponentBase::SendCommand(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_COMMANDTYPE Cmd,
    OMX_IN  OMX_U32 nParam1,
    OMX_IN  OMX_PTR pCmdData)
{
    ComponentBase *cbase;

    if (!hComponent)
        return OMX_ErrorBadParameter;

    cbase = static_cast<ComponentBase *>
        (((OMX_COMPONENTTYPE *)hComponent)->pComponentPrivate);
    if (!cbase)
        return OMX_ErrorBadParameter;

    return cbase->CBaseSendCommand(hComponent, Cmd, nParam1, pCmdData);
}

OMX_ERRORTYPE ComponentBase::CBaseSendCommand(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_COMMANDTYPE Cmd,
    OMX_IN  OMX_U32 nParam1,
    OMX_IN  OMX_PTR pCmdData)
{
    struct cmd_s *cmd;

    if (hComponent != handle)
        return OMX_ErrorInvalidComponent;

    /* basic error check */
    switch (Cmd) {
    case OMX_CommandStateSet:
        /*
         * Todo
         */
        break;
    case OMX_CommandFlush: {
        OMX_U32 port_index = nParam1;

        if ((port_index != OMX_ALL) && (port_index > nr_ports-1))
            return OMX_ErrorBadPortIndex;
        break;
    }
    case OMX_CommandPortDisable:
    case OMX_CommandPortEnable: {
        OMX_U32 port_index = nParam1;

        if ((port_index != OMX_ALL) && (port_index > nr_ports-1))
            return OMX_ErrorBadPortIndex;
        break;
    }
    case OMX_CommandMarkBuffer: {
        OMX_MARKTYPE *mark = (OMX_MARKTYPE *)pCmdData;
        OMX_MARKTYPE *copiedmark;
        OMX_U32 port_index = nParam1;

        if (port_index > nr_ports-1)
            return OMX_ErrorBadPortIndex;

        if (!mark || !mark->hMarkTargetComponent)
            return OMX_ErrorBadParameter;

        copiedmark = (OMX_MARKTYPE *)malloc(sizeof(*copiedmark));
        if (!copiedmark)
            return OMX_ErrorInsufficientResources;

        copiedmark->hMarkTargetComponent = mark->hMarkTargetComponent;
        copiedmark->pMarkData = mark->pMarkData;
        pCmdData = (OMX_PTR)copiedmark;
        break;
    }
    default:
        LOGE("command %d not supported\n", Cmd);
        return OMX_ErrorUnsupportedIndex;
    }

    cmd = (struct cmd_s *)malloc(sizeof(*cmd));
    if (!cmd)
        return OMX_ErrorInsufficientResources;

    cmd->cmd = Cmd;
    cmd->param1 = nParam1;
    cmd->cmddata = pCmdData;

    return cmdwork->PushCmdQueue(cmd);
}

OMX_ERRORTYPE ComponentBase::GetParameter(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nParamIndex,
    OMX_INOUT OMX_PTR pComponentParameterStructure)
{
    ComponentBase *cbase;

    if (!hComponent)
        return OMX_ErrorBadParameter;

    cbase = static_cast<ComponentBase *>
        (((OMX_COMPONENTTYPE *)hComponent)->pComponentPrivate);
    if (!cbase)
        return OMX_ErrorBadParameter;

    return cbase->CBaseGetParameter(hComponent, nParamIndex,
                                    pComponentParameterStructure);
}

OMX_ERRORTYPE ComponentBase::CBaseGetParameter(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nParamIndex,
    OMX_INOUT OMX_PTR pComponentParameterStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if (hComponent != handle)
        return OMX_ErrorBadParameter;
    switch (nParamIndex) {
    case OMX_IndexParamAudioInit:
    case OMX_IndexParamVideoInit:
    case OMX_IndexParamImageInit:
    case OMX_IndexParamOtherInit: {
        OMX_PORT_PARAM_TYPE *p =
            (OMX_PORT_PARAM_TYPE *)pComponentParameterStructure;

        ret = CheckTypeHeader(p, sizeof(*p));
        if (ret != OMX_ErrorNone)
            return ret;

        memcpy(p, &portparam, sizeof(*p));
        break;
    }
    case OMX_IndexParamPortDefinition: {
        OMX_PARAM_PORTDEFINITIONTYPE *p =
            (OMX_PARAM_PORTDEFINITIONTYPE *)pComponentParameterStructure;
        OMX_U32 index = p->nPortIndex;
        PortBase *port = NULL;

        ret = CheckTypeHeader(p, sizeof(*p));
        if (ret != OMX_ErrorNone)
            return ret;

        if (index < nr_ports)
            port = ports[index];

        if (!port)
            return OMX_ErrorBadPortIndex;

        memcpy(p, port->GetPortDefinition(), sizeof(*p));
        break;
    }
    case OMX_IndexParamCompBufferSupplier:
        /*
         * Todo
         */

        ret = OMX_ErrorUnsupportedIndex;
        break;

    default:
        ret = ComponentGetParameter(nParamIndex, pComponentParameterStructure);
    } /* switch */

    return ret;
}

OMX_ERRORTYPE ComponentBase::SetParameter(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nIndex,
    OMX_IN  OMX_PTR pComponentParameterStructure)
{
    ComponentBase *cbase;

    if (!hComponent)
        return OMX_ErrorBadParameter;

    cbase = static_cast<ComponentBase *>
        (((OMX_COMPONENTTYPE *)hComponent)->pComponentPrivate);
    if (!cbase)
        return OMX_ErrorBadParameter;

    return cbase->CBaseSetParameter(hComponent, nIndex,
                                    pComponentParameterStructure);
}

OMX_ERRORTYPE ComponentBase::CBaseSetParameter(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nIndex,
    OMX_IN  OMX_PTR pComponentParameterStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if (hComponent != handle)
        return OMX_ErrorBadParameter;

    switch (nIndex) {
    case OMX_IndexParamAudioInit:
    case OMX_IndexParamVideoInit:
    case OMX_IndexParamImageInit:
    case OMX_IndexParamOtherInit:
        /* preventing clients from setting OMX_PORT_PARAM_TYPE */
        ret = OMX_ErrorUnsupportedIndex;
        break;
    case OMX_IndexParamPortDefinition: {
        OMX_PARAM_PORTDEFINITIONTYPE *p =
            (OMX_PARAM_PORTDEFINITIONTYPE *)pComponentParameterStructure;
        OMX_U32 index = p->nPortIndex;
        PortBase *port = NULL;

        ret = CheckTypeHeader(p, sizeof(*p));
        if (ret != OMX_ErrorNone)
            return ret;

        if (index < nr_ports)
            port = ports[index];

        if (!port)
            return OMX_ErrorBadPortIndex;

        if (port->IsEnabled()) {
            if (state != OMX_StateLoaded && state != OMX_StateWaitForResources)
                return OMX_ErrorIncorrectStateOperation;
        }

        if (index == 1 && mEnableAdaptivePlayback == OMX_TRUE) {
            if (p->format.video.nFrameWidth < mMaxFrameWidth)
                p->format.video.nFrameWidth = mMaxFrameWidth;
            if (p->format.video.nFrameHeight < mMaxFrameHeight)
                p->format.video.nFrameHeight = mMaxFrameHeight;
        }

        if (working_role != NULL && !strncmp((char*)working_role, "video_encoder", 13)) {
            if (p->format.video.nFrameWidth > 2048 || p->format.video.nFrameHeight > 2048)
                return OMX_ErrorUnsupportedSetting;

            if(p->format.video.eColorFormat == OMX_COLOR_FormatUnused)
                p->nBufferSize = p->format.video.nFrameWidth * p->format.video.nFrameHeight *3/2;
        }

        ret = port->SetPortDefinition(p, false);
        if (ret != OMX_ErrorNone) {
            return ret;
        }
        break;
    }
    case OMX_IndexParamCompBufferSupplier:
        /*
         * Todo
         */

        ret = OMX_ErrorUnsupportedIndex;
        break;
    case OMX_IndexParamStandardComponentRole: {
        OMX_PARAM_COMPONENTROLETYPE *p =
            (OMX_PARAM_COMPONENTROLETYPE *)pComponentParameterStructure;

        if (state != OMX_StateLoaded && state != OMX_StateWaitForResources)
            return OMX_ErrorIncorrectStateOperation;

        ret = CheckTypeHeader(p, sizeof(*p));
        if (ret != OMX_ErrorNone)
            return ret;

        ret = SetWorkingRole((OMX_STRING)p->cRole);
        if (ret != OMX_ErrorNone)
            return ret;

        if (ports)
            FreePorts();

        ret = ApplyWorkingRole();
        if (ret != OMX_ErrorNone) {
            SetWorkingRole(NULL);
            return ret;
        }
        break;
    }
    default:
        if (nIndex == (OMX_INDEXTYPE)OMX_IndexExtPrepareForAdaptivePlayback) {
            android::PrepareForAdaptivePlaybackParams* p =
                    (android::PrepareForAdaptivePlaybackParams *)pComponentParameterStructure;

            ret = CheckTypeHeader(p, sizeof(*p));
            if (ret != OMX_ErrorNone)
                return ret;

            if (p->nPortIndex != 1)
                return OMX_ErrorBadPortIndex;

            if (!(working_role != NULL && !strncmp((char*)working_role, "video_decoder", 13)))
                return  OMX_ErrorBadParameter;

            if (p->nMaxFrameWidth > kMaxAdaptiveStreamingWidth
                    || p->nMaxFrameHeight > kMaxAdaptiveStreamingHeight) {
                LOGE("resolution %d x %d exceed max driver support %d x %d\n",p->nMaxFrameWidth, p->nMaxFrameHeight,
                        kMaxAdaptiveStreamingWidth, kMaxAdaptiveStreamingHeight);
                return OMX_ErrorBadParameter;
            }

            if (GetWorkingRole() != NULL &&
                        !strcmp (GetWorkingRole(),"video_decoder.vp9")) {
                if (p->nMaxFrameWidth < 640 && p->nMaxFrameHeight < 480) {
                    p->nMaxFrameHeight = kMaxAdaptiveStreamingHeight;
                    p->nMaxFrameWidth = kMaxAdaptiveStreamingWidth;
                }
            }

            mEnableAdaptivePlayback = p->bEnable;
            if (mEnableAdaptivePlayback != OMX_TRUE)
                return OMX_ErrorBadParameter;

            mMaxFrameWidth = p->nMaxFrameWidth;
            mMaxFrameHeight = p->nMaxFrameHeight;
            /* update output port definition */
            OMX_PARAM_PORTDEFINITIONTYPE paramPortDefinitionOutput;
            if (nr_ports > p->nPortIndex && ports[p->nPortIndex]) {
                memcpy(&paramPortDefinitionOutput,ports[p->nPortIndex]->GetPortDefinition(),
                        sizeof(paramPortDefinitionOutput));
                paramPortDefinitionOutput.format.video.nFrameWidth = mMaxFrameWidth;
                paramPortDefinitionOutput.format.video.nFrameHeight = mMaxFrameHeight;
                ports[p->nPortIndex]->SetPortDefinition(&paramPortDefinitionOutput, true);
            }
        } else {
            ret = ComponentSetParameter(nIndex, pComponentParameterStructure);
        }
        break;
    } /* switch */

    return ret;
}

OMX_ERRORTYPE ComponentBase::GetConfig(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nIndex,
    OMX_INOUT OMX_PTR pComponentConfigStructure)
{
    ComponentBase *cbase;

    if (!hComponent)
        return OMX_ErrorBadParameter;

    cbase = static_cast<ComponentBase *>
        (((OMX_COMPONENTTYPE *)hComponent)->pComponentPrivate);
    if (!cbase)
        return OMX_ErrorBadParameter;

    return cbase->CBaseGetConfig(hComponent, nIndex,
                                 pComponentConfigStructure);
}

OMX_ERRORTYPE ComponentBase::CBaseGetConfig(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nIndex,
    OMX_INOUT OMX_PTR pComponentConfigStructure)
{
    OMX_ERRORTYPE ret;

    if (hComponent != handle)
        return OMX_ErrorBadParameter;

    switch (nIndex) {
    default:
        ret = ComponentGetConfig(nIndex, pComponentConfigStructure);
    }

    return ret;
}

OMX_ERRORTYPE ComponentBase::SetConfig(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nIndex,
    OMX_IN  OMX_PTR pComponentConfigStructure)
{
    ComponentBase *cbase;

    if (!hComponent)
        return OMX_ErrorBadParameter;

    cbase = static_cast<ComponentBase *>
        (((OMX_COMPONENTTYPE *)hComponent)->pComponentPrivate);
    if (!cbase)
        return OMX_ErrorBadParameter;

    return cbase->CBaseSetConfig(hComponent, nIndex,
                                 pComponentConfigStructure);
}

OMX_ERRORTYPE ComponentBase::CBaseSetConfig(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nIndex,
    OMX_IN  OMX_PTR pComponentConfigStructure)
{
    OMX_ERRORTYPE ret;
 
    if (hComponent != handle)
        return OMX_ErrorBadParameter;

    switch (nIndex) {
    default:
        ret = ComponentSetConfig(nIndex, pComponentConfigStructure);
    }

    return ret;
}

OMX_ERRORTYPE ComponentBase::GetExtensionIndex(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_STRING cParameterName,
    OMX_OUT OMX_INDEXTYPE* pIndexType)
{
    ComponentBase *cbase;

    if (!hComponent)
        return OMX_ErrorBadParameter;

    cbase = static_cast<ComponentBase *>
        (((OMX_COMPONENTTYPE *)hComponent)->pComponentPrivate);
    if (!cbase)
        return OMX_ErrorBadParameter;

    return cbase->CBaseGetExtensionIndex(hComponent, cParameterName,
                                         pIndexType);
}

OMX_ERRORTYPE ComponentBase::CBaseGetExtensionIndex(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_STRING cParameterName,
    OMX_OUT OMX_INDEXTYPE* pIndexType)
{
    /*
     * Todo
     */
    if (hComponent != handle) {

        return OMX_ErrorBadParameter;
    }

    if (!strcmp(cParameterName, "OMX.google.android.index.storeMetaDataInBuffers")) {
        *pIndexType = static_cast<OMX_INDEXTYPE>(OMX_IndexStoreMetaDataInBuffers);
        return OMX_ErrorNone;
    }

    if (!strcmp(cParameterName, "OMX.google.android.index.enableAndroidNativeBuffers")) {
        *pIndexType = static_cast<OMX_INDEXTYPE>(OMX_IndexExtEnableNativeBuffer);
        return OMX_ErrorNone;
    }

    if (!strcmp(cParameterName, "OMX.google.android.index.getAndroidNativeBufferUsage")) {
        *pIndexType = static_cast<OMX_INDEXTYPE>(OMX_IndexExtGetNativeBufferUsage);
        return OMX_ErrorNone;
    }

    if (!strcmp(cParameterName, "OMX.google.android.index.useAndroidNativeBuffer")) {
        *pIndexType = static_cast<OMX_INDEXTYPE>(OMX_IndexExtUseNativeBuffer);
        return OMX_ErrorNone;
    }

    if (!strcmp(cParameterName, "OMX.Intel.index.rotation")) {
        *pIndexType = static_cast<OMX_INDEXTYPE>(OMX_IndexExtRotationDegrees);
        return OMX_ErrorNone;
    }

    if (!strcmp(cParameterName, "OMX.Intel.index.enableSyncEncoding")) {
        *pIndexType = static_cast<OMX_INDEXTYPE>(OMX_IndexExtSyncEncoding);
        return OMX_ErrorNone;
    }

    if (!strcmp(cParameterName, "OMX.google.android.index.prependSPSPPSToIDRFrames")) {
        *pIndexType = static_cast<OMX_INDEXTYPE>(OMX_IndexExtPrependSPSPPS);
        return OMX_ErrorNone;
    }

#ifdef TARGET_HAS_ISV
    if (!strcmp(cParameterName, "OMX.Intel.index.vppBufferNum")) {
        *pIndexType = static_cast<OMX_INDEXTYPE>(OMX_IndexExtVppBufferNum);
        return OMX_ErrorNone;
    }
#endif
    
    if (!strcmp(cParameterName, "OMX.Intel.index.enableErrorReport")) {
        *pIndexType = static_cast<OMX_INDEXTYPE>(OMX_IndexExtEnableErrorReport);
        return OMX_ErrorNone;
    }

    if (!strcmp(cParameterName, "OMX.google.android.index.prepareForAdaptivePlayback")) {
        *pIndexType = static_cast<OMX_INDEXTYPE>(OMX_IndexExtPrepareForAdaptivePlayback);
        return OMX_ErrorNone;
    }

    if (!strcmp(cParameterName, "OMX.Intel.index.requestBlackFramePointer")) {
        *pIndexType = static_cast<OMX_INDEXTYPE>(OMX_IndexExtRequestBlackFramePointer);
        return OMX_ErrorNone;
    }

    if (!strcmp(cParameterName, "OMX.Intel.index.vp8MaxFrameRatio")) {
        *pIndexType = static_cast<OMX_INDEXTYPE>(OMX_IndexExtVP8MaxFrameSizeRatio);
        return OMX_ErrorNone;
    }

    if (!strcmp(cParameterName, "OMX.Intel.index.temporalLayer")) {
        *pIndexType = static_cast<OMX_INDEXTYPE>(OMX_IndexExtTemporalLayer);
        return OMX_ErrorNone;
    }

    if (!strcmp(cParameterName, "OMX.Intel.index.vuiEnable")) {
        *pIndexType = static_cast<OMX_INDEXTYPE>(OMX_IndexParamIntelAVCVUI);
        return OMX_ErrorNone;
    }

    if (!strcmp(cParameterName, "OMX.Intel.index.sliceNumber")) {
        *pIndexType = static_cast<OMX_INDEXTYPE>(OMX_IndexConfigIntelSliceNumbers);
        return OMX_ErrorNone;
    }

    if (!strcmp(cParameterName, "OMX.Intel.index.intelBitrateConfig")) {
        *pIndexType = static_cast<OMX_INDEXTYPE>(OMX_IndexConfigIntelBitrate);
        return OMX_ErrorNone;
    }

    if (!strcmp(cParameterName, "OMX.Intel.index.autoIntraRefresh")) {
        *pIndexType = static_cast<OMX_INDEXTYPE>(OMX_IndexConfigIntelAIR);
        return OMX_ErrorNone;
    }

    return OMX_ErrorUnsupportedIndex;
}

OMX_ERRORTYPE ComponentBase::GetState(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_STATETYPE* pState)
{
    ComponentBase *cbase;

    if (!hComponent)
        return OMX_ErrorBadParameter;

    cbase = static_cast<ComponentBase *>
        (((OMX_COMPONENTTYPE *)hComponent)->pComponentPrivate);
    if (!cbase)
        return OMX_ErrorBadParameter;

    return cbase->CBaseGetState(hComponent, pState);
}

OMX_ERRORTYPE ComponentBase::CBaseGetState(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_STATETYPE* pState)
{
    if (hComponent != handle)
        return OMX_ErrorBadParameter;

    pthread_mutex_lock(&state_block);
    *pState = state;
    pthread_mutex_unlock(&state_block);
    return OMX_ErrorNone;
}
OMX_ERRORTYPE ComponentBase::UseBuffer(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE **ppBufferHdr,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate,
    OMX_IN OMX_U32 nSizeBytes,
    OMX_IN OMX_U8 *pBuffer)
{
    ComponentBase *cbase;

    if (!hComponent)
        return OMX_ErrorBadParameter;

    cbase = static_cast<ComponentBase *>
        (((OMX_COMPONENTTYPE *)hComponent)->pComponentPrivate);
    if (!cbase)
        return OMX_ErrorBadParameter;

    return cbase->CBaseUseBuffer(hComponent, ppBufferHdr, nPortIndex,
                                 pAppPrivate, nSizeBytes, pBuffer);
}

OMX_ERRORTYPE ComponentBase::CBaseUseBuffer(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE **ppBufferHdr,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate,
    OMX_IN OMX_U32 nSizeBytes,
    OMX_IN OMX_U8 *pBuffer)
{
    PortBase *port = NULL;
    OMX_ERRORTYPE ret;

    if (hComponent != handle)
        return OMX_ErrorBadParameter;

    if (!ppBufferHdr)
        return OMX_ErrorBadParameter;
    *ppBufferHdr = NULL;

    if (!pBuffer)
        return OMX_ErrorBadParameter;

    if (ports)
        if (nPortIndex < nr_ports)
            port = ports[nPortIndex];

    if (!port)
        return OMX_ErrorBadParameter;

    if (port->IsEnabled()) {
        if (state != OMX_StateLoaded && state != OMX_StateWaitForResources)
            return OMX_ErrorIncorrectStateOperation;
    }

    return port->UseBuffer(ppBufferHdr, nPortIndex, pAppPrivate, nSizeBytes,
                           pBuffer);
}

OMX_ERRORTYPE ComponentBase::AllocateBuffer(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE **ppBuffer,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate,
    OMX_IN OMX_U32 nSizeBytes)
{
    ComponentBase *cbase;

    if (!hComponent)
        return OMX_ErrorBadParameter;

    cbase = static_cast<ComponentBase *>
        (((OMX_COMPONENTTYPE *)hComponent)->pComponentPrivate);
    if (!cbase)
        return OMX_ErrorBadParameter;

    return cbase->CBaseAllocateBuffer(hComponent, ppBuffer, nPortIndex,
                                      pAppPrivate, nSizeBytes);
}

OMX_ERRORTYPE ComponentBase::CBaseAllocateBuffer(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE **ppBuffer,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate,
    OMX_IN OMX_U32 nSizeBytes)
{
    PortBase *port = NULL;
    OMX_ERRORTYPE ret;

    if (hComponent != handle)
        return OMX_ErrorBadParameter;

    if (!ppBuffer)
        return OMX_ErrorBadParameter;
    *ppBuffer = NULL;

    if (ports)
        if (nPortIndex < nr_ports)
            port = ports[nPortIndex];

    if (!port)
        return OMX_ErrorBadParameter;

    if (port->IsEnabled()) {
        if (state != OMX_StateLoaded && state != OMX_StateWaitForResources)
            return OMX_ErrorIncorrectStateOperation;
    }

    return port->AllocateBuffer(ppBuffer, nPortIndex, pAppPrivate, nSizeBytes);
}

OMX_ERRORTYPE ComponentBase::FreeBuffer(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_U32 nPortIndex,
    OMX_IN  OMX_BUFFERHEADERTYPE *pBuffer)
{
    ComponentBase *cbase;

    if (!hComponent)
        return OMX_ErrorBadParameter;

    cbase = static_cast<ComponentBase *>
        (((OMX_COMPONENTTYPE *)hComponent)->pComponentPrivate);
    if (!cbase)
        return OMX_ErrorBadParameter;

    return cbase->CBaseFreeBuffer(hComponent, nPortIndex, pBuffer);
}

OMX_ERRORTYPE ComponentBase::CBaseFreeBuffer(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_U32 nPortIndex,
    OMX_IN  OMX_BUFFERHEADERTYPE *pBuffer)
{
    PortBase *port = NULL;
    OMX_ERRORTYPE ret;

    if (hComponent != handle)
        return OMX_ErrorBadParameter;

    if (!pBuffer)
        return OMX_ErrorBadParameter;

    if (ports)
        if (nPortIndex < nr_ports)
            port = ports[nPortIndex];

    if (!port)
        return OMX_ErrorBadParameter;

    ProcessorPreFreeBuffer(nPortIndex, pBuffer);

    return port->FreeBuffer(nPortIndex, pBuffer);
}

OMX_ERRORTYPE ComponentBase::EmptyThisBuffer(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)
{
    ComponentBase *cbase;

    if (!hComponent)
        return OMX_ErrorBadParameter;

    cbase = static_cast<ComponentBase *>
        (((OMX_COMPONENTTYPE *)hComponent)->pComponentPrivate);
    if (!cbase)
        return OMX_ErrorBadParameter;

    return cbase->CBaseEmptyThisBuffer(hComponent, pBuffer);
}

OMX_ERRORTYPE ComponentBase::CBaseEmptyThisBuffer(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_BUFFERHEADERTYPE *pBuffer)
{
    PortBase *port = NULL;
    OMX_U32 port_index;
    OMX_ERRORTYPE ret;

    if ((hComponent != handle) || !pBuffer)
        return OMX_ErrorBadParameter;

    ret = CheckTypeHeader(pBuffer, sizeof(OMX_BUFFERHEADERTYPE));
    if (ret != OMX_ErrorNone)
        return ret;

    port_index = pBuffer->nInputPortIndex;
    if (port_index == (OMX_U32)-1)
        return OMX_ErrorBadParameter;

    if (ports)
        if (port_index < nr_ports)
            port = ports[port_index];

    if (!port)
        return OMX_ErrorBadParameter;

    if (port->IsEnabled()) {
        if (state != OMX_StateIdle && state != OMX_StateExecuting &&
            state != OMX_StatePause)
            return OMX_ErrorIncorrectStateOperation;
    }

    if (!pBuffer->hMarkTargetComponent) {
        OMX_MARKTYPE *mark;

        mark = port->PopMark();
        if (mark) {
            pBuffer->hMarkTargetComponent = mark->hMarkTargetComponent;
            pBuffer->pMarkData = mark->pMarkData;
            free(mark);
        }
    }

    ProcessorPreEmptyBuffer(pBuffer);

    ret = port->PushThisBuffer(pBuffer);
    if (ret == OMX_ErrorNone)
        bufferwork->ScheduleWork(this);

    return ret;
}

OMX_ERRORTYPE ComponentBase::FillThisBuffer(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_BUFFERHEADERTYPE *pBuffer)
{
    ComponentBase *cbase;

    if (!hComponent)
        return OMX_ErrorBadParameter;

    cbase = static_cast<ComponentBase *>
        (((OMX_COMPONENTTYPE *)hComponent)->pComponentPrivate);
    if (!cbase)
        return OMX_ErrorBadParameter;

    return cbase->CBaseFillThisBuffer(hComponent, pBuffer);
}

OMX_ERRORTYPE ComponentBase::CBaseFillThisBuffer(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_BUFFERHEADERTYPE *pBuffer)
{
    PortBase *port = NULL;
    OMX_U32 port_index;
    OMX_ERRORTYPE ret;

    if ((hComponent != handle) || !pBuffer)
        return OMX_ErrorBadParameter;

    ret = CheckTypeHeader(pBuffer, sizeof(OMX_BUFFERHEADERTYPE));
    if (ret != OMX_ErrorNone)
        return ret;

    port_index = pBuffer->nOutputPortIndex;
    if (port_index == (OMX_U32)-1)
        return OMX_ErrorBadParameter;

    if (ports)
        if (port_index < nr_ports)
            port = ports[port_index];

    if (!port)
        return OMX_ErrorBadParameter;

    if (port->IsEnabled()) {
        if (state != OMX_StateIdle && state != OMX_StateExecuting &&
            state != OMX_StatePause)
            return OMX_ErrorIncorrectStateOperation;
    }

    ProcessorPreFillBuffer(pBuffer);

    ret = port->PushThisBuffer(pBuffer);
    if (ret == OMX_ErrorNone)
        bufferwork->ScheduleWork(this);

    return ret;
}

OMX_ERRORTYPE ComponentBase::SetCallbacks(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_CALLBACKTYPE* pCallbacks,
    OMX_IN  OMX_PTR pAppData)
{
    ComponentBase *cbase;

    if (!hComponent)
        return OMX_ErrorBadParameter;

    cbase = static_cast<ComponentBase *>
        (((OMX_COMPONENTTYPE *)hComponent)->pComponentPrivate);
    if (!cbase)
        return OMX_ErrorBadParameter;

    return cbase->CBaseSetCallbacks(hComponent, pCallbacks, pAppData);
}

OMX_ERRORTYPE ComponentBase::CBaseSetCallbacks(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_CALLBACKTYPE *pCallbacks,
    OMX_IN  OMX_PTR pAppData)
{
    if (hComponent != handle)
        return OMX_ErrorBadParameter;

    appdata = pAppData;
    callbacks = pCallbacks;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE ComponentBase::ComponentRoleEnum(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_U8 *cRole,
    OMX_IN OMX_U32 nIndex)
{
    ComponentBase *cbase;

    if (!hComponent)
        return OMX_ErrorBadParameter;

    cbase = static_cast<ComponentBase *>
        (((OMX_COMPONENTTYPE *)hComponent)->pComponentPrivate);
    if (!cbase)
        return OMX_ErrorBadParameter;

    return cbase->CBaseComponentRoleEnum(hComponent, cRole, nIndex);
}

OMX_ERRORTYPE ComponentBase::CBaseComponentRoleEnum(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_U8 *cRole,
    OMX_IN OMX_U32 nIndex)
{
    if (hComponent != (OMX_HANDLETYPE *)this->handle)
        return OMX_ErrorBadParameter;

    if (nIndex >= nr_roles)
        return OMX_ErrorBadParameter;

    strncpy((char *)cRole, (const char *)roles[nIndex],
            OMX_MAX_STRINGNAME_SIZE);
    return OMX_ErrorNone;
}

/* implement CmdHandlerInterface */
static const char *cmd_name[OMX_CommandMarkBuffer+2] = {
    "OMX_CommandStateSet",
    "OMX_CommandFlush",
    "OMX_CommandPortDisable",
    "OMX_CommandPortEnable",
    "OMX_CommandMarkBuffer",
    "Unknown Command",
};

static inline const char *GetCmdName(OMX_COMMANDTYPE cmd)
{
    if (cmd > OMX_CommandMarkBuffer)
        cmd = (OMX_COMMANDTYPE)(OMX_CommandMarkBuffer+1);

    return cmd_name[cmd];
}

void ComponentBase::CmdHandler(struct cmd_s *cmd)
{
    LOGV("%s:%s: handling %s command\n",
         GetName(), GetWorkingRole(), GetCmdName(cmd->cmd));

    switch (cmd->cmd) {
    case OMX_CommandStateSet: {
        OMX_STATETYPE transition = (OMX_STATETYPE)cmd->param1;

        pthread_mutex_lock(&state_block);
        TransState(transition);
        pthread_mutex_unlock(&state_block);
        break;
    }
    case OMX_CommandFlush: {
        OMX_U32 port_index = cmd->param1;
        pthread_mutex_lock(&ports_block);
        ProcessorFlush(port_index);
        FlushPort(port_index, 1);
        pthread_mutex_unlock(&ports_block);
        break;
    }
    case OMX_CommandPortDisable: {
        OMX_U32 port_index = cmd->param1;

        TransStatePort(port_index, PortBase::OMX_PortDisabled);
        break;
    }
    case OMX_CommandPortEnable: {
        OMX_U32 port_index = cmd->param1;

        TransStatePort(port_index, PortBase::OMX_PortEnabled);
        break;
    }
    case OMX_CommandMarkBuffer: {
        OMX_U32 port_index = (OMX_U32)cmd->param1;
        OMX_MARKTYPE *mark = (OMX_MARKTYPE *)cmd->cmddata;

        PushThisMark(port_index, mark);
        break;
    }
    default:
        LOGE("%s:%s:%s: exit failure, command %d cannot be handled\n",
             GetName(), GetWorkingRole(), GetCmdName(cmd->cmd), cmd->cmd);
        break;
    } /* switch */

    LOGV("%s:%s: command %s handling done\n",
         GetName(), GetWorkingRole(), GetCmdName(cmd->cmd));
}

/*
 * SendCommand:OMX_CommandStateSet
 * called in CmdHandler or called in other parts of component for reporting
 * internal error (OMX_StateInvalid).
 */
/*
 * Todo
 *   Resource Management (OMX_StateWaitForResources)
 *   for now, we never notify OMX_ErrorInsufficientResources,
 *   so IL client doesn't try to set component' state OMX_StateWaitForResources
 */
static const char *state_name[OMX_StateWaitForResources+2] = {
    "OMX_StateInvalid",
    "OMX_StateLoaded",
    "OMX_StateIdle",
    "OMX_StateExecuting",
    "OMX_StatePause",
    "OMX_StateWaitForResources",
    "Unknown State",
};

static inline const char *GetStateName(OMX_STATETYPE state)
{
    if (state > OMX_StateWaitForResources)
        state = (OMX_STATETYPE)(OMX_StateWaitForResources+1);

    return state_name[state];
}

void ComponentBase::TransState(OMX_STATETYPE transition)
{
    OMX_STATETYPE current = this->state;
    OMX_EVENTTYPE event;
    OMX_U32 data1, data2;
    OMX_ERRORTYPE ret;

    LOGV("%s:%s: try to transit state from %s to %s\n",
         GetName(), GetWorkingRole(), GetStateName(current),
         GetStateName(transition));

    /* same state */
    if (current == transition) {
        ret = OMX_ErrorSameState;
        LOGE("%s:%s: exit failure, same state (%s)\n",
             GetName(), GetWorkingRole(), GetStateName(current));
        goto notify_event;
    }

    /* invalid state */
    if (current == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        LOGE("%s:%s: exit failure, current state is OMX_StateInvalid\n",
             GetName(), GetWorkingRole());
        goto notify_event;
    }

    if (transition == OMX_StateLoaded)
        ret = TransStateToLoaded(current);
    else if (transition == OMX_StateIdle)
        ret = TransStateToIdle(current);
    else if (transition == OMX_StateExecuting)
        ret = TransStateToExecuting(current);
    else if (transition == OMX_StatePause)
        ret = TransStateToPause(current);
    else if (transition == OMX_StateInvalid)
        ret = TransStateToInvalid(current);
    else if (transition == OMX_StateWaitForResources)
        ret = TransStateToWaitForResources(current);
    else
        ret = OMX_ErrorIncorrectStateTransition;

notify_event:
    if (ret == OMX_ErrorNone) {
        event = OMX_EventCmdComplete;
        data1 = OMX_CommandStateSet;
        data2 = transition;

        state = transition;
        LOGD("%s:%s: transition from %s to %s completed",
             GetName(), GetWorkingRole(),
             GetStateName(current), GetStateName(transition));
    }
    else {
        event = OMX_EventError;
        data1 = ret;
        data2 = 0;

        if (transition == OMX_StateInvalid || ret == OMX_ErrorInvalidState) {
            state = OMX_StateInvalid;
            LOGE("%s:%s: exit failure, transition from %s to %s, "
                 "current state is %s\n",
                 GetName(), GetWorkingRole(), GetStateName(current),
                 GetStateName(transition), GetStateName(state));
        }
    }

    callbacks->EventHandler(handle, appdata, event, data1, data2, NULL);

    /* WaitForResources workaround */
    if (ret == OMX_ErrorNone && transition == OMX_StateWaitForResources)
        callbacks->EventHandler(handle, appdata,
                                OMX_EventResourcesAcquired, 0, 0, NULL);
}

inline OMX_ERRORTYPE ComponentBase::TransStateToLoaded(OMX_STATETYPE current)
{
    OMX_ERRORTYPE ret;

    if (current == OMX_StateIdle) {
        OMX_U32 i;

        for (i = 0; i < nr_ports; i++)
	{
            if (ports[i]->GetPortBufferCount() > 0) {
                ports[i]->WaitPortBufferCompletion();
	    };
	};

        ret = ProcessorDeinit();
        if (ret != OMX_ErrorNone) {
            LOGE("%s:%s: ProcessorDeinit() failed "
                 "(ret : 0x%08x)\n", GetName(), GetWorkingRole(),
                 ret);
            goto out;
        }
    }
    else if (current == OMX_StateWaitForResources) {
        LOGV("%s:%s: "
             "state transition's requested from WaitForResources to Loaded\n",
             GetName(), GetWorkingRole());

        /*
         * from WaitForResources to Loaded considered from Loaded to Loaded.
         * do nothing
         */

        ret = OMX_ErrorNone;
    }
    else
        ret = OMX_ErrorIncorrectStateTransition;

out:
    return ret;
}

inline OMX_ERRORTYPE ComponentBase::TransStateToIdle(OMX_STATETYPE current)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if (current == OMX_StateLoaded) {
        OMX_U32 i;
        for (i = 0; i < nr_ports; i++) {
            if (ports[i]->IsEnabled()) {
                if (GetWorkingRole() != NULL &&
                        !strncmp (GetWorkingRole(),"video_decoder", 13 )) {
                    ret = ports[i]->WaitPortBufferCompletionTimeout(800);
                } else {
                    ports[i]->WaitPortBufferCompletion();
                }
            }
        }

        if (ret == OMX_ErrorNone) {
            ret = ProcessorInit();
        }
        if (ret != OMX_ErrorNone) {
            LOGE("%s:%s: ProcessorInit() failed (ret : 0x%08x)\n",
                 GetName(), GetWorkingRole(), ret);
            goto out;
        }
    }
    else if ((current == OMX_StatePause) || (current == OMX_StateExecuting)) {
        pthread_mutex_lock(&ports_block);
        FlushPort(OMX_ALL, 0);
        pthread_mutex_unlock(&ports_block);
        LOGV("%s:%s: flushed all ports\n", GetName(), GetWorkingRole());

        bufferwork->CancelScheduledWork(this);
        LOGV("%s:%s: discarded all scheduled buffer process work\n",
             GetName(), GetWorkingRole());

        if (current == OMX_StatePause) {
            bufferwork->ResumeWork();
            LOGV("%s:%s: buffer process work resumed\n",
                 GetName(), GetWorkingRole());
        }

        bufferwork->StopWork();
        LOGV("%s:%s: buffer process work stopped\n",
             GetName(), GetWorkingRole());

        ret = ProcessorStop();
        if (ret != OMX_ErrorNone) {
            LOGE("%s:%s: ProcessorStop() failed (ret : 0x%08x)\n",
                 GetName(), GetWorkingRole(), ret);
            goto out;
        }
    }
    else if (current == OMX_StateWaitForResources) {
        LOGV("%s:%s: "
             "state transition's requested from WaitForResources to Idle\n",
             GetName(), GetWorkingRole());

        /* same as Loaded to Idle BUT DO NOTHING for now */

        ret = OMX_ErrorNone;
    }
    else
        ret = OMX_ErrorIncorrectStateTransition;

out:
    return ret;
}

inline OMX_ERRORTYPE
ComponentBase::TransStateToExecuting(OMX_STATETYPE current)
{
    OMX_ERRORTYPE ret;

    if (current == OMX_StateIdle) {
        bufferwork->StartWork(true);
        LOGV("%s:%s: buffer process work started with executing state\n",
             GetName(), GetWorkingRole());

        ret = ProcessorStart();
        if (ret != OMX_ErrorNone) {
            LOGE("%s:%s: ProcessorStart() failed (ret : 0x%08x)\n",
                 GetName(), GetWorkingRole(), ret);
            goto out;
        }
    }
    else if (current == OMX_StatePause) {
        bufferwork->ResumeWork();
        LOGV("%s:%s: buffer process work resumed\n",
             GetName(), GetWorkingRole());

        ret = ProcessorResume();
        if (ret != OMX_ErrorNone) {
            LOGE("%s:%s: ProcessorResume() failed (ret : 0x%08x)\n",
                 GetName(), GetWorkingRole(), ret);
            goto out;
        }
    }
    else
        ret = OMX_ErrorIncorrectStateTransition;

out:
    return ret;
}

inline OMX_ERRORTYPE ComponentBase::TransStateToPause(OMX_STATETYPE current)
{
    OMX_ERRORTYPE ret;

    if (current == OMX_StateIdle) {
        bufferwork->StartWork(false);
        LOGV("%s:%s: buffer process work started with paused state\n",
             GetName(), GetWorkingRole());

        ret = ProcessorStart();
        if (ret != OMX_ErrorNone) {
            LOGE("%s:%s: ProcessorSart() failed (ret : 0x%08x)\n",
                 GetName(), GetWorkingRole(), ret);
            goto out;
        }
    }
    else if (current == OMX_StateExecuting) {
        bufferwork->PauseWork();
        LOGV("%s:%s: buffer process work paused\n",
             GetName(), GetWorkingRole());

        ret = ProcessorPause();
        if (ret != OMX_ErrorNone) {
            LOGE("%s:%s: ProcessorPause() failed (ret : 0x%08x)\n",
                 GetName(), GetWorkingRole(), ret);
            goto out;
        }
    }
    else
        ret = OMX_ErrorIncorrectStateTransition;

out:
    return ret;
}

inline OMX_ERRORTYPE ComponentBase::TransStateToInvalid(OMX_STATETYPE current)
{
    OMX_ERRORTYPE ret = OMX_ErrorInvalidState;
    LOGV("transit to invalid state from %d state",current);
    /*
     * Todo
     *   graceful escape
     */
    return ret;
}

inline OMX_ERRORTYPE
ComponentBase::TransStateToWaitForResources(OMX_STATETYPE current)
{
    OMX_ERRORTYPE ret;

    if (current == OMX_StateLoaded) {
        LOGV("%s:%s: "
             "state transition's requested from Loaded to WaitForResources\n",
             GetName(), GetWorkingRole());
        ret = OMX_ErrorNone;
    }
    else
        ret = OMX_ErrorIncorrectStateTransition;

    return ret;
}

/* mark buffer */
void ComponentBase::PushThisMark(OMX_U32 port_index, OMX_MARKTYPE *mark)
{
    PortBase *port = NULL;
    OMX_EVENTTYPE event;
    OMX_U32 data1, data2;
    OMX_ERRORTYPE ret;

    if (ports)
        if (port_index < nr_ports)
            port = ports[port_index];

    if (!port) {
        ret = OMX_ErrorBadPortIndex;
        goto notify_event;
    }

    ret = port->PushMark(mark);
    if (ret != OMX_ErrorNone) {
        /* don't report OMX_ErrorInsufficientResources */
        ret = OMX_ErrorUndefined;
        goto notify_event;
    }

notify_event:
    if (ret == OMX_ErrorNone) {
        event = OMX_EventCmdComplete;
        data1 = OMX_CommandMarkBuffer;
        data2 = port_index;
    }
    else {
        event = OMX_EventError;
        data1 = ret;
        data2 = 0;
    }

    callbacks->EventHandler(handle, appdata, event, data1, data2, NULL);
}

void ComponentBase::FlushPort(OMX_U32 port_index, bool notify)
{
    OMX_U32 i, from_index, to_index;

    if ((port_index != OMX_ALL) && (port_index > nr_ports-1))
        return;

    if (port_index == OMX_ALL) {
        from_index = 0;
        to_index = nr_ports - 1;
    }
    else {
        from_index = port_index;
        to_index = port_index;
    }

    LOGV("%s:%s: flush ports (from index %u to %u)\n",
         GetName(), GetWorkingRole(), from_index, to_index);

    for (i = from_index; i <= to_index; i++) {
        ports[i]->FlushPort();
        if (notify)
            callbacks->EventHandler(handle, appdata, OMX_EventCmdComplete,
                                    OMX_CommandFlush, i, NULL);
    }

    LOGV("%s:%s: flush ports done\n", GetName(), GetWorkingRole());
}

extern const char *GetPortStateName(OMX_U8 state); //portbase.cpp

void ComponentBase::TransStatePort(OMX_U32 port_index, OMX_U8 state)
{
    OMX_EVENTTYPE event;
    OMX_U32 data1, data2;
    OMX_U32 i, from_index, to_index;
    OMX_ERRORTYPE ret;

    if ((port_index != OMX_ALL) && (port_index > nr_ports-1))
        return;

    if (port_index == OMX_ALL) {
        from_index = 0;
        to_index = nr_ports - 1;
    }
    else {
        from_index = port_index;
        to_index = port_index;
    }

    LOGV("%s:%s: transit ports state to %s (from index %u to %u)\n",
         GetName(), GetWorkingRole(), GetPortStateName(state),
         from_index, to_index);

    pthread_mutex_lock(&ports_block);
    for (i = from_index; i <= to_index; i++) {
        ret = ports[i]->TransState(state);
        if (ret == OMX_ErrorNone) {
            event = OMX_EventCmdComplete;
            if (state == PortBase::OMX_PortEnabled) {
                data1 = OMX_CommandPortEnable;
                ProcessorReset();
            } else {
                data1 = OMX_CommandPortDisable;
            }
            data2 = i;
        } else {
            event = OMX_EventError;
            data1 = ret;
            data2 = 0;
        }
        callbacks->EventHandler(handle, appdata, event,
                                data1, data2, NULL);
    }
    pthread_mutex_unlock(&ports_block);

    LOGV("%s:%s: transit ports state to %s completed\n",
         GetName(), GetWorkingRole(), GetPortStateName(state));
}

/* set working role */
OMX_ERRORTYPE ComponentBase::SetWorkingRole(const OMX_STRING role)
{
    OMX_U32 i;

    if (state != OMX_StateUnloaded && state != OMX_StateLoaded)
        return OMX_ErrorIncorrectStateOperation;

    if (!role) {
        working_role = NULL;
        return OMX_ErrorNone;
    }

    for (i = 0; i < nr_roles; i++) {
        if (!strcmp((char *)&roles[i][0], role)) {
            working_role = (OMX_STRING)&roles[i][0];
            return OMX_ErrorNone;
        }
    }

    LOGE("%s: cannot find %s role\n", GetName(), role);
    return OMX_ErrorBadParameter;
}

/* apply a working role for a component having multiple roles */
OMX_ERRORTYPE ComponentBase::ApplyWorkingRole(void)
{
    OMX_U32 i;
    OMX_ERRORTYPE ret;

    if (state != OMX_StateUnloaded && state != OMX_StateLoaded)
        return OMX_ErrorIncorrectStateOperation;

    if (!working_role)
        return OMX_ErrorBadParameter;

    if (!callbacks || !appdata)
        return OMX_ErrorBadParameter;

    ret = AllocatePorts();
    if (ret != OMX_ErrorNone) {
        LOGE("failed to AllocatePorts() (ret = 0x%08x)\n", ret);
        return ret;
    }

    /* now we can access ports */
    for (i = 0; i < nr_ports; i++) {
        ports[i]->SetOwner(handle);
        ports[i]->SetCallbacks(handle, callbacks, appdata);
    }

    LOGI("%s: set working role %s:", GetName(), GetWorkingRole());
    return OMX_ErrorNone;
}

OMX_ERRORTYPE ComponentBase::AllocatePorts(void)
{
    OMX_DIRTYPE dir;
    bool has_input, has_output;
    OMX_U32 i;
    OMX_ERRORTYPE ret;

    if (ports)
        return OMX_ErrorBadParameter;

    ret = ComponentAllocatePorts();
    if (ret != OMX_ErrorNone) {
        LOGE("failed to %s::ComponentAllocatePorts(), ret = 0x%08x\n",
             name, ret);
        return ret;
    }

    has_input = false;
    has_output = false;
    ret = OMX_ErrorNone;
    for (i = 0; i < nr_ports; i++) {
        dir = ports[i]->GetPortDirection();
        if (dir == OMX_DirInput)
            has_input = true;
        else if (dir == OMX_DirOutput)
            has_output = true;
        else {
            ret = OMX_ErrorUndefined;
            break;
        }
    }
    if (ret != OMX_ErrorNone)
        goto free_ports;

    if ((has_input == false) && (has_output == true))
        cvariant = CVARIANT_SOURCE;
    else if ((has_input == true) && (has_output == true))
        cvariant = CVARIANT_FILTER;
    else if ((has_input == true) && (has_output == false))
        cvariant = CVARIANT_SINK;
    else
        goto free_ports;

    return OMX_ErrorNone;

free_ports:
    LOGE("%s(): exit, unknown component variant\n", __func__);
    FreePorts();
    return OMX_ErrorUndefined;
}

/* called int FreeHandle() */
OMX_ERRORTYPE ComponentBase::FreePorts(void)
{
    if (ports) {
        OMX_U32 i, this_nr_ports = this->nr_ports;

        for (i = 0; i < this_nr_ports; i++) {
            if (ports[i]) {
                OMX_MARKTYPE *mark;
                /* it should be empty before this */
                while ((mark = ports[i]->PopMark()))
                    free(mark);

                delete ports[i];
                ports[i] = NULL;
            }
        }
        delete []ports;
        ports = NULL;
    }

    return OMX_ErrorNone;
}

/* buffer processing */
/* implement WorkableInterface */
void ComponentBase::Work(void)
{
    OMX_BUFFERHEADERTYPE **buffers[nr_ports];
    OMX_BUFFERHEADERTYPE *buffers_hdr[nr_ports];
    OMX_BUFFERHEADERTYPE *buffers_org[nr_ports];
    buffer_retain_t retain[nr_ports];
    OMX_U32 i;
    OMX_ERRORTYPE ret;

    if (nr_ports == 0) {
        return;
    }

    memset(buffers, 0, sizeof(OMX_BUFFERHEADERTYPE *) * nr_ports);
    memset(buffers_hdr, 0, sizeof(OMX_BUFFERHEADERTYPE *) * nr_ports);
    memset(buffers_org, 0, sizeof(OMX_BUFFERHEADERTYPE *) * nr_ports);

    pthread_mutex_lock(&ports_block);

    while(IsAllBufferAvailable())
    {
        for (i = 0; i < nr_ports; i++) {
            buffers_hdr[i] = ports[i]->PopBuffer();
            buffers[i] = &buffers_hdr[i];
            buffers_org[i] = buffers_hdr[i];
            retain[i] = BUFFER_RETAIN_NOT_RETAIN;
        }

        if (working_role != NULL && !strncmp((char*)working_role, "video_decoder", 13)){
            ret = ProcessorProcess(buffers, &retain[0], nr_ports);
        }else{
            ret = ProcessorProcess(buffers_hdr, &retain[0], nr_ports);
        }

        if (ret == OMX_ErrorNone) {
            if (!working_role || (strncmp((char*)working_role, "video_encoder", 13) != 0))
                PostProcessBuffers(buffers, &retain[0]);

            for (i = 0; i < nr_ports; i++) {
                if (buffers_hdr[i] == NULL)
                    continue;

                if(retain[i] == BUFFER_RETAIN_GETAGAIN) {
                    ports[i]->RetainThisBuffer(*buffers[i], false);
                }
                else if (retain[i] == BUFFER_RETAIN_ACCUMULATE) {
                    ports[i]->RetainThisBuffer(*buffers[i], true);
                }
                else if (retain[i] == BUFFER_RETAIN_OVERRIDDEN) {
                    ports[i]->RetainAndReturnBuffer(buffers_org[i], *buffers[i]);
                }
                else if (retain[i] == BUFFER_RETAIN_CACHE) {
                    //nothing to do
                } else {
                    ports[i]->ReturnThisBuffer(*buffers[i]);
                }
            }
        }
        else {

            for (i = 0; i < nr_ports; i++) {
                if (buffers_hdr[i] == NULL)
                    continue;

                /* return buffers by hands, these buffers're not in queue */
                ports[i]->ReturnThisBuffer(*buffers[i]);
                /* flush ports */
                ports[i]->FlushPort();
            }

            callbacks->EventHandler(handle, appdata, OMX_EventError, ret,
                                    0, NULL);
        }
    }

    pthread_mutex_unlock(&ports_block);
}

bool ComponentBase::IsAllBufferAvailable(void)
{
    OMX_U32 i;
    OMX_U32 nr_avail = 0;

    for (i = 0; i < nr_ports; i++) {
        OMX_U32 length = 0;

        if (ports[i]->IsEnabled()) {
            length += ports[i]->BufferQueueLength();
            length += ports[i]->RetainedBufferQueueLength();
        }

        if (length)
            nr_avail++;
    }

    if (nr_avail == nr_ports)
        return true;
    else
        return false;
}

inline void ComponentBase::SourcePostProcessBuffers(
    OMX_BUFFERHEADERTYPE ***buffers)
{
    OMX_U32 i;

    for (i = 0; i < nr_ports; i++) {
        /*
         * in case of source component, buffers're marked when they come
         * from the ouput ports
         */
        if (!(*buffers[i])->hMarkTargetComponent) {
            OMX_MARKTYPE *mark;

            mark = ports[i]->PopMark();
            if (mark) {
                (*buffers[i])->hMarkTargetComponent = mark->hMarkTargetComponent;
                (*buffers[i])->pMarkData = mark->pMarkData;
                free(mark);
            }
        }
    }
}

inline void ComponentBase::FilterPostProcessBuffers(
    OMX_BUFFERHEADERTYPE ***buffers,
    const buffer_retain_t *retain)
{
    OMX_MARKTYPE *mark;
    OMX_U32 i, j;

    for (i = 0; i < nr_ports; i++) {
        if (ports[i]->GetPortDirection() == OMX_DirInput) {
            for (j = 0; j < nr_ports; j++) {
                if (ports[j]->GetPortDirection() != OMX_DirOutput)
                    continue;

                /* propagates EOS flag */
                /* clear input EOS at the end of this loop */
                if (retain[i] != BUFFER_RETAIN_GETAGAIN) {
                    if ((*buffers[i])->nFlags & OMX_BUFFERFLAG_EOS)
                        (*buffers[j])->nFlags |= OMX_BUFFERFLAG_EOS;
                }

                /* propagates marks */
                /*
                 * if hMarkTargetComponent == handle then the mark's not
                 * propagated
                 */
                if ((*buffers[i])->hMarkTargetComponent &&
                    ((*buffers[i])->hMarkTargetComponent != handle)) {
                    if ((*buffers[j])->hMarkTargetComponent) {
                        mark = (OMX_MARKTYPE *)malloc(sizeof(*mark));
                        if (mark) {
                            mark->hMarkTargetComponent =
                                (*buffers[i])->hMarkTargetComponent;
                            mark->pMarkData = (*buffers[i])->pMarkData;
                            ports[j]->PushMark(mark);
                            mark = NULL;
                            (*buffers[i])->hMarkTargetComponent = NULL;
                            (*buffers[i])->pMarkData = NULL;
                        }
                    }
                    else {
                        mark = ports[j]->PopMark();
                        if (mark) {
                            (*buffers[j])->hMarkTargetComponent =
                                mark->hMarkTargetComponent;
                            (*buffers[j])->pMarkData = mark->pMarkData;
                            free(mark);

                            mark = (OMX_MARKTYPE *)malloc(sizeof(*mark));
                            if (mark) {
                                mark->hMarkTargetComponent =
                                    (*buffers[i])->hMarkTargetComponent;
                                mark->pMarkData = (*buffers[i])->pMarkData;
                                ports[j]->PushMark(mark);
                                mark = NULL;
                                (*buffers[i])->hMarkTargetComponent = NULL;
                                (*buffers[i])->pMarkData = NULL;
                            }
                        }
                        else {
                            (*buffers[j])->hMarkTargetComponent =
                                (*buffers[i])->hMarkTargetComponent;
                            (*buffers[j])->pMarkData = (*buffers[i])->pMarkData;
                            (*buffers[i])->hMarkTargetComponent = NULL;
                            (*buffers[i])->pMarkData = NULL;
                        }
                    }
                }
            }
            /* clear input buffer's EOS */
            if (retain[i] != BUFFER_RETAIN_GETAGAIN)
                (*buffers[i])->nFlags &= ~OMX_BUFFERFLAG_EOS;
        }
    }
}

inline void ComponentBase::SinkPostProcessBuffers()
{
    return;
}

void ComponentBase::PostProcessBuffers(OMX_BUFFERHEADERTYPE ***buffers,
                                       const buffer_retain_t *retain)
{

    if (cvariant == CVARIANT_SOURCE)
        SourcePostProcessBuffers(buffers);
    else if (cvariant == CVARIANT_FILTER)
        FilterPostProcessBuffers(buffers, retain);
    else if (cvariant == CVARIANT_SINK) {
        SinkPostProcessBuffers();
    }
    else {
        LOGE("%s(): fatal error unknown component variant (%d)\n",
             __func__, cvariant);
    }
}

/* processor default callbacks */
OMX_ERRORTYPE ComponentBase::ProcessorInit(void)
{
    return OMX_ErrorNone;
}
OMX_ERRORTYPE ComponentBase::ProcessorDeinit(void)
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE ComponentBase::ProcessorStart(void)
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE ComponentBase::ProcessorReset(void)
{
    return OMX_ErrorNone;
}


OMX_ERRORTYPE ComponentBase::ProcessorStop(void)
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE ComponentBase::ProcessorPause(void)
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE ComponentBase::ProcessorResume(void)
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE ComponentBase::ProcessorFlush(OMX_U32)
{
    return OMX_ErrorNone;
}
OMX_ERRORTYPE ComponentBase::ProcessorPreFillBuffer(OMX_BUFFERHEADERTYPE*)
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE ComponentBase::ProcessorPreEmptyBuffer(OMX_BUFFERHEADERTYPE*)
{
    return OMX_ErrorNone;
}
OMX_ERRORTYPE ComponentBase::ProcessorProcess(OMX_BUFFERHEADERTYPE **,
                                           buffer_retain_t *,
                                           OMX_U32)
{
    LOGE("ProcessorProcess not be implemented");
    return OMX_ErrorNotImplemented;
}
OMX_ERRORTYPE ComponentBase::ProcessorProcess(OMX_BUFFERHEADERTYPE ***,
                                           buffer_retain_t *,
                                           OMX_U32)
{
    LOGE("ProcessorProcess not be implemented");
    return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE ComponentBase::ProcessorPreFreeBuffer(OMX_U32, OMX_BUFFERHEADERTYPE*)
{
    return OMX_ErrorNone;

}
/* end of processor callbacks */

/* helper for derived class */
OMX_STRING ComponentBase::GetWorkingRole(void)
{
    return &working_role[0];
}

const OMX_COMPONENTTYPE *ComponentBase::GetComponentHandle(void)
{
    return handle;
}
#if 0
void ComponentBase::DumpBuffer(const OMX_BUFFERHEADERTYPE *bufferheader,
                               bool dumpdata)
{
    OMX_U8 *pbuffer = bufferheader->pBuffer, *p;
    OMX_U32 offset = bufferheader->nOffset;
    OMX_U32 alloc_len = bufferheader->nAllocLen;
    OMX_U32 filled_len =  bufferheader->nFilledLen;
    OMX_U32 left = filled_len, oneline;
    OMX_U32 index = 0, i;
    /* 0x%04lx:  %02x %02x .. (n = 16)\n\0 */
    char prbuffer[8 + 3 * 0x10 + 2], *pp;
    OMX_U32 prbuffer_len;

    LOGD("Component %s DumpBuffer\n", name);
    LOGD("%s port index = %lu",
         (bufferheader->nInputPortIndex != 0x7fffffff) ? "input" : "output",
         (bufferheader->nInputPortIndex != 0x7fffffff) ?
         bufferheader->nInputPortIndex : bufferheader->nOutputPortIndex);
    LOGD("nAllocLen = %lu, nOffset = %lu, nFilledLen = %lu\n",
         alloc_len, offset, filled_len);
    LOGD("nTimeStamp = %lld, nTickCount = %lu",
         bufferheader->nTimeStamp,
         bufferheader->nTickCount);
    LOGD("nFlags = 0x%08lx\n", bufferheader->nFlags);
    LOGD("hMarkTargetComponent = %p, pMarkData = %p\n",
         bufferheader->hMarkTargetComponent, bufferheader->pMarkData);

    if (!pbuffer || !alloc_len || !filled_len)
        return;

    if (offset + filled_len > alloc_len)
        return;

    if (!dumpdata)
        return;

    p = pbuffer + offset;
    while (left) {
        oneline = left > 0x10 ? 0x10 : left; /* 16 items per 1 line */
        pp += sprintf(pp, "0x%04lx: ", index);
        for (i = 0; i < oneline; i++)
            pp += sprintf(pp, " %02x", *(p + i));
        pp += sprintf(pp, "\n");
        *pp = '\0';

        index += 0x10;
        p += oneline;
        left -= oneline;

        pp = &prbuffer[0];
        LOGD("%s", pp);
    }
}
#endif
/* end of component methods & helpers */

/*
 * omx header manipuation
 */
void ComponentBase::SetTypeHeader(OMX_PTR type, OMX_U32 size)
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

OMX_ERRORTYPE ComponentBase::CheckTypeHeader(const OMX_PTR type, OMX_U32 size)
{
    OMX_U32 *nsize;
    OMX_VERSIONTYPE *nversion;

    if (!type)
        return OMX_ErrorBadParameter;

    nsize = (OMX_U32 *)type;
    nversion = (OMX_VERSIONTYPE *)((OMX_U8 *)type + sizeof(OMX_U32));

    if (*nsize != size)
        return OMX_ErrorBadParameter;

    if (nversion->nVersion != OMX_SPEC_VERSION)
        return OMX_ErrorVersionMismatch;

    return OMX_ErrorNone;
}

/* end of ComponentBase */
