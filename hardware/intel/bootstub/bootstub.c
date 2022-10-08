/*
 * bootstub 32 bit entry setting routings 
 *
 * Copyright (C) 2008-2010 Intel Corporation.
 * Author: Alek Du <alek.du@intel.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "types.h"
#include "bootstub.h"
#include "bootparam.h"
#include "spi-uart.h"
#include "ssp-uart.h"
#include "mb.h"
#include "sfi.h"
#include "bootimg.h"

#include <stdint.h>
#include <stddef.h>
#include "imr_toc.h"

#define PAGE_SIZE_MASK	0xFFF
#define MASK_1K		0x3FF
#define PAGE_ALIGN_FWD(x)       ((x + PAGE_SIZE_MASK) & ~PAGE_SIZE_MASK)
#define PAGE_ALIGN_BACK(x)      ((x) & ~PAGE_SIZE_MASK)

#define IMR_START_ADDRESS(x)	(((x) & 0xFFFFFFFC) << 8)
#define IMR_END_ADDRESS(x)	((x == 0) ? (x) : ((((x) & 0xFFFFFFFC) << 8) | MASK_1K))

#define	IMR6_START_ADDRESS	IMR_START_ADDRESS(*((u32 *)0xff108160))
#define	IMR6_END_ADDRESS	IMR_END_ADDRESS(*((u32 *)0xff108164))
#define	IMR7_START_ADDRESS	IMR_START_ADDRESS(*((u32 *)0xff108170))
#define	IMR7_END_ADDRESS	IMR_END_ADDRESS(*((u32 *)0xff108174))

#define FATAL_HANG()  { asm("cli"); while (1) { asm("nop"); } }

extern int no_uart_used;

extern imr_toc_t imr6_toc;
static u32 imr7_size;

static u32 sps_load_adrs;

static memory_map_t mb_mmap[E820MAX];
u32 mb_magic, mb_info;

struct gdt_ptr {
        u16 len;
        u32 ptr;
} __attribute__((packed));

static void *memcpy(void *dest, const void *src, size_t count)
{
        char *tmp = dest;
        const char *s = src;
	size_t _count = count / 4;

	while (_count--) {
		*(long *)tmp = *(long *)s;
		tmp += 4;
		s += 4;
	}
	count %= 4;
        while (count--)
                *tmp++ = *s++;
        return dest;
}

static void *memset(void *s, unsigned char c, size_t count)
{
        char *xs = s;
	size_t _count = count / 4;
	unsigned long  _c = c << 24 | c << 16 | c << 8 | c;
 
	while (_count--) {
		*(long *)xs = _c;
		xs += 4;
	}
	count %= 4;
        while (count--)
                *xs++ = c; 
        return s;
}

static size_t strnlen(const char *s, size_t maxlen)
{
        const char *es = s;
        while (*es && maxlen) {
                es++;
                maxlen--;
        }

        return (es - s);
}

static const char *strnchr(const char *s, int c, size_t maxlen)
{
    int i;
    for (i = 0; i < maxlen && *s != c; s++, i++)
        ;
    return s;
}

int strncmp(const char *cs, const char *ct, size_t count)
{
	unsigned char c1, c2;

	while (count) {
		c1 = *cs++;
		c2 = *ct++;
		if (c1 != c2)
			return c1 < c2 ? -1 : 1;
		if (!c1)
			break;
		count--;
	}
	return 0;
}

static inline int is_image_aosp(unsigned char *magic)
{
	return !strncmp((char *)magic, (char *)BOOT_MAGIC, sizeof(BOOT_MAGIC)-1);
}

static void setup_boot_params(struct boot_params *bp, struct setup_header *sh)
{
	bp->screen_info.orig_video_mode = 0;
	bp->screen_info.orig_video_lines = 0;
	bp->screen_info.orig_video_cols = 0;
	bp->alt_mem_k = 128*1024; // hard coded 128M mem here, since SFI will override it
	memcpy(&bp->hdr, sh, sizeof (struct setup_header));
	bp->hdr.type_of_loader = 0xff; //bootstub is unknown bootloader for kernel :)
	bp->hdr.hardware_subarch = X86_SUBARCH_MRST;
}

static u32 bzImage_setup(struct boot_params *bp, struct setup_header *sh)
{
	void *cmdline = (void *)BOOT_CMDLINE_OFFSET;
	struct boot_img_hdr *aosp = (struct boot_img_hdr *)AOSP_HEADER_ADDRESS;
	size_t cmdline_len;
	u8 *initramfs, *ptr;

	if (is_image_aosp(aosp->magic)) {
		ptr = (u8*)aosp->kernel_addr;
		cmdline_len = strnlen((const char *)aosp->cmdline, sizeof(aosp->cmdline));

		/*
		* Copy the command line to be after bootparams so that it won't be
		* overwritten by the kernel executable.
		*/
		memset(cmdline, 0, sizeof(aosp->cmdline));
		memcpy(cmdline, (const void *)aosp->cmdline, cmdline_len);

		bp->hdr.ramdisk_size = aosp->ramdisk_size;

		initramfs = (u8 *)aosp->ramdisk_addr;
	} else {
		ptr = (u8*)BZIMAGE_OFFSET;
		cmdline_len = strnlen((const char *)CMDLINE_OFFSET, CMDLINE_SIZE);
		/*
		 * Copy the command line to be after bootparams so that it won't be
		 * overwritten by the kernel executable.
		 */
		memset(cmdline, 0, CMDLINE_SIZE);
		memcpy(cmdline, (const void *)CMDLINE_OFFSET, cmdline_len);

		bp->hdr.ramdisk_size = *(u32 *)INITRD_SIZE_OFFSET;

		initramfs = (u8 *)BZIMAGE_OFFSET + *(u32 *)BZIMAGE_SIZE_OFFSET;
	}

	bp->hdr.cmd_line_ptr = BOOT_CMDLINE_OFFSET;
	bp->hdr.cmdline_size = cmdline_len;
	bp->hdr.ramdisk_image = (bp->alt_mem_k*1024 - bp->hdr.ramdisk_size) & 0xFFFFF000;

	if (*initramfs) {
		bs_printk("Relocating initramfs to high memory ...\n");
		memcpy((u8*)bp->hdr.ramdisk_image, initramfs, bp->hdr.ramdisk_size);
	} else {
		bs_printk("Won't relocate initramfs, are you in SLE?\n");
	}

	while (1){
		if (*(u32 *)ptr == SETUP_SIGNATURE && *(u32 *)(ptr+4) == 0)
			break;
		ptr++;
	}
	ptr+=4;
	return (((unsigned int)ptr+511)/512)*512;
}

static inline void cpuid(u32 op, u32 regs[4])
{
	__asm__ volatile (
		"mov %%ebx, %%edi\n"
		"cpuid\n"
		"xchg %%edi, %%ebx\n"
		: "=a"(regs[0]), "=D"(regs[1]), "=c"(regs[2]), "=d"(regs[3])
		: "a"(op)
		);
}

enum cpuid_regs {
	CR_EAX = 0,
	CR_ECX,
	CR_EDX,
	CR_EBX
};

int mid_identify_cpu(void)
{
	u32 regs[4];

	cpuid(1, regs);

	switch ( regs[CR_EAX] & CPUID_MASK ) {

	case PENWELL_FAMILY:
		return MID_CPU_CHIP_PENWELL;
	case CLOVERVIEW_FAMILY:
		return MID_CPU_CHIP_CLOVERVIEW;
	case VALLEYVIEW2_FAMILY:
		return MID_CPU_CHIP_VALLEYVIEW2;
	case TANGIER_FAMILY:
		return MID_CPU_CHIP_TANGIER;
	case ANNIEDALE_FAMILY:
		return MID_CPU_CHIP_ANNIEDALE;
	default:
		return MID_CPU_CHIP_OTHER;
	}
}

static void setup_spi(void)
{
	if (!(*(int *)SPI_TYPE)) {
		switch ( mid_identify_cpu() ) {

		case MID_CPU_CHIP_PENWELL:
			*(int *)SPI_TYPE = SPI_1;
			bs_printk("PNW detected\n");
			break;

		case MID_CPU_CHIP_CLOVERVIEW:
			*(int *)SPI_TYPE = SPI_1;
			bs_printk("CLV detected\n");
			break;

		case MID_CPU_CHIP_TANGIER:
			*(int *)SPI_TYPE = SPI_2;
			bs_printk("MRD detected\n");
			break;

		case MID_CPU_CHIP_ANNIEDALE:
			*(int *)SPI_TYPE = SPI_2;
			bs_printk("ANN detected\n");
			break;

		case MID_CPU_CHIP_VALLEYVIEW2:
		case MID_CPU_CHIP_OTHER:
		default:
			no_uart_used = 1;
		}
	}
}

static void setup_gdt(void)
{
        static const u64 boot_gdt[] __attribute__((aligned(16))) = {
                /* CS: code, read/execute, 4 GB, base 0 */
                [GDT_ENTRY_BOOT_CS] = GDT_ENTRY(0xc09b, 0, 0xfffff),
                /* DS: data, read/write, 4 GB, base 0 */
                [GDT_ENTRY_BOOT_DS] = GDT_ENTRY(0xc093, 0, 0xfffff),
        };
        static struct gdt_ptr gdt;

        gdt.len = sizeof(boot_gdt)-1;
        gdt.ptr = (u32)&boot_gdt;

        asm volatile("lgdtl %0" : : "m" (gdt));
}

static void setup_idt(void)
{
        static const struct gdt_ptr null_idt = {0, 0};
        asm volatile("lidtl %0" : : "m" (null_idt));
}

static void vxe_fw_setup(void)
{
	u8 *vxe_fw_image;
	u32 vxe_fw_size;
	u32 vxe_fw_load_adrs;

	vxe_fw_size = *(u32*)VXE_FW_SIZE_OFFSET;
	/* do we have a VXE FW image? */
	if (vxe_fw_size == 0)
		return;

	/* Do we have enough room to load the image? */
	if (vxe_fw_size > imr6_toc.entries[IMR_TOC_ENTRY_VXE_FW].size) {
		bs_printk("FATAL ERROR: VXE FW image size is too large for IMR\n");
		FATAL_HANG();
	}

	vxe_fw_image = (u8 *)(
		BZIMAGE_OFFSET
		+ *(u32 *)BZIMAGE_SIZE_OFFSET
		+ *(u32 *)INITRD_SIZE_OFFSET
	);

	vxe_fw_load_adrs = IMR6_START_ADDRESS + imr6_toc.entries[IMR_TOC_ENTRY_VXE_FW].start_offset;
	memcpy((u8 *)vxe_fw_load_adrs, vxe_fw_image, vxe_fw_size);
}

static void load_imr_toc(u32 imr, u32 imrsize, imr_toc_t *toc, u32 tocsize)
{
	if (imr == 0 || imrsize == 0 || toc == NULL || tocsize == 0 || imrsize < tocsize )
	{
                bs_printk("FATAL ERROR: TOC size is too large for IMR\n");
		FATAL_HANG();
	}
	memcpy((u8 *)imr, (u8 *)toc, tocsize);
}


static u32 xen_multiboot_setup(void)
{
	u32 *magic, *xen_image, i;
	char *src, *dst;
	u32 xen_size;
	u32 xen_jump_adrs;
	static module_t modules[3];
	static multiboot_info_t mb = {
		.flags = MBI_CMDLINE | MBI_MODULES | MBI_MEMMAP | MBI_DRIVES,
		.mmap_addr = (u32)mb_mmap,
		.mods_count = 3,
		.mods_addr = (u32)modules,
	};

	xen_size =  *(u32 *)XEN_SIZE_OFFSET;
	/* do we have a xen image? */
	if (xen_size == 0) {
		return 0;
        }

	/* Compute the actual offset of the Xen image */
	xen_image = (u32*)(
		BZIMAGE_OFFSET
		+ *(u32 *)BZIMAGE_SIZE_OFFSET
		+ *(u32 *)INITRD_SIZE_OFFSET
		+ *(u32 *)VXE_FW_SIZE_OFFSET
		+ *(u32 *)SEC_PLAT_SVCS_SIZE_OFFSET
	);

	/* the multiboot signature should be located in the first 8192 bytes */
	for (magic = xen_image; magic < xen_image + 2048; magic++)
		if (*magic == MULTIBOOT_HEADER_MAGIC)
			break;
	if (*magic != MULTIBOOT_HEADER_MAGIC) {
		return 0;
        }

	mb.cmdline = (u32)strnchr((char *)CMDLINE_OFFSET, '$', CMDLINE_SIZE) + 1;
	dst = (char *)mb.cmdline + strnlen((const char *)mb.cmdline, CMDLINE_SIZE) - 1;
	*dst = ' ';
	dst++;
	src = (char *)CMDLINE_OFFSET;
	for (i = 0 ;i < strnlen((const char *)CMDLINE_OFFSET, CMDLINE_SIZE);i++) {
		if (!strncmp(src, "capfreq=", 8)) {
			while (*src != ' ' && *src != 0) {
				*dst = *src;
				dst++;
				src++;
			}
			break;
		}
		src++;
	}

	/* fill in the multiboot module information: dom0 kernel + initrd + Platform Services Image */
	modules[0].mod_start = BZIMAGE_OFFSET;
	modules[0].mod_end = BZIMAGE_OFFSET + *(u32 *)BZIMAGE_SIZE_OFFSET;
	modules[0].string = CMDLINE_OFFSET;

	modules[1].mod_start = modules[0].mod_end ;
	modules[1].mod_end = modules[1].mod_start + *(u32 *)INITRD_SIZE_OFFSET;
	modules[1].string = 0;

	modules[2].mod_start = sps_load_adrs;
	modules[2].mod_end = modules[2].mod_start + *(u32 *)SEC_PLAT_SVCS_SIZE_OFFSET;
	modules[2].string = 0;

	mb.drives_addr = IMR6_START_ADDRESS + imr6_toc.entries[IMR_TOC_ENTRY_XEN_EXTRA].start_offset;
	mb.drives_length = imr6_toc.entries[IMR_TOC_ENTRY_XEN_EXTRA].size;

	for(i = 0; i < E820MAX; i++)
		if (!mb_mmap[i].size)
			break;
	mb.mmap_length = i * sizeof(memory_map_t);

	/* relocate xen to start address */
	if (xen_size > imr7_size) {
		bs_printk("FATAL ERROR: Xen image size is too large for IMR\n");
		FATAL_HANG();
	}
	xen_jump_adrs = IMR7_START_ADDRESS;
	memcpy((u8 *)xen_jump_adrs, xen_image, xen_size);

	mb_info = (u32)&mb;
	mb_magic = MULTIBOOT_BOOTLOADER_MAGIC;

	return (u32)xen_jump_adrs;
}

static void sec_plat_svcs_setup(void)
{
	u8 *sps_image;
	u32 sps_size;

	sps_size = PAGE_ALIGN_FWD(*(u32*)SEC_PLAT_SVCS_SIZE_OFFSET);
	/* do we have a SPS image? */
	if (sps_size == 0)
		return;

	/* Do we have enough room to load the image? */
	if (sps_size > imr7_size) {
		bs_printk("FATAL ERROR: SPS image size is too large for IMR\n");
		FATAL_HANG();
	}

	sps_image = (u8 *)(
		BZIMAGE_OFFSET
		+ *(u32 *)BZIMAGE_SIZE_OFFSET
		+ *(u32 *)INITRD_SIZE_OFFSET
		+ *(u32 *)VXE_FW_SIZE_OFFSET
	);

	/* load SPS image (with assumed CHAABI Mailboxes suffixed) */
	/* at bottom of IMR7 */
	/* Must be page-aligned or Xen will panic */
	sps_load_adrs = PAGE_ALIGN_BACK(IMR7_START_ADDRESS + imr7_size - sps_size);
	memcpy((u8 *)sps_load_adrs, sps_image, sps_size);

	/* reduce remaining size for Xen image size check */
	imr7_size -= sps_size;
}

int bootstub(void)
{
	u32 jmp;
	struct boot_img_hdr *aosp = (struct boot_img_hdr *)AOSP_HEADER_ADDRESS;
	struct boot_params *bp = (struct boot_params *)BOOT_PARAMS_OFFSET;
	struct setup_header *sh;
	u32 imr_size;
	int nr_entries;

	if (is_image_aosp(aosp->magic)) {
		sh = (struct setup_header *)((unsigned  int)aosp->kernel_addr + 0x1F1);
		/* disable the bs_printk through SPI/UART */
		*(int *)SPI_UART_SUPPRESSION = 1;
		*(int *)SPI_TYPE = SPI_2;
	} else
		sh = (struct setup_header *)SETUP_HEADER_OFFSET;

	setup_idt();
	setup_gdt();
	setup_spi();
	bs_printk("Bootstub Version: 1.4 ...\n");

	memset(bp, 0, sizeof (struct boot_params));

	if (mid_identify_cpu() == MID_CPU_CHIP_VALLEYVIEW2) {
		nr_entries = get_e820_by_bios(bp->e820_map);
		bp->e820_entries = (nr_entries > 0) ? nr_entries : 0;
	} else {
	        sfi_setup_mmap(bp, mb_mmap);
	}

	if ((mid_identify_cpu() != MID_CPU_CHIP_TANGIER) && (mid_identify_cpu() != MID_CPU_CHIP_ANNIEDALE)) {
		if ((IMR6_END_ADDRESS > IMR6_START_ADDRESS) && (IMR7_END_ADDRESS > IMR7_START_ADDRESS)) {
			imr_size  = PAGE_ALIGN_FWD(IMR6_END_ADDRESS - IMR6_START_ADDRESS);
			load_imr_toc(IMR6_START_ADDRESS, imr_size, &imr6_toc, sizeof(imr6_toc));
			vxe_fw_setup();
			sfi_add_e820_entry(bp, mb_mmap, IMR6_START_ADDRESS, imr_size, E820_RESERVED);

			imr7_size  = PAGE_ALIGN_FWD(IMR7_END_ADDRESS - IMR7_START_ADDRESS);
			sec_plat_svcs_setup();
			sfi_add_e820_entry(bp, mb_mmap, IMR7_START_ADDRESS, imr7_size, E820_RESERVED);
		} else {
			*(u32 *)XEN_SIZE_OFFSET = 0;	/* Don't allow Xen to boot */
		}
	} else {
		*(u32 *)XEN_SIZE_OFFSET = 0;	/* Don't allow Xen to boot */
	}

	setup_boot_params(bp, sh);

	jmp = xen_multiboot_setup();
	if (!jmp) {
		bs_printk("Using bzImage to boot\n");
		jmp = bzImage_setup(bp, sh);
	} else
		bs_printk("Using multiboot image to boot\n");

	bs_printk("Jump to kernel 32bit entry\n");
	return jmp;
}

void bs_printk(const char *str)
{
        if (*(int *)SPI_UART_SUPPRESSION)
                return;

        switch (*(int *)SPI_TYPE) {

        case SPI_1:
                bs_spi_printk(str);
                break;

        case SPI_2:
                bs_ssp_printk(str);
                break;
        }
}
