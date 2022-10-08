/* define bootstub constrains here, like memory map etc. 
 */

#ifndef _BOOT_STUB_HEAD
#define _BOOT_STUB_HEAD

#define CPUID_MASK		0xffff0
#define PENWELL_FAMILY		0x20670
#define CLOVERVIEW_FAMILY	0x30650
#define VALLEYVIEW2_FAMILY	0x30670
#define TANGIER_FAMILY		0x406A0
#define ANNIEDALE_FAMILY	0x506A0

#define MID_CPU_CHIP_LINCROFT	1
#define MID_CPU_CHIP_PENWELL	2
#define MID_CPU_CHIP_CLOVERVIEW 3
#define MID_CPU_CHIP_VALLEYVIEW2 4
#define MID_CPU_CHIP_TANGIER	5
#define MID_CPU_CHIP_ANNIEDALE	6
#define MID_CPU_CHIP_OTHER		0xFF

#define BASE_ADDRESS		0x01100000

#define CMDLINE_OFFSET		BASE_ADDRESS
#define BZIMAGE_SIZE_OFFSET	(CMDLINE_OFFSET + CMDLINE_SIZE)
#define INITRD_SIZE_OFFSET	(BZIMAGE_SIZE_OFFSET + 4)
#define SPI_UART_SUPPRESSION	(INITRD_SIZE_OFFSET + 4)
#define AOSP_HEADER_ADDRESS     0x10007800

#define SPI_TYPE		(SPI_UART_SUPPRESSION + 4) /*0:SPI0  1:SPI1*/
#define SPI_0		0
#define SPI_1		1
#define SPI_2		2

#define FLAGS_RESERVED_0	(SPI_TYPE + 4)
#define FLAGS_RESERVED_1	(FLAGS_RESERVED_0 + 4)
#define VXE_FW_SIZE_OFFSET		(FLAGS_RESERVED_1 + 4)
#define SEC_PLAT_SVCS_SIZE_OFFSET	(VXE_FW_SIZE_OFFSET + 4)
#define XEN_SIZE_OFFSET		(SEC_PLAT_SVCS_SIZE_OFFSET + 4)

#define BOOTSTUB_OFFSET		(BASE_ADDRESS + 0x1000)
#define STACK_OFFSET		0x10f00000
#define BZIMAGE_OFFSET		(BASE_ADDRESS + 0x3000)

#define SETUP_HEADER_OFFSET (BZIMAGE_OFFSET + 0x1F1)
#define SETUP_HEADER_SIZE (0x0202 + *(unsigned char*)(0x0201+BZIMAGE_OFFSET))
#define BOOT_PARAMS_OFFSET 0x8000
#define BOOT_CMDLINE_OFFSET 0x10000
#define SETUP_SIGNATURE 0x5a5aaa55

#define GDT_ENTRY_BOOT_CS       2
#define __BOOT_CS               (GDT_ENTRY_BOOT_CS * 8)

#define GDT_ENTRY_BOOT_DS       (GDT_ENTRY_BOOT_CS + 1)
#define __BOOT_DS               (GDT_ENTRY_BOOT_DS * 8)

#ifdef __ASSEMBLY__
#define GDT_ENTRY(flags, base, limit)			\
	((((base)  & 0xff000000) << (56-24)) |	\
	 (((flags) & 0x0000f0ff) << 40) |	\
	 (((limit) & 0x000f0000) << (48-16)) |	\
	 (((base)  & 0x00ffffff) << 16) |	\
	 (((limit) & 0x0000ffff)))
#else
#define GDT_ENTRY(flags, base, limit)           \
        (((u64)(base & 0xff000000) << 32) |     \
         ((u64)flags << 40) |                   \
         ((u64)(limit & 0x00ff0000) << 32) |    \
         ((u64)(base & 0x00ffffff) << 16) |     \
         ((u64)(limit & 0x0000ffff)))
int get_e820_by_bios(void *e820_buf);
int mid_identify_cpu(void);
void bs_printk(const char *str);
#endif

#endif
