/*
 * dspbridge/mpu_api/inc/DSPStream.h
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
 *  ======== DSPStream.h ========
 *  Description: 
 *      This is the header for the DSP/BIOS Bridge stream module.
 *
 *  Public Functions:
 *      DSPStream_AllocateBuffers
 *      DSPStream_Close
 *      DSPStream_FreeBuffers
 *      DSPStream_GetInfo
 *      DSPStream_Idle
 *      DSPStream_Issue
 *      DSPStream_Open
 *      DSPStream_Reclaim
 *      DSPStream_RegisterNotify
 *      DSPStream_Select
 *
 *  Notes:
 *
 *! Revision History:
 *! ================
 *! 23-Nov-2002 gp: Comment change: uEventMask is really a "type".
 *! 17-Dec-2001 ag  Fix return codes in DSPStream_[Issue][Reclaim]
 *! 12-Dec-2001 ag  Added DSP_ENOTIMPL error code to DSPStream_Open().
 *! 17-Nov-2001 ag  Added DSP_ETRANSLATE error.
 *!                 Added bufSize param and renamed dwBytes to dwDataSize in
 *!                 DSPStream_[Issue][Reclaim]().
 *! 07-Jun-2001 sg: Made buffer alloc/free fxn names plural.
 *! 13-Feb-2001 kc: DSP/BIOS Bridge name updates.
 *! 27-Sep-2000 jeh Removed DSP_BUFFERATTR parameter from DSPStream_Allocate-
 *!                 Buffer(), since these have been moved to DSP_STREAMATTRIN.
 *! 07-Sep-2000 jeh Changed type HANDLE in DSPStream_RegisterNotify to
 *!                 DSP_HNOTIFICATION.
 *! 20-Jul-2000 rr: Updated to version 0.8
 *! 27-Jun-2000 rr: Created from DBAPI.h
 */

#include <host_os.h>

#ifndef DSPStream_
#define DSPStream_

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  ======== DSPStream_AllocateBuffers ========
 *  Purpose:
 *      Allocate data buffers for use with a specific stream.
 *  Parameters:
 *      hStream:            The stream handle.
 *      uSize:              Size of the buffer
 *      apBuffer:           Ptr to location to hold array of buffers.
 *      uNumBufs:           The number of buffers to allocate of size uSize.
 *  Returns:
 *      DSP_SOK:            Success.
 *      DSP_EHANDLE:        Invalid Stream handle.
 *      DSP_EMEMORY:        Insufficient memory
 *      DSP_EPOINTER:       Parameter apBuffer is not valid.
 *      DSP_EALIGNMENT:     Stream's alignment value not supported.
 *      DSP_ESIZE:          Illegal size.
 *      DSP_EFAIL:          General failure to allocate buffer.
 *  Details:
 */
	extern DBAPI DSPStream_AllocateBuffers(DSP_HSTREAM hStream,
					       UINT uSize, OUT BYTE ** apBuffer,
					       UINT uNumBufs);

/*
 *  ======== DSPStream_Close ========
 *  Purpose:
 *      Close a stream and free the underlying stream object.
 *  Parameters:
 *      hStream:            The stream handle.
 *  Returns:
 *      DSP_SOK:            Success.
 *      DSP_EHANDLE:        Invalid Stream handle.
 *      DSP_EPENDING:       Not all stream buffers have been reclaimed
 *      DSP_EFAIL:          Failure to Close the Stream
 *  Details:
 */
	extern DBAPI DSPStream_Close(DSP_HSTREAM hStream);

/*
 *  ======== DSPStream_FreeBuffers ========
 *  Purpose:
 *      Free a previously allocated stream data buffer.
 *  Parameters:
 *      hStream:            The stream handle.
 *      apBuffer:           The array of buffers to free.
 *      uNumBufs:           The number of buffers.
 *  Returns:
 *      DSP_SOK:            Success.
 *      DSP_EHANDLE:        Invalid Stream handle.
 *      DSP_EPOINTER:       Parameter apBuffer is not valid.
 *      DSP_EFAIL:          Failure to free the data buffers
 *  Details:
 */
	extern DBAPI DSPStream_FreeBuffers(DSP_HSTREAM hStream,
					   IN BYTE ** apBuffer, UINT uNumBufs);

/*
 *  ======== DSPStream_GetInfo ========
 *  Purpose:
 *      Get information about a stream.
 *  Parameters:
 *      hStream:            The stream handle.
 *      pStreamInfo:        Ptr to the DSP_STREAMINFO structure.
 *      uStreamInfoSize:    The size of structure.
 *  Returns:
 *      DSP_SOK:            Success.
 *      DSP_EHANDLE:        Invalid Stream handle.
 *      DSP_EPOINTER:       Parameter pStreamInfo is invalid.
 *      DSP_ESIZE:          uStreamInfoSize is too small to hold all stream
 *                          information.
 *      DSP_EFAIL:          Unable to retrieve Stream info
 *  Details:
 */
	extern DBAPI DSPStream_GetInfo(DSP_HSTREAM hStream,
				       OUT struct DSP_STREAMINFO * pStreamInfo,
				       UINT uStreamInfoSize);

/*
 *  ======== DSPStream_Idle ========
 *  Purpose:
 *      Terminate I/O with a particular stream, and (optionally)
 *      flush output data buffers.
 *  Parameters:
 *      hStream:            The stream handle.
 *      bFlush:             Boolean flag
 *  Returns:
 *      DSP_SOK:            Success.
 *      DSP_EHANDLE:        Invalid Stream handle.
 *      DSP_ETIMEOUT:       Time out occurred.
 *      DSP_ERESTART:       A critical error has
 *                          occurred and the DSP is being restarted.
 *      DSP_EFAIL:          Unable to Idle the stream
 *  Details:
 */
	extern DBAPI DSPStream_Idle(DSP_HSTREAM hStream, bool bFlush);

/*
 *  ======== DSPStream_Issue ========
 *  Purpose:
 *      Send a buffer of data to a stream.
 *  Parameters:
 *      hStream:            The stream handle.
 *      pBuffer:            Ptr to the buffer.
 *      dwDataSize:         Size of data in buffer in bytes.
 *      dwBufSize:          Size of actual buffer in bytes.
 *      dwArg:              User defined buffer context.
 *  Returns:
 *      DSP_SOK:            Success.
 *      DSP_EHANDLE:        Invalid Stream handle.
 *      DSP_EPOINTER:       Invalid pBuffer pointer
 *      DSP_ESTREAMFULL:    The stream has been issued the maximum number
 *                          of buffers allowed in the stream at once;
 *                          buffers must be reclaimed from the stream
 *                          before any more can be issued.
 *      DSP_EFAIL:          Unable to issue the buffer.
 *      DSP_ETRANSLATE:     Unable to map shared buffer to client process.
 *  Details:
 */
	extern DBAPI DSPStream_Issue(DSP_HSTREAM hStream, IN BYTE * pBuffer,
				     ULONG dwDataSize, ULONG dwBufSize,
				     IN DWORD dwArg);

/*
 *  ======== DSPStream_Open ========
 *  Purpose:
 *      Retrieve a stream handle for sending/receiving data buffers
 *      to/from a task node on a DSP.
 *  Parameters:
 *      hNode:              The node handle.
 *      uDirection:         Stream direction: {DSP_TONODE | DSP_FROMNODE}.
 *      uIndex:             Stream index (zero based).
 *      pAttrIn:            Ptr to the stream attributes (optional)
 *      phStream:           Ptr to location to store the stream handle.
 *  Returns:
 *      DSP_SOK:            Success.
 *      DSP_EPOINTER:       Invalid phStream pointer.
 *      DSP_ENODETYPE:      Stream can not be opened for this node type/
 *      DSP_EDIRECTION:     uDirection is invalid
 *      DSP_EVALUE:         uIndex is invalid, or, if pAttrIn != NULL,
 *                          pAttrIn->uSegment is invalid.
 *      DSP_EFAIL:          General failure.
 *      DSP_ESTRMMODE:      Stream mode is invalid.
 *      DSP_EDMACHNL:       DMAChnlId is invalid, if STRMMODE is LDMA or RDMA.
 *      DSP_EHANDLE:        Invalid Stream handle.
 *      DSP_ENOTIMPL:       Stream mode is not supported.
 *
 *  Details:
 */
	extern DBAPI DSPStream_Open(DSP_HNODE hNode, UINT uDirection,
				    UINT uIndex,
				    IN OPTIONAL struct DSP_STREAMATTRIN * pAttrIn,
				    OUT DSP_HSTREAM * phStream);

/*
 *  ======== DSPStream_PrepareBuffer ========
 *  Purpose:
 *      Prepare a buffer that was not allocated by DSPStream_AllocateBuffers
 *      for use with a stream
 *  Parameters:
 *      hStream:            Stream handle
 *      uSize:              Size of the allocated buffer(GPP bytes)
 *      pBffer:             Address of the Allocated buffer
 *  Returns:
 *      DSP_SOK:            Success
 *      DSP_EHANDLE:        Invalid Stream handle
 *      DSP_EPOINTER:       Invalid pBuffer
 *      DSP_EFAIL:          Failure to Prepare a buffer
 */
	extern DBAPI DSPStream_PrepareBuffer(DSP_HSTREAM hStream, UINT uSize,
					     BYTE * pBuffer);

/*
 *  ======== DSPStream_Reclaim ========
 *  Purpose:
 *      Request a buffer back from a stream.
 *  Parameters:
 *      hStream:            The stream handle.
 *      pBufPtr:            Ptr to location to store stream buffer.
 *      pDataSize:          Ptr to location to store data size of the buffer.
 *      pBufSize:           Ptr to location to store actual size of the buffer.
 *      pdwArg:             Ptr to location to store user defined context.
 *  Returns:
 *      DSP_SOK:            Success.
 *      DSP_EHANDLE:        Invalid Stream handle.
 *      DSP_EPOINTER:       One of pBufPtr or pBytes is invalid.
 *      DSP_ETIMEOUT:       Timeout waiting from I/O completion.
 *      DSP_ERESTART:       A critical error has occurred and
 *                          the DSP is being restarted.
 *      DSP_EFAIL:          Unable to Reclaim buffer.
 *      DSP_ETRANSLATE:     Unable to map shared buffer to client process.
 *  Details:
 */
	extern DBAPI DSPStream_Reclaim(DSP_HSTREAM hStream,
				       OUT BYTE ** pBufPtr,
				       OUT ULONG * pDataSize,
				       OUT ULONG * pBufSize,
				       OUT DWORD * pdwArg);

/*
 *  ======== DSPStream_RegisterNotify ========
 *  Purpose:
 *      Register to be notified of specific events for this stream.
 *  Parameters:
 *      hStream:            The stream handle.
 *      uEventMask:         Type of event to be notified about.
 *      uNotifyType:        Type of notification to be sent.
 *      hNotification:      Handle to be used for notification.
 *  Returns:
 *      DSP_SOK:            Success.
 *      DSP_EHANDLE:        Invalid stream handle or invalid hNotification
 *      DSP_EVALUE:         uEventMask is invalid
 *      DSP_ENOTIMP:        Not supported as specified in uNotifyType
 *      DSP_EFAIL:          Unable to Register for notification
 *  Details:
 */
	extern DBAPI DSPStream_RegisterNotify(DSP_HSTREAM hStream,
					      UINT uEventMask, UINT uNotifyType,
					      struct DSP_NOTIFICATION* hNotification);

/*
 *  ======== DSPStream_Select ========
 *  Purpose:
 *      Select a ready stream.
 *  Parameters:
 *      aStreamTab:         Array of stream handles.
 *      nStreams:           Number of streams in array.
 *      pMask:              Pointer to the mask of ready streams.
 *      uTimeout:           Timeout value in milliseconds.
 *  Returns:
 *      DSP_SOK:            Success.
 *      DSP_ERANGE:         nStreams is out of range
 *      DSP_EPOINTER:       Invalid aStreamTab or pMask pointer.
 *      DSP_ETIMEOUT        Timeout occured.
 *      DSP_EFAIL:          Failure to select a stream.
 *      DSP_ERESTART:       A critical error has occurred and
 *                          the DSP is being restarted.
 *  Details:
 */
	extern DBAPI DSPStream_Select(IN DSP_HSTREAM * aStreamTab,
				      UINT nStreams, OUT UINT * pMask,
				      UINT uTimeout);

/*
 *  ======== DSPStream_UnprepareBuffer ========
 *  Purpose:
 *      UnPrepare a buffer that was prepared by DSPStream_PrepareBuffer
 *      and will no longer be used with the stream
 *  Parameters:
 *      hStream:            Stream handle
 *      uSize:              Size of the allocated buffer(GPP bytes)
 *      pBffer:             Address of the Allocated buffer
 *  Returns:
 *      DSP_SOK:            Success
 *      DSP_EHANDLE:        Invalid Stream handle
 *      DSP_EPOINTER:       Invalid pBuffer
 *      DSP_EFAIL:          Failure to UnPrepare a buffer
 */
	extern DBAPI DSPStream_UnprepareBuffer(DSP_HSTREAM hStream, UINT uSize,
					       BYTE * pBuffer);

#ifdef __cplusplus
}
#endif
#endif				/* DSPStream_ */
