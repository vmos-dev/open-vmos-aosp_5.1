/*
 * Copyright 2014 The Android Open Source Project
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

#include <dumpstate.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

static const char base64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char pad64 = '=';

static void base64_output3(const unsigned char *src, int len)
{
    printf("%c", base64[src[0] >> 2]);
    printf("%c", base64[((src[0] & 0x03) << 4) | (src[1] >> 4)]);
    if (len == 1) {
        printf("==");
        return;
    }
    printf("%c", base64[((src[1] & 0x0F) << 2) | (src[2] >> 6)]);
    if (len == 2) {
        printf("=");
        return;
    }
    printf("%c", base64[src[2] & 0x3F]);
}

static void fugu_dump_base64(const char *path)
{

    printf("------ (%s) ------\n", path);
    int fd = open(path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) {
        printf("*** %s: %s\n\n", path, strerror(errno));
        return;
    }

    /* buffer size multiple of 3 for ease of use */
    unsigned char buffer[1200];
    int left = 0;
    int count = 0;
    for (;;) {
        int ret = read(fd, &buffer[left], sizeof(buffer) - left);
        if (ret <= 0) {
            break;
        }
        left += ret;
        int ofs = 0;
        while (left > 2) {
            base64_output3(&buffer[ofs], 3);
            left -= 3;
            ofs += 3;
            count += 4;
            if (count > 72) {
                printf("\n");
                count = 0;
            }
        }
        if (left) {
            memmove(buffer, &buffer[ofs], left);
        }
    }
    close(fd);

    if (!left) {
        printf("\n------ end ------\n");
        return;
    }

    /* finish padding */
    count = left;
    while (count < 3) {
        buffer[count++] = 0;
    }
    base64_output3(buffer, left);

    printf("\n------ end ------\n");
}

void dumpstate_board()
{
    dump_file("INTERRUPTS", "/proc/interrupts");
    dump_file("last ipanic_console", "/data/dontpanic/ipanic_console");
    dump_file("last ipanic_threads", "/data/dontpanic/ipanic_threads");
    fugu_dump_base64("/dev/snd_atvr_mSBC");
    fugu_dump_base64("/dev/snd_atvr_pcm");
};
