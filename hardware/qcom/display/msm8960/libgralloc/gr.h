/*
 * Copyright (C) 2008 The Android Open Source Project
 * Copyright (c) 2011-2012, The Linux Foundation. All rights reserved.
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

#ifndef GR_H_
#define GR_H_

#include <stdint.h>
#include <sys/user.h>
#include <limits.h>
#include <sys/cdefs.h>
#include <hardware/gralloc.h>
#include <pthread.h>
#include <errno.h>

#include <cutils/native_handle.h>
#include <utils/Singleton.h>

/*****************************************************************************/

struct private_module_t;
struct private_handle_t;

inline size_t roundUpToPageSize(size_t x) {
    return (x + (PAGE_SIZE-1)) & ~(PAGE_SIZE-1);
}

inline size_t ALIGN(size_t x, size_t align) {
    return (x + align-1) & ~(align-1);
}

#define FALSE 0
#define TRUE  1

int mapFrameBufferLocked(struct private_module_t* module);
int terminateBuffer(gralloc_module_t const* module, private_handle_t* hnd);
size_t getBufferSizeAndDimensions(int width, int height, int format,
                                  int& alignedw, int &alignedh);

int decideBufferHandlingMechanism(int format, const char *compositionUsed,
                                  int hasBlitEngine, int *needConversion,
                                  int *useBufferDirectly);

// Allocate buffer from width, height, format into a private_handle_t
// It is the responsibility of the caller to free the buffer
int alloc_buffer(private_handle_t **pHnd, int w, int h, int format, int usage);
void free_buffer(private_handle_t *hnd);

/*****************************************************************************/

class Locker {
    pthread_mutex_t mutex;
    public:
    class Autolock {
        Locker& locker;
        public:
        inline Autolock(Locker& locker) : locker(locker) {  locker.lock(); }
        inline ~Autolock() { locker.unlock(); }
    };
    inline Locker()        { pthread_mutex_init(&mutex, 0); }
    inline ~Locker()       { pthread_mutex_destroy(&mutex); }
    inline void lock()     { pthread_mutex_lock(&mutex); }
    inline void unlock()   { pthread_mutex_unlock(&mutex); }
};


class AdrenoMemInfo : public android::Singleton <AdrenoMemInfo>
{
    public:
    AdrenoMemInfo();

    ~AdrenoMemInfo();

    /*
     * Function to compute the adreno stride based on the width and format.
     *
     * @return stride.
     */
    int getStride(int width, int format);

    private:
        // Pointer to the padding library.
        void *libadreno_utils;

        // link to the surface padding library.
        int (*LINK_adreno_compute_padding) (int width, int bpp,
                                                int surface_tile_height,
                                                int screen_tile_height,
                                                int padding_threshold);
};
#endif /* GR_H_ */
