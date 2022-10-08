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

#include <cstdlib>
#include <getopt.h>
#include <cstring>
#include <iostream>

#include "TestSettings.h"

#include "TestForkerEventListener.h"

namespace android {
namespace camera2 {
namespace tests {

bool TestSettings::mForkingDisabled     = false;
int  TestSettings::mDeviceId            = 0;
char* const* TestSettings::mArgv;

// --forking-disabled, false by default
bool TestSettings::ForkingDisabled() {
    return mForkingDisabled;
}

// reverse of --forking-disabled (not a flag), true by default
bool TestSettings::ForkingEnabled() {
    return !ForkingDisabled();
}

// --device-id, 0 by default
int TestSettings::DeviceId() {
    return mDeviceId;
}

// returns false if usage should be printed and we should exit early
bool TestSettings::ParseArgs(int argc, char* const argv[])
{
    {
        char *env = getenv("CAMERA2_TEST_FORKING_DISABLED");
        if (env) {
            mForkingDisabled = atoi(env);
        }

        env = getenv("CAMERA2_TEST_DEVICE_ID");
        if (env) {
            mDeviceId = atoi(env);
        }
    }

    bool printHelp = false;
    bool unknownArgs = false;

    opterr = 0; // do not print errors for unknown arguments
    while (true) {
        int c;
        int option_index = 0;

        static struct option long_options[] = {
            /* name              has_arg          flag val */
            {"forking-disabled", optional_argument, 0,  0  },
            {"device-id",        required_argument, 0,  0  },
            {"help",             no_argument,       0, 'h' },
            {0,                  0,                 0,  0  }
        };

        // Note: '+' in optstring means do not mutate argv
        c = getopt_long(argc, argv, "+h", long_options, &option_index);

        if (c == -1) { // All arguments exhausted
            break;
        }
        if (c == '?') { // Argument not in option lists
            const char *arg = argv[optind-1];
            // Anything beginning with gtest_ will get handled by gtest
            if (strstr(arg, "--gtest_") != arg) {
                std::cerr << "Unknown argument: " << arg << std::endl;
                unknownArgs = true;
            }
            continue;
        }

        switch (c) {
        case 0: // long option
            switch (option_index) {
            case 0: {
                const char *arg = optarg ?: "1";
                mForkingDisabled = atoi(arg);
                break;
            }
            case 1: {
                mDeviceId = atoi(optarg);
                break;
            }
            default:
                std::cerr << "Unknown long option: " << option_index << std::endl;
                break;
            }
            break; // case 0
        case 'h': // help
            printHelp = true;
            break;
        default: // case '?'
            std::cerr << "Unknown option: " << optarg << std::endl;
        }
    }

    if (unknownArgs) {
        std::cerr << std::endl;
    }

    mArgv = argv;

    if (printHelp || unknownArgs) {
        return false;
    }

    std::cerr << "Forking Disabled: "
              << (mForkingDisabled ? "yes" : "no") << std::endl;

    std::cerr << "Device ID: " << mDeviceId << std::endl;

    return true;
}

// print usage/help list of commands (non-gtest)
void TestSettings::PrintUsage() {
    std::cerr << "Usage: " << mArgv[0] << " [OPTIONS]" << std::endl;
    std::cerr << std::endl;

    std::cerr << "Main modes of operation:"
              << std::endl;
    std::cerr << "   --forking-disabled[=1]  don't fork process before "
              << std::endl
              << "                           running a new test."
              << std::endl
              << "                           (default enabled)"
              << std::endl;
    std::cerr << "   --device-id=ID          specify a different camera ID"
              << std::endl
              << "                           (default 0)"
              << std::endl;

    std::cerr << "   -h, --help              print this help listing"
              << std::endl;


    std::cerr << std::endl;
}

}
}
}

