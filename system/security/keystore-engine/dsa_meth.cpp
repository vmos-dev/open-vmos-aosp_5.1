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
#define LOG_TAG "OpenSSL-keystore-dsa"
#include <cutils/log.h>

#include <binder/IServiceManager.h>
#include <keystore/IKeystoreService.h>

#include <openssl/dsa.h>
#include <openssl/engine.h>

#include "methods.h"


using namespace android;

struct DSA_SIG_Delete {
    void operator()(DSA_SIG* p) const {
        DSA_SIG_free(p);
    }
};
typedef UniquePtr<DSA_SIG, struct DSA_SIG_Delete> Unique_DSA_SIG;

static DSA_SIG* keystore_dsa_do_sign(const unsigned char *dgst, int dlen, DSA *dsa) {
    ALOGV("keystore_dsa_do_sign(%p, %d, %p)", dgst, dlen, dsa);

    uint8_t* key_id = reinterpret_cast<uint8_t*>(DSA_get_ex_data(dsa, dsa_key_handle));
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

    int num = DSA_size(dsa);

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

    Unique_DSA_SIG dsa_sig(d2i_DSA_SIG(NULL,
            const_cast<const unsigned char**>(reinterpret_cast<unsigned char**>(&reply)),
            replyLen));
    if (dsa_sig.get() == NULL) {
        ALOGW("conversion from DER to DSA_SIG failed");
        return 0;
    }

    ALOGV("keystore_dsa_do_sign(%p, %d, %p) => returning %p len %zu", dgst, dlen, dsa,
            dsa_sig.get(), replyLen);
    return dsa_sig.release();
}

static DSA_METHOD keystore_dsa_meth = {
        kKeystoreEngineId, /* name */
        keystore_dsa_do_sign, /* dsa_do_sign */
        NULL, /* dsa_sign_setup */
        NULL, /* dsa_do_verify */
        NULL, /* dsa_mod_exp */
        NULL, /* bn_mod_exp */
        NULL, /* init */
        NULL, /* finish */
        0, /* flags */
        NULL, /* app_data */
        NULL, /* dsa_paramgen */
        NULL, /* dsa_keygen */
};

static int register_dsa_methods() {
    const DSA_METHOD* dsa_meth = DSA_OpenSSL();

    keystore_dsa_meth.dsa_do_verify = dsa_meth->dsa_do_verify;

    return 1;
}

int dsa_pkey_setup(ENGINE *e, EVP_PKEY *pkey, const char *key_id) {
    Unique_DSA dsa(EVP_PKEY_get1_DSA(pkey));
    if (!DSA_set_ex_data(dsa.get(), dsa_key_handle, reinterpret_cast<void*>(strdup(key_id)))) {
        ALOGW("Could not set ex_data for loaded DSA key");
        return 0;
    }

    DSA_set_method(dsa.get(), &keystore_dsa_meth);

    /*
     * "DSA_set_ENGINE()" should probably be an OpenSSL API. Since it isn't,
     * and EVP_PKEY_free() calls ENGINE_finish(), we need to call ENGINE_init()
     * here.
     */
    ENGINE_init(e);
    dsa->engine = e;

    return 1;
}

int dsa_register(ENGINE* e) {
    if (!ENGINE_set_DSA(e, &keystore_dsa_meth)
            || !register_dsa_methods()) {
        ALOGE("Could not set up keystore DSA methods");
        return 0;
    }

    return 1;
}
