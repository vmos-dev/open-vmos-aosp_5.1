/*
* Copyright (c) 2009-2011 Intel Corporation.  All rights reserved.
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

#ifndef OMX_COMPONENT_CODEC_BASE_H_
#define OMX_COMPONENT_CODEC_BASE_H_

#include <unistd.h>

#include <OMX_Core.h>
#include <OMX_Component.h>
#include <OMX_IndexExt.h>
#include <OMX_IntelErrorTypes.h>

#include <portbase.h>
#include <portvideo.h>

#include <componentbase.h>
#include "OMXComponentDefines.h"
#include <HardwareAPI.h>

class OMXComponentCodecBase : public ComponentBase {
public:
    OMXComponentCodecBase();
    virtual ~OMXComponentCodecBase();

protected:
    virtual OMX_ERRORTYPE ComponentAllocatePorts(void);
    virtual OMX_ERRORTYPE InitInputPort(void) = 0;
    virtual OMX_ERRORTYPE InitOutputPort(void) = 0;

    virtual OMX_ERRORTYPE ComponentGetParameter(
            OMX_INDEXTYPE nIndex,
            OMX_PTR pComponentParameterStructure);

    virtual OMX_ERRORTYPE ComponentSetParameter(
            OMX_INDEXTYPE nIndex,
            OMX_PTR pComponentParameterStructure);

    virtual OMX_ERRORTYPE ComponentGetConfig(
            OMX_INDEXTYPE nIndex,
            OMX_PTR pComponentConfigStructure);

    virtual OMX_ERRORTYPE ComponentSetConfig(
            OMX_INDEXTYPE nIndex,
            OMX_PTR pComponentConfigStructure);

    virtual OMX_ERRORTYPE ProcessorInit(void);  /* Loaded to Idle */
    virtual OMX_ERRORTYPE ProcessorDeinit(void);/* Idle to Loaded */
    virtual OMX_ERRORTYPE ProcessorStart(void); /* Idle to Executing/Pause */
    virtual OMX_ERRORTYPE ProcessorStop(void);  /* Executing/Pause to Idle */
    virtual OMX_ERRORTYPE ProcessorPause(void); /* Executing to Pause */
    virtual OMX_ERRORTYPE ProcessorResume(void);/* Pause to Executing */

    // Derived class must implement  ProcessorFlush and ProcessorProcess

    enum {
        INPORT_INDEX = 0,
        OUTPORT_INDEX = 1,
        NUMBER_PORTS = 2,
    };

    typedef OMX_ERRORTYPE (*OMXHANDLER)(void *inst, OMX_PTR p);
    virtual OMX_ERRORTYPE AddHandler(OMX_INDEXTYPE type, OMXHANDLER getter, OMXHANDLER setter);
    virtual OMX_ERRORTYPE BuildHandlerList(void);

private:
    // return getter or setter
    OMXHANDLER FindHandler(OMX_INDEXTYPE type, bool get);

protected:
    pthread_mutex_t mSerializationLock;

private:
    struct HandlerEntry {
        OMX_INDEXTYPE type;
        OMXHANDLER getter;
        OMXHANDLER setter;
        HandlerEntry *next;
    };

    HandlerEntry *mHandlerList;
};

#endif /* OMX_COMPONENT_CODEC_BASE_H_ */
