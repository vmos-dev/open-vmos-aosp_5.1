/*
** Copyright 2008 The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

int main(int argc, char **argv) {
    int i;

    struct timeval tv1;
    struct timeval tv2;
    long max = 0;
    long avg = 0;

    for(i = 1; ; i++) {
        gettimeofday(&tv1, NULL);
        usleep(1000);
        gettimeofday(&tv2, NULL);

        long usec = (tv2.tv_sec - tv1.tv_sec) * 1000000 + tv2.tv_usec - tv1.tv_usec;
        avg += usec;

        if (usec > max) max = usec;
        if (!(i % 1000)) {
            avg /= 1000;
            printf("max %ld\tavg %ld\n", max, avg);
            max = 0;
            avg = 0;
        }
    }
    return 0;
}
