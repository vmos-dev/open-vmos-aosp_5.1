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
#ifndef INV_INCLUDE_H__
#define INV_INCLUDE_H__

#define INVENSENSE_FUNC_START  typedef int invensensePutFunctionCallsHere

#ifdef COVERAGE
#include "utestCommon.h"
#endif
#ifdef PROFILE
#include "profile.h"
#endif

#ifdef WIN32
#ifdef COVERAGE

extern int functionEnterLog(const char *file, const char *func);
extern int functionExitLog(const char *file, const char *func);

#undef INVENSENSE_FUNC_START
#define INVENSENSE_FUNC_START  __pragma(message(__FILE__ "|"__FUNCTION__ )) \
    int dslkQjDsd = functionEnterLog(__FILE__, __FUNCTION__)
#endif // COVERAGE
#endif // WIN32

#ifdef PROFILE
#undef INVENSENSE_FUNC_START
#define INVENSENSE_FUNC_START int dslkQjDsd = profileEnter(__FILE__, __FUNCTION__)
#define return if ( profileExit(__FILE__, __FUNCTION__) ) return
#endif // PROFILE

// #define return if ( functionExitLog(__FILE__, __FUNCTION__) ) return

#endif //INV_INCLUDE_H__
