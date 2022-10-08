/*
 * Copyright (C) 2012 The Android Open Source Project
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

#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "edify/expr.h"

// The bootloader block /dev/block/mmcblk0boot0 contains a low-level
// bootloader (BL1/BL2) followed by a "boot flag", followed by a
// primary copy of the full bootloader, then a backup copy of the full
// bootloader.  The boot flag tells the low-level code which copy of
// the big bootloader to boot.
//
// The strategy here is to read the boot flag, write the *other* copy
// of the big bootloader, flip the boot flag to that freshly-written
// copy, write remaining copy of the big bootloader, then flip the
// flag back (so most of the time it will be booting the primary
// copy).  At the end we update the BL1/BL2, which is slightly
// dangerous (there's no backup; if we fail during writing this part
// then the device is a brick).  We could remove that before launch,
// or restrict it to only being written on test-keys/dev-keys devices.

#define BOOTFLAG_OFFSET (31*1024)

#define BL1BL2_LENGTH (31*1024)

#define INPUT_OFFSET (35*1024)
#define PRIMARY_OUTPUT_OFFSET (35*1024)
#define SECONDARY_OUTPUT_OFFSET (1280*1024)
#define BIG_LENGTH (1245*1024)

static void copy_block(FILE *f, unsigned char* data, size_t in_offset,
                       size_t length, size_t out_offset) {
    if (fseek(f, out_offset, SEEK_SET) < 0) {
        fprintf(stderr, "failed to seek to %d: %s\n",
                out_offset, strerror(errno));
        return;
    }
    if (fwrite(data+in_offset, 1, length, f) != length) {
        fprintf(stderr, "failed to write bootloader: %s\n", strerror(errno));
        return;
    }
    fflush(f);
    fsync(fileno(f));
}

static int get_bootflag(FILE *f) {
    fseek(f, BOOTFLAG_OFFSET, SEEK_SET);
    char buffer[8];
    if (fread(buffer, 1, 8, f) != 8) {
        fprintf(stderr, "failed to read boot flag: %s\n", strerror(errno));
        return 0;
    }

    fprintf(stderr, "bootflag is [%c%c%c%c%c%c%c%c]\n",
            buffer[0], buffer[1], buffer[2], buffer[3],
            buffer[4], buffer[5], buffer[6], buffer[7]);

    if (strncmp(buffer, "MANTABL", 7) != 0) return 0;
    if (buffer[7] == '1') return 1;
    if (buffer[7] == '2') return 2;
    return 0;
}

static void set_bootflag(FILE* f, int value) {
    unsigned char buffer[9] = "MANTABLx";
    buffer[7] = '0' + value;
    copy_block(f, buffer, 0, 8, BOOTFLAG_OFFSET);
}

static int update_bootloader(unsigned char* img_data,
                             size_t img_size,
                             char* block_fn,
                             char* force_ro_fn) {
    if (img_size != INPUT_OFFSET + BIG_LENGTH) {
        fprintf(stderr, "expected bootloader.img of length %d; got %d\n",
                INPUT_OFFSET + BIG_LENGTH, img_size);
        return -1;
    }


    FILE* f = fopen(force_ro_fn, "w");
    if (!f) {
        fprintf(stderr, "failed to open %s: %s\n", force_ro_fn, strerror(errno));
        return -1;
    }
    if (fwrite("0", 1, 1, f) != 1) {
        fprintf(stderr, "failed to write %s: %s\n", force_ro_fn, strerror(errno));
        return -1;
    }
    fflush(f);
    fsync(fileno(f));
    if (fclose(f) != 0) {
        fprintf(stderr, "failed to close %s: %s\n", force_ro_fn, strerror(errno));
        return -1;
    }

    f = fopen(block_fn, "r+b");
    if (!f) {
        fprintf(stderr, "failed to open %s: %s\n", block_fn, strerror(errno));
        return -1;
    }

    int i;
    int bootflag = 0;
    for (i = 0; i < 2; ++i) {
        bootflag = get_bootflag(f);

        switch (bootflag) {
            case 1:
                fprintf(stderr, "updating secondary copy of bootloader\n");
                copy_block(f, img_data, INPUT_OFFSET, BIG_LENGTH, SECONDARY_OUTPUT_OFFSET);
                set_bootflag(f, 2);
                break;
            case 2:
                fprintf(stderr, "updating primary copy of bootloader\n");
                copy_block(f, img_data, INPUT_OFFSET, BIG_LENGTH, PRIMARY_OUTPUT_OFFSET);
                set_bootflag(f, 1);
                break;
            case 0:
                fprintf(stderr, "no bootflag; updating entire bootloader block\n");
                copy_block(f, img_data, 0, img_size, 0);
                i = 2;
                break;
        }
    }

    if (bootflag != 0) {
        fprintf(stderr, "updating BL1/BL2\n");
        copy_block(f, img_data, 0, BL1BL2_LENGTH, 0);
    }

    fclose(f);
    return 0;
}

Value* WriteBootloaderFn(const char* name, State* state, int argc, Expr* argv[])
{
    int result = -1;
    Value* img;
    Value* block_loc;
    Value* force_ro_loc;

    if (argc != 3) {
        return ErrorAbort(state, "%s() expects 3 args, got %d", name, argc);
    }

    if (ReadValueArgs(state, argv, 3, &img, &block_loc, &force_ro_loc) < 0) {
        return NULL;
    }

    if(img->type != VAL_BLOB ||
       block_loc->type != VAL_STRING ||
       force_ro_loc->type != VAL_STRING) {
      FreeValue(img);
      FreeValue(block_loc);
      FreeValue(force_ro_loc);
      return ErrorAbort(state, "%s(): argument types are incorrect", name);
    }

    result = update_bootloader(img->data, img->size,
                               block_loc->data, force_ro_loc->data);
    FreeValue(img);
    FreeValue(block_loc);
    FreeValue(force_ro_loc);
    return StringValue(strdup(result == 0 ? "t" : ""));
}

void Register_librecovery_updater_manta() {
    fprintf(stderr, "installing samsung.manta updater extensions\n");

    RegisterFunction("samsung.manta.write_bootloader", WriteBootloaderFn);
}
