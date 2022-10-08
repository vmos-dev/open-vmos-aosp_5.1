/*
 * Copyright (C) 2006 The Android Open Source Project
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

#define LOG_TAG "Character"

#include "JNIHelp.h"
#include "JniConstants.h"
#include "ScopedUtfChars.h"
#include "unicode/uchar.h"
#include "unicode/uscript.h"
#include <math.h>
#include <stdio.h> // For BUFSIZ
#include <stdlib.h>

static jint Character_digitImpl(JNIEnv*, jclass, jint codePoint, jint radix) {
    return u_digit(codePoint, radix);
}

static jint Character_getTypeImpl(JNIEnv*, jclass, jint codePoint) {
    return u_charType(codePoint);
}

static jbyte Character_getIcuDirectionality(JNIEnv*, jclass, jint codePoint) {
    return u_charDirection(codePoint);
}

static jboolean Character_isMirroredImpl(JNIEnv*, jclass, jint codePoint) {
    return u_isMirrored(codePoint);
}

static jstring Character_getNameImpl(JNIEnv* env, jclass, jint codePoint) {
    // U_UNICODE_CHAR_NAME gives us the modern names for characters. For control characters,
    // we need U_EXTENDED_CHAR_NAME to get "NULL" rather than "BASIC LATIN 0" and so on.
    // We could just use U_EXTENDED_CHAR_NAME except that it returns strings for characters
    // that aren't unassigned but that don't have names, and those strings aren't in the form
    // Java specifies.
    bool isControl = (codePoint <= 0x1f || (codePoint >= 0x7f && codePoint <= 0x9f));
    UCharNameChoice nameType = isControl ? U_EXTENDED_CHAR_NAME : U_UNICODE_CHAR_NAME;
    UErrorCode status = U_ZERO_ERROR;
    char buf[BUFSIZ]; // TODO: is there a more sensible upper bound?
    int32_t byteCount = u_charName(codePoint, nameType, &buf[0], sizeof(buf), &status);
    return (U_FAILURE(status) || byteCount == 0) ? NULL : env->NewStringUTF(buf);
}

static jint Character_getNumericValueImpl(JNIEnv*, jclass, jint codePoint) {
    double result = u_getNumericValue(codePoint);
    if (result == U_NO_NUMERIC_VALUE) {
        return -1;
    } else if (result < 0 || floor(result + 0.5) != result) {
        return -2;
    }
    return static_cast<jint>(result);
}

static jboolean Character_isDefinedImpl(JNIEnv*, jclass, jint codePoint) {
    return u_isdefined(codePoint);
}

static jboolean Character_isDigitImpl(JNIEnv*, jclass, jint codePoint) {
    return u_isdigit(codePoint);
}

static jboolean Character_isIdentifierIgnorableImpl(JNIEnv*, jclass, jint codePoint) {
    return u_isIDIgnorable(codePoint);
}

static jboolean Character_isLetterImpl(JNIEnv*, jclass, jint codePoint) {
    return u_isalpha(codePoint);
}

static jboolean Character_isLetterOrDigitImpl(JNIEnv*, jclass, jint codePoint) {
    return u_isalnum(codePoint);
}

static jboolean Character_isSpaceCharImpl(JNIEnv*, jclass, jint codePoint) {
    return u_isJavaSpaceChar(codePoint);
}

static jboolean Character_isTitleCaseImpl(JNIEnv*, jclass, jint codePoint) {
    return u_istitle(codePoint);
}

static jboolean Character_isUnicodeIdentifierPartImpl(JNIEnv*, jclass, jint codePoint) {
    return u_isIDPart(codePoint);
}

static jboolean Character_isUnicodeIdentifierStartImpl(JNIEnv*, jclass, jint codePoint) {
    return u_isIDStart(codePoint);
}

static jboolean Character_isWhitespaceImpl(JNIEnv*, jclass, jint codePoint) {
    return u_isWhitespace(codePoint);
}

static jint Character_toLowerCaseImpl(JNIEnv*, jclass, jint codePoint) {
    return u_tolower(codePoint);
}

static jint Character_toTitleCaseImpl(JNIEnv*, jclass, jint codePoint) {
    return u_totitle(codePoint);
}

static jint Character_toUpperCaseImpl(JNIEnv*, jclass, jint codePoint) {
    return u_toupper(codePoint);
}

static jboolean Character_isUpperCaseImpl(JNIEnv*, jclass, jint codePoint) {
    return u_isupper(codePoint);
}

static jboolean Character_isLowerCaseImpl(JNIEnv*, jclass, jint codePoint) {
    return u_islower(codePoint);
}

static int Character_unicodeBlockForName(JNIEnv* env, jclass, jstring javaBlockName) {
    ScopedUtfChars blockName(env, javaBlockName);
    if (blockName.c_str() == NULL) {
        return 0;
    }
    return u_getPropertyValueEnum(UCHAR_BLOCK, blockName.c_str());
}

static int Character_unicodeBlockForCodePoint(JNIEnv*, jclass, jint codePoint) {
    return ublock_getCode(codePoint);
}

static int Character_unicodeScriptForName(JNIEnv* env, jclass, jstring javaScriptName) {
    ScopedUtfChars scriptName(env, javaScriptName);
    if (scriptName.c_str() == NULL) {
        return -1;
    }

    return u_getPropertyValueEnum(UCHAR_SCRIPT, scriptName.c_str());
}

static int Character_unicodeScriptForCodePoint(JNIEnv*, jclass, jint codePoint) {
    UErrorCode status = U_ZERO_ERROR;
    const UScriptCode script = uscript_getScript(codePoint, &status);
    if (status != U_ZERO_ERROR) {
        return -1;
    }

    return script;
}

static jboolean Character_isAlphabetic(JNIEnv*, jclass, jint codePoint) {
  return u_hasBinaryProperty(codePoint, UCHAR_ALPHABETIC);
}

static jboolean Character_isIdeographic(JNIEnv*, jclass, jint codePoint) {
  return u_hasBinaryProperty(codePoint, UCHAR_IDEOGRAPHIC);
}

static JNINativeMethod gMethods[] = {
    NATIVE_METHOD(Character, digitImpl, "!(II)I"),
    NATIVE_METHOD(Character, getIcuDirectionality, "!(I)B"),
    NATIVE_METHOD(Character, getNameImpl, "(I)Ljava/lang/String;"),
    NATIVE_METHOD(Character, getNumericValueImpl, "!(I)I"),
    NATIVE_METHOD(Character, getTypeImpl, "!(I)I"),
    NATIVE_METHOD(Character, isAlphabetic, "!(I)Z"),
    NATIVE_METHOD(Character, isDefinedImpl, "!(I)Z"),
    NATIVE_METHOD(Character, isDigitImpl, "!(I)Z"),
    NATIVE_METHOD(Character, isIdentifierIgnorableImpl, "!(I)Z"),
    NATIVE_METHOD(Character, isIdeographic, "!(I)Z"),
    NATIVE_METHOD(Character, isLetterImpl, "!(I)Z"),
    NATIVE_METHOD(Character, isLetterOrDigitImpl, "!(I)Z"),
    NATIVE_METHOD(Character, isLowerCaseImpl, "!(I)Z"),
    NATIVE_METHOD(Character, isMirroredImpl, "!(I)Z"),
    NATIVE_METHOD(Character, isSpaceCharImpl, "!(I)Z"),
    NATIVE_METHOD(Character, isTitleCaseImpl, "!(I)Z"),
    NATIVE_METHOD(Character, isUnicodeIdentifierPartImpl, "!(I)Z"),
    NATIVE_METHOD(Character, isUnicodeIdentifierStartImpl, "!(I)Z"),
    NATIVE_METHOD(Character, isUpperCaseImpl, "!(I)Z"),
    NATIVE_METHOD(Character, isWhitespaceImpl, "!(I)Z"),
    NATIVE_METHOD(Character, toLowerCaseImpl, "!(I)I"),
    NATIVE_METHOD(Character, toTitleCaseImpl, "!(I)I"),
    NATIVE_METHOD(Character, toUpperCaseImpl, "!(I)I"),
    NATIVE_METHOD(Character, unicodeBlockForName, "(Ljava/lang/String;)I"),
    NATIVE_METHOD(Character, unicodeBlockForCodePoint, "!(I)I"),
    NATIVE_METHOD(Character, unicodeScriptForName, "(Ljava/lang/String;)I"),
    NATIVE_METHOD(Character, unicodeScriptForCodePoint, "!(I)I"),
};
void register_java_lang_Character(JNIEnv* env) {
    jniRegisterNativeMethods(env, "java/lang/Character", gMethods, NELEM(gMethods));
}
