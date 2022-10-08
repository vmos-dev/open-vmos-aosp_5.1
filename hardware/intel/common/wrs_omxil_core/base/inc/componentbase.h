/*
 * componentbase.h, component base class
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

#ifndef __COMPONENTBASE_H
#define __COMPONENTBASE_H

#include <OMX_Core.h>
#include <OMX_Component.h>
#include <OMX_IntelIndexExt.h>
#include <cmodule.h>
#include <portbase.h>

#include <queue.h>
#include <workqueue.h>

/* retain buffers */
typedef enum buffer_retain_e {
    BUFFER_RETAIN_NOT_RETAIN = 0,
    BUFFER_RETAIN_GETAGAIN,
    BUFFER_RETAIN_ACCUMULATE,
    BUFFER_RETAIN_OVERRIDDEN,
    BUFFER_RETAIN_CACHE,
} buffer_retain_t;

/* ProcessCmdWork */
struct cmd_s {
    OMX_COMMANDTYPE cmd;
    OMX_U32 param1;
    OMX_PTR cmddata;
};

class CmdHandlerInterface
{
public:
    virtual ~CmdHandlerInterface() {};
    virtual void CmdHandler(struct cmd_s *cmd) = 0;
};

class CmdProcessWork : public WorkableInterface
{
public:
    CmdProcessWork(CmdHandlerInterface *ci);
    ~CmdProcessWork();

    OMX_ERRORTYPE PushCmdQueue(struct cmd_s *cmd);

private:
    struct cmd_s *PopCmdQueue(void);

    virtual void Work(void); /* call ci->CmdHandler() */

    WorkQueue *workq;

    struct queue q;
    pthread_mutex_t lock;

    CmdHandlerInterface *ci; /* to run ComponentBase::CmdHandler() */
};

class ComponentBase : public CmdHandlerInterface, public WorkableInterface
{
public:
    /*
     * constructor & destructor
     */
    ComponentBase();
    ComponentBase(const OMX_STRING name);
    virtual ~ComponentBase();

    /*
     * accessor
     */
    /* name */
    void SetName(const OMX_STRING name);
    OMX_STRING GetName(void);

    /* working role */
    OMX_STRING GetWorkingRole(void);

    /* cmodule */
    void SetCModule(CModule *cmodule);
    CModule *GetCModule(void);

    /* end of accessor */

    /*
     * core methods & helpers
     */
    /* roles */
    OMX_ERRORTYPE SetRolesOfComponent(OMX_U32 nr_roles, const OMX_U8 **roles);

    /* GetHandle & FreeHandle */
    OMX_ERRORTYPE GetHandle(OMX_HANDLETYPE* pHandle,
                            OMX_PTR pAppData,
                            OMX_CALLBACKTYPE *pCallBacks);
    OMX_ERRORTYPE FreeHandle(OMX_HANDLETYPE hComponent);

    /* end of core methods & helpers */

    /*
     * component methods & helpers
     */

    static OMX_ERRORTYPE SendCommand(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_COMMANDTYPE Cmd,
        OMX_IN  OMX_U32 nParam1,
        OMX_IN  OMX_PTR pCmdData);
    OMX_ERRORTYPE CBaseSendCommand(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_COMMANDTYPE Cmd,
        OMX_IN  OMX_U32 nParam1,
        OMX_IN  OMX_PTR pCmdData);

    static OMX_ERRORTYPE GetParameter(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_INDEXTYPE nParamIndex,
        OMX_INOUT OMX_PTR pComponentParameterStructure);
    OMX_ERRORTYPE CBaseGetParameter(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_INDEXTYPE nParamIndex,
        OMX_INOUT OMX_PTR pComponentParameterStructure);

    static OMX_ERRORTYPE SetParameter(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_INDEXTYPE nIndex,
        OMX_IN  OMX_PTR pComponentParameterStructure);
    OMX_ERRORTYPE CBaseSetParameter(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_INDEXTYPE nIndex,
        OMX_IN  OMX_PTR pComponentParameterStructure);

    static OMX_ERRORTYPE GetConfig(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_INDEXTYPE nIndex,
        OMX_INOUT OMX_PTR pComponentConfigStructure);
    OMX_ERRORTYPE CBaseGetConfig(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_INDEXTYPE nIndex,
        OMX_INOUT OMX_PTR pComponentConfigStructure);

    static OMX_ERRORTYPE SetConfig(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_INDEXTYPE nIndex,
        OMX_IN  OMX_PTR pComponentConfigStructure);
    OMX_ERRORTYPE CBaseSetConfig(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_INDEXTYPE nIndex,
        OMX_IN  OMX_PTR pComponentConfigStructure);

    static OMX_ERRORTYPE GetExtensionIndex(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_STRING cParameterName,
        OMX_OUT OMX_INDEXTYPE* pIndexType);
    OMX_ERRORTYPE CBaseGetExtensionIndex(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_STRING cParameterName,
        OMX_OUT OMX_INDEXTYPE* pIndexType);

    static OMX_ERRORTYPE GetState(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_OUT OMX_STATETYPE* pState);
    OMX_ERRORTYPE CBaseGetState(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_OUT OMX_STATETYPE* pState);

    static OMX_ERRORTYPE UseBuffer(
        OMX_IN OMX_HANDLETYPE hComponent,
        OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
        OMX_IN OMX_U32 nPortIndex,
        OMX_IN OMX_PTR pAppPrivate,
        OMX_IN OMX_U32 nSizeBytes,
        OMX_IN OMX_U8* pBuffer);
    OMX_ERRORTYPE CBaseUseBuffer(
        OMX_IN OMX_HANDLETYPE hComponent,
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
    OMX_ERRORTYPE CBaseAllocateBuffer(
        OMX_IN OMX_HANDLETYPE hComponent,
        OMX_INOUT OMX_BUFFERHEADERTYPE** ppBuffer,
        OMX_IN OMX_U32 nPortIndex,
        OMX_IN OMX_PTR pAppPrivate,
        OMX_IN OMX_U32 nSizeBytes);

    static OMX_ERRORTYPE FreeBuffer(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_U32 nPortIndex,
        OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);
    OMX_ERRORTYPE CBaseFreeBuffer(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_U32 nPortIndex,
        OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);

    static OMX_ERRORTYPE EmptyThisBuffer(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);
    OMX_ERRORTYPE CBaseEmptyThisBuffer(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);

    static OMX_ERRORTYPE FillThisBuffer(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);
    OMX_ERRORTYPE CBaseFillThisBuffer(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);

    static OMX_ERRORTYPE SetCallbacks(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_CALLBACKTYPE* pCallbacks,
        OMX_IN  OMX_PTR pAppData);
    OMX_ERRORTYPE CBaseSetCallbacks(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_CALLBACKTYPE* pCallbacks,
        OMX_IN  OMX_PTR pAppData);

    static OMX_ERRORTYPE ComponentRoleEnum(
        OMX_IN OMX_HANDLETYPE hComponent,
        OMX_OUT OMX_U8 *cRole,
        OMX_IN OMX_U32 nIndex);
    OMX_ERRORTYPE CBaseComponentRoleEnum(
        OMX_IN OMX_HANDLETYPE hComponent,
        OMX_OUT OMX_U8 *cRole,
        OMX_IN OMX_U32 nIndex);

    /* end of component methods & helpers */

    /*
     * omx header manipuation
     */
    static void SetTypeHeader(OMX_PTR type, OMX_U32 size);
    static OMX_ERRORTYPE CheckTypeHeader(const OMX_PTR type, OMX_U32 size);

    /* end of omx header manipuation */

    /*
     * helper method for queury_roles()
     */
    static OMX_ERRORTYPE QueryRolesHelper(OMX_U32 nr_comp_roles,
                                          const OMX_U8 **comp_roles,
                                          OMX_U32 *nr_roles, OMX_U8 **roles);

    /* end of helper method for queury_roles() */

protected:
    /* helpers for derived class */
    const OMX_COMPONENTTYPE *GetComponentHandle(void);
#if 0
    void DumpBuffer(const OMX_BUFFERHEADERTYPE *bufferheader, bool dumpdata);
#endif
    /* check if all port has own pending buffer */
    virtual bool IsAllBufferAvailable(void);

    /* end of helpers for derived class */

    /* ports */
    /*
     * allocated with derived port classes by derived component classes
     */
    PortBase **ports;
    OMX_U32 nr_ports;
    OMX_PORT_PARAM_TYPE portparam;

    /* ports big lock, must be held when accessing all ports at one time */
    pthread_mutex_t ports_block;

    /* adaptive playback param */
    OMX_BOOL mEnableAdaptivePlayback;
private:
    /* common routines for constructor */
    void __ComponentBase(void);

    /*
     * core methods & helpers
     */
    /* called in GetHandle (nr_roles == 1) or SetParameter(ComponentRole) */
    OMX_ERRORTYPE SetWorkingRole(const OMX_STRING role);
    /* called int FreeHandle() */
    OMX_ERRORTYPE FreePorts(void);

    /* end of core methods & helpers */

    /*
     * component methods & helpers
     */
    /* SendCommand */
    /* implement CmdHandlerInterface */
    virtual void CmdHandler(struct cmd_s *cmd);

    /* SendCommand:OMX_CommandStateSet */
    /* called in CmdHandler() thread context or by component itself */
    void TransState(OMX_STATETYPE transition);
    inline OMX_ERRORTYPE TransStateToLoaded(OMX_STATETYPE current);
    inline OMX_ERRORTYPE TransStateToIdle(OMX_STATETYPE current);
    inline OMX_ERRORTYPE TransStateToExecuting(OMX_STATETYPE current);
    inline OMX_ERRORTYPE TransStateToPause(OMX_STATETYPE current);
    inline OMX_ERRORTYPE TransStateToWaitForResources(OMX_STATETYPE current);
    inline OMX_ERRORTYPE TransStateToInvalid(OMX_STATETYPE current);

    /* called in TransStateToIdle(Loaded) */
    OMX_ERRORTYPE ApplyWorkingRole(void);
    /* called in ApplyWorkingRole() */
    OMX_ERRORTYPE AllocatePorts(void);
    /* allocate specific port type derived from portbase */
    virtual OMX_ERRORTYPE ComponentAllocatePorts(void) = 0;

    /* SendCommand:OMX_CommandMarkBuffer */
    /* called in CmdHandler() thread context */
    void PushThisMark(OMX_U32 portindex, OMX_MARKTYPE *mark);
    /* SendCommand:OMX_CommandFlush (notify:1) or other parts (notify:0) */
    void FlushPort(OMX_U32 port_index, bool notify);
    /* SendCommand:OMX_CommandPortDisable/Enable */
    /* state: PortBase::OMX_PortEnabled/Disabled */
    void TransStatePort(OMX_U32 port_index, OMX_U8 state);

    /* Get/SetParameter */
    virtual OMX_ERRORTYPE
        ComponentGetParameter(OMX_INDEXTYPE nParamIndex,
                              OMX_PTR pComponentParameterStructure) = 0;
    virtual OMX_ERRORTYPE
        ComponentSetParameter(OMX_INDEXTYPE nIndex,
                              OMX_PTR pComponentParameterStructure) = 0;

    /* Get/SetConfig */
    virtual OMX_ERRORTYPE
        ComponentGetConfig(OMX_INDEXTYPE nIndex,
                           OMX_PTR pComponentConfigStructure) = 0;
    virtual OMX_ERRORTYPE
        ComponentSetConfig(OMX_INDEXTYPE nIndex,
                           OMX_PTR pComponentConfigStructure) = 0;

    /* buffer processing */
    /* implement WorkableInterface */
    virtual void Work(void); /* handle this->ports, hold ports_block */

    /* called in Work() after ProcessorProcess() */
    void PostProcessBuffers(OMX_BUFFERHEADERTYPE ***buffers,
                            const buffer_retain_t *retain);
    void SourcePostProcessBuffers(OMX_BUFFERHEADERTYPE ***buffers);
    void FilterPostProcessBuffers(OMX_BUFFERHEADERTYPE ***buffers,
                                  const buffer_retain_t *retain);
    void SinkPostProcessBuffers();

    /* processor callbacks */
    /* TransState */
    virtual OMX_ERRORTYPE ProcessorInit(void);  /* Loaded to Idle */
    virtual OMX_ERRORTYPE ProcessorDeinit(void);/* Idle to Loaded */
    virtual OMX_ERRORTYPE ProcessorStart(void); /* Idle to Executing/Pause */
    virtual OMX_ERRORTYPE ProcessorReset(void); /* Reset */
    virtual OMX_ERRORTYPE ProcessorStop(void);  /* Executing/Pause to Idle */
    virtual OMX_ERRORTYPE ProcessorPause(void); /* Executing to Pause */
    virtual OMX_ERRORTYPE ProcessorResume(void);/* Pause to Executing */
    virtual OMX_ERRORTYPE ProcessorFlush(OMX_U32 port_index); /* Flush */
    virtual OMX_ERRORTYPE ProcessorPreFillBuffer(OMX_BUFFERHEADERTYPE* buffer);
    virtual OMX_ERRORTYPE ProcessorPreEmptyBuffer(OMX_BUFFERHEADERTYPE* buffer);

    /* invoked when buffer is to be freed */
    virtual OMX_ERRORTYPE ProcessorPreFreeBuffer(OMX_U32 nPortIndex, OMX_BUFFERHEADERTYPE* pBuffer);
    /* Work */
    virtual OMX_ERRORTYPE ProcessorProcess(OMX_BUFFERHEADERTYPE ***pBuffers,
                                           buffer_retain_t *retain,
                                           OMX_U32 nr_buffers);
    virtual OMX_ERRORTYPE ProcessorProcess(OMX_BUFFERHEADERTYPE **pBuffers,
                                           buffer_retain_t *retain,
                                           OMX_U32 nr_buffers);

    /* end of component methods & helpers */

    /* process component's commands work */
    CmdProcessWork *cmdwork;

    /* buffer processing work */
    WorkQueue *bufferwork;

    /* component variant */
    typedef enum component_variant_e {
        CVARIANT_NULL = 0,
        CVARIANT_SOURCE,
        CVARIANT_FILTER,
        CVARIANT_SINK,
    } component_variant_t;

    component_variant_t cvariant;

    /* roles */
    OMX_U8 **roles;
    OMX_U32 nr_roles;

    OMX_STRING working_role;

    /* omx standard handle */
    /* allocated at GetHandle, freed at FreeHandle */
    OMX_COMPONENTTYPE *handle;

    /* component module */
    CModule *cmodule;

    OMX_STATETYPE state;

    const static OMX_STATETYPE OMX_StateUnloaded = OMX_StateVendorStartUnused;

    /* omx standard callbacks */
    OMX_PTR appdata;
    OMX_CALLBACKTYPE *callbacks;

    /* component name */
    char name[OMX_MAX_STRINGNAME_SIZE];

    /* state lock */
    pthread_mutex_t state_block;

    OMX_U32 mMaxFrameWidth;
    OMX_U32 mMaxFrameHeight;

    /* omx specification version */
#ifndef ANDROID
    const static OMX_U8 OMX_SPEC_VERSION_MAJOR = 1;
    const static OMX_U8 OMX_SPEC_VERSION_MINOR = 1;
    const static OMX_U8 OMX_SPEC_VERSION_REVISION = 2;
    const static OMX_U8 OMX_SPEC_VERSION_STEP = 0;
#else
    const static OMX_U8 OMX_SPEC_VERSION_MAJOR = 1;
    const static OMX_U8 OMX_SPEC_VERSION_MINOR = 0;
    const static OMX_U8 OMX_SPEC_VERSION_REVISION = 0;
    const static OMX_U8 OMX_SPEC_VERSION_STEP = 0;
#endif

    const static OMX_U32 OMX_SPEC_VERSION = 0
        | (OMX_SPEC_VERSION_MAJOR << 0)
        | (OMX_SPEC_VERSION_MINOR << 8)
        | (OMX_SPEC_VERSION_REVISION << 16)
        | (OMX_SPEC_VERSION_STEP << 24);
};

#endif /* __COMPONENTBASE_H */
