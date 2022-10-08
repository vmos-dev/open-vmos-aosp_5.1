/*
:qa
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

#ifndef __ANDROID_HAL_CAMERA2_TESTS_SETTINGS__
#define __ANDROID_HAL_CAMERA2_TESTS_SETTINGS__

namespace android {
namespace camera2 {
namespace tests {

class TestSettings {

public:
    // --forking-disabled, false by default
    static bool ForkingDisabled();

    // reverse of --forking-disabled (not a flag), true by default
    static bool ForkingEnabled();

    // --device-id, 0 by default
    static int DeviceId();

    // returns false if usage should be printed and we should exit early
    static bool ParseArgs(int argc, char* const argv[]);

    // print usage/help list of commands (non-gtest)
    static void PrintUsage();

private:
    TestSettings();
    ~TestSettings();

    static bool mForkingDisabled;
    static int  mDeviceId;
    static char* const* mArgv;
};

}
}
}

#endif
