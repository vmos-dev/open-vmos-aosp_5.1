/*
 * dspbridge/mpu_api/inc/std.h
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
 *  ======== std.h ========
 *
 *! Revision History
 *! ================
 *! 16-Feb-2004 vp	GNU compiler 3.x defines inline keyword. Added appropriate 
			macros not to redefine inline keyword in this file.
 *! 24-Oct-2002	ashu	defined _TI_ and _FIXED_ symbols for 28x. 
 *! 24-Oct-2002	ashu	defined _TI_ for 24x. 
 *! 01-Mar-2002 kln	changed LARGE_MODEL and Arg definition for 28x
 *! 01-Feb-2002 kln	added definitions for 28x
 *! 08-Dec-2000 kw:	added 'ArgToInt' and 'ArgToPtr' macros
 *! 30-Nov-2000 mf:	Added _64_, _6x_; removed _7d_
 *! 30-May-2000 srid:	Added   __TMS320C55X__ for 55x; Arg is void * for 55 .
 *! 18-Jun-1999 dr:	Added '_TI_', fixed __inline for SUN4, added inline
 *! 10-Feb-1999 rt:	Added '55' support, changed 54's symbol to _TMS320C5XX
 *! 29-Aug-1998 mf: 	fixed typo, removed obsolete targets
 *! 08-Jun-1998 mf: 	_67_ is synonym for _7d_
 *! 10-Oct-1997 rt;	added _7d_ for Raytheon C7DSP triggered by _TMS320C6700
 *! 04-Aug-1997 cc:	added _29_ for _TMS320C2XX
 *! 11-Jul-1997 dlr:	_5t_, and STD_SPOXTASK keyword for Tasking
 *! 12-Jun-1997 mf: 	_TMS320C60 -> _TMS320C6200
 *! 13-Feb-1997 mf:	_62_, with 32-bit LgInt
 *! 26-Nov-1996 kw: 	merged bios-c00's and wsx-a27's <std.h> changes
 *!			*and* revision history 
 *! 12-Sep-1996 kw: 	added C54x #ifdef's
 *! 21-Aug-1996 mf: 	removed #define main smain for _21_
 *! 14-May-1996 gp:     def'd out INT, FLOAT, and COMPLEX defines for WSX.
 *! 11-Apr-1996 kw:     define _W32_ based on _WIN32 (defined by MS compiler)
 *! 07-Mar-1996 mg:     added Win32 support
 *! 06-Sep-1995 dh:	added _77_ dynamic stack support via fxns77.h
 *! 27-Jun-1995 dh:	added _77_ support
 *! 16-Mar-1995 mf: 	for _21_: #define main smain
 *! 01-Mar-1995 mf: 	set _20_ and _60_ (as well as _21_ for both)
 *! 22-Feb-1995 mf: 	Float is float for _SUN_ and _80_
 *! 22-Dec-1994 mf: 	Added _80_ definition, for PP or MP.
 *! 09-Dec-1994 mf: 	Added _53_ definition.
 *! 22-Nov-1994 mf: 	Ptr is void * everywhere.
 *!			Added definitions of _30_, etc.
 *! 23-Aug-1994 dh	removed _21_ special case (kw)
 *! 17-Aug-1994 dh	added _51_ support
 *! 03-Aug-1994 kw	updated _80_ support
 *! 30-Jun-1994 kw	added _80_ support
 *! 05-Apr-1994 kw:	Added _SUN_ to _FLOAT_ definition
 *! 01-Mar-1994 kw: 	Made Bool an int (was MdUns) for _56_ (more efficient).
 *!			Added _53_ support.
 */

#ifndef STD_
#define STD_

#ifdef _TMS320C28X
#define _28_ 1
#ifdef LARGE_MODEL
#define _28l_ 1
#endif
#endif
#ifdef _TMS320C2XX
#define _29_ 1
#endif
#ifdef _TMS320C30
#define _30_ 1
#endif
#ifdef _TMS320C40
#define _40_ 1
#endif
#ifdef _TMS320C50
#define _50_ 1
#endif
#ifdef _TMS320C5XX
#define _54_ 1
#endif
#ifdef __TMS320C55X__
#define _55_ 1
#ifdef __LARGE_MODEL__
#define _55l_ 1
#endif
#endif
#ifdef _TMS320C6200
#define _62_ 1
#define _6x_ 1
#endif
#ifdef _TMS320C6400
#define _64_ 1
#define _6x_ 1
#endif
#ifdef _TMS320C6700
#define _67_ 1
#define _6x_ 1
#endif
#ifdef M_I86
#define _86_ 1
#endif
#ifdef _MVP_MP
#define _80_ 1
#endif
#ifdef _MVP_PP
#define _80_ 1
#endif
#ifdef _WIN32
#define _W32_ 1
#endif

/*
 *  ======== _TI_ ========
 *  _TI_ is defined for all TI targets
 */
#if defined(_29_) || defined(_30_) || defined(_40_) || defined(_50_) || defined(_54_) || defined(_55_) || defined (_6x_) || defined(_80_) || defined (_28_) || defined(_24_)
#define _TI_	1
#endif

/*
 *  ======== _FLOAT_ ========
 *  _FLOAT_ is defined for all targets that natively support floating point
 */
#if defined(_SUN_) || defined(_30_) || defined(_40_) || defined(_67_) || defined(_80_)
#define _FLOAT_	1
#endif

/*
 *  ======== _FIXED_ ========
 *  _FIXED_ is defined for all fixed point target architectures
 */
#if defined(_29_) || defined(_50_) || defined(_54_) || defined(_55_) || defined (_62_) || defined(_64_) || defined (_28_)
#define _FIXED_	1
#endif

/*
 *  ======== _TARGET_ ========
 *  _TARGET_ is defined for all target architectures (as opposed to
 *  host-side software)
 */
#if defined(_FIXED_) || defined(_FLOAT_)
#define _TARGET_ 1
#endif

/*
 *  8, 16, 32-bit type definitions
 *
 *  Sm*	- 8-bit type
 *  Md* - 16-bit type
 *  Lg* - 32-bit type
 *
 *  *Int - signed type
 *  *Uns - unsigned type
 *  *Bits - unsigned type (bit-maps)
 */
typedef char SmInt;		/* SMSIZE-bit signed integer */
typedef short MdInt;		/* MDSIZE-bit signed integer */
#if defined(_6x_)
typedef int LgInt;		/* LGSIZE-bit signed integer */
#else
typedef long LgInt;		/* LGSIZE-bit signed integer */
#endif

typedef unsigned char SmUns;	/* SMSIZE-bit unsigned integer */
typedef unsigned short MdUns;	/* MDSIZE-bit unsigned integer */
#if defined(_6x_)
typedef unsigned LgUns;		/* LGSIZE-bit unsigned integer */
#else
typedef unsigned long LgUns;	/* LGSIZE-bit unsigned integer */
#endif

typedef unsigned char SmBits;	/* SMSIZE-bit bit string */
typedef unsigned short MdBits;	/* MDSIZE-bit bit string */
#if defined(_6x_)
typedef unsigned LgBits;	/* LGSIZE-bit bit string */
#else
typedef unsigned long LgBits;	/* LGSIZE-bit bit string */
#endif

/*
 *  Aliases for standard C types
 */
typedef int Int;		/* for those rare occasions */
typedef long int Long;
typedef short int Short;
typedef char Char;
#define Void void

typedef char *String;		/* pointer to null-terminated character
				 * sequence
				 */

#if defined(_28_) || defined(_29_) || defined(_50_) || defined(_54_) || defined(_55_) || defined(_6x_)
typedef unsigned Uns;
#else
typedef unsigned long Uns;
#endif

#if 0
#if defined(_80_)
typedef int Bool;		/* boolean */
#elif defined(_W32_)
typedef long Bool;		/* boolean to match Windows boolean def */
#else
typedef MdUns Bool;		/* boolean */
#endif
#endif

typedef SmBits Byte;		/* smallest unit of addressable store */
typedef void *Ptr;		/* pointer to arbitrary type */

/* Arg should be size of Ptr */
#if defined(M_I86SM) || defined(_29_) || defined(_50_) || defined(_54_) || defined(_6x_)
typedef Int Arg;
#elif defined(_55_) || defined(_28_)
typedef void *Arg;
#else
typedef LgInt Arg;		/* uninterpreted LGSIZE-bit word */
#endif

typedef Int(*Fxn) ();		/* generic function type */

#if defined(_80_) || defined(_SUN_) || defined(_67_)
typedef float Float;
#else
typedef double Float;
#endif

#ifndef NULL
#define NULL 0
#endif

#if 0
#ifndef TRUE
#define FALSE ((Bool)0)
#define TRUE  ((Bool)1)
#endif
#endif

/*
 * These macros are used to cast 'Arg' types to 'Int' or 'Ptr'.
 * These macros were added for the 55x since Arg is not the same
 * size as Int and Ptr in 55x large model.
 */
#if defined(_28l_) || defined(_55l_)
#define ArgToInt(A)	((Int)((long)(A) & 0xffff))
#define ArgToPtr(A)	((Ptr)(A))
#else
#define ArgToInt(A)	((Int)(A))
#define ArgToPtr(A)	((Ptr)(A))
#endif

/*
 *  ======== __inline ========
 *  The following definitions define the macro __inline for those
 *  C compilers that do not use __inline to indicate inline
 *  expansion of functions.
 *
 *  The TI C compilers support the "inline" keyword (ala C++).  Both 
 *  Microsoft and GNU C compilers support the "__inline" keyword.  The
 *  native SUN OS 4.x C compiler doesn't understand either.
 */
#ifdef _TI_
#ifdef _LINT_
#define __inline
#else
#define __inline inline
#endif
#endif

#ifdef _SUN4_
#define __inline
#endif

/*
 *  ======== inline ========
 *  Define "inline" so that all C code can optionally use the "inline"
 *  keyword. don't define if we are compiling with GNU C compiler version greater than 3.x
 */
#if !defined(inline) && !defined(__cplusplus) && !defined(_TI_)
#if !((__GNUC__ > 3) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1))
#define inline	__inline
#endif
#endif

#endif				/* STD_ */
