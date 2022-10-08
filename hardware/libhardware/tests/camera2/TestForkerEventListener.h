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

#ifndef __ANDROID_HAL_CAMERA2_TESTS_FORKER_EVENT_LISTENER__
#define __ANDROID_HAL_CAMERA2_TESTS_FORKER_EVENT_LISTENER__

#include <gtest/gtest.h>

namespace android {
namespace camera2 {
namespace tests {

// Fork before each test runs.
class TestForkerEventListener : public ::testing::EmptyTestEventListener {

public:

    TestForkerEventListener();

private:

    // Called before a test starts.
    virtual void OnTestStart(const ::testing::TestInfo& test_info);

    // Called after a failed assertion or a SUCCEED() invocation.
    virtual void OnTestPartResult(
        const ::testing::TestPartResult& test_part_result);

    // Called after a test ends.
    virtual void OnTestEnd(const ::testing::TestInfo& test_info);

    bool mHasSucceeded;
    int mTermSignal;

public:
    // do not read directly. use TEST_EXTENSION macros instead
    static bool mIsForked;
};

}
}
}

#endif
