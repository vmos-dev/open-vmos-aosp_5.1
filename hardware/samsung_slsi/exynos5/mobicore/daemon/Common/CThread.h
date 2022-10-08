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
#ifndef CTHREAD_H_
#define CTHREAD_H_

#include <inttypes.h>
#include "CSemaphore.h"
#include "pthread.h"

using namespace std;


class CThread
{

public:

    CThread(void);

    virtual ~CThread(void);

    virtual void run(void) = 0;

    void start(void);

    void join(void);

    void sleep(void);

    void wakeup(void);

    void terminate(void);

    bool isExiting(void);

    void setExiting(void);

protected:

    bool shouldTerminate(void);

    void exit(int32_t exitcode);

private:

    CSemaphore *m_sem;
    pthread_t m_thread;
    bool m_terminate;
    bool m_isExiting;

};

extern "C" void *CThreadStartup(void *);

#endif /*CTHREAD_H_*/

/** @} */
