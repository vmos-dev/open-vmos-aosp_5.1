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
#include <cstdlib>

#include <sys/mman.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <inttypes.h>
#include <cstring>

#include "McTypes.h"
#include "mc_linux.h"
#include "mcVersionHelper.h"

#include "CMcKMod.h"

#include "log.h"

//------------------------------------------------------------------------------
MC_CHECK_VERSION(MCDRVMODULEAPI, 1, 1);

//------------------------------------------------------------------------------
mcResult_t CMcKMod::mapWsm(
    uint32_t    len,
    uint32_t    *pHandle,
    addr_t      *pVirtAddr,
    addr_t      *pPhysAddr)
{
    int ret = 0;
    LOG_V(" mapWsm(): len=%d", len);

    if (!isOpen()) {
        LOG_E("no connection to kmod");
        return MC_DRV_ERR_KMOD_NOT_OPEN;
    }

    // mapping response data is in the buffer
struct mc_ioctl_map mapParams = {
    .len = len
};

    ret = ioctl(fdKMod, MC_IO_MAP_WSM, &mapParams);
    if (ret != 0) {
        LOG_ERRNO("ioctl MC_IO_MAP_WSM");
        return MAKE_MC_DRV_KMOD_WITH_ERRNO(errno);
    }

    addr_t virtAddr = ::mmap(0, len, PROT_READ | PROT_WRITE, MAP_SHARED,
                             fdKMod, mapParams.phys_addr);
    if (virtAddr == MAP_FAILED) {
        LOG_ERRNO("mmap");
        return MAKE_MC_DRV_KMOD_WITH_ERRNO(errno);
    }


    LOG_V(" mapped to %p, handle=%d, phys=%p ", virtAddr,
          mapParams.handle, (addr_t) (mapParams.phys_addr));

    if (pVirtAddr != NULL) {
        *pVirtAddr = virtAddr;
    }

    if (pHandle != NULL) {
        *pHandle = mapParams.handle;
    }

    if (pPhysAddr != NULL) {
        *pPhysAddr = (addr_t) (mapParams.phys_addr);
    }

    return 0;
}

//------------------------------------------------------------------------------
mcResult_t CMcKMod::mapMCI(
    uint32_t    len,
    uint32_t    *pHandle,
    addr_t      *pVirtAddr,
    addr_t      *pPhysAddr,
    bool        *pReuse)
{
    LOG_I("Mapping MCI: len=%d", len);
    // mapping response data is in the buffer
    struct mc_ioctl_map mapParams = {
        .len = len
    };

    if (!isOpen()) {
        LOG_E("no connection to kmod");
        return MC_DRV_ERR_KMOD_NOT_OPEN;
    }

    int ret = ioctl(fdKMod, MC_IO_MAP_MCI, &mapParams);
    if (ret != 0) {
        LOG_ERRNO("ioctl MC_IO_MAP_MCI");
        return MAKE_MC_DRV_KMOD_WITH_ERRNO(errno);
    }

    addr_t virtAddr = ::mmap(0, len, PROT_READ | PROT_WRITE, MAP_SHARED,
                             fdKMod, 0);
    if (virtAddr == MAP_FAILED) {
        LOG_ERRNO("mmap");
        return MAKE_MC_DRV_KMOD_WITH_ERRNO(errno);
    }
    mapParams.addr = (unsigned long)virtAddr;
    *pReuse = mapParams.reused;

    LOG_V(" MCI mapped to %p, handle=%d, phys=%p, reused=%s",
          (void *)mapParams.addr, mapParams.handle, (addr_t) (mapParams.phys_addr),
          mapParams.reused ? "true" : "false");

    if (pVirtAddr != NULL) {
        *pVirtAddr = (void *)mapParams.addr;
    }

    if (pHandle != NULL) {
        *pHandle = mapParams.handle;
    }

    if (pPhysAddr != NULL) {
        *pPhysAddr = (addr_t) (mapParams.phys_addr);
    }

    // clean memory
    //memset(pMmapResp, 0, sizeof(*pMmapResp));

    return MC_DRV_OK;
}

//------------------------------------------------------------------------------
mcResult_t CMcKMod::mapPersistent(
    uint32_t    len,
    uint32_t    *pHandle,
    addr_t      *pVirtAddr,
    addr_t      *pPhysAddr)
{
    // Not currently supported by the driver
    LOG_E("MobiCore Driver does't support persistent buffers");
    return MC_DRV_ERR_NOT_IMPLEMENTED;
}


//------------------------------------------------------------------------------
int CMcKMod::read(addr_t buffer, uint32_t len)
{
    int ret = 0;

    if (!isOpen()) {
        LOG_E("no connection to kmod");
        return MC_DRV_ERR_KMOD_NOT_OPEN;
    }

    ret = ::read(fdKMod, buffer, len);
    if (ret == -1) {
        LOG_ERRNO("read");
    }
    return ret;
}


//------------------------------------------------------------------------------
bool CMcKMod::waitSSIQ(uint32_t *pCnt)
{
    uint32_t cnt;
    if (read(&cnt, sizeof(cnt)) != sizeof(cnt)) {
        return false;
    }

    if (pCnt != NULL) {
        *pCnt = cnt;
    }

    return true;
}


//------------------------------------------------------------------------------
int CMcKMod::fcInit(uint32_t nqOffset, uint32_t nqLength, uint32_t mcpOffset,
                    uint32_t mcpLength)
{
    int ret = 0;

    if (!isOpen()) {
        return MC_DRV_ERR_KMOD_NOT_OPEN;
    }

    // Init MC with NQ and MCP buffer addresses
    struct mc_ioctl_init fcInitParams = {
        .nq_offset = nqOffset,
        .nq_length = nqLength,
        .mcp_offset = mcpOffset,
        .mcp_length = mcpLength
    };

    ret = ioctl(fdKMod, MC_IO_INIT, &fcInitParams);
    if (ret != 0) {
        LOG_ERRNO("ioctl MC_IO_INIT");
    }

    return ret;
}

//------------------------------------------------------------------------------
int CMcKMod::fcInfo(uint32_t extInfoId, uint32_t *pState, uint32_t *pExtInfo)
{
    int ret = 0;

    if (!isOpen()) {
        LOG_E("no connection to kmod");
        return MC_DRV_ERR_KMOD_NOT_OPEN;
    }

    // Init MC with NQ and MCP buffer addresses
    struct mc_ioctl_info fcInfoParams = {
        .ext_info_id = extInfoId
    };
    ret = ioctl(fdKMod, MC_IO_INFO, &fcInfoParams);
    if (ret != 0) {
        LOG_ERRNO("ioctl MC_IO_INFO");
        return ret;
    }

    if (pState != NULL) {
        *pState = fcInfoParams.state;
    }

    if (pExtInfo != NULL) {
        *pExtInfo = fcInfoParams.ext_info;
    }

    return ret;
}


//------------------------------------------------------------------------------
int CMcKMod::fcYield(void)
{
    int ret = 0;

    if (!isOpen()) {
        LOG_E("no connection to kmod");
        return MC_DRV_ERR_KMOD_NOT_OPEN;
    }

    ret = ioctl(fdKMod, MC_IO_YIELD, NULL);
    if (ret != 0) {
        LOG_ERRNO("ioctl MC_IO_YIELD");
        LOG_E("ret = %d", ret);
    }

    return ret;
}


//------------------------------------------------------------------------------
int CMcKMod::fcNSIQ(void)
{
    int ret = 0;

    if (!isOpen()) {
        LOG_E("no connection to kmod");
        return  MC_DRV_ERR_KMOD_NOT_OPEN;
    }

    ret = ioctl(fdKMod, MC_IO_NSIQ, NULL);
    if (ret != 0) {
        LOG_ERRNO("ioctl MC_IO_NSIQ");
        LOG_E("ret = %d", ret);
    }

    return ret;
}


//------------------------------------------------------------------------------
mcResult_t CMcKMod::free(uint32_t handle, addr_t buffer, uint32_t len)
{
    LOG_V("free(): handle=%d", handle);

    if (!isOpen()) {
        LOG_E("no connection to kmod");
        return MC_DRV_ERR_KMOD_NOT_OPEN;
    }

    // Even if unmap fails we still go on with our request
    if (::munmap(buffer, len)) {
        LOG_I("buffer = %p, len = %d", buffer, len);
        LOG_ERRNO("mmap failed");
    }

    int ret = ioctl(fdKMod, MC_IO_FREE, handle);
    if (ret != 0) {
        LOG_ERRNO("ioctl MC_IO_FREE");
        return MAKE_MC_DRV_KMOD_WITH_ERRNO(errno);
    }

    return MC_DRV_OK;
}


//------------------------------------------------------------------------------
mcResult_t CMcKMod::registerWsmL2(
    addr_t      buffer,
    uint32_t    len,
    uint32_t    pid,
    uint32_t    *pHandle,
    addr_t      *pPhysWsmL2)
{
    LOG_I(" Registering virtual buffer at %p, len=%d as World Shared Memory", buffer, len);

    if (!isOpen()) {
        LOG_E("no connection to kmod");
        return MC_DRV_ERR_KMOD_NOT_OPEN;
    }

    struct mc_ioctl_reg_wsm params = {
        .buffer = (uint32_t) buffer,
        .len = len,
        .pid = pid
    };

    int ret = ioctl(fdKMod, MC_IO_REG_WSM, &params);
    if (ret != 0) {
        LOG_ERRNO("ioctl MC_IO_UNREG_WSM");
        return MAKE_MC_DRV_KMOD_WITH_ERRNO(errno);
    }

    LOG_I(" Registered, handle=%d, L2 phys=0x%x ", params.handle, params.table_phys);

    if (pHandle != NULL) {
        *pHandle = params.handle;
    }

    if (pPhysWsmL2 != NULL) {
        *pPhysWsmL2 = (addr_t) params.table_phys;
    }

    return MC_DRV_OK;
}


//------------------------------------------------------------------------------
mcResult_t CMcKMod::unregisterWsmL2(uint32_t handle)
{
    LOG_I(" Unregistering World Shared Memory with handle %d", handle);

    if (!isOpen()) {
        LOG_E("no connection to kmod");
        return MC_DRV_ERR_KMOD_NOT_OPEN;
    }

    int ret = ioctl(fdKMod, MC_IO_UNREG_WSM, handle);
    if (ret != 0) {
        LOG_ERRNO("ioctl MC_IO_UNREG_WSM");
        return MAKE_MC_DRV_KMOD_WITH_ERRNO(errno);
    }

    return MC_DRV_OK;
}

//------------------------------------------------------------------------------
mcResult_t CMcKMod::lockWsmL2(uint32_t handle)
{
    int ret = 0;

    LOG_I(" Locking World Shared Memory with handle %d", handle);

    if (!isOpen()) {
        LOG_E("no connection to kmod");
        return MC_DRV_ERR_KMOD_NOT_OPEN;
    }

    ret = ioctl(fdKMod, MC_IO_LOCK_WSM, handle);
    if (ret != 0) {
        LOG_ERRNO("ioctl MC_IO_UNREG_WSM");
        LOG_E("ret = %d", ret);
    }

    return ret;
}

//------------------------------------------------------------------------------
mcResult_t CMcKMod::unlockWsmL2(uint32_t handle)
{
    int ret = 0;

    LOG_I(" Unlocking World Shared Memory with handle %d", handle);

    if (!isOpen()) {
        LOG_E("no connection to kmod");
        return MC_DRV_ERR_KMOD_NOT_OPEN;
    }

    ret = ioctl(fdKMod, MC_IO_UNLOCK_WSM, handle);
    // Failure here is not really important
    if (ret != 0) {
        LOG_I("ret = %d", ret);
    }

    return ret;
}


//------------------------------------------------------------------------------
addr_t CMcKMod::findWsmL2(uint32_t handle)
{
    int ret = 0;
    uint32_t param = handle;

    LOG_I(" Resolving the WSM l2 for handle=%u", handle);

    if (!isOpen()) {
        LOG_E("no connection to kmod");
        return NULL;
    }

    ret = ioctl(fdKMod, MC_IO_RESOLVE_WSM, &param);
    if (ret != 0 || param == 0) {
        LOG_ERRNO("ioctl MC_IO_RESOLVE_WSM");
        LOG_E("param %u, ret = %d", param, ret);
    }

    return (addr_t)param;
}

//------------------------------------------------------------------------------
mcResult_t CMcKMod::findContiguousWsm(uint32_t handle, addr_t *phys, uint32_t *len)
{
    mcResult_t ret = MC_DRV_OK;
    struct mc_ioctl_resolv_cont_wsm wsm;

    wsm.handle = handle;

    LOG_I(" Resolving the contiguous WSM l2 for handle=%u", handle);

    if (!isOpen()) {
        LOG_E("no connection to kmod");
        return NULL;
    }

    ret = ioctl(fdKMod, MC_IO_RESOLVE_CONT_WSM, &wsm);
    if (ret != 0) {
        LOG_ERRNO("ioctl MC_IO_RESOLVE_CONT_WSM");
    } else {
        *phys = (addr_t)wsm.phys;
        *len = wsm.length;
    }

    return ret;
}

//------------------------------------------------------------------------------
mcResult_t CMcKMod::cleanupWsmL2(void)
{
    int ret = 0;

    LOG_I(" Cleaning up the orphaned bulk buffers");

    if (!isOpen()) {
        LOG_E("no connection to kmod");
        return MC_DRV_ERR_KMOD_NOT_OPEN;
    }

    ret = ioctl(fdKMod, MC_IO_CLEAN_WSM, 0);
    if (ret != 0) {
        LOG_ERRNO("ioctl MC_IO_UNREG_WSM");
        LOG_E("ret = %d", ret);
    }

    return ret;
}

//------------------------------------------------------------------------------
int CMcKMod::fcExecute(addr_t startAddr, uint32_t areaLength)
{
    int ret = 0;
    struct mc_ioctl_execute params = {
        .phys_start_addr = (uint32_t)startAddr,
        .length = areaLength
    };

    if (!isOpen()) {
        LOG_E("no connection to kmod");
        return MC_DRV_ERR_KMOD_NOT_OPEN;
    }

    ret = ioctl(fdKMod, MC_IO_EXECUTE, &params);
    if (ret != 0) {
        LOG_ERRNO("ioctl MC_IO_EXECUTE");
    }

    return ret;
}
//------------------------------------------------------------------------------
bool CMcKMod::checkVersion(void)
{
    uint32_t version;
    if (!isOpen()) {
        LOG_E("no connection to kmod");
        return false;
    }

    int ret = ioctl(fdKMod, MC_IO_VERSION, &version);
    if (ret != 0) {
        LOG_ERRNO("ioctl MC_IO_VERSION");
        LOG_E("ret = %d", ret);
        return false;
    }

    // Run-time check.
    char *errmsg;
    if (!checkVersionOkMCDRVMODULEAPI(version, &errmsg)) {
        LOG_E("%s", errmsg);
        return false;
    }
    LOG_I("%s", errmsg);

    return true;
}

/** @} */
