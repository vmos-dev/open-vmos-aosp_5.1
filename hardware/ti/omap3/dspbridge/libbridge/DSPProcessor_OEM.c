/*
 * dspbridge/src/api/linux/DSPProcessor_OEM.c
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
 *  ======== DSPProcessor_OEM.c ========
 *  Description:
 *      This is the source for the DSP/BIOS Bridge API processor module.
 *
 *  Public Functions:
 *      DSPProcessor_Ctrl       - OEM
 *      DSPProcessor_GetTrace   - OEM
 *      DSPProcessor_Load       - OEM
 *      DSPProcessor_Start      - OEM
 *
 *! Revision History
 *! ================
 *! 29-Nov-2000 rr: Seperated from DSPProcessor.c
 *
 */

/*  ----------------------------------- Host OS */
#include <host_os.h>

/*  ----------------------------------- DSP/BIOS Bridge */
#include <dbdefs.h>
#include <errbase.h>

/*  ----------------------------------- Others */
#include <dsptrap.h>

/*  ----------------------------------- This */
#include "_dbdebug.h"
#include "_dbpriv.h"
#include <DSPProcessor_OEM.h>
#ifdef DEBUG_BRIDGE_PERF
#include <perfutils.h>
#endif



/*
 *  ======== DSPProcessor_Ctrl ========
 *  Purpose:
 *      Pass control information to the GPP device driver managing the
 *      DSP processor.
 *      This will be an OEM-only function, and not part of the 'Bridge
 *      application developer's API.
 */
DBAPI DSPProcessor_Ctrl(DSP_HPROCESSOR hProcessor, ULONG dwCmd,
		  IN OPTIONAL struct DSP_CBDATA *pArgs)
{
	DSP_STATUS status = DSP_SOK;
	Trapped_Args tempStruct;

	DEBUGMSG(DSPAPI_ZONE_FUNCTION, (TEXT("PROC: DSPProcessor_Ctrl\r\n")));

	/* Check the handle */
	if (hProcessor) {
		tempStruct.ARGS_PROC_CTRL.hProcessor = hProcessor;
		tempStruct.ARGS_PROC_CTRL.dwCmd = dwCmd;
		tempStruct.ARGS_PROC_CTRL.pArgs = pArgs;
		status = DSPTRAP_Trap(&tempStruct, CMD_PROC_CTRL_OFFSET);
	} else {
		/* Invalid handle */
		status = DSP_EHANDLE;
		DEBUGMSG(DSPAPI_ZONE_ERROR,
				(TEXT("PROC: Invalid Handle \r\n")));
	}

	return status;
}

/*
 *  ======== DSPProcessor_Load ========
 *  Purpose:
 *      Reset a processor and load a new base program image.
 *      This will be an OEM-only function, and not part of the 'Bridge
 *      application developer's API.
 */
DBAPI DSPProcessor_Load(DSP_HPROCESSOR hProcessor, IN CONST INT iArgc,
		  IN CONST CHAR **aArgv, IN CONST CHAR **aEnvp)
{
	DSP_STATUS status = DSP_SOK;
	Trapped_Args tempStruct;
#ifdef DEBUG_BRIDGE_PERF
	struct timeval tv_beg;
	struct timeval tv_end;
	struct timezone tz;
	int timeRetVal = 0;

	timeRetVal = getTimeStamp(&tv_beg);
#endif


	DEBUGMSG(DSPAPI_ZONE_FUNCTION, (TEXT("PROC: DSPProcessor_Load\r\n")));

	/* Check the handle */
	if (hProcessor) {
		if (iArgc > 0) {
			if (!DSP_ValidReadPtr(aArgv, iArgc)) {
				tempStruct.ARGS_PROC_LOAD.hProcessor =
						hProcessor;
				tempStruct.ARGS_PROC_LOAD.iArgc = iArgc;
				tempStruct.ARGS_PROC_LOAD.aArgv =
						(CHAR **)aArgv;
				tempStruct.ARGS_PROC_LOAD.aEnvp =
						(CHAR **)aEnvp;
				status = DSPTRAP_Trap(&tempStruct,
						CMD_PROC_LOAD_OFFSET);
			} else {
				status = DSP_EPOINTER;
				DEBUGMSG(DSPAPI_ZONE_ERROR,
				(TEXT("PROC: Null pointer in input \r\n")));
			}
		} else {
			status = DSP_EINVALIDARG;
			DEBUGMSG(DSPAPI_ZONE_ERROR,
					(TEXT("PROC: iArgc is invalid. \r\n")));
		}
	} else {
		/* Invalid handle */
		status = DSP_EHANDLE;
		DEBUGMSG(DSPAPI_ZONE_ERROR,
				(TEXT("PROC: Invalid Handle \r\n")));
	}

#ifdef DEBUG_BRIDGE_PERF
	timeRetVal = getTimeStamp(&tv_end);
	PrintStatistics(&tv_beg, &tv_end, "DSPProcessor_Load", 0);

#endif

	return status;
}

/*
 *  ======== DSPProcessor_Start ========
 *  Purpose:
 *      Start a processor running.
 */
DBAPI DSPProcessor_Start(DSP_HPROCESSOR hProcessor)
{
	DSP_STATUS status = DSP_SOK;
	Trapped_Args tempStruct;

	DEBUGMSG(DSPAPI_ZONE_FUNCTION, (TEXT("PROC: DSPProcessor_Start\r\n")));

	/* Check the handle */
	if (hProcessor) {
		tempStruct.ARGS_PROC_START.hProcessor = hProcessor;
		status = DSPTRAP_Trap(&tempStruct, CMD_PROC_START_OFFSET);
	} else {
		/* Invalid handle */
		status = DSP_EHANDLE;
		DEBUGMSG(DSPAPI_ZONE_ERROR,
				(TEXT("PROC: Invalid Handle \r\n")));
	}

	return status;
}

/*
 *  ======== DSPProcessor_Stop ========
 *  Purpose:
 *      Stop a running processor .
 */
DBAPI DSPProcessor_Stop(DSP_HPROCESSOR hProcessor)
{
	DSP_STATUS status = DSP_SOK;
	Trapped_Args tempStruct;

	DEBUGMSG(DSPAPI_ZONE_FUNCTION, (TEXT("PROC: DSPProcessor_Stop\r\n")));

	/* Check the handle */
	if (hProcessor) {
		tempStruct.ARGS_PROC_START.hProcessor = hProcessor;
		status = DSPTRAP_Trap(&tempStruct, CMD_PROC_STOP_OFFSET);
	} else {
		/* Invalid handle */
		status = DSP_EHANDLE;
		DEBUGMSG(DSPAPI_ZONE_ERROR,
				(TEXT("PROC: Invalid Handle \r\n")));
	}

	return status;
}
