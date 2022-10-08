#ifndef QOMX_STREAMINGEXTENSIONS_H_
#define QOMX_STREAMINGEXTENSIONS_H_
/*--------------------------------------------------------------------------
Copyright (c) 2012, The Linux Foundation. All rights reserved.

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
/*========================================================================

*//** @file QOMX_StreamingExtensions.h

@par FILE SERVICES:
      Qualcomm extensions API for OpenMax IL Streaming Components.

      This file contains the description of the Qualcomm OpenMax IL
      streaming extention interface, through which the IL client and OpenMax
      components can access additional streaming capabilities.

*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/api/OpenMax/QCOM/main/latest/QOMX_StreamingExtensions.h#7 $
$DateTime: 2011/03/02 12:27:27 $
$Change: 1638323 $

========================================================================== */

/* =======================================================================
**               Includes and Public Data Declarations
** ======================================================================= */

/* =======================================================================

                     INCLUDE FILES FOR MODULE

========================================================================== */

#include <OMX_Types.h>
#include <OMX_Component.h>

#if defined( __cplusplus )
extern "C"
{
#endif /* end of macro __cplusplus */

/* =======================================================================

                        DATA DECLARATIONS

========================================================================== */
/* -----------------------------------------------------------------------
** Type Declarations
** ----------------------------------------------------------------------- */
/**
 * Qualcomm vendor streaming extension strings.
 */
#define OMX_QUALCOMM_INDEX_CONFIG_WATERMARK                       "OMX.Qualcomm.index.config.Watermark"
#define OMX_QUALCOMM_INDEX_CONFIG_WATERMARKSTATUS                 "OMX.Qualcomm.index.config.WatermarkStatus"
#define OMX_QUALCOMM_INDEX_CONFIG_BUFFERMARKING                   "OMX.Qualcomm.index.config.BufferMarking"
#define OMX_QUALCOMM_INDEX_PARAM_STREAMING_NETWORKINTERFACE       "OMX.Qualcomm.index.param.streaming.NetworkInterface"
#define OMX_QUALCOMM_INDEX_PARAM_STREAMING_NETWORKPROFILE         "OMX.Qualcomm.index.param.streaming.NetworkProfile"
#define OMX_QUALCOMM_INDEX_PARAM_STREAMING_PROXYSERVER            "OMX.Qualcomm.index.param.streaming.ProxyServer"
#define OMX_QUALCOMM_INDEX_PARAM_STREAMING_SOURCEPORTS            "OMX.Qualcomm.index.param.streaming.SourcePorts"
#define OMX_QUALCOMM_INDEX_CONFIG_STREAMING_PROTOCOLHEADER        "OMX.Qualcomm.index.param.streaming.ProtocolHeader"
#define OMX_QUALCOMM_INDEX_CONFIG_STREAMING_PROTOCOLEVENT         "OMX.Qualcomm.index.config.streaming.ProtocolEvent"
#define OMX_QUALCOMM_INDEX_CONFIG_STREAMING_DYNAMIC_SWITCH_CAPABILITY "OMX.Qualcomm.index.config.streaming.DynamicSessionSwitchCapability"
#define OMX_QUALCOMM_INDEX_CONFIG_STREAMING_PROTOCOLHEADERSEVENT  "OMX.QCOM.index.config.streaming.ProtocolHeadersEvent"
#define OMX_QCOM_INDEX_CONFIG_STREAMING_USERPAUSETIMEOUT          "OMX.QCOM.index.config.streaming.UserPauseTimeout"
#define OMX_QCOM_INDEX_CONFIG_STREAMING_NOTIFYERRORONOPTIONSTIMEOUT   "OMX.QCOM.index.config.streaming.NotifyErrorOnOptionsTimeout"
#define OMX_QCOM_INDEX_CONFIG_STREAMING_USEINTERLEAVEDTCP         "OMX.QCOM.index.config.streaming.UseInterleavedTCP"
#define OMX_QCOM_INDEX_CONFIG_STREAMING_DATAINACTIVITYTIMEOUT     "OMX.QCOM.index.config.streaming.DataInactivityTimeout"
#define OMX_QCOM_INDEX_CONFIG_STREAMING_RTSPOPTIONSKEEPALIVEINTERVAL   "OMX.QCOM.index.config.streaming.RTSPOptionsKeepaliveInterval"
#define OMX_QCOM_INDEX_CONFIG_STREAMING_RTCPRRINTERVAL            "OMX.QCOM.index.config.streaming.RTCPRRInterval"
#define OMX_QCOM_INDEX_CONFIG_STREAMING_RECONFIGUREPORT           "OMX.QCOM.index.config.streaming.ReconfigurePort"
#define OMX_QCOM_INDEX_CONFIG_STREAMING_DEFAULTRTSPMESSAGETIMEOUT "OMX.QCOM.index.config.streaming.DefaultRTSPMessageTimeout"
#define OMX_QCOM_INDEX_CONFIG_STREAMING_ENABLEFIREWALLPROBES      "OMX.QCOM.index.config.streaming.EnableFirewallProbes"
#define OMX_QCOM_INDEX_CONFIG_STREAMING_RTSPOPTIONSBEFORESETUP    "OMX.QCOM.index.config.streaming.RTSPOptionsBeforeSetup"
#define OMX_QCOM_INDEX_CONFIG_STREAMING_RTSPPIPELINEDFASTSTARTUP  "OMX.QCOM.index.config.streaming.RTSPPipelinedFastStartup"
#define OMX_QCOM_INDEX_CONFIG_STREAMING_WMFASTSTARTSPEED          "OMX.QCOM.index.config.streaming.WMFastStartSpeed"
#define OMX_QCOM_INDEX_CONFIG_STREAMING_ENABLEFASTRECONNECT       "OMX.QCOM.index.config.streaming.EnableFastReconnect"
#define OMX_QCOM_INDEX_CONFIG_STREAMING_FASTRECONNECTMAXATTEMPTS  "OMX.QCOM.index.config.streaming.FastReconnectMaxAttempts"
#define OMX_QCOM_INDEX_CONFIG_STREAMING_DOWNLOADPROGRESSUNITSTYPE "OMX.QCOM.index.config.streaming.DownloadProgressUnitsType"
#define OMX_QOMX_INDEX_CONFIG_STREAMING_DOWNLOADPROGRESS          "OMX.QCOM.index.config.streaming.DownloadProgress"
/**
 * Enumeration of the buffering watermark types
 */
typedef enum QOMX_WATERMARKTYPE
{
  QOMX_WATERMARK_UNDERRUN, /**< buffer has reached or is operating in an underrun condition */
  QOMX_WATERMARK_NORMAL /**< has reached or is operating in a normal (optimal) condition */
}QOMX_WATERMARKTYPE;

/**
 * Enumeration of type of buffering level tracking
 */
typedef enum QOMX_WATERMARKUNITSTYPE
{
  QOMX_WATERMARKUNITSTYPE_Time, /**< use a media time based reference */
  QOMX_WATERMARKUNITSTYPE_Data /**< use a data fullness based reference */
}QOMX_WATERMARKUNITSTYPE;

/**
 * Buffering watermark levels.
 *
 *  STRUCT MEMBERS:
 *  nSize        : Size of the structure in bytes
 *  nVersion     : OMX specification version information
 *  nPortIndex   : Port that this structure applies to
 *  eWaterMark   : eWaterMark specifies the type of buffering watermark being
 *                 configured
 *                 QOMX_WATERMARK_UNDERRUN Indicates the condition when the
 *                   buffer has reached or is operating in an underrun condition
 *                   - not enough data
 *                  QOMX_WATERMARK_NORMAL Indicates the condition when the buffer
 *                   has reached or is operating in a normal (optimal) condition
 *                    - sufficient data within the buffer.
 *
 *  nLevel       : specifies the buffering level associated with the watermark.
 *                 The units associated with the watermark level is dependent
 *                 on the eUnitsType being selected.
 *                   QOMX_WATERMARKUNITSTYPE_Time nLevel in units of microseconds.
 *                   QOMX_WATERMARKUNITSTYPE_Data nLevel in units of bytes.
 *
 *  nUnitsType  : specifies the type of buffering level tracking to be used.
 *                  QOMX_WATERMARKUNITSTYPE_Time the buffer watermark level
 *                    shall use a media time based reference.
 *                  QOMX_WATERMARKUNITSTYPE_Data the buffer watermark level
 *                    shall use a data fullness based reference.
 * bEnable      : specifies if the watermark type is being enabled or disabled
 */
typedef struct QOMX_BUFFERINGWATERMARKTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    QOMX_WATERMARKTYPE eWaterMark;
    OMX_U32 nLevel;
    QOMX_WATERMARKUNITSTYPE eUnitsType;
    OMX_BOOL bEnable;
} QOMX_BUFFERINGWATERMARKTYPE;

/**
 *  Current buffering status of the streaming source component, for a given
 *  media port
 *
 *  STRUCT MEMBERS:
 *  nSize        : Size of the structure in bytes
 *  nVersion     : OMX specification version information
 *  nPortIndex   : Port that this structure applies to
 *  eCurrentWaterMark : specifies the current buffer watermark level condition
 *                      QOMX_WATERMARK_UNDERRUN Indicates the condition when the
 *                        buffer has reached or is operating in an underrun
 *                        condition - not enough data
 *                      QOMX_WATERMARK_NORMAL Indicates the condition when the
 *                        buffer has reached or is operating in a normal
 *                        (optimal) condition - sufficient data within the buffer.
 *  eUnitsType      : specifies the type of buffering level tracking to be used.
 *                     QOMX_WATERMARKUNITSTYPE_Time the buffer watermark level
 *                       shall use a media time based reference.
 *                     QOMX_WATERMARKUNITSTYPE_Data the buffer watermark level
 *                       shall use a data fullness based reference.
 *  nCurrentLevel    : specifies the current buffer watermark level condition
 *                     The units associated with the watermark level is dependent
 *                     on the eUnitsType being selected.
 *                       QOMX_WATERMARKUNITSTYPE_Time nLevel in units of microseconds.
 *                       QOMX_WATERMARKUNITSTYPE_Data nLevel in units of bytes.
 */
typedef struct QOMX_BUFFERINGSTATUSTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    QOMX_WATERMARKTYPE eCurrentWaterMark;
    QOMX_WATERMARKUNITSTYPE eUnitsType;
    OMX_U32 nCurrentLevel;
} QOMX_BUFFERINGSTATUSTYPE;

/**
 *  marked buffer shall be emitted when the buffering level has reach an
 *  underrun condition (QOMX_WATERMARK_UNDERRUN).
 *
 *  STRUCT MEMBERS:
 *  nSize             : Size of the structure in bytes
 *  nVersion          : OMX specification version information
 *  nPortIndex        : Port that this structure applies to
 *  markInfo          : identifies the target component handle that shall emit
 *                      the mark buffer event and associated
 *  bEnable           : enables or disables the buffer marking insertion.
 *
 */
typedef struct QOMX_BUFFERMARKINGTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_MARKTYPE markInfo;
    OMX_BOOL  bEnable;
} QOMX_BUFFERMARKINGTYPE;

/**
 * Source ports.
 *
 *  STRUCT MEMBERS:
 *  nSize               : Size of the structure in bytes
 *  nVersion            : OMX specification version information
 *  nMinimumPortNumber  : Minimum port number the component may use
 *  nMaximumPortNumber  : Maximum port number the component may use
 */
typedef struct QOMX_PARAM_STREAMING_SOURCE_PORTS
{
  OMX_U32 nSize;
  OMX_VERSIONTYPE nVersion;
  OMX_U16 nMinimumPortNumber;
  OMX_U16 nMaximumPortNumber;
} QOMX_PARAM_STREAMING_SOURCE_PORTS;

/**
 * Enumeration used to define to the protocol message type.
 */
typedef enum QOMX_STREAMING_PROTOCOLMESSAGETYPE
{
  QOMX_STREAMING_PROTOCOLMESSAGE_REQUEST,
  QOMX_STREAMING_PROTOCOLMESSAGE_RESPONSE,
  QOMX_STREAMING_PROTOCOLMESSAGE_ALL
} QOMX_STREAMING_PROTOCOLMESSAGETYPE;

/**
 * Enumeration used to define the protocol header action type.
 */
typedef enum QOMX_STREAMING_PROTOCOLHEADERACTIONTYPE
{
  QOMX_STREAMING_PROTOCOLHEADERACTION_NONE,
  QOMX_STREAMING_PROTOCOLHEADERACTION_ADD,
  QOMX_STREAMING_PROTOCOLHEADERACTION_REMOVE
} QOMX_STREAMING_PROTOCOLHEADERACTIONTYPE;

/**
 * Protocol message header.
 *
 *  STRUCT MEMBERS:
 *  nSize             : Size of the structure in bytes (including size of
                        messageHeader parameter)
 *  nVersion          : OMX specification version information
 *  eMessageType      : enumeration to distinguish protocol message type
 *  eActionType       : enumeration indicating protocol header action type
 *  nMessageClassSize : size of the message class string (excluding any
 *                      terminating characters)
 *  nHeaderNameSize   : size of the header name string (excluding any
 *                      terminating characters)
 *  nHeaderValueSize  : size of the header value string (excluding any
 *                      terminating characters)
 *  messageHeader     : the NULL-terminated message header string formed by
 *                      concatenating message class, header name and value
 *                      strings, i.e. the first nMessageClassSize bytes of the
 *                      messageHeader parameter correspond to the message class
 *                      (without any terminating characters), followed by the
 *                      header name of size nHeaderNameSize bytes and then the
 *                      header value of size nHeaderValueSize bytes. The value
 *                      of message class is interpreted by what is mentioned in
 *                      eMessageType,
 *                       1) For request message
 *                          (QOMX_STREAMING_PROTOCOLMESSAGE_REQUEST) it is the
 *                          Method token (as specified in the RFC 2616 and RFC
 *                          2326).
 *                       2) For response message
 *                          (QOMX_STREAMING_PROTOCOLMESSAGE_RESPONSE) it is
 *                          either or both the Method token and a three digit
 *                          Status-Code (as specified in the RFC 2616 and
 *                          RFC 2326) or a class of the response Status-Codes
 *                          (1xx, 2xx, 3xx, 4xx, and 5xx). When both present,
 *                          the method token and status code are separated by
 *                          1 empty space.
 *                       3) For all messages
 *                          (QOMX_STREAMING_PROTOCOLMESSAGE_ALL) it will be
 *                          absent (nMessageClassSize will be zero).
 */
typedef struct QOMX_CONFIG_STREAMING_PROTOCOLHEADERTYPE
{
  OMX_U32 nSize;
  OMX_VERSIONTYPE nVersion;
  QOMX_STREAMING_PROTOCOLMESSAGETYPE eMessageType;
  QOMX_STREAMING_PROTOCOLHEADERACTIONTYPE eActionType;
  OMX_U32 nMessageClassSize;
  OMX_U32 nHeaderNameSize;
  OMX_U32 nHeaderValueSize;
  OMX_U8 messageHeader[1];
} QOMX_CONFIG_STREAMING_PROTOCOLHEADERTYPE;

/**
 * Protocol Event.
 *
 *  STRUCT MEMBERS:
 *  nSize             : Size of the structure in bytes (including size of
                        protocolEventText parameter)
 *  nVersion          : OMX specification version information
 *  nProtocolEvent    : 1xx, 2xx, 3xx, 4xx or 5xx codes for HTTP/RTSP protocol
 *  nReasonPhraseSize : size of the reason phrase string (excluding any
 *                      terminating characters)
 *  nEntityBodySize   : size of the entity body string (excluding any
 *                      terminating characters)
 *  nContentUriSize   : size of the url (exclusing any terminating characters)
 *                      url is used a key to identify for which operation this
 *                      event belongs to
 *  protocolEventText : NULL-terminated protocol event text string formed by
 *                      concatenating reason phrase and entity body
 *                      and uri, i.e. the first nReasonPhraseSize bytes of the
 *                      protocolEventText parameter correspond to the reason
 *                      phrase (without any terminating characters), followed
 *                      by the entity body of size nEntityBodySize bytes,
 *                      followed by nContentUriSize bytes of URI
 */
typedef struct QOMX_CONFIG_STREAMING_PROTOCOLEVENTTYPE
{
  OMX_U32 nSize;
  OMX_VERSIONTYPE nVersion;
  OMX_U32 nProtocolEvent;
  OMX_U32 nReasonPhraseSize;
  OMX_U32 nEntityBodySize;
  OMX_U32 nContentUriSize;
  OMX_U8 protocolEventText[1];
} QOMX_CONFIG_STREAMING_PROTOCOLEVENTTYPE;

/**
 * Protocol Headers Event
 *
 * STRUCT MEMBERS:
 * nSize:                   Size of the structure in bytes including
 *                          messageHeaders.
 * nVersion:                OMX specification version information
 * eMessageType:            enumeration to distinguish protocol message
 *                          type
 * nMessageClassSize:       Size of the message class string.
 * nMessageAttributesSize:  Size of the message attributes
 *                          string.
 *
 * This structure can be populated in 2 modes:
 * (i)  Query for required sizes of message class and message
 *      attributes. In this mode, nMessageClassSize and
 *      nMessageAtributesSize both need to be set to zero.
 * (ii) Request to populate messageHeaders. In this mode, at
 *      least one of nMessageClassSize or nMessageAttributesSize
 *      need to be non-zero. On output, messageHeaders will be
 *      populated with the message class and message attributes.
 *      nMessageClassSize and/or nMessageAtributesSize may be
 *      overwritten to reflect the actual start and end of
 *      message class and message attributes. The max sizes of
 *      message class and message attributes will not exceed the
 *      values input by the client. The strings are not null
 *      terminated.
 */
typedef struct QOMX_STREAMING_PROTOCOLHEADERSTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    QOMX_STREAMING_PROTOCOLMESSAGETYPE eMessageType;
    OMX_U32 nMessageClassSize;
    OMX_U32 nMessageAtributesSize;
    OMX_U8 messageHeaders[1];
} QOMX_STREAMING_PROTOCOLHEADERSTYPE;

/**
 * Enumeration of possible streaming network interfaces.
 */
typedef enum QOMX_STREAMING_NETWORKINTERFACETYPE
{
  QOMX_STREAMING_NETWORKINTERFACE_ANY_IFACE,
  QOMX_STREAMING_NETWORKINTERFACE_CDMA_SN_IFACE,
  QOMX_STREAMING_NETWORKINTERFACE_CDMA_AN_IFACE,
  QOMX_STREAMING_NETWORKINTERFACE_UMTS_IFACE,
  QOMX_STREAMING_NETWORKINTERFACE_SIO_IFACE,
  QOMX_STREAMING_NETWORKINTERFACE_CDMA_BCAST_IFACE,
  QOMX_STREAMING_NETWORKINTERFACE_WLAN_IFACE,
  QOMX_STREAMING_NETWORKINTERFACE_DUN_IFACE,
  QOMX_STREAMING_NETWORKINTERFACE_FLO_IFACE,
  QOMX_STREAMING_NETWORKINTERFACE_DVBH_IFACE,
  QOMX_STREAMING_NETWORKINTERFACE_STA_IFACE,
  QOMX_STREAMING_NETWORKINTERFACE_IPSEC_IFACE,
  QOMX_STREAMING_NETWORKINTERFACE_LO_IFACE,
  QOMX_STREAMING_NETWORKINTERFACE_MBMS_IFACE,
  QOMX_STREAMING_NETWORKINTERFACE_IWLAN_3GPP_IFACE,
  QOMX_STREAMING_NETWORKINTERFACE_IWLAN_3GPP2_IFACE,
  QOMX_STREAMING_NETWORKINTERFACE_MIP6_IFACE,
  QOMX_STREAMING_NETWORKINTERFACE_UW_FMC_IFACE,
  QOMX_STREAMING_NETWORKINTERFACE_CMMB_IFACE
} QOMX_STREAMING_NETWORKINTERFACETYPE;

/*
 * Network interface.
 *
 *  STRUCT MEMBERS:
 *  nSize             : Size of the structure in bytes (including size of
                        protocolErrorText parameter)
 *  nVersion          : OMX specification version information
 *  eNetworkInterface : Network interface the component may use
 */
typedef struct QOMX_PARAM_STREAMING_NETWORKINTERFACE
{
  OMX_U32 nSize;
  OMX_VERSIONTYPE nVersion;
  QOMX_STREAMING_NETWORKINTERFACETYPE eNetworkInterface;
} QOMX_PARAM_STREAMING_NETWORKINTERFACE;

/**
 * Enumeration of UnitType for DownloadProgress
 */
typedef enum QOMX_DOWNLOADPROGRESSUNITSTYPE
{
  QOMX_DOWNLOADPROGRESSUNITSTYPE_TIME,
  QOMX_DOWNLOADPROGRESSUNITSTYPE_DATA
} QOMX_DOWNLOADPROGRESSUNITSTYPE;


/**
 * DownloadProgress units
 *
 * STRUCT MEMBERS:
 *  nSize             : Size of the structure in bytes (including size of
                        protocolEventText parameter)
 *  nVersion          : OMX specification version information
 *  nPortIndex        : Port that this structure applies to
 *  eUnitsType        : Specifies the type of units type in
 *                      which download prgoress should be
 *                      reported
 */
typedef struct QOMX_CONFIG_STREAMING_DOWNLOADPROGRESSUNITS
{
  OMX_U32 nSize;
  OMX_VERSIONTYPE nVersion;
  OMX_U32 nPortIndex;
  QOMX_DOWNLOADPROGRESSUNITSTYPE eUnitsType;
} QOMX_CONFIG_STREAMING_DOWNLOADPROGRESSUNITS;


/**
 * Download Progress
 *
 * STRUCT MEMBERS:
 *  nSize             : Size of the structure in bytes (including size of
                        protocolEventText parameter)
 *  nVersion          : OMX specification version information
 *  nPortIndex        : Port that this structure applies to
 *  nDataDownloaded   : specifies the amount of data downloaded
 *                      in time or data scale (based on
 *                      eUnitsType) from the media position
 *                      specified by nStartOffset below. It
 *                      starts at zero and progressively
 *                      increases as more data is downloaded
 *  nCurrentStartOffset: specifies is the current download start
 *                       position in time or data scale (based
 *                       on eUnitsType)
 */
typedef struct QOMX_CONFIG_STREAMING_DOWNLOADPROGRESSTYPE
{
  OMX_U32 nSize;
  OMX_VERSIONTYPE nVersion;
  OMX_U32 nPortIndex;
  OMX_U32 nDataDownloaded;
  OMX_U32 nCurrentStartOffset;
} QOMX_CONFIG_STREAMING_DOWNLOADPROGRESSTYPE;

#if defined( __cplusplus )
}
#endif /* end of macro __cplusplus */

#endif /* QOMX_STREAMINGEXTENSIONS_H_ */

