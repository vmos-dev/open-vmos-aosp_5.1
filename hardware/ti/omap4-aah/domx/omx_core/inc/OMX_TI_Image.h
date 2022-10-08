/*
 * Copyright (c) 2010, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * @file:OMX_TI_Image.h
 * This header defines the structures specific to the param or config
 * indices of Openmax Image Component.
 *
 *  @path domx/system/omx_core/inc
 *
 *  @rev 1.0
 */

/*==============================================================
 *! Revision History
 *! ============================
 *! 11-Oct-2010  KMotov 	x0106653@ti.com  	Initial Version
 *================================================================*/

#ifndef OMX_TI_IMAGE_H
#define OMX_TI_IMAGE_H

/**
 * The OMX_TI_IMAGE_CODINGTYPE enumeration is used to define the
 * extended image coding types.
 */
typedef enum OMX_TI_IMAGE_CODINGTYPE {
	OMX_TI_IMAGE_CodingJPS = OMX_IMAGE_CodingVendorStartUnused + 1, /**< JPS format */
    OMX_TI_IMAGE_CodingMPO,           /**< MPO format */
    OMX_TI_IMAGE_CodingJPEG_SOC,
    OMX_TI_IMAGE_CodingJPEG_SOC_SPOOFED
} OMX_TI_IMAGE_CODINGTYPE;

#endif /* OMX_TI_IMAGE_H */
