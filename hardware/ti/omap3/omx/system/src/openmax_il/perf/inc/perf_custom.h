
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
/* NOTE: This header should be only included from perf_obj.h */

/*=============================================================================
    CUSTOMIZABLE INTERFACE

    If __PERF_CUSTOMIZABLE__ is defined, each PERF API is routed to a method
    that can be customized (set) upon creation of the PERF object.

    Currently we support 3 customizable interfaces that can be each enabled or
    disabled independently:
        - logging into a buffer (default behavior)
        - printing a message on STDOUT, STDERR or a file immediately at each
          instrumentation point
        - replaying logs from a file (this will repeat all the interface calls
          as they happened in the log file, but the time stamps will be set from
          the log file.  Currently, we do not allow logging a replayed session.          
=============================================================================*/

typedef struct PERF_Custom_Interface
{
    /** Customizable PERF interfaces.  These correspond to the
 *  __PERF and PERF macros */
    void (*Boundary)(PERF_OBJHANDLE hObject,
                     PERF_BOUNDARYTYPE eBoundary);

    void (*Buffer)(PERF_OBJHANDLE hObject,
                   unsigned long ulAddress1,
                   unsigned long ulAddress2,
                   unsigned long ulSize,
                   unsigned long ulModuleAndFlags);

    void (*Command)(PERF_OBJHANDLE hObject,
                    unsigned long ulCommand,
                    unsigned long ulArgument,
                    unsigned long ulModuleAndFlags);

    void (*Log)(PERF_OBJHANDLE hObject,
                unsigned long ulData1,
                unsigned long ulData2,
                unsigned long ulData3);

    void (*SyncAV)(PERF_OBJHANDLE hObject,
                   float fTimeAudio,
                   float fTimeVideo,
                   PERF_SYNCOPTYPE eSyncOperation);

    void (*ThreadCreated)(PERF_OBJHANDLE hObject,
                          unsigned long ulThreadID,
                          unsigned long ulThreadName);
#ifdef __PERF_LOG_LOCATION__
    void (*Location)(PERF_OBJHANDLE hObject,
                     char const *szFile,
                     unsigned long ulLine,
                     char const *szFunc);
#endif
} PERF_Custom_Interface;

/** In the customizable instrumentation, each call will be
 *  routed to the corresponding call as defined by the
 *  instrumentation module. */

#define __PERF_CUSTOM_Boundary(                \
        hObject,                               \
        eBoundary)                             \
    PERF_check((hObject),                      \
     ((PERF_OBJHANDLE)(hObject))->ci.Boundary( \
        (PERF_OBJHANDLE)(hObject),             \
        eBoundary) )  /* Macro End */

#define __PERF_CUSTOM_Buffer(                                 \
        hObject,                                              \
        ulFlagSending,                                        \
        ulFlagMultiple,                                       \
        ulFlagFrame,                                          \
        ulAddress1,                                           \
        ulAddress2,                                           \
        ulSize,                                               \
        eModule1,                                             \
        eModule2)                                             \
    PERF_check((hObject),                                     \
     ((PERF_OBJHANDLE)(hObject))->ci.Buffer(                  \
        (PERF_OBJHANDLE)(hObject),                            \
        ((unsigned long) (ulAddress1)),                       \
        ((unsigned long) (ulAddress2)),                       \
        ((unsigned long) (ulSize)),                           \
        ((unsigned long) (eModule1)) |                        \
        ( ((unsigned long) (eModule2)) << PERF_ModuleBits ) | \
        (ulFlagSending) | (ulFlagMultiple) | (ulFlagFrame)) ) /* Macro End */

#define __PERF_CUSTOM_Command(                \
        hObject,                              \
        ulFlagSending,                        \
        ulCommand,                            \
        ulArgument,                           \
        eModule)                              \
    PERF_check((hObject),                     \
     ((PERF_OBJHANDLE)(hObject))->ci.Command( \
        (PERF_OBJHANDLE)(hObject),            \
        ((unsigned long) (ulCommand)),        \
        ((unsigned long) (ulArgument)),       \
        ((unsigned long) (eModule)) | (ulFlagSending)) ) /* Macro End */

#define __PERF_CUSTOM_Log(                \
        hObject,                          \
        ulData1,                          \
        ulData2,                          \
        ulData3)                          \
    PERF_check((hObject),                 \
     ((PERF_OBJHANDLE)(hObject))->ci.Log( \
        (PERF_OBJHANDLE)(hObject),        \
        ((unsigned long) (ulData1)),      \
        ((unsigned long) (ulData2)),      \
        ((unsigned long) (ulData3)) ) ) /* Macro End */

#define __PERF_CUSTOM_SyncAV(                \
        hObject,                             \
        fTimeAudio,                          \
        fTimeVideo,                          \
        eSyncOperation)                      \
    PERF_check((hObject),                    \
     ((PERF_OBJHANDLE)(hObject))->ci.SyncAV( \
        (PERF_OBJHANDLE)(hObject),           \
        fTimeAudio,                          \
        fTimeVideo,                          \
        eSyncOperation) ) /* Macro End */

#define __PERF_CUSTOM_ThreadCreated(                \
        hObject,                                    \
        ulThreadID,                                 \
        ulThreadName)                               \
    PERF_check((hObject),                           \
     ((PERF_OBJHANDLE)(hObject))->ci.ThreadCreated( \
        (PERF_OBJHANDLE)(hObject),                  \
        ((unsigned long) (ulThreadID)),             \
        ((unsigned long) (ulThreadName)) ) )   /* Macro End */

#ifdef __PERF_LOG_LOCATION__
#define __PERF_CUSTOM_Location(                \
        hObject,                               \
        szFile,                                \
        ulLine,                                \
        szFunc)                                \
    PERF_check((hObject),                      \
     ((PERF_OBJHANDLE)(hObject))->ci.Location( \
        (PERF_OBJHANDLE)(hObject),             \
        (szFile),                              \
        ((unsigned long) (ulLine)),            \
        (szFunc)) )   /* Macro End */
#endif

/*=============================================================================
    OUTSIDE INTERFACES
=============================================================================*/

typedef struct PERF_Custom_Private
{
    /* debug interface */
    struct PERF_PRINT_Private *pDebug;

    /* real-time interface */
    struct PERF_RT_Private *pRT;
} PERF_Custom_Private;

/* PERF Custom instrumentation modes */
enum PERF_CUSTOM_MODE
{
    PERF_Mode_Print           =  0x2,
    PERF_Mode_Replay          =  0x4,
    PERF_Mode_RealTime        =  0x8,
};

