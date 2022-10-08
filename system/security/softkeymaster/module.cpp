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
#include <errno.h>
#include <string.h>
#include <stdint.h>

#include <keymaster/softkeymaster.h>

#include <keystore/keystore.h>

#include <hardware/hardware.h>
#include <hardware/keymaster.h>

#include <openssl/err.h>

#include <UniquePtr.h>

// For debugging
// #define LOG_NDEBUG 0

#define LOG_TAG "OpenSSLKeyMaster"
#include <cutils/log.h>

typedef UniquePtr<keymaster_device_t> Unique_keymaster_device_t;

/* Close an opened OpenSSL instance */
static int openssl_close(hw_device_t* dev) {
    delete dev;
    return 0;
}

/*
 * Generic device handling
 */
static int openssl_open(const hw_module_t* module, const char* name, hw_device_t** device) {
    if (strcmp(name, KEYSTORE_KEYMASTER) != 0)
        return -EINVAL;

    Unique_keymaster_device_t dev(new keymaster_device_t);
    if (dev.get() == NULL)
        return -ENOMEM;

    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 1;
    dev->common.module = (struct hw_module_t*)module;
    dev->common.close = openssl_close;

    dev->flags = KEYMASTER_SOFTWARE_ONLY | KEYMASTER_BLOBS_ARE_STANDALONE;

    dev->generate_keypair = openssl_generate_keypair;
    dev->import_keypair = openssl_import_keypair;
    dev->get_keypair_public = openssl_get_keypair_public;
    dev->delete_keypair = NULL;
    dev->delete_all = NULL;
    dev->sign_data = openssl_sign_data;
    dev->verify_data = openssl_verify_data;

    ERR_load_crypto_strings();
    ERR_load_BIO_strings();

    *device = reinterpret_cast<hw_device_t*>(dev.release());

    return 0;
}

static struct hw_module_methods_t keystore_module_methods = {
    .open = openssl_open,
};

struct keystore_module HAL_MODULE_INFO_SYM __attribute__((visibility("default"))) = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = KEYMASTER_MODULE_API_VERSION_0_2,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = KEYSTORE_HARDWARE_MODULE_ID,
        .name = "Keymaster OpenSSL HAL",
        .author = "The Android Open Source Project",
        .methods = &keystore_module_methods,
        .dso = 0,
        .reserved = {},
    },
};
