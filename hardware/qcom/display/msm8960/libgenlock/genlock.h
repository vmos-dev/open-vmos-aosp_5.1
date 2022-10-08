/*
 * Copyright (c) 2011-2012, The Linux Foundation. All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of The Linux Foundation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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
 */

#ifndef INCLUDE_LIBGENLOCK
#define INCLUDE_LIBGENLOCK

#include <cutils/native_handle.h>

#ifdef __cplusplus
extern "C" {
#endif

    /* Genlock lock types */
    typedef enum genlock_lock_type{
        GENLOCK_READ_LOCK  = 1<<0,  // Read lock
        GENLOCK_WRITE_LOCK = 1<<1,  // Write lock
    }genlock_lock_type_t;

    /* Genlock return values */
    typedef enum genlock_status{
        GENLOCK_NO_ERROR = 0,
        GENLOCK_TIMEDOUT,
        GENLOCK_FAILURE,
    } genlock_status_t;

    /* Genlock defines */
#define GENLOCK_MAX_TIMEOUT 1000 // Max 1s timeout

    /*
     * Create a genlock lock. The genlock lock file descriptor and the lock
     * handle are stored in the buffer_handle.
     *
     * @param: handle of the buffer
     * @return error status.
     */
    genlock_status_t genlock_create_lock(native_handle_t *buffer_handle);


    /*
     * Release a genlock lock associated with the handle.
     *
     * @param: handle of the buffer
     * @return error status.
     */
    genlock_status_t genlock_release_lock(native_handle_t *buffer_handle);

    /*
     * Attach a lock to the buffer handle passed via an IPC.
     *
     * @param: handle of the buffer
     * @return error status.
     */
    genlock_status_t genlock_attach_lock(native_handle_t *buffer_handle);

    /*
     * Lock the buffer specified by the buffer handle. The lock held by the
     * buffer is specified by the lockType. This function will block if a write
     * lock is requested on the buffer which has previously been locked for a
     * read or write operation. A buffer can be locked by multiple clients for
     * read. An optional timeout value can be specified.
     * By default, there is no timeout.
     *
     * @param: handle of the buffer
     * @param: type of lock to be acquired by the buffer.
     * @param: timeout value in ms. GENLOCK_MAX_TIMEOUT is the maximum timeout
     *         value.
     * @return error status.
     */
    genlock_status_t genlock_lock_buffer(native_handle_t *buffer_handle,
                                         genlock_lock_type_t lockType,
                                         int timeout);

    /*
     * Unlocks a buffer that has previously been locked by the client.
     *
     * @param: handle of the buffer to be unlocked.
     * @return: error status.
     */
    genlock_status_t genlock_unlock_buffer(native_handle_t *buffer_handle);

    /*
     * Blocks the calling process until the lock held on the handle is unlocked.
     *
     * @param: handle of the buffer
     * @param: timeout value for the wait.
     * return: error status.
     */
    genlock_status_t genlock_wait(native_handle_t *buffer_handle, int timeout);

    /*
     * Convert a write lock that we own to a read lock
     *
     * @param: handle of the buffer
     * @param: timeout value for the wait.
     * return: error status.
     */
    genlock_status_t genlock_write_to_read(native_handle_t *buffer_handle,
                                           int timeout);

#ifdef __cplusplus
}
#endif
#endif
