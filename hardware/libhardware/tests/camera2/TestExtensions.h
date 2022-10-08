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

#ifndef __ANDROID_HAL_CAMERA2_TESTS_EXTENSIONS__
#define __ANDROID_HAL_CAMERA2_TESTS_EXTENSIONS__

#include "TestForkerEventListener.h"
#include "TestSettings.h"

// Use at the beginning of each Test::SetUp() impl
#define TEST_EXTENSION_FORKING_SET_UP                                       \
    do {                                                                    \
        if (TEST_EXTENSION_FORKING_ENABLED) {                               \
            if (!TestForkerEventListener::mIsForked) {                      \
                return;                                                     \
            }                                                               \
        }                                                                   \
    } while (false)                                                         \

// Use at the beginning of each Test::TearDown() impl
#define TEST_EXTENSION_FORKING_TEAR_DOWN   TEST_EXTENSION_FORKING_SET_UP

// Use at the beginning of each Test::Test constructor
#define TEST_EXTENSION_FORKING_CONSTRUCTOR TEST_EXTENSION_FORKING_SET_UP

// Use at the beginning of each Test::~Test destructor
#define TEST_EXTENSION_FORKING_DESTRUCTOR  TEST_EXTENSION_FORKING_TEAR_DOWN

// Use at the beginning of each test body, e.g. TEST(x,y), TEST_F(x,y), etc
#define TEST_EXTENSION_FORKING_INIT                                         \
    do {                                                                    \
        TEST_EXTENSION_FORKING_SET_UP;                                      \
        if (HasFatalFailure()) return;                                      \
    } while(false)                                                          \

// Are we running each test by forking it?
#define TEST_EXTENSION_FORKING_ENABLED                                      \
    (android::camera2::tests::TestSettings::ForkingEnabled())



#endif

