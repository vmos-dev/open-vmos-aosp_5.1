
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
 * @file OMX_G726Decoder.h
 *
 * This is an header file for an audio G726 decoder that is fully
 * compliant with the OMX Audio specification.
 * This the file that the application that uses OMX would include
 * in its code.
 *
 * @path  $(CSLPATH)\OMAPSW_MPU\linux\audio\src\openmax_il\g726_dec\inc\
 *
 * @rev 1.0
 */
/* --------------------------------------------------------------------------- */


#ifndef OMX_G726DECODER_H
#define OMX_G726DECODER_H


#ifdef UNDER_CE
#include <windows.h>
#include <oaf_osal.h>
#include <omx_core.h>
#include <stdlib.h>
#else
#include <pthread.h>
#endif

#include <OMX_Component.h>

/*#define G726DEC_DEBUG  */    /* See all debug statement of the component */
/* #define SWAT_ANALYSIS */   /* Enable to use SWAT functionality */
/*#define G726DEC_MEMDETAILS  */ /* See memory details of the component */
/* #define G726DEC_BUFDETAILS */  /* See buffers details of the component */
/*#define G726DEC_STATEDETAILS  */ /* See all state transitions of the component */
/*#define G726DEC_SWATDETAILS  */  /* See SWAT debug statement of the component */


#define MAX_NUM_OF_BUFS 10 /* Max number of buffers used */
#define G726D_NUM_INPUT_BUFFERS 1  /* Default number of input buffers */
#define G726D_NUM_OUTPUT_BUFFERS 2 /* Default number of output buffers */
#define G726D_INPUT_BUFFER_SIZE  20 /* Default size of input buffer */
#define G726D_OUTPUT_BUFFER_SIZE 160 /* Default size of output buffer */

#define NUM_OF_PORTS 2 /* Number of ports of component */

#define INVALID_SAMPLING_FREQ  51

/* ======================================================================= */
/** OMX_G726DEC_INDEXAUDIOTYPE  Defines the custom configuration settings
 *                              for the component
 *
 *  @param  OMX_IndexCustomG726DecModeDasfConfig      Sets the DASF mode
 *
 */
/*  ==================================================================== */
typedef enum OMX_G726DEC_INDEXAUDIOTYPE {
    OMX_IndexCustomG726DecModeDasfConfig = OMX_IndexIndexVendorStartUnused + 1,
    OMX_IndexCustomG726DecHeaderInfoConfig
}OMX_G726DEC_INDEXAUDIOTYPE;


/* ============================================================================== * */
/** G726D_COMP_PORT_TYPE  describes the input and output port of indices of the
 * component.
 *
 * @param  G726D_INPUT_PORT  Input port index
 *
 * @param  G726D_OUTPUT_PORT Output port index
 */
/* ============================================================================ * */
typedef enum G726D_COMP_PORT_TYPE {
    G726D_INPUT_PORT = 0,
    G726D_OUTPUT_PORT
}G726D_COMP_PORT_TYPE;

#endif /* OMX_G726DECODER_H */
