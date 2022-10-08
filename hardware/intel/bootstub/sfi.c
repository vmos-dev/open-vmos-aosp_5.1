/*
 * sfi.c - driver for parsing sfi mmap table and build e820 table
 *
 * Copyright (c) 2009, Intel Corporation.
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
 */

#include "types.h"
#include "bootparam.h"
#include "bootstub.h"
#include "mb.h"
#include "sfi.h"

#define SFI_BASE_ADDR		0x000E0000
#define SFI_LENGTH		0x00020000

static unsigned long sfi_search_mmap(unsigned long start, int len)
{
	unsigned long i = 0;
	char *pchar = (char *)start;

	for (i = 0; i < len; i++, pchar++) {
		if (pchar[0] == 'M'
			&& pchar[1] == 'M'
			&& pchar[2] == 'A'
			&& pchar[3] == 'P')
			return start + i;
	}
	return 0;
}

int sfi_add_e820_entry(struct boot_params *bp, memory_map_t *mb_mmap, u64 start, u64 size, int type)
{
        struct e820entry * e820_entry;
	memory_map_t	*mb_mmap_entry;
	int	i;

	if (!bp || !mb_mmap) {
		bs_printk("Bootstub: sfi_add_e820_entry failed\n");
		return -1;
	}

	for (i=0; i < bp->e820_entries; i++) {
		e820_entry = &(bp->e820_map[i]);
		mb_mmap_entry = &(mb_mmap[i]);
		if (e820_entry->addr == start) {
			/* Override size and type */
			e820_entry->size = size;
			e820_entry->type = type;
			mb_mmap_entry->length_low = size;
			mb_mmap_entry->length_high = 0;
			mb_mmap_entry->type = (type == E820_RAM)?1:0;
			return 0;
		}
	}

	/* ASSERT: no duplicate start address found */
	if (bp->e820_entries == E820MAX)
		return -1;

	e820_entry = &(bp->e820_map[bp->e820_entries]);
	mb_mmap_entry = &(mb_mmap[bp->e820_entries]);

	e820_entry->addr = start;
	e820_entry->size = size;
	e820_entry->type = type;

	mb_mmap_entry->size = 20;
	mb_mmap_entry->base_addr_low = start;
	mb_mmap_entry->base_addr_high = 0;
	mb_mmap_entry->length_low = size;
	mb_mmap_entry->length_high = 0;
	mb_mmap_entry->type = (type == E820_RAM)?1:0;

	bp->e820_entries++;

	return 0;
}

void sfi_setup_mmap(struct boot_params *bp, memory_map_t *mb_mmap)
{
	struct sfi_table *sb;
	struct sfi_mem_entry *mentry;
	unsigned long long start, end, size;
	int i, num, type;

	if (!bp || !mb_mmap) {
		bs_printk("Bootstub: sfi_setup_mmap failed\n");
		return;
	}

	bp->e820_entries = 0;

	/* search for sfi mmap table */
	sb = (struct sfi_table *)sfi_search_mmap(SFI_BASE_ADDR, SFI_LENGTH);
	if (!sb) {
		bs_printk("Bootstub: SFI MMAP table not found\n");
		return;
	}
	bs_printk("Bootstub: map SFI MMAP to e820 table\n");
	num = SFI_GET_ENTRY_NUM(sb, sfi_mem_entry);
	mentry = (struct sfi_mem_entry *)sb->pentry;

	for (i = 0; i < num; i++) {
		start = mentry->phy_start;
		size = mentry->pages << 12;
		end = start + size;

		if (start > end)
			continue;

		/* translate SFI mmap type to E820 map type */
		switch (mentry->type) {
		case SFI_MEM_CONV:
			type = E820_RAM;
			break;
		case SFI_MEM_UNUSABLE:
		case SFI_RUNTIME_SERVICE_DATA:
			mentry++;
			continue;
		default:
			type = E820_RESERVED;
		}

		if (sfi_add_e820_entry(bp, mb_mmap, start, size, type) != 0)
			break;

		mentry++;
	}

}
