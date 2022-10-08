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

#include <cutils/log.h>
#include <cutils/native_handle.h>
#include <gralloc_priv.h>
#ifdef USE_GENLOCK
#include <linux/genlock.h>
#endif
#include <fcntl.h>
#include <sys/ioctl.h>

#include "genlock.h"

#define GENLOCK_DEVICE "/dev/genlock"

namespace {
/* Internal function to map the userspace locks to the kernel lock types */
    int get_kernel_lock_type(genlock_lock_type lockType)
    {
        int kLockType = 0;
#ifdef USE_GENLOCK
        // If the user sets both a read and write lock, higher preference is
        // given to the write lock.
        if (lockType & GENLOCK_WRITE_LOCK) {
            kLockType = GENLOCK_WRLOCK;
        } else if (lockType & GENLOCK_READ_LOCK) {
            kLockType = GENLOCK_RDLOCK;
        } else {
            ALOGE("%s: invalid lockType (lockType = %d)",
                  __FUNCTION__, lockType);
            return -1;
        }
#endif
        return kLockType;
    }

    /* Internal function to perform the actual lock/unlock operations */
    genlock_status_t perform_lock_unlock_operation(native_handle_t *buffer_handle,
                                                   int lockType, int timeout,
                                                   int flags)
    {
#ifdef USE_GENLOCK
        if (private_handle_t::validate(buffer_handle)) {
            ALOGE("%s: handle is invalid", __FUNCTION__);
            return GENLOCK_FAILURE;
        }

        private_handle_t *hnd = reinterpret_cast<private_handle_t*>
                                (buffer_handle);
        if ((hnd->flags & private_handle_t::PRIV_FLAGS_UNSYNCHRONIZED) == 0) {
            if (hnd->genlockPrivFd < 0) {
                ALOGE("%s: the lock has not been created,"
                      "or has not been attached", __FUNCTION__);
                return GENLOCK_FAILURE;
            }

            genlock_lock lock;
            lock.op = lockType;
            lock.flags = flags;
            lock.timeout = timeout;
            lock.fd = hnd->genlockHandle;

#ifdef GENLOCK_IOC_DREADLOCK
            if (ioctl(hnd->genlockPrivFd, GENLOCK_IOC_DREADLOCK, &lock)) {
                ALOGE("%s: GENLOCK_IOC_DREADLOCK failed (lockType0x%x,"
                       "err=%s fd=%d)", __FUNCTION__,
                      lockType, strerror(errno), hnd->fd);
                if (ETIMEDOUT == errno)
                    return GENLOCK_TIMEDOUT;

                return GENLOCK_FAILURE;
            }
#else
            // depreciated
            if (ioctl(hnd->genlockPrivFd, GENLOCK_IOC_LOCK, &lock)) {
                ALOGE("%s: GENLOCK_IOC_LOCK failed (lockType0x%x, err=%s fd=%d)"
                      ,__FUNCTION__, lockType, strerror(errno), hnd->fd);
                if (ETIMEDOUT == errno)
                    return GENLOCK_TIMEDOUT;

                return GENLOCK_FAILURE;
            }
#endif
        }
#endif
        return GENLOCK_NO_ERROR;
    }

    /* Internal function to close the fd and release the handle */
    void close_genlock_fd_and_handle(int& fd, int& handle)
    {
        if (fd >=0 ) {
            close(fd);
            fd = -1;
        }

        if (handle >= 0) {
            close(handle);
            handle = -1;
        }
    }
}
/*
 * Create a genlock lock. The genlock lock file descriptor and the lock
 * handle are stored in the buffer_handle.
 *
 * @param: handle of the buffer
 * @return error status.
 */
genlock_status_t genlock_create_lock(native_handle_t *buffer_handle)
{
    genlock_status_t ret = GENLOCK_NO_ERROR;
#ifdef USE_GENLOCK
    if (private_handle_t::validate(buffer_handle)) {
        ALOGE("%s: handle is invalid", __FUNCTION__);
        return GENLOCK_FAILURE;
    }

    private_handle_t *hnd = reinterpret_cast<private_handle_t*>(buffer_handle);
    if ((hnd->flags & private_handle_t::PRIV_FLAGS_UNSYNCHRONIZED) == 0) {
        // Open the genlock device
        int fd = open(GENLOCK_DEVICE, O_RDWR);
        if (fd < 0) {
            ALOGE("%s: open genlock device failed (err=%s)", __FUNCTION__,
                  strerror(errno));
            return GENLOCK_FAILURE;
        }

        // Create a new lock
        genlock_lock lock;
        if (ioctl(fd, GENLOCK_IOC_NEW, NULL)) {
            ALOGE("%s: GENLOCK_IOC_NEW failed (error=%s)", __FUNCTION__,
                  strerror(errno));
            close_genlock_fd_and_handle(fd, lock.fd);
            ret = GENLOCK_FAILURE;
        }

        // Export the lock for other processes to be able to use it.
        if (GENLOCK_FAILURE != ret) {
            if (ioctl(fd, GENLOCK_IOC_EXPORT, &lock)) {
                ALOGE("%s: GENLOCK_IOC_EXPORT failed (error=%s)", __FUNCTION__,
                      strerror(errno));
                close_genlock_fd_and_handle(fd, lock.fd);
                ret = GENLOCK_FAILURE;
            }
        }

        // Store the lock params in the handle.
        hnd->genlockPrivFd = fd;
        hnd->genlockHandle = lock.fd;
    } else {
        hnd->genlockHandle = 0;
    }
#endif
    return ret;
}


/*
 * Release a genlock lock associated with the handle.
 *
 * @param: handle of the buffer
 * @return error status.
 */
genlock_status_t genlock_release_lock(native_handle_t *buffer_handle)
{
    genlock_status_t ret = GENLOCK_NO_ERROR;
#ifdef USE_GENLOCK
    if (private_handle_t::validate(buffer_handle)) {
        ALOGE("%s: handle is invalid", __FUNCTION__);
        return GENLOCK_FAILURE;
    }

    private_handle_t *hnd = reinterpret_cast<private_handle_t*>(buffer_handle);
    if ((hnd->flags & private_handle_t::PRIV_FLAGS_UNSYNCHRONIZED) == 0) {
        if (hnd->genlockPrivFd < 0) {
            ALOGE("%s: the lock is invalid", __FUNCTION__);
            return GENLOCK_FAILURE;
        }

        // Close the fd and reset the parameters.
        close_genlock_fd_and_handle(hnd->genlockPrivFd, hnd->genlockHandle);
    }
#endif
    return ret;
}


/*
 * Attach a lock to the buffer handle passed via an IPC.
 *
 * @param: handle of the buffer
 * @return error status.
 */
genlock_status_t genlock_attach_lock(native_handle_t *buffer_handle)
{
    genlock_status_t ret = GENLOCK_NO_ERROR;
#ifdef USE_GENLOCK
    if (private_handle_t::validate(buffer_handle)) {
        ALOGE("%s: handle is invalid", __FUNCTION__);
        return GENLOCK_FAILURE;
    }

    private_handle_t *hnd = reinterpret_cast<private_handle_t*>(buffer_handle);
    if ((hnd->flags & private_handle_t::PRIV_FLAGS_UNSYNCHRONIZED) == 0) {
        // Open the genlock device
        int fd = open(GENLOCK_DEVICE, O_RDWR);
        if (fd < 0) {
            ALOGE("%s: open genlock device failed (err=%s)", __FUNCTION__,
                  strerror(errno));
            return GENLOCK_FAILURE;
        }

        // Attach the local handle to an existing lock
        genlock_lock lock;
        lock.fd = hnd->genlockHandle;
        if (ioctl(fd, GENLOCK_IOC_ATTACH, &lock)) {
            ALOGE("%s: GENLOCK_IOC_ATTACH failed (err=%s)", __FUNCTION__,
                  strerror(errno));
            close_genlock_fd_and_handle(fd, lock.fd);
            ret = GENLOCK_FAILURE;
        }

        // Store the relavant information in the handle
        hnd->genlockPrivFd = fd;
    }
#endif
    return ret;
}

/*
 * Lock the buffer specified by the buffer handle. The lock held by the buffer
 * is specified by the lockType. This function will block if a write lock is
 * requested on the buffer which has previously been locked for a read or write
 * operation. A buffer can be locked by multiple clients for read. An optional
 * timeout value can be specified. By default, there is no timeout.
 *
 * @param: handle of the buffer
 * @param: type of lock to be acquired by the buffer.
 * @param: timeout value in ms. GENLOCK_MAX_TIMEOUT is the maximum timeout value.
 * @return error status.
 */
genlock_status_t genlock_lock_buffer(native_handle_t *buffer_handle,
                                     genlock_lock_type_t lockType,
                                     int timeout)
{
    genlock_status_t ret = GENLOCK_NO_ERROR;
#ifdef USE_GENLOCK
    // Translate the locktype
    int kLockType = get_kernel_lock_type(lockType);
    if (-1 == kLockType) {
        ALOGE("%s: invalid lockType", __FUNCTION__);
        return GENLOCK_FAILURE;
    }

    if (0 == timeout) {
        ALOGW("%s: trying to lock a buffer with timeout = 0", __FUNCTION__);
    }
    // Call the private function to perform the lock operation specified.
    ret = perform_lock_unlock_operation(buffer_handle, kLockType, timeout, 0);
#endif
    return ret;
}


/*
 * Unlocks a buffer that has previously been locked by the client.
 *
 * @param: handle of the buffer to be unlocked.
 * @return: error status.
 */
genlock_status_t genlock_unlock_buffer(native_handle_t *buffer_handle)
{
    genlock_status_t ret = GENLOCK_NO_ERROR;
#ifdef USE_GENLOCK
    // Do the unlock operation by setting the unlock flag. Timeout is always
    // 0 in this case.
    ret = perform_lock_unlock_operation(buffer_handle, GENLOCK_UNLOCK, 0, 0);
#endif
    return ret;
}

/*
 * Blocks the calling process until the lock held on the handle is unlocked.
 *
 * @param: handle of the buffer
 * @param: timeout value for the wait.
 * return: error status.
 */
genlock_status_t genlock_wait(native_handle_t *buffer_handle, int timeout) {
#ifdef USE_GENLOCK
    if (private_handle_t::validate(buffer_handle)) {
        ALOGE("%s: handle is invalid", __FUNCTION__);
        return GENLOCK_FAILURE;
    }

    private_handle_t *hnd = reinterpret_cast<private_handle_t*>(buffer_handle);
    if ((hnd->flags & private_handle_t::PRIV_FLAGS_UNSYNCHRONIZED) == 0) {
        if (hnd->genlockPrivFd < 0) {
            ALOGE("%s: the lock is invalid", __FUNCTION__);
            return GENLOCK_FAILURE;
        }

        if (0 == timeout)
            ALOGW("%s: timeout = 0", __FUNCTION__);

        genlock_lock lock;
        lock.fd = hnd->genlockHandle;
        lock.timeout = timeout;
        if (ioctl(hnd->genlockPrivFd, GENLOCK_IOC_WAIT, &lock)) {
            ALOGE("%s: GENLOCK_IOC_WAIT failed (err=%s)",  __FUNCTION__,
                  strerror(errno));
            return GENLOCK_FAILURE;
        }
    }
#endif
    return GENLOCK_NO_ERROR;
}

/*
 * Convert a write lock that we own to a read lock
 *
 * @param: handle of the buffer
 * @param: timeout value for the wait.
 * return: error status.
 */
genlock_status_t genlock_write_to_read(native_handle_t *buffer_handle,
                                       int timeout) {
    genlock_status_t ret = GENLOCK_NO_ERROR;
#ifdef USE_GENLOCK
    if (0 == timeout) {
        ALOGW("%s: trying to lock a buffer with timeout = 0", __FUNCTION__);
    }
    // Call the private function to perform the lock operation specified.
#ifdef GENLOCK_IOC_DREADLOCK
    ret = perform_lock_unlock_operation(buffer_handle, GENLOCK_RDLOCK, timeout,
                                        GENLOCK_WRITE_TO_READ);
#else
    // depreciated
    ret = perform_lock_unlock_operation(buffer_handle, GENLOCK_RDLOCK,
                                        timeout, 0);
#endif
#endif
    return ret;
}
