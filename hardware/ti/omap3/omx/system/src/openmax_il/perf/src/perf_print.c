
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
#ifdef __PERF_CUSTOMIZABLE__

    #define __PERF_PRINT_C__

    #include "perf_config.h"
    #include "perf.h"
    #include "perf_print.h"

/* ============================================================================
   DEBUG PRINT METHODS
============================================================================ */

void PERF_PRINT_done(PERF_Private *perf) 
{
    PERF_PRINT_Private *me = perf->cip.pDebug;

    /* close debug file unless stdout or stderr */
    if (me->fDebug && me->fDebug != stdout &&
        me->fDebug != stderr) fclose(me->fDebug);

    /* free allocated strings */
    free(me->info);    me->info = NULL;

    /* free private structure */
    free(me);
    perf->cip.pDebug = NULL;
}

char * const domains_normal[] = { "AD,", "VD,", "ID,", "AE,", "VE,", "IE,"};
char * const domains_csv[]    = { "+AD", "+VD", "+ID", "+AE", "+VE", "+IE"};

static
void print_header(FILE *fOut)
{
    fprintf(fOut, "time,PID,handle,name,domAD,domVD,domID,domAE,domVE,domIE,"
            "type,operation\n");
}

int PERF_PRINT_setup(PERF_Private *perf, PERF_MODULETYPE eModule)
{
    PERF_PRINT_Private *me = perf->cip.pDebug;
    /* we concatenate the following fields for a unique and informative ID:
       PID, address, domain, type */

    /* common variables that differ from CSV and text output */
    char const * const * domains, *missing, *separator;

    /* data needed for info field */
    unsigned long type = eModule & PERF_ModuleMask;

    /* set up common variables */
    if (me->csv)
    {
        domains = (char const * const *) domains_normal;
        missing = separator = ",";
        me->prompt = "";
        print_header(me->fDebug);
    }
    else
    {
        domains = (char const * const *) domains_csv;
        missing = "";
        separator = ", ";
        me->prompt = "<";
    }

    me->info = (char *) malloc(3+9+1+8+2+6+4+2+7+12+8+16+2 + 100);
    if (me->info)
    {

        sprintf(me->info,
                "%s"           /* info separator start */
                "%ld"          /* PID */
                "%s"           /* separator */
                "%08lx"        /* handle */
                "%s"           /* separator */
                "%s"           /* name tag */            
                "%c%c%c%c"     /* name (fourcc) */
                "%s"           /* separator */
                "%s"           /* domain tag */
                "%s%s%s%s%s%s" /* domain */
                "%s"           /* module tag */
                "%s"           /* module */
                "%s"           /* info separator end */
                ,
                me->csv ? separator : "> {",
                perf->ulPID,
                me->csv ? separator : "-",
                (unsigned long) perf,
                separator,
                /* name tag */
                me->csv ? "" : "name: ",
                /* name (fourcc) */
                PERF_FOUR_CHARS(perf->ulID),
                separator,
                /* domain tag */
                me->csv ? "" : "domain: ",
                /* domain */
                (eModule & PERF_ModuleAudioDecode) ? domains[0] : missing,
                (eModule & PERF_ModuleVideoDecode) ? domains[1] : missing,
                (eModule & PERF_ModuleImageDecode) ? domains[2] : missing,
                (eModule & PERF_ModuleAudioEncode) ? domains[3] : missing,
                (eModule & PERF_ModuleVideoEncode) ? domains[4] : missing,
                (eModule & PERF_ModuleImageEncode) ? domains[5] : missing,
                /* module tag */
                me->csv ? "" : ", module: ",  /* note: separator added for CSV */
                /* module */
                (type < PERF_ModuleMax) ? PERF_ModuleTypes[type] : "INVALID",
                /* info separator end */
                me->csv ? separator : "} ");
    }

    /* we succeed if we could allocate the info string */
    return(me->info != NULL);
}

#ifdef __PERF_LOG_LOCATION__
void __print_Location(PERF_Private *perf,
                      char const *szFile,
                      unsigned long ulLine,
                      char const *szFunc)
{
    /* save location for printing */
    PERF_PRINT_Private *me = perf->cip.pDebug;

    me->szFile = szFile;
    me->szFunc = szFunc;
    me->ulLine = ulLine;
}

void clear_print_location(PERF_Private *perf)
{
    PERF_PRINT_Private *me = perf->cip.pDebug;

    /* clear location information */
    me->szFile = me->szFunc = NULL;
    me->ulLine = 0;
}

void print_print_location(PERF_Private *perf, FILE *fOut, int nValues)
{
    PERF_PRINT_Private *me = perf->cip.pDebug;

    /* print location information if specified */
    if (me->szFile && me->szFunc)
    {
        /* align filenames for CSV format */
        if (me->csv)
        {
            while (nValues < 7)
            {
                fprintf(fOut, ",");
                nValues++;
            }
        }
        fprintf(fOut, "%s%s%s%lu%s%s%s\n",
                me->csv ? "," : " in ",
                me->szFile,
                me->csv ? "," : ":line ",
                me->ulLine,
                me->csv ? "," : ":",
                me->szFunc,
                me->csv ? "" : "()");

        /* clear print location */
        clear_print_location(perf);
    }
    else
    {   /* if no info is specified, we still need to print the new line */
        fprintf(fOut, "\n");
    }
}

#define __LINE_END__ ""
#else
#define __LINE_END__ "\n"
#define print_print_location(perf, fOut, nValues)
#endif

PERF_PRINT_Private *
PERF_PRINT_create(PERF_Private *perf, PERF_Config *config,
                  PERF_MODULETYPE eModule)
{
    char *fOutFile = NULL;
    FILE *fOut = NULL;
    PERF_PRINT_Private *me =
    perf->cip.pDebug = malloc(sizeof(PERF_PRINT_Private));

    if (me)
    {
        me->csv = config->csv;
        me->fDebug = me->fPrint = NULL;
        me->info = me->prompt = NULL;
        perf->uMode |= PERF_Mode_Print;

#ifdef __PERF_LOG_LOCATION__
        /* no location information yet */
        clear_print_location(perf);
#endif
        /* set up fDebug and fPrint file pointers */
        if (config->log_file)
        {
            /* open log file unless STDOUT or STDERR is specified */
            if (!strcasecmp(config->log_file, "STDOUT")) fOut = stdout;
            else if (!strcasecmp(config->log_file, "STDERR")) fOut = stderr;
            else
            {
                /* expand file name with PID and name */
                fOutFile = (char *) malloc (strlen(config->log_file) + 32);
                if (fOutFile)
                {
                    sprintf(fOutFile, "%s-%05lu-%08lx-%c%c%c%c.log",
                            config->log_file, perf->ulPID, (unsigned long) perf,
                            PERF_FOUR_CHARS(perf->ulID));
                    fOut = fopen(fOutFile, "at");

                    /* free new file name */
                    free(fOutFile);
                    fOutFile = NULL;
                }

                /* if could not open output, set it to STDOUT */
                if (!fOut) fOut = stdout;
            }
            me->fDebug = me->fPrint = fOut;
                                       
        }
        else if (config->detailed_debug)
        {
            /* detailed debug is through stderr */
            me->fDebug = me->fPrint = stderr;
        }
        else if (config->debug)
        {
            /* normal debug is through stdout (buffers are not printed) */
            me->fDebug = stdout;
            me->fPrint = NULL;
        }

        PERF_PRINT_setup(perf, eModule);

        if (me->fDebug) __print_Create(me->fDebug, perf);
    }

    return(me);
}

void __print_Boundary(FILE *fOut,
                      PERF_Private *perf, PERF_BOUNDARYTYPE eBoundary)
{
    /* get debug private structure */
    PERF_PRINT_Private *me = perf->cip.pDebug;

    unsigned long boundary = ((unsigned long) eBoundary) & PERF_BoundaryMask;

    fprintf(fOut, "%s%ld.%06ld%sBoundary%s0x%x%s%s%s%s" __LINE_END__,
            me->prompt, TIME_SECONDS(perf->time), TIME_MICROSECONDS(perf->time),
            me->info,
            me->csv ? "," : "(",
            eBoundary,
            me->csv ? "," : " = ",
            PERF_IsStarted(eBoundary) ? "started " : "completed ",
            (boundary < PERF_BoundaryMax  ?
             PERF_BoundaryTypes[boundary] : "INVALID"),
            me->csv ? "" : ")");

    print_print_location(perf, fOut, 2);
}

static const char *sendRecvTxt[] = {
        "received", "sending", "requesting", "sent",
    };

void __print_Buffer(FILE *fOut,
                    PERF_Private *perf,
                    unsigned long ulAddress1,
                    unsigned long ulAddress2,
                    unsigned long ulSize,
                    PERF_MODULETYPE eModule)
{
    
    /* get debug private structure */
    PERF_PRINT_Private *me = perf->cip.pDebug;

    unsigned long module1 = ((unsigned long) eModule) & PERF_ModuleMask;
    unsigned long module2 = (((unsigned long) eModule) >> PERF_ModuleBits) & PERF_ModuleMask;
    int xfering  = PERF_IsXfering ((unsigned long) eModule);
    int sendIx   = (PERF_GetSendRecv ((unsigned long) eModule) >> 28) & 3;
    int sending  = PERF_IsSending ((unsigned long) eModule);
    int frame    = PERF_IsFrame   ((unsigned long) eModule);
    int multiple = PERF_IsMultiple((unsigned long) eModule);

    if (!xfering && sending) module2 = module1;

    fprintf(fOut, "%s%ld.%06ld%sBuffer%s%s%s%s%s%s%s%s%s0x%lx%s0x%lx",
            me->prompt, TIME_SECONDS(perf->time), TIME_MICROSECONDS(perf->time),
            me->info,
            me->csv ? "," : "(",           
            xfering ? "xfering" : sendRecvTxt[sendIx],
            me->csv ? "," : " ",
            frame ? "frame" : "buffer",
            me->csv ? "," : (xfering || !sending) ? " from=" : "",
            (xfering || !sending) ?
            (module1 < PERF_ModuleMax ? PERF_ModuleTypes[module1] : "INVALID") :
            "",
            me->csv ? "," : (xfering || sending) ? " to=" : "",
            (xfering || sending) ?
            (module2 < PERF_ModuleMax ? PERF_ModuleTypes[module2] : "INVALID") :
            "",
            me->csv ? "," : " size=",
            ulSize,
            me->csv ? "," : " addr=",
            ulAddress1);

    /* print second address if supplied */
    if (multiple)
    {
        fprintf(fOut, "%s0x%lx",
                me->csv ? "," : " addr=",
                ulAddress2);
    }

    fprintf(fOut, "%s" __LINE_END__,
            me->csv ? "" : ")");

    print_print_location(perf, fOut, 6 + (multiple ? 1 : 0));
}

void __print_Command(FILE *fOut,
                     PERF_Private *perf,
                     unsigned long ulCommand,
					 unsigned long ulArgument,
                     PERF_MODULETYPE eModule)
{
    /* get debug private structure */
    PERF_PRINT_Private *me = perf->cip.pDebug;

    unsigned long module = ((unsigned long) eModule) & PERF_ModuleMask;
    int sendIx   = (PERF_GetSendRecv ((unsigned long) eModule) >> 28) & 3;
    int sending  = PERF_IsSending(((unsigned long) eModule) & ~PERF_ModuleMask);

    fprintf(fOut, "%s%ld.%06ld%sCommand%s%s%s%s%s0x%lx%s0x%lx%s%s%s" __LINE_END__,
            me->prompt, TIME_SECONDS(perf->time), TIME_MICROSECONDS(perf->time),
            me->info,
            me->csv ? "," : "(",
            sendRecvTxt[sendIx],
            me->csv ? "," : sending ? " to=" : " from=",
            module < PERF_ModuleMax ? PERF_ModuleTypes[module] : "INVALID",
            me->csv ? "," : " cmd=",
            ulCommand,
			me->csv ? "," : "(",
			ulArgument,
            me->csv ? "," : ") = ",
            (ulCommand != PERF_CommandStatus  ?
             "INVALID" : PERF_CommandTypes[ulCommand]),			
            me->csv ? "" : ")");

    print_print_location(perf, fOut, 5);
}

void __print_Create(FILE *fOut,
                    PERF_Private *perf)
{
    /* get debug private structure */
    PERF_PRINT_Private *me = perf->cip.pDebug;

    fprintf(fOut, "%s%ld.%06ld%sCreate" __LINE_END__,
            me->prompt, TIME_SECONDS(perf->time), TIME_MICROSECONDS(perf->time),
            me->info);

    print_print_location(perf, fOut, 0);
}

void __print_Done(FILE *fOut,
                  PERF_Private *perf)
{
    /* get debug private structure */
    PERF_PRINT_Private *me = perf->cip.pDebug;

    fprintf(fOut, "%s%ld.%06ld%sDone" __LINE_END__,
            me->prompt, TIME_SECONDS(perf->time), TIME_MICROSECONDS(perf->time),
            me->info);

    print_print_location(perf, fOut, 0);
}

void __print_Log(FILE *fOut,
                 PERF_Private *perf,
                 unsigned long ulData1, unsigned long ulData2,
                 unsigned long ulData3)
{
    /* get debug private structure */
    PERF_PRINT_Private *me = perf->cip.pDebug;

    fprintf(fOut, "%s%ld.%06ld%sLog%s0x%lx%s0x%lx%s0x%lx%s" __LINE_END__,
            me->prompt, TIME_SECONDS(perf->time), TIME_MICROSECONDS(perf->time),
            me->info,
            me->csv ? "," : "(",
            ulData1,
            me->csv ? "," : ", ",
            ulData2,
            me->csv ? "," : ", ",
            ulData3,
            me->csv ? "" : ")");

    print_print_location(perf, fOut, 3);
}

void __print_SyncAV(FILE *fOut,
                    PERF_Private *perf,
                    float pfTimeAudio,
                    float pfTimeVideo,
                    PERF_SYNCOPTYPE eSyncOperation)
{
    /* get debug private structure */
    PERF_PRINT_Private *me = perf->cip.pDebug;

    unsigned long op = (unsigned long) eSyncOperation;

    fprintf(fOut, "%s%ld.%06ld%sSyncAV%s%g%s%g%s%s%s" __LINE_END__,
            me->prompt, TIME_SECONDS(perf->time), TIME_MICROSECONDS(perf->time),
            me->info,
            me->csv ? "," : "(audioTS=",
            pfTimeAudio,
            me->csv ? "," : " videoTS=",
            pfTimeVideo,
            me->csv ? "," : " op=",
            (op < PERF_SyncOpMax ? PERF_SyncOpTypes[op] : "INVALID"),
            me->csv ? "" : ")");

    print_print_location(perf, fOut, 3);
}

void __print_ThreadCreated(FILE *fOut,
                           PERF_Private *perf,
                           unsigned long ulThreadID,
                           unsigned long ulThreadName)
{
    /* get debug private structure */
    PERF_PRINT_Private *me = perf->cip.pDebug;

    fprintf(fOut, "%s%ld.%06ld%sThread%s%ld%s%c%c%c%c%s" __LINE_END__,
            me->prompt, TIME_SECONDS(perf->time), TIME_MICROSECONDS(perf->time),
            me->info,
            me->csv ? "," : "(pid=",
            ulThreadID,
            me->csv ? "," : " name=",
            PERF_FOUR_CHARS(ulThreadName),
            me->csv ? "" : ")");

    print_print_location(perf, fOut, 2);
}

#endif
