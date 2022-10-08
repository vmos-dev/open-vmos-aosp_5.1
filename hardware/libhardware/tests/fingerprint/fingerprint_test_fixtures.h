/*
 * Copyright (C) 2014 The Android Open Source Project
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

#ifndef __ANDROID_HAL_FINGERPRINT_TEST_COMMON__
#define __ANDROID_HAL_FINGERPRINT_TEST_COMMON__

#include <gtest/gtest.h>
#include <hardware/hardware.h>
#include <hardware/fingerprint.h>

namespace tests {

static const uint16_t kVersion = HARDWARE_MODULE_API_VERSION(1, 0);

class FingerprintModule : public testing::Test {
 public:
    FingerprintModule() :
        fp_module_(NULL) {}
    ~FingerprintModule() {}
 protected:
    virtual void SetUp() {
        const hw_module_t *hw_module = NULL;
        ASSERT_EQ(0, hw_get_module(FINGERPRINT_HARDWARE_MODULE_ID, &hw_module))
                    << "Can't get fingerprint module";
        ASSERT_TRUE(NULL != hw_module)
                    << "hw_get_module didn't return a valid fingerprint module";

        fp_module_ = reinterpret_cast<const fingerprint_module_t*>(hw_module);
    }
    const fingerprint_module_t* fp_module() { return fp_module_; }
 private:
    const fingerprint_module_t *fp_module_;
};

class FingerprintDevice : public FingerprintModule {
 public:
    FingerprintDevice() :
        fp_device_(NULL) {}
    ~FingerprintDevice() {}
 protected:
    virtual void SetUp() {
        FingerprintModule::SetUp();
        hw_device_t *device = NULL;
        ASSERT_TRUE(NULL != fp_module()->common.methods->open)
                    << "Fingerprint open() is unimplemented";
        ASSERT_EQ(0, fp_module()->common.methods->open(
            (const hw_module_t*)fp_module(), NULL, &device))
                << "Can't open fingerprint device";
        ASSERT_TRUE(NULL != device)
                    << "Fingerprint open() returned a NULL device";
        ASSERT_EQ(kVersion, device->version)
                    << "Unsupported version";
        fp_device_ = reinterpret_cast<fingerprint_device_t*>(device);
    }
    fingerprint_device_t* fp_device() { return fp_device_; }
 private:
    fingerprint_device_t *fp_device_;
};

}  // namespace tests

#endif  // __ANDROID_HAL_FINGERPRINT_TEST_COMMON__
