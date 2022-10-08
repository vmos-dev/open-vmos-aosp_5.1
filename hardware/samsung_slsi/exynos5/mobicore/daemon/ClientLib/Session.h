/** @addtogroup MCD_IMPL_LIB
 * @{
 * @file
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
#ifndef SESSION_H_
#define SESSION_H_

#include <stdint.h>
#include <list>

#include "mc_linux.h"
#include "Connection.h"
#include "CMcKMod.h"
#include "CMutex.h"


class BulkBufferDescriptor
{
public:
    addr_t    virtAddr; /**< The virtual address of the Bulk buffer*/
    addr_t    sVirtualAddr; /**< The secure virtual address of the Bulk buffer*/
    uint32_t  len; /**< Length of the Bulk buffer*/
    uint32_t  handle;
    addr_t    physAddrWsmL2; /**< The physical address of the L2 table of the Bulk buffer*/

    BulkBufferDescriptor(
        addr_t    virtAddr,
        addr_t    sVirtAddr,
        uint32_t  len,
        uint32_t  handle,
        addr_t    physAddrWsmL2
    ) :
        virtAddr(virtAddr),
        sVirtualAddr(sVirtAddr),
        len(len),
        handle(handle),
        physAddrWsmL2(physAddrWsmL2)
    {};

};

typedef std::list<BulkBufferDescriptor *>  bulkBufferDescrList_t;
typedef bulkBufferDescrList_t::iterator   bulkBufferDescrIterator_t;


/** Session states.
 * At the moment not used !!.
 */
typedef enum {
    SESSION_STATE_INITIAL,
    SESSION_STATE_OPEN,
    SESSION_STATE_TRUSTLET_DEAD
} sessionState_t;

#define SESSION_ERR_NO      0 /**< No session error */

/** Session information structure.
 * The information structure is used to hold the state of the session, which will limit further actions for the session.
 * Also the last error code will be stored till it's read.
 */
typedef struct {
    sessionState_t state;       /**< Session state */
    int32_t        lastErr;     /**< Last error of session */
} sessionInformation_t;


class Session
{
private:
    CMcKMod *mcKMod;
    CMutex workLock;
    bulkBufferDescrList_t bulkBufferDescriptors; /**< Descriptors of additional bulk buffer of a session */
    sessionInformation_t sessionInfo; /**< Informations about session */
public:
    uint32_t sessionId;
    Connection *notificationConnection;

    Session(uint32_t sessionId, CMcKMod *mcKMod, Connection *connection);

    virtual ~Session(void);

    /**
     * Add address information of additional bulk buffer memory to session and
     * register virtual memory in kernel module.
     *
     * @attention The virtual address can only be added one time. If the virtual address already exist, MC_DRV_ERR_BUFFER_ALREADY_MAPPED is returned.
     *
     * @param buf The virtual address of bulk buffer.
     * @param len Length of bulk buffer.
     * @param blkBuf pointer of the actual Bulk buffer descriptor with all address information.
     *
     * @return MC_DRV_OK on success
     * @return MC_DRV_ERR_BUFFER_ALREADY_MAPPED
     */
    mcResult_t addBulkBuf(addr_t buf, uint32_t len, BulkBufferDescriptor **blkBuf);

    /**
     * Remove address information of additional bulk buffer memory from session and
     * unregister virtual memory in kernel module
     *
     * @param buf The virtual address of the bulk buffer.
     *
     * @return true on success.
     */
    mcResult_t removeBulkBuf(addr_t buf);

    /**
     * Return the Kmod handle of the bulk buff
     *
     * @param buf The secure virtual address of the bulk buffer.
     *
     * @return the Handle or 0 for failure
     */
    uint32_t getBufHandle(addr_t sVirtualAddr);

    /**
     * Set additional error information of the last error that occured.
     *
     * @param errorCode The actual error.
     */
    void setErrorInfo(int32_t err);

    /**
     * Get additional error information of the last error that occured.
     *
     * @attention After request the information is set to SESSION_ERR_NO.
     *
     * @return Last stored error code or SESSION_ERR_NO.
     */
    int32_t getLastErr(void);

    /**
     * Lock session for operation
     */
    void lock() {
        workLock.lock();
    }

    /**
     * Unlock session for operation
     */
    void unlock()  {
        workLock.unlock();
    }
};

typedef std::list<Session *>            sessionList_t;
typedef sessionList_t::iterator        sessionIterator_t;

#endif /* SESSION_H_ */

/** @} */
