
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
* =========================================================================== */
/**
* @file OMX_VideoEnc_CustomCmd.h
*
* This is an header file for an video Mpeg4 encoder that is fully
* compliant with the OMX Video specification.
* This the file that the application that uses OMX would include
* in its code.
*
* @path $(CSLPATH)\
*
* @rev 0.1
*/
/* -------------------------------------------------------------------------- */

#ifndef OMX_VIDEOENC_CUSTOMCMD_H
#define OMX_VIDEOENC_CUSTOMCMD_H

#define VIDENC_PARAM_VBVSIZE               "OMX.TI.VideoEncode.Param.VBVSize";
#define VIDENC_PARAM_DEBLOCK_FILTER        "OMX.TI.VideoEncode.Param.DeblockFilter";
#define VIDENC_CONFIG_FORCE_I_FRAME        "OMX.TI.VideoEncode.Config.ForceIFrame";
#define VIDENC_CONFIG_INTRA_FRAME_INTERVAL "OMX.TI.VideoEncode.Config.IntraFrameInterval";
#define VIDENC_CONFIG_TARGET_FRAMERATE     "OMX.TI.VideoEncode.Config.TargetFrameRate";
#define VIDENC_CONFIG_QPI                  "OMX.TI.VideoEncode.Config.QPI";
#define VIDENC_CONFIG_AIRRATE              "OMX.TI.VideoEncode.Config.AIRRate";
#define VIDENC_CONFIG_TARGET_BITRATE       "OMX.TI.VideoEncode.Config.TargetBitRate";

#endif /* OMX_VIDEOENC_CUSTOMCMD_H */
