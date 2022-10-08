/******************************************************************************
 * $Id: AKFS_Disp.h 580 2012-03-29 09:56:21Z yamada.rj $
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
#ifndef AKFS_INC_DISP_H
#define AKFS_INC_DISP_H

/* Include file for AK8975 library. */
#include "AKFS_Compass.h"

/*** Constant definition ******************************************************/
#define REVERT_ACC(a)	((float)((a) * 9.8f / 720.0f))
#define REVERT_MAG(m)	((float)((m) * 0.06f))
#define REVERT_ORI(o)	((float)((o) / 64.0f))

/*** Type declaration *********************************************************/

/*! These defined types represents the current mode. */
typedef enum _MODE {
	MODE_ERROR,			/*!< Error */
	MODE_Measure,		/*!< Measurement */
	MODE_SelfTest,		/*!< Self-test */
	MODE_Quit			/*!< Quit */
} MODE;

/*** Prototype of function ****************************************************/
/*
	Disp_   : Display messages.
	Menu_   : Display menu (two or more selection) and wait for user input.
 */

void Disp_StartMessage(void);
void Disp_EndMessage(int ret);
void Disp_Result(int buf[YPR_DATA_SIZE]);

MODE Menu_Main(void);

#endif

