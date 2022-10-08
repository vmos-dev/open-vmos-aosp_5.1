/**
 * Copyright(c) 2011 Trusted Logic.   All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name Trusted Logic nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __LIB_MUTEX_H__
#define __LIB_MUTEX_H__

/**
 * This libray defines a type of simple non-recursive mutex that can
 * be statically initialized. The interface is very similar to pthread
 * mutexes.
 **/

#ifdef WIN32

#include <windows.h>
#include "s_type.h"

/* Windows API: use a critical section with a state describing
   whether it has been initialized or not or is being initialized.
   We use interlocked operations on the state to synchronize between
   multiple threads competing to initialize the critical section */
typedef struct
{
   volatile uint32_t nInitState;
   CRITICAL_SECTION sCriticalSection;
}
LIB_MUTEX;

#define LIB_MUTEX_WIN32_STATE_UNINITIALIZED 0
#define LIB_MUTEX_WIN32_STATE_INITIALIZING  1
#define LIB_MUTEX_WIN32_STATE_INITIALIZED   2

#define LIB_MUTEX_INITIALIZER { 0 }

#endif  /* WIN32 */

#ifdef LINUX

#include <pthread.h>
#include "s_type.h"

/* Linux: directly use a pthread mutex. */
typedef pthread_mutex_t LIB_MUTEX;
#define LIB_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER

#endif /* LINUX */

/**
 * Initialize a mutex. Note that thie function cannot fail. If there
 * is not enough resource to initialize the mutex, the implementation
 * resort to active-wait
 **/
void libMutexInit(LIB_MUTEX* pMutex);

/**
 * Lock the mutex
 */
void libMutexLock(LIB_MUTEX* pMutex);

/**
 * Unlock the mutex
 */
void libMutexUnlock(LIB_MUTEX* pMutex);

/**
 * Destroy the mutex
 */
void libMutexDestroy(LIB_MUTEX* pMutex);

#endif /* __LIB_MUTEX_H__ */
