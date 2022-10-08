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


/******************************************************************************
* Name         : img_types.h
* Title        : Global types for use by IMG APIs
* Author(s)    : Imagination Technologies
* Created      : 1st August 2003
*
* Description  : Defines type aliases for use by IMG APIs
*
* Platform     : Generic
*
* Modifications:-
* $Log: img_types.h $
******************************************************************************/

#ifndef __IMG_TYPES_H__
#define __IMG_TYPES_H__

#include "img_defs.h"

typedef unsigned int    IMG_UINT,      *IMG_PUINT;
typedef signed int      IMG_INT,       *IMG_PINT;

typedef unsigned char   IMG_UINT8,     *IMG_PUINT8;
typedef unsigned char   IMG_BYTE,      *IMG_PBYTE;
typedef signed char     IMG_INT8,      *IMG_PINT8;
typedef char            IMG_CHAR,      *IMG_PCHAR;

typedef unsigned short  IMG_UINT16,    *IMG_PUINT16;
typedef signed short    IMG_INT16,     *IMG_PINT16;
typedef unsigned int    IMG_UINT32,    *IMG_PUINT32;
typedef signed int      IMG_INT32,     *IMG_PINT32;
typedef unsigned long long IMG_UINT64, *IMG_PUINT64;
typedef signed long long IMG_INT64,    *IMG_PINT64;

typedef unsigned char	IMG_BOOL8,     *IMG_PBOOL8;
typedef unsigned short	IMG_BOOL16,    *IMG_PBOOL16;
typedef unsigned int    IMG_BOOL32,    *IMG_PBOOL32;

#if defined(_WIN32)

typedef unsigned __int64   IMG_UINT64, *IMG_PUINT64;
typedef __int64            IMG_INT64,  *IMG_PINT64;

#else
#if defined(LINUX) || defined (__SYMBIAN32__) || defined(_UITRON_)

#else

#error("define an OS")

#endif
#endif

#if !(defined(LINUX) && defined (__KERNEL__))
/* Linux kernel mode does not use floating point */
typedef float                   IMG_FLOAT,      *IMG_PFLOAT;
typedef double                  IMG_DOUBLE, *IMG_PDOUBLE;
#endif

typedef enum tag_img_bool {
    IMG_FALSE           = 0,
    IMG_TRUE            = 1,
    IMG_FORCE_ALIGN = 0x7FFFFFFF
} IMG_BOOL, *IMG_PBOOL;

typedef void                    IMG_VOID,       *IMG_PVOID;

typedef IMG_INT32               IMG_RESULT;

typedef unsigned long           IMG_UINTPTR_T;

typedef IMG_PVOID       IMG_HANDLE;

typedef void**                  IMG_HVOID,      * IMG_PHVOID;

typedef IMG_UINT32      IMG_SIZE_T;

#define IMG_NULL                0


/*
 * Address types.
 * All types used to refer to a block of memory are wrapped in structures
 * to enforce some type degree of type safety, i.e. a IMG_DEV_VIRTADDR cannot
 * be assigned to a variable of type IMG_DEV_PHYSADDR because they are not the
 * same thing.
 *
 * There is an assumption that the system contains at most one non-cpu mmu,
 * and a memory block is only mapped by the MMU once.
 *
 * Different devices could have offset views of the physical address space.
 *
 */


/*
 *
 * +------------+    +------------+      +------------+        +------------+
 * |    CPU     |    |    DEV     |      |    DEV     |        |    DEV     |
 * +------------+    +------------+      +------------+        +------------+
 *       |                 |                   |                     |
 *       | PVOID           |IMG_DEV_VIRTADDR   |IMG_DEV_VIRTADDR     |
 *       |                 \-------------------/                     |
 *       |                          |                                |
 * +------------+             +------------+                         |
 * |    MMU     |             |    MMU     |                         |
 * +------------+             +------------+                         |
 *       |                          |                                |
 *       |                          |                                |
 *       |                          |                                |
 *   +--------+                +---------+                      +--------+
 *   | Offset |                | (Offset)|                      | Offset |
 *   +--------+                +---------+                      +--------+
 *       |                          |                IMG_DEV_PHYADDR |
 *       |                          |                                |
 *       |                          | IMG_DEV_PHYADDR                |
 * +---------------------------------------------------------------------+
 * |                         System Address bus                          |
 * +---------------------------------------------------------------------+
 *
 */

typedef IMG_PVOID IMG_CPU_VIRTADDR;

/* cpu physical address */
typedef struct {
    IMG_UINT32 uiAddr;
} IMG_CPU_PHYADDR;

/* device virtual address */
typedef struct {
    IMG_UINT32 uiAddr;
} IMG_DEV_VIRTADDR;

/* device physical address */
typedef struct {
    IMG_UINT32 uiAddr;
} IMG_DEV_PHYADDR;

/* system physical address */
typedef struct {
    IMG_UINT32 uiAddr;
} IMG_SYS_PHYADDR;

/*
        system physical structure.
        specifies contiguous and non-contiguous system physical addresses
*/
typedef struct _SYSTEM_ADDR_ {
    /* if zero this is contiguous */
    IMG_UINT32  ui32PageCount;
    union {
        /*
                contiguous address:
                basic system address
        */
        IMG_SYS_PHYADDR sContig;

        /*
                non-contiguous address:
                multiple system page addresses representing system pages
                of which a single allocation is composed
                Note: if non-contiguous allocations don't always start at a
                page boundary then a page offset word is also required.
        */
        IMG_SYS_PHYADDR asNonContig[1];
    } u;
} SYSTEM_ADDR;

#endif  /* __IMG_TYPES_H__ */
/******************************************************************************
 End of file (img_types.h)
******************************************************************************/
