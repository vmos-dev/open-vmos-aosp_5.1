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
 */


/*!
******************************************************************************
 @file   : dma_api.h

 @brief

 @date   02/11/2005

 \n<b>Description:</b>\n
         This file contains header file for the MTX DMAC API.

         The MTX DMAC API can operate synchronously or asynchronously.

                 In synchronous case, the API uses an internal callback function
                 to detect state transitions and the SEMA API to block whilst
                 waiting for the transfer to complete.

                 In the asynchronous case, the caller is responsible for
                 detecting and handling the state transitions and synchronising
                 with other processes/processing.

 \n<b>Platform:</b>\n
         MSVDX/MTX

******************************************************************************/
/*
******************************************************************************
 Modifications :-

 $Log: dma_api.h $

  --- Revision Logs Removed ---

  --- Revision Logs Removed ---

  --- Revision Logs Removed ---

  --- Revision Logs Removed ---

  --- Revision Logs Removed ---

  --- Revision Logs Removed ---

  --- Revision Logs Removed ---

  --- Revision Logs Removed ---

  --- Revision Logs Removed ---

  --- Revision Logs Removed ---

  --- Revision Logs Removed ---

  --- Revision Logs Removed ---

  --- Revision Logs Removed ---

  --- Revision Logs Removed ---

  --- Revision Logs Removed ---

  --- Revision Logs Removed ---

  --- Revision Logs Removed ---

  --- Revision Logs Removed ---

  --- Revision Logs Removed ---

  --- Revision Logs Removed ---

  --- Revision Logs Removed ---

  --- Revision Logs Removed ---

  --- Revision Logs Removed ---


*****************************************************************************/


#if !defined (__DMA_API_H__)
#define __DMA_API_H__

#include <img_types.h>
#include "msvdx_dmac_regs_io2.h"
#include "msvdx_dmac_linked_list.h"

#if (__cplusplus)
extern "C" {
#endif

    /*!
    ******************************************************************************
     This type defines the DMAC status
    ******************************************************************************/
    typedef enum {
        DMA_STATUS_IDLE,                        //!< The DMAC is idle.
        DMA_STATUS_BUSY,                        //!< The DMAC is busy - a DMA is in progress.
        DMA_STATUS_COMPLETE,            /*!< The DMAC operation has completed - return by
                                                                         DMA_GetStatus()once before the DMAC returns
                                                                         to #DMA_STATUS_IDLE.                                                   */
        DMA_STATUS_TIMEOUT,                 /*!< The DMAC operation has timed-out - return by
                                                                         DMA_GetStatus()once before the DMAC returns
                                                                         to #DMA_STATUS_IDLE.                                                   */

    }
    DMA_eStatus;

    /*!
    ******************************************************************************
     This type defines the DMA channel Ids
    ******************************************************************************/
    typedef enum {
        DMA_CHANNEL_MTX = 0x0,          //!< DMA channel for MTX
        DMA_CHANNEL_RESERVED,           //!< DMA channel 1 is reserved for VEC use
        DMA_CHANNEL_SR1,                        //!< DMA channel for 1st shift register
        DMA_CHANNEL_SR2,                        //!< DMA channel for 2nd shift register
        DMA_CHANNEL_SR3,                        //!< DMA channel for 3rd shift register
        DMA_CHANNEL_SR4,                        //!< DMA channel for 4th shift register

    } DMA_eChannelId;

    /*!
    ******************************************************************************
     Used with DMA_SyncAction() and DMA_AsyncAction() to indicate whether
     the peripheral is the mtx or not.
    ******************************************************************************/
    enum {
        DMA_PERIPH_IS_NOT_MTX   = 0,   //!< The peripheral is not the mtx.
        DMA_PERIPH_IS_MTX               = 1,   //!< The peripheral is the mtx.
    };

    /*!
    ******************************************************************************
     This type defines the byte swap settings.
    ******************************************************************************/
    typedef enum {
        DMA_BSWAP_NO_SWAP = 0x0,   //!< No byte swapping will be performed.
        DMA_BSWAP_REVERSE = 0x1,   //!< Byte order will be reversed.

    } DMA_eBSwap;

    /*!
    ******************************************************************************
     This type defines the peripheral width settings
    ******************************************************************************/
    typedef enum {
        DMA_PWIDTH_32_BIT = 0x0,       //!< Peripheral width 32-bit.
        DMA_PWIDTH_16_BIT = 0x1,       //!< Peripheral width 16-bit.
        DMA_PWIDTH_8_BIT  = 0x2,       //!< Peripheral width 8-bit.

    } DMA_ePW;

    /*!
    ******************************************************************************
     This type defines the direction of the DMA transfer
    ******************************************************************************/
    typedef enum {
        DMA_DIR_MEM_TO_PERIPH = 0x0, //!< Data from memory to peripheral.
        DMA_DIR_PERIPH_TO_MEM = 0x1, //!< Data from peripheral to memory.

    } DMA_eDir;

    /*!
    ******************************************************************************
     This type defines whether the peripheral address is to be incremented
    ******************************************************************************/
    typedef enum {
        DMA_PERIPH_INCR_ON      = 0x1,          //!< Peripheral address will be incremented
        DMA_PERIPH_INCR_OFF     = 0x0,          //!< Peripheral address will not be incremented

    } DMA_ePeriphIncr;

    /*!
    ******************************************************************************
     This type defines how much the peripheral address is incremented by
    ******************************************************************************/
    typedef enum {
        DMA_PERIPH_INCR_1       = 0x2,          //!< Increment peripheral address by 1
        DMA_PERIPH_INCR_2       = 0x1,          //!< Increment peripheral address by 2
        DMA_PERIPH_INCR_4       = 0x0,          //!< Increment peripheral address by 4

    } DMA_ePeriphIncrSize;

    /*!
    ******************************************************************************
     This type defines whether the 2d mode is enabled or disabled
    ******************************************************************************/
    typedef enum {
        DMA_MODE_2D_ON  = 0x1,          //!< the 2d mode will be used
        DMA_MODE_2D_OFF = 0x0,          //!< the 2d mode will not be used

    } DMA_eMode2D;

    /*!
    ******************************************************************************

     @Function              DMA_LL_SET_WD0

     @Description

     Set word 0 in a dmac linked list entry.

     @Input    pList            : pointer to start of linked list entry

     @Input    BSWAP        : big/little endian byte swap (see DMA_eBSwap).

     @Input    DIR                      : transfer direction (see DMA_eDir).

     @Input    PW                       : peripheral width (see DMA_ePW).

     @Return   nothing

    ******************************************************************************/
#define DMA_LL_SET_WD0(pList, BSWAP, DIR, PW)                   \
        do{                                                                                                     \
                MEMIO_WRITE_FIELD(pList, DMAC_LL_BSWAP, BSWAP); \
                MEMIO_WRITE_FIELD(pList, DMAC_LL_DIR,   DIR);   \
                MEMIO_WRITE_FIELD(pList, DMAC_LL_PW,    PW);    \
        }while(0)


    /*!
    ******************************************************************************

     @Function              DMA_LL_SET_WD1

     @Description

     Set word 1 in a dmac linked list entry.

     @Input    pList            : pointer to start of linked list entry

     @Input    INCR                     : whether to increment the peripeheral address (see DMA_ePeriphIncr)

     @Input    PI                       : how much to increment the peripheral address by (see DMA_ePeriphIncrSize)

     @Input    LEN                      : length of transfer in peripheral width units

     @Return   nothing

    ******************************************************************************/
#define DMA_LL_SET_WD1(pList, INCR, PI, LEN)                    \
        do      {                                                                                                       \
                MEMIO_WRITE_FIELD(pList, DMAC_LL_PI,    PI);            \
                MEMIO_WRITE_FIELD(pList, DMAC_LL_INCR,  INCR);          \
                MEMIO_WRITE_FIELD(pList, DMAC_LL_LEN,   LEN);           \
        }while(0)

    /*!
    ******************************************************************************

     @Function              DMA_LL_SET_WD2

     @Description

     Set word 2 in a dmac linked list entry.

     @Input    pList            : pointer to start of linked list entry

     @Input    PERI_ADDR        : the perihperal address to transfer to/from

     @Return   nothing

    ******************************************************************************/
#define DMA_LL_SET_WD2(pList, PERI_ADDR)                                        \
        do {                                                                                                    \
                MEMIO_WRITE_FIELD(pList, DMAC_LL_ADDR, PERI_ADDR);      \
        }while(0)

    /*!
    ******************************************************************************

     @Function              DMA_LL_SET_WD3

     @Description

     Set word 3 in a dmac linked list entry.

     @Input    pList            : pointer to start of linked list entry

     @Input    ACC_DEL          : access delay (see DMA_eAccDel)

     @Input    BURST            : burst size (see DMA_eBurst)

     @Return   nothing

    ******************************************************************************/
#define DMA_LL_SET_WD3(pList, ACC_DEL, BURST , EXTSA )                  \
        do {                                                                                                            \
                MEMIO_WRITE_FIELD(pList, DMAC_LL_ACC_DEL,       ACC_DEL);       \
                MEMIO_WRITE_FIELD(pList, DMAC_LL_BURST,         BURST);         \
                MEMIO_WRITE_FIELD(pList, DMAC_LL_EXT_SA,        EXTSA);         \
        }while(0)

    /*!
    ******************************************************************************

     @Function              DMA_LL_SET_WD4

     @Description

     Set word 4 in a dmac linked list entry.

     @Input    pList            : pointer to start of linked list entry

     @Input    MODE_2D          : enable/disable 2d mode (see DMA_eMode2D)

     @Input    REP_COUNT        : repeat count (the number of rows transferred)

     @Return   nothing

    ******************************************************************************/
#define DMA_LL_SET_WD4(pList, MODE_2D, REP_COUNT)                       \
        do {                                                                                                    \
        MEMIO_WRITE_FIELD(pList, DMAC_LL_MODE_2D,       MODE_2D);       \
        MEMIO_WRITE_FIELD(pList, DMAC_LL_REP_COUNT,     REP_COUNT); \
        } while(0)

    /*!
    ******************************************************************************

     @Function              DMA_LL_SET_WD5

     @Description

     Set word 5 in a dmac linked list entry.

     @Input    pList            : pointer to start of linked list entry

     @Input    LINE_ADD_OFF     : number of bytes from the end of one row to the start of the next row
                                                (only applicable when using 2D transfer mode)

     @Input    ROW_LENGTH       : number of bytes per row
                                                (only applicable when using 2D transfer mode)

     @Return   nothing

    ******************************************************************************/
#define DMA_LL_SET_WD5(pList, LINE_ADD_OFF, ROW_LENGTH)                                 \
        do{                                                                                                                                     \
        MEMIO_WRITE_FIELD(pList, DMAC_LL_LINE_ADD_OFF,  LINE_ADD_OFF);          \
        MEMIO_WRITE_FIELD(pList, DMAC_LL_ROW_LENGTH,    ROW_LENGTH);            \
        }while(0)

    /*!
    ******************************************************************************

     @Function              DMA_LL_SET_WD6

     @Description

     Set word 6 in a dmac linked list entry.

     @Input    pList            : pointer to start of linked list entry

     @Input    SA                       : the host memory address to transfer to/from

     @Return   nothing

    ******************************************************************************/
#define DMA_LL_SET_WD6(pList, SA)                                               \
        do{                                                                                                     \
                MEMIO_WRITE_FIELD(pList, DMAC_LL_SA,    SA);    \
        }while(0)

    /*!
    ******************************************************************************

    @Function              DMA_LL_SET_WD7

    @Description

    Set word 7 in a dmac linked list entry.

    @Input    pList:            pointer to start of linked list entry

    @Input    LISTPTR:          pointer to next linked list entry

    If the linked list entry is in MTX memory (eListLocation == DMA_LIST_IS_IN_MTX_MEM) then
    LISTPTR is a pointer to the start of the next linked list entry.  If the linked list entry
    is in HOST memory (eListLocation == DMA_LIST_IS_IN_SYS_MEM) then LISTPTR is a pointer to the
    start of the next linked list entry, but right shifted by 4 bits (i.e. ptr >> 4).  If this
    is the last entry in the linked list sequence then LISTPTR must be set to NULL.

    @Return   nothing

    ******************************************************************************/
#define DMA_LL_SET_WD7(pList, LISTPTR)                                                                  \
        do {                                                                                                                            \
                MEMIO_WRITE_FIELD(pList, DMAC_LL_LISTPTR,       LISTPTR);                       \
                MEMIO_WRITE_FIELD(pList, DMAC_LL_LIST_FIN,      (LISTPTR) ? 0 : 1);     \
        }while(0)


    /*!
    ******************************************************************************

     @Function              DMA_VALUE_COUNT

     @Description

     This MACRO is used to aid the generation of the ui32Count member of the DMA_sParams
     structure required by DMA_SyncAction() and DMA_AsyncAction().  If this is not suitable
     for a given application then the programmer is free to fill in the fields in any way they
     see fit.

     @Input    BSWAP        : Big/little endian byte swap (see DMA_eBSwap).

     @Input    PW           : The width of the peripheral DMA register (see DMA_ePW).

     @Input    DIR          : The direction of the transfer (see DMA_eDir).

     @Input    PERIPH_INCR      : How much to increment the peripheral address by (see DMA_ePeriphIncr).

     @Input    COUNT            : The length of the transfer in transfer units.

     @Return   img_uint32   : The value of the generated word.

    ******************************************************************************/
#define DMA_VALUE_COUNT(BSWAP,PW,DIR,PERIPH_INCR,COUNT)                                                 \
                                                                                                                                                                    \
    (((BSWAP)           & DMAC_DMAC_COUNT_BSWAP_LSBMASK)        << DMAC_DMAC_COUNT_BSWAP_SHIFT) |   \
        (((PW)                  & DMAC_DMAC_COUNT_PW_LSBMASK)           << DMAC_DMAC_COUNT_PW_SHIFT)    |   \
        (((DIR)                 & DMAC_DMAC_COUNT_DIR_LSBMASK)          << DMAC_DMAC_COUNT_DIR_SHIFT)   |   \
        (((PERIPH_INCR) & DMAC_DMAC_COUNT_PI_LSBMASK)           << DMAC_DMAC_COUNT_PI_SHIFT)    |   \
        (((COUNT)               & DMAC_DMAC_COUNT_CNT_LSBMASK)          << DMAC_DMAC_COUNT_CNT_SHIFT)

    /*!
    ******************************************************************************
     This type defines the access delay settings.
    ******************************************************************************/
    typedef enum {
        DMA_ACC_DEL_0       = 0x0,              //!< Access delay zero clock cycles
        DMA_ACC_DEL_256     = 0x1,      //!< Access delay 256 clock cycles
        DMA_ACC_DEL_512     = 0x2,      //!< Access delay 512 clock cycles
        DMA_ACC_DEL_768     = 0x3,      //!< Access delay 768 clock cycles
        DMA_ACC_DEL_1024    = 0x4,      //!< Access delay 1024 clock cycles
        DMA_ACC_DEL_1280    = 0x5,      //!< Access delay 1280 clock cycles
        DMA_ACC_DEL_1536    = 0x6,      //!< Access delay 1536 clock cycles
        DMA_ACC_DEL_1792    = 0x7,      //!< Access delay 1792 clock cycles

    } DMA_eAccDel;

    /*!
    ******************************************************************************
     This type defines whether the peripheral address is static or auto-incremented.
    ******************************************************************************/
    typedef enum {
        DMA_INCR_OFF            = 0,            //!< Static peripheral address.
        DMA_INCR_ON                 = 1                 //!< Incrementing peripheral address.

    } DMA_eIncr;

    /*!
    ******************************************************************************
     This type defines the burst size setting.
    ******************************************************************************/
    typedef enum {
        DMA_BURST_0             = 0x0,          //!< burst size of 0
        DMA_BURST_1     = 0x1,      //!< burst size of 1
        DMA_BURST_2     = 0x2,      //!< burst size of 2
        DMA_BURST_3     = 0x3,      //!< burst size of 3
        DMA_BURST_4     = 0x4,      //!< burst size of 4
        DMA_BURST_5     = 0x5,      //!< burst size of 5
        DMA_BURST_6     = 0x6,      //!< burst size of 6
        DMA_BURST_7     = 0x7,      //!< burst size of 7

    } DMA_eBurst;

    /*!
    ******************************************************************************

    @Function              DMA_VALUE_PERIPH_PARAM

    @Description

    This MACRO is used to aid the generation of the ui32PeripheralParam member of the
    DMA_sParams structure required by DMA_SyncAction() and DMA_AsyncAction().  If this is
    not suitable for a given application then the programmer is free to fill in the fields in
    any way they see fit.

    @Input      ACC_DEL:        The access delay (see DMA_eAccDel).

    @Input      INCR:           Whether the peripheral address is incremented (see DMA_eIncr).

    @Input      BURST:          The burst size.  This should correspond to the amount of data that the
                                        peripheral will either be able to supply or accept from its FIFO (see DMA_eBurst).

    @Return     img_uint32: The value of the generated word.

    ******************************************************************************/
#define DMA_VALUE_PERIPH_PARAM(ACC_DEL,INCR,BURST)                                                                  \
                                                                                                                                                                    \
        (((ACC_DEL)     & DMAC_DMAC_PERIPH_ACC_DEL_LSBMASK)     << DMAC_DMAC_PERIPH_ACC_DEL_SHIFT)      |   \
        (((INCR)        & DMAC_DMAC_PERIPH_INCR_LSBMASK)        << DMAC_DMAC_PERIPH_INCR_SHIFT)         |   \
        (((BURST)       & DMAC_DMAC_PERIPH_BURST_LSBMASK)       << DMAC_DMAC_PERIPH_BURST_SHIFT)



    /*!
    ******************************************************************************
     Used to describe the location of the linked list structure
    ******************************************************************************/
    typedef enum {
        DMA_LIST_IS_IN_MTX_MEM,
        DMA_LIST_IS_IN_SYS_MEM,
    } DMA_LIST_LOCATION;

    /*!
    ******************************************************************************
     DMAC linked list structure
    ******************************************************************************/
    typedef struct {
        IMG_UINT32      ui32Word_0;                             //!< Word 0 of the linked list (see DMA_LL_SET_WD0).
        IMG_UINT32      ui32Word_1;                             //!< Word 1 of the linked list (see DMA_LL_SET_WD1).
        IMG_UINT32      ui32Word_2;                             //!< Word 2 of the linked list (see DMA_LL_SET_WD2).
        IMG_UINT32      ui32Word_3;                             //!< Word 3 of the linked list (see DMA_LL_SET_WD3).
        IMG_UINT32      ui32Word_4;                             //!< Word 4 of the linked list (see DMA_LL_SET_WD4).
        IMG_UINT32      ui32Word_5;                             //!< Word 5 of the linked list (see DMA_LL_SET_WD5).
        IMG_UINT32      ui32Word_6;                             //!< Word 6 of the linked list (see DMA_LL_SET_WD6).
        IMG_UINT32      ui32Word_7;                             //!< Word 7 of the linked list (see DMA_LL_SET_WD7).

    } DMA_sLinkedList;

    /*!
    ******************************************************************************
     DMAC Parameter structure
    ******************************************************************************/
    typedef struct {
        IMG_UINT32                      ui32PerHold;                    //!< peripheral hold register (see PER_HOLD register in TRM)
        DMA_LIST_LOCATION       eListLocation;                  //!< is the linked list in mtx memory or system memory
        DMA_sLinkedList *       psDmaLinkedList;                //!< pointer to first element in the linked list
        IMG_UINT32                      ui32Ext_sa;
    } DMA_sParams;

    /*!
    ******************************************************************************

     @Function              DMA_Initialise

     @Description

     This function initialises the DMAC. Only has effect on the first call, second
     and subsequent calls are ignored.

     @Input             eChannel        : The channel to initialise.

     @Return    None.

    ******************************************************************************/
    extern IMG_VOID DMA_Initialise(DMA_eChannelId eChannel);

    /*!
    ******************************************************************************

     @Function              DMA_Reset

     @Description

     This function resets the DMAC, cancels any pending DMAC operation and
     return the DMAC to the idle state - #DMA_STATUS_IDLE.

     @Input             eChannel        : The channel to reset.

     @Return    None.

    ******************************************************************************/
    extern IMG_VOID DMA_Reset(DMA_eChannelId eChannel);

    /*!
    ******************************************************************************

     @Function              DMA_SyncAction

     @Description

     This function is used to initiate a synchronous (blocking) DMAC tranfer.

     An internal callback function is registered using DMA_RegisterStatusCallback()
     to detect and act upon status transitions.

     The DMAC driver also uses the SEMA API, SEMA_ID_B to block whilst waiting
     for the DMAC transfer to complete.  The callback function will set the
     semaphore when the

     NOTE: The DMAC must be in the idle state - #DMA_STATUS_IDLE - when the
     transfer is initiated.

     @Input             eChannel        : The channel to use.

     @Input             psParams        : A pointer to a #DMA_sParams structure set with the
                                                  required DMAC setup.

     @Input             bMtx            : If true then the peripheral address specifies an
                                                  offset in MTX memory

     @Return    DMA_eStatus : The completion status - #DMA_STATUS_COMPLETE or
                                                   #DMA_STATUS_TIMEOUT.

    ******************************************************************************/
    extern DMA_eStatus DMA_SyncAction(
        DMA_eChannelId                  eChannel,
        DMA_sParams *                   psParams,
        IMG_BOOL                                bMtx
    );

    /*!
    ******************************************************************************

     @Function              DMA_AsyncAction

     @Description

     This function is used to initiate an asynchronous (non-blocking) DMAC tranfer.

     NOTE: The DMAC must be in the idle state - #DMA_STATUS_IDLE - when the
     transfer is initiated.

     @Input             eChannel                        : The channel to use.

     @Input             psDmacLinkedList        : A pointer to a #DMA_sLinkedList structure set with the
                                                                  required DMAC setup.

     @Input             bPeriphIsMtx            : If true then the peripheral address specifies an
                                                                  offset in MTX memory.

     NOTE: If eListLocation is DMA_LIST_IS_IN_SYS_MEM and bPeriphIsMtx is IMG_TRUE the linked list can only contain a single entry.

     NOTE: If eListLocation is DMA_LIST_IS_IN_MTX_MEM then bPeriphIsMtx applies to all entries in the linked list (i.e.
           they all use the mtx as the peripheral, or none of them use the mtx as the peripheral).

     @Return    None.

    ******************************************************************************/
    extern IMG_VOID DMA_AsyncAction(
        DMA_eChannelId                  eChannel,
        DMA_sParams *                   psParams,
        IMG_BOOL                                bPeriphIsMtx
    );

    /*!
    ******************************************************************************

     @Function              DMA_WaitForTransfer

     @Description

     This function waits for the current transfer to complete or timeout.

     @Input             eChannel :      The channel to wait use.

     @Return DMA_eStatus :      DMA_STATUS_COMPLETE when transfer has completed or DMA_STATUS_IDLE
                                                if there wasn't an active transfer in progress to wait for.

    ******************************************************************************/
    extern DMA_eStatus DMA_WaitForTransfer(DMA_eChannelId eChannel);

    /*!
    ******************************************************************************

     @Function              DMA_GetStatus

     @Description

     This function returns the status of the DMAC.

     @Input             eChannel                : The channel to get the status of.

     @Return    DMA_eStatus     : The status of the DMAC.

    ******************************************************************************/
    extern DMA_eStatus DMA_GetStatus(DMA_eChannelId eChannel);

    /*!
    ******************************************************************************

     @Function              DMA_pfnStatusCallback

     @Description

     This is the prototype for a status callback functions.

     @Input             eChannel                : The channel that the status change is being reported on.

     @Input             DMA_eStatus     : The "new" state of the DMAC.

     @Return    None.

    ******************************************************************************/
    typedef IMG_VOID(*DMA_pfnStatusCallback)(
        DMA_eChannelId                          eChannel,
        DMA_eStatus                             eStatus
    );


    /*!
    ******************************************************************************

     @Function              DMA_RegisterStatusCallback

     @Description

     This function is used to register a status callback function.  The caller
     provides the address of a function that will be called when a change in the
     status occurs - see #DMA_eStatus.

     NOTE: This can happen asynchronously (at interrupt level) on a
     #DMA_STATUS_COMPLETE or #DMA_STATUS_TIMEOUT - or synchronously when
     DMA_Action() is called and the state changes to #DMA_STATUS_BUSY or
     when DMA_GetStatus() or DMA_Reset() are called and the state returns to
     #DMA_STATUS_IDLE.

     NOTE: Only one callback function can be registered with the API.  The
     callback function is persistent and is not removed by subsequent calls
     to DMA_Initialise() or DMA_Reset().

     NOTE: The function asserts if a callback function has already been set.

     @Input             eChannel                        : The channel that the status change is being reported on.

     @Input             pfnStatusCallback       : A pointer to a status callback function.

     @Return    None.

    ******************************************************************************/
    extern IMG_VOID DMA_RegisterStatusCallback(
        DMA_eChannelId                          eChannel,
        DMA_pfnStatusCallback           pfnStatusCallback
    );


#if (__cplusplus)
}
#endif

#endif /* __DMA_API_H__    */
