/*
 * Copyright (C) 2009 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef ANDROID_NATIVETEST_SYSTEM_EXTRAS_TESTS_SDCARD_SYSUTIL_H_
#define ANDROID_NATIVETEST_SYSTEM_EXTRAS_TESTS_SDCARD_SYSUTIL_H_

#include <stdlib.h>
namespace android {

// Collection of functions to access the system:
// .kernelVersion         Retrieve the full kernel description.
// .pidOutOfMemoryAdj     Get and set the OOM adj value.
// .setPidOutOfMemoryAdj
// .schedFeatures         Manipulate the scheduler using debugfs.
// .newFairSleepers
// .setNewFairSleepers
// .disableCpuScaling     Set cpu scaling to 'performance'.
// .forkOrExit            Fork a child or exit.
// .syncAnddropCaches     Call sync an drop page/dentries/inodes caches.
// .fsyncAnddropCaches    Call fsync an drop page/dentries/inodes caches.
// .resetDirectory        Delete (non-recursive) files in a directory.
//
// IPC function to synchonize a processes with their parent.
// .writePidAndWaitForReply To instruct the parent the child is ready.
//                          Blocks until the parent signals back.
// .waitForChildrenAndSignal Blocks until all the children have called
//                           writePidAndWaitForReply.
//                           Then unblock all the children.
// .waitForChildrenOrExit Wait and exit if a child exit with errors.
//

// Minimum size for the buffer to retrieve the kernel version.
static const size_t kMinKernelVersionBufferSize = 256;

// @param str points to the buffer where the kernel version should be
//            added. Must be at least kMinKernelVersionBufferSize chars.
// @param size of the buffer pointed by str.
// @return If successful the number of characters inserted in the
//         buffer (not including the trailing '\0' byte). -1 otherwise.
int kernelVersion(char *str, size_t size);


// Return the out of memory adj for this process. /proc/<pid>/oom_adj.
// @return the oom_adj of the current process. Typically:
//           0: a regular process. Should die on OOM.
//         -16: system_server level.
//         -17: disable, this process  will never be killed.
//        -127: error.
int pidOutOfMemoryAdj();
void setPidOutOfMemoryAdj(int level);

// Disable cpu scaling.
void disableCpuScaling();


// Minimum size for the buffer to retrieve the sched features.
static const size_t kMinSchedFeaturesBufferSize = 256;

// @param str points to the buffer where the sched features should be
//            added. Must be at least kMinSchedFeaturesBufferSize chars.
// @param size of the buffer pointed by str.
// @return If successful the number of characters inserted in the
//         buffer (not including the trailing '\0' byte). -1 otherwise.
int schedFeatures(char *str, size_t size);

// @return true if NEW_FAIR_SLEEPERS is set, false if NO_NEW_FAIR_SLEEPERS is set.
bool newFairSleepers();

// Turns NEW_FAIR_SLEEPERS on or off.
void setNewFairSleepers(bool on);

// @return true if NORMALIZED_SLEEPERS is set, false if NO_NORMALIZED_SLEEPERS is set.
bool normalizedSleepers();

// Turns NORMALIZED_SLEEPERS on or off.
void setNormalizedSleepers(bool on);

// Filesystem

// Sync and drop caches. Sync is needed because dirty objects are not
// freable.
// @param code:
//        * 1 To free pagecache.
//        * 2 To free dentries and inodes.
//        * 3 To free pagecache, dentries and inodes.
void syncAndDropCaches(int code = 3);

// Fsync the given fd and drop caches. Fsync is needed because dirty
// objects are not freable.
// @param code:
//        * 1 To free pagecache.
//        * 2 To free dentries and inodes.
//        * 3 To free pagecache, dentries and inodes.
void fsyncAndDropCaches(int fd, int code = 3);

// Delete all the files in the given directory.  If the directory does
// not exist, it is created.  Use this at the beginning of a test to
// make sure you have a clean existing directory, previous run may
// have crashed and left clutter around.
void resetDirectory(const char *directory);

// IPC

// Try to fork. exit on failure.
pid_t forkOrExit();

// Signal our parent we are alive and ready by sending our pid.
// Then do a blocking read for parent's reply.
bool writePidAndWaitForReply(int writefd, int readfd);

// Wait for all the children to report their pids.
// Then unblock them.
bool waitForChildrenAndSignal(int mProcessNb, int readfd, int writefd);

// Wait for 'num' children to complete.
// If a child did not exit cleanly, exit.
void waitForChildrenOrExit(int num);

}  // namespace android

#endif  // ANDROID_NATIVETEST_SYSTEM_EXTRAS_TESTS_SDCARD_SYSUTIL_H_
