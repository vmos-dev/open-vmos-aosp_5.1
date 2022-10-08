/*
 * Copyright (C) 2010 The Android Open Source Project
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

/* A simple test of emmc random read and write performance.  When testing write
 * performance, try it twice, once with O_SYNC compiled in, and once with it commented
 * out.  Without O_SYNC, the close(2) blocks until all the dirty buffers are written
 * out, but the numbers tend to be higher.
 */

#define _LARGEFILE64_SOURCE
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#define TST_BLK_SIZE 4096
/* Number of seconds to run the test */
#define TEST_LEN 10

struct stats {
    struct timeval start;
    struct timeval end;
    off64_t offset;
};

static void usage(void) {
        fprintf(stderr, "Usage: rand_emmc_perf [ -r | -w ] [-o] [-s count] [-f full_stats_filename] <size_in_mb> <block_dev>\n");
        exit(1);
}

static void print_stats(struct stats *stats_buf, int stats_count,
                        char * full_stats_file)
{
    int i;
    struct timeval t;
    struct timeval sum = { 0 };
    struct timeval max = { 0 };
    long long total_usecs;
    long long avg_usecs;
    long long max_usecs;
    long long variance = 0;;
    long long x;
    double sdev;
    FILE *full_stats = NULL;

    if (full_stats_file) {
        full_stats = fopen(full_stats_file, "w");
        if (full_stats == NULL) {
            fprintf(stderr, "Cannot open full stats output file %s, ignoring\n",
                    full_stats_file);
        }
    }

    for (i = 0; i < stats_count; i++) {
        timersub(&stats_buf[i].end, &stats_buf[i].start, &t);
        if (timercmp(&t, &max, >)) {
            max = t;
        }
        if (full_stats) {
            fprintf(full_stats, "%lld\n", (t.tv_sec * 1000000LL) + t.tv_usec);
        }
        timeradd(&sum, &t, &sum);
    }

    if (full_stats) {
        fclose(full_stats);
    }

    max_usecs = (max.tv_sec * 1000000LL) + max.tv_usec;
    total_usecs = (sum.tv_sec * 1000000LL) + sum.tv_usec;
    avg_usecs = total_usecs / stats_count;
    printf("average random %d byte iop time = %lld usecs\n",
           TST_BLK_SIZE, avg_usecs);
    printf("maximum random %d byte iop time = %lld usecs\n",
           TST_BLK_SIZE, max_usecs);

    /* Now that we have the average (aka mean) go through the data
     * again and compute the standard deviation.
     * The formula is sqrt(sum_1_to_n((Xi - avg)^2)/n)
     */
    for (i = 0; i < stats_count; i++) {
        timersub(&stats_buf[i].end, &stats_buf[i].start, &t);  /* Xi */
        x = (t.tv_sec * 1000000LL) + t.tv_usec;                /* Convert to long long */
        x = x - avg_usecs;                                     /* Xi - avg */
        x = x * x;                                             /* (Xi - avg) ^ 2 */
        variance += x;                                         /* Summation */
    }
    sdev = sqrt((double)variance/(double)stats_count);
    printf("standard deviation of iops is %.2f\n", sdev);
}

static void stats_test(int fd, int write_mode, off64_t max_blocks, int stats_count,
                       char *full_stats_file)
{
    struct stats *stats_buf;
    char buf[TST_BLK_SIZE] = { 0 };
    int i;

    stats_buf = malloc(stats_count * sizeof(struct stats));
    if (stats_buf == NULL) {
        fprintf(stderr, "Cannot allocate stats_buf\n");
        exit(1);
    }

    for (i = 0; i < stats_count; i++) {
        gettimeofday(&stats_buf[i].start, NULL);

        if (lseek64(fd, (rand() % max_blocks) * TST_BLK_SIZE, SEEK_SET) < 0) {
            fprintf(stderr, "lseek64 failed\n");
        }

        if (write_mode) {
            if (write(fd, buf, sizeof(buf)) != sizeof(buf)) {
                fprintf(stderr, "Short write\n");
            }
        } else {
            if (read(fd, buf, sizeof(buf)) != sizeof(buf)) {
                fprintf(stderr, "Short read\n");
            }
        }

        gettimeofday(&stats_buf[i].end, NULL);
    }

    print_stats(stats_buf, stats_count, full_stats_file);
}

static void perf_test(int fd, int write_mode, off64_t max_blocks)
{
    struct timeval start, end, res;
    char buf[TST_BLK_SIZE] = { 0 };
    int iops = 0;
    int msecs;

    res.tv_sec = 0;
    gettimeofday(&start, NULL);
    while (res.tv_sec < TEST_LEN) {
        if (lseek64(fd, (rand() % max_blocks) * TST_BLK_SIZE, SEEK_SET) < 0) {
            fprintf(stderr, "lseek64 failed\n");
        }
        if (write_mode) {
            if (write(fd, buf, sizeof(buf)) != sizeof(buf)) {
                fprintf(stderr, "Short write\n");
            }
        } else {
            if (read(fd, buf, sizeof(buf)) != sizeof(buf)) {
                fprintf(stderr, "Short read\n");
            }
        }
        iops++;
        gettimeofday(&end, NULL);
        timersub(&end, &start, &res);
    }
    close(fd);

    /* The close can take a while when in write_mode as buffers are flushed.
     * So get the time again. */
    gettimeofday(&end, NULL);
    timersub(&end, &start, &res);

    msecs = (res.tv_sec * 1000) + (res.tv_usec / 1000);
    printf("%d %dbyte iops/sec\n", iops * 1000 / msecs, TST_BLK_SIZE);
}

int main(int argc, char *argv[])
{
    int fd, fd2;
    int write_mode = 0;
    int o_sync = 0;
    int stats_mode = 0;
    int stats_count;
    char *full_stats_file = NULL;
    off64_t max_blocks;
    unsigned int seed;
    int c;

    while ((c = getopt(argc, argv, "+rwos:f:")) != -1) {
        switch (c) {
          case '?':
          default:
            usage();
            break;

          case 'r':
            /* Do nothing, read mode is the default */
            break;

          case 'w':
            write_mode = 1;
            break;

          case 'o':
            o_sync = O_SYNC;
            break;

          case 's':
            stats_mode = 1;
            stats_count = atoi(optarg);
            break;

          case 'f':
            full_stats_file = strdup(optarg);
            if (full_stats_file == NULL) {
                fprintf(stderr, "Cannot get full stats filename\n");
            }
            break;
        }
    }

    if (o_sync && !write_mode) {
        /* Can only specify o_sync in write mode.  Probably doesn't matter,
         * but clear o_sync if in read mode */
        o_sync = 0;
    }

    if ((argc - optind) != 2) {
        usage();
    }

    /* Size is given in megabytes, so compute the number of TST_BLK_SIZE blocks. */
    max_blocks = atoll(argv[optind]) * ((1024*1024) / TST_BLK_SIZE);

    if ((fd = open(argv[optind + 1], O_RDWR | o_sync)) < 0) {
        fprintf(stderr, "Cannot open block device %s\n", argv[optind + 1]);
        exit(1);
    }

    fd2 = open("/dev/urandom", O_RDONLY);
    if (fd2 < 0) {
        fprintf(stderr, "Cannot open /dev/urandom\n");
    }
    if (read(fd2, &seed, sizeof(seed)) != sizeof(seed)) {
        fprintf(stderr, "Cannot read /dev/urandom\n");
    }
    close(fd2);
    srand(seed);

    if (stats_mode) {
        stats_test(fd, write_mode, max_blocks, stats_count, full_stats_file);
    } else {
        perf_test(fd, write_mode, max_blocks);
    }

    exit(0);
}

