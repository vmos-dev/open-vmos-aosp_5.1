/*
* Copyright (c) 2011 Intel Corporation. All Rights Reserved.
* Copyright (c) Imagination Technologies Limited, UK
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sub license, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice (including the
* next paragraph) shall be included in all copies or substantial portions
* of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
* IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
* ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
*/
#ifndef _topazhp_cmdump_h
#define _topazhp_cmdump_h

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h> /* for libc5 */

#ifdef ANDROID
#define  outl(...)
#define  outw(...)
#define  inl(...)   0
#define  inw(...)   0
#else
#include <sys/io.h> /* for glibc */
#endif

#define MIN(a,b)  ((a)>(b)?(b):(a))

struct RegisterInfomation {
    char *name;
    int offset;
};

#define ui32TopazMulticoreRegId  1



#define PCI_DEVICE_ID_CFG	0x02	/* 16 bits */
#define PCI_BASE_ADDRESS_0	0x10	/* 32 bits */
#define PCI_BASE_ADDRESS_1	0x14	/* 32 bits [htype 0,1 only] */
#define PCI_BASE_ADDRESS_2	0x18	/* 32 bits [htype 0 only] */
#define PCI_BASE_ADDRESS_3	0x1c	/* 32 bits */
#define CONFIG_CMD(bus,device_fn,where)   \
   (0x80000000|((bus&0xff) << 16)|((device_fn&0xff) << 8)|((where&0xff) & ~3))

static inline unsigned long pci_get_long(int bus,int device_fn, int where)
{
    outl(CONFIG_CMD(bus,device_fn,where), 0xCF8);
    return inl(0xCFC);
}

static inline int pci_set_long(int bus,int device_fn, int where,unsigned long value)
{
    outl(CONFIG_CMD(bus,device_fn,where), 0xCF8);
    outl(value,0xCFC);
    return 0;
}

static inline int pci_get_short(int bus,int device_fn, int where)
{
    outl(CONFIG_CMD(bus,device_fn,where), 0xCF8);
    return inw(0xCFC + (where&2));
}


static inline int pci_set_short(int bus,int device_fn, int where,unsigned short value)
{
    outl(CONFIG_CMD(bus,device_fn,where), 0xCF8);
    outw(value,0xCFC + (where&2));
    return 0;
}

#define REG_OFFSET_TOPAZ_MULTICORE                      0x00000000
#define REG_OFFSET_TOPAZ_DMAC                           0x00000400
#define REG_OFFSET_TOPAZ_MTX                            0x00000800

#define REGNUM_TOPAZ_CR_MMU_DIR_LIST_BASE_ADDR          0x0030

#ifndef MV_OFFSET_IN_TABLE
#define MV_OFFSET_IN_TABLE(BDistance, Position) ((BDistance) * MV_ROW_STRIDE + (Position) * sizeof(IMG_MV_SETTINGS))
#endif

#define MULTICORE_READ32(offset, pointer)                               \
    do {                                                                \
	*(pointer) = *((unsigned long *)((unsigned char *)(linear_mmio_topaz) \
                                         + REG_OFFSET_TOPAZ_MULTICORE + offset)); \
    } while (0)

int topazhp_dump_command(unsigned int comm_dword[]);
int tng_command_parameter_dump(int cmdid, void *virt_addr);
    
#endif
