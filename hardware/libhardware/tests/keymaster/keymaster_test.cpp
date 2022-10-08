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

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <fstream>
#include <iostream>

#include <gtest/gtest.h>

#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/x509.h>

#define LOG_TAG "keymaster_test"
#include <utils/Log.h>

#include <UniquePtr.h>

#include <hardware/keymaster.h>

namespace android {

class UniqueBlob : public UniquePtr<uint8_t[]> {
public:
    UniqueBlob(size_t length) :
            mLength(length) {
    }

    UniqueBlob(uint8_t* bytes, size_t length) :
            UniquePtr<uint8_t[]>(bytes), mLength(length) {
    }

    bool operator==(const UniqueBlob &other) const {
        if (other.length() != mLength) {
            return false;
        }

        const uint8_t* mine = get();
        const uint8_t* theirs = other.get();

        for (size_t i = 0; i < mLength; i++) {
            if (mine[i] != theirs[i]) {
                return false;
            }
        }

        return true;
    }

    size_t length() const {
        return mLength;
    }

    friend std::ostream &operator<<(std::ostream &stream, const UniqueBlob& blob);

private:
    size_t mLength;
};

std::ostream &operator<<(std::ostream &stream, const UniqueBlob& blob) {
    const size_t length = blob.mLength;
    stream << "Blob length=" << length << " < ";

    const uint8_t* data = blob.get();
    for (size_t i = 0; i < length; i++) {
        stream << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<unsigned int>(data[i]) << ' ';
    }
    stream << '>' << std::endl;

    return stream;
}

class UniqueKey : public UniqueBlob {
public:
    UniqueKey(keymaster_device_t** dev, uint8_t* bytes, size_t length) :
            UniqueBlob(bytes, length), mDevice(dev) {
    }

    ~UniqueKey() {
        if (mDevice != NULL && *mDevice != NULL) {
            keymaster_device_t* dev = *mDevice;
            if (dev->delete_keypair != NULL) {
                dev->delete_keypair(dev, get(), length());
            }
        }
    }

private:
    keymaster_device_t** mDevice;
};

class UniqueReadOnlyBlob {
public:
    UniqueReadOnlyBlob(uint8_t* data, size_t dataSize) :
            mDataSize(dataSize) {
        int pageSize = sysconf(_SC_PAGE_SIZE);
        if (pageSize == -1) {
            return;
        }

        int fd = open("/dev/zero", O_RDONLY);
        if (fd == -1) {
            return;
        }

        mBufferSize = (dataSize + pageSize - 1) & ~(pageSize - 1);
        uint8_t* buffer = (uint8_t*) mmap(NULL, mBufferSize, PROT_READ | PROT_WRITE,
                                          MAP_PRIVATE, fd, 0);
        close(fd);

        if (buffer == NULL) {
            return;
        }

        memcpy(buffer, data, dataSize);
        if (mprotect(buffer, mBufferSize, PROT_READ) == -1) {
            munmap(buffer, mBufferSize);
            return;
        }

        mBuffer = buffer;
    }

    ~UniqueReadOnlyBlob() {
        munmap(mBuffer, mBufferSize);
    }

    uint8_t* get() const {
        return mBuffer;
    }

    size_t length() const {
        return mDataSize;
    }

private:
    uint8_t* mBuffer;
    size_t mBufferSize;
    size_t mDataSize;
};

struct BIGNUM_Delete {
    void operator()(BIGNUM* p) const {
        BN_free(p);
    }
};
typedef UniquePtr<BIGNUM, BIGNUM_Delete> Unique_BIGNUM;

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

struct RSA_Delete {
    void operator()(RSA* p) const {
        RSA_free(p);
    }
};
typedef UniquePtr<RSA, RSA_Delete> Unique_RSA;

struct EC_KEY_Delete {
    void operator()(EC_KEY* p) const {
        EC_KEY_free(p);
    }
};
typedef UniquePtr<EC_KEY, EC_KEY_Delete> Unique_EC_KEY;


/*
 * DER-encoded PKCS#8 format RSA key. Generated using:
 *
 * openssl genrsa 2048 | openssl pkcs8 -topk8 -nocrypt -outform der | recode ../x1
 */
static uint8_t TEST_RSA_KEY_1[] = {
        0x30, 0x82, 0x04, 0xBE, 0x02, 0x01, 0x00, 0x30, 0x0D, 0x06, 0x09, 0x2A,
        0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x01, 0x05, 0x00, 0x04, 0x82,
        0x04, 0xA8, 0x30, 0x82, 0x04, 0xA4, 0x02, 0x01, 0x00, 0x02, 0x82, 0x01,
        0x01, 0x00, 0xD8, 0x58, 0xD4, 0x9F, 0xC0, 0xE8, 0xF0, 0xFF, 0x87, 0x27,
        0x43, 0xE6, 0x2E, 0xE6, 0x9A, 0x42, 0x3B, 0x39, 0x94, 0x84, 0x43, 0x55,
        0x8D, 0x20, 0x5B, 0x71, 0x88, 0xE6, 0xD1, 0x62, 0xC8, 0xF2, 0x20, 0xD0,
        0x75, 0x13, 0x83, 0xA3, 0x5D, 0x19, 0xA8, 0x62, 0xD0, 0x5F, 0x3E, 0x8A,
        0x7C, 0x0E, 0x26, 0xA9, 0xFF, 0xB2, 0x5E, 0x63, 0xAA, 0x3C, 0x8D, 0x13,
        0x41, 0xAA, 0xD5, 0x03, 0x01, 0x01, 0x53, 0xC9, 0x02, 0x1C, 0xEC, 0xE8,
        0xC4, 0x70, 0x3F, 0x43, 0xE5, 0x51, 0xD0, 0x6E, 0x52, 0x0B, 0xC4, 0x0A,
        0xA3, 0x61, 0xDE, 0xE3, 0x72, 0x0C, 0x94, 0xF1, 0x1C, 0x2D, 0x36, 0x77,
        0xBB, 0x16, 0xA8, 0x63, 0x4B, 0xD1, 0x07, 0x00, 0x42, 0x2D, 0x2B, 0x10,
        0x80, 0x45, 0xF3, 0x0C, 0xF9, 0xC5, 0xAC, 0xCC, 0x64, 0x87, 0xFD, 0x5D,
        0xC8, 0x51, 0xD4, 0x1C, 0x9E, 0x6E, 0x9B, 0xC4, 0x27, 0x5E, 0x73, 0xA7,
        0x2A, 0xF6, 0x90, 0x42, 0x0C, 0x34, 0x93, 0xB7, 0x02, 0x19, 0xA9, 0x64,
        0x6C, 0x46, 0x3B, 0x40, 0x02, 0x2F, 0x54, 0x69, 0x79, 0x26, 0x7D, 0xF6,
        0x85, 0x90, 0x01, 0xD0, 0x21, 0x07, 0xD0, 0x14, 0x00, 0x65, 0x9C, 0xAC,
        0x24, 0xE8, 0x78, 0x42, 0x3B, 0x90, 0x75, 0x19, 0x55, 0x11, 0x4E, 0xD9,
        0xE6, 0x97, 0x87, 0xBC, 0x8D, 0x2C, 0x9B, 0xF0, 0x1F, 0x14, 0xEB, 0x6A,
        0x57, 0xCE, 0x78, 0xAD, 0xCE, 0xD9, 0xFB, 0xB9, 0xA1, 0xEF, 0x0C, 0x1F,
        0xDD, 0xE3, 0x5B, 0x73, 0xA0, 0xEC, 0x37, 0x9C, 0xE1, 0xFD, 0x86, 0x28,
        0xC3, 0x4A, 0x42, 0xD0, 0xA3, 0xFE, 0x57, 0x09, 0x29, 0xD8, 0xF6, 0xEC,
        0xE3, 0xC0, 0x71, 0x7C, 0x29, 0x27, 0xC2, 0xD1, 0x3E, 0x22, 0xBC, 0xBD,
        0x5A, 0x85, 0x41, 0xF6, 0x15, 0xDA, 0x0C, 0x58, 0x5A, 0x61, 0x5B, 0x78,
        0xB8, 0xAA, 0xEC, 0x5C, 0x1C, 0x79, 0x02, 0x03, 0x01, 0x00, 0x01, 0x02,
        0x82, 0x01, 0x00, 0x1D, 0x10, 0x31, 0xE0, 0x14, 0x26, 0x36, 0xD9, 0xDC,
        0xEA, 0x25, 0x70, 0xF2, 0xB3, 0xFF, 0xDD, 0x0D, 0xDF, 0xBA, 0x57, 0xDA,
        0x43, 0xCF, 0xE5, 0x9C, 0xE3, 0x2F, 0xA4, 0xF2, 0x53, 0xF6, 0xF2, 0xAF,
        0xFD, 0xD0, 0xFC, 0x82, 0x1E, 0x9C, 0x0F, 0x2A, 0x53, 0xBB, 0xF2, 0x4F,
        0x90, 0x83, 0x01, 0xD3, 0xA7, 0xDA, 0xB5, 0xB7, 0x80, 0x64, 0x0A, 0x26,
        0x59, 0x83, 0xE4, 0xD3, 0x20, 0xC8, 0x2D, 0xC9, 0x77, 0xA3, 0x55, 0x07,
        0x6E, 0x6D, 0x95, 0x36, 0xAA, 0x84, 0x4F, 0xED, 0x54, 0x24, 0xA9, 0x77,
        0xF8, 0x85, 0xE2, 0x4B, 0xF2, 0xFA, 0x0B, 0x3E, 0xA6, 0xF5, 0x46, 0x0D,
        0x9F, 0x1F, 0xFE, 0xF7, 0x37, 0xFF, 0xA3, 0x60, 0xF1, 0x63, 0xF2, 0x75,
        0x6A, 0x8E, 0x10, 0xD7, 0x89, 0xD2, 0xB3, 0xFF, 0x76, 0xA5, 0xBA, 0xAF,
        0x0A, 0xBE, 0x32, 0x5F, 0xF0, 0x48, 0x48, 0x4B, 0x9C, 0x9A, 0x3D, 0x12,
        0xA7, 0xD2, 0x07, 0xC7, 0x59, 0x32, 0x94, 0x95, 0x65, 0x2F, 0x87, 0x34,
        0x76, 0xBA, 0x7C, 0x08, 0x4B, 0xAB, 0xA6, 0x24, 0xDF, 0x64, 0xDB, 0x48,
        0x63, 0x42, 0x06, 0xE2, 0x2C, 0x3D, 0xFB, 0xE5, 0x47, 0x81, 0x94, 0x98,
        0xF7, 0x32, 0x4B, 0x28, 0xEB, 0x42, 0xB8, 0xE9, 0x8E, 0xFC, 0xC9, 0x43,
        0xC9, 0x47, 0xE6, 0xE7, 0x1C, 0xDC, 0x71, 0xEF, 0x4D, 0x8A, 0xB1, 0xFC,
        0x45, 0x37, 0xEC, 0xB3, 0x16, 0x88, 0x5B, 0xE2, 0xEC, 0x8B, 0x6B, 0x75,
        0x16, 0xBE, 0x6B, 0xF8, 0x2C, 0xF8, 0xC9, 0xD1, 0xF7, 0x55, 0x87, 0x57,
        0x5F, 0xDE, 0xF4, 0x7E, 0x72, 0x13, 0x06, 0x2A, 0x21, 0xB7, 0x78, 0x21,
        0x05, 0xFD, 0xE2, 0x5F, 0x7B, 0x7C, 0xF0, 0x26, 0x2B, 0x75, 0x7F, 0x68,
        0xF9, 0xA6, 0x98, 0xFD, 0x54, 0x0E, 0xCC, 0x22, 0x41, 0x7F, 0x29, 0x81,
        0x2F, 0xA3, 0x3C, 0x3D, 0x64, 0xC8, 0x41, 0x02, 0x81, 0x81, 0x00, 0xFA,
        0xFA, 0xE4, 0x2E, 0x30, 0xF0, 0x7A, 0x8D, 0x95, 0xB8, 0x39, 0x58, 0x27,
        0x0F, 0x89, 0x0C, 0xDF, 0xFE, 0x2F, 0x55, 0x3B, 0x6F, 0xDD, 0x5F, 0x12,
        0xB3, 0xD1, 0xCF, 0x5B, 0x8D, 0xB6, 0x10, 0x1C, 0x87, 0x0C, 0x30, 0x89,
        0x2D, 0xBB, 0xB8, 0xA1, 0x78, 0x0F, 0x54, 0xA6, 0x36, 0x46, 0x05, 0x8B,
        0x5A, 0xFF, 0x48, 0x03, 0x13, 0xAE, 0x95, 0x96, 0x5D, 0x6C, 0xDA, 0x5D,
        0xF7, 0xAD, 0x1D, 0x33, 0xED, 0x23, 0xF5, 0x4B, 0x03, 0x78, 0xE7, 0x50,
        0xD1, 0x2D, 0x95, 0x22, 0x35, 0x02, 0x5B, 0x4A, 0x4E, 0x73, 0xC9, 0xB7,
        0x05, 0xC4, 0x21, 0x86, 0x1F, 0x1E, 0x40, 0x83, 0xBC, 0x8A, 0x3A, 0x95,
        0x24, 0x62, 0xF4, 0x58, 0x38, 0x64, 0x4A, 0x89, 0x8A, 0x27, 0x59, 0x12,
        0x9D, 0x21, 0xC3, 0xA6, 0x42, 0x1E, 0x2A, 0x3F, 0xD8, 0x65, 0x1F, 0x6E,
        0x3E, 0x4D, 0x5C, 0xCC, 0xEA, 0x8E, 0x15, 0x02, 0x81, 0x81, 0x00, 0xDC,
        0xAC, 0x9B, 0x00, 0xDB, 0xF9, 0xB2, 0xBF, 0xC4, 0x5E, 0xB6, 0xB7, 0x63,
        0xEB, 0x13, 0x4B, 0xE2, 0xA6, 0xC8, 0x72, 0x90, 0xD8, 0xC2, 0x33, 0x33,
        0xF0, 0x66, 0x75, 0xBD, 0x50, 0x7C, 0xA4, 0x8F, 0x82, 0xFB, 0xFF, 0x44,
        0x3B, 0xE7, 0x15, 0x3A, 0x0C, 0x7A, 0xF8, 0x92, 0x86, 0x4A, 0x79, 0x32,
        0x08, 0x82, 0x1D, 0x6A, 0xBA, 0xAD, 0x8A, 0xB3, 0x3D, 0x7F, 0xA5, 0xB4,
        0x6F, 0x67, 0x86, 0x7E, 0xB2, 0x9C, 0x2A, 0xF6, 0x7C, 0x49, 0x21, 0xC5,
        0x3F, 0x00, 0x3F, 0x9B, 0xF7, 0x0F, 0x6C, 0x35, 0x80, 0x75, 0x73, 0xC0,
        0xF8, 0x3E, 0x30, 0x5F, 0x74, 0x2F, 0x15, 0x41, 0xEA, 0x0F, 0xCE, 0x0E,
        0x18, 0x17, 0x68, 0xBA, 0xC4, 0x29, 0xF2, 0xE2, 0x2C, 0x1D, 0x55, 0x83,
        0xB6, 0x64, 0x2E, 0x03, 0x12, 0xA4, 0x0D, 0xBF, 0x4F, 0x2E, 0xBE, 0x7C,
        0x41, 0xD9, 0xCD, 0xD0, 0x52, 0x91, 0xD5, 0x02, 0x81, 0x81, 0x00, 0xD4,
        0x55, 0xEB, 0x32, 0xC1, 0x28, 0xD3, 0x26, 0x72, 0x22, 0xB8, 0x31, 0x42,
        0x6A, 0xBC, 0x52, 0x6E, 0x37, 0x48, 0xA8, 0x5D, 0x6E, 0xD8, 0xE5, 0x14,
        0x97, 0x99, 0xCC, 0x4A, 0xF2, 0xEB, 0xB3, 0x59, 0xCF, 0x4F, 0x9A, 0xC8,
        0x94, 0x2E, 0x9B, 0x97, 0xD0, 0x51, 0x78, 0x16, 0x5F, 0x18, 0x82, 0x9C,
        0x51, 0xD2, 0x64, 0x84, 0x65, 0xE4, 0x70, 0x9E, 0x14, 0x50, 0x81, 0xB6,
        0xBA, 0x52, 0x75, 0xC0, 0x76, 0xC2, 0xD3, 0x46, 0x31, 0x9B, 0xDA, 0x67,
        0xDF, 0x71, 0x27, 0x19, 0x17, 0xAB, 0xF4, 0xBC, 0x3A, 0xFF, 0x6F, 0x0B,
        0x2F, 0x0F, 0xAE, 0x25, 0x20, 0xB2, 0xA1, 0x76, 0x52, 0xCE, 0xC7, 0x9D,
        0x62, 0x79, 0x6D, 0xAC, 0x2D, 0x99, 0x7C, 0x0E, 0x3D, 0x19, 0xE9, 0x1B,
        0xFC, 0x60, 0x92, 0x7C, 0x58, 0xB7, 0xD8, 0x9A, 0xC7, 0x63, 0x56, 0x62,
        0x18, 0xC7, 0xAE, 0xD9, 0x97, 0x1F, 0xB9, 0x02, 0x81, 0x81, 0x00, 0x91,
        0x40, 0xC4, 0x1E, 0x82, 0xAD, 0x0F, 0x6D, 0x8E, 0xD2, 0x51, 0x2E, 0xD1,
        0x84, 0x30, 0x85, 0x68, 0xC1, 0x23, 0x7B, 0xD5, 0xBF, 0xF7, 0xC4, 0x40,
        0x51, 0xE2, 0xFF, 0x69, 0x07, 0x8B, 0xA3, 0xBE, 0x1B, 0x17, 0xC8, 0x64,
        0x9F, 0x91, 0x71, 0xB5, 0x6D, 0xF5, 0x9B, 0x9C, 0xC6, 0xEC, 0x4A, 0x6E,
        0x16, 0x8F, 0x9E, 0xD1, 0x5B, 0xE3, 0x53, 0x42, 0xBC, 0x1E, 0x43, 0x72,
        0x4B, 0x4A, 0x37, 0x8B, 0x3A, 0x01, 0xF5, 0x7D, 0x9D, 0x3D, 0x7E, 0x0F,
        0x19, 0x73, 0x0E, 0x6B, 0x98, 0xE9, 0xFB, 0xEE, 0x13, 0x8A, 0x3C, 0x11,
        0x2E, 0xD5, 0xB0, 0x7D, 0x84, 0x3A, 0x61, 0xA1, 0xAB, 0x71, 0x8F, 0xCE,
        0x53, 0x29, 0x45, 0x74, 0x7A, 0x1E, 0xAA, 0x93, 0x19, 0x3A, 0x8D, 0xC9,
        0x4E, 0xCB, 0x0E, 0x46, 0x53, 0x84, 0xCC, 0xCF, 0xBA, 0x4D, 0x28, 0x71,
        0x1D, 0xDF, 0x41, 0xCB, 0xF8, 0x2D, 0xA9, 0x02, 0x81, 0x80, 0x04, 0x8B,
        0x4A, 0xEA, 0xBD, 0x39, 0x0B, 0x96, 0xC5, 0x1D, 0xA4, 0x47, 0xFD, 0x46,
        0xD2, 0x8A, 0xEA, 0x2A, 0xF3, 0x9D, 0x3A, 0x7E, 0x16, 0x74, 0xFC, 0x13,
        0xDE, 0x4D, 0xA9, 0x85, 0x42, 0x33, 0x02, 0x92, 0x0B, 0xB6, 0xDB, 0x7E,
        0xEA, 0x85, 0xC2, 0x94, 0x43, 0x52, 0x37, 0x5A, 0x77, 0xAB, 0xCB, 0x61,
        0x88, 0xDE, 0xF8, 0xFA, 0xDB, 0xE8, 0x0B, 0x95, 0x7D, 0x39, 0x19, 0xA2,
        0x89, 0xB9, 0x32, 0xB2, 0x50, 0x38, 0xF7, 0x88, 0x69, 0xFD, 0xA4, 0x63,
        0x1F, 0x9B, 0x03, 0xD8, 0xA6, 0x7A, 0x05, 0x76, 0x02, 0x28, 0x93, 0x82,
        0x73, 0x7F, 0x14, 0xCC, 0xBE, 0x29, 0x10, 0xAD, 0x8A, 0x2E, 0xAC, 0xED,
        0x11, 0xA7, 0x72, 0x7C, 0x60, 0x78, 0x72, 0xFB, 0x78, 0x20, 0x18, 0xC9,
        0x7E, 0x63, 0xAD, 0x55, 0x54, 0x51, 0xDB, 0x9F, 0x7B, 0xD4, 0x8F, 0xB2,
        0xDE, 0x3B, 0xF1, 0x70, 0x23, 0xE5,
};

/*
 * DER-encoded PKCS#8 format EC key. Generated using:
 *
 * openssl ecparam -name prime256v1 -genkey -noout | openssl pkcs8 -topk8 -nocrypt -outform der | recode ../x1
 */
static uint8_t TEST_EC_KEY_1[] = {
        0x30, 0x81, 0x87, 0x02, 0x01, 0x00, 0x30, 0x13, 0x06, 0x07, 0x2A, 0x86,
        0x48, 0xCE, 0x3D, 0x02, 0x01, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D,
        0x03, 0x01, 0x07, 0x04, 0x6D, 0x30, 0x6B, 0x02, 0x01, 0x01, 0x04, 0x20,
        0x25, 0xAC, 0x77, 0x2B, 0x04, 0x33, 0xC8, 0x16, 0x59, 0xA3, 0xC7, 0xE7,
        0x11, 0x42, 0xD0, 0x11, 0x71, 0x30, 0x7B, 0xB8, 0xD2, 0x67, 0xFF, 0x9C,
        0x5F, 0x50, 0x2E, 0xAB, 0x67, 0xD4, 0x17, 0x51, 0xA1, 0x44, 0x03, 0x42,
        0x00, 0x04, 0xCF, 0xCE, 0xB8, 0x7F, 0x88, 0x36, 0xC4, 0xF8, 0x51, 0x29,
        0xE2, 0xA7, 0x21, 0xC3, 0x3B, 0xFF, 0x88, 0xE3, 0x87, 0x98, 0xD1, 0xA6,
        0x4B, 0xB3, 0x4B, 0xD5, 0x44, 0xF8, 0xE0, 0x43, 0x6B, 0x50, 0x74, 0xFB,
        0xB0, 0xAD, 0x41, 0x1C, 0x11, 0x9D, 0xC6, 0x1E, 0x83, 0x8C, 0x49, 0xCA,
        0xBE, 0xC6, 0xCE, 0xB6, 0xC9, 0xA1, 0xBF, 0x69, 0xA9, 0xA0, 0xA3, 0x80,
        0x14, 0x39, 0x57, 0x94, 0xDA, 0x5D
};


/*
 * Generated using keys on the keyboard and lack of imagination.
 */
static unsigned char BOGUS_KEY_1[] = { 0xFF, 0xFF, 0xFF, 0xFF };


class KeymasterBaseTest : public ::testing::Test {
public:
    static void SetUpTestCase() {
        const hw_module_t* mod;
        ASSERT_EQ(0, hw_get_module_by_class(KEYSTORE_HARDWARE_MODULE_ID, NULL, &mod))
                << "Should be able to find a keymaster hardware module";

        std::cout << "Using keymaster module: " << mod->name << std::endl;

        ASSERT_EQ(0, keymaster_open(mod, &sDevice))
                << "Should be able to open the keymaster device";

        ASSERT_EQ(KEYMASTER_MODULE_API_VERSION_0_2, mod->module_api_version)
                << "Keymaster should implement API version 2";

        ASSERT_TRUE(sDevice->generate_keypair != NULL)
                << "Should implement generate_keypair";

        ASSERT_TRUE(sDevice->import_keypair != NULL)
                << "Should implement import_keypair";

        ASSERT_TRUE(sDevice->get_keypair_public != NULL)
                << "Should implement get_keypair_public";

        ASSERT_TRUE(sDevice->sign_data != NULL)
                << "Should implement sign_data";

        ASSERT_TRUE(sDevice->verify_data != NULL)
                << "Should implement verify_data";
    }

    static void TearDownTestCase() {
        ASSERT_EQ(0, keymaster_close(sDevice));
    }

protected:
    static keymaster_device_t* sDevice;
};

keymaster_device_t* KeymasterBaseTest::sDevice = NULL;

class KeymasterTest : public KeymasterBaseTest {
};

class KeymasterAllTypesTest : public KeymasterBaseTest,
                              public ::testing::WithParamInterface<keymaster_keypair_t> {
};

class KeymasterGenerateRSATest : public KeymasterBaseTest,
                              public ::testing::WithParamInterface<uint32_t> {
};

class KeymasterGenerateDSATest : public KeymasterBaseTest,
                              public ::testing::WithParamInterface<uint32_t> {
};

class KeymasterGenerateECTest : public KeymasterBaseTest,
                              public ::testing::WithParamInterface<uint32_t> {
};

TEST_P(KeymasterGenerateRSATest, GenerateKeyPair_RSA_Success) {
    keymaster_keypair_t key_type = TYPE_RSA;
    keymaster_rsa_keygen_params_t params = {
            modulus_size: GetParam(),
            public_exponent: RSA_F4,
    };

    uint8_t* key_blob;
    size_t key_blob_length;

    ASSERT_EQ(0,
            sDevice->generate_keypair(sDevice, key_type, &params, &key_blob, &key_blob_length))
            << "Should generate an RSA key with " << GetParam() << " bit modulus size";
    UniqueKey key(&sDevice, key_blob, key_blob_length);

    uint8_t* x509_data = NULL;
    size_t x509_data_length;
    ASSERT_EQ(0,
            sDevice->get_keypair_public(sDevice, key_blob, key_blob_length,
                    &x509_data, &x509_data_length))
            << "Should be able to retrieve RSA public key successfully";
    UniqueBlob x509_blob(x509_data, x509_data_length);
    ASSERT_FALSE(x509_blob.get() == NULL)
            << "X509 data should be allocated";

    const unsigned char *tmp = static_cast<const unsigned char*>(x509_blob.get());
    Unique_EVP_PKEY actual(d2i_PUBKEY((EVP_PKEY**) NULL, &tmp,
            static_cast<long>(x509_blob.length())));

    ASSERT_EQ(EVP_PKEY_RSA, EVP_PKEY_type(actual.get()->type))
            << "Generated key type should be of type RSA";

    Unique_RSA rsa(EVP_PKEY_get1_RSA(actual.get()));
    ASSERT_FALSE(rsa.get() == NULL)
            << "Should be able to extract RSA key from EVP_PKEY";

    ASSERT_EQ(static_cast<unsigned long>(RSA_F4), BN_get_word(rsa.get()->e))
            << "Exponent should be RSA_F4";

    ASSERT_EQ((GetParam() + 7) / 8, static_cast<uint32_t>(RSA_size(rsa.get())))
            << "Modulus size should be the specified parameter";
}

INSTANTIATE_TEST_CASE_P(RSA,
                        KeymasterGenerateRSATest,
                        ::testing::Values(512U, 1024U, 2048U, 3072U, 4096U));


TEST_P(KeymasterGenerateECTest, GenerateKeyPair_EC_Success) {
    keymaster_keypair_t key_type = TYPE_EC;
    keymaster_ec_keygen_params_t params = {
            field_size: GetParam(),
    };

    uint8_t* key_blob;
    size_t key_blob_length;

    ASSERT_EQ(0,
            sDevice->generate_keypair(sDevice, key_type, &params, &key_blob, &key_blob_length))
            << "Should generate an EC key with " << GetParam() << " field size";
    UniqueKey key(&sDevice, key_blob, key_blob_length);

    uint8_t* x509_data = NULL;
    size_t x509_data_length;
    ASSERT_EQ(0,
            sDevice->get_keypair_public(sDevice, key_blob, key_blob_length,
                    &x509_data, &x509_data_length))
            << "Should be able to retrieve EC public key successfully";
    UniqueBlob x509_blob(x509_data, x509_data_length);
    ASSERT_FALSE(x509_blob.get() == NULL)
            << "X509 data should be allocated";

    const unsigned char *tmp = static_cast<const unsigned char*>(x509_blob.get());
    Unique_EVP_PKEY actual(d2i_PUBKEY((EVP_PKEY**) NULL, &tmp,
            static_cast<long>(x509_blob.length())));

    ASSERT_EQ(EVP_PKEY_EC, EVP_PKEY_type(actual.get()->type))
            << "Generated key type should be of type EC";

    Unique_EC_KEY ecKey(EVP_PKEY_get1_EC_KEY(actual.get()));
    ASSERT_FALSE(ecKey.get() == NULL)
            << "Should be able to extract EC key from EVP_PKEY";

    ASSERT_FALSE(EC_KEY_get0_group(ecKey.get()) == NULL)
            << "EC key should have a EC_GROUP";

    ASSERT_TRUE(EC_KEY_check_key(ecKey.get()))
            << "EC key should check correctly";
}

INSTANTIATE_TEST_CASE_P(EC,
                        KeymasterGenerateECTest,
                        ::testing::Values(192U, 224U, 256U, 384U, 521U));


TEST_P(KeymasterAllTypesTest, GenerateKeyPair_NullParams_Failure) {
    keymaster_keypair_t key_type = GetParam();

    uint8_t* key_blob;
    size_t key_blob_length;

    ASSERT_EQ(-1,
            sDevice->generate_keypair(sDevice, key_type, NULL, &key_blob, &key_blob_length))
            << "Should not be able to generate a key with null params";
}

INSTANTIATE_TEST_CASE_P(Types,
                        KeymasterAllTypesTest,
                        ::testing::Values(TYPE_RSA, TYPE_DSA, TYPE_EC));

TEST_F(KeymasterTest, GenerateKeyPair_UnknownType_Failure) {
    keymaster_keypair_t key_type = static_cast<keymaster_keypair_t>(0xFFFF);

    uint8_t* key_blob;
    size_t key_blob_length;

    ASSERT_EQ(-1,
            sDevice->generate_keypair(sDevice, key_type, NULL, &key_blob, &key_blob_length))
            << "Should not generate an unknown key type";
}

TEST_F(KeymasterTest, ImportKeyPair_RSA_Success) {
    uint8_t* key_blob;
    size_t key_blob_length;

    ASSERT_EQ(0,
            sDevice->import_keypair(sDevice, TEST_RSA_KEY_1, sizeof(TEST_RSA_KEY_1),
                    &key_blob, &key_blob_length))
            << "Should successfully import an RSA key";
    UniqueKey key(&sDevice, key_blob, key_blob_length);

    uint8_t* x509_data;
    size_t x509_data_length;
    ASSERT_EQ(0,
            sDevice->get_keypair_public(sDevice, key_blob, key_blob_length,
                    &x509_data, &x509_data_length))
            << "Should be able to retrieve RSA public key successfully";
    UniqueBlob x509_blob(x509_data, x509_data_length);

    const unsigned char *tmp = static_cast<const unsigned char*>(x509_blob.get());
    Unique_EVP_PKEY actual(d2i_PUBKEY((EVP_PKEY**) NULL, &tmp,
            static_cast<long>(x509_blob.length())));

    ASSERT_EQ(EVP_PKEY_type(actual.get()->type), EVP_PKEY_RSA)
            << "Generated key type should be of type RSA";

    const unsigned char *expectedTmp = static_cast<const unsigned char*>(TEST_RSA_KEY_1);
    Unique_PKCS8_PRIV_KEY_INFO expectedPkcs8(
            d2i_PKCS8_PRIV_KEY_INFO((PKCS8_PRIV_KEY_INFO**) NULL, &expectedTmp,
                    sizeof(TEST_RSA_KEY_1)));

    Unique_EVP_PKEY expected(EVP_PKCS82PKEY(expectedPkcs8.get()));

    ASSERT_EQ(1, EVP_PKEY_cmp(expected.get(), actual.get()))
            << "Expected and actual keys should match";
}

TEST_F(KeymasterTest, ImportKeyPair_EC_Success) {
    uint8_t* key_blob;
    size_t key_blob_length;

    ASSERT_EQ(0,
            sDevice->import_keypair(sDevice, TEST_EC_KEY_1, sizeof(TEST_EC_KEY_1),
                    &key_blob, &key_blob_length))
            << "Should successfully import an EC key";
    UniqueKey key(&sDevice, key_blob, key_blob_length);

    uint8_t* x509_data;
    size_t x509_data_length;
    ASSERT_EQ(0,
            sDevice->get_keypair_public(sDevice, key_blob, key_blob_length,
                    &x509_data, &x509_data_length))
            << "Should be able to retrieve EC public key successfully";
    UniqueBlob x509_blob(x509_data, x509_data_length);

    const unsigned char *tmp = static_cast<const unsigned char*>(x509_blob.get());
    Unique_EVP_PKEY actual(d2i_PUBKEY((EVP_PKEY**) NULL, &tmp,
            static_cast<long>(x509_blob.length())));

    ASSERT_EQ(EVP_PKEY_type(actual.get()->type), EVP_PKEY_EC)
            << "Generated key type should be of type EC";

    const unsigned char *expectedTmp = static_cast<const unsigned char*>(TEST_EC_KEY_1);
    Unique_PKCS8_PRIV_KEY_INFO expectedPkcs8(
            d2i_PKCS8_PRIV_KEY_INFO((PKCS8_PRIV_KEY_INFO**) NULL, &expectedTmp,
                    sizeof(TEST_EC_KEY_1)));

    Unique_EVP_PKEY expected(EVP_PKCS82PKEY(expectedPkcs8.get()));

    ASSERT_EQ(1, EVP_PKEY_cmp(expected.get(), actual.get()))
            << "Expected and actual keys should match";
}

TEST_F(KeymasterTest, ImportKeyPair_BogusKey_Failure) {
    uint8_t* key_blob;
    size_t key_blob_length;

    ASSERT_EQ(-1,
            sDevice->import_keypair(sDevice, BOGUS_KEY_1, sizeof(BOGUS_KEY_1),
                    &key_blob, &key_blob_length))
            << "Should not import an unknown key type";
}

TEST_F(KeymasterTest, ImportKeyPair_NullKey_Failure) {
    uint8_t* key_blob;
    size_t key_blob_length;

    ASSERT_EQ(-1,
            sDevice->import_keypair(sDevice, NULL, 0,
                    &key_blob, &key_blob_length))
            << "Should not import a null key";
}

TEST_F(KeymasterTest, GetKeypairPublic_RSA_Success) {
    uint8_t* key_blob;
    size_t key_blob_length;

    UniqueReadOnlyBlob testKey(TEST_RSA_KEY_1, sizeof(TEST_RSA_KEY_1));
    ASSERT_TRUE(testKey.get() != NULL);

    ASSERT_EQ(0,
            sDevice->import_keypair(sDevice, testKey.get(), testKey.length(),
                    &key_blob, &key_blob_length))
            << "Should successfully import an RSA key";
    UniqueKey key(&sDevice, key_blob, key_blob_length);

    uint8_t* x509_data;
    size_t x509_data_length;
    ASSERT_EQ(0,
            sDevice->get_keypair_public(sDevice, key_blob, key_blob_length,
                    &x509_data, &x509_data_length))
            << "Should be able to retrieve RSA public key successfully";
    UniqueBlob x509_blob(x509_data, x509_data_length);
}

TEST_F(KeymasterTest, GetKeypairPublic_EC_Success) {
    uint8_t* key_blob;
    size_t key_blob_length;

    UniqueReadOnlyBlob testKey(TEST_EC_KEY_1, sizeof(TEST_EC_KEY_1));
    ASSERT_TRUE(testKey.get() != NULL);

    ASSERT_EQ(0,
            sDevice->import_keypair(sDevice, testKey.get(), testKey.length(),
                    &key_blob, &key_blob_length))
            << "Should successfully import an EC key";
    UniqueKey key(&sDevice, key_blob, key_blob_length);

    uint8_t* x509_data;
    size_t x509_data_length;
    ASSERT_EQ(0,
            sDevice->get_keypair_public(sDevice, key_blob, key_blob_length,
                    &x509_data, &x509_data_length))
            << "Should be able to retrieve EC public key successfully";
    UniqueBlob x509_blob(x509_data, x509_data_length);
}

TEST_F(KeymasterTest, GetKeypairPublic_NullKey_Failure) {
    uint8_t* key_blob;
    size_t key_blob_length;

    uint8_t* x509_data = NULL;
    size_t x509_data_length;
    ASSERT_EQ(-1,
            sDevice->get_keypair_public(sDevice, NULL, 0,
                    &x509_data, &x509_data_length))
            << "Should not be able to retrieve public key from null key";
    UniqueBlob x509_blob(x509_data, x509_data_length);
}

TEST_F(KeymasterTest, GetKeypairPublic_RSA_NullDestination_Failure) {
    uint8_t* key_blob;
    size_t key_blob_length;

    UniqueReadOnlyBlob testKey(TEST_RSA_KEY_1, sizeof(TEST_RSA_KEY_1));
    ASSERT_TRUE(testKey.get() != NULL);

    ASSERT_EQ(0,
            sDevice->import_keypair(sDevice, testKey.get(), testKey.length(),
                    &key_blob, &key_blob_length))
            << "Should successfully import an RSA key";
    UniqueKey key(&sDevice, key_blob, key_blob_length);

    ASSERT_EQ(-1,
            sDevice->get_keypair_public(sDevice, key.get(), key.length(),
                    NULL, NULL))
            << "Should not be able to succeed with NULL destination blob";
}

TEST_F(KeymasterTest, GetKeypairPublic_EC_NullDestination_Failure) {
    uint8_t* key_blob;
    size_t key_blob_length;

    UniqueReadOnlyBlob testKey(TEST_EC_KEY_1, sizeof(TEST_EC_KEY_1));
    ASSERT_TRUE(testKey.get() != NULL);

    ASSERT_EQ(0,
            sDevice->import_keypair(sDevice, testKey.get(), testKey.length(),
                    &key_blob, &key_blob_length))
            << "Should successfully import an RSA key";
    UniqueKey key(&sDevice, key_blob, key_blob_length);

    ASSERT_EQ(-1,
            sDevice->get_keypair_public(sDevice, key.get(), key.length(),
                    NULL, NULL))
            << "Should not be able to succeed with NULL destination blob";
}

TEST_F(KeymasterTest, DeleteKeyPair_RSA_Success) {
    uint8_t* key_blob;
    size_t key_blob_length;

    UniqueReadOnlyBlob testKey(TEST_RSA_KEY_1, sizeof(TEST_RSA_KEY_1));
    ASSERT_TRUE(testKey.get() != NULL);

    ASSERT_EQ(0,
            sDevice->import_keypair(sDevice, testKey.get(), testKey.length(),
                    &key_blob, &key_blob_length))
            << "Should successfully import an RSA key";
    UniqueKey key(&sDevice, key_blob, key_blob_length);
}

TEST_F(KeymasterTest, DeleteKeyPair_RSA_DoubleDelete_Failure) {
    uint8_t* key_blob;
    size_t key_blob_length;

    UniqueReadOnlyBlob testKey(TEST_RSA_KEY_1, sizeof(TEST_RSA_KEY_1));
    ASSERT_TRUE(testKey.get() != NULL);

    /*
     * This is only run if the module indicates it implements key deletion
     * by implementing delete_keypair.
     */
    if (sDevice->delete_keypair != NULL) {
        ASSERT_EQ(0,
                sDevice->import_keypair(sDevice, testKey.get(), testKey.length(),
                        &key_blob, &key_blob_length))
                << "Should successfully import an RSA key";
        UniqueBlob blob(key_blob, key_blob_length);

        ASSERT_EQ(0, sDevice->delete_keypair(sDevice, key_blob, key_blob_length))
                << "Should delete key after import";

        ASSERT_EQ(-1, sDevice->delete_keypair(sDevice, key_blob, key_blob_length))
                << "Should not be able to delete key twice";
    }
}

TEST_F(KeymasterTest, DeleteKeyPair_RSA_NullKey_Failure) {
    /*
     * This is only run if the module indicates it implements key deletion
     * by implementing delete_keypair.
     */
    if (sDevice->delete_keypair != NULL) {
        ASSERT_EQ(-1, sDevice->delete_keypair(sDevice, NULL, 0))
                << "Should not be able to delete null key";
    }
}

/*
 * DER-encoded PKCS#8 format RSA key. Generated using:
 *
 * openssl genrsa 512 | openssl pkcs8 -topk8 -nocrypt -outform der | recode ../x1
 */
static uint8_t TEST_SIGN_RSA_KEY_1[] = {
        0x30, 0x82, 0x01, 0x56, 0x02, 0x01, 0x00, 0x30, 0x0D, 0x06, 0x09, 0x2A,
        0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x01, 0x05, 0x00, 0x04, 0x82,
        0x01, 0x40, 0x30, 0x82, 0x01, 0x3C, 0x02, 0x01, 0x00, 0x02, 0x41, 0x00,
        0xBD, 0xC0, 0x7F, 0xEF, 0x75, 0x1D, 0x63, 0x2A, 0xD0, 0x9A, 0x26, 0xE5,
        0x5B, 0xB9, 0x84, 0x7C, 0xE5, 0xC7, 0xE7, 0xDE, 0xFE, 0xB6, 0x54, 0xD9,
        0xF0, 0x9B, 0xC2, 0xCF, 0x36, 0xDA, 0xE5, 0x4D, 0xC5, 0xD9, 0x25, 0x78,
        0xBD, 0x55, 0x05, 0xBD, 0x86, 0xFB, 0x37, 0x15, 0x33, 0x42, 0x52, 0xED,
        0xE5, 0xCD, 0xCB, 0xB7, 0xA2, 0x51, 0xFA, 0x36, 0xE9, 0x9C, 0x2E, 0x5D,
        0xE3, 0xA5, 0x1F, 0x01, 0x02, 0x03, 0x01, 0x00, 0x01, 0x02, 0x41, 0x00,
        0x96, 0x71, 0xDE, 0xBD, 0x83, 0x94, 0x96, 0x40, 0xA6, 0xFD, 0xE1, 0xA2,
        0xED, 0xD3, 0xAC, 0x28, 0xBE, 0xA2, 0x7D, 0xC3, 0xFF, 0x1D, 0x9F, 0x2E,
        0xE0, 0xA7, 0x0E, 0x90, 0xEE, 0x44, 0x25, 0x92, 0xE3, 0x54, 0xDD, 0x55,
        0xA3, 0xEF, 0x42, 0xF5, 0x52, 0x55, 0x41, 0x47, 0x5E, 0x00, 0xFB, 0x8B,
        0x47, 0x5E, 0x45, 0x49, 0xEA, 0x3D, 0x2C, 0xFD, 0x9F, 0xEC, 0xC8, 0x4E,
        0x4E, 0x86, 0x90, 0x31, 0x02, 0x21, 0x00, 0xE6, 0xA5, 0x55, 0xB3, 0x64,
        0xAB, 0x90, 0x5E, 0xA2, 0xF5, 0x6B, 0x21, 0x4B, 0x15, 0xD6, 0x4A, 0xB6,
        0x60, 0x24, 0x95, 0x65, 0xA2, 0xBE, 0xBA, 0x2A, 0x73, 0xFB, 0xFF, 0x2C,
        0x61, 0x88, 0x9D, 0x02, 0x21, 0x00, 0xD2, 0x9C, 0x5B, 0xFE, 0x82, 0xA5,
        0xFC, 0x52, 0x6A, 0x29, 0x38, 0xDB, 0x22, 0x3B, 0xEB, 0x74, 0x3B, 0xCA,
        0xB4, 0xDD, 0x1D, 0xE4, 0x48, 0x60, 0x70, 0x19, 0x9B, 0x81, 0xC1, 0x83,
        0x28, 0xB5, 0x02, 0x21, 0x00, 0x89, 0x2D, 0xFE, 0xF9, 0xF2, 0xBF, 0x43,
        0xDF, 0xB5, 0xA6, 0xA8, 0x30, 0x26, 0x1B, 0x77, 0xD7, 0xF9, 0xFE, 0xD6,
        0xE3, 0x70, 0x8E, 0xCA, 0x47, 0xA9, 0xA6, 0x50, 0x54, 0x25, 0xCE, 0x60,
        0xD5, 0x02, 0x21, 0x00, 0xBE, 0x5A, 0xF8, 0x82, 0xE6, 0xCE, 0xE3, 0x6A,
        0x11, 0xED, 0xC4, 0x27, 0xBB, 0x9F, 0x70, 0xC6, 0x93, 0xAC, 0x39, 0x20,
        0x89, 0x7D, 0xE5, 0x34, 0xD4, 0xDD, 0x30, 0x42, 0x6D, 0x07, 0x00, 0xE9,
        0x02, 0x20, 0x05, 0x91, 0xEF, 0x12, 0xD2, 0xD3, 0x6A, 0xD2, 0x96, 0x6B,
        0x10, 0x62, 0xF9, 0xBA, 0xA4, 0x91, 0x48, 0x84, 0x40, 0x61, 0x67, 0x80,
        0x68, 0x68, 0xC8, 0x60, 0xB3, 0x66, 0xC8, 0xF9, 0x08, 0x9A,
};

/*
 * DER-encoded PKCS#8 format EC key. Generated using:
 *
 * openssl ecparam -name prime256v1 -genkey -noout | openssl pkcs8 -topk8 -nocrypt -outform der | recode ../x1
 */
static uint8_t TEST_SIGN_EC_KEY_1[] = {
        0x30, 0x81, 0x87, 0x02, 0x01, 0x00, 0x30, 0x13, 0x06, 0x07, 0x2A, 0x86,
        0x48, 0xCE, 0x3D, 0x02, 0x01, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D,
        0x03, 0x01, 0x07, 0x04, 0x6D, 0x30, 0x6B, 0x02, 0x01, 0x01, 0x04, 0x20,
        0x9E, 0x66, 0x11, 0x6A, 0x89, 0xF5, 0x78, 0x57, 0xF3, 0x35, 0xA2, 0x46,
        0x09, 0x06, 0x4B, 0x4D, 0x81, 0xEC, 0xD3, 0x9B, 0x0A, 0xC4, 0x68, 0x06,
        0xB8, 0x42, 0x24, 0x5E, 0x74, 0x2C, 0x62, 0x79, 0xA1, 0x44, 0x03, 0x42,
        0x00, 0x04, 0x35, 0xB5, 0x9A, 0x5C, 0xE5, 0x52, 0x35, 0xF2, 0x10, 0x6C,
        0xD9, 0x98, 0x67, 0xED, 0x5E, 0xCB, 0x6B, 0xB8, 0x96, 0x5E, 0x54, 0x7C,
        0x34, 0x2A, 0xA3, 0x3B, 0xF3, 0xD1, 0x39, 0x48, 0x36, 0x7A, 0xEA, 0xD8,
        0xCA, 0xDD, 0x40, 0x8F, 0xE9, 0xE0, 0x95, 0x2E, 0x3F, 0x95, 0x0F, 0x14,
        0xD6, 0x14, 0x78, 0xB5, 0xAD, 0x17, 0xD2, 0x5A, 0x41, 0x96, 0x99, 0x20,
        0xC7, 0x5B, 0x0F, 0x60, 0xFD, 0xBA
};

/*
 * PKCS#1 v1.5 padded raw "Hello, world"  Can be generated be generated by verifying
 * the signature below in no padding mode:
 *
 * openssl rsautl -keyform der -inkey rsa.der -raw -verify -in test.sig
 */
static uint8_t TEST_SIGN_DATA_1[] = {
        0x00, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0x00, 0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x2C, 0x20, 0x77,
        0x6F, 0x72, 0x6C, 0x64,
};

/*
 * Signature of TEST_SIGN_DATA_1 using TEST_SIGN_RSA_KEY_1. Generated using:
 *
 * echo 'Hello, world' | openssl rsautl -keyform der -inkey rsa.der -sign | recode ../x1
 */
static uint8_t TEST_SIGN_RSA_SIGNATURE_1[] = {
        0xA4, 0xBB, 0x76, 0x87, 0xFE, 0x61, 0x0C, 0x9D, 0xD6, 0xFF, 0x4B, 0x76,
        0x96, 0x08, 0x36, 0x23, 0x11, 0xC6, 0x44, 0x3F, 0x88, 0x77, 0x97, 0xB2,
        0xA8, 0x3B, 0xFB, 0x9C, 0x3C, 0xD3, 0x20, 0x65, 0xFD, 0x26, 0x3B, 0x2A,
        0xB8, 0xB6, 0xD4, 0xDC, 0x91, 0xF7, 0xE2, 0xDE, 0x4D, 0xF7, 0x0E, 0xB9,
        0x72, 0xA7, 0x29, 0x72, 0x82, 0x12, 0x7C, 0x53, 0x23, 0x21, 0xC4, 0xFF,
        0x79, 0xE4, 0x91, 0x40,
};

/*
 * Identical to TEST_SIGN_RSA_SIGNATURE_1 except the last octet is '1' instead of '0'
 * This should fail any test.
 */
static uint8_t TEST_SIGN_SIGNATURE_BOGUS_1[] = {
        0xA4, 0xBB, 0x76, 0x87, 0xFE, 0x61, 0x0C, 0x9D, 0xD6, 0xFF, 0x4B, 0x76,
        0x96, 0x08, 0x36, 0x23, 0x11, 0xC6, 0x44, 0x3F, 0x88, 0x77, 0x97, 0xB2,
        0xA8, 0x3B, 0xFB, 0x9C, 0x3C, 0xD3, 0x20, 0x65, 0xFD, 0x26, 0x3B, 0x2A,
        0xB8, 0xB6, 0xD4, 0xDC, 0x91, 0xF7, 0xE2, 0xDE, 0x4D, 0xF7, 0x0E, 0xB9,
        0x72, 0xA7, 0x29, 0x72, 0x82, 0x12, 0x7C, 0x53, 0x23, 0x21, 0xC4, 0xFF,
        0x79, 0xE4, 0x91, 0x41,
};

TEST_F(KeymasterTest, SignData_RSA_Raw_Success) {
    uint8_t* key_blob;
    size_t key_blob_length;

    UniqueReadOnlyBlob testKey(TEST_SIGN_RSA_KEY_1, sizeof(TEST_SIGN_RSA_KEY_1));
    ASSERT_TRUE(testKey.get() != NULL);

    ASSERT_EQ(0,
            sDevice->import_keypair(sDevice, testKey.get(), testKey.length(),
                    &key_blob, &key_blob_length))
            << "Should successfully import an RSA key";
    UniqueKey key(&sDevice, key_blob, key_blob_length);

    keymaster_rsa_sign_params_t params = {
            digest_type: DIGEST_NONE,
            padding_type: PADDING_NONE,
    };

    uint8_t* sig;
    size_t sig_length;

    UniqueReadOnlyBlob testData(TEST_SIGN_DATA_1, sizeof(TEST_SIGN_DATA_1));
    ASSERT_TRUE(testData.get() != NULL);

    ASSERT_EQ(0,
            sDevice->sign_data(sDevice, &params, key_blob, key_blob_length,
                    testData.get(), testData.length(),
                    &sig, &sig_length))
            << "Should sign data successfully";
    UniqueBlob sig_blob(sig, sig_length);

    UniqueBlob expected_sig(TEST_SIGN_RSA_SIGNATURE_1, sizeof(TEST_SIGN_RSA_SIGNATURE_1));

    ASSERT_EQ(expected_sig, sig_blob)
            << "Generated signature should match expected signature";

    // The expected signature is actually stack data, so don't let it try to free.
    uint8_t* unused __attribute__((unused)) = expected_sig.release();
}

TEST_F(KeymasterTest, SignData_EC_Success) {
    uint8_t* key_blob;
    size_t key_blob_length;

    UniqueReadOnlyBlob testKey(TEST_SIGN_EC_KEY_1, sizeof(TEST_SIGN_EC_KEY_1));
    ASSERT_TRUE(testKey.get() != NULL);

    ASSERT_EQ(0,
            sDevice->import_keypair(sDevice, testKey.get(), testKey.length(),
                    &key_blob, &key_blob_length))
            << "Should successfully import an EC key";
    UniqueKey key(&sDevice, key_blob, key_blob_length);

    keymaster_ec_sign_params_t params = {
            digest_type: DIGEST_NONE,
    };

    uint8_t* sig;
    size_t sig_length;

    UniqueReadOnlyBlob testData(TEST_SIGN_DATA_1, sizeof(TEST_SIGN_DATA_1));
    ASSERT_TRUE(testData.get() != NULL);

    ASSERT_EQ(0,
            sDevice->sign_data(sDevice, &params, key_blob, key_blob_length,
                    testData.get(), testData.length(),
                    &sig, &sig_length))
            << "Should sign data successfully";
    UniqueBlob sig_blob(sig, sig_length);

    uint8_t* x509_data;
    size_t x509_data_length;
    ASSERT_EQ(0,
            sDevice->get_keypair_public(sDevice, key_blob, key_blob_length,
                    &x509_data, &x509_data_length))
            << "Should be able to retrieve RSA public key successfully";
    UniqueBlob x509_blob(x509_data, x509_data_length);

    const unsigned char *tmp = static_cast<const unsigned char*>(x509_blob.get());
    Unique_EVP_PKEY expected(d2i_PUBKEY((EVP_PKEY**) NULL, &tmp,
            static_cast<long>(x509_blob.length())));

    Unique_EC_KEY ecKey(EVP_PKEY_get1_EC_KEY(expected.get()));

    ASSERT_EQ(1, ECDSA_verify(0, testData.get(), testData.length(), sig_blob.get(), sig_blob.length(), ecKey.get()))
            << "Signature should verify";
}

TEST_F(KeymasterTest, SignData_RSA_Raw_InvalidSizeInput_Failure) {
    uint8_t* key_blob;
    size_t key_blob_length;

    UniqueReadOnlyBlob testKey(TEST_SIGN_RSA_KEY_1, sizeof(TEST_SIGN_RSA_KEY_1));
    ASSERT_TRUE(testKey.get() != NULL);

    ASSERT_EQ(0,
            sDevice->import_keypair(sDevice, testKey.get(), testKey.length(),
                    &key_blob, &key_blob_length))
            << "Should successfully import an RSA key";
    UniqueKey key(&sDevice, key_blob, key_blob_length);

    keymaster_rsa_sign_params_t params = {
            digest_type: DIGEST_NONE,
            padding_type: PADDING_NONE,
    };

    uint8_t* sig;
    size_t sig_length;

    UniqueReadOnlyBlob testData(TEST_RSA_KEY_1, sizeof(TEST_RSA_KEY_1));
    ASSERT_TRUE(testData.get() != NULL);

    ASSERT_EQ(-1,
            sDevice->sign_data(sDevice, &params, key_blob, key_blob_length,
                    testData.get(), testData.length(),
                    &sig, &sig_length))
            << "Should not be able to do raw signature on incorrect size data";
}

TEST_F(KeymasterTest, SignData_RSA_Raw_NullKey_Failure) {
    keymaster_rsa_sign_params_t params = {
            digest_type: DIGEST_NONE,
            padding_type: PADDING_NONE,
    };

    uint8_t* sig;
    size_t sig_length;

    UniqueReadOnlyBlob testData(TEST_RSA_KEY_1, sizeof(TEST_RSA_KEY_1));
    ASSERT_TRUE(testData.get() != NULL);

    ASSERT_EQ(-1,
            sDevice->sign_data(sDevice, &params, NULL, 0,
                    testData.get(), testData.length(),
                    &sig, &sig_length))
            << "Should not be able to do raw signature on incorrect size data";
}

TEST_F(KeymasterTest, SignData_RSA_Raw_NullInput_Failure) {
    uint8_t* key_blob;
    size_t key_blob_length;

    UniqueReadOnlyBlob testKey(TEST_SIGN_RSA_KEY_1, sizeof(TEST_SIGN_RSA_KEY_1));
    ASSERT_TRUE(testKey.get() != NULL);

    ASSERT_EQ(0,
            sDevice->import_keypair(sDevice, testKey.get(), testKey.length(),
                    &key_blob, &key_blob_length))
            << "Should successfully import an RSA key";
    UniqueKey key(&sDevice, key_blob, key_blob_length);

    keymaster_rsa_sign_params_t params = {
            digest_type: DIGEST_NONE,
            padding_type: PADDING_NONE,
    };

    uint8_t* sig;
    size_t sig_length;

    ASSERT_EQ(-1,
            sDevice->sign_data(sDevice, &params, key_blob, key_blob_length,
                    NULL, 0,
                    &sig, &sig_length))
            << "Should error when input data is null";
}

TEST_F(KeymasterTest, SignData_RSA_Raw_NullOutput_Failure) {
    uint8_t* key_blob;
    size_t key_blob_length;

    UniqueReadOnlyBlob testKey(TEST_SIGN_RSA_KEY_1, sizeof(TEST_SIGN_RSA_KEY_1));
    ASSERT_TRUE(testKey.get() != NULL);

    ASSERT_EQ(0,
            sDevice->import_keypair(sDevice, testKey.get(), testKey.length(),
                    &key_blob, &key_blob_length))
            << "Should successfully import an RSA key";
    UniqueKey key(&sDevice, key_blob, key_blob_length);

    keymaster_rsa_sign_params_t params = {
            digest_type: DIGEST_NONE,
            padding_type: PADDING_NONE,
    };

    uint8_t* sig;
    size_t sig_length;

    UniqueReadOnlyBlob testData(TEST_RSA_KEY_1, sizeof(TEST_RSA_KEY_1));
    ASSERT_TRUE(testData.get() != NULL);

    ASSERT_EQ(-1,
            sDevice->sign_data(sDevice, &params, key_blob, key_blob_length,
                    testData.get(), testData.length(),
                    NULL, NULL))
            << "Should error when output is null";
}

TEST_F(KeymasterTest, VerifyData_RSA_Raw_Success) {
    uint8_t* key_blob;
    size_t key_blob_length;

    UniqueReadOnlyBlob testKey(TEST_SIGN_RSA_KEY_1, sizeof(TEST_SIGN_RSA_KEY_1));
    ASSERT_TRUE(testKey.get() != NULL);

    ASSERT_EQ(0,
            sDevice->import_keypair(sDevice, testKey.get(), testKey.length(),
                    &key_blob, &key_blob_length))
            << "Should successfully import an RSA key";
    UniqueKey key(&sDevice, key_blob, key_blob_length);

    keymaster_rsa_sign_params_t params = {
            digest_type: DIGEST_NONE,
            padding_type: PADDING_NONE,
    };

    UniqueReadOnlyBlob testData(TEST_SIGN_DATA_1, sizeof(TEST_SIGN_DATA_1));
    ASSERT_TRUE(testData.get() != NULL);

    UniqueReadOnlyBlob testSig(TEST_SIGN_RSA_SIGNATURE_1, sizeof(TEST_SIGN_RSA_SIGNATURE_1));
    ASSERT_TRUE(testSig.get() != NULL);

    ASSERT_EQ(0,
            sDevice->verify_data(sDevice, &params, key_blob, key_blob_length,
                    testData.get(), testData.length(),
                    testSig.get(), testSig.length()))
            << "Should verify data successfully";
}

TEST_F(KeymasterTest, VerifyData_EC_Raw_Success) {
    uint8_t* key_blob;
    size_t key_blob_length;

    UniqueReadOnlyBlob testKey(TEST_SIGN_EC_KEY_1, sizeof(TEST_SIGN_EC_KEY_1));
    ASSERT_TRUE(testKey.get() != NULL);

    ASSERT_EQ(0,
            sDevice->import_keypair(sDevice, testKey.get(), testKey.length(),
                    &key_blob, &key_blob_length))
            << "Should successfully import an RSA key";
    UniqueKey key(&sDevice, key_blob, key_blob_length);

    keymaster_ec_sign_params_t params = {
            digest_type: DIGEST_NONE,
    };

    uint8_t* sig;
    size_t sig_length;

    UniqueReadOnlyBlob testData(TEST_SIGN_DATA_1, sizeof(TEST_SIGN_DATA_1));
    ASSERT_TRUE(testData.get() != NULL);

    ASSERT_EQ(0,
            sDevice->sign_data(sDevice, &params, key_blob, key_blob_length,
                    testData.get(), testData.length(),
                    &sig, &sig_length))
            << "Should sign data successfully";
    UniqueBlob sig_blob(sig, sig_length);

    ASSERT_EQ(0,
            sDevice->verify_data(sDevice, &params, key_blob, key_blob_length,
                    testData.get(), testData.length(),
                    sig_blob.get(), sig_blob.length()))
            << "Should verify data successfully";
}

TEST_F(KeymasterTest, VerifyData_RSA_Raw_BadSignature_Failure) {
    uint8_t* key_blob;
    size_t key_blob_length;

    UniqueReadOnlyBlob testKey(TEST_SIGN_RSA_KEY_1, sizeof(TEST_SIGN_RSA_KEY_1));
    ASSERT_TRUE(testKey.get() != NULL);

    ASSERT_EQ(0,
            sDevice->import_keypair(sDevice, testKey.get(), testKey.length(),
                    &key_blob, &key_blob_length))
            << "Should successfully import an RSA key";
    UniqueKey key(&sDevice, key_blob, key_blob_length);

    keymaster_rsa_sign_params_t params = {
            digest_type: DIGEST_NONE,
            padding_type: PADDING_NONE,
    };

    ASSERT_EQ(-1,
            sDevice->verify_data(sDevice, &params, key_blob, key_blob_length,
                    TEST_SIGN_DATA_1, sizeof(TEST_SIGN_DATA_1),
                    TEST_SIGN_SIGNATURE_BOGUS_1, sizeof(TEST_SIGN_SIGNATURE_BOGUS_1)))
            << "Should sign data successfully";
}

TEST_F(KeymasterTest, VerifyData_EC_Raw_BadSignature_Failure) {
    uint8_t* key_blob;
    size_t key_blob_length;

    UniqueReadOnlyBlob testKey(TEST_SIGN_EC_KEY_1, sizeof(TEST_SIGN_EC_KEY_1));
    ASSERT_TRUE(testKey.get() != NULL);

    ASSERT_EQ(0,
            sDevice->import_keypair(sDevice, testKey.get(), testKey.length(),
                    &key_blob, &key_blob_length))
            << "Should successfully import an RSA key";
    UniqueKey key(&sDevice, key_blob, key_blob_length);

    keymaster_ec_sign_params_t params = {
            digest_type: DIGEST_NONE,
    };

    ASSERT_EQ(-1,
            sDevice->verify_data(sDevice, &params, key_blob, key_blob_length,
                    TEST_SIGN_DATA_1, sizeof(TEST_SIGN_DATA_1),
                    TEST_SIGN_SIGNATURE_BOGUS_1, sizeof(TEST_SIGN_SIGNATURE_BOGUS_1)))
            << "Should sign data successfully";
}

TEST_F(KeymasterTest, VerifyData_RSA_Raw_NullKey_Failure) {
    keymaster_rsa_sign_params_t params = {
            digest_type: DIGEST_NONE,
            padding_type: PADDING_NONE,
    };

    UniqueReadOnlyBlob testData(TEST_SIGN_DATA_1, sizeof(TEST_SIGN_DATA_1));
    ASSERT_TRUE(testData.get() != NULL);

    UniqueReadOnlyBlob testSig(TEST_SIGN_SIGNATURE_BOGUS_1, sizeof(TEST_SIGN_SIGNATURE_BOGUS_1));
    ASSERT_TRUE(testSig.get() != NULL);

    ASSERT_EQ(-1,
            sDevice->verify_data(sDevice, &params, NULL, 0,
                    testData.get(), testData.length(),
                    testSig.get(), testSig.length()))
            << "Should fail when key is null";
}

TEST_F(KeymasterTest, VerifyData_RSA_NullInput_Failure) {
    uint8_t* key_blob;
    size_t key_blob_length;

    ASSERT_EQ(0,
            sDevice->import_keypair(sDevice, TEST_SIGN_RSA_KEY_1, sizeof(TEST_SIGN_RSA_KEY_1),
                    &key_blob, &key_blob_length))
            << "Should successfully import an RSA key";
    UniqueKey key(&sDevice, key_blob, key_blob_length);

    keymaster_rsa_sign_params_t params = {
            digest_type: DIGEST_NONE,
            padding_type: PADDING_NONE,
    };

    UniqueReadOnlyBlob testSig(TEST_SIGN_RSA_SIGNATURE_1, sizeof(TEST_SIGN_RSA_SIGNATURE_1));
    ASSERT_TRUE(testSig.get() != NULL);

    ASSERT_EQ(-1,
            sDevice->verify_data(sDevice, &params, key_blob, key_blob_length,
                    NULL, 0,
                    testSig.get(), testSig.length()))
            << "Should fail on null input";
}

TEST_F(KeymasterTest, VerifyData_RSA_NullSignature_Failure) {
    uint8_t* key_blob;
    size_t key_blob_length;

    UniqueReadOnlyBlob testKey(TEST_SIGN_RSA_KEY_1, sizeof(TEST_SIGN_RSA_KEY_1));
    ASSERT_TRUE(testKey.get() != NULL);

    ASSERT_EQ(0,
            sDevice->import_keypair(sDevice, testKey.get(), testKey.length(),
                    &key_blob, &key_blob_length))
            << "Should successfully import an RSA key";
    UniqueKey key(&sDevice, key_blob, key_blob_length);

    keymaster_rsa_sign_params_t params = {
            digest_type: DIGEST_NONE,
            padding_type: PADDING_NONE,
    };

    UniqueReadOnlyBlob testData(TEST_SIGN_DATA_1, sizeof(TEST_SIGN_DATA_1));
    ASSERT_TRUE(testData.get() != NULL);

    ASSERT_EQ(-1,
            sDevice->verify_data(sDevice, &params, key.get(), key.length(),
                    testData.get(), testData.length(),
                    NULL, 0))
            << "Should fail on null signature";
}

TEST_F(KeymasterTest, EraseAll_Success) {
    uint8_t *key1_blob, *key2_blob;
    size_t key1_blob_length, key2_blob_length;

    // Only test this if the device says it supports delete_all
    if (sDevice->delete_all == NULL) {
        return;
    }

    UniqueReadOnlyBlob testKey(TEST_SIGN_RSA_KEY_1, sizeof(TEST_SIGN_RSA_KEY_1));
    ASSERT_TRUE(testKey.get() != NULL);

    ASSERT_EQ(0,
            sDevice->import_keypair(sDevice, testKey.get(), testKey.length(),
                    &key1_blob, &key1_blob_length))
            << "Should successfully import an RSA key";
    UniqueKey key1(&sDevice, key1_blob, key1_blob_length);

    UniqueReadOnlyBlob testKey2(TEST_SIGN_RSA_KEY_1, sizeof(TEST_SIGN_RSA_KEY_1));
    ASSERT_TRUE(testKey2.get() != NULL);

    ASSERT_EQ(0,
            sDevice->import_keypair(sDevice, testKey2.get(), testKey2.length(),
                    &key2_blob, &key2_blob_length))
            << "Should successfully import an RSA key";
    UniqueKey key2(&sDevice, key2_blob, key2_blob_length);

    ASSERT_EQ(0, sDevice->delete_all(sDevice))
            << "Should erase all keys";

    key1.reset();

    uint8_t* x509_data;
    size_t x509_data_length;
    ASSERT_EQ(-1,
            sDevice->get_keypair_public(sDevice, key1_blob, key1_blob_length,
                    &x509_data, &x509_data_length))
            << "Should be able to retrieve RSA public key 1 successfully";

    ASSERT_EQ(-1,
            sDevice->get_keypair_public(sDevice, key2_blob, key2_blob_length,
                    &x509_data, &x509_data_length))
            << "Should be able to retrieve RSA public key 2 successfully";
}

}
