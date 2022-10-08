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

#include <cstdlib>
#include <pthread.h>
#include "McTypes.h"

#include "DeviceScheduler.h"
#include "DeviceIrqHandler.h"
#include "ExcDevice.h"
#include "Connection.h"
#include "TrustletSession.h"

#include "MobiCoreDevice.h"
#include "Mci/mci.h"
#include "mcLoadFormat.h"


#include "log.h"
#include "public/MobiCoreDevice.h"


//------------------------------------------------------------------------------
MobiCoreDevice::MobiCoreDevice()
{
    mcFault = false;
}

//------------------------------------------------------------------------------
MobiCoreDevice::~MobiCoreDevice()
{
    delete mcVersionInfo;
    mcVersionInfo = NULL;
}

//------------------------------------------------------------------------------
TrustletSession *MobiCoreDevice::getTrustletSession(uint32_t sessionId)
{
    for (trustletSessionIterator_t session = trustletSessions.begin();
            session != trustletSessions.end();
            ++session) {
        TrustletSession *tsTmp = *session;
        if (tsTmp->sessionId == sessionId) {
            return tsTmp;
        }
    }
    return NULL;
}


void MobiCoreDevice::cleanSessionBuffers(TrustletSession *session)
{
    CWsm_ptr pWsm = session->popBulkBuff();

    while (pWsm) {
        unlockWsmL2(pWsm->handle);
        pWsm = session->popBulkBuff();
    }
}
//------------------------------------------------------------------------------
void MobiCoreDevice::removeTrustletSession(uint32_t sessionId)
{
    for (trustletSessionIterator_t session = trustletSessions.begin();
            session != trustletSessions.end();
            ++session) {
        if ((*session)->sessionId == sessionId) {
            cleanSessionBuffers(*session);
            trustletSessions.erase(session);
            return;
        }
    }
}
//------------------------------------------------------------------------------
Connection *MobiCoreDevice::getSessionConnection(uint32_t sessionId, notification_t *notification)
{
    Connection *con = NULL;
    TrustletSession *ts = NULL;

    ts = getTrustletSession(sessionId);
    if (ts == NULL) {
        return NULL;
    }

    con = ts->notificationConnection;
    if (con == NULL) {
        ts->queueNotification(notification);
        return NULL;
    }

    return con;
}


//------------------------------------------------------------------------------
bool MobiCoreDevice::open(Connection *connection)
{
    // Link this device to the connection
    connection->connectionData = this;
    return true;
}


//------------------------------------------------------------------------------
/**
 * Close device.
 *
 * Removes all sessions to a connection. Though, clientLib rejects the closeDevice()
 * command if still sessions connected to the device, this is needed to clean up all
 * sessions if client dies.
 */
void MobiCoreDevice::close(Connection *connection)
{
    trustletSessionList_t::reverse_iterator interator;
    static CMutex mutex;
    // 1. Iterate through device session to find connection
    // 2. Decide what to do with open Trustlet sessions
    // 3. Remove & delete deviceSession from vector

    // Enter critical section
    mutex.lock();
    for (interator = trustletSessions.rbegin();
            interator != trustletSessions.rend();
            interator++) {
        TrustletSession *ts = *interator;

        if (ts->deviceConnection == connection) {
            closeSession(connection, ts->sessionId);
        }
    }
    // Leave critical section
    mutex.unlock();

    // After the trustlet is done make sure to tell the driver to cleanup
    // all the orphaned drivers
    cleanupWsmL2();

    connection->connectionData = NULL;
}


//------------------------------------------------------------------------------
void MobiCoreDevice::start(void)
{
    // Call the device specific initialization
    //  initDevice();

    LOG_I("Starting DeviceIrqHandler...");
    // Start the irq handling thread
    DeviceIrqHandler::start();

    if (schedulerAvailable()) {
        LOG_I("Starting DeviceScheduler...");
        // Start the scheduling handling thread
        DeviceScheduler::start();
    } else {
        LOG_I("No DeviceScheduler available.");
    }
}


//------------------------------------------------------------------------------
void MobiCoreDevice::signalMcpNotification(void)
{
    mcpSessionNotification.signal();
}


//------------------------------------------------------------------------------
bool MobiCoreDevice::waitMcpNotification(void)
{
    int counter = 5;
    while (1) {
        // In case of fault just return, nothing to do here
        if (mcFault) {
            return false;
        }
        // Wait 10 seconds for notification
        if (mcpSessionNotification.wait(10) == false) {
            // No MCP answer received and mobicore halted, dump mobicore status
            // then throw exception
            LOG_I("No MCP answer received in 2 seconds.");
            if (getMobicoreStatus() == MC_STATUS_HALT) {
                dumpMobicoreStatus();
                mcFault = true;
                return false;
            } else {
                counter--;
                if (counter < 1) {
                    mcFault = true;
                    return false;
                }
            }
        } else {
            break;
        }
    }

    // Check healthiness state of the device
    if (DeviceIrqHandler::isExiting()) {
        LOG_I("waitMcpNotification(): IrqHandler thread died! Joining");
        DeviceIrqHandler::join();
        LOG_I("waitMcpNotification(): Joined");
        LOG_E("IrqHandler thread died!");
        return false;
    }

    if (DeviceScheduler::isExiting()) {
        LOG_I("waitMcpNotification(): Scheduler thread died! Joining");
        DeviceScheduler::join();
        LOG_I("waitMcpNotification(): Joined");
        LOG_E("Scheduler thread died!");
        return false;
    }
    return true;
}


//------------------------------------------------------------------------------
mcResult_t MobiCoreDevice::openSession(
    Connection                      *deviceConnection,
    loadDataOpenSession_ptr         pLoadDataOpenSession,
    MC_DRV_CMD_OPEN_SESSION_struct  *cmdOpenSession,
    mcDrvRspOpenSessionPayload_ptr  pRspOpenSessionPayload
)
{
    do {
        addr_t tci;
        uint32_t len;
        uint32_t handle = cmdOpenSession->handle;

        if (!findContiguousWsm(handle, &tci, &len)) {
            LOG_E("Failed to find contiguous WSM %u", handle);
            return MC_DRV_ERR_DAEMON_WSM_HANDLE_NOT_FOUND;
        }

        if (!lockWsmL2(handle)) {
            LOG_E("Failed to lock contiguous WSM %u", handle);
            return MC_DRV_ERR_DAEMON_WSM_HANDLE_NOT_FOUND;
        }

        // Write MCP open message to buffer
        mcpMessage->cmdOpen.cmdHeader.cmdId = MC_MCP_CMD_OPEN_SESSION;
        mcpMessage->cmdOpen.uuid = cmdOpenSession->uuid;
        mcpMessage->cmdOpen.wsmTypeTci = WSM_CONTIGUOUS;
        mcpMessage->cmdOpen.adrTciBuffer = (uint32_t)(tci);
        mcpMessage->cmdOpen.ofsTciBuffer = 0;
        mcpMessage->cmdOpen.lenTciBuffer = len;

        LOG_I(" Using phys=%p, len=%d as TCI buffer",
              (addr_t)(cmdOpenSession->tci),
              cmdOpenSession->len);

        // check if load data is provided
        mcpMessage->cmdOpen.wsmTypeLoadData = WSM_L2;
        mcpMessage->cmdOpen.adrLoadData = (uint32_t)pLoadDataOpenSession->baseAddr;
        mcpMessage->cmdOpen.ofsLoadData = pLoadDataOpenSession->offs;
        mcpMessage->cmdOpen.lenLoadData = pLoadDataOpenSession->len;
        memcpy(&mcpMessage->cmdOpen.tlHeader, pLoadDataOpenSession->tlHeader, sizeof(*pLoadDataOpenSession->tlHeader));

        // Clear the notifications queue. We asume the race condition we have
        // seen in openSession never happens elsewhere
        notifications = std::queue<notification_t>();
        // Notify MC about a new command inside the MCP buffer
        notify(SID_MCP);

        // Wait till response from MC is available
        if (!waitMcpNotification()) {
            // Here Mobicore can be considered dead.
            unlockWsmL2(handle);
            return MC_DRV_ERR_DAEMON_MCI_ERROR;
        }

        // Check if the command response ID is correct
        if ((MC_MCP_CMD_OPEN_SESSION | FLAG_RESPONSE) != mcpMessage->rspHeader.rspId) {
            LOG_E("CMD_OPEN_SESSION got invalid MCP command response(0x%X)", mcpMessage->rspHeader.rspId);
            // Something is messing with our MCI memory, we cannot know if the Trustlet was loaded.
            // Had in been loaded, we are loosing track of it here.
            unlockWsmL2(handle);
            return MC_DRV_ERR_DAEMON_MCI_ERROR;
        }

        uint32_t mcRet = mcpMessage->rspOpen.rspHeader.result;

        if (mcRet != MC_MCP_RET_OK) {
            LOG_E("MCP OPEN returned code %d.", mcRet);
            unlockWsmL2(handle);
            return MAKE_MC_DRV_MCP_ERROR(mcRet);
        }

        LOG_I(" After MCP OPEN, we have %d queued notifications",
              notifications.size());
        // Read MC answer from MCP buffer
        TrustletSession *trustletSession = new TrustletSession(
            deviceConnection,
            mcpMessage->rspOpen.sessionId);

        pRspOpenSessionPayload->sessionId = trustletSession->sessionId;
        pRspOpenSessionPayload->deviceSessionId = (uint32_t)trustletSession;
        pRspOpenSessionPayload->sessionMagic = trustletSession->sessionMagic;

        trustletSessions.push_back(trustletSession);

        trustletSession->addBulkBuff(new CWsm((void *)pLoadDataOpenSession->offs, pLoadDataOpenSession->len, handle, 0));

        // We have some queued notifications and we need to send them to them
        // trustlet session
        while (!notifications.empty()) {
            trustletSession->queueNotification(&notifications.front());
            notifications.pop();
        }

    } while (0);
    return MC_DRV_OK;
}


//------------------------------------------------------------------------------
TrustletSession *MobiCoreDevice::registerTrustletConnection(
    Connection                    *connection,
    MC_DRV_CMD_NQ_CONNECT_struct *cmdNqConnect
)
{
    LOG_I(" Registering notification socket with Service session %d.",
          cmdNqConnect->sessionId);
    LOG_V("  Searching sessionId %d with sessionMagic %d",
          cmdNqConnect->sessionId,
          cmdNqConnect->sessionMagic);

    for (trustletSessionIterator_t iterator = trustletSessions.begin();
            iterator != trustletSessions.end();
            ++iterator) {
        TrustletSession *ts = *iterator;

        if (ts != (TrustletSession *) (cmdNqConnect->deviceSessionId)) {
            continue;
        }

        if ( (ts->sessionMagic != cmdNqConnect->sessionMagic)
                || (ts->sessionId != cmdNqConnect->sessionId)) {
            continue;
        }

        ts->notificationConnection = connection;

        LOG_I(" Found Service session, registered connection.");

        return ts;
    }

    LOG_I("registerTrustletConnection(): search failed");
    return NULL;
}


//------------------------------------------------------------------------------
mcResult_t MobiCoreDevice::closeSession(uint32_t sessionId)
{
    LOG_I(" Write MCP CLOSE message to MCI, notify and wait");

    // Write MCP close message to buffer
    mcpMessage->cmdClose.cmdHeader.cmdId = MC_MCP_CMD_CLOSE_SESSION;
    mcpMessage->cmdClose.sessionId = sessionId;

    // Notify MC about the availability of a new command inside the MCP buffer
    notify(SID_MCP);

    // Wait till response from MSH is available
    if (!waitMcpNotification()) {
        return MC_DRV_ERR_DAEMON_MCI_ERROR;
    }

    // Check if the command response ID is correct
    if ((MC_MCP_CMD_CLOSE_SESSION | FLAG_RESPONSE) != mcpMessage->rspHeader.rspId) {
        LOG_E("CMD_CLOSE_SESSION got invalid MCP response");
        return MC_DRV_ERR_DAEMON_MCI_ERROR;
    }

    // Read MC answer from MCP buffer
    uint32_t mcRet = mcpMessage->rspOpen.rspHeader.result;

    if (mcRet != MC_MCP_RET_OK) {
        LOG_E("CMD_CLOSE_SESSION error %d", mcRet);
        return MAKE_MC_DRV_MCP_ERROR(mcRet);
    }

    return MC_DRV_OK;
}

//------------------------------------------------------------------------------
/**
 * TODO-2012-09-19-haenellu: Do some more checks here, otherwise rogue clientLib
 * can close sessions from different TLCs. That is, deviceConnection is ignored below.
 *
 * Need connection as well as according session ID, so that a client can not
 * close sessions not belonging to him.
 */
mcResult_t MobiCoreDevice::closeSession(Connection *deviceConnection, uint32_t sessionId)
{
    TrustletSession *ts = getTrustletSession(sessionId);
    if (ts == NULL) {
        LOG_E("no session found with id=%d", sessionId);
        return MC_DRV_ERR_DAEMON_UNKNOWN_SESSION;
    }

    uint32_t mcRet = closeSession(sessionId);
    if (mcRet != MC_DRV_OK) {
        return mcRet;
    }

    // remove objects
    removeTrustletSession(sessionId);
    delete ts;

    return MC_DRV_OK;
}


//------------------------------------------------------------------------------
mcResult_t MobiCoreDevice::mapBulk(uint32_t sessionId, uint32_t handle, uint32_t pAddrL2,
                                   uint32_t offsetPayload, uint32_t lenBulkMem, uint32_t *secureVirtualAdr)
{
    TrustletSession *ts = getTrustletSession(sessionId);
    if (ts == NULL) {
        LOG_E("no session found with id=%d", sessionId);
        return MC_DRV_ERR_DAEMON_UNKNOWN_SESSION;
    }

    // TODO-2012-09-06-haenellu: Think about not ignoring the error case, ClientLib does not allow this.
    ts->addBulkBuff(new CWsm((void *)offsetPayload, lenBulkMem, handle, (void *)pAddrL2));
    // Write MCP map message to buffer
    mcpMessage->cmdMap.cmdHeader.cmdId = MC_MCP_CMD_MAP;
    mcpMessage->cmdMap.sessionId = sessionId;
    mcpMessage->cmdMap.wsmType = WSM_L2;
    mcpMessage->cmdMap.adrBuffer = (uint32_t)(pAddrL2);
    mcpMessage->cmdMap.ofsBuffer = offsetPayload;
    mcpMessage->cmdMap.lenBuffer = lenBulkMem;

    // Notify MC about the availability of a new command inside the MCP buffer
    notify(SID_MCP);

    // Wait till response from MC is available
    if (!waitMcpNotification()) {
        return MC_DRV_ERR_DAEMON_MCI_ERROR;
    }

    // Check if the command response ID is correct
    if (mcpMessage->rspHeader.rspId != (MC_MCP_CMD_MAP | FLAG_RESPONSE)) {
        LOG_E("CMD_MAP got invalid MCP response");
        return MC_DRV_ERR_DAEMON_MCI_ERROR;
    }

    uint32_t mcRet = mcpMessage->rspMap.rspHeader.result;

    if (mcRet != MC_MCP_RET_OK) {
        LOG_E("MCP MAP returned code %d.", mcRet);
        return MAKE_MC_DRV_MCP_ERROR(mcRet);
    }

    *secureVirtualAdr = mcpMessage->rspMap.secureVirtualAdr;
    return MC_DRV_OK;
}


//------------------------------------------------------------------------------
mcResult_t MobiCoreDevice::unmapBulk(uint32_t sessionId, uint32_t handle,
                                     uint32_t secureVirtualAdr, uint32_t lenBulkMem)
{
    TrustletSession *ts = getTrustletSession(sessionId);
    if (ts == NULL) {
        LOG_E("no session found with id=%d", sessionId);
        return MC_DRV_ERR_DAEMON_UNKNOWN_SESSION;
    }

    // Write MCP unmap command to buffer
    mcpMessage->cmdUnmap.cmdHeader.cmdId = MC_MCP_CMD_UNMAP;
    mcpMessage->cmdUnmap.sessionId = sessionId;
    mcpMessage->cmdUnmap.wsmType = WSM_L2;
    mcpMessage->cmdUnmap.secureVirtualAdr = secureVirtualAdr;
    mcpMessage->cmdUnmap.lenVirtualBuffer = lenBulkMem;

    // Notify MC about the availability of a new command inside the MCP buffer
    notify(SID_MCP);

    // Wait till response from MC is available
    if (!waitMcpNotification()) {
        return MC_DRV_ERR_DAEMON_MCI_ERROR;
    }

    // Check if the command response ID is correct
    if (mcpMessage->rspHeader.rspId != (MC_MCP_CMD_UNMAP | FLAG_RESPONSE)) {
        LOG_E("CMD_OPEN_SESSION got invalid MCP response");
        return MC_DRV_ERR_DAEMON_MCI_ERROR;
    }

    uint32_t mcRet = mcpMessage->rspUnmap.rspHeader.result;

    if (mcRet != MC_MCP_RET_OK) {
        LOG_E("MCP UNMAP returned code %d.", mcRet);
        return MAKE_MC_DRV_MCP_ERROR(mcRet);
    } else {
        // Just remove the buffer
        // TODO-2012-09-06-haenellu: Haven't we removed it already?
        if (!ts->removeBulkBuff(handle))
            LOG_I("unmapBulk(): no buffer found found with handle=%u", handle);
    }

    return MC_DRV_OK;
}


//------------------------------------------------------------------------------
void MobiCoreDevice::donateRam(const uint32_t donationSize)
{
    // Donate additional RAM to the MobiCore
    CWsm_ptr ram = allocateContiguousPersistentWsm(donationSize);
    if (NULL == ram) {
        LOG_E("Allocation of additional RAM failed");
        return;
    }
    ramType_t ramType = RAM_GENERIC;
    addr_t adrBuffer = ram->physAddr;
    const uint32_t numPages = donationSize / (4 * 1024);


    LOG_I("donateRam(): adrBuffer=%p, numPages=%d, ramType=%d",
          adrBuffer,
          numPages,
          ramType);

    do {
        // Write MCP open message to buffer
        mcpMessage->cmdDonateRam.cmdHeader.cmdId = MC_MCP_CMD_DONATE_RAM;
        mcpMessage->cmdDonateRam.adrBuffer = (uint32_t) adrBuffer;
        mcpMessage->cmdDonateRam.numPages = numPages;
        mcpMessage->cmdDonateRam.ramType = ramType;

        // Notify MC about a new command inside the MCP buffer
        notify(SID_MCP);

        // Wait till response from MC is available
        if (!waitMcpNotification()) {
            break;
        }

        // Check if the command response ID is correct
        if ((MC_MCP_CMD_DONATE_RAM | FLAG_RESPONSE) != mcpMessage->rspHeader.rspId) {
            LOG_E("donateRam(): CMD_DONATE_RAM got invalid MCP response - rspId is: %d",
                  mcpMessage->rspHeader.rspId);
            break;
        }

        uint32_t mcRet = mcpMessage->rspDonateRam.rspHeader.result;
        if (MC_MCP_RET_OK != mcRet) {
            LOG_E("donateRam(): CMD_DONATE_RAM error %d", mcRet);
            break;
        }

        LOG_I("donateRam() succeeded.");

    } while (0);
}

//------------------------------------------------------------------------------
mcResult_t MobiCoreDevice::getMobiCoreVersion(
    mcDrvRspGetMobiCoreVersionPayload_ptr pRspGetMobiCoreVersionPayload
)
{
    // If MobiCore version info already fetched.
    if (mcVersionInfo != NULL) {
        pRspGetMobiCoreVersionPayload->versionInfo = *mcVersionInfo;
        return MC_DRV_OK;
        // Otherwise, fetch it via MCP.
    } else {
        // Write MCP unmap command to buffer
        mcpMessage->cmdGetMobiCoreVersion.cmdHeader.cmdId = MC_MCP_CMD_GET_MOBICORE_VERSION;

        // Notify MC about the availability of a new command inside the MCP buffer
        notify(SID_MCP);

        // Wait till response from MC is available
        if (!waitMcpNotification()) {
            return MC_DRV_ERR_DAEMON_MCI_ERROR;
        }

        // Check if the command response ID is correct
        if ((MC_MCP_CMD_GET_MOBICORE_VERSION | FLAG_RESPONSE) != mcpMessage->rspHeader.rspId) {
            LOG_E("MC_MCP_CMD_GET_MOBICORE_VERSION got invalid MCP response");
            return MC_DRV_ERR_DAEMON_MCI_ERROR;
        }

        uint32_t  mcRet = mcpMessage->rspGetMobiCoreVersion.rspHeader.result;

        if (mcRet != MC_MCP_RET_OK) {
            LOG_E("MC_MCP_CMD_GET_MOBICORE_VERSION error %d", mcRet);
            return MAKE_MC_DRV_MCP_ERROR(mcRet);
        }

        pRspGetMobiCoreVersionPayload->versionInfo = mcpMessage->rspGetMobiCoreVersion.versionInfo;

        // Store MobiCore info for future reference.
        mcVersionInfo = new mcVersionInfo_t();
        *mcVersionInfo = pRspGetMobiCoreVersionPayload->versionInfo;
        return MC_DRV_OK;
    }
}

//------------------------------------------------------------------------------
void MobiCoreDevice::queueUnknownNotification(
    notification_t notification
)
{
    notifications.push(notification);
}

/** @} */
