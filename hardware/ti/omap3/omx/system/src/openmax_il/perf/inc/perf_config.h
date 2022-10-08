
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
#ifndef __PERF_CONFIG_H__
#define __PERF_CONFIG_H__

/******************************************************************************
    CONFIGURATION
******************************************************************************/

#define TRUE (1)
#define FALSE (0)

#ifdef _WIN32
    #define PERF_CONFIG_FILE "C:\\perf.ini"
    #define strcasecmp stricmp
    #define strncasecmp strnicmp
#else
    #define PERF_CONFIG_FILE "./perf.ini"
#endif

#define PERF_CONFIG_LINELENGTH 1024

/** in order to prevent writing into the same log files during
*   operations, each component writes its log or debug into the
*   following files:
* 
*   base_PID_handle.pref - for binary logs
*   base_PID_handle.log  - for text logs
*  */

typedef struct PERF_Config
{
    /* logging interface */
    unsigned long  mask;           /* bitmask for enabled components */
    unsigned long  buffer_size;    /* log buffer size */
    unsigned long  delayed_open;   /* open trace file only when first block
                                      is written */
    char          *trace_file;     /* file base to save trace */

    /* debug interface */
    unsigned long  csv;            /* comma-separated value output */
    unsigned long  debug;          /* debug flag - will print some events,
                                      but not all */
    unsigned long  detailed_debug; /* debug flag - will print ALL events */

    char          *log_file;       /* file to save all event logs */

    /* replay interface */
    char          *replay_file;    /* file to print replayed event logs */

    /* real-time interface */
    unsigned long  realtime;       /* real-time enabled flag */
    unsigned long  rt_granularity; /* real-time granularity in seconds
                                      valid range 1 - 15 */
    unsigned long  rt_detailed;    /* real-time detailed flag: 
                                      0: will only report preferred rates
                                      1: will report all rates to/from Hardware,
                                         LLMM, HLMM
                                      2: will report all rates measured */
    unsigned long  rt_debug;       /* print current statistics on every update */
    unsigned long  rt_summary;     /* print summary on close */
    char          *rt_file;        /* file to save all real-time logs */
} PERF_Config;

void PERF_Config_Init(PERF_Config *sConfig);
void PERF_Config_Read(PERF_Config *sConfig, char const *tag);
void PERF_Config_Release(PERF_Config *sConfig);

#endif

