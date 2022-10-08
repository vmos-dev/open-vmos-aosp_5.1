#ifndef _SFI_H
#define _SFI_H

/* Memory type definitions */
enum sfi_mem_type {
	SFI_MEM_RESERVED,
	SFI_LOADER_CODE,
	SFI_LOADER_DATA,
	SFI_BOOT_SERVICE_CODE,
	SFI_BOOT_SERVICE_DATA,
	SFI_RUNTIME_SERVICE_CODE,
	SFI_RUNTIME_SERVICE_DATA,
	SFI_MEM_CONV,
	SFI_MEM_UNUSABLE,
	SFI_ACPI_RECLAIM,
	SFI_ACPI_NVS,
	SFI_MEM_MMIO,
	SFI_MEM_IOPORT,
	SFI_PAL_CODE,
	SFI_MEM_TYPEMAX,
};

struct sfi_mem_entry {
	enum sfi_mem_type type;
	u64 phy_start;
	u64 vir_start;
	u64 pages;
	u64 attrib;
}__attribute__((packed));

struct sfi_table_header {
	char signature[4];
	u32 length;
	u8 revision;
	u8 checksum;
	char oem_id[6];
	char oem_table_id[8];
}__attribute__((packed));

struct sfi_table {
	struct sfi_table_header header;
	u64 pentry[1];
}__attribute__((packed));

#define SFI_TBL_HEADER_LEN      24

#define SFI_GET_ENTRY_NUM(ptable, entry) \
	((ptable->header.length - SFI_TBL_HEADER_LEN) / \
	(sizeof(struct entry)))

#define E820_RAM	1
#define E820_RESERVED	2
#define E820_ACPI	3
#define E820_NVS	4
#define E820_UNUSABLE	5

extern void sfi_setup_mmap(struct boot_params *bp, memory_map_t *mb_mmap);
extern int sfi_add_e820_entry(struct boot_params *bp, memory_map_t *mb_mmap, u64 start, u64 size, int type);

#endif /* _SFI_H */
