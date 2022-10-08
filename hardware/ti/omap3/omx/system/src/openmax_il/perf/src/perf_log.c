
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
#include "perf.h"
#include "perf_config.h"

#define PERF_MAX_LOG_LENGTH (sizeof(unsigned long) * 8)

/* ============================================================================
   PERF LOG Methods
============================================================================ */

/* Effects: flush log */
void __PERF_LOG_flush(PERF_LOG_Private *me)
{
    /* only flush if we collected data */
    if (me->puPtr > me->puBuffer)
    {
		/* open file if we have not yet opened it */
		if (!me->fOut) me->fOut = fopen(me->fOutFile, "wb");
		
		if (me->fOut)
		{
			fwrite(me->puBuffer, me->puPtr - me->puBuffer, sizeof(*me->puPtr),
				   me->fOut);
			me->uBufferCount++;
		}
		
		/* reset pointer to start of buffer */
		me->puPtr = me->puBuffer;
    }
}

/** The Done method is called at the end of the UC test or UI
*   application.
*   @param phObject
*       Pointer to a handle to the PERF object, which will be
*       deleted and set to NULL upon completion.
*  */

void __PERF_LOG_done(PERF_Private *perf)
{
    PERF_LOG_Private *me = perf->pLog;

    if (me)
    {
        /* if we could allocate a buffer, we can log the completion */
        if (me->puBuffer && me->fOutFile)
        {
            __PERF_log1(perf, PERF_LOG_Done);

            __PERF_LOG_flush(me);   /* flush log */
        }

        /* free buffer */
        if (me->puBuffer) free(me->puBuffer);
        me->puBuffer = NULL;

        /* free file name string */
        if (me->fOutFile) free(me->fOutFile);
        me->fOutFile = NULL;

        /* close file */
        if (me->fOut) fclose(me->fOut);
        me->fOut = NULL;

        fprintf(stderr,
                "PERF Instrumentation [%c%c%c%c %05ld-%08lx] produced"
                " %ld buffers\n",
                PERF_FOUR_CHARS(perf->ulID), perf->ulPID,
                (unsigned long) perf,
                me->uBufferCount);

        /* delete LOG private structure */
        free(me);
        perf->pLog = NULL;
    }
}

/* Effects: creates LOG private object and logs initial block of information */
PERF_LOG_Private *__PERF_LOG_create(PERF_Private *perf,
                                    PERF_Config *config,
                                    PERF_MODULETYPE eModule)
{
    PERF_LOG_Private *me =
        perf->pLog = (PERF_LOG_Private *) malloc (sizeof (PERF_LOG_Private));

    if (me)
    {
        me->fOut = NULL;
        me->uBufferCount = 0;
        me->uBufSize = config->buffer_size;

        /* limit buffer size to allow at least one log creation */
        if (me->uBufSize < PERF_MAX_LOG_LENGTH)
        {
            me->uBufSize = PERF_MAX_LOG_LENGTH;
        }

        me->puBuffer =
        (unsigned long *) malloc (sizeof (unsigned long) * me->uBufSize);
        me->fOutFile = (char *) malloc (strlen(config->trace_file) + 34);
        if (me->puBuffer && me->fOutFile)
        {
            perf->uMode |= PERF_Mode_Log; /* we are logging */

            me->puPtr = me->puBuffer;   /* start at beginning of buffer */
            me->puEnd = me->puBuffer + me->uBufSize - PERF_MAX_LOG_LENGTH;

            sprintf(me->fOutFile, "%s-%05lu-%08lx-%c%c%c%c.trace",
                    config->trace_file, perf->ulPID, (unsigned long) perf,
                    PERF_FOUR_CHARS(perf->ulID));

            /* for delayed open we don't try to open the file until we need to
               save a buffer into it */
            if (!config->delayed_open)
            {
                me->fOut = fopen(me->fOutFile, "ab");				
            }
            else
            {
                me->fOut = NULL;
            }
            /* save initial data to the log */

            *me->puPtr++ = (unsigned long) eModule; /* module ID */
            *me->puPtr++ = perf->ulID;  /* ID */
            *me->puPtr++ = perf->ulPID; /* process ID */

            /* original tempTime stamp */
            *me->puPtr++ = TIME_SECONDS(perf->time);
            *me->puPtr++ = TIME_MICROSECONDS(perf->time);
        }

		/* if some allocation or opening failed, delete object */
		if (!me->puBuffer || !me->fOutFile || (!config->delayed_open && !me->fOut))
        {
			perf->uMode &= ~PERF_Mode_Log; /* delete logging flag */
            __PERF_LOG_done(perf);
        }
    }

    /* it may have been deleted already, so we read it again */
    return(perf->pLog);
}

/* Effects: appends tempTime stamp to log */
void __PERF_LOG_log_common(PERF_Private *perf, unsigned long *time_loc)
{
    unsigned long delta = 0;
    PERF_LOG_Private *me = perf->pLog; /* get LOG private object */

    /* get tempTime of from last tempTime stamp */
    TIME_GET(perf->tempTime);
    delta = TIME_DELTA(perf->tempTime, perf->time);

    *time_loc = delta;   /* save time stamp */

    /* save tempTime stamp as last tempTime stamp */
    TIME_COPY(perf->time, perf->tempTime);

    /* flush if we reached end of the buffer */
    if (me->puPtr > me->puEnd) __PERF_LOG_flush(me);
}

