/*
 *  Copyright (C) 2012 The Android Open Source Project
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you
 *  may not use this file except in compliance with the License.  You may
 *  obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 *  implied.  See the License for the specific language governing
 *  permissions and limitations under the License.
 */

#include <errno.h>
#include <string.h>
#include <stdint.h>

#include <hardware/hardware.h>
#include <hardware/keymaster.h>

#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/rsa.h>
#include <openssl/err.h>
#include <openssl/x509.h>

#include <linux/ioctl.h>
#include <linux/msm_ion.h>
#include <sys/mman.h>

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <dlfcn.h>

#include <UniquePtr.h>

#include "QSEEComAPI.h"
#include "keymaster_qcom.h"

// For debugging
//#define LOG_NDEBUG 0

#define LOG_TAG "QCOMKeyMaster"
#include <cutils/log.h>
struct qcom_km_ion_info_t {
    int32_t ion_fd;
    int32_t ifd_data_fd;
    struct ion_handle_data ion_alloc_handle;
    unsigned char * ion_sbuffer;
    uint32_t sbuf_len;
};

struct qcom_keymaster_handle {
    struct QSEECom_handle *qseecom;
    void *libhandle;
    int (*QSEECom_start_app)(struct QSEECom_handle ** handle, char* path,
                          char* appname, uint32_t size);
    int (*QSEECom_shutdown_app)(struct QSEECom_handle **handle);
    int (*QSEECom_send_cmd)(struct QSEECom_handle* handle, void *cbuf,
                          uint32_t clen, void *rbuf, uint32_t rlen);
    int (*QSEECom_send_modified_cmd)(struct QSEECom_handle* handle, void *cbuf,
                          uint32_t clen, void *rbuf, uint32_t rlen,
                          struct QSEECom_ion_fd_info *ihandle);
    int (*QSEECom_set_bandwidth)(struct QSEECom_handle* handle, bool high);
};
typedef struct qcom_keymaster_handle qcom_keymaster_handle_t;

struct EVP_PKEY_Delete {
    void operator()(EVP_PKEY* p) const {
        EVP_PKEY_free(p);
    }
};
typedef UniquePtr<EVP_PKEY, EVP_PKEY_Delete> Unique_EVP_PKEY;

struct RSA_Delete {
    void operator()(RSA* p) const {
        RSA_free(p);
    }
};
typedef UniquePtr<RSA, RSA_Delete> Unique_RSA;

typedef UniquePtr<keymaster_device_t> Unique_keymaster_device_t;

/**
 * Many OpenSSL APIs take ownership of an argument on success but don't free the argument
 * on failure. This means we need to tell our scoped pointers when we've transferred ownership,
 * without triggering a warning by not using the result of release().
 */
#define OWNERSHIP_TRANSFERRED(obj) \
    typeof (obj.release()) _dummy __attribute__((unused)) = obj.release()

static int qcom_km_get_keypair_public(const keymaster_device* dev,
        const uint8_t* keyBlob, const size_t keyBlobLength,
        uint8_t** x509_data, size_t* x509_data_length) {

    struct qcom_km_key_blob * keyblob_ptr = (struct qcom_km_key_blob *)keyBlob;

    if (x509_data == NULL || x509_data_length == NULL) {
        ALOGE("Output public key buffer == NULL");
        return -1;
    }

    if (keyBlob == NULL) {
        ALOGE("Supplied key blob was NULL");
        return -1;
    }

    // Should be large enough for keyblob data:
    if (keyBlobLength < (sizeof(qcom_km_key_blob_t))) {
        ALOGE("key blob appears to be truncated");
        return -1;
    }

    if (keyblob_ptr->magic_num != KM_MAGIC_NUM) {
        ALOGE("Cannot read key; it was not made by this keymaster");
        return -1;
    }

    if (keyblob_ptr->public_exponent_size == 0 ) {
        ALOGE("Key blob appears to have incorrect exponent length");
        return -1;
    }
    if (keyblob_ptr->modulus_size == 0 ) {
        ALOGE("Key blob appears to have incorrect modulus length");
        return -1;
    }

    Unique_RSA rsa(RSA_new());
    if (rsa.get() == NULL) {
        ALOGE("Could not allocate RSA structure");
        return -1;
    }

    rsa->n = BN_bin2bn(reinterpret_cast<const unsigned char*>(keyblob_ptr->modulus),
                               keyblob_ptr->modulus_size, NULL);
    if (rsa->n == NULL) {
       ALOGE("Failed to initialize  modulus");
        return -1;
    }

    rsa->e = BN_bin2bn(reinterpret_cast<const unsigned char*>(&keyblob_ptr->public_exponent),
                               keyblob_ptr->public_exponent_size, NULL);
    if (rsa->e == NULL) {
        ALOGE("Failed to initialize public exponent");
        return -1;
    }

    Unique_EVP_PKEY pkey(EVP_PKEY_new());
    if (pkey.get() == NULL) {
        ALOGE("Could not allocate EVP_PKEY structure");
        return -1;
    }
    if (EVP_PKEY_assign_RSA(pkey.get(), rsa.get()) != 1) {
        ALOGE("Failed to assign rsa  parameters \n");
        return -1;
    }
    OWNERSHIP_TRANSFERRED(rsa);

    int len = i2d_PUBKEY(pkey.get(), NULL);
    if (len <= 0) {
        ALOGE("Len returned is < 0 len = %d", len);
        return -1;
    }

    UniquePtr<uint8_t> key(static_cast<uint8_t*>(malloc(len)));
    if (key.get() == NULL) {
        ALOGE("Could not allocate memory for public key data");
        return -1;
    }

    unsigned char* tmp = reinterpret_cast<unsigned char*>(key.get());
    if (i2d_PUBKEY(pkey.get(), &tmp) != len) {
        ALOGE("Len 2 returned is < 0 len = %d", len);
        return -1;
    }
    *x509_data_length = len;
    *x509_data = key.release();

    return 0;
}

static int32_t qcom_km_ION_memalloc(struct qcom_km_ion_info_t *handle,
                                uint32_t size)
{
    int32_t ret = 0;
    int32_t iret = 0;
    int32_t fd = 0;
    unsigned char *v_addr;
    struct ion_allocation_data ion_alloc_data;
    int32_t ion_fd;
    int32_t rc;
    struct ion_fd_data ifd_data;
    struct ion_handle_data handle_data;

    /* open ION device for memory management
     * O_DSYNC -> uncached memory
    */
    if(handle == NULL){
      ALOGE("Error:: null handle received");
      return -1;
    }
    ion_fd  = open("/dev/ion", O_RDONLY | O_DSYNC);
    if (ion_fd < 0) {
       ALOGE("Error::Cannot open ION device");
       return -1;
    }
    handle->ion_sbuffer = NULL;
    handle->ifd_data_fd = 0;

    /* Size of allocation */
    ion_alloc_data.len = (size + 4095) & (~4095);

    /* 4K aligned */
    ion_alloc_data.align = 4096;

    /* memory is allocated from EBI heap */
   ion_alloc_data.ION_HEAP_MASK = ION_HEAP(ION_QSECOM_HEAP_ID);

    /* Set the memory to be uncached */
    ion_alloc_data.flags = 0;

    /* IOCTL call to ION for memory request */
    rc = ioctl(ion_fd, ION_IOC_ALLOC, &ion_alloc_data);
    if (rc) {
       ret = -1;
       goto alloc_fail;
    }

    if (ion_alloc_data.handle != NULL) {
       ifd_data.handle = ion_alloc_data.handle;
    } else {
       ret = -1;
       goto alloc_fail;
    }
    /* Call MAP ioctl to retrieve the ifd_data.fd file descriptor */
    rc = ioctl(ion_fd, ION_IOC_MAP, &ifd_data);
    if (rc) {
       ret = -1;
       goto ioctl_fail;
    }

    /* Make the ion mmap call */
    v_addr = (unsigned char *)mmap(NULL, ion_alloc_data.len,
                                    PROT_READ | PROT_WRITE,
                                    MAP_SHARED, ifd_data.fd, 0);
    if (v_addr == MAP_FAILED) {
       ALOGE("Error::ION MMAP failed");
       ret = -1;
       goto map_fail;
    }
    handle->ion_fd = ion_fd;
    handle->ifd_data_fd = ifd_data.fd;
    handle->ion_sbuffer = v_addr;
    handle->ion_alloc_handle.handle = ion_alloc_data.handle;
    handle->sbuf_len = size;
    return ret;

map_fail:
    if (handle->ion_sbuffer != NULL) {
        iret = munmap(handle->ion_sbuffer, ion_alloc_data.len);
        if (iret)
           ALOGE("Error::Failed to unmap memory for load image. ret = %d", ret);
    }

ioctl_fail:
    handle_data.handle = ion_alloc_data.handle;
    if (handle->ifd_data_fd)
        close(handle->ifd_data_fd);
    iret = ioctl(ion_fd, ION_IOC_FREE, &handle_data);
    if (iret) {
       ALOGE("Error::ION FREE ioctl returned error = %d",iret);
    }

alloc_fail:
    if (ion_fd > 0)
       close(ion_fd);
    return ret;
}

/** @brief: Deallocate ION memory
 *
 *
 */
static int32_t qcom_km_ion_dealloc(struct qcom_km_ion_info_t *handle)
{
    struct ion_handle_data handle_data;
    int32_t ret = 0;

    /* Deallocate the memory for the listener */
    ret = munmap(handle->ion_sbuffer, (handle->sbuf_len + 4095) & (~4095));
    if (ret) {
        ALOGE("Error::Unmapping ION Buffer failed with ret = %d", ret);
    }

    handle_data.handle = handle->ion_alloc_handle.handle;
    close(handle->ifd_data_fd);
    ret = ioctl(handle->ion_fd, ION_IOC_FREE, &handle_data);
    if (ret) {
        ALOGE("Error::ION Memory FREE ioctl failed with ret = %d", ret);
    }
    close(handle->ion_fd);
    return ret;
}

static int qcom_km_generate_keypair(const keymaster_device_t* dev,
        const keymaster_keypair_t key_type, const void* key_params,
        uint8_t** keyBlob, size_t* keyBlobLength) {

    if (dev->context == NULL) {
        ALOGE("qcom_km_generate_keypair: Context == NULL");
        return -1;
    }

    if (key_type != TYPE_RSA) {
        ALOGE("Unsupported key type %d", key_type);
        return -1;
    } else if (key_params == NULL) {
        ALOGE("key_params == null");
        return -1;
    }
    if (keyBlob == NULL || keyBlobLength == NULL) {
        ALOGE("output key blob or length == NULL");
        return -1;
    }
    keymaster_rsa_keygen_params_t* rsa_params = (keymaster_rsa_keygen_params_t*) key_params;

    keymaster_gen_keypair_cmd_t *send_cmd = NULL;
    keymaster_gen_keypair_resp_t  *resp = NULL;
    struct QSEECom_handle *handle = NULL;
    struct qcom_keymaster_handle *km_handle =(struct qcom_keymaster_handle *)dev->context;
    int ret = 0;

    handle = (struct QSEECom_handle *)(km_handle->qseecom);
    send_cmd = (keymaster_gen_keypair_cmd_t *)handle->ion_sbuffer;
    resp = (keymaster_gen_keypair_resp_t *)(handle->ion_sbuffer +
                               QSEECOM_ALIGN(sizeof(keymaster_gen_keypair_cmd_t)));
    send_cmd->cmd_id = KEYMASTER_GENERATE_KEYPAIR;
    send_cmd->key_type = key_type;
    send_cmd->rsa_params.modulus_size = rsa_params->modulus_size;
    send_cmd->rsa_params.public_exponent = rsa_params->public_exponent;
    resp->status = KEYMASTER_FAILURE;
    resp->key_blob_len =  sizeof(qcom_km_key_blob_t);

    ret = (*km_handle->QSEECom_set_bandwidth)(handle, true);
    if (ret < 0) {
        ALOGE("Generate key command failed (unable to enable clks) ret =%d", ret);
        return -1;
    }

    ret = (*km_handle->QSEECom_send_cmd)(handle, send_cmd,
                               QSEECOM_ALIGN(sizeof(keymaster_gen_keypair_cmd_t)), resp,
                               QSEECOM_ALIGN(sizeof(keymaster_gen_keypair_resp_t)));

    if((*km_handle->QSEECom_set_bandwidth)(handle, false))
        ALOGE("Import key command: (unable to disable clks)");

    if ( (ret < 0)  ||  (resp->status  < 0)) {
        ALOGE("Generate key command failed resp->status = %d ret =%d", resp->status, ret);
        return -1;
    } else {
        UniquePtr<unsigned char[]> keydata(new unsigned char[resp->key_blob_len]);
        if (keydata.get() == NULL) {
            ALOGE("could not allocate memory for key blob");
            return -1;
        }
        unsigned char* p = keydata.get();
        memcpy(p, (unsigned char *)(&resp->key_blob), resp->key_blob_len);
        *keyBlob = keydata.release();
        *keyBlobLength = resp->key_blob_len;
    }
    return 0;
}

static int qcom_km_import_keypair(const keymaster_device_t* dev,
        const uint8_t* key, const size_t key_length,
        uint8_t** keyBlob, size_t* keyBlobLength)
{
    if (dev->context == NULL) {
        ALOGE("qcom_km_import_keypair: Context  == NULL");
        return -1;
    }

    if (key == NULL) {
        ALOGE("Input key == NULL");
        return -1;
    } else if (keyBlob == NULL || keyBlobLength == NULL) {
        ALOGE("Output key blob or length == NULL");
        return -1;
    }

    struct QSEECom_ion_fd_info  ion_fd_info;
    struct qcom_km_ion_info_t ihandle;
    int ret = 0;

    ihandle.ion_fd = 0;
    ihandle.ion_alloc_handle.handle = NULL;
    if (qcom_km_ION_memalloc(&ihandle, QSEECOM_ALIGN(key_length)) < 0) {
        ALOGE("ION allocation  failed");
        return -1;
    }
    memset(&ion_fd_info, 0, sizeof(struct QSEECom_ion_fd_info));

    /* Populate the send data structure */
    ion_fd_info.data[0].fd = ihandle.ifd_data_fd;
    ion_fd_info.data[0].cmd_buf_offset = sizeof(enum keymaster_cmd_t);


    struct QSEECom_handle *handle = NULL;
    keymaster_import_keypair_cmd_t *send_cmd = NULL;
    keymaster_import_keypair_resp_t  *resp = NULL;
    struct qcom_keymaster_handle *km_handle =(struct qcom_keymaster_handle *)dev->context;

    handle = (struct QSEECom_handle *)(km_handle->qseecom);
    send_cmd = (keymaster_import_keypair_cmd_t *)handle->ion_sbuffer;
    resp = (keymaster_import_keypair_resp_t *)(handle->ion_sbuffer +
                                        QSEECOM_ALIGN(sizeof(keymaster_import_keypair_cmd_t)));
    send_cmd->cmd_id = KEYMASTER_IMPORT_KEYPAIR;
    send_cmd->pkcs8_key = (uint32_t)ihandle.ion_sbuffer;

    memcpy((unsigned char *)ihandle.ion_sbuffer, key, key_length);

    send_cmd->pkcs8_key_len = key_length;
    resp->status = KEYMASTER_FAILURE;
    resp->key_blob_len =  sizeof(qcom_km_key_blob_t);

    ret = (*km_handle->QSEECom_set_bandwidth)(handle, true);
    if (ret < 0) {
        ALOGE("Import key command failed (unable to enable clks) ret =%d", ret);
        qcom_km_ion_dealloc(&ihandle);
        return -1;
    }
    ret = (*km_handle->QSEECom_send_modified_cmd)(handle, send_cmd,
                               QSEECOM_ALIGN(sizeof(*send_cmd)), resp,
                               QSEECOM_ALIGN(sizeof(*resp)), &ion_fd_info);

    if((*km_handle->QSEECom_set_bandwidth)(handle, false))
        ALOGE("Import key command: (unable to disable clks)");

    if ( (ret < 0)  ||  (resp->status  < 0)) {
        ALOGE("Import key command failed resp->status = %d ret =%d", resp->status, ret);
        qcom_km_ion_dealloc(&ihandle);
        return -1;
    } else {
        UniquePtr<unsigned char[]> keydata(new unsigned char[resp->key_blob_len]);
        if (keydata.get() == NULL) {
            ALOGE("could not allocate memory for key blob");
            return -1;
        }
        unsigned char* p = keydata.get();
        memcpy(p, (unsigned char *)(&resp->key_blob), resp->key_blob_len);
        *keyBlob = keydata.release();
        *keyBlobLength = resp->key_blob_len;

    }
    qcom_km_ion_dealloc(&ihandle);
    return 0;
}

static int qcom_km_sign_data(const keymaster_device_t* dev,
        const void* params,
        const uint8_t* keyBlob, const size_t keyBlobLength,
        const uint8_t* data, const size_t dataLength,
        uint8_t** signedData, size_t* signedDataLength)
{
    if (dev->context == NULL) {
        ALOGE("qcom_km_sign_data: Context  == NULL");
        return -1;
    }
    if (dataLength > KM_KEY_SIZE_MAX) {
        ALOGE("Input data to be signed is too long %d bytes", dataLength);
        return -1;
    }
    if (data == NULL) {
        ALOGE("input data to sign == NULL");
        return -1;
    } else if (signedData == NULL || signedDataLength == NULL) {
        ALOGE("Output signature buffer == NULL");
        return -1;
    }
    keymaster_rsa_sign_params_t* sign_params = (keymaster_rsa_sign_params_t*) params;
    if (sign_params->digest_type != DIGEST_NONE) {
        ALOGE("Cannot handle digest type %d", sign_params->digest_type);
        return -1;
    } else if (sign_params->padding_type != PADDING_NONE) {
        ALOGE("Cannot handle padding type %d", sign_params->padding_type);
        return -1;
    }

    struct QSEECom_handle *handle = NULL;
    keymaster_sign_data_cmd_t *send_cmd = NULL;
    keymaster_sign_data_resp_t  *resp = NULL;
    struct QSEECom_ion_fd_info  ion_fd_info;
    struct qcom_km_ion_info_t ihandle;
    struct qcom_keymaster_handle *km_handle =(struct qcom_keymaster_handle *)dev->context;
    int ret = 0;

    handle = (struct QSEECom_handle *)(km_handle->qseecom);
    ihandle.ion_fd = 0;
    ihandle.ion_alloc_handle.handle = NULL;
    if (qcom_km_ION_memalloc(&ihandle, dataLength) < 0) {
        ALOGE("ION allocation  failed");
        return -1;
    }
    memset(&ion_fd_info, 0, sizeof(struct QSEECom_ion_fd_info));

    /* Populate the send data structure */
    ion_fd_info.data[0].fd = ihandle.ifd_data_fd;
    ion_fd_info.data[0].cmd_buf_offset = sizeof(enum keymaster_cmd_t) +
         sizeof(qcom_km_key_blob_t) + sizeof(keymaster_rsa_sign_params_t);

    send_cmd = (keymaster_sign_data_cmd_t *)handle->ion_sbuffer;
    resp = (keymaster_sign_data_resp_t *)(handle->ion_sbuffer +
                            QSEECOM_ALIGN(sizeof(keymaster_sign_data_cmd_t)));
    send_cmd->cmd_id = KEYMASTER_SIGN_DATA ;
    send_cmd->sign_param.digest_type = sign_params->digest_type;
    send_cmd->sign_param.padding_type = sign_params->padding_type;
    memcpy((unsigned char *)(&send_cmd->key_blob), keyBlob, keyBlobLength);
    memcpy((unsigned char *)ihandle.ion_sbuffer, data, dataLength);

    send_cmd->data = (uint32_t)ihandle.ion_sbuffer;
    send_cmd->dlen = dataLength;
    resp->sig_len = KM_KEY_SIZE_MAX;
    resp->status = KEYMASTER_FAILURE;

    ret = (*km_handle->QSEECom_set_bandwidth)(handle, true);
    if (ret < 0) {
        ALOGE("Sign data command failed (unable to enable clks) ret =%d", ret);
        qcom_km_ion_dealloc(&ihandle);
        return -1;
    }

    ret = (*km_handle->QSEECom_send_modified_cmd)(handle, send_cmd,
                               QSEECOM_ALIGN(sizeof(*send_cmd)), resp,
                               QSEECOM_ALIGN(sizeof(*resp)), &ion_fd_info);

    if((*km_handle->QSEECom_set_bandwidth)(handle, false))
        ALOGE("Sign data command: (unable to disable clks)");

    if ( (ret < 0)  ||  (resp->status  < 0)) {
        ALOGE("Sign data command failed resp->status = %d ret =%d", resp->status, ret);
        qcom_km_ion_dealloc(&ihandle);
        return -1;
    } else {
        UniquePtr<uint8_t> signedDataPtr(reinterpret_cast<uint8_t*>(malloc(resp->sig_len)));
        if (signedDataPtr.get() == NULL) {
            ALOGE("Sign data memory allocation failed");
            qcom_km_ion_dealloc(&ihandle);
            return -1;
        }
        unsigned char* p = signedDataPtr.get();
        memcpy(p, (unsigned char *)(&resp->signed_data), resp->sig_len);

        *signedDataLength = resp->sig_len;
        *signedData = signedDataPtr.release();
    }
    qcom_km_ion_dealloc(&ihandle);
    return 0;
}

static int qcom_km_verify_data(const keymaster_device_t* dev,
        const void* params,
        const uint8_t* keyBlob, const size_t keyBlobLength,
        const uint8_t* signedData, const size_t signedDataLength,
        const uint8_t* signature, const size_t signatureLength)
{
    if (dev->context == NULL) {
        ALOGE("qcom_km_verify_data: Context  == NULL");
        return -1;
    }

    if (signedData == NULL || signature == NULL) {
        ALOGE("data or signature buffers == NULL");
        return -1;
    }

    keymaster_rsa_sign_params_t* sign_params = (keymaster_rsa_sign_params_t*) params;
    if (sign_params->digest_type != DIGEST_NONE) {
        ALOGE("Cannot handle digest type %d", sign_params->digest_type);
        return -1;
    } else if (sign_params->padding_type != PADDING_NONE) {
        ALOGE("Cannot handle padding type %d", sign_params->padding_type);
        return -1;
    } else if (signatureLength != signedDataLength) {
        ALOGE("signed data length must be signature length");
        return -1;
    }

    struct QSEECom_handle *handle = NULL;
    keymaster_verify_data_cmd_t *send_cmd = NULL;
    keymaster_verify_data_resp_t  *resp = NULL;

    struct QSEECom_ion_fd_info  ion_fd_info;
    struct qcom_km_ion_info_t ihandle;
    struct qcom_keymaster_handle *km_handle =(struct qcom_keymaster_handle *)dev->context;
    int ret = 0;

    handle = (struct QSEECom_handle *)(km_handle->qseecom);
    ihandle.ion_fd = 0;
    ihandle.ion_alloc_handle.handle = NULL;
    if (qcom_km_ION_memalloc(&ihandle, signedDataLength + signatureLength) <0) {
        ALOGE("ION allocation  failed");
        return -1;
    }
    memset(&ion_fd_info, 0, sizeof(struct QSEECom_ion_fd_info));

    /* Populate the send data structure */
    ion_fd_info.data[0].fd = ihandle.ifd_data_fd;
    ion_fd_info.data[0].cmd_buf_offset = sizeof(enum keymaster_cmd_t) +
        sizeof(qcom_km_key_blob_t ) + sizeof(keymaster_rsa_sign_params_t);

    send_cmd = (keymaster_verify_data_cmd_t *)handle->ion_sbuffer;
    resp = (keymaster_verify_data_resp_t *)((char *)handle->ion_sbuffer +
                               sizeof(keymaster_verify_data_cmd_t));
    send_cmd->cmd_id = KEYMASTER_VERIFY_DATA ;
    send_cmd->sign_param.digest_type = sign_params->digest_type;
    send_cmd->sign_param.padding_type = sign_params->padding_type;
    memcpy((unsigned char *)(&send_cmd->key_blob), keyBlob, keyBlobLength);

    send_cmd->signed_data = (uint32_t)ihandle.ion_sbuffer;
    send_cmd->signed_dlen = signedDataLength;
    memcpy((unsigned char *)ihandle.ion_sbuffer, signedData, signedDataLength);

    send_cmd->signature = signedDataLength;
    send_cmd->slen = signatureLength;
    memcpy(((unsigned char *)ihandle.ion_sbuffer + signedDataLength),
                                  signature, signatureLength);
    resp->status = KEYMASTER_FAILURE;

    ret = (*km_handle->QSEECom_set_bandwidth)(handle, true);
    if (ret < 0) {
        ALOGE("Verify data  command failed (unable to enable clks) ret =%d", ret);
        qcom_km_ion_dealloc(&ihandle);
        return -1;
    }

    ret = (*km_handle->QSEECom_send_modified_cmd)(handle, send_cmd,
                               QSEECOM_ALIGN(sizeof(*send_cmd)), resp,
                               QSEECOM_ALIGN(sizeof(*resp)), &ion_fd_info);

    if((*km_handle->QSEECom_set_bandwidth)(handle, false))
        ALOGE("Verify data  command: (unable to disable clks)");

    if ( (ret < 0)  ||  (resp->status  < 0)) {
        ALOGE("Verify data command failed resp->status = %d ret =%d", resp->status, ret);
        qcom_km_ion_dealloc(&ihandle);
        return -1;
    }
    qcom_km_ion_dealloc(&ihandle);
    return 0;
}

/* Close an opened OpenSSL instance */
static int qcom_km_close(hw_device_t *dev)
{
    keymaster_device_t* km_dev = (keymaster_device_t *)dev;
    struct qcom_keymaster_handle *km_handle =(struct qcom_keymaster_handle *)km_dev->context;

    if (km_handle->qseecom == NULL) {
        ALOGE("Context  == NULL");
        return -1;
    }
    (*km_handle->QSEECom_shutdown_app)((struct QSEECom_handle **)&km_handle->qseecom);
    free(km_dev->context);
    free(dev);
    return 0;
}

static int qcom_km_get_lib_sym(qcom_keymaster_handle_t* km_handle)
{
    km_handle->libhandle = dlopen("libQSEEComAPI.so", RTLD_NOW);
    if (  km_handle->libhandle  ) {
        *(void **)(&km_handle->QSEECom_start_app) =
                               dlsym(km_handle->libhandle,"QSEECom_start_app");
        if (km_handle->QSEECom_start_app == NULL) {
               ALOGE("dlsym: Error Loading QSEECom_start_app");
                   dlclose(km_handle->libhandle );
                   km_handle->libhandle  = NULL;
                   return -1;
            }
            *(void **)(&km_handle->QSEECom_shutdown_app) =
                               dlsym(km_handle->libhandle,"QSEECom_shutdown_app");
            if (km_handle->QSEECom_shutdown_app == NULL) {
                   ALOGE("dlsym: Error Loading QSEECom_shutdown_app");
                   dlclose(km_handle->libhandle );
                   km_handle->libhandle  = NULL;
                   return -1;
             }
            *(void **)(&km_handle->QSEECom_send_cmd) =
                               dlsym(km_handle->libhandle,"QSEECom_send_cmd");
            if (km_handle->QSEECom_send_cmd == NULL) {
                   ALOGE("dlsym: Error Loading QSEECom_send_cmd");
                   dlclose(km_handle->libhandle );
                   km_handle->libhandle  = NULL;
                   return -1;
             }
            *(void **)(&km_handle->QSEECom_send_modified_cmd) =
                               dlsym(km_handle->libhandle,"QSEECom_send_modified_cmd");
            if (km_handle->QSEECom_send_modified_cmd == NULL) {
                   ALOGE("dlsym: Error Loading QSEECom_send_modified_cmd");
                   dlclose(km_handle->libhandle );
                   km_handle->libhandle  = NULL;
                   return -1;
             }
            *(void **)(&km_handle->QSEECom_set_bandwidth) =
                               dlsym(km_handle->libhandle,"QSEECom_set_bandwidth");
            if (km_handle->QSEECom_set_bandwidth == NULL) {
                   ALOGE("dlsym: Error Loading QSEECom_set_bandwidth");
                   dlclose(km_handle->libhandle );
                   km_handle->libhandle  = NULL;
                   return -1;
             }

    } else {
        ALOGE("failed to load qseecom library");
        return -1;
    }
    return 0;
}

/*
 * Generic device handling
 */
static int qcom_km_open(const hw_module_t* module, const char* name,
        hw_device_t** device)
{
    int ret = 0;
    qcom_keymaster_handle_t* km_handle;
    if (strcmp(name, KEYSTORE_KEYMASTER) != 0)
        return -EINVAL;

    km_handle = (qcom_keymaster_handle_t *)malloc(sizeof(qcom_keymaster_handle_t));
    if (km_handle == NULL) {
        ALOGE("Memalloc for keymaster handle failed");
        return -1;
    }
    km_handle->qseecom = NULL;
    km_handle->libhandle = NULL;
    ret = qcom_km_get_lib_sym(km_handle);
    if (ret) {
        free(km_handle);
        return -1;
    }
    Unique_keymaster_device_t dev(new keymaster_device_t);
    if (dev.get() == NULL){
        free(km_handle);
        return -ENOMEM;
    }
    dev->context = (void *)km_handle;
    ret = (*km_handle->QSEECom_start_app)((struct QSEECom_handle **)&km_handle->qseecom,
                         "/vendor/firmware/keymaster", "keymaster", 4096*2);
    if (ret) {
        ALOGE("Loading keymaster app failed");
        free(km_handle);
        return -1;
    }
    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 1;
    dev->common.module = (struct hw_module_t*) module;
    dev->common.close = qcom_km_close;
    dev->flags = KEYMASTER_BLOBS_ARE_STANDALONE;

    dev->generate_keypair = qcom_km_generate_keypair;
    dev->import_keypair = qcom_km_import_keypair;
    dev->get_keypair_public = qcom_km_get_keypair_public;
    dev->delete_keypair = NULL;
    dev->delete_all = NULL;
    dev->sign_data = qcom_km_sign_data;
    dev->verify_data = qcom_km_verify_data;

    *device = reinterpret_cast<hw_device_t*>(dev.release());

    return 0;
}

static struct hw_module_methods_t keystore_module_methods = {
    .open = qcom_km_open,
};

struct keystore_module HAL_MODULE_INFO_SYM
__attribute__ ((visibility ("default"))) = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = QCOM_KEYMASTER_API_VERSION,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = KEYSTORE_HARDWARE_MODULE_ID,
        .name = "Keymaster QCOM HAL",
        .author = "The Android Open Source Project",
        .methods = &keystore_module_methods,
        .dso = 0,
        .reserved = {},
    },
};
