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

#include <keystore/IKeystoreService.h>
#include <binder/IServiceManager.h>

#include <keystore/keystore_get.h>

using namespace android;

ssize_t keystore_get(const char *key, size_t keyLength, uint8_t** value) {
    sp<IServiceManager> sm = defaultServiceManager();
    sp<IBinder> binder = sm->getService(String16("android.security.keystore"));
    sp<IKeystoreService> service = interface_cast<IKeystoreService>(binder);

    if (service == NULL) {
        return -1;
    }

    size_t valueLength;
    int32_t ret = service->get(String16(key, keyLength), value, &valueLength);
    if (ret < 0) {
        return -1;
    } else if (ret != ::NO_ERROR) {
        return -1;
    } else {
        return valueLength;
    }
}
