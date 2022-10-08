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

#ifndef _DEVMAPPER_H
#define _DEVMAPPER_H

#include <unistd.h>
#include <linux/dm-ioctl.h>

class SocketClient;

class Devmapper {
public:
    static int create(const char *name, const char *loopFile, const char *key,
                      unsigned int numSectors, char *buffer, size_t len);
    static int destroy(const char *name);
    static int lookupActive(const char *name, char *buffer, size_t len);
    static int dumpState(SocketClient *c);

private:
    static void *_align(void *ptr, unsigned int a);
    static void ioctlInit(struct dm_ioctl *io, size_t data_size,
                          const char *name, unsigned flags);
};

#endif
