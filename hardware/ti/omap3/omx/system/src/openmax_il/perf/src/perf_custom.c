
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
/* NOTE: This source file should be only included from perf.c */

/*=============================================================================
    CUSTOMIZABLE INTERFACES
=============================================================================*/

#include "perf_config.h"
#include "perf_print.h"
#include "perf_rt.h"

void __PERF_CUSTOM_setup_for_log_only(PERF_OBJHANDLE hObject);
void __PERF_CUSTOM_setup_common(PERF_OBJHANDLE hObject);

/*=============================================================================
    time stamp methods
=============================================================================*/

INLINEORSTATIC
void get_time(PERF_Private *me)
{
    /* get replay tempTime if replaying */
    if (me->uMode & PERF_Mode_Replay) return;

    /* otherwise, get time of the day */
    TIME_GET(me->time);
}

/** Customizable PERF interfaces for logging only.  These
 *  correspond -- and are wrappers -- to the __PERF macros */
static
void __log_Boundary(PERF_OBJHANDLE hObject,
                    PERF_BOUNDARYTYPE eBoundary)
{
    __PERF_LOG_Boundary(hObject, eBoundary);
}

static
void __log_Buffer(PERF_OBJHANDLE hObject,
                  unsigned long ulAddress1,
                  unsigned long ulAddress2,
                  unsigned long ulSize,
                  unsigned long ulModuleAndFlags)
{
    __PERF_LOG_Buffer(hObject,
                      PERF_GetXferSendRecv(ulModuleAndFlags),
                      ulModuleAndFlags & PERF_FlagMultiple,
                      ulModuleAndFlags & PERF_FlagFrame,
                      ulAddress1,
                      ulAddress2,
                      ulSize,
                      ulModuleAndFlags & PERF_ModuleMask,
					  (ulModuleAndFlags >> PERF_ModuleBits) & PERF_ModuleMask);
}

static
void __log_Command(PERF_OBJHANDLE hObject,
                   unsigned long ulCommand,
				   unsigned long ulArgument,
                   unsigned long ulModuleAndFlags)
{
    __PERF_LOG_Command(hObject,
                       PERF_GetSendRecv(ulModuleAndFlags),
                       ulCommand,
					   ulArgument,
                       ulModuleAndFlags & PERF_ModuleMask);
}

static
void __log_Log(PERF_OBJHANDLE hObject,
               unsigned long ulData1,
               unsigned long ulData2,
               unsigned long ulData3)
{
    __PERF_LOG_Log(hObject, ulData1, ulData2, ulData3);
}

static
void __log_SyncAV(PERF_OBJHANDLE hObject,
                  float fTimeAudio,
                  float fTimeVideo,
                  PERF_SYNCOPTYPE eSyncOperation)
{
    __PERF_LOG_SyncAV(hObject, fTimeAudio, fTimeVideo, eSyncOperation);
}

static
void __log_ThreadCreated(PERF_OBJHANDLE hObject,
                         unsigned long ulThreadID,
                         unsigned long ulThreadName)
{
    __PERF_LOG_ThreadCreated(hObject, ulThreadID, ulThreadName);
}

/** Customizable PERF interfaces for all operations other than
 *  logging only. These correspond to the __PERF macros */

static
void __common_Boundary(PERF_OBJHANDLE hObject,
                       PERF_BOUNDARYTYPE eBoundary)
{
    PERF_Private *me  = get_Private(hObject);

    if (me->pLog)
    {   /* perform log specific operations */
        __log_Boundary(hObject, eBoundary);
    }
    else
    {   /* we need to get the time stamp to print */
        get_time(me);
    }

    if (me->cip.pDebug)
    {
        __print_Boundary(me->cip.pDebug->fDebug,
                         me, eBoundary);
    }

    if (me->cip.pRT)
    {
        __rt_Boundary(me, eBoundary);
    }
}

static
void __common_Buffer(PERF_OBJHANDLE hObject,
                     unsigned long ulAddress1,
                     unsigned long ulAddress2,
                     unsigned long ulSize,
                     unsigned long ulModuleAndFlags)
{
    PERF_Private *me  = get_Private(hObject);

    if (me->pLog)
    {   /* perform log specific operations */
        __log_Buffer(hObject, ulAddress1, ulAddress2, ulSize, ulModuleAndFlags);
    }
    else
    {   /* we need to get the time stamp to print */
        get_time(me);
    }

    if (me->cip.pDebug && me->cip.pDebug->fPrint)
    {
        __print_Buffer(me->cip.pDebug->fPrint,
                       me, ulAddress1, ulAddress2, ulSize, ulModuleAndFlags);
    }

    if (me->cip.pRT)
    {
        __rt_Buffer(me, ulAddress1, ulAddress2, ulSize, ulModuleAndFlags);
    }
}

static
void __common_Command(PERF_OBJHANDLE hObject,
                      unsigned long ulCommand,
					  unsigned long ulArgument,
                      unsigned long ulModuleAndFlags)
{
    PERF_Private *me  = get_Private(hObject);

    if (me->pLog)
    {   /* perform log specific operations */
        __log_Command(hObject, ulCommand, ulArgument, ulModuleAndFlags);
    }
    else
    {   /* we need to get the time stamp to print */
        get_time(me);
    }

    if (me->cip.pDebug && me->cip.pDebug->fDebug)
    {
        __print_Command(me->cip.pDebug->fDebug,
                        me, ulCommand, ulArgument, ulModuleAndFlags);
    }

    if (me->cip.pRT)
    {
        __rt_Command(me, ulCommand, ulArgument, ulModuleAndFlags);
    }
}

static
void __common_Log(PERF_OBJHANDLE hObject,
                  unsigned long ulData1,
                  unsigned long ulData2,
                  unsigned long ulData3)
{
    PERF_Private *me  = get_Private(hObject);

    if (me->pLog)
    {   /* perform log specific operations */
        __log_Log(hObject, ulData1, ulData2, ulData3);
    }
    else
    {   /* we need to get the time stamp to print */
        get_time(me);
    }

    if (me->cip.pDebug && me->cip.pDebug->fDebug)
    {
        __print_Log(me->cip.pDebug->fDebug,
                    me, ulData1, ulData2, ulData3);
    }

    if (me->cip.pRT)
    {
        __rt_Log(me, ulData1, ulData2, ulData3);
    }
}

static
void __common_SyncAV(PERF_OBJHANDLE hObject,
                     float pfTimeAudio,
                     float pfTimeVideo,
                     PERF_SYNCOPTYPE eSyncOperation)
{
    PERF_Private *me  = get_Private(hObject);

    if (me->pLog)
    {   /* perform log specific operations */
        __log_SyncAV(hObject, pfTimeAudio, pfTimeVideo, eSyncOperation);
    }
    else
    {   /* we need to get the time stamp to print */
        get_time(me);
    }

    if (me->cip.pDebug && me->cip.pDebug->fDebug)
    {
        __print_SyncAV(me->cip.pDebug->fDebug,
                       me, pfTimeAudio, pfTimeVideo, eSyncOperation);
    }

    if (me->cip.pRT)
    {
        __rt_SyncAV(me, pfTimeAudio, pfTimeVideo, eSyncOperation);
    }
}

static
void __common_ThreadCreated(PERF_OBJHANDLE hObject,
                            unsigned long ulThreadID,
                            unsigned long ulThreadName)
{
    PERF_Private *me  = get_Private(hObject);

    if (me->pLog)
    {   /* perform log specific operations */
        __log_ThreadCreated(hObject, ulThreadID, ulThreadName);
    }
    else
    {   /* we need to get the time stamp to print */
        get_time(me);
    }

    if (me->cip.pDebug && me->cip.pDebug->fDebug)
    {
        __print_ThreadCreated(me->cip.pDebug->fDebug,
                              me, ulThreadID, ulThreadName);
    }

    if (me->cip.pRT)
    {
        __rt_ThreadCreated(me, ulThreadID, ulThreadName);
    }
}

#ifdef __PERF_LOG_LOCATION__
static
void __log_Location(PERF_OBJHANDLE hObject,
                   char const *szFile,
				   unsigned long ulLine,
                   char const *szFunc)
{
    __PERF_LOG_Location(hObject,
                        szFile,
                        ulLine,
                        szFunc);
}

static
void __common_Location(PERF_OBJHANDLE hObject,
                            char const *szFile,
				   unsigned long ulLine,
                   char const *szFunc)
{
    PERF_Private *me  = get_Private(hObject);

    if (me->pLog)
    {   /* perform log specific operations */
        __log_Location(hObject, szFile, ulLine, szFunc);
    }

    if (me->cip.pDebug && me->cip.pDebug->fDebug)
    {
        __print_Location(me, szFile, ulLine, szFunc);
    }
}
#endif


void
__PERF_CUSTOM_setup_common(PERF_OBJHANDLE hObject)
{
    hObject->ci.Boundary      = __common_Boundary;
    hObject->ci.Buffer        = __common_Buffer;
    hObject->ci.Command       = __common_Command;
    hObject->ci.Log           = __common_Log;
    hObject->ci.SyncAV        = __common_SyncAV;
    hObject->ci.ThreadCreated = __common_ThreadCreated;
#ifdef __PERF_LOG_LOCATION__
    hObject->ci.Location      = __common_Location;
#endif
}

void
__PERF_CUSTOM_setup_for_log_only(PERF_OBJHANDLE hObject)
{
    hObject->ci.Boundary      = __log_Boundary;
    hObject->ci.Buffer        = __log_Buffer;
    hObject->ci.Command       = __log_Command;
    hObject->ci.Log           = __log_Log;
    hObject->ci.SyncAV        = __log_SyncAV;
    hObject->ci.ThreadCreated = __log_ThreadCreated;
#ifdef __PERF_LOG_LOCATION__
    hObject->ci.Location      = __log_Location;
#endif
}

void
__PERF_CUSTOM_create(PERF_OBJHANDLE hObject, PERF_Config *config,
                     PERF_MODULETYPE eModule)
{
    PERF_Private *me = get_Private(hObject);

    /* initialize custom fields of the PERF_Private */
    me->cip.pDebug = NULL;
    me->cip.pRT = NULL;

    /* add replay flag and set log file to the replay file so that replayed
       prints are saved into the log (replay) file */
    if (config->replay_file)
    {
        me->uMode |= PERF_Mode_Replay;

        if (config->log_file) free(config->log_file);
        config->log_file    = config->replay_file;
        config->replay_file = NULL;

        /** we need to get time stamps set up before we continue
            at the 2nd call, replay_file is already NULL so we will
            proceed */
        return;
    }

    /* handle correct output (debug and debug log) */
    if (config->log_file || config->debug || config->detailed_debug)
    {
        /* check if we could create the print structure */
        if (!PERF_PRINT_create(me, config, eModule))
        {
            /* if not, delete replay flag */
            me->uMode &= ~PERF_Mode_Replay;
        }
    }

    /* handle correct output (debug and debug log) */
    if (config->realtime)
    {
        /* check if we could create the print structure */
        PERF_RT_create(me, config, eModule);
    }

    /* THIS has to be done at the end:
       if we are only logging, we set up the function table to the log shortcut
       interface for speed.  Otherwise, we set them to the common interface */
    if (me->uMode == PERF_Mode_Log)
    {
        __PERF_CUSTOM_setup_for_log_only(hObject);
    }
    else
    {
        __PERF_CUSTOM_setup_common(hObject);
    }
}

void __PERF_CUSTOM_done(PERF_Private *me)
{
    if (me->uMode & PERF_Mode_Log)
    {   /* close log */
        __PERF_LOG_done(me);
    }
    else
    {
        get_time(me);
    }

    /* Complete any debug structures */
    if (me->cip.pDebug)
    {
		if (me->cip.pDebug->fDebug) __print_Done(me->cip.pDebug->fDebug, me);
        PERF_PRINT_done(me);
    }

    if (me->cip.pRT)
    {
        __rt_Done(me);
        PERF_RT_done(me);
    }
}

