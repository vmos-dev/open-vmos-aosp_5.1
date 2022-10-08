
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

#ifndef __PERF_RT_H
#define __PERF_RT_H

/* ============================================================================
   DEBUG STRUCTURES
============================================================================ */
typedef struct PERF_RTdata_uptime
{
    /* data needed for uptime calculations */
    double start_uptime;
    double start_idletime;
    double last_uptime;
    double last_idletime;
    int    measuring;
    int    success;

    /* statistics */
    long   n;
    double x, xx;

    /* real-time data */
    TIME_STRUCT last_reporting;
} PERF_RTdata_uptime;

typedef struct PERF_RTdata_rate
{
    /* rate parameters */
    PERF_MODULETYPE modulesAndFlags;
    unsigned long   size;

    /* data needed for frame rate calculations */
    TIME_STRUCT last_timestamp;
    int skip;

    /* statistics:
        n, x, xx: whole lifecycle
        tn, tx, txx: temporal statistics (for grandularity)
        tn0: tn in the last temporal statistics phase (since the last executing phase)
             if frame rate is less than 0.5fps and tn0 is less than 10 in the last phase,
             we ignore and do not print the frames unless debug & 4 is set.
        an, ax, axx: average temporal statistics - yields stdev */
    long   n, tn, tn0, an;
    unsigned long x, tx;
    double xx, txx, ax, axx;

    /* real-time data */
    TIME_STRUCT last_reporting;
} PERF_RTdata_rate;

typedef struct PERF_RTdata_delay
{
    /* delay parameters */
    /* NONE for now */

    /* data needed for delay calculations */
    TIME_STRUCT last_timestamp;

    /* statistics: this is real-time in nature, so no "buffering" is performed.
       therefore, no need for temporal statistics
    */
    long n;
    unsigned long x;
    double xx;
} PERF_RTdata_delay;

typedef struct PERF_RTdata_sts
{
    int capturing;
    unsigned long size_min, size_max;

    unsigned long last_burst;             /* last burst so we don't count the
                                             last delay of each raw burst into
                                             the modified number */

    PERF_RTdata_delay dSingle;            /* single shot-to-shot */
    PERF_RTdata_delay dBurst, dABurst;    /* raw burst, average of all bursts */
    PERF_RTdata_delay dBurst2, dABurst2;  /* modified burst, average */
} PERF_RTdata_sts;

typedef struct PERF_RT_Private
{
    /* configuration */
    FILE *fRt;                       /* file to real-time output (Buffer) */
    long   granularity;
    int    summary;
    int    detailed;
    int    debug;                    /* bit: 1 & any - print temporal stats
                                             2       - print difference between temporal average and true average
                                             4       - use all frames for rates, not just after 10 frames
                                     */

    /* state data for reporting */
    TIME_STRUCT first_time;

    /* uptime data */
    struct PERF_RTdata_uptime *dUptime; /* uptime data */
    
    /* rate data */
    int    steadyState;              /* are we in steady state? */
    int    needSteadyState;          /* do we need steady state? */
    struct PERF_RTdata_rate *dRate;  /* rate data */
    int    nDRate;                   /* number of dRate structures */
    int    maxDRate;                 /* maximum number of dRates */
    int    encoder;                  /* encoder, sending arbitrary sizes */
    int    decoder;                  /* decoder, receiving arbitrary sizes */
    unsigned long only_moduleandflags;  /* the module and flags we care about - if detailed is 0 */

    /* shot-to-shot data */
    struct PERF_RTdata_sts *dSTS;    /* single-shot and burst modes */

} PERF_RT_Private;

void
PERF_RT_done(PERF_Private *perf);

PERF_RT_Private *
PERF_RT_create(PERF_Private *perf, PERF_Config *config, PERF_MODULETYPE eModule);

void
__rt_Boundary(PERF_Private *perf,PERF_BOUNDARYTYPE eBoundary);

void
__rt_Buffer(PERF_Private *perf,unsigned long ulAddress1,
            unsigned long ulAddress2,
            unsigned long ulSize,
            PERF_MODULETYPE eModule);

void
__rt_Command(PERF_Private *perf,
             unsigned long ulCommand,
             unsigned long ulArgument,
             PERF_MODULETYPE eModule);

void
__rt_Create(PERF_Private *perf);

void
__rt_Done(PERF_Private *perf);

void
__rt_Log(PERF_Private *perf,
         unsigned long ulData1, unsigned long ulData2,
         unsigned long ulData3);

void
__rt_SyncAV(PERF_Private *perf,
            float pfTimeAudio,
            float pfTimeVideo,
            PERF_SYNCOPTYPE eSyncOperation);
    
void
__rt_ThreadCreated(PERF_Private *perf,
                   unsigned long ulThreadID,
                   unsigned long ulThreadName);
    
#endif

