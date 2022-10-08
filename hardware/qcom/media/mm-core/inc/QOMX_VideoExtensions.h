/*--------------------------------------------------------------------------
Copyright (c) 2011 The Linux Foundation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of The Linux Foundation nor
      the names of its contributors may be used to endorse or promote
      products derived from this software without specific prior written
      permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
--------------------------------------------------------------------------*/

#ifndef __H_QOMX_VIDEOEXTENSIONS_H__
#define __H_QOMX_VIDEOEXTENSIONS_H__ 

/*========================================================================

*//** @file QOMX_VideoExtensions.h 

@par FILE SERVICES:
      Qualcomm extensions API for OpenMax IL Video.

      This file contains the description of the Qualcomm OpenMax IL
      video extention interface, through which the IL client and OpenMax 
      components can access additional video capabilities.
   
*//*====================================================================== */


/*========================================================================== */
 
/*======================================================================== 
 
                     INCLUDE FILES FOR MODULE 
 
========================================================================== */ 
#include <OMX_Core.h>
#include <OMX_Video.h>

/*========================================================================

                      DEFINITIONS AND DECLARATIONS

========================================================================== */

#if defined( __cplusplus )
extern "C"
{
#endif /* end of macro __cplusplus */

/* Video extension strings */ 
#define OMX_QCOM_INDEX_PARAM_VIDEO_SYNTAXHDR                "OMX.QCOM.index.param.video.SyntaxHdr"
#define OMX_QCOM_INDEX_PARAM_VIDEO_ENCODERMODE              "OMX.QCOM.index.param.video.EncoderMode"
#define OMX_QCOM_INDEX_CONFIG_VIDEO_INTRAREFRESH            "OMX.QCOM.index.config.video.IntraRefresh"
#define OMX_QCOM_INDEX_CONFIG_VIDEO_INTRAPERIOD             "OMX.QCOM.index.config.video.IntraPeriod"
#define OMX_QCOM_INDEX_CONFIG_VIDEO_TEMPORALSPATIALTRADEOFF "OMX.QCOM.index.config.video.TemporalSpatialTradeOff"
#define OMX_QCOM_INDEX_CONFIG_VIDEO_MBCONCEALMENTREPORTING  "OMX.QCOM.index.config.video.MBConcealmentReporting"
#define OMX_QCOM_INDEX_PARAM_VIDEO_EXTRADATAMULTISLICEINFO  "OMX.QCOM.index.param.video.ExtraDataMultiSliceInfo" /**< reference: QOMX_ENABLETYPE */
#define OMX_QCOM_INDEX_CONFIG_VIDEO_FLOWSTATUS              "OMX.QCOM.index.config.video.FlowStatus"             /**< reference: QOMX_FLOWSTATUSTYPE */    
#define OMX_QCOM_INDEX_PARAM_VIDEO_PICTURETYPEDECODE        "OMX.QCOM.index.param.video.PictureTypeDecode"       /**< reference: QOMX_VIDEO_DECODEPICTURETYPE */
#define OMX_QCOM_INDEX_PARAM_VIDEO_SAMPLEASPECTRATIO        "OMX.QCOM.index.param.video.SampleAspectRatio"       /**< reference: QOMX_VIDEO_SAMPLEASPECTRATIO */
#define OMX_QCOM_INDEX_PARAM_VIDEO_EXTRADATALTRINFO         "OMX.QCOM.index.param.video.ExtraDataLTRInfo"        /**< reference: QOMX_ENABLETYPE */

/* Video coding types */
#define OMX_QCOM_INDEX_PARAM_VIDEO_DIVX                     "OMX.QCOM.index.param.video.DivX"
#define OMX_QCOM_INDEX_PARAM_VIDEO_VP                       "OMX.QCOM.index.param.video.VP"
#define OMX_QCOM_INDEX_PARAM_VIDEO_SPARK                    "OMX.QCOM.index.param.video.Spark"
#define OMX_QCOM_INDEX_PARAM_VIDEO_VC1                      "OMX.QCOM.index.param.video.VC1"

/**
 * Enumeration used to define the extended video compression
 * codings, not present in the OpenMax IL 1.1.2 specification.
 * NOTE:  This essentially refers to file extensions. If the
 *        coding is being used to specify the ENCODE type, then
 *        additional work must be done to configure the exact
 *        flavor of the compression to be used.
 */
typedef enum QOMX_VIDEO_CODINGTYPE 
{
    QOMX_VIDEO_CodingDivX   = 0x7F000001, /**< all versions of DivX */
    QOMX_VIDEO_CodingVP     = 0x7F000002, /**< all versions of On2 VP codec */
    QOMX_VIDEO_CodingSpark  = 0x7F000003, /**< Sorenson Spark */
    QOMX_VIDEO_CodingVC1    = 0x7F000004, /**< VC-1 */
    QOMX_VIDEO_MPEG1        = 0x7F000005  /**< MPEG-1 */
} QOMX_VIDEO_CODINGTYPE;

/** 
 * DivX Versions
 */
typedef enum QOMX_VIDEO_DIVXFORMATTYPE {
    QOMX_VIDEO_DIVXFormatUnused = 0x01, /**< Format unused or unknown */
    QOMX_VIDEO_DIVXFormat311    = 0x02, /**< DivX 3.11 */
    QOMX_VIDEO_DIVXFormat4      = 0x04, /**< DivX 4 */
    QOMX_VIDEO_DIVXFormat5      = 0x08, /**< DivX 5 */
    QOMX_VIDEO_DIVXFormat6      = 0x10, /**< DivX 6 */
    QOMX_VIDEO_DIVXFormatKhronosExtensions = 0x6F000000, 
    QOMX_VIDEO_DIVXFormatVendorStartUnused = 0x7F000000,
    QOMX_VIDEO_DIVXFormatMax = 0x7FFFFFFF
} QOMX_VIDEO_DIVXFORMATTYPE;

/** 
 * DivX profile types, each profile indicates support for
 * various performance bounds.
 */
typedef enum QOMX_VIDEO_DIVXPROFILETYPE {
    QOMX_VIDEO_DivXProfileqMobile = 0x01, /**< qMobile Profile */
    QOMX_VIDEO_DivXProfileMobile  = 0x02, /**< Mobile Profile */
    QOMX_VIDEO_DivXProfileMT      = 0x04, /**< Mobile Theatre Profile */
    QOMX_VIDEO_DivXProfileHT      = 0x08, /**< Home Theatre Profile */
    QOMX_VIDEO_DivXProfileHD      = 0x10, /**< High Definition Profile */
    QOMX_VIDEO_DIVXProfileKhronosExtensions = 0x6F000000, 
    QOMX_VIDEO_DIVXProfileVendorStartUnused = 0x7F000000,
    QOMX_VIDEO_DIVXProfileMax = 0x7FFFFFFF
} QOMX_VIDEO_DIVXPROFILETYPE;

/** 
 * DivX Video Params
 * 
 *  STRUCT MEMBERS:
 *  nSize      : Size of the structure in bytes
 *  nVersion   : OMX specification version information
 *  nPortIndex : Port that this structure applies to
 *  eFormat    : Version of DivX stream / data
 *  eProfile   : Profile of DivX stream / data
 */
typedef struct QOMX_VIDEO_PARAM_DIVXTYPE {
    OMX_U32 nSize; 
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    QOMX_VIDEO_DIVXFORMATTYPE eFormat;
    QOMX_VIDEO_DIVXPROFILETYPE eProfile;
} QOMX_VIDEO_PARAM_DIVXTYPE;

/** 
 * VP Versions
 */
typedef enum QOMX_VIDEO_VPFORMATTYPE {
    QOMX_VIDEO_VPFormatUnused = 0x01, /**< Format unused or unknown */
    QOMX_VIDEO_VPFormat6      = 0x02, /**< VP6 Video Format */
    QOMX_VIDEO_VPFormat7      = 0x04, /**< VP7 Video Format */
    QOMX_VIDEO_VPFormat8      = 0x08, /**< VP8 Video Format */
    QOMX_VIDEO_VPFormatKhronosExtensions = 0x6F000000, 
    QOMX_VIDEO_VPFormatVendorStartUnused = 0x7F000000,
    QOMX_VIDEO_VPFormatMax = 0x7FFFFFFF
} QOMX_VIDEO_VPFORMATTYPE;

/** 
 * VP profile types, each profile indicates support for various
 * encoding tools.
 */
typedef enum QOMX_VIDEO_VPPROFILETYPE {
    QOMX_VIDEO_VPProfileSimple   = 0x01, /**< Simple Profile, applies to VP6 only */
    QOMX_VIDEO_VPProfileAdvanced = 0x02, /**< Advanced Profile, applies to VP6 only */
    QOMX_VIDEO_VPProfileVersion0 = 0x04, /**< Version 0, applies to VP7 and VP8 */
    QOMX_VIDEO_VPProfileVersion1 = 0x08, /**< Version 1, applies to VP7 and VP8 */
    QOMX_VIDEO_VPProfileVersion2 = 0x10, /**< Version 2, applies to VP8 only */
    QOMX_VIDEO_VPProfileVersion3 = 0x20, /**< Version 3, applies to VP8 only */
    QOMX_VIDEO_VPProfileKhronosExtensions = 0x6F000000, 
    QOMX_VIDEO_VPProfileVendorStartUnused = 0x7F000000,
    QOMX_VIDEO_VPProfileMax = 0x7FFFFFFF
} QOMX_VIDEO_VPPROFILETYPE;

/** 
 * VP Video Params
 * 
 *  STRUCT MEMBERS:
 *  nSize      : Size of the structure in bytes
 *  nVersion   : OMX specification version information
 *  nPortIndex : Port that this structure applies to
 *  eFormat    : Format of VP stream / data
 *  eProfile   : Profile or Version of VP stream / data
 */ 
typedef struct QOMX_VIDEO_PARAM_VPTYPE {
    OMX_U32 nSize; 
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    QOMX_VIDEO_VPFORMATTYPE eFormat;
    QOMX_VIDEO_VPPROFILETYPE eProfile;
} QOMX_VIDEO_PARAM_VPTYPE;

/** 
 * Spark Versions
 */
typedef enum QOMX_VIDEO_SPARKFORMATTYPE {
    QOMX_VIDEO_SparkFormatUnused = 0x01, /**< Format unused or unknown */
    QOMX_VIDEO_SparkFormat0      = 0x02, /**< Video Format Version 0 */
    QOMX_VIDEO_SparkFormat1      = 0x04, /**< Video Format Version 1 */
    QOMX_VIDEO_SparkFormatKhronosExtensions = 0x6F000000, 
    QOMX_VIDEO_SparkFormatVendorStartUnused = 0x7F000000,
    QOMX_VIDEO_SparkFormatMax = 0x7FFFFFFF
} QOMX_VIDEO_SPARKFORMATTYPE;

/** 
 * Spark Video Params
 * 
 *  STRUCT MEMBERS:
 *  nSize      : Size of the structure in bytes
 *  nVersion   : OMX specification version information
 *  nPortIndex : Port that this structure applies to
 *  eFormat    : Version of Spark stream / data
 */
typedef struct QOMX_VIDEO_PARAM_SPARKTYPE {
    OMX_U32 nSize; 
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    QOMX_VIDEO_SPARKFORMATTYPE eFormat;
} QOMX_VIDEO_PARAM_SPARKTYPE;

/** 
 * VC-1 profile types, each profile indicates support for
 * various encoding tools.
 */
typedef enum QOMX_VIDEO_VC1PROFILETYPE {
    QOMX_VIDEO_VC1ProfileSimple   = 0x01, /**< Simple Profile */
    QOMX_VIDEO_VC1ProfileMain     = 0x02, /**< Main Profile */
    QOMX_VIDEO_VC1ProfileAdvanced = 0x04, /**< Advanced Profile */
    QOMX_VIDEO_VC1ProfileKhronosExtensions = 0x6F000000, 
    QOMX_VIDEO_VC1ProfileVendorStartUnused = 0x7F000000,
    QOMX_VIDEO_VC1ProfileMax = 0x7FFFFFFF
} QOMX_VIDEO_VC1PROFILETYPE;

/** 
 * VC-1 level types, each level indicates support for various
 * performance bounds.
 */
typedef enum QOMX_VIDEO_VC1LEVELTYPE {
    QOMX_VIDEO_VC1LevelLow    = 0x01, /**< Low Level, applies to simple and main profiles*/
    QOMX_VIDEO_VC1LevelMedium = 0x02, /**< Medium Level, applies to simple and main profiles */
    QOMX_VIDEO_VC1LevelHigh   = 0x04, /**< High Level, applies to main profile only */
    QOMX_VIDEO_VC1Level0      = 0x08, /**< Level 0, applies to advanced profile only */
    QOMX_VIDEO_VC1Level1      = 0x10, /**< Level 1, applies to advanced profile only */
    QOMX_VIDEO_VC1Level2      = 0x20, /**< Level 2, applies to advanced profile only */
    QOMX_VIDEO_VC1Level3      = 0x40, /**< Level 3, applies to advanced profile only */
    QOMX_VIDEO_VC1Level4      = 0x80, /**< Level 4, applies to advanced profile only */
    QOMX_VIDEO_VC1LevelKhronosExtensions = 0x6F000000, 
    QOMX_VIDEO_VC1LevelVendorStartUnused = 0x7F000000,
    QOMX_VIDEO_VC1LevelMax = 0x7FFFFFFF
} QOMX_VIDEO_VC1LEVELTYPE;

/** 
 * VC-1 Video Params
 * 
 *  STRUCT MEMBERS:
 *  nSize      : Size of the structure in bytes
 *  nVersion   : OMX specification version information
 *  nPortIndex : Port that this structure applies to
 *  eProfile   : Profile of VC-1 stream / data
 *  eLevel     : Level of VC-1 stream / data
 */
typedef struct QOMX_VIDEO_PARAM_VC1TYPE {
    OMX_U32 nSize; 
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    QOMX_VIDEO_VC1PROFILETYPE eProfile;
    QOMX_VIDEO_VC1LEVELTYPE eLevel;
} QOMX_VIDEO_PARAM_VC1TYPE;

/** 
 * Extended MPEG-4 level types not defined in the OpenMax IL
 * 1.1.2 specification, each level indicates support for various
 * frame sizes, bit rates, decoder frame rates.
 */
typedef enum QOMX_VIDEO_MPEG4LEVELTYPE {
    QOMX_VIDEO_MPEG4Level6 = 0x7F000001, /**< Level 6 */  
    QOMX_VIDEO_MPEG4Level7 = 0x7F000002, /**< Level 7 */  
    QOMX_VIDEO_MPEG4Level8 = 0x7F000003, /**< Level 8 */  
    QOMX_VIDEO_MPEG4Level9 = 0x7F000004, /**< Level 9 */  
    QOMX_VIDEO_MPEG4LevelMax = 0x7FFFFFFF 
} QOMX_VIDEO_MPEG4LEVELTYPE;

/**
 * This structure is used in retrieving the syntax header from a
 * video encoder component, or setting the out of band syntax
 * header configuration data on a video decoder component.
 * 
 * STRUCT MEMBERS:
 *  nSize      : Size of the structure in bytes
 *  nVersion   : OMX specification version info
 *  nPortIndex : Port that this structure applies to
 *  nBytes     : When used with OMX_GetParameter for the encoder
 *               component, it is a read-write field. When
 *               QOMX_VIDEO_SYNTAXHDRTYPE is passed in
 *               OMX_GetParameter this is the size of the buffer
 *               array pointed by data field. When the
 *               OMX_GetParameter call returns this is the
 *               amount of data within the buffer array.
 *
 *               The IL client needs to allocate the buffer
 *               array and then request for the syntax header.
 *               If the size of buffer array to allocate is
 *               unknown to the IL client, then it can call
 *               OMX_GetParamter with nBytes set to 0. In this
 *               case, when OMX_GetParameter returns, the nBytes
 *               field will be set to the size of the syntax
 *               header. IL Client can then allocate a buffer of
 *               this size and call OMX_GetParamter again.
 * 
 *               When used with OMX_SetParameter for the decoder
 *               component, it is a read-only field specifying
 *               the amount of data in the buffer array.
 *  data       : The syntax header data. The format of the
 *               syntax header is specific to the video codec,
 *               and is described below.
 *
 *   H.263      : N/A
 *   H.264      : The SPS and PPS parameter sets
 *   MPEG-4     : The VO, VOS, and VOL header
 *   WMV7       : The "Extra Data" info, in the ASF Stream
 *                Properties Object.
 *   WMV8       : The "Extra Data" info, in the ASF Stream
 *                Properties Object.
 *   WMV9 SP/MP : The STRUCT_C portion of the sequence layer
 *                meta data, defined in Table 263 of the VC-1
 *                specification.
 *   VC-1 SP/MP : The STRUCT_C portion of the sequence layer
 *                meta data, defined in Table 263 of the VC-1
 *                specification.
 *   VC-1 AP    : The sequence and entry point header
 *   DivX 3     : N/A
 *   DivX 4.x   : The VO, VOS, and VOL header
 *   DivX 5.x   : The VO, VOS, and VOL header
 *   DivX 6.x   : The VO, VOS, and VOL header
 *   VP6        : N/A
 *   Spark      : N/A
 */
typedef struct QOMX_VIDEO_SYNTAXHDRTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nBytes;
    OMX_U8  data[1];
} QOMX_VIDEO_SYNTAXHDRTYPE;


/** 
 * Enumeration used to define the extended video intra refresh types, not 
 * present in the OpenMax IL 1.1.2 specification. 
 *
 * ENUMS:
 *  IntraRefreshRandom         : Random intra refresh mode.
 */
typedef enum QOMX_VIDEO_INTRAREFRESHTYPE
{
    QOMX_VIDEO_IntraRefreshRandom      = 0x7F100000
} QOMX_VIDEO_INTRAREFRESHTYPE;


/**
 * This structure is used to configure the intra periodicity for encoder.
 * 
 * STRUCT MEMBERS:
 *  nSize      : Size of the structure in bytes
 *  nVersion   : OMX specification version info
 *  nPortIndex : Port that this structure applies to
 *  nIDRPeriod : Defines the periodicity of IDR occurrence. This specifies
 *               coding a frame as IDR after a specific number of intra
 *               frames. The periodicity of intra frame coding is specified by
 *               the nPFrames.  If nIDRPeriod is set to 0, only the first
 *               frame of the encode session is an IDR frame. This field is
 *               ignored for non-AVC codecs and is used only for codecs that
 *               support IDR Period.
 *  nPFrames : Specifies the number of P frames between each I Frame.
 *  nBFrames : Specifies the number of B frames between each I Frame.
 */
typedef struct QOMX_VIDEO_INTRAPERIODTYPE  {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nIDRPeriod;
    OMX_U32 nPFrames;
    OMX_U32 nBFrames;
} QOMX_VIDEO_INTRAPERIODTYPE;


/**
 * Enumeration used to define the extended video extra data payload types not 
 * present in the OpenMax IL 1.1.2 specification. 
 *
 * ENUMS:
 *  VideoMultiSliceInfo : Multi slice layout information
 *  
 *  Slice information layout:
 *  First 4 bytes = Number of Slice Entries
 *
 *  Then individual slice entries: 8 bytes per entry.
 *  Slice1 information: offset (4 bytes), Length (4 bytes)
 *  Slice2 information: offset (4 bytes), Length (4 bytes)
 *  Slice3 information: offset (4 bytes), Length (4 bytes)
 *  ...................................
 *  ...................................
 *  SliceN information: offset (4 bytes), Length (4 bytes)
 * 
 * 
 *  VideoNumConcealedMB : Number of concealed MBs
 * 
 *  The data array consists of an unsigned 32-bit size field
 *  indicating the number of concealed macroblocks in the
 *  uncompressed frame.
 *
 *
 *  QOMX_ExtraDataOMXIndex : Indicates that the data payload contains an 
 *  OpenMax index and associated payload.
 *
 *  The data of the extra data payload shall contain the value of the 
 *  OMX_INDEXTYPE corresponding to the requested operation as an unsigned 
 *  32 bit number occupying the first four bytes of the payload. The index 
 *  will be immediately followed by the associated structure. Padding bytes
 *  are appended to ensure 32 bit address alignment if needed. 
 */
typedef enum QOMX_VIDEO_EXTRADATATYPE
{
   QOMX_ExtraDataVideoMultiSliceInfo = 0x7F100000,
   QOMX_ExtraDataVideoNumConcealedMB,
   QOMX_ExtraDataOMXIndex,
   QOMX_ExtraDataHDCPEncryptionInfo
} QOMX_VIDEO_EXTRADATATYPE;


/** 
 * Enumeration used to define the video encoder modes 
 *
 * ENUMS:
 *  EncoderModeDefault : Default video recording mode.
 *                       All encoder settings made through
 *                       OMX_SetParameter/OMX_SetConfig are applied. No
 *                       parameter is overridden.
 *  EncoderModeMMS : Video recording mode for MMS (Multimedia Messaging
 *                   Service). This mode is similar to EncoderModeDefault
 *                   except that here the Rate control mode is overridden
 *                   internally and set as a variant of variable bitrate with
 *                   variable frame rate. After this mode is set if the IL
 *                   client tries to set OMX_VIDEO_CONTROLRATETYPE via
 *                   OMX_IndexParamVideoBitrate that would be rejected. For
 *                   this, client should set mode back to EncoderModeDefault
 *                   first and then change OMX_VIDEO_CONTROLRATETYPE.
 */
typedef enum QOMX_VIDEO_ENCODERMODETYPE
{
    QOMX_VIDEO_EncoderModeDefault        = 0x01,
    QOMX_VIDEO_EncoderModeMMS            = 0x02,
    QOMX_VIDEO_EncoderModeMax            = 0x7FFFFFFF
} QOMX_VIDEO_ENCODERMODETYPE;

/**
 * This structure is used to set the video encoder mode. 
 *  
 * STRUCT MEMBERS:
 *  nSize      : Size of the structure in bytes
 *  nVersion   : OMX specification version info
 *  nPortIndex : Port that this structure applies to
 *  nMode : defines the video encoder mode
 */
typedef struct QOMX_VIDEO_PARAM_ENCODERMODETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    QOMX_VIDEO_ENCODERMODETYPE nMode;
} QOMX_VIDEO_PARAM_ENCODERMODETYPE;


/**
 * This structure is used to set the temporal (picture rate) - spatial 
 * (picture quality) trade-off factor. 
 * This setting is only valid when rate control is enabled and set to a mode 
 * with variable frame rate. For all other rate control modes this setting is
 * ignored. 
 *  
 * STRUCT MEMBERS:
 *  nSize      : Size of the structure in bytes
 *  nVersion   : OMX specification version info
 *  nPortIndex : Port that this structure applies to
 *  nTSFactor : temporal-spatial tradeoff factor value in the range of 0-100.
 *              A factor of 0 won't emphasizes picture rate in rate
 *  control decisions at all i.e only picture quality is emphasized. For
 *  increasing values from 1 to 99 the emphasis of picture rate in rate
 *  control decisions increases. A factor of 100 emphasizes only picture rate
 *  in rate control decisions.
 */
typedef struct QOMX_VIDEO_TEMPORALSPATIALTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nTSFactor;
} QOMX_VIDEO_TEMPORALSPATIALTYPE;

/**
 * This structure is used to enable or disable the MB concealmenet reporting 
 * for the uncompressed frames emitted from the port.
 * 
 * STRUCT MEMBERS:
 *  nSize      : Size of the structure in bytes
 *  nVersion   : OMX specification version info
 *  nPortIndex : Port that this structure applies to
 *  bEnableMBConcealmentReporting : Flag indicating whether MB concealment
 *               reporting is enabled or disabled.
 *               OMX_TRUE: Enables MB concealment reporting
 *               OMX_FALSE: Disables MB concealment reporting
 */
typedef struct QOMX_VIDEO_MBCONCEALMENTREPORTINGTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bEnableMBConcealmentReporting;
} QOMX_VIDEO_MBCONCEALMENTREPORTINGTYPE;

/** 
 * Specifies the extended picture types. These values should be
 * OR'd along with the types defined in OMX_VIDEO_PICTURETYPE to
 * signal all pictures types which are allowed.
 *
 * ENUMS:
 *  H.264 Specific Picture Types:   IDR
 */
typedef enum QOMX_VIDEO_PICTURETYPE {
    QOMX_VIDEO_PictureTypeIDR = OMX_VIDEO_PictureTypeVendorStartUnused + 0x1000
} QOMX_VIDEO_PICTURETYPE;

/**
 * This structure is used to configure the processing of
 * specific picture types.
 * 
 * STRUCT MEMBERS:
 *  nSize         : Size of the structure in bytes
 *  nVersion      : OMX specification version info
 *  nPortIndex    : Port that this structure applies to
 *  nPictureTypes : Specifies the picture type(s)
 *                  that shall be processed. The value consists
 *                  of the desired picture types, defined by the
 *                  OMX_VIDEO_PICTURETYPE and
 *                  QOMX_VIDEO_PICTURETYPE enumerations, OR'd to
 *                  signal all the pictures types which are
 *                  allowed.
 */
typedef struct QOMX_VIDEO_DECODEPICTURETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nPictureTypes;
} QOMX_VIDEO_DECODEPICTURETYPE;

/**
 * This structure describes the sample aspect ratio information.
 * 
 * STRUCT MEMBERS:
 *  nSize        : Size of the structure in bytes
 *  nVersion     : OMX specification version info
 *  nPortIndex   : Port that this structure applies to
 *  nWidth       : Specifies the horizontal aspect size of
 *                 the sample
 *  nHeight      : Specifies the vertical aspect size of the
 *                 sample
 */
typedef struct QOMX_VIDEO_SAMPLEASPECTRATIO {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U16 nWidth;
    OMX_U16 nHeight;
} QOMX_VIDEO_SAMPLEASPECTRATIO;

#if defined( __cplusplus )
}
#endif /* end of macro __cplusplus */

#endif /* end of macro __H_QOMX_VIDEOEXTENSIONS_H__ */
