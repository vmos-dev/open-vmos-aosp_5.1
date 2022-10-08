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

#include <stdio.h>

#include <dlfcn.h>
#include <assert.h>

#include <f2fs_fs.h>
#include <f2fs_format_utils.h>
#define F2FS_DYN_LIB "libf2fs_fmt_host_dyn.so"

int (*f2fs_format_device_dl)(void);
void (*f2fs_init_configuration_dl)(struct f2fs_configuration *);

int f2fs_format_device(void) {
	assert(f2fs_format_device_dl);
	return f2fs_format_device_dl();
}
void f2fs_init_configuration(struct f2fs_configuration *config) {
	assert(f2fs_init_configuration_dl);
	f2fs_init_configuration_dl(config);
}

int dlopenf2fs() {
	void* f2fs_lib;

	f2fs_lib = dlopen(F2FS_DYN_LIB, RTLD_NOW);
	if (!f2fs_lib) {
		return -1;
	}
	f2fs_format_device_dl = dlsym(f2fs_lib, "f2fs_format_device");
	f2fs_init_configuration_dl = dlsym(f2fs_lib, "f2fs_init_configuration");
	if (!f2fs_format_device_dl || !f2fs_init_configuration_dl) {
		return -1;
	}
	return 0;
}
