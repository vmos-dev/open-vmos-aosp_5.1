/*
 * Copyright (C) 2009 The Android Open Source Project
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

#ifndef __KEYSTORE_GET_H__
#define __KEYSTORE_GET_H__

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* This function is provided for native components to get values from keystore.
 * Users are required to link against libkeystore_binder.
 *
 * Keys and values are 8-bit safe. The first two arguments are the key and its
 * length. The third argument is a pointer to an array that will be malloc()
 * and the caller is responsible for calling free() on the buffer.
 */
ssize_t keystore_get(const char *key, size_t length, uint8_t** value);

#ifdef __cplusplus
}
#endif

#endif
