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

#ifndef TOPAZHP_DEFAULT_PARAMS_H
#define TOPAZHP_DEFAULT_PARAMS_H

/* moved from rc.h as only used in Host code */
#define TH_SKIP_IPE			6
#define TH_INTER			60
#define TH_INTER_QP			10
#define TH_INTER_MAX_LEVEL	1500
#define TH_SKIP_SPE			6
#define SPE_ZERO_THRESHOLD	6

#define UNINIT_PARAM 0xCDCDCDCD
#define UNINIT_PARAM_BYTE 0xCD


/* Comandline Params defaults */
#define TOPAZHP_DEFAULT_ui32pseudo_rand_seed		UNINIT_PARAM
#define TOPAZHP_DEFAULT_bRCBiases					true
#define TOPAZHP_DEFAULT_uJPEGQuality				90
#define TOPAZHP_DEFAULT_iFrameCount					2

#define TOPAZHP_DEFAULT_iFrameSpeed					1
#define TOPAZHP_DEFAULT_uiFcode						4
#define TOPAZHP_DEFAULT_uiSlicesPerFrame			1
#define TOPAZHP_DEFAULT_iJitterMagnitude			1
#define TOPAZHP_DEFAULT_bEnableIntra				1
#define TOPAZHP_DEFAULT_iFineYSearchSize			2
#define TOPAZHP_DEFAULT_eMinBlockSize				E4x4Blocks
#define TOPAZHP_DEFAULT_uVaryQpStep					1
#define TOPAZHP_DEFAULT_uIterations					1

#define TOPAZHP_DEFAULT_eRCEnable					ERCDisable
#define TOPAZHP_DEFAULT_eRCMode						IMG_RCMODE_NONE

#define TOPAZHP_DEFAULT_iQP_Luma					params->iQP_Luma
#define TOPAZHP_DEFAULT_iQP_Chroma					TOPAZHP_DEFAULT_iQP_Luma
#define TOPAZHP_DEFAULT_iQP_ILuma					TOPAZHP_DEFAULT_iQP_Luma
#define TOPAZHP_DEFAULT_iQP_PLuma					TOPAZHP_DEFAULT_iQP_Luma
#define TOPAZHP_DEFAULT_iQP_IChroma					TOPAZHP_DEFAULT_iQP_Luma
#define TOPAZHP_DEFAULT_iQP_PChroma					TOPAZHP_DEFAULT_iQP_Luma

#define TOPAZHP_DEFAULT_uTHInter					TH_INTER
#define TOPAZHP_DEFAULT_uTHInterQP					TH_INTER_QP
#define TOPAZHP_DEFAULT_uTHInterMaxLevel			TH_INTER_MAX_LEVEL
#define TOPAZHP_DEFAULT_uTHSkipIPE					TH_SKIP_IPE
#define TOPAZHP_DEFAULT_uTHSkipSPE					TH_SKIP_SPE
#define TOPAZHP_DEFAULT_uiSpeZeroThreshold			SPE_ZERO_THRESHOLD
#define TOPAZHP_DEFAULT_bPowerTest					IMG_FALSE

#define TOPAZHP_DEFAULT_uCABACBinLimit				2800

// advised number
#define TOPAZHP_DEFAULT_uCABACBinFlex				2800

// Fairly random defaults.
#define TOPAZHP_DEFAULT_iMvcalcGridMBXStep			7
#define TOPAZHP_DEFAULT_iMvcalcGridMBYStep			13
#define TOPAZHP_DEFAULT_iMvcalcGridSubStep			3
#define TOPAZHP_DEFAULT_uDupVectorThreshold			1
#define TOPAZHP_DEFAULT_uZeroBlock4x4Threshold		6
#define TOPAZHP_DEFAULT_uZeroBlock8x8Threshold		4

#define TOPAZHP_DEFAULT_bH2648x8Transform			false
#define TOPAZHP_DEFAULT_bZeroDetectionDisable		false
#define TOPAZHP_DEFAULT_bH264IntraConstrained		false
#define TOPAZHP_DEFAULT_uDirectVecBias				UNINIT_PARAM
#define TOPAZHP_DEFAULT_uIPESkipVecBias				UNINIT_PARAM

#define TOPAZHP_DEFAULT_iInterMBBias				UNINIT_PARAM
#define TOPAZHP_DEFAULT_iInterMBBiasB				UNINIT_PARAM
#define TOPAZHP_DEFAULT_iIntra16Bias				UNINIT_PARAM

//register defaults	
#define TOPAZHP_DEFAULT_uChunksPerMb				0x40
#define TOPAZHP_DEFAULT_uMaxChunks					0xA0
#define TOPAZHP_DEFAULT_uPriorityChunks				(TOPAZHP_DEFAULT_uMaxChunks - 0x60)

#define TOPAZHP_DEFAULT_bUseOffScreenMVUserSetting	false;

#define TOPAZHP_DEFAULT_ui8EnableSelStatsFlags		0
#define TOPAZHP_DEFAULT_bEnableInpCtrl				false;
#define TOPAZHP_DEFAULT_bEnableAIR					false;

// 0 = _ALL_ MBs over threshold will be marked as AIR Intras, -1 = Automatically generate 10% Blocks Per Frame
#define TOPAZHP_DEFAULT_i32NumAIRMBs				-1
// -1 = Automatically generate and adjust threshold
#define TOPAZHP_DEFAULT_i32AIRThreshold				-1
// -1 = Random (0 - NumAIRMbs) skip between frames in AIR table
#define TOPAZHP_DEFAULT_i16AIRSkipCnt				-1

#define TOPAZHP_DEFAULT_uBasePipe					0

//Set the default to 10
#define TOPAZHP_DEFAULT_uiCbrBufferTenths			10

/*default value for CARC*/
#define TOPAZHP_DEFAULT_uCARCThreshold               1
#define TOPAZHP_DEFAULT_uCARCCutoff	                    15
#define TOPAZHP_DEFAULT_uCARCPosRange               5
#define TOPAZHP_DEFAULT_uCARCPosScale                12
#define TOPAZHP_DEFAULT_uCARCNegRange               5
#define TOPAZHP_DEFAULT_uCARCNegScale                12
#define TOPAZHP_DEFAULT_uCARCShift                        3
#define TOPAZHP_DEFAULT_ui8InterIntraIndex                3
#define TOPAZHP_DEFAULT_ui8CodedSkippedIndex        3

#define TOPAZHP_DEFAULT_ui8MPEG2IntraDCPrecision    10
#define TOPAZHP_DEFAULT_uDebugCRCs                         0

// Default to adaptive rounding active
#define TOPAZHP_DEFAULT_bTPAdaptiveRoundingDisable	false

#define TOPAZHP_DEFAULT_ui16VirtRatio				1
#define TOPAZHP_DEFAULT_ui16ChunksPerPic			1
#define TOPAZHP_DEFAULT_bHierarchical				false
#define TOPAZHP_DEFAULT_bForceReferences			false
#define TOPAZHP_DEFAULT_bBaseMvcParams				true
#define TOPAZHP_DEFAULT_bIPEHighLatency				true
#define TOPAZHP_DEFAULT_uAppQuant					2
#define TOPAZHP_DEFAULT_bUseFirmwareALLRC			false
#define TOPAZHP_DEFAULT_ui8Fourcc					"YUV"



/* Context defaults */
#define TOPAZHP_DEFAULT_ui32IdrSec					0
#define TOPAZHP_DEFAULT_ui32VopTimeResolution		15
// Only used for MPEG2, other formats must ensure this is set to zero
#define TOPAZHP_DEFAULT_ui8ActiveSourceBuffer		ACTIVE_BUFFER_NONE

#endif /* TOPAZHP_DEFAULT_PARAMS_H */
