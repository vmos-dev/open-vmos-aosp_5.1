/*
 * dspbridge/src/api/linux/Timer.c
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
 *  ======== Timer.c ========
 *  Description:
 *      Source for API time measurements. For Debugging only
 *
 *
 *! Revision History
 *! =================
 *! 01-May-2008 RG: Initial version
 *
 */

/*  ----------------------------------- Host OS */
#include <perfutils.h>


/*
 * ======== StartTimer ========
 */
INT getTimeStamp(struct timeval *tv)
{
	INT Result = 0;
	struct timezone tz;
	Result = gettimeofday(tv, &tz);
	if (Result != 0)
		fprintf(stdout, "FAIL: gettimeofday is failed\n");

	return Result;
}
/*
 * ======== PrintStatisticsStartTimer ========
 */
void PrintStatistics(struct timeval *tv_beg, struct timeval *tv_end,
			char *ModuleName, INT BufferSize)
{
	ULONG   totalTimeuSec = 0;

	if (tv_end->tv_usec < tv_beg->tv_usec) {
		tv_end->tv_usec += 1000000;
		tv_end->tv_sec  -= 1;
	}

	totalTimeuSec = (tv_end->tv_sec - tv_beg->tv_sec) * 1000000 +
			(tv_end->tv_usec - tv_beg->tv_usec);
	fprintf(stdout, "LOG: *********BEGIN STATISTICS************"
				"********************\n");
	fprintf(stdout, "LOG: MODULE: %s \n", ModuleName);
	if (BufferSize != 0)
		fprintf(stdout, "LOG: BufferSize: 0x%x \n", BufferSize);

	fprintf(stdout, "LOG: RESULT: %lu\n", totalTimeuSec);
	fprintf(stdout, "LOG: **********END STATISTICS*************"
				"******************\n");

}
