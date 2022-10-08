/*
 * dspbridge/mpu_api/inc/errbase.h
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
 *  ======== errbase.h ========
 *  Description:
 *      Central repository for DSP/BIOS Bridge error and status code.
 *
 *  Error codes are of the form:
 *      [<MODULE>]_E<ERRORCODE>
 *
 *  Success codes are of the form:
 *      [<MODULE>]_S<SUCCESSCODE>
 *
 *! Revision History:
 *! ================
 *! 24-Jan-2003 map Added DSP_SALREADYLOADED for persistent library checking
 *! 23-Nov-2002 gp: Minor comment cleanup.
 *! 13-May-2002 sg  Added DSP_SALREADYASLEEP and DSP_SALREADYWAKE.
 *! 18-Feb-2002 mk: Added DSP_EOVERLAYMEMORY, EFWRITE, ENOSECT.
 *! 31-Jan-2002 mk: Added definitions of DSP_STRUE and DSP_SFALSE.
 *! 29-Jan-2002 mk: Added definition of CFG_E_INSUFFICIENTBUFSIZE.
 *! 24-Oct-2001 sp: Consolidated all the error codes into this file.
 *! 24-Jul-2001 mk: Type-casted all definitions of WSX_STATUS types for
 *!                 removal of compile warnings.
 *! 22-Nov-1999 kc: Changes from code review.
 *! 18-Aug-1999 rr: Ported From WSX.
 *! 29-May-1996 gp: Removed WCD_ and WMD_ error ranges. Redefined format of
 *!                 error codes.
 *! 10-May-1996 gp: Created.
 */

#ifndef ERRBASE_
#define ERRBASE_

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/* Base of generic errors and component errors                                */
/* ========================================================================== */
#define DSP_SBASE               (DSP_STATUS)0x00008000
#define DSP_EBASE               (DSP_STATUS)0x80008000

#define DSP_COMP_EBASE          (DSP_STATUS)0x80040200
#define DSP_COMP_ELAST          (DSP_STATUS)0x80047fff

/* ========================================================================== */
/* SUCCESS Codes                                                              */
/* ========================================================================== */

/* -------------------------------------------------------------------------- */
/* Generic success code                                                       */
/* -------------------------------------------------------------------------- */
#define DSP_SOK                     (DSP_SBASE + 0)

/* -------------------------------------------------------------------------- */
/* GPP is already attached to this DSP processor                              */
/* -------------------------------------------------------------------------- */
#define DSP_SALREADYATTACHED        (DSP_SBASE + 1)

/* -------------------------------------------------------------------------- */
/* This is the last object available for enumeration.                         */
/* -------------------------------------------------------------------------- */
#define DSP_SENUMCOMPLETE           (DSP_SBASE + 2)

/* -------------------------------------------------------------------------- */
/* The DSP is already asleep.                                                 */
/* -------------------------------------------------------------------------- */
#define DSP_SALREADYASLEEP          (DSP_SBASE + 3)

/* -------------------------------------------------------------------------- */
/* The DSP is already awake.                                                 */
/* -------------------------------------------------------------------------- */
#define DSP_SALREADYAWAKE           (DSP_SBASE + 4)

/* -------------------------------------------------------------------------- */
/* TRUE                                                                       */
/* -------------------------------------------------------------------------- */
#define DSP_STRUE                   (DSP_SBASE + 5)

/* -------------------------------------------------------------------------- */
/* FALSE                                                                      */
/* -------------------------------------------------------------------------- */
#define DSP_SFALSE                  (DSP_SBASE + 6)

/* -------------------------------------------------------------------------- */
/* A library contains no dependent library references                         */
/* -------------------------------------------------------------------------- */
#define DSP_SNODEPENDENTLIBS        (DSP_SBASE + 7)

/* -------------------------------------------------------------------------- */
/* A persistent library is already loaded by the dynamic loader               */
/* -------------------------------------------------------------------------- */
#define DSP_SALREADYLOADED          (DSP_SBASE + 8)

/* -------------------------------------------------------------------------- */
/* Some error occured, but it is OK to continue                               */
/* -------------------------------------------------------------------------- */
#define DSP_OKTO_CONTINUE          (DSP_SBASE + 9)

/* ========================================================================== */
/* FAILURE Codes                                                              */
/* ========================================================================== */

/* -------------------------------------------------------------------------- */
/* The caller does not have access privileges to call this function           */
/* -------------------------------------------------------------------------- */
#define DSP_EACCESSDENIED           (DSP_EBASE + 0)

/* -------------------------------------------------------------------------- */
/* The Specified Connection already exists                                    */
/* -------------------------------------------------------------------------- */
#define DSP_EALREADYCONNECTED       (DSP_EBASE + 1)

/* -------------------------------------------------------------------------- */
/* The GPP must be detached from the DSP before this function is called       */
/* -------------------------------------------------------------------------- */
#define DSP_EATTACHED               (DSP_EBASE + 2)

/* -------------------------------------------------------------------------- */
/* During enumeration a change in the number or properties of the objects     */
/* has occurred.                                                              */
/* -------------------------------------------------------------------------- */
#define DSP_ECHANGEDURINGENUM       (DSP_EBASE + 3)

/* -------------------------------------------------------------------------- */
/* An error occurred while parsing the DSP executable file                    */
/* -------------------------------------------------------------------------- */
#define DSP_ECORRUPTFILE            (DSP_EBASE + 4)

/* -------------------------------------------------------------------------- */
/* A failure occurred during a delete operation                               */
/* -------------------------------------------------------------------------- */
#define DSP_EDELETE                 (DSP_EBASE + 5)

/* -------------------------------------------------------------------------- */
/* The specified direction is invalid                                         */
/* -------------------------------------------------------------------------- */
#define DSP_EDIRECTION              (DSP_EBASE + 6)

/* -------------------------------------------------------------------------- */
/* A stream has been issued the maximum number of buffers allowed in the      */
/* stream at once ;  buffers must be reclaimed from the stream before any     */
/* more can be issued.                                                        */
/* -------------------------------------------------------------------------- */
#define DSP_ESTREAMFULL             (DSP_EBASE + 7)

/* -------------------------------------------------------------------------- */
/* A general failure occurred                                                 */
/* -------------------------------------------------------------------------- */
#define DSP_EFAIL                   (DSP_EBASE + 8)

/* -------------------------------------------------------------------------- */
/* The specified executable file could not be found.                          */
/* -------------------------------------------------------------------------- */
#define DSP_EFILE                   (DSP_EBASE + 9)

/* -------------------------------------------------------------------------- */
/* The specified handle is invalid.                                           */
/* -------------------------------------------------------------------------- */
#define DSP_EHANDLE                 (DSP_EBASE + 0xa)

/* -------------------------------------------------------------------------- */
/* An invalid argument was specified.                                         */
/* -------------------------------------------------------------------------- */
#define DSP_EINVALIDARG             (DSP_EBASE + 0xb)

/* -------------------------------------------------------------------------- */
/* A memory allocation failure occurred.                                      */
/* -------------------------------------------------------------------------- */
#define DSP_EMEMORY                 (DSP_EBASE + 0xc)

/* -------------------------------------------------------------------------- */
/* The requested operation is invalid for this node type.                     */
/* -------------------------------------------------------------------------- */
#define DSP_ENODETYPE               (DSP_EBASE + 0xd)

/* -------------------------------------------------------------------------- */
/* No error text was found for the specified error code.                      */
/* -------------------------------------------------------------------------- */
#define DSP_ENOERRTEXT              (DSP_EBASE + 0xe)

/* -------------------------------------------------------------------------- */
/* No more connections can be made for this node.                             */
/* -------------------------------------------------------------------------- */
#define DSP_ENOMORECONNECTIONS      (DSP_EBASE + 0xf)

/* -------------------------------------------------------------------------- */
/* The indicated operation is not supported.                                  */
/* -------------------------------------------------------------------------- */
#define DSP_ENOTIMPL                (DSP_EBASE + 0x10)

/* -------------------------------------------------------------------------- */
/* I/O is currently pending.                                                  */
/* -------------------------------------------------------------------------- */
#define DSP_EPENDING                (DSP_EBASE + 0x11)

/* -------------------------------------------------------------------------- */
/* An invalid pointer was specified.                                          */
/* -------------------------------------------------------------------------- */
#define DSP_EPOINTER                (DSP_EBASE + 0x12)

/* -------------------------------------------------------------------------- */
/* A parameter is specified outside its valid range.                          */
/* -------------------------------------------------------------------------- */
#define DSP_ERANGE                  (DSP_EBASE + 0x13)

/* -------------------------------------------------------------------------- */
/* An invalid size parameter was specified.                                    */
/* -------------------------------------------------------------------------- */
#define DSP_ESIZE                   (DSP_EBASE + 0x14)

/* -------------------------------------------------------------------------- */
/* A stream creation failure occurred on the DSP.                             */
/* -------------------------------------------------------------------------- */
#define DSP_ESTREAM                 (DSP_EBASE + 0x15)

/* -------------------------------------------------------------------------- */
/* A task creation failure occurred on the DSP.                               */
/* -------------------------------------------------------------------------- */
#define DSP_ETASK                   (DSP_EBASE + 0x16)

/* -------------------------------------------------------------------------- */
/* A timeout occurred before the requested operation could complete.          */
/* -------------------------------------------------------------------------- */
#define DSP_ETIMEOUT                (DSP_EBASE + 0x17)

/* -------------------------------------------------------------------------- */
/* A data truncation occurred, e.g., when requesting a descriptive error      */
/* string, not enough space was allocated for the complete error message.     */
/* -------------------------------------------------------------------------- */
#define DSP_ETRUNCATED              (DSP_EBASE + 0x18)

/* -------------------------------------------------------------------------- */
/* A parameter is invalid.                                                    */
/* -------------------------------------------------------------------------- */
#define DSP_EVALUE                  (DSP_EBASE + 0x1a)

/* -------------------------------------------------------------------------- */
/* The state of the specified object is incorrect for the requested           */
/* operation.                                                                 */
/* -------------------------------------------------------------------------- */
#define DSP_EWRONGSTATE             (DSP_EBASE + 0x1b)

/* -------------------------------------------------------------------------- */
/* Symbol not found in the COFF file.  DSPNode_Create will return this if     */
/* the iAlg function table for an xDAIS socket is not found in the COFF file. */
/* In this case, force the symbol to be linked into the COFF file.            */
/* DSPNode_Create, DSPNode_Execute, and DSPNode_Delete will return this if    */
/* the create, execute, or delete phase function, respectively, could not be  */
/* found in the COFF file.                                                    */
/* -------------------------------------------------------------------------- */
#define DSP_ESYMBOL                 (DSP_EBASE + 0x1c)

/* -------------------------------------------------------------------------- */
/* UUID not found in registry.                                                */
/* -------------------------------------------------------------------------- */
#define DSP_EUUID                   (DSP_EBASE + 0x1d)

/* -------------------------------------------------------------------------- */
/* Unable to read content of DCD data section ; this is typically caused by   */
/* improperly configured nodes.                                               */
/* -------------------------------------------------------------------------- */
#define DSP_EDCDREADSECT            (DSP_EBASE + 0x1e)

/* -------------------------------------------------------------------------- */
/* Unable to decode DCD data section content ; this is typically caused by    */
/* changes to DSP/BIOS Bridge data structures.                                */
/* -------------------------------------------------------------------------- */
#define DSP_EDCDPARSESECT           (DSP_EBASE + 0x1f)

/* -------------------------------------------------------------------------- */
/* Unable to get pointer to DCD data section ; this is typically caused by    */
/* improperly configured UUIDs.                                               */
/* -------------------------------------------------------------------------- */
#define DSP_EDCDGETSECT             (DSP_EBASE + 0x20)

/* -------------------------------------------------------------------------- */
/* Unable to load file containing DCD data section ; this is typically        */
/* caused by a missing COFF file.                                             */
/* -------------------------------------------------------------------------- */
#define DSP_EDCDLOADBASE            (DSP_EBASE + 0x21)

/* -------------------------------------------------------------------------- */
/* The specified COFF file does not contain a valid node registration         */
/* section.                                                                   */
/* -------------------------------------------------------------------------- */
#define DSP_EDCDNOAUTOREGISTER      (DSP_EBASE + 0x22)

/* -------------------------------------------------------------------------- */
/* A requested resource is not available.                                     */
/* -------------------------------------------------------------------------- */
#define DSP_ERESOURCE               (DSP_EBASE + 0x28)

/* -------------------------------------------------------------------------- */
/* A critical error has occurred, and the DSP is being re-started.            */
/* -------------------------------------------------------------------------- */
#define DSP_ERESTART                (DSP_EBASE + 0x29)

/* -------------------------------------------------------------------------- */
/* A DSP memory free operation failed.                                        */
/* -------------------------------------------------------------------------- */
#define DSP_EFREE                   (DSP_EBASE + 0x2a)

/* -------------------------------------------------------------------------- */
/* A DSP I/O free operation failed.                                           */
/* -------------------------------------------------------------------------- */
#define DSP_EIOFREE                 (DSP_EBASE + 0x2b)

/* -------------------------------------------------------------------------- */
/* Multiple instances are not allowed.                                        */
/* -------------------------------------------------------------------------- */
#define DSP_EMULINST                (DSP_EBASE + 0x2c)

/* -------------------------------------------------------------------------- */
/* A specified entity was not found.                                          */
/* -------------------------------------------------------------------------- */
#define DSP_ENOTFOUND               (DSP_EBASE + 0x2d)

/* -------------------------------------------------------------------------- */
/* A DSP I/O resource is not available.                                       */
/* -------------------------------------------------------------------------- */
#define DSP_EOUTOFIO                (DSP_EBASE + 0x2e)

/* -------------------------------------------------------------------------- */
/* A shared memory buffer contained in a message or stream could not be       */
/* mapped to the GPP client process's virtual space.                          */
/* -------------------------------------------------------------------------- */
#define DSP_ETRANSLATE              (DSP_EBASE + 0x2f)

/* -------------------------------------------------------------------------- */
/* File or section load write function failed to write to DSP                 */
/* -------------------------------------------------------------------------- */
#define DSP_EFWRITE                 (DSP_EBASE + 0x31)

/* -------------------------------------------------------------------------- */
/* Unable to find a named section in DSP executable                           */
/* -------------------------------------------------------------------------- */
#define DSP_ENOSECT                 (DSP_EBASE + 0x32)

/* -------------------------------------------------------------------------- */
/* Unable to open file                                                        */
/* -------------------------------------------------------------------------- */
#define DSP_EFOPEN                  (DSP_EBASE + 0x33)

/* -------------------------------------------------------------------------- */
/* Unable to read file                                                        */
/* -------------------------------------------------------------------------- */
#define DSP_EFREAD                  (DSP_EBASE + 0x34)

/* -------------------------------------------------------------------------- */
/* A non-existent memory segment identifier was specified                     */
/* -------------------------------------------------------------------------- */
#define DSP_EOVERLAYMEMORY          (DSP_EBASE + 0x37)

/* -------------------------------------------------------------------------- */
/* Invalid segment ID                                                         */
/* -------------------------------------------------------------------------- */
#define DSP_EBADSEGID               (DSP_EBASE + 0x38)

/* -------------------------------------------------------------------------- */
/* Invalid alignment                                                          */
/* -------------------------------------------------------------------------- */
#define DSP_EALIGNMENT               (DSP_EBASE + 0x39)

/* -------------------------------------------------------------------------- */
/* Invalid stream mode                                                        */
/* -------------------------------------------------------------------------- */
#define DSP_ESTRMMODE               (DSP_EBASE + 0x3a)

/* -------------------------------------------------------------------------- */
/* Nodes not connected                                                        */
/* -------------------------------------------------------------------------- */
#define DSP_ENOTCONNECTED           (DSP_EBASE + 0x3b)

/* -------------------------------------------------------------------------- */
/* Not shared memory                                                          */
/* -------------------------------------------------------------------------- */
#define DSP_ENOTSHAREDMEM           (DSP_EBASE + 0x3c)

/* -------------------------------------------------------------------------- */
/* Error occurred in a dynamic loader library function                        */
/* -------------------------------------------------------------------------- */
#define DSP_EDYNLOAD                (DSP_EBASE + 0x3d)

/* -------------------------------------------------------------------------- */
/* Device in 'sleep/suspend' mode due to DPM                                  */
/* -------------------------------------------------------------------------- */
#define DSP_EDPMSUSPEND             (DSP_EBASE + 0x3e)

/* -------------------------------------------------------------------------- */
/* A node-specific error has occurred.                                        */
/* -------------------------------------------------------------------------- */
#define DSP_EUSER1                  (DSP_EBASE + 0x40)
#define DSP_EUSER2                  (DSP_EBASE + 0x41)
#define DSP_EUSER3                  (DSP_EBASE + 0x42)
#define DSP_EUSER4                  (DSP_EBASE + 0x43)
#define DSP_EUSER5                  (DSP_EBASE + 0x44)
#define DSP_EUSER6                  (DSP_EBASE + 0x45)
#define DSP_EUSER7                  (DSP_EBASE + 0x46)
#define DSP_EUSER8                  (DSP_EBASE + 0x47)
#define DSP_EUSER9                  (DSP_EBASE + 0x48)
#define DSP_EUSER10                 (DSP_EBASE + 0x49)
#define DSP_EUSER11                 (DSP_EBASE + 0x4a)
#define DSP_EUSER12                 (DSP_EBASE + 0x4b)
#define DSP_EUSER13                 (DSP_EBASE + 0x4c)
#define DSP_EUSER14                 (DSP_EBASE + 0x4d)
#define DSP_EUSER15                 (DSP_EBASE + 0x4e)
#define DSP_EUSER16                 (DSP_EBASE + 0x4f)

/* ========================================================================== */
/* FAILURE Codes : DEV                                                        */
/* ========================================================================== */
#define DEV_EBASE                   (DSP_COMP_EBASE + 0x000)

/* -------------------------------------------------------------------------- */
/* The mini-driver expected a newer version of the class driver.              */
/* -------------------------------------------------------------------------- */
#define DEV_E_NEWWMD                (DEV_EBASE + 0x00)

/* -------------------------------------------------------------------------- */
/* WMD_DRV_Entry function returned a NULL function interface table.           */
/* -------------------------------------------------------------------------- */
#define DEV_E_NULLWMDINTF           (DEV_EBASE + 0x01)

/* ========================================================================== */
/* FAILURE Codes : LDR                                                        */
/* ========================================================================== */
#define LDR_EBASE                   (DSP_COMP_EBASE + 0x100)

/* -------------------------------------------------------------------------- */
/* Insufficient memory to export class driver services.                       */
/* -------------------------------------------------------------------------- */
#define LDR_E_NOMEMORY              (LDR_EBASE + 0x00)

/* -------------------------------------------------------------------------- */
/* Unable to find WMD file in system directory.                               */
/* -------------------------------------------------------------------------- */
#define LDR_E_FILEUNABLETOOPEN      (LDR_EBASE + 0x01)

/* ========================================================================== */
/* FAILURE Codes : CFG                                                        */
/* ========================================================================== */
#define CFG_EBASE                   (DSP_COMP_EBASE + 0x200)

/* -------------------------------------------------------------------------- */
/* Invalid pointer passed into a configuration module function                */
/* -------------------------------------------------------------------------- */
#define CFG_E_INVALIDPOINTER        (CFG_EBASE + 0x00)

/* -------------------------------------------------------------------------- */
/* Invalid device node handle passed into a configuration module function.    */
/* -------------------------------------------------------------------------- */
#define CFG_E_INVALIDHDEVNODE       (CFG_EBASE + 0x01)

/* -------------------------------------------------------------------------- */
/* Unable to retrieve resource information from the registry.                 */
/* -------------------------------------------------------------------------- */
#define CFG_E_RESOURCENOTAVAIL      (CFG_EBASE + 0x02)

/* -------------------------------------------------------------------------- */
/* Unable to find board name key in registry.                                 */
/* -------------------------------------------------------------------------- */
#define CFG_E_INVALIDBOARDNAME      (CFG_EBASE + 0x03)

/* -------------------------------------------------------------------------- */
/* Unable to find a device node in registry with given unit number.           */
/* -------------------------------------------------------------------------- */
#define CFG_E_INVALIDUNITNUM        (CFG_EBASE + 0x04)

/* -------------------------------------------------------------------------- */
/* Insufficient buffer size                                                   */
/* -------------------------------------------------------------------------- */
#define CFG_E_INSUFFICIENTBUFSIZE   (CFG_EBASE + 0x05)

/* ========================================================================== */
/* FAILURE Codes : BRD                                                        */
/* ========================================================================== */
#define BRD_EBASE                   (DSP_COMP_EBASE + 0x300)

/* -------------------------------------------------------------------------- */
/* Board client does not have sufficient access rights for this operation.    */
/* -------------------------------------------------------------------------- */
#define BRD_E_ACCESSDENIED          (BRD_EBASE + 0x00)

/* -------------------------------------------------------------------------- */
/* Unable to find trace buffer symbols in the DSP executable COFF file.       */
/* -------------------------------------------------------------------------- */
#define BRD_E_NOTRACEBUFFER         (BRD_EBASE + 0x01)

/* -------------------------------------------------------------------------- */
/* Attempted to auto-start board, but no default DSP executable configured.   */
/* -------------------------------------------------------------------------- */
#define BRD_E_NOEXEC                (BRD_EBASE + 0x02)

/* -------------------------------------------------------------------------- */
/* The operation failed because it was started from a wrong state             */
/* -------------------------------------------------------------------------- */
#define BRD_E_WRONGSTATE            (BRD_EBASE + 0x03)

/* ========================================================================== */
/* FAILURE Codes : COD                                                        */
/* ========================================================================== */
#define COD_EBASE                   (DSP_COMP_EBASE + 0x400)

/* -------------------------------------------------------------------------- */
/* No symbol table is loaded for this board.                                  */
/* -------------------------------------------------------------------------- */
#define COD_E_NOSYMBOLSLOADED       (COD_EBASE + 0x00)

/* -------------------------------------------------------------------------- */
/* Symbol not found in for this board.                                        */
/* -------------------------------------------------------------------------- */
#define COD_E_SYMBOLNOTFOUND        (COD_EBASE + 0x01)

/* -------------------------------------------------------------------------- */
/* ZL DLL module is not exporting the correct function interface.             */
/* -------------------------------------------------------------------------- */
#define COD_E_NOZLFUNCTIONS         (COD_EBASE + 0x02)

/* -------------------------------------------------------------------------- */
/* Unable to initialize the ZL COFF parsing module.                           */
/* -------------------------------------------------------------------------- */
#define COD_E_ZLCREATEFAILED        (COD_EBASE + 0x03)

/* -------------------------------------------------------------------------- */
/* Unable to open DSP executable COFF file.                                   */
/* -------------------------------------------------------------------------- */
#define COD_E_OPENFAILED            (COD_EBASE + 0x04)

/* -------------------------------------------------------------------------- */
/* Unable to parse DSP executable COFF file.                                  */
/* -------------------------------------------------------------------------- */
#define COD_E_LOADFAILED            (COD_EBASE + 0x05)

/* -------------------------------------------------------------------------- */
/* Unable to read DSP executable COFF file.                                   */
/* -------------------------------------------------------------------------- */
#define COD_E_READFAILED            (COD_EBASE + 0x06)

/* ========================================================================== */
/* FAILURE Codes : CHNL                                                       */
/* ========================================================================== */
#define CHNL_EBASE                  (DSP_COMP_EBASE + 0x500)

/* -------------------------------------------------------------------------- */
/* Attempt to created channel manager with too many channels.                 */
/* -------------------------------------------------------------------------- */
#define CHNL_E_MAXCHANNELS          (CHNL_EBASE + 0x00)

/* -------------------------------------------------------------------------- */
/* No channel manager exists for this mini-driver.                            */
/* -------------------------------------------------------------------------- */
#define CHNL_E_NOMGR                (CHNL_EBASE + 0x01)

/* -------------------------------------------------------------------------- */
/* No free channels are available.                                            */
/* -------------------------------------------------------------------------- */
#define CHNL_E_OUTOFSTREAMS         (CHNL_EBASE + 0x02)

/* -------------------------------------------------------------------------- */
/* Channel ID is out of range.                                                */
/* -------------------------------------------------------------------------- */
#define CHNL_E_BADCHANID            (CHNL_EBASE + 0x03)

/* -------------------------------------------------------------------------- */
/* Channel is already in use.                                                 */
/* -------------------------------------------------------------------------- */
#define CHNL_E_CHANBUSY             (CHNL_EBASE + 0x04)

/* -------------------------------------------------------------------------- */
/* Invalid channel mode argument.                                             */
/* -------------------------------------------------------------------------- */
#define CHNL_E_BADMODE              (CHNL_EBASE + 0x05)

/* -------------------------------------------------------------------------- */
/* dwTimeOut parameter was CHNL_IOCNOWAIT, yet no I/O completions were        */
/* queued.                                                                    */
/* -------------------------------------------------------------------------- */
#define CHNL_E_NOIOC                (CHNL_EBASE + 0x06)

/* -------------------------------------------------------------------------- */
/* I/O has been cancelled on this channel.                                    */
/* -------------------------------------------------------------------------- */
#define CHNL_E_CANCELLED            (CHNL_EBASE + 0x07)

/* -------------------------------------------------------------------------- */
/* End of stream was already requested on this output channel.                */
/* -------------------------------------------------------------------------- */
#define CHNL_E_EOS                  (CHNL_EBASE + 0x09)

/* -------------------------------------------------------------------------- */
/* Unable to create the channel event object.                                 */
/* -------------------------------------------------------------------------- */
#define CHNL_E_CREATEEVENT          (CHNL_EBASE + 0x0A)

/* -------------------------------------------------------------------------- */
/* Board name and unit number do not identify a valid board name.             */
/* -------------------------------------------------------------------------- */
#define CHNL_E_BRDID                (CHNL_EBASE + 0x0B)

/* -------------------------------------------------------------------------- */
/* Invalid IRQ configured for this WMD for this system.                       */
/* -------------------------------------------------------------------------- */
#define CHNL_E_INVALIDIRQ           (CHNL_EBASE + 0x0C)

/* -------------------------------------------------------------------------- */
/* DSP word size of zero configured for this device.                          */
/* -------------------------------------------------------------------------- */
#define CHNL_E_INVALIDWORDSIZE      (CHNL_EBASE + 0x0D)

/* -------------------------------------------------------------------------- */
/* A zero length memory base was specified for a shared memory class driver.  */
/* -------------------------------------------------------------------------- */
#define CHNL_E_INVALIDMEMBASE       (CHNL_EBASE + 0x0E)

/* -------------------------------------------------------------------------- */
/* Memory map is not configured, or unable to map physical to linear          */
/* address.                                                                   */
/* -------------------------------------------------------------------------- */
#define CHNL_E_NOMEMMAP             (CHNL_EBASE + 0x0F)

/* -------------------------------------------------------------------------- */
/* Attempted to create a channel manager  when one already exists.            */
/* -------------------------------------------------------------------------- */
#define CHNL_E_MGREXISTS            (CHNL_EBASE + 0x10)

/* -------------------------------------------------------------------------- */
/* Unable to plug channel ISR for configured IRQ.                             */
/* -------------------------------------------------------------------------- */
#define CHNL_E_ISR                  (CHNL_EBASE + 0x11)

/* -------------------------------------------------------------------------- */
/* No free I/O request packets are available.                                 */
/* -------------------------------------------------------------------------- */
#define CHNL_E_NOIORPS              (CHNL_EBASE + 0x12)

/* -------------------------------------------------------------------------- */
/* Buffer size is larger than the size of physical channel.                   */
/* -------------------------------------------------------------------------- */
#define CHNL_E_BUFSIZE              (CHNL_EBASE + 0x13)

/* -------------------------------------------------------------------------- */
/* User cannot mark end of stream on an input channel.                        */
/* -------------------------------------------------------------------------- */
#define CHNL_E_NOEOS                (CHNL_EBASE + 0x14)

/* -------------------------------------------------------------------------- */
/* Wait for flush operation on an output channel timed out.                   */
/* -------------------------------------------------------------------------- */
#define CHNL_E_WAITTIMEOUT          (CHNL_EBASE + 0x15)

/* -------------------------------------------------------------------------- */
/* User supplied hEvent must be specified with pstrEventName attribute        */
/* -------------------------------------------------------------------------- */
#define CHNL_E_BADUSEREVENT         (CHNL_EBASE + 0x16)

/* -------------------------------------------------------------------------- */
/* Illegal user event name specified                                          */
/* -------------------------------------------------------------------------- */
#define CHNL_E_USEREVENTNAME        (CHNL_EBASE + 0x17)

/* -------------------------------------------------------------------------- */
/* Unable to prepare buffer specified                                         */
/* -------------------------------------------------------------------------- */
#define CHNL_E_PREPFAILED           (CHNL_EBASE + 0x18)

/* -------------------------------------------------------------------------- */
/* Unable to Unprepare buffer specified                                       */
/* -------------------------------------------------------------------------- */
#define CHNL_E_UNPREPFAILED         (CHNL_EBASE + 0x19)

/* ========================================================================== */
/* FAILURE Codes : SYNC                                                       */
/* ========================================================================== */
#define SYNC_EBASE                  (DSP_COMP_EBASE + 0x600)

/* -------------------------------------------------------------------------- */
/* Wait on a kernel event failed.                                             */
/* -------------------------------------------------------------------------- */
#define SYNC_E_FAIL                 (SYNC_EBASE + 0x00)

/* -------------------------------------------------------------------------- */
/* Timeout expired while waiting for event to be signalled.                   */
/* -------------------------------------------------------------------------- */
#define SYNC_E_TIMEOUT              (SYNC_EBASE + 0x01)

/* ========================================================================== */
/* FAILURE Codes : WMD                                                        */
/* ========================================================================== */
#define WMD_EBASE                   (DSP_COMP_EBASE + 0x700)

/* -------------------------------------------------------------------------- */
/* A test of hardware assumptions or integrity failed.                        */
/* -------------------------------------------------------------------------- */
#define WMD_E_HARDWARE              (WMD_EBASE + 0x00)

/* -------------------------------------------------------------------------- */
/* One or more configuration parameters violated WMD hardware assumptions.    */
/* -------------------------------------------------------------------------- */
#define WMD_E_BADCONFIG             (WMD_EBASE + 0x01)

/* -------------------------------------------------------------------------- */
/* Timeout occurred waiting for a response from the hardware.                 */
/* -------------------------------------------------------------------------- */
#define WMD_E_TIMEOUT               (WMD_EBASE + 0x02)

/* ========================================================================== */
/* FAILURE Codes : REG                                                        */
/* ========================================================================== */
#define REG_EBASE                   (DSP_COMP_EBASE + 0x800)

/* -------------------------------------------------------------------------- */
/* Invalid subkey parameter.                                                  */
/* -------------------------------------------------------------------------- */
#define REG_E_INVALIDSUBKEY         (REG_EBASE + 0x00)

/* -------------------------------------------------------------------------- */
/* Invalid entry parameter.                                                   */
/* -------------------------------------------------------------------------- */
#define REG_E_INVALIDENTRY          (REG_EBASE + 0x01)

/* -------------------------------------------------------------------------- */
/* No more registry values.                                                   */
/* -------------------------------------------------------------------------- */
#define REG_E_NOMOREITEMS           (REG_EBASE + 0x02)

/* -------------------------------------------------------------------------- */
/* Insufficient space to hold data in registry value.                         */
/* -------------------------------------------------------------------------- */
#define REG_E_MOREDATA              (REG_EBASE + 0x03)

/* ========================================================================== */
/* FAILURE Codes : KFILE                                                      */
/* ========================================================================== */
#define KFILE_EBASE                 (DSP_COMP_EBASE + 0x900)

/* -------------------------------------------------------------------------- */
/* Invalid file handle.                                                       */
/* -------------------------------------------------------------------------- */
#define E_KFILE_INVALIDHANDLE       (KFILE_EBASE + 0x01)

/* -------------------------------------------------------------------------- */
/* Bad file name.                                                             */
/* -------------------------------------------------------------------------- */
#define E_KFILE_BADFILENAME         (KFILE_EBASE + 0x02)

/* -------------------------------------------------------------------------- */
/* Invalid file mode.                                                         */
/* -------------------------------------------------------------------------- */
#define E_KFILE_INVALIDMODE         (KFILE_EBASE + 0x03)

/* -------------------------------------------------------------------------- */
/* No resources available.                                                    */
/* -------------------------------------------------------------------------- */
#define E_KFILE_NORESOURCES         (KFILE_EBASE + 0x04)

/* -------------------------------------------------------------------------- */
/* Invalid file buffer        .                                               */
/* -------------------------------------------------------------------------- */
#define E_KFILE_INVALIDBUFFER       (KFILE_EBASE + 0x05)

/* -------------------------------------------------------------------------- */
/* Bad origin argument.                                                       */
/* -------------------------------------------------------------------------- */
#define E_KFILE_BADORIGINFLAG       (KFILE_EBASE + 0x06)

/* -------------------------------------------------------------------------- */
/* Invalid file offset value.                                                 */
/* -------------------------------------------------------------------------- */
#define E_KFILE_INVALIDOFFSET       (KFILE_EBASE + 0x07)

/* -------------------------------------------------------------------------- */
/* General KFILE error condition                                              */
/* -------------------------------------------------------------------------- */
#define E_KFILE_ERROR               (KFILE_EBASE + 0x08)

#ifdef __cplusplus
}
#endif
#endif				/* ERRBASE_ */
