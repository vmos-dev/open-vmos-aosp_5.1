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


/******************************************************************************

@File         dxva_msg.h

@Title        va Firmware Message Flags

@Platform     Independent

@Description  </b>\n This file contains the header file information for the VA
              specific MTX/Host messages

******************************************************************************/
#if !defined (__VA_MSG_H__)
#define __VA_MSG_H__

#if (__cplusplus)
extern "C" {
#endif

    /* These come from fwrk_api.h */
    /* #include <fwrk_api.h>  */
#define FWRK_MSGID_START_PSR_HOSTMTX_MSG        (0x80)  //!< Start of parser specific Host->MTX messages.
#define FWRK_MSGID_START_PSR_MTXHOST_MSG        (0xC0)  //!< Start of parser specific MTX->Host messages.
#define FWRK_MSGID_START_USER_DEFINED_MSG       (0xF0)  //!< Start of parser user defined messages.
#define FWRK_MSGID_PADDING                                      ( 0 )

#define FWRK_MSGID_HOST_EMULATED                (0x40)


    /*!
    ******************************************************************************
     This type defines the framework specified message ids

     The messages are packed memory based structures accessed using the mem_io.h
     macros.  The control values for these are generated from a file called
     dxva_cmdseq_msg.def using the "regdef" tool.

    ******************************************************************************/
    enum {
        /*! Sent by the VA driver on the host to the mtx firmware.
         */
        VA_MSGID_INIT                           = FWRK_MSGID_START_PSR_HOSTMTX_MSG,
        VA_MSGID_RENDER,
        VA_MSGID_DEBLOCK,
        VA_MSGID_OOLD,

        /* Test Messages */
        VA_MSGID_TEST1,
        DAVA_MSGID_HOST_BE_OPP,
#ifdef SLICE_HEADER_PARSING
        VA_MSGID_SLICE_HEADER_EXTRACT = 0x87,
        VA_MSGID_MODULAR_SLICE_HEADER_EXTRACT = 0x88,
#endif
        /*! Sent by the mtx firmware to itself.
         */
        VA_MSGID_RENDER_MC_INTERRUPT,

        VA_MSGID_DEBLOCK_MFLD = FWRK_MSGID_HOST_EMULATED,
        VA_MSGID_OOLD_MFLD,
        VA_MSGID_TEST1_MFLD,
        VA_MSGID_HOST_BE_OPP_MFLD,

        /*! Sent by the VA firmware on the MTX to the host.
         */
        VA_MSGID_CMD_COMPLETED = FWRK_MSGID_START_PSR_MTXHOST_MSG,
        VA_MSGID_CMD_COMPLETED_BATCH,
        VA_MSGID_DEBLOCK_REQUIRED,
        VA_MSGID_TEST_RESPONCE,
        VA_MSGID_ACK,

        VA_MSGID_CMD_FAILED,
        VA_MSGID_CMD_UNSUPPORTED,
        VA_MSGID_CMD_HW_PANIC,
        VA_MSGID_FRAME_INFO = FWRK_MSGID_START_USER_DEFINED_MSG,
    };

#if (__cplusplus)
}
#endif

#endif
