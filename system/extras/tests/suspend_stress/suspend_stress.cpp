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

#include <getopt.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/epoll.h>
#include <sys/timerfd.h>

#include <cutils/klog.h>

#define NSEC_PER_SEC (1000*1000*1000)
#define MSEC_PER_SEC 1000
#define NSEC_PER_MSEC (NSEC_PER_SEC/MSEC_PER_SEC)

long long timediff_ns(const struct timespec *a, const struct timespec *b) {
    return ((long long)(a->tv_sec - b->tv_sec)) * NSEC_PER_SEC +
            (a->tv_nsec - b->tv_nsec);
}

void usage(void)
{
    printf("usage: suspend_stress [ <options> ]\n"
           "options:\n"
           "  -a,--abort                abort test on late alarm\n"
           "  -c,--count=<count>        number of times to suspend (default infinite)\n"
           "  -t,--time=<seconds>       time to suspend for (default 5)\n"
        );
}

int main(int argc, char **argv)
{
    int alarm_time = 5;
    int count = -1;
    bool abort_on_failure = false;

    while (1) {
        const static struct option long_options[] = {
            {"abort", no_argument, 0, 'a'},
            {"count", required_argument, 0, 'c'},
            {"time", required_argument, 0, 't'},
        };
        int c = getopt_long(argc, argv, "ac:t:", long_options, NULL);
        if (c < 0) {
            break;
        }

        switch (c) {
        case 'a':
            abort_on_failure = true;
            break;
        case 'c':
            count = strtoul(optarg, NULL, 0);
            break;
        case 't':
            alarm_time = strtoul(optarg, NULL, 0);
            break;
        case '?':
            usage();
            exit(EXIT_FAILURE);
        default:
            abort();
        }
    }

    klog_init();
    klog_set_level(KLOG_INFO_LEVEL);

    if (optind < argc) {
        fprintf(stderr, "Unexpected argument: %s\n", argv[optind]);
        usage();
        exit(EXIT_FAILURE);
    }

    int fd = timerfd_create(CLOCK_BOOTTIME_ALARM, 0);
    if (fd < 0) {
        perror("timerfd_create failed");
        exit(EXIT_FAILURE);
    }

    struct itimerspec delay = itimerspec();
    delay.it_value.tv_sec = alarm_time;
    int i = 0;

    int epoll_fd = epoll_create(1);
    if (epoll_fd < 0) {
        perror("epoll_create failed");
        exit(EXIT_FAILURE);
    }

    struct epoll_event ev = epoll_event();
    ev.events = EPOLLIN | EPOLLWAKEUP;
    int ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
    if (ret < 0) {
        perror("epoll_ctl failed");
        exit(EXIT_FAILURE);
    }

    while (count != 0) {
        struct timespec expected_time;
        struct timespec actual_time;
        uint64_t fired = 0;

        ret = timerfd_settime(fd, 0, &delay, NULL);
        if (ret < 0) {
            perror("timerfd_settime failed");
            exit(EXIT_FAILURE);
        }

        ret = clock_gettime(CLOCK_BOOTTIME, &expected_time);
        if (ret < 0) {
            perror("failed to get time");
            exit(EXIT_FAILURE);
        }
        expected_time.tv_sec += alarm_time;

        ret = 0;
        while (ret != 1) {
            struct epoll_event out_ev;
            ret = epoll_wait(epoll_fd, &out_ev, 1, -1);
            if (ret < 0 && errno != EINTR) {
                perror("epoll_wait failed");
                exit(EXIT_FAILURE);
            }
        }

        ssize_t bytes = read(fd, &fired, sizeof(fired));
        if (bytes < 0) {
            perror("read from timer fd failed");
            exit(EXIT_FAILURE);
        } else if (bytes < (ssize_t)sizeof(fired)) {
            fprintf(stderr, "unexpected read from timer fd: %zd\n", bytes);
        }

        if (fired != 1) {
            fprintf(stderr, "unexpected timer fd fired count: %" PRIu64 "\n", fired);
        }

        ret = clock_gettime(CLOCK_BOOTTIME, &actual_time);
        if (ret < 0) {
            perror("failed to get time");
            exit(EXIT_FAILURE);
        }

        long long diff = timediff_ns(&actual_time, &expected_time);
        if (llabs(diff) > NSEC_PER_SEC) {
            fprintf(stderr, "alarm arrived %lld.%03lld seconds %s\n",
                    llabs(diff) / NSEC_PER_SEC,
                    (llabs(diff) / NSEC_PER_MSEC) % MSEC_PER_SEC,
                    diff > 0 ? "late" : "early");
            KLOG_ERROR("suspend_stress", "alarm arrived %lld.%03lld seconds %s\n",
                    llabs(diff) / NSEC_PER_SEC,
                    (llabs(diff) / NSEC_PER_MSEC) % MSEC_PER_SEC,
                    diff > 0 ? "late" : "early");
            if (abort_on_failure) {
                exit(EXIT_FAILURE);
            }
        }

        time_t t = time(NULL);
        i += fired;
        printf("timer fired: %d at boottime %lld.%.3ld, %s", i,
                   (long long)actual_time.tv_sec,
                   actual_time.tv_nsec / NSEC_PER_MSEC,
                   ctime(&t));

        KLOG_INFO("suspend_stress", "timer fired: %d at boottime %lld.%.3ld, %s", i,
                   (long long)actual_time.tv_sec,
                   actual_time.tv_nsec / NSEC_PER_MSEC,
                   ctime(&t));

        if (count > 0)
            count--;
    }
    return 0;
}
