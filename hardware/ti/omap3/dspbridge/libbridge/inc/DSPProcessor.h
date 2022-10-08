/*
 * dspbridge/mpu_api/inc/DSPProcessor.h
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
 *  ======== DSPProcessor.h ========
 *  Description:
 *      This is the header for the DSP/BIOS Bridge processor module.
 *
 *  Public Functions:
 *      DSPProcessor_Attach
 *      DSPProcessor_Detach
 *      DSPProcessor_EnumNodes
 *      DSPProcessor_FlushMemory
 *      DSPProcessor_GetResourceInfo
 *      DSPProcessor_GetState
 *      DSPProcessor_Map
 *      DSPProcessor_RegisterNotify
 *      DSPProcessor_ReserveMemory
 *      DSPProcessor_UnMap
 *      DSPProcessor_UnReserveMemory
 *      DSPProcessor_InvalidateMemory

 *  Notes:
 *
 *! Revision History:
 *! ================
 *! 04-04-2007  sh  Added DSPProcessor_InvalidateMemory
 *! 19-Apr-2004 sb  Aligned DMM definitions with Symbian
 *! 08-Mar-2004 sb  Added the Dynamic Memory Mapping APIs
 *! 23-Nov-2002 gp: Comment cleanup.
 *! 13-Feb-2001 kc: DSP/BIOS Bridge name updates.
 *! 29-Nov-2000 rr: OEM Fxns moved to DSPProcessor_OEM.h
 *!                 Incorporated code review changes.
 *! 27-Oct-2000 jeh Changed uNotifyMask to uNotifyType.
 *! 28-Sep-2000 rr: Updated to Version 0.9.
 *! 07-Sep-2000 jeh Changed type HANDLE in DSPProcessor_RegisterNotify to
 *!                 DSP_HNOTIFICATION.
 *! 14-Aug-2000 rr: Cleaned up.
 *! 20-Jul-2000 rr: Updated to Version 0.8
 *! 27-Jun-2000 rr: Created from DBAPI.h
 */

#ifndef DSPPROCESSOR_
#define DSPPROCESSOR_

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  ======== DSPProcessor_Attach ========
 *  Purpose:
 *      Prepare for communication with a particular DSP processor, and return
 *      a processor handle for subsequent operations.
 *  Parameters:
 *      uProcessor:             The processor index.
 *      pAttrIn:                Ptr to the DSP_PROCESSORATTRIN structure.
 *                              A NULL value means use default values.
 *      phProcessor:            Ptr to location to store processor handle.
 *  Returns:
 *      DSP_SOK:                Success.
 *      DSP_EPOINTER:           Parameter phProcessor is not valid.
 *      DSP_EINVALIDARG:        Parameter uProcessor is invalid
 *      DSP_EFAIL:              Unable to attach the processor
 *      DSP_SALREADYATTACHED:   Success; Processor already attached.
 *  Details:
 *      Returns DSP_EINVALIDARG if uProcessor index >= number of processors.
 *      When pAttrIn is NULL, the default timeout value is 10 seconds.
 *      This call does not affect the actual execution state of the DSP.
 */
	extern DBAPI DSPProcessor_Attach(UINT uProcessor,
					 OPTIONAL CONST struct DSP_PROCESSORATTRIN *
					 pAttrIn,
					 OUT DSP_HPROCESSOR * phProcessor);

/*
 *  ======== DSPProcessor_Detach ========
 *  Purpose:
 *      Detach from a DSP processor and de-allocate all (GPP) resources reserved
 *      for it.
 *  Parameters:
 *      hProcessor:     The processor handle.
 *  Returns:
 *      DSP_SOK:        Success.
 *      DSP_EHANDLE:    Invalid processor handle.
 *      DSP_EFAIL:      Unable to detach from the processor.
 *  Details:
 *      This call does not affect the actual execution state of the DSP.
 */
	extern DBAPI DSPProcessor_Detach(DSP_HPROCESSOR hProcessor);

/*
 *  ======== DSPProcessor_EnumNodes ========
 *  Purpose:
 *      Enumerate the nodes currently allocated on a processor.
 *  Parameters:
 *      hProcessor:     The processor handle.
 *      aNodeTab:       An array allocated to receive the node handles.
 *      uNodeTabSize:   The number of (DSP_HNODE) handles that can be held
 *                      in aNodeTab.
 *      puNumNodes:     Location where DSPProcessor_EnumNodes will return
 *                      the number of valid handles written to aNodeTab
 *      puAllocated:    Location where DSPProcessor_EnumNodes will return
 *                      the number of nodes that are allocated on the DSP.
 *  Returns:
 *      DSP_SOK:        Success.
 *      DSP_EHANDLE:    Invalid processor handle.
 *      DSP_EPOINTER:   Parameters puNumNodes or puAllocated is not valid
 *      DSP_ESIZE:      The amount of memory allocated for aNodeTab is
 *                      insufficent.   The number of nodes actually
 *                      allocated on the DSP is greater than the value
 *                      specified for uNodeTabSize.
 *      DSP_EFAIL:      A failure occurred during enumeration.
 *  Details:
 */
	extern DBAPI DSPProcessor_EnumNodes(DSP_HPROCESSOR hProcessor,
					    IN DSP_HNODE * aNodeTab,
					    IN UINT uNodeTabSize,
					    OUT UINT * puNumNodes,
					    OUT UINT * puAllocated);

/*
 *  ======== DSPProcessor_FlushMemory ========
 *  Purpose:
 *      Flushes a buffer from the MPU data cache.
 *  Parameters:
 *      hProcessor      :   The processor handle.
 *      pMpuAddr        :   Buffer start address
 *      ulSize          :   Buffer size
 *      ulFlags         :   Reserved.
 *  Returns:
 *      DSP_SOK         :   Success.
 *      DSP_EHANDLE     :   Invalid processor handle.
 *      DSP_EFAIL       :   General failure.
 *  Requires:
 *      PROC Initialized.
 *  Ensures:
 *  Details:
 *      All the arguments are currently ignored.
 */
	extern DBAPI DSPProcessor_FlushMemory(DSP_HPROCESSOR hProcessor,
					      PVOID pMpuAddr,
					      ULONG ulSize, ULONG ulFlags);

/*
 *  ======== DSPProcessor_InvalidateMemory ========
 *  Purpose:
 *      Invalidates  a buffer from the MPU data cache.
 *  Parameters:
 *      hProcessor      :   The processor handle.
 *      pMpuAddr        :   Buffer start address
 *      ulSize          :   Buffer size
 *  Returns:
 *      DSP_SOK         :   Success.
 *      DSP_EHANDLE     :   Invalid processor handle.
 *      DSP_EFAIL       :   General failure.
 *  Requires:
 *      PROC Initialized.
 *  Ensures:
 *  Details:
 */
        extern DBAPI DSPProcessor_InvalidateMemory(DSP_HPROCESSOR hProcessor,
                                              PVOID pMpuAddr,
	                                             ULONG ulSize);

/*
 *  ======== DSPProcessor_GetResourceInfo ========
 *  Purpose:
 *      Get information about a DSP Resources.
 *  Parameters:
 *      hProcessor:         The processor handle.
 *      uResourceType:      Type of resource to be reported.
 *      pResourceInfo:      Ptr to the DSP_RESOURCEINFO structure in which
 *                          the processor resource information will be returned.
 *      uResourceInfoSize:  Size of the DSP_RESOURCEINFO structure.
 *  Returns:
 *      DSP_SOK:            Success.
 *      DSP_EHANDLE:        Invalid processor handle.
 *      DSP_EPOINTER:       Parameter pResourceInfo is not valid
 *      DSP_EVALUE:         Parameter uResourceType is invalid.
 *      DSP_EWRONGSTATE:    The processor is not in the PROC_RUNNING state.
 *      DSP_ETIMEOUT:       A timeout occured before the DSP responded to the
 *                          querry.
 *      DSP_ERESTART:       A Critical error has occured and the DSP is being
 *                          restarted.
 *      DSP_ESIZE:          The size of the specified DSP_RESOURCEINFO struct
 *                          is too small to hold all the information.
 *      DSP_EFAIL:          Unable to get Resource Information
 *  Details:
 */
	extern DBAPI DSPProcessor_GetResourceInfo(DSP_HPROCESSOR hProcessor,
						  UINT uResourceType,
						  OUT struct DSP_RESOURCEINFO *
						  pResourceInfo,
						  UINT uResourceInfoSize);

/*
 *  ======== DSPProcessor_GetState ========
 *  Purpose:
 *      Report the state of the specified DSP processor.
 *  Parameters:
 *      hProcessor:         The processor handle.
 *      pProcStatus:        Ptr to location to store the DSP_PROCESSORSTATE
 *                          structure.
 *      uStateInfoSize:     Size of DSP_PROCESSORSTATE.
 *  Returns:
 *      DSP_SOK:            Success.
 *      DSP_EHANDLE:        Invalid processor handle.
 *      DSP_EPOINTER:       Parameter pProcStatus is not valid.
 *      DSP_EFAIL:          General failure while querying processor state.
 *      DSP_ESIZE:          uStateInfoSize is smaller than sizeof
 *                          DSP_PROCESSORSTATE.
 *  Details:
 */
	extern DBAPI DSPProcessor_GetState(DSP_HPROCESSOR hProcessor,
					   OUT struct DSP_PROCESSORSTATE * pProcStatus,
					   UINT uStateInfoSize);

/*
 *  ======== DSPProcessor_Map ========
 *  Purpose:
 *      Maps a MPU buffer to DSP address space.
 *  Parameters:
 *      hProcessor      :   The processor handle.
 *      pMpuAddr        :   Starting address of the memory region to map.
 *      ulSize          :   Size of the memory region to map.
 *      pReqAddr        :   Requested DSP start address. Offset-adjusted actual
 *                          mapped address is in the last argument.
 *      ppMapAddr       :   Ptr to DSP side mapped BYTE address.
 *      ulMapAttr       :   Optional endianness attributes, virt to phys flag.
 *  Returns:
 *      DSP_SOK         :   Success.
 *      DSP_EHANDLE     :   Invalid processor handle.
 *      DSP_EFAIL       :   General failure.
 *      DSP_EMEMORY     :   MPU side memory allocation error.
 *      DSP_ENOTFOUND   :   Cannot find a reserved region starting with this
 *                      :   address.
 *  Requires:
 *      pMpuAddr is not NULL
 *      ulSize is not zero
 *      ppMapAddr is not NULL
 *      PROC Initialized.
 *  Ensures:
 *  Details:
 */
	extern DBAPI DSPProcessor_Map(DSP_HPROCESSOR hProcessor,
				      PVOID pMpuAddr,
				      ULONG ulSize,
				      PVOID pReqAddr,
				      PVOID * ppMapAddr, ULONG ulMapAttr);

/*
 *  ======== DSPProcessor_RegisterNotify ========
 *  Purpose:
 *      Register to be notified of specific processor events
 *  Parameters:
 *      hProcessor:         The processor handle.
 *      uEventMask:         Type of event to be notified about.
 *      uNotifyType:        Type of notification to be sent.
 *      hNotification:      Handle or event name to be used for notification.
 *                          about, or to de-register this notification.
 *  Returns:
 *      DSP_SOK:            Success.
 *      DSP_EHANDLE:        Invalid processor handle or hNotification.
 *      DSP_EVALUE:         Parameter uEventMask is Invalid
 *      DSP_ENOTIMP:        The notification type specified in uNotifyType
 *                          is not supported.
 *      DSP_EFAIL:          Unable to register for notification.
 *  Details:
 */
	extern DBAPI DSPProcessor_RegisterNotify(DSP_HPROCESSOR hProcessor,
						 UINT uEventMask,
						 UINT uNotifyType,
						 struct DSP_NOTIFICATION*
						 hNotification);

/*
 *  ======== DSPProcessor_ReserveMemory ========
 *  Purpose:
 *      Reserve a virtually contiguous region of DSP address space.
 *  Parameters:
 *      hProcessor      :   The processor handle.
 *      ulSize          :   Size of the address space to reserve.
 *      ppRsvAddr       :   Ptr to DSP side reserved BYTE address.
 *  Returns:
 *      DSP_SOK         :   Success.
 *      DSP_EHANDLE     :   Invalid processor handle.
 *      DSP_EFAIL       :   General failure.
 *      DSP_EMEMORY     :   Cannot reserve chunk of this size.
 *  Requires:
 *      ppRsvAddr is not NULL
 *      PROC Initialized.
 *  Ensures:
 *  Details:
 */
	extern DBAPI DSPProcessor_ReserveMemory(DSP_HPROCESSOR hProcessor,
						ULONG ulSize,
						PVOID * ppRsvAddr);

/*
 *  ======== DSPProcessor_UnMap ========
 *  Purpose:
 *      Removes a MPU buffer mapping from the DSP address space.
 *  Parameters:
 *      hProcessor      :   The processor handle.
 *      pMapAddr        :   Starting address of the mapped memory region.
 *  Returns:
 *      DSP_SOK         :   Success.
 *      DSP_EHANDLE     :   Invalid processor handle.
 *      DSP_EFAIL       :   General failure.
 *      DSP_ENOTFOUND   :   Cannot find a mapped region starting with this
 *                      :   address.
 *  Requires:
 *      pMapAddr is not NULL
 *      PROC Initialized.
 *  Ensures:
 *  Details:
 */
	extern DBAPI DSPProcessor_UnMap(DSP_HPROCESSOR hProcessor,
					PVOID pMapAddr);

/*
 *  ======== DSPProcessor_UnReserveMemory ========
 *  Purpose:
 *      Frees a previously reserved region of DSP address space.
 *  Parameters:
 *      hProcessor      :   The processor handle.
 *      pRsvAddr        :   Ptr to DSP side reservedBYTE address.
 *  Returns:
 *      DSP_SOK         :   Success.
 *      DSP_EHANDLE     :   Invalid processor handle.
 *      DSP_EFAIL       :   General failure.
 *      DSP_ENOTFOUND   :   Cannot find a reserved region starting with this
 *                      :   address.
 *  Requires:
 *      pRsvAddr is not NULL
 *      PROC Initialized.
 *  Ensures:
 *  Details:
 */
	extern DBAPI DSPProcessor_UnReserveMemory(DSP_HPROCESSOR hProcessor,
						  PVOID pRsvAddr);

#ifdef __cplusplus
}
#endif
#endif				/* DSPPROCESSOR_ */
