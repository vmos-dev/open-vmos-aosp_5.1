/*
 * Copyright 2013 The Android Open Source Project
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

/* For ENGINE method registration purposes. */
extern const char* kKeystoreEngineId;

extern int dsa_key_handle;
extern int rsa_key_handle;

struct DSA_Delete {
    void operator()(DSA* p) const {
        DSA_free(p);
    }
};
typedef UniquePtr<DSA, struct DSA_Delete> Unique_DSA;

struct EC_KEY_Delete {
    void operator()(EC_KEY* p) const {
        EC_KEY_free(p);
    }
};
typedef UniquePtr<EC_KEY, EC_KEY_Delete> Unique_EC_KEY;

struct RSA_Delete {
    void operator()(RSA* p) const {
        RSA_free(p);
    }
};
typedef UniquePtr<RSA, struct RSA_Delete> Unique_RSA;


/* Keyhandles for ENGINE metadata */
int keyhandle_new(void*, void*, CRYPTO_EX_DATA* ad, int idx, long, void*);
void keyhandle_free(void *, void *ptr, CRYPTO_EX_DATA*, int, long, void*);
int keyhandle_dup(CRYPTO_EX_DATA* to, CRYPTO_EX_DATA*, void *ptrRef, int idx, long, void *);

/* For EC_EX_DATA stuff */
void *ex_data_dup(void *);
void ex_data_free(void *);
void ex_data_clear_free(void *);

/* ECDSA */
int ecdsa_register(ENGINE *);
int ecdsa_pkey_setup(ENGINE *, EVP_PKEY*, const char*);

/* DSA */
int dsa_register(ENGINE *);
int dsa_pkey_setup(ENGINE *, EVP_PKEY*, const char*);

/* RSA */
int rsa_register(ENGINE *);
int rsa_pkey_setup(ENGINE *, EVP_PKEY*, const char*);
