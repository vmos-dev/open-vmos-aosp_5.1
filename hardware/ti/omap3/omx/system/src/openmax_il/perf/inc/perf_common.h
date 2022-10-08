
/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
/* This should be only included from perf.h and perf_config.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* This file contains the implementation of the PERF instrumentation object,
   including the customizable interfaces */

/******************************************************************************
    GENERIC TYPES
******************************************************************************/
#ifdef _WIN32
    #undef INLINE_SUPPORTED
    #define INLINE
    #define INLINEORSTATIC static

    #include <time.h>

/* time and process ID routines */

    #define TIME_STRUCT                    unsigned long
    #define TIME_GET(target)               time(&target)
    #define TIME_COPY(target, source)      target = source
    #define TIME_MICROSECONDS(time)        0
    #define TIME_SECONDS(time)             (time)
    #define TIME_INCREASE(time, microsecs) time += microsecs / 1000000
    #define TIME_SET(time, sec, microsec)  time = sec
    #define PID_GET(target)                target = 0

#else
    #ifdef __STRICT_ANSI__
    /* for some reason strdup, strcasecmp and strncasecmp does not get
       defined on ANSI builds */
    extern char *strdup(char const *);
    extern int strcasecmp(const char *, const char *);
    extern int strncasecmp(const char *, const char *, size_t);
    #endif

    #undef INLINE_SUPPORTED
    #define INLINE
    #define INLINEORSTATIC static

    #include <sys/time.h>
    #include <sys/types.h>
    #include <unistd.h>

/* time and process ID routines */

    #define TIME_STRUCT                    struct timeval
    #define TIME_GET(target)               gettimeofday(&target, NULL)
    #define TIME_COPY(target, source) \
    ((target).tv_sec = (source).tv_sec), ((target).tv_usec = (source).tv_usec)
    #define TIME_MICROSECONDS(time)        (time).tv_usec
    #define TIME_SECONDS(time)             (time).tv_sec
    #define TIME_INCREASE(time, microsecs) \
    ((time).tv_sec += ((microsecs) / 1000000) +                             \
                       ((time).tv_usec + (microsecs) % 1000000) / 1000000), \
    ((time).tv_usec = ((time).tv_usec + (microsecs) % 1000000) % 1000000)
    #define TIME_SET(time, sec, microsec) \
    ((time).tv_sec = (sec)), ((time).tv_usec = (microsec))
    #define PID_GET(target)                target = getpid()

#endif

#define TIME_DELTA(time, base)                             \
    ((TIME_SECONDS(time) - TIME_SECONDS(base)) * 1000000 + \
     (TIME_MICROSECONDS(time) - TIME_MICROSECONDS(base))) 

