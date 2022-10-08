/*
 * Copyright (C) 2010 The Android Open Source Project
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

#define LOG_TAG "VolumeManager_test"
#include <utils/Log.h>
#include <openssl/md5.h>
#include "../VolumeManager.h"

#include <gtest/gtest.h>

namespace android {

class VolumeManagerTest : public testing::Test {
protected:
    virtual void SetUp() {
    }

    virtual void TearDown() {
    }
};

TEST_F(VolumeManagerTest, AsecHashTests) {
    char buffer[MD5_ASCII_LENGTH_PLUS_NULL];
    char* dst = reinterpret_cast<char*>(&buffer);

    const char* src1 = "";
    const char* exp1 = "d41d8cd98f00b204e9800998ecf8427e";

    EXPECT_TRUE(VolumeManager::asecHash(exp1, (char*)NULL, sizeof(buffer)) == NULL && errno == ESPIPE)
            << "Should return NULL and set errno to ESPIPE when destination buffer is NULL";
    EXPECT_TRUE(VolumeManager::asecHash(exp1, dst, 0) == NULL && errno == ESPIPE)
            << "Should return NULL and set errno to ESPIPE when destination buffer length is 0";
    EXPECT_TRUE(VolumeManager::asecHash((const char*)NULL, dst, sizeof(buffer)) == NULL && errno == ESPIPE)
            << "Should return NULL and set errno to ESPIPE when source buffer is NULL";

    EXPECT_FALSE(VolumeManager::asecHash(src1, dst, sizeof(buffer)) == NULL)
            << "Should not return NULL on valid source, destination, and destination size";
    EXPECT_STREQ(exp1, dst)
            << "MD5 summed output should match";

    const char* src2 = "android";
    const char* exp2 = "c31b32364ce19ca8fcd150a417ecce58";
    EXPECT_FALSE(VolumeManager::asecHash(src2, dst, sizeof(buffer)) == NULL)
            << "Should not return NULL on valid source, destination, and destination size";
    EXPECT_STREQ(exp2, dst)
            << "MD5 summed output should match";
}

}
