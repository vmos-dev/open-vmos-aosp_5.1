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

#include <UniquePtr.h>

//#define LOG_NDEBUG 0
#define LOG_TAG "OpenSSL-keystore-ecdsa"
#include <cutils/log.h>

#include <binder/IServiceManager.h>
#include <keystore/IKeystoreService.h>

#include <openssl/ecdsa.h>
#include <openssl/engine.h>

// TODO replace this with real OpenSSL API when it exists
#include "crypto/ec/ec_lcl.h"
#include "crypto/ecdsa/ecs_locl.h"

#include "methods.h"


using namespace android;

struct ECDSA_SIG_Delete {
    void operator()(ECDSA_SIG* p) const {
        ECDSA_SIG_free(p);
    }
};
typedef UniquePtr<ECDSA_SIG, struct ECDSA_SIG_Delete> Unique_ECDSA_SIG;

static ECDSA_SIG* keystore_ecdsa_do_sign(const unsigned char *dgst, int dlen,
        const BIGNUM*, const BIGNUM*, EC_KEY *eckey) {
    ALOGV("keystore_ecdsa_do_sign(%p, %d, %p)", dgst, dlen, eckey);

    uint8_t* key_id = reinterpret_cast<uint8_t*>(EC_KEY_get_key_method_data(eckey,
                ex_data_dup, ex_data_free, ex_data_clear_free));
    if (key_id == NULL) {
        ALOGE("key had no key_id!");
        return 0;
    }

    sp<IServiceManager> sm = defaultServiceManager();
    sp<IBinder> binder = sm->getService(String16("android.security.keystore"));
    sp<IKeystoreService> service = interface_cast<IKeystoreService>(binder);

    if (service == NULL) {
        ALOGE("could not contact keystore");
        return 0;
    }

    int num = ECDSA_size(eckey);

    uint8_t* reply = NULL;
    size_t replyLen;
    int32_t ret = service->sign(String16(reinterpret_cast<const char*>(key_id)), dgst,
            dlen, &reply, &replyLen);
    if (ret < 0) {
        ALOGW("There was an error during dsa_do_sign: could not connect");
        return 0;
    } else if (ret != 0) {
        ALOGW("Error during sign from keystore: %d", ret);
        return 0;
    } else if (replyLen <= 0) {
        ALOGW("No valid signature returned");
        return 0;
    } else if (replyLen > (size_t) num) {
        ALOGW("Signature is too large");
        return 0;
    }

    Unique_ECDSA_SIG ecdsa_sig(d2i_ECDSA_SIG(NULL,
            const_cast<const unsigned char**>(reinterpret_cast<unsigned char**>(&reply)),
            replyLen));
    if (ecdsa_sig.get() == NULL) {
        ALOGW("conversion from DER to ECDSA_SIG failed");
        return 0;
    }

    ALOGV("keystore_ecdsa_do_sign(%p, %d, %p) => returning %p len %zu", dgst, dlen, eckey,
            ecdsa_sig.get(), replyLen);
    return ecdsa_sig.release();
}

static ECDSA_METHOD keystore_ecdsa_meth = {
        kKeystoreEngineId, /* name */
        keystore_ecdsa_do_sign, /* ecdsa_do_sign */
        NULL, /* ecdsa_sign_setup */
        NULL, /* ecdsa_do_verify */
        0, /* flags */
        NULL, /* app_data */
};

static int register_ecdsa_methods() {
    const ECDSA_METHOD* ecdsa_meth = ECDSA_OpenSSL();

    keystore_ecdsa_meth.ecdsa_do_verify = ecdsa_meth->ecdsa_do_verify;

    return 1;
}

int ecdsa_pkey_setup(ENGINE *e, EVP_PKEY *pkey, const char *key_id) {
    Unique_EC_KEY eckey(EVP_PKEY_get1_EC_KEY(pkey));
    void* oldData = EC_KEY_insert_key_method_data(eckey.get(),
            reinterpret_cast<void*>(strdup(key_id)), ex_data_dup, ex_data_free,
            ex_data_clear_free);
    if (oldData != NULL) {
        free(oldData);
    }

    ECDSA_set_method(eckey.get(), &keystore_ecdsa_meth);

    /*
     * "ECDSA_set_ENGINE()" should probably be an OpenSSL API. Since it isn't,
     * and EC_KEY_free() calls ENGINE_finish(), we need to call ENGINE_init()
     * here.
     */
    ECDSA_DATA *ecdsa = ecdsa_check(eckey.get());
    ENGINE_init(e);
    ecdsa->engine = e;

    return 1;
}

int ecdsa_register(ENGINE* e) {
    if (!ENGINE_set_ECDSA(e, &keystore_ecdsa_meth)
            || !register_ecdsa_methods()) {
        ALOGE("Could not set up keystore ECDSA methods");
        return 0;
    }

    return 1;
}
