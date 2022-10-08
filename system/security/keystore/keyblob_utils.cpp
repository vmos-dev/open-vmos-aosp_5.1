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

#include <sys/types.h>
#include <unistd.h>

/**
 * When a key is being migrated from a software keymaster implementation
 * to a hardware keymaster implementation, the first 4 bytes of the key_blob
 * given to the hardware implementation will be equal to SOFT_KEY_MAGIC.
 * The hardware implementation should import these PKCS#8 format keys which
 * are encoded like this:
 *
 * 4-byte SOFT_KEY_MAGIC
 *
 * 4-byte 32-bit integer big endian for public_key_length. This may be zero
 *     length which indicates the public key should be derived from the
 *     private key.
 *
 * public_key_length bytes of public key (may be empty)
 *
 * 4-byte 32-bit integer big endian for private_key_length
 *
 * private_key_length bytes of private key
 */
static const uint8_t SOFT_KEY_MAGIC[] = { 'P', 'K', '#', '8' };

size_t get_softkey_header_size() {
    return sizeof(SOFT_KEY_MAGIC);
}

uint8_t* add_softkey_header(uint8_t* key_blob, size_t key_blob_length) {
    if (key_blob_length < sizeof(SOFT_KEY_MAGIC)) {
        return NULL;
    }

    memcpy(key_blob, SOFT_KEY_MAGIC, sizeof(SOFT_KEY_MAGIC));

    return key_blob + sizeof(SOFT_KEY_MAGIC);
}

bool is_softkey(const uint8_t* key_blob, const size_t key_blob_length) {
    if (key_blob_length < sizeof(SOFT_KEY_MAGIC)) {
        return false;
    }

    return !memcmp(key_blob, SOFT_KEY_MAGIC, sizeof(SOFT_KEY_MAGIC));
}
