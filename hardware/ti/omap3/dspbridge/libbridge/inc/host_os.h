/*
 * dspbridge/mpu_api/inc/host_os.h
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
 *  ======== windows.h ========
 *
 *! Revision History
 *! ================
 *! 08-Mar-2004 sb Added cacheflush.h to support Dynamic Memory Mapping feature
 *! 16-Feb-2004 sb Added headers required for consistent_alloc  
 */

#ifndef _HOST_OS_H_
#define _HOST_OS_H_

#ifdef __KERNEL__

#include <linux/autoconf.h>
#include <asm/system.h>
#include <asm/atomic.h>
#include <asm/semaphore.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/syscalls.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/stddef.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/ctype.h>
#include <linux/mm.h>
#include <linux/device.h>
#include <linux/vmalloc.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
//#include <asm/arch/bus.h>

 
#include <asm/arch/clock.h>
#include <linux/clk.h>
//  #include <asm-arm/hardware/clock.h>

#include <linux/pagemap.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
#include <asm/cacheflush.h>
#include <linux/dma-mapping.h>
#else
// #include <asm/proc/cache.h>
#include <asm/pci.h>
#include <linux/pci.h>
#endif

/*  ----------------------------------- Macros */

#define SEEK_SET        0	/* Seek from beginning of file.  */
#define SEEK_CUR        1	/* Seek from current position.  */
#define SEEK_END        2	/* Seek from end of file.  */


/* TODO -- Remove, once BP defines them */
#define INT_MAIL_MPU_IRQ        26
#define INT_DSP_MMU_IRQ        28


#else

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdbool.h>
#ifdef DEBUG_BRIDGE_PERF
#include <sys/time.h>
#endif
#endif

#include <dbtype.h>

#endif
