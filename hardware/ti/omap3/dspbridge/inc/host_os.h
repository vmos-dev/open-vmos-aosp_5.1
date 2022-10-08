/*
 *  Copyright 2001-2008 Texas Instruments - http://www.ti.com/
 * 
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */


/*
 *  ======== windows.h ========
 *  DSP-BIOS Bridge driver support functions for TI OMAP processors.
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


#if defined (OMAP_2430) || defined (OMAP_3430)
#include <asm/arch/clock.h>
#ifdef OMAP_3430
#include <linux/clk.h>
//  #include <asm-arm/hardware/clock.h>
#endif
#endif

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
#ifdef OMAP_3430
#define INT_MAIL_MPU_IRQ        26
#define INT_DSP_MMU_IRQ        28
#endif


#else

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include  <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#endif

#include <dbtype.h>

#endif
