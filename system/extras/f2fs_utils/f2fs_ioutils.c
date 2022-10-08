/*
 * Copyright (C) 2014 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#define _LARGEFILE64_SOURCE

#include <asm/types.h>
#include <errno.h>
#include <linux/fs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>  /* memset() */
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <dlfcn.h>

#include <assert.h>

#include <f2fs_fs.h>
#include <f2fs_format_utils.h>

#include <sparse/sparse.h>

struct selabel_handle;

#include "make_f2fs.h"

#ifdef USE_MINGW

#include <winsock2.h>

/* These match the Linux definitions of these flags.
   L_xx is defined to avoid conflicting with the win32 versions.
*/
#define L_S_IRUSR 00400
#define L_S_IWUSR 00200
#define L_S_IXUSR 00100
#define S_IRWXU (L_S_IRUSR | L_S_IWUSR | L_S_IXUSR)
#define S_IRGRP 00040
#define S_IWGRP 00020
#define S_IXGRP 00010
#define S_IRWXG (S_IRGRP | S_IWGRP | S_IXGRP)
#define S_IROTH 00004
#define S_IWOTH 00002
#define S_IXOTH 00001
#define S_IRWXO (S_IROTH | S_IWOTH | S_IXOTH)
#define S_ISUID 0004000
#define S_ISGID 0002000
#define S_ISVTX 0001000

#else

#include <selinux/selinux.h>
#include <selinux/label.h>
#include <selinux/android.h>

#define O_BINARY 0

#endif

struct f2fs_configuration config;
struct sparse_file *f2fs_sparse_file;

struct buf_item {
	void *buf;
	size_t len;
	struct buf_item *next;
};

struct buf_item *buf_list;

static int dev_write_fd(void *buf, __u64 offset, size_t len)
{
	if (lseek64(config.fd, (off64_t)offset, SEEK_SET) < 0)
		return -1;
	if (write(config.fd, buf, len) != len)
		return -1;
	return 0;
}

void flush_sparse_buffs()
{
	while (buf_list) {
		struct buf_item *bi = buf_list;
		buf_list = buf_list->next;
		free(bi->buf);
		free(bi);
	}
}

static int dev_write_sparse(void *buf, __u64 byte_offset, size_t byte_len)
{
	struct buf_item *bi = calloc(1, sizeof(struct buf_item));

	if (bi == NULL) {
		return -ENOMEM;
	}
	bi->buf = malloc(byte_len);
	if (bi->buf == NULL) {
		free(bi);
		return -ENOMEM;
	}

	bi->len = byte_len;
	memcpy(bi->buf, buf, byte_len);
	bi->next = buf_list;
	buf_list = bi;

	sparse_file_add_data(f2fs_sparse_file, bi->buf, byte_len, byte_offset/F2FS_BLKSIZE);
	return 0;
}

void f2fs_finalize_device(struct f2fs_configuration *c)
{
}

int f2fs_trim_device()
{
	return 0;
}

/*
 * IO interfaces
 */
int dev_read(void  *buf, __u64 offset, size_t len)
{
	return 0;
}

int dev_write(void *buf, __u64 offset, size_t len)
{
	if (config.fd >= 0) {
		return dev_write_fd(buf, offset, len);
	} else {
		return dev_write_sparse(buf, offset, len);
	}
}


int dev_fill(void *buf, __u64 offset, size_t len)
{
	int ret;
	if (config.fd >= 0) {
		return dev_write_fd(buf, offset, len);
	}
        // sparse file fills with zero by default.
	// return sparse_file_add_fill(f2fs_sparse_file, ((__u8*)(bi->buf))[0], byte_len, byte_offset/F2FS_BLKSIZE);
	return 0;
}

int dev_read_block(void *buf, __u64 blk_addr)
{
	assert(false); // Must not be invoked.
	return 0;
}

int dev_read_blocks(void *buf, __u64 addr, __u32 nr_blks)
{
	assert(false); // Must not be invoked.
	return 0;
}

