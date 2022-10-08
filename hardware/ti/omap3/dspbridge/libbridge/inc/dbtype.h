/*
 * dspbridge/mpu_api/inc/dbtype.h
 *
 * DSP-BIOS Bridge driver support functions for TI OMAP processors.
 *
 * Copyright (C) 2007 Texas Instruments, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published 
 * by the Free Software Foundation version 2.1 of the License.
 *
 * This program is distributed .as is. WITHOUT ANY WARRANTY of any kind,
 * whether express or implied; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */


/*
 *  ======== dbtype.h ========
 *  Description:
 *      This header defines data types for DSP/BIOS Bridge APIs and device 
 *      driver modules. It also defines the Hungarian 
 *      prefix to use for each base type.
 *   
 *
 *! Revision History:
 *! =================
 *! 23-Nov-2002 gp: Purpose -> Description in file header.
 *! 13-Feb-2001 kc: Name changed from ddsptype.h dbtype.h.
 *! 09-Oct-2000 jeh Added CHARACTER.
 *! 14-Sep-2000 jeh Moved PSTRING from dspdefs.h to dbtype.h. Define
 *!                 DEF_WINCE_ so PSTRING gets defined.
 *! 11-Aug-2000 ag: Added 'typedef void VOID'. 
 *! 08-Apr-2000 ww: Cloned. 
 */

#ifndef DBTYPE_
#define DBTYPE_

#ifndef DEF_LINUX_
#define DEF_LINUX_
#endif

#ifdef DEAD_CODE
/* Stifle compiler warnings: */
#ifndef UNUSED_PARAMETER
#define UNUSED_PARAMETER(P)   (P)
#endif
#endif

/*============================================================================*/
/*  Argument specification syntax                                             */
/*============================================================================*/

#ifndef IN
#define IN			/* Following parameter is for input. */
#endif

#ifndef OUT
#define OUT			/* Following parameter is for output. */
#endif

#ifndef OPTIONAL
#define OPTIONAL		/* Function may optionally use previous parameter. */
#endif

#ifndef CONST
#define CONST   const
#endif

/*============================================================================*/
/*  Boolean constants                                                         */
/*============================================================================*/

#ifndef FALSE
#define FALSE   0
#endif
#ifndef TRUE
#define TRUE    1
#endif

/*============================================================================*/
/*  NULL    (Definition is language specific)                                 */
/*============================================================================*/

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)	/* Null pointer. */
#endif
#endif

/*============================================================================*/
/*  NULL character   (normally used for string termination)                   */
/*============================================================================*/

#ifndef NULL_CHAR
#define NULL_CHAR    '\0'	/* Null character. */
#endif

/*============================================================================*/
/*  Basic Type definitions (with Prefixes for Hungarian notation)             */
/*============================================================================*/
typedef unsigned char BYTE;	/* b    */

typedef unsigned short WORD;	/* w    */
typedef unsigned long DWORD;	/* dw   */

typedef char CHAR;		/* ch   */
typedef short SHORT;		/* s    */
typedef int INT;		/* n    */
typedef long LONG;		/* l    */

typedef unsigned short USHORT;	/* us   */
typedef unsigned int UINT;	/* u    */
typedef unsigned long ULONG;	/* ul   */

typedef double DOUBLE;		/* dbl  */

typedef CHAR SZ[];		/* sz   */
typedef CHAR *PSTR;		/* pstr */

#ifndef OMAPBRIDGE_TYPES
#define OMAPBRIDGE_TYPES

typedef unsigned char UCHAR;	/* uch  */
typedef float FLOAT;		/* flt  */
typedef int BOOL;		/* f    */

typedef volatile unsigned short REG_UWORD16;

#endif

#ifndef VOID
#define VOID void
#endif

typedef VOID *PVOID;		/* p    */
typedef PVOID HANDLE;		/* h    */

#ifdef DEF_WINCE_
/*------------------------------ WINCE ---------------------------------------*/

typedef unsigned short WCHAR;	/* wch  */

#if defined(UNICODE)
typedef WCHAR TCHAR;
#else
typedef CHAR TCHAR;
#endif

typedef WCHAR *PWCHAR;
typedef TCHAR *PSTRING;		/* Generic character string type */
typedef TCHAR CHARACTER;

#ifdef ERROR			/* Definition of ERROR in wingdi.h clashes with gt.h  */
#undef ERROR
#endif

/*------------------------------ WINCE ---------------------------------------*/
#endif				/* ifdef DEF_WINCE_ */

#ifdef DEF_EPOC_
/*------------------------------ EPOC ----------------------------------------*/

/*------------------------------ EPOC ----------------------------------------*/
#endif				/* ifdef DEF_EPOC_ */

#ifdef DEF_LINUX_
/*------------------------------ LINUX -------------------------------------*/

typedef unsigned short WCHAR;	/* wch  */

#if defined(UNICODE)
typedef WCHAR TCHAR;
#else
typedef CHAR TCHAR;
#endif

typedef WCHAR *PWCHAR;
typedef TCHAR *PSTRING;		/* Generic character string type */
typedef TCHAR CHARACTER;

typedef BYTE *PBYTE;		/* p    */

typedef DWORD *PDWORD;		/* dw   */

typedef VOID *LPVOID;		/*lp   */
typedef VOID *LPCVOID;		/*lpcvoid    */

typedef long long LARGE_INTEGER;
#define TEXT(x) x

#ifdef ERROR			/* Definition of ERROR in wingdi.h clashes with gt.h  */
#undef ERROR
#endif

/*------------------------------ LINUX -------------------------------------*/
#endif				/* ifdef DEF_LINUX_ */

#ifdef DEAD_CODE
/*============================================================================*/
/*  Standard calling conventions                                              */
/*============================================================================*/

#define STATIC          static
#define EXTERN          extern
#endif

#ifdef DEF_WINCE_
/*------------------------------ WINCE ---------------------------------------*/

#ifndef CDECL
#define CDECL           _cdecl
#endif

#ifndef WINAPI
#if (_MSC_VER >= 800) || defined(_STDCALL_SUPPORTED)
#define WINAPI          __stdcall
#else
#define WINAPI
#endif
#endif

#define STDCALL         WINAPI

#define DLLIMPORT       __declspec(dllexport)
#define DLLEXPORT       __declspec(dllexport)

/*------------------------------ WINCE ---------------------------------------*/
#endif				/* ifdef DEF_WINCE_ */

#ifdef DEF_EPOC_
/*------------------------------ EPOC ----------------------------------------*/

#define CDECL           __cdecl

#define STDCALL

#ifdef __VC32__
#define DLLIMPORT       __declspec(dllexport)
#define DLLEXPORT       __declspec(dllexport)
#endif

#ifdef __GCC32__
#define DLLIMPORT
#define DLLEXPORT       __declspec(dllexport)
#endif

/*------------------------------ EPOC ----------------------------------------*/
#endif				/* ifdef DEF_EPOC_ */

#ifdef DEF_LINUX_
/*------------------------------ LINUX -------------------------------------*/

#define CDECL

#define WINAPI

#define STDCALL

#define DLLIMPORT
#define DLLEXPORT

/* Define DSPAPIDLL correctly in dspapi.h */
#define _DSPSYSDLL32_

/*------------------------------ LINUX -------------------------------------*/
#endif				/* ifdef DEF_LINUX_ */

#ifdef DEAD_CODE
/*============================================================================*/
/*  Derived calling conventions                                               */
/*============================================================================*/

#define DSPNORMALAPI    STDCALL
#define DSPKERNELAPI    CDECL
#define DSPEXPORTAPI    DLLEXPORT
#endif

#endif				/* DBTYPE_ */
