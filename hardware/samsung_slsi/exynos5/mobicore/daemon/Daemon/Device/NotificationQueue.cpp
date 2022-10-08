/** @addtogroup MCD_MCDIMPL_DAEMON_DEV
 * @{
 * @file
 *
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
#include "NotificationQueue.h"
#include <stddef.h>

#include "log.h"

//------------------------------------------------------------------------------
NotificationQueue::NotificationQueue(
    notificationQueue_t *i,
    notificationQueue_t *o,
    uint32_t size
) : in(i), out(o)
{
    in->hdr.queueSize = size;
    out->hdr.queueSize = size;
}


//------------------------------------------------------------------------------
void NotificationQueue::putNotification(
    notification_t *notification
)
{
    mutex.lock();
    if ((out->hdr.writeCnt - out->hdr.readCnt) < out->hdr.queueSize) {
        out->notification[out->hdr.writeCnt & (out->hdr.queueSize - 1)]
        = *notification;
        out->hdr.writeCnt++;
    }
    mutex.unlock();
}


//------------------------------------------------------------------------------
notification_t *NotificationQueue::getNotification(
    void
)
{
    notification_t *ret = NULL;
    mutex.lock();
    if ((in->hdr.writeCnt - in->hdr.readCnt) > 0) {
        ret = &(in->notification[in->hdr.readCnt & (in->hdr.queueSize - 1)]);
        in->hdr.readCnt++;
    }
    mutex.unlock();
    return ret;
}

/** @} */
