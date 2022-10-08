/*
 * Copyright (C) 2013 The Android Open Source Project
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

#define LOG_TAG "AlphabeticIndex"

#include "IcuUtilities.h"
#include "JNIHelp.h"
#include "JniConstants.h"
#include "JniException.h"
#include "ScopedIcuLocale.h"
#include "ScopedJavaUnicodeString.h"
#include "unicode/alphaindex.h"
#include "unicode/uniset.h"

static AlphabeticIndex* fromPeer(jlong peer) {
  return reinterpret_cast<AlphabeticIndex*>(static_cast<uintptr_t>(peer));
}

static jlong AlphabeticIndex_create(JNIEnv* env, jclass, jstring javaLocaleName) {
  UErrorCode status = U_ZERO_ERROR;
  ScopedIcuLocale icuLocale(env, javaLocaleName);
  if (!icuLocale.valid()) {
    return 0;
  }
  AlphabeticIndex* ai = new AlphabeticIndex(icuLocale.locale(), status);
  if (maybeThrowIcuException(env, "AlphabeticIndex", status)) {
    return 0;
  }
  return reinterpret_cast<uintptr_t>(ai);
}

static void AlphabeticIndex_destroy(JNIEnv*, jclass, jlong peer) {
  delete fromPeer(peer);
}

static jint AlphabeticIndex_getMaxLabelCount(JNIEnv*, jclass, jlong peer) {
  AlphabeticIndex* ai = fromPeer(peer);
  return ai->getMaxLabelCount();
}

static void AlphabeticIndex_setMaxLabelCount(JNIEnv* env, jclass, jlong peer, jint count) {
  AlphabeticIndex* ai = fromPeer(peer);
  UErrorCode status = U_ZERO_ERROR;
  ai->setMaxLabelCount(count, status);
  maybeThrowIcuException(env, "AlphabeticIndex::setMaxLabelCount", status);
}

static void AlphabeticIndex_addLabels(JNIEnv* env, jclass, jlong peer, jstring javaLocaleName) {
  AlphabeticIndex* ai = fromPeer(peer);
  ScopedIcuLocale icuLocale(env, javaLocaleName);
  if (!icuLocale.valid()) {
    return;
  }
  UErrorCode status = U_ZERO_ERROR;
  ai->addLabels(icuLocale.locale(), status);
  maybeThrowIcuException(env, "AlphabeticIndex::addLabels", status);
}

static void AlphabeticIndex_addLabelRange(JNIEnv* env, jclass, jlong peer,
                                          jint codePointStart, jint codePointEnd) {
  AlphabeticIndex* ai = fromPeer(peer);
  UErrorCode status = U_ZERO_ERROR;
  ai->addLabels(UnicodeSet(codePointStart, codePointEnd), status);
  maybeThrowIcuException(env, "AlphabeticIndex::addLabels", status);
}

static jint AlphabeticIndex_getBucketCount(JNIEnv* env, jclass, jlong peer) {
  AlphabeticIndex* ai = fromPeer(peer);
  UErrorCode status = U_ZERO_ERROR;
  jint result = ai->getBucketCount(status);
  if (maybeThrowIcuException(env, "AlphabeticIndex::getBucketCount", status)) {
    return -1;
  }
  return result;
}

static jint AlphabeticIndex_getBucketIndex(JNIEnv* env, jclass, jlong peer, jstring javaString) {
  AlphabeticIndex* ai = fromPeer(peer);
  ScopedJavaUnicodeString string(env, javaString);
  if (!string.valid()) {
    return -1;
  }
  UErrorCode status = U_ZERO_ERROR;
  jint result = ai->getBucketIndex(string.unicodeString(), status);
  if (maybeThrowIcuException(env, "AlphabeticIndex::getBucketIndex", status)) {
    return -1;
  }
  return result;
}

static jstring AlphabeticIndex_getBucketLabel(JNIEnv* env, jclass, jlong peer, jint index) {
  if (index < 0) {
    jniThrowExceptionFmt(env, "java/lang/IllegalArgumentException", "Invalid index: %d", index);
    return NULL;
  }

  // Iterate to the nth bucket.
  AlphabeticIndex* ai = fromPeer(peer);
  UErrorCode status = U_ZERO_ERROR;
  ai->resetBucketIterator(status);
  if (maybeThrowIcuException(env, "AlphabeticIndex::resetBucketIterator", status)) {
    return NULL;
  }
  for (jint i = 0; i <= index; ++i) {
    if (!ai->nextBucket(status)) {
      jniThrowExceptionFmt(env, "java/lang/IllegalArgumentException", "Invalid index: %d", index);
      return NULL;
    }
    if (maybeThrowIcuException(env, "AlphabeticIndex::nextBucket", status)) {
      return NULL;
    }
  }

  // Return "" for the underflow/inflow/overflow buckets.
  if (ai->getBucketLabelType() != U_ALPHAINDEX_NORMAL) {
    return env->NewStringUTF("");
  }

  const UnicodeString& label(ai->getBucketLabel());
  return env->NewString(label.getBuffer(), label.length());
}

static jlong AlphabeticIndex_buildImmutableIndex(JNIEnv* env, jclass, jlong peer) {
  AlphabeticIndex* ai = fromPeer(peer);
  UErrorCode status = U_ZERO_ERROR;
  AlphabeticIndex::ImmutableIndex* ii = ai->buildImmutableIndex(status);
  if (maybeThrowIcuException(env, "AlphabeticIndex::buildImmutableIndex", status)) {
    return 0;
  }
  return reinterpret_cast<uintptr_t>(ii);
}

static AlphabeticIndex::ImmutableIndex* immutableIndexFromPeer(jlong peer) {
  return reinterpret_cast<AlphabeticIndex::ImmutableIndex*>(static_cast<uintptr_t>(peer));
}

static jint ImmutableIndex_getBucketCount(JNIEnv*, jclass, jlong peer) {
  AlphabeticIndex::ImmutableIndex* ii = immutableIndexFromPeer(peer);
  return ii->getBucketCount();
}

static jint ImmutableIndex_getBucketIndex(JNIEnv* env, jclass, jlong peer, jstring javaString) {
  AlphabeticIndex::ImmutableIndex* ii = immutableIndexFromPeer(peer);
  ScopedJavaUnicodeString string(env, javaString);
  if (!string.valid()) {
    return -1;
  }
  UErrorCode status = U_ZERO_ERROR;
  jint result = ii->getBucketIndex(string.unicodeString(), status);
  if (maybeThrowIcuException(env, "AlphabeticIndex::ImmutableIndex::getBucketIndex", status)) {
    return -1;
  }
  return result;
}

static jstring ImmutableIndex_getBucketLabel(JNIEnv* env, jclass, jlong peer, jint index) {
  AlphabeticIndex::ImmutableIndex* ii = immutableIndexFromPeer(peer);
  const AlphabeticIndex::Bucket* bucket = ii->getBucket(index);
  if (bucket == NULL) {
    jniThrowExceptionFmt(env, "java/lang/IllegalArgumentException", "Invalid index: %d", index);
    return NULL;
  }

  // Return "" for the underflow/inflow/overflow buckets.
  if (bucket->getLabelType() != U_ALPHAINDEX_NORMAL) {
    return env->NewStringUTF("");
  }

  const UnicodeString& label(bucket->getLabel());
  return env->NewString(label.getBuffer(), label.length());
}

static JNINativeMethod gMethods[] = {
  NATIVE_METHOD(AlphabeticIndex, create, "(Ljava/lang/String;)J"),
  NATIVE_METHOD(AlphabeticIndex, destroy, "(J)V"),
  NATIVE_METHOD(AlphabeticIndex, getMaxLabelCount, "(J)I"),
  NATIVE_METHOD(AlphabeticIndex, setMaxLabelCount, "(JI)V"),
  NATIVE_METHOD(AlphabeticIndex, addLabels, "(JLjava/lang/String;)V"),
  NATIVE_METHOD(AlphabeticIndex, addLabelRange, "(JII)V"),
  NATIVE_METHOD(AlphabeticIndex, getBucketCount, "(J)I"),
  NATIVE_METHOD(AlphabeticIndex, getBucketIndex, "(JLjava/lang/String;)I"),
  NATIVE_METHOD(AlphabeticIndex, getBucketLabel, "(JI)Ljava/lang/String;"),
  NATIVE_METHOD(AlphabeticIndex, buildImmutableIndex, "(J)J"),
};
static JNINativeMethod gImmutableIndexMethods[] = {
  NATIVE_METHOD(ImmutableIndex, getBucketCount, "(J)I"),
  NATIVE_METHOD(ImmutableIndex, getBucketIndex, "(JLjava/lang/String;)I"),
  NATIVE_METHOD(ImmutableIndex, getBucketLabel, "(JI)Ljava/lang/String;"),
};
void register_libcore_icu_AlphabeticIndex(JNIEnv* env) {
  jniRegisterNativeMethods(env, "libcore/icu/AlphabeticIndex", gMethods, NELEM(gMethods));
  jniRegisterNativeMethods(env, "libcore/icu/AlphabeticIndex$ImmutableIndex", gImmutableIndexMethods, NELEM(gImmutableIndexMethods));
}
