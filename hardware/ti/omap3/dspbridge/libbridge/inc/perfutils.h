/*
 * dspbridge/mpu_api/inc/perfutils.h
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

#include <sys/time.h>
#include <stdio.h>
#include <dbdefs.h>

INT getTimeStamp(struct timeval *tv);

VOID PrintStatistics(struct timeval *tv_beg,struct timeval* tv_end, char * ModuleName,INT BufferSize);
