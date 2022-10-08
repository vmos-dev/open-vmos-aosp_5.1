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

#define LOG_TAG "DateIntervalFormat"

#include "IcuUtilities.h"
#include "JniConstants.h"
#include "ScopedIcuLocale.h"
#include "ScopedJavaUnicodeString.h"
#include "UniquePtr.h"
#include "cutils/log.h"
#include "unicode/dtitvfmt.h"

static jlong DateIntervalFormat_createDateIntervalFormat(JNIEnv* env, jclass, jstring javaSkeleton, jstring javaLocaleName, jstring javaTzName) {
  ScopedIcuLocale icuLocale(env, javaLocaleName);
  if (!icuLocale.valid()) {
    return 0;
  }

  ScopedJavaUnicodeString skeletonHolder(env, javaSkeleton);
  if (!skeletonHolder.valid()) {
    return 0;
  }

  UErrorCode status = U_ZERO_ERROR;
  DateIntervalFormat* formatter(DateIntervalFormat::createInstance(skeletonHolder.unicodeString(), icuLocale.locale(), status));
  if (maybeThrowIcuException(env, "DateIntervalFormat::createInstance", status)) {
    return 0;
  }

  ScopedJavaUnicodeString tzNameHolder(env, javaTzName);
  if (!tzNameHolder.valid()) {
    return 0;
  }
  formatter->adoptTimeZone(TimeZone::createTimeZone(tzNameHolder.unicodeString()));

  return reinterpret_cast<uintptr_t>(formatter);
}

static void DateIntervalFormat_destroyDateIntervalFormat(JNIEnv*, jclass, jlong address) {
  delete reinterpret_cast<DateIntervalFormat*>(address);
}

static jstring DateIntervalFormat_formatDateInterval(JNIEnv* env, jclass, jlong address, jlong fromDate, jlong toDate) {
  DateIntervalFormat* formatter(reinterpret_cast<DateIntervalFormat*>(address));
  DateInterval date_interval(fromDate, toDate);

  UnicodeString s;
  FieldPosition pos = 0;
  UErrorCode status = U_ZERO_ERROR;
  formatter->format(&date_interval, s, pos, status);
  if (maybeThrowIcuException(env, "DateIntervalFormat::format", status)) {
    return NULL;
  }

  return env->NewString(s.getBuffer(), s.length());
}

static JNINativeMethod gMethods[] = {
  NATIVE_METHOD(DateIntervalFormat, createDateIntervalFormat, "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)J"),
  NATIVE_METHOD(DateIntervalFormat, destroyDateIntervalFormat, "(J)V"),
  NATIVE_METHOD(DateIntervalFormat, formatDateInterval, "(JJJ)Ljava/lang/String;"),
};
void register_libcore_icu_DateIntervalFormat(JNIEnv* env) {
  jniRegisterNativeMethods(env, "libcore/icu/DateIntervalFormat", gMethods, NELEM(gMethods));
}
