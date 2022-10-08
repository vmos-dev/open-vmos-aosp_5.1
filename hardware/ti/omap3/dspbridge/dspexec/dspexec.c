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
 *  ======== cexec.c ========
 *  "cexec" is a Linux console-based utility that allows developers to load 
 *  and start a new DSP/BIOS Bridge based DSP program. If "cexec" encounters
 *  an error, it will display a DSP/BIOS Bridge GPP API error code.
 *
 *  Usage:
 *      cexec [optional args] <dsp_program>
 *
 *  Options:
 *      -?: displays "cexec" usage. If this option is set, cexec does not 
 *          load the DSP program.
 *      -w: waits for the user to hit the enter key on the keyboard, which
 *          stops DSP program execution by placing the DSP into reset. This 
 *          will also display on the WinCE console window the contents of
 *          DSP/BIOS Bridge trace buffer, which is used internally by the 
 *          DSP/BIOS Bridge runtime. If this option is not specified, "cexec"
 *          loads and starts the DSP program, then returns immeidately, 
 *          leaving the DSP program running.
 *      -v: verbose mode.
 *      -p [processor]: defines the DSP processor on which to execute code,
 *          where processor is the processor number provided by cexec to 
 *          the DSPProcessor_Attach() function. If this option is not 
 *          specified, the default processor is 0.
 *      -T: Set scriptable mode. If set, cexec will run to completion after
 *          loading and running a DSP target. User will not be able to stop 
 *          the loaded DSP target.
 *  
 *  Example:
 *      1.  Load and execute a DSP/BIOS Bridge base image, waiting for user to 
 *          hit enter key to halt the DSP.
 *  
 *          cexec -w chnltest_tiomap1510.x55l
 *
 *      2.  Start a program running on the second processor (zero-indexed). 
 *          This will halt the currently running DSP program, if any.
 *
 *          cexec -w -p l chnltest_tiomap1510.x55l
 *      
 *      3.  Load and start a program running on the DSP. Cexec will run to 
 *          completion and leave the DSP loaded and running.
 *
 *          cexec -T chnltest_tiomap1510.x55l
 *
 *! Revision History:
 *! ================
 *! 20-Oct-2002 map: 	Instrumented to measure performance on PROC_Load
 *! 31-Jul-2002 kc:     Added scriptable mode to enable batch testing.
 *! 05-Apr-2001 kc:     Adapted from existing cexec. Updated cexec program 
 *!                     options. Added command line argument handling.
 *!                     Based on DSP/BIOS Bridge API version 0.??.
 *! 13-Feb-2001 kc:     DSP/BIOS Bridge name update.
 *! 15-Nov-2000 jeh     Converted to use DSPProcessor.
 */ 
    
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dbdefs.h>
/* #include <varargs.h> */ 
#include <stdarg.h>
    
#include <dbapi.h>
#include <DSPManager.h>
#include <DSPProcessor.h>
#include <DSPProcessor_OEM.h>
    
/* global constants. */ 
#define MAXTRACESIZE 256   /* size of trace buffer. */
    
/* function prototype. */ 
VOID DisplayUsage();
VOID PrintVerbose(PSTR pstrFmt,...);

/* global variables. */ 
bool g_fVerbose = false;

/* 
 *  ======== main ========
 */ 
INT main(INT argc, CHAR * argv[]) 
{
	INT opt;
	bool fWaitForTerminate = false;
	UINT uProcId = 0;	/* default proc ID is 0. */
	bool fError = false;
	DSP_HPROCESSOR hProc;
	DSP_STATUS status = DSP_SOK;
	INT cArgc = 0;		/* local argc count. */
	bool fScriptable = false;
	extern char *optarg;
	struct DSP_PROCESSORINFO dspInfo;
	UINT numProcs;
	UINT index = 0;
	while ((opt = getopt(argc, argv, "+T+v+w+?p:")) != EOF) {
		switch (opt) {
		case 'v':
			/* verbose mode */ 
			fprintf(stdout, "Verbose mode: ON\n");
			g_fVerbose = true;
			cArgc++;
			break;
		case 'w':
			/* wait for user input to terminate */ 
			fprintf(stdout, "Not supported \n");
			fWaitForTerminate = true;
			cArgc++;
			break;
		case 'T':
			fScriptable = true;
			cArgc++;
			break;
		case 'p':
			/* user specified DSP processor ID (based on zero-index) */ 
			uProcId = atoi(optarg);
			cArgc++;
			break;
		case '?':
		default:
			fError = true;
			break;
		}
	}
	argv += cArgc + 1;
	argc -= cArgc + 1;
	if (fError) {
		DisplayUsage();
	} else {
		status = (DBAPI)DspManager_Open(0, NULL);
		if (DSP_FAILED(status)) {
			PrintVerbose("DSPManager_Open failed \n");
			return -1;
		} 
		while (DSP_SUCCEEDED(DSPManager_EnumProcessorInfo(index,&dspInfo,
						(UINT)sizeof(struct DSP_PROCESSORINFO),&numProcs))) {
			if ((dspInfo.uProcessorType == DSPTYPE_55) || 
									(dspInfo.uProcessorType == DSPTYPE_64)) {
				printf("DSP device detected !! \n");
				uProcId = index;
				status = DSP_SOK;
				break;
			}
			index++;
		}
		status = DSPProcessor_Attach(uProcId, NULL, &hProc);
		if (DSP_SUCCEEDED(status)) {
			PrintVerbose("DSPProcessor_Attach succeeded.\n");
			status = DSPProcessor_Stop(hProc);
			if (DSP_SUCCEEDED(status)) {
				PrintVerbose("DSPProcessor_Stop succeeded.\n");
				status = DSPProcessor_Load(hProc,argc,(CONST CHAR **)argv,NULL);
				if (DSP_SUCCEEDED(status)) {
					PrintVerbose("DSPProcessor_Load succeeded.\n");
					status = DSPProcessor_Start(hProc);
					if (DSP_SUCCEEDED(status)) {
						fprintf(stdout,"DSPProcessor_Start succeeded.\n");
#if 0                    
						/* It seems Linux bridge does n't yet support
						 * * DSPProcessor_GetTrace */ 
						if (fWaitForTerminate) { 
							/* wait for user */ 
							fprintf(stdout,"Hit \"return\" to stop DSP and"
													"dump trace buffer:\n");
							(void)getchar();
							status = DSPProcessor_GetTrace(hProc,
												(BYTE *)&traceBuf,MAXTRACESIZE);
							fprintf(stdout,"%s\n",traceBuf);
						} else {
							PrintVerbose("in run free mode...\n");
						}
#endif	/* 0 */
					} else {
						PrintVerbose("DSPProcessor_Start failed: 0x%x.\n",
																		status);
					}
				} else {
					PrintVerbose("DSPProcessor_Load failed: 0x%x.\n",status);
				}
				DSPProcessor_Detach(hProc);
			}
		} else {
			PrintVerbose("DSPProcessor_Attach failed: 0x%x.\n",status);
		}
	}
	if (!fScriptable) {
		/* Wait for user to hit any key before exiting. */ 
		fprintf(stdout, "Hit any key to terminate cexec.\n");
		(void)getchar();
	}
	status = DspManager_Close(0, NULL);
	if (DSP_FAILED(status)) {
		printf("\nERROR: DSPManager Close FAILED\n");
	}
	return (DSP_SUCCEEDED(status) ? 0 : -1);
}

VOID DisplayUsage() 
{
	fprintf(stdout, "Usage: cexec [options] <dsp program>\n");
	fprintf(stdout, "\t[optional arguments]:\n");
	fprintf(stdout, "\t-?: Display cexec usage\n");
	fprintf(stdout, "\t-v: Verbose mode\n");
	fprintf(stdout, "\t-w: Waits for user to hit enter key before\n");
	fprintf(stdout, "\t    terminating. Displays trace buffer\n");
	fprintf(stdout, "\t-p [processor]: User-specified processor #\n");
	fprintf(stdout, "\n\t[required arguments]:\n");
	fprintf(stdout, "\t<dsp program>\n");
	fprintf(stdout, "\n\tExample: cexec -w -p 1 prog.x55l\n\n");
}


/*
 * ======== PrintVerbose ========
 */ 
VOID PrintVerbose(PSTR pstrFmt,...) 
{
	va_list args;
	if (g_fVerbose) {
		va_start(args, pstrFmt);
		vfprintf(stdout, pstrFmt, args);
		va_end(args);
		fflush(stdout);
	}
}


