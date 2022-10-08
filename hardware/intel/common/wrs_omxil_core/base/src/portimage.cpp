/*
 * portimage.cpp, port class for image
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
#include <portimage.h>

#define LOG_TAG "portimage"
#include <log.h>

PortImage::PortImage()
{
    memset(&imageparam, 0, sizeof(imageparam));
    ComponentBase::SetTypeHeader(&imageparam, sizeof(imageparam));
}

OMX_ERRORTYPE PortImage::SetPortImageParam(
    const OMX_IMAGE_PARAM_PORTFORMATTYPE *p, bool internal)
{
    if (!internal) {
        OMX_ERRORTYPE ret;

        ret = ComponentBase::CheckTypeHeader((void *)p, sizeof(*p));
        if (ret != OMX_ErrorNone)
            return ret;
        if (imageparam.nPortIndex != p->nPortIndex)
            return OMX_ErrorBadPortIndex;
    }
    else
        imageparam.nPortIndex = p->nPortIndex;

    imageparam.nIndex = p->nIndex;
    imageparam.eCompressionFormat = p->eCompressionFormat;
    imageparam.eColorFormat = p->eColorFormat;

    return OMX_ErrorNone;
}

const OMX_IMAGE_PARAM_PORTFORMATTYPE *PortImage::GetPortImageParam(void)
{
    return &imageparam;
}

/* end of PortImage */
