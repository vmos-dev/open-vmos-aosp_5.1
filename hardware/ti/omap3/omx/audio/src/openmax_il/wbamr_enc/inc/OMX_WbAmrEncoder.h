
/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
/* =============================================================================
*             Texas Instruments OMAP(TM) Platform Software
*  (c) Copyright Texas Instruments, Incorporated.  All Rights Reserved.
*
*  Use of this software is controlled by the terms and conditions found
*  in the license agreement under which this software has been supplied.
* ============================================================================ */
/**
* @file OMX_WbAmrEncoder.h
*
* This is an header file for an WBAMR Encoder that is fully
* compliant with the OMX Audio specification 1.5.
* This the file that the application that uses OMX would include in its code.
*
* @path $(CSLPATH)\OMAPSW_MPU\linux\audio\src\openmax_il\wbamr_enc\inc
*
* @rev 1.0
*/
/* --------------------------------------------------------------------------- */
/* ----------------------------------------------------------------------------
*!
*! Revision History
*! ===================================
*! 21-sept-2006 bk: updated review findings for alpha release
*! 24-Aug-2006 bk: Khronos OpenMAX (TM) 1.0 Conformance tests some more
*! 18-July-2006 bk: Khronos OpenMAX (TM) 1.0 Conformance tests validated for few cases
*! 21-Jun-2006 bk: Khronos OpenMAX (TM) 1.0 migration done
*! 22-May-2006 bk: DASF recording quality improved
*! 19-Apr-2006 bk: DASF recording speed issue resloved
*! 23-Feb-2006 bk: DASF functionality added
*! 18-Jan-2006 bk: Repated recording issue fixed and LCML changes taken care
*! 14-Dec-2005 bk: Initial Version
*! 16-Nov-2005 bk: Initial Version
*! 23-Sept-2005 bk: Initial Version
*! 10-Sept-2005 bk: Initial Version
*! 10-Sept-2005 bk:
*! This is newest file
* =========================================================================== */

#ifndef OMX_WBAMRENCODER_H
#define OMX_WBAMRENCODER_H

#include <OMX_Component.h>
#include <pthread.h>

/* ======================================================================= */
/**
 * @def WBAMRENC_NUM_INPUT_BUFFERS   Default number of input buffers
 */
/* ======================================================================= */
#define WBAMRENC_NUM_INPUT_BUFFERS 5
/* ======================================================================= */
/**
 * @def WBAMRENC_NUM_INPUT_BUFFERS_DASF  Default No.of input buffers DASF
 */
/* ======================================================================= */
#define WBAMRENC_NUM_INPUT_BUFFERS_DASF 2
/* ======================================================================= */
/**
 * @def WBAMRENC_NUM_OUTPUT_BUFFERS   Default number of output buffers
 */
/* ======================================================================= */
#define WBAMRENC_NUM_OUTPUT_BUFFERS 9
/* ======================================================================= */
/**
 * @def WBAMRENC_INPUT_BUFFER_SIZE       Default input buffer size
 *      WBAMRENC_INPUT_BUFFER_SIZE_DASF  Default input buffer size DASF
 *      WBAMRENC_INPUT_FRAME_SIZE        Default input Frame Size
 */
/* ======================================================================= */
#define WBAMRENC_INPUT_BUFFER_SIZE 640
#define WBAMRENC_INPUT_BUFFER_SIZE_DASF 640
#define WBAMRENC_INPUT_FRAME_SIZE 640
/* ======================================================================= */
/**
 * @def WBAMRENC_OUTPUT_BUFFER_SIZE   Default output buffer size
 *      WBAMRENC_OUTPUT_FRAME_SIZE    Default output frame size
 */
/* ======================================================================= */
#define WBAMRENC_OUTPUT_BUFFER_SIZE 116
#define WBAMRENC_OUTPUT_FRAME_SIZE 116
/* ======================================================================= */
/**
 * @def WBAMRENC_OUTPUT_BUFFER_SIZE_MIME  Default input buffer size MIME
 */
/* ======================================================================= */
#define WBAMRENC_OUTPUT_BUFFER_SIZE_MIME 61
/* ======================================================================= */
/**
 * @def WBAMRENC_OUTPUT_BUFFER_SIZE_MIME  Default input buffer size IF2
 */
/* ======================================================================= */
#define WBAMRENC_OUTPUT_BUFFER_SIZE_IF2 32

/* ======================================================================= */
/*
 * @def WBAMRENC_APP_ID  App ID Value setting
 */
/* ======================================================================= */
#define WBAMRENC_APP_ID 100

/* ======================================================================= */
/** WBAMRENC_OMX_INDEXAUDIOTYPE  Defines the custom configuration settings
*                                for the component
*  @param  OMX_WbIndexCustomModeEfrConfig  Sets the EFR mode
*
*  @param  OMX_WbIndexCustomModeAmrConfig  Sets the WBAMR mode
*
*  @param  OMX_WbIndexCustomModeDasfConfig  Sets the DASF mode
*
*  @param  OMX_WbIndexCustomModeAcdnConfig  Sets the ACDN mode
*
*  @param  OMX_WbIndexCustomModeMimeConfig  Sets the MIME mode
*
*  @param  OMX_WbIndexCustomModeMultiFrameConfig  Sets the MultiFrame mode
*/
/* ======================================================================= */
typedef enum WBAMRENC_OMX_INDEXAUDIOTYPE {
    OMX_WbIndexCustomModeEfrConfig = 0xFF000001,
    OMX_WbIndexCustomModeAmrConfig,
    OMX_WbIndexCustomModeAcdnConfig,
    OMX_WbIndexCustomModeDasfConfig,
    OMX_WbIndexCustomModeMimeConfig,
    OMX_WbIndexCustomModeMultiFrameConfig,
    OMX_IndexCustomWbAmrEncHeaderInfoConfig,
    OMX_IndexCustomWbAmrEncStreamIDConfig,
    OMX_WbIndexCustomDataPath,
    OMX_IndexCustomDebug
} WBAMRENC_OMX_INDEXAUDIOTYPE;

#endif /* OMX_WBAMRENCODER_H */
