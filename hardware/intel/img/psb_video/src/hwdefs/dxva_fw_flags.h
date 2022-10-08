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

 @File         dxva_fw_flags.h

 @Title        Dxva Firmware Message Flags

 @Platform

 @Description

******************************************************************************/
#ifndef _VA_FW_FLAGS_H_
#define _VA_FW_FLAGS_H_

/* Flags */

#define FW_VA_RENDER_IS_FIRST_SLICE                                             0x00000001
#define FW_VA_RENDER_IS_H264_MBAFF                                              0x00000002
#define FW_VA_RENDER_IS_LAST_SLICE                                              0x00000004
#define FW_VA_RENDER_IS_TWO_PASS_DEBLOCK                                        0x00000008

#define REPORT_STATUS                                                                           0x00000010

#define FW_VA_RENDER_IS_VLD_NOT_MC                                              0x00000800
#define FW_DEVA_IMMEDIATE_ABORT_FAULTED                                         0x00000800

#define FW_ERROR_DETECTION_AND_RECOVERY                                         0x00000100
#define FW_VA_RENDER_NO_RESPONCE_MSG                                            0x00002000  /* Cause no responce message to be sent, and no interupt generation on successfull completion */
#define FW_VA_RENDER_HOST_INT                                                           0x00004000
#define FW_DEVA_DEBLOCK_ENABLE                                              0x00000400
#define FW_VA_RENDER_VC1_BITPLANE_PRESENT                                   0x00008000
#define FW_INTERNAL_CONTEXT_SWITCH                                          0x00000040
#endif /*_VA_FW_FLAGS_H_*/
