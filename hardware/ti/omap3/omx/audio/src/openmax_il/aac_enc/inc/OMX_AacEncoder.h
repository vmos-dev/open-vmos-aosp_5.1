
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
* @file OMX_AacEncoder.h
*
* This is an header file for an audio AAC encoder that is fully
* compliant with the OMX Audio specification.
* This the file that the application that uses OMX would include
* in its code.
*
* @path $(CSLPATH)\
*
* @rev 1.0
*/
/* --------------------------------------------------------------------------- */


#ifndef OMX_AACENCODER_H
#define OMX_AACENCODER_H


#include "LCML_DspCodec.h"
#include <OMX_Component.h>
#include <pthread.h>

/*
 *     M A C R O S
 */
#define AACENC_TIMEOUT (1000) /* millisecs, default timeout used to come out of blocking calls*/
#define NUM_AACENC_INPUT_BUFFERS 8
#define NUM_AACENC_INPUT_BUFFERS_DASF 2
#define NUM_AACENC_OUTPUT_BUFFERS 8
#define INPUT_AACENC_BUFFER_SIZE 8192
#define INPUT_AACENC_BUFFER_SIZE_DASF 8192
#define OUTPUT_AACENC_BUFFER_SIZE 9200
#define SAMPLING_FREQUENCY 44100
#define MAX_NUM_OF_BUFS 10
#define NUM_OF_PORTS 2

typedef enum OMX_AACENC_INDEXAUDIOTYPE {
    OMX_IndexCustomAacEncHeaderInfoConfig = 0xFF000001,
    OMX_IndexCustomAacEncStreamIDConfig,
    OMX_IndexCustomAacEncFramesPerOutBuf,
    OMX_IndexCustomAacEncDataPath,
    OMX_IndexCustomDebug
}OMX_AACENC_INDEXAUDIOTYPE;

#endif /* OMX_AACENCODER_H */


