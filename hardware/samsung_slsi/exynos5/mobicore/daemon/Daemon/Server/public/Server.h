/** @addtogroup MCD_MCDIMPL_DAEMON_SRV
 * @{
 * @file
 *
 * Connection server.
 *
 * Handles incoming socket connections from clients using the MobiCore driver.
 *
 * Iterative socket server using UNIX domain stream protocol.
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
#ifndef SERVER_H_
#define SERVER_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string>
#include <cstdio>
#include <vector>
#include "CThread.h"
#include "ConnectionHandler.h"

/** Number of incoming connections that can be queued.
 * Additional clients will generate the error ECONNREFUSED. */
#define LISTEN_QUEUE_LEN    (16)


class Server: public CThread
{

public:
    /**
     * Server contructor.
     *
     * @param connectionHanler Connection handler to pass incoming connections to.
     * @param localAdrerss Pointer to a zero terminated string containing the file to listen to.
     */
    Server(
        ConnectionHandler *connectionHandler,
        const char *localAddr
    );

    /**
     * Server destructor.
     * All available connections will be terminated. Resources will be freed.
     */
    virtual ~Server(
        void
    );

    /**
     * Start server and listen for incoming connections.
     * Implements the central socket server loop. Incoming connections will be stored.
     */
    virtual void run(
    );

    /**
     * Remove a connection object from the list of available connections.
     * Detaching is required for notification connections wich are never used to transfer command
     * data from TLCs to the driver. If the function succeeds, the connection object will no longer
     * be handled by the server.
     *
     * @param connection The connection object to remove.
     */
    virtual void detachConnection(
        Connection *connection
    );

protected:
    int serverSock;
    string socketAddr;
    ConnectionHandler   *connectionHandler; /**< Connection handler registered to the server */

private:
    connectionList_t    peerConnections; /**< Connections to devices */

};

#endif /* SERVER_H_ */

/** @} */
