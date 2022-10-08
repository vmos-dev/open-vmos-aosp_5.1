
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
/* =============================================================================
*             Texas Instruments OMAP(TM) Platform Software
*  (c) Copyright Texas Instruments, Incorporated.  All Rights Reserved.
*
*  Use of this software is controlled by the terms and conditions found 
*  in the license agreement under which this software has been supplied.
* =========================================================================== */
/**
* @file OMX_TI_Debug.h
*
* This file implements the TI-specific OMX Debug interface 
*
* @path  $(CSLPATH)\inc
*
* @rev  0.1
*/
/* -------------------------------------------------------------------------- */
/* ============================================================================= 
*! 
*! Revision History 
*! ================================================================
*!
*! 10-Mar-2009 lm: Revisions appear in reverse chronological order; 
*! that is, newest first.  The date format is dd-Mon-yyyy.
* =========================================================================== */

#ifndef OMX_TI_DEBUG__H
#define OMX_TI_DEBUG__H

#include <ctype.h>
#include <utils/Log.h>
/* 
    The OMX TI Debug interface allows debug messages to be classified by
    domain and importance.  There are no preset classifications required, but
    rather these are arbitrary.  Nonetheless, all modules using this interface
    must adhere to these domain and level conventions.

    While it would make sense that errors should be printed at the highest
    level, we separated errors into their own domain, as even error printing
    could have additional details that are not always desirable to print.

    In order to facilitate using the debug interface, a set of macros are
    defined for the common domains and levels.

    Features of the TI Debug interface:

    - debug messages are maskable: only messages for domains marked, and that
      have levels at least of the mask's level will be printed
    - there are two output files supported.  Error domain output is going to
      the error file, whereas other outputs usually go into the other output
      file.  This also allows only printing error messages.
    - colors can be specified for each domain - through this file or the
      macros
    - line number information can be added (via a compile time option)

    Compile time flags:

    __OMX_DEBUG__      - enable OMX Debug print calls
    __OMX_DBG_COLOR__  - enable color output
    __OMX_DBG_FN__     - print function
    __OMX_DBG_FILE__   - print file
    __OMX_DBG_LINE__   - print line number
    __OMX_DBG_LEVEL__  - print level
    __OMX_DBG_DOMAIN__ - print domain
    __OMX_DBG_4ISERROR__ - print level 4 messages into the error file

    __OMX_DBG_ANDROID__ - always outputs to Android log.  Otherwise, Android
                          log is only used if output is stderr or stdout.

    Output format is:
    file:function():line domain<level> message

*/

/*  You can change this file here to globally set the debug configurations

 */
#define __OMX_DEBUG__
#undef  __OMX_DBG_COLOR__
#undef  __OMX_DBG_FILE__
#define __OMX_DBG_FN__
#define __OMX_DBG_LINE__
#undef  __OMX_DBG_DOMAIN__
#undef  __OMX_DBG_LEVEL__
#undef  __OMX_DBG_4ISERROR__
#undef  __OMX_DBG_ANDROID__

/*
 *  OMX Debug levels specify the importance of the debug print
 *
 *  0 - very verbose      -- any details, such as system return values, etc.
 *  1 - quite verbose     -- details
 *  2 - verbose           -- main details
 *  3 - normal            -- what an average user would want to see
 *  4 - quiet             -- only important messages -- this would not be
 *                           normally used by OMX components except for error
 *                           messages as they don't output anything normally.
 *                           This could be used by applications and tests.
 *  5 - very quiet        -- crucial/error messages - goes to error file
 */

/* this is used as a bitmask currently, so must be 2^n-1 */
#define OMX_DBG_MAX_LEVEL 0x0F

/*
 *  OMX Debug domains specify the system that the messages relate to
 */
#define OMX_DBG_DOM_ERROR  0x1ul         /* an error */
#define OMX_DBG_DOM_PRINT  0x10ul        /* generic prints */
#define OMX_DBG_DOM_SYSTEM 0x100ul       /* generic OS systems: memory, etc. */
#define OMX_DBG_DOM_STATE  0x1000ul      /* state transitions */
#define OMX_DBG_DOM_BUFFER 0x10000ul     /* buffer transfer between */
#define OMX_DBG_DOM_DSP    0x100000ul    /* DSP communication/transfer */
#define OMX_DBG_DOM_COMM   0x1000000ul   /* communication */
#define OMX_DBG_DOM_MGR    0x10000000ul  /* communication with managers */

#define OMX_DBG_BASEMASK   0x33333333ul  /* all domains at normal verbosity */

/*
 *  Generic DEBUG Settings
 */
struct OMX_TI_Debug
{
    FILE *out;    /* standard output file */
    FILE *err;    /* error output file - used for the error domain
                     and crucial messages */
    FILE *out_opened;  /* opened output file */
    FILE *err_opened;  /* opened error file */
    OMX_U32 mask; /* debug mask */
};

/*
 *  :TRICKY: do { } while (0) makes this behave like any other C function call
 *           even though it contains several statements
 */

/* This macro sets the debug mask in the debug structure based on a mask
   string */
#define OMX_DBG_SET_MASK(dbg, mask_str) \
    do { \
        if (mask_str) { \
            sscanf((mask_str), "%lx", &(dbg).mask); \
            if (strlen(mask_str) < 8) { \
                (dbg).mask &= ~0ul >> (32 - 4 * strlen(mask_str)); \
                (dbg).mask |= OMX_DBG_BASEMASK << (4 * strlen(mask_str)); \
            } \
        } else { \
            (dbg).mask = OMX_DBG_BASEMASK; \
        } \
    } while (0)

/* This macro initializes the debug structure.  Default configuration is
   stdout/stderr output and base mask. */
#define OMX_DBG_INIT_BASE(dbg) \
    do { \
        (dbg).err = stderr; \
        (dbg).out = stdout; \
        (dbg).out_opened = (dbg).err_opened = NULL; \
        (dbg).mask = OMX_DBG_BASEMASK; \
    } while (0)

/* This macro initializes the debug structure.  Default configuration is
   stdout/stderr output and base mask.  If the OMX_DBG_CONFIG environment
   variable is defined, the file given by this variable is opened and each line
   is compared to the tag string followed by _OUT, _ERR or _LEVEL to specify
   the output and error file names, or the debug level.  If the debug level
   string is less than 8 hexadecimal digits, the remaining domain mask levels
   are set to the base mask (3). */
#define OMX_DBG_INITALL(dbg, tag_str, file) \
    do { \
        (dbg).err = stderr; \
        (dbg).out = stdout; \
        (dbg).out_opened = (dbg).err_opened = NULL; \
        (dbg).mask = OMX_DBG_BASEMASK; \
        char *config_file = getenv("OMX_DBG_CONFIG"); \
        FILE *config = (config_file && *config_file) ? fopen(config_file, "rt") : NULL; \
        if (config) { \
            char line[80], *ptr, *end, out[75], err[75]; \
            out[0] = err[0] = '\0'; \
            while (fgets(line, sizeof(line), config)) { \
                for (ptr = line; isspace(*ptr); ptr++); \
                if (!strncmp(ptr, tag_str, strlen(tag_str))) { \
                    ptr += strlen(tag_str); \
                    end = ptr + strlen(ptr); \
                    while (end > ptr && isspace(end[-1])) *--end = '\0'; \
                    if (file && !strncmp(ptr, "_ERR", 4) && isspace(ptr[4])) { \
                        ptr += 4; \
                        while (isspace(*ptr)) ptr++; \
                        strcpy(err, ptr); \
                    } else if (file && !strncmp(ptr, "_OUT", 4) && isspace(ptr[4])) { \
                        ptr += 4; \
                        while (isspace(*ptr)) ptr++; \
                        strcpy(out, ptr); \
                    } else if (!strncmp(ptr, "_LEVEL", 6) && isspace(ptr[6])) { \
                        ptr += 6; \
                        while (isspace(*ptr)) ptr++; \
                        OMX_DBG_SET_MASK(dbg, ptr); \
                    } \
                } \
            } \
            if (file) { \
                if (!strcmp(out, "stdout")) {} \
                else if (!strcmp(out, "stderr")) (dbg).out = stderr; \
                else if (!strcmp(out, "null")) (dbg).out = NULL; \
                else if (*out) (dbg).out_opened = (dbg).out = fopen(out, "w"); \
                if (!strcmp(err, "stderr")) {} \
                else if (!strcmp(err, "stdout")) (dbg).err = stdout; \
                else if (!strcmp(err, "null")) (dbg).err = NULL; \
                else if (!strcmp(err, out)) (dbg).err = (dbg).out; \
                else if (*err) (dbg).err_opened = (dbg).err = fopen(err, "w"); \
            } \
            fclose(config); \
        } \
    } while (0)


#define OMX_DBG_INIT(dbg, tag_str)      OMX_DBG_INITALL(dbg, tag_str, 1)
#define OMX_DBG_INIT_MASK(dbg, tag_str) OMX_DBG_INITALL(dbg, tag_str, 0)


/* Use this macro to copy a OMX_TI_Debug config received. */
#define OMX_DBG_SETCONFIG(dbg, pConfig) \
    do { \
        struct OMX_TI_Debug *pConfDbg = (struct OMX_TI_Debug *) pConfig; \
        (dbg).mask = pConfDbg->mask; \
        (dbg).out = pConfDbg->out; \
        (dbg).err = pConfDbg->err; \
    } while (0)

/* Use this macro to copy a OMX_TI_Debug config to be sent. */
#define OMX_DBG_GETCONFIG(dbg, pConfig) \
    do { \
        struct OMX_TI_Debug *pConfDbg = (struct OMX_TI_Debug *) pConfig; \
        pConfDbg->mask = dbg.mask; \
        pConfDbg->out = dbg.out; \
        pConfDbg->err = dbg.err; \
        pConfDbg->err_opened = pConfDbg->out_opened = NULL; \
    } while (0)

/* Use this macro to close any opened file that was opened by OMX_DBG. */
#define OMX_DBG_CLOSE(dbg) \
    do { \
        if ((dbg).err_opened) { \
            if ((dbg).err == (dbg).err_opened) (dbg).err = stderr; \
            fclose((dbg).err_opened); \
        } \
        if ((dbg).out_opened) { \
            if ((dbg).out == (dbg).out_opened) (dbg).out = stdout; \
            if ((dbg).err == (dbg).out_opened) (dbg).err = stderr; \
            fclose((dbg).out_opened); \
        } \
        (dbg).err_opened = (dbg).out_opened = NULL; \
    } while (0)

#ifdef __OMX_DEBUG__

/*
 *  GENERIC OMX TI DEBUG STATEMENT
 *
 *  file   - output of the debug message
 *  level  - level of the debug message
 *  domain - domain of the debug message
 *  mask   - debug print mask, only messages for domains marked, and that have
 *           levels at least of the mask's level will be printed
 *  format, list - debug message
 */

#ifdef ANDROID
#ifdef __OMX_DEBUG_ANDROID__
    #define OMX_DBG_PRINT(file, domain, level, mask, format, list...) \
        do {\
            if ((file) && (OMX_U32) (level * domain) >= (OMX_U32) ((mask) & (domain * OMX_DBG_MAX_LEVEL))) \
                ALOGD(OMX_DBG_FN_FMT OMX_DBG_LINE_FMT OMX_DBG_FMT \
                    format OMX_DBG_FN OMX_DBG_LINE, ##list); \
        } while (0)
#else
    #define OMX_DBG_PRINT(file, domain, level, mask, format, list...) \
        do {\
            if ((file) && (OMX_U32) (level * domain) >= (OMX_U32) ((mask) & (domain * OMX_DBG_MAX_LEVEL))) { \
                if (file == stderr || file == stdout) { \
                    ALOGD(OMX_DBG_FN_FMT OMX_DBG_LINE_FMT OMX_DBG_FMT \
                        format OMX_DBG_FN OMX_DBG_LINE, ##list); \
                } else { \
                fprintf((file), \
                        OMX_DBG_FILE OMX_DBG_FN_FMT OMX_DBG_LINE_FMT OMX_DBG_FMT \
                        format OMX_DBG_FN OMX_DBG_LINE, ##list); \
                } \
            } \
        } while (0)
#endif
#else
    #define OMX_DBG_PRINT(file, domain, level, mask, format, list...) \
        do {\
            if ((file) && (OMX_U32) (level * domain) >= (OMX_U32) ((mask) & (domain * OMX_DBG_MAX_LEVEL))) \
                fprintf((file), \
                        OMX_DBG_FILE OMX_DBG_FN_FMT OMX_DBG_LINE_FMT OMX_DBG_FMT \
                        format OMX_DBG_FN OMX_DBG_LINE, ##list); \
        } while (0)
#endif

/*  Alternate mask understanding implementation.  Use this if we cannot specify a
    level for each domain */
/*
    #define OMX_DBG_PRINT(file, domain, level, mask, format, list...) \
        do {\
            if (file && (mask & domain) && level >= (mask & OMX_MASK_LEVEL)) \
                fprintf(file, \
                        OMX_DBG_FILE OMX_DBG_FN_FMT OMX_DBG_LINE_FMT OMX_DBG_FMT \
                        format, OMX_DBG_FN OMX_DBG_LINE ##list); \
        } while (0)
*/

#else
/* empty definitions */
    #define OMX_DBG_PRINT(file, domain, level, mask, format, list...)
#endif

/*
 *  OMX BAIL Shortcut macros
 */
#define OMX_DBG_BAIL_IF_ERROR(_eError, dbg, cmd, format, list...) \
    do { \
        if (_eError) { \
            cmd(dbg, format, ##list); \
            OMX_CONF_BAIL_IF_ERROR(_eError); \
        } \
    } while (0)

#define OMX_DBG_SET_ERROR_BAIL(_eError, _eErrorValue, dbg, cmd, format, list...) \
    do { \
        cmd(dbg, format, ##list); \
        OMX_CONF_SET_ERROR_BAIL(_eError, _eErrorValue); \
    } while (0)

#define OMX_DBG_CHECK_CMD(dbg, _ptr1, _ptr2, _ptr3) \
    do { \
        if(!_ptr1) OMX_ERROR4(dbg, "NULL parameter (" #_ptr1 ").\n"); \
        else if(!_ptr2) OMX_ERROR4(dbg, "NULL parameter (" #_ptr2 ").\n"); \
        else if(!_ptr3) OMX_ERROR4(dbg, "NULL parameter (" #_ptr3 ").\n"); \
        OMX_CONF_CHECK_CMD(_ptr1, _ptr2, _ptr3); \
    } while (0)


/*
 *  GENERIC OMX TI DEBUG Shortcut macros
 */

/* This variant uses a shortcut for the domain notation, but is not traceable
   by source understanders.  It also adds the color to the format string. */
#define OMXDBG_PRINT(file, domain, level, mask, format, list...) \
   OMX_DBG_PRINT(file, OMX_DBG_DOM_##domain, level, mask, \
                 OMX_DBG_COL_##domain OMX_DBG_DOM_##domain##_STR OMX_DBG_LEVEL(level) format OMX_DBG_COL_WHITE, ##list)

/* Shortcuts */
#define OMX_ERROR0(dbg, format, list...) OMXDBG_PRINT((dbg).err, ERROR, 0, (dbg).mask, format, ##list)
#define OMX_ERROR1(dbg, format, list...) OMXDBG_PRINT((dbg).err, ERROR, 1, (dbg).mask, format, ##list)
#define OMX_ERROR2(dbg, format, list...) OMXDBG_PRINT((dbg).err, ERROR, 2, (dbg).mask, format, ##list)
#define OMX_ERROR3(dbg, format, list...) OMXDBG_PRINT((dbg).err, ERROR, 3, (dbg).mask, format, ##list)
#define OMX_ERROR4(dbg, format, list...) OMXDBG_PRINT((dbg).err, ERROR, 4, (dbg).mask, format, ##list)
#define OMX_ERROR5(dbg, format, list...) OMXDBG_PRINT((dbg).err, ERROR, 5, (dbg).mask, format, ##list)
#define OMX_TRACE0(dbg, format, list...) OMXDBG_PRINT((dbg).out, SYSTEM, 0, (dbg).mask, format, ##list)
#define OMX_TRACE1(dbg, format, list...) OMXDBG_PRINT((dbg).out, SYSTEM, 1, (dbg).mask, format, ##list)
#define OMX_TRACE2(dbg, format, list...) OMXDBG_PRINT((dbg).out, SYSTEM, 2, (dbg).mask, format, ##list)
#define OMX_TRACE3(dbg, format, list...) OMXDBG_PRINT((dbg).out, SYSTEM, 3, (dbg).mask, format, ##list)
#define OMX_TRACE4(dbg, format, list...) OMXDBG_PRINT((dbg).OMX_DBG_LEVEL4, SYSTEM, 4, (dbg).mask, format, ##list)
#define OMX_TRACE5(dbg, format, list...) OMXDBG_PRINT((dbg).err, SYSTEM, 5, (dbg).mask, format, ##list)
#define OMX_PRINT0(dbg, format, list...) OMXDBG_PRINT((dbg).out, PRINT, 0, (dbg).mask, format, ##list)
#define OMX_PRINT1(dbg, format, list...) OMXDBG_PRINT((dbg).out, PRINT, 1, (dbg).mask, format, ##list)
#define OMX_PRINT2(dbg, format, list...) OMXDBG_PRINT((dbg).out, PRINT, 2, (dbg).mask, format, ##list)
#define OMX_PRINT3(dbg, format, list...) OMXDBG_PRINT((dbg).out, PRINT, 3, (dbg).mask, format, ##list)
#define OMX_PRINT4(dbg, format, list...) OMXDBG_PRINT((dbg).OMX_DBG_LEVEL4, PRINT, 4, (dbg).mask, format, ##list)
#define OMX_PRINT5(dbg, format, list...) OMXDBG_PRINT((dbg).err, PRINT, 5, (dbg).mask, format, ##list)
#define OMX_PRBUFFER0(dbg, format, list...) OMXDBG_PRINT((dbg).out, BUFFER, 0, (dbg).mask, format, ##list)
#define OMX_PRBUFFER1(dbg, format, list...) OMXDBG_PRINT((dbg).out, BUFFER, 1, (dbg).mask, format, ##list)
#define OMX_PRBUFFER2(dbg, format, list...) OMXDBG_PRINT((dbg).out, BUFFER, 2, (dbg).mask, format, ##list)
#define OMX_PRBUFFER3(dbg, format, list...) OMXDBG_PRINT((dbg).out, BUFFER, 3, (dbg).mask, format, ##list)
#define OMX_PRBUFFER4(dbg, format, list...) OMXDBG_PRINT((dbg).OMX_DBG_LEVEL4, BUFFER, 4, (dbg).mask, format, ##list)
#define OMX_PRBUFFER5(dbg, format, list...) OMXDBG_PRINT((dbg).err, BUFFER, 5, (dbg).mask, format, ##list)
#define OMX_PRMGR0(dbg, format, list...) OMXDBG_PRINT((dbg).out, MGR, 0, (dbg).mask, format, ##list)
#define OMX_PRMGR1(dbg, format, list...) OMXDBG_PRINT((dbg).out, MGR, 1, (dbg).mask, format, ##list)
#define OMX_PRMGR2(dbg, format, list...) OMXDBG_PRINT((dbg).out, MGR, 2, (dbg).mask, format, ##list)
#define OMX_PRMGR3(dbg, format, list...) OMXDBG_PRINT((dbg).out, MGR, 3, (dbg).mask, format, ##list)
#define OMX_PRMGR4(dbg, format, list...) OMXDBG_PRINT((dbg).OMX_DBG_LEVEL4, MGR, 4, (dbg).mask, format, ##list)
#define OMX_PRMGR5(dbg, format, list...) OMXDBG_PRINT((dbg).err, MGR, 5, (dbg).mask, format, ##list)
#define OMX_PRDSP0(dbg, format, list...) OMXDBG_PRINT((dbg).out, DSP, 0, (dbg).mask, format, ##list)
#define OMX_PRDSP1(dbg, format, list...) OMXDBG_PRINT((dbg).out, DSP, 1, (dbg).mask, format, ##list)
#define OMX_PRDSP2(dbg, format, list...) OMXDBG_PRINT((dbg).out, DSP, 2, (dbg).mask, format, ##list)
#define OMX_PRDSP3(dbg, format, list...) OMXDBG_PRINT((dbg).out, DSP, 3, (dbg).mask, format, ##list)
#define OMX_PRDSP4(dbg, format, list...) OMXDBG_PRINT((dbg).OMX_DBG_LEVEL4, DSP, 4, (dbg).mask, format, ##list)
#define OMX_PRDSP5(dbg, format, list...) OMXDBG_PRINT((dbg).err, DSP, 5, (dbg).mask, format, ##list)
#define OMX_PRCOMM0(dbg, format, list...) OMXDBG_PRINT((dbg).out, COMM, 0, (dbg).mask, format, ##list)
#define OMX_PRCOMM1(dbg, format, list...) OMXDBG_PRINT((dbg).out, COMM, 1, (dbg).mask, format, ##list)
#define OMX_PRCOMM2(dbg, format, list...) OMXDBG_PRINT((dbg).out, COMM, 2, (dbg).mask, format, ##list)
#define OMX_PRCOMM3(dbg, format, list...) OMXDBG_PRINT((dbg).out, COMM, 3, (dbg).mask, format, ##list)
#define OMX_PRCOMM4(dbg, format, list...) OMXDBG_PRINT((dbg).OMX_DBG_LEVEL4, COMM, 4, (dbg).mask, format, ##list)
#define OMX_PRCOMM5(dbg, format, list...) OMXDBG_PRINT((dbg).err, COMM, 5, (dbg).mask, format, ##list)
#define OMX_PRSTATE0(dbg, format, list...) OMXDBG_PRINT((dbg).out, STATE, 0, (dbg).mask, format, ##list)
#define OMX_PRSTATE1(dbg, format, list...) OMXDBG_PRINT((dbg).out, STATE, 1, (dbg).mask, format, ##list)
#define OMX_PRSTATE2(dbg, format, list...) OMXDBG_PRINT((dbg).out, STATE, 2, (dbg).mask, format, ##list)
#define OMX_PRSTATE3(dbg, format, list...) OMXDBG_PRINT((dbg).out, STATE, 3, (dbg).mask, format, ##list)
#define OMX_PRSTATE4(dbg, format, list...) OMXDBG_PRINT((dbg).OMX_DBG_LEVEL4, STATE, 4, (dbg).mask, format, ##list)
#define OMX_PRSTATE5(dbg, format, list...) OMXDBG_PRINT((dbg).err, STATE, 5, (dbg).mask, format, ##list)

/*
 *  Compile Time Customizations
 */

/*
 *  Context
 */
#ifdef __OMX_DBG_FN__
    #define OMX_DBG_FN_FMT "%s()"
    #define OMX_DBG_FN , __FUNCTION__
#else
    #define OMX_DBG_FN_FMT
    #define OMX_DBG_FN
#endif

#ifdef __OMX_DBG_LINE__
    #define OMX_DBG_LINE_FMT ":%d"
    #define OMX_DBG_LINE , __LINE__
#else
    #define OMX_DBG_LINE_FMT
    #define OMX_DBG_LINE
#endif

#ifdef __OMX_DBG_FILE__
    #ifdef __OMX_DBG_FN__
        #define OMX_DBG_FILE __FILE__ ":"
    #else
        #define OMX_DBG_FILE __FILE__
    #endif
#else
    #define OMX_DBG_FILE
#endif

#if defined(__OMX_DBG_LEVEL__)
    #define OMX_DBG_LEVEL(level) "<" #level "> "
#elif defined(__OMX_DBG_DOMAIN__)
    #define OMX_DBG_LEVEL(level) " "
#else
    #define OMX_DBG_LEVEL(level)
#endif

#ifdef __OMX_DBG_DOMAIN__
    #define OMX_DBG_DOM_ERROR_STR  "ERR"
    #define OMX_DBG_DOM_BUFFER_STR "BUF"
    #define OMX_DBG_DOM_COMM_STR   "COM"
    #define OMX_DBG_DOM_DSP_STR    "DSP"
    #define OMX_DBG_DOM_SYSTEM_STR "SYS"
    #define OMX_DBG_DOM_MGR_STR    "MGR"
    #define OMX_DBG_DOM_STATE_STR  "STA"
    #define OMX_DBG_DOM_PRINT_STR  "GEN"
#else
    #define OMX_DBG_DOM_ERROR_STR
    #define OMX_DBG_DOM_BUFFER_STR
    #define OMX_DBG_DOM_COMM_STR
    #define OMX_DBG_DOM_DSP_STR
    #define OMX_DBG_DOM_SYSTEM_STR
    #define OMX_DBG_DOM_MGR_STR
    #define OMX_DBG_DOM_STATE_STR
    #define OMX_DBG_DOM_PRINT_STR
#endif

#ifdef __OMX_DBG_4ISERROR__
    #define OMX_DBG_LEVEL4 err
#else
    #define OMX_DBG_LEVEL4 out
#endif

#if defined(__OMX_DBG_LINE__) || defined(__OMX_DBG_FN__) || defined(__OMX_DBG_FILE__)
    #define OMX_DBG_FMT " "
#else
    #define OMX_DBG_FMT
#endif

/*
 *  Color (ANSI escape sequences)
 */
#ifdef __OMX_DBG_COLOR__
    #define OMX_DBG_COL_RED     "\x1b[1;31;40m"
    #define OMX_DBG_COL_GREEN   "\x1b[1;32;40m"
    #define OMX_DBG_COL_YELLOW  "\x1b[1;33;40m"
    #define OMX_DBG_COL_BLUE    "\x1b[1;34;40m"
    #define OMX_DBG_COL_MAGENTA "\x1b[1;35;40m" 
    #define OMX_DBG_COL_CYAN    "\x1b[1;36;40m"
    #define OMX_DBG_COL_WHITE   "\x1b[1;37;40m"
#else
    #define OMX_DBG_COL_BLUE
    #define OMX_DBG_COL_CYAN
    #define OMX_DBG_COL_GREEN
    #define OMX_DBG_COL_MAGENTA
    #define OMX_DBG_COL_RED
    #define OMX_DBG_COL_WHITE
    #define OMX_DBG_COL_YELLOW
#endif

/*
 *  Domain colors - these can be predefined differently
 */

#ifndef OMX_DBG_COL_ERROR
#define OMX_DBG_COL_ERROR OMX_DBG_COL_RED
#endif
#ifndef OMX_DBG_COL_SYSTEM
#define OMX_DBG_COL_SYSTEM OMX_DBG_COL_WHITE
#endif
#ifndef OMX_DBG_COL_PRINT
#define OMX_DBG_COL_PRINT OMX_DBG_COL_WHITE
#endif
#ifndef OMX_DBG_COL_BUFFER
#define OMX_DBG_COL_BUFFER OMX_DBG_COL_GREEN
#endif
#ifndef OMX_DBG_COL_MGR
#define OMX_DBG_COL_MGR OMX_DBG_COL_MAGENTA
#endif
#ifndef OMX_DBG_COL_DSP
#define OMX_DBG_COL_DSP OMX_DBG_COL_CYAN
#endif
#ifndef OMX_DBG_COL_COMM
#define OMX_DBG_COL_COMM OMX_DBG_COL_YELLOW
#endif
#ifndef OMX_DBG_COL_STATE
#define OMX_DBG_COL_STATE OMX_DBG_COL_BLUE
#endif

#ifdef OMX_MEMDEBUG
#define mem_array_size=500;

void *arr[mem_array_size];
int lines[mem_array_size];
int bytes[mem_array_size];
char file[mem_array_size][50];

#define newmalloc(x) mymalloc(__LINE__,__FILE__,x)
#define newfree(z) myfree(z,__LINE__,__FILE__)

void * mymalloc(int line, char *s, int size);
int myfree(void *dp, int line, char *s);

void * mymalloc(int line, char *s, int size)
{
   void *p;    
   int e=0;
   p = malloc(size);
   if(p==NULL){
       OMXDBG_PRINT(stderr, ERROR, 4, 0, "Memory not available\n");
       /* ddexit(1); */
       }
   else{
         while((lines[e]!=0)&& (e<(mem_array_size - 1)) ){
              e++;
         }
         arr[e]=p;
         lines[e]=line;
         bytes[e]=size;
         strcpy(file[e],s);
         OMXDBG_PRINT(stderr, BUFFER, 2, 0, 
            "Allocating %d bytes on address %p, line %d file %s pos %d\n", size, p, line, s, e);
   }
   return p;
}

int myfree(void *dp, int line, char *s){
    int q;
    for(q=0;q<mem_array_size;q++){
        if(arr[q]==dp){
           OMXDBG_PRINT(stderr, PRINT, 2, 0, "Deleting %d bytes on address %p, line %d file %s\n",
                   bytes[q],dp, line, s);
           free(dp);
           dp = NULL;
           lines[q]=0;
           strcpy(file[q],"");
           break;
        }            
     }    
     if(mem_array_size==q){
         OMXDBG_PRINT(stderr, PRINT, 2, 0, "\n\nPointer not found. Line:%d    File%s!!\n\n",line, s);
     }
}

#else

#define newmalloc(x) malloc(x)
#define newfree(z) free(z)

#endif


#endif
