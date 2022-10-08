/*
* Copyright (C) 2012 Invensense, Inc.
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

#ifndef ANDROID_MPL_SUPPORT_H
#define ANDROID_MPL_SUPPORT_H

#include <stdint.h>
#include <time.h>

int64_t getTimestamp();
int64_t timevalToNano(timeval const& t);

int inv_read_data(char *fname, long *data);
int read_attribute_sensor(int fd, char* data, unsigned int size);
int enable_sysfs_sensor(int fd, int en);
int write_attribute_sensor(int fd, long data);
int read_sysfs_int(char*, int*);
int write_sysfs_int(char*, int);

#endif //  ANDROID_MPL_SUPPORT_H
