/*--------------------------------------------------------------------------
Copyright (c) 2009, The Linux Foundation. All rights reserved.

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
#ifndef __OMX_QCOM_EXTENSIONS_H__
#define __OMX_QCOM_EXTENSIONS_H__

/*============================================================================
*//** @file OMX_QCOMExtns.h
  This header contains constants and type definitions that specify the
  extensions added to the OpenMAX Vendor specific APIs.

*//*========================================================================*/


//////////////////////////////////////////////////////////////////////////////
//                             Include Files
//////////////////////////////////////////////////////////////////////////////
#include "OMX_Core.h"
#include "OMX_Video.h"

/**
 * This extension is used to register mapping of a virtual
 * address to a physical address. This extension is a parameter
 * which can be set using the OMX_SetParameter macro. The data
 * pointer corresponding to this extension is
 * OMX_QCOM_MemMapEntry. This parameter is a 'write only'
 * parameter (Current value cannot be queried using
 * OMX_GetParameter macro).
 */
#define OMX_QCOM_EXTN_REGISTER_MMAP     "OMX.QCOM.index.param.register_mmap"

/**
 * This structure describes the data pointer corresponding to
 * the OMX_QCOM_MMAP_REGISTER_EXTN extension. This parameter
 * must be set only 'after' populating a port with a buffer
 * using OMX_UseBuffer, wherein the data pointer of the buffer
 * corresponds to the virtual address as specified in this
 * structure.
 */
struct OMX_QCOM_PARAM_MEMMAPENTRYTYPE
{
    OMX_U32 nSize;              /** Size of the structure in bytes */
    OMX_VERSIONTYPE nVersion;   /**< OMX specification version information */
    OMX_U32 nPortIndex;         /**< Port number the structure applies to */

    /**
     * The virtual address of memory block
     */
    OMX_U64 nVirtualAddress;

    /**
     * The physical address corresponding to the virtual address. The physical
     * address is contiguous for the entire valid range of the virtual
     * address.
     */
    OMX_U64 nPhysicalAddress;
};

#define QOMX_VIDEO_IntraRefreshRandom (OMX_VIDEO_IntraRefreshVendorStartUnused + 0)

#define OMX_QCOM_PORTDEFN_EXTN   "OMX.QCOM.index.param.portdefn"
/* Allowed APIs on the above Index: OMX_GetParameter() and OMX_SetParameter() */

typedef enum OMX_QCOMMemoryRegion
{
    OMX_QCOM_MemRegionInvalid,
    OMX_QCOM_MemRegionEBI1,
    OMX_QCOM_MemRegionSMI,
    OMX_QCOM_MemRegionMax = 0X7FFFFFFF
} OMX_QCOMMemoryRegion;

typedef enum OMX_QCOMCacheAttr
{
    OMX_QCOM_CacheAttrNone,
    OMX_QCOM_CacheAttrWriteBack,
    OMX_QCOM_CacheAttrWriteThrough,
    OMX_QCOM_CacheAttrMAX = 0X7FFFFFFF
} OMX_QCOMCacheAttr;

typedef struct OMX_QCOMRectangle
{
   OMX_S32 x;
   OMX_S32 y;
   OMX_S32 dx;
   OMX_S32 dy;
} OMX_QCOMRectangle;

/** OMX_QCOMFramePackingFormat
  * Input or output buffer format
  */
typedef enum OMX_QCOMFramePackingFormat
{
  /* 0 - unspecified
   */
  OMX_QCOM_FramePacking_Unspecified,

  /*  1 - Partial frames may be present OMX IL 1.1.1 Figure 2-10:
   *  Case 1??Each Buffer Filled In Whole or In Part
   */
  OMX_QCOM_FramePacking_Arbitrary,

  /*  2 - Multiple complete frames per buffer (integer number)
   *  OMX IL 1.1.1 Figure 2-11: Case 2—Each Buffer Filled with
   *  Only Complete Frames of Data
   */
  OMX_QCOM_FramePacking_CompleteFrames,

  /*  3 - Only one complete frame per buffer, no partial frame
   *  OMX IL 1.1.1 Figure 2-12: Case 3—Each Buffer Filled with
   *  Only One Frame of Compressed Data. Usually at least one
   *  complete unit of data will be delivered in a buffer for
   *  uncompressed data formats.
   */
  OMX_QCOM_FramePacking_OnlyOneCompleteFrame,

  /*  4 - Only one complete subframe per buffer, no partial subframe
   *  Example: In H264, one complete NAL per buffer, where one frame
   *  can contatin multiple NAL
   */
  OMX_QCOM_FramePacking_OnlyOneCompleteSubFrame,

  OMX_QCOM_FramePacking_MAX = 0X7FFFFFFF
} OMX_QCOMFramePackingFormat;

typedef struct OMX_QCOM_PARAM_PORTDEFINITIONTYPE {
 OMX_U32 nSize;           /** Size of the structure in bytes */
 OMX_VERSIONTYPE nVersion;/** OMX specification version information */
 OMX_U32 nPortIndex;    /** Portindex which is extended by this structure */

 /** Platform specific memory region EBI1, SMI, etc.,*/
 OMX_QCOMMemoryRegion nMemRegion;

 OMX_QCOMCacheAttr nCacheAttr; /** Cache attributes */

 /** Input or output buffer format */
 OMX_U32 nFramePackingFormat;

} OMX_QCOM_PARAM_PORTDEFINITIONTYPE;

#define OMX_QCOM_PLATFORMPVT_EXTN   "OMX.QCOM.index.param.platformprivate"
/** Allowed APIs on the above Index: OMX_SetParameter() */

typedef enum OMX_QCOM_PLATFORM_PRIVATE_ENTRY_TYPE
{
    /** Enum for PMEM information */
    OMX_QCOM_PLATFORM_PRIVATE_PMEM = 0x1
} OMX_QCOM_PLATFORM_PRIVATE_ENTRY_TYPE;

/** IL client will set the following structure. A failure
 *  code will be returned if component does not support the
 *  value provided for 'type'.
 */
struct OMX_QCOM_PLATFORMPRIVATE_EXTN
{
    OMX_U32 nSize;        /** Size of the structure in bytes */
    OMX_VERSIONTYPE nVersion; /** OMX spec version information */
    OMX_U32 nPortIndex;  /** Port number on which usebuffer extn is applied */

    /** Type of extensions should match an entry from
     OMX_QCOM_PLATFORM_PRIVATE_ENTRY_TYPE
    */
    OMX_QCOM_PLATFORM_PRIVATE_ENTRY_TYPE type;
};

typedef struct OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO
{
    /** pmem file descriptor */
    OMX_U32 pmem_fd;
    /** Offset from pmem device base address */
    OMX_U32 offset;
}OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO;

typedef struct OMX_QCOM_PLATFORM_PRIVATE_ENTRY
{
    /** Entry type */
    OMX_QCOM_PLATFORM_PRIVATE_ENTRY_TYPE type;

    /** Pointer to platform specific entry */
    void* entry;
}OMX_QCOM_PLATFORM_PRIVATE_ENTRY;

typedef struct OMX_QCOM_PLATFORM_PRIVATE_LIST
{
    /** Number of entries */
    OMX_U32 nEntries;

    /** Pointer to array of platform specific entries *
     * Contiguous block of OMX_QCOM_PLATFORM_PRIVATE_ENTRY element
    */
    OMX_QCOM_PLATFORM_PRIVATE_ENTRY* entryList;
}OMX_QCOM_PLATFORM_PRIVATE_LIST;

#define OMX_QCOM_FRAME_PACKING_FORMAT   "OMX.QCOM.index.param.framepackfmt"
/* Allowed API call: OMX_GetParameter() */
/* IL client can use this index to rerieve the list of frame formats *
 * supported by the component */

typedef struct OMX_QCOM_FRAME_PACKINGFORMAT_TYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nIndex;
    OMX_QCOMFramePackingFormat eframePackingFormat;
} OMX_QCOM_FRAME_PACKINGFORMAT_TYPE;


/**
 * Following is the enum for color formats supported on Qualcomm
 * MSMs YVU420SemiPlanar color format is not defined in OpenMAX
 * 1.1.1 and prior versions of OpenMAX specification.
 */

enum OMX_QCOM_COLOR_FORMATTYPE
{

/** YVU420SemiPlanar: YVU planar format, organized with a first
 *  plane containing Y pixels, and a second plane containing
 *  interleaved V and U pixels. V and U pixels are sub-sampled
 *  by a factor of two both horizontally and vertically.
 */
    OMX_QCOM_COLOR_FormatYVU420SemiPlanar = 0x7FA30C00,
    QOMX_COLOR_FormatYVU420PackedSemiPlanar32m4ka,
    QOMX_COLOR_FormatYUV420PackedSemiPlanar16m2ka,
    QOMX_COLOR_FormatYUV420PackedSemiPlanar64x32Tile2m8ka
};

enum OMX_QCOM_VIDEO_CODINGTYPE
{
/** Codecs support by qualcomm which are not listed in OMX 1.1.x
 *  spec
 *   */
    OMX_QCOM_VIDEO_CodingVC1  = 0x7FA30C00 ,
    OMX_QCOM_VIDEO_CodingWMV9 = 0x7FA30C01,
    QOMX_VIDEO_CodingDivx = 0x7FA30C02,     /**< Value when coding is Divx */
    QOMX_VIDEO_CodingSpark = 0x7FA30C03,     /**< Value when coding is Sorenson Spark */
    QOMX_VIDEO_CodingVp = 0x7FA30C04
};

enum OMX_QCOM_EXTN_INDEXTYPE
{
    /** Qcom proprietary extension index list */

    /* "OMX.QCOM.index.param.register_mmap" */
    OMX_QcomIndexRegmmap = 0x7F000000,

    /* "OMX.QCOM.index.param.platformprivate" */
    OMX_QcomIndexPlatformPvt = 0x7F000001,

    /* "OMX.QCOM.index.param.portdefn" */
    OMX_QcomIndexPortDefn = 0x7F000002,

    /* "OMX.QCOM.index.param.framepackingformat" */
    OMX_QcomIndexPortFramePackFmt = 0x7F000003,

    /*"OMX.QCOM.index.param.Interlaced */
    OMX_QcomIndexParamInterlaced = 0x7F000004,

    /*"OMX.QCOM.index.config.interlaceformat */
    OMX_QcomIndexConfigInterlaced = 0x7F000005,

    /*"OMX.QCOM.index.param.syntaxhdr" */
    QOMX_IndexParamVideoSyntaxHdr = 0x7F000006,

    /*"OMX.QCOM.index.config.intraperiod" */
    QOMX_IndexConfigVideoIntraperiod = 0x7F000007,

    /*"OMX.QCOM.index.config.randomIntrarefresh" */
    QOMX_IndexConfigVideoIntraRefresh = 0x7F000008,

    /*"OMX.QCOM.index.config.video.TemporalSpatialTradeOff" */
    QOMX_IndexConfigVideoTemporalSpatialTradeOff = 0x7F000009,

    /*"OMX.QCOM.index.param.video.EncoderMode" */
    QOMX_IndexParamVideoEncoderMode = 0x7F00000A,

    /*"OMX.QCOM.index.param.Divxtype */
    OMX_QcomIndexParamVideoDivx = 0x7F00000B,

    /*"OMX.QCOM.index.param.Sparktype */
    OMX_QcomIndexParamVideoSpark = 0x7F00000C,

    /*"OMX.QCOM.index.param.Vptype */
    OMX_QcomIndexParamVideoVp = 0x7F00000D,

    OMX_QcomIndexQueryNumberOfVideoDecInstance = 0x7F00000E
};

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
    QOMX_VIDEO_EncoderModeDefault        = 0x00,
    QOMX_VIDEO_EncoderModeMMS            = 0x01,
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
 * This structure describes the parameters corresponding to the
 * QOMX_VIDEO_SYNTAXHDRTYPE extension. This parameter can be queried
 * during the loaded state.
 */

typedef struct QOMX_VIDEO_SYNTAXHDRTYPE
{
   OMX_U32 nSize;           /** Size of the structure in bytes */
   OMX_VERSIONTYPE nVersion;/** OMX specification version information */
   OMX_U32 nPortIndex;      /** Portindex which is extended by this structure */
   OMX_U32 nBytes;    	    /** The number of bytes filled in to the buffer */
   OMX_U8 data[1];          /** Buffer to store the header information */
} QOMX_VIDEO_SYNTAXHDRTYPE;

/**
 * This structure describes the parameters corresponding to the
 * QOMX_VIDEO_TEMPORALSPATIALTYPE extension. This parameter can be set
 * dynamically during any state except the state invalid.  This is primarily
 * used for setting MaxQP from the application.  This is set on the out port.
 */

typedef struct QOMX_VIDEO_TEMPORALSPATIALTYPE
{
   OMX_U32 nSize;           /** Size of the structure in bytes */
   OMX_VERSIONTYPE nVersion;/** OMX specification version information */
   OMX_U32 nPortIndex;      /** Portindex which is extended by this structure */
   OMX_U32 nTSFactor;       /** Temoral spatial tradeoff factor value in 0-100 */
} QOMX_VIDEO_TEMPORALSPATIALTYPE;

/**
 * This structure describes the parameters corresponding to the
 * OMX_QCOM_VIDEO_CONFIG_INTRAPERIODTYPE extension. This parameter can be set
 * dynamically during any state except the state invalid.  This is set on the out port.
 */

typedef struct QOMX_VIDEO_INTRAPERIODTYPE
{
   OMX_U32 nSize;           /** Size of the structure in bytes */
   OMX_VERSIONTYPE nVersion;/** OMX specification version information */
   OMX_U32 nPortIndex;      /** Portindex which is extended by this structure */
   OMX_U32 nIDRPeriod;      /** This specifies coding a frame as IDR after every nPFrames
			        of intra frames. If this parameter is set to 0, only the
				first frame of the encode session is an IDR frame. This
				field is ignored for non-AVC codecs and is used only for
				codecs that support IDR Period */
   OMX_U32 nPFrames;         /** The number of "P" frames between two "I" frames */
   OMX_U32 nBFrames;         /** The number of "B" frames between two "I" frames */
} QOMX_VIDEO_INTRAPERIODTYPE;

/**
 * This structure describes the parameters corresponding to the
 * OMX_QCOM_VIDEO_CONFIG_ULBUFFEROCCUPANCYTYPE extension. This parameter can be set
 * dynamically during any state except the state invalid. This is used for the buffer negotiation
 * with other clients.  This is set on the out port.
 */
typedef struct OMX_QCOM_VIDEO_CONFIG_ULBUFFEROCCUPANCYTYPE
{
   OMX_U32 nSize;            /** Size of the structure in bytes */
   OMX_VERSIONTYPE nVersion; /** OMX specification version information */
   OMX_U32 nPortIndex;       /** Portindex which is extended by this structure */
   OMX_U32 nBufferOccupancy; /** The number of bytes to be set for the buffer occupancy */
} OMX_QCOM_VIDEO_CONFIG_ULBUFFEROCCUPANCYTYPE;

/**
 * This structure describes the parameters corresponding to the
 * OMX_QCOM_VIDEO_CONFIG_RANDOMINTRAREFRESHTYPE extension. This parameter can be set
 * dynamically during any state except the state invalid. This is primarily used for the dynamic/random
 * intrarefresh.  This is set on the out port.
 */
typedef struct OMX_QCOM_VIDEO_CONFIG_RANDOMINTRAREFRESHTYPE
{
   OMX_U32 nSize;           /** Size of the structure in bytes */
   OMX_VERSIONTYPE nVersion;/** OMX specification version information */
   OMX_U32 nPortIndex;      /** Portindex which is extended by this structure */
   OMX_U32 nRirMBs;         /** The number of MBs to be set for intrarefresh */
} OMX_QCOM_VIDEO_CONFIG_RANDOMINTRAREFRESHTYPE;


/**
 * This structure describes the parameters corresponding to the
 * OMX_QCOM_VIDEO_CONFIG_QPRANGE extension. This parameter can be set
 * dynamically during any state except the state invalid. This is primarily
 * used for the min/max QP to be set from the application.  This
 * is set on the out port.
 */
typedef struct OMX_QCOM_VIDEO_CONFIG_QPRANGE
{
   OMX_U32 nSize;           /** Size of the structure in bytes */
   OMX_VERSIONTYPE nVersion;/** OMX specification version information */
   OMX_U32 nPortIndex;      /** Portindex which is extended by this structure */
   OMX_U32 nMinQP;          /** The number for minimum quantization parameter */
   OMX_U32 nMaxQP;          /** The number for maximum quantization parameter */
} OMX_QCOM_VIDEO_CONFIG_QPRANGE;


typedef struct OMX_VENDOR_EXTRADATATYPE  {
    OMX_U32 nPortIndex;
    OMX_U32 nDataSize;
    OMX_U8  *pData;     // cdata (codec_data/extradata)
} OMX_VENDOR_EXTRADATATYPE;

typedef enum OMX_INDEXVENDORTYPE {
    OMX_IndexVendorFileReadInputFilename = 0xFF000001,
    OMX_IndexVendorParser3gpInputFilename = 0xFF000002,
    OMX_IndexVendorVideoExtraData = 0xFF000003,
    OMX_IndexVendorAudioExtraData = 0xFF000004
} OMX_INDEXVENDORTYPE;

typedef enum OMX_QCOM_VC1RESOLUTIONTYPE
{
   OMX_QCOM_VC1_PICTURE_RES_1x1,
   OMX_QCOM_VC1_PICTURE_RES_2x1,
   OMX_QCOM_VC1_PICTURE_RES_1x2,
   OMX_QCOM_VC1_PICTURE_RES_2x2
} OMX_QCOM_VC1RESOLUTIONTYPE;

typedef enum OMX_QCOM_INTERLACETYPE
{
    OMX_QCOM_InterlaceFrameProgressive,
    OMX_QCOM_InterlaceInterleaveFrameTopFieldFirst,
    OMX_QCOM_InterlaceInterleaveFrameBottomFieldFirst,
    OMX_QCOM_InterlaceFrameTopFieldFirst,
    OMX_QCOM_InterlaceFrameBottomFieldFirst,
    OMX_QCOM_InterlaceFieldTop,
    OMX_QCOM_InterlaceFieldBottom
}OMX_QCOM_INTERLACETYPE;

typedef struct OMX_QCOM_PARAM_VIDEO_INTERLACETYPE
{
    OMX_U32 nSize;           /** Size of the structure in bytes */
    OMX_VERSIONTYPE nVersion;/** OMX specification version information */
    OMX_U32 nPortIndex;    /** Portindex which is extended by this structure */
    OMX_BOOL bInterlace;  /** Interlace content **/
}OMX_QCOM_PARAM_VIDEO_INTERLACETYPE;

typedef struct OMX_QCOM_CONFIG_INTERLACETYPE
{
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nIndex;
    OMX_QCOM_INTERLACETYPE eInterlaceType;
}OMX_QCOM_CONFIG_INTERLACETYPE;

#define MAX_PAN_SCAN_WINDOWS 4

typedef struct OMX_QCOM_PANSCAN
{
   OMX_U32 numWindows;
   OMX_QCOMRectangle window[MAX_PAN_SCAN_WINDOWS];
} OMX_QCOM_PANSCAN;

typedef struct OMX_QCOM_EXTRADATA_FRAMEINFO
{
   // common frame meta data. interlace related info removed
   OMX_VIDEO_PICTURETYPE  ePicType;
   OMX_QCOM_INTERLACETYPE interlaceType;
   OMX_QCOM_PANSCAN       panScan;
   OMX_U32                nConcealedMacroblocks;
} OMX_QCOM_EXTRADATA_FRAMEINFO;

typedef struct OMX_QCOM_EXTRADATA_FRAMEDIMENSION
{
   /** Frame Dimensions added to each YUV buffer */
   OMX_U32   nDecWidth;  /** Width  rounded to multiple of 16 */
   OMX_U32   nDecHeight; /** Height rounded to multiple of 16 */
   OMX_U32   nActualWidth; /** Actual Frame Width */
   OMX_U32   nActualHeight; /** Actual Frame Height */

}OMX_QCOM_EXTRADATA_FRAMEDIMENSION;

typedef struct OMX_QCOM_H264EXTRADATA
{
   OMX_U64 seiTimeStamp;
} OMX_QCOM_H264EXTRADATA;

typedef struct OMX_QCOM_VC1EXTRADATA
{
   OMX_U32                     nVC1RangeY;
   OMX_U32                     nVC1RangeUV;
   OMX_QCOM_VC1RESOLUTIONTYPE eVC1PicResolution;
} OMX_QCOM_VC1EXTRADATA;

typedef union OMX_QCOM_EXTRADATA_CODEC_DATA
{
   OMX_QCOM_H264EXTRADATA h264ExtraData;
   OMX_QCOM_VC1EXTRADATA vc1ExtraData;
} OMX_QCOM_EXTRADATA_CODEC_DATA;

typedef enum OMX_QCOM_EXTRADATATYPE
{
   OMX_ExtraDataFrameInfo = 0x7F000001,
   OMX_ExtraDataH264 = 0x7F000002,
   OMX_ExtraDataVC1 = 0x7F000003,
   OMX_ExtraDataFrameDimension = 0x7F000004,
   OMX_ExtraDataVideoEncoderSliceInfo = 0x7F000005
} OMX_QCOM_EXTRADATATYPE;


#define OMX_EXTRADATA_HEADER_SIZE 20

/**
 * DivX Versions
 */
typedef enum  QOMX_VIDEO_DIVXFORMATTYPE {
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


typedef struct QOMX_VIDEO_QUERY_DECODER_INSTANCES {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nNumOfInstances;
} QOMX_VIDEO_QUERY_DECODER_INSTANCES;



#endif /* __OMX_QCOM_EXTENSIONS_H__ */
