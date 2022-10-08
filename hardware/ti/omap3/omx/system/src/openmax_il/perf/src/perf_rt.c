
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

#ifdef __PERF_CUSTOMIZABLE__

    #define __PERF_RT_C__

    #include "perf_config.h"
    #include "perf.h"
    #include "perf_rt.h"
    #include "math.h"

/* ============================================================================
   DEBUG RT METHODS
============================================================================ */

#define MAX_RATES_TRACKED   10
#define MAX_GRANULARITY     15
#define MIN_FRAMES_FOR_RATE 10

static int uptime_started = 0;

static void init_delay(PERF_RTdata_delay *dDelay, long n0)
{
    dDelay->xx = dDelay->x = 0;
    dDelay->n = n0;
}

PERF_RT_Private *
PERF_RT_create(PERF_Private *perf, PERF_Config *config,
               PERF_MODULETYPE eModule)
{
    char *fOutFile = NULL;
    FILE *fOut = NULL;

    /* check if we support this component */
    if (perf->ulID != PERF_FOURS("CAM_") && perf->ulID != PERF_FOURS("CAMT") &&
        perf->ulID != PERF_FOURS("VP__") && perf->ulID != PERF_FOURS("VP_T") &&
        perf->ulID != PERF_FOURS("VD__") && perf->ulID != PERF_FOURS("VD_T") &&
        perf->ulID != PERF_FOURS("VE__") && perf->ulID != PERF_FOURS("VE_T"))
    {
        /* if we don't support this component, we don't create the real-time
           interface */
        return (NULL);
    }

    PERF_RT_Private *me =
    perf->cip.pRT = malloc(sizeof(PERF_RT_Private));

    if (me)
    {
        int succeed = 1;

        /* we track steady state on the component thread only */
        me->needSteadyState = (perf->ulID & 0xff) == 'T';
        me->steadyState = 0;

        /* allocate rate tracking structures */
        me->maxDRate = MAX_RATES_TRACKED;
        me->dRate = malloc(sizeof(PERF_RTdata_rate) * me->maxDRate);
        succeed = succeed && me->dRate;

        me->decoder = (perf->ulID == PERF_FOURS("VD__") || perf->ulID == PERF_FOURS("VD_T"));
        me->encoder = (perf->ulID == PERF_FOURS("VE__") || perf->ulID == PERF_FOURS("VE_T"));
        me->nDRate = 0;

        /* allocate shot-to-shot tracking structures */
        if (succeed && perf->ulID == PERF_FOURS("CAMT"))
        {
            me->dSTS = malloc(sizeof(PERF_RTdata_sts));
            succeed = succeed && me->dSTS;
            if (me->dSTS)
            {
                init_delay(&me->dSTS->dBurst, -1);   /* no first timestamp yet */
                init_delay(&me->dSTS->dABurst, 0);
                init_delay(&me->dSTS->dBurst2, 0);
                init_delay(&me->dSTS->dABurst2, 0);
                init_delay(&me->dSTS->dSingle, -1);  /* no first timestamp yet */
                me->dSTS->size_max = me->dSTS->size_min = me->dSTS->capturing = 0;
            }
        }
        else
        {
            me->dSTS = NULL;
        }

        /* allocate uptime tracking structures */

        /* :NOTE: for now we restrict creations of uptime to steady state
            only */
        if (succeed && !uptime_started && me->needSteadyState &&
            !(perf->uMode & PERF_Mode_Replay))
        {
            me->dUptime = malloc(sizeof(PERF_RTdata_uptime));
            succeed = succeed && me->dUptime;

            if (succeed)
            {
                uptime_started = 1;
                me->dUptime->measuring = 0;
                me->dUptime->last_idletime = me->dUptime->last_uptime = 0;
                me->dUptime->start_idletime = me->dUptime->start_uptime = 0;
                me->dUptime->success = 1;
                me->dUptime->xx = me->dUptime->x = me->dUptime->n = 0;
                TIME_GET(me->dUptime->last_reporting);
            }
        }
        else
        {
            me->dUptime = NULL;
        }

        /* configuration */
        me->summary     = config->rt_summary != 0;
        me->debug       = config->rt_debug & 0x1FF;
        me->detailed    = (config->rt_detailed > 2) ? 2 : (int) config->rt_detailed;

        me->granularity = (config->rt_granularity < 1) ? 1 :
                          (config->rt_granularity > MAX_GRANULARITY) ?
                          MAX_GRANULARITY : (long) config->rt_granularity;
        me->granularity *= 1000000;  /* convert to microsecs */
        TIME_COPY(me->first_time, perf->time);

        /* if we do not care about detailed statistics, only report significant
           statistics for each component */
        if (succeed && !me->detailed)
        {
            /* VP_T - display rate */
            if (perf->ulID == PERF_FOURS("VP_T"))
            {
                me->only_moduleandflags = PERF_FlagSending | PERF_FlagFrame | PERF_ModuleHardware;
            }
            /* VD_T - decode rate */
            else if (perf->ulID == PERF_FOURS("VD_T"))
            {
                me->only_moduleandflags = PERF_FlagSending | PERF_FlagFrame | PERF_ModuleLLMM;
            }
            /* VE_T - encode rate */
            else if (perf->ulID == PERF_FOURS("VE_T"))
            {
                me->only_moduleandflags = PERF_FlagSending | PERF_FlagFrame | PERF_ModuleLLMM;
            }
            /* CAMT - capture rate */
            else if (perf->ulID == PERF_FOURS("CAMT"))
            {
                me->only_moduleandflags = PERF_FlagReceived | PERF_FlagFrame | PERF_ModuleHardware;
            }
            /* otherwise, we don't care about rates */
            else
            {
                free(me->dRate);
                me->dRate = NULL;
                me->maxDRate = 0;
            }
        }

        /* set up fRt file pointers */
        if (config->rt_file && succeed)
        {
            /* open log file unless STDOUT or STDERR is specified */
            if (!strcasecmp(config->rt_file, "STDOUT")) fOut = stdout;
            else if (!strcasecmp(config->rt_file, "STDERR")) fOut = stderr;
            else
            {
                /* expand file name with PID and name */
                fOutFile = (char *) malloc (strlen(config->rt_file) + 32);
                if (fOutFile)
                {
                    sprintf(fOutFile, "%s-%05lu-%08lx-%c%c%c%c.log",
                            config->rt_file, perf->ulPID, (unsigned long) perf,
                            PERF_FOUR_CHARS(perf->ulID));
                    fOut = fopen(fOutFile, "at");

                    /* free new file name */
                    free(fOutFile);
                    fOutFile = NULL;
                }

                /* if could not open output, set it to STDOUT */
                if (!fOut) fOut = stderr;
            }
            me->fRt = fOut;
        }

        /* if we had allocation failures, free resources and return NULL */
        if (succeed)
        {
            perf->uMode |= PERF_Mode_RealTime;
        }
        else
        {
            PERF_RT_done(perf);
            me = NULL;
        }
    }

    return(me);
}

void PERF_RT_done(PERF_Private *perf) 
{
    PERF_RT_Private *me = perf->cip.pRT;

    /* close debug file unless stdout or stderr */
    if (me->fRt && me->fRt != stdout &&
        me->fRt != stderr) fclose(me->fRt);

    /* free allocated structures */
    free(me->dRate);   me->dRate = NULL;
    free(me->dUptime); me->dUptime = NULL;
    free(me->dSTS);    me->dSTS = NULL;

    /* free private structure */
    free(me);
    perf->cip.pRT = NULL;
}

static void get_uptime(double *uptime, double *idletime)
{
    FILE *fUptime = fopen("/proc/uptime", "r");
    if (fUptime)
    {
        fscanf(fUptime, "%lg %lg", uptime, idletime);
        fclose (fUptime);
    }
    else
    {
        *uptime = *idletime = 0.;
    }
}

static void start_stop_uptime(PERF_RTdata_uptime *dUptime, int start)
{
    double uptime, idletime;

    if (dUptime && dUptime->measuring != start)
    {
        /* add uptime since last time */
        get_uptime(&uptime, &idletime);

        /* if successful */
        if (dUptime->success && uptime && idletime)
        {
            dUptime->start_idletime = idletime - dUptime->start_idletime;
            dUptime->start_uptime = uptime - dUptime->start_uptime;
            dUptime->last_idletime = idletime - dUptime->last_idletime;
            dUptime->last_uptime = uptime - dUptime->last_uptime;
        }
        else
        {
            dUptime->start_idletime = dUptime->start_uptime = dUptime->success = 0;
            dUptime->last_idletime = dUptime->last_uptime = 0;
        }

        dUptime->measuring = start;
    }
}

extern char const * const PERF_ModuleTypes[];

double my_sqrt(double a)
{
    double b = (a + 1) / 2;
    b = (b + a/b) / 2;
    b = (b + a/b) / 2;
    b = (b + a/b) / 2;
    b = (b + a/b) / 2;
    b = (b + a/b) / 2;
    b = (b + a/b) / 2;
    return (b + a/b) / 2;
}

static const char *sendRecvTxt[] = {
        "received", "sending", "requesting", "sent",
    };

static void print_rate_info(FILE *fOut,
                            unsigned long ID, PERF_MODULETYPE modulesAndFlags,
                            unsigned long size, long n)
{
    unsigned long module1 = modulesAndFlags & PERF_ModuleMask;
    unsigned long module2 = (modulesAndFlags >> PERF_ModuleBits) & PERF_ModuleMask;
    int xfering  = PERF_IsXfering(modulesAndFlags);
    int sendIx   = (PERF_GetSendRecv(modulesAndFlags) >> 28) & 3;
    int sending  = PERF_IsSending(modulesAndFlags);
    int frame    = PERF_IsFrame  (modulesAndFlags);

    fprintf(fOut, "%c%c%c%c %s %ld+ %s[0x%lX]%s%s%s%s",
            PERF_FOUR_CHARS(ID),
            xfering ? "xfering" : sendRecvTxt[sendIx],
            n,
            frame ? "frames" : "buffers",
            size,
            (xfering || !sending) ? " from " : " to ",
            (module1 < PERF_ModuleMax ? PERF_ModuleTypes[module1] : "INVALID"),
            xfering ? " to " : "",
            xfering ? (module2 < PERF_ModuleMax ? PERF_ModuleTypes[module2] : "INVALID") : "");
}

void count_temporal_rate(unsigned long ID, PERF_RT_Private *me, PERF_RTdata_rate *dRate)
{
    /* get the temporal rate */
    double x = (dRate->tn ? (dRate->tx ? ((1e6 * dRate->tn) / dRate->tx) : 1e6) : 0.);
    if (me->debug)
    {
        fprintf(me->fRt, "rtPERF: [%ld] ", TIME_DELTA(dRate->last_reporting, me->first_time)/1000000);
        print_rate_info(me->fRt,
                        ID, dRate->modulesAndFlags, dRate->size, dRate->tn);

        /* calculate smoothness */
        double s = dRate->txx ? (dRate->tx * (double) dRate->tx / dRate->txx / dRate->tn) : 1;

        fprintf(me->fRt, ": %.3g fps (s=%.3g)\n", x, s);
    }

    /* calculate the average of the temporal rate */
    dRate->axx += x * x;
    dRate->ax  += x;
    dRate->an  ++;

    dRate->txx = dRate->tx = dRate->tn = 0;
}

static void delay_add(PERF_RTdata_delay *dDelay, unsigned long delta)
{
    dDelay->x  += delta;
    dDelay->xx += delta * (double) delta;
    dDelay->n  ++;
}

static void delay_delta(PERF_RTdata_delay *dDelay, PERF_Private *perf)
{
    if (dDelay->n < 0)
    {
        dDelay->n++;
    }
    else
    {
        delay_add(dDelay, TIME_DELTA(perf->time, dDelay->last_timestamp));
    }
    TIME_COPY(dDelay->last_timestamp, perf->time);
}

static void count_delay(PERF_RT_Private *me, char *tag, PERF_RTdata_delay *dDelay, long n0)
{
    fprintf(me->fRt, "rtPERF: %s[0x%lX]: ", tag, me->dSTS->size_min);
    if (dDelay->n > 0)
    {
        double x = 1e-6 * dDelay->x / dDelay->n;
        double xx = 1e-12 * dDelay->xx / dDelay->n;
        xx = my_sqrt(xx - x * x);

        if (dDelay->n > 1)
        {
            fprintf(me->fRt, "%.3g +- %.3g s (%ld samples)\n",
                    x, xx, dDelay->n);
        }
        else
        {
            fprintf(me->fRt, "%.3g\n", x);
        }
    }
    else
    {
        fprintf(me->fRt, "UNABLE TO CALCULATE\n");
    }

    dDelay->n = n0;
    dDelay->xx = dDelay->x = 0;
}

void __rt_Boundary(PERF_Private *perf, PERF_BOUNDARYTYPE eBoundary)
{
    /* get real-time private structure */
    PERF_RT_Private *me = perf->cip.pRT;

    /* check steady state if we need it */
    if (me->needSteadyState)
    {
        if (eBoundary == (PERF_BoundaryStart | PERF_BoundarySteadyState))
        {
            if (!me->steadyState)
            {
                /* continue uptime measurement */
                start_stop_uptime(me->dUptime, 1);

                /* for each rate, reset skip count as well as tn0 */
                int i;
                for (i = 0; i < me->nDRate; i++)
                {                    
                    me->dRate[i].txx = me->dRate[i].tx = me->dRate[i].tn = me->dRate[i].tn0 = 0;
                    me->dRate[i].skip = 0;
                }                
            }

            me->steadyState = 1;
        }
        else if (eBoundary == (PERF_BoundaryComplete | PERF_BoundarySteadyState))
        {
            if (me->steadyState)
            {
                /* stop uptime measurement */
                start_stop_uptime(me->dUptime, 0);

                /* complete any temporary rate measurements */
                int i;
                for (i = 0; i < me->nDRate; i++)
                {
                    /* only if we had any buffers in this cycle */
                    if (me->dRate[i].tn0 >= MIN_FRAMES_FOR_RATE ||
                        (me->dRate[i].tn && me->debug & 4))
                    {
                        count_temporal_rate(perf->ulID, me, me->dRate + i);
                    }
                }
            }

            me->steadyState = 0;
        }
    }
    else
    {
        /* if we do not check steady state, we still complete on cleanup */
        if (eBoundary == (PERF_BoundaryStart | PERF_BoundaryCleanup))
        {
            /* stop measurements */
            start_stop_uptime(me->dUptime, 0);
        }
    }
}

void __rt_Buffer(PERF_Private *perf,
                 unsigned long ulAddress1,
                 unsigned long ulAddress2,
                 unsigned long ulSize,
                 PERF_MODULETYPE eModuleAndFlags)
{
    /* get real-time private structure */
    PERF_RT_Private *me = perf->cip.pRT;

    /* see if we care about this buffer in the rate calculation */
    unsigned long module = eModuleAndFlags & PERF_ModuleMask;

    /* ------------------------ RATE METRICS ------------------------ */

    /* change HLMM to LLMM for detailed = 0 and 1 */
    if (me->detailed < 2 && module == PERF_ModuleHLMM)
    {
        module = PERF_ModuleLLMM;
    }

    int rate = (me->detailed == 2) ||
               (me->detailed == 0 &&
                (eModuleAndFlags == me->only_moduleandflags && ulSize >= 8)) ||
               (me->detailed == 1 &&
                ((module == PERF_ModuleHardware || module == PERF_ModuleLLMM)));

    if (rate && me->dRate && (!me->needSteadyState || me->steadyState))
    {
        /* change encoded filled frame sizes to 0xBEEFED, as they tend to
           have varying sizes and thus not be counted */
        unsigned long sending = PERF_GetXferSendRecv(eModuleAndFlags);
        unsigned long size = ulSize;
        if ((me->encoder || me->decoder) && !PERF_IsXfering(sending))
        {
            /* reverse sending direction to common layer or socket node */
            if (module >= PERF_ModuleCommonLayer &&
                module <= PERF_ModuleSocketNode)
            {
                sending ^= PERF_FlagSending;
            }
            
            if ((me->encoder && (sending == PERF_FlagSending)) ||
                (me->decoder && (sending == PERF_FlagReceived)))
            {
                size = size ? 0xBEEFED : 0;
            }
        }

        /* see if we are tracking this buffer size */
        int i, j = -1;  /* j is one of the lest used indexes */
        for (i=0; i < me->nDRate; i++)
        {
            if (me->dRate[i].modulesAndFlags == eModuleAndFlags &&
                me->dRate[i].size == size) break;
            if (j < 0 || me->dRate[i].n < me->dRate[j].n)
            {
                j = i;
            }
        }

        /* if we are not yet tracking this buffer, see if we can track
           it. */
        if (i == me->nDRate)
        {
            /* we have space to track it */
            if (i < me->maxDRate)
            {
                me->nDRate++;
            }
            /* if we cannot replace another rate, we don't track it */
            else if (j < 0 || me->dRate[j].n < 2)
            {
                i = me->maxDRate;
            }
            else
            {
                i = j;
            }

            /* start tracking */
            if (i < me->maxDRate)
            {
                me->dRate[i].modulesAndFlags = eModuleAndFlags;
                me->dRate[i].size = size;
                me->dRate[i].xx = me->dRate[i].x = me->dRate[i].n = 0;
                me->dRate[i].txx = me->dRate[i].tx = me->dRate[i].tn = me->dRate[i].tn0 = 0;
                me->dRate[i].axx = me->dRate[i].ax = me->dRate[i].an = 0;
                me->dRate[i].skip = me->needSteadyState ? 0 : 4;
                TIME_COPY(me->dRate[i].last_timestamp, perf->time);
                TIME_COPY(me->dRate[i].last_reporting, perf->time);
            }
        }
        else
        {
            if (me->dRate[i].skip == 0)
            {
                /* see if we passed our granularity */
                int steps = TIME_DELTA(perf->time, me->dRate[i].last_reporting);
                if (steps >= me->granularity) 
                {
                    steps /= me->granularity;

                    /* unless debug bit 4 is set, ignore temporal statistics if
                       we passed the last time by more than a second, and less than
                       the minimul frames were processed in this burst so far, and
                       the last fps was less than 1. */
                    if (!(me->debug & 4) &&
                        (me->dRate[i].tn0 < MIN_FRAMES_FOR_RATE) &&
                        (me->dRate[i].tn < me->granularity * steps))
                    {
                        if (me->debug & 256)
                        {
                            fprintf(me->fRt, "rtPERF: [%ld] IGNORED (steps=%d, tn0=%ld, tn=%ld)\n",
                                    TIME_DELTA(me->dRate[i].last_reporting, me->first_time)/1000000,
                                    steps, me->dRate[i].tn0, me->dRate[i].tn);
                        }
                        me->dRate[i].txx = me->dRate[i].tx = me->dRate[i].tn = me->dRate[i].tn0 = 0;

                        TIME_INCREASE(me->dRate[i].last_reporting, me->granularity * steps);
                        steps = 0;
                    }
                    else if (me->debug & 256)
                    {
                        fprintf(me->fRt, "rtPERF: [%ld] not-ignored (steps=%d, tn0=%ld, tn=%ld)\n",
                                TIME_DELTA(me->dRate[i].last_reporting, me->first_time)/1000000,
                                steps, me->dRate[i].tn0, me->dRate[i].tn);
                    }

                    /* see if we surpassed our granularity.  if yes, calculate
                       temporal statistics */
                    while (steps)
                    {
                        /* count temporal rate */
                        count_temporal_rate(perf->ulID, me, me->dRate + i);
    
                        TIME_INCREASE(me->dRate[i].last_reporting, me->granularity);
                        steps--;
                    }
                }

                /* rate is */
                unsigned long delta = TIME_DELTA(perf->time, me->dRate[i].last_timestamp);
                me->dRate[i].x   += delta;
                me->dRate[i].tx  += delta;
                me->dRate[i].xx  += delta * (double) delta;
                me->dRate[i].txx += delta * (double) delta;
                me->dRate[i].n   ++;
                me->dRate[i].tn  ++;
                me->dRate[i].tn0 ++;
            }
            else
            {
                me->dRate[i].skip--;
                if (me->dRate[i].skip == 0)
                {
                    TIME_COPY(me->dRate[i].last_reporting, perf->time);
                    me->dRate[i].txx = me->dRate[i].tx = me->dRate[i].tn = 0;
                }
            }

            TIME_COPY(me->dRate[i].last_timestamp, perf->time);
        }
    }

    /* ------------------------ SHOT-TO-SHOT METRICS ------------------------ */
    if (me->dSTS)
    {
        if (eModuleAndFlags == (PERF_FlagSending | PERF_FlagFrame | PERF_ModuleHardware))
        {
            /* queueing buffers to camera */

            /* see if resolution has changed */
            if (ulSize < me->dSTS->size_min ||
                ulSize > me->dSTS->size_max)
            {
                /* report burst rate if we have any */
                if (me->debug)
                {
                    if (me->dSTS->dBurst2.n > 0)
                    {
                        count_delay(me, "Modified burst shot-to-shot", &me->dSTS->dBurst2, 0);
                    }
                    if (me->dSTS->dBurst.n > 0)
                    {
                        count_delay(me, "Raw burst shot-to-shot", &me->dSTS->dBurst, -1);
                    }
                }

                me->dSTS->dBurst.n = -1;
                me->dSTS->dBurst2.n = 0;

                /* set new size */
                me->dSTS->size_min = ulSize > 2048 ? ulSize - 2048 : 0;
                me->dSTS->size_max = ulSize;

                /* if more than D1-PAL, we assume it is an image, not a preview */
                if (ulSize > 0x119000)
                {
                    /* new burst mode start */
                    me->dSTS->capturing = 1;
                }
                else
                {
                    /* preview start */
                    me->dSTS->capturing = 0;                    
                }
            }
        }
        else if (eModuleAndFlags == (PERF_FlagReceived | PERF_FlagFrame | PERF_ModuleHardware))
        {
            /* gotten buffers from camera */
            if (me->dSTS->capturing &&
                ulSize >= me->dSTS->size_min &&
                ulSize <= me->dSTS->size_max)
            {
                /* see if we have a capture already (we ignore the first) to
                   count into the modified capture */
                if (me->dSTS->dBurst.n > 1)
                {
                    /* count last time delta */
                    if (me->dSTS->dBurst.n > 2)
                    {
                        delay_add(&me->dSTS->dBurst2, me->dSTS->last_burst);
                        delay_add(&me->dSTS->dABurst2, me->dSTS->last_burst);
                        if (me->debug)
                        {
                            fprintf(me->fRt, "rtPERF: [%ld] Modified burst shot-to-shot[0x%lX]: %.3g s\n",
                                    me->dSTS->dBurst2.n, me->dSTS->size_min, 1e-6 * me->dSTS->last_burst);
                        }
                    }
                    me->dSTS->last_burst = TIME_DELTA(perf->time, me->dSTS->dBurst.last_timestamp);
                }
                else if (me->dSTS->dBurst.n < 0)
                {
                    /* if this is the first shot in the burst sequence */
                    /* calculate single shot-to-shot delay */
                    if (me->dSTS->dSingle.n >= 0)
                    {
                        if (me->debug)
                        {
                            fprintf(me->fRt, "rtPERF: [#%ld] Single shot-to-shot[0x%lX]: %.3g s\n",
                                    me->dSTS->dSingle.n + 1, me->dSTS->size_min, 1e-6 * TIME_DELTA(perf->time, me->dSTS->dSingle.last_timestamp));
                        }
                        delay_delta(&me->dSTS->dSingle, perf);
                    }
                }

                if (me->dSTS->dBurst.n >= 0)
                {
                    if (me->debug)
                    {
                        fprintf(me->fRt, "rtPERF: [#%ld] Raw burst shot-to-shot[0x%lX]: %.3g s\n",
                                me->dSTS->dBurst.n + 1, me->dSTS->size_min, 1e-6 * TIME_DELTA(perf->time, me->dSTS->dBurst.last_timestamp));
                    }
                    delay_add(&me->dSTS->dABurst, TIME_DELTA(perf->time, me->dSTS->dBurst.last_timestamp));
                }
                delay_delta(&me->dSTS->dBurst, perf);

                /* keep last captured image time stamp for single shot-to-shot */
                TIME_COPY(me->dSTS->dSingle.last_timestamp, perf->time);
                if (me->dSTS->dSingle.n < 0)
                {
                    me->dSTS->dSingle.n = 0;  /* captured first time stamp */
                }
            }
        }
    }

    /* ------------------------ UPTIME METRICS ------------------------ */
    if (0 && me->dUptime && me->dUptime->measuring)
    {
        /* see if we passed our granularity.  if yes, calculate uptime */
        int steps = TIME_DELTA(perf->time, me->dUptime->last_reporting);
        if (steps >= me->granularity) 
        {
            steps /= me->granularity;

            double uptime, idletime, load = 0;

            /* calculate MHz load */
            get_uptime(&uptime, &idletime);
            if (uptime > 0 && me->dUptime->success)
            {
                me->dUptime->last_idletime = idletime - me->dUptime->last_idletime;
                me->dUptime->last_uptime = uptime - me->dUptime->last_uptime;
                if (me->dUptime->last_uptime > 0)
                {
                    load = 100. * ((me->dUptime->last_uptime - me->dUptime->last_idletime) /
                                   me->dUptime->last_uptime);

                    me->dUptime->n += steps;
                    me->dUptime->x += load * steps;
                    me->dUptime->xx += load * load * steps;
                }
            }
           
            TIME_INCREASE(me->dUptime->last_reporting, steps * me->granularity);

            if (uptime > 0 && me->dUptime->success)
            {
                me->dUptime->last_uptime = uptime;
                me->dUptime->last_idletime = idletime;
                if (me->debug)
                {
                    fprintf(me->fRt, "rtPERF: [%ld] ARM CPU-load is %.3g%%\n",
                            TIME_DELTA(perf->time, me->first_time)/1000000,
                            load);
                }
            }
            else
            {
                me->dUptime->success = 0;
            }
        }
    }
}

void __rt_Command(PERF_Private *perf,
                  unsigned long ulCommand,
                  unsigned long ulArgument,
                  PERF_MODULETYPE eModule)
{
    /* get real-time private structure */
    /* PERF_RT_Private *me = perf->cip.pRT; */

    /* there is nothing to do at this point */
}

void __rt_Log(PERF_Private *perf,
              unsigned long ulData1, unsigned long ulData2,
              unsigned long ulData3)
{
    /* get real-time private structure */
    /* PERF_RT_Private *me = perf->cip.pRT; */

    /* there is nothing to do at this point */
}

void __rt_SyncAV(PERF_Private *perf,
                 float pfTimeAudio,
                 float pfTimeVideo,
                 PERF_SYNCOPTYPE eSyncOperation)
{
    /* get real-time private structure */
    /* PERF_RT_Private *me = perf->cip.pRT; */

    /* there is nothing to do at this point */
}

void __rt_ThreadCreated(PERF_Private *perf,
                        unsigned long ulThreadID,
                        unsigned long ulThreadName)
{
    /* get real-time private structure */
    /* PERF_RT_Private *me = perf->cip.pRT; */

    /* there is nothing to do at this point. Perhaps we can enable uptime if
       we still have not created it and it is an audio thread. */
}

void __rt_Done(PERF_Private *perf)
{
    /* get real-time private structure */
    PERF_RT_Private *me = perf->cip.pRT;

    /* print summaries if required */
    if (me->summary)
    {
        /* uptime summary */
        if (me->dUptime)
        {
            /* get last uptime */
            start_stop_uptime(me->dUptime, 0);

            fprintf(me->fRt, "rtPERF: ARM CPU-load for%s %c%c%c%c component: ",
                    me->needSteadyState ? " steady state of" : "",
                    PERF_FOUR_CHARS(perf->ulID));

            if (me->dUptime->success && me->dUptime->start_uptime)
            {
                double load = (100. * (me->dUptime->start_uptime -
                                      me->dUptime->start_idletime) /
                               me->dUptime->start_uptime);
                if (me->dUptime->n)
                {
                    double x = me->dUptime->x / me->dUptime->n;
                    double xx = me->dUptime->xx / me->dUptime->n;
                    xx = my_sqrt(xx - x * x);
                    if (me->debug & 2)
                    {
                        fprintf(me->fRt, ": %.3g +- %.3g%%\n"
                                "(temporal difference is: %.3g)\n",
                                load, xx, x - load);
                    }
                    else
                    {
                        fprintf(me->fRt, ": %.3g +- %.3g%%\n",
                                load, xx);
                    }
                }
                else
                {
                    fprintf(me->fRt, "%.3g%%\n", load);
                }
            }
            else
            {
                fprintf(me->fRt, "FAILED TO CALCULATE\n"); 
            }
        }

        /* rate summary */
        if (me->nDRate)
        {
            int i;
            for (i = 0; i < me->nDRate; i++)
            {
                if (me->dRate[i].n >= MIN_FRAMES_FOR_RATE &&
                    ((me->debug & 4) || me->dRate[i].tn0 >= MIN_FRAMES_FOR_RATE))
                {

                    double x = me->dRate[i].x * 1e-6 / me->dRate[i].n;

                    double s = (me->dRate[i].xx ?
                                (me->dRate[i].x * (double) me->dRate[i].x /
                                 me->dRate[i].xx / me->dRate[i].n) : 1);

                    fprintf(me->fRt, "rtPERF: ");
                    print_rate_info(me->fRt,
                                    perf->ulID, me->dRate[i].modulesAndFlags,
                                    me->dRate[i].size, me->dRate[i].n);
                    if (x > 0)
                    {
                        if (me->dRate[i].an)
                        {
                            double x2 = me->dRate[i].ax / me->dRate[i].an;
                            double xx = me->dRate[i].axx / me->dRate[i].an;
                            xx = my_sqrt(xx - x2 * x2);
                            if (me->debug & 2)
                            {
                                fprintf(me->fRt, ": %.3g +- %.3g fps (s=%.3g)\n"
                                        "(temporal difference is: %.3g)\n",
                                        1/x, xx, s, x2-1/x);
                            }
                            else
                            {
                                fprintf(me->fRt, ": %.3g +- %.3g fps (s=%.3g)\n",
                                        1/x, xx, s);
                            }
                        }
                        else
                        {
                            fprintf(me->fRt, ": %.3g fps (s=%.3g)\n", 1/x, s);
                        }
                    }
                    else
                    {
                        fprintf(me->fRt, ": FAILED TO CALCULATE\n");
                    }
                }
            }
        }

        /* shot-to-shot summary */
        if (me->dSTS)
        {
            if (me->dSTS->dABurst2.n > 0)
            {
                count_delay(me, "Avg. modified burst shot-to-shot", &me->dSTS->dABurst2, 0);
            }
            if (me->dSTS->dABurst.n > 0)
            {
                count_delay(me, "Avg. raw burst shot-to-shot", &me->dSTS->dABurst, 0);            
            }
            if (me->dSTS->dSingle.n > 0)
            {
                count_delay(me, "Avg. single shot-to-shot", &me->dSTS->dSingle, -1);            
            }
        }
    }
}


#endif
