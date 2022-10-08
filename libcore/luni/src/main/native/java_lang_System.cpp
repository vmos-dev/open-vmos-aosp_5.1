/*
 * Copyright (C) 2008 The Android Open Source Project
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

#define LOG_TAG "System"

#include "JNIHelp.h"
#include "JniConstants.h"
#include "ScopedUtfChars.h"
#include "cutils/log.h"
#include "openssl/opensslv.h"
#include "toStringArray.h"
#include "zlib.h"

#include <string>
#include <vector>

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#if defined(HAVE_ANDROID_OS)
extern "C" void android_get_LD_LIBRARY_PATH(char*, size_t);
#endif

static void System_log(JNIEnv* env, jclass, jchar type, jstring javaMessage, jthrowable exception) {
    ScopedUtfChars message(env, javaMessage);
    if (message.c_str() == NULL) {
        // Since this function is used for last-gasp debugging output, be noisy on failure.
        ALOGE("message.c_str() == NULL");
        return;
    }
    int priority;
    switch (type) {
    case 'D': case 'd': priority = ANDROID_LOG_DEBUG;   break;
    case 'E': case 'e': priority = ANDROID_LOG_ERROR;   break;
    case 'F': case 'f': priority = ANDROID_LOG_FATAL;   break;
    case 'I': case 'i': priority = ANDROID_LOG_INFO;    break;
    case 'S': case 's': priority = ANDROID_LOG_SILENT;  break;
    case 'V': case 'v': priority = ANDROID_LOG_VERBOSE; break;
    case 'W': case 'w': priority = ANDROID_LOG_WARN;    break;
    default:            priority = ANDROID_LOG_DEFAULT; break;
    }
    LOG_PRI(priority, LOG_TAG, "%s", message.c_str());
    if (exception != NULL) {
        jniLogException(env, priority, LOG_TAG, exception);
    }
}

// Sets a field via JNI. Used for the standard streams, which are read-only otherwise.
static void System_setFieldImpl(JNIEnv* env, jclass clazz,
        jstring javaName, jstring javaSignature, jobject object) {
    ScopedUtfChars name(env, javaName);
    if (name.c_str() == NULL) {
        return;
    }
    ScopedUtfChars signature(env, javaSignature);
    if (signature.c_str() == NULL) {
        return;
    }
    jfieldID fieldID = env->GetStaticFieldID(clazz, name.c_str(), signature.c_str());
    env->SetStaticObjectField(clazz, fieldID, object);
}

static jobjectArray System_specialProperties(JNIEnv* env, jclass) {
    std::vector<std::string> properties;

    char path[PATH_MAX];
    properties.push_back(std::string("user.dir=") + getcwd(path, sizeof(path)));

    properties.push_back("android.zlib.version=" ZLIB_VERSION);
    properties.push_back("android.openssl.version=" OPENSSL_VERSION_TEXT);

    const char* library_path = getenv("LD_LIBRARY_PATH");
#if defined(HAVE_ANDROID_OS)
    if (library_path == NULL) {
        android_get_LD_LIBRARY_PATH(path, sizeof(path));
        library_path = path;
    }
#endif
    if (library_path == NULL) {
        library_path = "";
    }
    properties.push_back(std::string("java.library.path=") + library_path);

    return toStringArray(env, properties);
}

static jlong System_currentTimeMillis(JNIEnv*, jclass) {
    timeval now;
    gettimeofday(&now, NULL);
    jlong when = now.tv_sec * 1000LL + now.tv_usec / 1000;
    return when;
}

static jlong System_nanoTime(JNIEnv*, jclass) {
#if defined(HAVE_POSIX_CLOCKS)
    timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now.tv_sec * 1000000000LL + now.tv_nsec;
#else
    timeval now;
    gettimeofday(&now, NULL);
    return static_cast<jlong>(now.tv_sec) * 1000000000LL + now.tv_usec * 1000LL;
#endif
}

static jstring System_mapLibraryName(JNIEnv* env, jclass, jstring javaName) {
    ScopedUtfChars name(env, javaName);
    if (name.c_str() == NULL) {
        return NULL;
    }
    char* mappedName = NULL;
    asprintf(&mappedName, OS_SHARED_LIB_FORMAT_STR, name.c_str());
    jstring result = env->NewStringUTF(mappedName);
    free(mappedName);
    return result;
}

static JNINativeMethod gMethods[] = {
    NATIVE_METHOD(System, currentTimeMillis, "!()J"),
    NATIVE_METHOD(System, log, "(CLjava/lang/String;Ljava/lang/Throwable;)V"),
    NATIVE_METHOD(System, mapLibraryName, "(Ljava/lang/String;)Ljava/lang/String;"),
    NATIVE_METHOD(System, nanoTime, "!()J"),
    NATIVE_METHOD(System, setFieldImpl, "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/Object;)V"),
    NATIVE_METHOD(System, specialProperties, "()[Ljava/lang/String;"),
};
void register_java_lang_System(JNIEnv* env) {
    jniRegisterNativeMethods(env, "java/lang/System", gMethods, NELEM(gMethods));
}
