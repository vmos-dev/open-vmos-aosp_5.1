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

#define LOG_NDEBUG 1
#define LOG_TAG "OMXComponentCodecBase"
#include <wrs_omxil_core/log.h>
#include "OMXComponentCodecBase.h"


OMXComponentCodecBase::OMXComponentCodecBase()
    : mHandlerList(NULL) {
    pthread_mutex_init(&mSerializationLock, NULL);
}

OMXComponentCodecBase::~OMXComponentCodecBase(){
    HandlerEntry *p = NULL;
    while (mHandlerList) {
        p = mHandlerList->next;
        delete mHandlerList;
        mHandlerList = p;
    }

    if (this->ports) {
        delete this->ports;
        this->ports = NULL;
    }

    pthread_mutex_destroy(&mSerializationLock);
}

OMX_ERRORTYPE OMXComponentCodecBase::ComponentAllocatePorts(void) {
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    this->ports = new PortBase* [NUMBER_PORTS];
    if (this->ports == NULL) {
        return OMX_ErrorInsufficientResources;
    }

    ret = InitInputPort();
    CHECK_RETURN_VALUE("InitInputPort");

    ret = InitOutputPort();
    CHECK_RETURN_VALUE("InitOutputPort");

    this->nr_ports = NUMBER_PORTS;

    // OMX_PORT_PARAM_TYPE
    // Return to OMX client through index OMX_IndexParamVideoInit/OMX_IndexParamAudioInit
    // TODO: replace portparam with mPortParam
    memset(&this->portparam, 0, sizeof(this->portparam));
    SetTypeHeader(&this->portparam, sizeof(this->portparam));
    this->portparam.nPorts = NUMBER_PORTS;
    this->portparam.nStartPortNumber = INPORT_INDEX;

    return ret;
}


OMX_ERRORTYPE OMXComponentCodecBase::ComponentGetParameter(
    OMX_INDEXTYPE nIndex,
    OMX_PTR pComponentParameterStructure) {

    OMXHANDLER handler = FindHandler(nIndex, true);
    if (handler == NULL) {
        LOGE("ComponentGetParameter: No handler for index %d", nIndex);
        return OMX_ErrorUnsupportedIndex;
    }

    LOGV("ComponentGetParameter: Index = 0x%x", nIndex);
    return (*handler)(this, pComponentParameterStructure);
}

OMX_ERRORTYPE OMXComponentCodecBase::ComponentSetParameter(
    OMX_INDEXTYPE nIndex,
    OMX_PTR pComponentParameterStructure) {

    OMXHANDLER handler = FindHandler(nIndex, false);
    if (handler == NULL) {
        LOGE("ComponentSetParameter: No handler for index %d", nIndex);
        return OMX_ErrorUnsupportedIndex;
    }

    LOGV("ComponentSetParameter: Index = 0x%x", nIndex);
    return (*handler)(this, pComponentParameterStructure);
}

OMX_ERRORTYPE OMXComponentCodecBase::ComponentGetConfig(
    OMX_INDEXTYPE nIndex,
    OMX_PTR pComponentConfigStructure) {

    OMXHANDLER handler = FindHandler(nIndex, true);
    if (handler == NULL) {
        LOGE("ComponentGetConfig: No handler for index %d", nIndex);
        return OMX_ErrorUnsupportedIndex;
    }

    LOGV("ComponentGetConfig: Index = 0x%x", nIndex);
    return (*handler)(this, pComponentConfigStructure);
}

OMX_ERRORTYPE OMXComponentCodecBase::ComponentSetConfig(
    OMX_INDEXTYPE nIndex,
    OMX_PTR pComponentConfigStructure) {

    OMX_ERRORTYPE ret = OMX_ErrorNone;

    OMXHANDLER handler = FindHandler(nIndex, false);
    if (handler == NULL) {
        LOGE("ComponentSetConfig: No handler for index %d", nIndex);
        return OMX_ErrorUnsupportedIndex;
    }

    LOGV("ComponentSetConfig: Index = 0x%x", nIndex);
    pthread_mutex_lock(&mSerializationLock);
    ret = (*handler)(this, pComponentConfigStructure);
    pthread_mutex_unlock(&mSerializationLock);
    return ret;
}

OMX_ERRORTYPE OMXComponentCodecBase::ProcessorInit(void) {
    LOGV("OMXComponentCodecBase::ProcessorInit");

    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXComponentCodecBase::ProcessorDeinit(void) {
    LOGV("OMXComponentCodecBase::ProcessorDeinit");
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXComponentCodecBase::ProcessorStart(void) {
    LOGV("OMXComponentCodecBase::ProcessorStart");
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXComponentCodecBase::ProcessorStop(void) {
    LOGV("OMXComponentCodecBase::ProcessorStop");
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXComponentCodecBase::ProcessorPause(void) {
    LOGV("OMXComponentCodecBase::ProcessorPause");
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXComponentCodecBase::ProcessorResume(void) {
    LOGV("OMXComponentCodecBase::ProcessorResume");
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXComponentCodecBase::AddHandler(
        OMX_INDEXTYPE type,
        OMXComponentCodecBase::OMXHANDLER getter,
        OMXComponentCodecBase::OMXHANDLER setter) {

    HandlerEntry *p = mHandlerList;
    HandlerEntry *last = NULL;
    while (p) {
        if (p->type == type) {
            p->getter = getter;
            p->setter = setter;
            return OMX_ErrorNone;
        }
        last = p;
        p = p->next;
    }
    p = new HandlerEntry;
    if (p == NULL) {
        return OMX_ErrorInsufficientResources;
    }
    p->type = type;
    p->getter = getter;
    p->setter = setter;
    p->next = NULL;

    if (last) {
        last->next = p;
    } else {
        mHandlerList = p;
    }
    return OMX_ErrorNone;
}

OMXComponentCodecBase::OMXHANDLER OMXComponentCodecBase::FindHandler(OMX_INDEXTYPE type, bool get) {
    HandlerEntry *p = mHandlerList;
    while (p) {
        if (p->type == type) {
            return get ? p->getter : p->setter;
        }
        p = p->next;
    }
    return NULL;
}

OMX_ERRORTYPE OMXComponentCodecBase::BuildHandlerList(void) {
    return OMX_ErrorNone;
}


