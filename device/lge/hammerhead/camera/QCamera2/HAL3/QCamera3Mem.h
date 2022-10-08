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

#ifndef __QCAMERA3HWI_MEM_H__
#define __QCAMERA3HWI_MEM_H__
#include <hardware/camera3.h>
#include <utils/Mutex.h>

extern "C" {
#include <sys/types.h>
#include <linux/msm_ion.h>
#include <mm_camera_interface.h>
}

namespace qcamera {

// Base class for all memory types. Abstract.
class QCamera3Memory {

public:
    int cleanCache(int index) {return cacheOps(index, ION_IOC_CLEAN_CACHES);}
    int invalidateCache(int index) {return cacheOps(index, ION_IOC_INV_CACHES);}
    int cleanInvalidateCache(int index) {return cacheOps(index, ION_IOC_CLEAN_INV_CACHES);}
    int getFd(int index) const;
    int getSize(int index) const;
    int getCnt() const;

    virtual int cacheOps(int index, unsigned int cmd) = 0;
    virtual int getRegFlags(uint8_t *regFlags) const = 0;
    virtual int getMatchBufIndex(void *object) = 0;
    virtual void *getPtr(int index) const= 0;

    QCamera3Memory();
    virtual ~QCamera3Memory();

    int32_t getBufDef(const cam_frame_len_offset_t &offset,
                mm_camera_buf_def_t &bufDef, int index) const;

protected:
    struct QCamera3MemInfo {
        int fd;
        int main_ion_fd;
        struct ion_handle *handle;
        uint32_t size;
    };

    int cacheOpsInternal(int index, unsigned int cmd, void *vaddr);

    int mBufferCount;
    struct QCamera3MemInfo mMemInfo[MM_CAMERA_MAX_NUM_FRAMES];
    void *mPtr[MM_CAMERA_MAX_NUM_FRAMES];
};

// Internal heap memory is used for memories used internally
// They are allocated from /dev/ion. Examples are: capabilities,
// parameters, metadata, and internal YUV data for jpeg encoding.
class QCamera3HeapMemory : public QCamera3Memory {
public:
    QCamera3HeapMemory();
    virtual ~QCamera3HeapMemory();

    int allocate(int count, int size, bool queueAll);
    void deallocate();

    virtual int cacheOps(int index, unsigned int cmd);
    virtual int getRegFlags(uint8_t *regFlags) const;
    virtual int getMatchBufIndex(void *object);
    virtual void *getPtr(int index) const;
private:
    int alloc(int count, int size, int heap_id);
    void dealloc();

    int allocOneBuffer(struct QCamera3MemInfo &memInfo, int heap_id, int size);
    void deallocOneBuffer(struct QCamera3MemInfo &memInfo);
    bool mQueueAll;
};

// Gralloc Memory shared with frameworks
class QCamera3GrallocMemory : public QCamera3Memory {
public:
    QCamera3GrallocMemory();
    virtual ~QCamera3GrallocMemory();

    int registerBuffer(buffer_handle_t *buffer);
    void unregisterBuffers();
    virtual int cacheOps(int index, unsigned int cmd);
    virtual int getRegFlags(uint8_t *regFlags) const;
    virtual int getMatchBufIndex(void *object);
    virtual void *getPtr(int index) const;
    int32_t markFrameNumber(int index, uint32_t frameNumber);
    int32_t getFrameNumber(int index);
    void *getBufferHandle(int index);
private:
    buffer_handle_t *mBufferHandle[MM_CAMERA_MAX_NUM_FRAMES];
    struct private_handle_t *mPrivateHandle[MM_CAMERA_MAX_NUM_FRAMES];
    uint32_t mCurrentFrameNumbers[MM_CAMERA_MAX_NUM_FRAMES];
};

};
#endif
