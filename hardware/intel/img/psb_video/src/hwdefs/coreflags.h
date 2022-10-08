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

#ifndef _COREFLAGS_H_
#define _COREFLAGS_H_


#ifdef __cplusplus
extern "C" {
#endif


#define SERIALIZED_PIPES 1


#if defined(ENCFMT_H264MVC) || !defined(TOPAZ_MTX_HW)
#	define ENCFMT_H264
#	define _ENABLE_MVC_        /* Defined for enabling MVC specific code */
#	define _MVC_RC_  1         /* Defined for enabling MVC rate control code, later on it will be replaced by _ENABLE_MVC_ */
#endif

#if defined(RCMODE_ALL) && !defined(ENCFMT_H264)
	#		error "H264 is not defined when RCMODE is ALL."
#endif

#if defined(RCMODE_ALL) && (defined(ENCFMT_JPEG) || defined(ENCFMT_JPEG) || defined(ENCFMT_JPEG) || defined(ENCFMT_MPG4) || defined(ENCFMT_MPG2) || defined(ENCFMT_H263) || defined(ENCFMT_H264MVC))
	#		error "RCMODE_ALL is only allowed for H264."
#endif

#if defined(RCMODE_ALL) || !defined(TOPAZ_MTX_HW)
#	define RCMODE_CBR
#	define RCMODE_VBR
#	define RCMODE_VCM
#	define __COREFLAGS_RC_ALLOWED__ 1
#endif

#define TOPAZHP_MAX_NUM_STREAMS 2

/*
	Hierarhical B picture support is disabled:
		* for H264 VCM mode, as VCM mode is not expected to use B pictures at all
*/
#if  !defined(TOPAZ_MTX_HW) || \
		(defined(ENCFMT_H264) && !(defined(RCMODE_VCM))) \
		|| defined(RCMODE_ALL)
#	define INCLUDE_HIER_SUPPORT	1
#else
#	define INCLUDE_HIER_SUPPORT	0
#endif

// Add this back in for SEI inclusion
#if !defined(RCMODE_NONE)
#	if  (!defined(TOPAZ_MTX_HW) || ((defined(RCMODE_VBR) || defined(RCMODE_CBR) || defined(RCMODE_VCM) || defined(RCMODE_LLRC)) && defined(ENCFMT_H264)))
#		define SEI_INSERTION 1  
#	endif
#endif

#if	(!defined(TOPAZ_MTX_HW) || defined(ENCFMT_H264))
#	define WEIGHTED_PREDICTION 1
#	define MULTI_REF_P
#	define LONG_TERM_REFS
#	define CUSTOM_QUANTIZATION
#endif

// Firmware bias table control is not compatible with parallel encoding
//#define FIRMWARE_BIAS

/* This define controls whether the hard VCM bitrate limit is applied to I frames */
//#define TOPAZHP_IMPOSE_VCM_BITRATE_LIMIT_ON_INTRA 

#if INCLUDE_HIER_SUPPORT
#if ( !defined(TOPAZ_MTX_HW) || !defined(ENCFMT_H264MVC) )
#	define MAX_REF_B_LEVELS_FW	(MAX_REF_B_LEVELS)
#else
#	define MAX_REF_B_LEVELS_FW	2
#endif
#else
#	define MAX_REF_B_LEVELS_FW	0
#endif

#ifdef MULTI_REF_P
#	define MAX_REF_I_OR_P_LEVELS_FW	(MAX_REF_I_OR_P_LEVELS)
#else
#	define MAX_REF_I_OR_P_LEVELS_FW	2
#endif

#define MAX_REF_LEVELS_FW	(MAX_REF_B_LEVELS_FW + MAX_REF_I_OR_P_LEVELS_FW)
#define MAX_PIC_NODES_FW	(MAX_REF_LEVELS_FW + 1)


#if defined(TOPAZ_MTX_HW)
#	if defined(ENABLE_FORCED_INLINE)
/*
	__attribute__((always_inline)) should be only used when all C code is compiled by
	GCC to a blob object with `-combine` swithch.
*/
#		define MTX_INLINE  inline __attribute__((always_inline))
#else
#		define MTX_INLINE  inline
#endif
#else
#	define MTX_INLINE
#endif

/* 
	Check that only one `RCMODE_` symbol is defined.

	If `RCMODE_NONE` is set, block any possebility for RC sources to be included.
*/


#if defined(RCMODE_NONE)
#	define __COREFLAGS_RC_ALLOWED__	0
#else
#	define __COREFLAGS_RC_ALLOWED__ 1
#endif

/* 
	Determine possible rate control modes.
*/
#if !defined(TOPAZ_MTX_HW)
#	define	CBR_RC_MODE_POSSIBLE	__COREFLAGS_RC_ALLOWED__
#	define  VCM_RC_MODE_POSSIBLE	__COREFLAGS_RC_ALLOWED__
#	define  VBR_RC_MODE_POSSIBLE	__COREFLAGS_RC_ALLOWED__
#	define  ERC_RC_MODE_POSSIBLE	__COREFLAGS_RC_ALLOWED__
#else
#	if defined(RCMODE_CBR)
#		define	CBR_RC_MODE_POSSIBLE	__COREFLAGS_RC_ALLOWED__
#	else
#		define	CBR_RC_MODE_POSSIBLE	FALSE
#	endif
#	if defined(ENCFMT_H264) && (defined(RCMODE_VCM) || defined(RCMODE_LLRC))
		/* VCM is possible only for H264 */
#		define  VCM_RC_MODE_POSSIBLE	__COREFLAGS_RC_ALLOWED__
#	else
#		define	VCM_RC_MODE_POSSIBLE	FALSE
#	endif
#	if defined(RCMODE_VBR)
#		define  VBR_RC_MODE_POSSIBLE	__COREFLAGS_RC_ALLOWED__
#	else
#		define	VBR_RC_MODE_POSSIBLE	FALSE
#	endif
#	if defined(RCMODE_ERC)
#		define  ERC_RC_MODE_POSSIBLE	__COREFLAGS_RC_ALLOWED__
#	else
#		define	ERC_RC_MODE_POSSIBLE	FALSE
#	endif
#endif


#define RATE_CONTROL_AVAILABLE (\
		CBR_RC_MODE_POSSIBLE || VCM_RC_MODE_POSSIBLE || \
		VBR_RC_MODE_POSSIBLE || ERC_RC_MODE_POSSIBLE\
	)
#define RC_MODE_POSSIBLE(MODE) (MODE ## _RC_MODE_POSSIBLE)

#define RC_MODES_POSSIBLE2(M1, M2) \
	(RC_MODE_POSSIBLE(M1) || RC_MODE_POSSIBLE(M2))

#define RC_MODES_POSSIBLE3(M1, M2, M3) \
	(RC_MODES_POSSIBLE2(M1, M2) || RC_MODE_POSSIBLE(M3))

/*
	Declare `CUR_ENCODE_RC_MODE` as proper function only in FAKE_MTX.
	Alternatively, declare it as constant.
*/
#if defined(FAKE_MTX) || defined(RCMODE_ALL)
#	define	CUR_ENCODE_RC_MODE (GLOBALS(g_sEncContext).eRCMode)
#elif defined(RCMODE_NONE)
#	define CUR_ENCODE_RC_MODE (IMG_RCMODE_NONE)
#elif defined(RCMODE_CBR)
#	define CUR_ENCODE_RC_MODE (IMG_RCMODE_CBR)
#elif defined(RCMODE_VBR)
#	define CUR_ENCODE_RC_MODE (IMG_RCMODE_VBR)
#elif defined(RCMODE_ERC)
#	define CUR_ENCODE_RC_MODE (IMG_RCMODE_ERC)
#elif defined(RCMODE_LLRC)
#	define CUR_ENCODE_RC_MODE (IMG_RCMODE_LLRC)
#elif defined(RCMODE_VCM)
#	define CUR_ENCODE_RC_MODE (IMG_RCMODE_VCM)
#endif


#define USE_VCM_HW_SUPPORT 1

#define INPUT_SCALER_SUPPORTED 0	/* controls the firmwares ability to support the optional hardware input scaler */

#define SECURE_MODE_POSSIBLE 1		/* controls the firmwares ability to support secure mode firmware upload */

#endif  // #ifndef _COREFLAGS_H_
