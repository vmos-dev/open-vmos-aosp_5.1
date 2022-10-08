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
 * "find_lock.exe", for Windows only.
 */

#ifdef _WIN32

#include "utils.h"
#include "find_lock.h"
#include <io.h>
#include <fcntl.h>

int main(int argc, char* argv[]) {

    gIsConsole = true; // tell utils to to print errors to stderr
    gIsDebug = (getenv("ANDROID_SDKMAN_DEBUG") != NULL);
    CPath dirPath;
    bool doPrintUsage = false;

    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "-d", 2) == 0) {
            gIsDebug = true;

        } else if (dirPath.isEmpty()) {
            dirPath.set(argv[i]);

        } else {
            doPrintUsage = true;
        }
    }

    if (dirPath.isEmpty()) {
        fprintf(stderr, "Error: Missing directory path\n");
        doPrintUsage = true;

    } else if (!dirPath.dirExists()) {
        fprintf(stderr, "Error: '%s' is not a valid directory.\n", dirPath.cstr());
        return 1;
    }

    if (doPrintUsage) {
        printf(
            "Usage: find_lock.exe [options] sdk_directory_path\n"
            "\n"
            "Outputs the names of modules that are locking the given directory.\n"
            "Returns code 0 when found, 1 when not found.\n"
            "\n"
            "Options:\n"
            "-h / -help   : This help.\n"
            "-t / -test   : Internal test.\n"
            );
        return 2;
    }

    CString result;
    if (findLock(dirPath, &result)) {
        fflush(stdout);
        fflush(stderr);
        printf("%s", result.cstr());
        return 0;
    }
    return 1;
}

#endif /* _WIN32 */
