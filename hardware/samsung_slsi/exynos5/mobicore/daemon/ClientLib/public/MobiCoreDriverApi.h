/**
 * @defgroup MCD_API                    MobiCore Driver API
 * @addtogroup MCD_API
 * @{
 *
 * @if DOXYGEN_MCDRV_API
 *   @mainpage MobiCore Driver API.
 * @endif
 *
 * MobiCore Driver API.
 *
 * The MobiCore (MC) Driver API provides access functions to the MobiCore runtime environment and the contained Trustlets.
 *
 * @image html DoxyOverviewDrvApi500x.png
 * @image latex DoxyOverviewDrvApi500x.png "MobiCore Overview" width=12cm
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
#ifndef MCDRIVER_H_
#define MCDRIVER_H_

#if (!defined(__MC_CLIENT_LIB_API)) && __cplusplus
#define __MC_CLIENT_LIB_API       extern "C"
#else
#define __MC_CLIENT_LIB_API
#endif // __cplusplus


#include <stdint.h>
#include <stdbool.h>

#include "mcUuid.h"
#include "mcVersionInfo.h"

/*
 *  MobiCore driver API error codes.
 *  MAJOR part of error code is stable.
 *  MCP part contains MCP result code. See MCI/mcimcp.h
 *  DETAIL part may be used in testing for specific error code.
 *
 *  Detail error codes may change in different releases
 *  Please do not test DETAIL part when comparing error codes.
 */
#define MC_DRV_ERROR_MAJOR(ecode)    ((ecode)       & 0xFF)     /**< Get MAJOR part of error code. */
#define MC_DRV_ERROR_MCP(ecode)     (((ecode)>>8)   & 0xFF)     /**< Get MCP part of error code. */
#define MC_DRV_ERROR_DETAIL(ecode)  (((ecode)>>16)  & 0x0FFF)   /**< Get detail part of error code. */

typedef uint32_t mcResult_t;
/**
 * Return values of MobiCore driver functions.
 */
#define MC_DRV_OK                                   0x00000000 /**< Function call succeeded. */
#define MC_DRV_NO_NOTIFICATION                      0x00000001 /**< No notification available. */
#define MC_DRV_ERR_NOTIFICATION                     0x00000002 /**< Error during notification on communication level. */
#define MC_DRV_ERR_NOT_IMPLEMENTED                  0x00000003 /**< Function not implemented. */
#define MC_DRV_ERR_OUT_OF_RESOURCES                 0x00000004 /**< No more resources available. */
#define MC_DRV_ERR_INIT                             0x00000005 /**< Driver initialization failed. */
#define MC_DRV_ERR_UNKNOWN                          0x00000006 /**< Unknown error. */
#define MC_DRV_ERR_UNKNOWN_DEVICE                   0x00000007 /**< The specified device is unknown. */
#define MC_DRV_ERR_UNKNOWN_SESSION                  0x00000008 /**< The specified session is unknown. */
#define MC_DRV_ERR_INVALID_OPERATION                0x00000009 /**< The specified operation is not allowed. */
#define MC_DRV_ERR_INVALID_RESPONSE                 0x0000000a /**< The response header from the MC is invalid. */
#define MC_DRV_ERR_TIMEOUT                          0x0000000b /**< Function call timed out. */
#define MC_DRV_ERR_NO_FREE_MEMORY                   0x0000000c /**< Can not allocate additional memory. */
#define MC_DRV_ERR_FREE_MEMORY_FAILED               0x0000000d /**< Free memory failed. */
#define MC_DRV_ERR_SESSION_PENDING                  0x0000000e /**< Still some open sessions pending. */
#define MC_DRV_ERR_DAEMON_UNREACHABLE               0x0000000f /**< MC daemon not reachable */
#define MC_DRV_ERR_INVALID_DEVICE_FILE              0x00000010 /**< The device file of the kernel module could not be opened. */
#define MC_DRV_ERR_INVALID_PARAMETER                0x00000011 /**< Invalid parameter. */
#define MC_DRV_ERR_KERNEL_MODULE                    0x00000012 /**< Error from Kernel Module, see DETAIL for errno. */
#define MC_DRV_ERR_BULK_MAPPING                     0x00000013 /**< Error during mapping of additional bulk memory to session. */
#define MC_DRV_ERR_BULK_UNMAPPING                   0x00000014 /**< Error during unmapping of additional bulk memory to session. */
#define MC_DRV_INFO_NOTIFICATION                    0x00000015 /**< Notification received, exit code available. */
#define MC_DRV_ERR_NQ_FAILED                        0x00000016 /**< Set up of NWd connection failed. */

#define MC_DRV_ERR_DAEMON_VERSION                   0x00000017 /**< Wrong daemon version. */
#define MC_DRV_ERR_CONTAINER_VERSION                0x00000018 /**< Wrong container version. */

// those should become MCP or even detail codes on top of MC_DRV_ERR_MCP_ERROR
#define MC_DRV_ERR_WRONG_PUBLIC_KEY                 0x00000019 /**< System Trustlet public key is wrong. */
#define MC_DRV_ERR_CONTAINER_TYPE_MISMATCH          0x0000001a /**< Wrong containter type(s). */
#define MC_DRV_ERR_CONTAINER_LOCKED                 0x0000001b /**< Container is locked (or not activated). */
#define MC_DRV_ERR_SP_NO_CHILD                      0x0000001c /**< SPID is not registered with root container. */
#define MC_DRV_ERR_TL_NO_CHILD                      0x0000001d /**< UUID is not registered with sp container. */
#define MC_DRV_ERR_UNWRAP_ROOT_FAILED               0x0000001e /**< Unwrapping of root container failed. */
#define MC_DRV_ERR_UNWRAP_SP_FAILED                 0x0000001f /**< Unwrapping of service provider container failed. */
#define MC_DRV_ERR_UNWRAP_TRUSTLET_FAILED           0x00000020 /**< Unwrapping of Trustlet container failed. */

// use separate numbers for those in the future
#define MC_DRV_ERR_DEVICE_ALREADY_OPEN              MC_DRV_ERR_INVALID_OPERATION /** < Device is already open. */
#define MC_DRV_ERR_SOCKET_CONNECT                   MC_DRV_ERR_DAEMON_UNREACHABLE /**< MC daemon socket not reachable. */
#define MC_DRV_ERR_SOCKET_WRITE                     MC_DRV_ERR_DAEMON_UNREACHABLE /**< MC daemon socket write error. */
#define MC_DRV_ERR_SOCKET_READ                      MC_DRV_ERR_DAEMON_UNREACHABLE /**< MC daemon socket read error. */
#define MC_DRV_ERR_SOCKET_LENGTH                    MC_DRV_ERR_DAEMON_UNREACHABLE /**< MC daemon socket read error. */
#define MC_DRV_ERR_DAEMON_SOCKET                    MC_DRV_ERR_DAEMON_UNREACHABLE /**< MC daemon had problems with socket. */
#define MC_DRV_ERR_DEVICE_FILE_OPEN                 MC_DRV_ERR_INVALID_DEVICE_FILE /**< The device file of the kernel module could not be opened. */
#define MC_DRV_ERR_NULL_POINTER                     MC_DRV_ERR_INVALID_PARAMETER /**< Null pointer passed as parameter. */
#define MC_DRV_ERR_TCI_TOO_BIG                      MC_DRV_ERR_INVALID_PARAMETER /**< Requested TCI length is too high. */
#define MC_DRV_ERR_WSM_NOT_FOUND                    MC_DRV_ERR_INVALID_PARAMETER /**< Requested TCI was not allocated with mallocWsm(). */
#define MC_DRV_ERR_TCI_GREATER_THAN_WSM             MC_DRV_ERR_INVALID_PARAMETER /**< Requested TCI length is bigger than allocated WSM. */
#define MC_DRV_ERR_TRUSTLET_NOT_FOUND               MC_DRV_ERR_INVALID_DEVICE_FILE /** < Trustlet could not be found in mcRegistry. */
#define MC_DRV_ERR_DAEMON_KMOD_ERROR                MC_DRV_ERR_DAEMON_UNREACHABLE /**< Daemon cannot use Kernel module as expected. */
#define MC_DRV_ERR_DAEMON_MCI_ERROR                 MC_DRV_ERR_DAEMON_UNREACHABLE /**< Daemon cannot use MCI as expected. */
#define MC_DRV_ERR_MCP_ERROR                        MC_DRV_ERR_DAEMON_UNREACHABLE /**< MobiCore Control Protocol error. See MC_DRV_ERROR_MCP(). */
#define MC_DRV_ERR_INVALID_LENGTH                   MC_DRV_ERR_NO_FREE_MEMORY /**< Invalid length. */
#define MC_DRV_ERR_KMOD_NOT_OPEN                    MC_DRV_ERR_NO_FREE_MEMORY /**< Device not open. */
#define MC_DRV_ERR_BUFFER_ALREADY_MAPPED            MC_DRV_ERR_BULK_MAPPING /**< Buffer is already mapped to this Trustlet. */
#define MC_DRV_ERR_BLK_BUFF_NOT_FOUND               MC_DRV_ERR_BULK_UNMAPPING /**< Unable to find internal handle for buffer. */

#define MC_DRV_ERR_DAEMON_DEVICE_NOT_OPEN           0x00000021 /**< No device associated with connection. */
#define MC_DRV_ERR_DAEMON_WSM_HANDLE_NOT_FOUND      MC_DRV_ERR_WSM_NOT_FOUND /**< Daemon could not find WSM handle. */
#define MC_DRV_ERR_DAEMON_UNKNOWN_SESSION           MC_DRV_ERR_UNKNOWN_SESSION /**< The specified session is unknown to Daemon. */

#define MAKE_MC_DRV_MCP_ERROR(mcpCode)              (MC_DRV_ERR_MCP_ERROR | ((mcpCode&0x000FFFFF)<<8))
#define MAKE_MC_DRV_KMOD_WITH_ERRNO(theErrno)       (MC_DRV_ERR_KERNEL_MODULE| (((theErrno)&0x0000FFFF)<<16))

/**
 * Driver control command.
 */
typedef enum {
    MC_CTRL_DUMMY = 1 /**< Dummy. */
} mcDriverCtrl_t;


/** Structure of Session Handle, includes the Session ID and the Device ID the Session belongs to.
 * The session handle will be used for session-based MobiCore communication.
 * It will be passed to calls which address a communication end point in the MobiCore environment.
 */
typedef struct {
    uint32_t sessionId; /**< MobiCore session ID */
    uint32_t deviceId; /**< Device ID the session belongs to */
} mcSessionHandle_t;

/** Information structure about additional mapped Bulk buffer between the Trustlet Connector (Nwd) and
 * the Trustlet (Swd). This structure is initialized from a Trustlet Connector by calling mcMap().
 * In order to use the memory within a Trustlet the Trustlet Connector has to inform the Trustlet with
 * the content of this structure via the TCI.
 */
typedef struct {
    void *sVirtualAddr;         /**< The virtual address of the Bulk buffer regarding the address space of the Trustlet, already includes a possible offset! */
    uint32_t sVirtualLen;       /**< Length of the mapped Bulk buffer */
} mcBulkMap_t;


#define MC_DEVICE_ID_DEFAULT       0 /**< The default device ID */
#define MC_INFINITE_TIMEOUT        ((int32_t)(-1)) /**< Wait infinite for a response of the MC. */
#define MC_NO_TIMEOUT              0   /**< Do not wait for a response of the MC. */
#define MC_MAX_TCI_LEN             0x100000 /**< TCI/DCI must not exceed 1MiB */

/* Mark only the following functions for export */
#pragma GCC visibility push(default)

/** Open a new connection to a MobiCore device.
 *
 * mcOpenDevice() initializes all device specific resources required to communicate
 * with an MobiCore instance located on the specified device in the system. If the device
 * does not exist the function will return MC_DRV_ERR_UNKNOWN_DEVICE.
 *
 * @param [in] deviceId Identifier for the MobiCore device to be used. MC_DEVICE_ID_DEFAULT refers to the default device.
 *
 * @return MC_DRV_OK if operation has been successfully completed.
 * @return MC_DRV_ERR_INVALID_OPERATION if device already opened.
 * @return MC_DRV_ERR_DAEMON_UNREACHABLE when problems with daemon occur.
 * @return MC_DRV_ERR_UNKNOWN_DEVICE when deviceId is unknown.
 * @return MC_DRV_ERR_INVALID_DEVICE_FILE if kernel module under /dev/mobicore cannot be opened
 *
 * Uses a Mutex.
 */
__MC_CLIENT_LIB_API mcResult_t mcOpenDevice(
    uint32_t deviceId
);

/** Close the connection to a MobiCore device.
 * When closing a device, active sessions have to be closed beforehand.
 * Resources associated with the device will be released.
 * The device may be opened again after it has been closed.
 *
 * @param [in] deviceId Identifier for the MobiCore device. MC_DEVICE_ID_DEFAULT refers to the default device.
 *
 * @return MC_DRV_OK if operation has been successfully completed.
 * @return MC_DRV_ERR_UNKNOWN_DEVICE when device id is invalid.
 * @return MC_DRV_ERR_SESSION_PENDING when a session is still open.
 * @return MC_DRV_ERR_DAEMON_UNREACHABLE when problems with daemon occur.
 *
 * Uses a Mutex.
 */
__MC_CLIENT_LIB_API mcResult_t mcCloseDevice(
    uint32_t deviceId
);

/** Open a new session to a Trustlet. The trustlet with the given UUID has to be available in the flash filesystem.
 *
 * Write MCP open message to buffer and notify MobiCore about the availability of a new command.
 * Waits till the MobiCore responses with the new session ID (stored in the MCP buffer).
 *
 * @param [in,out] session On success, the session data will be returned. Note that session.deviceId has to be the device id of an opened device.
 * @param [in] uuid UUID of the Trustlet to be opened.
 * @param [in] tci TCI buffer for communicating with the trustlet.
 * @param [in] tciLen Length of the TCI buffer. Maximum allowed value is MC_MAX_TCI_LEN.
 *
 * @return MC_DRV_OK if operation has been successfully completed.
 * @return MC_DRV_INVALID_PARAMETER if session parameter is invalid.
 * @return MC_DRV_ERR_UNKNOWN_DEVICE when device id is invalid.
 * @return MC_DRV_ERR_DAEMON_UNREACHABLE when problems with daemon socket occur.
 * @return MC_DRV_ERR_UNKNOWN_DEVICE when daemon returns an error.
 *
 * Uses a Mutex.
 */
__MC_CLIENT_LIB_API mcResult_t mcOpenSession(
    mcSessionHandle_t  *session,
    const mcUuid_t       *uuid,
    uint8_t            *tci,
    uint32_t           tciLen
);

/** Close a Trustlet session.
 *
 * Closes the specified MobiCore session. The call will block until the session has been closed.
 *
 * @pre Device deviceId has to be opened in advance.
 *
 * @param [in] session Session to be closed.
 *
 * @return MC_DRV_OK if operation has been successfully completed.
 * @return MC_DRV_INVALID_PARAMETER if session parameter is invalid.
 * @return MC_DRV_ERR_UNKNOWN_SESSION when session id is invalid.
 * @return MC_DRV_ERR_UNKNOWN_DEVICE when device id of session is invalid.
 * @return MC_DRV_ERR_DAEMON_UNREACHABLE when problems with daemon occur.
 * @return MC_DRV_ERR_INVALID_DEVICE_FILE when daemon cannot open trustlet file.
 *
 * Uses a Mutex.
 */
__MC_CLIENT_LIB_API mcResult_t mcCloseSession(
    mcSessionHandle_t *session
);

/** Notify a session.
 * Notifies the session end point about available message data.
 * If the session parameter is correct, notify will always succeed.
 * Corresponding errors can only be received by mcWaitNotification().
 * @pre A session has to be opened in advance.
 *
 * @param session The session to be notified.
 *
 * @return MC_DRV_OK if operation has been successfully completed.
 * @return MC_DRV_INVALID_PARAMETER if session parameter is invalid.
 * @return MC_DRV_ERR_UNKNOWN_SESSION when session id is invalid.
 * @return MC_DRV_ERR_UNKNOWN_DEVICE when device id of session is invalid.
 */
__MC_CLIENT_LIB_API mcResult_t mcNotify(
    mcSessionHandle_t *session
);

/** Wait for a notification.
 *
 * Wait for a notification issued by the MobiCore for a specific session.
 * The timeout parameter specifies the number of milliseconds the call will wait for a notification.
 * If the caller passes 0 as timeout value the call will immediately return. If timeout value is below 0 the call will block
 * until a notification for the session has been received.
 *
 * @attention if timeout is below 0, call will block:
 * Caller has to trust the other side to send a notification to wake him up again.
 *
 * @param [in] session The session the notification should correspond to.
 * @param [in] timeout Time in milliseconds to wait (MC_NO_TIMEOUT : direct return, > 0 : milliseconds, MC_INFINITE_TIMEOUT : wait infinitely)
 *
 * @return MC_DRV_OK if notification is available.
 * @return MC_DRV_ERR_TIMEOUT if no notification arrived in time.
 * @return MC_DRV_INFO_NOTIFICATION if a problem with the session was encountered. Get more details with mcGetSessionErrorCode().
 * @return MC_DRV_ERR_NOTIFICATION if a problem with the socket occurred.
 * @return MC_DRV_INVALID_PARAMETER if a parameter is invalid.
 * @return MC_DRV_ERR_UNKNOWN_SESSION when session id is invalid.
 * @return MC_DRV_ERR_UNKNOWN_DEVICE when device id of session is invalid.
 */
__MC_CLIENT_LIB_API mcResult_t mcWaitNotification(
    mcSessionHandle_t  *session,
    int32_t            timeout
);

/**
 * Allocate a block of world shared memory (WSM).
 * The MC driver allocates a contiguous block of memory which can be used as WSM.
 * This implicates that the allocated memory is aligned according to the alignment parameter.
 * Always returns a buffer of size WSM_SIZE aligned to 4K.
 *
 * @param [in]  deviceId The ID of an opened device to retrieve the WSM from.
 * @param [in]  align The alignment (number of pages) of the memory block (e.g. 0x00000001 for 4kb).
 * @param [in]  len Length of the block in bytes.
 * @param [out] wsm Virtual address of the world shared memory block.
 * @param [in]  wsmFlags Platform specific flags describing the memory to be allocated.
 *
 * @attention: align and wsmFlags are currently ignored
 *
 * @return MC_DRV_OK if operation has been successfully completed.
 * @return MC_DRV_INVALID_PARAMETER if a parameter is invalid.
 * @return MC_DRV_ERR_UNKNOWN_DEVICE when device id is invalid.
 * @return MC_DRV_ERR_NO_FREE_MEMORY if no more contiguous memory is available in this size or for this process.
 *
 * Uses a Mutex.
 */
__MC_CLIENT_LIB_API mcResult_t mcMallocWsm(
    uint32_t  deviceId,
    uint32_t  align,
    uint32_t  len,
    uint8_t   **wsm,
    uint32_t  wsmFlags
);

/**
 * Free a block of world shared memory (WSM).
 * The MC driver will free a block of world shared memory (WSM) previously allocated with
 * mcMallocWsm(). The caller has to assure that the address handed over to the driver
 * is a valid WSM address.
 *
 * @param [in] deviceId The ID to which the given address belongs.
 * @param [in] wsm Address of WSM block to be freed.
 *
 * @return MC_DRV_OK if operation has been successfully completed.
 * @return MC_DRV_INVALID_PARAMETER if a parameter is invalid.
 * @return MC_DRV_ERR_UNKNOWN_DEVICE when device id is invalid.
 * @return MC_DRV_ERR_FREE_MEMORY_FAILED on failures.
 *
 * Uses a Mutex.
 */
__MC_CLIENT_LIB_API mcResult_t mcFreeWsm(
    uint32_t  deviceId,
    uint8_t   *wsm
);

/**
 * Map additional bulk buffer between a Trustlet Connector (TLC) and the Trustlet (TL) for a session.
 * Memory allocated in user space of the TLC can be mapped as additional communication channel
 * (besides TCI) to the Trustlet. Limitation of the Trustlet memory structure apply: only 6 chunks can be mapped
 * with a maximum chunk size of 1 MiB each.
 *
 * @attention It is up to the application layer (TLC) to inform the Trustlet about the additional mapped bulk memory.
 *
 * @param [in] session Session handle with information of the deviceId and the sessionId. The
 * given buffer is mapped to the session specified in the sessionHandle.
 * @param [in] buf Virtual address of a memory portion (relative to TLC) to be shared with the Trustlet, already includes a possible offset!
 * @param [in] len length of buffer block in bytes.
 * @param [out] mapInfo Information structure about the mapped Bulk buffer between the TLC (Nwd) and
 * the TL (Swd).
 *
 * @return MC_DRV_OK if operation has been successfully completed.
 * @return MC_DRV_INVALID_PARAMETER if a parameter is invalid.
 * @return MC_DRV_ERR_UNKNOWN_SESSION when session id is invalid.
 * @return MC_DRV_ERR_UNKNOWN_DEVICE when device id of session is invalid.
 * @return MC_DRV_ERR_DAEMON_UNREACHABLE when problems with daemon occur.
 * @return MC_DRV_ERR_BULK_MAPPING when buf is already uses as bulk buffer or when registering the buffer failed.
 *
 * Uses a Mutex.
 */
__MC_CLIENT_LIB_API mcResult_t mcMap(
    mcSessionHandle_t  *session,
    void               *buf,
    uint32_t           len,
    mcBulkMap_t        *mapInfo
);

/**
 * Remove additional mapped bulk buffer between Trustlet Connector (TLC) and the Trustlet (TL) for a session.
 *
 * @attention The bulk buffer will immediately be unmapped from the session context.
 * @attention The application layer (TLC) must inform the TL about unmapping of the additional bulk memory before calling mcUnmap!
 *
 * @param [in] session Session handle with information of the deviceId and the sessionId. The
 * given buffer is unmapped from the session specified in the sessionHandle.
 * @param [in] buf Virtual address of a memory portion (relative to TLC) shared with the TL, already includes a possible offset!
 * @param [in] mapInfo Information structure about the mapped Bulk buffer between the TLC (Nwd) and
 * the TL (Swd).
 * @attention The clientlib currently ignores the len field in mapInfo.
 *
 * @return MC_DRV_OK if operation has been successfully completed.
 * @return MC_DRV_INVALID_PARAMETER if a parameter is invalid.
 * @return MC_DRV_ERR_UNKNOWN_SESSION when session id is invalid.
 * @return MC_DRV_ERR_UNKNOWN_DEVICE when device id of session is invalid.
 * @return MC_DRV_ERR_DAEMON_UNREACHABLE when problems with daemon occur.
 * @return MC_DRV_ERR_BULK_UNMAPPING when buf was not registered earlier or when unregistering failed.
 *
 * Uses a Mutex.
 */
__MC_CLIENT_LIB_API mcResult_t mcUnmap(
    mcSessionHandle_t  *session,
    void               *buf,
    mcBulkMap_t        *mapInfo
);


/**
 * @attention: Not implemented.
 * Execute driver specific command.
 * mcDriverCtrl() can be used to execute driver specific commands.
 * Besides the control command MC_CTRL_GET_VERSION commands are implementation specific.
 * Please refer to the corresponding specification of the driver manufacturer.
 *
 * @param [in] param Command ID of the command to be executed.
 * @param [in, out] data  Command data and response depending on command.
 * @param [in] len Length of the data block.
 *
 * @return MC_DRV_ERR_NOT_IMPLEMENTED.
 */
__MC_CLIENT_LIB_API mcResult_t mcDriverCtrl(
    mcDriverCtrl_t  param,
    uint8_t         *data,
    uint32_t        len
);

/**
 * Get additional error information of the last error that occured on a session.
 * After the request the stored error code will be deleted.
 *
 * @param [in] session Session handle with information of the deviceId and the sessionId.
 * @param [out] lastErr >0 Trustlet has terminated itself with this value, <0 Trustlet is dead because of an error within the MobiCore (e.g. Kernel exception).
 * See also notificationPayload_t enum in MCI definition at "mcinq.h".
 *
 * @return MC_DRV_OK if operation has been successfully completed.
 * @return MC_DRV_INVALID_PARAMETER if a parameter is invalid.
 * @return MC_DRV_ERR_UNKNOWN_SESSION when session id is invalid.
 * @return MC_DRV_ERR_UNKNOWN_DEVICE when device id of session is invalid.
 */
__MC_CLIENT_LIB_API mcResult_t mcGetSessionErrorCode(
    mcSessionHandle_t  *session,
    int32_t            *lastErr
);

/**
 * Get MobiCore version information of a device.
 *
 * @param [in] deviceId of an open device.
 * @param [out] versionInfo MobiCore version info.
 *
 * @return MC_DRV_OK if operation has been successfully completed.
 * @return MC_DRV_ERR_UNKNOWN_DEVICE when device is not open.
 * @return MC_DRV_INVALID_PARAMETER if a parameter is invalid.
 * @return MC_DRV_ERR_DAEMON_UNREACHABLE when problems with daemon occur.
 */
__MC_CLIENT_LIB_API mcResult_t mcGetMobiCoreVersion(
    uint32_t  deviceId,
    mcVersionInfo_t *versionInfo
);
#pragma GCC visibility pop
#endif /** MCDRIVER_H_ */

/** @} */
