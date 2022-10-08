/* Copyright (c) 2012-2013, The Linux Foundataion. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#define LOG_TAG "QCameraHWI_Mem"

#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <utils/Log.h>
#include <utils/Errors.h>
#include <gralloc_priv.h>
#include "QCamera3Mem.h"

extern "C" {
#include <mm_camera_interface.h>
}

using namespace android;

namespace qcamera {

// QCaemra2Memory base class

/*===========================================================================
 * FUNCTION   : QCamera3Memory
 *
 * DESCRIPTION: default constructor of QCamera3Memory
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
QCamera3Memory::QCamera3Memory()
{
    mBufferCount = 0;
    for (int i = 0; i < MM_CAMERA_MAX_NUM_FRAMES; i++) {
        mMemInfo[i].fd = 0;
        mMemInfo[i].main_ion_fd = 0;
        mMemInfo[i].handle = NULL;
        mMemInfo[i].size = 0;
    }
}

/*===========================================================================
 * FUNCTION   : ~QCamera3Memory
 *
 * DESCRIPTION: deconstructor of QCamera3Memory
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
QCamera3Memory::~QCamera3Memory()
{
}

/*===========================================================================
 * FUNCTION   : cacheOpsInternal
 *
 * DESCRIPTION: ion related memory cache operations
 *
 * PARAMETERS :
 *   @index   : index of the buffer
 *   @cmd     : cache ops command
 *   @vaddr   : ptr to the virtual address
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int QCamera3Memory::cacheOpsInternal(int index, unsigned int cmd, void *vaddr)
{
    struct ion_flush_data cache_inv_data;
    struct ion_custom_data custom_data;
    int ret = OK;

    if (index >= mBufferCount) {
        ALOGE("%s: index %d out of bound [0, %d)", __func__, index, mBufferCount);
        return BAD_INDEX;
    }

    memset(&cache_inv_data, 0, sizeof(cache_inv_data));
    memset(&custom_data, 0, sizeof(custom_data));
    cache_inv_data.vaddr = vaddr;
    cache_inv_data.fd = mMemInfo[index].fd;
    cache_inv_data.handle = mMemInfo[index].handle;
    cache_inv_data.length = mMemInfo[index].size;
    custom_data.cmd = cmd;
    custom_data.arg = (unsigned long)&cache_inv_data;

    ALOGV("%s: addr = %p, fd = %d, handle = %p length = %d, ION Fd = %d",
         __func__, cache_inv_data.vaddr, cache_inv_data.fd,
         cache_inv_data.handle, cache_inv_data.length,
         mMemInfo[index].main_ion_fd);
    ret = ioctl(mMemInfo[index].main_ion_fd, ION_IOC_CUSTOM, &custom_data);
    if (ret < 0)
        ALOGE("%s: Cache Invalidate failed: %s\n", __func__, strerror(errno));

    return ret;
}

/*===========================================================================
 * FUNCTION   : getFd
 *
 * DESCRIPTION: return file descriptor of the indexed buffer
 *
 * PARAMETERS :
 *   @index   : index of the buffer
 *
 * RETURN     : file descriptor
 *==========================================================================*/
int QCamera3Memory::getFd(int index) const
{
    if (index >= mBufferCount)
        return BAD_INDEX;

    return mMemInfo[index].fd;
}

/*===========================================================================
 * FUNCTION   : getSize
 *
 * DESCRIPTION: return buffer size of the indexed buffer
 *
 * PARAMETERS :
 *   @index   : index of the buffer
 *
 * RETURN     : buffer size
 *==========================================================================*/
int QCamera3Memory::getSize(int index) const
{
    if (index >= mBufferCount)
        return BAD_INDEX;

    return (int)mMemInfo[index].size;
}

/*===========================================================================
 * FUNCTION   : getCnt
 *
 * DESCRIPTION: query number of buffers allocated
 *
 * PARAMETERS : none
 *
 * RETURN     : number of buffers allocated
 *==========================================================================*/
int QCamera3Memory::getCnt() const
{
    return mBufferCount;
}

/*===========================================================================
 * FUNCTION   : getBufDef
 *
 * DESCRIPTION: query detailed buffer information
 *
 * PARAMETERS :
 *   @offset  : [input] frame buffer offset
 *   @bufDef  : [output] reference to struct to store buffer definition
 *   @index   : [input] index of the buffer
 *
 * RETURN     : none
 *==========================================================================*/
void QCamera3Memory::getBufDef(const cam_frame_len_offset_t &offset,
        mm_camera_buf_def_t &bufDef, int index) const
{
    if (!mBufferCount) {
        ALOGE("Memory not allocated");
        return;
    }
    bufDef.fd = mMemInfo[index].fd;
    bufDef.frame_len = mMemInfo[index].size;
    bufDef.mem_info = (void *)this;
    bufDef.num_planes = offset.num_planes;
    bufDef.buffer = getPtr(index);
    bufDef.buf_idx = index;

    /* Plane 0 needs to be set separately. Set other planes in a loop */
    bufDef.planes[0].length = offset.mp[0].len;
    bufDef.planes[0].m.userptr = mMemInfo[index].fd;
    bufDef.planes[0].data_offset = offset.mp[0].offset;
    bufDef.planes[0].reserved[0] = 0;
    for (int i = 1; i < bufDef.num_planes; i++) {
         bufDef.planes[i].length = offset.mp[i].len;
         bufDef.planes[i].m.userptr = mMemInfo[i].fd;
         bufDef.planes[i].data_offset = offset.mp[i].offset;
         bufDef.planes[i].reserved[0] =
                 bufDef.planes[i-1].reserved[0] +
                 bufDef.planes[i-1].length;
    }
}

/*===========================================================================
 * FUNCTION   : QCamera3HeapMemory
 *
 * DESCRIPTION: constructor of QCamera3HeapMemory for ion memory used internally in HAL
 *
 * PARAMETERS : none
 *
 * RETURN     : none
 *==========================================================================*/
QCamera3HeapMemory::QCamera3HeapMemory()
    : QCamera3Memory()
{
    for (int i = 0; i < MM_CAMERA_MAX_NUM_FRAMES; i ++)
        mPtr[i] = NULL;
}

/*===========================================================================
 * FUNCTION   : ~QCamera3HeapMemory
 *
 * DESCRIPTION: deconstructor of QCamera3HeapMemory
 *
 * PARAMETERS : none
 *
 * RETURN     : none
 *==========================================================================*/
QCamera3HeapMemory::~QCamera3HeapMemory()
{
}

/*===========================================================================
 * FUNCTION   : alloc
 *
 * DESCRIPTION: allocate requested number of buffers of certain size
 *
 * PARAMETERS :
 *   @count   : number of buffers to be allocated
 *   @size    : lenght of the buffer to be allocated
 *   @heap_id : heap id to indicate where the buffers will be allocated from
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int QCamera3HeapMemory::alloc(int count, int size, int heap_id)
{
    int rc = OK;
    if (count > MM_CAMERA_MAX_NUM_FRAMES) {
        ALOGE("Buffer count %d out of bound. Max is %d", count, MM_CAMERA_MAX_NUM_FRAMES);
        return BAD_INDEX;
    }
    if (mBufferCount) {
        ALOGE("Allocating a already allocated heap memory");
        return INVALID_OPERATION;
    }

    for (int i = 0; i < count; i ++) {
        rc = allocOneBuffer(mMemInfo[i], heap_id, size);
        if (rc < 0) {
            ALOGE("AllocateIonMemory failed");
            for (int j = i-1; j >= 0; j--)
                deallocOneBuffer(mMemInfo[j]);
            break;
        }
    }
    return rc;
}

/*===========================================================================
 * FUNCTION   : dealloc
 *
 * DESCRIPTION: deallocate buffers
 *
 * PARAMETERS : none
 *
 * RETURN     : none
 *==========================================================================*/
void QCamera3HeapMemory::dealloc()
{
    for (int i = 0; i < mBufferCount; i++)
        deallocOneBuffer(mMemInfo[i]);
}

/*===========================================================================
 * FUNCTION   : allocOneBuffer
 *
 * DESCRIPTION: impl of allocating one buffers of certain size
 *
 * PARAMETERS :
 *   @memInfo : [output] reference to struct to store additional memory allocation info
 *   @heap    : [input] heap id to indicate where the buffers will be allocated from
 *   @size    : [input] lenght of the buffer to be allocated
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int QCamera3HeapMemory::allocOneBuffer(QCamera3MemInfo &memInfo, int heap_id, int size)
{
    int rc = OK;
    struct ion_handle_data handle_data;
    struct ion_allocation_data alloc;
    struct ion_fd_data ion_info_fd;
    int main_ion_fd = 0;

    main_ion_fd = open("/dev/ion", O_RDONLY);
    if (main_ion_fd <= 0) {
        ALOGE("Ion dev open failed: %s\n", strerror(errno));
        goto ION_OPEN_FAILED;
    }

    memset(&alloc, 0, sizeof(alloc));
    alloc.len = size;
    /* to make it page size aligned */
    alloc.len = (alloc.len + 4095) & (~4095);
    alloc.align = 4096;
    alloc.flags = ION_FLAG_CACHED;
    alloc.heap_id_mask = heap_id;
    rc = ioctl(main_ion_fd, ION_IOC_ALLOC, &alloc);
    if (rc < 0) {
        ALOGE("ION allocation for len %d failed: %s\n", alloc.len,
            strerror(errno));
        goto ION_ALLOC_FAILED;
    }

    memset(&ion_info_fd, 0, sizeof(ion_info_fd));
    ion_info_fd.handle = alloc.handle;
    rc = ioctl(main_ion_fd, ION_IOC_SHARE, &ion_info_fd);
    if (rc < 0) {
        ALOGE("ION map failed %s\n", strerror(errno));
        goto ION_MAP_FAILED;
    }

    memInfo.main_ion_fd = main_ion_fd;
    memInfo.fd = ion_info_fd.fd;
    memInfo.handle = ion_info_fd.handle;
    memInfo.size = alloc.len;
    return OK;

ION_MAP_FAILED:
    memset(&handle_data, 0, sizeof(handle_data));
    handle_data.handle = ion_info_fd.handle;
    ioctl(main_ion_fd, ION_IOC_FREE, &handle_data);
ION_ALLOC_FAILED:
    close(main_ion_fd);
ION_OPEN_FAILED:
    return NO_MEMORY;
}

/*===========================================================================
 * FUNCTION   : deallocOneBuffer
 *
 * DESCRIPTION: impl of deallocating one buffers
 *
 * PARAMETERS :
 *   @memInfo : reference to struct that stores additional memory allocation info
 *
 * RETURN     : none
 *==========================================================================*/
void QCamera3HeapMemory::deallocOneBuffer(QCamera3MemInfo &memInfo)
{
    struct ion_handle_data handle_data;

    if (memInfo.fd > 0) {
        close(memInfo.fd);
        memInfo.fd = 0;
    }

    if (memInfo.main_ion_fd > 0) {
        memset(&handle_data, 0, sizeof(handle_data));
        handle_data.handle = memInfo.handle;
        ioctl(memInfo.main_ion_fd, ION_IOC_FREE, &handle_data);
        close(memInfo.main_ion_fd);
        memInfo.main_ion_fd = 0;
    }
    memInfo.handle = NULL;
    memInfo.size = 0;
}

/*===========================================================================
 * FUNCTION   : getPtr
 *
 * DESCRIPTION: return buffer pointer
 *
 * PARAMETERS :
 *   @index   : index of the buffer
 *
 * RETURN     : buffer ptr
 *==========================================================================*/
void *QCamera3HeapMemory::getPtr(int index) const
{
    if (index >= mBufferCount) {
        ALOGE("index out of bound");
        return (void *)BAD_INDEX;
    }
    return mPtr[index];
}

/*===========================================================================
 * FUNCTION   : allocate
 *
 * DESCRIPTION: allocate requested number of buffers of certain size
 *
 * PARAMETERS :
 *   @count   : number of buffers to be allocated
 *   @size    : lenght of the buffer to be allocated
 *   @queueAll: whether to queue all allocated buffers at the beginning
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int QCamera3HeapMemory::allocate(int count, int size, bool queueAll)
{
    int heap_id_mask = 0x1 << ION_IOMMU_HEAP_ID;
    int rc = alloc(count, size, heap_id_mask);
    if (rc < 0)
        return rc;

    for (int i = 0; i < count; i ++) {
        void *vaddr = mmap(NULL,
                    mMemInfo[i].size,
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED,
                    mMemInfo[i].fd, 0);
        if (vaddr == MAP_FAILED) {
            for (int j = i-1; j >= 0; j --) {
                munmap(mPtr[i], mMemInfo[i].size);
                rc = NO_MEMORY;
                break;
            }
        } else
            mPtr[i] = vaddr;
    }
    if (rc == 0)
        mBufferCount = count;

    mQueueAll = queueAll;
    return OK;
}

/*===========================================================================
 * FUNCTION   : deallocate
 *
 * DESCRIPTION: deallocate buffers
 *
 * PARAMETERS : none
 *
 * RETURN     : none
 *==========================================================================*/
void QCamera3HeapMemory::deallocate()
{
    for (int i = 0; i < mBufferCount; i++) {
        munmap(mPtr[i], mMemInfo[i].size);
        mPtr[i] = NULL;
    }
    dealloc();
    mBufferCount = 0;
}

/*===========================================================================
 * FUNCTION   : cacheOps
 *
 * DESCRIPTION: ion related memory cache operations
 *
 * PARAMETERS :
 *   @index   : index of the buffer
 *   @cmd     : cache ops command
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int QCamera3HeapMemory::cacheOps(int index, unsigned int cmd)
{
    if (index >= mBufferCount)
        return BAD_INDEX;
    return cacheOpsInternal(index, cmd, mPtr[index]);
}

/*===========================================================================
 * FUNCTION   : getRegFlags
 *
 * DESCRIPTION: query initial reg flags
 *
 * PARAMETERS :
 *   @regFlags: initial reg flags of the allocated buffers
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int QCamera3HeapMemory::getRegFlags(uint8_t * regFlags) const
{
    int i;
    for (i = 0; i < mBufferCount; i ++)
        regFlags[i] = (mQueueAll ? 1 : 0);
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : getMatchBufIndex
 *
 * DESCRIPTION: query buffer index by object ptr
 *
 * PARAMETERS :
 *   @object  : object ptr
 *
 * RETURN     : buffer index if match found,
 *              -1 if failed
 *==========================================================================*/
int QCamera3HeapMemory::getMatchBufIndex(void * /*object*/)
{

/*
    TODO for HEAP memory type, would there be an equivalent requirement?

    int index = -1;
    buffer_handle_t *key = (buffer_handle_t*) object;
    if (!key) {
        return BAD_VALUE;
    }
    for (int i = 0; i < mBufferCount; i++) {
        if (mBufferHandle[i] == key) {
            index = i;
            break;
        }
    }
    return index;
*/
    ALOGE("%s: FATAL: Not supposed to come here", __func__);
    return -1;
}

/*===========================================================================
 * FUNCTION   : QCamera3GrallocMemory
 *
 * DESCRIPTION: constructor of QCamera3GrallocMemory
 *              preview stream buffers are allocated from gralloc native_windoe
 *
 * PARAMETERS :
 *   @getMemory : camera memory request ops table
 *
 * RETURN     : none
 *==========================================================================*/
QCamera3GrallocMemory::QCamera3GrallocMemory()
        : QCamera3Memory()
{
    for (int i = 0; i < MM_CAMERA_MAX_NUM_FRAMES; i ++) {
        mBufferHandle[i] = NULL;
        mPrivateHandle[i] = NULL;
        mCurrentFrameNumbers[i] = -1;
    }
}

/*===========================================================================
 * FUNCTION   : ~QCamera3GrallocMemory
 *
 * DESCRIPTION: deconstructor of QCamera3GrallocMemory
 *
 * PARAMETERS : none
 *
 * RETURN     : none
 *==========================================================================*/
QCamera3GrallocMemory::~QCamera3GrallocMemory()
{
}

/*===========================================================================
 * FUNCTION   : registerBuffers
 *
 * DESCRIPTION: register frameworks-allocated gralloc buffer_handle_t
 *
 * PARAMETERS :
 *   @num_buffer : number of buffers to be registered
 *   @buffers    : array of buffer_handle_t pointers
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int QCamera3GrallocMemory::registerBuffers(uint32_t num_buffers, buffer_handle_t **buffers)
{
    status_t ret = NO_ERROR;
    struct ion_fd_data ion_info_fd;
    ALOGV(" %s : E ", __FUNCTION__);

    memset(&ion_info_fd, 0, sizeof(ion_info_fd));


    if (num_buffers > MM_CAMERA_MAX_NUM_FRAMES) {
        ALOGE("%s: Number of buffers %d greater than what's supported %d",
            __func__, num_buffers, MM_CAMERA_MAX_NUM_FRAMES);
        return -EINVAL;
    }

    for (size_t cnt = 0; cnt < num_buffers; cnt++) {
        if (buffers[cnt] == NULL) {
            ALOGE("%s: Invalid buffers[%d].", __func__, cnt);
            return -EINVAL;
        }
        mBufferHandle[cnt] = buffers[cnt];
        mPrivateHandle[cnt] =
            (struct private_handle_t *)(*mBufferHandle[cnt]);
        mMemInfo[cnt].main_ion_fd = open("/dev/ion", O_RDONLY);
        if (mMemInfo[cnt].main_ion_fd < 0) {
            ALOGE("%s: failed: could not open ion device", __func__);
            for(size_t i = 0; i < cnt; i++) {
                struct ion_handle_data ion_handle;
                memset(&ion_handle, 0, sizeof(ion_handle));
                ion_handle.handle = mMemInfo[i].handle;
                if (ioctl(mMemInfo[i].main_ion_fd, ION_IOC_FREE, &ion_handle) < 0) {
                    ALOGE("%s: ion free failed", __func__);
                }
                close(mMemInfo[i].main_ion_fd);
                ALOGD("%s: cancel_buffer: hdl =%p", __func__, (*mBufferHandle[i]));
                mBufferHandle[i] = NULL;
            }
            memset(&mMemInfo, 0, sizeof(mMemInfo));
            ret = -ENOMEM;
            goto end;
        } else {
            ion_info_fd.fd = mPrivateHandle[cnt]->fd;
            if (ioctl(mMemInfo[cnt].main_ion_fd,
                      ION_IOC_IMPORT, &ion_info_fd) < 0) {
                ALOGE("%s: ION import failed\n", __func__);
                for(size_t i = 0; i < cnt; i++) {
                    struct ion_handle_data ion_handle;
                    memset(&ion_handle, 0, sizeof(ion_handle));
                    ion_handle.handle = mMemInfo[i].handle;
                    if (ioctl(mMemInfo[i].main_ion_fd, ION_IOC_FREE, &ion_handle) < 0) {
                        ALOGE("ion free failed");
                    }
                    close(mMemInfo[i].main_ion_fd);
                    mBufferHandle[i] = NULL;
                }
                close(mMemInfo[cnt].main_ion_fd);
                memset(&mMemInfo, 0, sizeof(mMemInfo));
                ret = -ENOMEM;
                goto end;
            }
        }
        ALOGD("%s: idx = %d, fd = %d, size = %d, offset = %d",
              __func__, cnt, mPrivateHandle[cnt]->fd,
              mPrivateHandle[cnt]->size,
              mPrivateHandle[cnt]->offset);
        mMemInfo[cnt].fd =
            mPrivateHandle[cnt]->fd;
        mMemInfo[cnt].size =
            mPrivateHandle[cnt]->size;
        mMemInfo[cnt].handle = ion_info_fd.handle;

        void *vaddr = mmap(NULL,
                    mMemInfo[cnt].size,
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED,
                    mMemInfo[cnt].fd, 0);
        if (vaddr == MAP_FAILED) {
            for (int j = cnt-1; j >= 0; j --) {
                munmap(mPtr[cnt], mMemInfo[cnt].size);
                ret = -ENOMEM;
                break;
            }
        } else
            mPtr[cnt] = vaddr;
    }
    mBufferCount = num_buffers;

end:
    ALOGV(" %s : X ",__func__);
    return ret;
}

/*===========================================================================
 * FUNCTION   : unregisterBuffers
 *
 * DESCRIPTION: unregister buffers
 *
 * PARAMETERS : none
 *
 * RETURN     : none
 *==========================================================================*/
void QCamera3GrallocMemory::unregisterBuffers()
{
    ALOGV("%s: E ", __FUNCTION__);

    for (int cnt = 0; cnt < mBufferCount; cnt++) {
        munmap(mPtr[cnt], mMemInfo[cnt].size);
        mPtr[cnt] = NULL;

        struct ion_handle_data ion_handle;
        memset(&ion_handle, 0, sizeof(ion_handle));
        ion_handle.handle = mMemInfo[cnt].handle;
        if (ioctl(mMemInfo[cnt].main_ion_fd, ION_IOC_FREE, &ion_handle) < 0) {
            ALOGE("ion free failed");
        }
        close(mMemInfo[cnt].main_ion_fd);
        ALOGD("put buffer %d successfully", cnt);
    }
    mBufferCount = 0;
    ALOGV(" %s : X ",__FUNCTION__);
}

/*===========================================================================
 * FUNCTION   : markFrameNumber
 *
 * DESCRIPTION: We use this function from the request call path to mark the
 *              buffers with the frame number they are intended for this info
 *              is used later when giving out callback & it is duty of PP to
 *              ensure that data for that particular frameNumber/Request is
 *              written to this buffer.
 * PARAMETERS :
 *   @index   : index of the buffer
 *   @frame#  : Frame number from the framework
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCamera3GrallocMemory::markFrameNumber(int index, uint32_t frameNumber)
{
    if(index >= mBufferCount || index >= MM_CAMERA_MAX_NUM_FRAMES) {
        ALOGE("%s: Index out of bounds",__func__);
        return BAD_INDEX;
    }
    mCurrentFrameNumbers[index] = frameNumber;
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : getFrameNumber
 *
 * DESCRIPTION: We use this to fetch the frameNumber for the request with which
 *              this buffer was given to HAL
 *
 *
 * PARAMETERS :
 *   @index   : index of the buffer
 *
 * RETURN     : int32_t frameNumber
 *              postive/zero  -- success
 *              negetive failure
 *==========================================================================*/
int32_t QCamera3GrallocMemory::getFrameNumber(int index)
{
    if(index >= mBufferCount || index >= MM_CAMERA_MAX_NUM_FRAMES) {
        ALOGE("%s: Index out of bounds",__func__);
        return -1;
    }

    return mCurrentFrameNumbers[index];
}

/*===========================================================================
 * FUNCTION   : cacheOps
 *
 * DESCRIPTION: ion related memory cache operations
 *
 * PARAMETERS :
 *   @index   : index of the buffer
 *   @cmd     : cache ops command
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int QCamera3GrallocMemory::cacheOps(int index, unsigned int cmd)
{
    if (index >= mBufferCount)
        return BAD_INDEX;
    return cacheOpsInternal(index, cmd, mPtr[index]);
}

/*===========================================================================
 * FUNCTION   : getRegFlags
 *
 * DESCRIPTION: query initial reg flags
 *
 * PARAMETERS :
 *   @regFlags: initial reg flags of the allocated buffers
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int QCamera3GrallocMemory::getRegFlags(uint8_t *regFlags) const
{
    int i;
    for (i = 0; i < mBufferCount; i ++)
        regFlags[i] = 0;
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : getMatchBufIndex
 *
 * DESCRIPTION: query buffer index by object ptr
 *
 * PARAMETERS :
 *   @opaque  : opaque ptr
 *
 * RETURN     : buffer index if match found,
 *              -1 if failed
 *==========================================================================*/
int QCamera3GrallocMemory::getMatchBufIndex(void *object)
{
    int index = -1;
    buffer_handle_t *key = (buffer_handle_t*) object;
    if (!key) {
        return BAD_VALUE;
    }
    for (int i = 0; i < mBufferCount; i++) {
        if (mBufferHandle[i] == key) {
            index = i;
            break;
        }
    }
    return index;
}

/*===========================================================================
 * FUNCTION   : getPtr
 *
 * DESCRIPTION: return buffer pointer
 *
 * PARAMETERS :
 *   @index   : index of the buffer
 *
 * RETURN     : buffer ptr
 *==========================================================================*/
void *QCamera3GrallocMemory::getPtr(int index) const
{
    if (index >= mBufferCount) {
        ALOGE("index out of bound");
        return (void *)BAD_INDEX;
    }
    return mPtr[index];
}

}; //namespace qcamera
