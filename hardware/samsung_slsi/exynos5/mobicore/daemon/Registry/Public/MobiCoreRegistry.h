/**
 * @addtogroup MCD_MCDIMPL_DAEMON_REG
 * @{
 * G&D MobiCore Registry
 *
 * @file
 * Mobicore Driver Registry.
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
#ifndef MOBICORE_REGISTRY_H_
#define MOBICORE_REGISTRY_H_

#include "MobiCoreDriverApi.h"
#include "mcContainer.h"

#ifdef __cplusplus
extern "C" {
#endif

    /**
     * Registry object.
     */
    typedef struct {
        uint32_t len;
        uint8_t value[];
    } regObject_t;

    /** Maximum size of a trustlet in bytes. */
#define MAX_TL_SIZE     (1 * 1024 * 1024)

//-----------------------------------------------------------------

    /** Stores an authentication token in registry.
     * @param  so Authentication token secure object.
     * @return MC_DRV_OK if successful, otherwise error code.
     */
    mcResult_t mcRegistryStoreAuthToken(const mcSoAuthTokenCont_t *so);

    /** Reads an authentication token from registry.
     * @param[out] so Authentication token secure object.
     * @return MC_DRV_OK if successful, otherwise error code.
     */
    mcResult_t mcRegistryReadAuthToken(mcSoAuthTokenCont_t *so);

    /** Deletes the authentication token secure object from the registry.
     * @return MC_DRV_OK if successful, otherwise error code.
     */
    mcResult_t mcRegistryDeleteAuthToken(void);

    /** Stores a root container secure object in the registry.
     * @param so Root container secure object.
     * @return MC_DRV_OK if successful, otherwise error code.
     */
    mcResult_t mcRegistryStoreRoot(const mcSoRootCont_t *so);

    /** Reads a root container secure object from the registry.
     * @param[out] so Root container secure object.
     * @return MC_DRV_OK if successful, otherwise error code.
     */
    mcResult_t mcRegistryReadRoot(mcSoRootCont_t *so);

    /** Stores a service provider container secure object in the registry.
     * @param spid Service provider ID.
     * @param so Service provider container secure object.
     * @return MC_DRV_OK if successful, otherwise error code.
     */
    mcResult_t mcRegistryStoreSp(mcSpid_t spid, const mcSoSpCont_t *so);

    /** Reads a service provider container secure object from the registry.
     * @param spid Service provider ID.
     * @param[out] so Service provider container secure object.
     * @return MC_DRV_OK if successful, otherwise error code.
     */
    mcResult_t mcRegistryReadSp(mcSpid_t spid, mcSoSpCont_t *so);

    /** Deletes a service provider recursively, including all trustlets and
     * data.
     * @param spid Service provider ID.
     * @return MC_DRV_OK if successful, otherwise error code.
     */
    mcResult_t mcRegistryCleanupSp(mcSpid_t spid);

    /** Stores a trustlet container secure object in the registry.
     * @param uuid Trustlet UUID.
     * @param so Trustlet container secure object.
     * @return MC_DRV_OK if successful, otherwise error code.
     */
    mcResult_t mcRegistryStoreTrustletCon(const mcUuid_t *uuid, const mcSoTltCont_t *so);

    /** Reads a trustlet container secure object from the registry.
     * @param uuid Trustlet UUID.
     * @param[out] so Trustlet container secure object.
     * @return MC_DRV_OK if successful, otherwise error code.
     */
    mcResult_t mcRegistryReadTrustletCon(const mcUuid_t *uuid, mcSoTltCont_t *so);

    /** Deletes a trustlet container secure object and all of its associated data.
     * @param uuid Trustlet UUID.
     * @return MC_DRV_OK if successful, otherwise error code.
     */
    mcResult_t mcRegistryCleanupTrustlet(const mcUuid_t *uuid);

    /** Stores a data container secure object in the registry.
     * @param so Data container secure object.
     * @return MC_DRV_OK if successful, otherwise error code.
     */
    mcResult_t mcRegistryStoreData(const mcSoDataCont_t *so);

    /** Reads a data container secure object from the registry.
     * @param context (service provider = 0; trustlet = 1).
     * @param cid Service provider or UUID.
     * @param pid Personalization data identifier.
     * @param[out] so Data container secure object.
     * @param maxLen Maximum size (in bytes) of the destination buffer (so).
     * @return MC_DRV_OK if successful, otherwise error code.
     */
    mcResult_t mcRegistryReadData(
        uint32_t context,
        const mcCid_t *cid,
        mcPid_t pid,
        mcSoDataCont_t *so,
        uint32_t maxLen
    );

    /** Deletes the root container and all of its associated service provider
     * containers.
     * @return MC_DRV_OK if successful, otherwise error code.
     */
    mcResult_t mcRegistryCleanupRoot(void);

    /** Returns a registry object for a given service.
     * @param uuid service UUID
     * @return Registry object.
     * @note It is the responsibility of the caller to free the registry object
     * allocated by this function.
     */
    regObject_t *mcRegistryGetServiceBlob(const mcUuid_t  *uuid);

    /** Returns a registry object for a given service.
     * @param driverFilename driver filename
     * @return Registry object.
     * @note It is the responsibility of the caller to free the registry object
     * allocated by this function.
     */
    regObject_t *mcRegistryGetDriverBlob(const char *driverFilename);

#ifdef __cplusplus
}
#endif

#endif // MOBICORE_REGISTRY_H_

/** @} */
