/** @addtogroup MCD_MCDIMPL_DAEMON_DEV
 * @{
 * @file
 *
 * MobiCore Notification Queue handling.
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
#ifndef NOTIFICATIONQUEUE_H_
#define NOTIFICATIONQUEUE_H_

#include <inttypes.h> //C99 data
#include "Mci/mcinq.h"
#include "CMutex.h"


class NotificationQueue
{

public:

    /** NQ Constructor, initializes the NQ component.
     *
     * makes the given queue object usable with the queue<Command> type of functions
     *
     * @param in queue to initialize
     * @param out beginning of queue header
     * @param queueSize Size of the queue
     */
    NotificationQueue(
        notificationQueue_t *in,
        notificationQueue_t *out,
        uint32_t size
    );

    /** Places an element to the outgoing queue.
     *
     * @param notification Data to be placed in queue.
     */
    void putNotification(
        notification_t *notification
    );

    /** Retrieves the first element from the queue.
     *
     * @return first notification Queue element.
     * @return NULL if the queue is empty.
     */
    notification_t *getNotification(
        void
    );

private:

    notificationQueue_t *in;
    notificationQueue_t *out;
    CMutex mutex;

};

#endif /* NOTIFICATIONQUEUE_H_ */

/** @} */
