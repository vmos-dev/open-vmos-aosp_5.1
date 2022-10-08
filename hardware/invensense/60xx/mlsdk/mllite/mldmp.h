/*
 $License:
   Copyright 2011 InvenSense, Inc.

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
  $
 */
/***************************************************************************** *
 * $Id: mldmp.h 5629 2011-06-11 03:13:08Z mcaramello $
 ******************************************************************************/

/**
 * @defgroup MLDMP
 * @brief 
 *
 *  These are the top level functions that define how to load the MPL.  In order
 *  to use most of the features, the DMP must be loaded with some code.  The 
 *  loading procedure takes place when calling inv_dmp_open with a given DMP set 
 *  function, after having open the serial communication with the device via 
 *  inv_serial_start().  
 *  The DMP set function will load the DMP memory and enable a certain
 *  set of features.
 *
 *  First select a DMP version from one of the released DMP sets.  
 *  These could be:
 *  - DMP default to load and use the default DMP code featuring pedometer, 
 *    gestures, and orientation.  Use inv_dmp_open().
 *  - DMP pedometer stand-alone to load and use the standalone pedometer
 *    implementation. Use inv_open_low_power_pedometer().
 *  <!-- - DMP EIS ... Use inv_eis_open_dmp(). -->
 *
 *  After inv_dmp_openXXX any number of appropriate initialization and configuration 
 *  routines can be called. Each one of these routines will return an error code 
 *  and will check to make sure that it is compatible with the the DMP version 
 *  selected during the call to inv_dmp_open.
 *
 *  Once the configuration is complete, make a call to inv_dmp_start(). This will
 *  finally turn on the DMP and run the code previously loaded.
 *
 *  While the DMP is running, all data fetching, polling or other functions can 
 *  be called and will return valid data. Some parameteres can be changed while 
 *  the DMP is runing, while others cannot.  Therefore it is important to always 
 *  check the return code of each function.  Check the error code list in mltypes
 *  to know what each returned error corresponds to.
 *
 *  When no more motion processing is required, the library can be shut down and
 *  the DMP turned off.  We can do that by calling inv_dmp_close().  Note that 
 *  inv_dmp_close() will not close the serial communication automatically, which will
 *  remain open an active, in case another module needs to be loaded instead.
 *  If the intention is shutting down the MPL as well, an explicit call to 
 *  inv_serial_stop() following inv_dmp_close() has to be made.
 *
 *  The MPL additionally implements a basic state machine, whose purpose is to
 *  give feedback to the user on whether he is following all the required 
 *  initialization steps.  If an anomalous transition is detected, the user will
 *  be warned by a terminal message with the format:
 *
 *  <tt>"Error : illegal state transition from STATE_1 to STATE_3"</tt>
 *
 *  @{
 *      @file     mldmp.h
 *      @brief    Top level entry functions to the MPL library with DMP support
 */

#ifndef MLDMP_H
#define MLDMP_H
#ifdef INV_INCLUDE_LEGACY_HEADERS
#include "mldmp_legacy.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

    inv_error_t inv_dmp_open(void);
    inv_error_t inv_dmp_start(void);
    inv_error_t inv_dmp_stop(void);
    inv_error_t inv_dmp_close(void);

#ifdef __cplusplus
}
#endif
#endif                          /* MLDMP_H */
/**
 * @}
**/
