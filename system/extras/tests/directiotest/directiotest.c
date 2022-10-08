/*
 * Copyright 2010 by Garmin Ltd. or its subsidiaries
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
 *
 * Performs a simple write/readback test to verify correct functionality
 * of direct i/o on a block device node.
 */

/* For large-file support */
#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE

/* For O_DIRECT */
#define _GNU_SOURCE

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <linux/fs.h>

#define NUM_TEST_BLKS 128

/*
 * Allocate page-aligned memory.  Could use posix_memalign(3), but some
 * systems don't support it.  Also pre-faults memory since we'll be using
 * it all right away anyway.
 */
static void *pagealign_alloc(size_t size)
{
	void *ret = mmap(NULL, size, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE | MAP_LOCKED,
			-1, 0);
	if (ret == MAP_FAILED) {
		perror("mmap");
		ret = NULL;
	}
	return ret;
}

static void pagealign_free(void *addr, size_t size)
{
	int ret = munmap(addr, size);
	if (ret == -1)
		perror("munmap");
}

static ssize_t do_read(int fd, void *buf, off64_t start, size_t count)
{
	ssize_t ret;
	size_t bytes_read = 0;

	lseek64(fd, start, SEEK_SET);

	do {
		ret = read(fd, (char *)buf + bytes_read, count - bytes_read);
		if (ret == -1) {
			perror("read");
			return -1;
		} else if (ret == 0) {
			fprintf(stderr, "Unexpected end-of-file\n");
			return -1;
		}
		bytes_read += ret;
	} while (bytes_read < count);

	return bytes_read;
}

static ssize_t do_write(int fd, const void *buf, off64_t start, size_t count)
{
	ssize_t ret;
	size_t bytes_out = 0;

	lseek64(fd, start, SEEK_SET);

	do {
		ret = write(fd, (char *)buf + bytes_out, count - bytes_out);
		if (ret == -1) {
			perror("write");
			return -1;
		} else if (ret == 0) {
			fprintf(stderr, "write returned 0\n");
			return -1;
		}
		bytes_out += ret;
	} while (bytes_out < count);

	return bytes_out;
}

/*
 * Initializes test buffer with locally-unique test pattern.  High 16-bits of
 * each 32-bit word contain first disk block number of the test area, low
 * 16-bits contain word offset into test area.  The goal is that a given test
 * area should never contain the same data as a nearby test area, and that the
 * data for a given test area be easily reproducable given the start block and
 * test area size.
 */
static void init_test_buf(void *buf, uint64_t start_blk, size_t len)
{
	uint32_t *data = buf;
	size_t i;

	len /= sizeof(uint32_t);
	for (i = 0; i < len; i++)
		data[i] = (start_blk & 0xFFFF) << 16 | (i & 0xFFFF);
}

static void dump_hex(const void *buf, int len)
{
	const uint8_t *data = buf;
	int i;
	char ascii_buf[17];

	ascii_buf[16] = '\0';

	for (i = 0; i < len; i++) {
		int val = data[i];
		int off = i % 16;

		if (off == 0)
			printf("%08x  ", i);
		printf("%02x ", val);
		ascii_buf[off] = isprint(val) ? val : '.';
		if (off == 15)
			printf(" %-16s\n", ascii_buf);
	}

	i %= 16;
	if (i) {
		ascii_buf[i] = '\0';
		while (i++ < 16)
			printf("   ");
		printf(" %-16s\n", ascii_buf);
	}
}

static void update_progress(int current, int total)
{
	double pct_done = (double)current * 100 / total;
	printf("Testing area %d/%d (%6.2f%% complete)\r", current, total,
			pct_done);
	fflush(stdout);
}

int main(int argc, const char *argv[])
{
	int ret = 1;
	const char *path;
	int fd;
	struct stat stat;
	void *read_buf = NULL, *write_buf = NULL;
	int blk_size;
	uint64_t num_blks;
	size_t test_size;
	int test_areas, i;

	if (argc != 2) {
		printf("Usage: directiotest blkdev_path\n");
		exit(1);
	}

	path = argv[1];
	fd = open(path, O_RDWR | O_DIRECT | O_LARGEFILE);
	if (fd == -1) {
		perror("open");
		exit(1);
	}
	if (fstat(fd, &stat) == -1) {
		perror("stat");
		goto cleanup;
	} else if (!S_ISBLK(stat.st_mode)) {
		fprintf(stderr, "%s is not a block device\n", path);
		goto cleanup;
	}

	if (ioctl(fd, BLKSSZGET, &blk_size) == -1) {
		perror("ioctl");
		goto cleanup;
	}
	if (ioctl(fd, BLKGETSIZE64, &num_blks) == -1) {
		perror("ioctl");
		goto cleanup;
	}
	num_blks /= blk_size;

	test_size = (size_t)blk_size * NUM_TEST_BLKS;
	read_buf = pagealign_alloc(test_size);
	write_buf = pagealign_alloc(test_size);
	if (!read_buf || !write_buf) {
		fprintf(stderr, "Error allocating test buffers\n");
		goto cleanup;
	}

	/*
	 * Start the actual test.  Go through the entire device, writing
	 * locally-unique patern to each test block and then reading it
	 * back.
	 */
	if (num_blks / NUM_TEST_BLKS > INT_MAX) {
		printf("Warning: Device too large for test variables\n");
		printf("Entire device will not be tested\n");
		test_areas = INT_MAX;
	} else {
		test_areas = num_blks / NUM_TEST_BLKS;
	}

	printf("Starting test\n");

	for (i = 0; i < test_areas; i++) {
		uint64_t cur_blk = (uint64_t)i * NUM_TEST_BLKS;

		update_progress(i + 1, test_areas);

		init_test_buf(write_buf, cur_blk, test_size);

		if (do_write(fd, write_buf, cur_blk * blk_size, test_size) !=
				(ssize_t)test_size) {
			fprintf(stderr, "write failed, aborting test\n");
			goto cleanup;
		}
		if (do_read(fd, read_buf, cur_blk * blk_size, test_size) !=
				(ssize_t)test_size) {
			fprintf(stderr, "read failed, aborting test\n");
			goto cleanup;
		}

		if (memcmp(write_buf, read_buf, test_size)) {
			printf("Readback verification failed at block %" PRIu64 "\n\n",
					cur_blk);
			printf("Written data:\n");
			dump_hex(write_buf, test_size);
			printf("\nRead data:\n");
			dump_hex(read_buf, test_size);
			goto cleanup;
		}
	}

	printf("\nTest complete\n");
	ret = 0;

cleanup:
	if (read_buf)
		pagealign_free(read_buf, test_size);
	if (write_buf)
		pagealign_free(write_buf, test_size);
	close(fd);
	return ret;
}
