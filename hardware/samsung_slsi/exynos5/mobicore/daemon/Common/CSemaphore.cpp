/** @addtogroup MCD_MCDIMPL_DAEMON_SRV
 * @{
 * @file
 *
 * Semaphore implementation (pthread wrapper).
 *
 * <!-- Copyright Giesecke & Devrient GmbH 2009 - 2012 -->
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <time.h>
#include <limits.h>
#include "CSemaphore.h"
#include <stdio.h>

//------------------------------------------------------------------------------
CSemaphore::CSemaphore(int size) : m_waiters_count(0), m_count(size)
{
    pthread_mutex_init(&m_mutex, NULL);
    pthread_cond_init(&m_cond, NULL);
}


//------------------------------------------------------------------------------
CSemaphore::~CSemaphore()
{
    pthread_mutex_destroy(&m_mutex);
    pthread_cond_destroy(&m_cond);
}


//------------------------------------------------------------------------------
void CSemaphore::wait()
{
    pthread_mutex_lock(&m_mutex);
    m_waiters_count ++;
    while ( m_count == 0 )
        pthread_cond_wait(&m_cond, &m_mutex);
    m_waiters_count --;
    m_count --;
    pthread_mutex_unlock(&m_mutex);
}

//------------------------------------------------------------------------------
bool CSemaphore::wait(int sec)
{
    int rc = 0;
    struct timespec tm;
    if (sec < 0)
        sec = LONG_MAX;
    clock_gettime(CLOCK_REALTIME, &tm);
    tm.tv_sec += sec;

    pthread_mutex_lock(&m_mutex);
    m_waiters_count ++;
    if ( m_count == 0 ) {
        rc = pthread_cond_timedwait(&m_cond, &m_mutex, &tm);
    }
    m_waiters_count --;
    // Decrement only if waiting actually succeeded, otherwise we
    // just timed out
    if (!rc)
        m_count --;
    pthread_mutex_unlock(&m_mutex);
    return (rc == 0);
}


//------------------------------------------------------------------------------
bool CSemaphore::wouldWait()
{
    bool ret = false;
    pthread_mutex_lock(&m_mutex);
    if ( m_count == 0 )
        ret = true;
    pthread_mutex_unlock(&m_mutex);
    return ret;
}


//------------------------------------------------------------------------------
void CSemaphore::signal()
{
    pthread_mutex_lock(&m_mutex);
    if ( m_waiters_count > 0 )
        pthread_cond_signal(&m_cond);
    m_count ++;
    pthread_mutex_unlock(&m_mutex);
}

/** @} */
