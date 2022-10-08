#include <openssl/evp.h>
#include <sparse/sparse.h>

#undef NDEBUG

#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct sparse_hash_ctx {
    unsigned char *hashes;
    const unsigned char *salt;
    uint64_t salt_size;
    uint64_t hash_size;
    uint64_t block_size;
    const unsigned char *zero_block_hash;
    const EVP_MD *md;
};

#define div_round_up(x,y) (((x) + (y) - 1)/(y))

#define round_up(x,y) (div_round_up(x,y)*(y))

#define FATAL(x...) { \
    fprintf(stderr, x); \
    exit(1); \
}

size_t verity_tree_blocks(uint64_t data_size, size_t block_size, size_t hash_size,
                          int level)
{
    size_t level_blocks = div_round_up(data_size, block_size);
    int hashes_per_block = div_round_up(block_size, hash_size);

    do {
        level_blocks = div_round_up(level_blocks, hashes_per_block);
    } while (level--);

    return level_blocks;
}

int hash_block(const EVP_MD *md,
               const unsigned char *block, size_t len,
               const unsigned char *salt, size_t salt_len,
               unsigned char *out, size_t *out_size)
{
    EVP_MD_CTX *mdctx;
    unsigned int s;
    int ret = 1;

    mdctx = EVP_MD_CTX_create();
    assert(mdctx);
    ret &= EVP_DigestInit_ex(mdctx, md, NULL);
    ret &= EVP_DigestUpdate(mdctx, salt, salt_len);
    ret &= EVP_DigestUpdate(mdctx, block, len);
    ret &= EVP_DigestFinal_ex(mdctx, out, &s);
    EVP_MD_CTX_destroy(mdctx);
    assert(ret == 1);
    if (out_size) {
        *out_size = s;
    }
    return 0;
}

int hash_blocks(const EVP_MD *md,
                const unsigned char *in, size_t in_size,
                unsigned char *out, size_t *out_size,
                const unsigned char *salt, size_t salt_size,
                size_t block_size)
{
    size_t s;
    *out_size = 0;
    for (size_t i = 0; i < in_size; i += block_size) {
        hash_block(md, in + i, block_size, salt, salt_size, out, &s);
        out += s;
        *out_size += s;
    }

    return 0;
}

int hash_chunk(void *priv, const void *data, int len)
{
    struct sparse_hash_ctx *ctx = (struct sparse_hash_ctx *)priv;
    assert(len % ctx->block_size == 0);
    if (data) {
        size_t s;
        hash_blocks(ctx->md, (const unsigned char *)data, len,
                    ctx->hashes, &s,
                    ctx->salt, ctx->salt_size, ctx->block_size);
        ctx->hashes += s;
    } else {
        for (size_t i = 0; i < (size_t)len; i += ctx->block_size) {
            memcpy(ctx->hashes, ctx->zero_block_hash, ctx->hash_size);
            ctx->hashes += ctx->hash_size;
        }
    }
    return 0;
}

void usage(void)
{
    printf("usage: build_verity_tree [ <options> ] -s <size> | <data> <verity>\n"
           "options:\n"
           "  -a,--salt-str=<string>       set salt to <string>\n"
           "  -A,--salt-hex=<hex digits>   set salt to <hex digits>\n"
           "  -h                           show this help\n"
           "  -s,--verity-size=<data size> print the size of the verity tree\n"
           "  -S                           treat <data image> as a sparse file\n"
        );
}

int main(int argc, char **argv)
{
    char *data_filename;
    char *verity_filename;
    unsigned char *salt = NULL;
    size_t salt_size = 0;
    bool sparse = false;
    size_t block_size = 4096;
    size_t calculate_size = 0;

    while (1) {
        const static struct option long_options[] = {
            {"salt-str", required_argument, 0, 'a'},
            {"salt-hex", required_argument, 0, 'A'},
            {"help", no_argument, 0, 'h'},
            {"sparse", no_argument, 0, 'S'},
            {"verity-size", required_argument, 0, 's'},
        };
        int c = getopt_long(argc, argv, "a:A:hSs:", long_options, NULL);
        if (c < 0) {
            break;
        }

        switch (c) {
        case 'a':
            salt_size = strlen(optarg);
            salt = new unsigned char[salt_size]();
            if (salt == NULL) {
                FATAL("failed to allocate memory for salt\n");
            }
            memcpy(salt, optarg, salt_size);
            break;
        case 'A': {
                BIGNUM *bn = NULL;
                if(!BN_hex2bn(&bn, optarg)) {
                    FATAL("failed to convert salt from hex\n");
                }
                salt_size = BN_num_bytes(bn);
                salt = new unsigned char[salt_size]();
                if (salt == NULL) {
                    FATAL("failed to allocate memory for salt\n");
                }
                if((size_t)BN_bn2bin(bn, salt) != salt_size) {
                    FATAL("failed to convert salt to bytes\n");
                }
            }
            break;
        case 'h':
            usage();
            return 1;
        case 'S':
            sparse = true;
            break;
        case 's':
            calculate_size = strtoul(optarg, NULL, 0);
            break;
        case '?':
            usage();
            return 1;
        default:
            abort();
        }
    }

    argc -= optind;
    argv += optind;

    const EVP_MD *md = EVP_sha256();
    if (!md) {
        FATAL("failed to get digest\n");
    }

    size_t hash_size = EVP_MD_size(md);
    assert(hash_size * 2 < block_size);

    if (!salt || !salt_size) {
        salt_size = hash_size;
        salt = new unsigned char[salt_size];
        if (salt == NULL) {
            FATAL("failed to allocate memory for salt\n");
        }

        int random_fd = open("/dev/urandom", O_RDONLY);
        if (random_fd < 0) {
            FATAL("failed to open /dev/urandom\n");
        }

        ssize_t ret = read(random_fd, salt, salt_size);
        if (ret != (ssize_t)salt_size) {
            FATAL("failed to read %zu bytes from /dev/urandom: %zd %d\n", salt_size, ret, errno);
        }
        close(random_fd);
    }

    if (calculate_size) {
        if (argc != 0) {
            usage();
            return 1;
        }
        size_t verity_blocks = 0;
        size_t level_blocks;
        int levels = 0;
        do {
            level_blocks = verity_tree_blocks(calculate_size, block_size, hash_size, levels);
            levels++;
            verity_blocks += level_blocks;
        } while (level_blocks > 1);

        printf("%zu\n", verity_blocks * block_size);
        return 0;
    }

    if (argc != 2) {
        usage();
        return 1;
    }

    data_filename = argv[0];
    verity_filename = argv[1];

    int fd = open(data_filename, O_RDONLY);
    if (fd < 0) {
        FATAL("failed to open %s\n", data_filename);
    }

    struct sparse_file *file;
    if (sparse) {
        file = sparse_file_import(fd, false, false);
    } else {
        file = sparse_file_import_auto(fd, false);
    }

    if (!file) {
        FATAL("failed to read file %s\n", data_filename);
    }

    int64_t len = sparse_file_len(file, false, false);
    if (len % block_size != 0) {
        FATAL("file size %" PRIu64 " is not a multiple of %zu bytes\n",
                len, block_size);
    }

    int levels = 0;
    size_t verity_blocks = 0;
    size_t level_blocks;

    do {
        level_blocks = verity_tree_blocks(len, block_size, hash_size, levels);
        levels++;
        verity_blocks += level_blocks;
    } while (level_blocks > 1);

    unsigned char *verity_tree = new unsigned char[verity_blocks * block_size]();
    unsigned char **verity_tree_levels = new unsigned char *[levels + 1]();
    size_t *verity_tree_level_blocks = new size_t[levels]();
    if (verity_tree == NULL || verity_tree_levels == NULL || verity_tree_level_blocks == NULL) {
        FATAL("failed to allocate memory for verity tree\n");
    }

    unsigned char *ptr = verity_tree;
    for (int i = levels - 1; i >= 0; i--) {
        verity_tree_levels[i] = ptr;
        verity_tree_level_blocks[i] = verity_tree_blocks(len, block_size, hash_size, i);
        ptr += verity_tree_level_blocks[i] * block_size;
    }
    assert(ptr == verity_tree + verity_blocks * block_size);
    assert(verity_tree_level_blocks[levels - 1] == 1);

    unsigned char zero_block_hash[hash_size];
    unsigned char zero_block[block_size];
    memset(zero_block, 0, block_size);
    hash_block(md, zero_block, block_size, salt, salt_size, zero_block_hash, NULL);

    unsigned char root_hash[hash_size];
    verity_tree_levels[levels] = root_hash;

    struct sparse_hash_ctx ctx;
    ctx.hashes = verity_tree_levels[0];
    ctx.salt = salt;
    ctx.salt_size = salt_size;
    ctx.hash_size = hash_size;
    ctx.block_size = block_size;
    ctx.zero_block_hash = zero_block_hash;
    ctx.md = md;

    sparse_file_callback(file, false, false, hash_chunk, &ctx);

    sparse_file_destroy(file);
    close(fd);

    for (int i = 0; i < levels; i++) {
        size_t out_size;
        hash_blocks(md,
                verity_tree_levels[i], verity_tree_level_blocks[i] * block_size,
                verity_tree_levels[i + 1], &out_size,
                salt, salt_size, block_size);
          if (i < levels - 1) {
              assert(div_round_up(out_size, block_size) == verity_tree_level_blocks[i + 1]);
          } else {
              assert(out_size == hash_size);
          }
    }

    for (size_t i = 0; i < hash_size; i++) {
        printf("%02x", root_hash[i]);
    }
    printf(" ");
    for (size_t i = 0; i < salt_size; i++) {
        printf("%02x", salt[i]);
    }
    printf("\n");

    fd = open(verity_filename, O_WRONLY|O_CREAT, 0666);
    if (fd < 0) {
        FATAL("failed to open output file '%s'\n", verity_filename);
    }
    write(fd, verity_tree, verity_blocks * block_size);
    close(fd);

    delete[] verity_tree_levels;
    delete[] verity_tree_level_blocks;
    delete[] verity_tree;
    delete[] salt;
}
