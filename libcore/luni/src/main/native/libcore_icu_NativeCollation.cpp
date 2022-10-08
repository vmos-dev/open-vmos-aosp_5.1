/**
*******************************************************************************
* Copyright (C) 1996-2005, International Business Machines Corporation and    *
* others. All Rights Reserved.                                                *
*******************************************************************************
*
*******************************************************************************
*/

#define LOG_TAG "NativeCollation"

#include "IcuUtilities.h"
#include "JNIHelp.h"
#include "JniConstants.h"
#include "JniException.h"
#include "ScopedStringChars.h"
#include "ScopedUtfChars.h"
#include "UniquePtr.h"
#include "unicode/ucol.h"
#include "unicode/ucoleitr.h"
#include <cutils/log.h>

// Manages a UCollationElements instance along with the jchar
// array it is iterating over. The associated array can be unpinned
// only after a call to ucol_closeElements. This means we have to
// keep a reference to the string (so that it isn't collected) and
// make a call to GetStringChars to ensure the underlying array is
// pinned.
class CollationElements {
public:
    CollationElements()
        : mElements(NULL), mString(NULL), mChars(NULL) {
    }

    UCollationElements* get() const {
        return mElements;
    }

    // Starts a new iteration sequence over the string |string|. If
    // we have a valid UCollationElements object, we call ucol_setText
    // on it. Otherwise, we create a new object with the specified
    // collator.
    UErrorCode start(JNIEnv* env, jstring string, UCollator* collator) {
        release(env, false /* don't close the collator */);
        mChars = env->GetStringChars(string, NULL);
        if (mChars != NULL) {
            mString = static_cast<jstring>(env->NewGlobalRef(string));
            const size_t size = env->GetStringLength(string);

            UErrorCode status = U_ZERO_ERROR;
            // If we don't have a UCollationElements object yet, create
            // a new one. If we do, reset it.
            if (mElements == NULL) {
                mElements = ucol_openElements(collator, mChars, size, &status);
            } else {
               ucol_setText(mElements, mChars, size, &status);
            }

            return status;
        }

        return U_ILLEGAL_ARGUMENT_ERROR;
    }

    void release(JNIEnv* env, bool closeCollator) {
        if (mElements != NULL && closeCollator) {
            ucol_closeElements(mElements);
        }

        if (mChars != NULL) {
            env->ReleaseStringChars(mString, mChars);
            env->DeleteGlobalRef(mString);
            mChars = NULL;
            mString = NULL;
        }
    }

private:
    UCollationElements* mElements;
    jstring mString;
    const jchar* mChars;
};

static UCollator* toCollator(jlong address) {
    return reinterpret_cast<UCollator*>(static_cast<uintptr_t>(address));
}

static CollationElements* toCollationElements(jlong address) {
    return reinterpret_cast<CollationElements*>(static_cast<uintptr_t>(address));
}

static void NativeCollation_closeCollator(JNIEnv*, jclass, jlong address) {
    ucol_close(toCollator(address));
}

static void NativeCollation_closeElements(JNIEnv* env, jclass, jlong address) {
    CollationElements* elements = toCollationElements(address);
    elements->release(env, true /* close collator */);
    delete elements;
}

static jint NativeCollation_compare(JNIEnv* env, jclass, jlong address, jstring javaLhs, jstring javaRhs) {
    ScopedStringChars lhs(env, javaLhs);
    if (lhs.get() == NULL) {
        return 0;
    }
    ScopedStringChars rhs(env, javaRhs);
    if (rhs.get() == NULL) {
        return 0;
    }
    return ucol_strcoll(toCollator(address), lhs.get(), lhs.size(), rhs.get(), rhs.size());
}

static jint NativeCollation_getAttribute(JNIEnv* env, jclass, jlong address, jint type) {
    UErrorCode status = U_ZERO_ERROR;
    jint result = ucol_getAttribute(toCollator(address), (UColAttribute) type, &status);
    maybeThrowIcuException(env, "ucol_getAttribute", status);
    return result;
}

static jlong NativeCollation_getCollationElementIterator(JNIEnv* env, jclass, jlong address, jstring javaSource) {
    ScopedStringChars source(env, javaSource);
    if (source.get() == NULL) {
        return -1;
    }

    UniquePtr<CollationElements> ce(new CollationElements);
    UErrorCode status = ce->start(env, javaSource, toCollator(address));
    maybeThrowIcuException(env, "ucol_openElements", status);
    if (status == U_ZERO_ERROR) {
        return static_cast<jlong>(reinterpret_cast<uintptr_t>(ce.release()));
    }

    return 0L;
}

static jint NativeCollation_getMaxExpansion(JNIEnv*, jclass, jlong address, jint order) {
    return ucol_getMaxExpansion(toCollationElements(address)->get(), order);
}

static jint NativeCollation_getOffset(JNIEnv*, jclass, jlong address) {
    return ucol_getOffset(toCollationElements(address)->get());
}

static jstring NativeCollation_getRules(JNIEnv* env, jclass, jlong address) {
    int32_t length = 0;
    const UChar* rules = ucol_getRules(toCollator(address), &length);
    return env->NewString(rules, length);
}

static jbyteArray NativeCollation_getSortKey(JNIEnv* env, jclass, jlong address, jstring javaSource) {
    ScopedStringChars source(env, javaSource);
    if (source.get() == NULL) {
        return NULL;
    }
    const UCollator* collator  = toCollator(address);
    // The buffer size prevents reallocation for most strings.
    uint8_t byteArray[128];
    UniquePtr<uint8_t[]> largerByteArray;
    uint8_t* usedByteArray = byteArray;
    size_t byteArraySize = ucol_getSortKey(collator, source.get(), source.size(), usedByteArray, sizeof(byteArray) - 1);
    if (byteArraySize > sizeof(byteArray) - 1) {
        // didn't fit, try again with a larger buffer.
        largerByteArray.reset(new uint8_t[byteArraySize + 1]);
        usedByteArray = largerByteArray.get();
        byteArraySize = ucol_getSortKey(collator, source.get(), source.size(), usedByteArray, byteArraySize);
    }
    if (byteArraySize == 0) {
        return NULL;
    }
    jbyteArray result = env->NewByteArray(byteArraySize);
    env->SetByteArrayRegion(result, 0, byteArraySize, reinterpret_cast<jbyte*>(usedByteArray));
    return result;
}

static jint NativeCollation_next(JNIEnv* env, jclass, jlong address) {
    UErrorCode status = U_ZERO_ERROR;
    jint result = ucol_next(toCollationElements(address)->get(), &status);
    maybeThrowIcuException(env, "ucol_next", status);
    return result;
}

static jlong NativeCollation_openCollator(JNIEnv* env, jclass, jstring javaLocaleName) {
    ScopedUtfChars localeChars(env, javaLocaleName);
    if (localeChars.c_str() == NULL) {
        return 0;
    }

    UErrorCode status = U_ZERO_ERROR;
    UCollator* c = ucol_open(localeChars.c_str(), &status);
    maybeThrowIcuException(env, "ucol_open", status);
    return static_cast<jlong>(reinterpret_cast<uintptr_t>(c));
}

static jlong NativeCollation_openCollatorFromRules(JNIEnv* env, jclass, jstring javaRules, jint mode, jint strength) {
    ScopedStringChars rules(env, javaRules);
    if (rules.get() == NULL) {
        return -1;
    }
    UErrorCode status = U_ZERO_ERROR;
    UCollator* c = ucol_openRules(rules.get(), rules.size(),
            UColAttributeValue(mode), UCollationStrength(strength), NULL, &status);
    maybeThrowIcuException(env, "ucol_openRules", status);
    return static_cast<jlong>(reinterpret_cast<uintptr_t>(c));
}

static jint NativeCollation_previous(JNIEnv* env, jclass, jlong address) {
    UErrorCode status = U_ZERO_ERROR;
    jint result = ucol_previous(toCollationElements(address)->get(), &status);
    maybeThrowIcuException(env, "ucol_previous", status);
    return result;
}

static void NativeCollation_reset(JNIEnv*, jclass, jlong address) {
    ucol_reset(toCollationElements(address)->get());
}

static jlong NativeCollation_safeClone(JNIEnv* env, jclass, jlong address) {
    UErrorCode status = U_ZERO_ERROR;
    UCollator* c = ucol_safeClone(toCollator(address), NULL, NULL, &status);
    maybeThrowIcuException(env, "ucol_safeClone", status);
    return static_cast<jlong>(reinterpret_cast<uintptr_t>(c));
}

static void NativeCollation_setAttribute(JNIEnv* env, jclass, jlong address, jint type, jint value) {
    UErrorCode status = U_ZERO_ERROR;
    ucol_setAttribute(toCollator(address), (UColAttribute)type, (UColAttributeValue)value, &status);
    maybeThrowIcuException(env, "ucol_setAttribute", status);
}

static void NativeCollation_setOffset(JNIEnv* env, jclass, jlong address, jint offset) {
    UErrorCode status = U_ZERO_ERROR;
    ucol_setOffset(toCollationElements(address)->get(), offset, &status);
    maybeThrowIcuException(env, "ucol_setOffset", status);
}

static void NativeCollation_setText(JNIEnv* env, jclass, jlong address, jstring javaSource) {
    ScopedStringChars source(env, javaSource);
    if (source.get() == NULL) {
        return;
    }
    UErrorCode status = toCollationElements(address)->start(env, javaSource, NULL);
    maybeThrowIcuException(env, "ucol_setText", status);
}

static JNINativeMethod gMethods[] = {
    NATIVE_METHOD(NativeCollation, closeCollator, "(J)V"),
    NATIVE_METHOD(NativeCollation, closeElements, "(J)V"),
    NATIVE_METHOD(NativeCollation, compare, "(JLjava/lang/String;Ljava/lang/String;)I"),
    NATIVE_METHOD(NativeCollation, getAttribute, "(JI)I"),
    NATIVE_METHOD(NativeCollation, getCollationElementIterator, "(JLjava/lang/String;)J"),
    NATIVE_METHOD(NativeCollation, getMaxExpansion, "(JI)I"),
    NATIVE_METHOD(NativeCollation, getOffset, "(J)I"),
    NATIVE_METHOD(NativeCollation, getRules, "(J)Ljava/lang/String;"),
    NATIVE_METHOD(NativeCollation, getSortKey, "(JLjava/lang/String;)[B"),
    NATIVE_METHOD(NativeCollation, next, "(J)I"),
    NATIVE_METHOD(NativeCollation, openCollator, "(Ljava/lang/String;)J"),
    NATIVE_METHOD(NativeCollation, openCollatorFromRules, "(Ljava/lang/String;II)J"),
    NATIVE_METHOD(NativeCollation, previous, "(J)I"),
    NATIVE_METHOD(NativeCollation, reset, "(J)V"),
    NATIVE_METHOD(NativeCollation, safeClone, "(J)J"),
    NATIVE_METHOD(NativeCollation, setAttribute, "(JII)V"),
    NATIVE_METHOD(NativeCollation, setOffset, "(JI)V"),
    NATIVE_METHOD(NativeCollation, setText, "(JLjava/lang/String;)V"),
};
void register_libcore_icu_NativeCollation(JNIEnv* env) {
    jniRegisterNativeMethods(env, "libcore/icu/NativeCollation", gMethods, NELEM(gMethods));
}
