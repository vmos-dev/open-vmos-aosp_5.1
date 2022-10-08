/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "CameraHal.h"
#include "TICameraParameters.h"

extern "C" {

//#include <timm_osal_interfaces.h>
//#include <timm_osal_trace.h>


};

namespace Ti {
namespace Camera {

///@todo Move these constants to a common header file, preferably in tiler.h
#define STRIDE_8BIT (4 * 1024)
#define STRIDE_16BIT (4 * 1024)

#define ALLOCATION_2D 2

///Utility Macro Declarations

/*--------------------MemoryManager Class STARTS here-----------------------------*/
MemoryManager::MemoryManager() {
    mIonFd = -1;
}

MemoryManager::~MemoryManager() {
    if ( mIonFd >= 0 ) {
        ion_close(mIonFd);
        mIonFd = -1;
    }
}

status_t MemoryManager::initialize() {
    if ( mIonFd == -1 ) {
        mIonFd = ion_open();
        if ( mIonFd < 0 ) {
            CAMHAL_LOGE("ion_open() failed, error: %d", mIonFd);
            mIonFd = -1;
            return NO_INIT;
        }
    }

    return OK;
}

CameraBuffer* MemoryManager::allocateBufferList(int width, int height, const char* format, int &size, int numBufs)
{
    LOG_FUNCTION_NAME;

    CAMHAL_ASSERT(mIonFd != -1);

    ///We allocate numBufs+1 because the last entry will be marked NULL to indicate end of array, which is used when freeing
    ///the buffers
    const uint numArrayEntriesC = (uint)(numBufs+1);

    ///Allocate a buffer array
    CameraBuffer *buffers = new CameraBuffer [numArrayEntriesC];
    if(!buffers) {
        CAMHAL_LOGEB("Allocation failed when creating buffers array of %d CameraBuffer elements", numArrayEntriesC);
        goto error;
    }

    ///Initialize the array with zeros - this will help us while freeing the array in case of error
    ///If a value of an array element is NULL, it means we didnt allocate it
    memset(buffers, 0, sizeof(CameraBuffer) * numArrayEntriesC);

    //2D Allocations are not supported currently
    if(size != 0) {
        struct ion_handle *handle;
        int mmap_fd;
        size_t stride;

        ///1D buffers
        for (int i = 0; i < numBufs; i++) {
            unsigned char *data;
            int ret = ion_alloc(mIonFd, size, 0, 1 << ION_HEAP_TYPE_CARVEOUT,
                    &handle);
            if((ret < 0) || ((int)handle == -ENOMEM)) {
                ret = ion_alloc_tiler(mIonFd, (size_t)size, 1, TILER_PIXEL_FMT_PAGE,
                OMAP_ION_HEAP_TILER_MASK, &handle, &stride);
            }

            if((ret < 0) || ((int)handle == -ENOMEM)) {
                CAMHAL_LOGEB("FAILED to allocate ion buffer of size=%d. ret=%d(0x%x)", size, ret, ret);
                goto error;
            }

            CAMHAL_LOGDB("Before mapping, handle = %p, nSize = %d", handle, size);
            if ((ret = ion_map(mIonFd, handle, size, PROT_READ | PROT_WRITE, MAP_SHARED, 0,
                          &data, &mmap_fd)) < 0) {
                CAMHAL_LOGEB("Userspace mapping of ION buffers returned error %d", ret);
                ion_free(mIonFd, handle);
                goto error;
            }

            buffers[i].type = CAMERA_BUFFER_ION;
            buffers[i].opaque = data;
            buffers[i].mapped = data;
            buffers[i].ion_handle = handle;
            buffers[i].ion_fd = mIonFd;
            buffers[i].fd = mmap_fd;
            buffers[i].size = size;
            buffers[i].format = CameraHal::getPixelFormatConstant(format);

        }
    }

    LOG_FUNCTION_NAME_EXIT;

    return buffers;

error:

    CAMHAL_LOGE("Freeing buffers already allocated after error occurred");
    if(buffers)
        freeBufferList(buffers);

    if ( NULL != mErrorNotifier.get() )
        mErrorNotifier->errorNotify(-ENOMEM);
    LOG_FUNCTION_NAME_EXIT;

    return NULL;
}

CameraBuffer* MemoryManager::getBufferList(int *numBufs) {
    LOG_FUNCTION_NAME;
    if (numBufs) *numBufs = -1;

    return NULL;
}

//TODO: Get needed data to map tiler buffers
//Return dummy data for now
uint32_t * MemoryManager::getOffsets()
{
    LOG_FUNCTION_NAME;

    LOG_FUNCTION_NAME_EXIT;

    return NULL;
}

int MemoryManager::getFd()
{
    LOG_FUNCTION_NAME;

    LOG_FUNCTION_NAME_EXIT;

    return -1;
}

int MemoryManager::freeBufferList(CameraBuffer *buffers)
{
    status_t ret = NO_ERROR;
    LOG_FUNCTION_NAME;

    int i;

    if(!buffers)
        {
        CAMHAL_LOGEA("NULL pointer passed to freebuffer");
        LOG_FUNCTION_NAME_EXIT;
        return BAD_VALUE;
        }

    i = 0;
    while(buffers[i].type == CAMERA_BUFFER_ION)
        {
        if(buffers[i].size)
            {
            munmap(buffers[i].opaque, buffers[i].size);
            close(buffers[i].fd);
            ion_free(mIonFd, buffers[i].ion_handle);
            }
        else
            {
            CAMHAL_LOGEA("Not a valid Memory Manager buffer");
            }
        i++;
        }

    delete [] buffers;

    LOG_FUNCTION_NAME_EXIT;
    return ret;
}

status_t MemoryManager::setErrorHandler(ErrorNotifier *errorNotifier)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    if ( NULL == errorNotifier )
        {
        CAMHAL_LOGEA("Invalid Error Notifier reference");
        ret = -EINVAL;
        }

    if ( NO_ERROR == ret )
        {
        mErrorNotifier = errorNotifier;
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

} // namespace Camera
} // namespace Ti


/*--------------------MemoryManager Class ENDS here-----------------------------*/
