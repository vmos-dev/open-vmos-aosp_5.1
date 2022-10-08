/**
 * @defgroup MC_DATA_TYPES MobiCore generic data types
 *
 * @addtogroup MC_SO mcSo - Secure objects definitions.
 * <!-- Copyright Giesecke & Devrient GmbH 2011-2012 -->
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @ingroup  MC_DATA_TYPES
 * @{
 *
 */

#ifndef MC_SO_H_
#define MC_SO_H_

#include "mcUuid.h"
#include "mcSpid.h"

#define SO_USE_VERSION_22 TRUE

#if SO_USE_VERSION_22
  #define SO_VERSION_MAJOR   2
  #define SO_VERSION_MINOR   2
#else
  #define SO_VERSION_MAJOR   2
  #define SO_VERSION_MINOR   1
#endif

#define MC_ENUM_32BIT_SPACER           ((int32_t)-1)

/** Secure object type. */
typedef enum {
    /** Regular secure object. */
    MC_SO_TYPE_REGULAR = 0x00000001,
    /** Dummy to ensure that enum is 32 bit wide. */
    MC_SO_TYPE_DUMMY = MC_ENUM_32BIT_SPACER,
} mcSoType_t;


/** Secure object context.
 * A context defines which key to use to encrypt/decrypt a secure object.
 */
typedef enum {
    /** Trustlet context. */
    MC_SO_CONTEXT_TLT = 0x00000001,
     /** Service provider context. */
    MC_SO_CONTEXT_SP = 0x00000002,
     /** Device context. */
    MC_SO_CONTEXT_DEVICE = 0x00000003,
    /** Dummy to ensure that enum is 32 bit wide. */
    MC_SO_CONTEXT_DUMMY = MC_ENUM_32BIT_SPACER,
} mcSoContext_t;

/** Secure object lifetime.
 * A lifetime defines how long a secure object is valid.
 */
typedef enum {
    /** SO does not expire. */
    MC_SO_LIFETIME_PERMANENT = 0x00000000,
    /** SO expires on reboot (coldboot). */
    MC_SO_LIFETIME_POWERCYCLE = 0x00000001,
    /** SO expires when Trustlet is closed. */
    MC_SO_LIFETIME_SESSION = 0x00000002,
    /** Dummy to ensure that enum is 32 bit wide. */
    MC_SO_LIFETIME_DUMMY = MC_ENUM_32BIT_SPACER,
} mcSoLifeTime_t;

/** Service provider Trustlet id.
 * The combination of service provider id and Trustlet UUID forms a unique
 * Trustlet identifier.
 */
typedef struct {
    /** Service provider id. */
    mcSpid_t spid;
    /** Trustlet UUID. */
    mcUuid_t uuid;
} tlApiSpTrustletId_t;

/** Secure object header v2.2.
 * A secure object header introduces a secure object.
 * Layout of a secure object:
 * <pre>
 * <code>
 *
 *     +--------+------------------+------------------+--------+--------+
 *     | Header |   plain-data     |  encrypted-data  |  hash  | random |
 *     +--------+------------------+------------------+--------+--------+
 *
 *     /--------/---- plainLen ----/-- encryptedLen --/-- 32 --/-- 16 --/
 *
 *     /----------------- toBeHashedLen --------------/
 *
 *                                 /-- toBeEncryptedLen --/
 *
 *     /--------------------------- totalSoSize ------------------------/
 *
 * </code>
 * </pre>
 */

/** Secure object header v2.1.
 * A secure object header introduces a secure object.
 * Layout of a secure object:
 * <pre>
 * <code>
 *
 *     +--------+------------------+------------------+--------+--------+---------+
 *     | Header |   plain-data     |  encrypted-data  |  hash  | random | padding |
 *     +--------+------------------+------------------+--------+--------+---------+
 *
 *     /--------/---- plainLen ----/-- encryptedLen --/-- 24 --/--- 9 --/- 0..15 -/
 *
 *     /----------------- toBeHashedLen --------------/
 *
 *                                 /-- toBeEncryptedLen --/
 *
 *     /--------------------------- totalSoSize ----------------------------------/
 *
 * </code>
 * </pre>
 */

/** Secure object header v2.0.
 * A secure object header introduces a secure object.
 * Layout of a secure object:
 * <pre>
 * <code>
 *
 *     +--------+------------------+------------------+--------+---------+
 *     | Header |   plain-data     |  encrypted-data  |  hash  | padding |
 *     +--------+------------------+------------------+--------+---------+
 *
 *     /--------/---- plainLen ----/-- encryptedLen --/-- 32 --/- 1..16 -/
 *
 *     /----------------- toBeHashedLen --------------/
 *
 *                                 /---------- toBeEncryptedLen ---------/
 *
 *     /--------------------------- totalSoSize -------------------------/
 *
 * </code>
 * </pre>
 */
typedef struct {
    /** Type of secure object. */
    uint32_t type;
    /** Secure object version. */
    uint32_t version;
    /** Secure object context. */
    mcSoContext_t context;
    /** Secure object lifetime. */
    mcSoLifeTime_t lifetime;
    /** Producer Trustlet id. */
    tlApiSpTrustletId_t producer;
    /** Length of unencrypted user data (after the header). */
    uint32_t plainLen;
    /** Length of encrypted user data (after unencrypted data, excl. checksum
     * and excl. padding bytes). */
    uint32_t encryptedLen;
} mcSoHeader_t;

/** Maximum size of the payload (plain length + encrypted length) of a secure object. */
#define MC_SO_PAYLOAD_MAX_SIZE      1000000

/** Block size of encryption algorithm used for secure objects. */
#define MC_SO_ENCRYPT_BLOCK_SIZE    16

/** Maximum number of ISO padding bytes. */
#define MC_SO_MAX_PADDING_SIZE (MC_SO_ENCRYPT_BLOCK_SIZE)

/** Size of hash used for secure objects v2. */
#define MC_SO_HASH_SIZE             32

/** Size of hash used for secure object v2.1. */
#define MC_SO21_HASH_SIZE            24
/** Size of random used for secure objects v2.1. */
#define MC_SO21_RND_SIZE             9

/** Size of hash used for secure object v2.2. */
#define MC_SO22_HASH_SIZE            32
/** Size of random used for secure objects v2.2. */
#define MC_SO22_RND_SIZE             16

/** Hash size for current generated wrapping */
#define MC_SO2X_HASH_SIZE (SO_USE_VERSION_22 ? MC_SO22_HASH_SIZE : MC_SO21_HASH_SIZE)
/** Random size for current generated wrapping */
#define MC_SO2X_RND_SIZE (SO_USE_VERSION_22 ? MC_SO22_RND_SIZE : MC_SO21_RND_SIZE)

#define MC_SO_ENCRYPT_PADDED_SIZE_F21(netsize) ( (netsize) + \
    MC_SO_MAX_PADDING_SIZE - (netsize) % MC_SO_MAX_PADDING_SIZE )

#if SO_USE_VERSION_22
    // No encryption padding at all.
#else
    /** Calculates gross size of cryptogram within secure object including ISO padding bytes. */
    #define MC_SO_ENCRYPT_PADDED_SIZE(netsize) MC_SO_ENCRYPT_PADDED_SIZE_F21(netsize)
#endif


/** Calculates the total size of a secure object.
 * @param plainLen Length of plain text part within secure object.
 * @param encryptedLen Length of encrypted part within secure object (excl.
 * hash, padding).
 * @return Total (gross) size of the secure object or 0 if given parameters are
 * illegal or would lead to a secure object of invalid size.
 */
#define MC_SO_SIZE_F22(plainLen, encryptedLen) ( \
    ((plainLen) + (encryptedLen) < (encryptedLen) || (plainLen) + (encryptedLen) > MC_SO_PAYLOAD_MAX_SIZE) ? 0 : \
            sizeof(mcSoHeader_t) + (plainLen) + (encryptedLen) +MC_SO22_HASH_SIZE +MC_SO22_RND_SIZE \
    )
#define MC_SO_SIZE_F21(plainLen, encryptedLen) ( \
    ((plainLen) + (encryptedLen) < (encryptedLen) || (plainLen) + (encryptedLen) > MC_SO_PAYLOAD_MAX_SIZE) ? 0 : \
            sizeof(mcSoHeader_t) + (plainLen) + MC_SO_ENCRYPT_PADDED_SIZE_F21((encryptedLen) + MC_SO_HASH_SIZE) \
)

#if SO_USE_VERSION_22
    #define MC_SO_SIZE(plainLen, encryptedLen) MC_SO_SIZE_F22(plainLen, encryptedLen)
#else
    #define MC_SO_SIZE(plainLen, encryptedLen) MC_SO_SIZE_F21(plainLen, encryptedLen)
#endif

#endif // MC_SO_H_

/** @} */
