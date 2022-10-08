/** @addtogroup MCD_MCDIMPL_DAEMON_SRV
 * @{
 * @file
 *
 * Connection data.
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
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <cstring>
#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>
#include <linux/netlink.h>

#include "NetlinkConnection.h"

#include "log.h"


uint64_t hashConnection(
    pid_t pid,
    uint32_t seq
)
{
    return (((uint64_t)seq) << 32) | (uint64_t)pid;
}


//------------------------------------------------------------------------------
NetlinkConnection::NetlinkConnection(
    void
) : Connection(),
    dataLeft(0),
    manager(NULL)
{
    detached = false;
    dataMsg = NULL;
    dataStart = NULL;
    dataLen = 0;


    selfPid = getpid();
    peerPid = 0;
    sequenceMagic = 0;
    hash = hashConnection(peerPid, sequenceMagic);
}


//------------------------------------------------------------------------------
NetlinkConnection::NetlinkConnection(
    NetlinkConnectionManager    *manager,
    int                         socketDescriptor,
    uint32_t                    pid,
    uint32_t                    seq
): Connection(),
    dataLeft(0),
    manager(manager)
{
    detached = false;
    dataMsg = NULL;
    dataStart = NULL;
    dataLen = 0;

    this->socketDescriptor = socketDescriptor;
    selfPid = getpid();
    peerPid = pid;
    sequenceMagic = seq;
    hash = hashConnection(pid, seq);
}


//------------------------------------------------------------------------------
NetlinkConnection::~NetlinkConnection(
    void
)
{
    LOG_I("%s: destroy connection for PID 0x%X", __FUNCTION__, peerPid);
    socketDescriptor = -1;
    free(dataMsg);

    if (manager) {
        manager->removeConnection(hash);
    }
}


//------------------------------------------------------------------------------
bool NetlinkConnection::connect(
    const char *dest
)
{
    struct sockaddr_nl addr;
    bool ret = false;

    assert(NULL != dest);

    LOG_I("%s: Connecting to SEQ 0x%X", __FUNCTION__, MC_DAEMON_PID);
    do {
        if ((socketDescriptor = socket(PF_NETLINK, SOCK_DGRAM, MC_DAEMON_NETLINK)) < 0) {
            LOG_E("%s: Can't open netlink socket - errno: %d(%s)",
                  __FUNCTION__, errno, strerror(errno));
            break;
        }
        memset(&addr, 0, sizeof(addr));
        addr.nl_family = AF_NETLINK;
        addr.nl_pid = selfPid;  /* self pid */
        addr.nl_groups = 0;  /* not in mcast groups */

        if (bind(socketDescriptor, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            LOG_E("%s: bind() failed - errno: %d(%s)", __FUNCTION__, errno, strerror(errno));
            close(socketDescriptor);

            // Set invalid socketDescriptor
            socketDescriptor = -1;
            break;
        }
        ret = true;


    } while (false);

    return ret;
}


//------------------------------------------------------------------------------
void NetlinkConnection::handleMessage(
    struct nlmsghdr *nlh
)
{
    dataMutex.lock();
    /* Takeover the buffer */
    dataMsg = nlh;
    dataLen = NLMSG_PAYLOAD(dataMsg, 0);
    dataStart = static_cast<uint8_t *>(NLMSG_DATA(dataMsg));
    dataMutex.unlock();
    dataLeft.signal();
}

//------------------------------------------------------------------------------
size_t NetlinkConnection::readData(
    void *buffer,
    uint32_t len
)
{
    return readData(buffer, len, -1);
}


//------------------------------------------------------------------------------
size_t NetlinkConnection::readData(
    void      *buffer,
    uint32_t  len,
    int32_t   timeout
)
{
    size_t ret = -1;
    assert(NULL != buffer);

    if (!dataLeft.wait(timeout)) {
        return -2;
    }
    dataMutex.lock();
    // No data left?? Could we get this far?
    if (dataLen <= 0) {
        dataMutex.unlock();
        return -2;
    }

    //LOG_I("%s: reading connection data %u, connection data left %u",
    //      __FUNCTION__, len, dataLen);

    assert(dataStart != NULL);

    // trying to read more than the left data
    if (len > dataLen) {
        ret = dataLen;
        memcpy(buffer, dataStart, dataLen);
        dataLen = 0;
    } else {
        ret = len;
        memcpy(buffer, dataStart, len);
        dataLen -= len;
        dataStart += len;
    }

    if (dataLen == 0) {
        dataStart = NULL;
        free(dataMsg);
        dataMsg = NULL;
    } else {
        // Still some data left
        dataLeft.signal();
    }
    dataMutex.unlock();

    //LOG_I("%s: read %u", __FUNCTION__, ret);
    return ret;
}

//------------------------------------------------------------------------------
size_t NetlinkConnection::writeData(
    void *buffer,
    uint32_t len
)
{
    size_t ret;
    struct sockaddr_nl dest_addr;
    struct nlmsghdr *nlh = NULL;
    struct iovec iov;
    struct msghdr msg;

    assert(NULL != buffer);
    assert(-1 != socketDescriptor);

    //LOG_I("%s: send data %u to PID %u", __FUNCTION__, len, sequenceMagic);

    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = peerPid;
    dest_addr.nl_groups = 0; /* unicast */

    nlh = (struct nlmsghdr *)malloc(
              NLMSG_SPACE(len));
    /* Fill the netlink message header */
    nlh->nlmsg_len = NLMSG_SPACE(len);
    nlh->nlmsg_pid = selfPid;
    nlh->nlmsg_flags = NLM_F_REQUEST;
    nlh->nlmsg_seq = sequenceMagic;

    /* Fill in the netlink message payload */
    memcpy(NLMSG_DATA(nlh), buffer, len);

    iov.iov_base = (void *)nlh;
    iov.iov_len = nlh->nlmsg_len;
    msg.msg_name = (void *)&dest_addr;
    msg.msg_namelen = sizeof(dest_addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = NULL;
    msg.msg_controllen = 0;

    ret = sendmsg(socketDescriptor, &msg, 0);
    if (ret != NLMSG_SPACE(len)) {
        LOG_E( "%s: could no send all data, ret=%d, errno: %d(%s)",
               __FUNCTION__, ret, errno, strerror(errno));
        ret = -1;
    } else {
        /* The whole message sent also includes the header, so make sure to
         * return only the number of payload data sent, not everything */
        ret = len;
    }

    free(nlh);

    return ret;
}

/** @} */
