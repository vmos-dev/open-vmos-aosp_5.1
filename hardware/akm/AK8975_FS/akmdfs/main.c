/******************************************************************************
 * $Id: main.c 580 2012-03-29 09:56:21Z yamada.rj $
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
#include "AKFS_Common.h"
#include "AKFS_Compass.h"
#include "AKFS_Disp.h"
#include "AKFS_FileIO.h"
#include "AKFS_Measure.h"
#include "AKFS_APIs.h"

#ifndef WIN32
#include <sched.h>
#include <pthread.h>
#include <linux/input.h>
#endif

#define ERROR_INITDEVICE		(-1)
#define ERROR_OPTPARSE			(-2)
#define ERROR_SELF_TEST			(-3)
#define ERROR_READ_FUSE			(-4)
#define ERROR_INIT				(-5)
#define ERROR_GETOPEN_STAT		(-6)
#define ERROR_STARTCLONE		(-7)
#define ERROR_GETCLOSE_STAT		(-8)

/* Global variable. See AKFS_Common.h file. */
int g_stopRequest = 0;
int g_opmode = 0;
int g_dbgzone = 0;
int g_mainQuit = AKD_FALSE;

/* Static variable. */
static pthread_t s_thread;  /*!< Thread handle */

/*!
 A thread function which is raised when measurement is started.
 @param[in] args This parameter is not used currently.
 */
static void* thread_main(void* args)
{
	AKFS_MeasureLoop();
	return ((void*)0);
}

/*!
  Signal handler.  This should be used only in DEBUG mode.
  @param[in] sig Event
 */
static void signal_handler(int sig)
{
	if (sig == SIGINT) {
		ALOGE("SIGINT signal");
		g_stopRequest = 1;
		g_mainQuit = AKD_TRUE;
	}
}

/*!
 Starts new thread.
 @return If this function succeeds, the return value is 1. Otherwise,
 the return value is 0.
 */
static int startClone(void)
{
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	g_stopRequest = 0;
	if (pthread_create(&s_thread, &attr, thread_main, NULL) == 0) {
		return 1;
	} else {
		return 0;
	}
}

/*!
 This function parse the option.
 @retval 1 Parse succeeds.
 @retval 0 Parse failed.
 @param[in] argc Argument count
 @param[in] argv Argument vector
 @param[out] layout_patno
 */
int OptParse(
	int		argc,
	char*	argv[],
	AKFS_PATNO*	layout_patno)
{
#ifdef WIN32
	/* Static */
#if defined(AKFS_WIN32_PAT1)
	*layout_patno = PAT1;
#elif defined(AKFS_WIN32_PAT2)
	*layout_patno = PAT2;
#elif defined(AKFS_WIN32_PAT3)
	*layout_patno = PAT3;
#elif defined(AKFS_WIN32_PAT4)
	*layout_patno = PAT4;
#elif defined(AKFS_WIN32_PAT5)
	*layout_patno = PAT5;
#else
	*layout_patno = PAT1;
#endif
	g_opmode = OPMODE_CONSOLE;
	/*g_opmode = 0;*/
	g_dbgzone = AKMDATA_LOOP | AKMDATA_TEST;
#else
	int		opt;
	char	optVal;

	*layout_patno = PAT_INVALID;

	while ((opt = getopt(argc, argv, "sm:z:")) != -1) {
		switch(opt){
			case 'm':
				optVal = (char)(optarg[0] - '0');
				if ((PAT1 <= optVal) && (optVal <= PAT8)) {
					*layout_patno = (AKFS_PATNO)optVal;
					AKMDEBUG(DBG_LEVEL2, "%s: Layout=%d\n", __FUNCTION__, optVal);
				}
				break;
			case 's':
				g_opmode |= OPMODE_CONSOLE;
				break;
            case 'z':
                /* If error detected, hopefully 0 is returned. */
                errno = 0;
                g_dbgzone = (int)strtol(optarg, (char**)NULL, 0); 
                AKMDEBUG(DBG_LEVEL2, "%s: Dbg Zone=%d\n", __FUNCTION__, g_dbgzone);
                break;
			default:
				ALOGE("%s: Invalid argument", argv[0]);
				return 0;
		}
	}

	/* If layout is not specified with argument, get parameter from driver */
	if (*layout_patno == PAT_INVALID) {
		int16_t n;
		if (AKD_GetLayout(&n) == AKM_SUCCESS) {
			if ((PAT1 <= n) && (n <= PAT8)) {
				*layout_patno = (AKFS_PATNO)n;
			}
		}
	}
	/* Error */
	if (*layout_patno == PAT_INVALID) {
		ALOGE("No layout is specified.");
		return 0;
	}
#endif

	return 1;
}

void ConsoleMode(void)
{
	/*** Console Mode *********************************************/
	while (AKD_TRUE) {
		/* Select operation */
		switch (Menu_Main()) {
		case MODE_SelfTest:
			AKFS_SelfTest();
			break;
		case MODE_Measure:
			/* Reset flag */
			g_stopRequest = 0;
			/* Measurement routine */
			AKFS_MeasureLoop();
			break;

		case MODE_Quit:
			return;

		default:
			AKMDEBUG(DBG_LEVEL0, "Unknown operation mode.\n");
			break;
		}
	}
}

int main(int argc, char **argv)
{
	int			retValue = 0;
	AKFS_PATNO	pat;
	uint8		regs[3];

	/* Show the version info of this software. */
	Disp_StartMessage();

#if ENABLE_AKMDEBUG
	/* Register signal handler */
	signal(SIGINT, signal_handler);
#endif

	/* Open device driver */
	if(AKD_InitDevice() != AKD_SUCCESS) {
		retValue = ERROR_INITDEVICE;
		goto MAIN_QUIT;
	}

	/* Parse command-line options */
	/* This function calls device driver function to get layout */
	if (OptParse(argc, argv, &pat) == 0) {
		retValue = ERROR_OPTPARSE;
		goto MAIN_QUIT;
	}

	/* Self Test */
	if (g_opmode & OPMODE_FST){
		if (AKFS_SelfTest() != AKD_SUCCESS) {
			retValue = ERROR_SELF_TEST;
			goto MAIN_QUIT;
		}
	}

	/* OK, then start */
	if (AKFS_ReadAK8975FUSEROM(regs) != AKM_SUCCESS) {
		retValue = ERROR_READ_FUSE;
		goto MAIN_QUIT;
	}

	/* Initialize library. */
	if (AKFS_Init(pat, regs) != AKM_SUCCESS) {
		retValue = ERROR_INIT;
		goto MAIN_QUIT;
	}

	/* Start console mode */
	if (g_opmode & OPMODE_CONSOLE) {
		ConsoleMode();
		goto MAIN_QUIT;
	}

	/*** Start Daemon ********************************************/
	while (g_mainQuit == AKD_FALSE) {
		int st = 0;
		/* Wait until device driver is opened. */
		if (AKD_GetOpenStatus(&st) != AKD_SUCCESS) {
			retValue = ERROR_GETOPEN_STAT;
			goto MAIN_QUIT;
		}
		if (st == 0) {
			ALOGI("Suspended.");
		} else {
			ALOGI("Compass Opened.");
			/* Reset flag */
			g_stopRequest = 0;
			/* Start measurement thread. */
			if (startClone() == 0) {
				retValue = ERROR_STARTCLONE;
				goto MAIN_QUIT;
			}

			/* Wait until device driver is closed. */
			if (AKD_GetCloseStatus(&st) != AKD_SUCCESS) {
				retValue = ERROR_GETCLOSE_STAT;
				g_mainQuit = AKD_TRUE;
			}
			/* Wait thread completion. */
			g_stopRequest = 1;
			pthread_join(s_thread, NULL);
			ALOGI("Compass Closed.");
		}
	}

MAIN_QUIT:

	/* Release library */
	AKFS_Release();
	/* Close device driver. */
	AKD_DeinitDevice();
	/* Show the last message. */
	Disp_EndMessage(retValue);

	return retValue;
}


