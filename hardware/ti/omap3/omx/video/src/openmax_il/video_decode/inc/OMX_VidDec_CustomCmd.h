
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
* @file OMX_VidDec_CustomCmd.h
*
* This is an header file for an video Mpeg4 decoder that is fully 
* compliant with the OMX Video specification.
* This the file that the application that uses OMX would include 
* in its code.
*
* @path $(CSLPATH)\
* 
* @rev 0.1
*/
/* -------------------------------------------------------------------------- */

#ifndef OMX_VIDDEC_CUSTOMCMD_H
#define OMX_VIDDEC_CUSTOMCMD_H
#define VIDDEC_CUSTOMPARAM_PROCESSMODE "OMX.TI.VideoDecode.Param.ProcessMode"
#define VIDDEC_CUSTOMPARAM_H264BITSTREAMFORMAT "OMX.TI.VideoDecode.Param.H264BitStreamFormat"
#define VIDDEC_CUSTOMPARAM_WMVPROFILE "OMX.TI.VideoDecode.Param.WMVProfile"
#define VIDDEC_CUSTOMPARAM_WMVFILETYPE "OMX.TI.VideoDecode.Param.WMVFileType"
#define VIDDEC_CUSTOMPARAM_PARSERENABLED "OMX.TI.VideoDecode.Param.ParserEnabled"
#define VIDDEC_CUSTOMPARAM_ISNALBIGENDIAN "OMX.TI.VideoDecode.Param.IsNALBigEndian"
#define VIDDEC_CUSTOMCONFIG_DEBUG "OMX.TI.VideoDecode.Debug"
#ifdef VIDDEC_SPARK_CODE 
 #define VIDDEC_CUSTOMPARAM_ISSPARKINPUT "OMX.TI.VideoDecode.Param.IsSparkInput"
#endif

/*------- Program Header Files ----------------------------------------*/
#include <OMX_Component.h>

/*------- Structures ----------------------------------------*/

#endif /* OMX_VIDDEC_CUSTOMCMD_H */
