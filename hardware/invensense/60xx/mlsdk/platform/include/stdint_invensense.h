/*
 $License:
   Copyright 2011 InvenSense, Inc.

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
  $
 */
#ifndef STDINT_INVENSENSE_H
#define STDINT_INVENSENSE_H

#ifndef WIN32

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
#endif

#else

#include <windows.h>

typedef char int8_t;
typedef short int16_t;
typedef long int32_t;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;

typedef int int_fast8_t;
typedef int int_fast16_t;
typedef long int_fast32_t;

typedef unsigned int uint_fast8_t;
typedef unsigned int uint_fast16_t;
typedef unsigned long uint_fast32_t;

#endif

#endif // STDINT_INVENSENSE_H
