
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
/* NOTE: This header should be only included from perf.h */

/* This header defined the macros that translate the external interface
   into the implementation calls */

#ifndef __PERF_OBJ_H
#define __PERF_OBJ_H

#include "perf_common.h"
#include "perf.h"

#ifdef __PERF_CUSTOMIZABLE__
/* define any customizable interface */
/* this header includes any customized implementation headers */
    #include "perf_custom.h"
#endif

/******************************************************************************
    PRIVATE (INTERNAL) STRUCTURES - NEEDED FOR INLINE LOG IMPLEMENTATIONS
******************************************************************************/

/* private PERF structure - common to all implementations */
typedef struct PERF_Private
{
    struct PERF_LOG_Private *pLog;
    TIME_STRUCT time, tempTime;    /* last and current timestamp */

    unsigned long uMode;           /* PERF instrumentation mode */
    unsigned long ulPID;           /* PID for thread */
    unsigned long ulID;            /* Performance object name (FOURCC) */

#ifdef __PERF_CUSTOMIZABLE__
    /* additional information for each kind of implementation */
    PERF_Custom_Private cip;
#endif
} PERF_Private;

#define get_Private(handle) \
    ((PERF_Private *) ( (handle)->pComponentPrivate ))

/* PERF Instrumentation mode */
enum PERF_MODE
{
    PERF_Mode_None            =  0x0,
    PERF_Mode_Log             =  0x1,
};

/*=============================================================================
    INSTRUMENTATION OBJECT
=============================================================================*/
typedef struct PERF_OBJTYPE
{
    /** pComponentPrivate is a pointer to the component private
    *   data area. This member is allocated and initialized by the
    *   component when the component is created.  The application
    *   should not access this data area.
    *  */
    void *pComponentPrivate;

    /** pApplicationPrivate is a 32 bit pointer that is unused by
    *   PERF. The value is set to NULL when the component is
    *   created and not read or written thereafter.
    * */
    void *pApplicationPrivate;

    /** The Done method is called at the end of the UC test or UI
    *   application.
    *   @param phObject
    *       Pointer to a handle to the PERF object, which will be
    *       deleted and set to NULL upon completion.
    *  */
    void (*Done)(
                PERF_OBJHANDLE *phObject);

#ifdef __PERF_CUSTOMIZABLE__
    PERF_Custom_Interface ci;
#endif
} PERF_OBJTYPE;

/* check if handle is defined */
#define PERF_check(handle, exp) ((handle) ? (exp),1 : 0)

#define __PERF_Done(                                         \
        hObject)                                             \
    PERF_check((hObject), ((PERF_OBJHANDLE)(hObject))->Done( \
        (PERF_OBJHANDLE *)&(hObject)) )  /* Macro End */

/* define the PERF log interface - standard implementation */
#include "perf_log.h"

#endif
