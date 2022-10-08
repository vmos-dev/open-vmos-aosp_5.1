/** Mobicore Driver Registry.
 *
 * Implements the MobiCore driver registry which maintains trustlets.
 *
 * @file
 * @ingroup MCD_MCDIMPL_DAEMON_REG
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

#include "MobiCoreRegistry.h"
#include <stdlib.h>
#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>
#include <assert.h>
#include <string>
#include <cstring>
#include <cstddef>
#include "mcLoadFormat.h"
#include "mcSpid.h"
#include "mcVersionHelper.h"

#include "log.h"

MC_CHECK_DATA_OBJECT_VERSION(MCLF, 2, 0);
MC_CHECK_DATA_OBJECT_VERSION(CONTAINER, 2, 0);

// Asserts expression at compile-time (to be used within a function body).
#define ASSERT_STATIC(e) do { enum { assert_static__ = 1 / (e) }; } while (0)

using namespace std;

static const string MC_REGISTRY_DEFAULT_PATH = "/data/app/mcRegistry";
static const string AUTH_TOKEN_FILE_NAME = "00000000.authtokcont";
static const string ROOT_FILE_NAME = "00000000.rootcont";
static const string SP_CONT_FILE_EXT = ".spcont";
static const string TL_CONT_FILE_EXT = ".tlcont";
static const string TL_BIN_FILE_EXT = ".tlbin";
static const string DATA_CONT_FILE_EXT = ".datacont";

static const string ENV_MC_REGISTRY_PATH = "MC_REGISTRY_PATH";
static const string ENV_MC_REGISTRY_FALLBACK_PATH = "MC_REGISTRY_FALLBACK_PATH";
static const string ENV_MC_AUTH_TOKEN_PATH = "MC_AUTH_TOKEN_PATH";

static const string getRegistryPath();
static const string getAuthTokenFilePath();
static const string getRootContFilePath();
static const string getSpDataPath(mcSpid_t spid);
static const string getSpContFilePath(mcSpid_t spid);
static const string getTlContFilePath(const mcUuid_t *uuid);
static const string getTlDataPath(const mcUuid_t *uuid);
static const string getTlDataFilePath(const mcUuid_t *uuid, mcPid_t pid);
static const string getTlBinFilePath(const mcUuid_t *uuid);

static const string uint32ToString(mcSpid_t spid);
static const string byteArrayToString(const void *bytes, size_t elems);
static bool doesDirExist(const char *path);

//------------------------------------------------------------------------------
mcResult_t mcRegistryStoreAuthToken(
    const mcSoAuthTokenCont_t *so
)
{
    if (NULL == so) {
        LOG_E("mcRegistry store So.Soc failed: %d", MC_DRV_ERR_INVALID_PARAMETER);
        return MC_DRV_ERR_INVALID_PARAMETER;
    }
    if (CONT_TYPE_SOC != so->coSoc.type) {
        LOG_E("mcRegistry store So.Soc failed: %d", MC_DRV_ERR_INVALID_PARAMETER);
        return MC_DRV_ERR_INVALID_PARAMETER;
    }
    const string &authTokenFilePath = getAuthTokenFilePath();
    LOG_I("store AuthToken: %s", authTokenFilePath.c_str());

    FILE *fs = fopen(authTokenFilePath.c_str(), "wb");
    if (!fs) {
        LOG_E("mcRegistry store So.Soc failed: %d", MC_DRV_ERR_INVALID_DEVICE_FILE);
        return MC_DRV_ERR_INVALID_DEVICE_FILE;
    }
    fseek(fs, 0, SEEK_SET);
    fwrite((char *)so, 1, sizeof(mcSoAuthTokenCont_t), fs);
    fflush(fs);
    fclose(fs);

    return MC_DRV_OK;
}


//------------------------------------------------------------------------------
mcResult_t mcRegistryReadAuthToken(
    mcSoAuthTokenCont_t *so
)
{
    if (NULL == so) {
        LOG_E("mcRegistry read So.Soc failed: %d", MC_DRV_ERR_INVALID_PARAMETER);
        return MC_DRV_ERR_INVALID_PARAMETER;
    }
    const string &authTokenFilePath = getAuthTokenFilePath();
    LOG_I("read AuthToken: %s", authTokenFilePath.c_str());

    FILE *fs = fopen(authTokenFilePath.c_str(), "rb");
    if (!fs) {
        LOG_E("mcRegistry read So.Soc failed: %d", MC_DRV_ERR_INVALID_DEVICE_FILE);
        return MC_DRV_ERR_INVALID_DEVICE_FILE;
    }
    fseek(fs, 0, SEEK_END);
    int32_t filesize = ftell(fs);
    if (sizeof(mcSoAuthTokenCont_t) != filesize) {
        fclose(fs);
        LOG_E("mcRegistry read So.Soc failed: %d", MC_DRV_ERR_OUT_OF_RESOURCES);
        return MC_DRV_ERR_OUT_OF_RESOURCES;
    }
    fseek(fs, 0, SEEK_SET);
    fread((char *)so, 1, sizeof(mcSoAuthTokenCont_t), fs);
    fclose(fs);

    return MC_DRV_OK;
}

//------------------------------------------------------------------------------
mcResult_t mcRegistryDeleteAuthToken(
    void
)
{
    remove(getAuthTokenFilePath().c_str());
    // @TODO: is return to check ?
    return MC_DRV_OK;
}


//------------------------------------------------------------------------------
mcResult_t mcRegistryStoreRoot(
    const mcSoRootCont_t *so
)
{
    if (NULL == so) {
        LOG_E("mcRegistry store So.Root failed: %d", MC_DRV_ERR_INVALID_PARAMETER);
        return MC_DRV_ERR_INVALID_PARAMETER;
    }
    if (CONT_TYPE_ROOT != so->cont.type) {
        LOG_E("mcRegistry store So.Root failed: %d", MC_DRV_ERR_INVALID_PARAMETER);
        return MC_DRV_ERR_INVALID_PARAMETER;
    }
    const string &rootContFilePath = getRootContFilePath();
    LOG_I("store Root: %s", rootContFilePath.c_str());

    FILE *fs = fopen(rootContFilePath.c_str(), "wb");
    if (!fs) {
        LOG_E("mcRegistry store So.Root failed: %d", MC_DRV_ERR_INVALID_DEVICE_FILE);
        return MC_DRV_ERR_INVALID_DEVICE_FILE;
    }
    fseek(fs, 0, SEEK_SET);
    fwrite((char *)so, 1, sizeof(mcSoRootCont_t), fs);
    fflush(fs);
    fclose(fs);

    return MC_DRV_OK;
}


//------------------------------------------------------------------------------
mcResult_t mcRegistryReadRoot(
    mcSoRootCont_t *so
)
{
    if (NULL == so) {
        LOG_E("mcRegistry read So.Root failed: %d", MC_DRV_ERR_INVALID_PARAMETER);
        return MC_DRV_ERR_INVALID_PARAMETER;
    }
    const string &rootContFilePath = getRootContFilePath();
    LOG_I("read Root: %s", rootContFilePath.c_str());

    FILE *fs = fopen(rootContFilePath.c_str(), "rb");
    if (!fs) {
        LOG_E("mcRegistry read So.Root failed: %d", MC_DRV_ERR_INVALID_DEVICE_FILE);
        return MC_DRV_ERR_INVALID_DEVICE_FILE;
    }
    fseek(fs, 0, SEEK_END);
    int32_t filesize = ftell(fs);
    if (sizeof(mcSoRootCont_t) != filesize) {
        fclose(fs);
        LOG_E("mcRegistry read So.Root failed: %d", MC_DRV_ERR_OUT_OF_RESOURCES);
        return MC_DRV_ERR_OUT_OF_RESOURCES;
    }
    fseek(fs, 0, SEEK_SET);
    fread((char *)so, 1, sizeof(mcSoRootCont_t), fs);
    fclose(fs);

    return MC_DRV_OK;
}


//------------------------------------------------------------------------------
mcResult_t mcRegistryStoreSp(
    mcSpid_t            spid,
    const mcSoSpCont_t  *so
)
{
    if ((0 == spid) || (NULL == so)) {
        LOG_E("mcRegistry store So.Sp(SpId) failed: %d", MC_DRV_ERR_INVALID_PARAMETER);
        return MC_DRV_ERR_INVALID_PARAMETER;
    }
    if (CONT_TYPE_SP != so->cont.type) {
        LOG_E("mcRegistry store So.Sp(SpId) failed: %d", MC_DRV_ERR_INVALID_PARAMETER);
        return MC_DRV_ERR_INVALID_PARAMETER;
    }
    const string &spContFilePath = getSpContFilePath(spid);
    LOG_I("store SP: %s", spContFilePath.c_str());

    FILE *fs = fopen(spContFilePath.c_str(), "wb");
    if (!fs) {
        LOG_E("mcRegistry store So.Sp(SpId) failed: %d", MC_DRV_ERR_INVALID_DEVICE_FILE);
        return MC_DRV_ERR_INVALID_DEVICE_FILE;
    }
    fseek(fs, 0, SEEK_SET);
    fwrite((char *)so, 1, sizeof(mcSoSpCont_t), fs);
    fflush(fs);
    fclose(fs);

    return MC_DRV_OK;
}


//------------------------------------------------------------------------------
mcResult_t mcRegistryReadSp(
    mcSpid_t        spid,
    mcSoSpCont_t    *so
)
{
    if ((0 == spid) || (NULL == so)) {
        LOG_E("mcRegistry read So.Sp(SpId) failed: %d", MC_DRV_ERR_INVALID_PARAMETER);
        return MC_DRV_ERR_INVALID_PARAMETER;
    }
    const string &spContFilePath = getSpContFilePath(spid);
    LOG_I("read SP: %s", spContFilePath.c_str());

    FILE *fs = fopen(spContFilePath.c_str(), "rb");
    if (!fs) {
        LOG_E("mcRegistry read So.Sp(SpId) failed: %d", MC_DRV_ERR_INVALID_DEVICE_FILE);
        return MC_DRV_ERR_INVALID_DEVICE_FILE;
    }
    fseek(fs, 0, SEEK_END);
    int32_t filesize = ftell(fs);
    if (sizeof(mcSoSpCont_t) != filesize) {
        fclose(fs);
        LOG_E("mcRegistry read So.Sp(SpId) failed: %d", MC_DRV_ERR_OUT_OF_RESOURCES);
        return MC_DRV_ERR_OUT_OF_RESOURCES;
    }
    fseek(fs, 0, SEEK_SET);
    fread((char *)so, 1, sizeof(mcSoSpCont_t), fs);
    fclose(fs);

    return MC_DRV_OK;
}


//------------------------------------------------------------------------------
mcResult_t mcRegistryStoreTrustletCon(
    const mcUuid_t      *uuid,
    const mcSoTltCont_t *so
)
{
    if ((NULL == uuid) || (NULL == so)) {
        LOG_E("mcRegistry store So.TrustletCont(uuid) failed: %d", MC_DRV_ERR_INVALID_PARAMETER);
        return MC_DRV_ERR_INVALID_PARAMETER;
    }
    if (CONT_TYPE_TLCON != so->cont.type) {
        LOG_E("mcRegistry store So.TrustletCont(uuid) failed: %d", MC_DRV_ERR_INVALID_PARAMETER);
        return MC_DRV_ERR_INVALID_PARAMETER;
    }
    const string &tlContFilePath = getTlContFilePath(uuid);
    LOG_I("store TLc: %s", tlContFilePath.c_str());

    FILE *fs = fopen(tlContFilePath.c_str(), "wb");
    if (!fs) {
        LOG_E("mcRegistry store So.TrustletCont(uuid) failed: %d", MC_DRV_ERR_INVALID_DEVICE_FILE);
        return MC_DRV_ERR_INVALID_DEVICE_FILE;
    }
    fseek(fs, 0, SEEK_SET);
    fwrite((char *)so, 1, sizeof(mcSoTltCont_t), fs);
    fflush(fs);
    fclose(fs);

    return MC_DRV_OK;
}


//------------------------------------------------------------------------------
mcResult_t mcRegistryReadTrustletCon(
    const mcUuid_t  *uuid,
    mcSoTltCont_t   *so
)
{
    if ((NULL == uuid) || (NULL == so)) {
        LOG_E("mcRegistry read So.TrustletCont(uuid) failed: %d", MC_DRV_ERR_INVALID_PARAMETER);
        return MC_DRV_ERR_INVALID_PARAMETER;
    }
    const string &tlContFilePath = getTlContFilePath(uuid);
    LOG_I("read TLc: %s", tlContFilePath.c_str());

    FILE *fs = fopen(tlContFilePath.c_str(), "rb");
    if (!fs) {
        LOG_E("mcRegistry read So.TrustletCont(uuid) failed: %d", MC_DRV_ERR_INVALID_DEVICE_FILE);
        return MC_DRV_ERR_INVALID_DEVICE_FILE;
    }
    fseek(fs, 0, SEEK_END);
    int32_t filesize = ftell(fs);
    if (sizeof(mcSoTltCont_t) != filesize) {
        fclose(fs);
        LOG_E("mcRegistry read So.TrustletCont(uuid) failed: %d. Size=%i, expected=%i", MC_DRV_ERR_OUT_OF_RESOURCES, filesize, sizeof(mcSoTltCont_t));
        return MC_DRV_ERR_OUT_OF_RESOURCES;
    }
    fseek(fs, 0, SEEK_SET);
    fread((char *)so, 1, sizeof(mcSoTltCont_t), fs);
    fclose(fs);

    return MC_DRV_OK;
}


//------------------------------------------------------------------------------
mcResult_t mcRegistryStoreData(
    const mcSoDataCont_t *so
)
{
    if (NULL == so) {
        LOG_E("mcRegistry store So.Data failed: %d", MC_DRV_ERR_INVALID_PARAMETER);
        return MC_DRV_ERR_INVALID_PARAMETER;
    }
    string pathname, filename;

    switch (so->cont.type) {
    case CONT_TYPE_SPDATA:
        LOG_E("SPDATA not supported");
        return MC_DRV_ERR_INVALID_PARAMETER;
        break;
    case CONT_TYPE_TLDATA:
        pathname = getTlDataPath(&so->cont.uuid);
        filename = getTlDataFilePath(&so->cont.uuid, so->cont.pid);
        break;
    default:
        LOG_E("mcRegistry store So.Data(cid/pid) failed: %d", MC_DRV_ERR_INVALID_PARAMETER);
        return MC_DRV_ERR_INVALID_PARAMETER;
    }
    mkdir(pathname.c_str(), 0777);

    LOG_I("store DT: %s", filename.c_str());

    FILE *fs = fopen(filename.c_str(), "wb");
    if (!fs) {
        LOG_E("mcRegistry store So.Data(cid/pid) failed: %d", MC_DRV_ERR_INVALID_DEVICE_FILE);
        return MC_DRV_ERR_INVALID_DEVICE_FILE;
    }
    fseek(fs, 0, SEEK_SET);
    fwrite((char *)so, 1, MC_SO_SIZE(so->soHeader.plainLen, so->soHeader.encryptedLen), fs);
    fflush(fs);
    fclose(fs);

    return MC_DRV_OK;
}


//------------------------------------------------------------------------------
mcResult_t mcRegistryReadData(
    uint32_t        context,
    const mcCid_t   *cid,
    mcPid_t         pid,
    mcSoDataCont_t  *so,
    uint32_t        maxLen
)
{
    if ((NULL == cid) || (NULL == so)) {
        LOG_E("mcRegistry read So.Data failed: %d", MC_DRV_ERR_INVALID_PARAMETER);
        return MC_DRV_ERR_INVALID_PARAMETER;
    }
    string filename;
    switch (context) {
    case 0:
        LOG_E("SPDATA not supported");
        return MC_DRV_ERR_INVALID_PARAMETER;
        break;
    case 1:
        filename = getTlDataFilePath(&so->cont.uuid, so->cont.pid);
        break;
    default:
        LOG_E("mcRegistry read So.Data(cid/pid) failed: %d", MC_DRV_ERR_INVALID_PARAMETER);
        return MC_DRV_ERR_INVALID_PARAMETER;
    }
    LOG_I("read DT: %s", filename.c_str());

    FILE *fs = fopen(filename.c_str(), "rb");
    if (!fs) {
        LOG_E("mcRegistry read So.Data(cid/pid) failed: %d", MC_DRV_ERR_INVALID_DEVICE_FILE);
        return MC_DRV_ERR_INVALID_DEVICE_FILE;
    }
    fseek(fs, 0, SEEK_END);
    uint32_t filesize = ftell(fs);
    if (maxLen < filesize) {
        fclose(fs);
        LOG_E("mcRegistry read So.Data(cid/pid) failed: %d", MC_DRV_ERR_OUT_OF_RESOURCES);
        return MC_DRV_ERR_OUT_OF_RESOURCES;
    }
    fseek(fs, 0, SEEK_SET);
    char *p = (char *) so;
    fread(p, 1, sizeof(mcSoHeader_t), fs);
    p += sizeof(mcSoHeader_t);
    fread(p, 1, MC_SO_SIZE(so->soHeader.plainLen, so->soHeader.encryptedLen) - sizeof(mcSoHeader_t), fs);
    fclose(fs);

    return MC_DRV_OK;
}


//------------------------------------------------------------------------------
mcResult_t mcRegistryCleanupTrustlet(
    const mcUuid_t *uuid
)
{
    DIR            *dp;
    struct dirent  *de;
    int             e;

    if (NULL == uuid) {
        LOG_E("mcRegistry cleanupTrustlet(uuid) failed: %d", MC_DRV_ERR_INVALID_PARAMETER);
        return MC_DRV_ERR_INVALID_PARAMETER;
    }
    string pathname = getTlDataPath(uuid);
    if (NULL != (dp = opendir(pathname.c_str()))) {
        while (NULL != (de = readdir(dp))) {
            if (de->d_name[0] != '.') {
                string dname = pathname + "/" + string (de->d_name);
                LOG_I("delete DT: %s", dname.c_str());
                if (0 != (e = remove(dname.c_str()))) {
                    LOG_E("remove UUID-data %s failed! error: %d", dname.c_str(), e);
                    return MC_DRV_ERR_UNKNOWN;
                }
            }
        }
        LOG_I("delete dir: %s", pathname.c_str());
        if (0 != (e = rmdir(pathname.c_str()))) {
            LOG_E("remove UUID-dir failed! errno: %d", e);
            return MC_DRV_ERR_UNKNOWN;
        }
    }
    string tlBinFilePath = getTlBinFilePath(uuid);
    LOG_I("delete Tlb: %s", tlBinFilePath.c_str());
    if (0 != (e = remove(tlBinFilePath.c_str()))) {
        LOG_E("remove Tlb failed! errno: %d", e);
//        return MC_DRV_ERR_UNKNOWN;     // a trustlet-binary must not be present ! (registered but not usable)
    }
    string tlContFilePath = getTlContFilePath(uuid);
    LOG_I("delete Tlc: %s", tlContFilePath.c_str());
    if (0 != (e = remove(tlContFilePath.c_str()))) {
        LOG_E("remove Tlc failed! errno: %d", e);
        return MC_DRV_ERR_UNKNOWN;
    }
    return MC_DRV_OK;
}


//------------------------------------------------------------------------------
mcResult_t mcRegistryCleanupSp(
    mcSpid_t spid
)
{
    mcResult_t      ret;
    mcSoSpCont_t    data;
    uint32_t        i;
    DIR            *dp;
    struct dirent  *de;
    int             e;

    if (0 == spid) {
        LOG_E("mcRegistry cleanupSP(SpId) failed: %d", MC_DRV_ERR_INVALID_PARAMETER);
        return MC_DRV_ERR_INVALID_PARAMETER;
    }
    ret = mcRegistryReadSp(spid, &data);
    if (MC_DRV_OK != ret) {
        LOG_E("read SP->UUID aborted! Return code: %d", ret);
        return ret;
    }
    for (i = 0; (i < MC_CONT_CHILDREN_COUNT) && (ret == MC_DRV_OK); i++) {
        if (0 != strncmp((const char *) & (data.cont.children[i]), (const char *)&MC_UUID_FREE, sizeof(mcUuid_t))) {
            ret = mcRegistryCleanupTrustlet(&(data.cont.children[i]));
        }
    }
    if (MC_DRV_OK != ret) {
        LOG_E("delete SP->UUID failed! Return code: %d", ret);
        return ret;
    }
    string pathname = getSpDataPath(spid);

    if (NULL != (dp = opendir(pathname.c_str()))) {
        while (NULL != (de = readdir(dp))) {
            if (de->d_name[0] != '.') {
                string dname = pathname + "/" + string (de->d_name);
                LOG_I("delete DT: %s", dname.c_str());
                if (0 != (e = remove(dname.c_str()))) {
                    LOG_E("remove SPID-data %s failed! error: %d", dname.c_str(), e);
                    return MC_DRV_ERR_UNKNOWN;
                }
            }
        }
        LOG_I("delete dir: %s", pathname.c_str());
        if (0 != (e = rmdir(pathname.c_str()))) {
            LOG_E("remove SPID-dir failed! error: %d", e);
            return MC_DRV_ERR_UNKNOWN;
        }
    }
    string spContFilePath = getSpContFilePath(spid);
    LOG_I("delete Sp: %s", spContFilePath.c_str());
    if (0 != (e = remove(spContFilePath.c_str()))) {
        LOG_E("remove SP failed! error: %d", e);
        return MC_DRV_ERR_UNKNOWN;
    }
    return MC_DRV_OK;
}


//------------------------------------------------------------------------------
mcResult_t mcRegistryCleanupRoot(void)
{
    mcResult_t ret;
    mcSoRootCont_t data;
    uint32_t i;
    int e;

    ret = mcRegistryReadRoot(&data);
    if (MC_DRV_OK != ret) {
        LOG_E("read Root aborted! Return code: %d", ret);
        return ret;
    }
    for (i = 0; (i < MC_CONT_CHILDREN_COUNT) && (ret == MC_DRV_OK); i++) {
        mcSpid_t spid = data.cont.children[i];
        if (spid != MC_SPID_FREE) {
            ret = mcRegistryCleanupSp(spid);
            if (MC_DRV_OK != ret) {
                LOG_E("Cleanup SP failed! Return code: %d", ret);
                return ret;
            }
        }
    }

    string rootContFilePath = getRootContFilePath();
    LOG_I("Delete root: %s", rootContFilePath.c_str());
    if (0 != (e = remove(rootContFilePath.c_str()))) {
        LOG_E("Delete root failed! error: %d", e);
        return MC_DRV_ERR_UNKNOWN;
    }
    return MC_DRV_OK;
}


//------------------------------------------------------------------------------
regObject_t *mcRegistryGetServiceBlob(
    const mcUuid_t *uuid
)
{
    regObject_t *regobj = NULL;

    // Ensure that a UUID is provided.
    if (NULL == uuid) {
        LOG_E("No UUID given");
        return NULL;
    }

    // Open service blob file.
    string tlBinFilePath = getTlBinFilePath(uuid);
    LOG_I(" Loading %s", tlBinFilePath.c_str());

    FILE *fs = fopen(tlBinFilePath.c_str(), "rb");
    if (!fs) {
        LOG_E("Cannot open %s", tlBinFilePath.c_str());
        return NULL;
    }

    // Determine and check service blob size.
    fseek(fs, 0, SEEK_END);
    int32_t tlSize = ftell(fs);
    fseek(fs, 0, SEEK_SET);
    if (MAX_TL_SIZE < tlSize) {
        LOG_E("mcRegistryGetServiceBlob() failed: service blob too big: %d", tlSize);
        return NULL;
    }

    // Check TL magic value.
    fseek(fs, offsetof(mclfIntro_t, magic), SEEK_SET);
    uint32_t magic;
    fread((char *)&magic, 1, sizeof(magic), fs);
    if (magic != MC_SERVICE_HEADER_MAGIC_BE) {
        fclose(fs);
        LOG_E("mcRegistryGetServiceBlob() failed: wrong header magic value: %d", magic);
        return NULL;
    }

    // Check header version.
    fseek(fs, offsetof(mclfIntro_t, version), SEEK_SET);
    uint32_t version;
    fread((char *)&version, 1, sizeof(version), fs);

    char *msg;
    if (!checkVersionOkDataObjectMCLF(version, &msg)) {
        fclose(fs);
        LOG_E("%s", msg);
        return NULL;
    }

    // Get service type.
    fseek(fs, offsetof(mclfHeaderV2_t, serviceType), SEEK_SET);
    serviceType_t serviceType;
    fread((char *)&serviceType, 1, sizeof(serviceType), fs);
    fseek(fs, 0, SEEK_SET);

#ifndef NDEBUG
    {
        const char *service_types[] = {
            "illegal", "Driver", "Trustlet", "System Trustlet"
        };
        int serviceType_safe = serviceType > SERVICE_TYPE_SYSTEM_TRUSTLET ? SERVICE_TYPE_ILLEGAL : serviceType;
        LOG_I(" Service is a %s (service type %d)", service_types[serviceType_safe], serviceType);
    }
#endif

    // If loadable driver or system trustlet.
    if (SERVICE_TYPE_DRIVER == serviceType || SERVICE_TYPE_SYSTEM_TRUSTLET == serviceType) {
        // Take trustlet blob 'as is'.
        if (NULL == (regobj = (regObject_t *) (malloc(sizeof(regObject_t) + tlSize)))) {
            fclose(fs);
            LOG_E("mcRegistryGetServiceBlob() failed: Out of memory");
            return NULL;
        }
        regobj->len = tlSize;
        fread((char *)regobj->value, 1, tlSize, fs);
        fclose(fs);
        // If user trustlet.
    } else if (SERVICE_TYPE_SP_TRUSTLET == serviceType) {
        // Take trustlet blob and append root, sp, and tl container.
        size_t regObjValueSize = tlSize + sizeof(mcSoContainerPath_t);

        // Prepare registry object.
        if (NULL == (regobj = (regObject_t *) malloc(sizeof(regObject_t) + regObjValueSize))) {
            fclose(fs);
            LOG_E("mcRegistryGetServiceBlob() failed: Out of memory");
            return NULL;
        }
        regobj->len = regObjValueSize;

        // Read and fill in trustlet blob at beginning.
        fread((char *)regobj->value, 1, tlSize, fs);
        fclose(fs);

        // Goto end of allocated space and fill in tl container, sp container,
        // and root container from back to front. Final registry object value
        // looks like this:
        //
        //    +---------------------------+-----------+---------+---------+
        //    | TL-Header TL-Code TL-Data | Root Cont | SP Cont | TL Cont |
        //    +---------------------------+-----------+-------------------+
        //    /------ Trustlet BLOB ------/
        //
        //    /------------------ regobj->header.len ---------------------/

        uint8_t *p = regobj->value + regobj->len;
        mcResult_t ret;
        do {
            char *msg;

            // Fill in TL container.
            p -= sizeof(mcSoTltCont_t);
            mcSoTltCont_t *soTlt = (mcSoTltCont_t *)p;
            if (MC_DRV_OK != (ret = mcRegistryReadTrustletCon(uuid, soTlt))) {
                break;
            }
            mcTltCont_t *tltCont = &soTlt->cont;
            if (!checkVersionOkDataObjectCONTAINER(tltCont->version, &msg)) {
                LOG_E("Tlt container %s", msg);
                ret = MC_DRV_ERR_CONTAINER_VERSION;
                break;
            }

            // Fill in SP container.
            mcSpid_t spid = tltCont->parent;
            p -= sizeof(mcSoSpCont_t);
            mcSoSpCont_t *soSp = (mcSoSpCont_t *)p;
            if (MC_DRV_OK != (ret = mcRegistryReadSp(spid, soSp))) {
                break;
            }
            mcSpCont_t *spCont = &soSp->cont;
            if (!checkVersionOkDataObjectCONTAINER(spCont->version, &msg)) {
                LOG_E("SP container %s", msg);
                ret = MC_DRV_ERR_CONTAINER_VERSION;
                break;
            }

            // Fill in root container.
            p -= sizeof(mcSoRootCont_t);
            mcSoRootCont_t *soRoot = (mcSoRootCont_t *)p;
            if (MC_DRV_OK != (ret = mcRegistryReadRoot(soRoot))) {
                break;
            }
            mcRootCont_t *rootCont = &soRoot->cont;
            if (!checkVersionOkDataObjectCONTAINER(rootCont->version, &msg)) {
                LOG_E("Root container %s", msg);
                ret = MC_DRV_ERR_CONTAINER_VERSION;
                break;
            }

            // Ensure order of elements in registry object value.
            assert(p - tlSize - sizeof(regObject_t) == (uint8_t *)regobj);
        } while (false);

        if (MC_DRV_OK != ret) {
            LOG_E("mcRegistryGetServiceBlob() failed: Error code: %d", ret);
            free(regobj);
            return NULL;
        }
        // Any other service type.
    } else {
        fclose(fs);
        LOG_E("mcRegistryGetServiceBlob() failed: Unsupported service type %u", serviceType);
    }

    return regobj;
}

//------------------------------------------------------------------------------
regObject_t *mcRegistryGetDriverBlob(
    const char *driverFilename
)
{
    regObject_t *regobj = NULL;

    // Open service blob file.
    FILE *fs = fopen(driverFilename, "rb");
    if (!fs) {
        LOG_E("mcRegistryGetDriverBlob() failed: cannot open %s", driverFilename);
        return NULL;
    }

    // Determine and check service blob size.
    fseek(fs, 0, SEEK_END);
    int32_t tlSize = ftell(fs);
    fseek(fs, 0, SEEK_SET);
    if (MAX_TL_SIZE < tlSize) {
        LOG_E("mcRegistryGetDriverBlob() failed: service blob too big: %d", tlSize);
        fclose(fs);
        return NULL;
    }

    // Check TL magic value.
    fseek(fs, offsetof(mclfIntro_t, magic), SEEK_SET);
    uint32_t magic;
    fread((char *)&magic, 1, sizeof(magic), fs);
    if (magic != MC_SERVICE_HEADER_MAGIC_BE) {
        LOG_E("mcRegistryGetDriverBlob() failed: wrong header magic value: %d", magic);
        fclose(fs);
        return NULL;
    }

    // Check header version.
    fseek(fs, offsetof(mclfIntro_t, version), SEEK_SET);
    uint32_t version;
    fread((char *)&version, 1, sizeof(version), fs);

    char *msg;
    if (!checkVersionOkDataObjectMCLF(version, &msg)) {
        LOG_E("%s", msg);
        fclose(fs);
        return NULL;
    }

    // Get service type.
    fseek(fs, offsetof(mclfHeaderV2_t, serviceType), SEEK_SET);
    serviceType_t serviceType;
    fread((char *)&serviceType, 1, sizeof(serviceType), fs);
    fseek(fs, 0, SEEK_SET);

    LOG_I("mcRegistryGetDriverBlob() Service is of type: %d", serviceType);

    // If loadable driver or system trustlet.
    if (SERVICE_TYPE_DRIVER == serviceType) {
        // Take trustlet blob 'as is'.
        if (NULL == (regobj = (regObject_t *) (malloc(sizeof(regObject_t) + tlSize)))) {
            LOG_E("mcRegistryGetDriverBlob() failed: Out of memory");
            fclose(fs);
            return NULL;
        }
        regobj->len = tlSize;
        fread((char *)regobj->value, 1, tlSize, fs);
        // Otherwise we are not interested
    } else {
        LOG_E("mcRegistryGetServiceBlob() failed: Unsupported service type %u", serviceType);
    }

    fclose(fs);

    return regobj;
}

//------------------------------------------------------------------------------
static const string getRegistryPath()
{
    const char *path;
    string registryPath;

    // First, attempt to use regular registry environment variable.
    path = getenv(ENV_MC_REGISTRY_PATH.c_str());
    if (doesDirExist(path)) {
        LOG_I("getRegistryPath(): Using MC_REGISTRY_PATH %s", path);
        registryPath = path;
    } else {
        // Second, attempt to use fallback registry environment variable.
        path = getenv(ENV_MC_REGISTRY_FALLBACK_PATH.c_str());
        if (doesDirExist(path)) {
            LOG_I("getRegistryPath(): Using MC_REGISTRY_FALLBACK_PATH %s", path);
            registryPath = path;
        }
    }

    // As a last resort, use the default registry path.
    if (registryPath.length() == 0) {
        registryPath = MC_REGISTRY_DEFAULT_PATH;
        LOG_I(" Using default registry path %s", registryPath.c_str());
    }

    assert(registryPath.length() != 0);

    return registryPath;
}

//------------------------------------------------------------------------------
static const string getAuthTokenFilePath()
{
    const char *path;
    string authTokenPath;

    // First, attempt to use regular auth token path environment variable.
    path = getenv(ENV_MC_AUTH_TOKEN_PATH.c_str());
    if (doesDirExist(path)) {
        LOG_I("getAuthTokenFilePath(): Using MC_AUTH_TOKEN_PATH %s", path);
        authTokenPath = path;
    } else {
        authTokenPath = getRegistryPath();
        LOG_I("getAuthTokenFilePath(): Using path %s", authTokenPath.c_str());
    }

    return authTokenPath + "/" + AUTH_TOKEN_FILE_NAME;
}

//------------------------------------------------------------------------------
static const string getRootContFilePath()
{
    return getRegistryPath() + "/" + ROOT_FILE_NAME;
}

//------------------------------------------------------------------------------
static const string getSpDataPath(mcSpid_t spid)
{
    return getRegistryPath() + "/" + uint32ToString(spid);
}

//------------------------------------------------------------------------------
static const string getSpContFilePath(mcSpid_t spid)
{
    return getRegistryPath() + "/" + uint32ToString(spid) + SP_CONT_FILE_EXT;
}

//------------------------------------------------------------------------------
static const string getTlContFilePath(const mcUuid_t *uuid)
{
    return getRegistryPath() + "/" + byteArrayToString(uuid, sizeof(*uuid)) + TL_CONT_FILE_EXT;
}

//------------------------------------------------------------------------------
static const string getTlDataPath(const mcUuid_t *uuid)
{
    return getRegistryPath() + "/" + byteArrayToString(uuid, sizeof(*uuid));
}

//------------------------------------------------------------------------------
static const string getTlDataFilePath(const mcUuid_t *uuid, mcPid_t pid)
{
    return getTlDataPath(uuid) + "/" + uint32ToString(pid.data) + DATA_CONT_FILE_EXT;
}

//------------------------------------------------------------------------------
static const string getTlBinFilePath(const mcUuid_t *uuid)
{
    return getRegistryPath() + "/" + byteArrayToString(uuid, sizeof(*uuid)) + TL_BIN_FILE_EXT;
}

//------------------------------------------------------------------------------
static const string byteArrayToString(const void *bytes, size_t elems)
{
    char hx[elems * 2 + 1];

    for (size_t i = 0; i < elems; i++) {
        sprintf(&hx[i * 2], "%02x", ((uint8_t *)bytes)[i]);
    }
    return string(hx);
}

//------------------------------------------------------------------------------
static const string uint32ToString(
    uint32_t value
)
{
    char hx[sizeof(uint32_t) * 2 + 1];
    uint32_t i;

    for (i = 0; i < (2 * sizeof(value)); i++) {
        hx[i] = (value >> (28 - (i * 4))) & 0x0F;
        if (hx[i] > 9) {
            hx[i] = (hx[i] - 9) | 0x40;
        } else {
            hx[i] |= 0x30;
        }
    }
    hx[i] = '\0';
    return string(hx);
}

//------------------------------------------------------------------------------
static bool doesDirExist(const char *path)
{
    struct stat ss;
    if (path != NULL && stat(path, &ss) == 0 && S_ISDIR(ss.st_mode)) {
        return true;
    }
    return false;
}


/** @} */
