/*
 * Copyright (C) 2008 The Android Open Source Project
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

#ifndef _LOOP_H
#define _LOOP_H

#include <unistd.h>
#include <linux/loop.h>

class SocketClient;

class Loop {
public:
    static const int LOOP_MAX = 4096;
public:
    static int lookupActive(const char *id, char *buffer, size_t len);
    static int lookupInfo(const char *loopDevice, struct asec_superblock *sb, unsigned int *nr_sec);
    static int create(const char *id, const char *loopFile, char *loopDeviceBuffer, size_t len);
    static int destroyByDevice(const char *loopDevice);
    static int destroyByFile(const char *loopFile);
    static int createImageFile(const char *file, unsigned int numSectors);
    static int resizeImageFile(const char *file, unsigned int numSectors);

    static int dumpState(SocketClient *c);
};

#endif
