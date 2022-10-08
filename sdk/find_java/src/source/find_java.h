/*
 * Copyright (C) 2011 The Android Open Source Project
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

#ifndef _H_FIND_JAVA
#define _H_FIND_JAVA

#ifdef _WIN32

#include "utils.h"

// We currently search for a Java version for at least 1.6
#define MIN_JAVA_VERSION_MAJOR 1
#define MIN_JAVA_VERSION_MINOR 6
#define MIN_JAVA_VERSION (MIN_JAVA_VERSION_MAJOR * 1000 + MIN_JAVA_VERSION_MINOR)


int findJavaInEnvPath(CPath *outJavaPath);
int findJavaInRegistry(CPath *outJavaPath);
int findJavaInProgramFiles(CPath *outJavaPath);
bool getJavaVersion(CPath &javaPath, CString *outVersionStr, int *outVersionInt);

#endif /* _WIN32 */
#endif /* _H_FIND_JAVA */
