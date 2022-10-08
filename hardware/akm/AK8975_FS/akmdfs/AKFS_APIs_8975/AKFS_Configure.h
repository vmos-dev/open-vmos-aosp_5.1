/******************************************************************************
 * $Id: AKFS_Configure.h 580 2012-03-29 09:56:21Z yamada.rj $
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
#ifndef AKFS_INC_CONFIG_H
#define AKFS_INC_CONFIG_H

/***** Language configuration *************************************************/
#if defined(__cplusplus)
#define AKLIB_C_API_START	extern "C" {
#define AKLIB_C_API_END		}
#else
#define AKLIB_C_API_START
#define AKLIB_C_API_END
#endif

/*! If following line is commented in, double type is used for floating point
   calculation */
/*
#define AKFS_PRECISION_DOUBLE
*/

#endif

