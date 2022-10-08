/*
 * Copyright (C) 2011 The Android Open Source Project
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

// Minimal test program for clock

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

// this thread soaks the CPU so that clock() function will advance
void *cpu_hog(void *arg)
{
    for (;;) {
        // the system call should not be optimized away by the compiler
        (void) getpid();
    }
}

int main(int argc, char **argv)
{
    pthread_t thread;
    clock_t ticks10, ticks15;

    // do not call clock() here so we can test initialization

    // soak the CPU for 10 seconds, then read clock
    pthread_create(&thread, NULL, cpu_hog, NULL);
    sleep(10);
    ticks10 = clock();

    // soak the CPU for 5 more seconds, then read clock
    sleep(5);
    ticks15 = clock();

    // print the results
    printf("CLOCKS_PER_SEC = %ld ticks/sec\n", (clock_t) CLOCKS_PER_SEC);
    printf("At 10 secs clock=%lu, at 15 secs clock=%lu\n", ticks10, ticks15);

    // exit could wait for the other thread to complete
    _exit(EXIT_SUCCESS);
}
