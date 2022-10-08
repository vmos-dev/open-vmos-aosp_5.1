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

/*
 * The "AVD Manager" is for Windows only.
 * This simple .exe will sit at the root of the Windows SDK
 * and currently simply executes tools\android.bat.
 *
 * TODO: replace by a jar-exe wrapper.
 */

#ifdef _WIN32

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <windows.h>


int _enable_dprintf = 0;

void dprintf(char *msg, ...) {
    va_list ap;
    va_start(ap, msg);

    if (_enable_dprintf) {
        vfprintf(stderr, msg, ap);
    }

    va_end(ap);
}

void display_error(LPSTR description) {
    DWORD err = GetLastError();
    LPSTR s, s2;

    fprintf(stderr, "%s, error %ld\n", description, err);

    if (FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | /* dwFlags */
                      FORMAT_MESSAGE_FROM_SYSTEM,
                      NULL,                             /* lpSource */
                      err,                              /* dwMessageId */
                      0,                                /* dwLanguageId */
                      (LPSTR)&s,                        /* lpBuffer */
                      0,                                /* nSize */
                      NULL) != 0) {                     /* va_list args */
        fprintf(stderr, "%s", s);

        s2 = (LPSTR) malloc(strlen(description) + strlen(s) + 5);
        sprintf(s2, "%s\r\n%s", description, s);
        MessageBox(NULL, s2, "Android AVD Manager - Error", MB_OK);
        free(s2);
        LocalFree(s);
    }
}


int avd_launcher() {
    int                   result = 0;
    STARTUPINFO           startup;
    PROCESS_INFORMATION   pinfo;
    CHAR                  program_dir[MAX_PATH];
    int                   ret, pos;

    ZeroMemory(&pinfo, sizeof(pinfo));

    ZeroMemory(&startup, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags     = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_HIDE|SW_MINIMIZE;

    /* get path of current program, to switch dirs here when executing the command. */
    ret = GetModuleFileName(NULL, program_dir, sizeof(program_dir));
    if (ret == 0) {
        display_error("Failed to get program's filename:");
        result = 1;
    } else {
        /* Remove the last segment to keep only the directory. */
        pos = ret - 1;
        while (pos > 0 && program_dir[pos] != '\\') {
            --pos;
        }
        program_dir[pos] = 0;
    }

    if (!result) {
        dprintf("Program dir: %s\n", program_dir);

        ret = CreateProcess(
                NULL,                                       /* program path */
                "tools\\android.bat avd",                   /* command-line */
                NULL,                  /* process handle is not inheritable */
                NULL,                   /* thread handle is not inheritable */
                TRUE,                          /* yes, inherit some handles */
                CREATE_NO_WINDOW,                /* we don't want a console */
                NULL,                     /* use parent's environment block */
                program_dir,             /* use parent's starting directory */
                &startup,                 /* startup info, i.e. std handles */
                &pinfo);

        dprintf("CreateProcess returned %d\n", ret);

        if (!ret) {
            display_error("Failed to execute tools\\android.bat:");
            result = 1;
        }
    }

    dprintf("Cleanup.\n");

    return result;
}

int main(int argc, char **argv) {
    _enable_dprintf = argc > 1 && strcmp(argv[1], "-v") == 0;
    dprintf("Verbose debug mode.\n");

    return avd_launcher();
}

#endif /* _WIN32 */
