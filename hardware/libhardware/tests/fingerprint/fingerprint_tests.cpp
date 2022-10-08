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

#include <gtest/gtest.h>
#include "fingerprint_test_fixtures.h"

namespace tests {

TEST_F(FingerprintDevice, isThereEnroll) {
    ASSERT_TRUE(NULL != fp_device()->enroll)
        << "enroll() function is not implemented";
}

TEST_F(FingerprintDevice, isThereRemove) {
    ASSERT_TRUE(NULL != fp_device()->remove)
        << "remove() function is not implemented";
}

TEST_F(FingerprintDevice, isThereSetNotify) {
    ASSERT_TRUE(NULL != fp_device()->set_notify)
        << "set_notify() function is not implemented";
}

}  // namespace tests
