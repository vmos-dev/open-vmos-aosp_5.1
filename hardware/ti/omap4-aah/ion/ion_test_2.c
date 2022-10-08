/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
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

/*
 * Test case to test ION Memory Allocator module
 */

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <ion/ion.h>
#include <linux/ion.h>
#include <linux/omap_ion.h>

size_t len = 1024*1024, align = 0;
int prot = PROT_READ | PROT_WRITE;
int map_flags = MAP_SHARED;
int alloc_flags = 0;
int test = -1;
size_t width = 1024*1024, height = 1024*1024;
int fmt = TILER_PIXEL_FMT_32BIT;
int tiler_test = 0;
size_t stride;

int _ion_alloc_test(int fd, struct ion_handle **handle)
{
	int ret;

	if (tiler_test)
		ret = ion_alloc_tiler(fd, width, height, fmt, alloc_flags,
					  handle, &stride);
	else
		ret = ion_alloc(fd, len, align, alloc_flags, handle);

	if (ret)
		printf("%s() failed: %s\n", __func__, strerror(ret));
	return ret;
}

int ion_alloc_test(int count)
{
	int fd, ret = 0, i, count_alloc;
	struct ion_handle **handle;

	fd = ion_open();
	if (fd < 0) {
		printf("%s(): FAILED to open ion device\n",	__func__);
		return -1;
	}

	handle = (struct ion_handle **)malloc(count * sizeof(struct ion_handle *));
	if(handle == NULL) {
		printf("%s() : FAILED to allocate memory for ion_handles\n", __func__);
		return -ENOMEM;
	}

	/* Allocate ion_handles */
	count_alloc = count;
	for(i = 0; i < count; i++) {
		ret = _ion_alloc_test(fd, &(handle[i]));
		printf("%s(): Alloc handle[%d]=%p\n", __func__, i, handle[i]);
		if(ret || ((int)handle[i]  == -ENOMEM)) {
			printf("%s(): Alloc handle[%d]=%p FAILED, err:%s\n",
					__func__, i, handle[i], strerror(ret));
			count_alloc = i;
			goto err_alloc;
		}
	}

	err_alloc:
	/* Free ion_handles */
	for (i = 0; i < count_alloc; i++) {
		printf("%s(): Free  handle[%d]=%p\n", __func__, i, handle[i]);
		ret = ion_free(fd, handle[i]);
		if (ret) {
			printf("%s(): Free handle[%d]=%p FAILED, err:%s\n",
					__func__, i, handle[i], strerror(ret));
		}
	}

	ion_close(fd);
	free(handle);
	handle = NULL;

	if(ret || (count_alloc != count)) {
		printf("\nion alloc test: FAILED\n\n");
		if(count_alloc != count)
			ret = -ENOMEM;
	}
	else
		printf("\nion alloc test: PASSED\n\n");

	return ret;
}

void _ion_tiler_map_test(unsigned char *ptr)
{
	size_t row, col;

	for (row = 0; row < height; row++)
		for (col = 0; col < width; col++) {
			int i = (row * stride) + col;
			ptr[i] = (unsigned char)i;
		}
	for (row = 0; row < height; row++)
		for (col = 0; col < width; col++) {
			int i = (row * stride) + col;
			if (ptr[i] != (unsigned char)i)
				printf("%s(): FAILED, wrote %d read %d from mapped "
					   "memory\n", __func__, i, ptr[i]);
		}
}

void _ion_map_test(unsigned char *ptr)
{
	size_t i;

	for (i = 0; i < len; i++) {
		ptr[i] = (unsigned char)i;
	}
	for (i = 0; i < len; i++) {
		if (ptr[i] != (unsigned char)i)
			printf("%s(): failed wrote %d read %d from mapped "
				   "memory\n", __func__, i, ptr[i]);
	}
}

int ion_map_test(int count)
{
	int fd, ret = 0, i, count_alloc, count_map;
	struct ion_handle **handle;
	unsigned char **ptr;
	int *map_fd;

	fd = ion_open();
	if (fd < 0) {
		printf("%s(): FAILED to open ion device\n",	__func__);
		return -1;
	}

	handle = (struct ion_handle **)malloc(count * sizeof(struct ion_handle *));
	if(handle == NULL) {
		printf("%s(): FAILED to allocate memory for ion_handles\n", __func__);
		return -ENOMEM;
	}

	count_alloc = count;
	count_map = count;

	/* Allocate ion_handles */
	for(i = 0; i < count; i++) {
		ret = _ion_alloc_test(fd, &(handle[i]));
		printf("%s(): Alloc handle[%d]=%p\n", __func__, i, handle[i]);
		if(ret || ((int)handle[i]  == -ENOMEM)) {
			printf("%s(): Alloc handle[%d]=%p FAILED, err:%s\n",
					__func__, i, handle[i], strerror(ret));
			count_alloc = i;
			goto err_alloc;
		}
	}

	/* Map ion_handles and validate */
	if (tiler_test)
		len = height * stride;

	ptr = (unsigned char **)malloc(count * sizeof(unsigned char **));
	map_fd = (int *)malloc(count * sizeof(int *));

	for(i = 0; i < count; i++) {
		/* Map ion_handle on userside */
		ret = ion_map(fd, handle[i], len, prot, map_flags, 0, &(ptr[i]), &(map_fd[i]));
		printf("%s(): Map handle[%d]=%p, map_fd=%d, ptr=%p\n",
				__func__, i, handle[i], map_fd[i], ptr[i]);
		if(ret) {
			printf("%s Map handle[%d]=%p FAILED, err:%s\n",
					__func__, i, handle[i], strerror(ret));
			count_map = i;
			goto err_map;
		}

		/* Validate mapping by writing the data and reading it back */
		if (tiler_test)
			_ion_tiler_map_test(ptr[i]);
		else
			_ion_map_test(ptr[i]);
	}

	/* clean up properly */
	err_map:
	for(i = 0; i < count_map; i++) {
		/* Unmap ion_handles */
		ret = munmap(ptr[i], len);
		printf("%s(): Unmap handle[%d]=%p, map_fd=%d, ptr=%p\n",
				__func__, i, handle[i], map_fd[i], ptr[i]);
		if(ret) {
			printf("%s(): Unmap handle[%d]=%p FAILED, err:%s\n",
					__func__, i, handle[i], strerror(ret));
			goto err_map;
		}
		/* Close fds */
		close(map_fd[i]);
	}
	free(map_fd);
	free(ptr);

	err_alloc:
	/* Free ion_handles */
	for (i = 0; i < count_alloc; i++) {
		printf("%s(): Free handle[%d]=%p\n", __func__, i, handle[i]);
		ret = ion_free(fd, handle[i]);
		if (ret) {
			printf("%s(): Free handle[%d]=%p FAILED, err:%s\n",
					__func__, i, handle[i], strerror(ret));
		}
	}

	ion_close(fd);
	free(handle);
	handle = NULL;

	if(ret || (count_alloc != count) || (count_map != count))
	{
		printf("\nion map test: FAILED\n\n");
		if((count_alloc != count) || (count_map != count))
			ret = -ENOMEM;
	}	else
		printf("\nion map test: PASSED\n");

	return ret;
}

/**
 * Go on allocating buffers of specified size & type, untill the allocation fails.
 * Then free 10 buffers and allocate 10 buffers again.
 */
int ion_alloc_fail_alloc_test()
{
	int fd, ret = 0, i;
	struct ion_handle **handle;
	const int  COUNT_ALLOC_MAX = 200;
	const int  COUNT_REALLOC_MAX = 10;
	int count_alloc = COUNT_ALLOC_MAX, count_realloc = COUNT_ALLOC_MAX;

	fd = ion_open();
	if (fd < 0) {
		printf("%s(): FAILED to open ion device\n", __func__);
		return -1;
	}

	handle = (struct ion_handle **)malloc(COUNT_ALLOC_MAX * sizeof(struct ion_handle *));
	if(handle == NULL) {
		printf("%s(): FAILED to allocate memory for ion_handles\n", __func__);
		return -ENOMEM;
	}

	/* Allocate ion_handles as much as possible */
	for(i = 0; i < COUNT_ALLOC_MAX; i++) {
		ret = _ion_alloc_test(fd, &(handle[i]));
		printf("%s(): Alloc handle[%d]=%p\n", __func__, i, handle[i]);
		if(ret || ((int)handle[i]  == -ENOMEM)) {
			printf("%s(): Alloc handle[%d]=%p FAILED, err:%s\n\n",
					__func__, i, handle[i], strerror(ret));
			count_alloc = i;
			break;
		}
	}

	/* Free COUNT_REALLOC_MAX ion_handles */
	for (i = count_alloc-1; i > (count_alloc-1 - COUNT_REALLOC_MAX); i--) {
		printf("%s(): Free  handle[%d]=%p\n", __func__, i, handle[i]);
		ret = ion_free(fd, handle[i]);
		if (ret) {
			printf("%s(): Free  handle[%d]=%p FAILED, err:%s\n\n",
					__func__, i, handle[i], strerror(ret));
		}
	}

	/* Again allocate COUNT_REALLOC_MAX ion_handles to test
	   that we are still able to allocate */
	for(i = (count_alloc - COUNT_REALLOC_MAX); i < count_alloc; i++) {
		ret = _ion_alloc_test(fd, &(handle[i]));
		printf("%s(): Alloc handle[%d]=%p\n", __func__, i, handle[i]);
		if(ret || ((int)handle[i]  == -ENOMEM)) {
			printf("%s(): Alloc handle[%d]=%p FAILED, err:%s\n\n",
					__func__, i, handle[i], strerror(ret));
			count_realloc = i;
			goto err_alloc;
		}
	}
	count_realloc = i;

	err_alloc:
	/* Free all ion_handles */
	for (i = 0; i < count_alloc; i++) {
		printf("%s(): Free  handle[%d]=%p\n", __func__, i, handle[i]);
		ret = ion_free(fd, handle[i]);
		if (ret) {
			printf("%s(): Free  handle[%d]=%p FAILED, err:%s\n",
					__func__, i, handle[i], strerror(ret));
		}
	}

	ion_close(fd);
	free(handle);
	handle = NULL;

	printf("\ncount_alloc=%d, count_realloc=%d\n",count_alloc, count_realloc);

	if(ret || (count_alloc != count_realloc)) {
		printf("\nion alloc->fail->alloc test: FAILED\n\n");
		if(count_alloc != COUNT_ALLOC_MAX)
			ret = -ENOMEM;
	}
	else
		printf("\nion alloc->fail->alloc test: PASSED\n\n");

	return ret;
}

int custom_test(int test_number)
{
	switch(test_number) {
		case 1 :
			return ion_alloc_fail_alloc_test();
		default :
			printf("%s(): Invalid custom_test_number=%d\n", __func__, test_number);
			return -EINVAL;
	}
}

int main(int argc, char* argv[]) {
	int c, ret;
	unsigned int count = 1, iteration = 1, j, custom_test_num = 1;
	enum tests {
		ALLOC_TEST = 0, MAP_TEST, CUSTOM_TEST,
	};

	while (1) {
		static struct option opts[] = {
			{"alloc", no_argument, 0, 'a'},
			{"alloc_flags", required_argument, 0, 'f'},
			{"map", no_argument, 0, 'm'},
			{"custom", required_argument, 0, 'c'},
			{"len", required_argument, 0, 'l'},
			{"align", required_argument, 0, 'g'},
			{"map_flags", required_argument, 0, 'z'},
			{"prot", required_argument, 0, 'p'},
			{"alloc_tiler", no_argument, 0, 't'},
			{"width", required_argument, 0, 'w'},
			{"height", required_argument, 0, 'h'},
			{"fmt", required_argument, 0, 'r'},
			{"count", required_argument, 0, 'n'},
			{"iteration", required_argument, 0, 'i'},
		};
		int i = 0;
		c = getopt_long(argc, argv, "af:h:l:mr:stw:c:n:i:", opts, &i);
		if (c == -1)
			break;

		switch (c) {
		case 'l':
			len = atol(optarg);
			break;
		case 'g':
			align = atol(optarg);
			break;
		case 'z':
			map_flags = 0;
			map_flags |= strstr(optarg, "PROT_EXEC") ?
				PROT_EXEC : 0;
			map_flags |= strstr(optarg, "PROT_READ") ?
				PROT_READ: 0;
			map_flags |= strstr(optarg, "PROT_WRITE") ?
				PROT_WRITE: 0;
			map_flags |= strstr(optarg, "PROT_NONE") ?
				PROT_NONE: 0;
			break;
		case 'p':
			prot = 0;
			prot |= strstr(optarg, "MAP_PRIVATE") ?
				MAP_PRIVATE	 : 0;
			prot |= strstr(optarg, "MAP_SHARED") ?
				MAP_PRIVATE	 : 0;
			break;
		case 'f':
			alloc_flags = atol(optarg);
			break;
		case 'a':
			test = ALLOC_TEST;
			break;
		case 'm':
			test = MAP_TEST;
			break;
		case 'c':
			test = CUSTOM_TEST;
			printf("KALP : Case 'c'\n");
			custom_test_num = atol(optarg);
			break;
		case 'r':
			fmt = atol(optarg);
			break;
		case 'w':
			width = atol(optarg);
			break;
		case 'h':
			height = atol(optarg);
			break;
		case 't':
			tiler_test = 1;
			break;
		case 'n':
			printf("KALP : Case 'n'\n");
			count = atol(optarg);
			break;
		case 'i':
			printf("KALP : Case 'i'\n");
			iteration = atol(optarg);
			break;
		}
	}
	printf("test %d, len %u, width %u, height %u, fmt %u, align %u, count %d, "
		   "iteration %d, map_flags %d, prot %d, alloc_flags %d\n", test, len, width,
		   height, fmt, align, count, iteration, map_flags, prot, alloc_flags);

	switch (test) {
		case ALLOC_TEST:
			for(j = 0; j < iteration; j++) {
				ret = ion_alloc_test(count);
				if(ret) {
					printf("\nion alloc test: FAILED at iteration-%d\n", j+1);
					break;
				}
			}
			break;

		case MAP_TEST:
			for(j = 0; j < iteration; j++) {
				ret = ion_map_test(count);
				if(ret) {
					printf("\nion map test: FAILED at iteration-%d\n", j+1);
					break;
				}
			}
			break;

		case CUSTOM_TEST:
			ret = custom_test(custom_test_num);
			if(ret) {
				printf("\nion custom test #%d: FAILED\n", custom_test_num);
			}
			break;

		default:
			printf("must specify a test (alloc, map, custom)\n");
	}

	return 0;
}
