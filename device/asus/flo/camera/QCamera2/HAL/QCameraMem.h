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

#ifndef __QCAMERA2HWI_MEM_H__
#define __QCAMERA2HWI_MEM_H__

#include <hardware/camera.h>
#include <utils/Mutex.h>

extern "C" {
#include <sys/types.h>
#include <linux/msm_ion.h>
#include <mm_camera_interface.h>
}

namespace qcamera {

// Base class for all memory types. Abstract.
class QCameraMemory {

public:
    int cleanCache(int index) {return cacheOps(index, ION_IOC_CLEAN_CACHES);}
    int invalidateCache(int index) {return cacheOps(index, ION_IOC_INV_CACHES);}
    int cleanInvalidateCache(int index) {return cacheOps(index, ION_IOC_CLEAN_INV_CACHES);}
    int getFd(int index) const;
    int getSize(int index) const;
    int getCnt() const;

    virtual int allocate(int count, int size) = 0;
    virtual void deallocate() = 0;
    virtual int cacheOps(int index, unsigned int cmd) = 0;
    virtual int getRegFlags(uint8_t *regFlags) const = 0;
    virtual camera_memory_t *getMemory(int index, bool metadata) const = 0;
    virtual int getMatchBufIndex(const void *opaque, bool metadata) const = 0;
    virtual void *getPtr(int index) const= 0;

    QCameraMemory(bool cached);
    virtual ~QCameraMemory();

    void getBufDef(const cam_frame_len_offset_t &offset,
                mm_camera_buf_def_t &bufDef, int index) const;

protected:
    struct QCameraMemInfo {
        int fd;
        int main_ion_fd;
        struct ion_handle *handle;
        uint32_t size;
    };

    int alloc(int count, int size, int heap_id);
    void dealloc();
    int allocOneBuffer(struct QCameraMemInfo &memInfo, int heap_id, int size);
    void deallocOneBuffer(struct QCameraMemInfo &memInfo);
    int cacheOpsInternal(int index, unsigned int cmd, void *vaddr);

    bool m_bCached;
    int mBufferCount;
    struct QCameraMemInfo mMemInfo[MM_CAMERA_MAX_NUM_FRAMES];
};

// Internal heap memory is used for memories used internally
// They are allocated from /dev/ion.
class QCameraHeapMemory : public QCameraMemory {
public:
    QCameraHeapMemory(bool cached);
    virtual ~QCameraHeapMemory();

    virtual int allocate(int count, int size);
    virtual void deallocate();
    virtual int cacheOps(int index, unsigned int cmd);
    virtual int getRegFlags(uint8_t *regFlags) const;
    virtual camera_memory_t *getMemory(int index, bool metadata) const;
    virtual int getMatchBufIndex(const void *opaque, bool metadata) const;
	virtual void *getPtr(int index) const;

private:
    void *mPtr[MM_CAMERA_MAX_NUM_FRAMES];
};

// Externel heap memory is used for memories shared with
// framework. They are allocated from /dev/ion or gralloc.
class QCameraStreamMemory : public QCameraMemory {
public:
    QCameraStreamMemory(camera_request_memory getMemory, bool cached);
    virtual ~QCameraStreamMemory();

    virtual int allocate(int count, int size);
    virtual void deallocate();
    virtual int cacheOps(int index, unsigned int cmd);
    virtual int getRegFlags(uint8_t *regFlags) const;
    virtual camera_memory_t *getMemory(int index, bool metadata) const;
    virtual int getMatchBufIndex(const void *opaque, bool metadata) const;
	virtual void *getPtr(int index) const;

protected:
    camera_request_memory mGetMemory;
    camera_memory_t *mCameraMemory[MM_CAMERA_MAX_NUM_FRAMES];
};

// Externel heap memory is used for memories shared with
// framework. They are allocated from /dev/ion or gralloc.
class QCameraVideoMemory : public QCameraStreamMemory {
public:
    QCameraVideoMemory(camera_request_memory getMemory, bool cached);
    virtual ~QCameraVideoMemory();

    virtual int allocate(int count, int size);
    virtual void deallocate();
    virtual camera_memory_t *getMemory(int index, bool metadata) const;
    virtual int getMatchBufIndex(const void *opaque, bool metadata) const;

private:
    camera_memory_t *mMetadata[MM_CAMERA_MAX_NUM_FRAMES];
};
;

// Gralloc Memory is acquired from preview window
class QCameraGrallocMemory : public QCameraMemory {
    enum {
        BUFFER_NOT_OWNED,
        BUFFER_OWNED,
    };
public:
    QCameraGrallocMemory(camera_request_memory getMemory);
    void setNativeWindow(preview_stream_ops_t *anw);
    virtual ~QCameraGrallocMemory();

    virtual int allocate(int count, int size);
    virtual void deallocate();
    virtual int cacheOps(int index, unsigned int cmd);
    virtual int getRegFlags(uint8_t *regFlags) const;
    virtual camera_memory_t *getMemory(int index, bool metadata) const;
    virtual int getMatchBufIndex(const void *opaque, bool metadata) const;
	virtual void *getPtr(int index) const;

    void setWindowInfo(preview_stream_ops_t *window, int width, int height, int format);
    // Enqueue/display buffer[index] onto the native window,
    // and dequeue one buffer from it.
    // Returns the buffer index of the dequeued buffer.
    int displayBuffer(int index);

private:
    buffer_handle_t *mBufferHandle[MM_CAMERA_MAX_NUM_FRAMES];
    int mLocalFlag[MM_CAMERA_MAX_NUM_FRAMES];
    struct private_handle_t *mPrivateHandle[MM_CAMERA_MAX_NUM_FRAMES];
    preview_stream_ops_t *mWindow;
    int mWidth, mHeight, mFormat;
    camera_request_memory mGetMemory;
    camera_memory_t *mCameraMemory[MM_CAMERA_MAX_NUM_FRAMES];
    int mMinUndequeuedBuffers;
};

}; // namespace qcamera

#endif /* __QCAMERA2HWI_MEM_H__ */
