/*--------------------------------------------------------------------------
Copyright (c) 2011-2013, The Linux Foundation. All rights reserved.

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

#ifndef __H_QOMX_IVCOMMONEXTENSIONS_H__
#define __H_QOMX_IVCOMMONEXTENSIONS_H__ 

/*========================================================================

*//** @file QOMX_CommonExtensions.h 

@par FILE SERVICES:
      common extensions API for OpenMax IL.

      This file contains the description of the Qualcomm OpenMax IL
      common extention interface, through which the IL client and OpenMax 
      components can access additional capabilities.

*//*====================================================================== */

 
/*======================================================================== 
 
                     INCLUDE FILES FOR MODULE 
 
========================================================================== */ 
#include <OMX_Core.h>

/*========================================================================

                      DEFINITIONS AND DECLARATIONS

========================================================================== */

#if defined( __cplusplus )
extern "C"
{
#endif /* end of macro __cplusplus */

/* IV common extension strings */
#define OMX_QCOM_INDEX_CONFIG_MEDIAINFO                 "OMX.QCOM.index.config.mediainfo"  /**< reference: QOMX_MEDIAINFOTYPE */
#define OMX_QCOM_INDEX_CONFIG_CONTENTURI                "OMX.QCOM.index.config.contenturi" /**< reference: OMX_PARAM_CONTENTURITYPE */
#define OMX_QCOM_INDEX_PARAM_IMAGESIZECONTROL           "OMX.Qualcomm.index.param.ImageSizeControl" /**< reference: QOMX_IMAGE_IMAGESIZECONTROLTYPE */
#define OMX_QCOM_INDEX_CONFIG_PAUSEPORT                 "OMX.QCOM.index.config.PausePort" /**< reference: QOMX_CONFIG_PAUSEPORTTYPE */

/** reference: QOMX_URANGETYPE 
 *  nMin, nMax, nStepSize give width in pixels */
#define OMX_QCOM_INDEX_PARAM_FRAMEWIDTHRANGESUPPORTED   "OMX.QCOM.index.param.FrameWidthRangeSupported" 

/** reference: QOMX_URANGETYPE 
 *  nMin, nMax, nStepSize give height in pixels */
#define OMX_QCOM_INDEX_PARAM_FRAMEHEIGHTRANGESUPPORTED  "OMX.QCOM.index.param.FrameHeightRangeSupported" 

/** reference: QOMX_URANGETYPE 
 *  nMin, nMax, nStepSize give the number of macroblocks per
 *  frame. */
#define OMX_QCOM_INDEX_PARAM_MACROBLOCKSPERFRAMERANGESUPPORTED "OMX.QCOM.index.param.MacroblocksPerFrameRangeSupported" 

/** reference: QOMX_URANGETYPE 
 *  nMin, nMax, nStepSize give the number of macroblocks per 
 *  second. */
#define OMX_QCOM_INDEX_PARAM_MACROBLOCKSPERSECONDRANGESUPPORTED "OMX.QCOM.index.param.MacroblocksPerSecondRangeSupported" 

/** reference: QOMX_URANGETYPE 
 *  nMin, nMax, nStepSize give frame rate in frames per second
 *  in Q16 format. */
#define OMX_QCOM_INDEX_PARAM_FRAMERATERANGESUPPORTED    "OMX.QCOM.index.param.FrameRateRangeSupported" 

#define OMX_QCOM_INDEX_PARAM_PLANEDEFINITION            "OMX.QCOM.index.param.PlaneDefinition" /** reference: QOMX_PLANEDEFINITIONTYPE */

/** reference: QOMX_URANGETYPE 
 *  nMin, nMax, nStepSize give the crop width in pixels */
#define OMX_QOMX_INDEX_PARAM_CROPWIDTHRANGESUPPORTED        "OMX.QCOM.index.param.CropWidthRangeSupported"

/** reference: QOMX_URANGETYPE 
 *  nMin, nMax, nStepSize give the crop height in pixels */
#define OMX_QOMX_INDEX_PARAM_CROPHEIGHTRANGESUPPORTED        "OMX.QCOM.index.param.CropHeightRangeSupported"

/** reference: QOMX_URANGETYPE 
 *  nMin, nMax, nStepSize give the digital zoom factor on width
 *  in Q16 format. */
#define OMX_QCOM_INDEX_PARAM_DIGITALZOOMWIDTHRANGESUPPORTED    "OMX.QCOM.index.param.DigitalZoomWidthRangeSupported"

/** reference: QOMX_URANGETYPE 
 *  nMin, nMax, nStepSize give the digital zoom factor on height
 *  in Q16 format. */
#define OMX_QCOM_INDEX_PARAM_DIGITALZOOMHEIGHTRANGESUPPORTED    "OMX.QCOM.index.param.DigitalZoomHeightRangeSupported"

    // new externsions for vidpp
#define OMX_QCOM_INDEX_CONFIG_ACTIVE_REGION_DETECTION           "OMX.QCOM.index.config.activeregiondetection"
#define OMX_QCOM_INDEX_CONFIG_ACTIVE_REGION_DETECTION_STATUS    "OMX.QCOM.index.config.activeregiondetectionstatus"
#define OMX_QCOM_INDEX_CONFIG_SCALING_MODE                      "OMX.QCOM.index.config.scalingmode"
#define OMX_QCOM_INDEX_CONFIG_NOISEREDUCTION                    "OMX.QCOM.index.config.noisereduction"
#define OMX_QCOM_INDEX_CONFIG_IMAGEENHANCEMENT                  "OMX.QCOM.index.config.imageenhancement"
/**
 * Enumeration defining the extended uncompressed image/video
 * formats.
 *
 * ENUMS:
 *  YVU420PackedSemiPlanar       : Buffer containing all Y, and then V and U 
 *                                 interleaved.
 *  YVU420PackedSemiPlanar32m4ka : YUV planar format, similar to the 
 *                                 YVU420PackedSemiPlanar format, but with the
 *                                 following restrictions:
 *
 *                                 1. The width and height of both plane must 
 *                                 be a multiple of 32 texels.
 *
 *                                 2. The base address of both planes must be 
 *                                 aligned to a 4kB boundary.
 * 
 *  YUV420PackedSemiPlanar16m2ka : YUV planar format, similar to the
 *                                 YUV420PackedSemiPlanar format, but with the
 *                                 following restrictions:
 *
 *                                 1. The width of the luma plane must be a
 *                                 multiple of 16 pixels.
 *
 *                                 2. The address of both planes must be 
 *                                 aligned to a 2kB boundary.
 * 
 *  YUV420PackedSemiPlanar64x32Tile2m8ka : YUV planar format, similar to the 
 *                                 YUV420PackedSemiPlanar format, but with the
 *                                 following restrictions:
 *
 *                                 1. The data is laid out in a 4x2 MB tiling 
 *                                 memory structure
 *
 *                                 2. The width of each plane is a multiple of
 *                                 2 4x2 MB tiles.
 *
 *                                 3. The height of each plan is a multiple of
 *                                 a 4x2 MB tile.
 *
 *                                 4. The base address of both planes must be 
 *                                 aligned to an 8kB boundary.
 *
 *                                 5. The tiles are scanned in the order 
 *                                 defined in the MFCV5.1 User's Manual.
 */
typedef enum QOMX_COLOR_FORMATTYPE
{
    QOMX_COLOR_FormatYVU420PackedSemiPlanar       = 0x7F000001,
    QOMX_COLOR_FormatYVU420PackedSemiPlanar32m4ka,
    QOMX_COLOR_FormatYUV420PackedSemiPlanar16m2ka,
    QOMX_COLOR_FormatYUV420PackedSemiPlanar64x32Tile2m8ka,
    QOMX_COLOR_FORMATYUV420PackedSemiPlanar32m,
} QOMX_COLOR_FORMATTYPE;

typedef enum QOMX_MEDIAINFOTAGTYPE {
    QOMX_MediaInfoTagVersion,       /**< OMX_VERSIONTYPE. Version of the standard specifying the media information.*/
    QOMX_MediaInfoTagUID,           /**< OMX_U8*. Unique ID of the media data, ie image unique ID.*/
    QOMX_MediaInfoTagDescription,   /**< OMX_U8*. Comments about the media.*/
    QOMX_MediaInfoTagTitle,         /**< OMX_U8*. Title of the media.*/
    QOMX_MediaInfoTagAuthor,        /**< OMX_U8*. Author of the media.*/
    QOMX_MediaInfoTagCopyright,     /**< OMX_U8*. Copyright information.*/
    QOMX_MediaInfoTagTrackNum,      /**< OMX_U32. Track number.*/
    QOMX_MediaInfoTagGenre,         /**< OMX_U8*. The genre of the media.*/
    QOMX_MediaInfoTagEquipmentMake, /**< OMX_U8*. Manufacturer of recording equipment.*/
    QOMX_MediaInfoTagEquipmentModel,/**< OMX_U8*. Model or name of the recording equipment.*/
    QOMX_MediaInfoTagSoftware,      /**< OMX_U8*. Name and version of the software or firmware of the device generating the media.*/
    QOMX_MediaInfoTagAssociatedFile,/**< OMX_U8*. The name of the file related to the media.  For example, an audio file related to an image file.*/
    QOMX_MediaInfoTagResolution,    /**< QOMX_RESOLUTIONTYPE. Number of pixels per resolution unit.*/
    QOMX_MediaInfoTagDateCreated,   /**< QOMX_DATESTAMPTYPE. Date when media was created.*/
    QOMX_MediaInfoTagTimeCreated,   /**< QOMX_TIMESTAMPTYPE. Time when media was created.*/
    QOMX_MediaInfoTagDateModified,  /**< QOMX_DATESTAMPETYPE. Date when file was last modified.*/
    QOMX_MediaInfoTagTimeModified,  /**< QOMX_TIMESTAMPTYPE. Time when file was last modified.*/
    QOMX_MediaInfoTagGPSAreaName,   /**< OMX_U8*. The name of the location.*/
    QOMX_MediaInfoTagGPSVersion,    /**< OMX_VERSIONTYPE. GPS version.*/
    QOMX_MediaInfoTagGPSCoordinates,/**< QOMX_GEODETICTYPE. The longitude, latitude, and altitude.*/
    QOMX_MediaInfoTagGPSSatellites, /**< OMX_U8*. The GPS satellites used for measurements.*/
    QOMX_MediaInfoTagGPSPrecision,  /**< OMX_U32. GPS degree of precision.*/
    QOMX_MediaInfoTagGPSDateStamp,  /**< QOMX_DATESTAMPTYPE. Date of the GPS data.*/
    QOMX_MediaInfoTagGPSTimeStamp,  /**< QOMX_TIMESTAMPTYPE. Time of the GPS data.*/
    QOMX_MediaInfoTagMediaStreamType,/**< QOMX_MEDIASTREAMTYPE. Type of the stream. */ 
    QOMX_MediaInfoDuration,         /**< OMX_TICKS. Total duration of the media.*/
    QOMX_MediaInfoSize,                          /**< OMX_U32. Total size of the media in bytes.*/ 
    QOMX_MediaInfoTagAlbum,                     /**< OMX_U8*. Name of album/movie/show.*/ 
    QOMX_MediaInfoTagLocation,                  /**< OMX_U8*. Recording location information.*/ 
    QOMX_MediaInfoTagClassification,            /**< OMX_U8*. Classification information of media.*/ 
    QOMX_MediaInfoTagRatings,                   /**< OMX_U8*. Media Ratings based on popularity & rating criteria.*/ 
    QOMX_MediaInfoTagKeyword,                   /**< OMX_U8*. Keyword associated with media which are intended to reflect mood of the A/V.*/ 
    QOMX_MediaInfoTagPerformance,               /**< OMX_U8*. Media Performer information..*/ 
    QOMX_MediaInfoTagYear,                      /**< OMX_U8*. Production year information of media.*/ 
    QOMX_MediaInfoTagComposer,                  /**< OMX_U8*. Name of the composer of media i.e. audio.*/ 
    QOMX_MediaInfoTagEncoderName,                  /**< OMX_U8*. Name of the person or organisation who encoded media.*/ 
    QOMX_MediaInfoTagCopyProhibitFlag,          /**< OMX_U8*. Flag to indicate if copy is allowed or not.*/ 
    QOMX_MediaInfoTagLyricist,                  /**< OMX_U8*. Name of the lyricist or text writer in recording. Specific to ID3 tag.*/ 
    QOMX_MediaInfoTagSubtitle,                  /**< OMX_U8*. Subtitle/Description used for informaton directly related to title of media.*/ 
    QOMX_MediaInfoTagOriginalFileName,          /**< OMX_U8*. Original file name.*/ 
    QOMX_MediaInfoTagOriginalLyricist,          /**< OMX_U8*. Name of the original lyricist/text writer of original recording.*/ 
    QOMX_MediaInfoTagOriginalArtist,            /**< OMX_U8*. Name of the original artist.*/ 
    QOMX_MediaInfoTagOriginalReleaseYear,       /**< OMX_U8*. Original release year of recorded media.*/ 
    QOMX_MediaInfoTagFileOwner,                 /**< OMX_U8*. Licensee or name of the file owner.*/ 
    QOMX_MediaInfoTagOrchestra,                 /**< OMX_U8*. Name of the orchestra or performers during recording.*/ 
    QOMX_MediaInfoTagConductor,                 /**< OMX_U8*. Name of the conductor.*/ 
    QOMX_MediaInfoTagRemixedBy,                 /**< OMX_U8*. Person or organization name who did the remix.*/ 
    QOMX_MediaInfoTagAlbumArtist,               /**< OMX_U8*. Name of the album artist.*/ 
    QOMX_MediaInfoTagPublisher,                 /**< OMX_U8*. Name of the publisher or label.*/ 
    QOMX_MediaInfoTagRecordingDates,            /**< OMX_U8*. Recording date of media.*/ 
    QOMX_MediaInfoTagInternetRadioStationName,  /**< OMX_U8*. Name of the Internet radio station from which the audio is streamed.*/ 
    QOMX_MediaInfoTagInternetRadioStationOwner, /**< OMX_U8*. Name of the owner of the Internet radio station from which the audio is streamed.*/ 
    QOMX_MediaInfoTagInternationalRecordingCode,/**< OMX_U8*. International standard recording code.*/ 
    QOMX_MediaInfoTagEncoderSwHwSettings,       /**< OMX_U8*. Software,hardware settings used by encoder.*/ 
    QOMX_MediaInfoTagInvolvedPeopleList,        /**< OMX_U8*. List of people involved. Specific to ID3 tag.*/ 
    QOMX_MediaInfoTagComments,                  /**< OMX_U8*. Comments about the media. It can be any kind of full text informaton.*/ 
    QOMX_MediaInfoTagCommissioned,              /**< OMX_U8*. Commissioned information of media.*/ 
    QOMX_MediaInfoTagSubject,                   /**< OMX_U8*. Subject associated with media.*/ 
    QOMX_MediaInfoTagContact,                   /**< OMX_U8*. Conatct information. URL information of the seller.*/ 
    QOMX_MediaInfoTagValidityPeriod,            /**< OMX_U8*. Length or period of validity of media.*/ 
    QOMX_MediaInfoTagValidityEffectiveDate,     /**< OMX_U8*. Validity effective date of media*/ 
    QOMX_MediaInfoTagNumberOfAllowedPlaybacks,  /**< OMX_U8*. Number of allowed playbacks for this media*/ 
    QOMX_MediaInfoTagPlayCounter,               /**< OMX_U8*. Current play counter of the media.Its number of times a file has been played.*/ 
    QOMX_MediaInfoTagMemo,                      /**< OMX_U8*. Memo associatd with media.*/ 
    QOMX_MediaInfoTagDeviceName,                /**< OMX_U8*. Name of the devices used in creating media.*/ 
    QOMX_MediaInfoTagURL,                       /**< OMX_U8*. List artist /genre /movie sites URL.*/ 
    QOMX_MediaInfoTagFileType,                  /**< OMX_U8*. Indicates type of audio track.*/ 
    QOMX_MediaInfoTagContentGroupDesc,          /**< OMX_U8*. Content group description if the sound belongs to a larger category of of music /sound.*/ 
    QOMX_MediaInfoTagInitialKeys,               /**< OMX_U8*. Contains the musical key in which media starts.*/ 
    QOMX_MediaInfoTagLanguages,                 /**< OMX_U8*. Languages of the text or lyrics spoken or sung in the media.*/ 
    QOMX_MediaInfoTagMediaType,                 /**< OMX_U8*. Describes from which media the media sound originated.*/ 
    QOMX_MediaInfoTagPlaylistDelay,             /**< OMX_U8*. Denotes number of milliseconds between each song of the playlist.*/ 
    QOMX_MediaInfoTagBeatsPerMinute,            /**< OMX_U8*. Number of beats per minute in main part of audio.*/ 
    QOMX_MediaInfoTagPartOfSet,                 /**< OMX_U8*. Describes part of the set selected or played. */ 
    QOMX_MediaInfoTagInstrumentName,            /**< OMX_U8*. Name of the instrument used in creating media.*/ 
    QOMX_MediaInfoTagLyrics,                    /**< OMX_U8*. Lyrics of the media/audio track.*/ 
    QOMX_MediaInfoTagTrackName,                 /**< OMX_U8*. Name of the media/audio track.*/ 
    QOMX_MediaInfoTagMarker,                    /**< OMX_U8*. Text string cotnents placed at a specific location to denote information about the music at that point.*/ 
    QOMX_MediaInfoTagCuePoint,                  /**< OMX_U8*. Subset of the content which can be optionally played.*/ 
    QOMX_MediaInfoTagGPSPositioningName,        /**< OMX_U8*. GPS positioning name. */ 
    QOMX_MediaInfoTagGPSPositioningMethod,      /**< OMX_U8*. GPS positioning method.*/ 
    QOMX_MediaInfoTagGPSSurveyData,             /**< OMX_U8*. GPS survey data. */ 
    QOMX_MediaInfoTagGPSByteOrder,              /**< OMX_U16.GPS byte order. */ 
    QOMX_MediaInfoTagGPSLatitudeRef,            /**< OMX_U32.Reference GPS latitude. */ 
    QOMX_MediaInfoTagGPSLongitudeRef,           /**< OMX_U32.Reference GPS longitude */ 
    QOMX_MediaInfoTagGPSAltitudeRef,            /**< OMX_U32. Reference GPS altitude.*/ 
    QOMX_MediaInfoTagGPSExtensionMapScaleInfo,  /**< OMX_U64. GPS extension map scale information.*/ 
    QOMX_MediaInfoTagUUIDAtomInfo,              /**< OMX_U8*. The user defined data associated with UUID.*/ 
    QOMX_MediaInfoTagUUIDAtomCount,             /**< OMX_U32 UUID atom count.*/ 
    QOMX_MediaInfoTagLocationRole,              /**< OMX_32. Indicates the role of the place. i.e. ‘0’ indicate ‘shooting location'. ‘1’ ‘real location’.*/ 
    QOMX_MediaInfoTagAstronomicalBody,          /**< OMX_U8*. Astronomical body on which the location exists.*/ 
    QOMX_MediaInfoTagUserInfoData               /**< OMX_U8*. The user defined tag informaton.*/ 
} QOMX_MEDIAINFOTAGTYPE;

typedef struct QOMX_MEDIAINFOTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex; /**< Read-only value containing the index of the output port. */
    QOMX_MEDIAINFOTAGTYPE eTag; /**< The type of media info being specified. */
    OMX_U32 nDataSize; /**< The size of the associated cData.  Set nDataSize to 0 to retrieve the size required for cData. */
    OMX_U8 cData[1]; /**< The media data info */
} QOMX_MEDIAINFOTYPE;


typedef enum QOMX_RESOLUTIONUNITTYPE {
    QOMX_ResolutionUnitInch,
    QOMX_ResolutionCentimeter
} QOMX_RESOLUTIONUNITTYPE;

typedef struct QOMX_RESOLUTIONTYPE {
    QOMX_RESOLUTIONUNITTYPE eUnit; /**< The unit of measurement. */
    OMX_U32 nX; /**< The number of pixels per unit in the width direction. */
    OMX_U32 nY; /**< The number of pixels per unit in the height direction. */
} QOMX_RESOLUTIONTYPE;

typedef struct QOMX_TIMESTAMPTYPE {
    OMX_U32 nHour; /**< The hour portion of the time stamp, based on a 24-hour format. */
    OMX_U32 nMinute; /**< The minute portion of the time stamp. */
    OMX_U32 nSecond; /**< The second portion of the time stamp. */
    OMX_U32 nMillisecond; /**< the millisecond portion of the time stamp. */
} QOMX_TIMESTAMPTYPE;

typedef struct QOMX_DATESTAMPTYPE {
    OMX_U32 nYear;  /**< The year portion of the date stamp. */
    OMX_U32 nMonth; /**< The monthportion of the date stamp. Valid values are 1 to 12.*/
    OMX_U32 nDay; /**< The day portion of the date stamp. Valid values are 1 to 31 depending on the month specified.*/
} QOMX_DATESTAMPTYPE;

typedef enum QOMX_GEODETICREFTYPE {
    QOMX_GeodeticRefNorth,  /**< North latitude. */
    QOMX_GeodeticRefSouth,  /**< South latitude. */
    QOMX_GeodeticRefEast,   /**< East longitude. */
    QOMX_GeodeticRefWest    /**< West longitude. */
} QOMX_GEODETICREFTYPE;

/** QOMX_GEODETICANGLETYPE is used to set geodetic angle coordinates on an ellipsoid (the Earth),
and is explicitly used to specify latitude and longitude.  This structure is referenced by QOMX_GEODETICTYPE. */
typedef struct QOMX_GEODETICANGLETYPE {
    QOMX_GEODETICREFTYPE eReference; /**< Indicates whether the geodetic angle is a latitude or longitude. */
    OMX_U32 nDegree; /**< The degree of the latitude or longitude. */
    OMX_U32 nMinute; /**< The minute of the latitude or longitude. */
    OMX_U32 nSecond; /**< The second of the latitude or longitude. */
} QOMX_GEODETICANGLETYPE;

typedef enum QOMX_ALTITUDEREFTYPE {
    QOMX_AltitudeRefSeaLevel, /**< At sea level. */
    QOMX_AltitudeRefBelowSeaLevel /**< Below sea level. */
} QOMX_ALTITUDEREFTYPE;

typedef struct QOMX_ALTITUDETYPE {
    QOMX_ALTITUDEREFTYPE eReference; /**< The reference point for the altitude. */
    OMX_U32 nMeter; /**< The absolute value of the number of meters above or below sea level. */
    OMX_U32 nMillimeter; /**< The absolute value of the number of millimeters above or below sea level. */
} QOMX_ALTITUDETYPE;

/** QOMX_GEODETICTYPE is used to set geodetic coordinates such as longitude, latitude, and altitude.
This structure references QOMX_GEODETICANGLETYPE and QOMX_ALTITUDETYPE. */
typedef struct QOMX_GEODETICTYPE {
    QOMX_GEODETICANGLETYPE sLatitude; /**< Indicates the latitude.*/
    QOMX_GEODETICANGLETYPE sLongitude; /**< Indicates the longitude.*/
    QOMX_ALTITUDETYPE sAltitude; /**< Indicates the altitude.*/
} QOMX_GEODETICTYPE;


typedef struct QOMX_IMAGE_IMAGESIZECONTROLTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex; /**< port index on which size control needs to be applied */
    OMX_U32 nTargetImageSize; /**< expected max target size in Bytes */
} QOMX_IMAGE_IMAGESIZECONTROLTYPE;

typedef enum QOMX_URITYPE {
    QOMX_URITYPE_RTSP, /**< RTSP URI Type. */ 
    QOMX_URITYPE_HTTP, /**< HTTP URI Type. */
    QOMX_URITYPE_LOCAL /**< Local URI Type.(i.e Non Network) */
}QOMX_URITYPE;


typedef enum QOMX_STREAMTYPE {
    QOMX_STREAMTYPE_VOD, /**< Video On demand Stream */
    QOMX_STREAMTYPE_LIVE,/**< Live Stream */
    QOMX_STREAMTYPE_FILE /**< File based Stream */
}QOMX_STREAMTYPE;


typedef struct QOMX_MEDIASTREAMTYPE{
    QOMX_URITYPE eURIType;
    QOMX_STREAMTYPE eStreamType;
}QOMX_MEDIASTREAMTYPE;               


/**
 * This structure specifies the parameters associated with each
 * plane of the uncompressed image/video format.
 */
typedef struct QOMX_PLANEDEFINITIONTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;               /**< Represents the port that this structure applies to */
    OMX_U32 nPlaneIndex;              /**< Specifies the plane enumeration index that this structure applies to, starting with a base value of 1 */
    OMX_U32 nMinStride;               /**< Read-only parameter that specifies the minimum buffer stride */
    OMX_U32 nMaxStride;               /**< Read-only parameter that specifies the maximum buffer stride */
    OMX_U32 nStrideMultiples;         /**< Read-only parameter that specifies the buffer stride multiple supported */
    OMX_S32 nActualStride;            /**< Specifies the actual stride to be applied */
    OMX_U32 nMinPlaneBufferHeight;    /**< Read-only parameter that specifies the minimum buffer height (number of stride lines) */
    OMX_U32 nActualPlaneBufferHeight; /**< Specifies the actual buffer height (number of stride lines) to be applied */
    OMX_U32 nBufferSize;              /**< Read-only parameter that specifies the minimum size of the buffer, in bytes */
    OMX_U32 nBufferAlignment;         /**< Read-only field that specifies the required alignment of the buffer, in bytes */
} QOMX_PLANEDEFINITIONTYPE;

/**
 *  Pause port parameters
 *
 *  STRUCT MEMBERS:
 *  nSize           : Size of the structure in bytes
 *  nVersion        : OMX specification version information
 *  nPortIndex      : Index of port that this structure represent
 *  bPausePort      : Boolean field which indicates if port is paused or resume. By default bPausePort = OMX_FALSE 
 *                    & port will be paused when bPausePort = OMX_TRUE
 */
typedef struct QOMX_CONFIG_PAUSEPORTTYPE {
  OMX_U32 nSize;
  OMX_VERSIONTYPE nVersion;
  OMX_U32 nPortIndex;                /**< Represents the port that this structure applies to */
  OMX_BOOL bPausePort;               /**< Specifies if port need to PAUSE or RESUME */
} QOMX_CONFIG_PAUSEPORTTYPE;


typedef struct QOMX_RECTTYPE {
    OMX_S32 nLeft;
    OMX_S32 nTop;
    OMX_U32 nWidth;
    OMX_U32 nHeight;
} QOMX_RECTTYPE;

typedef struct QOMX_ACTIVEREGIONDETECTIONTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bEnable;
    QOMX_RECTTYPE sROI;
    OMX_U32 nNumExclusionRegions;
    QOMX_RECTTYPE sExclusionRegions[1];
} QOMX_ACTIVEREGIONDETECTIONTYPE;

typedef struct QOMX_ACTIVEREGIONDETECTION_STATUSTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bDetected;
    QOMX_RECTTYPE sDetectedRegion;
} QOMX_ACTIVEREGIONDETECTION_STATUSTYPE;

typedef enum QOMX_SCALE_MODETYPE {
    QOMX_SCALE_MODE_Normal,
    QOMX_SCALE_MODE_Anamorphic,
    QOMX_SCALE_MODE_Max = 0x7FFFFFFF
} QOMX_SCALE_MODETYPE;

typedef struct QOMX_SCALINGMODETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    QOMX_SCALE_MODETYPE  eScaleMode;
} QOMX_SCALINGMODETYPE;

typedef struct QOMX_NOISEREDUCTIONTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bEnable;
    OMX_BOOL bAutoMode;
    OMX_S32 nNoiseReduction;
} QOMX_NOISEREDUCTIONTYPE;

typedef struct QOMX_IMAGEENHANCEMENTTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bEnable;
    OMX_BOOL bAutoMode;
    OMX_S32 nImageEnhancement;
} QOMX_IMAGEENHANCEMENTTYPE;

/*
 * these are part of OMX1.2 but JB MR2 branch doesn't have them defined
 * OMX_IndexParamInterlaceFormat
 * OMX_INTERLACEFORMATTYPE
 */
#ifndef OMX_IndexParamInterlaceFormat
#define OMX_IndexParamInterlaceFormat (0x7FF00000)
typedef enum OMX_INTERLACETYPE
{
   OMX_InterlaceFrameProgressive,
   OMX_InterlaceInterleaveFrameTopFieldFirst,
   OMX_InterlaceInterleaveFrameBottomFieldFirst,
   OMX_InterlaceFrameTopFieldFirst,
   OMX_InterlaceFrameBottomFieldFirst
}OMX_INTERLACEs;

typedef struct OMX_INTERLACEFORMATTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nFormat;
    OMX_TICKS nTimeStamp;
} OMX_INTERLACEFORMATTYPE;
#endif
#if defined( __cplusplus )
}
#endif /* end of macro __cplusplus */

#endif /* end of macro __H_QOMX_IVCOMMONEXTENSIONS_H__ */
