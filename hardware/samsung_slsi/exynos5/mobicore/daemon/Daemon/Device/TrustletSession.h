/** @addtogroup MCD_MCDIMPL_DAEMON_DEV
 * @{
 * @file
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
#ifndef TRUSTLETSESSION_H_
#define TRUSTLETSESSION_H_

#include "NotificationQueue.h"
#include "CWsm.h"
#include "Connection.h"
#include <queue>
#include <map>


class TrustletSession
{
private:
    std::queue<notification_t> notifications;
    std::map<uint32_t, CWsm_ptr> buffers;

public:
    uint32_t sessionId;
    uint32_t sessionMagic; // Random data
    Connection *deviceConnection;
    Connection *notificationConnection;

    TrustletSession(Connection *deviceConnection, uint32_t sessionId);

    ~TrustletSession(void);

    void queueNotification(notification_t *notification);

    void processQueuedNotifications(void);

    bool addBulkBuff(CWsm_ptr pWsm);

    bool removeBulkBuff(uint32_t handle);

    CWsm_ptr popBulkBuff();

};

typedef std::list<TrustletSession *> trustletSessionList_t;
typedef trustletSessionList_t::iterator trustletSessionIterator_t;

#endif /* TRUSTLETSESSION_H_ */

/** @} */
