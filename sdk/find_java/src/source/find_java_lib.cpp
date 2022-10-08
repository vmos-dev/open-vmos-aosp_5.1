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

// Indicate we want at least all Windows Server 2003 (5.2) APIs.
// Note: default is set by system/core/include/arch/windows/AndroidConfig.h to 0x0500
// which is Win2K. However our minimum SDK tools requirement is Win XP (0x0501).
// However we do need 0x0502 to get access to the WOW-64-32 constants for the
// registry, except we'll need to be careful since they are not available on XP.
#undef  _WIN32_WINNT
#define _WIN32_WINNT 0x0502
// Indicate we want at least all IE 5 shell APIs
#define _WIN32_IE    0x0500

#include "find_java.h"
#include <shlobj.h>
#include <ctype.h>

// Define some types missing in MingW
#ifndef LSTATUS
typedef LONG LSTATUS;
#endif


// Extract the first thing that looks like (digit.digit+).
// Note: this will break when java reports a version with major > 9.
// However it will reasonably cope with "1.10", if that ever happens.
static bool extractJavaVersion(const char *start,
                               int length,
                               CString *outVersionStr,
                               int *outVersionInt) {
    const char *end = start + length;
    for (const char *c = start; c < end - 2; c++) {
        if (isdigit(c[0]) &&
                c[1] == '.' &&
                isdigit(c[2])) {
            const char *e = c+2;
            while (isdigit(e[1])) {
                e++;
            }
            if (outVersionStr != NULL) {
                outVersionStr->set(c, e - c + 1);
            }
            if (outVersionInt != NULL) {
                // add major * 1000, currently only 1 digit
                int value = (*c - '0') * 1000;
                // add minor
                for (int m = 1; *e != '.'; e--, m *= 10) {
                    value += (*e - '0') * m;
                }
                *outVersionInt = value;
            }
            return true;
        }
    }
    return false;
}

// Check whether we can find $PATH/java.exe.
// inOutPath should be the directory where we're looking at.
// In output, it will be the java path we tested.
// Returns the java version integer found (e.g. 1006 for 1.6).
// Return 0 in case of error.
static int checkPath(CPath *inOutPath) {
    inOutPath->addPath("java.exe");

    int result = 0;
    PVOID oldWow64Value = disableWow64FsRedirection();
    if (inOutPath->fileExists()) {
        // Run java -version
        // Reject the version if it's not at least our current minimum.
        if (!getJavaVersion(*inOutPath, NULL /*versionStr*/, &result)) {
            result = 0;
        }
    }

    revertWow64FsRedirection(oldWow64Value);
    return result;
}

// Check whether we can find $PATH/bin/java.exe
// Returns the Java version found (e.g. 1006 for 1.6) or 0 in case of error.
static int checkBinPath(CPath *inOutPath) {
    inOutPath->addPath("bin");
    return checkPath(inOutPath);
}

// Search java.exe in the environment
int findJavaInEnvPath(CPath *outJavaPath) {
    SetLastError(0);

    int currVersion = 0;

    const char* envPath = getenv("JAVA_HOME");
    if (envPath != NULL) {
        CPath p(envPath);
        currVersion = checkBinPath(&p);
        if (currVersion > 0) {
            if (gIsDebug) {
                fprintf(stderr, "Java %d found via JAVA_HOME: %s\n", currVersion, p.cstr());
            }
            *outJavaPath = p;
        }
        if (currVersion >= MIN_JAVA_VERSION) {
            // As an optimization for runtime, if we find a suitable java
            // version in JAVA_HOME we won't waste time looking at the PATH.
            return currVersion;
        }
    }

    envPath = getenv("PATH");
    if (!envPath) return currVersion;

    // Otherwise look at the entries in the current path.
    // If we find more than one, keep the one with the highest version.

    CArray<CString> *paths = CString(envPath).split(';');
    for(int i = 0; i < paths->size(); i++) {
        CPath p((*paths)[i].cstr());
        int v = checkPath(&p);
        if (v > currVersion) {
            if (gIsDebug) {
                fprintf(stderr, "Java %d found via env PATH: %s\n", v, p.cstr());
            }
            currVersion = v;
            *outJavaPath = p;
        }
    }

    delete paths;
    return currVersion;
}

// --------------

static bool getRegValue(const char *keyPath,
                        const char *keyName,
                        REGSAM access,
                        CString *outValue) {
    HKEY key;
    LSTATUS status = RegOpenKeyExA(
        HKEY_LOCAL_MACHINE,         // hKey
        keyPath,                    // lpSubKey
        0,                          // ulOptions
        KEY_READ | access,          // samDesired,
        &key);                      // phkResult
    if (status == ERROR_SUCCESS) {

        LSTATUS ret = ERROR_MORE_DATA;
        DWORD size = 4096; // MAX_PATH is 260, so 4 KB should be good enough
        char* buffer = (char*) malloc(size);

        while (ret == ERROR_MORE_DATA && size < (1<<16) /*64 KB*/) {
            ret = RegQueryValueExA(
                key,                // hKey
                keyName,            // lpValueName
                NULL,               // lpReserved
                NULL,               // lpType
                (LPBYTE) buffer,    // lpData
                &size);             // lpcbData

            if (ret == ERROR_MORE_DATA) {
                size *= 2;
                buffer = (char*) realloc(buffer, size);
            } else {
                buffer[size] = 0;
            }
        }

        if (ret != ERROR_MORE_DATA) outValue->set(buffer);

        free(buffer);
        RegCloseKey(key);

        return (ret != ERROR_MORE_DATA);
    }

    return false;
}

// Explore the registry to find a suitable version of Java.
// Returns an int which is the version of Java found (e.g. 1006 for 1.6) and the
// matching path in outJavaPath.
// Returns 0 if nothing suitable was found.
static int exploreJavaRegistry(const char *entry, REGSAM access, CPath *outJavaPath) {

    // Let's visit HKEY_LOCAL_MACHINE\SOFTWARE\JavaSoft\Java Runtime Environment [CurrentVersion]
    CPath rootKey("SOFTWARE\\JavaSoft\\");
    rootKey.addPath(entry);

    int versionInt = 0;
    CString currentVersion;
    CPath subKey(rootKey);
    if (getRegValue(subKey.cstr(), "CurrentVersion", access, &currentVersion)) {
        // CurrentVersion should be something like "1.7".
        // We want to read HKEY_LOCAL_MACHINE\SOFTWARE\JavaSoft\Java Runtime Environment\1.7 [JavaHome]
        subKey.addPath(currentVersion);
        CPath javaHome;
        if (getRegValue(subKey.cstr(), "JavaHome", access, &javaHome)) {
            versionInt = checkBinPath(&javaHome);
            if (versionInt >= 0) {
                if (gIsDebug) {
                    fprintf(stderr,
                            "Java %d found via registry: %s\n",
                            versionInt, javaHome.cstr());
                }
                *outJavaPath = javaHome;
            }
            if (versionInt >= MIN_JAVA_VERSION) {
                // Heuristic: if the current version is good enough, stop here
                return versionInt;
            }
        }
    }

    // Try again, but this time look at all the versions available
    HKEY javaHomeKey;
    LSTATUS status = RegOpenKeyExA(
        HKEY_LOCAL_MACHINE,         // hKey
        "SOFTWARE\\JavaSoft",       // lpSubKey
        0,                          // ulOptions
        KEY_READ | access,          // samDesired
        &javaHomeKey);              // phkResult
    if (status == ERROR_SUCCESS) {
        char name[256];
        DWORD index = 0;
        CPath javaHome;
        for (LONG result = ERROR_SUCCESS; result == ERROR_SUCCESS; index++) {
            DWORD nameLen = 255;
            name[nameLen] = 0;
            result = RegEnumKeyExA(
                            javaHomeKey,  // hKey
                            index,        // dwIndex
                            name,         // lpName
                            &nameLen,     // lpcName
                            NULL,         // lpReserved
                            NULL,         // lpClass
                            NULL,         // lpcClass,
                            NULL);        // lpftLastWriteTime
            if (result == ERROR_SUCCESS && nameLen < 256) {
                name[nameLen] = 0;
                CPath subKey(rootKey);
                subKey.addPath(name);

                if (getRegValue(subKey.cstr(), "JavaHome", access, &javaHome)) {
                    int v = checkBinPath(&javaHome);
                    if (v > versionInt) {
                        if (gIsDebug) {
                            fprintf(stderr,
                                    "Java %d found via registry: %s\n",
                                    versionInt, javaHome.cstr());
                        }
                        *outJavaPath = javaHome;
                        versionInt = v;
                    }
                }
            }
        }

        RegCloseKey(javaHomeKey);
    }

    return 0;
}

static bool getMaxJavaInRegistry(const char *entry, REGSAM access, CPath *outJavaPath, int *inOutVersion) {
    CPath path;
    int version = exploreJavaRegistry(entry, access, &path);
    if (version > *inOutVersion) {
        *outJavaPath  = path;
        *inOutVersion = version;
        return true;
    }
    return false;
}

int findJavaInRegistry(CPath *outJavaPath) {
    // We'll do the registry test 3 times: first using the default mode,
    // then forcing the use of the 32-bit registry then forcing the use of
    // 64-bit registry. On Windows 2k, the 2 latter will fail since the
    // flags are not supported. On a 32-bit OS the 64-bit is obviously
    // useless and the 2 first tests should be equivalent so we just
    // need the first case.

    // Check the JRE first, then the JDK.
    int version = MIN_JAVA_VERSION - 1;
    bool result = false;
    result |= getMaxJavaInRegistry("Java Runtime Environment", 0, outJavaPath, &version);
    result |= getMaxJavaInRegistry("Java Development Kit",     0, outJavaPath, &version);

    // Get the app sysinfo state (the one hidden by WOW64)
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    WORD programArch = sysInfo.wProcessorArchitecture;
    // Check the real sysinfo state (not the one hidden by WOW64) for x86
    GetNativeSystemInfo(&sysInfo);
    WORD actualArch = sysInfo.wProcessorArchitecture;

    // Only try to access the WOW64-32 redirected keys on a 64-bit system.
    // There's no point in doing this on a 32-bit system.
    if (actualArch == PROCESSOR_ARCHITECTURE_AMD64) {
        if (programArch != PROCESSOR_ARCHITECTURE_INTEL) {
            // If we did the 32-bit case earlier, don't do it twice.
            result |= getMaxJavaInRegistry(
                "Java Runtime Environment", KEY_WOW64_32KEY, outJavaPath, &version);
            result |= getMaxJavaInRegistry(
                "Java Development Kit",     KEY_WOW64_32KEY, outJavaPath, &version);

        } else if (programArch != PROCESSOR_ARCHITECTURE_AMD64) {
            // If we did the 64-bit case earlier, don't do it twice.
            result |= getMaxJavaInRegistry(
                "Java Runtime Environment", KEY_WOW64_64KEY, outJavaPath, &version);
            result |= getMaxJavaInRegistry(
                "Java Development Kit",     KEY_WOW64_64KEY, outJavaPath, &version);
        }
    }

    return result ? version : 0;
}

// --------------

static bool checkProgramFiles(CPath *outJavaPath, int *inOutVersion) {

    char programFilesPath[MAX_PATH + 1];
    HRESULT result = SHGetFolderPathA(
        NULL,                       // hwndOwner
        CSIDL_PROGRAM_FILES,        // nFolder
        NULL,                       // hToken
        SHGFP_TYPE_CURRENT,         // dwFlags
        programFilesPath);          // pszPath
    if (FAILED(result)) return false;

    CPath path(programFilesPath);
    path.addPath("Java");

    // Do we have a C:\\Program Files\\Java directory?
    if (!path.dirExists()) return false;

    CPath glob(path);
    glob.addPath("j*");

    bool found = false;
    WIN32_FIND_DATAA findData;
    HANDLE findH = FindFirstFileA(glob.cstr(), &findData);
    if (findH == INVALID_HANDLE_VALUE) return false;
    do {
        if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
            CPath temp(path);
            temp.addPath(findData.cFileName);
            // Check C:\\Program Files[x86]\\Java\\j*\\bin\\java.exe
            int v = checkBinPath(&temp);
            if (v > *inOutVersion) {
                found = true;
                *inOutVersion = v;
                *outJavaPath = temp;
            }
        }
    } while (!found && FindNextFileA(findH, &findData) != 0);
    FindClose(findH);

    return found;
}

int findJavaInProgramFiles(CPath *outJavaPath) {

    // Check the C:\\Program Files (x86) directory
    // With WOW64 fs redirection in place by default, we should get the x86
    // version on a 64-bit OS since this app is a 32-bit itself.
    bool result = false;
    int version = MIN_JAVA_VERSION - 1;
    result |= checkProgramFiles(outJavaPath, &version);

    // Check the real sysinfo state (not the one hidden by WOW64) for x86
    SYSTEM_INFO sysInfo;
    GetNativeSystemInfo(&sysInfo);

    if (sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) {
        // On a 64-bit OS, try again by disabling the fs redirection so
        // that we can try the real C:\\Program Files directory.
        PVOID oldWow64Value = disableWow64FsRedirection();
        result |= checkProgramFiles(outJavaPath, &version);
        revertWow64FsRedirection(oldWow64Value);
    }

    return result ? version : 0;
}

// --------------


// Tries to invoke the java.exe at the given path and extract it's
// version number.
// - outVersionStr: if not null, will capture version as a string (e.g. "1.6")
// - outVersionInt: if not null, will capture version as an int (major * 1000 + minor, e.g. 1006).
bool getJavaVersion(CPath &javaPath, CString *outVersionStr, int *outVersionInt) {
    bool result = false;

    // Run "java -version", which outputs something like to *STDERR*:
    //
    // java version "1.6.0_29"
    // Java(TM) SE Runtime Environment (build 1.6.0_29-b11)
    // Java HotSpot(TM) Client VM (build 20.4-b02, mixed mode, sharing)
    //
    // We want to capture the first line, and more exactly the "1.6" part.


    CString cmd;
    cmd.setf("\"%s\" -version", javaPath.cstr());

    SECURITY_ATTRIBUTES   saAttr;
    STARTUPINFO           startup;
    PROCESS_INFORMATION   pinfo;

    // Want to inherit pipe handle
    ZeroMemory(&saAttr, sizeof(saAttr));
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
    saAttr.bInheritHandle = TRUE; 
    saAttr.lpSecurityDescriptor = NULL; 

    // Create pipe for stdout
    HANDLE stdoutPipeRd, stdoutPipeWt;
    if (!CreatePipe(
            &stdoutPipeRd,      // hReadPipe,
            &stdoutPipeWt,      // hWritePipe,
            &saAttr,            // lpPipeAttributes,
            0)) {               // nSize (0=default buffer size)
        if (gIsConsole || gIsDebug) displayLastError("CreatePipe failed: ");
        return false;
    }
    if (!SetHandleInformation(stdoutPipeRd, HANDLE_FLAG_INHERIT, 0)) {
        if (gIsConsole || gIsDebug) displayLastError("SetHandleInformation failed: ");
        return false;
    }

    ZeroMemory(&pinfo, sizeof(pinfo));

    ZeroMemory(&startup, sizeof(startup));
    startup.cb          = sizeof(startup);
    startup.dwFlags     = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    startup.wShowWindow = SW_HIDE|SW_MINIMIZE;
    // Capture both stderr and stdout
    startup.hStdError   = stdoutPipeWt;
    startup.hStdOutput  = stdoutPipeWt;
    startup.hStdInput   = GetStdHandle(STD_INPUT_HANDLE);

    BOOL ok = CreateProcessA(
            NULL,                   // program path
            (LPSTR) cmd.cstr(),     // command-line
            NULL,                   // process handle is not inheritable
            NULL,                   // thread handle is not inheritable
            TRUE,                   // yes, inherit some handles
            0,                      // process creation flags
            NULL,                   // use parent's environment block
            NULL,                   // use parent's starting directory
            &startup,               // startup info, i.e. std handles
            &pinfo);

    if ((gIsConsole || gIsDebug) && !ok) displayLastError("CreateProcess failed: ");

    // Close the write-end of the output pipe (we're only reading from it)
    CloseHandle(stdoutPipeWt);

    // Read from the output pipe. We don't need to read everything,
    // the first line should be 'Java version "1.2.3_45"\r\n'
    // so reading about 32 chars is all we need.
    char first32[32 + 1];
    int index = 0;
    first32[0] = 0;

    if (ok) {

        #define SIZE 1024
        char buffer[SIZE];
        DWORD sizeRead = 0;

        while (ok) {
            // Keep reading in the same buffer location
            ok = ReadFile(stdoutPipeRd,     // hFile
                          buffer,           // lpBuffer
                          SIZE,             // DWORD buffer size to read
                          &sizeRead,        // DWORD buffer size read
                          NULL);            // overlapped
            if (!ok || sizeRead == 0 || sizeRead > SIZE) break;

            // Copy up to the first 32 characters
            if (index < 32) {
                DWORD n = 32 - index;
                if (n > sizeRead) n = sizeRead;
                // copy as lowercase to simplify checks later
                for (char *b = buffer; n > 0; n--, b++, index++) {
                    char c = *b;
                    if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
                    first32[index] = c;
                }
                first32[index] = 0;
            }
        }

        WaitForSingleObject(pinfo.hProcess, INFINITE);

        DWORD exitCode;
        if (GetExitCodeProcess(pinfo.hProcess, &exitCode)) {
            // this should not return STILL_ACTIVE (259)
            result = exitCode == 0;
        }

        CloseHandle(pinfo.hProcess);
        CloseHandle(pinfo.hThread);
    }
    CloseHandle(stdoutPipeRd);

    if (result && index > 0) {
        // Look for a few keywords in the output however we don't
        // care about specific ordering or case-senstiviness.
        // We only captures roughtly the first line in lower case.
        char *j = strstr(first32, "java");
        char *v = strstr(first32, "version");
        if ((gIsConsole || gIsDebug) && (!j || !v)) {
            fprintf(stderr, "Error: keywords 'java version' not found in '%s'\n", first32);
        }
        if (j != NULL && v != NULL) {
            result = extractJavaVersion(first32, index, outVersionStr, outVersionInt);
        }
    }

    return result;
}


#endif /* _WIN32 */
