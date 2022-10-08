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
#include "public/Server.h"
#include <unistd.h>
#include <string.h>
#include <errno.h>

//#define LOG_VERBOSE
#include "log.h"

//------------------------------------------------------------------------------
Server::Server(
    ConnectionHandler *connectionHandler,
    const char *localAddr
) : socketAddr(localAddr)
{
    this->connectionHandler = connectionHandler;
}


//------------------------------------------------------------------------------
void Server::run(
    void
)
{
    do {
        LOG_I("Server: start listening on socket %s", socketAddr.c_str());

        // Open a socket (a UNIX domain stream socket)
        serverSock = socket(AF_UNIX, SOCK_STREAM, 0);
        if (serverSock < 0) {
            LOG_ERRNO("Can't open stream socket, because socket");
            break;
        }

        // Fill in address structure and bind to socket
        struct sockaddr_un  serverAddr;
        serverAddr.sun_family = AF_UNIX;
        strncpy(serverAddr.sun_path, socketAddr.c_str(), sizeof(serverAddr.sun_path) - 1);

        uint32_t len = strlen(serverAddr.sun_path) + sizeof(serverAddr.sun_family);
        // Make the socket in the Abstract Domain(no path but everyone can connect)
        serverAddr.sun_path[0] = 0;
        if (bind(serverSock, (struct sockaddr *) &serverAddr, len) < 0) {
            LOG_ERRNO("Binding to server socket failed, because bind");
        }

        // Start listening on the socket
        if (listen(serverSock, LISTEN_QUEUE_LEN) < 0) {
            LOG_ERRNO("listen");
            break;
        }

        LOG_I("\n********* successfully initialized Daemon *********\n");

        for (;;) {
            fd_set fdReadSockets;

            // Clear FD for select()
            FD_ZERO(&fdReadSockets);

            // Select server socket descriptor
            FD_SET(serverSock, &fdReadSockets);
            int maxSocketDescriptor = serverSock;

            // Select socket descriptor of all connections
            for (connectionIterator_t iterator = peerConnections.begin();
                    iterator != peerConnections.end();
                    ++iterator) {
                Connection *connection = (*iterator);
                int peerSocket = connection->socketDescriptor;
                FD_SET(peerSocket, &fdReadSockets);
                if (peerSocket > maxSocketDescriptor) {
                    maxSocketDescriptor = peerSocket;
                }
            }

            // Wait for activities, select() returns the number of sockets
            // which require processing
            LOG_V(" Server: waiting on sockets");
            int numSockets = select(
                                 maxSocketDescriptor + 1,
                                 &fdReadSockets,
                                 NULL, NULL, NULL);

            // Check if select failed
            if (numSockets < 0) {
                LOG_ERRNO("select");
                break;
            }

            // actually, this should not happen.
            if (0 == numSockets) {
                LOG_W(" Server: select() returned 0, spurious event?.");
                continue;
            }

            LOG_V(" Server: events on %d socket(s).", numSockets);

            // Check if a new client connected to the server socket
            if (FD_ISSET(serverSock, &fdReadSockets)) {
                do {
                    LOG_V(" Server: new connection attempt.");
                    numSockets--;

                    struct sockaddr_un clientAddr;
                    socklen_t clientSockLen = sizeof(clientAddr);
                    int clientSock = accept(
                                         serverSock,
                                         (struct sockaddr *) &clientAddr,
                                         &clientSockLen);

                    if (clientSock <= 0) {
                        LOG_ERRNO("accept");
                        break;
                    }

                    Connection *connection = new Connection(clientSock, &clientAddr);
                    peerConnections.push_back(connection);
                    LOG_I(" Server: new socket connection established and start listening.");
                } while (false);

                // we can ignore any errors from accepting a new connection.
                // If this fail, the client has to deal with it, we are done
                // and nothing has changed.
            }

            // Handle traffic on existing client connections
            connectionIterator_t iterator = peerConnections.begin();
            while ( (iterator != peerConnections.end())
                    && (numSockets > 0) ) {
                Connection *connection = (*iterator);
                int peerSocket = connection->socketDescriptor;

                if (!FD_ISSET(peerSocket, &fdReadSockets)) {
                    ++iterator;
                    continue;
                }

                numSockets--;

                // the connection will be terminated if command processing
                // fails
                if (!connectionHandler->handleConnection(connection)) {
                    LOG_I(" Server: dropping connection.");

                    //Inform the driver
                    connectionHandler->dropConnection(connection);

                    // Remove connection from list
                    delete connection;
                    iterator = peerConnections.erase(iterator);
                    continue;
                }

                ++iterator;
            }
        }

    } while (false);

    LOG_ERRNO("Exiting Server, because");
}


//------------------------------------------------------------------------------
void Server::detachConnection(
    Connection *connection
)
{
    LOG_V(" Stopping to listen on notification socket.");

    for (connectionIterator_t iterator = peerConnections.begin();
            iterator != peerConnections.end();
            ++iterator) {
        Connection *tmpConnection = (*iterator);
        if (tmpConnection == connection) {
            peerConnections.erase(iterator);
            LOG_I(" Stopped listening on notification socket.");
            break;
        }
    }
}


//------------------------------------------------------------------------------
Server::~Server(
    void
)
{
    // Shut down the server socket
    close(serverSock);

    // Destroy all client connections
    connectionIterator_t iterator = peerConnections.begin();
    while (iterator != peerConnections.end()) {
        Connection *tmpConnection = (*iterator);
        delete tmpConnection;
        iterator = peerConnections.erase(iterator);
    }
}

/** @} */
