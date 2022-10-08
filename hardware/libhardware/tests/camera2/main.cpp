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
#include "TestForkerEventListener.h"
#include "TestSettings.h"

using android::camera2::tests::TestForkerEventListener;
using android::camera2::tests::TestSettings;

int main(int argc, char **argv) {

    bool printUsage = !TestSettings::ParseArgs(argc, argv);

    ::testing::InitGoogleTest(&argc, argv);

    if (printUsage) {
        TestSettings::PrintUsage();
        return 0;
    }

    // Gets hold of the event listener list.
    ::testing::TestEventListeners& listeners =
        ::testing::UnitTest::GetInstance()->listeners();
    // Adds a listener to the end.  Google Test takes the ownership.
    listeners.Append(new TestForkerEventListener());

    int ret = RUN_ALL_TESTS();

    return ret;
}
