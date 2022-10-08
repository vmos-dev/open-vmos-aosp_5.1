/** @addtogroup MCD_MCDIMPL_DAEMON_CONHDLR
 * @{
 * @file
 *
 * MobiCore driver class.
 * The MobiCore driver class implements the ConnectionHandler interface.
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
#ifndef MOBICOREDRIVER_H_
#define MOBICOREDRIVER_H_

#include "Server/public/ConnectionHandler.h"
#include "Server/public/Server.h"

#include "MobiCoreDevice.h"
#include <string>
#include <list>


#define MAX_SERVERS 2

class MobicoreDriverResources
{
public:
    Connection *conn;
    CWsm *pTciWsm;
    uint8_t *tci;
    uint32_t sessionId;

    MobicoreDriverResources(
        Connection *conn,
        uint8_t *tci,
        CWsm *pTciWsm,
        uint32_t sessionId
    ) {
        this->conn = conn;
        this->pTciWsm = pTciWsm;
        this->sessionId = sessionId;
    };
};

typedef std::list<MobicoreDriverResources *> driverResourcesList_t;

class MobiCoreDriverDaemon : ConnectionHandler
{

public:

    /**
     * Create daemon object
     *
     * @param enableScheduler Enable NQ IRQ scheduler
     * @param loadMobicore Load mobicore image to DDR
     * @param mobicoreImage Mobicore image path
     * @param donateRamSize Ram donation size in bytes
     */
    MobiCoreDriverDaemon(
        bool enableScheduler,
        /**< Mobicore loading to DDR */
        bool loadMobicore,
        std::string mobicoreImage,
        unsigned int donateRamSize,
        /**< Mobicore driver loading at start-up */
        bool loadDriver,
        std::string driverPath
    );

    virtual ~MobiCoreDriverDaemon(
        void
    );

    void dropConnection(
        Connection *connection
    );

    bool handleConnection(
        Connection *connection
    );

    void run(
        void
    );

private:

    MobiCoreDevice *mobiCoreDevice;
    /**< Flag to start/stop the scheduler */
    bool enableScheduler;
    /**< Flag to load mobicore image to DDR */
    bool loadMobicore;
    /**< Mobicore image location */
    std::string mobicoreImage;
    /**< Ram size to donate */
    unsigned int donateRamSize;
    bool loadDriver;
    std::string driverPath;
    /**< List of resources for the loaded drivers */
    driverResourcesList_t driverResources;
    /**< List of servers processing connections */
    Server *servers[MAX_SERVERS];

    size_t writeResult(
        Connection  *connection,
        mcResult_t  code
    );

    /**
        * Resolve a device ID to a MobiCore device.
        *
        * @param deviceId Device identifier of the device.
        * @return Reference to the device or NULL if device could not be found.
        */
    MobiCoreDevice *getDevice(
        uint32_t deviceId
    );

    /**
     * Load Device driver
     *
     * @param driverPath Path to the driver file
     * @return True for success/false for failure
     */
    bool loadDeviceDriver(
        std::string driverPath
    );

    void processOpenDevice(
        Connection *connection
    );

    void processOpenSession(
        Connection *connection
    );

    void processNqConnect(
        Connection *connection
    );

    void processCloseDevice(
        Connection *connection
    );

    void processNotify(
        Connection *connection
    );

    void processCloseSession(
        Connection *connection
    );

    void processMapBulkBuf(
        Connection *connection
    );

    void processUnmapBulkBuf(
        Connection *connection
    );

    void processGetVersion(
        Connection *connection
    );

    void processGetMobiCoreVersion(
        Connection *connection
    );
};

#endif /* MOBICOREDRIVER_H_ */

/** @} */
