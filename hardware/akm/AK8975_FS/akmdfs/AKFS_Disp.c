/******************************************************************************
 * $Id: AKFS_Disp.c 580 2012-03-29 09:56:21Z yamada.rj $
 ******************************************************************************
 *
 * Copyright (C) 2012 Asahi Kasei Microdevices Corporation, Japan
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "AKFS_Disp.h"
#include "AKFS_Common.h"

/*!
 Print startup message to Android Log daemon.
 */
void Disp_StartMessage(void)
{
	ALOGI("AK8975 Daemon for Open Source v20120329.");
	ALOGI("Debug: %s", ((ENABLE_AKMDEBUG)?("ON"):("OFF")));
	ALOGI("Debug level: %d", DBG_LEVEL);
}

/*!
 Print ending message to Android Log daemon.
 */
void Disp_EndMessage(int ret)
{
	ALOGI("AK8975 for Android end (%d).", ret);
}

/*!
 Print result
 */
void Disp_Result(int buf[YPR_DATA_SIZE])
{
	AKMDEBUG(DBG_LEVEL1,
		"Flag=%d\n", buf[0]);
	AKMDEBUG(DBG_LEVEL1,
		"Acc(%d):%8.2f, %8.2f, %8.2f\n",
		buf[4], REVERT_ACC(buf[1]), REVERT_ACC(buf[2]), REVERT_ACC(buf[3]));
	AKMDEBUG(DBG_LEVEL1,
		"Mag(%d):%8.2f, %8.2f, %8.2f\n",
		buf[8], REVERT_MAG(buf[5]), REVERT_MAG(buf[6]), REVERT_MAG(buf[7]));
	AKMDEBUG(DBG_LEVEL1,
		"Ori(%d)=%8.2f, %8.2f, %8.2f\n",
		buf[8], REVERT_ORI(buf[9]), REVERT_ORI(buf[10]), REVERT_ORI(buf[11]));
}

/*!
 Output main menu to stdout and wait for user input from stdin.
 @return Selected mode.
 */
MODE Menu_Main(void)
{
	char msg[20];
	memset(msg, 0, sizeof(msg));

	AKMDEBUG(DBG_LEVEL1,
	" --------------------  AK8975 Console Application -------------------- \n"
	"   1. Start measurement. \n"
	"   2. Self-test. \n"
	"   Q. Quit application. \n"
	" --------------------------------------------------------------------- \n"
	" Please select a number.\n"
	"   ---> ");
	fgets(msg, 10, stdin);
	AKMDEBUG(DBG_LEVEL1, "\n");

	/* BUG : If 2-digits number is input, */
	/*    only the first character is compared. */
	if (!strncmp(msg, "1", 1)) {
		return MODE_Measure;
	} else if (!strncmp(msg, "2", 1)) {
		return MODE_SelfTest;
	} else if (strncmp(msg, "Q", 1) == 0 || strncmp(msg, "q", 1) == 0) {
		return MODE_Quit;
	} else {
		return MODE_ERROR;
	}
}

