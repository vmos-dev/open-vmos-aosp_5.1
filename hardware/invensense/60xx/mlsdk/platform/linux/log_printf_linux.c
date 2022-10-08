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
/*******************************************************************************
 *
 * $Id: log_printf_linux.c 5629 2011-06-11 03:13:08Z mcaramello $ 
 *
 ******************************************************************************/
 
/**
 * @addtogroup MPL_LOG
 *
 * @{
 *      @file   log_printf.c
 *      @brief  printf replacement for _MLWriteLog.
 */

#include <stdio.h>
#include "log.h"

int _MLWriteLog (const char * buf, int buflen)
{
    return fputs(buf, stdout);
}

/**
 * @}
 */

