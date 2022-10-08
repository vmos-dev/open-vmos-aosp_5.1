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

#define LOG_TAG "Posix"

#include "AsynchronousCloseMonitor.h"
#include "cutils/log.h"
#include "ExecStrings.h"
#include "JNIHelp.h"
#include "JniConstants.h"
#include "JniException.h"
#include "NetworkUtilities.h"
#include "Portability.h"
#include "readlink.h"
#include "../../bionic/libc/dns/include/resolv_netid.h"  // For android_getaddrinfofornet.
#include "ScopedBytes.h"
#include "ScopedLocalRef.h"
#include "ScopedPrimitiveArray.h"
#include "ScopedUtfChars.h"
#include "toStringArray.h"
#include "UniquePtr.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <pwd.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#ifndef __APPLE__
#include <sys/prctl.h>
#endif
#include <sys/socket.h>
#include <sys/stat.h>
#ifdef __APPLE__
#include <sys/statvfs.h>
#endif
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>


#ifndef __unused
#define __unused __attribute__((__unused__))
#endif

#define TO_JAVA_STRING(NAME, EXP) \
        jstring NAME = env->NewStringUTF(EXP); \
        if (NAME == NULL) return NULL;

struct addrinfo_deleter {
    void operator()(addrinfo* p) const {
        if (p != NULL) { // bionic's freeaddrinfo(3) crashes when passed NULL.
            freeaddrinfo(p);
        }
    }
};

/**
 * Used to retry networking system calls that can be interrupted with a signal. Unlike
 * TEMP_FAILURE_RETRY, this also handles the case where
 * AsynchronousCloseMonitor::signalBlockedThreads(fd) is used to signal a close() or
 * Thread.interrupt(). Other signals that result in an EINTR result are ignored and the system call
 * is retried.
 *
 * Returns the result of the system call though a Java exception will be pending if the result is
 * -1:  a SocketException if signaled via AsynchronousCloseMonitor, or ErrnoException for other
 * failures.
 */
#define NET_FAILURE_RETRY(jni_env, return_type, syscall_name, java_fd, ...) ({ \
    return_type _rc = -1; \
    do { \
        bool _wasSignaled; \
        int _syscallErrno; \
        { \
            int _fd = jniGetFDFromFileDescriptor(jni_env, java_fd); \
            AsynchronousCloseMonitor _monitor(_fd); \
            _rc = syscall_name(_fd, __VA_ARGS__); \
            _syscallErrno = errno; \
            _wasSignaled = _monitor.wasSignaled(); \
        } \
        if (_wasSignaled) { \
            jniThrowException(jni_env, "java/net/SocketException", "Socket closed"); \
            _rc = -1; \
            break; \
        } \
        if (_rc == -1 && _syscallErrno != EINTR) { \
            /* TODO: with a format string we could show the arguments too, like strace(1). */ \
            throwErrnoException(jni_env, # syscall_name); \
            break; \
        } \
    } while (_rc == -1); /* _syscallErrno == EINTR && !_wasSignaled */ \
    _rc; })

/**
 * Used to retry system calls that can be interrupted with a signal. Unlike TEMP_FAILURE_RETRY, this
 * also handles the case where AsynchronousCloseMonitor::signalBlockedThreads(fd) is used to signal
 * a close() or Thread.interrupt(). Other signals that result in an EINTR result are ignored and the
 * system call is retried.
 *
 * Returns the result of the system call though a Java exception will be pending if the result is
 * -1: an IOException if the file descriptor is already closed, a InterruptedIOException if signaled
 * via AsynchronousCloseMonitor, or ErrnoException for other failures.
 */
#define IO_FAILURE_RETRY(jni_env, return_type, syscall_name, java_fd, ...) ({ \
    return_type _rc = -1; \
    do { \
        bool _wasSignaled; \
        int _syscallErrno; \
        { \
            int _fd = jniGetFDFromFileDescriptor(jni_env, java_fd); \
            AsynchronousCloseMonitor _monitor(_fd); \
            _rc = syscall_name(_fd, __VA_ARGS__); \
            _syscallErrno = errno; \
            _wasSignaled = _monitor.wasSignaled(); \
        } \
        if (_wasSignaled) { \
            jniThrowException(jni_env, "java/io/InterruptedIOException", # syscall_name " interrupted"); \
            _rc = -1; \
            break; \
        } \
        if (_rc == -1 && _syscallErrno != EINTR) { \
            /* TODO: with a format string we could show the arguments too, like strace(1). */ \
            throwErrnoException(jni_env, # syscall_name); \
            break; \
        } \
    } while (_rc == -1); /* && _syscallErrno == EINTR && !_wasSignaled */ \
    _rc; })

static void throwException(JNIEnv* env, jclass exceptionClass, jmethodID ctor3, jmethodID ctor2,
        const char* functionName, int error) {
    jthrowable cause = NULL;
    if (env->ExceptionCheck()) {
        cause = env->ExceptionOccurred();
        env->ExceptionClear();
    }

    ScopedLocalRef<jstring> detailMessage(env, env->NewStringUTF(functionName));
    if (detailMessage.get() == NULL) {
        // Not really much we can do here. We're probably dead in the water,
        // but let's try to stumble on...
        env->ExceptionClear();
    }

    jobject exception;
    if (cause != NULL) {
        exception = env->NewObject(exceptionClass, ctor3, detailMessage.get(), error, cause);
    } else {
        exception = env->NewObject(exceptionClass, ctor2, detailMessage.get(), error);
    }
    env->Throw(reinterpret_cast<jthrowable>(exception));
}

static void throwErrnoException(JNIEnv* env, const char* functionName) {
    int error = errno;
    static jmethodID ctor3 = env->GetMethodID(JniConstants::errnoExceptionClass,
            "<init>", "(Ljava/lang/String;ILjava/lang/Throwable;)V");
    static jmethodID ctor2 = env->GetMethodID(JniConstants::errnoExceptionClass,
            "<init>", "(Ljava/lang/String;I)V");
    throwException(env, JniConstants::errnoExceptionClass, ctor3, ctor2, functionName, error);
}

static void throwGaiException(JNIEnv* env, const char* functionName, int error) {
  // Cache the methods ids before we throw, so we don't call GetMethodID with a pending exception.
  static jmethodID ctor3 = env->GetMethodID(JniConstants::gaiExceptionClass, "<init>",
                                            "(Ljava/lang/String;ILjava/lang/Throwable;)V");
  static jmethodID ctor2 = env->GetMethodID(JniConstants::gaiExceptionClass, "<init>",
                                            "(Ljava/lang/String;I)V");
  if (errno != 0) {
        // EAI_SYSTEM should mean "look at errno instead", but both glibc and bionic seem to
        // mess this up. In particular, if you don't have INTERNET permission, errno will be EACCES
        // but you'll get EAI_NONAME or EAI_NODATA. So we want our GaiException to have a
        // potentially-relevant ErrnoException as its cause even if error != EAI_SYSTEM.
        // http://code.google.com/p/android/issues/detail?id=15722
        throwErrnoException(env, functionName);
        // Deliberately fall through to throw another exception...
    }
    throwException(env, JniConstants::gaiExceptionClass, ctor3, ctor2, functionName, error);
}

template <typename rc_t>
static rc_t throwIfMinusOne(JNIEnv* env, const char* name, rc_t rc) {
    if (rc == rc_t(-1)) {
        throwErrnoException(env, name);
    }
    return rc;
}

template <typename ScopedT>
class IoVec {
public:
    IoVec(JNIEnv* env, size_t bufferCount) : mEnv(env), mBufferCount(bufferCount) {
    }

    bool init(jobjectArray javaBuffers, jintArray javaOffsets, jintArray javaByteCounts) {
        // We can't delete our local references until after the I/O, so make sure we have room.
        if (mEnv->PushLocalFrame(mBufferCount + 16) < 0) {
            return false;
        }
        ScopedIntArrayRO offsets(mEnv, javaOffsets);
        if (offsets.get() == NULL) {
            return false;
        }
        ScopedIntArrayRO byteCounts(mEnv, javaByteCounts);
        if (byteCounts.get() == NULL) {
            return false;
        }
        // TODO: Linux actually has a 1024 buffer limit. glibc works around this, and we should too.
        // TODO: you can query the limit at runtime with sysconf(_SC_IOV_MAX).
        for (size_t i = 0; i < mBufferCount; ++i) {
            jobject buffer = mEnv->GetObjectArrayElement(javaBuffers, i); // We keep this local ref.
            mScopedBuffers.push_back(new ScopedT(mEnv, buffer));
            jbyte* ptr = const_cast<jbyte*>(mScopedBuffers.back()->get());
            if (ptr == NULL) {
                return false;
            }
            struct iovec iov;
            iov.iov_base = reinterpret_cast<void*>(ptr + offsets[i]);
            iov.iov_len = byteCounts[i];
            mIoVec.push_back(iov);
        }
        return true;
    }

    ~IoVec() {
        for (size_t i = 0; i < mScopedBuffers.size(); ++i) {
            delete mScopedBuffers[i];
        }
        mEnv->PopLocalFrame(NULL);
    }

    iovec* get() {
        return &mIoVec[0];
    }

    size_t size() {
        return mBufferCount;
    }

private:
    JNIEnv* mEnv;
    size_t mBufferCount;
    std::vector<iovec> mIoVec;
    std::vector<ScopedT*> mScopedBuffers;
};

static jobject makeSocketAddress(JNIEnv* env, const sockaddr_storage& ss) {
    jint port;
    jobject inetAddress = sockaddrToInetAddress(env, ss, &port);
    if (inetAddress == NULL) {
        return NULL;
    }
    static jmethodID ctor = env->GetMethodID(JniConstants::inetSocketAddressClass, "<init>",
            "(Ljava/net/InetAddress;I)V");
    return env->NewObject(JniConstants::inetSocketAddressClass, ctor, inetAddress, port);
}

static jobject makeStructPasswd(JNIEnv* env, const struct passwd& pw) {
    TO_JAVA_STRING(pw_name, pw.pw_name);
    TO_JAVA_STRING(pw_dir, pw.pw_dir);
    TO_JAVA_STRING(pw_shell, pw.pw_shell);
    static jmethodID ctor = env->GetMethodID(JniConstants::structPasswdClass, "<init>",
            "(Ljava/lang/String;IILjava/lang/String;Ljava/lang/String;)V");
    return env->NewObject(JniConstants::structPasswdClass, ctor,
            pw_name, static_cast<jint>(pw.pw_uid), static_cast<jint>(pw.pw_gid), pw_dir, pw_shell);
}

static jobject makeStructStat(JNIEnv* env, const struct stat& sb) {
    static jmethodID ctor = env->GetMethodID(JniConstants::structStatClass, "<init>",
            "(JJIJIIJJJJJJJ)V");
    return env->NewObject(JniConstants::structStatClass, ctor,
            static_cast<jlong>(sb.st_dev), static_cast<jlong>(sb.st_ino),
            static_cast<jint>(sb.st_mode), static_cast<jlong>(sb.st_nlink),
            static_cast<jint>(sb.st_uid), static_cast<jint>(sb.st_gid),
            static_cast<jlong>(sb.st_rdev), static_cast<jlong>(sb.st_size),
            static_cast<jlong>(sb.st_atime), static_cast<jlong>(sb.st_mtime),
            static_cast<jlong>(sb.st_ctime), static_cast<jlong>(sb.st_blksize),
            static_cast<jlong>(sb.st_blocks));
}

static jobject makeStructStatVfs(JNIEnv* env, const struct statvfs& sb) {
#if defined(__APPLE__)
    // Mac OS has no f_namelen field in struct statfs.
    jlong max_name_length = 255; // __DARWIN_MAXNAMLEN
#else
    jlong max_name_length = static_cast<jlong>(sb.f_namemax);
#endif

    static jmethodID ctor = env->GetMethodID(JniConstants::structStatVfsClass, "<init>",
            "(JJJJJJJJJJJ)V");
    return env->NewObject(JniConstants::structStatVfsClass, ctor,
                          static_cast<jlong>(sb.f_bsize),
                          static_cast<jlong>(sb.f_frsize),
                          static_cast<jlong>(sb.f_blocks),
                          static_cast<jlong>(sb.f_bfree),
                          static_cast<jlong>(sb.f_bavail),
                          static_cast<jlong>(sb.f_files),
                          static_cast<jlong>(sb.f_ffree),
                          static_cast<jlong>(sb.f_favail),
                          static_cast<jlong>(sb.f_fsid),
                          static_cast<jlong>(sb.f_flag),
                          max_name_length);
}

static jobject makeStructLinger(JNIEnv* env, const struct linger& l) {
    static jmethodID ctor = env->GetMethodID(JniConstants::structLingerClass, "<init>", "(II)V");
    return env->NewObject(JniConstants::structLingerClass, ctor, l.l_onoff, l.l_linger);
}

static jobject makeStructTimeval(JNIEnv* env, const struct timeval& tv) {
    static jmethodID ctor = env->GetMethodID(JniConstants::structTimevalClass, "<init>", "(JJ)V");
    return env->NewObject(JniConstants::structTimevalClass, ctor,
            static_cast<jlong>(tv.tv_sec), static_cast<jlong>(tv.tv_usec));
}

static jobject makeStructUcred(JNIEnv* env, const struct ucred& u __unused) {
#ifdef __APPLE__
  jniThrowException(env, "java/lang/UnsupportedOperationException", "unimplemented support for ucred on a Mac");
  return NULL;
#else
  static jmethodID ctor = env->GetMethodID(JniConstants::structUcredClass, "<init>", "(III)V");
  return env->NewObject(JniConstants::structUcredClass, ctor, u.pid, u.uid, u.gid);
#endif
}

static jobject makeStructUtsname(JNIEnv* env, const struct utsname& buf) {
    TO_JAVA_STRING(sysname, buf.sysname);
    TO_JAVA_STRING(nodename, buf.nodename);
    TO_JAVA_STRING(release, buf.release);
    TO_JAVA_STRING(version, buf.version);
    TO_JAVA_STRING(machine, buf.machine);
    static jmethodID ctor = env->GetMethodID(JniConstants::structUtsnameClass, "<init>",
            "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
    return env->NewObject(JniConstants::structUtsnameClass, ctor,
            sysname, nodename, release, version, machine);
};

static bool fillIfreq(JNIEnv* env, jstring javaInterfaceName, struct ifreq& req) {
    ScopedUtfChars interfaceName(env, javaInterfaceName);
    if (interfaceName.c_str() == NULL) {
        return false;
    }
    memset(&req, 0, sizeof(req));
    strncpy(req.ifr_name, interfaceName.c_str(), sizeof(req.ifr_name));
    req.ifr_name[sizeof(req.ifr_name) - 1] = '\0';
    return true;
}

static bool fillInetSocketAddress(JNIEnv* env, jint rc, jobject javaInetSocketAddress, const sockaddr_storage& ss) {
    if (rc == -1 || javaInetSocketAddress == NULL) {
        return true;
    }
    // Fill out the passed-in InetSocketAddress with the sender's IP address and port number.
    jint port;
    jobject sender = sockaddrToInetAddress(env, ss, &port);
    if (sender == NULL) {
        return false;
    }
    static jfieldID addressFid = env->GetFieldID(JniConstants::inetSocketAddressClass, "addr", "Ljava/net/InetAddress;");
    static jfieldID portFid = env->GetFieldID(JniConstants::inetSocketAddressClass, "port", "I");
    env->SetObjectField(javaInetSocketAddress, addressFid, sender);
    env->SetIntField(javaInetSocketAddress, portFid, port);
    return true;
}

static jobject doStat(JNIEnv* env, jstring javaPath, bool isLstat) {
    ScopedUtfChars path(env, javaPath);
    if (path.c_str() == NULL) {
        return NULL;
    }
    struct stat sb;
    int rc = isLstat ? TEMP_FAILURE_RETRY(lstat(path.c_str(), &sb))
                     : TEMP_FAILURE_RETRY(stat(path.c_str(), &sb));
    if (rc == -1) {
        throwErrnoException(env, isLstat ? "lstat" : "stat");
        return NULL;
    }
    return makeStructStat(env, sb);
}

static jobject doGetSockName(JNIEnv* env, jobject javaFd, bool is_sockname) {
  int fd = jniGetFDFromFileDescriptor(env, javaFd);
  sockaddr_storage ss;
  sockaddr* sa = reinterpret_cast<sockaddr*>(&ss);
  socklen_t byteCount = sizeof(ss);
  memset(&ss, 0, byteCount);
  int rc = is_sockname ? TEMP_FAILURE_RETRY(getsockname(fd, sa, &byteCount))
      : TEMP_FAILURE_RETRY(getpeername(fd, sa, &byteCount));
  if (rc == -1) {
    throwErrnoException(env, is_sockname ? "getsockname" : "getpeername");
    return NULL;
  }
  return makeSocketAddress(env, ss);
}

class Passwd {
public:
    Passwd(JNIEnv* env) : mEnv(env), mResult(NULL) {
        mBufferSize = sysconf(_SC_GETPW_R_SIZE_MAX);
        mBuffer.reset(new char[mBufferSize]);
    }

    jobject getpwnam(const char* name) {
        return process("getpwnam_r", getpwnam_r(name, &mPwd, mBuffer.get(), mBufferSize, &mResult));
    }

    jobject getpwuid(uid_t uid) {
        return process("getpwuid_r", getpwuid_r(uid, &mPwd, mBuffer.get(), mBufferSize, &mResult));
    }

    struct passwd* get() {
        return mResult;
    }

private:
    jobject process(const char* syscall, int error) {
        if (mResult == NULL) {
            errno = error;
            throwErrnoException(mEnv, syscall);
            return NULL;
        }
        return makeStructPasswd(mEnv, *mResult);
    }

    JNIEnv* mEnv;
    UniquePtr<char[]> mBuffer;
    size_t mBufferSize;
    struct passwd mPwd;
    struct passwd* mResult;
};

static jobject Posix_accept(JNIEnv* env, jobject, jobject javaFd, jobject javaInetSocketAddress) {
    sockaddr_storage ss;
    socklen_t sl = sizeof(ss);
    memset(&ss, 0, sizeof(ss));
    sockaddr* peer = (javaInetSocketAddress != NULL) ? reinterpret_cast<sockaddr*>(&ss) : NULL;
    socklen_t* peerLength = (javaInetSocketAddress != NULL) ? &sl : 0;
    jint clientFd = NET_FAILURE_RETRY(env, int, accept, javaFd, peer, peerLength);
    if (clientFd == -1 || !fillInetSocketAddress(env, clientFd, javaInetSocketAddress, ss)) {
        close(clientFd);
        return NULL;
    }
    return (clientFd != -1) ? jniCreateFileDescriptor(env, clientFd) : NULL;
}

static jboolean Posix_access(JNIEnv* env, jobject, jstring javaPath, jint mode) {
    ScopedUtfChars path(env, javaPath);
    if (path.c_str() == NULL) {
        return JNI_FALSE;
    }
    int rc = TEMP_FAILURE_RETRY(access(path.c_str(), mode));
    if (rc == -1) {
        throwErrnoException(env, "access");
    }
    return (rc == 0);
}

static void Posix_bind(JNIEnv* env, jobject, jobject javaFd, jobject javaAddress, jint port) {
    sockaddr_storage ss;
    socklen_t sa_len;
    if (!inetAddressToSockaddr(env, javaAddress, port, ss, sa_len)) {
        return;
    }
    const sockaddr* sa = reinterpret_cast<const sockaddr*>(&ss);
    // We don't need the return value because we'll already have thrown.
    (void) NET_FAILURE_RETRY(env, int, bind, javaFd, sa, sa_len);
}

static void Posix_chmod(JNIEnv* env, jobject, jstring javaPath, jint mode) {
    ScopedUtfChars path(env, javaPath);
    if (path.c_str() == NULL) {
        return;
    }
    throwIfMinusOne(env, "chmod", TEMP_FAILURE_RETRY(chmod(path.c_str(), mode)));
}

static void Posix_chown(JNIEnv* env, jobject, jstring javaPath, jint uid, jint gid) {
    ScopedUtfChars path(env, javaPath);
    if (path.c_str() == NULL) {
        return;
    }
    throwIfMinusOne(env, "chown", TEMP_FAILURE_RETRY(chown(path.c_str(), uid, gid)));
}

static void Posix_close(JNIEnv* env, jobject, jobject javaFd) {
    // Get the FileDescriptor's 'fd' field and clear it.
    // We need to do this before we can throw an IOException (http://b/3222087).
    int fd = jniGetFDFromFileDescriptor(env, javaFd);
    jniSetFileDescriptorOfFD(env, javaFd, -1);

    // Even if close(2) fails with EINTR, the fd will have been closed.
    // Using TEMP_FAILURE_RETRY will either lead to EBADF or closing someone else's fd.
    // http://lkml.indiana.edu/hypermail/linux/kernel/0509.1/0877.html
    throwIfMinusOne(env, "close", close(fd));
}

static void Posix_connect(JNIEnv* env, jobject, jobject javaFd, jobject javaAddress, jint port) {
    sockaddr_storage ss;
    socklen_t sa_len;
    if (!inetAddressToSockaddr(env, javaAddress, port, ss, sa_len)) {
        return;
    }
    const sockaddr* sa = reinterpret_cast<const sockaddr*>(&ss);
    // We don't need the return value because we'll already have thrown.
    (void) NET_FAILURE_RETRY(env, int, connect, javaFd, sa, sa_len);
}

static jobject Posix_dup(JNIEnv* env, jobject, jobject javaOldFd) {
    int oldFd = jniGetFDFromFileDescriptor(env, javaOldFd);
    int newFd = throwIfMinusOne(env, "dup", TEMP_FAILURE_RETRY(dup(oldFd)));
    return (newFd != -1) ? jniCreateFileDescriptor(env, newFd) : NULL;
}

static jobject Posix_dup2(JNIEnv* env, jobject, jobject javaOldFd, jint newFd) {
    int oldFd = jniGetFDFromFileDescriptor(env, javaOldFd);
    int fd = throwIfMinusOne(env, "dup2", TEMP_FAILURE_RETRY(dup2(oldFd, newFd)));
    return (fd != -1) ? jniCreateFileDescriptor(env, fd) : NULL;
}

static jobjectArray Posix_environ(JNIEnv* env, jobject) {
    extern char** environ; // Standard, but not in any header file.
    return toStringArray(env, environ);
}

static void Posix_execve(JNIEnv* env, jobject, jstring javaFilename, jobjectArray javaArgv, jobjectArray javaEnvp) {
    ScopedUtfChars path(env, javaFilename);
    if (path.c_str() == NULL) {
        return;
    }

    ExecStrings argv(env, javaArgv);
    ExecStrings envp(env, javaEnvp);
    execve(path.c_str(), argv.get(), envp.get());

    throwErrnoException(env, "execve");
}

static void Posix_execv(JNIEnv* env, jobject, jstring javaFilename, jobjectArray javaArgv) {
    ScopedUtfChars path(env, javaFilename);
    if (path.c_str() == NULL) {
        return;
    }

    ExecStrings argv(env, javaArgv);
    execv(path.c_str(), argv.get());

    throwErrnoException(env, "execv");
}

static void Posix_fchmod(JNIEnv* env, jobject, jobject javaFd, jint mode) {
    int fd = jniGetFDFromFileDescriptor(env, javaFd);
    throwIfMinusOne(env, "fchmod", TEMP_FAILURE_RETRY(fchmod(fd, mode)));
}

static void Posix_fchown(JNIEnv* env, jobject, jobject javaFd, jint uid, jint gid) {
    int fd = jniGetFDFromFileDescriptor(env, javaFd);
    throwIfMinusOne(env, "fchown", TEMP_FAILURE_RETRY(fchown(fd, uid, gid)));
}

static jint Posix_fcntlVoid(JNIEnv* env, jobject, jobject javaFd, jint cmd) {
    int fd = jniGetFDFromFileDescriptor(env, javaFd);
    return throwIfMinusOne(env, "fcntl", TEMP_FAILURE_RETRY(fcntl(fd, cmd)));
}

static jint Posix_fcntlLong(JNIEnv* env, jobject, jobject javaFd, jint cmd, jlong arg) {
    int fd = jniGetFDFromFileDescriptor(env, javaFd);
    return throwIfMinusOne(env, "fcntl", TEMP_FAILURE_RETRY(fcntl(fd, cmd, arg)));
}

static jint Posix_fcntlFlock(JNIEnv* env, jobject, jobject javaFd, jint cmd, jobject javaFlock) {
    static jfieldID typeFid = env->GetFieldID(JniConstants::structFlockClass, "l_type", "S");
    static jfieldID whenceFid = env->GetFieldID(JniConstants::structFlockClass, "l_whence", "S");
    static jfieldID startFid = env->GetFieldID(JniConstants::structFlockClass, "l_start", "J");
    static jfieldID lenFid = env->GetFieldID(JniConstants::structFlockClass, "l_len", "J");
    static jfieldID pidFid = env->GetFieldID(JniConstants::structFlockClass, "l_pid", "I");

    struct flock64 lock;
    memset(&lock, 0, sizeof(lock));
    lock.l_type = env->GetShortField(javaFlock, typeFid);
    lock.l_whence = env->GetShortField(javaFlock, whenceFid);
    lock.l_start = env->GetLongField(javaFlock, startFid);
    lock.l_len = env->GetLongField(javaFlock, lenFid);
    lock.l_pid = env->GetIntField(javaFlock, pidFid);

    int rc = IO_FAILURE_RETRY(env, int, fcntl, javaFd, cmd, &lock);
    if (rc != -1) {
        env->SetShortField(javaFlock, typeFid, lock.l_type);
        env->SetShortField(javaFlock, whenceFid, lock.l_whence);
        env->SetLongField(javaFlock, startFid, lock.l_start);
        env->SetLongField(javaFlock, lenFid, lock.l_len);
        env->SetIntField(javaFlock, pidFid, lock.l_pid);
    }
    return rc;
}

static void Posix_fdatasync(JNIEnv* env, jobject, jobject javaFd) {
    int fd = jniGetFDFromFileDescriptor(env, javaFd);
    throwIfMinusOne(env, "fdatasync", TEMP_FAILURE_RETRY(fdatasync(fd)));
}

static jobject Posix_fstat(JNIEnv* env, jobject, jobject javaFd) {
    int fd = jniGetFDFromFileDescriptor(env, javaFd);
    struct stat sb;
    int rc = TEMP_FAILURE_RETRY(fstat(fd, &sb));
    if (rc == -1) {
        throwErrnoException(env, "fstat");
        return NULL;
    }
    return makeStructStat(env, sb);
}

static jobject Posix_fstatvfs(JNIEnv* env, jobject, jobject javaFd) {
    int fd = jniGetFDFromFileDescriptor(env, javaFd);
    struct statvfs sb;
    int rc = TEMP_FAILURE_RETRY(fstatvfs(fd, &sb));
    if (rc == -1) {
        throwErrnoException(env, "fstatvfs");
        return NULL;
    }
    return makeStructStatVfs(env, sb);
}

static void Posix_fsync(JNIEnv* env, jobject, jobject javaFd) {
    int fd = jniGetFDFromFileDescriptor(env, javaFd);
    throwIfMinusOne(env, "fsync", TEMP_FAILURE_RETRY(fsync(fd)));
}

static void Posix_ftruncate(JNIEnv* env, jobject, jobject javaFd, jlong length) {
    int fd = jniGetFDFromFileDescriptor(env, javaFd);
    throwIfMinusOne(env, "ftruncate", TEMP_FAILURE_RETRY(ftruncate64(fd, length)));
}

static jstring Posix_gai_strerror(JNIEnv* env, jobject, jint error) {
    return env->NewStringUTF(gai_strerror(error));
}

static jobjectArray Posix_android_getaddrinfo(JNIEnv* env, jobject, jstring javaNode,
        jobject javaHints, jint netId) {
    ScopedUtfChars node(env, javaNode);
    if (node.c_str() == NULL) {
        return NULL;
    }

    static jfieldID flagsFid = env->GetFieldID(JniConstants::structAddrinfoClass, "ai_flags", "I");
    static jfieldID familyFid = env->GetFieldID(JniConstants::structAddrinfoClass, "ai_family", "I");
    static jfieldID socktypeFid = env->GetFieldID(JniConstants::structAddrinfoClass, "ai_socktype", "I");
    static jfieldID protocolFid = env->GetFieldID(JniConstants::structAddrinfoClass, "ai_protocol", "I");

    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = env->GetIntField(javaHints, flagsFid);
    hints.ai_family = env->GetIntField(javaHints, familyFid);
    hints.ai_socktype = env->GetIntField(javaHints, socktypeFid);
    hints.ai_protocol = env->GetIntField(javaHints, protocolFid);

    addrinfo* addressList = NULL;
    errno = 0;
    int rc = android_getaddrinfofornet(node.c_str(), NULL, &hints, netId, 0, &addressList);
    UniquePtr<addrinfo, addrinfo_deleter> addressListDeleter(addressList);
    if (rc != 0) {
        throwGaiException(env, "android_getaddrinfo", rc);
        return NULL;
    }

    // Count results so we know how to size the output array.
    int addressCount = 0;
    for (addrinfo* ai = addressList; ai != NULL; ai = ai->ai_next) {
        if (ai->ai_family == AF_INET || ai->ai_family == AF_INET6) {
            ++addressCount;
        } else {
            ALOGE("android_getaddrinfo unexpected ai_family %i", ai->ai_family);
        }
    }
    if (addressCount == 0) {
        return NULL;
    }

    // Prepare output array.
    jobjectArray result = env->NewObjectArray(addressCount, JniConstants::inetAddressClass, NULL);
    if (result == NULL) {
        return NULL;
    }

    // Examine returned addresses one by one, save them in the output array.
    int index = 0;
    for (addrinfo* ai = addressList; ai != NULL; ai = ai->ai_next) {
        if (ai->ai_family != AF_INET && ai->ai_family != AF_INET6) {
            // Unknown address family. Skip this address.
            ALOGE("android_getaddrinfo unexpected ai_family %i", ai->ai_family);
            continue;
        }

        // Convert each IP address into a Java byte array.
        sockaddr_storage& address = *reinterpret_cast<sockaddr_storage*>(ai->ai_addr);
        ScopedLocalRef<jobject> inetAddress(env, sockaddrToInetAddress(env, address, NULL));
        if (inetAddress.get() == NULL) {
            return NULL;
        }
        env->SetObjectArrayElement(result, index, inetAddress.get());
        ++index;
    }
    return result;
}

static jint Posix_getegid(JNIEnv*, jobject) {
    return getegid();
}

static jint Posix_geteuid(JNIEnv*, jobject) {
    return geteuid();
}

static jint Posix_getgid(JNIEnv*, jobject) {
    return getgid();
}

static jstring Posix_getenv(JNIEnv* env, jobject, jstring javaName) {
    ScopedUtfChars name(env, javaName);
    if (name.c_str() == NULL) {
        return NULL;
    }
    return env->NewStringUTF(getenv(name.c_str()));
}

static jstring Posix_getnameinfo(JNIEnv* env, jobject, jobject javaAddress, jint flags) {
    sockaddr_storage ss;
    socklen_t sa_len;
    if (!inetAddressToSockaddrVerbatim(env, javaAddress, 0, ss, sa_len)) {
        return NULL;
    }
    char buf[NI_MAXHOST]; // NI_MAXHOST is longer than INET6_ADDRSTRLEN.
    errno = 0;
    int rc = getnameinfo(reinterpret_cast<sockaddr*>(&ss), sa_len, buf, sizeof(buf), NULL, 0, flags);
    if (rc != 0) {
        throwGaiException(env, "getnameinfo", rc);
        return NULL;
    }
    return env->NewStringUTF(buf);
}

static jobject Posix_getpeername(JNIEnv* env, jobject, jobject javaFd) {
  return doGetSockName(env, javaFd, false);
}

static jint Posix_getpid(JNIEnv*, jobject) {
    return getpid();
}

static jint Posix_getppid(JNIEnv*, jobject) {
    return getppid();
}

static jobject Posix_getpwnam(JNIEnv* env, jobject, jstring javaName) {
    ScopedUtfChars name(env, javaName);
    if (name.c_str() == NULL) {
        return NULL;
    }
    return Passwd(env).getpwnam(name.c_str());
}

static jobject Posix_getpwuid(JNIEnv* env, jobject, jint uid) {
    return Passwd(env).getpwuid(uid);
}

static jobject Posix_getsockname(JNIEnv* env, jobject, jobject javaFd) {
  return doGetSockName(env, javaFd, true);
}

static jint Posix_getsockoptByte(JNIEnv* env, jobject, jobject javaFd, jint level, jint option) {
    int fd = jniGetFDFromFileDescriptor(env, javaFd);
    u_char result = 0;
    socklen_t size = sizeof(result);
    throwIfMinusOne(env, "getsockopt", TEMP_FAILURE_RETRY(getsockopt(fd, level, option, &result, &size)));
    return result;
}

static jobject Posix_getsockoptInAddr(JNIEnv* env, jobject, jobject javaFd, jint level, jint option) {
    int fd = jniGetFDFromFileDescriptor(env, javaFd);
    sockaddr_storage ss;
    memset(&ss, 0, sizeof(ss));
    ss.ss_family = AF_INET; // This is only for the IPv4-only IP_MULTICAST_IF.
    sockaddr_in* sa = reinterpret_cast<sockaddr_in*>(&ss);
    socklen_t size = sizeof(sa->sin_addr);
    int rc = TEMP_FAILURE_RETRY(getsockopt(fd, level, option, &sa->sin_addr, &size));
    if (rc == -1) {
        throwErrnoException(env, "getsockopt");
        return NULL;
    }
    return sockaddrToInetAddress(env, ss, NULL);
}

static jint Posix_getsockoptInt(JNIEnv* env, jobject, jobject javaFd, jint level, jint option) {
    int fd = jniGetFDFromFileDescriptor(env, javaFd);
    jint result = 0;
    socklen_t size = sizeof(result);
    throwIfMinusOne(env, "getsockopt", TEMP_FAILURE_RETRY(getsockopt(fd, level, option, &result, &size)));
    return result;
}

static jobject Posix_getsockoptLinger(JNIEnv* env, jobject, jobject javaFd, jint level, jint option) {
    int fd = jniGetFDFromFileDescriptor(env, javaFd);
    struct linger l;
    socklen_t size = sizeof(l);
    memset(&l, 0, size);
    int rc = TEMP_FAILURE_RETRY(getsockopt(fd, level, option, &l, &size));
    if (rc == -1) {
        throwErrnoException(env, "getsockopt");
        return NULL;
    }
    return makeStructLinger(env, l);
}

static jobject Posix_getsockoptTimeval(JNIEnv* env, jobject, jobject javaFd, jint level, jint option) {
    int fd = jniGetFDFromFileDescriptor(env, javaFd);
    struct timeval tv;
    socklen_t size = sizeof(tv);
    memset(&tv, 0, size);
    int rc = TEMP_FAILURE_RETRY(getsockopt(fd, level, option, &tv, &size));
    if (rc == -1) {
        throwErrnoException(env, "getsockopt");
        return NULL;
    }
    return makeStructTimeval(env, tv);
}

static jobject Posix_getsockoptUcred(JNIEnv* env, jobject, jobject javaFd, jint level, jint option) {
  int fd = jniGetFDFromFileDescriptor(env, javaFd);
  struct ucred u;
  socklen_t size = sizeof(u);
  memset(&u, 0, size);
  int rc = TEMP_FAILURE_RETRY(getsockopt(fd, level, option, &u, &size));
  if (rc == -1) {
    throwErrnoException(env, "getsockopt");
    return NULL;
  }
  return makeStructUcred(env, u);
}

static jint Posix_gettid(JNIEnv* env __unused, jobject) {
#if defined(__APPLE__)
  uint64_t owner;
  int rc = pthread_threadid_np(NULL, &owner);  // Requires Mac OS 10.6
  if (rc != 0) {
    throwErrnoException(env, "gettid");
    return 0;
  }
  return static_cast<jint>(owner);
#else
  // Neither bionic nor glibc exposes gettid(2).
  return syscall(__NR_gettid);
#endif
}

static jint Posix_getuid(JNIEnv*, jobject) {
    return getuid();
}

static jstring Posix_if_indextoname(JNIEnv* env, jobject, jint index) {
    char buf[IF_NAMESIZE];
    char* name = if_indextoname(index, buf);
    // if_indextoname(3) returns NULL on failure, which will come out of NewStringUTF unscathed.
    // There's no useful information in errno, so we don't bother throwing. Callers can null-check.
    return env->NewStringUTF(name);
}

static jobject Posix_inet_pton(JNIEnv* env, jobject, jint family, jstring javaName) {
    ScopedUtfChars name(env, javaName);
    if (name.c_str() == NULL) {
        return NULL;
    }
    sockaddr_storage ss;
    memset(&ss, 0, sizeof(ss));
    // sockaddr_in and sockaddr_in6 are at the same address, so we can use either here.
    void* dst = &reinterpret_cast<sockaddr_in*>(&ss)->sin_addr;
    if (inet_pton(family, name.c_str(), dst) != 1) {
        return NULL;
    }
    ss.ss_family = family;
    return sockaddrToInetAddress(env, ss, NULL);
}

static jobject Posix_ioctlInetAddress(JNIEnv* env, jobject, jobject javaFd, jint cmd, jstring javaInterfaceName) {
    struct ifreq req;
    if (!fillIfreq(env, javaInterfaceName, req)) {
        return NULL;
    }
    int fd = jniGetFDFromFileDescriptor(env, javaFd);
    int rc = throwIfMinusOne(env, "ioctl", TEMP_FAILURE_RETRY(ioctl(fd, cmd, &req)));
    if (rc == -1) {
        return NULL;
    }
    return sockaddrToInetAddress(env, reinterpret_cast<sockaddr_storage&>(req.ifr_addr), NULL);
}

static jint Posix_ioctlInt(JNIEnv* env, jobject, jobject javaFd, jint cmd, jobject javaArg) {
    // This is complicated because ioctls may return their result by updating their argument
    // or via their return value, so we need to support both.
    int fd = jniGetFDFromFileDescriptor(env, javaFd);
    static jfieldID valueFid = env->GetFieldID(JniConstants::mutableIntClass, "value", "I");
    jint arg = env->GetIntField(javaArg, valueFid);
    int rc = throwIfMinusOne(env, "ioctl", TEMP_FAILURE_RETRY(ioctl(fd, cmd, &arg)));
    if (!env->ExceptionCheck()) {
        env->SetIntField(javaArg, valueFid, arg);
    }
    return rc;
}

static jboolean Posix_isatty(JNIEnv* env, jobject, jobject javaFd) {
    int fd = jniGetFDFromFileDescriptor(env, javaFd);
    return TEMP_FAILURE_RETRY(isatty(fd)) == 1;
}

static void Posix_kill(JNIEnv* env, jobject, jint pid, jint sig) {
    throwIfMinusOne(env, "kill", TEMP_FAILURE_RETRY(kill(pid, sig)));
}

static void Posix_lchown(JNIEnv* env, jobject, jstring javaPath, jint uid, jint gid) {
    ScopedUtfChars path(env, javaPath);
    if (path.c_str() == NULL) {
        return;
    }
    throwIfMinusOne(env, "lchown", TEMP_FAILURE_RETRY(lchown(path.c_str(), uid, gid)));
}

static void Posix_link(JNIEnv* env, jobject, jstring javaOldPath, jstring javaNewPath) {
    ScopedUtfChars oldPath(env, javaOldPath);
    if (oldPath.c_str() == NULL) {
        return;
    }
    ScopedUtfChars newPath(env, javaNewPath);
    if (newPath.c_str() == NULL) {
        return;
    }
    throwIfMinusOne(env, "link", TEMP_FAILURE_RETRY(link(oldPath.c_str(), newPath.c_str())));
}

static void Posix_listen(JNIEnv* env, jobject, jobject javaFd, jint backlog) {
    int fd = jniGetFDFromFileDescriptor(env, javaFd);
    throwIfMinusOne(env, "listen", TEMP_FAILURE_RETRY(listen(fd, backlog)));
}

static jlong Posix_lseek(JNIEnv* env, jobject, jobject javaFd, jlong offset, jint whence) {
    int fd = jniGetFDFromFileDescriptor(env, javaFd);
    return throwIfMinusOne(env, "lseek", TEMP_FAILURE_RETRY(lseek64(fd, offset, whence)));
}

static jobject Posix_lstat(JNIEnv* env, jobject, jstring javaPath) {
    return doStat(env, javaPath, true);
}

static void Posix_mincore(JNIEnv* env, jobject, jlong address, jlong byteCount, jbyteArray javaVector) {
    ScopedByteArrayRW vector(env, javaVector);
    if (vector.get() == NULL) {
        return;
    }
    void* ptr = reinterpret_cast<void*>(static_cast<uintptr_t>(address));
    unsigned char* vec = reinterpret_cast<unsigned char*>(vector.get());
    throwIfMinusOne(env, "mincore", TEMP_FAILURE_RETRY(mincore(ptr, byteCount, vec)));
}

static void Posix_mkdir(JNIEnv* env, jobject, jstring javaPath, jint mode) {
    ScopedUtfChars path(env, javaPath);
    if (path.c_str() == NULL) {
        return;
    }
    throwIfMinusOne(env, "mkdir", TEMP_FAILURE_RETRY(mkdir(path.c_str(), mode)));
}

static void Posix_mkfifo(JNIEnv* env, jobject, jstring javaPath, jint mode) {
    ScopedUtfChars path(env, javaPath);
    if (path.c_str() == NULL) {
        return;
    }
    throwIfMinusOne(env, "mkfifo", TEMP_FAILURE_RETRY(mkfifo(path.c_str(), mode)));
}

static void Posix_mlock(JNIEnv* env, jobject, jlong address, jlong byteCount) {
    void* ptr = reinterpret_cast<void*>(static_cast<uintptr_t>(address));
    throwIfMinusOne(env, "mlock", TEMP_FAILURE_RETRY(mlock(ptr, byteCount)));
}

static jlong Posix_mmap(JNIEnv* env, jobject, jlong address, jlong byteCount, jint prot, jint flags, jobject javaFd, jlong offset) {
    int fd = jniGetFDFromFileDescriptor(env, javaFd);
    void* suggestedPtr = reinterpret_cast<void*>(static_cast<uintptr_t>(address));
    void* ptr = mmap(suggestedPtr, byteCount, prot, flags, fd, offset);
    if (ptr == MAP_FAILED) {
        throwErrnoException(env, "mmap");
    }
    return static_cast<jlong>(reinterpret_cast<uintptr_t>(ptr));
}

static void Posix_msync(JNIEnv* env, jobject, jlong address, jlong byteCount, jint flags) {
    void* ptr = reinterpret_cast<void*>(static_cast<uintptr_t>(address));
    throwIfMinusOne(env, "msync", TEMP_FAILURE_RETRY(msync(ptr, byteCount, flags)));
}

static void Posix_munlock(JNIEnv* env, jobject, jlong address, jlong byteCount) {
    void* ptr = reinterpret_cast<void*>(static_cast<uintptr_t>(address));
    throwIfMinusOne(env, "munlock", TEMP_FAILURE_RETRY(munlock(ptr, byteCount)));
}

static void Posix_munmap(JNIEnv* env, jobject, jlong address, jlong byteCount) {
    void* ptr = reinterpret_cast<void*>(static_cast<uintptr_t>(address));
    throwIfMinusOne(env, "munmap", TEMP_FAILURE_RETRY(munmap(ptr, byteCount)));
}

static jobject Posix_open(JNIEnv* env, jobject, jstring javaPath, jint flags, jint mode) {
    ScopedUtfChars path(env, javaPath);
    if (path.c_str() == NULL) {
        return NULL;
    }
    int fd = throwIfMinusOne(env, "open", TEMP_FAILURE_RETRY(open(path.c_str(), flags, mode)));
    return fd != -1 ? jniCreateFileDescriptor(env, fd) : NULL;
}

static jobjectArray Posix_pipe(JNIEnv* env, jobject) {
    int fds[2];
    throwIfMinusOne(env, "pipe", TEMP_FAILURE_RETRY(pipe(&fds[0])));
    jobjectArray result = env->NewObjectArray(2, JniConstants::fileDescriptorClass, NULL);
    if (result == NULL) {
        return NULL;
    }
    for (int i = 0; i < 2; ++i) {
        ScopedLocalRef<jobject> fd(env, jniCreateFileDescriptor(env, fds[i]));
        if (fd.get() == NULL) {
            return NULL;
        }
        env->SetObjectArrayElement(result, i, fd.get());
        if (env->ExceptionCheck()) {
            return NULL;
        }
    }
    return result;
}

static jint Posix_poll(JNIEnv* env, jobject, jobjectArray javaStructs, jint timeoutMs) {
    static jfieldID fdFid = env->GetFieldID(JniConstants::structPollfdClass, "fd", "Ljava/io/FileDescriptor;");
    static jfieldID eventsFid = env->GetFieldID(JniConstants::structPollfdClass, "events", "S");
    static jfieldID reventsFid = env->GetFieldID(JniConstants::structPollfdClass, "revents", "S");

    // Turn the Java android.system.StructPollfd[] into a C++ struct pollfd[].
    size_t arrayLength = env->GetArrayLength(javaStructs);
    UniquePtr<struct pollfd[]> fds(new struct pollfd[arrayLength]);
    memset(fds.get(), 0, sizeof(struct pollfd) * arrayLength);
    size_t count = 0; // Some trailing array elements may be irrelevant. (See below.)
    for (size_t i = 0; i < arrayLength; ++i) {
        ScopedLocalRef<jobject> javaStruct(env, env->GetObjectArrayElement(javaStructs, i));
        if (javaStruct.get() == NULL) {
            break; // We allow trailing nulls in the array for caller convenience.
        }
        ScopedLocalRef<jobject> javaFd(env, env->GetObjectField(javaStruct.get(), fdFid));
        if (javaFd.get() == NULL) {
            break; // We also allow callers to just clear the fd field (this is what Selector does).
        }
        fds[count].fd = jniGetFDFromFileDescriptor(env, javaFd.get());
        fds[count].events = env->GetShortField(javaStruct.get(), eventsFid);
        ++count;
    }

    std::vector<AsynchronousCloseMonitor*> monitors;
    for (size_t i = 0; i < count; ++i) {
        monitors.push_back(new AsynchronousCloseMonitor(fds[i].fd));
    }
    int rc = poll(fds.get(), count, timeoutMs);
    for (size_t i = 0; i < monitors.size(); ++i) {
        delete monitors[i];
    }
    if (rc == -1) {
        throwErrnoException(env, "poll");
        return -1;
    }

    // Update the revents fields in the Java android.system.StructPollfd[].
    for (size_t i = 0; i < count; ++i) {
        ScopedLocalRef<jobject> javaStruct(env, env->GetObjectArrayElement(javaStructs, i));
        if (javaStruct.get() == NULL) {
            return -1;
        }
        env->SetShortField(javaStruct.get(), reventsFid, fds[i].revents);
    }
    return rc;
}

static void Posix_posix_fallocate(JNIEnv* env, jobject, jobject javaFd __unused,
                                  jlong offset __unused, jlong length __unused) {
#ifdef __APPLE__
    jniThrowException(env, "java/lang/UnsupportedOperationException",
                      "fallocate doesn't exist on a Mac");
#else
    int fd = jniGetFDFromFileDescriptor(env, javaFd);
    errno = TEMP_FAILURE_RETRY(posix_fallocate64(fd, offset, length));
    if (errno != 0) {
        throwErrnoException(env, "posix_fallocate");
    }
#endif
}

static jint Posix_prctl(JNIEnv* env, jobject, jint option __unused, jlong arg2 __unused,
                        jlong arg3 __unused, jlong arg4 __unused, jlong arg5 __unused) {
#ifdef __APPLE__
    jniThrowException(env, "java/lang/UnsupportedOperationException", "prctl doesn't exist on a Mac");
    return 0;
#else
    int result = prctl(static_cast<int>(option),
                       static_cast<unsigned long>(arg2), static_cast<unsigned long>(arg3),
                       static_cast<unsigned long>(arg4), static_cast<unsigned long>(arg5));
    return throwIfMinusOne(env, "prctl", result);
#endif
}

static jint Posix_preadBytes(JNIEnv* env, jobject, jobject javaFd, jobject javaBytes, jint byteOffset, jint byteCount, jlong offset) {
    ScopedBytesRW bytes(env, javaBytes);
    if (bytes.get() == NULL) {
        return -1;
    }
    return IO_FAILURE_RETRY(env, ssize_t, pread64, javaFd, bytes.get() + byteOffset, byteCount, offset);
}

static jint Posix_pwriteBytes(JNIEnv* env, jobject, jobject javaFd, jbyteArray javaBytes, jint byteOffset, jint byteCount, jlong offset) {
    ScopedBytesRO bytes(env, javaBytes);
    if (bytes.get() == NULL) {
        return -1;
    }
    return IO_FAILURE_RETRY(env, ssize_t, pwrite64, javaFd, bytes.get() + byteOffset, byteCount, offset);
}

static jint Posix_readBytes(JNIEnv* env, jobject, jobject javaFd, jobject javaBytes, jint byteOffset, jint byteCount) {
    ScopedBytesRW bytes(env, javaBytes);
    if (bytes.get() == NULL) {
        return -1;
    }
    return IO_FAILURE_RETRY(env, ssize_t, read, javaFd, bytes.get() + byteOffset, byteCount);
}

static jstring Posix_readlink(JNIEnv* env, jobject, jstring javaPath) {
    ScopedUtfChars path(env, javaPath);
    if (path.c_str() == NULL) {
        return NULL;
    }

    std::string result;
    if (!readlink(path.c_str(), result)) {
        throwErrnoException(env, "readlink");
        return NULL;
    }
    return env->NewStringUTF(result.c_str());
}

static jint Posix_readv(JNIEnv* env, jobject, jobject javaFd, jobjectArray buffers, jintArray offsets, jintArray byteCounts) {
    IoVec<ScopedBytesRW> ioVec(env, env->GetArrayLength(buffers));
    if (!ioVec.init(buffers, offsets, byteCounts)) {
        return -1;
    }
    return IO_FAILURE_RETRY(env, ssize_t, readv, javaFd, ioVec.get(), ioVec.size());
}

static jint Posix_recvfromBytes(JNIEnv* env, jobject, jobject javaFd, jobject javaBytes, jint byteOffset, jint byteCount, jint flags, jobject javaInetSocketAddress) {
    ScopedBytesRW bytes(env, javaBytes);
    if (bytes.get() == NULL) {
        return -1;
    }
    sockaddr_storage ss;
    socklen_t sl = sizeof(ss);
    memset(&ss, 0, sizeof(ss));
    sockaddr* from = (javaInetSocketAddress != NULL) ? reinterpret_cast<sockaddr*>(&ss) : NULL;
    socklen_t* fromLength = (javaInetSocketAddress != NULL) ? &sl : 0;
    jint recvCount = NET_FAILURE_RETRY(env, ssize_t, recvfrom, javaFd, bytes.get() + byteOffset, byteCount, flags, from, fromLength);
    fillInetSocketAddress(env, recvCount, javaInetSocketAddress, ss);
    return recvCount;
}

static void Posix_remove(JNIEnv* env, jobject, jstring javaPath) {
    ScopedUtfChars path(env, javaPath);
    if (path.c_str() == NULL) {
        return;
    }
    throwIfMinusOne(env, "remove", TEMP_FAILURE_RETRY(remove(path.c_str())));
}

static void Posix_rename(JNIEnv* env, jobject, jstring javaOldPath, jstring javaNewPath) {
    ScopedUtfChars oldPath(env, javaOldPath);
    if (oldPath.c_str() == NULL) {
        return;
    }
    ScopedUtfChars newPath(env, javaNewPath);
    if (newPath.c_str() == NULL) {
        return;
    }
    throwIfMinusOne(env, "rename", TEMP_FAILURE_RETRY(rename(oldPath.c_str(), newPath.c_str())));
}

static jlong Posix_sendfile(JNIEnv* env, jobject, jobject javaOutFd, jobject javaInFd, jobject javaOffset, jlong byteCount) {
    int outFd = jniGetFDFromFileDescriptor(env, javaOutFd);
    int inFd = jniGetFDFromFileDescriptor(env, javaInFd);
    static jfieldID valueFid = env->GetFieldID(JniConstants::mutableLongClass, "value", "J");
    off_t offset = 0;
    off_t* offsetPtr = NULL;
    if (javaOffset != NULL) {
        // TODO: fix bionic so we can have a 64-bit off_t!
        offset = env->GetLongField(javaOffset, valueFid);
        offsetPtr = &offset;
    }
    jlong result = throwIfMinusOne(env, "sendfile", TEMP_FAILURE_RETRY(sendfile(outFd, inFd, offsetPtr, byteCount)));
    if (javaOffset != NULL) {
        env->SetLongField(javaOffset, valueFid, offset);
    }
    return result;
}

static jint Posix_sendtoBytes(JNIEnv* env, jobject, jobject javaFd, jobject javaBytes, jint byteOffset, jint byteCount, jint flags, jobject javaInetAddress, jint port) {
    ScopedBytesRO bytes(env, javaBytes);
    if (bytes.get() == NULL) {
        return -1;
    }
    sockaddr_storage ss;
    socklen_t sa_len = 0;
    if (javaInetAddress != NULL && !inetAddressToSockaddr(env, javaInetAddress, port, ss, sa_len)) {
        return -1;
    }
    const sockaddr* to = (javaInetAddress != NULL) ? reinterpret_cast<const sockaddr*>(&ss) : NULL;
    return NET_FAILURE_RETRY(env, ssize_t, sendto, javaFd, bytes.get() + byteOffset, byteCount, flags, to, sa_len);
}

static void Posix_setegid(JNIEnv* env, jobject, jint egid) {
    throwIfMinusOne(env, "setegid", TEMP_FAILURE_RETRY(setegid(egid)));
}

static void Posix_setenv(JNIEnv* env, jobject, jstring javaName, jstring javaValue, jboolean overwrite) {
    ScopedUtfChars name(env, javaName);
    if (name.c_str() == NULL) {
        return;
    }
    ScopedUtfChars value(env, javaValue);
    if (value.c_str() == NULL) {
        return;
    }
    throwIfMinusOne(env, "setenv", setenv(name.c_str(), value.c_str(), overwrite));
}

static void Posix_seteuid(JNIEnv* env, jobject, jint euid) {
    throwIfMinusOne(env, "seteuid", TEMP_FAILURE_RETRY(seteuid(euid)));
}

static void Posix_setgid(JNIEnv* env, jobject, jint gid) {
    throwIfMinusOne(env, "setgid", TEMP_FAILURE_RETRY(setgid(gid)));
}

static jint Posix_setsid(JNIEnv* env, jobject) {
    return throwIfMinusOne(env, "setsid", TEMP_FAILURE_RETRY(setsid()));
}

static void Posix_setsockoptByte(JNIEnv* env, jobject, jobject javaFd, jint level, jint option, jint value) {
    int fd = jniGetFDFromFileDescriptor(env, javaFd);
    u_char byte = value;
    throwIfMinusOne(env, "setsockopt", TEMP_FAILURE_RETRY(setsockopt(fd, level, option, &byte, sizeof(byte))));
}

static void Posix_setsockoptIfreq(JNIEnv* env, jobject, jobject javaFd, jint level, jint option, jstring javaInterfaceName) {
    struct ifreq req;
    if (!fillIfreq(env, javaInterfaceName, req)) {
        return;
    }
    int fd = jniGetFDFromFileDescriptor(env, javaFd);
    throwIfMinusOne(env, "setsockopt", TEMP_FAILURE_RETRY(setsockopt(fd, level, option, &req, sizeof(req))));
}

static void Posix_setsockoptInt(JNIEnv* env, jobject, jobject javaFd, jint level, jint option, jint value) {
    int fd = jniGetFDFromFileDescriptor(env, javaFd);
    throwIfMinusOne(env, "setsockopt", TEMP_FAILURE_RETRY(setsockopt(fd, level, option, &value, sizeof(value))));
}

#if defined(__APPLE__) && MAC_OS_X_VERSION_MAX_ALLOWED < 1070
// Mac OS didn't support modern multicast APIs until 10.7.
static void Posix_setsockoptIpMreqn(JNIEnv*, jobject, jobject, jint, jint, jint) { abort(); }
static void Posix_setsockoptGroupReq(JNIEnv*, jobject, jobject, jint, jint, jobject) { abort(); }
static void Posix_setsockoptGroupSourceReq(JNIEnv*, jobject, jobject, jint, jint, jobject) { abort(); }
#else
static void Posix_setsockoptIpMreqn(JNIEnv* env, jobject, jobject javaFd, jint level, jint option, jint value) {
    ip_mreqn req;
    memset(&req, 0, sizeof(req));
    req.imr_ifindex = value;
    int fd = jniGetFDFromFileDescriptor(env, javaFd);
    throwIfMinusOne(env, "setsockopt", TEMP_FAILURE_RETRY(setsockopt(fd, level, option, &req, sizeof(req))));
}

static void Posix_setsockoptGroupReq(JNIEnv* env, jobject, jobject javaFd, jint level, jint option, jobject javaGroupReq) {
    struct group_req req;
    memset(&req, 0, sizeof(req));

    static jfieldID grInterfaceFid = env->GetFieldID(JniConstants::structGroupReqClass, "gr_interface", "I");
    req.gr_interface = env->GetIntField(javaGroupReq, grInterfaceFid);
    // Get the IPv4 or IPv6 multicast address to join or leave.
    static jfieldID grGroupFid = env->GetFieldID(JniConstants::structGroupReqClass, "gr_group", "Ljava/net/InetAddress;");
    ScopedLocalRef<jobject> javaGroup(env, env->GetObjectField(javaGroupReq, grGroupFid));
    socklen_t sa_len;
    if (!inetAddressToSockaddrVerbatim(env, javaGroup.get(), 0, req.gr_group, sa_len)) {
        return;
    }

    int fd = jniGetFDFromFileDescriptor(env, javaFd);
    int rc = TEMP_FAILURE_RETRY(setsockopt(fd, level, option, &req, sizeof(req)));
    if (rc == -1 && errno == EINVAL) {
        // Maybe we're a 32-bit binary talking to a 64-bit kernel?
        // glibc doesn't automatically handle this.
        // http://sourceware.org/bugzilla/show_bug.cgi?id=12080
        struct group_req64 {
            uint32_t gr_interface;
            uint32_t my_padding;
            sockaddr_storage gr_group;
        };
        group_req64 req64;
        req64.gr_interface = req.gr_interface;
        memcpy(&req64.gr_group, &req.gr_group, sizeof(req.gr_group));
        rc = TEMP_FAILURE_RETRY(setsockopt(fd, level, option, &req64, sizeof(req64)));
    }
    throwIfMinusOne(env, "setsockopt", rc);
}

static void Posix_setsockoptGroupSourceReq(JNIEnv* env, jobject, jobject javaFd, jint level, jint option, jobject javaGroupSourceReq) {
    socklen_t sa_len;
    struct group_source_req req;
    memset(&req, 0, sizeof(req));

    static jfieldID gsrInterfaceFid = env->GetFieldID(JniConstants::structGroupSourceReqClass, "gsr_interface", "I");
    req.gsr_interface = env->GetIntField(javaGroupSourceReq, gsrInterfaceFid);
    // Get the IPv4 or IPv6 multicast address to join or leave.
    static jfieldID gsrGroupFid = env->GetFieldID(JniConstants::structGroupSourceReqClass, "gsr_group", "Ljava/net/InetAddress;");
    ScopedLocalRef<jobject> javaGroup(env, env->GetObjectField(javaGroupSourceReq, gsrGroupFid));
    if (!inetAddressToSockaddrVerbatim(env, javaGroup.get(), 0, req.gsr_group, sa_len)) {
        return;
    }

    // Get the IPv4 or IPv6 multicast address to add to the filter.
    static jfieldID gsrSourceFid = env->GetFieldID(JniConstants::structGroupSourceReqClass, "gsr_source", "Ljava/net/InetAddress;");
    ScopedLocalRef<jobject> javaSource(env, env->GetObjectField(javaGroupSourceReq, gsrSourceFid));
    if (!inetAddressToSockaddrVerbatim(env, javaSource.get(), 0, req.gsr_source, sa_len)) {
        return;
    }

    int fd = jniGetFDFromFileDescriptor(env, javaFd);
    int rc = TEMP_FAILURE_RETRY(setsockopt(fd, level, option, &req, sizeof(req)));
    if (rc == -1 && errno == EINVAL) {
        // Maybe we're a 32-bit binary talking to a 64-bit kernel?
        // glibc doesn't automatically handle this.
        // http://sourceware.org/bugzilla/show_bug.cgi?id=12080
        struct group_source_req64 {
            uint32_t gsr_interface;
            uint32_t my_padding;
            sockaddr_storage gsr_group;
            sockaddr_storage gsr_source;
        };
        group_source_req64 req64;
        req64.gsr_interface = req.gsr_interface;
        memcpy(&req64.gsr_group, &req.gsr_group, sizeof(req.gsr_group));
        memcpy(&req64.gsr_source, &req.gsr_source, sizeof(req.gsr_source));
        rc = TEMP_FAILURE_RETRY(setsockopt(fd, level, option, &req64, sizeof(req64)));
    }
    throwIfMinusOne(env, "setsockopt", rc);
}
#endif

static void Posix_setsockoptLinger(JNIEnv* env, jobject, jobject javaFd, jint level, jint option, jobject javaLinger) {
    static jfieldID lOnoffFid = env->GetFieldID(JniConstants::structLingerClass, "l_onoff", "I");
    static jfieldID lLingerFid = env->GetFieldID(JniConstants::structLingerClass, "l_linger", "I");
    int fd = jniGetFDFromFileDescriptor(env, javaFd);
    struct linger value;
    value.l_onoff = env->GetIntField(javaLinger, lOnoffFid);
    value.l_linger = env->GetIntField(javaLinger, lLingerFid);
    throwIfMinusOne(env, "setsockopt", TEMP_FAILURE_RETRY(setsockopt(fd, level, option, &value, sizeof(value))));
}

static void Posix_setsockoptTimeval(JNIEnv* env, jobject, jobject javaFd, jint level, jint option, jobject javaTimeval) {
    static jfieldID tvSecFid = env->GetFieldID(JniConstants::structTimevalClass, "tv_sec", "J");
    static jfieldID tvUsecFid = env->GetFieldID(JniConstants::structTimevalClass, "tv_usec", "J");
    int fd = jniGetFDFromFileDescriptor(env, javaFd);
    struct timeval value;
    value.tv_sec = env->GetLongField(javaTimeval, tvSecFid);
    value.tv_usec = env->GetLongField(javaTimeval, tvUsecFid);
    throwIfMinusOne(env, "setsockopt", TEMP_FAILURE_RETRY(setsockopt(fd, level, option, &value, sizeof(value))));
}

static void Posix_setuid(JNIEnv* env, jobject, jint uid) {
    throwIfMinusOne(env, "setuid", TEMP_FAILURE_RETRY(setuid(uid)));
}

static void Posix_shutdown(JNIEnv* env, jobject, jobject javaFd, jint how) {
    int fd = jniGetFDFromFileDescriptor(env, javaFd);
    throwIfMinusOne(env, "shutdown", TEMP_FAILURE_RETRY(shutdown(fd, how)));
}

static jobject Posix_socket(JNIEnv* env, jobject, jint domain, jint type, jint protocol) {
    int fd = throwIfMinusOne(env, "socket", TEMP_FAILURE_RETRY(socket(domain, type, protocol)));
    return fd != -1 ? jniCreateFileDescriptor(env, fd) : NULL;
}

static void Posix_socketpair(JNIEnv* env, jobject, jint domain, jint type, jint protocol, jobject javaFd1, jobject javaFd2) {
    int fds[2];
    int rc = throwIfMinusOne(env, "socketpair", TEMP_FAILURE_RETRY(socketpair(domain, type, protocol, fds)));
    if (rc != -1) {
        jniSetFileDescriptorOfFD(env, javaFd1, fds[0]);
        jniSetFileDescriptorOfFD(env, javaFd2, fds[1]);
    }
}

static jobject Posix_stat(JNIEnv* env, jobject, jstring javaPath) {
    return doStat(env, javaPath, false);
}

static jobject Posix_statvfs(JNIEnv* env, jobject, jstring javaPath) {
    ScopedUtfChars path(env, javaPath);
    if (path.c_str() == NULL) {
        return NULL;
    }
    struct statvfs sb;
    int rc = TEMP_FAILURE_RETRY(statvfs(path.c_str(), &sb));
    if (rc == -1) {
        throwErrnoException(env, "statvfs");
        return NULL;
    }
    return makeStructStatVfs(env, sb);
}

static jstring Posix_strerror(JNIEnv* env, jobject, jint errnum) {
    char buffer[BUFSIZ];
    const char* message = jniStrError(errnum, buffer, sizeof(buffer));
    return env->NewStringUTF(message);
}

static jstring Posix_strsignal(JNIEnv* env, jobject, jint signal) {
    return env->NewStringUTF(strsignal(signal));
}

static void Posix_symlink(JNIEnv* env, jobject, jstring javaOldPath, jstring javaNewPath) {
    ScopedUtfChars oldPath(env, javaOldPath);
    if (oldPath.c_str() == NULL) {
        return;
    }
    ScopedUtfChars newPath(env, javaNewPath);
    if (newPath.c_str() == NULL) {
        return;
    }
    throwIfMinusOne(env, "symlink", TEMP_FAILURE_RETRY(symlink(oldPath.c_str(), newPath.c_str())));
}

static jlong Posix_sysconf(JNIEnv* env, jobject, jint name) {
    // Since -1 is a valid result from sysconf(3), detecting failure is a little more awkward.
    errno = 0;
    long result = sysconf(name);
    if (result == -1L && errno == EINVAL) {
        throwErrnoException(env, "sysconf");
    }
    return result;
}

static void Posix_tcdrain(JNIEnv* env, jobject, jobject javaFd) {
    int fd = jniGetFDFromFileDescriptor(env, javaFd);
    throwIfMinusOne(env, "tcdrain", TEMP_FAILURE_RETRY(tcdrain(fd)));
}

static void Posix_tcsendbreak(JNIEnv* env, jobject, jobject javaFd, jint duration) {
  int fd = jniGetFDFromFileDescriptor(env, javaFd);
  throwIfMinusOne(env, "tcsendbreak", TEMP_FAILURE_RETRY(tcsendbreak(fd, duration)));
}

static jint Posix_umaskImpl(JNIEnv*, jobject, jint mask) {
    return umask(mask);
}

static jobject Posix_uname(JNIEnv* env, jobject) {
    struct utsname buf;
    if (TEMP_FAILURE_RETRY(uname(&buf)) == -1) {
        return NULL; // Can't happen.
    }
    return makeStructUtsname(env, buf);
}

static void Posix_unsetenv(JNIEnv* env, jobject, jstring javaName) {
    ScopedUtfChars name(env, javaName);
    if (name.c_str() == NULL) {
        return;
    }
    throwIfMinusOne(env, "unsetenv", unsetenv(name.c_str()));
}

static jint Posix_waitpid(JNIEnv* env, jobject, jint pid, jobject javaStatus, jint options) {
    int status;
    int rc = throwIfMinusOne(env, "waitpid", TEMP_FAILURE_RETRY(waitpid(pid, &status, options)));
    if (rc != -1) {
        static jfieldID valueFid = env->GetFieldID(JniConstants::mutableIntClass, "value", "I");
        env->SetIntField(javaStatus, valueFid, status);
    }
    return rc;
}

static jint Posix_writeBytes(JNIEnv* env, jobject, jobject javaFd, jbyteArray javaBytes, jint byteOffset, jint byteCount) {
    ScopedBytesRO bytes(env, javaBytes);
    if (bytes.get() == NULL) {
        return -1;
    }
    return IO_FAILURE_RETRY(env, ssize_t, write, javaFd, bytes.get() + byteOffset, byteCount);
}

static jint Posix_writev(JNIEnv* env, jobject, jobject javaFd, jobjectArray buffers, jintArray offsets, jintArray byteCounts) {
    IoVec<ScopedBytesRO> ioVec(env, env->GetArrayLength(buffers));
    if (!ioVec.init(buffers, offsets, byteCounts)) {
        return -1;
    }
    return IO_FAILURE_RETRY(env, ssize_t, writev, javaFd, ioVec.get(), ioVec.size());
}

static JNINativeMethod gMethods[] = {
    NATIVE_METHOD(Posix, accept, "(Ljava/io/FileDescriptor;Ljava/net/InetSocketAddress;)Ljava/io/FileDescriptor;"),
    NATIVE_METHOD(Posix, access, "(Ljava/lang/String;I)Z"),
    NATIVE_METHOD(Posix, android_getaddrinfo, "(Ljava/lang/String;Landroid/system/StructAddrinfo;I)[Ljava/net/InetAddress;"),
    NATIVE_METHOD(Posix, bind, "(Ljava/io/FileDescriptor;Ljava/net/InetAddress;I)V"),
    NATIVE_METHOD(Posix, chmod, "(Ljava/lang/String;I)V"),
    NATIVE_METHOD(Posix, chown, "(Ljava/lang/String;II)V"),
    NATIVE_METHOD(Posix, close, "(Ljava/io/FileDescriptor;)V"),
    NATIVE_METHOD(Posix, connect, "(Ljava/io/FileDescriptor;Ljava/net/InetAddress;I)V"),
    NATIVE_METHOD(Posix, dup, "(Ljava/io/FileDescriptor;)Ljava/io/FileDescriptor;"),
    NATIVE_METHOD(Posix, dup2, "(Ljava/io/FileDescriptor;I)Ljava/io/FileDescriptor;"),
    NATIVE_METHOD(Posix, environ, "()[Ljava/lang/String;"),
    NATIVE_METHOD(Posix, execv, "(Ljava/lang/String;[Ljava/lang/String;)V"),
    NATIVE_METHOD(Posix, execve, "(Ljava/lang/String;[Ljava/lang/String;[Ljava/lang/String;)V"),
    NATIVE_METHOD(Posix, fchmod, "(Ljava/io/FileDescriptor;I)V"),
    NATIVE_METHOD(Posix, fchown, "(Ljava/io/FileDescriptor;II)V"),
    NATIVE_METHOD(Posix, fcntlVoid, "(Ljava/io/FileDescriptor;I)I"),
    NATIVE_METHOD(Posix, fcntlLong, "(Ljava/io/FileDescriptor;IJ)I"),
    NATIVE_METHOD(Posix, fcntlFlock, "(Ljava/io/FileDescriptor;ILandroid/system/StructFlock;)I"),
    NATIVE_METHOD(Posix, fdatasync, "(Ljava/io/FileDescriptor;)V"),
    NATIVE_METHOD(Posix, fstat, "(Ljava/io/FileDescriptor;)Landroid/system/StructStat;"),
    NATIVE_METHOD(Posix, fstatvfs, "(Ljava/io/FileDescriptor;)Landroid/system/StructStatVfs;"),
    NATIVE_METHOD(Posix, fsync, "(Ljava/io/FileDescriptor;)V"),
    NATIVE_METHOD(Posix, ftruncate, "(Ljava/io/FileDescriptor;J)V"),
    NATIVE_METHOD(Posix, gai_strerror, "(I)Ljava/lang/String;"),
    NATIVE_METHOD(Posix, getegid, "()I"),
    NATIVE_METHOD(Posix, geteuid, "()I"),
    NATIVE_METHOD(Posix, getgid, "()I"),
    NATIVE_METHOD(Posix, getenv, "(Ljava/lang/String;)Ljava/lang/String;"),
    NATIVE_METHOD(Posix, getnameinfo, "(Ljava/net/InetAddress;I)Ljava/lang/String;"),
    NATIVE_METHOD(Posix, getpeername, "(Ljava/io/FileDescriptor;)Ljava/net/SocketAddress;"),
    NATIVE_METHOD(Posix, getpid, "()I"),
    NATIVE_METHOD(Posix, getppid, "()I"),
    NATIVE_METHOD(Posix, getpwnam, "(Ljava/lang/String;)Landroid/system/StructPasswd;"),
    NATIVE_METHOD(Posix, getpwuid, "(I)Landroid/system/StructPasswd;"),
    NATIVE_METHOD(Posix, getsockname, "(Ljava/io/FileDescriptor;)Ljava/net/SocketAddress;"),
    NATIVE_METHOD(Posix, getsockoptByte, "(Ljava/io/FileDescriptor;II)I"),
    NATIVE_METHOD(Posix, getsockoptInAddr, "(Ljava/io/FileDescriptor;II)Ljava/net/InetAddress;"),
    NATIVE_METHOD(Posix, getsockoptInt, "(Ljava/io/FileDescriptor;II)I"),
    NATIVE_METHOD(Posix, getsockoptLinger, "(Ljava/io/FileDescriptor;II)Landroid/system/StructLinger;"),
    NATIVE_METHOD(Posix, getsockoptTimeval, "(Ljava/io/FileDescriptor;II)Landroid/system/StructTimeval;"),
    NATIVE_METHOD(Posix, getsockoptUcred, "(Ljava/io/FileDescriptor;II)Landroid/system/StructUcred;"),
    NATIVE_METHOD(Posix, gettid, "()I"),
    NATIVE_METHOD(Posix, getuid, "()I"),
    NATIVE_METHOD(Posix, if_indextoname, "(I)Ljava/lang/String;"),
    NATIVE_METHOD(Posix, inet_pton, "(ILjava/lang/String;)Ljava/net/InetAddress;"),
    NATIVE_METHOD(Posix, ioctlInetAddress, "(Ljava/io/FileDescriptor;ILjava/lang/String;)Ljava/net/InetAddress;"),
    NATIVE_METHOD(Posix, ioctlInt, "(Ljava/io/FileDescriptor;ILandroid/util/MutableInt;)I"),
    NATIVE_METHOD(Posix, isatty, "(Ljava/io/FileDescriptor;)Z"),
    NATIVE_METHOD(Posix, kill, "(II)V"),
    NATIVE_METHOD(Posix, lchown, "(Ljava/lang/String;II)V"),
    NATIVE_METHOD(Posix, link, "(Ljava/lang/String;Ljava/lang/String;)V"),
    NATIVE_METHOD(Posix, listen, "(Ljava/io/FileDescriptor;I)V"),
    NATIVE_METHOD(Posix, lseek, "(Ljava/io/FileDescriptor;JI)J"),
    NATIVE_METHOD(Posix, lstat, "(Ljava/lang/String;)Landroid/system/StructStat;"),
    NATIVE_METHOD(Posix, mincore, "(JJ[B)V"),
    NATIVE_METHOD(Posix, mkdir, "(Ljava/lang/String;I)V"),
    NATIVE_METHOD(Posix, mkfifo, "(Ljava/lang/String;I)V"),
    NATIVE_METHOD(Posix, mlock, "(JJ)V"),
    NATIVE_METHOD(Posix, mmap, "(JJIILjava/io/FileDescriptor;J)J"),
    NATIVE_METHOD(Posix, msync, "(JJI)V"),
    NATIVE_METHOD(Posix, munlock, "(JJ)V"),
    NATIVE_METHOD(Posix, munmap, "(JJ)V"),
    NATIVE_METHOD(Posix, open, "(Ljava/lang/String;II)Ljava/io/FileDescriptor;"),
    NATIVE_METHOD(Posix, pipe, "()[Ljava/io/FileDescriptor;"),
    NATIVE_METHOD(Posix, poll, "([Landroid/system/StructPollfd;I)I"),
    NATIVE_METHOD(Posix, posix_fallocate, "(Ljava/io/FileDescriptor;JJ)V"),
    NATIVE_METHOD(Posix, prctl, "(IJJJJ)I"),
    NATIVE_METHOD(Posix, preadBytes, "(Ljava/io/FileDescriptor;Ljava/lang/Object;IIJ)I"),
    NATIVE_METHOD(Posix, pwriteBytes, "(Ljava/io/FileDescriptor;Ljava/lang/Object;IIJ)I"),
    NATIVE_METHOD(Posix, readBytes, "(Ljava/io/FileDescriptor;Ljava/lang/Object;II)I"),
    NATIVE_METHOD(Posix, readlink, "(Ljava/lang/String;)Ljava/lang/String;"),
    NATIVE_METHOD(Posix, readv, "(Ljava/io/FileDescriptor;[Ljava/lang/Object;[I[I)I"),
    NATIVE_METHOD(Posix, recvfromBytes, "(Ljava/io/FileDescriptor;Ljava/lang/Object;IIILjava/net/InetSocketAddress;)I"),
    NATIVE_METHOD(Posix, remove, "(Ljava/lang/String;)V"),
    NATIVE_METHOD(Posix, rename, "(Ljava/lang/String;Ljava/lang/String;)V"),
    NATIVE_METHOD(Posix, sendfile, "(Ljava/io/FileDescriptor;Ljava/io/FileDescriptor;Landroid/util/MutableLong;J)J"),
    NATIVE_METHOD(Posix, sendtoBytes, "(Ljava/io/FileDescriptor;Ljava/lang/Object;IIILjava/net/InetAddress;I)I"),
    NATIVE_METHOD(Posix, setegid, "(I)V"),
    NATIVE_METHOD(Posix, setenv, "(Ljava/lang/String;Ljava/lang/String;Z)V"),
    NATIVE_METHOD(Posix, seteuid, "(I)V"),
    NATIVE_METHOD(Posix, setgid, "(I)V"),
    NATIVE_METHOD(Posix, setsid, "()I"),
    NATIVE_METHOD(Posix, setsockoptByte, "(Ljava/io/FileDescriptor;III)V"),
    NATIVE_METHOD(Posix, setsockoptIfreq, "(Ljava/io/FileDescriptor;IILjava/lang/String;)V"),
    NATIVE_METHOD(Posix, setsockoptInt, "(Ljava/io/FileDescriptor;III)V"),
    NATIVE_METHOD(Posix, setsockoptIpMreqn, "(Ljava/io/FileDescriptor;III)V"),
    NATIVE_METHOD(Posix, setsockoptGroupReq, "(Ljava/io/FileDescriptor;IILandroid/system/StructGroupReq;)V"),
    NATIVE_METHOD(Posix, setsockoptGroupSourceReq, "(Ljava/io/FileDescriptor;IILandroid/system/StructGroupSourceReq;)V"),
    NATIVE_METHOD(Posix, setsockoptLinger, "(Ljava/io/FileDescriptor;IILandroid/system/StructLinger;)V"),
    NATIVE_METHOD(Posix, setsockoptTimeval, "(Ljava/io/FileDescriptor;IILandroid/system/StructTimeval;)V"),
    NATIVE_METHOD(Posix, setuid, "(I)V"),
    NATIVE_METHOD(Posix, shutdown, "(Ljava/io/FileDescriptor;I)V"),
    NATIVE_METHOD(Posix, socket, "(III)Ljava/io/FileDescriptor;"),
    NATIVE_METHOD(Posix, socketpair, "(IIILjava/io/FileDescriptor;Ljava/io/FileDescriptor;)V"),
    NATIVE_METHOD(Posix, stat, "(Ljava/lang/String;)Landroid/system/StructStat;"),
    NATIVE_METHOD(Posix, statvfs, "(Ljava/lang/String;)Landroid/system/StructStatVfs;"),
    NATIVE_METHOD(Posix, strerror, "(I)Ljava/lang/String;"),
    NATIVE_METHOD(Posix, strsignal, "(I)Ljava/lang/String;"),
    NATIVE_METHOD(Posix, symlink, "(Ljava/lang/String;Ljava/lang/String;)V"),
    NATIVE_METHOD(Posix, sysconf, "(I)J"),
    NATIVE_METHOD(Posix, tcdrain, "(Ljava/io/FileDescriptor;)V"),
    NATIVE_METHOD(Posix, tcsendbreak, "(Ljava/io/FileDescriptor;I)V"),
    NATIVE_METHOD(Posix, umaskImpl, "(I)I"),
    NATIVE_METHOD(Posix, uname, "()Landroid/system/StructUtsname;"),
    NATIVE_METHOD(Posix, unsetenv, "(Ljava/lang/String;)V"),
    NATIVE_METHOD(Posix, waitpid, "(ILandroid/util/MutableInt;I)I"),
    NATIVE_METHOD(Posix, writeBytes, "(Ljava/io/FileDescriptor;Ljava/lang/Object;II)I"),
    NATIVE_METHOD(Posix, writev, "(Ljava/io/FileDescriptor;[Ljava/lang/Object;[I[I)I"),
};
void register_libcore_io_Posix(JNIEnv* env) {
    jniRegisterNativeMethods(env, "libcore/io/Posix", gMethods, NELEM(gMethods));
}
