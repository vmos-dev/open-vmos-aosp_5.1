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

//#define LOG_NDEBUG 0
#define LOG_TAG "IntelMetadataBuffer"
#include <wrs_omxil_core/log.h>

#include "IntelMetadataBuffer.h"
#include <string.h>
#include <stdio.h>

#ifdef INTEL_VIDEO_XPROC_SHARING
#include <binder/IServiceManager.h>
#include <binder/MemoryBase.h>
#include <binder/Parcel.h>
#include <utils/List.h>
#include <utils/threads.h>
#include <ui/GraphicBuffer.h>

//#define TEST

struct ShareMemMap {
    uint32_t sessionflag;
    intptr_t value;
    intptr_t value_backup;
    uint32_t type;
    sp<MemoryBase> membase;
    sp<GraphicBuffer> gbuffer;
};

List <ShareMemMap *> gShareMemMapList;
Mutex gShareMemMapListLock;

enum {
    SHARE_MEM = IBinder::FIRST_CALL_TRANSACTION,
    GET_MEM,
    CLEAR_MEM,
};

enum {
    ST_MEMBASE = 0,
    ST_GFX,
    ST_MAX,
};

#define REMOTE_PROVIDER 0x80000000
#define REMOTE_CONSUMER 0x40000000

static ShareMemMap* ReadMemObjFromBinder(const Parcel& data, uint32_t sessionflag, intptr_t value) {

    uint32_t type = data.readInt32();
    if (type >= ST_MAX)
        return NULL;

    ShareMemMap* map = new ShareMemMap;
    map->sessionflag = sessionflag;
    map->type = type;
    map->value_backup = value;
    map->membase = NULL;
    map->gbuffer= NULL;

//    LOGI("ReadMemObjFromBinder");

    if (type == ST_MEMBASE) /*offset, size, heap*/
    {
        ssize_t offset = data.readInt32();
        size_t size = data.readInt32();

        sp<IMemoryHeap> heap = interface_cast<IMemoryHeap>(data.readStrongBinder());

        sp<MemoryBase> mem = new MemoryBase(heap, offset, size);
        if (mem == NULL)
        {
            delete map;
            return NULL;
        }

        map->value = (intptr_t)( mem->pointer() + 0x0FFF) & ~0x0FFF;
        map->membase = mem;

#ifdef TEST
        ALOGI("membase heapID:%d, pointer:%x data:%x, aligned value:%x", \
           heap->getHeapID(), mem->pointer(), *((intptr_t *)(mem->pointer())), map->value);
#endif

    }
    else if (type == ST_GFX) /*graphicbuffer*/
    {
        sp<GraphicBuffer> buffer = new GraphicBuffer();
        if (buffer == NULL)
        {
            delete map;
            return NULL;
        }
        data.read(*buffer);

        map->value = (intptr_t)buffer->handle;
        map->gbuffer = buffer;

#ifdef TEST
        void* usrptr[3];
        buffer->lock(GraphicBuffer::USAGE_HW_TEXTURE | GraphicBuffer::USAGE_SW_READ_OFTEN, &usrptr[0]);
        buffer->unlock();
        ALOGI("gfx handle:%p data:%x", (intptr_t)buffer->handle, *((intptr_t *)usrptr[0]));
#endif
    }

    gShareMemMapListLock.lock();
    gShareMemMapList.push_back(map);
    gShareMemMapListLock.unlock();
    return map;
}

static status_t WriteMemObjToBinder(Parcel& data, ShareMemMap* smem) {

    if (smem->type >= ST_MAX)
        return BAD_VALUE;

//    LOGI("WriteMemObjToBinder");

    data.writeInt32(smem->type);

    if (smem->type == ST_MEMBASE) /*offset, size, heap*/
    {
        ssize_t offset;
        size_t size;
        sp<IMemoryHeap> heap = smem->membase->getMemory(&offset, &size);
        data.writeInt32(offset);
        data.writeInt32(size);
        data.writeStrongBinder(heap->asBinder());
#ifdef TEST
        ALOGI("membase heapID:%d pointer:%x data:%x", \
            heap->getHeapID(), smem->membase->pointer(), *((int *)(smem->membase->pointer())));
#endif
    }
    else if (smem->type == ST_GFX) /*graphicbuffer*/
        data.write(*(smem->gbuffer));

    return NO_ERROR;
}

static void ClearLocalMem(uint32_t sessionflag)
{
    List<ShareMemMap *>::iterator node;

    gShareMemMapListLock.lock();

    for(node = gShareMemMapList.begin(); node != gShareMemMapList.end(); )
    {
        if ((*node)->sessionflag == sessionflag) //remove all buffers belong to this session
        {
            (*node)->membase = NULL;
            (*node)->gbuffer = NULL;
            delete (*node);
            node = gShareMemMapList.erase(node);
        }
        else
            node ++;
    }

    gShareMemMapListLock.unlock();
}

static ShareMemMap* FindShareMem(uint32_t sessionflag, intptr_t value, bool isBackup)
{
    List<ShareMemMap *>::iterator node;

    gShareMemMapListLock.lock();
    for(node = gShareMemMapList.begin(); node !=  gShareMemMapList.end(); node++)
    {
        if (isBackup)
        {
            if ((*node)->sessionflag == sessionflag && (*node)->value_backup == value)
            {
                gShareMemMapListLock.unlock();
                return (*node);
            }
        }
        else if ((*node)->sessionflag == sessionflag && (*node)->value == value)
        {
            gShareMemMapListLock.unlock();
            return (*node);
        }
    }
    gShareMemMapListLock.unlock();

    return NULL;
}

static ShareMemMap* PopShareMem(uint32_t sessionflag, intptr_t value)
{
    List<ShareMemMap *>::iterator node;

    gShareMemMapListLock.lock();
    for(node = gShareMemMapList.begin(); node != gShareMemMapList.end(); node++)
    {
        if ((*node)->sessionflag == sessionflag && (*node)->value == value)
        {
            gShareMemMapList.erase(node);
            gShareMemMapListLock.unlock();
            return (*node);
        }
    }
    gShareMemMapListLock.unlock();

    return NULL;
}

static void PushShareMem(ShareMemMap* &smem)
{
    gShareMemMapListLock.lock();
    gShareMemMapList.push_back(smem);
    gShareMemMapListLock.unlock();
}

static sp<IBinder> GetIntelBufferSharingService() {

    sp<IServiceManager> sm = defaultServiceManager();
    sp<IBinder> binder = sm->checkService(String16("media.IntelBufferSharing"));

    if (binder == 0)
        ALOGE("media.IntelBufferSharing service is not published");

    return binder;
}

IntelBufferSharingService* IntelBufferSharingService::gBufferService = NULL;

status_t IntelBufferSharingService::instantiate(){
    status_t ret = NO_ERROR;

    if (gBufferService == NULL) {
        gBufferService = new IntelBufferSharingService();
        ret = defaultServiceManager()->addService(String16("media.IntelBufferSharing"), gBufferService);
        LOGI("IntelBufferSharingService::instantiate() ret = %d\n", ret);
    }

    return ret;
}

status_t IntelBufferSharingService::onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags) {

    //TODO: if pid is int32?
    pid_t pid = data.readInt32();
    uint32_t sessionflag = data.readInt32();

    switch(code)
    {
        case SHARE_MEM:
        {

            if (pid == getpid()) //in same process, should not use binder
            {
                ALOGE("onTransact in same process, wrong sessionflag?");
                return UNKNOWN_ERROR;
            }

            intptr_t value = data.readIntPtr();

//            LOGI("onTransact SHARE_MEM value=%x", value);

            //different process
            ShareMemMap* map = ReadMemObjFromBinder(data, sessionflag, value);
            if (map == NULL)
                return UNKNOWN_ERROR;

            reply->writeIntPtr(map->value);

            return NO_ERROR;
        }
        case CLEAR_MEM:
        {
//            LOGI("onTransact CLEAR_MEM sessionflag=%x", sessionflag);

            if (pid == getpid()) //in same process, should not use binder
            {
                //same process, return same pointer in data
                ALOGE("onTransact CLEAR_MEM in same process, wrong sessionflag?");
                return UNKNOWN_ERROR;
            }

            ClearLocalMem(sessionflag);
            return NO_ERROR;
        }
        case GET_MEM:
        {

            if (pid == getpid()) //in same process, should not use binder
            {
                ALOGE("onTransact GET_MEM in same process, wrong sessionflag?");
                return UNKNOWN_ERROR;
            }

            intptr_t value = data.readIntPtr();

//            LOGI("onTransact GET_MEM value=%x", value);

            ShareMemMap* smem = FindShareMem(sessionflag, value, false);
            if (smem && (NO_ERROR == WriteMemObjToBinder(*reply, smem)))
                return NO_ERROR;
            else
                ALOGE("onTransact GET_MEM: Not find mem");

            return UNKNOWN_ERROR;
        }
        default:
            return BBinder::onTransact(code, data, reply, flags);

    }
    return NO_ERROR;
}
#endif

IntelMetadataBuffer::IntelMetadataBuffer()
{
    mType = IntelMetadataBufferTypeCameraSource;
    mValue = 0;
    mInfo = NULL;
    mExtraValues = NULL;
    mExtraValues_Count = 0;
    mBytes = NULL;
    mSize = 0;
#ifdef INTEL_VIDEO_XPROC_SHARING
    mSessionFlag = 0;
#endif
}

IntelMetadataBuffer::IntelMetadataBuffer(IntelMetadataBufferType type, intptr_t value)
{
    mType = type;
    mValue = value;
    mInfo = NULL;
    mExtraValues = NULL;
    mExtraValues_Count = 0;
    mBytes = NULL;
    mSize = 0;
#ifdef INTEL_VIDEO_XPROC_SHARING
    mSessionFlag = 0;
#endif
}

IntelMetadataBuffer::~IntelMetadataBuffer()
{
    if (mInfo)
        delete mInfo;

    if (mExtraValues)
        delete[] mExtraValues;

    if (mBytes)
        delete[] mBytes;
}


IntelMetadataBuffer::IntelMetadataBuffer(const IntelMetadataBuffer& imb)
     :mType(imb.mType), mValue(imb.mValue), mInfo(NULL), mExtraValues(NULL),
      mExtraValues_Count(imb.mExtraValues_Count), mBytes(NULL), mSize(imb.mSize)
#ifdef INTEL_VIDEO_XPROC_SHARING
      ,mSessionFlag(imb.mSessionFlag)
#endif
{
    if (imb.mInfo)
        mInfo = new ValueInfo(*imb.mInfo);

    if (imb.mExtraValues)
    {
        mExtraValues = new intptr_t[mExtraValues_Count];
        memcpy(mExtraValues, imb.mExtraValues, sizeof(mValue) * mExtraValues_Count);
    }

    if (imb.mBytes)
    {
        mBytes = new uint8_t[mSize];
        memcpy(mBytes, imb.mBytes, mSize);
    }
}

const IntelMetadataBuffer& IntelMetadataBuffer::operator=(const IntelMetadataBuffer& imb)
{
    mType = imb.mType;
    mValue = imb.mValue;
    mInfo = NULL;
    mExtraValues = NULL;
    mExtraValues_Count = imb.mExtraValues_Count;
    mBytes = NULL;
    mSize = imb.mSize;
#ifdef INTEL_VIDEO_XPROC_SHARING
    mSessionFlag = imb.mSessionFlag;
#endif

    if (imb.mInfo)
        mInfo = new ValueInfo(*imb.mInfo);

    if (imb.mExtraValues)
    {
        mExtraValues = new intptr_t[mExtraValues_Count];
        memcpy(mExtraValues, imb.mExtraValues, sizeof(mValue) * mExtraValues_Count);
    }

    if (imb.mBytes)
    {
        mBytes = new uint8_t[mSize];
        memcpy(mBytes, imb.mBytes, mSize);
    }

    return *this;
}

IMB_Result IntelMetadataBuffer::GetType(IntelMetadataBufferType& type)
{
    type = mType;

    return IMB_SUCCESS;
}

IMB_Result IntelMetadataBuffer::SetType(IntelMetadataBufferType type)
{
    if (type < IntelMetadataBufferTypeLast)
        mType = type;
    else
        return IMB_INVAL_PARAM;

    return IMB_SUCCESS;
}

IMB_Result IntelMetadataBuffer::GetValue(intptr_t& value)
{
    value = mValue;

#ifndef INTEL_VIDEO_XPROC_SHARING
    return IMB_SUCCESS;
#else
    if ((mSessionFlag & REMOTE_CONSUMER) == 0) //no sharing or is local consumer
        return IMB_SUCCESS;

    //try to find if it is already cached.
    ShareMemMap* smem = FindShareMem(mSessionFlag, mValue, true);
    if(smem)
    {
        value = smem->value;
        return IMB_SUCCESS;
    }

    //is remote provider and not find from cache, then pull from service
    sp<IBinder> binder = GetIntelBufferSharingService();
    if (binder == 0)
        return IMB_NO_SERVICE;

    //Detect IntelBufferSharingService, share mem to service
    Parcel data, reply;

    //send pid, sessionflag, and memtype
    pid_t pid = getpid();
    //TODO: if pid is int32?
    data.writeInt32(pid);
    data.writeInt32(mSessionFlag);
    data.writeIntPtr(mValue);

    //do transcation
    if (binder->transact(GET_MEM, data, &reply) != NO_ERROR)
        return IMB_SERVICE_FAIL;

    //get type/Mem OBJ
    smem = ReadMemObjFromBinder(reply, mSessionFlag, mValue);
    if (smem)
        value = smem->value;
    else
        return IMB_SERVICE_FAIL;

    return IMB_SUCCESS;
#endif
}

IMB_Result IntelMetadataBuffer::SetValue(intptr_t value)
{
    mValue = value;

    return IMB_SUCCESS;
}

IMB_Result IntelMetadataBuffer::GetValueInfo(ValueInfo* &info)
{
    info = mInfo;

    return IMB_SUCCESS;
}

IMB_Result IntelMetadataBuffer::SetValueInfo(ValueInfo* info)
{
    if (info)
    {
        if (mInfo == NULL)
            mInfo = new ValueInfo;

        memcpy(mInfo, info, sizeof(ValueInfo));
    }
    else
        return IMB_INVAL_PARAM;

    return IMB_SUCCESS;
}

IMB_Result IntelMetadataBuffer::GetExtraValues(intptr_t* &values, uint32_t& num)
{
    values = mExtraValues;
    num = mExtraValues_Count;

    return IMB_SUCCESS;
}

IMB_Result IntelMetadataBuffer::SetExtraValues(intptr_t* values, uint32_t num)
{
    if (values && num > 0)
    {
        if (mExtraValues && mExtraValues_Count != num)
        {
            delete[] mExtraValues;
            mExtraValues = NULL;
        }

        if (mExtraValues == NULL)
            mExtraValues = new intptr_t[num];

        memcpy(mExtraValues, values, sizeof(intptr_t) * num);
        mExtraValues_Count = num;
    }
    else
        return IMB_INVAL_PARAM;

    return IMB_SUCCESS;
}

IMB_Result IntelMetadataBuffer::UnSerialize(uint8_t* data, uint32_t size)
{
    if (!data || size == 0)
        return IMB_INVAL_PARAM;

    IntelMetadataBufferType type;
    intptr_t value;
    uint32_t extrasize = size - sizeof(type) - sizeof(value);
    ValueInfo* info = NULL;
    intptr_t* ExtraValues = NULL;
    uint32_t ExtraValues_Count = 0;

    memcpy(&type, data, sizeof(type));
    data += sizeof(type);
    memcpy(&value, data, sizeof(value));
    data += sizeof(value);

    switch (type)
    {
        case IntelMetadataBufferTypeCameraSource:
        case IntelMetadataBufferTypeEncoder:
        case IntelMetadataBufferTypeUser:
        {
            if (extrasize >0 && extrasize < sizeof(ValueInfo))
                return IMB_INVAL_BUFFER;

            if (extrasize > sizeof(ValueInfo)) //has extravalues
            {
                if ( (extrasize - sizeof(ValueInfo)) % sizeof(mValue) != 0 )
                    return IMB_INVAL_BUFFER;
                ExtraValues_Count = (extrasize - sizeof(ValueInfo)) / sizeof(mValue);
            }

            if (extrasize > 0)
            {
                info = new ValueInfo;
                memcpy(info, data, sizeof(ValueInfo));
                data += sizeof(ValueInfo);
            }

            if (ExtraValues_Count > 0)
            {
                ExtraValues = new intptr_t[ExtraValues_Count];
                memcpy(ExtraValues, data, ExtraValues_Count * sizeof(mValue));
            }

            break;
        }
        case IntelMetadataBufferTypeGrallocSource:
            if (extrasize > 0)
                return IMB_INVAL_BUFFER;

            break;
        default:
            return IMB_INVAL_BUFFER;
    }

    //store data
    mType = type;
    mValue = value;
    if (mInfo)
        delete mInfo;
    mInfo = info;
    if (mExtraValues)
        delete[] mExtraValues;
    mExtraValues = ExtraValues;
    mExtraValues_Count = ExtraValues_Count;
#ifdef INTEL_VIDEO_XPROC_SHARING
    if (mInfo != NULL)
        mSessionFlag = mInfo->sessionFlag;
#endif
    return IMB_SUCCESS;
}

IMB_Result IntelMetadataBuffer::Serialize(uint8_t* &data, uint32_t& size)
{
    if (mBytes == NULL)
    {
        if (mType == IntelMetadataBufferTypeGrallocSource && mInfo)
            return IMB_INVAL_PARAM;

        //assemble bytes according members
        mSize = sizeof(mType) + sizeof(mValue);
        if (mInfo)
        {
            mSize += sizeof(ValueInfo);
            if (mExtraValues)
                mSize += sizeof(mValue) * mExtraValues_Count;
        }

        mBytes = new uint8_t[mSize];
        uint8_t *ptr = mBytes;
        memcpy(ptr, &mType, sizeof(mType));
        ptr += sizeof(mType);
        memcpy(ptr, &mValue, sizeof(mValue));
        ptr += sizeof(mValue);

        if (mInfo)
        {
        #ifdef INTEL_VIDEO_XPROC_SHARING
            mInfo->sessionFlag = mSessionFlag;
        #endif
            memcpy(ptr, mInfo, sizeof(ValueInfo));
            ptr += sizeof(ValueInfo);

            if (mExtraValues)
                memcpy(ptr, mExtraValues, mExtraValues_Count * sizeof(mValue));
        }
    }

    data = mBytes;
    size = mSize;

    return IMB_SUCCESS;
}

uint32_t IntelMetadataBuffer::GetMaxBufferSize()
{
    return 256;
}

#ifdef INTEL_VIDEO_XPROC_SHARING
IMB_Result IntelMetadataBuffer::GetSessionFlag(uint32_t& sessionflag)
{
    sessionflag = mSessionFlag;

    return IMB_SUCCESS;
}

IMB_Result IntelMetadataBuffer::SetSessionFlag(uint32_t sessionflag)
{
    mSessionFlag = sessionflag;

    return IMB_SUCCESS;
}

IMB_Result IntelMetadataBuffer::ShareValue(sp<MemoryBase> mem)
{
    mValue = (intptr_t)((intptr_t) ( mem->pointer() + 0x0FFF) & ~0x0FFF);

    if ( !(mSessionFlag & REMOTE_PROVIDER) && !(mSessionFlag & REMOTE_CONSUMER)) //no sharing
        return IMB_SUCCESS;

    if (mSessionFlag & REMOTE_PROVIDER) //is remote provider
    {
        sp<IBinder> binder = GetIntelBufferSharingService();
        if (binder == 0)
            return IMB_NO_SERVICE;

        //Detect IntelBufferSharingService, share mem to service
        Parcel data, reply;

        //send pid, sessionflag, and value
        pid_t pid = getpid();
        //TODO: if pid is int32?
        data.writeInt32(pid);
        data.writeInt32(mSessionFlag);
        data.writeIntPtr(mValue);

        //send type/obj (offset/size/MemHeap)
        ShareMemMap smem;
        smem.membase = mem;
        smem.type = ST_MEMBASE;
        if (WriteMemObjToBinder(data, &smem) != NO_ERROR)
            return IMB_SERVICE_FAIL;

        //do transcation
        if (binder->transact(SHARE_MEM, data, &reply) != NO_ERROR)
            return IMB_SERVICE_FAIL;

        //set new value gotten from peer
        mValue = reply.readIntPtr();
//        LOGI("ShareValue(membase) Get reply from sevice, new value:%x\n", mValue);
    }
    else  //is local provider , direct access list
    {
        ShareMemMap* smem = new ShareMemMap;
        smem->sessionflag = mSessionFlag;
        smem->value = mValue;
        smem->value_backup = mValue;
        smem->type = ST_MEMBASE;
        smem->membase = mem;
        smem->gbuffer = NULL;
        PushShareMem(smem);
    }

    return IMB_SUCCESS;
}

IMB_Result IntelMetadataBuffer::ShareValue(sp<GraphicBuffer> gbuffer)
{
    mValue = (intptr_t)gbuffer->handle;

    if ( !(mSessionFlag & REMOTE_PROVIDER) && !(mSessionFlag & REMOTE_CONSUMER)) //no sharing
        return IMB_SUCCESS;

    if (mSessionFlag & REMOTE_PROVIDER == 0) //is remote provider
    {
        sp<IBinder> binder = GetIntelBufferSharingService();
        if (binder == 0)
            return IMB_NO_SERVICE;

        Parcel data, reply;

        //send pid, sessionflag, and memtype
        pid_t pid = getpid();
        //TODO: if pid is int32 ?
        data.writeInt32(pid);
        data.writeInt32(mSessionFlag);
        data.writeIntPtr(mValue);

        //send value/graphicbuffer obj
        ShareMemMap smem;
        smem.gbuffer = gbuffer;
        smem.type = ST_GFX;
        if (WriteMemObjToBinder(data, &smem) != NO_ERROR)
            return IMB_SERVICE_FAIL;

        //do transcation
        if (binder->transact(SHARE_MEM, data, &reply) != NO_ERROR)
            return IMB_SERVICE_FAIL;

        //set new value gotten from peer
        mValue = reply.readIntPtr();
//        LOGI("ShareValue(gfx) Get reply from sevice, new value:%x\n", mValue);
    }
    else //is local provider, direct access list
    {
        ShareMemMap* smem = new ShareMemMap;
        smem->sessionflag = mSessionFlag;
        smem->value = mValue;
        smem->value_backup = mValue;
        smem->type = ST_GFX;
        smem->membase = NULL;
        smem->gbuffer = gbuffer;
        PushShareMem(smem);
    }

    return IMB_SUCCESS;
}

IMB_Result IntelMetadataBuffer::ClearContext(uint32_t sessionflag, bool isProvider)
{
    if ( !(sessionflag & REMOTE_PROVIDER) && !(sessionflag & REMOTE_CONSUMER)) //no sharing
        return IMB_SUCCESS;

    //clear local firstly
    ClearLocalMem(sessionflag);

    //clear mem on service if it is remote user
    if ((isProvider && (sessionflag & REMOTE_PROVIDER)) || (!isProvider && (sessionflag & REMOTE_CONSUMER)))
    {
//        LOGI("CLEAR_MEM sessionflag=%x", sessionflag);

        sp<IBinder> binder = GetIntelBufferSharingService();
        if (binder == 0)
            return IMB_NO_SERVICE;

        //Detect IntelBufferSharingService, unshare mem from service
        Parcel data, reply;

        //send pid and sessionflag
        pid_t pid = getpid();
        //TODO: if pid is int32?
        data.writeInt32(pid);
        data.writeInt32(sessionflag);

        if (binder->transact(CLEAR_MEM, data, &reply) != NO_ERROR)
            return IMB_SERVICE_FAIL;
    }

    return IMB_SUCCESS;
}

uint32_t IntelMetadataBuffer::MakeSessionFlag(bool romoteProvider, bool remoteConsumer, uint16_t sindex)
{
    uint32_t sessionflag = 0;

    if (romoteProvider)
        sessionflag |= REMOTE_PROVIDER;

    if (remoteConsumer)
        sessionflag |= REMOTE_CONSUMER;

    return sessionflag + sindex;
}
#endif
