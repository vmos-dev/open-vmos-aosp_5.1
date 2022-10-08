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

#include <fcntl.h>
#include <dlfcn.h>

#include <f2fs_fs.h>
#include <f2fs_format_utils.h>

#include <sparse/sparse.h>

struct selabel_handle;

#include "make_f2fs.h"

extern void flush_sparse_buffs();

struct f2fs_configuration config;
struct sparse_file *f2fs_sparse_file;
extern int dlopenf2fs();

static void reset_f2fs_info() {
	// Reset all the global data structures used by make_f2fs so it
	// can be called again.
	memset(&config, 0, sizeof(config));
	config.fd = -1;
	if (f2fs_sparse_file) {
		sparse_file_destroy(f2fs_sparse_file);
		f2fs_sparse_file = NULL;
	}
}

int make_f2fs_sparse_fd(int fd, long long len,
		const char *mountpoint, struct selabel_handle *sehnd)
{
	if (dlopenf2fs() < 0) {
		return -1;
	}
	reset_f2fs_info();
	f2fs_init_configuration(&config);
	len &= ~((__u64)F2FS_BLKSIZE);
	config.total_sectors = len / config.sector_size;
	config.start_sector = 0;
	f2fs_sparse_file = sparse_file_new(F2FS_BLKSIZE, len);
	f2fs_format_device();
	sparse_file_write(f2fs_sparse_file, fd, /*gzip*/0, /*sparse*/1, /*crc*/0);
	sparse_file_destroy(f2fs_sparse_file);
	flush_sparse_buffs();
	f2fs_sparse_file = NULL;
	return 0;
}
