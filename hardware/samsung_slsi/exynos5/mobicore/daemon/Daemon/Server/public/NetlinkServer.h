/** @addtogroup MCD_MCDIMPL_DAEMON_SRV
 * @{
 * @file
 *
 * Connection server.
 *
 * Handles incoming socket connections from clients using the MobiCore driver.
 *
 * Iterative socket server using Netlink dgram protocol.
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
#ifndef NETLINKSERVER_H_
#define NETLINKSERVER_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <linux/netlink.h>
#include <cstdio>
#include <map>

#include "NetlinkConnection.h"
#include "ConnectionHandler.h"
#include "Server.h"

class NetlinkServer: public Server, public NetlinkConnectionManager
{
public:
    /**
     * Server contructor.
     *
     * @param connectionHanler Connection handler to pass incoming connections to.
     */
    NetlinkServer(
        ConnectionHandler *connectionHandler
    );

    /**
     * Server destructor.
     * All available connections will be terminated. Resources will be freed.
     */
    virtual ~NetlinkServer(
        void
    );

    /**
     * Start server and listen for incoming connections.
     * Implements the central socket server loop. Incoming connections will be stored.
     */
    virtual void run(
        void
    );

    /**
     * Remove a connection object from the list of available connections.
     * Detaching is required for notification connections wich are never used to transfer command
     * data from TLCs to the driver. If the function succeeds, freeing the connection will no longer
     * be the server's responsability.
     *
     * @param connection The connection object to remove.
     */
    virtual void detachConnection(
        Connection *connection
    );

private:
    /**
     * Handle incomming Netlink message.
     * It routes the incomming packet to the apropriate connection based on the packet's
     * session magic.
     *
     * @param nlh The netlink message's header + payload
     */
    void handleMessage(
        struct nlmsghdr *nlh
    );

    /**
     * Retreive connection based on hash.
     * Search the peer connections hashmap for a hash and return
     * the associated Connection object
     *
     * @param seq The seq to search
     * @return The NetlinkConnection object if found or NULL if not found
     */
    NetlinkConnection *findConnection(
        uint64_t hash
    );

    /**
     * Insert a connection in the peer connection hashmap
     * Insert a new connection in the peer connections hashmap. If there is
     * already such a connection it will be overriden!
     *
     * @param seq The seq to use
     * @param connection The connection object to insert
     */
    void insertConnection(
        uint64_t hash,
        NetlinkConnection *connection
    );

    /**
     * Remove a connection from the peer connections
     * Remove the connection associated with seq from the peer list.
     * This doesn't actually free the connection object!
     * If the seq is invalid nothing happens.
     *
     * @param seq The seq to use
     */
    void removeConnection(
        uint64_t hash
    );


    /**
     * Check for sessions started by applications that died(exited)
     * Remove the connections to applications that are not active anymore
     * If the application has died then all the sessions associated with it
     * should be closed!
     *
     */
    void cleanupConnections(
        void
    );

    connectionMap_t peerConnections; /**< Hashmap with connections to clients */
};

#endif /* SERVER_H_ */

/** @} */
