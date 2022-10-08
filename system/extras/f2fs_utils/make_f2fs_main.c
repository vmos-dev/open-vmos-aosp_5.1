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

#include <fcntl.h>
#include <libgen.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#if defined(__linux__)
#include <linux/fs.h>
#elif defined(__APPLE__) && defined(__MACH__)
#include <sys/disk.h>
#endif

#ifndef USE_MINGW /* O_BINARY is windows-specific flag */
#define O_BINARY 0
#endif

static void usage(char *path)
{
	fprintf(stderr, "%s -l <len>\n", basename(path));
	fprintf(stderr, "    <filename>\n");
}

int main(int argc, char **argv)
{
	int opt;
	const char *filename = NULL;
	int fd;
	int exitcode;
	long long len;
	while ((opt = getopt(argc, argv, "l:")) != -1) {
		switch (opt) {
		case 'l':
			len = atoll(optarg);
			break;
		default: /* '?' */
			usage(argv[0]);
			exit(EXIT_FAILURE);
		}
	}


	if (optind >= argc) {
		fprintf(stderr, "Expected filename after options\n");
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	filename = argv[optind++];

	if (optind < argc) {
		fprintf(stderr, "Unexpected argument: %s\n", argv[optind]);
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	if (strcmp(filename, "-")) {
		fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0644);
		if (fd < 0) {
			perror("open");
			return EXIT_FAILURE;
		}
	} else {
		fd = STDOUT_FILENO;
	}

        exitcode = make_f2fs_sparse_fd(fd, len, NULL, NULL);

	close(fd);
	if (exitcode && strcmp(filename, "-"))
		unlink(filename);
	return exitcode;
}
