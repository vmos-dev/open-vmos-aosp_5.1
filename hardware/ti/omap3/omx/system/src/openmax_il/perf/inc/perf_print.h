
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
#ifndef __PERF_PRINT_H
#define __PERF_PRINT_H

#include "stdio.h"
#include "perf_obj.h"
/* ============================================================================
   DEBUG STRUCTURES
============================================================================ */
typedef struct PERF_PRINT_Private
{
    char *info;                    /* PERF instance information text */
    char *prompt;                  /* PERF prompt */
    FILE *fDebug;                  /* file to event debugs */
    FILE *fPrint;                  /* file to event prints (Buffer) */
    /* NOTE: fPrint == fDebug or fPrint == NULL */
    int   csv;                     /* CSV format */
#ifdef __PERF_LOG_LOCATION__
    /* location information to be printed */
    const char *szFile;
    const char *szFunc;
    unsigned long ulLine;
#endif
} PERF_PRINT_Private;

void
PERF_PRINT_done(PERF_Private *perf);

PERF_PRINT_Private *
PERF_PRINT_create(PERF_Private *perf, PERF_Config *config,
                  PERF_MODULETYPE eModule);

void
__print_Boundary(FILE *fOut,
                 PERF_Private *perf,PERF_BOUNDARYTYPE eBoundary);

void
__print_Buffer(FILE *fOut,
               PERF_Private *perf,unsigned long ulAddress1,
               unsigned long ulAddress2,
               unsigned long ulSize,
               PERF_MODULETYPE eModule);

void
__print_Command(FILE *fOut,
                PERF_Private *perf,
                unsigned long ulCommand,
                unsigned long ulArgument,
                PERF_MODULETYPE eModule);

void
__print_Create(FILE *fOut,
               PERF_Private *perf);

void
__print_Done(FILE *fOut,
             PERF_Private *perf);

void
__print_Log(FILE *fOut,
            PERF_Private *perf,
            unsigned long ulData1, unsigned long ulData2,
            unsigned long ulData3);

void
__print_SyncAV(FILE *fOut,
               PERF_Private *perf,
               float pfTimeAudio,
               float pfTimeVideo,
               PERF_SYNCOPTYPE eSyncOperation);

void
__print_ThreadCreated(FILE *fOut,
                      PERF_Private *perf,
                      unsigned long ulThreadID,
                      unsigned long ulThreadName);

#ifdef __PERF_LOG_LOCATION__
void
__print_Location(PERF_Private *perf,
                 char const *szFile,
                 unsigned long ulLine,
                 char const *szFunc);
#endif

#endif

