/*
 * Copyright (C) 2014 The Android Open Source Project
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

#define _LARGEFILE64_SOURCE

#include <endian.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include <openssl/asn1.h>
#include <openssl/asn1t.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>

#include "bootimg.h"

#define FORMAT_VERSION 1
#define BUFFER_SIZE (1024 * 1024)

typedef struct {
    ASN1_STRING *target;
    ASN1_INTEGER *length;
} AuthAttrs;

ASN1_SEQUENCE(AuthAttrs) = {
    ASN1_SIMPLE(AuthAttrs, target, ASN1_PRINTABLE),
    ASN1_SIMPLE(AuthAttrs, length, ASN1_INTEGER)
} ASN1_SEQUENCE_END(AuthAttrs)

IMPLEMENT_ASN1_FUNCTIONS(AuthAttrs)

typedef struct {
    ASN1_INTEGER *formatVersion;
    X509 *certificate;
    X509_ALGOR *algorithmIdentifier;
    AuthAttrs *authenticatedAttributes;
    ASN1_OCTET_STRING *signature;
} BootSignature;

ASN1_SEQUENCE(BootSignature) = {
    ASN1_SIMPLE(BootSignature, formatVersion, ASN1_INTEGER),
    ASN1_SIMPLE(BootSignature, certificate, X509),
    ASN1_SIMPLE(BootSignature, algorithmIdentifier, X509_ALGOR),
    ASN1_SIMPLE(BootSignature, authenticatedAttributes, AuthAttrs),
    ASN1_SIMPLE(BootSignature, signature, ASN1_OCTET_STRING)
} ASN1_SEQUENCE_END(BootSignature)

IMPLEMENT_ASN1_FUNCTIONS(BootSignature)

static BIO *g_error = NULL;

/**
 * Rounds n up to the nearest multiple of page_size
 * @param n The value to round
 * @param page_size Page size
 */
static uint64_t page_align(uint64_t n, uint64_t page_size)
{
    return (((n + page_size - 1) / page_size) * page_size);
}

/**
 * Calculates the offset to the beginning of the BootSignature block
 * based on the boot image header. The signature will start after the
 * the boot image contents.
 * @param fd File descriptor to the boot image
 * @param offset Receives the offset in bytes
 */
static int get_signature_offset(int fd, off64_t *offset)
{
    int i;
    struct boot_img_hdr hdr;

    if (!offset) {
        return -1;
    }

    if (read(fd, &hdr, sizeof(hdr)) != sizeof(hdr)) {
        return -1;
    }

    if (memcmp(BOOT_MAGIC, hdr.magic, BOOT_MAGIC_SIZE) != 0) {
        printf("Invalid boot image: missing magic\n");
        return -1;
    }

    if (!hdr.page_size) {
        printf("Invalid boot image: page size must be non-zero\n");
        return -1;
    }

    *offset = page_align(hdr.page_size
                    + page_align(hdr.kernel_size,  hdr.page_size)
                    + page_align(hdr.ramdisk_size, hdr.page_size)
                    + page_align(hdr.second_size,  hdr.page_size),
                hdr.page_size);

    return 0;
}

/**
 * Reads and parses the ASN.1 BootSignature block from the given offset
 * @param fd File descriptor to the boot image
 * @param offset Offset from the beginning of file to the signature
 * @param bs Pointer to receive the BootImage structure
 */
static int read_signature(int fd, off64_t offset, BootSignature **bs)
{
    BIO *in = NULL;

    if (!bs) {
        return -1;
    }

    if (lseek64(fd, offset, SEEK_SET) == -1) {
        return -1;
    }

    if ((in = BIO_new_fd(fd, BIO_NOCLOSE)) == NULL) {
        ERR_print_errors(g_error);
        return -1;
    }

    if ((*bs = ASN1_item_d2i_bio(ASN1_ITEM_rptr(BootSignature), in, bs)) == NULL) {
        ERR_print_errors(g_error);
        BIO_free(in);
        return -1;
    }

    BIO_free(in);
    return 0;
}

/**
 * Validates the format of the boot signature block, and checks that
 * the length in authenticated attributes matches the actual length of
 * the image.
 * @param bs The boot signature block to validate
 * @param length The actual length of the boot image without the signature
 */
static int validate_signature_block(const BootSignature *bs, uint64_t length)
{
    BIGNUM expected;
    BIGNUM value;
    int rc = -1;

    if (!bs) {
        return -1;
    }

    BN_init(&expected);
    BN_init(&value);

    /* Confirm that formatVersion matches our supported version */
    if (!BN_set_word(&expected, FORMAT_VERSION)) {
        ERR_print_errors(g_error);
        goto vsb_done;
    }

    ASN1_INTEGER_to_BN(bs->formatVersion, &value);

    if (BN_cmp(&expected, &value) != 0) {
        printf("Unsupported signature version\n");
        goto vsb_done;
    }

    BN_clear(&expected);
    BN_clear(&value);

    /* Confirm that the length of the image matches with the length in
        the authenticated attributes */
    length = htobe64(length);
    BN_bin2bn((const unsigned char *) &length, sizeof(length), &expected);

    ASN1_INTEGER_to_BN(bs->authenticatedAttributes->length, &value);

    if (BN_cmp(&expected, &value) != 0) {
        printf("Image length doesn't match signature attributes\n");
        goto vsb_done;
    }

    rc = 0;

vsb_done:
    BN_free(&expected);
    BN_free(&value);

    return rc;
}

/**
 * Creates a SHA-256 hash from the boot image contents and the encoded
 * authenticated attributes.
 * @param fd File descriptor to the boot image
 * @param length Length of the boot image without the signature block
 * @param aa Pointer to AuthAttrs
 * @param digest Pointer to a buffer where the hash is written
 */
static int hash_image(int fd, uint64_t length, const AuthAttrs *aa,
        unsigned char *digest)
{
    EVP_MD_CTX *ctx = NULL;
    int rc = -1;

    ssize_t bytes = 0;
    unsigned char *attrs = NULL;
    unsigned char *buffer = NULL;
    unsigned char *p = NULL;
    uint64_t total = 0;

    if (!aa || !digest) {
        goto hi_done;
    }

    if ((buffer = malloc(BUFFER_SIZE)) == NULL) {
        goto hi_done;
    }

    if (lseek64(fd, 0, SEEK_SET) != 0) {
        goto hi_done;
    }

    if ((ctx = EVP_MD_CTX_create()) == NULL) {
        ERR_print_errors(g_error);
        goto hi_done;
    }

    EVP_DigestInit(ctx, EVP_sha256());

    do {
        bytes = BUFFER_SIZE;

        if ((length - total) < BUFFER_SIZE) {
            bytes = length - total;
        }

        if ((bytes = read(fd, buffer, bytes)) == -1) {
            printf("%s\n", strerror(errno));
            goto hi_done;
        }

        EVP_DigestUpdate(ctx, buffer, bytes);
        total += bytes;
    } while (total < length);

    if ((bytes = i2d_AuthAttrs((AuthAttrs *) aa, NULL)) < 0) {
        ERR_print_errors(g_error);
        goto hi_done;
    }

    if ((attrs = OPENSSL_malloc(bytes)) == NULL) {
        ERR_print_errors(g_error);
        goto hi_done;
    }

    p = attrs;

    if (i2d_AuthAttrs((AuthAttrs *) aa, &p) < 0) {
        ERR_print_errors(g_error);
        goto hi_done;
    }

    EVP_DigestUpdate(ctx, attrs, bytes);
    EVP_DigestFinal(ctx, digest, NULL);

    rc = 0;

hi_done:
    if (buffer) {
        free(buffer);
    }

    if (ctx) {
        EVP_MD_CTX_destroy(ctx);
    }

    if (attrs) {
        OPENSSL_free(attrs);
    }

    return rc;
}

/**
 * Verifies the RSA signature
 * @param fd File descriptor to the boot image
 * @param length Length of the boot image without the signature block
 * @param bs The boot signature block
 */
static int verify_signature(int fd, uint64_t length, const BootSignature *bs)
{
    int rc = -1;
    EVP_PKEY *pkey = NULL;
    RSA *rsa = NULL;
    unsigned char digest[SHA256_DIGEST_LENGTH];

    if (!bs) {
        goto vs_done;
    }

    if (hash_image(fd, length, bs->authenticatedAttributes, digest) == -1) {
        goto vs_done;
    }

    if ((pkey = X509_get_pubkey(bs->certificate)) == NULL) {
        ERR_print_errors(g_error);
        goto vs_done;
    }

    if ((rsa = EVP_PKEY_get1_RSA(pkey)) == NULL) {
        ERR_print_errors(g_error);
        goto vs_done;
    }

    if (!RSA_verify(NID_sha256, digest, SHA256_DIGEST_LENGTH,
                bs->signature->data, bs->signature->length, rsa)) {
        ERR_print_errors(g_error);
        goto vs_done;
    }

    rc = 0;

vs_done:
    if (pkey) {
        EVP_PKEY_free(pkey);
    }

    if (rsa) {
        RSA_free(rsa);
    }

    return rc;
}

/**
 * Given the file name of a signed boot image, verifies the signature
 * @param image_file Name of the boot image file
 */
static int verify(const char *image_file)
{
    BootSignature *bs = NULL;
    int fd = -1;
    int rc = 1;
    off64_t offset = 0;

    if (!image_file) {
        return rc;
    }

    if ((fd = open(image_file, O_RDONLY | O_LARGEFILE)) == -1) {
        return rc;
    }

    if (get_signature_offset(fd, &offset) == -1) {
        goto out;
    }

    if (read_signature(fd, offset, &bs) == -1) {
        goto out;
    }

    if (validate_signature_block(bs, offset) == -1) {
        goto out;
    }

    if (verify_signature(fd, offset, bs) == -1) {
        goto out;
    }

    printf("Signature is VALID\n");
    rc = 0;

out:
    if (bs) {
        BootSignature_free(bs);
    }

    if (fd != -1) {
        close(fd);
    }

    return rc;
}

static void usage()
{
    printf("Usage: verify_boot_signature <path-to-boot-image>\n");
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        usage();
        return 1;
    }

    /* BIO descriptor for logging OpenSSL errors to stderr */
    if ((g_error = BIO_new_fd(STDERR_FILENO, BIO_NOCLOSE)) == NULL) {
        printf("Failed to allocate a BIO handle for error output\n");
        return 1;
    }

    ERR_load_crypto_strings();

    return verify(argv[1]);
}
