
/*
 *  Copyright 2001-2008 Texas Instruments - http://www.ti.com/
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 * limitations under the License.
 */
/* ================================================================================
 *             Texas Instruments OMAP(TM) Platform Software
 *  (c) Copyright Texas Instruments, Incorporated.  All Rights Reserved.
 *
 *  Use of this software is controlled by the terms and conditions found 
 *  in the license agreement under which this software has been supplied.
 * ================================================================================ */
/**
 * @file G729DecTest.h
 *
 * This header file contains data and function prototypes for G729 DECODER OMX tests
 *
 * @path  $(OMAPSW_MPU)\linux\audio\src\openmax_il\g729_dec\tests
 *
 * @rev  0.3
 */
/* ----------------------------------------------------------------------------- 
 *! 
 *! Revision History 
 *! ===================================
 *! Date         Author(s)            Version  Description
 *! ---------    -------------------  -------  ---------------------------------
 *! 05-Jan-2007  A.Donjon             0.1      Creation
 *! 19-Feb-2007  A.Donjon             0.2      Frame lost parameter
 *! 08-Jun-2007  A.Donjon             0.3      Variable input buffer size
 *! 
 *!
 * ================================================================================= */
/* ------compilation control switches -------------------------*/

#define OUTPUT_G729DEC_BUFFER_SIZE  80<<1   /* Output frame size in bytes: 10ms 8kHz 16-bit samples */
#define INPUT_G729DEC_BUFFER_SIZE   11      /* Input buffer size in bytes: 10+1 bytes (1 byte header)*/

#define SYNC_WORD               0x6b21      /* BFI untransmitted frame */

#define BIT_0                   0x007f      /* definition of bit 0 in ITU input bit-stream */
#define BIT_1                   0x0081      /* definition of bit 1 in ITU input bit-stream */

#define SHIFT_CONST             24       /* Shift to get number of packets in input buffer */

/* G729 frame size */
#define SPEECH_FRAME_SIZE       80          /* Speech: 80 bits */
#define SID_FRAME_SIZE          15          /* SID: 15 bits */
#define SID_OCTET_FRAME_SIZE    16          /* SID: 16 bits */

/* G729 frame type */
#define NO_TX_FRAME_TYPE        0           /* No tx frame */
#define SPEECH_FRAME_TYPE       1           /* Speech Frame flag */
#define SID_FRAME_TYPE          2           /* SID frame flag */
#define ERASURE_FRAME           3           /* Erasure frame flag */

#define ITU_INPUT_SIZE          82          /*  ITU bitstream 
                                                bfi+ number of bits in frame + serial bitstream */

#define G729SPEECHPACKETSIZE    10          /* G729 speech packet size in bytes */
#define G729SIDPACKETSIZE       2           /* G729 SID packet size in bytes */

/****************************************************************
 *  INCLUDE FILES                                                 
 ****************************************************************/
/* ----- system and platform files ----------------------------*/
/*-------program files ----------------------------------------*/


/****************************************************************
 * PUBLIC DECLARATIONS Defined here, used elsewhere
 ****************************************************************/
/*--------data declarations -----------------------------------*/

/* ===========================================================================*/
/** 
 *   Private data that application associates with buffer 
 */
/* ===========================================================================*/


/*--------function prototypes ---------------------------------*/


/****************************************************************
 * PRIVATE DECLARATIONS Defined here, used only here
 ****************************************************************/
/*--------data declarations -----------------------------------*/

/*--------function prototypes ---------------------------------*/



/*--------macros ----------------------------------------------*/
