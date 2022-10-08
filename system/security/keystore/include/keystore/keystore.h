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

#ifndef __KEYSTORE_H__
#define __KEYSTORE_H__

#include <stdint.h>

// note state values overlap with ResponseCode for the purposes of the state() API
enum State {
    STATE_NO_ERROR      = 1,
    STATE_LOCKED        = 2,
    STATE_UNINITIALIZED = 3,
};

enum ResponseCode {
    NO_ERROR          =  STATE_NO_ERROR, // 1
    LOCKED            =  STATE_LOCKED, // 2
    UNINITIALIZED     =  STATE_UNINITIALIZED, // 3
    SYSTEM_ERROR      =  4,
    PROTOCOL_ERROR    =  5,
    PERMISSION_DENIED =  6,
    KEY_NOT_FOUND     =  7,
    VALUE_CORRUPTED   =  8,
    UNDEFINED_ACTION  =  9,
    WRONG_PASSWORD_0  = 10,
    WRONG_PASSWORD_1  = 11,
    WRONG_PASSWORD_2  = 12,
    WRONG_PASSWORD_3  = 13, // MAX_RETRY = 4
    SIGNATURE_INVALID = 14,
};

/*
 * All the flags for import and insert calls.
 */
enum {
    KEYSTORE_FLAG_NONE = 0,
    KEYSTORE_FLAG_ENCRYPTED = 1 << 0,
    KEYSTORE_FLAG_FALLBACK = 1 << 1,
};

/**
 * Returns the size of the softkey magic header value for measuring
 * and allocating purposes.
 */
size_t get_softkey_header_size();

/**
 * Adds the magic softkey header to a key blob.
 *
 * Returns NULL if the destination array is too small. Otherwise it
 * returns the offset directly after the magic value.
 */
uint8_t* add_softkey_header(uint8_t* key_blob, size_t key_blob_length);

/**
 * Returns true if the key blob has a magic softkey header at the beginning.
 */
bool is_softkey(const uint8_t* key_blob, const size_t key_blob_length);

#endif
