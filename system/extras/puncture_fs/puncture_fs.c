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

#include <assert.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <logwrap/logwrap.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <utils/Log.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#define MAX_IO_WRITE_CHUNK_SIZE 0x100000

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

typedef unsigned long u64;

static void usage(const char * const progname) {
    fprintf(stderr,
            "Usage: %s [-s <seed>] -h <hole size in bytes> -t <total hole size in bytes> "
            "path\n",
            progname);
}

static u64 get_free_space(const char * const path) {
    struct statvfs s;

    if (statvfs(path, &s) < 0) {
        fprintf(stderr, "\nerrno: %d. Failed to get free disk space on %s\n",
                errno, path);
        return 0;
    } else {
        return (u64)s.f_bsize * (u64)s.f_bfree;
    }
}

static u64 get_random_num(const u64 start, const u64 end) {
    if (end - start <= 0)
        return start;
    assert(RAND_MAX >= 0x7FFFFFFF);
    if ((end - start) > 0x7FFFFFFF)
        return start + (((u64)random() << 31) | (u64)random()) % (end - start);
    return start + (random() % (end - start));
}

static char get_random_char() {
    return 'A' + random() % ('Z' - 'A');
}

static bool create_unique_file(const char * const dir_path, const u64 size,
                               const u64 id, char * const base,
                               const u64 base_length) {
    u64 length = 0;
    int fd;
    char file_path[FILENAME_MAX];
    bool ret = true;

    base[random() % min(base_length, size)] = get_random_char();

    sprintf(file_path, "%s/file_%lu", dir_path, id);
    fd = open(file_path, O_WRONLY | O_CREAT | O_SYNC, 0777);
    if (fd < 0) {
        // We suppress ENOSPC erros as that is common as we approach the
        // last few MBs of the fs as we don't account for the size of the newly
        // added meta data after the initial free space computation.
        if (errno != 28) {
            fprintf(stderr, "\nerrno: %d. Failed to create %s\n", errno, file_path);
        }
        return false;
    }
    while (length + base_length < size) {
        if (write(fd, base, base_length) < 0) {
            if (errno != 28) {
                fprintf(stderr, "\nerrno: %d. Failed to write %lu bytes to %s\n",
                        errno, base_length, file_path);
            }
            ret = false;
            goto done;
        }
        length += base_length;
    }
    if (write(fd, base, size - length) < 0) {
        if (errno != 28) {
            fprintf(stderr, "\nerrno: %d. Failed to write last %lu bytes to %s\n",
                    errno, size - length, file_path);
        }
        ret = false;
        goto done;
    }
done:
    if (close(fd) < 0) {
        fprintf(stderr, "\nFailed to close %s\n", file_path);
        ret = false;
    }
    return ret;
}

static bool create_unique_dir(char *dir, const char * const root_path) {
    char random_string[15];
    int i;

    for (i = 0; i < 14; ++i) {
        random_string[i] = get_random_char();
    }
    random_string[14] = '\0';

    sprintf(dir, "%s/%s", root_path, random_string);

    if (mkdir(dir, 0777) < 0) {
        fprintf(stderr, "\nerrno: %d. Failed to create %s\n", errno, dir);
        return false;
    }
    return true;
}

static bool puncture_fs (const char * const path, const u64 total_size,
                         const u64 hole_size, const u64 total_hole_size) {
    u64 increments = (hole_size * total_size) / total_hole_size;
    u64 hole_max;
    u64 starting_max = 0;
    u64 ending_max = increments;
    char stay_dir[FILENAME_MAX], delete_dir[FILENAME_MAX];
    char *rm_bin_argv[] = { "/system/bin/rm", "-rf", ""};
    u64 file_id = 1;
    char *base_file_data;
    u64 i = 0;

    if (!create_unique_dir(stay_dir, path) ||
        !create_unique_dir(delete_dir, path)) {
        return false;
    }

    base_file_data = (char*) malloc(MAX_IO_WRITE_CHUNK_SIZE);
    for (i = 0; i < MAX_IO_WRITE_CHUNK_SIZE; ++i) {
        base_file_data[i] = get_random_char();
    }
    fprintf(stderr, "\n");
    while (ending_max <= total_size) {
        fprintf(stderr, "\rSTAGE 1/2: %d%% Complete",
                (int) (100.0 * starting_max / total_size));
        hole_max = get_random_num(starting_max, ending_max);

        create_unique_file(stay_dir,
                           hole_max - starting_max,
                           file_id++,
                           base_file_data,
                           MAX_IO_WRITE_CHUNK_SIZE);
        create_unique_file(delete_dir,
                           hole_size,
                           file_id++,
                           base_file_data,
                           MAX_IO_WRITE_CHUNK_SIZE);

        starting_max = hole_max + hole_size;
        ending_max += increments;
    }
    create_unique_file(stay_dir,
                       (ending_max - increments - starting_max),
                       file_id++,
                       base_file_data,
                       MAX_IO_WRITE_CHUNK_SIZE);
    fprintf(stderr, "\rSTAGE 1/2: 100%% Complete\n");
    fprintf(stderr, "\rSTAGE 2/2: 0%% Complete");
    free(base_file_data);
    rm_bin_argv[2] = delete_dir;
    if (android_fork_execvp_ext(ARRAY_SIZE(rm_bin_argv), rm_bin_argv,
                                NULL, 1, LOG_KLOG, 0, NULL) < 0) {
        fprintf(stderr, "\nFailed to delete %s\n", rm_bin_argv[2]);
        return false;
    }
    fprintf(stderr, "\rSTAGE 2/2: 100%% Complete\n");
    return true;
}

int main (const int argc, char ** const argv) {
    int opt;
    int mandatory_opt;
    char *path = NULL;
    int seed = time(NULL);

    u64 total_size = 0;
    u64 hole_size = 0;
    u64 total_hole_size = 0;

    mandatory_opt = 2;
    while ((opt = getopt(argc, argv, "s:h:t:")) != -1) {
        switch(opt) {
            case 's':
                seed = atoi(optarg);
                break;
            case 'h':
                hole_size = atoll(optarg);
                mandatory_opt--;
                break;
            case 't':
                total_hole_size = atoll(optarg);
                mandatory_opt--;
                break;
            default:
                usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    if (mandatory_opt) {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }
    if (optind >= argc) {
        fprintf(stderr, "\nExpected path name after options.\n");
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }
    path = argv[optind++];

    if (optind < argc) {
        fprintf(stderr, "\nUnexpected argument: %s\n", argv[optind]);
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    srandom(seed);
    fprintf(stderr, "\nRandom seed is: %d\n", seed);

    total_size = get_free_space(path);
    if (!total_size) {
        exit(EXIT_FAILURE);
    }
    if (total_size < total_hole_size || total_hole_size < hole_size) {
        fprintf(stderr, "\nInvalid sizes: total available size should be "
                        "larger than total hole size which is larger than "
                        "hole size\n");
        exit(EXIT_FAILURE);
    }

    if (!puncture_fs(path, total_size, hole_size, total_hole_size)) {
        exit(EXIT_FAILURE);
    }
    return 0;
}
