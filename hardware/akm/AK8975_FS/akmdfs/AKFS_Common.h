/******************************************************************************
 * $Id: AKFS_Common.h 580 2012-03-29 09:56:21Z yamada.rj $
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
#ifndef AKFS_INC_COMMON_H
#define AKFS_INC_COMMON_H

#ifdef WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif    					
  					
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <stdarg.h>
#include <crtdbg.h>
#include "Android.h"

#define DBG_LEVEL	DBG_LEVEL4
#define ENABLE_AKMDEBUG	1

#else
#include <stdio.h>     /* frpintf */
#include <stdlib.h>    /* atoi */
#include <string.h>    /* memset */
#include <unistd.h>
#include <stdarg.h>    /* va_list */
#include <utils/Log.h> /* LOGV */
#include <errno.h>     /* errno */

#endif

/*** Constant definition ******************************************************/
#define AKM_TRUE	1	/*!< Represents true */
#define AKM_FALSE	0	/*!< Represents false */
#define AKM_SUCCESS	1	/*!< Represents success */
#define AKM_FAIL	0	/*!< Represents fail */

#undef LOG_TAG
#define LOG_TAG "AKMD_FS"

#define DBG_LEVEL0	0	/* Critical */
#define DBG_LEVEL1	1	/* Notice */
#define DBG_LEVEL2	2	/* Information */
#define DBG_LEVEL3	3	/* Debug */
#define DBG_LEVEL4	4	/* Verbose */

#ifndef DBG_LEVEL
#define DBG_LEVEL	DBG_LEVEL0
#endif

#define DATA_AREA01	0x0001
#define DATA_AREA02	0x0002
#define DATA_AREA03	0x0004
#define DATA_AREA04	0x0008
#define DATA_AREA05	0x0010
#define DATA_AREA06	0x0020
#define DATA_AREA07	0x0040
#define DATA_AREA08	0x0080
#define DATA_AREA09	0x0100
#define DATA_AREA10	0x0200
#define DATA_AREA11	0x0400
#define DATA_AREA12	0x0800
#define DATA_AREA13	0x1000
#define DATA_AREA14	0x2000
#define DATA_AREA15	0x4000
#define DATA_AREA16	0x8000


/* Debug area definition */
#define AKMDATA_DUMP		DATA_AREA01	/*<! Dump data */
#define AKMDATA_BDATA		DATA_AREA02	/*<! BDATA */
#define AKMDATA_MAG			DATA_AREA03 /*<! Magnetic Field */
#define AKMDATA_ACC			DATA_AREA04 /*<! Accelerometer */
#define AKMDATA_ORI			DATA_AREA05 /*<! Orientation */
#define AKMDATA_GETINTERVAL	DATA_AREA06
#define AKMDATA_LOOP		DATA_AREA07
#define AKMDATA_DRV			DATA_AREA08

#ifndef ENABLE_AKMDEBUG
#define ENABLE_AKMDEBUG		0	/* Eanble debug output when it is 1. */
#endif

#define OPMODE_CONSOLE		0x01
#define OPMODE_FST			0x02

/***** Debug Level Output *************************************/
#if ENABLE_AKMDEBUG
#define AKMDEBUG(level, format, ...) \
    (((level) <= DBG_LEVEL) \
	  ? (fprintf(stdout, (format), ##__VA_ARGS__)) \
	  : ((void)0))
#else
#define AKMDEBUG(level, format, ...)
#endif

/***** Dbg Zone Output ***************************************/
#if ENABLE_AKMDEBUG
#define AKMDATA(flag, format, ...)  \
	((((int)flag) & g_dbgzone) \
	  ? (fprintf(stdout, (format), ##__VA_ARGS__)) \
	  : ((void)0))
#else
#define AKMDATA(flag, format, ...)
#endif

/***** Log output ********************************************/
#ifdef AKM_LOG_ENABLE
#define AKM_LOG(format, ...)	ALOGD((format), ##__VA_ARGS__)
#else
#define AKM_LOG(format, ...)
#endif

/***** Error output *******************************************/
#define AKMERROR \
	((g_opmode & OPMODE_CONSOLE) \
	  ? (fprintf(stderr, "%s:%d Error.\n", __FUNCTION__, __LINE__)) \
	  : (ALOGE("%s:%d Error.", __FUNCTION__, __LINE__))) 

#define AKMERROR_STR(api) \
	((g_opmode & OPMODE_CONSOLE) \
	  ? (fprintf(stderr, "%s:%d %s Error (%s).\n", \
	  		  __FUNCTION__, __LINE__, (api), strerror(errno))) \
	  : (ALOGE("%s:%d %s Error (%s).", \
	  		  __FUNCTION__, __LINE__, (api), strerror(errno))))

/*** Type declaration *********************************************************/

/*** Global variables *********************************************************/
extern int g_stopRequest;	/*!< 0:Not stop,  1:Stop */
extern int g_opmode;		/*!< 0:Daemon mode, 1:Console mode. */
extern int g_dbgzone;		/*!< Debug zone. */

/*** Prototype of function ****************************************************/

#endif /* AKMD_INC_AKCOMMON_H */

