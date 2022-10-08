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
#ifndef NETLINKCONNECTION_H_
#define NETLINKCONNECTION_H_

#include <unistd.h>
#include <map>
#include <exception>
#include <inttypes.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "Connection.h"
#include "CMutex.h"
#include "CSemaphore.h"

/** PID(address) of MC daemon.  */
#define MC_DAEMON_PID 0xFFFFFFFF
/** Maximum Netlink payload size
 * TODO: figure out the best value for this */
#define MAX_PAYLOAD 1024

#define MC_DAEMON_NETLINK  17


class NetlinkConnection;

/**
 * Hash function for unique ID of a connection.
 *
 * @param pid Connection PID
 * @param seq Connection sequenceMagic
 *
 * @return Unique identifier of the connection
 */
uint64_t hashConnection(pid_t pid, uint32_t seq);

/** Netlink connection manager interface.
 * This inteface has to be implemented by the handling server
 * to ensure connection will be removed from accounting when destroied. */
class NetlinkConnectionManager
{
public:
    virtual ~NetlinkConnectionManager() {};
    /**
     * Retreive connection based on a unique hash.
     * Search the peer connections hashmap for a hash and return
     * the associated Connection object
     *
     * @param hash The hash to search
     * @return The NetlinkConnection object if found or NULL if not found
     */
    virtual NetlinkConnection *findConnection(
        uint64_t hash
    ) = 0;

    /**
     * Insert a connection in connection lisst
     * Insert a new connection in the peer connections list. If there
     * is already such a connection
     * it will be overriden!
     *
     * @param hash The hash to use
     * @param connection The connection object to insert
     */
    virtual void insertConnection(
        uint64_t hash,
        NetlinkConnection *connection
    ) = 0;

    /**
     * Remove a connection from the peer connections
     * Remove the connection associated with seq from the peer list.
     * This doesn't actually free the connection object!
     * If the hash is invalid nothing happens.
     *
     * @param hash The hash hash use
     */
    virtual void removeConnection(
        uint64_t hash
    ) = 0;
};

class NetlinkConnection: public Connection
{
public:
    pid_t selfPid; /**< Which PID to use to identify when writing data */
    pid_t peerPid; /**< Destination PID for sending data */
    uint32_t sequenceMagic; /**< Random? magic to match requests/answers */
    uint64_t hash; /**< Unique connection ID, see hashConnection */

    NetlinkConnection(
        void
    );

    /**
     * Connection main constructor
     *
     * @param manager Connection manager pointer.
     * @param socketDescriptor Socket descriptor to use for writing
     * @param pid Connection PID
     * @param seq Connection sequence magic number
     */
    NetlinkConnection(
        NetlinkConnectionManager *manager,
        int   socketDescriptor,
        uint32_t pid,
        uint32_t seq
    );

    virtual ~NetlinkConnection(
        void
    );

    /**
     * Connect to destination.
     *
     * @param Destination pointer.
     * @return true on success.
     */
    virtual bool connect(
        const char *dest
    );

    /**
     * Read bytes from the connection(compatiblity method).
     *
     * @param buffer    Pointer to destination buffer.
     * @param len       Number of bytes to read.
     * @param timeout   Timeout in milliseconds(ignored)
     * @return Number of bytes read.
     * @return -1 if select() failed (returned -1)
     * @return -2 if no data available, i.e. timeout
     */
    virtual size_t readData(
        void      *buffer,
        uint32_t  len,
        int32_t   timeout
    );

    /**
     * Read bytes from the connection.
     *
     * @param buffer    Pointer to destination buffer.
     * @param len       Number of bytes to read.
     * @return Number of bytes read.
     */
    virtual size_t readData(
        void       *buffer,
        uint32_t  len
    );

    /**
     * Write bytes to the connection.
     *
     * @param buffer    Pointer to source buffer.
     * @param len       Number of bytes to read.
     * @return Number of bytes written.
     */
    virtual size_t writeData(
        void      *buffer,
        uint32_t  len
    );

    /**
     * Set the internal data connection.
     * This method is called by the
     *
     * @param nlh    Netlink structure pointing to data.
     */
    void handleMessage(
        struct nlmsghdr *nlh
    );

private:
    CMutex dataMutex;
    CSemaphore dataLeft;
    struct nlmsghdr *dataMsg; /**< Last message received */
    uint32_t dataLen; /**< How much connection data is left */
    uint8_t *dataStart; /**< Start pointer of remaining data */
    NetlinkConnectionManager *manager; /**< Netlink connection manager(eg. NetlinkServer) */
};

typedef std::map<uint64_t, NetlinkConnection *>  connectionMap_t;

#endif /* NETLINKCONNECTION_H_ */

/** @} */
