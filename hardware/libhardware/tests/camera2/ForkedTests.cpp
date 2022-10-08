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

#include <gtest/gtest.h>

#include "TestExtensions.h"

namespace android {
namespace camera2 {
namespace tests {

// Intentionally disabled since 2 of these tests are supposed to fail
class DISABLED_ForkedTest : public ::testing::Test {

    virtual void SetUp() {
        TEST_EXTENSION_FORKING_SET_UP;
    }

    virtual void TearDown() {
        TEST_EXTENSION_FORKING_TEAR_DOWN;
    }
};

// intentionally fail
TEST_F(DISABLED_ForkedTest, FailCrash) {
    TEST_EXTENSION_FORKING_INIT;

    //intentionally crash
    *(int*)0 = 0xDEADBEEF;
}

TEST_F(DISABLED_ForkedTest, SucceedNormal) {
    TEST_EXTENSION_FORKING_INIT;

    EXPECT_TRUE(true);
}

// intentionally fail
TEST_F(DISABLED_ForkedTest, FailNormal) {
    TEST_EXTENSION_FORKING_INIT;

    EXPECT_TRUE(false);
}

}
}
}

