
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
 *             Texas Instruments OMAP (TM) Platform Software
 *  (c) Copyright Texas Instruments, Incorporated.  All Rights Reserved.
 *
 *  Use of this software is controlled by the terms and conditions found
 *  in the license agreement under which this software has been supplied.
 * =========================================================================== */
/**
 * @file OMX_G722Encoder.h
 *
 * This header file contains data and function prototypes for G722 ENCODER OMX 
 *
 * @path  $(OMAPSW_MPU)\linux\audio\src\openmax_il\g722_enc\inc
 *
 * @rev  0.1
 */
/* ----------------------------------------------------------------------------- 
 *! 
 *! Revision History 
 *! ===================================
 *! Date         Author(s)            Version  Description
 *! ---------    -------------------  -------  ---------------------------------
 *! 08-Mar-2007  A.Donjon             0.1      Code update for G722 ENCODER
 *! 
 *!
 * ================================================================================= */
#ifndef OMX_G722ENCODER_H
#define OMX_G722ENCODER_H

#include "LCML_DspCodec.h"
#include <OMX_Component.h>
#include <pthread.h>

/* ======================================================================= */
/** OMX_G722ENC_INDEXAUDIOTYPE  Defines the custom configuration settings
 *                              for the component
 *
 *  @param  OMX_IndexCustomG722EncModeDasfConfig      Sets the DASF mode
 *
 *  @param  OMX_IndexCustomG722EncModeTeeModeConfig   Sets the TEE mode
 *  
 */
/*  ==================================================================== */
typedef enum OMX_G722ENC_INDEXAUDIOTYPE {
    OMX_IndexCustomG722EncModeConfig = 0xFF000001,
    OMX_IndexCustomG722EncHeaderInfoConfig,
    OMX_IndexCustomG722EncStreamIDConfig,
    OMX_IndexCustomG722EncDataPath
}OMX_G722ENC_INDEXAUDIOTYPE;


/* ======================================================================= */
/**
 * @def    G722ENC_NUM_INPUT_BUFFERS    Component default number of input buffers
 */
/* ======================================================================= */
#define G722ENC_NUM_INPUT_BUFFERS 1


/* ======================================================================= */
/**
 * @def    G722ENC_NUM_OUTPUT_BUFFERS    Component default number of output buffers
 */
/* ======================================================================= */
#define G722ENC_NUM_OUTPUT_BUFFERS 1


/* ======================================================================= */
/**
 * @def    G722ENC_INPUT_BUFFER_SIZE    Component default input buffer size
 */
/* ======================================================================= */
#define G722ENC_INPUT_BUFFER_SIZE 320

/* ======================================================================= */
/**
 * @def    G722ENC_OUTPUT_BUFFER_SIZE_BYTES     Component default output buffer size
 */
/* ======================================================================= */
#define G722ENC_OUTPUT_BUFFER_SIZE_BYTES 320


#endif
