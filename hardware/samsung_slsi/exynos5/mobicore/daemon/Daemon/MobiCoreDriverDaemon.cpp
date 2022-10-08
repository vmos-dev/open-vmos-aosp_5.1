/** @addtogroup MCD_MCDIMPL_DAEMON_CONHDLR
 * @{
 * @file
 *
 * Entry of the MobiCore Driver.
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

#include <cstdlib>
#include <signal.h>
#include <fcntl.h>
#include <stdio.h>

#include "MobiCoreDriverApi.h"
#include "MobiCoreDriverCmd.h"
#include "mcVersion.h"
#include "mcVersionHelper.h"
#include "mc_linux.h"

#include "MobiCoreDriverDaemon.h"
#include "MobiCoreRegistry.h"
#include "MobiCoreDevice.h"

#include "NetlinkServer.h"

#include "log.h"

#define DRIVER_TCI_LEN 100

#include "Mci/mci.h"

MC_CHECK_VERSION(MCI, 0, 2);
MC_CHECK_VERSION(SO, 2, 0);
MC_CHECK_VERSION(MCLF, 2, 0);
MC_CHECK_VERSION(CONTAINER, 2, 0);

static void checkMobiCoreVersion(MobiCoreDevice *mobiCoreDevice);

//------------------------------------------------------------------------------
MobiCoreDriverDaemon::MobiCoreDriverDaemon(
    bool enableScheduler,
    bool loadMobicore,
    std::string mobicoreImage,
    unsigned int donateRamSize,
    bool loadDriver,
    std::string driverPath
)
{
    mobiCoreDevice = NULL;

    this->enableScheduler = enableScheduler;
    this->loadMobicore = loadMobicore;
    this->mobicoreImage = mobicoreImage;
    this->donateRamSize = donateRamSize;
    this->loadDriver = loadDriver;
    this->driverPath = driverPath;

    for (int i = 0; i < MAX_SERVERS; i++) {
        servers[i] = NULL;
    }
}

//------------------------------------------------------------------------------
MobiCoreDriverDaemon::~MobiCoreDriverDaemon(
    void
)
{
    // Unload any device drivers might have been loaded
    driverResourcesList_t::iterator it;
    for (it = driverResources.begin(); it != driverResources.end(); it++) {
        MobicoreDriverResources *res = *it;
        mobiCoreDevice->closeSession(res->conn, res->sessionId);
        mobiCoreDevice->unregisterWsmL2(res->pTciWsm);
    }
    delete mobiCoreDevice;
    for (int i = 0; i < MAX_SERVERS; i++) {
        delete servers[i];
        servers[i] = NULL;
    }
}


//------------------------------------------------------------------------------
void MobiCoreDriverDaemon::run(
    void
)
{
    LOG_I("Daemon starting up...");
    LOG_I("Socket interface version is %u.%u", DAEMON_VERSION_MAJOR, DAEMON_VERSION_MINOR);
#ifdef MOBICORE_COMPONENT_BUILD_TAG
    LOG_I("%s", MOBICORE_COMPONENT_BUILD_TAG);
#else
#warning "MOBICORE_COMPONENT_BUILD_TAG is not defined!"
#endif
    LOG_I("Build timestamp is %s %s", __DATE__, __TIME__);

    int i;

    mobiCoreDevice = getDeviceInstance();

    LOG_I("Daemon scheduler is %s", enableScheduler ? "enabled" : "disabled");
    LOG_I("Initializing MobiCore Device");
    if (!mobiCoreDevice->initDevice(
                "/dev/" MC_ADMIN_DEVNODE,
                loadMobicore,
                mobicoreImage.c_str(),
                enableScheduler)) {
        LOG_E("Could not initialize MobiCore!");
        return;
    }
    mobiCoreDevice->start();

    LOG_I("Checking version of MobiCore");
    checkMobiCoreVersion(mobiCoreDevice);

    if (donateRamSize > 0) {
        // Donate additional RAM to MC
        LOG_I("Donating %u Kbytes to Mobicore", donateRamSize / 1024);
        mobiCoreDevice->donateRam(donateRamSize);
    }

    if (mobiCoreDevice->mobicoreAlreadyRunning()) {
        // MC is already initialized, remove all pending sessions
        #define NUM_DRIVERS         2
        #define NUM_TRUSTLETS       4
        #define NUM_SESSIONS        (1 + NUM_DRIVERS + NUM_TRUSTLETS)
        for (i = 2; i < NUM_SESSIONS; i++) {
            LOG_I("Closing session %i",i);
            mobiCoreDevice->closeSession(i);
        }
    }

    // Load device driver if requested
    if (loadDriver) {
        loadDeviceDriver(driverPath);
    }

    LOG_I("Creating socket servers");
    // Start listening for incoming TLC connections
    servers[0] = new NetlinkServer(this);
    servers[1] = new Server(this, SOCK_PATH);
    LOG_I("Successfully created servers");

    // Start all the servers
    for (i = 0; i < MAX_SERVERS; i++) {
        servers[i]->start();
    }

    // then wait for them to exit
    for (i = 0; i < MAX_SERVERS; i++) {
        servers[i]->join();
    }
}


//------------------------------------------------------------------------------
MobiCoreDevice *MobiCoreDriverDaemon::getDevice(
    uint32_t deviceId
)
{
    // Always return the trustZoneDevice as it is currently the only one supported
    if (MC_DEVICE_ID_DEFAULT != deviceId)
        return NULL;
    return mobiCoreDevice;
}


//------------------------------------------------------------------------------
void MobiCoreDriverDaemon::dropConnection(
    Connection *connection
)
{
    // Check if a Device has already been registered with the connection
    MobiCoreDevice *device = (MobiCoreDevice *) (connection->connectionData);

    if (device != NULL) {
        LOG_I("dropConnection(): closing still open device.");
        // A connection has been found and has to be closed
        device->close(connection);
    }
}


//------------------------------------------------------------------------------
size_t MobiCoreDriverDaemon::writeResult(
    Connection  *connection,
    mcResult_t  code
)
{
    if (0 != code) {
        LOG_V(" sending error code %d", code);
    }
    return connection->writeData(&code, sizeof(mcResult_t));
}

//------------------------------------------------------------------------------
bool MobiCoreDriverDaemon::loadDeviceDriver(
    std::string driverPath
)
{
    bool ret = false;
    CWsm_ptr pWsm = NULL, pTciWsm = NULL;
    regObject_t *regObj = NULL;
    Connection *conn = NULL;
    uint8_t *tci = NULL;
    mcDrvRspOpenSession_t rspOpenSession;

    do {
        //mobiCoreDevice
        FILE *fs = fopen (driverPath.c_str(), "rb");
        if (!fs) {
            LOG_E("%s: failed: cannot open %s", __FUNCTION__, driverPath.c_str());
            break;
        }
        fclose(fs);

        LOG_I("%s: loading %s", __FUNCTION__, driverPath.c_str());

        regObj = mcRegistryGetDriverBlob(driverPath.c_str());
        if (regObj == NULL) {
            break;;
        }

        LOG_I("registering L2 in kmod, p=%p, len=%i",
              regObj->value, regObj->len);

        pWsm = mobiCoreDevice->registerWsmL2(
                   (addr_t)(regObj->value), regObj->len, 0);
        if (pWsm == NULL) {
            LOG_E("allocating WSM for Trustlet failed");
            break;
        }
        // Initialize information data of open session command
        loadDataOpenSession_t loadDataOpenSession;
        loadDataOpenSession.baseAddr = pWsm->physAddr;
        loadDataOpenSession.offs = ((uint32_t) regObj->value) & 0xFFF;
        loadDataOpenSession.len = regObj->len;
        loadDataOpenSession.tlHeader = (mclfHeader_ptr) regObj->value;

        MC_DRV_CMD_OPEN_SESSION_struct  cmdOpenSession;
        tci = (uint8_t *)malloc(DRIVER_TCI_LEN);
        pTciWsm = mobiCoreDevice->allocateContiguousPersistentWsm(DRIVER_TCI_LEN);
        if (pTciWsm == NULL) {
            LOG_E("allocating WSM TCI for Trustlet failed");
            break;
        }
        cmdOpenSession.deviceId = MC_DEVICE_ID_DEFAULT;
        cmdOpenSession.tci = (uint32_t)pTciWsm->physAddr;
        cmdOpenSession.len = DRIVER_TCI_LEN;
        cmdOpenSession.handle = pTciWsm->handle;

        conn = new Connection();
        uint32_t mcRet = mobiCoreDevice->openSession(
                             conn,
                             &loadDataOpenSession,
                             &cmdOpenSession,
                             &(rspOpenSession.payload));

        // Unregister physical memory from kernel module.
        // This will also destroy the WSM object.
        mobiCoreDevice->unregisterWsmL2(pWsm);
        pWsm = NULL;

        // Free memory occupied by Trustlet data
        free(regObj);
        regObj = NULL;

        if (mcRet != MC_MCP_RET_OK) {
            LOG_E("open session error %d", mcRet);
            break;
        }

        ret = true;
    } while (false);
    // Free all allocated resources
    if (ret == false) {
        LOG_I("%s: Freeing previously allocated resources!", __FUNCTION__);
        if (pWsm != NULL) {
            if (!mobiCoreDevice->unregisterWsmL2(pWsm)) {
                // At least make sure we don't leak the WSM object
                delete pWsm;
            }
        }
        // No matter if we free NULL objects
        free(regObj);

        if (conn != NULL) {
            delete conn;
        }
    } else if (conn != NULL) {
        driverResources.push_back(new MobicoreDriverResources(
                                      conn, tci, pTciWsm, rspOpenSession.payload.sessionId));
    }

    return ret;
}

#define RECV_PAYLOAD_FROM_CLIENT(CONNECTION, CMD_BUFFER) \
{ \
    void *payload = (void*)((uint32_t)CMD_BUFFER + sizeof(mcDrvCommandHeader_t)); \
    uint32_t payload_len = sizeof(*CMD_BUFFER) - sizeof(mcDrvCommandHeader_t); \
    uint32_t rlen = CONNECTION->readData(payload, payload_len); \
    if (rlen < 0) { \
        LOG_E("reading from Client failed"); \
        /* it is questionable, if writing to broken socket has any effect here. */ \
        writeResult(CONNECTION, MC_DRV_ERR_DAEMON_SOCKET); \
        return; \
    } \
    if (rlen != payload_len) {\
        LOG_E("wrong buffer length %i received from Client", rlen); \
        writeResult(CONNECTION, MC_DRV_ERR_DAEMON_SOCKET); \
        return; \
    } \
}

#define CHECK_DEVICE(DEVICE, CONNECTION) \
    if (DEVICE == NULL) \
    { \
        LOG_V("%s: no device associated with connection",__FUNCTION__); \
        writeResult(CONNECTION, MC_DRV_ERR_DAEMON_DEVICE_NOT_OPEN); \
        return; \
    }

//------------------------------------------------------------------------------
void MobiCoreDriverDaemon::processOpenDevice(
    Connection  *connection
)
{
    MC_DRV_CMD_OPEN_DEVICE_struct cmdOpenDevice;
    RECV_PAYLOAD_FROM_CLIENT(connection, &cmdOpenDevice);

    // Check if device has been registered to the connection
    MobiCoreDevice *device = (MobiCoreDevice *) (connection->connectionData);
    if (NULL != device) {
        LOG_E("processOpenDevice(): device already set");
        writeResult(connection, MC_DRV_ERR_DEVICE_ALREADY_OPEN);
        return;
    }

    LOG_I(" Opening deviceId %d ", cmdOpenDevice.deviceId);

    // Get device for device ID
    device = getDevice(cmdOpenDevice.deviceId);

    // Check if a device for the given name has been found
    if (device == NULL) {
        LOG_E("invalid deviceId");
        writeResult(connection, MC_DRV_ERR_UNKNOWN_DEVICE);
        return;
    }

    // Register device object with connection
    device->open(connection);

    // Return result code to client lib (no payload)
    writeResult(connection, MC_DRV_OK);
}


//------------------------------------------------------------------------------
void MobiCoreDriverDaemon::processCloseDevice(
    Connection  *connection
)
{
    // there is no payload to read

    // Device required
    MobiCoreDevice *device = (MobiCoreDevice *) (connection->connectionData);
    CHECK_DEVICE(device, connection);

    // No command data will be read
    // Unregister device object with connection
    device->close(connection);

    // there is no payload
    writeResult(connection, MC_DRV_OK);
}


//------------------------------------------------------------------------------
void MobiCoreDriverDaemon::processOpenSession(
    Connection  *connection
)
{
    MC_DRV_CMD_OPEN_SESSION_struct cmdOpenSession;
    RECV_PAYLOAD_FROM_CLIENT(connection, &cmdOpenSession);

    // Device required
    MobiCoreDevice  *device = (MobiCoreDevice *) (connection->connectionData);
    CHECK_DEVICE(device, connection);

    // Get service blob from registry
    regObject_t *regObj = mcRegistryGetServiceBlob(&(cmdOpenSession.uuid));
    if (NULL == regObj) {
        writeResult(connection, MC_DRV_ERR_TRUSTLET_NOT_FOUND);
        return;
    }
    if (regObj->len == 0) {
        free(regObj);
        writeResult(connection, MC_DRV_ERR_TRUSTLET_NOT_FOUND);
        return;
    }
    LOG_I(" Sharing Service loaded at %p with Secure World", (addr_t)(regObj->value));

    CWsm_ptr pWsm = device->registerWsmL2((addr_t)(regObj->value), regObj->len, 0);
    if (pWsm == NULL) {
        LOG_E("allocating WSM for Trustlet failed");
        writeResult(connection, MC_DRV_ERR_DAEMON_KMOD_ERROR);
        return;
    }
    // Initialize information data of open session command
    loadDataOpenSession_t loadDataOpenSession;
    loadDataOpenSession.baseAddr = pWsm->physAddr;
    loadDataOpenSession.offs = ((uint32_t) regObj->value) & 0xFFF;
    loadDataOpenSession.len = regObj->len;
    loadDataOpenSession.tlHeader = (mclfHeader_ptr) regObj->value;

    mcDrvRspOpenSession_t rspOpenSession;
    mcResult_t ret = device->openSession(
                         connection,
                         &loadDataOpenSession,
                         &cmdOpenSession,
                         &(rspOpenSession.payload));

    // Unregister physical memory from kernel module.
    LOG_I(" Service buffer was copied to Secure world and processed. Stop sharing of buffer.");

    // This will also destroy the WSM object.
    if (!device->unregisterWsmL2(pWsm)) {
        // TODO-2012-07-02-haenellu: Can this ever happen? And if so, we should assert(), also TL might still be running.
        writeResult(connection, MC_DRV_ERR_DAEMON_KMOD_ERROR);
        return;
    }

    // Free memory occupied by Trustlet data
    free(regObj);

    if (ret != MC_DRV_OK) {
        LOG_E("Service could not be loaded.");
        writeResult(connection, ret);
    } else {
        rspOpenSession.header.responseId = ret;
        connection->writeData(
            &rspOpenSession,
            sizeof(rspOpenSession));
    }
}


//------------------------------------------------------------------------------
void MobiCoreDriverDaemon::processCloseSession(Connection *connection)
{
    MC_DRV_CMD_CLOSE_SESSION_struct cmdCloseSession;
    RECV_PAYLOAD_FROM_CLIENT(connection, &cmdCloseSession)

    // Device required
    MobiCoreDevice *device = (MobiCoreDevice *) (connection->connectionData);
    CHECK_DEVICE(device, connection);

    mcResult_t ret = device->closeSession(connection, cmdCloseSession.sessionId);

    // there is no payload
    writeResult(connection, ret);
}


//------------------------------------------------------------------------------
void MobiCoreDriverDaemon::processNqConnect(Connection *connection)
{
    // Set up the channel for sending SWd notifications to the client
    // MC_DRV_CMD_NQ_CONNECT is only allowed on new connections not
    // associated with a device. If a device is registered to the
    // connection NQ_CONNECT is not allowed.

    // Read entire command data
    MC_DRV_CMD_NQ_CONNECT_struct cmd;
    RECV_PAYLOAD_FROM_CLIENT(connection, &cmd);

    // device must be empty since this is a new connection
    MobiCoreDevice *device = (MobiCoreDevice *)(connection->connectionData);
    if (device != NULL) {
        LOG_E("device already set\n");
        writeResult(connection, MC_DRV_ERR_NQ_FAILED);
        return;
    }

    // Remove the connection from the list of known client connections
    for (int i = 0; i < MAX_SERVERS; i++) {
        servers[i]->detachConnection(connection);
    }

    device = getDevice(cmd.deviceId);
    // Check if a device for the given name has been found
    if (NULL == device) {
        LOG_E("invalid deviceId");
        writeResult(connection, MC_DRV_ERR_UNKNOWN_DEVICE);
        return;
    }

    TrustletSession *ts = device->registerTrustletConnection(
                              connection,
                              &cmd);
    if (!ts) {
        LOG_E("registerTrustletConnection() failed!");
        writeResult(connection, MC_DRV_ERR_UNKNOWN);
        return;
    }

    writeResult(connection, MC_DRV_OK);
    ts->processQueuedNotifications();
}


//------------------------------------------------------------------------------
void MobiCoreDriverDaemon::processNotify(
    Connection  *connection
)
{

    // Read entire command data
    MC_DRV_CMD_NOTIFY_struct  cmd;
    //RECV_PAYLOAD_FROM_CLIENT(connection, &cmd);
    void *payload = (void *)((uint32_t)&cmd + sizeof(mcDrvCommandHeader_t));
    uint32_t payload_len = sizeof(cmd) - sizeof(mcDrvCommandHeader_t);
    uint32_t rlen = connection->readData(payload, payload_len);
    if (rlen < 0) {
        LOG_E("reading from Client failed");
        /* it is questionable, if writing to broken socket has any effect here. */
        // NOTE: notify fails silently
        //writeResult(connection, MC_DRV_RSP_SOCKET_ERROR);
        return;
    }
    if (rlen != payload_len) {
        LOG_E("wrong buffer length %i received from Client", rlen);
        // NOTE: notify fails silently
        //writeResult(connection, MC_DRV_RSP_PAYLOAD_LENGTH_ERROR);
        return;
    }

    // Device required
    MobiCoreDevice *device = (MobiCoreDevice *) (connection->connectionData);
    if (NULL == device) {
        LOG_V("%s: no device associated with connection", __FUNCTION__);
        // NOTE: notify fails silently
        // writeResult(connection,MC_DRV_RSP_DEVICE_NOT_OPENED);
        return;
    }

    // REV axh: we cannot trust the clientLib to give us a valid
    //          sessionId here. Thus we have to check that it belongs to
    //          the clientLib's process.

    device->notify(cmd.sessionId);
    // NOTE: for notifications there is no response at all
}


//------------------------------------------------------------------------------
void MobiCoreDriverDaemon::processMapBulkBuf(Connection *connection)
{
    MC_DRV_CMD_MAP_BULK_BUF_struct cmd;

    RECV_PAYLOAD_FROM_CLIENT(connection, &cmd);

    // Device required
    MobiCoreDevice *device = (MobiCoreDevice *) (connection->connectionData);
    CHECK_DEVICE(device, connection);

    if (!device->lockWsmL2(cmd.handle)) {
        LOG_E("Couldn't lock the buffer!");
        writeResult(connection, MC_DRV_ERR_DAEMON_WSM_HANDLE_NOT_FOUND);
        return;
    }

    uint32_t secureVirtualAdr = NULL;
    uint32_t pAddrL2 = (uint32_t)device->findWsmL2(cmd.handle);

    if (pAddrL2 == 0) {
        LOG_E("Failed to resolve WSM with handle %u", cmd.handle);
        writeResult(connection, MC_DRV_ERR_DAEMON_WSM_HANDLE_NOT_FOUND);
        return;
    }

    // Map bulk memory to secure world
    mcResult_t mcResult = device->mapBulk(cmd.sessionId, cmd.handle, pAddrL2,
                                          cmd.offsetPayload, cmd.lenBulkMem, &secureVirtualAdr);

    if (mcResult != MC_DRV_OK) {
        writeResult(connection, mcResult);
        return;
    }

    mcDrvRspMapBulkMem_t rsp;
    rsp.header.responseId = MC_DRV_OK;
    rsp.payload.sessionId = cmd.sessionId;
    rsp.payload.secureVirtualAdr = secureVirtualAdr;
    connection->writeData(&rsp, sizeof(mcDrvRspMapBulkMem_t));
}


//------------------------------------------------------------------------------
void MobiCoreDriverDaemon::processUnmapBulkBuf(Connection *connection)
{
    MC_DRV_CMD_UNMAP_BULK_BUF_struct cmd;
    RECV_PAYLOAD_FROM_CLIENT(connection, &cmd)

    // Device required
    MobiCoreDevice *device = (MobiCoreDevice *) (connection->connectionData);
    CHECK_DEVICE(device, connection);

    // Unmap bulk memory from secure world
    uint32_t mcResult = device->unmapBulk(cmd.sessionId, cmd.handle, cmd.secureVirtualAdr, cmd.lenBulkMem);

    if (mcResult != MC_DRV_OK) {
        LOG_V("MCP UNMAP returned code %d", mcResult);
        writeResult(connection, mcResult);
        return;
    }

    // TODO-2012-09-06-haenellu: Think about not ignoring the error case.
    device->unlockWsmL2(cmd.handle);

    writeResult(connection, MC_DRV_OK);
}


//------------------------------------------------------------------------------
void MobiCoreDriverDaemon::processGetVersion(
    Connection  *connection
)
{
    // there is no payload to read

    mcDrvRspGetVersion_t rspGetVersion;
    rspGetVersion.version = MC_MAKE_VERSION(DAEMON_VERSION_MAJOR, DAEMON_VERSION_MINOR);
    rspGetVersion.responseId = MC_DRV_OK;

    connection->writeData(&rspGetVersion, sizeof(mcDrvRspGetVersion_t));
}

//------------------------------------------------------------------------------
void MobiCoreDriverDaemon::processGetMobiCoreVersion(
    Connection  *connection
)
{
    // there is no payload to read

    // Device required
    MobiCoreDevice *device = (MobiCoreDevice *) (connection->connectionData);
    CHECK_DEVICE(device, connection);

    // Get MobiCore version info from secure world.
    mcDrvRspGetMobiCoreVersion_t rspGetMobiCoreVersion;

    mcResult_t mcResult = device->getMobiCoreVersion(&rspGetMobiCoreVersion.payload);

    if (mcResult != MC_DRV_OK) {
        LOG_V("MC GET_MOBICORE_VERSION returned code %d", mcResult);
        writeResult(connection, mcResult);
        return;
    }

    rspGetMobiCoreVersion.header.responseId = MC_DRV_OK;
    connection->writeData(
        &rspGetMobiCoreVersion,
        sizeof(rspGetMobiCoreVersion));
}


//------------------------------------------------------------------------------
bool MobiCoreDriverDaemon::handleConnection(
    Connection *connection
)
{
    bool ret = false;
    static CMutex mutex;

    /* In case of RTM fault do not try to signal anything to MobiCore
     * just answer NO to all incoming connections! */
    if (mobiCoreDevice->getMcFault()) {
        return false;
    }

    mutex.lock();
    LOG_I("handleConnection()==== %p", connection);
    do {
        // Read header
        mcDrvCommandHeader_t mcDrvCommandHeader;
        ssize_t rlen = connection->readData(
                           &(mcDrvCommandHeader),
                           sizeof(mcDrvCommandHeader));

        if (rlen == 0) {
            LOG_V(" handleConnection(): Connection closed.");
            break;
        }
        if (rlen == -1) {
            LOG_E("Socket error.");
            break;
        }
        if (rlen == -2) {
            LOG_E("Timeout.");
            break;
        }
        if (rlen != sizeof(mcDrvCommandHeader)) {
            LOG_E("Header length %i is not right.", (int)rlen);
            break;
        }
        ret = true;

        switch (mcDrvCommandHeader.commandId) {
            //-----------------------------------------
        case MC_DRV_CMD_OPEN_DEVICE:
            processOpenDevice(connection);
            break;
            //-----------------------------------------
        case MC_DRV_CMD_CLOSE_DEVICE:
            processCloseDevice(connection);
            break;
            //-----------------------------------------
        case MC_DRV_CMD_OPEN_SESSION:
            processOpenSession(connection);
            break;
            //-----------------------------------------
        case MC_DRV_CMD_CLOSE_SESSION:
            processCloseSession(connection);
            break;
            //-----------------------------------------
        case MC_DRV_CMD_NQ_CONNECT:
            processNqConnect(connection);
            break;
            //-----------------------------------------
        case MC_DRV_CMD_NOTIFY:
            processNotify(connection);
            break;
            //-----------------------------------------
        case MC_DRV_CMD_MAP_BULK_BUF:
            processMapBulkBuf(connection);
            break;
            //-----------------------------------------
        case MC_DRV_CMD_UNMAP_BULK_BUF:
            processUnmapBulkBuf(connection);
            break;
            //-----------------------------------------
        case MC_DRV_CMD_GET_VERSION:
            processGetVersion(connection);
            break;
            //-----------------------------------------
        case MC_DRV_CMD_GET_MOBICORE_VERSION:
            processGetMobiCoreVersion(connection);
            break;
            //-----------------------------------------

        default:
            LOG_E("Unknown command: %d=0x%x",
                  mcDrvCommandHeader.commandId,
                  mcDrvCommandHeader.commandId);
            ret = false;
            break;
        }
    } while (0);
    mutex.unlock();
    LOG_I("handleConnection()<-------");

    return ret;
}

//------------------------------------------------------------------------------
/**
 * Print daemon command line options
 */

void printUsage(
    int argc,
    char *args[]
)
{
    fprintf(stderr, "usage: %s [-mdsbh]\n", args[0]);
    fprintf(stderr, "Start MobiCore Daemon\n\n");
    fprintf(stderr, "-h\t\tshow this help\n");
    fprintf(stderr, "-b\t\tfork to background\n");
    fprintf(stderr, "-m IMAGE\tload mobicore from IMAGE to DDR\n");
    fprintf(stderr, "-s\t\tdisable daemon scheduler(default enabled)\n");
    fprintf(stderr, "-d SIZE\t\tdonate SIZE bytes to mobicore(disabled on most platforms)\n");
    fprintf(stderr, "-r DRIVER\t\tMobiCore driver to load at start-up\n");
}

//------------------------------------------------------------------------------
/**
 * Signal handler for daemon termination
 * Using this handler instead of the standard libc one ensures the daemon
 * can cleanup everything -> read() on a FD will now return EINTR
 */
void terminateDaemon(
    int signum
)
{
    LOG_E("Signal %d received\n", signum);
}

//------------------------------------------------------------------------------
/**
 * Main entry of the MobiCore Driver Daemon.
 */
int main(
    int argc,
    char *args[]
)
{
    // Create the MobiCore Driver Singleton
    MobiCoreDriverDaemon *mobiCoreDriverDaemon = NULL;
    // Process signal action
    struct sigaction action;

    // Read the Command line options
    extern char *optarg;
    extern int optopt;
    int c, errFlag = 0;
    // Scheduler enabled by default
    int schedulerFlag = 1;
    // Mobicore loading disable by default
    int mobicoreFlag = 0;
    // Autoload driver at start-up
    int driverLoadFlag = 0;
    std::string mobicoreImage, driverPath;
    // Ram donation disabled by default
    int donationSize = 0;
    // By default don't fork
    bool forkDaemon = false;
    while ((c = getopt(argc, args, "m:d:r:sbh")) != -1) {
        switch (c) {
        case 'h': /* Help */
            errFlag++;
            break;
        case 's': /* Disable Scheduler */
            schedulerFlag = 0;
            break;
        case 'd': /* Ram Donation size */
            donationSize = atoi(optarg);
            break;
        case 'm': /* Load mobicore image */
            mobicoreFlag = 1;
            mobicoreImage = optarg;
            break;
        case 'b': /* Fork to background */
            forkDaemon = true;
            break;
        case 'r': /* Load mobicore driver at start-up */
            driverLoadFlag = 1;
            driverPath = optarg;
            break;
        case ':':       /* -d or -m without operand */
            fprintf(stderr, "Option -%c requires an operand\n", optopt);
            errFlag++;
            break;
        case '?':
            fprintf(stderr,
                    "Unrecognized option: -%c\n", optopt);
            errFlag++;
        }
    }
    if (errFlag) {
        printUsage(argc, args);
        exit(2);
    }

    // We should fork the daemon to background
    if (forkDaemon == true) {
        int i = fork();
        if (i < 0) {
            exit(1);
        }
        // Parent
        else if (i > 0) {
            exit(0);
        }

        // obtain a new process group */
        setsid();
        /* close all descriptors */
        for (i = sysconf(_SC_OPEN_MAX); i >= 0; --i) {
            close(i);
        }
        // STDIN, STDOUT and STDERR should all point to /dev/null */
        i = open("/dev/null", O_RDWR);
        dup(i);
        dup(i);
        /* ignore tty signals */
        signal(SIGTSTP, SIG_IGN);
        signal(SIGTTOU, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);
    }

    // Set up the structure to specify the new action.
    action.sa_handler = terminateDaemon;
    sigemptyset (&action.sa_mask);
    action.sa_flags = 0;
    sigaction (SIGINT, &action, NULL);
    sigaction (SIGHUP, &action, NULL);
    sigaction (SIGTERM, &action, NULL);
    signal(SIGPIPE, SIG_IGN);

    mobiCoreDriverDaemon = new MobiCoreDriverDaemon(
        /* Scheduler status */
        schedulerFlag,
        /* Mobicore loading to DDR */
        mobicoreFlag,
        mobicoreImage,
        /* Ram Donation */
        donationSize,
        /* Auto Driver loading */
        driverLoadFlag,
        driverPath);

    // Start the driver
    mobiCoreDriverDaemon->run();

    delete mobiCoreDriverDaemon;

    // This should not happen
    LOG_E("Exiting MobiCoreDaemon");

    return EXIT_FAILURE;
}

//------------------------------------------------------------------------------
static void checkMobiCoreVersion(
    MobiCoreDevice *mobiCoreDevice
)
{
    bool failed = false;

    // Get MobiCore version info.
    mcDrvRspGetMobiCoreVersionPayload_t versionPayload;
    mcResult_t mcResult = mobiCoreDevice->getMobiCoreVersion(&versionPayload);

    if (mcResult != MC_DRV_OK) {
        LOG_E("Failed to obtain MobiCore version info. MCP return code: %u", mcResult);
        failed = true;
    } else {
        LOG_I("Product ID is %s", versionPayload.versionInfo.productId);

        // Check MobiCore version info.
        char *msg;
        if (!checkVersionOkMCI(versionPayload.versionInfo.versionMci, &msg)) {
            LOG_E("%s", msg);
            failed = true;
        }
        LOG_I("%s", msg);
        if (!checkVersionOkSO(versionPayload.versionInfo.versionSo, &msg)) {
            LOG_E("%s", msg);
            failed = true;
        }
        LOG_I("%s", msg);
        if (!checkVersionOkMCLF(versionPayload.versionInfo.versionMclf, &msg)) {
            LOG_E("%s", msg);
            failed = true;
        }
        LOG_I("%s", msg);
        if (!checkVersionOkCONTAINER(versionPayload.versionInfo.versionContainer, &msg)) {
            LOG_E("%s", msg);
            failed = true;
        }
        LOG_I("%s", msg);
    }

    if (failed) {
        exit(1);
    }
}

/** @} */
