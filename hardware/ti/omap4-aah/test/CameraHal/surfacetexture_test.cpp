/*
 * Copyright (c) 2010, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <semaphore.h>
#include <pthread.h>
#include <string.h>
#include <climits>

#include <gui/GLConsumer.h>
#include <gui/Surface.h>
#include <ui/GraphicBuffer.h>
#include <ui/GraphicBufferMapper.h>

#include <camera/Camera.h>
#include <camera/ICamera.h>
#include <media/mediarecorder.h>

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <cutils/properties.h>
#include <camera/CameraParameters.h>
#include <camera/ShotParameters.h>
#include <camera/CameraMetadata.h>
#include <system/audio.h>
#include <system/camera.h>

#include <cutils/memory.h>
#include <utils/Log.h>

#include <sys/wait.h>

#include <sys/mman.h>

#ifdef ANDROID_API_JB_OR_LATER
#include <gui/Surface.h>
#include <gui/ISurface.h>
#include <gui/ISurfaceComposer.h>
#include <gui/ISurfaceComposerClient.h>
#include <gui/SurfaceComposerClient.h>
#include <ion/ion.h>
#else
#include <surfaceflinger/Surface.h>
#include <surfaceflinger/ISurface.h>
#include <surfaceflinger/ISurfaceComposer.h>
#include <surfaceflinger/ISurfaceComposerClient.h>
#include <surfaceflinger/SurfaceComposerClient.h>
#include "ion.h"
#endif

#include "camera_test.h"

#define ASSERT(X) \
    do { \
       if(!(X)) { \
           printf("error: %s():%d", __FUNCTION__, __LINE__); \
           return; \
       } \
    } while(0);

#define ALIGN_DOWN(x, n) ((x) & (~((n) - 1)))
#define ALIGN_UP(x, n) ((((x) + (n) - 1)) & (~((n) - 1)))
#define ALIGN_WIDTH 32 // Should be 32...but the calculated dimension causes an ion crash
#define ALIGN_HEIGHT 2 // Should be 2...but the calculated dimension causes an ion crash

//temporarily define format here
#define HAL_PIXEL_FORMAT_TI_NV12 0x100
#define HAL_PIXEL_FORMAT_TI_NV12_1D 0x102
#define HAL_PIXEL_FORMAT_TI_Y8 0x103
#define HAL_PIXEL_FORMAT_TI_Y16 0x104

using namespace android;

#define N_BUFFERS 15

static void
test_format (int format, int page_mode, int width, int height)
{
    sp<GLConsumer> st;
    Surface *stc;
    GLint tex_id = 0;
    sp<ANativeWindow> anw;
    ANativeWindowBuffer* anb[30] = { 0 };
    int i;
    unsigned int usage;
    GraphicBufferMapper &mapper = GraphicBufferMapper::get();

    printf("testing format %x, page_mode %d\n", format, page_mode);

    sp<BufferQueue> bq = new BufferQueue();
    st = new GLConsumer (bq, tex_id, GL_TEXTURE_EXTERNAL_OES);

    st->setDefaultBufferSize (width, height);

    stc = new Surface(st);
    anw = stc;

    usage = GRALLOC_USAGE_SW_READ_RARELY |
            GRALLOC_USAGE_SW_WRITE_NEVER;
    if (page_mode) {
        usage |= GRALLOC_USAGE_PRIVATE_0;
    }

    native_window_set_usage(anw.get(), usage);
    native_window_set_buffer_count(anw.get(), N_BUFFERS);
    native_window_set_buffers_geometry(anw.get(),
            width, height, format);

    for(i=0;i<N_BUFFERS;i++) {
        void *data = NULL;
        Rect bounds(width, height);

        anb[i] = NULL;
        anw->dequeueBuffer(anw.get(), &anb[i]);
        printf("%d: %p\n", i, anb[i]);
        if (anb[i] == NULL) {
            printf ("FAILED: buffer should be non-NULL\n");
            break;
        }

        mapper.lock(anb[i]->handle, GRALLOC_USAGE_SW_READ_RARELY,
                bounds, &data);
        if (data == NULL) {
            printf ("FAILED: mapping should be non-NULL\n");
            break;
        }

        mapper.unlock(anb[i]->handle);
    }
    for(i=0;i<N_BUFFERS;i++) {
        if (anb[i]) {
            anw->cancelBuffer (anw.get(), anb[i]);
        }
    }

    //delete stc;
    st.clear();
}

void
ion_test (void)
{
    struct ion_handle *handle;
    int fd;
    int map_fd;
    unsigned char *ptr;
    int i;
    int ret;
    int share_fd;

    fd = ion_open ();

#define SIZE (10*1024*1024)
    for(i=0;i<10;i++){
        handle = NULL;
        ret = ion_alloc (fd, SIZE, 4096, (1<<0), &handle);
        if (ret < 0) {
            printf("ion_alloc returned error %d, %s\n", ret, strerror(errno));
            break;
        }
        printf("ion_alloc returned %d\n", ret);

        ret = ion_share (fd, handle, &share_fd);
        if (ret < 0) {
            printf("ion_share returned error %d, %s\n", ret, strerror(errno));
            break;
        }
        printf("ion_share returned %d\n", ret);

        ptr = (unsigned char *)mmap (NULL, SIZE, PROT_READ|PROT_WRITE,
                MAP_SHARED, share_fd, 0);
        printf("mmap returned %p\n", ptr);

        ptr = (unsigned char *)mmap (NULL, SIZE, PROT_READ|PROT_WRITE,
                MAP_SHARED, share_fd, 0);
        printf("mmap returned %p\n", ptr);

#if 0
        ret = ion_map (fd, handle, SIZE, PROT_READ, 0, 0, &ptr, &map_fd);
        if (ret < 0) {
            printf("ion_map returned error %d, %s\n", ret, strerror(errno));
            break;
        }
        printf("ion_map returned %d\n", ret);
#endif

        printf("%d: %p\n", i, ptr);

        ion_free (fd, handle);
    }

}


int
main (int argc, char *argv[])
{
    int width, height;

    width = 640;
    height = 480;
    test_format (HAL_PIXEL_FORMAT_TI_NV12, 0, width, height);
    test_format (HAL_PIXEL_FORMAT_TI_NV12, 1, width, height);
    test_format (HAL_PIXEL_FORMAT_TI_NV12_1D, 0, width, height);
    test_format (HAL_PIXEL_FORMAT_TI_Y8, 1, width, height);
    test_format (HAL_PIXEL_FORMAT_TI_Y16, 1, width, height);

    width = 2608;
    height = 1960;
    test_format (HAL_PIXEL_FORMAT_TI_NV12, 1, width, height);
    test_format (HAL_PIXEL_FORMAT_TI_NV12_1D, 0, width, height);
    test_format (HAL_PIXEL_FORMAT_TI_Y8, 1, width, height);
    test_format (HAL_PIXEL_FORMAT_TI_Y16, 1, width, height);

    ion_test();

    return 0;
}

