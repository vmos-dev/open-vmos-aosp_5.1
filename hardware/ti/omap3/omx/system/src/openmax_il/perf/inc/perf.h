
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
/* This headerdescribes the external PERF interface */
#ifndef __PERF_H__
#define __PERF_H__

/* global flag */
#ifdef __PERF_INSTRUMENTATION__

/** Compile-tempTime configuration options
*
*   In order to facilitate more flexible configuration options,
*   yet maintain run-tempTime performance, we allow the following
*   configuration options:
*
*   __PERF_CUSTOMIZABLE__
*
*   If enabled, we allow customized implementation of the
*   performance interface, e.g. unbuffered debug prints to find
*   the point of a program crash. For more information on the
*   specific features supported, see perf_custom.h.
* 
*   If disabled, all instrumentation calls are hard-coded to be
*   logged into a logging buffer.
* */

#if defined(__PERF_LOG_LOCATION__)
    #define __PERF_Location(hObject) \
        __PERF_FN(Location) (hObject,__FILE__,__LINE__,__func__);
#elif defined(__PERF_LOCATE__)
    #define __PERF_Location(hObject) \
        fprintf(stderr,"PERF in [%s:%d func:%s]:",__FILE__,__LINE__,__func__);
#else
    #define __PERF_Location(hObject) 
#endif

#ifdef __PERF_CUSTOMIZABLE__
    #define __PERF_FN(a) __PERF_CUSTOM_##a
#else
    #define __PERF_FN(a) __PERF_LOG_##a
#endif

/*=============================================================================
    Overall API methodology.

    Since this is an instrumentation API, other than the create call, no
    other methods return any value, so there is no error checking
    requirements.

    If the create call encounters any errors, or if the instrumentation is run-
    tempTime disabled, it will return NULL.  All other API-s must handle a NULL
    argument, and will do nothing in such case.
=============================================================================*/

/* Four CC calculations from characters and string */

#define PERF_FOURCC(a,b,c,d)                   \
    ( ((((unsigned long) (a)) & 0xFF) << 24) | \
      ((((unsigned long) (b)) & 0xFF) << 16) | \
      ((((unsigned long) (c)) & 0xFF) << 8)  | \
       (((unsigned long) (d)) & 0xFF) )

#define PERF_FOURS(id) \
      PERF_FOURCC((id)[0], (id)[1], (id)[2], (id)[3])

#define PERF_FOUR_CHARS(i) \
      (char) (((i) >> 24) & 0xFF), (char) (((i) >> 16) & 0xFF), \
      (char) (((i) >> 8)  & 0xFF), (char) ((i) & 0xFF)

#define PREF(a,b) ((a) ? (a)->b : 0)

/*=============================================================================
    ENUMERATIONS
=============================================================================*/

/*-----------------------------------------------------------------------------
    PERF_BOUNDARYTYPE

    The phase of execution for the Boundary callback
-----------------------------------------------------------------------------*/
typedef enum PERF_BOUNDARYTYPE
{
    /* UC phase boundary type */
    PERF_BoundaryStart      = 0,
    PERF_BoundaryComplete   = 0x10000,

    /* UC phase type */
    PERF_BoundarySetup = 1,   /* UC initialization */
    PERF_BoundaryCleanup,     /* UC cleanup */
    PERF_BoundarySteadyState, /* UC is in 'steady state' - optional */

    PERF_BoundaryMax,
    PERF_BoundaryMask   = 3,
} PERF_BOUNDARYTYPE;

#ifdef __PERF_PRINT_C__

char const * const PERF_BoundaryTypes[] = {"NONE", "Setup", "Cleanup", "Steady"};

#endif

#define PERF_IsComplete(x) ((x) & PERF_BoundaryComplete)
#define PERF_IsStarted(x)  (!PERF_IsComplete(x))

/*-----------------------------------------------------------------------------
    PERF_MODULETYPE

    Type of module when creating the component, or when specifying source
    or destination of buffer transfers and/or commands
-----------------------------------------------------------------------------*/

/* These flags are also used for the LOG commands */
#define PERF_FlagSending     0x10000000ul   /* This is used for LOG */
#define PERF_FlagSent        0x30000000ul   /* This is used for LOG */
#define PERF_FlagRequesting  0x20000000ul   /* This is used for LOG */
#define PERF_FlagReceived    0x00000000ul   /* This is used for LOG */
#define PERF_FlagSendFlags   0x30000000ul
#define PERF_FlagXfering     0x40000000ul   /* This is used for LOG */
                                            /* = Received + Sending */
#define PERF_FlagFrame       0x08000000ul
#define PERF_FlagBuffer      0x00000000ul
#define PERF_FlagMultiple    0x80000000ul
#define PERF_FlagSingle      0x00000000ul

#define PERF_GetSendRecv(x) ((x) & PERF_FlagSent)
#define PERF_GetXferSendRecv(x) ((x) & (PERF_FlagSendFlags | PERF_FlagXfering))
#define PERF_GetFrameBuffer(x)     PERF_IsFrame(x)
#define PERF_GetMultipleSingle(x)  PERF_IsMultiple(x)

#define PERF_IsFrame(x)     ((x) & PERF_FlagFrame)
#define PERF_IsBuffer(x)    (!PERF_IsFrame(x))
#define PERF_IsSending(x)   ((x) & PERF_FlagSending)
#define PERF_IsReceived(x)  (!PERF_IsSending(x))
#define PERF_IsXfering(x)   ((x) & PERF_FlagXfering)
#define PERF_IsMultiple(x)  ((x) & PERF_FlagMultiple)
#define PERF_IsSingle(x)    (!PERF_IsMultiple(x))

typedef enum PERF_MODULETYPE
{
    PERF_ModuleApplication,  /* application */
    PERF_ModuleSystem,       /* system components */
    PERF_ModuleService,      /* service or server */
    PERF_ModuleHLMM,         /* high-level multimedia, e.g. MMF */    
    PERF_ModuleLLMM,         /* low-level multimedia, e.g. MDF, OMX */
    PERF_ModuleComponent,    /* multimedia component */
    PERF_ModuleCommonLayer,  /* common layer */
    PERF_ModuleSocketNode,   /* socket-node (e.g. on DSP) */
    PERF_ModuleAlgorithm,    /* algorithm (for possible future needs) */
    PERF_ModuleHardware,     /* hardware (e.g. speaker, microphone) */
    PERF_ModuleMemory,       /* memory or unspecified modules */
    PERF_ModuleMemoryMap,    /* memory mapping */
    PERF_ModuleMax,
    PERF_ModuleBits = 4,
    PERF_ModuleMask = (1 << PERF_ModuleBits) - 1,

    /* module functional types used for selectively enabling instrumentation */
    PERF_ModuleDomains     = 1 << (1 << PERF_ModuleBits),
    PERF_ModuleAudioDecode = PERF_ModuleDomains,
    PERF_ModuleAudioEncode = PERF_ModuleAudioDecode << 1,
    PERF_ModuleVideoDecode = PERF_ModuleAudioEncode << 1,
    PERF_ModuleVideoEncode = PERF_ModuleVideoDecode << 1,
    PERF_ModuleImageDecode = PERF_ModuleVideoEncode << 1,
    PERF_ModuleImageEncode = PERF_ModuleImageDecode << 1,
} PERF_MODULETYPE;

#ifdef __PERF_PRINT_C__

char const * const PERF_ModuleTypes[] = {
    "Application", "System",    "Service",     "HLMM",
    "LLMM",        "Component", "CommonLayer", "SocketNode",
    "Algorithm",   "Hardware",  "Memory",      "MemoryMap"
};

#endif

/*-----------------------------------------------------------------------------
    PERF_SYNCOPTYPE

    Type of module phase of execution for the cbBoundary callback
-----------------------------------------------------------------------------*/
typedef enum PERF_SYNCOPTYPE
{
    PERF_SyncOpNone,
    PERF_SyncOpDropVideoFrame,   /* drop a video frame */
    PERF_SyncOpMax,
} PERF_SYNCOPTYPE;

#ifdef __PERF_PRINT_C__

char const *PERF_SyncOpTypes[] = {
    "none", "drop_video_frame"
};

#endif

/*-----------------------------------------------------------------------------
    PERF_COMMANDTYPE

    PERF commands
-----------------------------------------------------------------------------*/
typedef enum PERF_COMMANDTYPE
{
    PERF_CommandMax,
    PERF_CommandStatus = 0x10000000,
} PERF_COMMANDTYPE;

#ifdef __PERF_PRINT_C__

char const *PERF_CommandTypes[] = {
    "none"
};

#endif


/*=============================================================================
    CREATION INTERFACE
=============================================================================*/
typedef struct PERF_OBJTYPE *PERF_OBJHANDLE;

/** PERF_Create method is the is used to create an
*   instrumentation object for a component.
* 
*   NOTE: It's arguments may depend on the operating system, as
*   the PERF object may need access to system resource
*   (terminal, file system).  For now, we assume that all
*   interfaces are accessible from system calls.
* 
*   This operation may include allocating resources from the
*   system, to create any structures required for managing the
*   data collected during instrumentation, but our aim is to
*   have these resources be minimal.
* 
*   Most options are run-tempTime configurable.  See perf_config.h
*   for such options.
* 
*   In case of error - or if performance instrumentation is
*   run-tempTime disabled, PERF_Create will return a NULL handle,
*   and all resource allocations will be undone.
* 
*   @param ulComponent
*       FourCC component ID.
*   @param eModule
*       ModuleType.
*   @return hObject
*       Handle to the instrumentation object.
 */

PERF_OBJHANDLE PERF_Create(
                          unsigned long ulComponentName,
                          PERF_MODULETYPE eModule);

/** The PERF_Done method is called at the end of the component
*   life cycle.
*   @param hObject
*       Handle to the PERF object.  This must be an lvalue, as
*       it will be deleted and set to NULL upon return.
* */
#define PERF_Done(           \
        hObject)             \
    do {                     \
    __PERF_Location(hObject) \
    __PERF_Done(hObject);    \
    } while (0)


/*=============================================================================
    INSTRUMENTATION INTERFACE
=============================================================================*/

/** The PERF_Boundary callback must be called by the component
*   at specific phases of the component.
*   @param hObject
*       Handle to the PERF object.
*   @param eBoundary
*       Boundary (phase) that is reached by the application.
* */

#define PERF_Boundary(       \
        hObject,             \
        eBoundary)           \
    do {                     \
    __PERF_Location(hObject) \
    __PERF_FN(Boundary)(hObject, eBoundary); \
    } while (0)

/** The PERF_SendingBuffer(s) and PERF_SendingFrame(s) method is
*   used to mark when a buffer (or frame) is to be sent to
*   another module.
* 
*   @param hObject
*       Handle to the PERF object.
*   @param ulAddress or ulAddress1 and ulAddress2
*       Buffer (or frame) address(es).
*   @param ulSize
*       size of buffer(s) that is sent regardless if it is sent
*       by pointer exchange or memory copy.
*   @param eModuleTo
*       Type of module that is receiving the data.
* */
#define PERF_SendingBuffer(                                             \
        hObject,                                                        \
        ulAddress,                                                      \
        ulSize,                                                         \
        eModuleTo)                                                      \
  do {                                                                  \
  __PERF_Location(hObject)                                              \
  __PERF_FN(Buffer)(hObject,                                            \
                    PERF_FlagSending, PERF_FlagSingle, PERF_FlagBuffer, \
                    ulAddress, NULL, ulSize, eModuleTo, 0);             \
  } while(0)

#define PERF_SendingFrame(                                             \
        hObject,                                                       \
        ulAddress,                                                     \
        ulSize,                                                        \
        eModuleTo)                                                     \
  do {                                                                 \
  __PERF_Location(hObject)                                             \
  __PERF_FN(Buffer)(hObject,                                           \
                    PERF_FlagSending, PERF_FlagSingle, PERF_FlagFrame, \
                    ulAddress, NULL, ulSize, eModuleTo, 0);            \
  } while (0)

#define PERF_SendingBuffers(                                              \
        hObject,                                                          \
        ulAddress1,                                                       \
        ulAddress2,                                                       \
        ulSize,                                                           \
        eModuleTo)                                                        \
  do {                                                                    \
  __PERF_Location(hObject)                                                \
  __PERF_FN(Buffer)(hObject,                                              \
                    PERF_FlagSending, PERF_FlagMultiple, PERF_FlagBuffer, \
                    ulAddress1, ulAddress2, ulSize, eModuleTo, 0);        \
  } while (0)

#define PERF_SendingFrames(                                              \
        hObject,                                                         \
        ulAddress1,                                                      \
        ulAddress2,                                                      \
        ulSize,                                                          \
        eModuleTo)                                                       \
  do {                                                                   \
  __PERF_Location(hObject)                                               \
  __PERF_FN(Buffer)(hObject,                                             \
                    PERF_FlagSending, PERF_FlagMultiple, PERF_FlagFrame, \
                    ulAddress1, ulAddress2, ulSize, eModuleTo, 0);       \
  } while (0)

/** The PERF_SendingCommand method is used to mark when a
*   command is to be sent to another module.
* 
*   @param hObject
*       Handle to the PERF object.
*   @param ulCommand
*       Command.
*   @param eModuleTo
*       Type of module that is receiving the data.
* */
#define PERF_SendingCommand(  \
        hObject,              \
        ulCommand,            \
        ulArgument,           \
        eModuleTo)            \
  do {                        \
  __PERF_Location(hObject)    \
  __PERF_FN(Command)(hObject, \
                     PERF_FlagSending, ulCommand, ulArgument, eModuleTo); \
  } while (0)

/** The PERF_SentBuffer(s) and PERF_SentFrame(s) method is
*   used to mark when a buffer (or frame) has been sent to
*   another module.
* 
*   @param hObject
*       Handle to the PERF object.
*   @param ulAddress or ulAddress1 and ulAddress2
*       Buffer (or frame) address(es).
*   @param ulSize
*       size of buffer(s) that is sent regardless if it is sent
*       by pointer exchange or memory copy.
*   @param eModuleTo
*       Type of module that is receiving the data.
* */
#define PERF_SentBuffer(                                             \
        hObject,                                                     \
        ulAddress,                                                   \
        ulSize,                                                      \
        eModuleTo)                                                   \
  do {                                                               \
  __PERF_Location(hObject)                                           \
  __PERF_FN(Buffer)(hObject,                                         \
                    PERF_FlagSent, PERF_FlagSingle, PERF_FlagBuffer, \
                    ulAddress, NULL, ulSize, eModuleTo, 0);          \
  } while(0)

#define PERF_SentFrame(                                             \
        hObject,                                                    \
        ulAddress,                                                  \
        ulSize,                                                     \
        eModuleTo)                                                  \
  do {                                                              \
  __PERF_Location(hObject)                                          \
  __PERF_FN(Buffer)(hObject,                                        \
                    PERF_FlagSent, PERF_FlagSingle, PERF_FlagFrame, \
                    ulAddress, NULL, ulSize, eModuleTo, 0);         \
  } while (0)

#define PERF_SentBuffers(                                              \
        hObject,                                                       \
        ulAddress1,                                                    \
        ulAddress2,                                                    \
        ulSize,                                                        \
        eModuleTo)                                                     \
  do {                                                                 \
  __PERF_Location(hObject)                                             \
  __PERF_FN(Buffer)(hObject,                                           \
                    PERF_FlagSent, PERF_FlagMultiple, PERF_FlagBuffer, \
                    ulAddress1, ulAddress2, ulSize, eModuleTo, 0);     \
  } while (0)

#define PERF_SentFrames(                                              \
        hObject,                                                      \
        ulAddress1,                                                   \
        ulAddress2,                                                   \
        ulSize,                                                       \
        eModuleTo)                                                    \
  do {                                                                \
  __PERF_Location(hObject)                                            \
  __PERF_FN(Buffer)(hObject,                                          \
                    PERF_FlagSent, PERF_FlagMultiple, PERF_FlagFrame, \
                    ulAddress1, ulAddress2, ulSize, eModuleTo, 0);    \
  } while (0)

/** The PERF_SentCommand method is used to mark when a
*   command has been sent to another module.
* 
*   @param hObject
*       Handle to the PERF object.
*   @param ulCommand
*       Command.
*   @param eModuleTo
*       Type of module that is receiving the data.
* */
#define PERF_SentCommand(     \
        hObject,              \
        ulCommand,            \
        ulArgument,           \
        eModuleTo)            \
  do {                        \
  __PERF_Location(hObject)    \
  __PERF_FN(Command)(hObject, \
                     PERF_FlagSent, ulCommand, ulArgument, eModuleTo); \
  } while (0)

/** The PERF_RequestingBuffer(s) and PERF_RequestingFrame(s) method
*   is used to mark when a buffer (or frame) is beeing requested
*   from another module. These go hand-in-hand with
*   ReceivedBuffer and ReceivedFrame, if one wants to measure
*   the time spent waiting (pending) for a buffer/frame.  Note,
*   that the buffer address or size may not be known upon
*   calling this method, in which case -1 should be used.
*   @param hAPI
*       Handle to the PERF object.
*   @param ulAddress
*       Buffer (or frame) address.
*   @param ulSize
*       size of buffer(s) that is sent regardless if it is sent
*       by pointer exchange or memory copy.
*   @param eModuleFrom
*       Type of module that is sending the data.
* */
#define PERF_RequestingBuffer(                                             \
        hObject,                                                           \
        ulAddress,                                                         \
        ulSize,                                                            \
        eModuleFrom)                                                       \
  do {                                                                     \
  __PERF_Location(hObject)                                                 \
  __PERF_FN(Buffer)(hObject,                                               \
                    PERF_FlagRequesting, PERF_FlagSingle, PERF_FlagBuffer, \
                    ulAddress, NULL, ulSize, eModuleFrom, 0);              \
  } while (0)

#define PERF_RequestingFrame(                                             \
        hObject,                                                          \
        ulAddress,                                                        \
        ulSize,                                                           \
        eModuleFrom)                                                      \
  do {                                                                    \
  __PERF_Location(hObject)                                                \
  __PERF_FN(Buffer)(hObject,                                              \
                    PERF_FlagRequesting, PERF_FlagSingle, PERF_FlagFrame, \
                    ulAddress, NULL, ulSize, eModuleFrom, 0);             \
  } while (0)

#define PERF_RequestingBuffers(                                              \
        hObject,                                                             \
        ulAddress1,                                                          \
        ulAddress2,                                                          \
        ulSize,                                                              \
        eModuleFrom)                                                         \
  do {                                                                       \
  __PERF_Location(hObject)                                                   \
  __PERF_FN(Buffer)(hObject,                                                 \
                    PERF_FlagRequesting, PERF_FlagMultiple, PERF_FlagBuffer, \
                    ulAddress1, ulAddress2, ulSize, eModuleFrom, 0);         \
  } while (0)

#define PERF_RequestingFrames(                                              \
        hObject,                                                            \
        ulAddress1,                                                         \
        ulAddress2,                                                         \
        ulSize,                                                             \
        eModuleFrom)                                                        \
  do {                                                                      \
  __PERF_Location(hObject)                                                  \
  __PERF_FN(Buffer)(hObject,                                                \
                    PERF_FlagRequesting, PERF_FlagMultiple, PERF_FlagFrame, \
                    ulAddress1, ulAddress2, ulSize, eModuleFrom, 0);        \
  } while (0)

/** The PERF_RequestingCommand method is used to mark when a
*   command is being requested from another module.  This method
*   goes in hand with the PERF_ReceivedCommand to measure the
*   time the application is "pending" for the command.  Note,
*   that the command is most likely not known when calling this
*   method, in which case -1 should be used.
* 
*   @param hObject
*       Handle to the PERF object.
*   @param ulCommand
*       Command.
*   @param eModuleTo
*       Type of module that sent the data.
* */
#define PERF_RequestingCommand( \
        hObject,                \
        ulCommand,              \
		ulArgument,             \
        eModuleFrom)            \
  do {                          \
  __PERF_Location(hObject)      \
  __PERF_FN(Command)(hObject,   \
                     PERF_FlagRequesting, ulCommand, ulArgument, eModuleFrom); \
  } while (0)


/** The PERF_ReceivedBuffer(s) and PERF_ReceivedFrame(s) method
*   is used to mark when a buffer (or frame) is received from
*   another module. These go hand-in-hand with SendingBuffer and
*   SendingFrame, if instrumentation is implemented at all
*   module boundaries.
*   @param hAPI
*       Handle to the PERF object.
*   @param ulAddress
*       Buffer (or frame) address.
*   @param ulSize
*       size of buffer(s) that is sent regardless if it is sent
*       by pointer exchange or memory copy.
*   @param eModuleFrom
*       Type of module that is sending the data.
* */
#define PERF_ReceivedBuffer(                                             \
        hObject,                                                         \
        ulAddress,                                                       \
        ulSize,                                                          \
        eModuleFrom)                                                     \
  do {                                                                   \
  __PERF_Location(hObject)                                               \
  __PERF_FN(Buffer)(hObject,                                             \
                    PERF_FlagReceived, PERF_FlagSingle, PERF_FlagBuffer, \
                    ulAddress, NULL, ulSize, eModuleFrom, 0);            \
  } while (0)

#define PERF_ReceivedFrame(                                             \
        hObject,                                                        \
        ulAddress,                                                      \
        ulSize,                                                         \
        eModuleFrom)                                                    \
  do {                                                                  \
  __PERF_Location(hObject)                                              \
  __PERF_FN(Buffer)(hObject,                                            \
                    PERF_FlagReceived, PERF_FlagSingle, PERF_FlagFrame, \
                    ulAddress, NULL, ulSize, eModuleFrom, 0);           \
  } while (0)

#define PERF_ReceivedBuffers(                                              \
        hObject,                                                           \
        ulAddress1,                                                        \
        ulAddress2,                                                        \
        ulSize,                                                            \
        eModuleFrom)                                                       \
  do {                                                                     \
  __PERF_Location(hObject)                                                 \
  __PERF_FN(Buffer)(hObject,                                               \
                    PERF_FlagReceived, PERF_FlagMultiple, PERF_FlagBuffer, \
                    ulAddress1, ulAddress2, ulSize, eModuleFrom, 0);       \
  } while (0)

#define PERF_ReceivedFrames(                                              \
        hObject,                                                          \
        ulAddress1,                                                       \
        ulAddress2,                                                       \
        ulSize,                                                           \
        eModuleFrom)                                                      \
  do {                                                                    \
  __PERF_Location(hObject)                                                \
  __PERF_FN(Buffer)(hObject,                                              \
                    PERF_FlagReceived, PERF_FlagMultiple, PERF_FlagFrame, \
                    ulAddress1, ulAddress2, ulSize, eModuleFrom, 0);      \
  } while (0)

/** The PERF_ReceivedCommand method is used to mark when a
*   command is received from another module.
* 
*   @param hObject
*       Handle to the PERF object.
*   @param ulCommand
*       Command.
*   @param eModuleTo
*       Type of module that sent the data.
* */
#define PERF_ReceivedCommand( \
        hObject,              \
        ulCommand,            \
		ulArgument,           \
        eModuleFrom)          \
  do {                        \
  __PERF_Location(hObject)    \
  __PERF_FN(Command)(hObject, \
                     PERF_FlagReceived, ulCommand, ulArgument, eModuleFrom); \
  } while (0)

/** The PERF_XferingBuffer(s) and PERF_XferingFrame(s)
*   method is used to mark when a buffer (or frame) is to be
*   transferred from one module to another.  This is shortcut
*   for PERF_Received followed by PERF_Sending
* 
*   @param hObject
*       Handle to the PERF object.
*   @param ulAddress or ulAddress1 and ulAddress2
*       Buffer (or frame) address(es).
*   @param ulSize
*       size of buffer(s) that is sent regardless if it is sent
*       by pointer exchange or memory copy.
*   @param eModuleFrom
*       Type of module that is sending the data.
*   @param eModuleTo
*       Type of module that is receiving the data.
* */
#define PERF_XferingBuffer(                                             \
        hObject,                                                        \
        ulAddress,                                                      \
        ulSize,                                                         \
        eModuleFrom,                                                    \
        eModuleTo)                                                      \
  do {                                                                  \
  __PERF_Location(hObject)                                              \
  __PERF_FN(Buffer)(hObject,                                            \
                    PERF_FlagXfering, PERF_FlagSingle, PERF_FlagBuffer, \
                    ulAddress, NULL, ulSize, eModuleFrom, eModuleTo);   \
  } while (0)

#define PERF_XferingFrame(                                             \
        hObject,                                                       \
        ulAddress,                                                     \
        ulSize,                                                        \
        eModuleFrom,                                                   \
        eModuleTo)                                                     \
  do {                                                                 \
  __PERF_Location(hObject)                                             \
  __PERF_FN(Buffer)(hObject,                                           \
                    PERF_FlagXfering, PERF_FlagSingle, PERF_FlagFrame, \
                    ulAddress, NULL, ulSize, eModuleFrom, eModuleTo);  \
  } while (0)

#define PERF_XferingBuffers(                                              \
        hObject,                                                          \
        ulAddress1,                                                       \
        ulAddress2,                                                       \
        ulSize,                                                           \
        eModuleFrom,                                                      \
        eModuleTo)                                                        \
  do {                                                                    \
  __PERF_Location(hObject)                                                \
  __PERF_FN(Buffer)(hObject,                                              \
                    PERF_FlagXfering, PERF_FlagMultiple, PERF_FlagBuffer, \
                    ulAddress1, ulAddress2, ulSize, eModuleFrom, eModuleTo);\
  } while (0)

#define PERF_XferingFrames(                                              \
        hObject,                                                         \
        ulAddress1,                                                      \
        ulAddress2,                                                      \
        ulSize,                                                          \
        eModuleFrom,                                                     \
        eModuleTo)                                                       \
  do {                                                                   \
  __PERF_Location(hObject)                                               \
  __PERF_FN(Buffer)(hObject,                                             \
                    PERF_FlagXfering, PERF_FlagMultiple, PERF_FlagFrame, \
                    ulAddress1, ulAddress2, ulSize, eModuleFrom, eModuleTo);\
  } while (0)

/** The PERF_SyncData method shall be called by a module that
*   synchronizes audio/video streams.  It informs the
*   instrumentation module about the tempTime stamps of the two
*   streams, as well as the operation (if any) to restore
*   synchronization.
*   @param hObject
*       Handle to the PERF object.
*   @param pfTimeAudio
*       Time stamp of audio frame
*   @param pfTimeVideo
*       Time stamp of video frame
*   @param eSyncOperation
*       Synchronization operation that is being taken to get
*       streams in sync.
* */
#define PERF_SyncAV(       \
        hObject,           \
        fTimeAudio,        \
        fTimeVideo,        \
        eSyncOperation)    \
  do {                     \
  __PERF_Location(hObject) \
    __PERF_FN(SyncAV)(hObject, fTimeAudio, fTimeVideo, eSyncOperation); \
  } while (0)

/** The PERF_TheadCreated method shall be called after creating
*   a new thread.
*   @param hObject
*       Handle to the PERF object.
*   @param pfTimeAudio
*       Time stamp of audio frame
*   @param pfTimeVideo
*       Time stamp of video frame
*   @param eSyncOperation
*       Synchronization operation that is being taken to get
*       streams in sync.
* */
#define PERF_ThreadCreated( \
        hObject,            \
        ulThreadID,         \
        ulThreadName)       \
  do {                      \
  __PERF_Location(hObject)  \
    __PERF_FN(ThreadCreated)(hObject, ulThreadID, ulThreadName); \
  }  while (0)

/** The PERF_Log method can be called to log 3 values into the
*   log.  The 1st argument is capped at 28 bits.
*   @param hObject
*       Handle to the PERF object.
*   @param ulData1, ulData2, ulData3
*       Data to be logged
* */
#define PERF_Log( \
        hObject,  \
        ulData1,  \
        ulData2,  \
        ulData3)  \
  do {            \
  __PERF_Location(hObject) \
    __PERF_FN(Log)(hObject, ulData1, ulData2, ulData3);\
  } while (0)

/* define PERF_OBJ and the used __PERF macros */
#include "perf_obj.h"

#endif /* __PERF_INSTURMENTATION__ */

#endif /* __PERF_H__ */


