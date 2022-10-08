
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
#ifdef __PERF_READER__

#define __DECODE(c) (((c) < 0 || (c) > 63) ? '#' : ("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ123456789./_" [c]))

#include "perf.h"
/*=============================================================================
    DEFINITIONS   
=============================================================================*/

    /* minimum and maximum time-correction handled */
    #define MIN_DELTA 1U
    #define MAX_DELTA 4292967295U


/** __PERF_CUSTOMIZABLE__ must be enabled, as we are using the
 *  standard PERF module for printing */
    #ifndef __PERF_CUSTOMIZABLE__
        #error "Must define __PERF_CUSTOMIZABLE__ to enable printing for reader"
    #else

        #include "perf_config.h"
        #include "perf.h"

        #include <errno.h>

typedef unsigned long U32;

/*=============================================================================
    GLOBALS    
=============================================================================*/

static U32 read_U32(FILE *fLog)
{
    U32 data;
    fread(&data, sizeof(U32), 1, fLog);
    return(data);
}

PERF_OBJHANDLE __PERF_common_Create(struct PERF_Config *config,
                                    unsigned long ulID,
                                    PERF_MODULETYPE eModule);
void __PERF_CUSTOM_create(PERF_OBJHANDLE hObject,
                          struct PERF_Config *config,
                          PERF_MODULETYPE eModule);

void PERF_Replay(FILE *fLog, PERF_Config *pConfig)
{
    U32 ulData0, ulData1, ulData2, ulData3, ulData4, ulData5, ulData6, ulData7, operation;
    char szFile[21], szFunc[21];
    U32 sending, multiple, frame, size;
    PERF_OBJHANDLE hObject = NULL;
    PERF_Private *me = NULL;
    long time_correction = 0;
    union __PERF_float_long uA, uV;

    /* logging is disabled during replay because the __log_ API-s get the
       time on their own, and are not feasible to be modified to work with
       replayed times */
    if (pConfig->trace_file)
    {
        free(pConfig->trace_file);
        pConfig->trace_file = NULL;
    }

    /* read initialization info */

    /* we support having multiple log files concatenated into one log file */
    /* read through each log file */
    /* we have to pre-read to detect end of file */
    while ((ulData0 = read_U32(fLog)), !feof(fLog))
    {

        /* if there is no object, create one */
        if (!hObject)
        {
            /* create PERF replay object */
            /* pre-read word is the eModuleType */
            ulData1 = read_U32(fLog);    /* ID */

            hObject = __PERF_common_Create(pConfig, ulData1, ulData0);
            if (!hObject)
            {
                fprintf(stderr, "error: could not create PERF replay object\n");
                exit(1);
            }

            me = get_Private(hObject);

            /* set up initial state */
            me->ulPID = read_U32(fLog);  /* PID */
            ulData1 = read_U32(fLog);    /* startTime.sec */
            ulData2 = read_U32(fLog);    /* startTime.usec */
            TIME_SET(me->time, ulData1, ulData2);
            time_correction = 0;

            /* continue setting up the PERF object */
            __PERF_CUSTOM_create(hObject, pConfig, ulData0);
            if (me->uMode == PERF_Mode_Replay)
            {
                fprintf(stderr, "Only replay mode is selected.  Aborting...\n");
                PERF_Done(hObject);
            }
        }
        else
        {
            /* pre-read word is replay time difference, except for PERF_LOG_Location logs */
            /* get operation */
            ulData1 = read_U32(fLog);
            operation = ulData1 & PERF_LOG_Mask;

            if (operation != PERF_LOG_Location)
            {
                /* invariant: time_replayed = time_logged + time_correction */
    
                /* if a negative or too-small time-stamp is encountered */
                if (ulData0 > MAX_DELTA || ulData0 < MIN_DELTA ||
                    /* or if we cannot completely correct a prior time correction */
                    (time_correction && ulData0 < MIN_DELTA + (U32) (-time_correction)))
                {   /* store the time difference than cannot be replayed in the
                       time_correction variable, and replay a MIN_DELTA time
                       difference */
                    time_correction += (long) ulData0 - (long) MIN_DELTA;
                    ulData0 = MIN_DELTA;
                }
                else if (time_correction)
                {
                    ulData0 = ulData0 + (U32) time_correction;
                    time_correction = 0;
                }
                TIME_INCREASE(me->time, ulData0);
                ulData0 = ulData1;
            }

            /* Check for buffer operations */
            if (operation & PERF_LOG_Buffer)
            {
                /* Buffer operation */
                if (operation & PERF_LOG_Xfering) {
                    sending = PERF_FlagXfering;
                    size = PERF_bits(ulData0, 2 * PERF_ModuleBits,
                                     30 - 2 * PERF_ModuleBits) << 3;
                } else {
                    sending = operation & PERF_LOG_Sending;
                    size = PERF_bits(ulData0, PERF_ModuleBits,
                                     28 - PERF_ModuleBits);
                    ulData0 &= PERF_ModuleMask;
                }

                /* read address */
                ulData1 = read_U32(fLog);
                multiple = (ulData1 & PERF_LOG_Multiple) ? PERF_FlagMultiple : PERF_FlagSingle;
                frame    = (ulData1 & PERF_LOG_Frame)    ? PERF_FlagFrame    : PERF_FlagBuffer;

                /* read 2nd address if logged multiple buffers */
                ulData2 = PERF_IsMultiple(multiple) ? read_U32(fLog) : 0;

                __PERF_CUSTOM_Buffer(hObject,
                                     sending,
                                     multiple,
                                     frame,
                                     ulData1 & ~3,
                                     ulData2,
                                     size,
                                     PERF_bits(ulData0, 0, PERF_ModuleBits),
                                     PERF_bits(ulData0, PERF_ModuleBits, PERF_ModuleBits) );
            }
            /* Check for command operations */
            else if (operation & PERF_LOG_Command)
            {
                ulData1 = read_U32(fLog);
                ulData2 = read_U32(fLog);
                __PERF_CUSTOM_Command(hObject,
                                      operation & PERF_LOG_Sending,
                                      ulData1,
                                      ulData2,
                                      ulData0 & PERF_LOG_NotMask);
            }
            else switch (operation)
            {
                /* Log operation */
            case PERF_LOG_Log:
                ulData1 = read_U32(fLog);
                ulData2 = read_U32(fLog);

                __PERF_CUSTOM_Log(hObject,
                                  ulData0 & PERF_LOG_NotMask,
                                  ulData1,
                                  ulData2);
                break;

                /* SyncAV operation */
            case PERF_LOG_Sync:
                uA.l = read_U32(fLog);
                uV.l = read_U32(fLog);

                __PERF_CUSTOM_SyncAV(hObject, uA.f, uV.f,
                                     ulData0 & PERF_LOG_NotMask);
                break;

            case PERF_LOG_Done:
                /* This can be also PERF_Thread, PERF_Boundary */
                operation = ulData0 & PERF_LOG_Mask2;
                switch (operation)
                {
                    /* Thread Creation operation */
                case PERF_LOG_Thread:
                    ulData1 = read_U32(fLog);
    
                    __PERF_CUSTOM_ThreadCreated(hObject,
                                                ulData0 & PERF_LOG_NotMask2,
                                                ulData1);
                    break;

                    /* Boundary operation */
                case PERF_LOG_Boundary:
                    __PERF_CUSTOM_Boundary(hObject,
                                           ulData0 & PERF_LOG_NotMask2);
                    break;

                case PERF_LOG_Done:
                    __PERF_Done(hObject);

                    break;
                }
                break;

                /* location log */
            case PERF_LOG_Location:
                ulData2 = read_U32(fLog);
                ulData3 = read_U32(fLog);
                ulData4 = read_U32(fLog);
                ulData5 = read_U32(fLog);
                ulData6 = read_U32(fLog);
                ulData7 = read_U32(fLog);

                /* decode szFile */
                szFile[19] = __DECODE(ulData2 & 0x3f);
                szFile[18] = __DECODE((ulData2 >> 6) & 0x3f);
                szFile[17] = __DECODE((ulData2 >> 12) & 0x3f);
                szFile[16] = __DECODE((ulData2 >> 18) & 0x3f);
                szFile[15] = __DECODE((ulData2 >> 24) & 0x3f);
                szFile[14] = __DECODE(((ulData2 >> 26) & 0x30) | ((ulData0 >> 24) & 0x0f));
                szFile[13] = __DECODE(ulData3 & 0x3f);
                szFile[12] = __DECODE((ulData3 >> 6) & 0x3f);
                szFile[11] = __DECODE((ulData3 >> 12) & 0x3f);
                szFile[10] = __DECODE((ulData3 >> 18) & 0x3f);
                szFile[9] = __DECODE((ulData3 >> 24) & 0x3f);
                szFile[8] = __DECODE(((ulData3 >> 26) & 0x30) | ((ulData0 >> 28) & 0x0f));
                szFile[7] = __DECODE(ulData4 & 0x3f);
                szFile[6] = __DECODE((ulData4 >> 6) & 0x3f);
                szFile[5] = __DECODE((ulData4 >> 12) & 0x3f);
                szFile[4] = __DECODE((ulData4 >> 18) & 0x3f);
                szFile[3] = __DECODE((ulData4 >> 24) & 0x3f);
                szFile[2] = __DECODE(((ulData4 >> 26) & 0x30) | (ulData1 & 0x0f));
                szFile[1] = __DECODE(ulData0 & 0x3f);
                szFile[0] = __DECODE((ulData0 >> 6) & 0x3f);
                szFile[20] = '\0';

                szFunc[19] = __DECODE(ulData5 & 0x3f);
                szFunc[18] = __DECODE((ulData5 >> 6) & 0x3f);
                szFunc[17] = __DECODE((ulData5 >> 12) & 0x3f);
                szFunc[16] = __DECODE((ulData5 >> 18) & 0x3f);
                szFunc[15] = __DECODE((ulData5 >> 24) & 0x3f);
                szFunc[14] = __DECODE(((ulData5 >> 26) & 0x30) | ((ulData1 >> 4) & 0x0f));
                szFunc[13] = __DECODE(ulData6 & 0x3f);
                szFunc[12] = __DECODE((ulData6 >> 6) & 0x3f);
                szFunc[11] = __DECODE((ulData6 >> 12) & 0x3f);
                szFunc[10] = __DECODE((ulData6 >> 18) & 0x3f);
                szFunc[9] = __DECODE((ulData6 >> 24) & 0x3f);
                szFunc[8] = __DECODE(((ulData6 >> 26) & 0x30) | ((ulData1 >> 8) & 0x0f));
                szFunc[7] = __DECODE(ulData7 & 0x3f);
                szFunc[6] = __DECODE((ulData7 >> 6) & 0x3f);
                szFunc[5] = __DECODE((ulData7 >> 12) & 0x3f);
                szFunc[4] = __DECODE((ulData7 >> 18) & 0x3f);
                szFunc[3] = __DECODE((ulData7 >> 24) & 0x3f);
                szFunc[2] = __DECODE(((ulData7 >> 26) & 0x30) | ((ulData1 >> 12) & 0x0f));
                szFunc[1] = __DECODE((ulData0 >> 12) & 0x3f);
                szFunc[0] = __DECODE((ulData0 >> 18) & 0x3f);
                szFunc[20] = '\0';

                /* skip leading /-s */
                for (ulData2 = 0; szFile[ulData2] == '/'; ulData2++);
                for (ulData3 = 0; szFunc[ulData3] == '/'; ulData3++);

                ulData1 = (ulData1 >> 16) & 0xfff;                
                __PERF_CUSTOM_Location(hObject,szFile + ulData2, ulData1,
                                       szFunc + ulData3);

                break;

            default:
                fprintf(stderr, "Unknown operation recorded: %lx\n", ulData0);
                exit(1);
                break;
            }
        }
    }

    if (hObject)
    {
        fprintf(stderr, "Incomplete log ended...\n");
        PERF_Done(hObject);
    }
}

int main(int argc, char **argv)
{
    int i;
    FILE *log = NULL;
    PERF_Config config;


    for (i = 1; i < argc; i++)
    {
        /* replay file */

        /* open input, or stdin if '-' is specified */
        log = strcmp(argv [i], "-") ? fopen(argv [i], "rb") : stdin;

        if (log)
        {
            /* read config file */
            PERF_Config_Init(&config);
            PERF_Config_Read(&config, "replay");
            config.mask = 0xFFFFFFFF;

            /* note config gets modified during Replay */
            PERF_Replay(log, &config);

            PERF_Config_Release(&config);

            /* don't close stdin! */
            if (log != stdin) fclose(log);
        }
        else
        {
            fprintf(stderr, "Could not open log file %s: %d\n",
                    argv [i], errno);
        }
    }

    return (0);
}

    #endif  /* __PERF_CUSTOMIZABLE__ */

#endif  /* __PERF_READER__ */

