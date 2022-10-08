/**
 * @file   tlcTeeKeymaster_if.c
 * @brief  Contains trustlet connector interface implementations to
 * handle key operations with TEE Keymaster trustlet
 *
 * Copyright Giesecke & Devrient GmbH 2012
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

#include <stdlib.h>

#include "MobiCoreDriverApi.h"
#include "tlTeeKeymaster_Api.h"
#include "tlcTeeKeymaster_log.h"
#include "tlcTeeKeymaster_if.h"


/* Global definitions */
static const uint32_t DEVICE_ID = MC_DEVICE_ID_DEFAULT;
static const mcUuid_t uuid = TEE_KEYMASTER_TL_UUID;

/**
 * TEE_Open
 *
 * Open session to the TEE Keymaster trustlet
 *
 * @param  pSessionHandle  [out] Return pointer to the session handle
 */
static tciMessage_ptr TEE_Open(
    mcSessionHandle_t *pSessionHandle
){
    tciMessage_ptr pTci = NULL;
    mcResult_t     mcRet;

    do
    {

        /* Validate session handle */
        if (!pSessionHandle)
        {
            LOG_E("TEE_Open(): Invalid session handle\n");
            break;
        }

        /* Initialize session handle data */
        bzero(pSessionHandle, sizeof(mcSessionHandle_t));

        /* Open MobiCore device */
        mcRet = mcOpenDevice(DEVICE_ID);
        if (MC_DRV_OK != mcRet)
        {
            LOG_E("TEE_Open(): mcOpenDevice returned: %d\n", mcRet);
            break;
        }

        /* Allocating WSM for TCI */
        mcRet = mcMallocWsm(DEVICE_ID, 0, sizeof(tciMessage_t), (uint8_t **) &pTci, 0);
        if (MC_DRV_OK != mcRet)
        {
            LOG_E("TEE_Open(): mcMallocWsm returned: %d\n", mcRet);
            break;
        }

        /* Open session the TEE Keymaster trustlet */
        pSessionHandle->deviceId = DEVICE_ID;
        mcRet = mcOpenSession(pSessionHandle,
                              &uuid,
                              (uint8_t *) pTci,
                              (uint32_t) sizeof(tciMessage_t));
        if (MC_DRV_OK != mcRet)
        {
            LOG_E("TEE_Open(): mcOpenSession returned: %d\n", mcRet);
            break;
        }

    } while (false);

    return pTci;
}


/**
 * TEE_Close
 *
 * Close session to the TEE Keymaster trustlet
 *
 * @param  sessionHandle  [in] Session handle
 */
static void TEE_Close(
    mcSessionHandle_t sessionHandle
){
    teeResult_t   ret = TEE_ERR_NONE;
    mcResult_t    mcRet;

    do {

        /* Close session */
        mcRet = mcCloseSession(&sessionHandle);
        if (MC_DRV_OK != mcRet)
        {
            LOG_E("TEE_Close(): mcCloseSession returned: %d\n", mcRet);
            ret = TEE_ERR_SESSION;
            break;
        }

        /* Close MobiCore device */
        mcRet = mcCloseDevice(DEVICE_ID);
        if (MC_DRV_OK != mcRet)
        {
            LOG_E("TEE_Close(): mcCloseDevice returned: %d\n", mcRet);
            ret = TEE_ERR_MC_DEVICE;
        }

    } while (false);
}


/**
 * TEE_RSAGenerateKeyPair
 *
 * Generates RSA key pair and returns key pair data as wrapped object
 *
 * @param  keyType        [in]  Key pair type. RSA or RSACRT
 * @param  keyData        [in]  Pointer to the key data buffer
 * @param  keyDataLength  [in]  Key data buffer length
 * @param  keySize        [in]  Key size
 * @param  exponent       [in]  Exponent number
 * @param  soLen          [out] Key data secure object length
 */
teeResult_t TEE_RSAGenerateKeyPair(
    teeRsaKeyPairType_t keyType,
    uint8_t*            keyData,
    uint32_t            keyDataLength,
    uint32_t            keySize,
    uint32_t            exponent,
    uint32_t*           soLen
){
    teeResult_t         ret = TEE_ERR_NONE;
    tciMessage_ptr      pTci = NULL;
    mcSessionHandle_t   sessionHandle;
    mcBulkMap_t         mapInfo;
    mcResult_t          mcRet;

    do {

        /* Open session to the trustlet */
        pTci = TEE_Open(&sessionHandle);
        if (!pTci) {
            ret = TEE_ERR_MEMORY;
            break;
        }

        /* Map memory to the secure world */
        mcRet = mcMap(&sessionHandle, keyData, keyDataLength, &mapInfo);
        if (MC_DRV_OK != mcRet) {
            ret = TEE_ERR_MAP;
            break;
        }

        /* Update TCI buffer */
        pTci->command.header.commandId = CMD_ID_TEE_RSA_GEN_KEY_PAIR;
        pTci->rsagenkey.type        = keyType;
        pTci->rsagenkey.keysize     = keySize;
        pTci->rsagenkey.keydata     = (uint32_t)mapInfo.sVirtualAddr;
        pTci->rsagenkey.keydatalen  = keyDataLength;
        pTci->rsagenkey.exponent    = exponent;

        /* Notify the trustlet */
        mcRet = mcNotify(&sessionHandle);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_NOTIFICATION;
            break;
        }

        /* Wait for response from the trustlet */
        if (MC_DRV_OK != mcWaitNotification(&sessionHandle, MC_INFINITE_TIMEOUT))
        {
            ret = TEE_ERR_NOTIFICATION;
            break;
        }

        /* Unmap memory */
        mcRet = mcUnmap(&sessionHandle, keyData, &mapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        if (RET_OK != pTci->response.header.returnCode)
        {
            LOG_E("TEE_RSAGenerateKeyPair(): TEE Keymaster trustlet returned: 0x%.8x\n",
                        pTci->response.header.returnCode);
            ret = TEE_ERR_FAIL;
            break;
        }

        /* Update secure object length */
        *soLen =  pTci->rsagenkey.solen;

    } while (false);

    /* Close session to the trustlet */
    TEE_Close(sessionHandle);

    return ret;
}


/**
 * TEE_RSASign
 *
 * Signs given plain data and returns signature data
 *
 * @param  keyData          [in]  Pointer to key data buffer
 * @param  keyDataLength    [in]  Key data buffer length
 * @param  plainData        [in]  Pointer to plain data to be signed
 * @param  plainDataLength  [in]  Plain data length
 * @param  signatureData    [out] Pointer to signature data
 * @param  signatureDataLength  [out] Signature data length
 * @param  algorithm        [in]  RSA signature algorithm
 */
teeResult_t TEE_RSASign(
    const uint8_t*  keyData,
    const uint32_t  keyDataLength,
    const uint8_t*  plainData,
    const uint32_t  plainDataLength,
    uint8_t*        signatureData,
    uint32_t*       signatureDataLength,
    teeRsaSigAlg_t  algorithm
){
    teeResult_t        ret = TEE_ERR_NONE;
    tciMessage_ptr     pTci = NULL;
    mcSessionHandle_t  sessionHandle;
    mcBulkMap_t        keyMapInfo;
    mcBulkMap_t        plainMapInfo;
    mcBulkMap_t        signatureMapInfo;
    mcResult_t         mcRet;

    do {

        /* Open session to the trustlet */
        pTci = TEE_Open(&sessionHandle);
        if (!pTci) {
            ret = TEE_ERR_MEMORY;
            break;
        }

        /* Map memory to the secure world */
        mcRet = mcMap(&sessionHandle, (void*)keyData, keyDataLength, &keyMapInfo);
        if (MC_DRV_OK != mcRet) {
            ret = TEE_ERR_MAP;
            break;
        }

        mcRet = mcMap(&sessionHandle, (void*)plainData, plainDataLength, &plainMapInfo);
        if (MC_DRV_OK != mcRet) {
            ret = TEE_ERR_MAP;
            break;
        }

        mcRet = mcMap(&sessionHandle, (void*)signatureData, *signatureDataLength, &signatureMapInfo);
        if (MC_DRV_OK != mcRet) {
            ret = TEE_ERR_MAP;
            break;
        }

        /* Update TCI buffer */
        pTci->command.header.commandId = CMD_ID_TEE_RSA_SIGN;
        pTci->rsasign.keydata = (uint32_t)keyMapInfo.sVirtualAddr;
        pTci->rsasign.keydatalen = keyDataLength;

        pTci->rsasign.plaindata = (uint32_t)plainMapInfo.sVirtualAddr;
        pTci->rsasign.plaindatalen = plainDataLength;

        pTci->rsasign.signaturedata = (uint32_t)signatureMapInfo.sVirtualAddr;
        pTci->rsasign.signaturedatalen = *signatureDataLength;

        pTci->rsasign.algorithm = algorithm;

        /* Notify the trustlet */
        mcRet = mcNotify(&sessionHandle);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_NOTIFICATION;
            break;
        }

        /* Wait for response from the trustlet */
        if (MC_DRV_OK != mcWaitNotification(&sessionHandle, MC_INFINITE_TIMEOUT))
        {
            ret = TEE_ERR_NOTIFICATION;
            break;
        }

        /* Unmap memory */
        mcRet = mcUnmap(&sessionHandle, (void*)keyData, &keyMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        mcRet = mcUnmap(&sessionHandle, (void*)plainData, &plainMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        mcRet = mcUnmap(&sessionHandle, (void*)signatureData, &signatureMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        if (RET_OK != pTci->response.header.returnCode)
        {
            LOG_E("TEE_RSASign(): TEE Keymaster trustlet returned: 0x%.8x\n",
                        pTci->response.header.returnCode);
            ret = TEE_ERR_FAIL;
            break;
        }

        /* Retrieve signature data length */
        *signatureDataLength = pTci->rsasign.signaturedatalen;

    } while (false);

    /* Close session to the trustlet */
    TEE_Close(sessionHandle);

    return ret;
}


/**
 * TEE_RSAVerify
 *
 * Verifies given data with RSA public key and return status
 *
 * @param  keyData          [in]  Pointer to key data buffer
 * @param  keyDataLength    [in]  Key data buffer length
 * @param  plainData        [in]  Pointer to plain data to be signed
 * @param  plainDataLength  [in]  Plain data length
 * @param  signatureData    [in]  Pointer to signed data
 * @param  signatureData    [in]  Plain  data length
 * @param  algorithm        [in]  RSA signature algorithm
 * @param  validity         [out] Signature validity
 */
teeResult_t TEE_RSAVerify(
    const uint8_t*  keyData,
    const uint32_t  keyDataLength,
    const uint8_t*  plainData,
    const uint32_t  plainDataLength,
    const uint8_t*  signatureData,
    const uint32_t  signatureDataLength,
    teeRsaSigAlg_t  algorithm,
    bool            *validity
){
    teeResult_t        ret = TEE_ERR_NONE;
    tciMessage_ptr     pTci = NULL;
    mcSessionHandle_t  sessionHandle;
    mcBulkMap_t        keyMapInfo;
    mcBulkMap_t        plainMapInfo;
    mcBulkMap_t        signatureMapInfo;
    mcResult_t         mcRet;

    do {

        /* Open session to the trustlet */
        pTci = TEE_Open(&sessionHandle);
        if (!pTci) {
            ret = TEE_ERR_MEMORY;
            break;
        }

        /* Map memory to the secure world */
        mcRet = mcMap(&sessionHandle, (void*)keyData, keyDataLength, &keyMapInfo);
        if (MC_DRV_OK != mcRet) {
            ret = TEE_ERR_MAP;
            break;
        }

        mcRet = mcMap(&sessionHandle, (void*)plainData, plainDataLength, &plainMapInfo);
        if (MC_DRV_OK != mcRet) {
            ret = TEE_ERR_MAP;
            break;
        }

        mcRet = mcMap(&sessionHandle, (void*)signatureData, signatureDataLength, &signatureMapInfo);
        if (MC_DRV_OK != mcRet) {
            ret = TEE_ERR_MAP;
            break;
        }

        /* Update TCI buffer */
        pTci->command.header.commandId = CMD_ID_TEE_RSA_VERIFY;
        pTci->rsaverify.keydata = (uint32_t)keyMapInfo.sVirtualAddr;
        pTci->rsaverify.keydatalen = keyDataLength;

        pTci->rsaverify.plaindata = (uint32_t)plainMapInfo.sVirtualAddr;
        pTci->rsaverify.plaindatalen = plainDataLength;

        pTci->rsaverify.signaturedata = (uint32_t)signatureMapInfo.sVirtualAddr;
        pTci->rsaverify.signaturedatalen = signatureDataLength;

        pTci->rsaverify.algorithm = algorithm;
        pTci->rsaverify.validity = false;

        /* Notify the trustlet */
        mcRet = mcNotify(&sessionHandle);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_NOTIFICATION;
            break;
        }

        /* Wait for response from the trustlet */
        if (MC_DRV_OK != mcWaitNotification(&sessionHandle, MC_INFINITE_TIMEOUT))
        {
            ret = TEE_ERR_NOTIFICATION;
            break;
        }

        /* Unmap memory */
        mcRet = mcUnmap(&sessionHandle, (void*)keyData, &keyMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        mcRet = mcUnmap(&sessionHandle, (void*)plainData, &plainMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        mcRet = mcUnmap(&sessionHandle, (void*)signatureData, &signatureMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        if (RET_OK != pTci->response.header.returnCode)
        {
            LOG_E("TEE_RSAVerify(): TEE Keymaster trustlet returned: 0x%.8x\n",
                        pTci->response.header.returnCode);
            ret = TEE_ERR_FAIL;
            break;
        }

        *validity =  pTci->rsaverify.validity;

    } while (false);

    /* Close session to the trustlet */
    TEE_Close(sessionHandle);

    return ret;
}


/**
 * TEE_HMACKeyGenerate
 *
 * Generates random key for HMAC calculation and returns key data as wrapped object
 * (key is encrypted)
 *
 * @param  keyData        [out] Pointer to key data
 * @param  keyDataLength  [in]  Key data buffer length
 * @param  soLen          [out] Key data secure object length
 */
teeResult_t TEE_HMACKeyGenerate(
    uint8_t*  keyData,
    uint32_t  keyDataLength,
    uint32_t* soLen
){
    teeResult_t        ret = TEE_ERR_NONE;
    tciMessage_ptr     pTci = NULL;
    mcSessionHandle_t  sessionHandle;
    mcBulkMap_t        keyMapInfo;
    mcResult_t         mcRet;

    do {

        /* Open session to the trustlet */
        pTci = TEE_Open(&sessionHandle);
        if (!pTci) {
            ret = TEE_ERR_MEMORY;
            break;
        }

        /* Map memory to the secure world */
        mcRet = mcMap(&sessionHandle, (void*)keyData, keyDataLength, &keyMapInfo);
        if (MC_DRV_OK != mcRet) {
            ret = TEE_ERR_MAP;
            break;
        }

        /* Update TCI buffer */
        pTci->command.header.commandId = CMD_ID_TEE_HMAC_GEN_KEY;
        pTci->hmacgenkey.keydata = (uint32_t)keyMapInfo.sVirtualAddr;
        pTci->hmacgenkey.keydatalen = keyDataLength;

        /* Notify the trustlet */
        mcRet = mcNotify(&sessionHandle);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_NOTIFICATION;
            break;
        }

        /* Wait for response from the trustlet */
        if (MC_DRV_OK != mcWaitNotification(&sessionHandle, MC_INFINITE_TIMEOUT))
        {
            ret = TEE_ERR_NOTIFICATION;
            break;
        }

        /* Unmap memory */
        mcRet = mcUnmap(&sessionHandle, (void*)keyData, &keyMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        if (RET_OK != pTci->response.header.returnCode)
        {
            LOG_E("TEE_RSAVerify(): TEE Keymaster trustlet returned: 0x%.8x\n",
                        pTci->response.header.returnCode);
            ret = TEE_ERR_FAIL;
        }

        /* Update secure object length */
        *soLen =  pTci->hmacgenkey.solen;

    }while (false);

    /* Close session to the trustlet */
    TEE_Close(sessionHandle);

    return ret;
}

/**
 * TEE_HMACSign
 *
 * Signs given plain data and returns HMAC signature data
 *
 * @param  keyData          [in]  Pointer to key data buffer
 * @param  keyDataLength    [in]  Key data buffer length
 * @param  plainData        [in]  Pointer to plain data to be signed
 * @param  plainDataLength  [in]  Plain data length
 * @param  signatureData    [out] Pointer to signature data
 * @param  signatureDataLength  [out] Signature data length
 * @param  digest           [in]  Digest type
 */
teeResult_t TEE_HMACSign(
    const uint8_t*  keyData,
    const uint32_t  keyDataLength,
    const uint8_t*  plainData,
    const uint32_t  plainDataLength,
    uint8_t*        signatureData,
    uint32_t*       signatureDataLength,
    teeDigest_t     digest
){
    teeResult_t        ret = TEE_ERR_NONE;
    tciMessage_ptr     pTci = NULL;
    mcSessionHandle_t  sessionHandle;
    mcBulkMap_t        keyMapInfo;
    mcBulkMap_t        plainMapInfo;
    mcBulkMap_t        signatureMapInfo;
    mcResult_t         mcRet;

    do {

        /* Open session to the trustlet */
        pTci = TEE_Open(&sessionHandle);
        if (!pTci) {
            ret = TEE_ERR_MEMORY;
            break;
        }

        /* Map memory to the secure world */
        mcRet = mcMap(&sessionHandle, (void*)keyData, keyDataLength, &keyMapInfo);
        if (MC_DRV_OK != mcRet) {
            ret = TEE_ERR_MAP;
            break;
        }

        mcRet = mcMap(&sessionHandle, (void*)plainData, plainDataLength, &plainMapInfo);
        if (MC_DRV_OK != mcRet) {
            ret = TEE_ERR_MAP;
            break;
        }

        mcRet = mcMap(&sessionHandle, (void*)signatureData, *signatureDataLength, &signatureMapInfo);
        if (MC_DRV_OK != mcRet) {
            ret = TEE_ERR_MAP;
            break;
        }

        /* Update TCI buffer */
        pTci->command.header.commandId = CMD_ID_TEE_HMAC_SIGN;
        pTci->hmacsign.keydata = (uint32_t)keyMapInfo.sVirtualAddr;
        pTci->hmacsign.keydatalen = keyDataLength;

        pTci->hmacsign.plaindata = (uint32_t)plainMapInfo.sVirtualAddr;
        pTci->hmacsign.plaindatalen = plainDataLength;

        pTci->hmacsign.signaturedata = (uint32_t)signatureMapInfo.sVirtualAddr;
        pTci->hmacsign.signaturedatalen = *signatureDataLength;

        pTci->hmacsign.digest = digest;

        /* Notify the trustlet */
        mcRet = mcNotify(&sessionHandle);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_NOTIFICATION;
            break;
        }

        /* Wait for response from the trustlet */
        if (MC_DRV_OK != mcWaitNotification(&sessionHandle, MC_INFINITE_TIMEOUT))
        {
            ret = TEE_ERR_NOTIFICATION;
            break;
        }

        /* Unmap memory */
        mcRet = mcUnmap(&sessionHandle, (void*)keyData, &keyMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        mcRet = mcUnmap(&sessionHandle, (void*)plainData, &plainMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        mcRet = mcUnmap(&sessionHandle, (void*)signatureData, &signatureMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        if (RET_OK != pTci->response.header.returnCode)
        {
            LOG_E("TEE_HMACSign(): TEE Keymaster trustlet returned: 0x%.8x\n",
                        pTci->response.header.returnCode);
            ret = TEE_ERR_FAIL;
            break;
        }

        /* Retrieve signature data length */
        *signatureDataLength = pTci->hmacsign.signaturedatalen;

    } while (false);

    /* Close session to the trustlet */
    TEE_Close(sessionHandle);

    return ret;
}


/**
 * TEE_HMACVerify
 *
 * Verifies given data HMAC key data and return status
 *
 * @param  plainData        [in]  Pointer to plain data to be signed
 * @param  plainDataLength  [in]  Plain data length
 * @param  signatureData    [in]  Pointer to signed data
 * @param  signatureData    [in]  Plain  data length
 * @param  digest           [in]  Digest type
 * @param  validity         [out] Signature validity
 */
teeResult_t TEE_HMACVerify(
    const uint8_t*  keyData,
    const uint32_t  keyDataLength,
    const uint8_t*  plainData,
    const uint32_t  plainDataLength,
    const uint8_t*  signatureData,
    const uint32_t  signatureDataLength,
    teeDigest_t     digest,
    bool            *validity
){
    teeResult_t        ret = TEE_ERR_NONE;
    tciMessage_ptr     pTci = NULL;
    mcSessionHandle_t  sessionHandle;
    mcBulkMap_t        keyMapInfo;
    mcBulkMap_t        plainMapInfo;
    mcBulkMap_t        signatureMapInfo;
    mcResult_t         mcRet;

    do {

        /* Open session to the trustlet */
        pTci = TEE_Open(&sessionHandle);
        if (!pTci) {
            ret = TEE_ERR_MEMORY;
            break;
        }

        /* Map memory to the secure world */
        mcRet = mcMap(&sessionHandle, (void*)keyData, keyDataLength, &keyMapInfo);
        if (MC_DRV_OK != mcRet) {
            ret = TEE_ERR_MAP;
            break;
        }

        mcRet = mcMap(&sessionHandle, (void*)plainData, plainDataLength, &plainMapInfo);
        if (MC_DRV_OK != mcRet) {
            ret = TEE_ERR_MAP;
            break;
        }

        mcRet = mcMap(&sessionHandle, (void*)signatureData, signatureDataLength, &signatureMapInfo);
        if (MC_DRV_OK != mcRet) {
            ret = TEE_ERR_MAP;
            break;
        }

        /* Update TCI buffer */
        pTci->command.header.commandId = CMD_ID_TEE_HMAC_VERIFY;
        pTci->hmacverify.keydata = (uint32_t)keyMapInfo.sVirtualAddr;
        pTci->hmacverify.keydatalen = keyDataLength;

        pTci->hmacverify.plaindata = (uint32_t)plainMapInfo.sVirtualAddr;
        pTci->hmacverify.plaindatalen = plainDataLength;

        pTci->hmacverify.signaturedata = (uint32_t)signatureMapInfo.sVirtualAddr;
        pTci->hmacverify.signaturedatalen = signatureDataLength;

        pTci->hmacverify.digest = digest;
        pTci->hmacverify.validity = false;

        /* Notify the trustlet */
        mcRet = mcNotify(&sessionHandle);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_NOTIFICATION;
            break;
        }

        /* Wait for response from the trustlet */
        if (MC_DRV_OK != mcWaitNotification(&sessionHandle, MC_INFINITE_TIMEOUT))
        {
            ret = TEE_ERR_NOTIFICATION;
            break;
        }

        /* Unmap memory */
        mcRet = mcUnmap(&sessionHandle, (void*)keyData, &keyMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        mcRet = mcUnmap(&sessionHandle, (void*)plainData, &plainMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        mcRet = mcUnmap(&sessionHandle, (void*)signatureData, &signatureMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        if (RET_OK != pTci->response.header.returnCode)
        {
            LOG_E("TEE_HMACVerify(): TEE Keymaster trustlet returned: 0x%.8x\n",
                        pTci->response.header.returnCode);
            ret = TEE_ERR_FAIL;
            break;
        }

        *validity =  pTci->hmacverify.validity;

    } while (false);

    /* Close session to the trustlet */
    TEE_Close(sessionHandle);

    return ret;
}


/**
 * TEE_KeyImport
 *
 * Imports key data and returns key data as secure object
 *
 * Key data needs to be in the following format
 *
 * RSA key data:
 * |--key metadata--|--public modulus--|--public exponent--|--private exponent--|
 *
 * RSA CRT key data:
 * |--key metadata--|--public modulus--|--public exponent--|--P--|--Q--|--DP--|--DQ--|--Qinv--|
 *
 * Where:
 * P:     secret prime factor
 * Q:     secret prime factor
 * DP:    d mod (p-1)
 * DQ:    d mod (q-1)
 * Qinv:  q^-1 mod p
 *
 * @param  keyData          [in]  Pointer to key data
 * @param  keyDataLength    [in]  Key data length
 * @param  soData           [out] Pointer to wrapped key data
 * @param  soDataLength     [out] Wrapped key data length
 */
teeResult_t TEE_KeyImport(
    const uint8_t*  keyData,
    const uint32_t  keyDataLength,
    uint8_t*        soData,
    uint32_t*       soDataLength
){
    teeResult_t         ret = TEE_ERR_NONE;
    tciMessage_ptr      pTci = NULL;
    mcSessionHandle_t   sessionHandle;
    mcBulkMap_t         keyMapInfo;
    mcBulkMap_t         soMapInfo;
    mcResult_t          mcRet;

    do {

        /* Open session to the trustlet */
        pTci = TEE_Open(&sessionHandle);
        if (!pTci) {
            ret = TEE_ERR_MEMORY;
            break;
        }

        /* Map memory to the secure world */
        mcRet = mcMap(&sessionHandle, (void*)keyData, keyDataLength, &keyMapInfo);
        if (MC_DRV_OK != mcRet) {
            ret = TEE_ERR_MAP;
            break;
        }

        mcRet = mcMap(&sessionHandle, (void*)soData, *soDataLength, &soMapInfo);
        if (MC_DRV_OK != mcRet) {
            ret = TEE_ERR_MAP;
            break;
        }

        /* Update TCI buffer */
        pTci->command.header.commandId = CMD_ID_TEE_KEY_IMPORT;
        pTci->keyimport.keydata        = (uint32_t)keyMapInfo.sVirtualAddr;
        pTci->keyimport.keydatalen     = keyDataLength;
        pTci->keyimport.sodata         = (uint32_t)soMapInfo.sVirtualAddr;
        pTci->keyimport.sodatalen      = *soDataLength;

        /* Notify the trustlet */
        mcRet = mcNotify(&sessionHandle);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_NOTIFICATION;
            break;
        }

        /* Wait for response from the trustlet */
        if (MC_DRV_OK != mcWaitNotification(&sessionHandle, MC_INFINITE_TIMEOUT))
        {
            ret = TEE_ERR_NOTIFICATION;
            break;
        }

        /* Unmap memory */
        mcRet = mcUnmap(&sessionHandle, (void*)keyData, &keyMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        mcRet = mcUnmap(&sessionHandle, (void*)soData, &soMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        if (RET_OK != pTci->response.header.returnCode)
        {
            LOG_E("TEE_KeyWrap(): TEE Keymaster trustlet returned: 0x%.8x\n",
                        pTci->response.header.returnCode);
            ret = TEE_ERR_FAIL;
            break;
        }

        /* Update secure object length */
        *soDataLength =  pTci->keyimport.sodatalen;

    } while (false);

    /* Close session to the trustlet */
    TEE_Close(sessionHandle);

    return ret;
}


/** * TEE_GetPubKey
 *
 * Retrieves public key daya (modulus and exponent) from wrapped key data
 *
 * @param  keyData          [in]  Pointer to key data
 * @param  keyDataLength    [in]  Key data length
 * @param  modulus          [out] Pointer to public key modulus data
 * @param  modulusLength    [out] Modulus data length
 * @param  exponent         [out] Pointer to public key exponent data
 * @param  exponentLength   [out] Exponent data length
 */
teeResult_t TEE_GetPubKey(
    const uint8_t*  keyData,
    const uint32_t  keyDataLength,
    uint8_t*        modulus,
    uint32_t*       modulusLength,
    uint8_t*        exponent,
    uint32_t*       exponentLength
){
    teeResult_t         ret = TEE_ERR_NONE;
    tciMessage_ptr      pTci = NULL;
    mcSessionHandle_t   sessionHandle;
    mcBulkMap_t         keyMapInfo;
    mcBulkMap_t         modMapInfo;
    mcBulkMap_t         expMapInfo;
    mcResult_t          mcRet;

    do {

        /* Open session to the trustlet */
        pTci = TEE_Open(&sessionHandle);
        if (!pTci) {
            ret = TEE_ERR_MEMORY;
            break;
        }

        /* Map memory to the secure world */
        mcRet = mcMap(&sessionHandle, (void*)keyData, keyDataLength, &keyMapInfo);
        if (MC_DRV_OK != mcRet) {
            ret = TEE_ERR_MAP;
            break;
        }

        mcRet = mcMap(&sessionHandle, (void*)modulus, *modulusLength, &modMapInfo);
        if (MC_DRV_OK != mcRet) {
            ret = TEE_ERR_MAP;
            break;
        }

        mcRet = mcMap(&sessionHandle, (void*)exponent, *exponentLength, &expMapInfo);
        if (MC_DRV_OK != mcRet) {
            ret = TEE_ERR_MAP;
            break;
        }

        /* Update TCI buffer */
        pTci->command.header.commandId = CMD_ID_TEE_GET_PUB_KEY;
        pTci->getpubkey.keydata        = (uint32_t)keyMapInfo.sVirtualAddr;
        pTci->getpubkey.keydatalen     = keyDataLength;
        pTci->getpubkey.modulus        = (uint32_t)modMapInfo.sVirtualAddr;
        pTci->getpubkey.moduluslen     = *modulusLength;
        pTci->getpubkey.exponent       = (uint32_t)expMapInfo.sVirtualAddr;
        pTci->getpubkey.exponentlen    = *exponentLength;

        /* Notify the trustlet */
        mcRet = mcNotify(&sessionHandle);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_NOTIFICATION;
            break;
        }

        /* Wait for response from the trustlet */
        if (MC_DRV_OK != mcWaitNotification(&sessionHandle, MC_INFINITE_TIMEOUT))
        {
            ret = TEE_ERR_NOTIFICATION;
            break;
        }

        /* Unmap memory */
        mcRet = mcUnmap(&sessionHandle, (void*)keyData, &keyMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        mcRet = mcUnmap(&sessionHandle, (void*)modulus, &modMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        mcRet = mcUnmap(&sessionHandle, (void*)exponent, &expMapInfo);
        if (MC_DRV_OK != mcRet)
        {
            ret = TEE_ERR_MAP;
            break;
        }

        if (RET_OK != pTci->response.header.returnCode)
        {
            LOG_E("TEE_GetPubKey(): TEE Keymaster trustlet returned: 0x%.8x\n",
                        pTci->response.header.returnCode);
            ret = TEE_ERR_FAIL;
            break;
        }

        /* Update  modulus and exponent lengths */
        *modulusLength =   pTci->getpubkey.moduluslen;
        *exponentLength =   pTci->getpubkey.exponentlen;

    } while (false);

    /* Close session to the trustlet */
    TEE_Close(sessionHandle);

    return ret;
}
