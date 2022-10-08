/** @addtogroup MCD_MCDIMPL_DAEMON_KERNEL
 * @{
 * @file
 *
 * MobiCore Driver Kernel Module Interface.
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
#ifndef CMCKMOD_H_
#define CMCKMOD_H_

#include <stdint.h>

#include "McTypes.h"
#include "CKMod.h"


/**
 * As this is also used by the ClientLib, we do not use exceptions.
 */
class CMcKMod : public CKMod
{
public:
    /**
    * Map data.
    *
    * @param len
    * @param pHandle
    * @param pVirtAddr
    * @param pPhysAddr
    *
    * @return 0 if all went fine
    * @return MC_DRV_ERR_KMOD_NOT_OPEN
    * @return MC_DRV_ERR_KERNEL_MODULE or'ed with errno<<16
    */
    mcResult_t mapWsm(uint32_t  len,
                      uint32_t    *pHandle,
                      addr_t      *pVirtAddr,
                      addr_t      *pPhysAddr);
    /**
    * Map data.
    *
    * @param len
    * @param pHandle
    * @param pVirtAddr
    * @param pPhysAddr
    * @param pMciReuse [in|out] set to true [in] for reusing MCI buffer
    *                 is set to true [out] if MCI buffer has been reused
    * @return 0 if all went fine
    * @return MC_DRV_ERR_KMOD_NOT_OPEN
    * @return MC_DRV_ERR_KERNEL_MODULE or'ed with errno<<16
    */
    mcResult_t mapMCI(
        uint32_t    len,
        uint32_t    *pHandle,
        addr_t      *pVirtAddr,
        addr_t      *pPhysAddr,
        bool        *pReuse);

    /**
    * Map persistent WSM which will not be freed up once the calling process dies.
    */
    mcResult_t mapPersistent(
        uint32_t    len,
        uint32_t    *pHandle,
        addr_t      *pVirtAddr,
        addr_t      *pPhysAddr);

    int read(addr_t buffer, uint32_t len);

    bool waitSSIQ(uint32_t *pCnt);

    int fcInit(uint32_t nqOffset,
               uint32_t    nqLength,
               uint32_t    mcpOffset,
               uint32_t    mcpLength);

    int fcInfo(
        uint32_t    extInfoId,
        uint32_t    *pState,
        uint32_t    *pExtInfo);

    int fcYield(void);

    int fcNSIQ(void);

    mcResult_t free(uint32_t handle, addr_t buffer, uint32_t len);

    mcResult_t registerWsmL2(
        addr_t      buffer,
        uint32_t    len,
        uint32_t    pid,
        uint32_t    *pHandle,
        addr_t      *pPhysWsmL2);

    mcResult_t unregisterWsmL2(uint32_t handle);

    mcResult_t lockWsmL2(uint32_t handle);

    mcResult_t unlockWsmL2(uint32_t handle);

    mcResult_t cleanupWsmL2(void);

    addr_t findWsmL2(uint32_t handle);

    mcResult_t findContiguousWsm(uint32_t handle, addr_t *phys, uint32_t *len);

    /**
    * Tell stub to start MobiCore from given physical address
    */
    int fcExecute(addr_t startAddr, uint32_t areaLength);

    bool checkVersion(void);
};

typedef CMcKMod  *CMcKMod_ptr;

#endif // CMCKMOD_H_
