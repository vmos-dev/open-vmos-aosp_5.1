/******************************************************************************
 * $Id: AKFS_FileIO.h 580 2012-03-29 09:56:21Z yamada.rj $
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
#ifndef AKFS_INC_FILEIO_H
#define AKFS_INC_FILEIO_H

/* Common include files. */
#include "AKFS_Common.h"

/* Include file for AK8975 library. */
#include "AKFS_Compass.h"

/*** Constant definition ******************************************************/

/*** Type declaration *********************************************************/

/*** Global variables *********************************************************/

/*** Prototype of function ****************************************************/
int16 AKFS_LoadParameters(AK8975PRMS *prms, const char* path);

int16 AKFS_SaveParameters(AK8975PRMS* prms, const char* path);

#endif

