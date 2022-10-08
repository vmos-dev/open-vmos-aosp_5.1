/*
 * e820_bios.S: read e820 by int 15h call.
 *
 * The C language function exported by this file is:
 * int get_e820_by_bios(void *e820_buf);
 * @e820_buf: e820 mem map buffer, allocated by caller
 * return: number of e820 entries
 *
 * Copyright (C) 2013 Intel Corporation.
 * Author: Bin Gao <bin.gao@intel.com>
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

#include "bootstub.h"

/* Real mode low memory layout */
#define IDT_START	0x0
#define RELOCATED_START	0xa000
#define	STACK_START	0xb000
#define DATA_START	0xb200

#define SAVED_GDTR_ADDR	0xb100
#define SAVED_IDTR_ADDR	0xb110
#define	COUNT_ADDR	0xb120
#define	TOTAL_COUNT_ADDR	0xb130
#define MIN_BUF_LEN	20
#define	BUF_LEN		2048
#define MAX_NR_ENTRIES	128

#define SMAP	0x534d4150
#define E820	0xe820

.text
.section ".text.head","ax",@progbits

	.code32
	.globl get_e820_by_bios
get_e820_by_bios:
	jmp	start_32bit

	.balign 16
idtr:
	.word	0xffff
	.long	IDT_START

	.balign 16
gdt:
	.quad	0
	.quad	GDT_ENTRY(0x009b, 0, 0xffff)
	.quad	GDT_ENTRY(0x0093, 0, 0xffff)
gdtr:
	.word	3*8-1
	.long	gdt

saved_esp:
	.long	0

start_32bit:
	pushal
	pushfl

	/* Save ESP, GDTR and IDTR registers */
        movl	$saved_esp, %eax
	movl	%esp, (%eax)
	xorl	%eax, %eax
	sidtl	SAVED_IDTR_ADDR(%eax)
	sgdtl	SAVED_GDTR_ADDR(%eax)

	/* Relocate real mode codes to 64k segment */
	movl    $relocated_end + 4, %ecx
	subl	$relocated_start, %ecx
	shrl	$2, %ecx
        movl    $relocated_start, %esi
        movl    $RELOCATED_START, %edi
        rep     movsl

	/* Set up real mode IDT */
	lidtl	%cs:idtr

	/* Set up real mode GDT */
	lgdtl	%cs:gdtr
	movl	$16, %ecx
	movl	%ecx, %ds
	movl	%ecx, %es
	movl	%ecx, %fs
	movl	%ecx, %gs
	movl	%ecx, %ss

	/* Switch to 16bit segment */
	ljmpl	$8, $RELOCATED_START

	.code16
relocated_start:
reloc_base = .

	/* Switch to real mode */
	andb	$0x10, %al
	movl	%eax, %cr0
	ljmpw	$0, $realmode_entry - relocated_start + RELOCATED_START

realmode_entry = .
	/* In real mode now, set up segment selectors */
	movl	$0, %eax
	movl	%eax, %ds
	movl	%eax, %es
	movl	%eax, %ss
	movl	%eax, %gs
	movl	%eax, %fs

	movl	$STACK_START, %esp

	/* Do int 15h call */
	movl	$COUNT_ADDR, %eax
	movl	$0, (%eax)
	movl	$TOTAL_COUNT_ADDR, %eax
	movl	$0, (%eax)
	xorl	%ebx, %ebx
	movw	$DATA_START, %di
again:
	movw	$E820, %ax
	movw	$BUF_LEN, %cx
	movl	$SMAP, %edx
	int	$0x15
	jc	error	/* EFLGAS.CF is set */
	cmpl	$SMAP, %eax
	jne	error	/* eax is not 'SMAP' */
	cmpw	$MIN_BUF_LEN, %cx
	jl	error /* returned buffer len < 20 */
	cmpw	$BUF_LEN, %cx
	jg	error /* returned buffer len > provided buffer len */
	movl	$TOTAL_COUNT_ADDR, %eax
	addw	%cx, (%eax)
	movl	$COUNT_ADDR, %eax
	incl	(%eax)
	movl	(%eax), %eax
	cmpl	$MAX_NR_ENTRIES, %eax /* max supported entries: 128 */
	jge	done
	testl	%ebx, %ebx /* ebx == 0: done, ebx != 0: continue */
	je	done
	addw	%cx, %di
	jmp	again
done:
	jmp	2f
error:
	movl	$COUNT_ADDR, %eax
	movl	$~0, (%eax)
2:

	/* Switch back to protected mode */
	xorl	%ebx, %ebx
	lidtl	SAVED_IDTR_ADDR(%ebx)
	lgdtl	SAVED_GDTR_ADDR(%ebx)
	movl    %cr0, %ebx
	orb	$1, %bl
	movl    %ebx, %cr0
	.byte	0x66, 0xea /* opcode(JMP FAR) with operand size override */
	.long	resumed_protected_mode	/* offset */
	.word	__BOOT_CS /* segment selector */
relocated_end = .

	.code32
resumed_protected_mode:
	cli /* in case real mode codes turn on interrrupt! */
	/* Restore segment registers */
	movl	$__BOOT_DS, %ebx
	movl	%ebx, %ds
	movl	%ebx, %es
	movl	%ebx, %gs
	movl	%ebx, %fs
	movl	%ebx, %ss

	/* Restore stack pointer */
	movl	$saved_esp, %eax
	movl	(%eax), %esp

	/* Copy e820 data from our buffer to caller's buffer */
	xorl	%eax, %eax
	movl	TOTAL_COUNT_ADDR(%eax), %ecx
	movl    $DATA_START, %esi
	movl    40(%esp), %edi
	rep     movsb

	popfl
	popal

	/* Return number of e820 entries */
	movl	$COUNT_ADDR, %eax
	movl	(%eax), %eax
	ret
