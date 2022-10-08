
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

/* this union is used to convert a float to a long representation without
   breaking strict aliasing rules */
union __PERF_float_long
{
    float f;
    unsigned long l;
};

/* combine 2 values in a unsigned long */
#define PERF_mask(field, len) \
    ( ((unsigned long) (field)) & ((1 << (len)) - 1) )

#define PERF_bits(field, start, len) \
    ( (((unsigned long) (field)) >> start) & ((1 << (len)) - 1) )

#define PERF_log_combine(flag, field, len) \
    ( ((unsigned long) (flag)) | (PERF_mask(field, len) & PERF_LOG_NotMask) )

#define PERF_COMBINE2(field1, len1, field2, len2) \
    ( ( PERF_mask(field1, len1) << (len2) ) |     \
        PERF_mask(field2, len2) )

#define PERF_LOGCOMBINE2(flag, field1, len1, field2, len2)            \
    PERF_log_combine(flag, PERF_COMBINE2(field1, len1, field2, len2), \
    (len1) + (len2))

#define PERF_COMBINE3(field1, len1, field2, len2, field3, len3) \
    ((( ( PERF_mask(field1, len1)   << (len2) ) |               \
          PERF_mask(field2, len2) ) << (len3) ) |               \
          PERF_mask(field3, len3) )

#define PERF_LOGCOMBINE3(flag, field1, len1, field2, len2, field3, len3)     \
    PERF_log_combine(flag,                                                   \
                    PERF_COMBINE3(field1, len1, field2, len2, field3, len3), \
                    (len1) + (len2) + (len3))

/*=============================================================================
    HARD CODED (default) INTERFACE

    This translates each instrumentation directly into a logging call.  It is
    always defined, so that the logs can be done by the custom interface.
=============================================================================*/

#define __PERF_LOG_Boundary(           \
        hObject,                       \
        eBoundary)                     \
    PERF_check((hObject),              \
     __PERF_log1(get_Private(hObject), \
                 PERF_log_combine(PERF_LOG_Boundary, eBoundary, 26)) ) /* Macro End */

/* we squeeze PERF_LOG flag into 2 or 4 bits */
#define __PERF_LOG_Buffer(                                                  \
        hObject,                                                            \
        flSending,                                                          \
        flMultiple,                                                         \
        flFrame,                                                            \
        ulAddress1,                                                         \
        ulAddress2,                                                         \
        ulSize,                                                             \
        eModule1,                                                           \
        eModule2)                                                           \
    PERF_check((hObject),                                                   \
     PERF_IsMultiple(flMultiple) ?                                          \
      __PERF_log3(get_Private(hObject),                                     \
                  PERF_IsXfering(flSending) ?                               \
                  (PERF_LOG_Buffer | PERF_LOG_Xfering |  /* 2 bits */       \
                   PERF_COMBINE3((ulSize) >> 3, 30 - 2 * PERF_ModuleBits,   \
                                 eModule2, PERF_ModuleBits,                 \
                                 eModule1, PERF_ModuleBits) ) :             \
                  (PERF_LOG_Buffer | flSending |  /* 4 bits */              \
                   PERF_COMBINE2(ulSize, 28 - PERF_ModuleBits,              \
                                 eModule1, PERF_ModuleBits) ),              \
                  ( ((unsigned long) (ulAddress1)) & ~3 ) |                 \
                  ( PERF_IsFrame   (flFrame)    ? PERF_LOG_Frame    : 0 ) | \
                  ( PERF_IsMultiple(flMultiple) ? PERF_LOG_Multiple : 0 ),  \
                  (unsigned long) (ulAddress2) ) :                          \
      __PERF_log2(get_Private(hObject),                                     \
                  PERF_IsXfering(flSending) ?                               \
                  (PERF_LOG_Buffer | PERF_LOG_Xfering |  /* 2 bits */       \
                   PERF_COMBINE3((ulSize) >> 3, 30 - 2 * PERF_ModuleBits,   \
                                 eModule2, PERF_ModuleBits,                 \
                                 eModule1, PERF_ModuleBits) ) :             \
                  (PERF_LOG_Buffer | flSending | /* 4 bits */               \
                   PERF_COMBINE2(ulSize, 28 - PERF_ModuleBits,              \
                                 eModule1, PERF_ModuleBits) ),              \
                  ( ((unsigned long) (ulAddress1)) & ~3 ) |                 \
                  ( PERF_IsFrame   (flFrame)    ? PERF_LOG_Frame    : 0 ) | \
                  ( PERF_IsMultiple(flMultiple) ? PERF_LOG_Multiple : 0 ) ))

#define __PERF_LOG_Command(                                               \
        hObject,                                                          \
        ulSending,                                                        \
        ulArgument,                                                       \
        ulCommand,                                                        \
        eModule)                                                          \
    PERF_check((hObject),                                                 \
     __PERF_log3(get_Private(hObject),                                    \
                 PERF_log_combine(PERF_LOG_Command | ulSending, eModule, 28), \
                 ((unsigned long) (ulCommand)),                           \
                 ((unsigned long) (ulArgument)) ) ) /* Macro End */

#define __PERF_LOG_Log(                              \
        hObject,                                     \
        ulData1,                                     \
        ulData2,                                     \
        ulData3)                                     \
    PERF_check((hObject),                            \
    __PERF_log3(get_Private(hObject),                \
        PERF_log_combine(PERF_LOG_Log, ulData1, 28), \
        ((unsigned long) (ulData2)),                 \
        ((unsigned long) (ulData3)) ) ) /* Macro End */

#define __PERF_LOG_SyncAV(                                                \
        hObject,                                                          \
        fTimeAudio,                                                       \
        fTimeVideo,                                                       \
        eSyncOperation)                                                   \
    do                                                                    \
    {                                                                     \
        union __PERF_float_long uA,uV;                                    \
        uA.f = fTimeAudio;  uV.f = fTimeVideo;                            \
        PERF_check((hObject),                                             \
         __PERF_log3(get_Private(hObject),                                \
                     PERF_log_combine(PERF_LOG_Sync, eSyncOperation, 28), \
                     uA.l, uV.l));                                        \
    }\
    while(0) /* Macro End */

#define __PERF_LOG_ThreadCreated(                          \
        hObject,                                           \
        ulThreadID,                                        \
        ulThreadName)                                      \
    PERF_check((hObject),                                  \
    __PERF_log2(get_Private(hObject),                      \
        PERF_log_combine(PERF_LOG_Thread, ulThreadID, 26), \
        ((unsigned long) (ulThreadName)) ) ) /* Macro End */


/* ============================================================================
   PERF LOG Data Structures
============================================================================ */

/* private PERF structure for logging */
typedef struct PERF_LOG_Private
{
    unsigned long   uBufferCount; /* number of buffers filled */
    unsigned long   uBufSize;     /* size of buffer */
    unsigned long  *puBuffer;     /* start of current buffer */
    unsigned long  *puEnd;        /* 'end' of current buffer */
    unsigned long  *puPtr;        /* current buffer pointer */
    FILE *fOut;                   /* output file */
    char *fOutFile;               /* output file name */
} PERF_LOG_Private;

/* log flags used */
enum PERF_LogStamps
{
    /* 1 arguments */
    PERF_LOG_Done     = 0x00000000,
    PERF_LOG_Boundary = 0x08000000,
    /* 2 arguments */
    PERF_LOG_Thread   = 0x0C000000,
    /* 3 arguments */
    PERF_LOG_Log      = 0x10000000,
    PERF_LOG_Sync     = 0x20000000,
#ifdef __PERF_LOG_LOCATION__
    /* many arguments */
    PERF_LOG_Location = 0x30000000,
#endif
    PERF_LOG_Command  = 0x40000000,  /* - 0x70000000 */
    /* 2 or 3 arguments */
    PERF_LOG_Buffer   = 0x80000000,  /* - 0xF0000000 */

    /* flags and masks */
    PERF_LOG_Mask     = 0xF0000000,
    PERF_LOG_NotMask  = ~PERF_LOG_Mask,
    PERF_LOG_Mask2    = 0xFC000000,
    PERF_LOG_NotMask2 = ~PERF_LOG_Mask2,
    PERF_LOG_Frame    = 0x00000002,
    PERF_LOG_Multiple = 0x00000001,

    /* NOTE: we identify xfer buffer from upper 2 bits (11) */
    PERF_LOG_Xfering  = 0x40000000,
    /* NOTE: we identify other buffer logs from upper 4 bits (1000 or 1011) */
    PERF_LOG_Sending  = 0x30000000,
};


#ifdef __PERF_LOG_LOCATION__

/* PERF log locations are encoded in 6-bits/char.  The last 20 characters of
   the filename and functionname are saved (20*6*2 bits), along with the last
   12 bits of the line.  4-bit log stamp makes this 240+12+4=256 bits.  */

/* __PERF_ENCODE_CHAR converts a character to a 6-bit value:
    a-z => 0-25
    A-Z => 26-51
    1-9 => 52-60
      0 => 'O' (41)
      . => 61
    /,\ => 62
 else,_ => 63         */
#define __PERF_ENCODE_CHAR(c) \
   ( ((c) >= 'a' && (c) <= 'z') ? ((c) - 'a')      : \
     ((c) >= 'A' && (c) <= 'Z') ? ((c) - 'A' + 26) : \
     ((c) >= '1' && (c) <= '9') ? ((c) - '1' + 52) : \
                     (c) == '.' ? 61 :               \
                     (c) == '0' ? ('O' - 'A' + 26) : \
    ((c) == '/' || (c) == '\\') ? 62 : 63 )

/* Get the i-th character from the end of a string, or '/' if i is too big */
#define __PERF_GET_INDEXED_CHAR(sz, i) (strlen(sz) <= i ? '/' : sz[strlen(sz) - i - 1])                                        

/* Encode i-th character of a string (from the end) */
#define __PERF_ENCODE_INDEXED(sz, i) \
    __PERF_ENCODE_CHAR(__PERF_GET_INDEXED_CHAR(sz, i))

/* Encode and pack 6 characters into 32 bits.  Only the left 2 bits of the
   6th character will fit */
#define __PERF_PACK6(a1,a2,a3,a4,a5,a6) \
    (((unsigned long) (a1))        | \
    (((unsigned long) (a2)) << 6)  | \
    (((unsigned long) (a3)) << 12) | \
    (((unsigned long) (a4)) << 18) | \
    (((unsigned long) (a5)) << 24) | \
   ((((unsigned long) (a6)) & 0x30) << 26))

#endif

/* ============================================================================
   PERF LOG External methods
============================================================================ */

extern void __PERF_LOG_log_common(PERF_Private *perf, unsigned long *time_loc);

/* ============================================================================
   PERF LOG Inline methods
============================================================================ */
#if defined(__PERF_C__) || defined(INLINE_SUPPORTED)

/* Effects: logs 1 data */
INLINE void __PERF_log1(PERF_Private *priv,
                        unsigned long ulData1)
{
    /* get log private structures */
    PERF_LOG_Private *me   = priv->pLog;
    unsigned long *time_loc = me->puPtr++;

    *me->puPtr++ = ulData1;
    __PERF_LOG_log_common(priv, time_loc);
}

/* Effects: logs 2 data */
INLINE void __PERF_log2(PERF_Private *priv,
                        unsigned long ulData1,
                        unsigned long ulData2)
{
    /* get log private structures */
    PERF_LOG_Private *me   = priv->pLog;
    unsigned long *time_loc = me->puPtr++;

    *me->puPtr++ = ulData1;
    *me->puPtr++ = ulData2;
    __PERF_LOG_log_common(priv, time_loc);
}

/* Effects: logs 3 data */
INLINE void __PERF_log3(PERF_Private *priv,
                        unsigned long ulData1,
                        unsigned long ulData2,
                        unsigned long ulData3)
{
    /* get log private structures */
    PERF_LOG_Private *me   = priv->pLog;
    unsigned long *time_loc = me->puPtr++;

    *me->puPtr++ = ulData1;
    *me->puPtr++ = ulData2;
    *me->puPtr++ = ulData3;
    __PERF_LOG_log_common(priv, time_loc);
}

#ifdef __PERF_LOG_LOCATION__

void __PERF_LOG_flush(PERF_LOG_Private *me);

/* Effects: logs 3 data */
INLINE void
__PERF_log8(PERF_Private *priv,
            unsigned long ulData1,
            unsigned long ulData2,
            unsigned long ulData3,
            unsigned long ulData4,
            unsigned long ulData5,
            unsigned long ulData6,
            unsigned long ulData7,
            unsigned long ulData8)
{
    /* get log private structures */
    PERF_LOG_Private *me   = priv->pLog;

    *me->puPtr++ = ulData8;
    *me->puPtr++ = (ulData1 & PERF_LOG_NotMask) | PERF_LOG_Location;
    *me->puPtr++ = ulData2;
    *me->puPtr++ = ulData3;
    *me->puPtr++ = ulData4;
    *me->puPtr++ = ulData5;
    *me->puPtr++ = ulData6;
    *me->puPtr++ = ulData7;

    /* flush if we reached end of the buffer */
    if (me->puPtr > me->puEnd) __PERF_LOG_flush(me);
}

INLINE void
__PERF_LOG_Location(PERF_OBJHANDLE hObject,
                    const char *szFile,                          
                    unsigned long ulLine,
                    const char *szFunc)
{
    if (hObject)
    {
        unsigned long a6  = __PERF_ENCODE_INDEXED(szFile, 5);
        unsigned long a12 = __PERF_ENCODE_INDEXED(szFile, 11);
        unsigned long a18 = __PERF_ENCODE_INDEXED(szFile, 17);
        unsigned long b6  = __PERF_ENCODE_INDEXED(szFunc, 5);
        unsigned long b12 = __PERF_ENCODE_INDEXED(szFunc, 11);
        unsigned long b18 = __PERF_ENCODE_INDEXED(szFunc, 17);
    
        /* get log private structures */
        __PERF_log8(get_Private(hObject),
                    (a18 & 0xf) | ((b6 & 0xf) << 4) | ((b12 & 0xf) << 8) |
                    ((b18 & 0xf) << 12) | ((ulLine & 0xfff) << 16),
                    __PERF_PACK6(__PERF_ENCODE_INDEXED(szFile, 0),
                                 __PERF_ENCODE_INDEXED(szFile, 1),
                                 __PERF_ENCODE_INDEXED(szFile, 2),
                                 __PERF_ENCODE_INDEXED(szFile, 3),
                                 __PERF_ENCODE_INDEXED(szFile, 4), a6),
                    __PERF_PACK6(__PERF_ENCODE_INDEXED(szFile, 6),
                                 __PERF_ENCODE_INDEXED(szFile, 7),
                                 __PERF_ENCODE_INDEXED(szFile, 8),
                                 __PERF_ENCODE_INDEXED(szFile, 9),
                                 __PERF_ENCODE_INDEXED(szFile, 10), a12),
                    __PERF_PACK6(__PERF_ENCODE_INDEXED(szFile, 12),
                                 __PERF_ENCODE_INDEXED(szFile, 13),
                                 __PERF_ENCODE_INDEXED(szFile, 14),
                                 __PERF_ENCODE_INDEXED(szFile, 15),
                                 __PERF_ENCODE_INDEXED(szFile, 16), a18),
                    __PERF_PACK6(__PERF_ENCODE_INDEXED(szFunc, 0),
                                 __PERF_ENCODE_INDEXED(szFunc, 1),
                                 __PERF_ENCODE_INDEXED(szFunc, 2),
                                 __PERF_ENCODE_INDEXED(szFunc, 3),
                                 __PERF_ENCODE_INDEXED(szFunc, 4), b6),
                    __PERF_PACK6(__PERF_ENCODE_INDEXED(szFunc, 6),
                                 __PERF_ENCODE_INDEXED(szFunc, 7),
                                 __PERF_ENCODE_INDEXED(szFunc, 8),
                                 __PERF_ENCODE_INDEXED(szFunc, 9),
                                 __PERF_ENCODE_INDEXED(szFunc, 10), b12),
                    __PERF_PACK6(__PERF_ENCODE_INDEXED(szFunc, 12),
                                 __PERF_ENCODE_INDEXED(szFunc, 13),
                                 __PERF_ENCODE_INDEXED(szFunc, 14),
                                 __PERF_ENCODE_INDEXED(szFunc, 15),
                                 __PERF_ENCODE_INDEXED(szFunc, 16), b18),
                    __PERF_ENCODE_INDEXED(szFile, 18) |
                    (__PERF_ENCODE_INDEXED(szFile, 19) << 6)  |
                    (__PERF_ENCODE_INDEXED(szFunc, 18) << 12) |
                    (__PERF_ENCODE_INDEXED(szFunc, 19) << 18) |
                    ((a6 & 0xf) << 24) | ((a12 & 0xf) << 28) );
    }
}
 
#endif

#else

extern void __PERF_log1(PERF_Private *priv,
                        unsigned long ulData1);
extern void __PERF_log2(PERF_Private *priv,
                        unsigned long ulData1,
                        unsigned long ulData2);
extern void __PERF_log3(PERF_Private *priv,
                        unsigned long ulData1,
                        unsigned long ulData2,
                        unsigned long ulData3);

#ifdef __PERF_LOG_LOCATION__

extern void __PERF_log8(PERF_Private *priv,
                        unsigned long ulData1,
                        unsigned long ulData2,
                        unsigned long ulData3,
                        unsigned long ulData4,
                        unsigned long ulData5,
                        unsigned long ulData6,
                        unsigned long ulData7,
                        unsigned long ulData8);


#define __PERF_LOG_Location(hObject, szFile, ulLine, szFunc)           \
do                                                                     \
{                                                                      \
    if (hObject)                                                       \
    {                                                                  \
        unsigned long a6  = __PERF_ENCODE_INDEXED(szFile, 5);          \
        unsigned long a12 = __PERF_ENCODE_INDEXED(szFile, 11);         \
        unsigned long a18 = __PERF_ENCODE_INDEXED(szFile, 17);         \
        unsigned long b6  = __PERF_ENCODE_INDEXED(szFunc, 5);          \
        unsigned long b12 = __PERF_ENCODE_INDEXED(szFunc, 11);         \
        unsigned long b18 = __PERF_ENCODE_INDEXED(szFunc, 17);         \
                                                                       \
        __PERF_log8(get_Private(hObject),                              \
                    (a18 & 0xf)        | ((b6 & 0xf) << 4)   |         \
                    ((b12 & 0xf) << 8) | ((b18 & 0xf) << 12) |         \
                    ((ulLine & 0xfff) << 16),                          \
                    __PERF_PACK6(__PERF_ENCODE_INDEXED(szFile, 0),     \
                              __PERF_ENCODE_INDEXED(szFile, 1),        \
                              __PERF_ENCODE_INDEXED(szFile, 2),        \
                              __PERF_ENCODE_INDEXED(szFile, 3),        \
                              __PERF_ENCODE_INDEXED(szFile, 4), a6),   \
                    __PERF_PACK6(__PERF_ENCODE_INDEXED(szFile, 6),     \
                              __PERF_ENCODE_INDEXED(szFile, 7),        \
                              __PERF_ENCODE_INDEXED(szFile, 8),        \
                              __PERF_ENCODE_INDEXED(szFile, 9),        \
                              __PERF_ENCODE_INDEXED(szFile, 10), a12), \
                    __PERF_PACK6(__PERF_ENCODE_INDEXED(szFile, 12),    \
                              __PERF_ENCODE_INDEXED(szFile, 13),       \
                              __PERF_ENCODE_INDEXED(szFile, 14),       \
                              __PERF_ENCODE_INDEXED(szFile, 15),       \
                              __PERF_ENCODE_INDEXED(szFile, 16), a18), \
                    __PERF_PACK6(__PERF_ENCODE_INDEXED(szFunc, 0),     \
                              __PERF_ENCODE_INDEXED(szFunc, 1),        \
                              __PERF_ENCODE_INDEXED(szFunc, 2),        \
                              __PERF_ENCODE_INDEXED(szFunc, 3),        \
                              __PERF_ENCODE_INDEXED(szFunc, 4), b6),   \
                    __PERF_PACK6(__PERF_ENCODE_INDEXED(szFunc, 6),     \
                              __PERF_ENCODE_INDEXED(szFunc, 7),        \
                              __PERF_ENCODE_INDEXED(szFunc, 8),        \
                              __PERF_ENCODE_INDEXED(szFunc, 9),        \
                              __PERF_ENCODE_INDEXED(szFunc, 10), b12), \
                    __PERF_PACK6(__PERF_ENCODE_INDEXED(szFunc, 12),    \
                              __PERF_ENCODE_INDEXED(szFunc, 13),       \
                              __PERF_ENCODE_INDEXED(szFunc, 14),       \
                              __PERF_ENCODE_INDEXED(szFunc, 15),       \
                              __PERF_ENCODE_INDEXED(szFunc, 16), b18), \
                    __PERF_ENCODE_INDEXED(szFile, 18) |                \
                    (__PERF_ENCODE_INDEXED(szFile, 19) << 6)  |        \
                    (__PERF_ENCODE_INDEXED(szFunc, 18) << 12) |        \
                    (__PERF_ENCODE_INDEXED(szFunc, 19) << 18) |        \
                    ((a6 & 0xf) << 24) |  ((a12 & 0xf) << 28));        \
    }                                                                  \
}                                                                      \
while (0);

#endif

#endif

