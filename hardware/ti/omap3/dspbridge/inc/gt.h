/*
 *  Copyright 2001-2008 Texas Instruments - http://www.ti.com/
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */


/*
 *  ======== gt.h ========
 *  Purpose:
 *      There are two definitions that affect which portions of trace 
 *      are acutally compiled into the client: GT_TRACE and GT_ASSERT. If 
 *      GT_TRACE is set to 0 then all trace statements (except for assertions)
 *      will be compiled out of the client. If GT_ASSERT is set to 0 then 
 *      assertions will be compiled out of the client. GT_ASSERT can not be 
 *      set to 0 unless GT_TRACE is also set to 0 (i.e. GT_TRACE == 1 implies 
 *      GT_ASSERT == 1).
 *
 *! Revision History
 *! ================
 *! 02-Feb-2000 rr: Renamed this file to gtce.h. GT CLASS and trace definitions
 *!                 are WinCE Specific.
 *! 03-Jan-1997	ge	Replaced "GT_" prefix to GT_Config structure members
 *!                 to eliminate preprocessor confusion with other macros.
 */

#ifndef GT_
#define GT_

#ifndef GT_TRACE
#define GT_TRACE 0		/* 0 = "trace compiled out"; 1 = "trace active" */
#endif

#if !defined(GT_ASSERT) || GT_TRACE
#define GT_ASSERT 1
#endif

struct GT_Config {
	Fxn PRINTFXN;
	Fxn PIDFXN;
	Fxn TIDFXN;
	Fxn ERRORFXN;
};

extern struct GT_Config *GT;

struct GT_Mask {
	String modName;
	SmBits *flags;
} ;

/* 
 *  New GT Class defenitions.
 *  
 *  The following are the explanations and how it could be used in the code
 *
 *  -   GT_ENTER    On Entry to Functions
 *
 *  -   GT_1CLASS   Display level of debugging status- Object/Automatic 
 *                  variables
 *  -   GT_2CLASS   ---- do ----
 *
 *  -   GT_3CLASS   ---- do ---- + It can be used(recommended) for debug 
                    status in the ISR, IST
 *  -   GT_4CLASS   ---- do ----
 *
 *  -   GT_5CLASS   Display entry for module init/exit functions
 *
 *  -   GT_6CLASS   Warn whenever SERVICES function fails
 *
 *  -   GT_7CLASS   Warn failure of Critical failures
 *
 */

#define GT_ENTER	((SmBits)0x01)
#define GT_1CLASS	((SmBits)0x02)
#define GT_2CLASS	((SmBits)0x04)
#define GT_3CLASS	((SmBits)0x08)
#define GT_4CLASS	((SmBits)0x10)
#define GT_5CLASS	((SmBits)0x20)
#define GT_6CLASS	((SmBits)0x40)
#define GT_7CLASS	((SmBits)0x80)

#ifdef _LINT_

/* LINTLIBRARY */

/*
 *  ======== GT_assert ========
 */
/* ARGSUSED */
Void
GT_assert(struct GT_Mask mask, Int expr)
{
}

/*
 *  ======== GT_config ========
 */
/* ARGSUSED */
Void
GT_config(struct GT_Config config)
{
}

/*
 *  ======== GT_create ========
 */
/* ARGSUSED */
Void
GT_create(struct GT_Mask * mask /* OUT */ , String modName)
{
}

/*
 *  ======== GT_curLine ========
 *  Purpose: 
 *      Returns the current source code line number. Is useful for performing
 *      branch testing using trace.  For example,
 *
 *      GT_1trace(curTrace, GT_1CLASS,
 *          "in module XX_mod, executing line %u\n", GT_curLine());
 */
/* ARGSUSED */
MdUns
GT_curLine(Void)
{
	return ((MdUns) NULL);
}

/*
 *  ======== GT_exit ========
 */
/* ARGSUSED */
Void
GT_exit(Void)
{
}

/*
 *  ======== GT_init ========
 */
/* ARGSUSED */
Void
GT_init(Void)
{
}

/*
 *  ======== GT_query ========
 */
/* ARGSUSED */
bool
GT_query(struct GT_Mask mask, SmBits class)
{
	return (false);
}

/*
 *  ======== GT_set ========
 *  sets trace mask according to settings
 */

/* ARGSUSED */
Void
GT_set(String settings)
{
}

/*
 *  ======== GT_setprintf ========
 *  sets printf function
 */

/* ARGSUSED */
Void
GT_setprintf(Fxn fxn)
{
}

/* ARGSUSED */
Void
GT_0trace(struct GT_Mask mask, SmBits class, String format)
{
}

/* ARGSUSED */
Void
GT_1trace(struct GT_Mask mask, SmBits class, String format, ...)
{
}

/* ARGSUSED */
Void
GT_2trace(struct GT_Mask mask, SmBits class, String format, ...)
{
}

/* ARGSUSED */
Void
GT_3trace(struct GT_Mask mask, SmBits class, String format, ...)
{
}

/* ARGSUSED */
Void
GT_4trace(struct GT_Mask mask, SmBits class, String format, ...)
{
}

/* ARGSUSED */
Void
GT_5trace(struct GT_Mask mask, SmBits class, String format, ...)
{
}

/* ARGSUSED */
Void
GT_6trace(struct GT_Mask mask, SmBits class, String format, ...)
{
}

#else

#define	GT_BOUND    26		/* 26 letters in alphabet */

extern Void _GT_create(struct GT_Mask * mask, String modName);

#define GT_exit()

extern Void GT_init(Void);
extern Void _GT_set(String str);
extern Int _GT_trace(struct GT_Mask * mask, String format, ...);

#if GT_ASSERT == 0

#define GT_assert( mask, expr )
#define GT_config( config )
#define GT_configInit( config )
#define GT_seterror( fxn )

#else

extern struct GT_Config _GT_params;

#define GT_assert( mask, expr ) \
	(!(expr) ? \
	    (*GT->ERRORFXN)("assertion violation: %s, line %d\n", \
			    __FILE__, __LINE__), NULL : NULL)

#define GT_config( config )     (_GT_params = *(config))
#define GT_configInit( config ) (*(config) = _GT_params)
#define GT_seterror( fxn )      (_GT_params.ERRORFXN = (Fxn)(fxn))

#endif

#if GT_TRACE == 0

#define GT_curLine()                ((MdUns)__LINE__)
#define GT_create(mask, modName)
#define GT_exit()
#define GT_init()
#define GT_set( settings )
#define GT_setprintf( fxn )

#define GT_query( mask, class )     false

#define GT_0trace( mask, class, format )
#define GT_1trace( mask, class, format, arg1 )
#define GT_2trace( mask, class, format, arg1, arg2 )
#define GT_3trace( mask, class, format, arg1, arg2, arg3 )
#define GT_4trace( mask, class, format, arg1, arg2, arg3, arg4 )
#define GT_5trace( mask, class, format, arg1, arg2, arg3, arg4, arg5 )
#define GT_6trace( mask, class, format, arg1, arg2, arg3, arg4, arg5, arg6 )

#else				/* GT_TRACE == 1 */

extern String GT_format;
extern SmBits *GT_tMask[GT_BOUND];

#define GT_create(mask, modName)    _GT_create((mask), (modName))
#define GT_curLine()                ((MdUns)__LINE__)
#define GT_set( settings )          _GT_set( settings )
#define GT_setprintf( fxn )         (_GT_params.PRINTFXN = (Fxn)(fxn))

#define GT_query( mask, class ) ((*(mask).flags & (class)))

#define GT_0trace( mask, class, format ) \
    ((*(mask).flags & (class)) ? \
    _GT_trace(&(mask), (format)) : 0)

#define GT_1trace( mask, class, format, arg1 ) \
    ((*(mask).flags & (class)) ? \
    _GT_trace(&(mask), (format), (arg1)) : 0)

#define GT_2trace( mask, class, format, arg1, arg2 ) \
    ((*(mask).flags & (class)) ? \
    _GT_trace(&(mask), (format), (arg1), (arg2)) : 0)

#define GT_3trace( mask, class, format, arg1, arg2, arg3 ) \
    ((*(mask).flags & (class)) ? \
    _GT_trace(&(mask), (format), (arg1), (arg2), (arg3)) : 0)

#define GT_4trace( mask, class, format, arg1, arg2, arg3, arg4 ) \
    ((*(mask).flags & (class)) ? \
    _GT_trace(&(mask), (format), (arg1), (arg2), (arg3), (arg4)) : 0)

#define GT_5trace( mask, class, format, arg1, arg2, arg3, arg4, arg5 ) \
    ((*(mask).flags & (class)) ? \
    _GT_trace(&(mask), (format), (arg1), (arg2), (arg3), (arg4), (arg5)) : 0)

#define GT_6trace( mask, class, format, arg1, arg2, arg3, arg4, arg5, arg6 ) \
    ((*(mask).flags & (class)) ? \
    _GT_trace(&(mask), (format), (arg1), (arg2), (arg3), (arg4), (arg5), \
	(arg6)) : 0)

#endif				/* GT_TRACE */

#endif				/* _LINT_ */

#endif				/* GTCE_ */

