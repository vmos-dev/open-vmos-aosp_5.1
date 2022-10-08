/** @addtogroup MCD_MCDIMPL_DAEMON_SRV
 * @{
 * @file
 *
 * Thread implementation (pthread abstraction).
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
#include "CThread.h"

#include "log.h"


//------------------------------------------------------------------------------
CThread::CThread(void) :
    m_terminate(false), m_isExiting(false)
{
    m_sem = new CSemaphore();
}


//------------------------------------------------------------------------------
CThread::~CThread(
    void
)
{
    delete m_sem;
}


//------------------------------------------------------------------------------
void CThread::terminate(
    void
)
{
    m_terminate = true;
}


//------------------------------------------------------------------------------
bool CThread::isExiting(
    void
)
{
    return m_isExiting;
}


//------------------------------------------------------------------------------
void CThread::setExiting(
    void
)
{
    m_isExiting = true;
}


//------------------------------------------------------------------------------
void CThread::exit(
    int32_t exitcode
)
{
    setExiting();
    pthread_exit((void *)exitcode);
}


//------------------------------------------------------------------------------
bool CThread::shouldTerminate(
    void
)
{
    return m_terminate;
}


//------------------------------------------------------------------------------
void CThread::start(
    void
)
{
    int ret;
    ret = pthread_create(&m_thread, NULL, CThreadStartup, this);
    if (0 != ret)
        LOG_E("pthread_create failed with error code %d", ret);
}


//------------------------------------------------------------------------------
void CThread::join(
    void
)
{
    int ret;
    ret = pthread_join(m_thread, NULL);
    if (0 != ret)
        LOG_E("pthread_join failed with error code %d", ret);
}


//------------------------------------------------------------------------------
void CThread::sleep(
    void
)
{
    m_sem->wait();
}


//------------------------------------------------------------------------------
void CThread::wakeup(
    void
)
{
    m_sem->signal();
}


//------------------------------------------------------------------------------
void *CThreadStartup(
    void *_tgtObject
)
{
    CThread *tgtObject = (CThread *) _tgtObject;
    tgtObject->run();
    return NULL;
}

/** @} */
