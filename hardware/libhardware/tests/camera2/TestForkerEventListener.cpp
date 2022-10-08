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

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

#include <gtest/gtest.h>

#include "TestForkerEventListener.h"
#include "TestExtensions.h"

#define DEBUG_TEST_FORKER_EVENT_LISTENER 0

#define RETURN_CODE_PASSED 0
#define RETURN_CODE_FAILED 1

namespace android {
namespace camera2 {
namespace tests {

bool TestForkerEventListener::mIsForked         = false;

TestForkerEventListener::TestForkerEventListener() {
    mIsForked = false;
    mHasSucceeded = true;
    mTermSignal = 0;
}

// Called before a test starts.
void TestForkerEventListener::OnTestStart(const ::testing::TestInfo&) {

    if (!TEST_EXTENSION_FORKING_ENABLED) {
        return;
    }

    pid_t childPid = fork();
    if (childPid != 0) {
        int status;
        waitpid(childPid, &status, /*options*/0);

        // terminated normally?
        mHasSucceeded = WIFEXITED(status);
        // terminate with return code 0 = test passed, 1 = test failed
        if (mHasSucceeded) {
          mHasSucceeded = WEXITSTATUS(status) == RETURN_CODE_PASSED;
        } else if (WIFSIGNALED(status)) {
          mTermSignal = WTERMSIG(status);
        }

        /* the test is then skipped by inserting the various
        TEST_EXTENSION_ macros in TestExtensions.h */

    } else {
        mIsForked = true;
    }
}

// Called after a failed assertion or a SUCCEED() invocation.
void TestForkerEventListener::OnTestPartResult(
    const ::testing::TestPartResult& test_part_result) {

    if (DEBUG_TEST_FORKER_EVENT_LISTENER) {
        printf("%s in %s:%d\n%s\n",
             test_part_result.failed() ? "*** Failure" : "Success",
             test_part_result.file_name(),
             test_part_result.line_number(),
             test_part_result.summary());
    }
}

// Called after a test ends.
void TestForkerEventListener::OnTestEnd(const ::testing::TestInfo& test_info) {

    if (!TEST_EXTENSION_FORKING_ENABLED) {
        return;
    }

    if (mIsForked) {
        exit(test_info.result()->Passed()
            ? RETURN_CODE_PASSED : RETURN_CODE_FAILED);
    } else if (!mHasSucceeded && mTermSignal != 0) {

      printf("*** Test %s.%s crashed with signal = %s\n",
             test_info.test_case_name(), test_info.name(),
             strsignal(mTermSignal));
    }

    //TODO: overload the default event listener to suppress this message
    // dynamically (e.g. by skipping OnTestPartResult after OnTestEnd )

    // trigger a test failure if the child has failed
    if (!mHasSucceeded) {
        ADD_FAILURE();
    }
    mTermSignal = 0;
}


}
}
}

