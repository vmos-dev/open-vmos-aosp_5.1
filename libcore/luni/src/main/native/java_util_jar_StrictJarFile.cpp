/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "StrictJarFile"

#include <string>

#include "JNIHelp.h"
#include "JniConstants.h"
#include "ScopedLocalRef.h"
#include "ScopedUtfChars.h"
#include "UniquePtr.h"
#include "jni.h"
#include "ziparchive/zip_archive.h"
#include "cutils/log.h"

static void throwIoException(JNIEnv* env, const int32_t errorCode) {
  jniThrowException(env, "java/io/IOException", ErrorCodeString(errorCode));
}

// Constructs a string out of |name| with the default charset (UTF-8 on android).
// We prefer this to JNI's NewStringUTF because the string constructor will
// replace unmappable and malformed bytes instead of throwing. See b/18584205
//
// Returns |NULL| iff. we couldn't allocate the string object or its constructor
// arguments.
//
// TODO: switch back to NewStringUTF after libziparchive is modified to reject
// files whose names aren't valid UTF-8.
static jobject constructString(JNIEnv* env, const char* name, const uint16_t nameLength) {
  jbyteArray javaNameBytes = env->NewByteArray(nameLength);
  if (javaNameBytes == NULL) {
      return NULL;
  }
  env->SetByteArrayRegion(javaNameBytes, 0, nameLength, reinterpret_cast<const jbyte*>(name));

  ScopedLocalRef<jclass> stringClass(env, env->FindClass("java/lang/String"));
  const jmethodID stringCtor = env->GetMethodID(stringClass.get(), "<init>", "([B)V");
  return env->NewObject(stringClass.get(), stringCtor, javaNameBytes);
}

static jobject newZipEntry(JNIEnv* env, const ZipEntry& entry, const jobject entryName,
                           const uint16_t nameLength) {
  ScopedLocalRef<jclass> zipEntryClass(env, env->FindClass("java/util/zip/ZipEntry"));
  const jmethodID zipEntryCtor = env->GetMethodID(zipEntryClass.get(), "<init>",
                                   "(Ljava/lang/String;Ljava/lang/String;JJJIII[BIJJ)V");

  return env->NewObject(zipEntryClass.get(),
                        zipEntryCtor,
                        entryName,
                        NULL,  // comment
                        static_cast<jlong>(entry.crc32),
                        static_cast<jlong>(entry.compressed_length),
                        static_cast<jlong>(entry.uncompressed_length),
                        static_cast<jint>(entry.method),
                        static_cast<jint>(0),  // time
                        static_cast<jint>(0),  // modData
                        NULL,  // byte[] extra
                        static_cast<jint>(nameLength),
                        static_cast<jlong>(-1),  // local header offset
                        static_cast<jlong>(entry.offset));
}

static jobject newZipEntry(JNIEnv* env, const ZipEntry& entry, const char* name,
                           const uint16_t nameLength) {
  return newZipEntry(env, entry, constructString(env, name, nameLength), nameLength);
}

static jlong StrictJarFile_nativeOpenJarFile(JNIEnv* env, jobject, jstring fileName) {
  ScopedUtfChars fileChars(env, fileName);
  if (fileChars.c_str() == NULL) {
    return static_cast<jlong>(-1);
  }

  ZipArchiveHandle handle;
  int32_t error = OpenArchive(fileChars.c_str(), &handle);
  if (error) {
    throwIoException(env, error);
    return static_cast<jlong>(-1);
  }

  return reinterpret_cast<jlong>(handle);
}

class IterationHandle {
 public:
  IterationHandle(const char* prefix) :
    cookie_(NULL), prefix_(strdup(prefix)) {
  }

  void** CookieAddress() {
    return &cookie_;
  }

  const char* Prefix() const {
    return prefix_;
  }

  ~IterationHandle() {
    free(prefix_);
  }

 private:
  void* cookie_;
  char* prefix_;
};


static jlong StrictJarFile_nativeStartIteration(JNIEnv* env, jobject, jlong nativeHandle,
                                                jstring prefix) {
  ScopedUtfChars prefixChars(env, prefix);
  if (prefixChars.c_str() == NULL) {
    return static_cast<jlong>(-1);
  }

  IterationHandle* handle = new IterationHandle(prefixChars.c_str());
  int32_t error = 0;
  if (prefixChars.size() == 0) {
    error = StartIteration(reinterpret_cast<ZipArchiveHandle>(nativeHandle),
                           handle->CookieAddress(), NULL);
  } else {
    error = StartIteration(reinterpret_cast<ZipArchiveHandle>(nativeHandle),
                           handle->CookieAddress(), handle->Prefix());
  }

  if (error) {
    throwIoException(env, error);
    return static_cast<jlong>(-1);
  }

  return reinterpret_cast<jlong>(handle);
}

static jobject StrictJarFile_nativeNextEntry(JNIEnv* env, jobject, jlong iterationHandle) {
  ZipEntry data;
  ZipEntryName entryName;

  IterationHandle* handle = reinterpret_cast<IterationHandle*>(iterationHandle);
  const int32_t error = Next(*handle->CookieAddress(), &data, &entryName);
  if (error) {
    delete handle;
    return NULL;
  }

  UniquePtr<char[]> entryNameCString(new char[entryName.name_length + 1]);
  memcpy(entryNameCString.get(), entryName.name, entryName.name_length);
  entryNameCString[entryName.name_length] = '\0';

  return newZipEntry(env, data, entryNameCString.get(), entryName.name_length);
}

static jobject StrictJarFile_nativeFindEntry(JNIEnv* env, jobject, jlong nativeHandle,
                                             jstring entryName) {
  ScopedUtfChars entryNameChars(env, entryName);
  if (entryNameChars.c_str() == NULL) {
    return NULL;
  }

  ZipEntry data;
  const int32_t error = FindEntry(reinterpret_cast<ZipArchiveHandle>(nativeHandle),
                                  entryNameChars.c_str(), &data);
  if (error) {
    return NULL;
  }

  return newZipEntry(env, data, entryName, entryNameChars.size());
}

static void StrictJarFile_nativeClose(JNIEnv*, jobject, jlong nativeHandle) {
  CloseArchive(reinterpret_cast<ZipArchiveHandle>(nativeHandle));
}

static JNINativeMethod gMethods[] = {
  NATIVE_METHOD(StrictJarFile, nativeOpenJarFile, "(Ljava/lang/String;)J"),
  NATIVE_METHOD(StrictJarFile, nativeStartIteration, "(JLjava/lang/String;)J"),
  NATIVE_METHOD(StrictJarFile, nativeNextEntry, "(J)Ljava/util/zip/ZipEntry;"),
  NATIVE_METHOD(StrictJarFile, nativeFindEntry, "(JLjava/lang/String;)Ljava/util/zip/ZipEntry;"),
  NATIVE_METHOD(StrictJarFile, nativeClose, "(J)V"),
};

void register_java_util_jar_StrictJarFile(JNIEnv* env) {
  jniRegisterNativeMethods(env, "java/util/jar/StrictJarFile", gMethods, NELEM(gMethods));
}
