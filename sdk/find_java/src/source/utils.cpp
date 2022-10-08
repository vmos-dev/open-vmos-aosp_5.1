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

#ifdef _WIN32

#include "utils.h"

#define _CRT_SECURE_NO_WARNINGS 1

// Set to true to get some extra debug information
bool gIsDebug = false;
// Set to true to output errors to stderr (for a Console app)
// or to false to output using msg box (for a Windows UI app)
bool gIsConsole = false;

// Displays a message in an ok+info dialog box.
void msgBox(const char* text, ...) {
    CString formatted;
    va_list ap;
    va_start(ap, text);
    formatted.setv(text, ap);
    va_end(ap);

    MessageBoxA(NULL, formatted.cstr(), "Android SDK Manager", MB_OK | MB_ICONINFORMATION);
}

// Displays GetLastError prefixed with a description in an error dialog box
void displayLastError(const char *description, ...) {
    CString formatted;
    va_list ap;
    va_start(ap, description);
    formatted.setv(description, ap);
    va_end(ap);

    CString error;
    error.setLastWin32Error(NULL);
    formatted.add("\r\n");
    formatted.add(error.cstr());

    if (gIsConsole) {
        fprintf(stderr, "%s\n", formatted.cstr());
    } else {
        MessageBox(NULL, formatted.cstr(), "Android SDK Manager - Error", MB_OK | MB_ICONERROR);
    }
}

// Executes the command line. Does not wait for the program to finish.
// The return code is from CreateProcess (0 means failure), not the running app.
int execNoWait(const char *app, const char *params, const char *workDir) {
    STARTUPINFO           startup;
    PROCESS_INFORMATION   pinfo;

    ZeroMemory(&pinfo, sizeof(pinfo));

    ZeroMemory(&startup, sizeof(startup));
    startup.cb          = sizeof(startup);
    startup.dwFlags     = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWDEFAULT;

    int ret = CreateProcessA(
            (LPSTR) app,                                /* program path */
            (LPSTR) params,                             /* command-line */
            NULL,                  /* process handle is not inheritable */
            NULL,                   /* thread handle is not inheritable */
            TRUE,                          /* yes, inherit some handles */
            0,                                          /* create flags */
            NULL,                     /* use parent's environment block */
            workDir,                 /* use parent's starting directory */
            &startup,                 /* startup info, i.e. std handles */
            &pinfo);

    if (ret) {
        CloseHandle(pinfo.hProcess);
        CloseHandle(pinfo.hThread);
    }

    return ret;
}

// Executes command, waits for completion and returns exit code.
// As indicated in MSDN for CreateProcess, callers should double-quote the program name
// e.g. cmd="\"c:\program files\myapp.exe\" arg1 arg2";
int execWait(const char *cmd) {
    STARTUPINFO           startup;
    PROCESS_INFORMATION   pinfo;

    ZeroMemory(&pinfo, sizeof(pinfo));

    ZeroMemory(&startup, sizeof(startup));
    startup.cb          = sizeof(startup);
    startup.dwFlags     = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_HIDE|SW_MINIMIZE;

    int ret = CreateProcessA(
            NULL,                                       /* program path */
            (LPSTR) cmd,                                /* command-line */
            NULL,                  /* process handle is not inheritable */
            NULL,                   /* thread handle is not inheritable */
            TRUE,                          /* yes, inherit some handles */
            CREATE_NO_WINDOW,                /* we don't want a console */
            NULL,                     /* use parent's environment block */
            NULL,                    /* use parent's starting directory */
            &startup,                 /* startup info, i.e. std handles */
            &pinfo);

    int result = -1;
    if (ret) {
        WaitForSingleObject(pinfo.hProcess, INFINITE);

        DWORD exitCode;
        if (GetExitCodeProcess(pinfo.hProcess, &exitCode)) {
            // this should not return STILL_ACTIVE (259)
            result = exitCode;
        }
        CloseHandle(pinfo.hProcess);
        CloseHandle(pinfo.hThread);
    }

    return result;
}

bool getModuleDir(CPath *outDir) {
    CHAR programDir[MAX_PATH];
    int ret = GetModuleFileName(NULL, programDir, sizeof(programDir));
    if (ret != 0) {
        // Remove the last segment to keep only the directory.
        int pos = ret - 1;
        while (pos > 0 && programDir[pos] != '\\') {
            --pos;
        }
        outDir->set(programDir, pos);
        return true;
    }
    return false;
}

// Disables the FS redirection done by WOW64.
// Because this runs as a 32-bit app, Windows automagically remaps some
// folder under the hood (e.g. "Programs Files(x86)" is mapped as "Program Files").
// This prevents the app from correctly searching for java.exe in these folders.
// The registry is also remapped. This method disables this redirection.
// Caller should restore the redirection later by using revertWow64FsRedirection().
PVOID disableWow64FsRedirection() {

    // The call we want to make is the following:
    //    PVOID oldWow64Value;
    //    Wow64DisableWow64FsRedirection(&oldWow64Value);
    // However that method may not exist (e.g. on XP non-64 systems) so
    // we must not call it directly.

    PVOID oldWow64Value = 0;

    HMODULE hmod = LoadLibrary("kernel32.dll");
    if (hmod != NULL) {
        FARPROC proc = GetProcAddress(hmod, "Wow64DisableWow64FsRedirection");
        if (proc != NULL) {
            typedef BOOL (WINAPI *disableWow64FuncType)(PVOID *);
            disableWow64FuncType funcPtr = (disableWow64FuncType)proc;
            funcPtr(&oldWow64Value);
        }

        FreeLibrary(hmod);
    }

    return oldWow64Value;
}

// Reverts the redirection disabled in disableWow64FsRedirection.
void revertWow64FsRedirection(PVOID oldWow64Value) {

    // The call we want to make is the following:
    //    Wow64RevertWow64FsRedirection(oldWow64Value);
    // However that method may not exist (e.g. on XP non-64 systems) so
    // we must not call it directly.

    HMODULE hmod = LoadLibrary("kernel32.dll");
    if (hmod != NULL) {
        FARPROC proc = GetProcAddress(hmod, "Wow64RevertWow64FsRedirection");
        if (proc != NULL) {
            typedef BOOL (WINAPI *revertWow64FuncType)(PVOID);
            revertWow64FuncType funcPtr = (revertWow64FuncType)proc;
            funcPtr(oldWow64Value);
        }

        FreeLibrary(hmod);
    }
}

#endif /* _WIN32 */
