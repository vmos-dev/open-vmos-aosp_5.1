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

#include <unistd.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include "ext4fixup.h"

static void usage(char *me)
{
    fprintf(stderr, "%s: usage: %s [-vn] <image or block device>\n", me, me);
}

int main(int argc, char **argv)
{
    int opt;
    int verbose = 0;
    int no_write = 0;
    char *fsdev;
    char *me;
    int stop_phase = 0, stop_loc = 0, stop_count = 0;

    me = basename(argv[0]);

    while ((opt = getopt(argc, argv, "vnd:")) != -1) {
        switch (opt) {
        case 'v':
            verbose = 1;
            break;
        case 'n':
            no_write = 1;
            break;
        case 'd':
            sscanf(optarg, "%d,%d,%d", &stop_phase, &stop_loc, &stop_count);
            break;
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "expected image or block device after options\n");
        usage(me);
        exit(EXIT_FAILURE);
    }

    fsdev = argv[optind++];

    if (optind < argc) {
        fprintf(stderr, "Unexpected argument: %s\n", argv[optind]);
        usage(me);
        exit(EXIT_FAILURE);
    }

    return ext4fixup_internal(fsdev, verbose, no_write, stop_phase, stop_loc, stop_count);
}
