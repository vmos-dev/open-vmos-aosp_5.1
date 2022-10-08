/*
 * portother.cpp, port class for other
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

#include <OMX_Core.h>
#include <OMX_Component.h>

#include <componentbase.h>
#include <portother.h>

#define LOG_TAG "portother"
#include <log.h>

PortOther::PortOther()
{
    memset(&otherparam, 0, sizeof(otherparam));
    ComponentBase::SetTypeHeader(&otherparam, sizeof(otherparam));
}

OMX_ERRORTYPE PortOther::SetPortOtherParam(
    const OMX_OTHER_PARAM_PORTFORMATTYPE *p, bool internal)
{
    if (!internal) {
        OMX_ERRORTYPE ret;

        ret = ComponentBase::CheckTypeHeader((void *)p, sizeof(*p));
        if (ret != OMX_ErrorNone)
            return ret;
        if (otherparam.nPortIndex != p->nPortIndex)
            return OMX_ErrorBadPortIndex;
    }
    else
        otherparam.nPortIndex = p->nPortIndex;

    otherparam.nIndex = p->nIndex;
    otherparam.eFormat = p->eFormat;

    return OMX_ErrorNone;
}

const OMX_OTHER_PARAM_PORTFORMATTYPE *PortOther::GetPortOtherParam(void)
{
    return &otherparam;
}

/* end of PortOther */
