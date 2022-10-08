/*
 * Copyright (c) 2011 Intel Corporation. All Rights Reserved.
 * Copyright (c) Imagination Technologies Limited, UK
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/*!****************************************************************************
@File                   img_defs.h

@Title                  Common header containing type definitions for portability

@Author                 Imagination Technologies

@date                   August 2001

@Platform               cross platform / environment

@Description    Contains variable and structure definitions. Any platform
                                specific types should be defined in this file.

@DoxygenVer

******************************************************************************/

/******************************************************************************
Modifications :-

$Log: img_defs.h $
*****************************************************************************/
#if !defined (__IMG_DEFS_H__)
#define __IMG_DEFS_H__

#include "img_types.h"

/*!
 *****************************************************************************
 *      These types should be changed on a per-platform basis to retain
 *      the indicated number of bits per type. e.g.: A DP_INT_16 should be
 *      always reflect a signed 16 bit value.
 *****************************************************************************/

typedef         enum    img_tag_TriStateSwitch {
    IMG_ON              =       0x00,
    IMG_OFF,
    IMG_IGNORE

} img_TriStateSwitch, * img_pTriStateSwitch;

#define         IMG_SUCCESS                             0
#define     IMG_FAILED             -1

#define         IMG_NULL                                0
#define         IMG_NO_REG                              1

#if defined (NO_INLINE_FUNCS)
#define INLINE
#define FORCE_INLINE
#else
#if defined(_UITRON_)
#define INLINE
#define FORCE_INLINE                    static
#define INLINE_IS_PRAGMA
#else
#if defined (__cplusplus)
#define INLINE                                  inline
#define FORCE_INLINE                    inline
#else
#define INLINE                                  __inline
#if defined(UNDER_CE) || defined(UNDER_XP) || defined(UNDER_VISTA)
#define FORCE_INLINE                    __forceinline
#else
#define FORCE_INLINE                    static __inline
#endif
#endif
#endif
#endif

/* Use this in any file, or use attributes under GCC - see below */
#ifndef PVR_UNREFERENCED_PARAMETER
#define PVR_UNREFERENCED_PARAMETER(param) (param) = (param)
#endif

/* The best way to supress unused parameter warnings using GCC is to use a
 * variable attribute.  Place the unref__ between the type and name of an
 * unused parameter in a function parameter list, eg `int unref__ var'. This
 * should only be used in GCC build environments, for example, in files that
 * compile only on Linux. Other files should use UNREFERENCED_PARAMETER */
#ifdef __GNUC__
#define unref__ __attribute__ ((unused))
#else
#define unref__
#endif

#if defined(UNDER_CE)

/* This may need to be _stdcall */
#define IMG_CALLCONV
#define IMG_INTERNAL
#define IMG_RESTRICT

#define IMG_EXPORT
#define IMG_IMPORT      IMG_EXPORT

#ifdef DEBUG
#define IMG_ABORT()     TerminateProcess(GetCurrentProcess(), 0)
#else
#define IMG_ABORT()     for(;;);
#endif
#else
#if defined(_WIN32)

#define IMG_CALLCONV __stdcall
#define IMG_INTERNAL
#define IMG_EXPORT      __declspec(dllexport)
#define IMG_RESTRICT __restrict


/* IMG_IMPORT is defined as IMG_EXPORT so that headers and implementations match.
 * Some compilers require the header to be declared IMPORT, while the implementation is declared EXPORT
 */
#define IMG_IMPORT      IMG_EXPORT
#if defined( UNDER_VISTA ) && !defined(USE_CODE)
#ifndef _INC_STDLIB
void __cdecl abort(void);
#endif
__forceinline void __declspec(noreturn) img_abort(void)
{
    for (;;) abort();
}
#define IMG_ABORT()     img_abort()
#endif
#else
#if defined (__SYMBIAN32__)

#if defined (__GNUC__)
#define IMG_IMPORT
#define IMG_EXPORT      __declspec(dllexport)
#define IMG_RESTRICT __restrict__
#define NONSHARABLE_CLASS(x) class x
#else
#if defined (__CC_ARM)
#define IMG_IMPORT __declspec(dllimport)
#define IMG_EXPORT __declspec(dllexport)
#pragma warning("Please make sure that you've enabled the --restrict mode t")
/* The ARMCC compiler currently requires that you've enabled the --restrict mode
   to permit the use of this keyword */
# define IMG_RESTRICT /*restrict*/
#endif
#endif

#define IMG_CALLCONV
#define IMG_INTERNAL
#define IMG_EXTERNAL

#else
#if defined(__linux__)

#define IMG_CALLCONV
#define IMG_INTERNAL    __attribute__ ((visibility ("hidden")))
#define IMG_EXPORT
#define IMG_IMPORT
#define IMG_RESTRICT    __restrict__

#else
#if defined(_UITRON_)
#define IMG_CALLCONV
#define IMG_INTERNAL
#define IMG_EXPORT
#define IMG_RESTRICT

#define __cdecl

/* IMG_IMPORT is defined as IMG_EXPORT so that headers and implementations match.
* Some compilers require the header to be declared IMPORT, while the implementation is declared EXPORT
*/
#define IMG_IMPORT      IMG_EXPORT
#ifndef USE_CODE
void SysAbort(char const *pMessage);
#define IMG_ABORT()     SysAbort("ImgAbort")
#endif
#else
#error("define an OS")
#endif
#endif
#endif
#endif
#endif

// Use default definition if not overridden
#ifndef IMG_ABORT
#define IMG_ABORT()     abort()
#endif


#endif /* #if !defined (__IMG_DEFS_H__) */
/*****************************************************************************
 End of file (IMG_DEFS.H)
*****************************************************************************/

