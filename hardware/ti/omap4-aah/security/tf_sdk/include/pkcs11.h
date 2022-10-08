/**
 * Copyright(c) 2011 Trusted Logic.   All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name Trusted Logic nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This header file contains the definition of the PKCS#11 types and functions
 * supported by the Trusted Foundations Software. This header file is
 * derived from the RSA Security Inc. PKCS #11 Cryptographic Token Interface
 * (Cryptoki)
 */
#ifndef __PKCS11_H__
#define __PKCS11_H__

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------
* Types and constants
*------------------------------------------*/

#include "s_type.h"

#define CK_TRUE true
#define CK_FALSE false

#ifndef FALSE
#define FALSE CK_FALSE
#endif

#ifndef TRUE
#define TRUE CK_TRUE
#endif

#define NULL_PTR NULL

typedef uint8_t      CK_BYTE, *CK_BYTE_PTR;
typedef CK_BYTE      CK_CHAR, *CK_CHAR_PTR;
typedef CK_BYTE      CK_UTF8CHAR, *CK_UTF8CHAR_PTR;
typedef bool         CK_BBOOL;
typedef uint32_t     CK_ULONG, *CK_ULONG_PTR;
typedef int32_t      CK_LONG;
typedef CK_ULONG     CK_FLAGS;
typedef void*        CK_VOID_PTR, *CK_VOID_PTR_PTR;

#define CK_INVALID_HANDLE 0

typedef struct CK_VERSION
{
   CK_BYTE       major;
   CK_BYTE       minor;
}
CK_VERSION, *CK_VERSION_PTR;

typedef struct CK_INFO
{
   CK_VERSION    cryptokiVersion;
   CK_UTF8CHAR   manufacturerID[32];
   CK_FLAGS      flags;
   CK_UTF8CHAR   libraryDescription[32];
   CK_VERSION    libraryVersion;
}
CK_INFO, *CK_INFO_PTR;

typedef CK_ULONG   CK_NOTIFICATION;
typedef CK_ULONG   CK_SLOT_ID, *CK_SLOT_ID_PTR;
typedef CK_ULONG   CK_SESSION_HANDLE, *CK_SESSION_HANDLE_PTR;

typedef CK_ULONG          CK_USER_TYPE;
#define CKU_SO                 0
#define CKU_USER               1
#define CKU_CONTEXT_SPECIFIC   2

typedef CK_ULONG          CK_STATE;
#define CKS_RO_PUBLIC_SESSION  0
#define CKS_RO_USER_FUNCTIONS  1
#define CKS_RW_PUBLIC_SESSION  2
#define CKS_RW_USER_FUNCTIONS  3
#define CKS_RW_SO_FUNCTIONS    4

typedef struct CK_SESSION_INFO
{
   CK_SLOT_ID    slotID;
   CK_STATE      state;
   CK_FLAGS      flags;
   CK_ULONG      ulDeviceError;
}
CK_SESSION_INFO, *CK_SESSION_INFO_PTR;

#define CKF_RW_SESSION          0x00000002
#define CKF_SERIAL_SESSION      0x00000004
#define CKVF_OPEN_SUB_SESSION   0x00000008

typedef CK_ULONG          CK_OBJECT_HANDLE, *CK_OBJECT_HANDLE_PTR;

typedef CK_ULONG          CK_OBJECT_CLASS, *CK_OBJECT_CLASS_PTR;

#define CKO_DATA              0x00000000
#define CKO_PUBLIC_KEY        0x00000002
#define CKO_PRIVATE_KEY       0x00000003
#define CKO_SECRET_KEY        0x00000004

typedef CK_ULONG          CK_KEY_TYPE;

#define CKK_RSA             0x00000000
#define CKK_DSA             0x00000001
#define CKK_DH              0x00000002
#define CKK_EC              0x00000003

#define CKK_GENERIC_SECRET  0x00000010

#define CKK_RC4             0x00000012
#define CKK_DES             0x00000013
#define CKK_DES2            0x00000014
#define CKK_DES3            0x00000015

#define CKK_AES             0x0000001F

#define CKK_VENDOR_DEFINED  0x80000000

typedef CK_ULONG          CK_ATTRIBUTE_TYPE;

#define CKF_ARRAY_ATTRIBUTE    0x40000000

#define CKA_CLASS              0x00000000
#define CKA_TOKEN              0x00000001
#define CKA_PRIVATE            0x00000002
#define CKA_VALUE              0x00000011

#define CKA_OBJECT_ID          0x00000012

#define CKA_KEY_TYPE           0x00000100
#define CKA_ID                 0x00000102
#define CKA_SENSITIVE          0x00000103
#define CKA_ENCRYPT            0x00000104
#define CKA_DECRYPT            0x00000105
#define CKA_WRAP               0x00000106
#define CKA_UNWRAP             0x00000107
#define CKA_SIGN               0x00000108
#define CKA_VERIFY             0x0000010A
#define CKA_DERIVE             0x0000010C
#define CKA_MODULUS            0x00000120
#define CKA_MODULUS_BITS       0x00000121
#define CKA_PUBLIC_EXPONENT    0x00000122
#define CKA_PRIVATE_EXPONENT   0x00000123
#define CKA_PRIME_1            0x00000124
#define CKA_PRIME_2            0x00000125
#define CKA_EXPONENT_1         0x00000126
#define CKA_EXPONENT_2         0x00000127
#define CKA_COEFFICIENT        0x00000128
#define CKA_PRIME              0x00000130
#define CKA_SUBPRIME           0x00000131
#define CKA_BASE               0x00000132

#define CKA_VALUE_BITS         0x00000160
#define CKA_VALUE_LEN          0x00000161

#define CKA_EXTRACTABLE        0x00000162

#define CKA_MODIFIABLE         0x00000170
#define CKA_COPYABLE           0x00000171
#define CKA_ALWAYS_AUTHENTICATE  0x00000202

#define CKA_VENDOR_DEFINED     0x80000000

#define CKAV_ALLOW_NON_SENSITIVE_DERIVED_KEY 0x80000001

typedef struct CK_ATTRIBUTE
{
   CK_ATTRIBUTE_TYPE type;
   void*             pValue;
   CK_ULONG          ulValueLen;
}
CK_ATTRIBUTE, *CK_ATTRIBUTE_PTR;

typedef CK_ULONG          CK_MECHANISM_TYPE, *CK_MECHANISM_TYPE_PTR;

#define CKM_RSA_PKCS_KEY_PAIR_GEN      0x00000000
#define CKM_RSA_PKCS                   0x00000001
#define CKM_RSA_X_509                  0x00000003
#define CKM_MD5_RSA_PKCS               0x00000005
#define CKM_SHA1_RSA_PKCS              0x00000006
#define CKM_RSA_PKCS_OAEP              0x00000009
#define CKM_RSA_PKCS_PSS               0x0000000D
#define CKM_SHA1_RSA_PKCS_PSS          0x0000000E
#define CKM_DSA_KEY_PAIR_GEN           0x00000010
#define CKM_DSA                        0x00000011
#define CKM_DSA_SHA1                   0x00000012
#define CKM_DH_PKCS_KEY_PAIR_GEN       0x00000020
#define CKM_DH_PKCS_DERIVE             0x00000021
#define CKM_SHA256_RSA_PKCS            0x00000040
#define CKM_SHA384_RSA_PKCS            0x00000041
#define CKM_SHA512_RSA_PKCS            0x00000042
#define CKM_SHA256_RSA_PKCS_PSS        0x00000043
#define CKM_SHA384_RSA_PKCS_PSS        0x00000044
#define CKM_SHA512_RSA_PKCS_PSS        0x00000045
#define CKM_SHA224_RSA_PKCS            0x00000046
#define CKM_SHA224_RSA_PKCS_PSS        0x00000047
#define CKM_RC4_KEY_GEN                0x00000110
#define CKM_RC4                        0x00000111
#define CKM_DES_KEY_GEN                0x00000120
#define CKM_DES_ECB                    0x00000121
#define CKM_DES_CBC                    0x00000122
#define CKM_DES_MAC                    0x00000123
#define CKM_DES2_KEY_GEN               0x00000130
#define CKM_DES3_KEY_GEN               0x00000131
#define CKM_DES3_ECB                   0x00000132
#define CKM_DES3_CBC                   0x00000133
#define CKM_DES3_MAC                   0x00000134
#define CKM_MD5                        0x00000210
#define CKM_MD5_HMAC                   0x00000211
#define CKM_SHA_1                      0x00000220
#define CKM_SHA_1_HMAC                 0x00000221
#define CKM_SHA256                     0x00000250
#define CKM_SHA256_HMAC                0x00000251
#define CKM_SHA224                     0x00000255
#define CKM_SHA224_HMAC                0x00000256
#define CKM_SHA384                     0x00000260
#define CKM_SHA384_HMAC                0x00000261
#define CKM_SHA512                     0x00000270
#define CKM_SHA512_HMAC                0x00000271
#define CKM_GENERIC_SECRET_KEY_GEN     0x00000350
#define CKM_AES_KEY_GEN                0x00001080
#define CKM_AES_ECB                    0x00001081
#define CKM_AES_CBC                    0x00001082
#define CKM_AES_MAC                    0x00001083
#define CKM_AES_CTR                    0x00001086
#define CKM_VENDOR_DEFINED             0x80000000
#define CKMV_AES_CTR                   0x80000001

#define CKMV_IMPLEMENTATION_DEFINED_0   0xC0000000
#define CKMV_IMPLEMENTATION_DEFINED_1   0xC0000001
#define CKMV_IMPLEMENTATION_DEFINED_2   0xC0000002
#define CKMV_IMPLEMENTATION_DEFINED_3   0xC0000003
#define CKMV_IMPLEMENTATION_DEFINED_4   0xC0000004
#define CKMV_IMPLEMENTATION_DEFINED_5   0xC0000005
#define CKMV_IMPLEMENTATION_DEFINED_6   0xC0000006
#define CKMV_IMPLEMENTATION_DEFINED_7   0xC0000007
#define CKMV_IMPLEMENTATION_DEFINED_8   0xC0000008
#define CKMV_IMPLEMENTATION_DEFINED_9   0xC0000009
#define CKMV_IMPLEMENTATION_DEFINED_10  0xC000000A
#define CKMV_IMPLEMENTATION_DEFINED_11  0xC000000B
#define CKMV_IMPLEMENTATION_DEFINED_12  0xC000000C
#define CKMV_IMPLEMENTATION_DEFINED_13  0xC000000D
#define CKMV_IMPLEMENTATION_DEFINED_14  0xC000000E
#define CKMV_IMPLEMENTATION_DEFINED_15  0xC000000F

typedef struct CK_MECHANISM
{
   CK_MECHANISM_TYPE mechanism;
   void*             pParameter;
   CK_ULONG          ulParameterLen;  /* in bytes */
}
CK_MECHANISM, *CK_MECHANISM_PTR;

typedef CK_ULONG          CK_RV;

#define CKR_OK                                0x00000000
#define CKR_CANCEL                            0x00000001
#define CKR_HOST_MEMORY                       0x00000002
#define CKR_SLOT_ID_INVALID                   0x00000003
#define CKR_GENERAL_ERROR                     0x00000005
#define CKR_ARGUMENTS_BAD                     0x00000007
#define CKR_ATTRIBUTE_SENSITIVE               0x00000011
#define CKR_ATTRIBUTE_TYPE_INVALID            0x00000012
#define CKR_ATTRIBUTE_VALUE_INVALID           0x00000013
#define CKR_COPY_PROHIBITED                   0x0000001A
#define CKR_DATA_INVALID                      0x00000020
#define CKR_DATA_LEN_RANGE                    0x00000021
#define CKR_DEVICE_ERROR                      0x00000030
#define CKR_DEVICE_MEMORY                     0x00000031
#define CKR_ENCRYPTED_DATA_INVALID            0x00000040
#define CKR_ENCRYPTED_DATA_LEN_RANGE          0x00000041
#define CKR_KEY_HANDLE_INVALID                0x00000060
#define CKR_KEY_SIZE_RANGE                    0x00000062
#define CKR_KEY_TYPE_INCONSISTENT             0x00000063
#define CKR_KEY_FUNCTION_NOT_PERMITTED        0x00000068
#define CKR_KEY_NOT_WRAPPABLE                 0x00000069
#define CKR_MECHANISM_INVALID                 0x00000070
#define CKR_MECHANISM_PARAM_INVALID           0x00000071
#define CKR_OBJECT_HANDLE_INVALID             0x00000082
#define CKR_OPERATION_ACTIVE                  0x00000090
#define CKR_OPERATION_NOT_INITIALIZED         0x00000091
#define CKR_PIN_INCORRECT                     0x000000A0
#define CKR_SESSION_COUNT                     0x000000B1
#define CKR_SESSION_HANDLE_INVALID            0x000000B3
#define CKR_SESSION_PARALLEL_NOT_SUPPORTED    0x000000B4
#define CKR_SESSION_READ_ONLY                 0x000000B5
#define CKR_SIGNATURE_INVALID                 0x000000C0
#define CKR_SIGNATURE_LEN_RANGE               0x000000C1
#define CKR_TEMPLATE_INCOMPLETE               0x000000D0
#define CKR_TEMPLATE_INCONSISTENT             0x000000D1
#define CKR_TOKEN_NOT_PRESENT                 0x000000E0
#define CKR_USER_ALREADY_LOGGED_IN            0x00000100
#define CKR_USER_NOT_LOGGED_IN                0x00000101
#define CKR_USER_TYPE_INVALID                 0x00000103
#define CKR_WRAPPED_KEY_LEN_RANGE             0x00000112
#define CKR_WRAPPING_KEY_HANDLE_INVALID       0x00000113
#define CKR_RANDOM_SEED_NOT_SUPPORTED         0x00000120
#define CKR_RANDOM_NO_RNG                     0x00000121
#define CKR_BUFFER_TOO_SMALL                  0x00000150
#define CKR_CRYPTOKI_NOT_INITIALIZED          0x00000190
#define CKR_CRYPTOKI_ALREADY_INITIALIZED      0x00000191
#define CKR_VENDOR_DEFINED                    0x80000000

typedef CK_RV (*CK_NOTIFY)(
   CK_SESSION_HANDLE hSession,
   CK_NOTIFICATION   event,
   void*       pApplication
);

typedef CK_ULONG CK_RSA_PKCS_MGF_TYPE, *CK_RSA_PKCS_MGF_TYPE_PTR;

#define CKG_MGF1_SHA1         0x00000001
#define CKG_MGF1_SHA256       0x00000002
#define CKG_MGF1_SHA384       0x00000003
#define CKG_MGF1_SHA512       0x00000004
#define CKG_MGF1_SHA224       0x00000005

typedef CK_ULONG CK_RSA_PKCS_OAEP_SOURCE_TYPE, *CK_RSA_PKCS_OAEP_SOURCE_TYPE_PTR;

#define CKZ_DATA_SPECIFIED    0x00000001
typedef struct CK_RSA_PKCS_OAEP_PARAMS
{
   CK_MECHANISM_TYPE hashAlg;
   CK_RSA_PKCS_MGF_TYPE mgf;
   CK_RSA_PKCS_OAEP_SOURCE_TYPE source;
   void*    pSourceData;
   CK_ULONG ulSourceDataLen;
}
CK_RSA_PKCS_OAEP_PARAMS, *CK_RSA_PKCS_OAEP_PARAMS_PTR;

typedef struct CK_RSA_PKCS_PSS_PARAMS
{
   CK_MECHANISM_TYPE    hashAlg;
   CK_RSA_PKCS_MGF_TYPE mgf;
   CK_ULONG             sLen;
}
CK_RSA_PKCS_PSS_PARAMS, *CK_RSA_PKCS_PSS_PARAMS_PTR;

typedef struct CK_AES_CTR_PARAMS
{
   CK_ULONG ulCounterBits;
   CK_BYTE cb[16];
}
CK_AES_CTR_PARAMS, *CK_AES_CTR_PARAMS_PTR;

/*------------------------------------------
* Functions
*------------------------------------------*/
CK_RV PKCS11_EXPORT C_Initialize(void* pInitArgs);

CK_RV PKCS11_EXPORT C_Finalize(void* pReserved);

CK_RV PKCS11_EXPORT C_GetInfo(CK_INFO* pInfo);

CK_RV PKCS11_EXPORT C_OpenSession(
   CK_SLOT_ID            slotID,
   CK_FLAGS              flags,
   void*                 pApplication,
   CK_NOTIFY             Notify,
   CK_SESSION_HANDLE*    phSession);

CK_RV PKCS11_EXPORT C_CloseSession(
   CK_SESSION_HANDLE hSession);

CK_RV PKCS11_EXPORT C_Login(
   CK_SESSION_HANDLE   hSession,
   CK_USER_TYPE        userType,
   const CK_UTF8CHAR*  pPin,
   CK_ULONG            ulPinLen);

CK_RV PKCS11_EXPORT C_Logout(
   CK_SESSION_HANDLE hSession);

CK_RV PKCS11_EXPORT C_CreateObject(
   CK_SESSION_HANDLE   hSession,
   const CK_ATTRIBUTE* pTemplate,
   CK_ULONG            ulCount,
   CK_OBJECT_HANDLE*   phObject);

CK_RV PKCS11_EXPORT C_DestroyObject(
   CK_SESSION_HANDLE   hSession,
   CK_OBJECT_HANDLE    hObject);

CK_RV PKCS11_EXPORT C_GetAttributeValue(
   CK_SESSION_HANDLE   hSession,
   CK_OBJECT_HANDLE    hObject,
   CK_ATTRIBUTE*       pTemplate,
   CK_ULONG            ulCount);

CK_RV PKCS11_EXPORT C_FindObjectsInit(
   CK_SESSION_HANDLE   hSession,
   const CK_ATTRIBUTE* pTemplate,
   CK_ULONG            ulCount);

CK_RV PKCS11_EXPORT C_FindObjects(
   CK_SESSION_HANDLE   hSession,
   CK_OBJECT_HANDLE*   phObject,
   CK_ULONG            ulMaxObjectCount,
   CK_ULONG*           pulObjectCount);

CK_RV PKCS11_EXPORT C_FindObjectsFinal(
   CK_SESSION_HANDLE hSession);

CK_RV PKCS11_EXPORT C_EncryptInit(
   CK_SESSION_HANDLE   hSession,
   const CK_MECHANISM* pMechanism,
   CK_OBJECT_HANDLE    hKey);

CK_RV PKCS11_EXPORT C_Encrypt(
   CK_SESSION_HANDLE hSession,
   const CK_BYTE*    pData,
   CK_ULONG          ulDataLen,
   CK_BYTE*          pEncryptedData,
   CK_ULONG*         pulEncryptedDataLen);

CK_RV PKCS11_EXPORT C_EncryptUpdate(
   CK_SESSION_HANDLE hSession,
   const CK_BYTE*    pPart,
   CK_ULONG          ulPartLen,
   CK_BYTE*          pEncryptedPart,
   CK_ULONG*         pulEncryptedPartLen);

CK_RV PKCS11_EXPORT C_EncryptFinal(
   CK_SESSION_HANDLE hSession,
   CK_BYTE*          pLastEncryptedPart,
   CK_ULONG*         pulLastEncryptedPartLen);

CK_RV PKCS11_EXPORT C_DecryptInit(
   CK_SESSION_HANDLE   hSession,
   const CK_MECHANISM* pMechanism,
   CK_OBJECT_HANDLE    hKey);

CK_RV PKCS11_EXPORT C_Decrypt(
   CK_SESSION_HANDLE hSession,
   const CK_BYTE*    pEncryptedData,
   CK_ULONG          ulEncryptedDataLen,
   CK_BYTE*          pData,
   CK_ULONG*         pulDataLen);

CK_RV PKCS11_EXPORT C_DecryptUpdate(
   CK_SESSION_HANDLE hSession,
   const CK_BYTE*    pEncryptedPart,
   CK_ULONG          ulEncryptedPartLen,
   CK_BYTE*          pPart,
   CK_ULONG*         pulPartLen);

CK_RV PKCS11_EXPORT C_DecryptFinal(
   CK_SESSION_HANDLE hSession,
   CK_BYTE*          pLastPart,
   CK_ULONG*         pulLastPartLen);

CK_RV PKCS11_EXPORT C_DigestInit(
   CK_SESSION_HANDLE   hSession,
   const CK_MECHANISM* pMechanism);

CK_RV PKCS11_EXPORT C_Digest(
   CK_SESSION_HANDLE hSession,
   const CK_BYTE*    pData,
   CK_ULONG          ulDataLen,
   CK_BYTE*          pDigest,
   CK_ULONG*         pulDigestLen);

CK_RV PKCS11_EXPORT C_DigestUpdate(
   CK_SESSION_HANDLE hSession,
   const CK_BYTE*    pPart,
   CK_ULONG          ulPartLen);

CK_RV PKCS11_EXPORT C_DigestFinal(
   CK_SESSION_HANDLE hSession,
   CK_BYTE*          pDigest,
   CK_ULONG*         pulDigestLen);

CK_RV PKCS11_EXPORT C_SignInit(
   CK_SESSION_HANDLE   hSession,
   const CK_MECHANISM* pMechanism,
   CK_OBJECT_HANDLE    hKey);

CK_RV PKCS11_EXPORT C_Sign(
   CK_SESSION_HANDLE  hSession,
   const CK_BYTE*     pData,
   CK_ULONG           ulDataLen,
   CK_BYTE*           pSignature,
   CK_ULONG*          pulSignatureLen);

CK_RV PKCS11_EXPORT C_SignUpdate(
   CK_SESSION_HANDLE hSession,
   const CK_BYTE*    pPart,
   CK_ULONG          ulPartLen);

CK_RV PKCS11_EXPORT C_SignFinal(
   CK_SESSION_HANDLE hSession,
   CK_BYTE*          pSignature,
   CK_ULONG*         pulSignatureLen);

CK_RV PKCS11_EXPORT C_VerifyInit(
   CK_SESSION_HANDLE   hSession,
   const CK_MECHANISM* pMechanism,
   CK_OBJECT_HANDLE    hKey);

CK_RV PKCS11_EXPORT C_Verify(
   CK_SESSION_HANDLE hSession,
   const CK_BYTE*    pData,
   CK_ULONG          ulDataLen,
   CK_BYTE*          pSignature,
   CK_ULONG          ulSignatureLen);

CK_RV PKCS11_EXPORT C_VerifyUpdate(
   CK_SESSION_HANDLE hSession,
   const CK_BYTE*    pPart,
   CK_ULONG          ulPartLen);

CK_RV PKCS11_EXPORT C_VerifyFinal(
   CK_SESSION_HANDLE hSession,
   const CK_BYTE*          pSignature,
   CK_ULONG          ulSignatureLen);

CK_RV PKCS11_EXPORT C_GenerateKey(
   CK_SESSION_HANDLE   hSession,
   const CK_MECHANISM* pMechanism,
   const CK_ATTRIBUTE* pTemplate,
   CK_ULONG            ulCount,
   CK_OBJECT_HANDLE*   phKey);

CK_RV PKCS11_EXPORT C_GenerateKeyPair(
   CK_SESSION_HANDLE    hSession,
   const CK_MECHANISM*  pMechanism,
   const CK_ATTRIBUTE*  pPublicKeyTemplate,
   CK_ULONG             ulPublicKeyAttributeCount,
   const CK_ATTRIBUTE*  pPrivateKeyTemplate,
   CK_ULONG             ulPrivateKeyAttributeCount,
   CK_OBJECT_HANDLE*    phPublicKey,
   CK_OBJECT_HANDLE*    phPrivateKey);

CK_RV PKCS11_EXPORT C_DeriveKey(
   CK_SESSION_HANDLE    hSession,
   const CK_MECHANISM*  pMechanism,
   CK_OBJECT_HANDLE     hBaseKey,
   const CK_ATTRIBUTE*  pTemplate,
   CK_ULONG             ulAttributeCount,
   CK_OBJECT_HANDLE*    phKey);

CK_RV PKCS11_EXPORT C_SeedRandom(
   CK_SESSION_HANDLE hSession,
   const CK_BYTE*    pSeed,
   CK_ULONG          ulSeedLen);

CK_RV PKCS11_EXPORT C_GenerateRandom(
   CK_SESSION_HANDLE hSession,
   CK_BYTE*          pRandomData,
   CK_ULONG          ulRandomLen);

CK_RV PKCS11_EXPORT C_CloseObjectHandle(
   CK_SESSION_HANDLE hSession,
   CK_OBJECT_HANDLE    hObject);

CK_RV PKCS11_EXPORT C_CopyObject(
   CK_SESSION_HANDLE    hSession,
   CK_OBJECT_HANDLE     hObject,
   const CK_ATTRIBUTE*  pTemplate,
   CK_ULONG             ulAttributeCount,
   CK_OBJECT_HANDLE*    phNewObject);

#ifdef __cplusplus
}
#endif

#endif /* __PKCS11_H__ */
