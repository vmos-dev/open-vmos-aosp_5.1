/** @addtogroup MCD_MCDIMPL_DAEMON_SRV
 * @{
 * @file
 *
 * Connection server.
 *
 * Handles incoming socket connections from clients using the MobiCore driver.
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
#include "public/NetlinkServer.h"
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <linux/netlink.h>

#include <stdlib.h>
#include "NetlinkConnection.h"
#include <signal.h>

#define LOG_TAG "McDaemon"
#include "log.h"

//------------------------------------------------------------------------------
NetlinkServer::NetlinkServer(
    ConnectionHandler *connectionHandler
): Server(connectionHandler, "dummy")
{
}


//------------------------------------------------------------------------------
void NetlinkServer::run(
)
{
    do {
        LOG_I("NetlinkServer: Starting to listen on netlink bus");

        // Open a socket
        serverSock = socket(PF_NETLINK, SOCK_DGRAM,  MC_DAEMON_NETLINK);
        if (serverSock < 0) {
            LOG_ERRNO("Opening socket");
            break;
        }

        // Fill in address structure and bind to socket
        struct sockaddr_nl src_addr;
        struct nlmsghdr *nlh = NULL;
        struct iovec iov;
        struct msghdr msg;
        uint32_t len;

        memset(&src_addr, 0, sizeof(src_addr));
        src_addr.nl_family = AF_NETLINK;
        src_addr.nl_pid = MC_DAEMON_PID;  /* daemon pid */
        src_addr.nl_groups = 0;  /* not in mcast groups */
        if (bind(serverSock, (struct sockaddr *)&src_addr, sizeof(src_addr)) < 0) {
            LOG_ERRNO("Binding to server socket failed, because bind");
            close(serverSock);
            break;
        }

        // Start reading the socket
        LOG_I("\n********* successfully initialized *********\n");

        for (;;) {
            // This buffer will be taken over by the connection it was routed to
            nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
            memset(&msg, 0, sizeof(msg));
            iov.iov_base = (void *)nlh;
            iov.iov_len = NLMSG_SPACE(MAX_PAYLOAD);
            msg.msg_iov = &iov;
            msg.msg_iovlen = 1;
            msg.msg_name = &src_addr;
            msg.msg_namelen = sizeof(src_addr);

            memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));

            // Read the incomming message and route it to the connection based
            // on the incomming PID
            if ((len = recvmsg(serverSock, &msg, 0)) < 0) {
                LOG_ERRNO("recvmsg");
                break;
            }

            if (NLMSG_OK(nlh, len)) {
                handleMessage(nlh);
            } else {
                break;
            }
        }
    } while (false);

    LOG_ERRNO("Exiting NetlinkServer! Because it");
}

//------------------------------------------------------------------------------
void NetlinkServer::handleMessage(
    struct nlmsghdr *nlh
)
{
    uint32_t seq = nlh->nlmsg_seq;
    uint32_t pid = nlh->nlmsg_pid;
    //LOG_I("%s: Handling NQ message for pid %u seq %u...", __FUNCTION__, pid, seq);
    uint64_t hash = hashConnection(pid, seq);
    /* First cleanup the connection list */
    cleanupConnections();

    NetlinkConnection *connection = findConnection(hash);
    // This is a message from a new client
    if (connection == NULL) {
        //LOG_I("%s: Cound't find the connection, creating a new one", __FUNCTION__);
        connection = new NetlinkConnection(this, serverSock, pid, seq);
        // Add the new connection
        insertConnection(hash, connection);
    }

    connection->handleMessage(nlh);

    // Only handle connections which have not been detached
    if (connection->detached == false) {
        if (!connectionHandler->handleConnection(connection)) {
            LOG_I("%s: No command processed.", __FUNCTION__);
            connection->socketDescriptor = -1;
            //Inform the driver
            connectionHandler->dropConnection(connection);

            // Remove connection from list
            removeConnection(hash);
            connection->socketDescriptor = -1;
            delete connection;
        }
        // If connection data is set to NULL then device close has been called
        // so we must remove all connections associated with this hash
        else if (connection->connectionData == NULL &&
                 connection->detached == false) {
            delete connection;
        }
    }
}


//------------------------------------------------------------------------------
void NetlinkServer::detachConnection(
    Connection *connection
)
{
    connection->detached = true;
}


//------------------------------------------------------------------------------
NetlinkServer::~NetlinkServer(
    void
)
{
    connectionMap_t::iterator i;
    // Shut down the server socket
    close(serverSock);

    // Destroy all client connections
    for (i = peerConnections.begin(); i != peerConnections.end(); i++) {
        if (i->second->detached == false) {
            delete i->second;
        }
    }
    peerConnections.clear();
}


//------------------------------------------------------------------------------
NetlinkConnection *NetlinkServer::findConnection(
    uint64_t hash
)
{
    connectionMap_t::iterator i = peerConnections.find(hash);
    if (i != peerConnections.end()) {
        return i->second;
    }

    return NULL;
}


//------------------------------------------------------------------------------
void NetlinkServer::insertConnection(
    uint64_t hash,
    NetlinkConnection *connection
)
{
    peerConnections[hash] = connection;
}

/* This is called from multiple threads! */
//------------------------------------------------------------------------------
void NetlinkServer::removeConnection(
    uint64_t hash
)
{
    connectionMap_t::iterator i = peerConnections.find(hash);
    if (i != peerConnections.end()) {
        peerConnections.erase(i);
    }
}

//------------------------------------------------------------------------------
void NetlinkServer::cleanupConnections(
    void
)
{
    connectionMap_t::reverse_iterator i;
    pid_t pid;
    NetlinkConnection *connection = NULL;
    // Destroy all client connections
    for (i = peerConnections.rbegin(); i != peerConnections.rend(); ++i) {
        connection = i->second;
        // Only 16 bits are for the actual PID, the rest is session magic
        pid = connection->peerPid & 0xFFFF;
        //LOG_I("%s: checking PID %u", __FUNCTION__, pid);
        // Check if the peer pid is still alive
        if (pid == 0) {
            continue;
        }
        if (kill(pid, 0)) {
            bool detached = connection->detached;
            LOG_I("%s: PID %u has died, cleaning up session 0x%X",
                  __FUNCTION__, pid, connection->peerPid);

            connection->socketDescriptor = -1;
            //Inform the driver
            connectionHandler->dropConnection(connection);

            // We aren't handling this connection anymore no matter what
            removeConnection(connection->hash);

            // Remove connection from list only if detached, the detached
            // connections are managed by the device
            if (detached == false) {
                delete connection;
            }
            if (peerConnections.size() == 0) {
                break;
            }
            i = peerConnections.rbegin();
        }
    }
}

/** @} */
