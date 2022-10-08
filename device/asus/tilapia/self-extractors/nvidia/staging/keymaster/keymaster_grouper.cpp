/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <errno.h>
#include <string.h>
#include <stdint.h>

// For debugging
#define LOG_NDEBUG 0

// TEE is the Trusted Execution Environment
#define LOG_TAG "TEEKeyMaster"
#include <cutils/log.h>

#include <hardware/hardware.h>
#include <hardware/keymaster.h>

#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/x509.h>

#include <cryptoki.h>
#include <pkcs11.h>

#include <UniquePtr.h>


/** The size of a key ID in bytes */
#define ID_LENGTH 32

/** The current stored key version. */
const static uint32_t KEY_VERSION = 1;


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

struct PKCS8_PRIV_KEY_INFO_Delete {
    void operator()(PKCS8_PRIV_KEY_INFO* p) const {
        PKCS8_PRIV_KEY_INFO_free(p);
    }
};
typedef UniquePtr<PKCS8_PRIV_KEY_INFO, PKCS8_PRIV_KEY_INFO_Delete> Unique_PKCS8_PRIV_KEY_INFO;

typedef UniquePtr<keymaster_device_t> Unique_keymaster_device_t;

typedef UniquePtr<CK_BYTE[]> Unique_CK_BYTE;

typedef UniquePtr<CK_ATTRIBUTE[]> Unique_CK_ATTRIBUTE;

class ByteArray {
public:
    ByteArray(CK_BYTE* array, size_t len) :
            mArray(array), mLength(len) {
    }

    ByteArray(size_t len) :
            mLength(len) {
        mArray = new CK_BYTE[len];
    }

    ~ByteArray() {
        if (mArray != NULL) {
            delete[] mArray;
        }
    }

    CK_BYTE* get() const {
        return mArray;
    }

    void setLength(size_t length) {
        mLength = length;
    }

    size_t length() const {
        return mLength;
    }

    CK_BYTE* release() {
        CK_BYTE* array = mArray;
        mArray = NULL;
        return array;
    }

private:
    CK_BYTE* mArray;
    size_t mLength;
};
typedef UniquePtr<ByteArray> Unique_ByteArray;

class CryptoSession {
public:
    CryptoSession(CK_SESSION_HANDLE masterHandle) :
            mHandle(masterHandle), mSubsession(CK_INVALID_HANDLE) {
        CK_SESSION_HANDLE subsessionHandle = mHandle;
        CK_RV openSessionRV = C_OpenSession(CKV_TOKEN_USER,
                CKF_SERIAL_SESSION | CKF_RW_SESSION | CKVF_OPEN_SUB_SESSION,
                NULL,
                NULL,
                &subsessionHandle);

        if (openSessionRV != CKR_OK || subsessionHandle == CK_INVALID_HANDLE) {
            (void) C_Finalize(NULL_PTR);
            ALOGE("Error opening secondary session with TEE: 0x%x", openSessionRV);
        } else {
            ALOGV("Opening subsession 0x%x", subsessionHandle);
            mSubsession = subsessionHandle;
        }
    }

    ~CryptoSession() {
        if (mSubsession != CK_INVALID_HANDLE) {
            CK_RV rv = C_CloseSession(mSubsession);
            ALOGV("Closing subsession 0x%x: 0x%x", mSubsession, rv);
            mSubsession = CK_INVALID_HANDLE;
        }
    }

    CK_SESSION_HANDLE get() const {
        return mSubsession;
    }

    CK_SESSION_HANDLE getPrimary() const {
        return mHandle;
    }

private:
    CK_SESSION_HANDLE mHandle;
    CK_SESSION_HANDLE mSubsession;
};

class ObjectHandle {
public:
    ObjectHandle(const CryptoSession* session, CK_OBJECT_HANDLE handle = CK_INVALID_HANDLE) :
            mSession(session), mHandle(handle) {
    }

    ~ObjectHandle() {
        if (mHandle != CK_INVALID_HANDLE) {
            CK_RV rv = C_CloseObjectHandle(mSession->getPrimary(), mHandle);
            if (rv != CKR_OK) {
                ALOGW("Couldn't close object handle 0x%x: 0x%x", mHandle, rv);
            } else {
                ALOGV("Closing object handle 0x%x", mHandle);
                mHandle = CK_INVALID_HANDLE;
            }
        }
    }

    CK_OBJECT_HANDLE get() const {
        return mHandle;
    }

    void reset(CK_OBJECT_HANDLE handle) {
        mHandle = handle;
    }

private:
    const CryptoSession* mSession;
    CK_OBJECT_HANDLE mHandle;
};


/**
 * Many OpenSSL APIs take ownership of an argument on success but don't free the argument
 * on failure. This means we need to tell our scoped pointers when we've transferred ownership,
 * without triggering a warning by not using the result of release().
 */
#define OWNERSHIP_TRANSFERRED(obj) \
    typeof (obj.release()) _dummy __attribute__((unused)) = obj.release()


/*
 * Checks this thread's OpenSSL error queue and logs if
 * necessary.
 */
static void logOpenSSLError(const char* location) {
    int error = ERR_get_error();

    if (error != 0) {
        char message[256];
        ERR_error_string_n(error, message, sizeof(message));
        ALOGE("OpenSSL error in %s %d: %s", location, error, message);
    }

    ERR_clear_error();
    ERR_remove_state(0);
}


/**
 * Convert from OpenSSL's BIGNUM format to TEE's Big Integer format.
 */
static ByteArray* bignum_to_array(const BIGNUM* bn) {
    const int bignumSize = BN_num_bytes(bn);

    Unique_CK_BYTE bytes(new CK_BYTE[bignumSize]);

    unsigned char* tmp = reinterpret_cast<unsigned char*>(bytes.get());
    if (BN_bn2bin(bn, tmp) != bignumSize) {
        ALOGE("public exponent size wasn't what was expected");
        return NULL;
    }

    return new ByteArray(bytes.release(), bignumSize);
}

static void set_attribute(CK_ATTRIBUTE* attrib, CK_ATTRIBUTE_TYPE type, void* pValue,
        CK_ULONG ulValueLen) {
    attrib->type = type;
    attrib->pValue = pValue;
    attrib->ulValueLen = ulValueLen;
}

static ByteArray* generate_random_id() {
    Unique_ByteArray id(new ByteArray(ID_LENGTH));
    if (RAND_pseudo_bytes(reinterpret_cast<unsigned char*>(id->get()), id->length()) < 0) {
        return NULL;
    }

    return id.release();
}

static int keyblob_save(ByteArray* objId, uint8_t** key_blob, size_t* key_blob_length) {
    Unique_ByteArray handleBlob(new ByteArray(sizeof(uint32_t) + objId->length()));
    if (handleBlob.get() == NULL) {
        ALOGE("Could not allocate key blob");
        return -1;
    }
    uint8_t* tmp = handleBlob->get();
    for (size_t i = 0; i < sizeof(uint32_t); i++) {
        *tmp++ = KEY_VERSION >> ((sizeof(uint32_t) - i - 1) * 8);
    }
    memcpy(tmp, objId->get(), objId->length());

    *key_blob_length = handleBlob->length();
    *key_blob = handleBlob->get();
    ByteArray* unused __attribute__((unused)) = handleBlob.release();

    return 0;
}

static int find_single_object(const uint8_t* obj_id, const size_t obj_id_length,
        CK_OBJECT_CLASS obj_class, const CryptoSession* session, ObjectHandle* object) {

    // Note that the CKA_ID attribute is never written, so we can cast away const here.
    void* obj_id_ptr = reinterpret_cast<void*>(const_cast<uint8_t*>(obj_id));
    CK_ATTRIBUTE attributes[] = {
            { CKA_ID,    obj_id_ptr, obj_id_length },
            { CKA_CLASS, &obj_class, sizeof(obj_class) },
    };

    CK_RV rv = C_FindObjectsInit(session->get(), attributes,
            sizeof(attributes) / sizeof(CK_ATTRIBUTE));
    if (rv != CKR_OK) {
        ALOGE("Error in C_FindObjectsInit: 0x%x", rv);
        return -1;
    }

    CK_OBJECT_HANDLE tmpHandle;
    CK_ULONG tmpCount;

    rv = C_FindObjects(session->get(), &tmpHandle, 1, &tmpCount);
    ALOGV("Found %d object 0x%x : class 0x%x", tmpCount, tmpHandle, obj_class);
    if (rv != CKR_OK || tmpCount != 1) {
        C_FindObjectsFinal(session->get());
        ALOGE("Couldn't find key!");
        return -1;
    }
    C_FindObjectsFinal(session->get());

    object->reset(tmpHandle);
    return 0;
}

static int keyblob_restore(const CryptoSession* session, const uint8_t* keyBlob,
        const size_t keyBlobLength, ObjectHandle* public_key, ObjectHandle* private_key) {
    if (keyBlob == NULL) {
        ALOGE("key blob was null");
        return -1;
    }

    if (keyBlobLength < (sizeof(KEY_VERSION) + ID_LENGTH)) {
        ALOGE("key blob is not correct size");
        return -1;
    }

    uint32_t keyVersion = 0;

    const uint8_t* p = keyBlob;
    for (size_t i = 0; i < sizeof(keyVersion); i++) {
        keyVersion = (keyVersion << 8) | *p++;
    }

    if (keyVersion != 1) {
        ALOGE("Invalid key version %d", keyVersion);
        return -1;
    }

    return find_single_object(p, ID_LENGTH, CKO_PUBLIC_KEY, session, public_key)
            || find_single_object(p, ID_LENGTH, CKO_PRIVATE_KEY, session, private_key);
}

static int tee_generate_keypair(const keymaster_device_t* dev,
        const keymaster_keypair_t type, const void* key_params,
        uint8_t** key_blob, size_t* key_blob_length) {
    CK_BBOOL bTRUE = CK_TRUE;

    if (type != TYPE_RSA) {
        ALOGW("Unknown key type %d", type);
        return -1;
    }

    if (key_params == NULL) {
        ALOGW("generate_keypair params were NULL");
        return -1;
    }

    keymaster_rsa_keygen_params_t* rsa_params = (keymaster_rsa_keygen_params_t*) key_params;

    CK_MECHANISM mechanism = {
            CKM_RSA_PKCS_KEY_PAIR_GEN, NULL, 0,
    };
    CK_ULONG modulusBits = (CK_ULONG) rsa_params->modulus_size;

    /**
     * Convert our unsigned 64-bit integer to the TEE Big Integer class. It's
     * an unsigned array of bytes with MSB first.
     */
    CK_BYTE publicExponent[sizeof(uint64_t)];
    const uint64_t exp = rsa_params->public_exponent;
    size_t offset = sizeof(publicExponent) - 1;
    for (size_t i = 0; i < sizeof(publicExponent); i++) {
        publicExponent[offset--] = (exp >> (i * CHAR_BIT)) & 0xFF;
    }

    Unique_ByteArray objId(generate_random_id());
    if (objId.get() == NULL) {
        ALOGE("Couldn't generate random key ID");
        return -1;
    }

    CK_ATTRIBUTE publicKeyTemplate[] = {
            {CKA_ID,              objId->get(),   objId->length()},
            {CKA_TOKEN,           &bTRUE,         sizeof(bTRUE)},
            {CKA_ENCRYPT,         &bTRUE,         sizeof(bTRUE)},
            {CKA_VERIFY,          &bTRUE,         sizeof(bTRUE)},
            {CKA_MODULUS_BITS,    &modulusBits,   sizeof(modulusBits)},
            {CKA_PUBLIC_EXPONENT, publicExponent, sizeof(publicExponent)},
    };

    CK_ATTRIBUTE privateKeyTemplate[] = {
            {CKA_ID,              objId->get(),   objId->length()},
            {CKA_TOKEN,           &bTRUE,         sizeof(bTRUE)},
            {CKA_DECRYPT,         &bTRUE,         sizeof(bTRUE)},
            {CKA_SIGN,            &bTRUE,         sizeof(bTRUE)},
    };

    CryptoSession session(reinterpret_cast<CK_SESSION_HANDLE>(dev->context));

    CK_OBJECT_HANDLE hPublicKey, hPrivateKey;
    CK_RV rv = C_GenerateKeyPair(session.get(),
            &mechanism,
            publicKeyTemplate,
            sizeof(publicKeyTemplate)/sizeof(CK_ATTRIBUTE),
            privateKeyTemplate,
            sizeof(privateKeyTemplate)/sizeof(CK_ATTRIBUTE),
            &hPublicKey,
            &hPrivateKey);

    if (rv != CKR_OK) {
        ALOGE("Generate keypair failed: 0x%x", rv);
        return -1;
    }

    ObjectHandle publicKey(&session, hPublicKey);
    ObjectHandle privateKey(&session, hPrivateKey);
    ALOGV("public handle = 0x%x, private handle = 0x%x", publicKey.get(), privateKey.get());

    return keyblob_save(objId.get(), key_blob, key_blob_length);
}

static int tee_import_keypair(const keymaster_device_t* dev,
        const uint8_t* key, const size_t key_length,
        uint8_t** key_blob, size_t* key_blob_length) {
    CK_RV rv;
    CK_BBOOL bTRUE = CK_TRUE;

    if (key == NULL) {
        ALOGW("provided key is null");
        return -1;
    }

    Unique_PKCS8_PRIV_KEY_INFO pkcs8(d2i_PKCS8_PRIV_KEY_INFO(NULL, &key, key_length));
    if (pkcs8.get() == NULL) {
        logOpenSSLError("tee_import_keypair");
        return -1;
    }

    /* assign to EVP */
    Unique_EVP_PKEY pkey(EVP_PKCS82PKEY(pkcs8.get()));
    if (pkey.get() == NULL) {
        logOpenSSLError("tee_import_keypair");
        return -1;
    }

    if (EVP_PKEY_type(pkey->type) != EVP_PKEY_RSA) {
        ALOGE("Unsupported key type: %d", EVP_PKEY_type(pkey->type));
        return -1;
    }

    Unique_RSA rsa(EVP_PKEY_get1_RSA(pkey.get()));
    if (rsa.get() == NULL) {
        logOpenSSLError("tee_import_keypair");
        return -1;
    }

    Unique_ByteArray modulus(bignum_to_array(rsa->n));
    if (modulus.get() == NULL) {
        ALOGW("Could not convert modulus to array");
        return -1;
    }

    Unique_ByteArray publicExponent(bignum_to_array(rsa->e));
    if (publicExponent.get() == NULL) {
        ALOGW("Could not convert publicExponent to array");
        return -1;
    }

    CK_KEY_TYPE rsaType = CKK_RSA;

    CK_OBJECT_CLASS pubClass = CKO_PUBLIC_KEY;

    Unique_ByteArray objId(generate_random_id());
    if (objId.get() == NULL) {
        ALOGE("Couldn't generate random key ID");
        return -1;
    }

    CK_ATTRIBUTE publicKeyTemplate[] = {
            {CKA_ID,              objId->get(),          objId->length()},
            {CKA_TOKEN,           &bTRUE,                sizeof(bTRUE)},
            {CKA_CLASS,           &pubClass,             sizeof(pubClass)},
            {CKA_KEY_TYPE,        &rsaType,              sizeof(rsaType)},
            {CKA_ENCRYPT,         &bTRUE,                sizeof(bTRUE)},
            {CKA_VERIFY,          &bTRUE,                sizeof(bTRUE)},
            {CKA_MODULUS,         modulus->get(),        modulus->length()},
            {CKA_PUBLIC_EXPONENT, publicExponent->get(), publicExponent->length()},
    };

    CryptoSession session(reinterpret_cast<CK_SESSION_HANDLE>(dev->context));

    CK_OBJECT_HANDLE hPublicKey;
    rv = C_CreateObject(session.get(),
            publicKeyTemplate,
            sizeof(publicKeyTemplate)/sizeof(CK_ATTRIBUTE),
            &hPublicKey);
    if (rv != CKR_OK) {
        ALOGE("Creation of public key failed: 0x%x", rv);
        return -1;
    }
    ObjectHandle publicKey(&session, hPublicKey);

    Unique_ByteArray privateExponent(bignum_to_array(rsa->d));
    if (privateExponent.get() == NULL) {
        ALOGW("Could not convert private exponent");
        return -1;
    }


    /*
     * Normally we need:
     *   CKA_ID
     *   CKA_TOKEN
     *   CKA_CLASS
     *   CKA_KEY_TYPE
     *
     *   CKA_DECRYPT
     *   CKA_SIGN
     *
     *   CKA_MODULUS
     *   CKA_PUBLIC_EXPONENT
     *   CKA_PRIVATE_EXPONENT
     */
#define PRIV_ATTRIB_NORMAL_NUM (4 + 2 + 3)

    /*
     * For additional private key values:
     *   CKA_PRIME_1
     *   CKA_PRIME_2
     *
     *   CKA_EXPONENT_1
     *   CKA_EXPONENT_2
     *
     *   CKA_COEFFICIENT
     */
#define PRIV_ATTRIB_EXTENDED_NUM (PRIV_ATTRIB_NORMAL_NUM + 5)

    /*
     * If we have the prime, prime exponents, and coefficient, we can
     * copy them in.
     */
    bool has_extra_data = (rsa->p != NULL) && (rsa->q != NULL) && (rsa->dmp1 != NULL) &&
            (rsa->dmq1 != NULL) && (rsa->iqmp != NULL);

    Unique_CK_ATTRIBUTE privateKeyTemplate(new CK_ATTRIBUTE[
            has_extra_data ? PRIV_ATTRIB_EXTENDED_NUM : PRIV_ATTRIB_NORMAL_NUM]);

    CK_OBJECT_CLASS privClass = CKO_PRIVATE_KEY;

    size_t templateOffset = 0;

    set_attribute(&privateKeyTemplate[templateOffset++], CKA_ID, objId->get(), objId->length());
    set_attribute(&privateKeyTemplate[templateOffset++], CKA_TOKEN, &bTRUE, sizeof(bTRUE));
    set_attribute(&privateKeyTemplate[templateOffset++], CKA_CLASS, &privClass, sizeof(privClass));
    set_attribute(&privateKeyTemplate[templateOffset++], CKA_KEY_TYPE, &rsaType, sizeof(rsaType));

    set_attribute(&privateKeyTemplate[templateOffset++], CKA_DECRYPT, &bTRUE, sizeof(bTRUE));
    set_attribute(&privateKeyTemplate[templateOffset++], CKA_SIGN, &bTRUE, sizeof(bTRUE));

    set_attribute(&privateKeyTemplate[templateOffset++], CKA_MODULUS, modulus->get(),
            modulus->length());
    set_attribute(&privateKeyTemplate[templateOffset++], CKA_PUBLIC_EXPONENT,
            publicExponent->get(), publicExponent->length());
    set_attribute(&privateKeyTemplate[templateOffset++], CKA_PRIVATE_EXPONENT,
            privateExponent->get(), privateExponent->length());

    Unique_ByteArray prime1, prime2, exp1, exp2, coeff;
    if (has_extra_data) {
        prime1.reset(bignum_to_array(rsa->p));
        if (prime1->get() == NULL) {
            ALOGW("Could not convert prime1");
            return -1;
        }
        set_attribute(&privateKeyTemplate[templateOffset++], CKA_PRIME_1, prime1->get(),
                prime1->length());

        prime2.reset(bignum_to_array(rsa->q));
        if (prime2->get() == NULL) {
            ALOGW("Could not convert prime2");
            return -1;
        }
        set_attribute(&privateKeyTemplate[templateOffset++], CKA_PRIME_2, prime2->get(),
                prime2->length());

        exp1.reset(bignum_to_array(rsa->dmp1));
        if (exp1->get() == NULL) {
            ALOGW("Could not convert exponent 1");
            return -1;
        }
        set_attribute(&privateKeyTemplate[templateOffset++], CKA_EXPONENT_1, exp1->get(),
                exp1->length());

        exp2.reset(bignum_to_array(rsa->dmq1));
        if (exp2->get() == NULL) {
            ALOGW("Could not convert exponent 2");
            return -1;
        }
        set_attribute(&privateKeyTemplate[templateOffset++], CKA_EXPONENT_2, exp2->get(),
                exp2->length());

        coeff.reset(bignum_to_array(rsa->iqmp));
        if (coeff->get() == NULL) {
            ALOGW("Could not convert coefficient");
            return -1;
        }
        set_attribute(&privateKeyTemplate[templateOffset++], CKA_COEFFICIENT, coeff->get(),
                coeff->length());
    }

    CK_OBJECT_HANDLE hPrivateKey;
    rv = C_CreateObject(session.get(),
            privateKeyTemplate.get(),
            templateOffset,
            &hPrivateKey);
    if (rv != CKR_OK) {
        ALOGE("Creation of private key failed: 0x%x", rv);
        return -1;
    }
    ObjectHandle privateKey(&session, hPrivateKey);

    ALOGV("public handle = 0x%x, private handle = 0x%x", publicKey.get(), privateKey.get());

    return keyblob_save(objId.get(), key_blob, key_blob_length);
}

static int tee_get_keypair_public(const struct keymaster_device* dev,
        const uint8_t* key_blob, const size_t key_blob_length,
        uint8_t** x509_data, size_t* x509_data_length) {

    CryptoSession session(reinterpret_cast<CK_SESSION_HANDLE>(dev->context));

    ObjectHandle publicKey(&session);
    ObjectHandle privateKey(&session);

    if (keyblob_restore(&session, key_blob, key_blob_length, &publicKey, &privateKey)) {
        return -1;
    }

    if (x509_data == NULL || x509_data_length == NULL) {
        ALOGW("Provided destination variables were null");
        return -1;
    }

    CK_ATTRIBUTE attributes[] = {
            {CKA_MODULUS,         NULL, 0},
            {CKA_PUBLIC_EXPONENT, NULL, 0},
    };

    // Call first to get the sizes of the values.
    CK_RV rv = C_GetAttributeValue(session.get(), publicKey.get(), attributes,
            sizeof(attributes)/sizeof(CK_ATTRIBUTE));
    if (rv != CKR_OK) {
        ALOGW("Could not query attribute value sizes: 0x%02x", rv);
        return -1;
    }

    ByteArray modulus(new CK_BYTE[attributes[0].ulValueLen], attributes[0].ulValueLen);
    ByteArray exponent(new CK_BYTE[attributes[1].ulValueLen], attributes[1].ulValueLen);

    attributes[0].pValue = modulus.get();
    attributes[1].pValue = exponent.get();

    rv = C_GetAttributeValue(session.get(), publicKey.get(), attributes,
            sizeof(attributes) / sizeof(CK_ATTRIBUTE));
    if (rv != CKR_OK) {
        ALOGW("Could not query attribute values: 0x%02x", rv);
        return -1;
    }

    ALOGV("modulus is %d (ret=%d), exponent is %d (ret=%d)",
            modulus.length(), attributes[0].ulValueLen,
            exponent.length(), attributes[1].ulValueLen);

    /*
     * Work around a bug in the implementation. The first call to measure how large the array
     * should be sometimes returns values that are too large. The call to get the actual value
     * returns the correct length of the array, so use that instead.
     */
    modulus.setLength(attributes[0].ulValueLen);
    exponent.setLength(attributes[1].ulValueLen);

    Unique_RSA rsa(RSA_new());
    if (rsa.get() == NULL) {
        ALOGE("Could not allocate RSA structure");
        return -1;
    }

    rsa->n = BN_bin2bn(reinterpret_cast<const unsigned char*>(modulus.get()), modulus.length(),
            NULL);
    if (rsa->n == NULL) {
        logOpenSSLError("tee_get_keypair_public");
        return -1;
    }

    rsa->e = BN_bin2bn(reinterpret_cast<const unsigned char*>(exponent.get()), exponent.length(),
            NULL);
    if (rsa->e == NULL) {
        logOpenSSLError("tee_get_keypair_public");
        return -1;
    }

    Unique_EVP_PKEY pkey(EVP_PKEY_new());
    if (pkey.get() == NULL) {
        ALOGE("Could not allocate EVP_PKEY structure");
        return -1;
    }
    if (EVP_PKEY_assign_RSA(pkey.get(), rsa.get()) != 1) {
        logOpenSSLError("tee_get_keypair_public");
        return -1;
    }
    OWNERSHIP_TRANSFERRED(rsa);

    int len = i2d_PUBKEY(pkey.get(), NULL);
    if (len <= 0) {
        logOpenSSLError("tee_get_keypair_public");
        return -1;
    }

    UniquePtr<uint8_t> key(static_cast<uint8_t*>(malloc(len)));
    if (key.get() == NULL) {
        ALOGE("Could not allocate memory for public key data");
        return -1;
    }

    unsigned char* tmp = reinterpret_cast<unsigned char*>(key.get());
    if (i2d_PUBKEY(pkey.get(), &tmp) != len) {
        logOpenSSLError("tee_get_keypair_public");
        return -1;
    }

    ALOGV("Length of x509 data is %d", len);
    *x509_data_length = len;
    *x509_data = key.release();

    return 0;
}

static int tee_delete_keypair(const struct keymaster_device* dev,
            const uint8_t* key_blob, const size_t key_blob_length) {

    CryptoSession session(reinterpret_cast<CK_SESSION_HANDLE>(dev->context));

    ObjectHandle publicKey(&session);
    ObjectHandle privateKey(&session);

    if (keyblob_restore(&session, key_blob, key_blob_length, &publicKey, &privateKey)) {
        return -1;
    }

    // Delete the private key.
    CK_RV rv = C_DestroyObject(session.get(), privateKey.get());
    if (rv != CKR_OK) {
        ALOGW("Could destroy private key object: 0x%02x", rv);
        return -1;
    }

    // Delete the public key.
    rv = C_DestroyObject(session.get(), publicKey.get());
    if (rv != CKR_OK) {
        ALOGW("Could destroy public key object: 0x%02x", rv);
        return -1;
    }

    return 0;
}

static int tee_sign_data(const keymaster_device_t* dev,
        const void* params,
        const uint8_t* key_blob, const size_t key_blob_length,
        const uint8_t* data, const size_t dataLength,
        uint8_t** signedData, size_t* signedDataLength) {
    ALOGV("tee_sign_data(%p, %p, %llu, %p, %llu, %p, %p)", dev, key_blob,
            (unsigned long long) key_blob_length, data, (unsigned long long) dataLength, signedData,
            signedDataLength);

    if (params == NULL) {
        ALOGW("Signing params were null");
        return -1;
    }

    CryptoSession session(reinterpret_cast<CK_SESSION_HANDLE>(dev->context));

    ObjectHandle publicKey(&session);
    ObjectHandle privateKey(&session);

    if (keyblob_restore(&session, key_blob, key_blob_length, &publicKey, &privateKey)) {
        return -1;
    }
    ALOGV("public handle = 0x%x, private handle = 0x%x", publicKey.get(), privateKey.get());

    keymaster_rsa_sign_params_t* sign_params = (keymaster_rsa_sign_params_t*) params;
    if (sign_params->digest_type != DIGEST_NONE) {
        ALOGW("Cannot handle digest type %d", sign_params->digest_type);
        return -1;
    } else if (sign_params->padding_type != PADDING_NONE) {
        ALOGW("Cannot handle padding type %d", sign_params->padding_type);
        return -1;
    }

    CK_MECHANISM rawRsaMechanism = {
            CKM_RSA_X_509, NULL, 0
    };

    CK_RV rv = C_SignInit(session.get(), &rawRsaMechanism, privateKey.get());
    if (rv != CKR_OK) {
        ALOGV("C_SignInit failed: 0x%x", rv);
        return -1;
    }

    CK_BYTE signature[1024];
    CK_ULONG signatureLength = 1024;

    rv = C_Sign(session.get(), data, dataLength, signature, &signatureLength);
    if (rv != CKR_OK) {
        ALOGV("C_SignFinal failed: 0x%x", rv);
        return -1;
    }

    UniquePtr<uint8_t[]> finalSignature(new uint8_t[signatureLength]);
    if (finalSignature.get() == NULL) {
        ALOGE("Couldn't allocate memory to copy signature");
        return -1;
    }

    memcpy(finalSignature.get(), signature, signatureLength);

    *signedData = finalSignature.release();
    *signedDataLength = static_cast<size_t>(signatureLength);

    ALOGV("tee_sign_data(%p, %p, %llu, %p, %llu, %p, %p) => %p size %llu", dev, key_blob,
            (unsigned long long) key_blob_length, data, (unsigned long long) dataLength, signedData,
            signedDataLength, *signedData, (unsigned long long) *signedDataLength);

    return 0;
}

static int tee_verify_data(const keymaster_device_t* dev,
        const void* params,
        const uint8_t* keyBlob, const size_t keyBlobLength,
        const uint8_t* signedData, const size_t signedDataLength,
        const uint8_t* signature, const size_t signatureLength) {
    ALOGV("tee_verify_data(%p, %p, %llu, %p, %llu, %p, %llu)", dev, keyBlob,
            (unsigned long long) keyBlobLength, signedData, (unsigned long long) signedDataLength,
            signature, (unsigned long long) signatureLength);

    if (params == NULL) {
        ALOGW("Verification params were null");
        return -1;
    }

    CryptoSession session(reinterpret_cast<CK_SESSION_HANDLE>(dev->context));

    ObjectHandle publicKey(&session);
    ObjectHandle privateKey(&session);

    if (keyblob_restore(&session, keyBlob, keyBlobLength, &publicKey, &privateKey)) {
        return -1;
    }
    ALOGV("public handle = 0x%x, private handle = 0x%x", publicKey.get(), privateKey.get());

    keymaster_rsa_sign_params_t* sign_params = (keymaster_rsa_sign_params_t*) params;
    if (sign_params->digest_type != DIGEST_NONE) {
        ALOGW("Cannot handle digest type %d", sign_params->digest_type);
        return -1;
    } else if (sign_params->padding_type != PADDING_NONE) {
        ALOGW("Cannot handle padding type %d", sign_params->padding_type);
        return -1;
    }

    CK_MECHANISM rawRsaMechanism = {
            CKM_RSA_X_509, NULL, 0
    };

    CK_RV rv = C_VerifyInit(session.get(), &rawRsaMechanism, publicKey.get());
    if (rv != CKR_OK) {
        ALOGV("C_VerifyInit failed: 0x%x", rv);
        return -1;
    }

    // This is a bad prototype for this function. C_Verify should have only const args.
    rv = C_Verify(session.get(), signedData, signedDataLength,
            const_cast<unsigned char*>(signature), signatureLength);
    if (rv != CKR_OK) {
        ALOGV("C_Verify failed: 0x%x", rv);
        return -1;
    }

    return 0;
}

/* Close an opened OpenSSL instance */
static int tee_close(hw_device_t *dev) {
    keymaster_device_t *keymaster_dev = (keymaster_device_t *) dev;
    if (keymaster_dev != NULL) {
        CK_SESSION_HANDLE handle = reinterpret_cast<CK_SESSION_HANDLE>(keymaster_dev->context);
        if (handle != CK_INVALID_HANDLE) {
            C_CloseSession(handle);
        }
    }

    CK_RV finalizeRV = C_Finalize(NULL_PTR);
    if (finalizeRV != CKR_OK) {
        ALOGE("Error closing the TEE");
    }
    free(dev);

    return 0;
}

/*
 * Generic device handling
 */
static int tee_open(const hw_module_t* module, const char* name,
        hw_device_t** device) {
    if (strcmp(name, KEYSTORE_KEYMASTER) != 0)
        return -EINVAL;

    Unique_keymaster_device_t dev(new keymaster_device_t);
    if (dev.get() == NULL)
        return -ENOMEM;

    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 1;
    dev->common.module = (struct hw_module_t*) module;
    dev->common.close = tee_close;

    dev->generate_keypair = tee_generate_keypair;
    dev->import_keypair = tee_import_keypair;
    dev->get_keypair_public = tee_get_keypair_public;
    dev->delete_keypair = tee_delete_keypair;
    dev->sign_data = tee_sign_data;
    dev->verify_data = tee_verify_data;
    dev->delete_all = NULL;

    CK_RV initializeRV = C_Initialize(NULL);
    if (initializeRV != CKR_OK) {
        ALOGE("Error initializing TEE: 0x%x", initializeRV);
        return -ENODEV;
    }

    CK_INFO info;
    CK_RV infoRV = C_GetInfo(&info);
    if (infoRV != CKR_OK) {
        (void) C_Finalize(NULL_PTR);
        ALOGE("Error getting information about TEE during initialization: 0x%x", infoRV);
        return -ENODEV;
    }

    ALOGI("C_GetInfo cryptokiVer=%d.%d manufID=%s flags=%d libDesc=%s libVer=%d.%d\n",
           info.cryptokiVersion.major, info.cryptokiVersion.minor,
           info.manufacturerID, info.flags, info.libraryDescription,
           info.libraryVersion.major, info.libraryVersion.minor);

    CK_SESSION_HANDLE sessionHandle = CK_INVALID_HANDLE;

    CK_RV openSessionRV = C_OpenSession(CKV_TOKEN_USER,
            CKF_SERIAL_SESSION | CKF_RW_SESSION,
            NULL,
            NULL,
            &sessionHandle);

    if (openSessionRV != CKR_OK || sessionHandle == CK_INVALID_HANDLE) {
        (void) C_Finalize(NULL_PTR);
        ALOGE("Error opening primary session with TEE: 0x%x", openSessionRV);
        return -1;
    }

    ERR_load_crypto_strings();
    ERR_load_BIO_strings();

    dev->context = reinterpret_cast<void*>(sessionHandle);
    *device = reinterpret_cast<hw_device_t*>(dev.release());

    return 0;
}

static struct hw_module_methods_t keystore_module_methods = {
    open: tee_open,
};

struct keystore_module HAL_MODULE_INFO_SYM
__attribute__ ((visibility ("default"))) = {
    common: {
        tag: HARDWARE_MODULE_TAG,
        version_major: 1,
        version_minor: 0,
        id: KEYSTORE_HARDWARE_MODULE_ID,
        name: "Keymaster TEE HAL",
        author: "The Android Open Source Project",
        methods: &keystore_module_methods,
        dso: 0,
        reserved: {},
    },
};
