/*
 * Copyright (C) 2007 The Android Open Source Project
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

#define LOG_TAG "NativeTestTarget"

#include "JNIHelp.h"
#include "JniConstants.h"

static void NativeTestTarget_emptyJniMethod0(JNIEnv*, jobject) { }
static void NativeTestTarget_emptyJniMethod6(JNIEnv*, jclass, int, int, int, int, int, int) { }
static void NativeTestTarget_emptyJniMethod6L(JNIEnv*, jclass, jobject, jarray, jarray, jobject, jarray, jarray) { }
static void NativeTestTarget_emptyJniStaticMethod0(JNIEnv*, jclass) { }
static void NativeTestTarget_emptyJniStaticMethod6(JNIEnv*, jclass, int, int, int, int, int, int) { }
static void NativeTestTarget_emptyJniStaticMethod6L(JNIEnv*, jclass, jobject, jarray, jarray, jobject, jarray, jarray) { }
static void NativeTestTarget_emptyJniStaticSynchronizedMethod0(JNIEnv*, jclass) { }
static void NativeTestTarget_emptyJniSynchronizedMethod0(JNIEnv*, jclass) { }

static JNINativeMethod gMethods[] = {
    NATIVE_METHOD(NativeTestTarget, emptyJniMethod0, "()V"),
    NATIVE_METHOD(NativeTestTarget, emptyJniMethod6, "(IIIIII)V"),
    NATIVE_METHOD(NativeTestTarget, emptyJniMethod6L, "(Ljava/lang/String;[Ljava/lang/String;[[ILjava/lang/Object;[Ljava/lang/Object;[[[[Ljava/lang/Object;)V"),
    NATIVE_METHOD(NativeTestTarget, emptyJniStaticMethod0, "()V"),
    NATIVE_METHOD(NativeTestTarget, emptyJniStaticMethod6, "(IIIIII)V"),
    NATIVE_METHOD(NativeTestTarget, emptyJniStaticMethod6L, "(Ljava/lang/String;[Ljava/lang/String;[[ILjava/lang/Object;[Ljava/lang/Object;[[[[Ljava/lang/Object;)V"),
    NATIVE_METHOD(NativeTestTarget, emptyJniStaticSynchronizedMethod0, "()V"),
    NATIVE_METHOD(NativeTestTarget, emptyJniSynchronizedMethod0, "()V"),
};
int register_org_apache_harmony_dalvik_NativeTestTarget(JNIEnv* env) {
    return jniRegisterNativeMethods(env, "org/apache/harmony/dalvik/NativeTestTarget", gMethods, NELEM(gMethods));
}
