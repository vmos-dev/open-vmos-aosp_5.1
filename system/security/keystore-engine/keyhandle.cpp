/*
 * Copyright 2012 The Android Open Source Project
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <openssl/engine.h>

/**
 * Makes sure the ex_data for the keyhandle is initially set to NULL.
 */
int keyhandle_new(void*, void*, CRYPTO_EX_DATA* ad, int idx, long, void*) {
    return CRYPTO_set_ex_data(ad, idx, NULL);
}

/**
 * Frees a previously allocated keyhandle stored in ex_data.
 */
void keyhandle_free(void *, void *ptr, CRYPTO_EX_DATA*, int, long, void*) {
    char* keyhandle = reinterpret_cast<char*>(ptr);
    if (keyhandle != NULL) {
        free(keyhandle);
    }
}

/**
 * Duplicates a keyhandle stored in ex_data in case we copy a key.
 */
int keyhandle_dup(CRYPTO_EX_DATA* to, CRYPTO_EX_DATA*, void *ptrRef, int idx, long, void *) {
    // This appears to be a bug in OpenSSL.
    void** ptr = reinterpret_cast<void**>(ptrRef);
    char* keyhandle = reinterpret_cast<char*>(*ptr);
    if (keyhandle != NULL) {
        char* keyhandle_copy = strdup(keyhandle);
        *ptr = keyhandle_copy;

        // Call this in case OpenSSL is fixed in the future.
        (void) CRYPTO_set_ex_data(to, idx, keyhandle_copy);
    }
    return 1;
}

void *ex_data_dup(void *data) {
    char* keyhandle = reinterpret_cast<char*>(data);
    return strdup(keyhandle);
}

void ex_data_free(void *data) {
    char* keyhandle = reinterpret_cast<char*>(data);
    free(keyhandle);
}

void ex_data_clear_free(void *data) {
    char* keyhandle = reinterpret_cast<char*>(data);
    memset(data, '\0', strlen(keyhandle));
    free(keyhandle);
}
