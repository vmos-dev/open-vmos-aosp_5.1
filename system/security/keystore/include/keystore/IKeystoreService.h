/*
 * Copyright (C) 2012 The Android Open Source Project
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

#ifndef KEYSTORE_IKEYSTORESERVICE_H
#define KEYSTORE_IKEYSTORESERVICE_H

#include <utils/RefBase.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>

namespace android {

class KeystoreArg : public RefBase {
public:
    KeystoreArg(const void *data, size_t len);
    ~KeystoreArg();

    const void* data() const;
    size_t size() const;

private:
    const void* mData;
    size_t mSize;
};

/*
 * This must be kept manually in sync with frameworks/base's IKeystoreService.java
 */
class IKeystoreService: public IInterface {
public:
    enum {
        TEST = IBinder::FIRST_CALL_TRANSACTION + 0,
        GET = IBinder::FIRST_CALL_TRANSACTION + 1,
        INSERT = IBinder::FIRST_CALL_TRANSACTION + 2,
        DEL = IBinder::FIRST_CALL_TRANSACTION + 3,
        EXIST = IBinder::FIRST_CALL_TRANSACTION + 4,
        SAW = IBinder::FIRST_CALL_TRANSACTION + 5,
        RESET = IBinder::FIRST_CALL_TRANSACTION + 6,
        PASSWORD = IBinder::FIRST_CALL_TRANSACTION + 7,
        LOCK = IBinder::FIRST_CALL_TRANSACTION + 8,
        UNLOCK = IBinder::FIRST_CALL_TRANSACTION + 9,
        ZERO = IBinder::FIRST_CALL_TRANSACTION + 10,
        GENERATE = IBinder::FIRST_CALL_TRANSACTION + 11,
        IMPORT = IBinder::FIRST_CALL_TRANSACTION + 12,
        SIGN = IBinder::FIRST_CALL_TRANSACTION + 13,
        VERIFY = IBinder::FIRST_CALL_TRANSACTION + 14,
        GET_PUBKEY = IBinder::FIRST_CALL_TRANSACTION + 15,
        DEL_KEY = IBinder::FIRST_CALL_TRANSACTION + 16,
        GRANT = IBinder::FIRST_CALL_TRANSACTION + 17,
        UNGRANT = IBinder::FIRST_CALL_TRANSACTION + 18,
        GETMTIME = IBinder::FIRST_CALL_TRANSACTION + 19,
        DUPLICATE = IBinder::FIRST_CALL_TRANSACTION + 20,
        IS_HARDWARE_BACKED = IBinder::FIRST_CALL_TRANSACTION + 21,
        CLEAR_UID = IBinder::FIRST_CALL_TRANSACTION + 22,
        RESET_UID = IBinder::FIRST_CALL_TRANSACTION + 23,
        SYNC_UID = IBinder::FIRST_CALL_TRANSACTION + 24,
        PASSWORD_UID = IBinder::FIRST_CALL_TRANSACTION + 25,
    };

    DECLARE_META_INTERFACE(KeystoreService);

    virtual int32_t test() = 0;

    virtual int32_t get(const String16& name, uint8_t** item, size_t* itemLength) = 0;

    virtual int32_t insert(const String16& name, const uint8_t* item, size_t itemLength, int uid,
            int32_t flags) = 0;

    virtual int32_t del(const String16& name, int uid) = 0;

    virtual int32_t exist(const String16& name, int uid) = 0;

    virtual int32_t saw(const String16& name, int uid, Vector<String16>* matches) = 0;

    virtual int32_t reset() = 0;

    virtual int32_t password(const String16& password) = 0;

    virtual int32_t lock() = 0;

    virtual int32_t unlock(const String16& password) = 0;

    virtual int32_t zero() = 0;

    virtual int32_t generate(const String16& name, int32_t uid, int32_t keyType, int32_t keySize,
            int32_t flags, Vector<sp<KeystoreArg> >* args) = 0;

    virtual int32_t import(const String16& name, const uint8_t* data, size_t length, int uid,
            int32_t flags) = 0;

    virtual int32_t sign(const String16& name, const uint8_t* data, size_t length, uint8_t** out,
            size_t* outLength) = 0;

    virtual int32_t verify(const String16& name, const uint8_t* data, size_t dataLength,
            const uint8_t* signature, size_t signatureLength) = 0;

    virtual int32_t get_pubkey(const String16& name, uint8_t** pubkey, size_t* pubkeyLength) = 0;

    virtual int32_t del_key(const String16& name, int uid) = 0;

    virtual int32_t grant(const String16& name, int32_t granteeUid) = 0;

    virtual int32_t ungrant(const String16& name, int32_t granteeUid) = 0;

    virtual int64_t getmtime(const String16& name) = 0;

    virtual int32_t duplicate(const String16& srcKey, int32_t srcUid, const String16& destKey,
            int32_t destUid) = 0;

    virtual int32_t is_hardware_backed(const String16& keyType) = 0;

    virtual int32_t clear_uid(int64_t uid) = 0;

    virtual int32_t reset_uid(int32_t uid) = 0;

    virtual int32_t sync_uid(int32_t sourceUid, int32_t targetUid) = 0;

    virtual int32_t password_uid(const String16& password, int32_t uid) = 0;
};

// ----------------------------------------------------------------------------

class BnKeystoreService: public BnInterface<IKeystoreService> {
public:
    virtual status_t onTransact(uint32_t code, const Parcel& data, Parcel* reply,
            uint32_t flags = 0);
};

} // namespace android

#endif
