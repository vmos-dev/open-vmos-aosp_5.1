/*
 * Copyright (C) 2009 The Android Open Source Project
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

//#define LOG_NDEBUG 0
#define LOG_TAG "keystore"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <arpa/inet.h>

#include <openssl/aes.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include <openssl/pem.h>

#include <hardware/keymaster.h>

#include <keymaster/softkeymaster.h>

#include <UniquePtr.h>
#include <utils/String8.h>
#include <utils/Vector.h>

#include <keystore/IKeystoreService.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>

#include <cutils/log.h>
#include <cutils/sockets.h>
#include <private/android_filesystem_config.h>

#include <keystore/keystore.h>

#include <selinux/android.h>

#include "defaults.h"

/* KeyStore is a secured storage for key-value pairs. In this implementation,
 * each file stores one key-value pair. Keys are encoded in file names, and
 * values are encrypted with checksums. The encryption key is protected by a
 * user-defined password. To keep things simple, buffers are always larger than
 * the maximum space we needed, so boundary checks on buffers are omitted. */

#define KEY_SIZE        ((NAME_MAX - 15) / 2)
#define VALUE_SIZE      32768
#define PASSWORD_SIZE   VALUE_SIZE


struct BIGNUM_Delete {
    void operator()(BIGNUM* p) const {
        BN_free(p);
    }
};
typedef UniquePtr<BIGNUM, BIGNUM_Delete> Unique_BIGNUM;

struct BIO_Delete {
    void operator()(BIO* p) const {
        BIO_free(p);
    }
};
typedef UniquePtr<BIO, BIO_Delete> Unique_BIO;

struct EVP_PKEY_Delete {
    void operator()(EVP_PKEY* p) const {
        EVP_PKEY_free(p);
    }
};
typedef UniquePtr<EVP_PKEY, EVP_PKEY_Delete> Unique_EVP_PKEY;

struct PKCS8_PRIV_KEY_INFO_Delete {
    void operator()(PKCS8_PRIV_KEY_INFO* p) const {
        PKCS8_PRIV_KEY_INFO_free(p);
    }
};
typedef UniquePtr<PKCS8_PRIV_KEY_INFO, PKCS8_PRIV_KEY_INFO_Delete> Unique_PKCS8_PRIV_KEY_INFO;


static int keymaster_device_initialize(keymaster_device_t** dev) {
    int rc;

    const hw_module_t* mod;
    rc = hw_get_module_by_class(KEYSTORE_HARDWARE_MODULE_ID, NULL, &mod);
    if (rc) {
        ALOGE("could not find any keystore module");
        goto out;
    }

    rc = keymaster_open(mod, dev);
    if (rc) {
        ALOGE("could not open keymaster device in %s (%s)",
            KEYSTORE_HARDWARE_MODULE_ID, strerror(-rc));
        goto out;
    }

    return 0;

out:
    *dev = NULL;
    return rc;
}

static void keymaster_device_release(keymaster_device_t* dev) {
    keymaster_close(dev);
}

/***************
 * PERMISSIONS *
 ***************/

/* Here are the permissions, actions, users, and the main function. */
typedef enum {
    P_TEST          = 1 << 0,
    P_GET           = 1 << 1,
    P_INSERT        = 1 << 2,
    P_DELETE        = 1 << 3,
    P_EXIST         = 1 << 4,
    P_SAW           = 1 << 5,
    P_RESET         = 1 << 6,
    P_PASSWORD      = 1 << 7,
    P_LOCK          = 1 << 8,
    P_UNLOCK        = 1 << 9,
    P_ZERO          = 1 << 10,
    P_SIGN          = 1 << 11,
    P_VERIFY        = 1 << 12,
    P_GRANT         = 1 << 13,
    P_DUPLICATE     = 1 << 14,
    P_CLEAR_UID     = 1 << 15,
    P_RESET_UID     = 1 << 16,
    P_SYNC_UID      = 1 << 17,
    P_PASSWORD_UID  = 1 << 18,
} perm_t;

static struct user_euid {
    uid_t uid;
    uid_t euid;
} user_euids[] = {
    {AID_VPN, AID_SYSTEM},
    {AID_WIFI, AID_SYSTEM},
    {AID_ROOT, AID_SYSTEM},
};

/* perm_labels associcated with keystore_key SELinux class verbs. */
const char *perm_labels[] = {
    "test",
    "get",
    "insert",
    "delete",
    "exist",
    "saw",
    "reset",
    "password",
    "lock",
    "unlock",
    "zero",
    "sign",
    "verify",
    "grant",
    "duplicate",
    "clear_uid",
    "reset_uid",
    "sync_uid",
    "password_uid",
};

static struct user_perm {
    uid_t uid;
    perm_t perms;
} user_perms[] = {
    {AID_SYSTEM, static_cast<perm_t>((uint32_t)(~0)) },
    {AID_VPN,    static_cast<perm_t>(P_GET | P_SIGN | P_VERIFY) },
    {AID_WIFI,   static_cast<perm_t>(P_GET | P_SIGN | P_VERIFY) },
    {AID_ROOT,   static_cast<perm_t>(P_GET) },
};

static const perm_t DEFAULT_PERMS = static_cast<perm_t>(P_TEST | P_GET | P_INSERT | P_DELETE | P_EXIST | P_SAW | P_SIGN
        | P_VERIFY);

static char *tctx;
static int ks_is_selinux_enabled;

static const char *get_perm_label(perm_t perm) {
    unsigned int index = ffs(perm);
    if (index > 0 && index <= (sizeof(perm_labels) / sizeof(perm_labels[0]))) {
        return perm_labels[index - 1];
    } else {
        ALOGE("Keystore: Failed to retrieve permission label.\n");
        abort();
    }
}

/**
 * Returns the app ID (in the Android multi-user sense) for the current
 * UNIX UID.
 */
static uid_t get_app_id(uid_t uid) {
    return uid % AID_USER;
}

/**
 * Returns the user ID (in the Android multi-user sense) for the current
 * UNIX UID.
 */
static uid_t get_user_id(uid_t uid) {
    return uid / AID_USER;
}

static bool keystore_selinux_check_access(uid_t uid, perm_t perm, pid_t spid) {
    if (!ks_is_selinux_enabled) {
        return true;
    }

    char *sctx = NULL;
    const char *selinux_class = "keystore_key";
    const char *str_perm = get_perm_label(perm);

    if (!str_perm) {
        return false;
    }

    if (getpidcon(spid, &sctx) != 0) {
        ALOGE("SELinux: Failed to get source pid context.\n");
        return false;
    }

    bool allowed = selinux_check_access(sctx, tctx, selinux_class, str_perm,
            NULL) == 0;
    freecon(sctx);
    return allowed;
}

static bool has_permission(uid_t uid, perm_t perm, pid_t spid) {
    // All system users are equivalent for multi-user support.
    if (get_app_id(uid) == AID_SYSTEM) {
        uid = AID_SYSTEM;
    }

    for (size_t i = 0; i < sizeof(user_perms)/sizeof(user_perms[0]); i++) {
        struct user_perm user = user_perms[i];
        if (user.uid == uid) {
            return (user.perms & perm) &&
                keystore_selinux_check_access(uid, perm, spid);
        }
    }

    return (DEFAULT_PERMS & perm) &&
        keystore_selinux_check_access(uid, perm, spid);
}

/**
 * Returns the UID that the callingUid should act as. This is here for
 * legacy support of the WiFi and VPN systems and should be removed
 * when WiFi can operate in its own namespace.
 */
static uid_t get_keystore_euid(uid_t uid) {
    for (size_t i = 0; i < sizeof(user_euids)/sizeof(user_euids[0]); i++) {
        struct user_euid user = user_euids[i];
        if (user.uid == uid) {
            return user.euid;
        }
    }

    return uid;
}

/**
 * Returns true if the callingUid is allowed to interact in the targetUid's
 * namespace.
 */
static bool is_granted_to(uid_t callingUid, uid_t targetUid) {
    for (size_t i = 0; i < sizeof(user_euids)/sizeof(user_euids[0]); i++) {
        struct user_euid user = user_euids[i];
        if (user.euid == callingUid && user.uid == targetUid) {
            return true;
        }
    }

    return false;
}

/**
 * Allow the system to perform some privileged tasks that have to do with
 * system maintenance. This should not be used for any function that uses
 * the keys in any way (e.g., signing).
 */
static bool is_self_or_system(uid_t callingUid, uid_t targetUid) {
    return callingUid == targetUid || callingUid == AID_SYSTEM;
}

/* Here is the encoding of keys. This is necessary in order to allow arbitrary
 * characters in keys. Characters in [0-~] are not encoded. Others are encoded
 * into two bytes. The first byte is one of [+-.] which represents the first
 * two bits of the character. The second byte encodes the rest of the bits into
 * [0-o]. Therefore in the worst case the length of a key gets doubled. Note
 * that Base64 cannot be used here due to the need of prefix match on keys. */

static size_t encode_key_length(const android::String8& keyName) {
    const uint8_t* in = reinterpret_cast<const uint8_t*>(keyName.string());
    size_t length = keyName.length();
    for (int i = length; i > 0; --i, ++in) {
        if (*in < '0' || *in > '~') {
            ++length;
        }
    }
    return length;
}

static int encode_key(char* out, const android::String8& keyName) {
    const uint8_t* in = reinterpret_cast<const uint8_t*>(keyName.string());
    size_t length = keyName.length();
    for (int i = length; i > 0; --i, ++in, ++out) {
        if (*in < '0' || *in > '~') {
            *out = '+' + (*in >> 6);
            *++out = '0' + (*in & 0x3F);
            ++length;
        } else {
            *out = *in;
        }
    }
    *out = '\0';
    return length;
}

/*
 * Converts from the "escaped" format on disk to actual name.
 * This will be smaller than the input string.
 *
 * Characters that should combine with the next at the end will be truncated.
 */
static size_t decode_key_length(const char* in, size_t length) {
    size_t outLength = 0;

    for (const char* end = in + length; in < end; in++) {
        /* This combines with the next character. */
        if (*in < '0' || *in > '~') {
            continue;
        }

        outLength++;
    }
    return outLength;
}

static void decode_key(char* out, const char* in, size_t length) {
    for (const char* end = in + length; in < end; in++) {
        if (*in < '0' || *in > '~') {
            /* Truncate combining characters at the end. */
            if (in + 1 >= end) {
                break;
            }

            *out = (*in++ - '+') << 6;
            *out++ |= (*in - '0') & 0x3F;
        } else {
            *out++ = *in;
        }
    }
    *out = '\0';
}

static size_t readFully(int fd, uint8_t* data, size_t size) {
    size_t remaining = size;
    while (remaining > 0) {
        ssize_t n = TEMP_FAILURE_RETRY(read(fd, data, remaining));
        if (n <= 0) {
            return size - remaining;
        }
        data += n;
        remaining -= n;
    }
    return size;
}

static size_t writeFully(int fd, uint8_t* data, size_t size) {
    size_t remaining = size;
    while (remaining > 0) {
        ssize_t n = TEMP_FAILURE_RETRY(write(fd, data, remaining));
        if (n < 0) {
            ALOGW("write failed: %s", strerror(errno));
            return size - remaining;
        }
        data += n;
        remaining -= n;
    }
    return size;
}

class Entropy {
public:
    Entropy() : mRandom(-1) {}
    ~Entropy() {
        if (mRandom >= 0) {
            close(mRandom);
        }
    }

    bool open() {
        const char* randomDevice = "/dev/urandom";
        mRandom = TEMP_FAILURE_RETRY(::open(randomDevice, O_RDONLY));
        if (mRandom < 0) {
            ALOGE("open: %s: %s", randomDevice, strerror(errno));
            return false;
        }
        return true;
    }

    bool generate_random_data(uint8_t* data, size_t size) const {
        return (readFully(mRandom, data, size) == size);
    }

private:
    int mRandom;
};

/* Here is the file format. There are two parts in blob.value, the secret and
 * the description. The secret is stored in ciphertext, and its original size
 * can be found in blob.length. The description is stored after the secret in
 * plaintext, and its size is specified in blob.info. The total size of the two
 * parts must be no more than VALUE_SIZE bytes. The first field is the version,
 * the second is the blob's type, and the third byte is flags. Fields other
 * than blob.info, blob.length, and blob.value are modified by encryptBlob()
 * and decryptBlob(). Thus they should not be accessed from outside. */

/* ** Note to future implementors of encryption: **
 * Currently this is the construction:
 *   metadata || Enc(MD5(data) || data)
 *
 * This should be the construction used for encrypting if re-implementing:
 *
 *   Derive independent keys for encryption and MAC:
 *     Kenc = AES_encrypt(masterKey, "Encrypt")
 *     Kmac = AES_encrypt(masterKey, "MAC")
 *
 *   Store this:
 *     metadata || AES_CTR_encrypt(Kenc, rand_IV, data) ||
 *             HMAC(Kmac, metadata || Enc(data))
 */
struct __attribute__((packed)) blob {
    uint8_t version;
    uint8_t type;
    uint8_t flags;
    uint8_t info;
    uint8_t vector[AES_BLOCK_SIZE];
    uint8_t encrypted[0]; // Marks offset to encrypted data.
    uint8_t digest[MD5_DIGEST_LENGTH];
    uint8_t digested[0]; // Marks offset to digested data.
    int32_t length; // in network byte order when encrypted
    uint8_t value[VALUE_SIZE + AES_BLOCK_SIZE];
};

typedef enum {
    TYPE_ANY = 0, // meta type that matches anything
    TYPE_GENERIC = 1,
    TYPE_MASTER_KEY = 2,
    TYPE_KEY_PAIR = 3,
} BlobType;

static const uint8_t CURRENT_BLOB_VERSION = 2;

class Blob {
public:
    Blob(const uint8_t* value, int32_t valueLength, const uint8_t* info, uint8_t infoLength,
            BlobType type) {
        mBlob.length = valueLength;
        memcpy(mBlob.value, value, valueLength);

        mBlob.info = infoLength;
        memcpy(mBlob.value + valueLength, info, infoLength);

        mBlob.version = CURRENT_BLOB_VERSION;
        mBlob.type = uint8_t(type);

        if (type == TYPE_MASTER_KEY) {
            mBlob.flags = KEYSTORE_FLAG_ENCRYPTED;
        } else {
            mBlob.flags = KEYSTORE_FLAG_NONE;
        }
    }

    Blob(blob b) {
        mBlob = b;
    }

    Blob() {}

    const uint8_t* getValue() const {
        return mBlob.value;
    }

    int32_t getLength() const {
        return mBlob.length;
    }

    const uint8_t* getInfo() const {
        return mBlob.value + mBlob.length;
    }

    uint8_t getInfoLength() const {
        return mBlob.info;
    }

    uint8_t getVersion() const {
        return mBlob.version;
    }

    bool isEncrypted() const {
        if (mBlob.version < 2) {
            return true;
        }

        return mBlob.flags & KEYSTORE_FLAG_ENCRYPTED;
    }

    void setEncrypted(bool encrypted) {
        if (encrypted) {
            mBlob.flags |= KEYSTORE_FLAG_ENCRYPTED;
        } else {
            mBlob.flags &= ~KEYSTORE_FLAG_ENCRYPTED;
        }
    }

    bool isFallback() const {
        return mBlob.flags & KEYSTORE_FLAG_FALLBACK;
    }

    void setFallback(bool fallback) {
        if (fallback) {
            mBlob.flags |= KEYSTORE_FLAG_FALLBACK;
        } else {
            mBlob.flags &= ~KEYSTORE_FLAG_FALLBACK;
        }
    }

    void setVersion(uint8_t version) {
        mBlob.version = version;
    }

    BlobType getType() const {
        return BlobType(mBlob.type);
    }

    void setType(BlobType type) {
        mBlob.type = uint8_t(type);
    }

    ResponseCode writeBlob(const char* filename, AES_KEY *aes_key, State state, Entropy* entropy) {
        ALOGV("writing blob %s", filename);
        if (isEncrypted()) {
            if (state != STATE_NO_ERROR) {
                ALOGD("couldn't insert encrypted blob while not unlocked");
                return LOCKED;
            }

            if (!entropy->generate_random_data(mBlob.vector, AES_BLOCK_SIZE)) {
                ALOGW("Could not read random data for: %s", filename);
                return SYSTEM_ERROR;
            }
        }

        // data includes the value and the value's length
        size_t dataLength = mBlob.length + sizeof(mBlob.length);
        // pad data to the AES_BLOCK_SIZE
        size_t digestedLength = ((dataLength + AES_BLOCK_SIZE - 1)
                                 / AES_BLOCK_SIZE * AES_BLOCK_SIZE);
        // encrypted data includes the digest value
        size_t encryptedLength = digestedLength + MD5_DIGEST_LENGTH;
        // move info after space for padding
        memmove(&mBlob.encrypted[encryptedLength], &mBlob.value[mBlob.length], mBlob.info);
        // zero padding area
        memset(mBlob.value + mBlob.length, 0, digestedLength - dataLength);

        mBlob.length = htonl(mBlob.length);

        if (isEncrypted()) {
            MD5(mBlob.digested, digestedLength, mBlob.digest);

            uint8_t vector[AES_BLOCK_SIZE];
            memcpy(vector, mBlob.vector, AES_BLOCK_SIZE);
            AES_cbc_encrypt(mBlob.encrypted, mBlob.encrypted, encryptedLength,
                            aes_key, vector, AES_ENCRYPT);
        }

        size_t headerLength = (mBlob.encrypted - (uint8_t*) &mBlob);
        size_t fileLength = encryptedLength + headerLength + mBlob.info;

        const char* tmpFileName = ".tmp";
        int out = TEMP_FAILURE_RETRY(open(tmpFileName,
                O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR));
        if (out < 0) {
            ALOGW("could not open file: %s: %s", tmpFileName, strerror(errno));
            return SYSTEM_ERROR;
        }
        size_t writtenBytes = writeFully(out, (uint8_t*) &mBlob, fileLength);
        if (close(out) != 0) {
            return SYSTEM_ERROR;
        }
        if (writtenBytes != fileLength) {
            ALOGW("blob not fully written %zu != %zu", writtenBytes, fileLength);
            unlink(tmpFileName);
            return SYSTEM_ERROR;
        }
        if (rename(tmpFileName, filename) == -1) {
            ALOGW("could not rename blob to %s: %s", filename, strerror(errno));
            return SYSTEM_ERROR;
        }
        return NO_ERROR;
    }

    ResponseCode readBlob(const char* filename, AES_KEY *aes_key, State state) {
        ALOGV("reading blob %s", filename);
        int in = TEMP_FAILURE_RETRY(open(filename, O_RDONLY));
        if (in < 0) {
            return (errno == ENOENT) ? KEY_NOT_FOUND : SYSTEM_ERROR;
        }
        // fileLength may be less than sizeof(mBlob) since the in
        // memory version has extra padding to tolerate rounding up to
        // the AES_BLOCK_SIZE
        size_t fileLength = readFully(in, (uint8_t*) &mBlob, sizeof(mBlob));
        if (close(in) != 0) {
            return SYSTEM_ERROR;
        }

        if (isEncrypted() && (state != STATE_NO_ERROR)) {
            return LOCKED;
        }

        size_t headerLength = (mBlob.encrypted - (uint8_t*) &mBlob);
        if (fileLength < headerLength) {
            return VALUE_CORRUPTED;
        }

        ssize_t encryptedLength = fileLength - (headerLength + mBlob.info);
        if (encryptedLength < 0) {
            return VALUE_CORRUPTED;
        }

        ssize_t digestedLength;
        if (isEncrypted()) {
            if (encryptedLength % AES_BLOCK_SIZE != 0) {
                return VALUE_CORRUPTED;
            }

            AES_cbc_encrypt(mBlob.encrypted, mBlob.encrypted, encryptedLength, aes_key,
                            mBlob.vector, AES_DECRYPT);
            digestedLength = encryptedLength - MD5_DIGEST_LENGTH;
            uint8_t computedDigest[MD5_DIGEST_LENGTH];
            MD5(mBlob.digested, digestedLength, computedDigest);
            if (memcmp(mBlob.digest, computedDigest, MD5_DIGEST_LENGTH) != 0) {
                return VALUE_CORRUPTED;
            }
        } else {
            digestedLength = encryptedLength;
        }

        ssize_t maxValueLength = digestedLength - sizeof(mBlob.length);
        mBlob.length = ntohl(mBlob.length);
        if (mBlob.length < 0 || mBlob.length > maxValueLength) {
            return VALUE_CORRUPTED;
        }
        if (mBlob.info != 0) {
            // move info from after padding to after data
            memmove(&mBlob.value[mBlob.length], &mBlob.value[maxValueLength], mBlob.info);
        }
        return ::NO_ERROR;
    }

private:
    struct blob mBlob;
};

class UserState {
public:
    UserState(uid_t userId) : mUserId(userId), mRetry(MAX_RETRY) {
        asprintf(&mUserDir, "user_%u", mUserId);
        asprintf(&mMasterKeyFile, "%s/.masterkey", mUserDir);
    }

    ~UserState() {
        free(mUserDir);
        free(mMasterKeyFile);
    }

    bool initialize() {
        if ((mkdir(mUserDir, S_IRUSR | S_IWUSR | S_IXUSR) < 0) && (errno != EEXIST)) {
            ALOGE("Could not create directory '%s'", mUserDir);
            return false;
        }

        if (access(mMasterKeyFile, R_OK) == 0) {
            setState(STATE_LOCKED);
        } else {
            setState(STATE_UNINITIALIZED);
        }

        return true;
    }

    uid_t getUserId() const {
        return mUserId;
    }

    const char* getUserDirName() const {
        return mUserDir;
    }

    const char* getMasterKeyFileName() const {
        return mMasterKeyFile;
    }

    void setState(State state) {
        mState = state;
        if (mState == STATE_NO_ERROR || mState == STATE_UNINITIALIZED) {
            mRetry = MAX_RETRY;
        }
    }

    State getState() const {
        return mState;
    }

    int8_t getRetry() const {
        return mRetry;
    }

    void zeroizeMasterKeysInMemory() {
        memset(mMasterKey, 0, sizeof(mMasterKey));
        memset(mSalt, 0, sizeof(mSalt));
        memset(&mMasterKeyEncryption, 0, sizeof(mMasterKeyEncryption));
        memset(&mMasterKeyDecryption, 0, sizeof(mMasterKeyDecryption));
    }

    ResponseCode initialize(const android::String8& pw, Entropy* entropy) {
        if (!generateMasterKey(entropy)) {
            return SYSTEM_ERROR;
        }
        ResponseCode response = writeMasterKey(pw, entropy);
        if (response != NO_ERROR) {
            return response;
        }
        setupMasterKeys();
        return ::NO_ERROR;
    }

    ResponseCode copyMasterKey(UserState* src) {
        if (mState != STATE_UNINITIALIZED) {
            return ::SYSTEM_ERROR;
        }
        if (src->getState() != STATE_NO_ERROR) {
            return ::SYSTEM_ERROR;
        }
        memcpy(mMasterKey, src->mMasterKey, MASTER_KEY_SIZE_BYTES);
        setupMasterKeys();
        return ::NO_ERROR;
    }

    ResponseCode writeMasterKey(const android::String8& pw, Entropy* entropy) {
        uint8_t passwordKey[MASTER_KEY_SIZE_BYTES];
        generateKeyFromPassword(passwordKey, MASTER_KEY_SIZE_BYTES, pw, mSalt);
        AES_KEY passwordAesKey;
        AES_set_encrypt_key(passwordKey, MASTER_KEY_SIZE_BITS, &passwordAesKey);
        Blob masterKeyBlob(mMasterKey, sizeof(mMasterKey), mSalt, sizeof(mSalt), TYPE_MASTER_KEY);
        return masterKeyBlob.writeBlob(mMasterKeyFile, &passwordAesKey, STATE_NO_ERROR, entropy);
    }

    ResponseCode readMasterKey(const android::String8& pw, Entropy* entropy) {
        int in = TEMP_FAILURE_RETRY(open(mMasterKeyFile, O_RDONLY));
        if (in < 0) {
            return SYSTEM_ERROR;
        }

        // we read the raw blob to just to get the salt to generate
        // the AES key, then we create the Blob to use with decryptBlob
        blob rawBlob;
        size_t length = readFully(in, (uint8_t*) &rawBlob, sizeof(rawBlob));
        if (close(in) != 0) {
            return SYSTEM_ERROR;
        }
        // find salt at EOF if present, otherwise we have an old file
        uint8_t* salt;
        if (length > SALT_SIZE && rawBlob.info == SALT_SIZE) {
            salt = (uint8_t*) &rawBlob + length - SALT_SIZE;
        } else {
            salt = NULL;
        }
        uint8_t passwordKey[MASTER_KEY_SIZE_BYTES];
        generateKeyFromPassword(passwordKey, MASTER_KEY_SIZE_BYTES, pw, salt);
        AES_KEY passwordAesKey;
        AES_set_decrypt_key(passwordKey, MASTER_KEY_SIZE_BITS, &passwordAesKey);
        Blob masterKeyBlob(rawBlob);
        ResponseCode response = masterKeyBlob.readBlob(mMasterKeyFile, &passwordAesKey,
                STATE_NO_ERROR);
        if (response == SYSTEM_ERROR) {
            return response;
        }
        if (response == NO_ERROR && masterKeyBlob.getLength() == MASTER_KEY_SIZE_BYTES) {
            // if salt was missing, generate one and write a new master key file with the salt.
            if (salt == NULL) {
                if (!generateSalt(entropy)) {
                    return SYSTEM_ERROR;
                }
                response = writeMasterKey(pw, entropy);
            }
            if (response == NO_ERROR) {
                memcpy(mMasterKey, masterKeyBlob.getValue(), MASTER_KEY_SIZE_BYTES);
                setupMasterKeys();
            }
            return response;
        }
        if (mRetry <= 0) {
            reset();
            return UNINITIALIZED;
        }
        --mRetry;
        switch (mRetry) {
            case 0: return WRONG_PASSWORD_0;
            case 1: return WRONG_PASSWORD_1;
            case 2: return WRONG_PASSWORD_2;
            case 3: return WRONG_PASSWORD_3;
            default: return WRONG_PASSWORD_3;
        }
    }

    AES_KEY* getEncryptionKey() {
        return &mMasterKeyEncryption;
    }

    AES_KEY* getDecryptionKey() {
        return &mMasterKeyDecryption;
    }

    bool reset() {
        DIR* dir = opendir(getUserDirName());
        if (!dir) {
            ALOGW("couldn't open user directory: %s", strerror(errno));
            return false;
        }

        struct dirent* file;
        while ((file = readdir(dir)) != NULL) {
            // We only care about files.
            if (file->d_type != DT_REG) {
                continue;
            }

            // Skip anything that starts with a "."
            if (file->d_name[0] == '.' && strcmp(".masterkey", file->d_name)) {
                continue;
            }

            unlinkat(dirfd(dir), file->d_name, 0);
        }
        closedir(dir);
        return true;
    }

private:
    static const int MASTER_KEY_SIZE_BYTES = 16;
    static const int MASTER_KEY_SIZE_BITS = MASTER_KEY_SIZE_BYTES * 8;

    static const int MAX_RETRY = 4;
    static const size_t SALT_SIZE = 16;

    void generateKeyFromPassword(uint8_t* key, ssize_t keySize, const android::String8& pw,
            uint8_t* salt) {
        size_t saltSize;
        if (salt != NULL) {
            saltSize = SALT_SIZE;
        } else {
            // pre-gingerbread used this hardwired salt, readMasterKey will rewrite these when found
            salt = (uint8_t*) "keystore";
            // sizeof = 9, not strlen = 8
            saltSize = sizeof("keystore");
        }

        PKCS5_PBKDF2_HMAC_SHA1(reinterpret_cast<const char*>(pw.string()), pw.length(), salt,
                saltSize, 8192, keySize, key);
    }

    bool generateSalt(Entropy* entropy) {
        return entropy->generate_random_data(mSalt, sizeof(mSalt));
    }

    bool generateMasterKey(Entropy* entropy) {
        if (!entropy->generate_random_data(mMasterKey, sizeof(mMasterKey))) {
            return false;
        }
        if (!generateSalt(entropy)) {
            return false;
        }
        return true;
    }

    void setupMasterKeys() {
        AES_set_encrypt_key(mMasterKey, MASTER_KEY_SIZE_BITS, &mMasterKeyEncryption);
        AES_set_decrypt_key(mMasterKey, MASTER_KEY_SIZE_BITS, &mMasterKeyDecryption);
        setState(STATE_NO_ERROR);
    }

    uid_t mUserId;

    char* mUserDir;
    char* mMasterKeyFile;

    State mState;
    int8_t mRetry;

    uint8_t mMasterKey[MASTER_KEY_SIZE_BYTES];
    uint8_t mSalt[SALT_SIZE];

    AES_KEY mMasterKeyEncryption;
    AES_KEY mMasterKeyDecryption;
};

typedef struct {
    uint32_t uid;
    const uint8_t* filename;
} grant_t;

class KeyStore {
public:
    KeyStore(Entropy* entropy, keymaster_device_t* device)
        : mEntropy(entropy)
        , mDevice(device)
    {
        memset(&mMetaData, '\0', sizeof(mMetaData));
    }

    ~KeyStore() {
        for (android::Vector<grant_t*>::iterator it(mGrants.begin());
                it != mGrants.end(); it++) {
            delete *it;
        }
        mGrants.clear();

        for (android::Vector<UserState*>::iterator it(mMasterKeys.begin());
                it != mMasterKeys.end(); it++) {
            delete *it;
        }
        mMasterKeys.clear();
    }

    keymaster_device_t* getDevice() const {
        return mDevice;
    }

    ResponseCode initialize() {
        readMetaData();
        if (upgradeKeystore()) {
            writeMetaData();
        }

        return ::NO_ERROR;
    }

    State getState(uid_t uid) {
        return getUserState(uid)->getState();
    }

    ResponseCode initializeUser(const android::String8& pw, uid_t uid) {
        UserState* userState = getUserState(uid);
        return userState->initialize(pw, mEntropy);
    }

    ResponseCode copyMasterKey(uid_t src, uid_t uid) {
        UserState *userState = getUserState(uid);
        UserState *initState = getUserState(src);
        return userState->copyMasterKey(initState);
    }

    ResponseCode writeMasterKey(const android::String8& pw, uid_t uid) {
        UserState* userState = getUserState(uid);
        return userState->writeMasterKey(pw, mEntropy);
    }

    ResponseCode readMasterKey(const android::String8& pw, uid_t uid) {
        UserState* userState = getUserState(uid);
        return userState->readMasterKey(pw, mEntropy);
    }

    android::String8 getKeyName(const android::String8& keyName) {
        char encoded[encode_key_length(keyName) + 1];	// add 1 for null char
        encode_key(encoded, keyName);
        return android::String8(encoded);
    }

    android::String8 getKeyNameForUid(const android::String8& keyName, uid_t uid) {
        char encoded[encode_key_length(keyName) + 1];	// add 1 for null char
        encode_key(encoded, keyName);
        return android::String8::format("%u_%s", uid, encoded);
    }

    android::String8 getKeyNameForUidWithDir(const android::String8& keyName, uid_t uid) {
        char encoded[encode_key_length(keyName) + 1];	// add 1 for null char
        encode_key(encoded, keyName);
        return android::String8::format("%s/%u_%s", getUserState(uid)->getUserDirName(), uid,
                encoded);
    }

    bool reset(uid_t uid) {
        android::String8 prefix("");
        android::Vector<android::String16> aliases;
        if (saw(prefix, &aliases, uid) != ::NO_ERROR) {
            return ::SYSTEM_ERROR;
        }

        UserState* userState = getUserState(uid);
        for (uint32_t i = 0; i < aliases.size(); i++) {
            android::String8 filename(aliases[i]);
            filename = android::String8::format("%s/%s", userState->getUserDirName(),
                    getKeyName(filename).string());
            del(filename, ::TYPE_ANY, uid);
        }

        userState->zeroizeMasterKeysInMemory();
        userState->setState(STATE_UNINITIALIZED);
        return userState->reset();
    }

    bool isEmpty(uid_t uid) const {
        const UserState* userState = getUserState(uid);
        if (userState == NULL || userState->getState() == STATE_UNINITIALIZED) {
            return true;
        }

        DIR* dir = opendir(userState->getUserDirName());
        if (!dir) {
            return true;
        }

        bool result = true;
        struct dirent* file;
        while ((file = readdir(dir)) != NULL) {
            // We only care about files.
            if (file->d_type != DT_REG) {
                continue;
            }

            // Skip anything that starts with a "."
            if (file->d_name[0] == '.') {
                continue;
            }

            result = false;
            break;
        }
        closedir(dir);
        return result;
    }

    void lock(uid_t uid) {
        UserState* userState = getUserState(uid);
        userState->zeroizeMasterKeysInMemory();
        userState->setState(STATE_LOCKED);
    }

    ResponseCode get(const char* filename, Blob* keyBlob, const BlobType type, uid_t uid) {
        UserState* userState = getUserState(uid);
        ResponseCode rc = keyBlob->readBlob(filename, userState->getDecryptionKey(),
                userState->getState());
        if (rc != NO_ERROR) {
            return rc;
        }

        const uint8_t version = keyBlob->getVersion();
        if (version < CURRENT_BLOB_VERSION) {
            /* If we upgrade the key, we need to write it to disk again. Then
             * it must be read it again since the blob is encrypted each time
             * it's written.
             */
            if (upgradeBlob(filename, keyBlob, version, type, uid)) {
                if ((rc = this->put(filename, keyBlob, uid)) != NO_ERROR
                        || (rc = keyBlob->readBlob(filename, userState->getDecryptionKey(),
                                userState->getState())) != NO_ERROR) {
                    return rc;
                }
            }
        }

        /*
         * This will upgrade software-backed keys to hardware-backed keys when
         * the HAL for the device supports the newer key types.
         */
        if (rc == NO_ERROR && type == TYPE_KEY_PAIR
                && mDevice->common.module->module_api_version >= KEYMASTER_MODULE_API_VERSION_0_2
                && keyBlob->isFallback()) {
            ResponseCode imported = importKey(keyBlob->getValue(), keyBlob->getLength(), filename,
                    uid, keyBlob->isEncrypted() ? KEYSTORE_FLAG_ENCRYPTED : KEYSTORE_FLAG_NONE);

            // The HAL allowed the import, reget the key to have the "fresh"
            // version.
            if (imported == NO_ERROR) {
                rc = get(filename, keyBlob, TYPE_KEY_PAIR, uid);
            }
        }

        if (type != TYPE_ANY && keyBlob->getType() != type) {
            ALOGW("key found but type doesn't match: %d vs %d", keyBlob->getType(), type);
            return KEY_NOT_FOUND;
        }

        return rc;
    }

    ResponseCode put(const char* filename, Blob* keyBlob, uid_t uid) {
        UserState* userState = getUserState(uid);
        return keyBlob->writeBlob(filename, userState->getEncryptionKey(), userState->getState(),
                mEntropy);
    }

    ResponseCode del(const char *filename, const BlobType type, uid_t uid) {
        Blob keyBlob;
        ResponseCode rc = get(filename, &keyBlob, type, uid);
        if (rc != ::NO_ERROR) {
            return rc;
        }

        if (keyBlob.getType() == ::TYPE_KEY_PAIR) {
            // A device doesn't have to implement delete_keypair.
            if (mDevice->delete_keypair != NULL && !keyBlob.isFallback()) {
                if (mDevice->delete_keypair(mDevice, keyBlob.getValue(), keyBlob.getLength())) {
                    rc = ::SYSTEM_ERROR;
                }
            }
        }
        if (rc != ::NO_ERROR) {
            return rc;
        }

        return (unlink(filename) && errno != ENOENT) ? ::SYSTEM_ERROR : ::NO_ERROR;
    }

    ResponseCode saw(const android::String8& prefix, android::Vector<android::String16> *matches,
            uid_t uid) {

        UserState* userState = getUserState(uid);
        size_t n = prefix.length();

        DIR* dir = opendir(userState->getUserDirName());
        if (!dir) {
            ALOGW("can't open directory for user: %s", strerror(errno));
            return ::SYSTEM_ERROR;
        }

        struct dirent* file;
        while ((file = readdir(dir)) != NULL) {
            // We only care about files.
            if (file->d_type != DT_REG) {
                continue;
            }

            // Skip anything that starts with a "."
            if (file->d_name[0] == '.') {
                continue;
            }

            if (!strncmp(prefix.string(), file->d_name, n)) {
                const char* p = &file->d_name[n];
                size_t plen = strlen(p);

                size_t extra = decode_key_length(p, plen);
                char *match = (char*) malloc(extra + 1);
                if (match != NULL) {
                    decode_key(match, p, plen);
                    matches->push(android::String16(match, extra));
                    free(match);
                } else {
                    ALOGW("could not allocate match of size %zd", extra);
                }
            }
        }
        closedir(dir);
        return ::NO_ERROR;
    }

    void addGrant(const char* filename, uid_t granteeUid) {
        const grant_t* existing = getGrant(filename, granteeUid);
        if (existing == NULL) {
            grant_t* grant = new grant_t;
            grant->uid = granteeUid;
            grant->filename = reinterpret_cast<const uint8_t*>(strdup(filename));
            mGrants.add(grant);
        }
    }

    bool removeGrant(const char* filename, uid_t granteeUid) {
        for (android::Vector<grant_t*>::iterator it(mGrants.begin());
                it != mGrants.end(); it++) {
            grant_t* grant = *it;
            if (grant->uid == granteeUid
                    && !strcmp(reinterpret_cast<const char*>(grant->filename), filename)) {
                mGrants.erase(it);
                return true;
            }
        }
        return false;
    }

    bool hasGrant(const char* filename, const uid_t uid) const {
        return getGrant(filename, uid) != NULL;
    }

    ResponseCode importKey(const uint8_t* key, size_t keyLen, const char* filename, uid_t uid,
            int32_t flags) {
        uint8_t* data;
        size_t dataLength;
        int rc;

        if (mDevice->import_keypair == NULL) {
            ALOGE("Keymaster doesn't support import!");
            return SYSTEM_ERROR;
        }

        bool isFallback = false;
        rc = mDevice->import_keypair(mDevice, key, keyLen, &data, &dataLength);
        if (rc) {
            /*
             * Maybe the device doesn't support this type of key. Try to use the
             * software fallback keymaster implementation. This is a little bit
             * lazier than checking the PKCS#8 key type, but the software
             * implementation will do that anyway.
             */
            rc = openssl_import_keypair(mDevice, key, keyLen, &data, &dataLength);
            isFallback = true;

            if (rc) {
                ALOGE("Error while importing keypair: %d", rc);
                return SYSTEM_ERROR;
            }
        }

        Blob keyBlob(data, dataLength, NULL, 0, TYPE_KEY_PAIR);
        free(data);

        keyBlob.setEncrypted(flags & KEYSTORE_FLAG_ENCRYPTED);
        keyBlob.setFallback(isFallback);

        return put(filename, &keyBlob, uid);
    }

    bool isHardwareBacked(const android::String16& keyType) const {
        if (mDevice == NULL) {
            ALOGW("can't get keymaster device");
            return false;
        }

        if (sRSAKeyType == keyType) {
            return (mDevice->flags & KEYMASTER_SOFTWARE_ONLY) == 0;
        } else {
            return (mDevice->flags & KEYMASTER_SOFTWARE_ONLY) == 0
                    && (mDevice->common.module->module_api_version
                            >= KEYMASTER_MODULE_API_VERSION_0_2);
        }
    }

    ResponseCode getKeyForName(Blob* keyBlob, const android::String8& keyName, const uid_t uid,
            const BlobType type) {
        android::String8 filepath8(getKeyNameForUidWithDir(keyName, uid));

        ResponseCode responseCode = get(filepath8.string(), keyBlob, type, uid);
        if (responseCode == NO_ERROR) {
            return responseCode;
        }

        // If this is one of the legacy UID->UID mappings, use it.
        uid_t euid = get_keystore_euid(uid);
        if (euid != uid) {
            filepath8 = getKeyNameForUidWithDir(keyName, euid);
            responseCode = get(filepath8.string(), keyBlob, type, uid);
            if (responseCode == NO_ERROR) {
                return responseCode;
            }
        }

        // They might be using a granted key.
        android::String8 filename8 = getKeyName(keyName);
        char* end;
        strtoul(filename8.string(), &end, 10);
        if (end[0] != '_' || end[1] == 0) {
            return KEY_NOT_FOUND;
        }
        filepath8 = android::String8::format("%s/%s", getUserState(uid)->getUserDirName(),
                filename8.string());
        if (!hasGrant(filepath8.string(), uid)) {
            return responseCode;
        }

        // It is a granted key. Try to load it.
        return get(filepath8.string(), keyBlob, type, uid);
    }

    /**
     * Returns any existing UserState or creates it if it doesn't exist.
     */
    UserState* getUserState(uid_t uid) {
        uid_t userId = get_user_id(uid);

        for (android::Vector<UserState*>::iterator it(mMasterKeys.begin());
                it != mMasterKeys.end(); it++) {
            UserState* state = *it;
            if (state->getUserId() == userId) {
                return state;
            }
        }

        UserState* userState = new UserState(userId);
        if (!userState->initialize()) {
            /* There's not much we can do if initialization fails. Trying to
             * unlock the keystore for that user will fail as well, so any
             * subsequent request for this user will just return SYSTEM_ERROR.
             */
            ALOGE("User initialization failed for %u; subsuquent operations will fail", userId);
        }
        mMasterKeys.add(userState);
        return userState;
    }

    /**
     * Returns NULL if the UserState doesn't already exist.
     */
    const UserState* getUserState(uid_t uid) const {
        uid_t userId = get_user_id(uid);

        for (android::Vector<UserState*>::const_iterator it(mMasterKeys.begin());
                it != mMasterKeys.end(); it++) {
            UserState* state = *it;
            if (state->getUserId() == userId) {
                return state;
            }
        }

        return NULL;
    }

private:
    static const char* sOldMasterKey;
    static const char* sMetaDataFile;
    static const android::String16 sRSAKeyType;
    Entropy* mEntropy;

    keymaster_device_t* mDevice;

    android::Vector<UserState*> mMasterKeys;

    android::Vector<grant_t*> mGrants;

    typedef struct {
        uint32_t version;
    } keystore_metadata_t;

    keystore_metadata_t mMetaData;

    const grant_t* getGrant(const char* filename, uid_t uid) const {
        for (android::Vector<grant_t*>::const_iterator it(mGrants.begin());
                it != mGrants.end(); it++) {
            grant_t* grant = *it;
            if (grant->uid == uid
                    && !strcmp(reinterpret_cast<const char*>(grant->filename), filename)) {
                return grant;
            }
        }
        return NULL;
    }

    /**
     * Upgrade code. This will upgrade the key from the current version
     * to whatever is newest.
     */
    bool upgradeBlob(const char* filename, Blob* blob, const uint8_t oldVersion,
            const BlobType type, uid_t uid) {
        bool updated = false;
        uint8_t version = oldVersion;

        /* From V0 -> V1: All old types were unknown */
        if (version == 0) {
            ALOGV("upgrading to version 1 and setting type %d", type);

            blob->setType(type);
            if (type == TYPE_KEY_PAIR) {
                importBlobAsKey(blob, filename, uid);
            }
            version = 1;
            updated = true;
        }

        /* From V1 -> V2: All old keys were encrypted */
        if (version == 1) {
            ALOGV("upgrading to version 2");

            blob->setEncrypted(true);
            version = 2;
            updated = true;
        }

        /*
         * If we've updated, set the key blob to the right version
         * and write it.
         */
        if (updated) {
            ALOGV("updated and writing file %s", filename);
            blob->setVersion(version);
        }

        return updated;
    }

    /**
     * Takes a blob that is an PEM-encoded RSA key as a byte array and
     * converts it to a DER-encoded PKCS#8 for import into a keymaster.
     * Then it overwrites the original blob with the new blob
     * format that is returned from the keymaster.
     */
    ResponseCode importBlobAsKey(Blob* blob, const char* filename, uid_t uid) {
        // We won't even write to the blob directly with this BIO, so const_cast is okay.
        Unique_BIO b(BIO_new_mem_buf(const_cast<uint8_t*>(blob->getValue()), blob->getLength()));
        if (b.get() == NULL) {
            ALOGE("Problem instantiating BIO");
            return SYSTEM_ERROR;
        }

        Unique_EVP_PKEY pkey(PEM_read_bio_PrivateKey(b.get(), NULL, NULL, NULL));
        if (pkey.get() == NULL) {
            ALOGE("Couldn't read old PEM file");
            return SYSTEM_ERROR;
        }

        Unique_PKCS8_PRIV_KEY_INFO pkcs8(EVP_PKEY2PKCS8(pkey.get()));
        int len = i2d_PKCS8_PRIV_KEY_INFO(pkcs8.get(), NULL);
        if (len < 0) {
            ALOGE("Couldn't measure PKCS#8 length");
            return SYSTEM_ERROR;
        }

        UniquePtr<unsigned char[]> pkcs8key(new unsigned char[len]);
        uint8_t* tmp = pkcs8key.get();
        if (i2d_PKCS8_PRIV_KEY_INFO(pkcs8.get(), &tmp) != len) {
            ALOGE("Couldn't convert to PKCS#8");
            return SYSTEM_ERROR;
        }

        ResponseCode rc = importKey(pkcs8key.get(), len, filename, uid,
                blob->isEncrypted() ? KEYSTORE_FLAG_ENCRYPTED : KEYSTORE_FLAG_NONE);
        if (rc != NO_ERROR) {
            return rc;
        }

        return get(filename, blob, TYPE_KEY_PAIR, uid);
    }

    void readMetaData() {
        int in = TEMP_FAILURE_RETRY(open(sMetaDataFile, O_RDONLY));
        if (in < 0) {
            return;
        }
        size_t fileLength = readFully(in, (uint8_t*) &mMetaData, sizeof(mMetaData));
        if (fileLength != sizeof(mMetaData)) {
            ALOGI("Metadata file is %zd bytes (%zd experted); upgrade?", fileLength,
                    sizeof(mMetaData));
        }
        close(in);
    }

    void writeMetaData() {
        const char* tmpFileName = ".metadata.tmp";
        int out = TEMP_FAILURE_RETRY(open(tmpFileName,
                O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR));
        if (out < 0) {
            ALOGE("couldn't write metadata file: %s", strerror(errno));
            return;
        }
        size_t fileLength = writeFully(out, (uint8_t*) &mMetaData, sizeof(mMetaData));
        if (fileLength != sizeof(mMetaData)) {
            ALOGI("Could only write %zd bytes to metadata file (%zd expected)", fileLength,
                    sizeof(mMetaData));
        }
        close(out);
        rename(tmpFileName, sMetaDataFile);
    }

    bool upgradeKeystore() {
        bool upgraded = false;

        if (mMetaData.version == 0) {
            UserState* userState = getUserState(0);

            // Initialize first so the directory is made.
            userState->initialize();

            // Migrate the old .masterkey file to user 0.
            if (access(sOldMasterKey, R_OK) == 0) {
                if (rename(sOldMasterKey, userState->getMasterKeyFileName()) < 0) {
                    ALOGE("couldn't migrate old masterkey: %s", strerror(errno));
                    return false;
                }
            }

            // Initialize again in case we had a key.
            userState->initialize();

            // Try to migrate existing keys.
            DIR* dir = opendir(".");
            if (!dir) {
                // Give up now; maybe we can upgrade later.
                ALOGE("couldn't open keystore's directory; something is wrong");
                return false;
            }

            struct dirent* file;
            while ((file = readdir(dir)) != NULL) {
                // We only care about files.
                if (file->d_type != DT_REG) {
                    continue;
                }

                // Skip anything that starts with a "."
                if (file->d_name[0] == '.') {
                    continue;
                }

                // Find the current file's user.
                char* end;
                unsigned long thisUid = strtoul(file->d_name, &end, 10);
                if (end[0] != '_' || end[1] == 0) {
                    continue;
                }
                UserState* otherUser = getUserState(thisUid);
                if (otherUser->getUserId() != 0) {
                    unlinkat(dirfd(dir), file->d_name, 0);
                }

                // Rename the file into user directory.
                DIR* otherdir = opendir(otherUser->getUserDirName());
                if (otherdir == NULL) {
                    ALOGW("couldn't open user directory for rename");
                    continue;
                }
                if (renameat(dirfd(dir), file->d_name, dirfd(otherdir), file->d_name) < 0) {
                    ALOGW("couldn't rename blob: %s: %s", file->d_name, strerror(errno));
                }
                closedir(otherdir);
            }
            closedir(dir);

            mMetaData.version = 1;
            upgraded = true;
        }

        return upgraded;
    }
};

const char* KeyStore::sOldMasterKey = ".masterkey";
const char* KeyStore::sMetaDataFile = ".metadata";

const android::String16 KeyStore::sRSAKeyType("RSA");

namespace android {
class KeyStoreProxy : public BnKeystoreService, public IBinder::DeathRecipient {
public:
    KeyStoreProxy(KeyStore* keyStore)
        : mKeyStore(keyStore)
    {
    }

    void binderDied(const wp<IBinder>&) {
        ALOGE("binder death detected");
    }

    int32_t test() {
        uid_t callingUid = IPCThreadState::self()->getCallingUid();
        pid_t spid = IPCThreadState::self()->getCallingPid();
        if (!has_permission(callingUid, P_TEST, spid)) {
            ALOGW("permission denied for %d: test", callingUid);
            return ::PERMISSION_DENIED;
        }

        return mKeyStore->getState(callingUid);
    }

    int32_t get(const String16& name, uint8_t** item, size_t* itemLength) {
        uid_t callingUid = IPCThreadState::self()->getCallingUid();
        pid_t spid = IPCThreadState::self()->getCallingPid();
        if (!has_permission(callingUid, P_GET, spid)) {
            ALOGW("permission denied for %d: get", callingUid);
            return ::PERMISSION_DENIED;
        }

        String8 name8(name);
        Blob keyBlob;

        ResponseCode responseCode = mKeyStore->getKeyForName(&keyBlob, name8, callingUid,
                TYPE_GENERIC);
        if (responseCode != ::NO_ERROR) {
            ALOGW("Could not read %s", name8.string());
            *item = NULL;
            *itemLength = 0;
            return responseCode;
        }

        *item = (uint8_t*) malloc(keyBlob.getLength());
        memcpy(*item, keyBlob.getValue(), keyBlob.getLength());
        *itemLength = keyBlob.getLength();

        return ::NO_ERROR;
    }

    int32_t insert(const String16& name, const uint8_t* item, size_t itemLength, int targetUid,
            int32_t flags) {
        pid_t spid = IPCThreadState::self()->getCallingPid();
        uid_t callingUid = IPCThreadState::self()->getCallingUid();
        if (!has_permission(callingUid, P_INSERT, spid)) {
            ALOGW("permission denied for %d: insert", callingUid);
            return ::PERMISSION_DENIED;
        }

        State state = mKeyStore->getState(callingUid);
        if ((flags & KEYSTORE_FLAG_ENCRYPTED) && !isKeystoreUnlocked(state)) {
            ALOGD("calling get in state: %d", state);
            return state;
        }

        if (targetUid == -1) {
            targetUid = callingUid;
        } else if (!is_granted_to(callingUid, targetUid)) {
            return ::PERMISSION_DENIED;
        }

        String8 name8(name);
        String8 filename(mKeyStore->getKeyNameForUidWithDir(name8, targetUid));

        Blob keyBlob(item, itemLength, NULL, 0, ::TYPE_GENERIC);
        keyBlob.setEncrypted(flags & KEYSTORE_FLAG_ENCRYPTED);

        return mKeyStore->put(filename.string(), &keyBlob, targetUid);
    }

    int32_t del(const String16& name, int targetUid) {
        uid_t callingUid = IPCThreadState::self()->getCallingUid();
        pid_t spid = IPCThreadState::self()->getCallingPid();
        if (!has_permission(callingUid, P_DELETE, spid)) {
            ALOGW("permission denied for %d: del", callingUid);
            return ::PERMISSION_DENIED;
        }

        if (targetUid == -1) {
            targetUid = callingUid;
        } else if (!is_granted_to(callingUid, targetUid)) {
            return ::PERMISSION_DENIED;
        }

        String8 name8(name);
        String8 filename(mKeyStore->getKeyNameForUidWithDir(name8, targetUid));
        return mKeyStore->del(filename.string(), ::TYPE_GENERIC, targetUid);
    }

    int32_t exist(const String16& name, int targetUid) {
        uid_t callingUid = IPCThreadState::self()->getCallingUid();
        pid_t spid = IPCThreadState::self()->getCallingPid();
        if (!has_permission(callingUid, P_EXIST, spid)) {
            ALOGW("permission denied for %d: exist", callingUid);
            return ::PERMISSION_DENIED;
        }

        if (targetUid == -1) {
            targetUid = callingUid;
        } else if (!is_granted_to(callingUid, targetUid)) {
            return ::PERMISSION_DENIED;
        }

        String8 name8(name);
        String8 filename(mKeyStore->getKeyNameForUidWithDir(name8, targetUid));

        if (access(filename.string(), R_OK) == -1) {
            return (errno != ENOENT) ? ::SYSTEM_ERROR : ::KEY_NOT_FOUND;
        }
        return ::NO_ERROR;
    }

    int32_t saw(const String16& prefix, int targetUid, Vector<String16>* matches) {
        uid_t callingUid = IPCThreadState::self()->getCallingUid();
        pid_t spid = IPCThreadState::self()->getCallingPid();
        if (!has_permission(callingUid, P_SAW, spid)) {
            ALOGW("permission denied for %d: saw", callingUid);
            return ::PERMISSION_DENIED;
        }

        if (targetUid == -1) {
            targetUid = callingUid;
        } else if (!is_granted_to(callingUid, targetUid)) {
            return ::PERMISSION_DENIED;
        }

        const String8 prefix8(prefix);
        String8 filename(mKeyStore->getKeyNameForUid(prefix8, targetUid));

        if (mKeyStore->saw(filename, matches, targetUid) != ::NO_ERROR) {
            return ::SYSTEM_ERROR;
        }
        return ::NO_ERROR;
    }

    int32_t reset() {
        uid_t callingUid = IPCThreadState::self()->getCallingUid();
        pid_t spid = IPCThreadState::self()->getCallingPid();
        if (!has_permission(callingUid, P_RESET, spid)) {
            ALOGW("permission denied for %d: reset", callingUid);
            return ::PERMISSION_DENIED;
        }

        return mKeyStore->reset(callingUid) ? ::NO_ERROR : ::SYSTEM_ERROR;
    }

    /*
     * Here is the history. To improve the security, the parameters to generate the
     * master key has been changed. To make a seamless transition, we update the
     * file using the same password when the user unlock it for the first time. If
     * any thing goes wrong during the transition, the new file will not overwrite
     * the old one. This avoids permanent damages of the existing data.
     */
    int32_t password(const String16& password) {
        uid_t callingUid = IPCThreadState::self()->getCallingUid();
        pid_t spid = IPCThreadState::self()->getCallingPid();
        if (!has_permission(callingUid, P_PASSWORD, spid)) {
            ALOGW("permission denied for %d: password", callingUid);
            return ::PERMISSION_DENIED;
        }

        const String8 password8(password);

        switch (mKeyStore->getState(callingUid)) {
            case ::STATE_UNINITIALIZED: {
                // generate master key, encrypt with password, write to file, initialize mMasterKey*.
                return mKeyStore->initializeUser(password8, callingUid);
            }
            case ::STATE_NO_ERROR: {
                // rewrite master key with new password.
                return mKeyStore->writeMasterKey(password8, callingUid);
            }
            case ::STATE_LOCKED: {
                // read master key, decrypt with password, initialize mMasterKey*.
                return mKeyStore->readMasterKey(password8, callingUid);
            }
        }
        return ::SYSTEM_ERROR;
    }

    int32_t lock() {
        uid_t callingUid = IPCThreadState::self()->getCallingUid();
        pid_t spid = IPCThreadState::self()->getCallingPid();
        if (!has_permission(callingUid, P_LOCK, spid)) {
            ALOGW("permission denied for %d: lock", callingUid);
            return ::PERMISSION_DENIED;
        }

        State state = mKeyStore->getState(callingUid);
        if (state != ::STATE_NO_ERROR) {
            ALOGD("calling lock in state: %d", state);
            return state;
        }

        mKeyStore->lock(callingUid);
        return ::NO_ERROR;
    }

    int32_t unlock(const String16& pw) {
        uid_t callingUid = IPCThreadState::self()->getCallingUid();
        pid_t spid = IPCThreadState::self()->getCallingPid();
        if (!has_permission(callingUid, P_UNLOCK, spid)) {
            ALOGW("permission denied for %d: unlock", callingUid);
            return ::PERMISSION_DENIED;
        }

        State state = mKeyStore->getState(callingUid);
        if (state != ::STATE_LOCKED) {
            ALOGD("calling unlock when not locked");
            return state;
        }

        const String8 password8(pw);
        return password(pw);
    }

    int32_t zero() {
        uid_t callingUid = IPCThreadState::self()->getCallingUid();
        pid_t spid = IPCThreadState::self()->getCallingPid();
        if (!has_permission(callingUid, P_ZERO, spid)) {
            ALOGW("permission denied for %d: zero", callingUid);
            return -1;
        }

        return mKeyStore->isEmpty(callingUid) ? ::KEY_NOT_FOUND : ::NO_ERROR;
    }

    int32_t generate(const String16& name, int32_t targetUid, int32_t keyType, int32_t keySize,
            int32_t flags, Vector<sp<KeystoreArg> >* args) {
        uid_t callingUid = IPCThreadState::self()->getCallingUid();
        pid_t spid = IPCThreadState::self()->getCallingPid();
        if (!has_permission(callingUid, P_INSERT, spid)) {
            ALOGW("permission denied for %d: generate", callingUid);
            return ::PERMISSION_DENIED;
        }

        if (targetUid == -1) {
            targetUid = callingUid;
        } else if (!is_granted_to(callingUid, targetUid)) {
            return ::PERMISSION_DENIED;
        }

        State state = mKeyStore->getState(callingUid);
        if ((flags & KEYSTORE_FLAG_ENCRYPTED) && !isKeystoreUnlocked(state)) {
            ALOGW("calling generate in state: %d", state);
            return state;
        }

        uint8_t* data;
        size_t dataLength;
        int rc;
        bool isFallback = false;

        const keymaster_device_t* device = mKeyStore->getDevice();
        if (device == NULL) {
            return ::SYSTEM_ERROR;
        }

        if (device->generate_keypair == NULL) {
            return ::SYSTEM_ERROR;
        }

        if (keyType == EVP_PKEY_DSA) {
            keymaster_dsa_keygen_params_t dsa_params;
            memset(&dsa_params, '\0', sizeof(dsa_params));

            if (keySize == -1) {
                keySize = DSA_DEFAULT_KEY_SIZE;
            } else if ((keySize % 64) != 0 || keySize < DSA_MIN_KEY_SIZE
                    || keySize > DSA_MAX_KEY_SIZE) {
                ALOGI("invalid key size %d", keySize);
                return ::SYSTEM_ERROR;
            }
            dsa_params.key_size = keySize;

            if (args->size() == 3) {
                sp<KeystoreArg> gArg = args->itemAt(0);
                sp<KeystoreArg> pArg = args->itemAt(1);
                sp<KeystoreArg> qArg = args->itemAt(2);

                if (gArg != NULL && pArg != NULL && qArg != NULL) {
                    dsa_params.generator = reinterpret_cast<const uint8_t*>(gArg->data());
                    dsa_params.generator_len = gArg->size();

                    dsa_params.prime_p = reinterpret_cast<const uint8_t*>(pArg->data());
                    dsa_params.prime_p_len = pArg->size();

                    dsa_params.prime_q = reinterpret_cast<const uint8_t*>(qArg->data());
                    dsa_params.prime_q_len = qArg->size();
                } else {
                    ALOGI("not all DSA parameters were read");
                    return ::SYSTEM_ERROR;
                }
            } else if (args->size() != 0) {
                ALOGI("DSA args must be 3");
                return ::SYSTEM_ERROR;
            }

            if (isKeyTypeSupported(device, TYPE_DSA)) {
                rc = device->generate_keypair(device, TYPE_DSA, &dsa_params, &data, &dataLength);
            } else {
                isFallback = true;
                rc = openssl_generate_keypair(device, TYPE_DSA, &dsa_params, &data, &dataLength);
            }
        } else if (keyType == EVP_PKEY_EC) {
            keymaster_ec_keygen_params_t ec_params;
            memset(&ec_params, '\0', sizeof(ec_params));

            if (keySize == -1) {
                keySize = EC_DEFAULT_KEY_SIZE;
            } else if (keySize < EC_MIN_KEY_SIZE || keySize > EC_MAX_KEY_SIZE) {
                ALOGI("invalid key size %d", keySize);
                return ::SYSTEM_ERROR;
            }
            ec_params.field_size = keySize;

            if (isKeyTypeSupported(device, TYPE_EC)) {
                rc = device->generate_keypair(device, TYPE_EC, &ec_params, &data, &dataLength);
            } else {
                isFallback = true;
                rc = openssl_generate_keypair(device, TYPE_EC, &ec_params, &data, &dataLength);
            }
        } else if (keyType == EVP_PKEY_RSA) {
            keymaster_rsa_keygen_params_t rsa_params;
            memset(&rsa_params, '\0', sizeof(rsa_params));
            rsa_params.public_exponent = RSA_DEFAULT_EXPONENT;

            if (keySize == -1) {
                keySize = RSA_DEFAULT_KEY_SIZE;
            } else if (keySize < RSA_MIN_KEY_SIZE || keySize > RSA_MAX_KEY_SIZE) {
                ALOGI("invalid key size %d", keySize);
                return ::SYSTEM_ERROR;
            }
            rsa_params.modulus_size = keySize;

            if (args->size() > 1) {
                ALOGI("invalid number of arguments: %zu", args->size());
                return ::SYSTEM_ERROR;
            } else if (args->size() == 1) {
                sp<KeystoreArg> pubExpBlob = args->itemAt(0);
                if (pubExpBlob != NULL) {
                    Unique_BIGNUM pubExpBn(
                            BN_bin2bn(reinterpret_cast<const unsigned char*>(pubExpBlob->data()),
                                    pubExpBlob->size(), NULL));
                    if (pubExpBn.get() == NULL) {
                        ALOGI("Could not convert public exponent to BN");
                        return ::SYSTEM_ERROR;
                    }
                    unsigned long pubExp = BN_get_word(pubExpBn.get());
                    if (pubExp == 0xFFFFFFFFL) {
                        ALOGI("cannot represent public exponent as a long value");
                        return ::SYSTEM_ERROR;
                    }
                    rsa_params.public_exponent = pubExp;
                }
            }

            rc = device->generate_keypair(device, TYPE_RSA, &rsa_params, &data, &dataLength);
        } else {
            ALOGW("Unsupported key type %d", keyType);
            rc = -1;
        }

        if (rc) {
            return ::SYSTEM_ERROR;
        }

        String8 name8(name);
        String8 filename(mKeyStore->getKeyNameForUidWithDir(name8, callingUid));

        Blob keyBlob(data, dataLength, NULL, 0, TYPE_KEY_PAIR);
        free(data);

        keyBlob.setEncrypted(flags & KEYSTORE_FLAG_ENCRYPTED);
        keyBlob.setFallback(isFallback);

        return mKeyStore->put(filename.string(), &keyBlob, callingUid);
    }

    int32_t import(const String16& name, const uint8_t* data, size_t length, int targetUid,
            int32_t flags) {
        uid_t callingUid = IPCThreadState::self()->getCallingUid();
        pid_t spid = IPCThreadState::self()->getCallingPid();
        if (!has_permission(callingUid, P_INSERT, spid)) {
            ALOGW("permission denied for %d: import", callingUid);
            return ::PERMISSION_DENIED;
        }

        if (targetUid == -1) {
            targetUid = callingUid;
        } else if (!is_granted_to(callingUid, targetUid)) {
            return ::PERMISSION_DENIED;
        }

        State state = mKeyStore->getState(targetUid);
        if ((flags & KEYSTORE_FLAG_ENCRYPTED) && !isKeystoreUnlocked(state)) {
            ALOGD("calling import in state: %d", state);
            return state;
        }

        String8 name8(name);
        String8 filename(mKeyStore->getKeyNameForUidWithDir(name8, targetUid));

        return mKeyStore->importKey(data, length, filename.string(), targetUid, flags);
    }

    int32_t sign(const String16& name, const uint8_t* data, size_t length, uint8_t** out,
            size_t* outLength) {
        uid_t callingUid = IPCThreadState::self()->getCallingUid();
        pid_t spid = IPCThreadState::self()->getCallingPid();
        if (!has_permission(callingUid, P_SIGN, spid)) {
            ALOGW("permission denied for %d: saw", callingUid);
            return ::PERMISSION_DENIED;
        }

        Blob keyBlob;
        String8 name8(name);

        ALOGV("sign %s from uid %d", name8.string(), callingUid);
        int rc;

        ResponseCode responseCode = mKeyStore->getKeyForName(&keyBlob, name8, callingUid,
                ::TYPE_KEY_PAIR);
        if (responseCode != ::NO_ERROR) {
            return responseCode;
        }

        const keymaster_device_t* device = mKeyStore->getDevice();
        if (device == NULL) {
            ALOGE("no keymaster device; cannot sign");
            return ::SYSTEM_ERROR;
        }

        if (device->sign_data == NULL) {
            ALOGE("device doesn't implement signing");
            return ::SYSTEM_ERROR;
        }

        keymaster_rsa_sign_params_t params;
        params.digest_type = DIGEST_NONE;
        params.padding_type = PADDING_NONE;

        if (keyBlob.isFallback()) {
            rc = openssl_sign_data(device, &params, keyBlob.getValue(), keyBlob.getLength(), data,
                    length, out, outLength);
        } else {
            rc = device->sign_data(device, &params, keyBlob.getValue(), keyBlob.getLength(), data,
                    length, out, outLength);
        }
        if (rc) {
            ALOGW("device couldn't sign data");
            return ::SYSTEM_ERROR;
        }

        return ::NO_ERROR;
    }

    int32_t verify(const String16& name, const uint8_t* data, size_t dataLength,
            const uint8_t* signature, size_t signatureLength) {
        uid_t callingUid = IPCThreadState::self()->getCallingUid();
        pid_t spid = IPCThreadState::self()->getCallingPid();
        if (!has_permission(callingUid, P_VERIFY, spid)) {
            ALOGW("permission denied for %d: verify", callingUid);
            return ::PERMISSION_DENIED;
        }

        State state = mKeyStore->getState(callingUid);
        if (!isKeystoreUnlocked(state)) {
            ALOGD("calling verify in state: %d", state);
            return state;
        }

        Blob keyBlob;
        String8 name8(name);
        int rc;

        ResponseCode responseCode = mKeyStore->getKeyForName(&keyBlob, name8, callingUid,
                TYPE_KEY_PAIR);
        if (responseCode != ::NO_ERROR) {
            return responseCode;
        }

        const keymaster_device_t* device = mKeyStore->getDevice();
        if (device == NULL) {
            return ::SYSTEM_ERROR;
        }

        if (device->verify_data == NULL) {
            return ::SYSTEM_ERROR;
        }

        keymaster_rsa_sign_params_t params;
        params.digest_type = DIGEST_NONE;
        params.padding_type = PADDING_NONE;

        if (keyBlob.isFallback()) {
            rc = openssl_verify_data(device, &params, keyBlob.getValue(), keyBlob.getLength(), data,
                    dataLength, signature, signatureLength);
        } else {
            rc = device->verify_data(device, &params, keyBlob.getValue(), keyBlob.getLength(), data,
                    dataLength, signature, signatureLength);
        }
        if (rc) {
            return ::SYSTEM_ERROR;
        } else {
            return ::NO_ERROR;
        }
    }

    /*
     * TODO: The abstraction between things stored in hardware and regular blobs
     * of data stored on the filesystem should be moved down to keystore itself.
     * Unfortunately the Java code that calls this has naming conventions that it
     * knows about. Ideally keystore shouldn't be used to store random blobs of
     * data.
     *
     * Until that happens, it's necessary to have a separate "get_pubkey" and
     * "del_key" since the Java code doesn't really communicate what it's
     * intentions are.
     */
    int32_t get_pubkey(const String16& name, uint8_t** pubkey, size_t* pubkeyLength) {
        uid_t callingUid = IPCThreadState::self()->getCallingUid();
        pid_t spid = IPCThreadState::self()->getCallingPid();
        if (!has_permission(callingUid, P_GET, spid)) {
            ALOGW("permission denied for %d: get_pubkey", callingUid);
            return ::PERMISSION_DENIED;
        }

        Blob keyBlob;
        String8 name8(name);

        ALOGV("get_pubkey '%s' from uid %d", name8.string(), callingUid);

        ResponseCode responseCode = mKeyStore->getKeyForName(&keyBlob, name8, callingUid,
                TYPE_KEY_PAIR);
        if (responseCode != ::NO_ERROR) {
            return responseCode;
        }

        const keymaster_device_t* device = mKeyStore->getDevice();
        if (device == NULL) {
            return ::SYSTEM_ERROR;
        }

        if (device->get_keypair_public == NULL) {
            ALOGE("device has no get_keypair_public implementation!");
            return ::SYSTEM_ERROR;
        }

        int rc;
        if (keyBlob.isFallback()) {
            rc = openssl_get_keypair_public(device, keyBlob.getValue(), keyBlob.getLength(), pubkey,
                    pubkeyLength);
        } else {
            rc = device->get_keypair_public(device, keyBlob.getValue(), keyBlob.getLength(), pubkey,
                    pubkeyLength);
        }
        if (rc) {
            return ::SYSTEM_ERROR;
        }

        return ::NO_ERROR;
    }

    int32_t del_key(const String16& name, int targetUid) {
        uid_t callingUid = IPCThreadState::self()->getCallingUid();
        pid_t spid = IPCThreadState::self()->getCallingPid();
        if (!has_permission(callingUid, P_DELETE, spid)) {
            ALOGW("permission denied for %d: del_key", callingUid);
            return ::PERMISSION_DENIED;
        }

        if (targetUid == -1) {
            targetUid = callingUid;
        } else if (!is_granted_to(callingUid, targetUid)) {
            return ::PERMISSION_DENIED;
        }

        String8 name8(name);
        String8 filename(mKeyStore->getKeyNameForUidWithDir(name8, targetUid));
        return mKeyStore->del(filename.string(), ::TYPE_KEY_PAIR, targetUid);
    }

    int32_t grant(const String16& name, int32_t granteeUid) {
        uid_t callingUid = IPCThreadState::self()->getCallingUid();
        pid_t spid = IPCThreadState::self()->getCallingPid();
        if (!has_permission(callingUid, P_GRANT, spid)) {
            ALOGW("permission denied for %d: grant", callingUid);
            return ::PERMISSION_DENIED;
        }

        State state = mKeyStore->getState(callingUid);
        if (!isKeystoreUnlocked(state)) {
            ALOGD("calling grant in state: %d", state);
            return state;
        }

        String8 name8(name);
        String8 filename(mKeyStore->getKeyNameForUidWithDir(name8, callingUid));

        if (access(filename.string(), R_OK) == -1) {
            return (errno != ENOENT) ? ::SYSTEM_ERROR : ::KEY_NOT_FOUND;
        }

        mKeyStore->addGrant(filename.string(), granteeUid);
        return ::NO_ERROR;
    }

    int32_t ungrant(const String16& name, int32_t granteeUid) {
        uid_t callingUid = IPCThreadState::self()->getCallingUid();
        pid_t spid = IPCThreadState::self()->getCallingPid();
        if (!has_permission(callingUid, P_GRANT, spid)) {
            ALOGW("permission denied for %d: ungrant", callingUid);
            return ::PERMISSION_DENIED;
        }

        State state = mKeyStore->getState(callingUid);
        if (!isKeystoreUnlocked(state)) {
            ALOGD("calling ungrant in state: %d", state);
            return state;
        }

        String8 name8(name);
        String8 filename(mKeyStore->getKeyNameForUidWithDir(name8, callingUid));

        if (access(filename.string(), R_OK) == -1) {
            return (errno != ENOENT) ? ::SYSTEM_ERROR : ::KEY_NOT_FOUND;
        }

        return mKeyStore->removeGrant(filename.string(), granteeUid) ? ::NO_ERROR : ::KEY_NOT_FOUND;
    }

    int64_t getmtime(const String16& name) {
        uid_t callingUid = IPCThreadState::self()->getCallingUid();
        pid_t spid = IPCThreadState::self()->getCallingPid();
        if (!has_permission(callingUid, P_GET, spid)) {
            ALOGW("permission denied for %d: getmtime", callingUid);
            return -1L;
        }

        String8 name8(name);
        String8 filename(mKeyStore->getKeyNameForUidWithDir(name8, callingUid));

        if (access(filename.string(), R_OK) == -1) {
            ALOGW("could not access %s for getmtime", filename.string());
            return -1L;
        }

        int fd = TEMP_FAILURE_RETRY(open(filename.string(), O_NOFOLLOW, O_RDONLY));
        if (fd < 0) {
            ALOGW("could not open %s for getmtime", filename.string());
            return -1L;
        }

        struct stat s;
        int ret = fstat(fd, &s);
        close(fd);
        if (ret == -1) {
            ALOGW("could not stat %s for getmtime", filename.string());
            return -1L;
        }

        return static_cast<int64_t>(s.st_mtime);
    }

    int32_t duplicate(const String16& srcKey, int32_t srcUid, const String16& destKey,
            int32_t destUid) {
        uid_t callingUid = IPCThreadState::self()->getCallingUid();
        pid_t spid = IPCThreadState::self()->getCallingPid();
        if (!has_permission(callingUid, P_DUPLICATE, spid)) {
            ALOGW("permission denied for %d: duplicate", callingUid);
            return -1L;
        }

        State state = mKeyStore->getState(callingUid);
        if (!isKeystoreUnlocked(state)) {
            ALOGD("calling duplicate in state: %d", state);
            return state;
        }

        if (srcUid == -1 || static_cast<uid_t>(srcUid) == callingUid) {
            srcUid = callingUid;
        } else if (!is_granted_to(callingUid, srcUid)) {
            ALOGD("migrate not granted from source: %d -> %d", callingUid, srcUid);
            return ::PERMISSION_DENIED;
        }

        if (destUid == -1) {
            destUid = callingUid;
        }

        if (srcUid != destUid) {
            if (static_cast<uid_t>(srcUid) != callingUid) {
                ALOGD("can only duplicate from caller to other or to same uid: "
                      "calling=%d, srcUid=%d, destUid=%d", callingUid, srcUid, destUid);
                return ::PERMISSION_DENIED;
            }

            if (!is_granted_to(callingUid, destUid)) {
                ALOGD("duplicate not granted to dest: %d -> %d", callingUid, destUid);
                return ::PERMISSION_DENIED;
            }
        }

        String8 source8(srcKey);
        String8 sourceFile(mKeyStore->getKeyNameForUidWithDir(source8, srcUid));

        String8 target8(destKey);
        String8 targetFile(mKeyStore->getKeyNameForUidWithDir(target8, destUid));

        if (access(targetFile.string(), W_OK) != -1 || errno != ENOENT) {
            ALOGD("destination already exists: %s", targetFile.string());
            return ::SYSTEM_ERROR;
        }

        Blob keyBlob;
        ResponseCode responseCode = mKeyStore->get(sourceFile.string(), &keyBlob, TYPE_ANY,
                srcUid);
        if (responseCode != ::NO_ERROR) {
            return responseCode;
        }

        return mKeyStore->put(targetFile.string(), &keyBlob, destUid);
    }

    int32_t is_hardware_backed(const String16& keyType) {
        return mKeyStore->isHardwareBacked(keyType) ? 1 : 0;
    }

    int32_t clear_uid(int64_t targetUid64) {
        uid_t targetUid = static_cast<uid_t>(targetUid64);
        uid_t callingUid = IPCThreadState::self()->getCallingUid();
        pid_t spid = IPCThreadState::self()->getCallingPid();
        if (!has_permission(callingUid, P_CLEAR_UID, spid)) {
            ALOGW("permission denied for %d: clear_uid", callingUid);
            return ::PERMISSION_DENIED;
        }

        if (targetUid64 == -1) {
            targetUid = callingUid;
        } else if (!is_self_or_system(callingUid, targetUid)) {
            ALOGW("permission denied for %d: clear_uid %d", callingUid, targetUid);
            return ::PERMISSION_DENIED;
        }

        const keymaster_device_t* device = mKeyStore->getDevice();
        if (device == NULL) {
            ALOGW("can't get keymaster device");
            return ::SYSTEM_ERROR;
        }

        String8 prefix = String8::format("%u_", targetUid);
        Vector<String16> aliases;
        if (mKeyStore->saw(prefix, &aliases, targetUid) != ::NO_ERROR) {
            return ::SYSTEM_ERROR;
        }

        for (uint32_t i = 0; i < aliases.size(); i++) {
            String8 name8(aliases[i]);
            String8 filename(mKeyStore->getKeyNameForUidWithDir(name8, targetUid));
            mKeyStore->del(filename.string(), ::TYPE_ANY, targetUid);
        }
        return ::NO_ERROR;
    }

    int32_t reset_uid(int32_t targetUid) {
        uid_t callingUid = IPCThreadState::self()->getCallingUid();
        pid_t spid = IPCThreadState::self()->getCallingPid();

        if (!has_permission(callingUid, P_RESET_UID, spid)) {
            ALOGW("permission denied for %d: reset_uid %d", callingUid, targetUid);
            return ::PERMISSION_DENIED;
        }
        if (!is_self_or_system(callingUid, targetUid)) {
            ALOGW("permission denied for %d: reset_uid %d", callingUid, targetUid);
            return ::PERMISSION_DENIED;
        }

        return mKeyStore->reset(targetUid) ? ::NO_ERROR : ::SYSTEM_ERROR;
    }

    int32_t sync_uid(int32_t sourceUid, int32_t targetUid) {
        uid_t callingUid = IPCThreadState::self()->getCallingUid();
        pid_t spid = IPCThreadState::self()->getCallingPid();
        if (!has_permission(callingUid, P_SYNC_UID, spid)) {
            ALOGW("permission denied for %d: sync_uid %d -> %d", callingUid, sourceUid, targetUid);
            return ::PERMISSION_DENIED;
        }
        if (callingUid != AID_SYSTEM) {
            ALOGW("permission denied for %d: sync_uid %d -> %d", callingUid, sourceUid, targetUid);
            return ::PERMISSION_DENIED;
        }
        if (sourceUid == targetUid) {
            return ::SYSTEM_ERROR;
        }

        // Initialise user keystore with existing master key held in-memory
        return mKeyStore->copyMasterKey(sourceUid, targetUid);
    }

    int32_t password_uid(const String16& pw, int32_t targetUid) {
        uid_t callingUid = IPCThreadState::self()->getCallingUid();
        pid_t spid = IPCThreadState::self()->getCallingPid();
        if (!has_permission(callingUid, P_PASSWORD_UID, spid)) {
            ALOGW("permission denied for %d: password_uid %d", callingUid, targetUid);
            return ::PERMISSION_DENIED;
        }
        if (callingUid != AID_SYSTEM) {
            ALOGW("permission denied for %d: password_uid %d", callingUid, targetUid);
            return ::PERMISSION_DENIED;
        }

        const String8 password8(pw);

        switch (mKeyStore->getState(targetUid)) {
            case ::STATE_UNINITIALIZED: {
                // generate master key, encrypt with password, write to file, initialize mMasterKey*.
                return mKeyStore->initializeUser(password8, targetUid);
            }
            case ::STATE_NO_ERROR: {
                // rewrite master key with new password.
                return mKeyStore->writeMasterKey(password8, targetUid);
            }
            case ::STATE_LOCKED: {
                // read master key, decrypt with password, initialize mMasterKey*.
                return mKeyStore->readMasterKey(password8, targetUid);
            }
        }
        return ::SYSTEM_ERROR;
    }

private:
    inline bool isKeystoreUnlocked(State state) {
        switch (state) {
        case ::STATE_NO_ERROR:
            return true;
        case ::STATE_UNINITIALIZED:
        case ::STATE_LOCKED:
            return false;
        }
        return false;
    }

    bool isKeyTypeSupported(const keymaster_device_t* device, keymaster_keypair_t keyType) {
        const int32_t device_api = device->common.module->module_api_version;
        if (device_api == KEYMASTER_MODULE_API_VERSION_0_2) {
            switch (keyType) {
                case TYPE_RSA:
                case TYPE_DSA:
                case TYPE_EC:
                    return true;
                default:
                    return false;
            }
        } else if (device_api >= KEYMASTER_MODULE_API_VERSION_0_3) {
            switch (keyType) {
                case TYPE_RSA:
                    return true;
                case TYPE_DSA:
                    return device->flags & KEYMASTER_SUPPORTS_DSA;
                case TYPE_EC:
                    return device->flags & KEYMASTER_SUPPORTS_EC;
                default:
                    return false;
            }
        } else {
            return keyType == TYPE_RSA;
        }
    }

    ::KeyStore* mKeyStore;
};

}; // namespace android

int main(int argc, char* argv[]) {
    if (argc < 2) {
        ALOGE("A directory must be specified!");
        return 1;
    }
    if (chdir(argv[1]) == -1) {
        ALOGE("chdir: %s: %s", argv[1], strerror(errno));
        return 1;
    }

    Entropy entropy;
    if (!entropy.open()) {
        return 1;
    }

    keymaster_device_t* dev;
    if (keymaster_device_initialize(&dev)) {
        ALOGE("keystore keymaster could not be initialized; exiting");
        return 1;
    }

    ks_is_selinux_enabled = is_selinux_enabled();
    if (ks_is_selinux_enabled) {
        union selinux_callback cb;
        cb.func_log = selinux_log_callback;
        selinux_set_callback(SELINUX_CB_LOG, cb);
        if (getcon(&tctx) != 0) {
            ALOGE("SELinux: Could not acquire target context. Aborting keystore.\n");
            return -1;
        }
    } else {
        ALOGI("SELinux: Keystore SELinux is disabled.\n");
    }

    KeyStore keyStore(&entropy, dev);
    keyStore.initialize();
    android::sp<android::IServiceManager> sm = android::defaultServiceManager();
    android::sp<android::KeyStoreProxy> proxy = new android::KeyStoreProxy(&keyStore);
    android::status_t ret = sm->addService(android::String16("android.security.keystore"), proxy);
    if (ret != android::OK) {
        ALOGE("Couldn't register binder service!");
        return -1;
    }

    /*
     * We're the only thread in existence, so we're just going to process
     * Binder transaction as a single-threaded program.
     */
    android::IPCThreadState::self()->joinThreadPool();

    keymaster_device_release(dev);
    return 1;
}
