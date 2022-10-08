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
 *
 * References used:
 *
 * http://drdobbs.com/windows/184411099
 * article by Sven B. Schreiber, November 01, 1999
 *
 * http://www.codeguru.com/Cpp/W-P/system/processesmodules/article.php/c2827/
 * by Zoltan Csizmadia, November 14, 2000
 *
 * http://stackoverflow.com/questions/860656/
 * (same technique, but written in unsafe C#)
 *
 * Starting with Vista, we can also use the Restart Manager API as
 * explained here: (TODO for next version)
 * http://msdn.microsoft.com/en-us/magazine/cc163450.aspx
 */

#ifdef _WIN32

#include "utils.h"
#include <ctype.h>
#include <fcntl.h>
#include <io.h>
#include <process.h>


// NtDll structures from the the Dr Dobbs article, adjusted for our needs:

typedef void *POBJECT;
typedef LONG KPRIORITY;
typedef LARGE_INTEGER QWORD;

typedef struct {
    WORD  Length;
    WORD  MaximumLength;
    PWORD Buffer;
} UNICODE_STRING;

typedef struct {
    DWORD       dIdProcess;
    BYTE        bObjectType;    // OB_TYPE_*
    BYTE        bFlags;         // bits 0..2 HANDLE_FLAG_*
    WORD        wValue;         // multiple of 4
    POBJECT     pObject;
    ACCESS_MASK GrantedAccess;
} SYSTEM_HANDLE;

typedef struct {
    DWORD         dCount;
    SYSTEM_HANDLE ash[1];
} SYSTEM_HANDLE_INFORMATION;

typedef struct {
    DWORD PeakVirtualSize;
    DWORD VirtualSize;
    DWORD PageFaultCount;
    DWORD PeakWorkingSetSize;
    DWORD WorkingSetSize;
    DWORD QuotaPeakPagedPoolUsage;
    DWORD QuotaPagedPoolUsage;
    DWORD QuotaPeakNonPagedPoolUsage;
    DWORD QuotaNonPagedPoolUsage;
    DWORD PagefileUsage;
    DWORD PeakPagefileUsage;
} VM_COUNTERS;

typedef struct {
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} CLIENT_ID;

typedef enum {
    // Ignored. We don't actually use these values.
    Unused
} KWAIT_REASON;

typedef struct {
    QWORD        qKernelTime;       // 100 nsec units
    QWORD        qUserTime;         // 100 nsec units
    QWORD        qCreateTime;       // relative to 01-01-1601
    DWORD        d18;
    PVOID        pStartAddress;
    CLIENT_ID    Cid;               // process/thread ids
    DWORD        dPriority;
    DWORD        dBasePriority;
    DWORD        dContextSwitches;
    DWORD        dThreadState;      // 2=running, 5=waiting
    KWAIT_REASON WaitReason;
    DWORD        dReserved01;
} SYSTEM_THREAD;

typedef struct {
    DWORD          dNext;           // relative offset
    DWORD          dThreadCount;
    DWORD          dReserved01;
    DWORD          dReserved02;
    DWORD          dReserved03;
    DWORD          dReserved04;
    DWORD          dReserved05;
    DWORD          dReserved06;
    QWORD          qCreateTime;     // relative to 01-01-1601
    QWORD          qUserTime;       // 100 nsec units
    QWORD          qKernelTime;     // 100 nsec units
    UNICODE_STRING usName;
    KPRIORITY      BasePriority;
    DWORD          dUniqueProcessId;
    DWORD          dInheritedFromUniqueProcessId;
    DWORD          dHandleCount;
    DWORD          dReserved07;
    DWORD          dReserved08;
    VM_COUNTERS    VmCounters;
    DWORD          dCommitCharge;   // bytes
    SYSTEM_THREAD  ast[1];
} SYSTEM_PROCESS_INFORMATION;

// The sic opcode for NtQuerySystemInformation
typedef enum {
    SystemProcessInformation = 5,
    SystemHandleInformation = 16,
} SYSTEMINFOCLASS;


#define STATUS_SUCCESS               0x00000000
#define STATUS_UNSUCCESSFUL          0xC0000001
#define STATUS_NOT_IMPLEMENTED       0xC0000002
#define STATUS_INVALID_INFO_CLASS    0xC0000003
#define STATUS_INFO_LENGTH_MISMATCH  0xC0000004
#define STATUS_INVALID_PARAMETER     0xC000000D

typedef DWORD (WINAPI *NtQuerySystemInformationFuncPtr)(
                                      DWORD sic, VOID* pData, DWORD sSize, ULONG* pdSize);
typedef DWORD (WINAPI *NtQueryInformationFileFuncPtr)(HANDLE, PVOID, PVOID, DWORD, DWORD);
typedef DWORD (WINAPI *NtQueryObjectFuncPtr)(HANDLE, DWORD, VOID*, DWORD, VOID*);

static NtQuerySystemInformationFuncPtr sNtQuerySystemInformationFunc;
static NtQueryInformationFileFuncPtr   sNtQueryInformationFileFunc;
static NtQueryObjectFuncPtr            sNtQueryObjectFunc;

//------------

// Get the NT DLL functions we need to use.
static bool init() {

    sNtQuerySystemInformationFunc =
        (NtQuerySystemInformationFuncPtr) GetProcAddress(
            GetModuleHandleA("ntdll.dll"), "NtQuerySystemInformation");

    sNtQueryInformationFileFunc =
        (NtQueryInformationFileFuncPtr) GetProcAddress(
            GetModuleHandleA("ntdll.dll"), "NtQueryInformationFile");

    sNtQueryObjectFunc =
        (NtQueryObjectFuncPtr) GetProcAddress(
            GetModuleHandleA("ntdll.dll"), "NtQueryObject");

    return sNtQuerySystemInformationFunc != NULL &&
           sNtQueryInformationFileFunc   != NULL &&
           sNtQueryObjectFunc            != NULL;
}

static void terminate() {
    sNtQuerySystemInformationFunc = NULL;
    sNtQueryInformationFileFunc = NULL;
    sNtQueryObjectFunc = NULL;
}

static bool adjustPrivileges() {
    char *error = NULL;
    HANDLE tokenH;

    // Open a process token that lets us adjust privileges
    BOOL ok = OpenProcessToken(GetCurrentProcess(),   // ProcessHandle
                               TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, // DesiredAccess
                               &tokenH);              // TokenHandle
    if (!ok) {
        error = "OpenProcessToken failed: ";
        goto bail_out;
    }

    // Lookup the privilege by name and get its local LUID token.
    // What we request:
    // SE_DEBUG_NAME, aka "SeDebugPrivilege"
    // MSDN: Required to debug and adjust the memory of a process owned by another account.
    //       User Right: Debug programs.
    TOKEN_PRIVILEGES priv;
    priv.PrivilegeCount = 1;
    priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    ok = LookupPrivilegeValueA(NULL,                        // lpSystemName
                               SE_DEBUG_NAME,               // lpName
                               &(priv.Privileges[0].Luid)); // lpLuid
    if (!ok) {
        error = "LookupPrivilegeValue failed: ";
        goto bail_out;
    }

    ok = AdjustTokenPrivileges(tokenH,  // TokenHandle
                               FALSE,   // DisableAllPrivileges
                               &priv,   // NewState
                               0,       // BufferLength
                               NULL,    // PreviousState
                               0);      // ReturnLength
    if (!ok) {
        error = "AdjustTokenPrivileges failed: ";
        goto bail_out;
    }

bail_out:
    if (error != NULL && gIsDebug) {
        CString err;
        err.setLastWin32Error(error);
        fprintf(stderr, "%s", err.cstr());
    }

    if (tokenH != NULL) {
        CloseHandle(tokenH);
    }

    return !!ok;
}

static bool getHandleType(HANDLE h, CString *type) {
    bool result = false;
    ULONG size = 0;
    // Get the size of the type string
    int status = sNtQueryObjectFunc(h, 2, NULL, 0, &size);
    if (status == STATUS_INFO_LENGTH_MISMATCH && size > 0) {
        // Get the type string itself
        char *buf = new char[size];
        status = sNtQueryObjectFunc(h, 2, buf, size, NULL);
        if (status == 0 && size > 96) {
            // The type string we want is a wide unicode (UTF16)
            // zero-terminated string located at offset 96 in the
            // buffer. In our case we want the string to be
            // "Directory" or "File" so we know the max useful length
            // is 9.
            // Since we can only deal with ansi strings in this program,
            // we'll make a crude copy of every other byte and just check
            // that the other bytes are zero.
            const char *c = buf + 96;
            const char *e = buf + 96 + size;
            // we'll write at the beginning of our buffer
            char *dest = buf;
            char *dend = dest + 9;
            for (; c < e && dest < dend && c[0] != '\0' && c[1] == '\0'; c += 2, dest++) {
                *dest = *c;
            }
            *(dest++) = '\0';
            type->set(buf, dest - buf);
            result = true;
        }

        free(buf);
    }
    return result;
}

// These is the wide unicode representations of the type we want to find.
static const char kFileW[] = "File";

static char isFileHandleType(HANDLE handle) {
    char type = 0;
    ULONG size = 0;
    // Get the size of the type string
    int status = sNtQueryObjectFunc(handle, 2, NULL, 0, &size);
    if (status == STATUS_INFO_LENGTH_MISMATCH && size > 0) {
        // Get the type string itself
        char *buf = new char[size];
        status = sNtQueryObjectFunc(handle, 2, buf, size, NULL);
        if (status == 0 && size > 96) {
            // The type string we want is a wide unicode (UTF16-LE)
            // zero-terminated string located at offset 96 in the
            // buffer. In our case we want the string to be "File".
            //
            // Since we're reading wide unicode, we want each character
            // to be the one from our string followed by a zero byte.
            // e.g. c should point to F \0 i \0 l \0 e \0 \0 \0.
            const char *c = buf + 96;
            type = c[0];

            int len = sizeof(kFileW);
            const char *d = kFileW;

            for (; type != 0 && len > 0; c+=2, d++, len--) {
                if (c[0] != *d || c[1] != 0) {
                    type = 0;
                    break;
                }
            }
        }

        free(buf);
    }
    return type;
}

typedef struct {
    HANDLE handle;
    CString *outStr;
    bool result;
} SFileNameInfo;

static unsigned __stdcall FileNameThreadFunc(void *param) {
    SFileNameInfo *info = (SFileNameInfo *)param;
    if (info == NULL) {
        return 1;
    }

    char buf[MAX_PATH*2 + 4];
    DWORD iob[2] = { 0, 0 };

    DWORD status = sNtQueryInformationFileFunc(info->handle, iob, buf, sizeof(buf), 9);
    if (status == STATUS_SUCCESS) {
        // The result is a buffer with:
        // - DWORD (4 bytes) for the *byte* length (so twice the character length)
        // - Actual string in Unicode
        // Not sure of the actual type, but it does look like a UNICODE_STRING struct.

        DWORD len = ((DWORD *)buf)[0];
        if (len <= MAX_PATH * 2) {
            // We can't handle wide Unicode. What we do is convert it into
            // straight ansi by just retaining the first of each couple bytes.
            // Bytes that cannot be mapped (e.g. 2nd byte is != 0) will be
            // simply converted to 0xFF.

            unsigned char *dest = (unsigned char *)buf + 4;
            unsigned char *src  = (unsigned char *)buf + 4;
            for (DWORD i = 0; i < len; dest++, src += 2, i += 2) {
                if (src[1] == 0) {
                    *dest = *src;
                } else {
                    *dest = 0xFF;
                }
            }
            *dest = '\0';
            info->outStr->set(buf + 4, len);
            info->result = true;
            return 0;
        }
    }
    return 1;
}

static bool getFileName(HANDLE handle, CString *outStr) {
    SFileNameInfo info;
    info.handle = handle;
    info.outStr = outStr;
    info.result = false;

    // sNtQueryInformationFileFunc might hang on some handles.
    // A trick is to do it in a thread and if it takes too loog then
    // just shutdown the thread, since it's deadlocked anyway.
    unsigned threadId;
    HANDLE th = (HANDLE)_beginthreadex(NULL,                    // security
                                       0,                       // stack_size
                                       &FileNameThreadFunc,     // address
                                       &info,                   // arglist
                                       0,                       // initflag
                                       &threadId);              // thrdaddr

    if (th == NULL) {
        // Failed to create thread. Shouldn't really happen.
        outStr->set("<failed to create thread>");
        return false;
    }

    bool result = false;

    // Wait for thread or kill it if it takes too long.
    if (WaitForSingleObject(th /*handle*/, 200 /*ms*/) == WAIT_TIMEOUT) {
        TerminateThread(th /*handle*/, 0 /*retCode*/);
        outStr->set("<timeout>");
    } else {
        result = info.result;
    }

    CloseHandle(th);
    return result;
}

// Find the name of the process (e.g. "java.exe") given its id.
// processesPtr must be the list returned by getAllProcesses().
// Special handling for javaw.exe: this isn't quite useful so
// we also try to find and append the parent process name.
static bool getProcessName(SYSTEM_PROCESS_INFORMATION *processesPtr,
                           DWORD remoteProcessId,
                           CString *outStr) {
    SYSTEM_PROCESS_INFORMATION *ptr = processesPtr;
    while (ptr != NULL) {
        if (ptr->dUniqueProcessId == remoteProcessId) {
            // This is the process we want.

            UNICODE_STRING *uniStr = &(ptr->usName);
            WORD len = uniStr->Length;

            char buf[MAX_PATH];
            if (len <= MAX_PATH * 2) {
                // We can't handle wide Unicode. What we do is convert it into
                // straight ansi by just retaining the first of each couple bytes.
                // Bytes that cannot be mapped (e.g. 2nd byte is != 0) will be
                // simply converted to 0xFF.

                unsigned char *dest = (unsigned char *)buf;
                unsigned char *src  = (unsigned char *)uniStr->Buffer;
                for (WORD i = 0; i < len; dest++, src += 2, i += 2) {
                    if (src[1] == 0) {
                        *dest = *src;
                    } else {
                        *dest = 0xFF;
                    }
                }
                *dest = '\0';
                outStr->set(buf, len);

                if (strcmp(buf, "javaw.exe") == 0) {
                    // Heuristic: eclipse often shows up as javaw.exe
                    // but what is useful is to report eclipse to the user
                    // instead.
                    // So in this case, look at the parent and report it too.
                    DWORD parentId = ptr->dInheritedFromUniqueProcessId;
                    if (parentId > 0) {
                        CString name2;
                        bool ok2 = getProcessName(processesPtr,
                                                  parentId,
                                                  &name2);
                        if (ok2) {
                            outStr->add(" (");
                            outStr->add(name2.cstr());
                            outStr->add(")");
                        }
                    }
                }

                return true;
            }
        }

        // Look at the next process, if any.
        if (ptr->dNext == NULL) {
            break;
        } else {
            ptr = (SYSTEM_PROCESS_INFORMATION *)((char *)ptr + ptr->dNext);
        }
    }

    outStr->setf("<process id %08x name not found>", remoteProcessId);
    return false;
}

// Query system for all processes information.
// Returns an error string in case of error.
// Returns the virtual_alloc-allocated buffer on success or NULL on error.
// It's up to the caller to do a VirtualFree on the returned buffer.
static SYSTEM_PROCESS_INFORMATION *queryAllProcess(const char **error) {
    // Allocate a buffer for the process information. We don't know the
    // exact size. A normal system might typically have between 100-200 processes.
    // We'll resize the buffer if not big enough.
    DWORD infoSize = 4096;
    SYSTEM_PROCESS_INFORMATION *infoPtr =
        (SYSTEM_PROCESS_INFORMATION *) VirtualAlloc(NULL, infoSize, MEM_COMMIT, PAGE_READWRITE);

    if (infoPtr != NULL) {
        // Query the actual size needed (or the data if it fits in the buffer)
        DWORD needed = 0;
        if (sNtQuerySystemInformationFunc(
                SystemProcessInformation, infoPtr, infoSize, &needed) != 0) {
            if (needed == 0) {
                // Shouldn't happen.
                *error = "No processes found";
                goto bail_out;
            }

            // Realloc
            VirtualFree(infoPtr, 0, MEM_RELEASE);
            infoSize += needed;
            infoPtr = (SYSTEM_PROCESS_INFORMATION *) VirtualAlloc(
                            NULL, infoSize, MEM_COMMIT, PAGE_READWRITE);

            // Query all the processes objects again
            if (sNtQuerySystemInformationFunc(
                    SystemProcessInformation, infoPtr, infoSize, NULL) != 0) {
                *error = "Failed to query system processes";
                goto bail_out;
            }
        }
    }

    if (infoPtr == NULL) {
        *error = "Failed to allocate system processes info buffer";
        goto bail_out;
    }

bail_out:
    if (*error != NULL) {
        VirtualFree(infoPtr, 0, MEM_RELEASE);
        infoPtr = NULL;
    }
    return infoPtr;
}

// Query system for all handle information.
// Returns an error string in case of error.
// Returns the virtual_alloc-allocated buffer on success or NULL on error.
// It's up to the caller to do a VirtualFree on the returned buffer.
static SYSTEM_HANDLE_INFORMATION *queryAllHandles(const char **error) {
    // Allocate a buffer. It won't be large enough to get the handles
    // (e.g. there might be 10k or 40k handles around). We'll resize
    // it once we know the actual size.
    DWORD infoSize = 4096;
    SYSTEM_HANDLE_INFORMATION *infoPtr =
        (SYSTEM_HANDLE_INFORMATION *) VirtualAlloc(NULL, infoSize, MEM_COMMIT, PAGE_READWRITE);

    if (infoPtr != NULL) {
        // Query the actual size needed
        DWORD needed = 0;
        if (sNtQuerySystemInformationFunc(
                SystemHandleInformation, infoPtr, infoSize, &needed) != 0) {
            if (needed == 0) {
                // Shouldn't happen.
                *error = "No handles found";
                goto bail_out;
            }

            // Realloc
            VirtualFree(infoPtr, 0, MEM_RELEASE);
            infoSize += needed;
            infoPtr = (SYSTEM_HANDLE_INFORMATION *) VirtualAlloc(
                            NULL, infoSize, MEM_COMMIT, PAGE_READWRITE);
        }
    }

    if (infoPtr == NULL) {
        *error = "Failed to allocate system handle info buffer";
        goto bail_out;
    }

    // Query all the handle objects
    if (sNtQuerySystemInformationFunc(SystemHandleInformation, infoPtr, infoSize, NULL) != 0) {
        *error = "Failed to query system handles";
        goto bail_out;
    }

bail_out:
    if (*error != NULL) {
        VirtualFree(infoPtr, 0, MEM_RELEASE);
        infoPtr = NULL;
    }
    return infoPtr;
}

bool findLock(CPath &path, CString *outModule) {
    bool result = false;
    const char *error = NULL;

    SYSTEM_PROCESS_INFORMATION *processesPtr = NULL;
    SYSTEM_HANDLE_INFORMATION  *handlesPtr   = NULL;

    const HANDLE currProcessH = GetCurrentProcess();
    const DWORD currProcessId = GetCurrentProcessId();
    HANDLE remoteProcessH = NULL;
    DWORD remoteProcessId = 0;
    DWORD matchProcessId = 0;

    int numHandleFound = 0;
    int numHandleChecked = 0;
    int numHandleDirs = 0;
    int numHandleFiles = 0;
    int numProcessMatch = 0;

    BYTE ob_type_file = 0;

    // Get the path to search, without the drive letter.
    const char *searchPath = path.cstr();
    if (isalpha(searchPath[0]) && searchPath[1] == ':') {
        searchPath += 2;
    }
    int searchPathLen = strlen(searchPath);

    if (gIsDebug) fprintf(stderr, "Search path: '%s'\n", searchPath);

    if (!init()) {
        error = "Failed to bind to ntdll.dll";
        goto bail_out;
    }

    if (!adjustPrivileges()) {
        // We can still continue even if the privilege escalation failed.
        // The apparent effect is that we'll fail to query the name of
        // some processes, yet it will work for some of them.
        if (gIsDebug) fprintf(stderr, "Warning: adusting privileges failed. Continuing anyway.\n");
    } else {
        if (gIsDebug) fprintf(stderr, "Privileges adjusted.\n"); // DEBUG remove lter
    }

    processesPtr = queryAllProcess(&error);
    if (processesPtr == NULL) goto bail_out;

    handlesPtr = queryAllHandles(&error);
    if (handlesPtr == NULL) goto bail_out;

    numHandleFound = handlesPtr->dCount;

    // Check all the handles
    for (int n = handlesPtr->dCount, i = 0; i < n; i++) {
        SYSTEM_HANDLE sysh = handlesPtr->ash[i];

        if (ob_type_file != 0 && sysh.bObjectType != ob_type_file) {
            continue;
        }

        HANDLE handle = (HANDLE) sysh.wValue;
        DWORD remoteId = sysh.dIdProcess;
        HANDLE remoteH = NULL;

        if (remoteId == matchProcessId) {
            // We already matched that process, we can skip its other entries.
            continue;
        }

        if (remoteId == currProcessId) {
            // We don't match ourselves
            continue;
        }

        // Open a remote process.
        // Most entries of a given process seem to be consecutive, so we
        // only open the remote process handle if it's a different id.
        if (remoteProcessH == NULL && remoteId == remoteProcessId) {
            // We already tried to open this process and it failed.
            // It's not going to be any better the next time so skip it.
            continue;
        }
        if (remoteProcessH == NULL || remoteId != remoteProcessId) {
            if (remoteProcessH != NULL) {
                CloseHandle(remoteProcessH);
            }

            remoteProcessId = remoteId;
            remoteProcessH = OpenProcess(PROCESS_DUP_HANDLE,
                                         FALSE /*inheritHandle*/,
                                         remoteProcessId);
            if (remoteProcessH == NULL) {
                continue;
            }
        }

        if (remoteProcessH != NULL) {
            // Duplicate the remote handle
            if (DuplicateHandle(remoteProcessH,     // hSourceProcessHandle
                                handle,             // hSourceHandle
                                currProcessH,       // hTargetProcessHandle
                                &remoteH,           // lpTargetHandle
                                0,                  // dwDesiredAccess (ignored by same access)
                                FALSE,              // bInheritHandle
                                DUPLICATE_SAME_ACCESS) == 0) {
                continue;
            }
        }

        numHandleChecked++;

        char type = isFileHandleType(remoteH);

        if (type != 0) {
            if (type == 'D') numHandleDirs++;
            else if (type == 'F') numHandleFiles++;

            // TODO simplify by not keeping directory handles
            if (ob_type_file == 0 && type == 'F') {
                // We found the first file handle. Remember it's system_handle object type
                // and then use it to filter the following system_handle.
                // For some reason OB_TYPE_FILE should be 0x1A but empirically I find it
                // to be 0x1C, so we just make this test more dynamic.
                ob_type_file = sysh.bObjectType;
            }

            // Try to get a filename out of that file or directory handle.
            CString name("<unknown>");
            bool ok = getFileName(remoteH, &name);

            if (gIsDebug) {
                fprintf(stderr, "P:%08x | t:%02x | f:%02x | v:%08x | %c | %s %s\n",
                    sysh.dIdProcess, sysh.bObjectType, sysh.bFlags, sysh.wValue,
                    type,
                    ok ? "OK" : "FAIL",
                    name.cstr()
                    );
            }

            if (ok) {
                // We got a file path. Let's check if it matches our target path.
                if (_strnicmp(searchPath, name.cstr(), searchPathLen) == 0) {
                    // Remember this process id so that we can ignore all its following entries.
                    matchProcessId = remoteId;

                    // Find out its process name
                    CString procName("<unknown>");
                    ok = getProcessName(processesPtr, remoteProcessId, &procName);
                    if (ok) {
                        numProcessMatch++;

                        if (!outModule->isEmpty()) {
                            outModule->add(";");
                        }
                        outModule->add(procName.cstr());
                        result = true;
                    }

                    if (gIsDebug) {
                        fprintf(stderr, "==> MATCH FOUND: %s  %s\n",
                            ok ? "OK" : "FAIL",
                            procName.cstr()
                            );
                    }
                }
            }

        }

        if (remoteH != NULL) {
            CloseHandle(remoteH);
            remoteH = NULL;
        }
    }

bail_out:

    if (gIsDebug) {
        fprintf(stderr, "Processes matched: %d\n", numProcessMatch);
        fprintf(stderr, "Handles: %d found, %d checked, %d dirs, %d files\n",
               numHandleFound,
               numHandleChecked,
               numHandleDirs,
               numHandleFiles);
    }

    if (error != NULL) {
        CString msg;
        msg.setLastWin32Error(NULL);
        if (gIsDebug) fprintf(stderr, "[ERROR] %s: %s", error, msg.cstr());
    }

    if (remoteProcessH != NULL) {
        CloseHandle(remoteProcessH);
    }

    if (currProcessH != NULL) {
        CloseHandle(currProcessH);
    }

    if (handlesPtr != NULL) {
        VirtualFree(handlesPtr, 0, MEM_RELEASE);
        handlesPtr = NULL;
    }

    terminate();

    return result;
}

#endif /* _WIN32 */
