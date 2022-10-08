/*
 * portbase.cpp, base port class
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

#include <portbase.h>
#include <componentbase.h>
//#define LOG_NDEBUG 0

#define LOG_TAG "portbase"
#include <log.h>

/*
 * constructor & destructor
 */
void PortBase::__PortBase(void)
{
    buffer_hdrs = NULL;
    nr_buffer_hdrs = 0;
    buffer_hdrs_completion = false;

    custom_mem_alloc = NULL;
    custom_mem_free = NULL;
    custom_mem_userdata = NULL;

    mem_alignment = 0;

    pthread_mutex_init(&hdrs_lock, NULL);
    pthread_cond_init(&hdrs_wait, NULL);

    __queue_init(&bufferq);
    pthread_mutex_init(&bufferq_lock, NULL);

    __queue_init(&retainedbufferq);
    pthread_mutex_init(&retainedbufferq_lock, NULL);

    __queue_init(&markq);
    pthread_mutex_init(&markq_lock, NULL);

    state = OMX_PortEnabled;
    pthread_mutex_init(&state_lock, NULL);

    memset(&portdefinition, 0, sizeof(portdefinition));
    ComponentBase::SetTypeHeader(&portdefinition, sizeof(portdefinition));
    memset(definition_format_mimetype, 0, OMX_MAX_STRINGNAME_SIZE);
    portdefinition.format.audio.cMIMEType = &definition_format_mimetype[0];
    portdefinition.format.video.cMIMEType = &definition_format_mimetype[0];
    portdefinition.format.image.cMIMEType = &definition_format_mimetype[0];

    memset(&audioparam, 0, sizeof(audioparam));

    owner = NULL;
    appdata = NULL;
    callbacks = NULL;

    cbase = NULL;
}

PortBase::PortBase()
{
    __PortBase();
}

PortBase::PortBase(const OMX_PARAM_PORTDEFINITIONTYPE *portdefinition)
{
    __PortBase();
    SetPortDefinition(portdefinition, true);
}

PortBase::~PortBase()
{
    struct list *entry, *temp;

    /* should've been already freed at FreeBuffer() */
    list_foreach_safe(buffer_hdrs, entry, temp) {
        free(entry->data); /* OMX_BUFFERHEADERTYPE */
        __list_delete(buffer_hdrs, entry);
    }

    pthread_cond_destroy(&hdrs_wait);
    pthread_mutex_destroy(&hdrs_lock);

    /* should've been already freed at buffer processing */
    queue_free_all(&bufferq);
    pthread_mutex_destroy(&bufferq_lock);

    /* should've been already freed at buffer processing */
    queue_free_all(&retainedbufferq);
    pthread_mutex_destroy(&retainedbufferq_lock);

    /* should've been already empty in PushThisBuffer () */
    queue_free_all(&markq);
    pthread_mutex_destroy(&markq_lock);

    pthread_mutex_destroy(&state_lock);
}

/* end of constructor & destructor */

/*
 * accessor
 */
/* owner */
void PortBase::SetOwner(OMX_COMPONENTTYPE *handle)
{
    owner = handle;
    cbase = static_cast<ComponentBase *>(handle->pComponentPrivate);
}

OMX_COMPONENTTYPE *PortBase::GetOwner(void)
{
    return owner;
}

OMX_ERRORTYPE PortBase::SetCallbacks(OMX_HANDLETYPE hComponent,
                                     OMX_CALLBACKTYPE *pCallbacks,
                                     OMX_PTR pAppData)
{
    if (owner != hComponent)
        return OMX_ErrorBadParameter;

    appdata = pAppData;
    callbacks = pCallbacks;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE PortBase::SetMemAllocator(CustomMemAlloc *pMemAlloc, CustomMemFree *pMemFree, OMX_PTR pUserData)
{
    custom_mem_alloc = pMemAlloc;
    custom_mem_free = pMemFree;
    custom_mem_userdata = pUserData;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE PortBase::SetMemAlignment(OMX_U32 nAlignment)
{
    mem_alignment = nAlignment;
    return OMX_ErrorNone;
}

OMX_U32 PortBase::getFrameBufSize(OMX_COLOR_FORMATTYPE colorFormat, OMX_U32 width, OMX_U32 height)
{
    switch (colorFormat) {
    case OMX_COLOR_FormatYCbYCr:
    case OMX_COLOR_FormatCbYCrY:
        return width * height * 2;
    case OMX_COLOR_FormatYUV420Planar:
    case OMX_COLOR_FormatYUV420SemiPlanar:
        return (width * height * 3) >> 1;

    default:
        LOGV("unsupport color format !");
        return -1;
    }
}

/* end of accessor */

/*
 * component methods & helpers
 */
/* Get/SetParameter */
OMX_ERRORTYPE PortBase::SetPortDefinition(
    const OMX_PARAM_PORTDEFINITIONTYPE *p, bool overwrite_readonly)
{
    OMX_PARAM_PORTDEFINITIONTYPE temp;

    memcpy(&temp, &portdefinition, sizeof(temp));

    if (!overwrite_readonly) {
        if (temp.nPortIndex != p->nPortIndex)
            return OMX_ErrorBadParameter;
        if (temp.eDir != p->eDir)
            return OMX_ErrorBadParameter;
        if (temp.eDomain != p->eDomain)
            return OMX_ErrorBadParameter;
        if (temp.nBufferCountActual != p->nBufferCountActual) {
            if (temp.nBufferCountMin > p->nBufferCountActual)
                return OMX_ErrorBadParameter;
            temp.nBufferCountActual = p->nBufferCountActual;
        }
        if ((p->nBufferSize > temp.nBufferSize) && (temp.eDir == OMX_DirInput)) {
            if (p->nBufferSize <= MAX_INPUT_PORT_SIZE) {
                LOGW("Input port size has been changed!");
                temp.nBufferSize = p->nBufferSize;
            } else {
                LOGE("Invalid input port size!");
                return OMX_ErrorBadParameter;
            }
        }
    }
    else {
        temp.nPortIndex = p->nPortIndex;
        temp.eDir = p->eDir;
        temp.nBufferCountActual = p->nBufferCountActual;
        temp.nBufferCountMin = p->nBufferCountMin;
        temp.nBufferSize = p->nBufferSize;
        temp.bEnabled = p->bEnabled;
        temp.bPopulated = p->bPopulated;
        temp.eDomain = p->eDomain;
        temp.bBuffersContiguous = p->bBuffersContiguous;
        temp.nBufferAlignment = p->nBufferAlignment;
    }

    switch (p->eDomain) {
    case OMX_PortDomainAudio: {
        OMX_AUDIO_PORTDEFINITIONTYPE *format = &temp.format.audio;
        const OMX_AUDIO_PORTDEFINITIONTYPE *pformat = &p->format.audio;
        OMX_U32 mimetype_len = strlen(pformat->cMIMEType);

        mimetype_len = OMX_MAX_STRINGNAME_SIZE-1 > mimetype_len ?
                       mimetype_len : OMX_MAX_STRINGNAME_SIZE-1;

        strncpy(format->cMIMEType, pformat->cMIMEType,
                mimetype_len);
        format->cMIMEType[mimetype_len+1] = '\0';
        format->pNativeRender = pformat->pNativeRender;
        format->bFlagErrorConcealment = pformat->bFlagErrorConcealment;
        format->eEncoding = pformat->eEncoding;

        break;
    }
    case OMX_PortDomainVideo: {
        OMX_VIDEO_PORTDEFINITIONTYPE *format = &temp.format.video;
        const OMX_VIDEO_PORTDEFINITIONTYPE *pformat = &p->format.video;
        OMX_U32 mimetype_len = strlen(pformat->cMIMEType);

        mimetype_len = OMX_MAX_STRINGNAME_SIZE-1 > mimetype_len ?
                       mimetype_len : OMX_MAX_STRINGNAME_SIZE-1;

        strncpy(format->cMIMEType, pformat->cMIMEType,
                mimetype_len);
        format->cMIMEType[mimetype_len+1] = '\0';
        format->pNativeRender = pformat->pNativeRender;
        format->nFrameWidth = pformat->nFrameWidth;
        format->nFrameHeight = pformat->nFrameHeight;
        format->nBitrate = pformat->nBitrate;
        format->xFramerate = pformat->xFramerate;
        format->bFlagErrorConcealment = pformat->bFlagErrorConcealment;
        format->eCompressionFormat = pformat->eCompressionFormat;
        format->eColorFormat = pformat->eColorFormat;
        format->pNativeWindow = pformat->pNativeWindow;
        OMX_S32 nFrameSize = getFrameBufSize(format->eColorFormat,format->nFrameWidth,format->nFrameHeight);
        if(nFrameSize!=-1)
            temp.nBufferSize = nFrameSize;
        if (overwrite_readonly) {
            format->nStride = pformat->nStride;
            format->nSliceHeight = pformat->nSliceHeight;
        } else {
            format->nStride = pformat->nFrameWidth;
            format->nSliceHeight = pformat->nFrameHeight;
        }

        break;
    }
    case OMX_PortDomainImage: {
        OMX_IMAGE_PORTDEFINITIONTYPE *format = &temp.format.image;
        const OMX_IMAGE_PORTDEFINITIONTYPE *pformat = &p->format.image;
        OMX_U32 mimetype_len = strlen(pformat->cMIMEType);

        mimetype_len = OMX_MAX_STRINGNAME_SIZE-1 > mimetype_len ?
                       mimetype_len : OMX_MAX_STRINGNAME_SIZE-1;

        strncpy(format->cMIMEType, pformat->cMIMEType,
                mimetype_len+1);
        format->cMIMEType[mimetype_len+1] = '\0';
        format->nFrameWidth = pformat->nFrameWidth;
        format->nFrameHeight = pformat->nFrameHeight;
        format->nStride = pformat->nStride;
        format->bFlagErrorConcealment = pformat->bFlagErrorConcealment;
        format->eCompressionFormat = pformat->eCompressionFormat;
        format->eColorFormat = pformat->eColorFormat;
        format->pNativeWindow = pformat->pNativeWindow;

        if (overwrite_readonly)
            format->nSliceHeight = pformat->nSliceHeight;

        break;
    }
    case OMX_PortDomainOther: {
        OMX_OTHER_PORTDEFINITIONTYPE *format = &temp.format.other;
        const OMX_OTHER_PORTDEFINITIONTYPE *pformat = &p->format.other;

        format->eFormat = pformat->eFormat;
        break;
    }
    default:
        LOGE("cannot find 0x%08x port domain\n", p->eDomain);
        return OMX_ErrorBadParameter;
    }

    memcpy(&portdefinition, &temp, sizeof(temp));
    return OMX_ErrorNone;
}

const OMX_PARAM_PORTDEFINITIONTYPE *PortBase::GetPortDefinition(void)
{
    return &portdefinition;
}

/* Use/Allocate/FreeBuffer */
OMX_ERRORTYPE PortBase::UseBuffer(OMX_BUFFERHEADERTYPE **ppBufferHdr,
                                  OMX_U32 nPortIndex,
                                  OMX_PTR pAppPrivate,
                                  OMX_U32 nSizeBytes,
                                  OMX_U8 *pBuffer)
{
    OMX_BUFFERHEADERTYPE *buffer_hdr;
    struct list *entry;

    LOGV("%s(): %s:%s:PortIndex %u: enter, nSizeBytes=%u\n", __FUNCTION__,
         cbase->GetName(), cbase->GetWorkingRole(), nPortIndex, nSizeBytes);

    pthread_mutex_lock(&hdrs_lock);

    if (portdefinition.bPopulated == OMX_TRUE) {
        pthread_mutex_unlock(&hdrs_lock);
        LOGV("%s(): %s:%s:PortIndex %u: exit done, already populated\n",
             __FUNCTION__, cbase->GetName(), cbase->GetWorkingRole(),
             nPortIndex);
        return OMX_ErrorNone;
    }

    buffer_hdr = (OMX_BUFFERHEADERTYPE *)calloc(1, sizeof(*buffer_hdr));
    if (!buffer_hdr) {
        pthread_mutex_unlock(&hdrs_lock);
        LOGE("%s(): %s:%s:PortIndex %u: exit failure, "
             "connot allocate buffer header\n", __FUNCTION__,
             cbase->GetName(), cbase->GetWorkingRole(), nPortIndex);
        return OMX_ErrorInsufficientResources;
    }

    entry = list_alloc(buffer_hdr);
    if (!entry) {
        free(buffer_hdr);
        pthread_mutex_unlock(&hdrs_lock);
        LOGE("%s(): %s:%s:PortIndex %u: exit failure, "
             "cannot allocate list entry\n", __FUNCTION__,
             cbase->GetName(), cbase->GetWorkingRole(), nPortIndex);
        return OMX_ErrorInsufficientResources;
    }

    ComponentBase::SetTypeHeader(buffer_hdr, sizeof(*buffer_hdr));
    buffer_hdr->pBuffer = pBuffer;
    buffer_hdr->nAllocLen = nSizeBytes;
    buffer_hdr->pAppPrivate = pAppPrivate;
    buffer_hdr->pInputPortPrivate = NULL;
    buffer_hdr->pOutputPortPrivate = NULL;
    if (portdefinition.eDir == OMX_DirInput) {
        buffer_hdr->nInputPortIndex = nPortIndex;
        buffer_hdr->nOutputPortIndex = 0x7fffffff;
    }
    else {
        buffer_hdr->nOutputPortIndex = nPortIndex;
        buffer_hdr->nInputPortIndex = 0x7fffffff;
    }

    buffer_hdrs = __list_add_tail(buffer_hdrs, entry);
    nr_buffer_hdrs++;

    LOGV("%s(): %s:%s:PortIndex %u: a buffer allocated (%p:%u/%u)\n",
         __FUNCTION__,
         cbase->GetName(), cbase->GetWorkingRole(), nPortIndex,
         buffer_hdr, nr_buffer_hdrs, portdefinition.nBufferCountActual);

    if (nr_buffer_hdrs >= portdefinition.nBufferCountActual) {
        portdefinition.bPopulated = OMX_TRUE;
        buffer_hdrs_completion = true;
        pthread_cond_signal(&hdrs_wait);
        LOGV("%s(): %s:%s:PortIndex %u: allocate all buffers (%u)\n",
             __FUNCTION__, cbase->GetName(), cbase->GetWorkingRole(),
             nPortIndex, portdefinition.nBufferCountActual);
    }

    *ppBufferHdr = buffer_hdr;

    pthread_mutex_unlock(&hdrs_lock);

    LOGV("%s(): %s:%s:PortIndex %u: exit done\n", __FUNCTION__,
         cbase->GetName(), cbase->GetWorkingRole(), nPortIndex);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE PortBase:: AllocateBuffer(OMX_BUFFERHEADERTYPE **ppBuffer,
                                       OMX_U32 nPortIndex,
                                       OMX_PTR pAppPrivate,
                                       OMX_U32 nSizeBytes)
{
    OMX_BUFFERHEADERTYPE *buffer_hdr;
    struct list *entry;

    LOGV("%s(): %s:%s:PortIndex %u: enter, nSizeBytes=%u\n", __FUNCTION__,
         cbase->GetName(), cbase->GetWorkingRole(), nPortIndex, nSizeBytes);

    pthread_mutex_lock(&hdrs_lock);
    if (portdefinition.bPopulated == OMX_TRUE) {
        pthread_mutex_unlock(&hdrs_lock);
        LOGV("%s(): %s:%s:PortIndex %u: exit done, already populated\n",
             __FUNCTION__, cbase->GetName(), cbase->GetWorkingRole(),
             nPortIndex);
        return OMX_ErrorNone;
    }

    if (custom_mem_alloc) {
        buffer_hdr = (OMX_BUFFERHEADERTYPE *) calloc(1, sizeof(*buffer_hdr));
    } else {
        if (mem_alignment > 0)
            buffer_hdr = (OMX_BUFFERHEADERTYPE *) calloc(1, sizeof(*buffer_hdr) + nSizeBytes + mem_alignment);
        else
            buffer_hdr = (OMX_BUFFERHEADERTYPE *) calloc(1, sizeof(*buffer_hdr) + nSizeBytes);
    }

    if (!buffer_hdr) {
        pthread_mutex_unlock(&hdrs_lock);
        LOGE("%s(): %s:%s:PortIndex %u: exit failure, "
             "connot allocate buffer header\n", __FUNCTION__,
             cbase->GetName(), cbase->GetWorkingRole(), nPortIndex);
        return OMX_ErrorInsufficientResources;
    }

    entry = list_alloc(buffer_hdr);
    if (!entry) {
        free(buffer_hdr);
        pthread_mutex_unlock(&hdrs_lock);
        LOGE("%s(): %s:%s:PortIndex %u: exit failure, "
             "connot allocate list entry\n", __FUNCTION__,
             cbase->GetName(), cbase->GetWorkingRole(), nPortIndex);
        return OMX_ErrorInsufficientResources;
    }

    ComponentBase::SetTypeHeader(buffer_hdr, sizeof(*buffer_hdr));
    if (custom_mem_alloc) {
        buffer_hdr->pBuffer = (*custom_mem_alloc)(nSizeBytes, custom_mem_userdata);
    } else {
        if (mem_alignment > 0)
            buffer_hdr->pBuffer = (OMX_U8 *)(((OMX_U32)((OMX_U8 *)buffer_hdr + sizeof(*buffer_hdr)) / mem_alignment + 1) * mem_alignment);
        else
            buffer_hdr->pBuffer = (OMX_U8 *)buffer_hdr + sizeof(*buffer_hdr);
    }
    if (buffer_hdr->pBuffer == NULL) {
        return OMX_ErrorInsufficientResources;
    }

    buffer_hdr->nAllocLen = nSizeBytes;
    buffer_hdr->pAppPrivate = pAppPrivate;
    buffer_hdr->pInputPortPrivate = NULL;
    buffer_hdr->pOutputPortPrivate = NULL;
    if (portdefinition.eDir == OMX_DirInput) {
        buffer_hdr->nInputPortIndex = nPortIndex;
        buffer_hdr->nOutputPortIndex = (OMX_U32)-1;
    }
    else {
        buffer_hdr->nOutputPortIndex = nPortIndex;
        buffer_hdr->nInputPortIndex = (OMX_U32)-1;
    }

    buffer_hdrs = __list_add_tail(buffer_hdrs, entry);
    nr_buffer_hdrs++;

    LOGV("%s(): %s:%s:PortIndex %u: a buffer allocated (%p:%u/%u)\n",
         __FUNCTION__,
         cbase->GetName(), cbase->GetWorkingRole(), nPortIndex,
         buffer_hdr, nr_buffer_hdrs, portdefinition.nBufferCountActual);

    if (nr_buffer_hdrs == portdefinition.nBufferCountActual) {
        portdefinition.bPopulated = OMX_TRUE;
        buffer_hdrs_completion = true;
        pthread_cond_signal(&hdrs_wait);
        LOGV("%s(): %s:%s:PortIndex %u: allocate all buffers (%u)\n",
             __FUNCTION__, cbase->GetName(), cbase->GetWorkingRole(),
             nPortIndex, portdefinition.nBufferCountActual);
    }

    *ppBuffer = buffer_hdr;

    pthread_mutex_unlock(&hdrs_lock);

    LOGV("%s(): %s:%s:PortIndex %u: exit done\n", __FUNCTION__,
         cbase->GetName(), cbase->GetWorkingRole(), nPortIndex);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE PortBase::FreeBuffer(OMX_U32 nPortIndex,
                                   OMX_BUFFERHEADERTYPE *pBuffer)
{
    struct list *entry;
    OMX_ERRORTYPE ret;

    LOGV("%s(): %s:%s:PortIndex %u:pBuffer %p: enter\n", __FUNCTION__,
         cbase->GetName(), cbase->GetWorkingRole(), nPortIndex, pBuffer);

    pthread_mutex_lock(&hdrs_lock);
    entry = list_find(buffer_hdrs, pBuffer);

    if (!entry) {
        pthread_mutex_unlock(&hdrs_lock);
        LOGE("%s(): %s:%s:PortIndex %u:pBuffer %p: exit failure, "
             "cannot find list entry for pBuffer\n", __FUNCTION__,
             cbase->GetName(), cbase->GetWorkingRole(), nPortIndex, pBuffer);
        return OMX_ErrorBadParameter;
    }

    if (entry->data != pBuffer) {
        pthread_mutex_unlock(&hdrs_lock);
        LOGE("%s(): %s:%s:PortIndex %u:pBuffer %p: exit failure,"
             "mismatch list entry\n" , __FUNCTION__,
             cbase->GetName(), cbase->GetWorkingRole(), nPortIndex, pBuffer);
        return OMX_ErrorBadParameter;
    }

    ret = ComponentBase::CheckTypeHeader(pBuffer, sizeof(*pBuffer));
    if (ret != OMX_ErrorNone) {
        pthread_mutex_unlock(&hdrs_lock);
        LOGE("%s(): %s:%s:PortIndex %u:pBuffer %p: exit failure,"
             "invalid type header\n", __FUNCTION__,
             cbase->GetName(), cbase->GetWorkingRole(), nPortIndex, pBuffer);
        return ret;
    }

    buffer_hdrs = __list_delete(buffer_hdrs, entry);
    nr_buffer_hdrs--;

    LOGV("%s(): %s:%s:PortIndex %u:pBuffer %p: free a buffer (%u/%u)\n",
         __FUNCTION__, cbase->GetName(), cbase->GetWorkingRole(), nPortIndex,
         pBuffer, nr_buffer_hdrs, portdefinition.nBufferCountActual);
    if (custom_mem_free) {
        (*custom_mem_free)(pBuffer->pBuffer, custom_mem_userdata);
        pBuffer->pBuffer = NULL;
    }
    free(pBuffer);

    portdefinition.bPopulated = OMX_FALSE;
    if (!nr_buffer_hdrs) {
        buffer_hdrs_completion = true;
        pthread_cond_signal(&hdrs_wait);
        LOGV("%s(): %s:%s:PortIndex %u: free all allocated buffers (%u)\n",
             __FUNCTION__, cbase->GetName(), cbase->GetWorkingRole(),
             nPortIndex, portdefinition.nBufferCountActual);
    }

    pthread_mutex_unlock(&hdrs_lock);

    LOGV("%s(): %s:%s:PortIndex %u: exit done\n", __FUNCTION__,
         cbase->GetName(), cbase->GetWorkingRole(), nPortIndex);
    return OMX_ErrorNone;
}

void PortBase::WaitPortBufferCompletion(void)
{
    pthread_mutex_lock(&hdrs_lock);
    if (!buffer_hdrs_completion) {
        LOGV("%s(): %s:%s:PortIndex %u: wait for buffer header completion\n",
             __FUNCTION__, cbase->GetName(), cbase->GetWorkingRole(),
             portdefinition.nPortIndex);
        pthread_cond_wait(&hdrs_wait, &hdrs_lock);
        LOGV("%s(): %s:%s:PortIndex %u: wokeup (buffer header completion)\n",
             __FUNCTION__, cbase->GetName(), cbase->GetWorkingRole(),
             portdefinition.nPortIndex);
    }
    buffer_hdrs_completion = !buffer_hdrs_completion;
    pthread_mutex_unlock(&hdrs_lock);
}

OMX_ERRORTYPE PortBase::WaitPortBufferCompletionTimeout(int64_t milliseconds)
{
    int rc = 0;
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    pthread_mutex_lock(&hdrs_lock);
    if (!buffer_hdrs_completion) {
        LOGV("%s(): %s:%s:PortIndex %u: wait for buffer header completion\n",
             __FUNCTION__, cbase->GetName(), cbase->GetWorkingRole(),
             portdefinition.nPortIndex);
        struct timespec tv;
        clock_gettime(CLOCK_REALTIME, &tv);
        tv.tv_sec += milliseconds/1000;
        tv.tv_nsec+= (milliseconds%1000) * 1000000;
        rc = pthread_cond_timedwait(&hdrs_wait, &hdrs_lock, &tv);
    }
    if (rc == ETIMEDOUT) {
        LOGE("%s(): %s:%s:PortIndex %u: wokeup (buffer header timeout)\n",
             __FUNCTION__, cbase->GetName(), cbase->GetWorkingRole(),
             portdefinition.nPortIndex);
        ret = OMX_ErrorTimeout;
    }
    buffer_hdrs_completion = !buffer_hdrs_completion;
    pthread_mutex_unlock(&hdrs_lock);
    return ret;
}

/* Empty/FillThisBuffer */
OMX_ERRORTYPE PortBase::PushThisBuffer(OMX_BUFFERHEADERTYPE *pBuffer)
{
    int ret;
    LOGV_IF(pBuffer != NULL, "%s(): %s:%s:PortIndex %u:pBuffer %p:\n",
            __FUNCTION__, cbase->GetName(), cbase->GetWorkingRole(),
           portdefinition.nPortIndex, pBuffer);
    pthread_mutex_lock(&bufferq_lock);
    ret = queue_push_tail(&bufferq, pBuffer);
    pthread_mutex_unlock(&bufferq_lock);

    if (ret)
        return OMX_ErrorInsufficientResources;

    return OMX_ErrorNone;
}

OMX_BUFFERHEADERTYPE *PortBase::PopBuffer(void)
{
    OMX_BUFFERHEADERTYPE *buffer;

    pthread_mutex_lock(&bufferq_lock);
    buffer = (OMX_BUFFERHEADERTYPE *)queue_pop_head(&bufferq);
    pthread_mutex_unlock(&bufferq_lock);
    LOGV_IF((buffer != NULL || RetainedBufferQueueLength() > 0), "%s(): %s:%s:PortIndex %u:pBuffer %p:\n",
            __FUNCTION__, cbase->GetName(), cbase->GetWorkingRole(),
            portdefinition.nPortIndex, buffer);
    return buffer;
}

OMX_U32 PortBase::BufferQueueLength(void)
{
    OMX_U32 length;

    pthread_mutex_lock(&bufferq_lock);
    length = queue_length(&bufferq);
    pthread_mutex_unlock(&bufferq_lock);

    return length;
}

OMX_U32 PortBase::RetainedBufferQueueLength(void)
{
    OMX_U32 length;

    pthread_mutex_lock(&retainedbufferq_lock);
    length = queue_length(&retainedbufferq);
    pthread_mutex_unlock(&retainedbufferq_lock);

    return length;
}

OMX_ERRORTYPE PortBase::ReturnThisBuffer(OMX_BUFFERHEADERTYPE *pBuffer)
{
    OMX_DIRTYPE direction = portdefinition.eDir;
    OMX_U32 port_index;
    OMX_ERRORTYPE (*bufferdone_callback)(OMX_HANDLETYPE,
                                         OMX_PTR,
                                         OMX_BUFFERHEADERTYPE *);
    OMX_ERRORTYPE ret;

    LOGV("%s(): %s:%s:PortIndex %u:pBuffer %p: enter\n", __FUNCTION__,
         cbase->GetName(), cbase->GetWorkingRole(), portdefinition.nPortIndex,
         pBuffer);

    if (!pBuffer) {
        LOGE("%s(): %s:%s:PortIndex %u:pBuffer %p: exit failure, "
             "invalid buffer pointer\n",
             __FUNCTION__, cbase->GetName(), cbase->GetWorkingRole(),
             portdefinition.nPortIndex, pBuffer);
        return OMX_ErrorBadParameter;
    }

    if (direction == OMX_DirInput) {
        port_index = pBuffer->nInputPortIndex;
        bufferdone_callback = callbacks->EmptyBufferDone;
    }
    else if (direction == OMX_DirOutput) {
        port_index = pBuffer->nOutputPortIndex;
        bufferdone_callback = callbacks->FillBufferDone;
    }
    else {
        LOGE("%s(): %s:%s:PortIndex %u:pBuffer %p: exit failure, "
             "invalid direction (%d)\n",
             __FUNCTION__, cbase->GetName(), cbase->GetWorkingRole(),
             portdefinition.nPortIndex, pBuffer,
             direction);
        return OMX_ErrorBadParameter;
    }

    if (port_index != portdefinition.nPortIndex) {
        LOGE("%s(): %s:%s:PortIndex %u:pBuffer %p: exit failure, "
             "invalid port index (%u)\n", __FUNCTION__,
             cbase->GetName(), cbase->GetWorkingRole(),
             portdefinition.nPortIndex, pBuffer, port_index);
        return OMX_ErrorBadParameter;
    }

    if (pBuffer->nFlags & OMX_BUFFERFLAG_EOS) {
        LOGV("%s(): %s:%s:PortIndex %u:pBuffer %p: "
             "Report OMX_EventBufferFlag (OMX_BUFFERFLAG_EOS)\n",
             __FUNCTION__, cbase->GetName(), cbase->GetWorkingRole(),
             portdefinition.nPortIndex, pBuffer);

        callbacks->EventHandler(owner, appdata,
                                OMX_EventBufferFlag,
                                port_index, pBuffer->nFlags, NULL);
    }

    if (pBuffer->hMarkTargetComponent == owner) {
        LOGV("%s(): %s:%s:PortIndex %u:pBuffer %p: "
             "Report OMX_EventMark\n",
             __FUNCTION__, cbase->GetName(), cbase->GetWorkingRole(),
             portdefinition.nPortIndex, pBuffer);

        callbacks->EventHandler(owner, appdata, OMX_EventMark,
                                0, 0, pBuffer->pMarkData);
        pBuffer->hMarkTargetComponent = NULL;
        pBuffer->pMarkData = NULL;
    }

    ret = bufferdone_callback(owner, appdata, pBuffer);

    LOGV("%s(): %s:%s:PortIndex %u: exit done, "
         "callback returned (0x%08x)\n", __FUNCTION__,
         cbase->GetName(), cbase->GetWorkingRole(), portdefinition.nPortIndex,
         ret);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE PortBase::RetainAndReturnBuffer( OMX_BUFFERHEADERTYPE *pRetain, OMX_BUFFERHEADERTYPE *pReturn)
{
    OMX_ERRORTYPE ret;
    OMX_U32 length;
    if (pReturn == pRetain) {
        return ReturnThisBuffer(pReturn);
    }
    ret = RetainThisBuffer(pRetain, false);
    if (ret != OMX_ErrorNone) {
        return ret;
    }

    pthread_mutex_lock(&bufferq_lock);
    length = queue_length(&bufferq);
    OMX_BUFFERHEADERTYPE *p;
    /* remove returned buffer from the queue */
    OMX_U32 i = 0;
    for (i = 0; i < length; i++) {
        p = (OMX_BUFFERHEADERTYPE *)queue_pop_head(&bufferq);
        if (p == pReturn) {
            break;
        }
        queue_push_tail(&bufferq, p);
    }
    pthread_mutex_unlock(&bufferq_lock);

    if (i == length) {
        return OMX_ErrorNone;
    }

    return ReturnThisBuffer(pReturn);
}

/* retain buffer */
OMX_ERRORTYPE PortBase::RetainThisBuffer(OMX_BUFFERHEADERTYPE *pBuffer,
        bool accumulate)
{
    int ret;

    LOGV("%s(): %s:%s:PortIndex %u:pBuffer %p: enter, %s\n", __FUNCTION__,
         cbase->GetName(), cbase->GetWorkingRole(), portdefinition.nPortIndex,
         pBuffer, (accumulate == true) ? "accumulate" : "getagain");

    /* push at tail of retainedbufferq */
    if (accumulate == true) {

        if (cbase->GetWorkingRole() == NULL || (strncmp((char*)cbase->GetWorkingRole(), "video_encoder", 13) != 0)) {
            /* do not accumulate a buffer set EOS flag if not video encoder*/
            if (pBuffer->nFlags & OMX_BUFFERFLAG_EOS) {
                LOGE("%s(): %s:%s:PortIndex %u:pBuffer %p: exit failure, "
                     "cannot accumulate EOS buffer\n", __FUNCTION__,
                     cbase->GetName(), cbase->GetWorkingRole(),
                     portdefinition.nPortIndex, pBuffer);
                return OMX_ErrorBadParameter;
            }
        }

        pthread_mutex_lock(&retainedbufferq_lock);
        if ((OMX_U32)queue_length(&retainedbufferq) <
                portdefinition.nBufferCountActual)
            ret = queue_push_tail(&retainedbufferq, pBuffer);
        else {
            ret = OMX_ErrorInsufficientResources;
            LOGE("%s(): %s:%s:PortIndex %u:pBuffer %p: exit failure, "
                 "retained bufferq length (%d) exceeds port's actual count "
                 "(%u)\n", __FUNCTION__,
                 cbase->GetName(), cbase->GetWorkingRole(),
                 portdefinition.nPortIndex, pBuffer,
                 queue_length(&retainedbufferq),
                 portdefinition.nBufferCountActual);
        }
        pthread_mutex_unlock(&retainedbufferq_lock);
    }
    /*
     * just push at head of bufferq to get this buffer again in
     * ComponentBase::ProcessorProcess()
     */
    else {
        pthread_mutex_lock(&bufferq_lock);
        ret = queue_push_head(&bufferq, pBuffer);
        pthread_mutex_unlock(&bufferq_lock);
    }

    if (ret)
        return OMX_ErrorInsufficientResources;

    LOGV("%s(): %s:%s:PortIndex %u:pBuffer %p: exit done\n", __FUNCTION__,
         cbase->GetName(), cbase->GetWorkingRole(),
         portdefinition.nPortIndex, pBuffer);
    return OMX_ErrorNone;
}

void PortBase::ReturnAllRetainedBuffers(void)
{
    OMX_BUFFERHEADERTYPE *buffer;
    OMX_ERRORTYPE ret;
    int i = 0;

    pthread_mutex_lock(&retainedbufferq_lock);

    do {
        buffer = (OMX_BUFFERHEADERTYPE *)queue_pop_head(&retainedbufferq);

        if (buffer) {
            LOGV("%s(): %s:%s:PortIndex %u: returns a retained buffer "
                 "(%p:%d/%d)\n", __FUNCTION__, cbase->GetName(),
                 cbase->GetWorkingRole(), portdefinition.nPortIndex,
                 buffer, i++, queue_length(&retainedbufferq));

            ret = ReturnThisBuffer(buffer);
            if (ret != OMX_ErrorNone)
                LOGE("%s(): %s:%s:PortIndex %u: failed (ret : 0x%x08x)\n",
                     __FUNCTION__,
                     cbase->GetName(), cbase->GetWorkingRole(),
                     portdefinition.nPortIndex, ret);
        }
    } while (buffer);

    pthread_mutex_unlock(&retainedbufferq_lock);
    LOGV_IF(i != 0,
            "%s(): %s:%s:PortIndex %u: returned all retained buffers (%d)\n",
            __FUNCTION__, cbase->GetName(), cbase->GetWorkingRole(),
            portdefinition.nPortIndex, i);
}

void PortBase::ReturnOneRetainedBuffer(void)
{
    OMX_BUFFERHEADERTYPE *buffer;
    OMX_ERRORTYPE ret;
    int i =0;

    pthread_mutex_lock(&retainedbufferq_lock);

    buffer = (OMX_BUFFERHEADERTYPE *)queue_pop_head(&retainedbufferq);

    if (buffer) {
        LOGV("%s(): %s:%s:PortIndex %u: returns a retained buffer "
             "(%p:%d/%d)\n", __FUNCTION__, cbase->GetName(),
             cbase->GetWorkingRole(), portdefinition.nPortIndex,
             buffer, i++, queue_length(&retainedbufferq));

        ret = ReturnThisBuffer(buffer);
        if (ret != OMX_ErrorNone)
            LOGE("%s(): %s:%s:PortIndex %u: failed (ret : 0x%x08x)\n",
                 __FUNCTION__,
                 cbase->GetName(), cbase->GetWorkingRole(),
                 portdefinition.nPortIndex, ret);
    }

    pthread_mutex_unlock(&retainedbufferq_lock);

}

/* SendCommand:Flush/PortEnable/Disable */
/* must be held ComponentBase::ports_block */
OMX_ERRORTYPE PortBase::FlushPort(void)
{
    OMX_BUFFERHEADERTYPE *buffer;

    LOGV("%s(): %s:%s:PortIndex %u: enter\n", __FUNCTION__,
         cbase->GetName(), cbase->GetWorkingRole(),
         portdefinition.nPortIndex);

    ReturnAllRetainedBuffers();

    while ((buffer = PopBuffer()))
        ReturnThisBuffer(buffer);

    LOGV("%s(): %s:%s:PortIndex %u: exit\n", __FUNCTION__,
         cbase->GetName(), cbase->GetWorkingRole(),
         portdefinition.nPortIndex);

    return OMX_ErrorNone;
}

OMX_STATETYPE PortBase::GetOwnerState(void)
{
    OMX_STATETYPE state = OMX_StateInvalid;

    if (owner) {
        ComponentBase *cbase;
        cbase = static_cast<ComponentBase *>(owner->pComponentPrivate);
        if (!cbase)
            return state;

        cbase->CBaseGetState((void *)owner, &state);
    }

    return state;
}

bool PortBase::IsEnabled(void)
{
    bool enabled;
    bool unlock = true;

    if (pthread_mutex_trylock(&state_lock))
        unlock = false;

    enabled = (state == OMX_PortEnabled) ? true : false;

    if (unlock)
        pthread_mutex_unlock(&state_lock);

    return enabled;
}

OMX_DIRTYPE PortBase::GetPortDirection(void)
{
    return portdefinition.eDir;
}

OMX_U32 PortBase::GetPortBufferCount(void)
{
    return nr_buffer_hdrs;
}

OMX_ERRORTYPE PortBase::PushMark(OMX_MARKTYPE *mark)
{
    int ret;

    pthread_mutex_lock(&markq_lock);
    ret = queue_push_tail(&markq, mark);
    pthread_mutex_unlock(&markq_lock);

    if (ret)
        return OMX_ErrorInsufficientResources;

    return OMX_ErrorNone;
}

OMX_MARKTYPE *PortBase::PopMark(void)
{
    OMX_MARKTYPE *mark;

    pthread_mutex_lock(&markq_lock);
    mark = (OMX_MARKTYPE *)queue_pop_head(&markq);
    pthread_mutex_unlock(&markq_lock);

    return mark;
}

static const char *state_name[PortBase::OMX_PortEnabled+2] = {
    "OMX_PortDisabled",
    "OMX_PortEnabled",
    "UnKnown Port State",
};

const char *GetPortStateName(OMX_U8 state)
{
    if (state > PortBase::OMX_PortEnabled)
        state = PortBase::OMX_PortEnabled+1;

    return state_name[state];
}

OMX_ERRORTYPE PortBase::TransState(OMX_U8 transition)
{
    OMX_U8 current;
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    LOGV("%s(): %s:%s:PortIndex %u: enter, transition from %s to %s\n",
         __FUNCTION__,
         cbase->GetName(), cbase->GetWorkingRole(), portdefinition.nPortIndex,
         GetPortStateName(state), GetPortStateName(transition));

    pthread_mutex_lock(&state_lock);

    current = state;

    if (current == transition) {
        ret = OMX_ErrorSameState;
        LOGE("%s(): %s:%s:PortIndex %u: exit failure, same state (%s)\n",
             __FUNCTION__,
             cbase->GetName(), cbase->GetWorkingRole(),
             portdefinition.nPortIndex, GetPortStateName(current));
        goto unlock;
    }

    if (transition == OMX_PortEnabled) {
        if (cbase->GetWorkingRole() != NULL &&
                !strncmp (cbase->GetWorkingRole(),"video_decoder", 13 )) {
            ret = WaitPortBufferCompletionTimeout(800); //0.8s timeout
            if (!nr_buffer_hdrs) {
                // event is trigger by freeing buffer instead of allocating buffer
                ret = OMX_ErrorBadParameter;
            }
            if (ret != OMX_ErrorNone) {
                goto unlock;
            }
        } else {
            WaitPortBufferCompletion();
        }
        portdefinition.bEnabled = OMX_TRUE;
    }
    else if(transition == OMX_PortDisabled) {
        /*need to flush only if port is not empty*/
        if (nr_buffer_hdrs)
        {
            FlushPort();
            WaitPortBufferCompletion();
        }
        portdefinition.bEnabled = OMX_FALSE;
    }
    else {
        ret = OMX_ErrorBadParameter;
        LOGE("%s(): %s:%s:PortIndex %u: exit failure, invalid transition "
             "(%s)\n", __FUNCTION__,
             cbase->GetName(), cbase->GetWorkingRole(),
             portdefinition.nPortIndex, GetPortStateName(transition));
        goto unlock;
    }

    state = transition;

    LOGV("%s(): %s:%s:PortIndex %u: transition from %s to %s complete\n",
         __FUNCTION__,
         cbase->GetName(), cbase->GetWorkingRole(), portdefinition.nPortIndex,
         GetPortStateName(current), GetPortStateName(state));

unlock:
    pthread_mutex_unlock(&state_lock);
    return ret;
}

OMX_ERRORTYPE PortBase::ReportPortSettingsChanged(void)
{
    OMX_ERRORTYPE ret;

    ret = callbacks->EventHandler(owner, appdata,
                                  OMX_EventPortSettingsChanged,
                                  portdefinition.nPortIndex,OMX_IndexParamPortDefinition, NULL);

    FlushPort();

    return ret;
}

OMX_ERRORTYPE PortBase::ReportOutputCrop(void)
{
    OMX_ERRORTYPE ret;

    ret = callbacks->EventHandler(owner, appdata,
                                  OMX_EventPortSettingsChanged,
                                  portdefinition.nPortIndex,OMX_IndexConfigCommonOutputCrop, NULL);

    return ret;
}


/* end of component methods & helpers */

/* end of PortBase */
