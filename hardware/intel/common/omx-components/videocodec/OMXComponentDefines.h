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

#ifndef OMX_COMPONENT_DEFINES_H_
#define OMX_COMPONENT_DEFINES_H_



#define DECLARE_HANDLER(CLASS, FUNC)\
    static OMX_ERRORTYPE Get##FUNC(void *inst, OMX_PTR pStructure) {\
        return ((CLASS*)inst)->Get##FUNC(pStructure);\
    }\
    static OMX_ERRORTYPE Set##FUNC(void *inst, OMX_PTR pStructure) {\
        return ((CLASS*)inst)->Set##FUNC(pStructure);\
    }\
    OMX_ERRORTYPE Get##FUNC(OMX_PTR pStructure);\
    OMX_ERRORTYPE Set##FUNC(OMX_PTR pStructure);

#define CHECK_TYPE_HEADER(P)\
    ret = CheckTypeHeader((P), sizeof(*(P)));\
    if (ret != OMX_ErrorNone) {\
        LOGE("Invalid type header.");\
        return ret;\
    }

#define CHECK_PORT_INDEX(P, INDEX)\
    if ((P)->nPortIndex != INDEX) {\
        LOGE("Bad port index %u, expected: %d", (P)->nPortIndex, INDEX);\
        return OMX_ErrorBadPortIndex;\
    }

#define CHECK_ENUMERATION_RANGE(INDEX, RANGE)\
    if (INDEX >= RANGE) {\
        LOGE("No more enumeration.");\
        return OMX_ErrorNoMore;\
    }

#define CHECK_PORT_INDEX_RANGE(P)\
    if ((P)->nPortIndex != 0 && (P)->nPortIndex != 1) {\
        LOGE("Port out of range %u", (P)->nPortIndex);\
        return OMX_ErrorBadPortIndex;\
    }

#define CHECK_RETURN_VALUE(FUNC)\
    if (ret != OMX_ErrorNone) {\
        LOGE(FUNC" failed: Error code = 0x%x", ret);\
        return ret;\
    }

#define CHECK_SET_PARAM_STATE()\
    OMX_STATETYPE state;\
    CBaseGetState((void *)GetComponentHandle(), &state);\
    if (state != OMX_StateLoaded && state != OMX_StateWaitForResources) {\
        LOGE("Invalid state to set param.");\
        return OMX_ErrorIncorrectStateOperation;\
    }

#define CHECK_SET_CONFIG_STATE()\
    OMX_STATETYPE state;\
    CBaseGetState((void *)GetComponentHandle(), &state);\
    if (state == OMX_StateLoaded || state == OMX_StateWaitForResources) {\
        LOGE("Invalid state to set config");\
        return OMX_ErrorNone;\
    }

#define CHECK_BS_STATE() \
    if (mBsState == BS_STATE_EXECUTING) { \
        LOGE("Wrong state"); \
        return OMX_ErrorUndefined; \
    }

#define CHECK_BS_STATUS(FUNC) \
    if (ret != BS_SUCCESS) { \
        LOGE(FUNC" Failed. ret = 0x%08x\n", ret); \
        return OMX_ErrorUndefined; \
    }

#define CHECK_STATUS(FUNC) \
    if (ret != OMX_ErrorNone) { \
        LOGE(FUNC" Failed. ret = 0x%08x\n", ret); \
        return ret; \
    }

#define CHECK_ENCODE_STATUS(FUNC)\
    if (ret < ENCODE_SUCCESS) { \
        LOGE(FUNC" Failed. ret = 0x%08x\n", ret); \
        return OMX_ErrorUndefined; \
    }

#define DECLARE_OMX_COMPONENT(NAME, ROLE, CLASS) \
    static const char *gName = (const char *)(NAME);\
    static const char *gRole = (const char *)(ROLE);\
    OMX_ERRORTYPE CreateInstance(OMX_PTR *instance) {\
        *instance = NULL;\
        ComponentBase *inst = new CLASS;\
        if (!inst) {\
            return OMX_ErrorInsufficientResources;\
        }\
        *instance = inst;\
        return OMX_ErrorNone;\
    }\
    struct wrs_omxil_cmodule_ops_s gOps = {CreateInstance};\
    struct wrs_omxil_cmodule_s WRS_OMXIL_CMODULE_SYMBOL = {gName, &gRole, 1, &gOps};

#endif /* OMX_COMPONENT_DEFINES_H_ */
