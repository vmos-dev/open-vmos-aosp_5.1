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

#define LOG_TAG "NativeBN"

#include "JNIHelp.h"
#include "JniConstants.h"
#include "JniException.h"
#include "ScopedPrimitiveArray.h"
#include "ScopedUtfChars.h"
#include "UniquePtr.h"
#include "jni.h"
#include <openssl/bn.h>
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <stdio.h>

struct BN_CTX_Deleter {
  void operator()(BN_CTX* p) const {
    BN_CTX_free(p);
  }
};
typedef UniquePtr<BN_CTX, BN_CTX_Deleter> Unique_BN_CTX;

static BIGNUM* toBigNum(jlong address) {
  return reinterpret_cast<BIGNUM*>(static_cast<uintptr_t>(address));
}

static bool throwExceptionIfNecessary(JNIEnv* env) {
  long error = ERR_get_error();
  if (error == 0) {
    return false;
  }
  char message[256];
  ERR_error_string_n(error, message, sizeof(message));
  int reason = ERR_GET_REASON(error);
  if (reason == BN_R_DIV_BY_ZERO) {
    jniThrowException(env, "java/lang/ArithmeticException", "BigInteger division by zero");
  } else if (reason == BN_R_NO_INVERSE) {
    jniThrowException(env, "java/lang/ArithmeticException", "BigInteger not invertible");
  } else if (reason == ERR_R_MALLOC_FAILURE) {
    jniThrowOutOfMemoryError(env, message);
  } else {
    jniThrowException(env, "java/lang/ArithmeticException", message);
  }
  return true;
}

static int isValidHandle(JNIEnv* env, jlong handle, const char* message) {
  if (handle == 0) {
    jniThrowNullPointerException(env, message);
    return JNI_FALSE;
  }
  return JNI_TRUE;
}

static int oneValidHandle(JNIEnv* env, jlong a) {
  return isValidHandle(env, a, "Mandatory handle (first) passed as null");
}

static int twoValidHandles(JNIEnv* env, jlong a, jlong b) {
  if (!oneValidHandle(env, a)) return JNI_FALSE;
  return isValidHandle(env, b, "Mandatory handle (second) passed as null");
}

static int threeValidHandles(JNIEnv* env, jlong a, jlong b, jlong c) {
  if (!twoValidHandles(env, a, b)) return JNI_FALSE;
  return isValidHandle(env, c, "Mandatory handle (third) passed as null");
}

static int fourValidHandles(JNIEnv* env, jlong a, jlong b, jlong c, jlong d) {
  if (!threeValidHandles(env, a, b, c)) return JNI_FALSE;
  return isValidHandle(env, d, "Mandatory handle (fourth) passed as null");
}

static jlong NativeBN_BN_new(JNIEnv* env, jclass) {
  jlong result = static_cast<jlong>(reinterpret_cast<uintptr_t>(BN_new()));
  throwExceptionIfNecessary(env);
  return result;
}

static void NativeBN_BN_free(JNIEnv* env, jclass, jlong a) {
  if (!oneValidHandle(env, a)) return;
  BN_free(toBigNum(a));
}

static int NativeBN_BN_cmp(JNIEnv* env, jclass, jlong a, jlong b) {
  if (!twoValidHandles(env, a, b)) return 1;
  return BN_cmp(toBigNum(a), toBigNum(b));
}

static void NativeBN_BN_copy(JNIEnv* env, jclass, jlong to, jlong from) {
  if (!twoValidHandles(env, to, from)) return;
  BN_copy(toBigNum(to), toBigNum(from));
  throwExceptionIfNecessary(env);
}

static void NativeBN_putULongInt(JNIEnv* env, jclass, jlong a0, jlong java_dw, jboolean neg) {
    if (!oneValidHandle(env, a0)) return;

    uint64_t dw = java_dw;

    // cf. litEndInts2bn:
    BIGNUM* a = toBigNum(a0);
    bn_check_top(a);
    if (bn_wexpand(a, 8/BN_BYTES) != NULL) {
#ifdef __LP64__
      a->d[0] = dw;
#else
      unsigned int hi = dw >> 32; // This shifts without sign extension.
      int lo = (int)dw; // This truncates implicitly.
      a->d[0] = lo;
      a->d[1] = hi;
#endif
      a->top = 8 / BN_BYTES;
      a->neg = neg;
      bn_correct_top(a);
    } else {
      throwExceptionIfNecessary(env);
    }
}

static void NativeBN_putLongInt(JNIEnv* env, jclass cls, jlong a, jlong dw) {
  if (dw >= 0) {
    NativeBN_putULongInt(env, cls, a, dw, JNI_FALSE);
  } else {
    NativeBN_putULongInt(env, cls, a, -dw, JNI_TRUE);
  }
}

static int NativeBN_BN_dec2bn(JNIEnv* env, jclass, jlong a0, jstring str) {
  if (!oneValidHandle(env, a0)) return -1;
  ScopedUtfChars chars(env, str);
  if (chars.c_str() == NULL) {
    return -1;
  }
  BIGNUM* a = toBigNum(a0);
  int result = BN_dec2bn(&a, chars.c_str());
  throwExceptionIfNecessary(env);
  return result;
}

static int NativeBN_BN_hex2bn(JNIEnv* env, jclass, jlong a0, jstring str) {
  if (!oneValidHandle(env, a0)) return -1;
  ScopedUtfChars chars(env, str);
  if (chars.c_str() == NULL) {
    return -1;
  }
  BIGNUM* a = toBigNum(a0);
  int result = BN_hex2bn(&a, chars.c_str());
  throwExceptionIfNecessary(env);
  return result;
}

static void NativeBN_BN_bin2bn(JNIEnv* env, jclass, jbyteArray arr, int len, jboolean neg, jlong ret) {
  if (!oneValidHandle(env, ret)) return;
  ScopedByteArrayRO bytes(env, arr);
  if (bytes.get() == NULL) {
    return;
  }
  BN_bin2bn(reinterpret_cast<const unsigned char*>(bytes.get()), len, toBigNum(ret));
  if (!throwExceptionIfNecessary(env) && neg) {
    BN_set_negative(toBigNum(ret), true);
  }
}

/**
 * Note:
 * This procedure directly writes the internal representation of BIGNUMs.
 * We do so as there is no direct interface based on Little Endian Integer Arrays.
 * Also note that the same representation is used in the Cordoba Java Implementation of BigIntegers,
 *        whereof certain functionality is still being used.
 */
static void NativeBN_litEndInts2bn(JNIEnv* env, jclass, jintArray arr, int len, jboolean neg, jlong ret0) {
  if (!oneValidHandle(env, ret0)) return;
  BIGNUM* ret = toBigNum(ret0);
  bn_check_top(ret);
  if (len > 0) {
    ScopedIntArrayRO scopedArray(env, arr);
    if (scopedArray.get() == NULL) {
      return;
    }
#ifdef __LP64__
    const int wlen = (len + 1) / 2;
#else
    const int wlen = len;
#endif
    const unsigned int* tmpInts = reinterpret_cast<const unsigned int*>(scopedArray.get());
    if ((tmpInts != NULL) && (bn_wexpand(ret, wlen) != NULL)) {
#ifdef __LP64__
      if (len % 2) {
        ret->d[wlen - 1] = tmpInts[--len];
      }
      if (len > 0) {
        for (int i = len - 2; i >= 0; i -= 2) {
          ret->d[i/2] = ((unsigned long long)tmpInts[i+1] << 32) | tmpInts[i];
        }
      }
#else
      int i = len; do { i--; ret->d[i] = tmpInts[i]; } while (i > 0);
#endif
      ret->top = wlen;
      ret->neg = neg;
      // need to call this due to clear byte at top if avoiding
      // having the top bit set (-ve number)
      // Basically get rid of top zero ints:
      bn_correct_top(ret);
    } else {
      throwExceptionIfNecessary(env);
    }
  } else { // (len = 0) means value = 0 and sign will be 0, too.
    ret->top = 0;
  }
}


#ifdef __LP64__
#define BYTES2ULONG(bytes, k) \
    ((bytes[k + 7] & 0xffULL)       | (bytes[k + 6] & 0xffULL) <<  8 | (bytes[k + 5] & 0xffULL) << 16 | (bytes[k + 4] & 0xffULL) << 24 | \
     (bytes[k + 3] & 0xffULL) << 32 | (bytes[k + 2] & 0xffULL) << 40 | (bytes[k + 1] & 0xffULL) << 48 | (bytes[k + 0] & 0xffULL) << 56)
#else
#define BYTES2ULONG(bytes, k) \
    ((bytes[k + 3] & 0xff) | (bytes[k + 2] & 0xff) << 8 | (bytes[k + 1] & 0xff) << 16 | (bytes[k + 0] & 0xff) << 24)
#endif
static void negBigEndianBytes2bn(JNIEnv*, jclass, const unsigned char* bytes, int bytesLen, jlong ret0) {
  BIGNUM* ret = toBigNum(ret0);

  bn_check_top(ret);
  // FIXME: assert bytesLen > 0
  int wLen = (bytesLen + BN_BYTES - 1) / BN_BYTES;
  int firstNonzeroDigit = -2;
  if (bn_wexpand(ret, wLen) != NULL) {
    BN_ULONG* d = ret->d;
    BN_ULONG di;
    ret->top = wLen;
    int highBytes = bytesLen % BN_BYTES;
    int k = bytesLen;
    // Put bytes to the int array starting from the end of the byte array
    int i = 0;
    while (k > highBytes) {
      k -= BN_BYTES;
      di = BYTES2ULONG(bytes, k);
      if (di != 0) {
        d[i] = -di;
        firstNonzeroDigit = i;
        i++;
        while (k > highBytes) {
          k -= BN_BYTES;
          d[i] = ~BYTES2ULONG(bytes, k);
          i++;
        }
        break;
      } else {
        d[i] = 0;
        i++;
      }
    }
    if (highBytes != 0) {
      di = -1;
      // Put the first bytes in the highest element of the int array
      if (firstNonzeroDigit != -2) {
        for (k = 0; k < highBytes; k++) {
          di = (di << 8) | (bytes[k] & 0xFF);
        }
        d[i] = ~di;
      } else {
        for (k = 0; k < highBytes; k++) {
          di = (di << 8) | (bytes[k] & 0xFF);
        }
        d[i] = -di;
      }
    }
    // The top may have superfluous zeros, so fix it.
    bn_correct_top(ret);
  }
}

static void NativeBN_twosComp2bn(JNIEnv* env, jclass cls, jbyteArray arr, int bytesLen, jlong ret0) {
  if (!oneValidHandle(env, ret0)) return;
  BIGNUM* ret = toBigNum(ret0);

  ScopedByteArrayRO bytes(env, arr);
  if (bytes.get() == NULL) {
    return;
  }
  const unsigned char* s = reinterpret_cast<const unsigned char*>(bytes.get());
  if ((bytes[0] & 0X80) == 0) { // Positive value!
    //
    // We can use the existing BN implementation for unsigned big endian bytes:
    //
    BN_bin2bn(s, bytesLen, ret);
    BN_set_negative(ret, false);
  } else { // Negative value!
    //
    // We need to apply two's complement:
    //
    negBigEndianBytes2bn(env, cls, s, bytesLen, ret0);
    BN_set_negative(ret, true);
  }
  throwExceptionIfNecessary(env);
}

static jlong NativeBN_longInt(JNIEnv* env, jclass, jlong a0) {
  if (!oneValidHandle(env, a0)) return -1;

  BIGNUM* a = toBigNum(a0);
  bn_check_top(a);
  int wLen = a->top;
  if (wLen == 0) {
    return 0;
  }

#ifdef __LP64__
  jlong result = a->d[0];
#else
  jlong result = static_cast<jlong>(a->d[0]) & 0xffffffff;
  if (wLen > 1) {
    result |= static_cast<jlong>(a->d[1]) << 32;
  }
#endif
  return a->neg ? -result : result;
}

static char* leadingZerosTrimmed(char* s) {
    char* p = s;
    if (*p == '-') {
        p++;
        while ((*p == '0') && (*(p + 1) != 0)) { p++; }
        p--;
        *p = '-';
    } else {
        while ((*p == '0') && (*(p + 1) != 0)) { p++; }
    }
    return p;
}

static jstring NativeBN_BN_bn2dec(JNIEnv* env, jclass, jlong a) {
  if (!oneValidHandle(env, a)) return NULL;
  char* tmpStr = BN_bn2dec(toBigNum(a));
  if (tmpStr == NULL) {
    return NULL;
  }
  char* retStr = leadingZerosTrimmed(tmpStr);
  jstring returnJString = env->NewStringUTF(retStr);
  OPENSSL_free(tmpStr);
  return returnJString;
}

static jstring NativeBN_BN_bn2hex(JNIEnv* env, jclass, jlong a) {
  if (!oneValidHandle(env, a)) return NULL;
  char* tmpStr = BN_bn2hex(toBigNum(a));
  if (tmpStr == NULL) {
    return NULL;
  }
  char* retStr = leadingZerosTrimmed(tmpStr);
  jstring returnJString = env->NewStringUTF(retStr);
  OPENSSL_free(tmpStr);
  return returnJString;
}

static jbyteArray NativeBN_BN_bn2bin(JNIEnv* env, jclass, jlong a0) {
  if (!oneValidHandle(env, a0)) return NULL;
  BIGNUM* a = toBigNum(a0);
  jbyteArray result = env->NewByteArray(BN_num_bytes(a));
  if (result == NULL) {
    return NULL;
  }
  ScopedByteArrayRW bytes(env, result);
  if (bytes.get() == NULL) {
    return NULL;
  }
  BN_bn2bin(a, reinterpret_cast<unsigned char*>(bytes.get()));
  return result;
}

static jintArray NativeBN_bn2litEndInts(JNIEnv* env, jclass, jlong a0) {
  if (!oneValidHandle(env, a0)) return NULL;
  BIGNUM* a = toBigNum(a0);
  bn_check_top(a);
  int wLen = a->top;
  if (wLen == 0) {
    return NULL;
  }
  jintArray result = env->NewIntArray(wLen * BN_BYTES/sizeof(unsigned int));
  if (result == NULL) {
    return NULL;
  }
  ScopedIntArrayRW ints(env, result);
  if (ints.get() == NULL) {
    return NULL;
  }
  unsigned int* uints = reinterpret_cast<unsigned int*>(ints.get());
  if (uints == NULL) {
    return NULL;
  }
#ifdef __LP64__
  int i = wLen; do { i--; uints[i*2+1] = a->d[i] >> 32; uints[i*2] = a->d[i]; } while (i > 0);
#else
  int i = wLen; do { i--; uints[i] = a->d[i]; } while (i > 0);
#endif
  return result;
}

static int NativeBN_sign(JNIEnv* env, jclass, jlong a) {
  if (!oneValidHandle(env, a)) return -2;
  if (BN_is_zero(toBigNum(a))) {
      return 0;
  } else if (BN_is_negative(toBigNum(a))) {
    return -1;
  }
  return 1;
}

static void NativeBN_BN_set_negative(JNIEnv* env, jclass, jlong b, int n) {
  if (!oneValidHandle(env, b)) return;
  BN_set_negative(toBigNum(b), n);
}

static int NativeBN_bitLength(JNIEnv* env, jclass, jlong a0) {
  if (!oneValidHandle(env, a0)) return JNI_FALSE;
  BIGNUM* a = toBigNum(a0);
  bn_check_top(a);
  int wLen = a->top;
  if (wLen == 0) return 0;
  BN_ULONG* d = a->d;
  int i = wLen - 1;
  BN_ULONG msd = d[i]; // most significant digit
  if (a->neg) {
    // Handle negative values correctly:
    // i.e. decrement the msd if all other digits are 0:
    // while ((i > 0) && (d[i] != 0)) { i--; }
    do { i--; } while (!((i < 0) || (d[i] != 0)));
    if (i < 0) msd--; // Only if all lower significant digits are 0 we decrement the most significant one.
  }
  return (wLen - 1) * BN_BYTES * 8 + BN_num_bits_word(msd);
}

static jboolean NativeBN_BN_is_bit_set(JNIEnv* env, jclass, jlong a, int n) {
  if (!oneValidHandle(env, a)) return JNI_FALSE;
  return BN_is_bit_set(toBigNum(a), n);
}

static void NativeBN_BN_shift(JNIEnv* env, jclass, jlong r, jlong a, int n) {
  if (!twoValidHandles(env, r, a)) return;
  if (n >= 0) {
    BN_lshift(toBigNum(r), toBigNum(a), n);
  } else {
    BN_rshift(toBigNum(r), toBigNum(a), -n);
  }
  throwExceptionIfNecessary(env);
}

static void NativeBN_BN_add_word(JNIEnv* env, jclass, jlong a, BN_ULONG w) {
  if (!oneValidHandle(env, a)) return;
  BN_add_word(toBigNum(a), w);
  throwExceptionIfNecessary(env);
}

static void NativeBN_BN_mul_word(JNIEnv* env, jclass, jlong a, BN_ULONG w) {
  if (!oneValidHandle(env, a)) return;
  BN_mul_word(toBigNum(a), w);
  throwExceptionIfNecessary(env);
}

static BN_ULONG NativeBN_BN_mod_word(JNIEnv* env, jclass, jlong a, BN_ULONG w) {
  if (!oneValidHandle(env, a)) return 0;
  int result = BN_mod_word(toBigNum(a), w);
  throwExceptionIfNecessary(env);
  return result;
}

static void NativeBN_BN_add(JNIEnv* env, jclass, jlong r, jlong a, jlong b) {
  if (!threeValidHandles(env, r, a, b)) return;
  BN_add(toBigNum(r), toBigNum(a), toBigNum(b));
  throwExceptionIfNecessary(env);
}

static void NativeBN_BN_sub(JNIEnv* env, jclass, jlong r, jlong a, jlong b) {
  if (!threeValidHandles(env, r, a, b)) return;
  BN_sub(toBigNum(r), toBigNum(a), toBigNum(b));
  throwExceptionIfNecessary(env);
}

static void NativeBN_BN_gcd(JNIEnv* env, jclass, jlong r, jlong a, jlong b) {
  if (!threeValidHandles(env, r, a, b)) return;
  Unique_BN_CTX ctx(BN_CTX_new());
  BN_gcd(toBigNum(r), toBigNum(a), toBigNum(b), ctx.get());
  throwExceptionIfNecessary(env);
}

static void NativeBN_BN_mul(JNIEnv* env, jclass, jlong r, jlong a, jlong b) {
  if (!threeValidHandles(env, r, a, b)) return;
  Unique_BN_CTX ctx(BN_CTX_new());
  BN_mul(toBigNum(r), toBigNum(a), toBigNum(b), ctx.get());
  throwExceptionIfNecessary(env);
}

static void NativeBN_BN_exp(JNIEnv* env, jclass, jlong r, jlong a, jlong p) {
  if (!threeValidHandles(env, r, a, p)) return;
  Unique_BN_CTX ctx(BN_CTX_new());
  BN_exp(toBigNum(r), toBigNum(a), toBigNum(p), ctx.get());
  throwExceptionIfNecessary(env);
}

static void NativeBN_BN_div(JNIEnv* env, jclass, jlong dv, jlong rem, jlong m, jlong d) {
  if (!fourValidHandles(env, (rem ? rem : dv), (dv ? dv : rem), m, d)) return;
  Unique_BN_CTX ctx(BN_CTX_new());
  BN_div(toBigNum(dv), toBigNum(rem), toBigNum(m), toBigNum(d), ctx.get());
  throwExceptionIfNecessary(env);
}

static void NativeBN_BN_nnmod(JNIEnv* env, jclass, jlong r, jlong a, jlong m) {
  if (!threeValidHandles(env, r, a, m)) return;
  Unique_BN_CTX ctx(BN_CTX_new());
  BN_nnmod(toBigNum(r), toBigNum(a), toBigNum(m), ctx.get());
  throwExceptionIfNecessary(env);
}

static void NativeBN_BN_mod_exp(JNIEnv* env, jclass, jlong r, jlong a, jlong p, jlong m) {
  if (!fourValidHandles(env, r, a, p, m)) return;
  Unique_BN_CTX ctx(BN_CTX_new());
  BN_mod_exp(toBigNum(r), toBigNum(a), toBigNum(p), toBigNum(m), ctx.get());
  throwExceptionIfNecessary(env);
}

static void NativeBN_BN_mod_inverse(JNIEnv* env, jclass, jlong ret, jlong a, jlong n) {
  if (!threeValidHandles(env, ret, a, n)) return;
  Unique_BN_CTX ctx(BN_CTX_new());
  BN_mod_inverse(toBigNum(ret), toBigNum(a), toBigNum(n), ctx.get());
  throwExceptionIfNecessary(env);
}

static void NativeBN_BN_generate_prime_ex(JNIEnv* env, jclass, jlong ret, int bits,
                                          jboolean safe, jlong add, jlong rem, jlong cb) {
  if (!oneValidHandle(env, ret)) return;
  BN_generate_prime_ex(toBigNum(ret), bits, safe, toBigNum(add), toBigNum(rem),
                       reinterpret_cast<BN_GENCB*>(cb));
  throwExceptionIfNecessary(env);
}

static jboolean NativeBN_BN_is_prime_ex(JNIEnv* env, jclass, jlong p, int nchecks, jlong cb) {
  if (!oneValidHandle(env, p)) return JNI_FALSE;
  Unique_BN_CTX ctx(BN_CTX_new());
  return BN_is_prime_ex(toBigNum(p), nchecks, ctx.get(), reinterpret_cast<BN_GENCB*>(cb));
}

static JNINativeMethod gMethods[] = {
   NATIVE_METHOD(NativeBN, BN_add, "(JJJ)V"),
   NATIVE_METHOD(NativeBN, BN_add_word, "(JI)V"),
   NATIVE_METHOD(NativeBN, BN_bin2bn, "([BIZJ)V"),
   NATIVE_METHOD(NativeBN, BN_bn2bin, "(J)[B"),
   NATIVE_METHOD(NativeBN, BN_bn2dec, "(J)Ljava/lang/String;"),
   NATIVE_METHOD(NativeBN, BN_bn2hex, "(J)Ljava/lang/String;"),
   NATIVE_METHOD(NativeBN, BN_cmp, "(JJ)I"),
   NATIVE_METHOD(NativeBN, BN_copy, "(JJ)V"),
   NATIVE_METHOD(NativeBN, BN_dec2bn, "(JLjava/lang/String;)I"),
   NATIVE_METHOD(NativeBN, BN_div, "(JJJJ)V"),
   NATIVE_METHOD(NativeBN, BN_exp, "(JJJ)V"),
   NATIVE_METHOD(NativeBN, BN_free, "(J)V"),
   NATIVE_METHOD(NativeBN, BN_gcd, "(JJJ)V"),
   NATIVE_METHOD(NativeBN, BN_generate_prime_ex, "(JIZJJJ)V"),
   NATIVE_METHOD(NativeBN, BN_hex2bn, "(JLjava/lang/String;)I"),
   NATIVE_METHOD(NativeBN, BN_is_bit_set, "(JI)Z"),
   NATIVE_METHOD(NativeBN, BN_is_prime_ex, "(JIJ)Z"),
   NATIVE_METHOD(NativeBN, BN_mod_exp, "(JJJJ)V"),
   NATIVE_METHOD(NativeBN, BN_mod_inverse, "(JJJ)V"),
   NATIVE_METHOD(NativeBN, BN_mod_word, "(JI)I"),
   NATIVE_METHOD(NativeBN, BN_mul, "(JJJ)V"),
   NATIVE_METHOD(NativeBN, BN_mul_word, "(JI)V"),
   NATIVE_METHOD(NativeBN, BN_new, "()J"),
   NATIVE_METHOD(NativeBN, BN_nnmod, "(JJJ)V"),
   NATIVE_METHOD(NativeBN, BN_set_negative, "(JI)V"),
   NATIVE_METHOD(NativeBN, BN_shift, "(JJI)V"),
   NATIVE_METHOD(NativeBN, BN_sub, "(JJJ)V"),
   NATIVE_METHOD(NativeBN, bitLength, "(J)I"),
   NATIVE_METHOD(NativeBN, bn2litEndInts, "(J)[I"),
   NATIVE_METHOD(NativeBN, litEndInts2bn, "([IIZJ)V"),
   NATIVE_METHOD(NativeBN, longInt, "(J)J"),
   NATIVE_METHOD(NativeBN, putLongInt, "(JJ)V"),
   NATIVE_METHOD(NativeBN, putULongInt, "(JJZ)V"),
   NATIVE_METHOD(NativeBN, sign, "(J)I"),
   NATIVE_METHOD(NativeBN, twosComp2bn, "([BIJ)V"),
};
void register_java_math_NativeBN(JNIEnv* env) {
    jniRegisterNativeMethods(env, "java/math/NativeBN", gMethods, NELEM(gMethods));
}
