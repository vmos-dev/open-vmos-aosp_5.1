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

/*
 * "find_java.exe", for Windows only.
 * Tries to find a Java binary in a variety of places and prints the
 * first one found on STDOUT and returns 0.
 *
 * If not found, returns error 1 with no message
 * (unless ANDROID_SDKMAN_DEBUG or -d if set, in which case there's a message on STDERR).
 *
 * Implementation details:
 * - We don't have access to ATL or MFC.
 * - We don't want to pull in things like STL.
 * - No Unicode/MBCS support for now.
 *
 * TODO for later version:
 * - provide an env variable to let users override which version is being used.
 * - if there's more than one java.exe found, enumerate them all.
 * - and in that case take the one with the highest Java version number.
 * - since that operation is expensive, do it only once and cache the result
 *   in a temp file. If the temp file is not found or the java binary no
 *   longer exists, re-run the enumaration.
 */

#ifdef _WIN32

#include "utils.h"
#include "find_java.h"
#include <io.h>
#include <fcntl.h>

static void testFindJava() {

    CPath javaPath("<not found>");
    int v = findJavaInEnvPath(&javaPath);
    printf("findJavaInEnvPath: [%d] %s\n", v, javaPath.cstr());

    javaPath.set("<not found>");
    v = findJavaInRegistry(&javaPath);
    printf("findJavaInRegistry [%d] %s\n", v, javaPath.cstr());

    javaPath.set("<not found>");
    v = findJavaInProgramFiles(&javaPath);
    printf("findJavaInProgramFiles [%d] %s\n", v, javaPath.cstr());
}


int main(int argc, char* argv[]) {

    gIsConsole = true; // tell utils to to print errors to stderr
    gIsDebug = (getenv("ANDROID_SDKMAN_DEBUG") != NULL);
    bool doShortPath = false;
    bool doVersion = false;
    bool doJavaW = false;

    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "-t", 2) == 0) {
            testFindJava();
            return 0;

        } else if (strncmp(argv[i], "-d", 2) == 0) {
            gIsDebug = true;

        } else if (strncmp(argv[i], "-s", 2) == 0) {
            doShortPath = true;

        } else if (strncmp(argv[i], "-v", 2) == 0) {
            doVersion = true;

        } else if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "-javaw") == 0) {
            doJavaW = true;

        } else {
            printf(
                "Outputs the path of the first Java.exe found on the local system.\n"
                "Returns code 0 when found, 1 when not found.\n"
                "Options:\n"
                "-h / -help   : This help.\n"
                "-t / -test   : Internal test.\n"
                "-s / -short  : Print path in short DOS form.\n"
                "-w / -javaw  : Search a matching javaw.exe; defaults to java.exe if not found.\n"
                "-v / -version: Only prints the Java version found.\n"
                );
            return 2;
        }
    }

    // Find the first suitable version of Java we can use.
    CPath javaPath;
    int version = findJavaInEnvPath(&javaPath);
    if (version < MIN_JAVA_VERSION) {
        version = findJavaInRegistry(&javaPath);
    }
    if (version < MIN_JAVA_VERSION) {
        version = findJavaInProgramFiles(&javaPath);
    }
    if (version < MIN_JAVA_VERSION || javaPath.isEmpty()) {
        if (gIsDebug) {
            fprintf(stderr, "Failed to find Java on your system.\n");
        }
        return 1;
    }
    _ASSERT(!javaPath.isEmpty());

    if (doShortPath) {
        PVOID oldWow64Value = disableWow64FsRedirection();
        if (!javaPath.toShortPath(&javaPath)) {
            revertWow64FsRedirection(&oldWow64Value);
            fprintf(stderr,
                "Failed to convert path to a short DOS path: %s\n",
                javaPath.cstr());
            return 1;
        }
        revertWow64FsRedirection(&oldWow64Value);
    }

    if (doVersion) {
        // Print version found. We already have the version as an integer
        // so we don't need to run java -version a second time.
        printf("%d.%d", version / 1000, version % 1000);
        return 0;
    }

    if (doJavaW) {
        // Try to find a javaw.exe instead of java.exe at the same location.
        CPath javawPath(javaPath);
        javawPath.replaceName("java.exe", "javaw.exe");
        // Only accept it if we can actually find the exec
        PVOID oldWow64Value = disableWow64FsRedirection();
        if (javawPath.fileExists()) {
            javaPath.set(javawPath.cstr());
        }
        revertWow64FsRedirection(&oldWow64Value);
    }

    // Print java.exe path found
    printf("%s", javaPath.cstr());
    return 0;
}

#endif /* _WIN32 */
