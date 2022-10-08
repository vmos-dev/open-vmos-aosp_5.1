/*
 * dspbridge/mpu_api/inc/memry.h
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
 *  ======== memry.h ========
 *  Purpose:
 *      Functional interface for the memory manager, exported by the DSP
 *      system API DLL.  This interface is not publicly documented.
 *
 *  Public Functions:
 *      MEMRY_Alloc
 *      MEMRY_BindMem
 *      MEMRY_Calloc
 *      MEMRY_Free
 *      MEMRY_FreeVM
 *      MEMRY_LinearAddress
 *      MEMRY_ReserveVM
 *      MEMRY_PageLock
 *      MEMRY_PageUnlock
 *      MEMRY_UnMapLinearAddress
 *
 *! Revision History:
 *! ================
 *! 01-Sep-2001 ag: Added MEMRY_[UnMap]LinearAddress.
 *! 11-Oct-2000 ag: Added MEMRY_Reserve[Free]VM() & MEMRY_BindMem().
 *! 12-Nov-1999 kc: Updated for WinCE.
 *!
 */

#ifndef MEMRY_
#define MEMRY_

#ifdef __cplusplus
extern "C" {
#endif

#include <dspapi.h>

#include <memdefs.h>

/*
 *  MEMRY_[GET]SET]VIRTUALSEGID is used by Node & Strm to access virtual 
 *  address space in the correct client process context. The virtual to 
 *  physical mapping is done in the client process context.
 */
#define MEMRY_SETVIRTUALSEGID   MEM_SETVIRTUALSEGID
#define MEMRY_GETVIRTUALSEGID   MEM_GETVIRTUALSEGID
#define MEMRY_MASKVIRTUALSEGID  MEM_MASKVIRTUALSEGID

#ifndef LINUX

/*
 *  ======== MEMRY_Alloc ========
 *  Purpose:
 *      Allocate memory from the paged or non-paged pools.
 *  Parameters:
 *      cBytes:     Number of bytes to allocate.
 *      type:       Type of memory to allocate; one of:
 *                  - MEM_PAGED:   Allocate from the pageable memory.
 *                  - MEM_NONPAGED:Allocate from page locked memory.
 *  Returns:
 *      Pointer to a block of memory; or NULL if memory couldn't be
 *      allocated.
 *  Requires:
 *  Ensures:
 *      PVOID pointer returned is a valid memory location.
 */
	extern PVOID MEMRY_Alloc(ULONG cBytes, MEM_POOLATTRS type);

/*
 *  ======== MEMRY_BindBuf ========
 *  Purpose:
 *      Bind a Physical address to a Virtual Address.
 *      In WinCE performs a VirtualCopy().
 *  Parameters:
 *      pVA:        Ptr to reserved memory allocated by MEMRY_ReserveVM().
 *      pPA:        Ptr to a physical memory location.
 *      ulBytes:    Size of physical memory in bytes.
 *  Returns:
 *      TRUE if successful, else FALSE.
 *  Requires:
 *     pPA != NULL.
 *  Ensures:
 */
	extern bool MEMRY_BindMem(PVOID pVA, PVOID pPA, ULONG ulBytes);

/*
 *  ======== MEMRY_Calloc ========
 *  Purpose:
 *      Allocate zero-initialized memory from the paged or non-paged pools.
 *  Parameters:
 *      cBytes:     Number of bytes to allocate.
 *      type:       Type of memory to allocate; one of:
 *                  - MEM_PAGED:     Allocate from the pageable memory.
 *                  - MEM_NONPAGED:  Allocate from page locked memory.
 *  Returns:
 *      Pointer to a contiguous block of zeroed memory; or NULL if memory
 *      couldn't be allocated.
 *  Requires:
 *  Ensures:
 *      PVOID pointer returned is a valid memory location.
 */
	extern PVOID WINAPI MEMRY_Calloc(ULONG cBytes, MEM_POOLATTRS type);

/*
 *  ======== MEMRY_Free ========
 *  Purpose:
 *      Free the given block of system memory.
 *  Parameters:
 *      pMemBuf: Pointer to memory allocated by MEMRY_Alloc().
 *  Returns:
 *  Requires:
 *  Ensures:
 *      pMemBuf is no longer a valid pointer to memory.
 */
	extern VOID MEMRY_Free(IN PVOID pMemBuf);

/*
 *  ======== MEMRY_FreeVM ========
 *  Purpose:
 *      Free VM reserved by MEMRY_ReserveVM.
 *  Parameters:
 *      pVirtualAddr: Pointer to memory VM allocated by MEMRY_ReserveVM().
 *  Returns:
 *      TRUE on success, else FALSE.
 *  Requires:
 *     pVirtualAddr != 0
 *  Ensures:
 *
 */
	extern bool MEMRY_FreeVM(PVOID pVirtualAddr);

/*
 *  ======== MEMRY_PageLock ========
 *  Purpose:
 *      Calls kernel services to map the set of pages identified by a private
 *      process pointer and a byte count into the calling process's globally
 *      shared address space.
 *  Parameters
 *      lpBuffer:       Pointer to a process-private data buffer.
 *      cSize:          Size in bytes of the data buffer.
 *  Returns:
 *      A pointer to linear page locked memory, or
 *      NULL if failure locking memory.
 *  Requires:
 *      The size (cSize) must accurately reflect the size of the buffer to
 *      be locked, since the page count is derived from this number.
 *  Ensures:
 *      Memory locked by this service can be accessed at interrupt time, or
 *      from other memory contexts.
 */
	extern DSPAPIDLL PVOID WINAPI MEMRY_PageLock(PVOID pBuffer,
						     ULONG cSize);

#endif				/* ifndef LINUX */

/*
 *  ======== MEMRY_LinearAddress ========
 *  Purpose:
 *      Get the linear address corresponding to the given physical address.
 *  Parameters:
 *      pPhysAddr:      Physical address to be mapped.
 *      cBytes:         Number of bytes in physical range to map.
 *  Returns:
 *      The corresponding linear address, or NULL if unsuccessful.
 *  Requires:
 *     PhysAddr != 0
 *  Ensures:
 *  Notes:
 *      If valid linear address is returned, be sure to call
 *      MEMRY_UnMapLinearAddress().
 */
	extern inline PVOID MEMRY_LinearAddress(PVOID pPhyAddr, ULONG cBytes) {
		return pPhyAddr;
	}
#ifndef LINUX
/*
 *  ======== MEMRY_PageUnlock ========
 *  Purpose:
 *      Unlocks a buffer previously locked using MEMRY_PageLock().
 *  Parameters:
 *      pBuffer:    Pointer to locked memory (as returned by MEMRY_PageLock()).
 *      cSize:      Size in bytes of the buffer.
 *  Returns:
 *      Returns DSP_SOK if unlock successful; else, returns DSP_EFAIL;
 *  Requires:
 *      pBuffer must be a pointer to a locked, shared data buffer previously
 *      locked with MEMRY_PageLock().
 *  Ensures:
 *      Will unlock the pages of memory when the lock count drops to zero.
 *      MEMRY_PageLock() increments the lock count, and MEMRY_PageUnlock
 *      decrements the count.
 */ extern DSPAPIDLL MEMRY_PageUnlock(PVOID pBuffer, ULONG cSize);

/*
 *  ======== MEMRY_ReserveVM ========
 *  Purpose:
 *    Reserve at least ulBytes (page size inc) virtual memory for this process.
 *  Parameters:
 *      ulBytes:   Size in bytes of the minimum space to reserve.
 *  Returns:
 *     Returns NULL on failure, else valid VA of at least ulBytes size.
 *  Requires:
 *  Ensures:
 */
	extern PVOID MEMRY_ReserveVM(ULONG cBytes);

#endif				/* ifndef LINUX */

/*
 *  ======== MEMRY_UnMapLinearAddress ========
 *  Purpose:
 *      Unmap the linear address mapped in MEMRY_LinearAddress.
 *  Parameters:
 *      pBaseAddr:  Ptr to mapped memory (as returned by MEMRY_LinearAddress()).
 *  Returns:
 *  Requires:
 *      - pBaseAddr is a valid linear address mapped in MEMRY_LinearAddress.
 *  Ensures:
 *      - pBaseAddr no longer points to a valid linear address.
 */
	extern inline VOID MEMRY_UnMapLinearAddress(PVOID pBaseAddr) {
	}

#ifdef __cplusplus
}
#endif

#endif				/* MEMRY_ */
