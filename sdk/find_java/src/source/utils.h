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

#ifndef _H_UTILS
#define _H_UTILS

#ifdef _WIN32

#define _CRT_SECURE_NO_WARNINGS 1

#include <direct.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <windows.h>

// VS vs MINGW specific includes
#ifdef USE_VS_CRT
    #include <crtdbg.h>     // for _ASSERT
#else
    #define _ASSERT(x)      // undef
#endif

extern bool gIsDebug;
extern bool gIsConsole;

// An array that knows its own size. Not dynamically resizable.
template <class T> class CArray {
    T* mPtr;
    int mSize;
public:
    explicit CArray(int size) {
        mSize = size;
        mPtr = new T[size];
    }

    ~CArray() {
        if (mPtr != NULL) {
            delete[] mPtr;
            mPtr = NULL;
        }
        mSize = 0;
    }

    T& operator[](int i) {
        _ASSERT(i >= 0 && i < mSize);
        return mPtr[i];
    }

    int size() const {
        return mSize;
    }
};

// A simple string class wrapper.
class CString {
protected:
    char *mStr;
public:
    CString()                              { mStr = NULL; }
    CString(const CString &str)            { mStr = NULL; set(str.mStr); }
    explicit CString(const char *str)      { mStr = NULL; set(str); }
    CString(const char *start, int length) { mStr = NULL; set(start, length); }

    CString& operator=(const CString &str) {
        return set(str.cstr());
    }

    CString& set(const char *str) {
        if (str != mStr) {
            _free();
            if (str != NULL) {
                mStr = _strdup(str);
            }
        }
        return *this;
    }

    CString& set(const char *start, int length) {
        _free();
        if (start != NULL) {
            mStr = (char *)malloc(length + 1);
            strncpy(mStr, start, length);
            mStr[length] = 0;
        }
        return *this;
    }

    CString& setv(const char *str, va_list ap) {
        _free();
        // _vscprintf(str, ap) is only available with the MSVCRT, not MinGW.
        // Instead we'll iterate till we have enough space to generate the string.
        int len = strlen(str) + 1024;
        mStr = (char *)malloc(len);
        strcpy(mStr, str); // provide a default in case vsnprintf totally fails
        for (int guard = 0; guard < 10; guard++) {
            int ret = vsnprintf(mStr, len, str, ap);
            if (ret == -1) {
                // Some implementations don't give the proper size needed
                // so double the space and try again.
                len *= 2;
            } else if (ret >= len) {
                len = ret + 1;
            } else {
                // There was enough space to write.
                break;
            }
            mStr = (char *)realloc((void *)mStr, len);
            strcpy(mStr, str); // provide a default in case vsnprintf totally fails
        }
        return *this;
    }

    CString& setf(const char *str, ...) {
        _free();
        va_list ap;
        va_start(ap, str);
        setv(str, ap);
        va_end(ap);
        return *this;
    }

    virtual ~CString() { _free(); }

    // Returns the C string owned by this CString. It will be
    // invalid as soon as this CString is deleted or out of scope.
    const char * cstr() const {
        return mStr;
    }

    bool isEmpty() const {
        return mStr == NULL || *mStr == 0;
    }

    int length() const {
        return mStr == NULL ? 0 : strlen(mStr);
    }

    CString& add(const char *str) {
        if (mStr == NULL) {
            set(str);
        } else {
            mStr = (char *)realloc((void *)mStr, strlen(mStr) + strlen(str) + 1);
            strcat(mStr, str);
        }
        return *this;
    }

    CString& add(const char *str, int length) {
        if (mStr == NULL) {
            set(str, length);
        } else {
            int   l1 = strlen(mStr);
            mStr = (char *)realloc((void *)mStr, l1 + length + 1);
            strncpy(mStr + l1, str, length);
            mStr[l1 + length] = 0;
        }
        return *this;
    }

    CArray<CString> * split(char sep) const {
        if (mStr == NULL) {
            return new CArray<CString>(0);
        }
        const char *last = NULL;
        int n = 0;
        for (const char *s = mStr; *s; s++) {
            if (*s == sep && s != mStr && (last == NULL || s > last+1)) {
                n++;
                last = s;
            }
        }

        CArray<CString> *result = new CArray<CString>(n);
        last = NULL;
        n = 0;
        for (const char *s = mStr; *s; s++) {
            if (*s == sep) {
                if (s != mStr && (last == NULL || s > last+1)) {
                    const char *start = last ? last : mStr;
                    (*result)[n++].set(start, s-start);
                }
                last = s+1;
            }
        }

        return result;
    }

    // Sets the string to the message matching Win32 GetLastError.
    // If message is non-null, it is prepended to the last error string.
    CString& setLastWin32Error(const char *message) {
        DWORD err = GetLastError();
        LPSTR errStr;
        if (FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | /* dwFlags */
                          FORMAT_MESSAGE_FROM_SYSTEM,
                          NULL,                             /* lpSource */
                          err,                              /* dwMessageId */
                          0,                                /* dwLanguageId */
                          (LPSTR)&errStr,                   /* lpBuffer */
                          0,                                /* nSize */
                          NULL) != 0) {                     /* va_list args */
            if (message == NULL) {
                setf("[%d] %s", err, errStr);
            } else {
                setf("%s[%d] %s", message, err, errStr);
            }
            LocalFree(errStr);
        }
        return *this;
    }

private:
    void _free() {
        if (mStr != NULL) {
            free((void *)mStr);
            mStr = NULL;
        }
    }

};

// A simple path class wrapper.
class CPath : public CString {
public:
    CPath()                              : CString()    { }
    CPath(const CString &str)            : CString(str) { }
    CPath(const CPath &str)              : CString(str) { }
    explicit CPath(const char *str)      : CString(str) { }
    CPath(const char *start, int length) : CString(start, length) { }

    CPath& operator=(const CPath &str) {
        set(str.cstr());
        return *this;
    }

    // Appends a path segment, adding a \ as necessary.
    CPath& addPath(const CString &s) {
        return addPath(s.cstr());
    }

    // Appends a path segment, adding a \ as necessary.
    CPath& addPath(const char *s) {
        _ASSERT(s != NULL);
        if (s != NULL && s[0] != 0) {
            int n = length();
            if (n > 0 && s[0] != '\\' && mStr[n-1] != '\\') add("\\");
            add(s);
        }
        return *this;
    }

    // Returns true if file exist and is not a directory.
    // There's no garantee we have rights to access it.
    bool fileExists() const {
        if (mStr == NULL) return false;
        DWORD attribs = GetFileAttributesA(mStr);
        return attribs != INVALID_FILE_ATTRIBUTES &&
             !(attribs & FILE_ATTRIBUTE_DIRECTORY);
    }

    // Returns true if file exist and is a directory.
    // There's no garantee we have rights to access it.
    bool dirExists() const {
        if (mStr == NULL) return false;
        DWORD attribs = GetFileAttributesA(mStr);
        return attribs != INVALID_FILE_ATTRIBUTES &&
              (attribs & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }

    // Returns a copy of the directory portion of the path, if any
    CPath dirName() const {
        CPath result;
        if (mStr != NULL) {
            char *pos = strrchr(mStr, '\\');
            if (pos != NULL) {
                result.set(mStr, pos - mStr);
            }
        }
        return result;
    }

    // Returns a pointer to the baseName part of the path.
    // It becomes invalid if the path changes.
    const char * baseName() const {
        if (mStr != NULL) {
            char *pos = strrchr(mStr, '\\');
            if (pos != NULL) {
                return pos + 1;
            }
        }
        return NULL;
    }

    // If the path ends with the given searchName, replace in-place by the new name
    void replaceName(const char *searchName, const char* newName) {
        if (mStr == NULL) return;
        int n = length();
        int sn = strlen(searchName);
        if (n < sn) return;
        // if mStr ends with searchName
        if (strcmp(mStr + n - sn, searchName) == 0) {
            int sn2 = strlen(newName);
            if (sn2 > sn) {
                mStr = (char *)realloc((void *)mStr, n + sn2 - sn + 1);
            }
            strcpy(mStr + n - sn, newName);
            mStr[n + sn2 - sn] = 0;
        }
    }

    // Returns a copy of this path as a DOS short path in the destination.
    // Returns true if the Win32 getShortPathName method worked.
    // In case of error, returns false and does not change the destination.
    // It's OK to invoke this->toShortPath(this).
    bool toShortPath(CPath *dest) {
        const char *longPath = mStr;
        if (mStr == NULL) return false;

        DWORD lenShort = strlen(longPath) + 1;
        char * shortPath = (char *)malloc(lenShort);

        DWORD length = GetShortPathName(longPath, shortPath, lenShort);
        if (length > lenShort) {
            // The buffer wasn't big enough, this is the size to use.
            free(shortPath);
            lenShort = length;
            shortPath = (char *)malloc(length);
            length = GetShortPathName(longPath, shortPath, lenShort);
        }

        if (length != 0) dest->set(shortPath);

        free(shortPath);
        return length != 0;
    }
};

// Displays a message in an ok+info dialog box.
void msgBox(const char* text, ...);

// Displays GetLastError prefixed with a description in an error dialog box
void displayLastError(const char *description, ...);

// Executes the command line. Does not wait for the program to finish.
// The return code is from CreateProcess (0 means failure), not the running app.
int execNoWait(const char *app, const char *params, const char *workDir);

// Executes command, waits for completion and returns exit code.
// As indicated in MSDN for CreateProcess, callers should double-quote the program name
// e.g. cmd="\"c:\program files\myapp.exe\" arg1 arg2";
int execWait(const char *cmd);

bool getModuleDir(CPath *outDir);

// Disables the FS redirection done by WOW64.
// Because this runs as a 32-bit app, Windows automagically remaps some
// folder under the hood (e.g. "Programs Files(x86)" is mapped as "Program Files").
// This prevents the app from correctly searching for java.exe in these folders.
// The registry is also remapped.
PVOID disableWow64FsRedirection();

// Reverts the redirection disabled in disableWow64FsRedirection.
void revertWow64FsRedirection(PVOID oldWow64Value);

#endif /* _WIN32 */
#endif /* _H_UTILS */
