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

#include <UniquePtr.h>

//#define LOG_NDEBUG 0
#define LOG_TAG "OpenSSL-keystore-rsa"
#include <cutils/log.h>

#include <binder/IServiceManager.h>
#include <keystore/IKeystoreService.h>

#include <openssl/rsa.h>
#include <openssl/engine.h>

#include "methods.h"


using namespace android;


int keystore_rsa_priv_enc(int flen, const unsigned char* from, unsigned char* to, RSA* rsa,
        int padding) {
    ALOGV("keystore_rsa_priv_enc(%d, %p, %p, %p, %d)", flen, from, to, rsa, padding);

    int num = RSA_size(rsa);
    UniquePtr<uint8_t> padded(new uint8_t[num]);
    if (padded.get() == NULL) {
        ALOGE("could not allocate padded signature");
        return 0;
    }

    switch (padding) {
    case RSA_PKCS1_PADDING:
        if (!RSA_padding_add_PKCS1_type_1(padded.get(), num, from, flen)) {
            return 0;
        }
        break;
    case RSA_X931_PADDING:
        if (!RSA_padding_add_X931(padded.get(), num, from, flen)) {
            return 0;
        }
        break;
    case RSA_NO_PADDING:
        if (!RSA_padding_add_none(padded.get(), num, from, flen)) {
            return 0;
        }
        break;
    default:
        ALOGE("Unknown padding type: %d", padding);
        return 0;
    }

    uint8_t* key_id = reinterpret_cast<uint8_t*>(RSA_get_ex_data(rsa, rsa_key_handle));
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

    uint8_t* reply = NULL;
    size_t replyLen;
    int32_t ret = service->sign(String16(reinterpret_cast<const char*>(key_id)), padded.get(),
            num, &reply, &replyLen);
    if (ret < 0) {
        ALOGW("There was an error during signing: could not connect");
        free(reply);
        return 0;
    } else if (ret != 0) {
        ALOGW("Error during signing from keystore: %d", ret);
        free(reply);
        return 0;
    } else if (replyLen <= 0) {
        ALOGW("No valid signature returned");
        return 0;
    }

    memcpy(to, reply, replyLen);
    free(reply);

    ALOGV("rsa=%p keystore_rsa_priv_enc => returning %p len %llu", rsa, to,
            (unsigned long long) replyLen);
    return static_cast<int>(replyLen);
}

int keystore_rsa_priv_dec(int flen, const unsigned char* from, unsigned char* to, RSA* rsa,
        int padding) {
    ALOGV("keystore_rsa_priv_dec(%d, %p, %p, %p, %d)", flen, from, to, rsa, padding);

    uint8_t* key_id = reinterpret_cast<uint8_t*>(RSA_get_ex_data(rsa, rsa_key_handle));
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

    int num = RSA_size(rsa);

    uint8_t* reply = NULL;
    size_t replyLen;
    int32_t ret = service->sign(String16(reinterpret_cast<const char*>(key_id)), from,
            flen, &reply, &replyLen);
    if (ret < 0) {
        ALOGW("There was an error during rsa_mod_exp: could not connect");
        return 0;
    } else if (ret != 0) {
        ALOGW("Error during sign from keystore: %d", ret);
        return 0;
    } else if (replyLen <= 0) {
        ALOGW("No valid signature returned");
        return 0;
    }

    /* Trim off the top zero if it's there */
    uint8_t* alignedReply;
    if (*reply == 0x00) {
        alignedReply = reply + 1;
        replyLen--;
    } else {
        alignedReply = reply;
    }

    int outSize;
    switch (padding) {
    case RSA_PKCS1_PADDING:
        outSize = RSA_padding_check_PKCS1_type_2(to, num, alignedReply, replyLen, num);
        break;
    case RSA_X931_PADDING:
        outSize = RSA_padding_check_X931(to, num, alignedReply, replyLen, num);
        break;
    case RSA_NO_PADDING:
        outSize = RSA_padding_check_none(to, num, alignedReply, replyLen, num);
        break;
    default:
        ALOGE("Unknown padding type: %d", padding);
        outSize = -1;
        break;
    }

    free(reply);

    ALOGV("rsa=%p keystore_rsa_priv_dec => returning %p len %d", rsa, to, outSize);
    return outSize;
}

static RSA_METHOD keystore_rsa_meth = {
        kKeystoreEngineId,
        NULL, /* rsa_pub_enc (wrap) */
        NULL, /* rsa_pub_dec (verification) */
        keystore_rsa_priv_enc, /* rsa_priv_enc (signing) */
        keystore_rsa_priv_dec, /* rsa_priv_dec (unwrap) */
        NULL, /* rsa_mod_exp */
        NULL, /* bn_mod_exp */
        NULL, /* init */
        NULL, /* finish */
        RSA_FLAG_EXT_PKEY | RSA_FLAG_NO_BLINDING, /* flags */
        NULL, /* app_data */
        NULL, /* rsa_sign */
        NULL, /* rsa_verify */
        NULL, /* rsa_keygen */
};

static int register_rsa_methods() {
    const RSA_METHOD* rsa_meth = RSA_PKCS1_SSLeay();

    keystore_rsa_meth.rsa_pub_enc = rsa_meth->rsa_pub_enc;
    keystore_rsa_meth.rsa_pub_dec = rsa_meth->rsa_pub_dec;
    keystore_rsa_meth.rsa_mod_exp = rsa_meth->rsa_mod_exp;
    keystore_rsa_meth.bn_mod_exp = rsa_meth->bn_mod_exp;

    return 1;
}

int rsa_pkey_setup(ENGINE *e, EVP_PKEY *pkey, const char *key_id) {
    Unique_RSA rsa(EVP_PKEY_get1_RSA(pkey));
    if (!RSA_set_ex_data(rsa.get(), rsa_key_handle, reinterpret_cast<void*>(strdup(key_id)))) {
        ALOGW("Could not set ex_data for loaded RSA key");
        return 0;
    }

    RSA_set_method(rsa.get(), &keystore_rsa_meth);
    RSA_blinding_off(rsa.get());

    /*
     * "RSA_set_ENGINE()" should probably be an OpenSSL API. Since it isn't,
     * and EVP_PKEY_free() calls ENGINE_finish(), we need to call ENGINE_init()
     * here.
     */
    ENGINE_init(e);
    rsa->engine = e;
    rsa->flags |= RSA_FLAG_EXT_PKEY;

    return 1;
}

int rsa_register(ENGINE* e) {
    if (!ENGINE_set_RSA(e, &keystore_rsa_meth)
            || !register_rsa_methods()) {
        ALOGE("Could not set up keystore RSA methods");
        return 0;
    }

    return 1;
}
