
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
#define __PERF_C__

#include "perf_config.h"
#include "perf.h"

/* used internal function declarations */
PERF_LOG_Private *
__PERF_LOG_create(PERF_Private *perf, PERF_Config *config,
                  PERF_MODULETYPE eModule);

void
__PERF_LOG_done(PERF_Private *perf);

/* get custom implementation */
#ifdef __PERF_CUSTOMIZABLE__
    #include "perf_custom.c"
#endif

/*=============================================================================
    INSTRUMENTATION INTERFACE
=============================================================================*/

static
void
__common_Done(PERF_OBJHANDLE *phObject)
{
    PERF_OBJHANDLE hObject = *phObject;
    PERF_Private *me  = get_Private(hObject);

    /* we may not have allocated the private structure */
    if (me)
    {
#ifdef __PERF_CUSTOMIZABLE__
        __PERF_CUSTOM_done(me);
#else
        if (me->uMode & PERF_Mode_Log)
        {   /* close log */
            __PERF_LOG_done(me);
        }
#endif

        /* delete private structure */
        free (me);
        (*phObject)->pComponentPrivate = NULL;
    }

    /* delete PERF opject */
    free (*phObject);

    /* invalidate handle */
    *phObject = NULL;
}

PERF_OBJHANDLE
__PERF_common_Create(PERF_Config *config,
                     unsigned long ulID,
                     PERF_MODULETYPE eModule)
{
    PERF_OBJHANDLE hPERF = NULL;
    PERF_Private *me = NULL;

    if ((config->mask & eModule & ~PERF_ModuleMask) &&
        (config->mask & (1 << (eModule & PERF_ModuleMask))))
    {
        /* allocate object */
        hPERF = (PERF_OBJHANDLE) malloc(sizeof(PERF_OBJTYPE));

        /* set up methods */
        if (hPERF != NULL)
        {
            hPERF->pApplicationPrivate = NULL;
            hPERF->Done = __common_Done;
            hPERF->pComponentPrivate = 
                me = (PERF_Private *) malloc(sizeof(PERF_Private));

            if (me)
            {
                /* no flags are selected, and capture creation time */
                me->uMode = PERF_Mode_None;
                me->ulID  = ulID;
                me->pLog  = NULL;
                PID_GET(me->ulPID);

                /* save original time stamp */
                TIME_GET(me->time);
                TIME_COPY(me->tempTime, me->time);

                /* create LOG private structures a log file is specified */
                if (config->trace_file)
                {
                    __PERF_LOG_create(me, config, eModule);
                }

#ifdef __PERF_CUSTOMIZABLE__
                __PERF_CUSTOM_create(hPERF, config, eModule);
#endif
            }

            /* if we could not create any logging object (no flag is enabled) */
            if (!me || me->uMode == PERF_Mode_None)
            {   /* free up object */
                __common_Done(&hPERF);
            }
        }
    }

    return(hPERF);
}

PERF_OBJHANDLE
PERF_Create(unsigned long ulID, PERF_MODULETYPE eModule)
{
    PERF_OBJTYPE *hPERF = NULL;  /* PERF object */
    char tag[5] = { PERF_FOUR_CHARS(ulID), 0 };
    int i;

    /* replace spaces in ID with _ */
    for (i=0; i<4; i++) if (tag[i] == ' ') tag[i] = '_';
    ulID = PERF_FOURS(tag);

    /* read config file */
    PERF_Config config;
    PERF_Config_Init(&config);
    PERF_Config_Read(&config, tag);

    /* remove replay file if any */
    if (config.replay_file)
    {
        free(config.replay_file);
        config.replay_file = NULL;
    }

    /* create for capturing */
    hPERF = __PERF_common_Create(&config, ulID, eModule);

    PERF_Config_Release(&config);
    return(hPERF);
}

