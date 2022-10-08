/*--------------------------------------------------------------------------
Copyright (c) 2010-2012, The Linux Foundation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Code Aurora nor
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
#include "omx_video_encoder.h"
#include <string.h>
#include "video_encoder_device.h"
#include <stdio.h>
#ifdef _ANDROID_ICS_
#include <media/hardware/HardwareAPI.h>
#endif
#ifdef _ANDROID_
#include <cutils/properties.h>
#endif
#ifndef _ANDROID_
#include <glib.h>
#define strlcpy g_strlcpy
#endif
/*----------------------------------------------------------------------------
* Preprocessor Definitions and Constants
* -------------------------------------------------------------------------*/

#define OMX_SPEC_VERSION 0x00000101
#define OMX_INIT_STRUCT(_s_, _name_)            \
   memset((_s_), 0x0, sizeof(_name_));          \
   (_s_)->nSize = sizeof(_name_);               \
   (_s_)->nVersion.nVersion = OMX_SPEC_VERSION

extern int m_pipe;

// factory function executed by the core to create instances
void *get_omx_component_factory_fn(void)
{
  return(new omx_venc);
}

//constructor

omx_venc::omx_venc()
{
#ifdef _ANDROID_ICS_
  meta_mode_enable = false;
  memset(meta_buffer_hdr,0,sizeof(meta_buffer_hdr));
  memset(meta_buffers,0,sizeof(meta_buffers));
  memset(opaque_buffer_hdr,0,sizeof(opaque_buffer_hdr));
  mUseProxyColorFormat = false;
#endif
}

omx_venc::~omx_venc()
{
  //nothing to do
}

/* ======================================================================
FUNCTION
  omx_venc::ComponentInit

DESCRIPTION
  Initialize the component.

PARAMETERS
  ctxt -- Context information related to the self.
  id   -- Event identifier. This could be any of the following:
          1. Command completion event
          2. Buffer done callback event
          3. Frame done callback event

RETURN VALUE
  None.

========================================================================== */
OMX_ERRORTYPE omx_venc::component_init(OMX_STRING role)
{

  OMX_ERRORTYPE eRet = OMX_ErrorNone;

  int fds[2];
  int r;

  OMX_VIDEO_CODINGTYPE codec_type;

  DEBUG_PRINT_HIGH("\n omx_venc(): Inside component_init()");
  // Copy the role information which provides the decoder m_nkind
  strlcpy((char *)m_nkind,role,OMX_MAX_STRINGNAME_SIZE);

  if(!strncmp((char *)m_nkind,"OMX.qcom.video.encoder.mpeg4",\
              OMX_MAX_STRINGNAME_SIZE))
  {
    strlcpy((char *)m_cRole, "video_encoder.mpeg4",\
            OMX_MAX_STRINGNAME_SIZE);
    codec_type = OMX_VIDEO_CodingMPEG4;
  }
  else if(!strncmp((char *)m_nkind, "OMX.qcom.video.encoder.h263",\
                   OMX_MAX_STRINGNAME_SIZE))
  {
    strlcpy((char *)m_cRole, "video_encoder.h263",OMX_MAX_STRINGNAME_SIZE);
    codec_type = OMX_VIDEO_CodingH263;
  }
  else if(!strncmp((char *)m_nkind, "OMX.qcom.video.encoder.avc",\
                   OMX_MAX_STRINGNAME_SIZE))
  {
    strlcpy((char *)m_cRole, "video_encoder.avc",OMX_MAX_STRINGNAME_SIZE);
    codec_type = OMX_VIDEO_CodingAVC;
  }
  else if(!strncmp((char *)m_nkind, "OMX.qcom.video.encoder.avc.secure",\
                   OMX_MAX_STRINGNAME_SIZE))
  {
    strlcpy((char *)m_cRole, "video_encoder.avc",OMX_MAX_STRINGNAME_SIZE);
    codec_type = OMX_VIDEO_CodingAVC;
    secure_session = true;
  }
  else
  {
    DEBUG_PRINT_ERROR("\nERROR: Unknown Component\n");
    eRet = OMX_ErrorInvalidComponentName;
  }


  if(eRet != OMX_ErrorNone)
  {
    return eRet;
  }

  handle = new venc_dev(this);

  if(handle == NULL)
  {
    DEBUG_PRINT_ERROR("\nERROR: handle is NULL");
    return OMX_ErrorInsufficientResources;
  }

  if(handle->venc_open(codec_type) != true)
  {
    DEBUG_PRINT_ERROR("\nERROR: venc_open failed");
    return OMX_ErrorInsufficientResources;
  }

  //Intialise the OMX layer variables
  memset(&m_pCallbacks,0,sizeof(OMX_CALLBACKTYPE));

  OMX_INIT_STRUCT(&m_sPortParam, OMX_PORT_PARAM_TYPE);
  m_sPortParam.nPorts = 0x2;
  m_sPortParam.nStartPortNumber = (OMX_U32) PORT_INDEX_IN;

  OMX_INIT_STRUCT(&m_sPortParam_audio, OMX_PORT_PARAM_TYPE);
  m_sPortParam_audio.nPorts = 0;
  m_sPortParam_audio.nStartPortNumber = 0;

  OMX_INIT_STRUCT(&m_sPortParam_img, OMX_PORT_PARAM_TYPE);
  m_sPortParam_img.nPorts = 0;
  m_sPortParam_img.nStartPortNumber = 0;

  OMX_INIT_STRUCT(&m_sParamBitrate, OMX_VIDEO_PARAM_BITRATETYPE);
  m_sParamBitrate.nPortIndex = (OMX_U32) PORT_INDEX_OUT;
  m_sParamBitrate.eControlRate = OMX_Video_ControlRateVariableSkipFrames;
  m_sParamBitrate.nTargetBitrate = 64000;

  OMX_INIT_STRUCT(&m_sConfigBitrate, OMX_VIDEO_CONFIG_BITRATETYPE);
  m_sConfigBitrate.nPortIndex = (OMX_U32) PORT_INDEX_OUT;
  m_sConfigBitrate.nEncodeBitrate = 64000;

  OMX_INIT_STRUCT(&m_sConfigFramerate, OMX_CONFIG_FRAMERATETYPE);
  m_sConfigFramerate.nPortIndex = (OMX_U32) PORT_INDEX_OUT;
  m_sConfigFramerate.xEncodeFramerate = 30 << 16;

  OMX_INIT_STRUCT(&m_sConfigIntraRefreshVOP, OMX_CONFIG_INTRAREFRESHVOPTYPE);
  m_sConfigIntraRefreshVOP.nPortIndex = (OMX_U32) PORT_INDEX_OUT;
  m_sConfigIntraRefreshVOP.IntraRefreshVOP = OMX_FALSE;

  OMX_INIT_STRUCT(&m_sConfigFrameRotation, OMX_CONFIG_ROTATIONTYPE);
  m_sConfigFrameRotation.nPortIndex = (OMX_U32) PORT_INDEX_IN;
  m_sConfigFrameRotation.nRotation = 0;

  OMX_INIT_STRUCT(&m_sSessionQuantization, OMX_VIDEO_PARAM_QUANTIZATIONTYPE);
  m_sSessionQuantization.nPortIndex = (OMX_U32) PORT_INDEX_OUT;
  m_sSessionQuantization.nQpI = 9;
  m_sSessionQuantization.nQpP = 6;
  m_sSessionQuantization.nQpB = 2;

  OMX_INIT_STRUCT(&m_sAVCSliceFMO, OMX_VIDEO_PARAM_AVCSLICEFMO);
  m_sAVCSliceFMO.nPortIndex = (OMX_U32) PORT_INDEX_OUT;
  m_sAVCSliceFMO.eSliceMode = OMX_VIDEO_SLICEMODE_AVCDefault;
  m_sAVCSliceFMO.nNumSliceGroups = 0;
  m_sAVCSliceFMO.nSliceGroupMapType = 0;
  OMX_INIT_STRUCT(&m_sParamProfileLevel, OMX_VIDEO_PARAM_PROFILELEVELTYPE);
  m_sParamProfileLevel.nPortIndex = (OMX_U32) PORT_INDEX_OUT;

  OMX_INIT_STRUCT(&m_sIntraperiod, QOMX_VIDEO_INTRAPERIODTYPE);
  m_sIntraperiod.nPortIndex = (OMX_U32) PORT_INDEX_OUT;
  m_sIntraperiod.nPFrames = (m_sConfigFramerate.xEncodeFramerate * 2) - 1;

  OMX_INIT_STRUCT(&m_sErrorCorrection, OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE);
  m_sErrorCorrection.nPortIndex = (OMX_U32) PORT_INDEX_OUT;
  m_sErrorCorrection.bEnableDataPartitioning = OMX_FALSE;
  m_sErrorCorrection.bEnableHEC = OMX_FALSE;
  m_sErrorCorrection.bEnableResync = OMX_FALSE;
  m_sErrorCorrection.bEnableRVLC = OMX_FALSE;
  m_sErrorCorrection.nResynchMarkerSpacing = 0;

  OMX_INIT_STRUCT(&m_sIntraRefresh, OMX_VIDEO_PARAM_INTRAREFRESHTYPE);
  m_sIntraRefresh.nPortIndex = (OMX_U32) PORT_INDEX_OUT;
  m_sIntraRefresh.eRefreshMode = OMX_VIDEO_IntraRefreshMax;

  if(codec_type == OMX_VIDEO_CodingMPEG4)
  {
    m_sParamProfileLevel.eProfile = (OMX_U32) OMX_VIDEO_MPEG4ProfileSimple;
    m_sParamProfileLevel.eLevel = (OMX_U32) OMX_VIDEO_MPEG4Level0;
  }
  else if(codec_type == OMX_VIDEO_CodingH263)
  {
    m_sParamProfileLevel.eProfile = (OMX_U32) OMX_VIDEO_H263ProfileBaseline;
    m_sParamProfileLevel.eLevel = (OMX_U32) OMX_VIDEO_H263Level10;
  }
  else if(codec_type == OMX_VIDEO_CodingAVC)
  {
    m_sParamProfileLevel.eProfile = (OMX_U32) OMX_VIDEO_AVCProfileBaseline;
    m_sParamProfileLevel.eLevel = (OMX_U32) OMX_VIDEO_AVCLevel1;
  }

  // Initialize the video parameters for input port
  OMX_INIT_STRUCT(&m_sInPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
  m_sInPortDef.nPortIndex= (OMX_U32) PORT_INDEX_IN;
  m_sInPortDef.bEnabled = OMX_TRUE;
  m_sInPortDef.bPopulated = OMX_FALSE;
  m_sInPortDef.eDomain = OMX_PortDomainVideo;
  m_sInPortDef.eDir = OMX_DirInput;
  m_sInPortDef.format.video.cMIMEType = "YUV420";
  m_sInPortDef.format.video.nFrameWidth = OMX_CORE_QCIF_WIDTH;
  m_sInPortDef.format.video.nFrameHeight = OMX_CORE_QCIF_HEIGHT;
  m_sInPortDef.format.video.nStride = OMX_CORE_QCIF_WIDTH;
  m_sInPortDef.format.video.nSliceHeight = OMX_CORE_QCIF_HEIGHT;
  m_sInPortDef.format.video.nBitrate = 64000;
  m_sInPortDef.format.video.xFramerate = 15 << 16;
  m_sInPortDef.format.video.eColorFormat =  OMX_COLOR_FormatYUV420SemiPlanar;
  m_sInPortDef.format.video.eCompressionFormat =  OMX_VIDEO_CodingUnused;

  if(dev_get_buf_req(&m_sInPortDef.nBufferCountMin,
                     &m_sInPortDef.nBufferCountActual,
                     &m_sInPortDef.nBufferSize,
                     m_sInPortDef.nPortIndex) != true)
  {
    eRet = OMX_ErrorUndefined;

  }

  // Initialize the video parameters for output port
  OMX_INIT_STRUCT(&m_sOutPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
  m_sOutPortDef.nPortIndex = (OMX_U32) PORT_INDEX_OUT;
  m_sOutPortDef.bEnabled = OMX_TRUE;
  m_sOutPortDef.bPopulated = OMX_FALSE;
  m_sOutPortDef.eDomain = OMX_PortDomainVideo;
  m_sOutPortDef.eDir = OMX_DirOutput;
  m_sOutPortDef.format.video.nFrameWidth = OMX_CORE_QCIF_WIDTH;
  m_sOutPortDef.format.video.nFrameHeight = OMX_CORE_QCIF_HEIGHT;
  m_sOutPortDef.format.video.nBitrate = 64000;
  m_sOutPortDef.format.video.xFramerate = 15 << 16;
  m_sOutPortDef.format.video.eColorFormat =  OMX_COLOR_FormatUnused;
  if(codec_type == OMX_VIDEO_CodingMPEG4)
  {
    m_sOutPortDef.format.video.eCompressionFormat =  OMX_VIDEO_CodingMPEG4;
  }
  else if(codec_type == OMX_VIDEO_CodingH263)
  {
    m_sOutPortDef.format.video.eCompressionFormat =  OMX_VIDEO_CodingH263;
  }
  else
  {
    m_sOutPortDef.format.video.eCompressionFormat =  OMX_VIDEO_CodingAVC;
  }
  if(dev_get_buf_req(&m_sOutPortDef.nBufferCountMin,
                     &m_sOutPortDef.nBufferCountActual,
                     &m_sOutPortDef.nBufferSize,
                     m_sOutPortDef.nPortIndex) != true)
  {
    eRet = OMX_ErrorUndefined;
  }

  // Initialize the video color format for input port
  OMX_INIT_STRUCT(&m_sInPortFormat, OMX_VIDEO_PARAM_PORTFORMATTYPE);
  m_sInPortFormat.nPortIndex = (OMX_U32) PORT_INDEX_IN;
  m_sInPortFormat.nIndex = 0;
  m_sInPortFormat.eColorFormat =  OMX_COLOR_FormatYUV420SemiPlanar;
  m_sInPortFormat.eCompressionFormat = OMX_VIDEO_CodingUnused;


  // Initialize the compression format for output port
  OMX_INIT_STRUCT(&m_sOutPortFormat, OMX_VIDEO_PARAM_PORTFORMATTYPE);
  m_sOutPortFormat.nPortIndex = (OMX_U32) PORT_INDEX_OUT;
  m_sOutPortFormat.nIndex = 0;
  m_sOutPortFormat.eColorFormat = OMX_COLOR_FormatUnused;
  if(codec_type == OMX_VIDEO_CodingMPEG4)
  {
    m_sOutPortFormat.eCompressionFormat =  OMX_VIDEO_CodingMPEG4;
  }
  else if(codec_type == OMX_VIDEO_CodingH263)
  {
    m_sOutPortFormat.eCompressionFormat =  OMX_VIDEO_CodingH263;
  }
  else
  {
    m_sOutPortFormat.eCompressionFormat =  OMX_VIDEO_CodingAVC;
  }

  // mandatory Indices for kronos test suite
  OMX_INIT_STRUCT(&m_sPriorityMgmt, OMX_PRIORITYMGMTTYPE);

  OMX_INIT_STRUCT(&m_sInBufSupplier, OMX_PARAM_BUFFERSUPPLIERTYPE);
  m_sInBufSupplier.nPortIndex = (OMX_U32) PORT_INDEX_IN;

  OMX_INIT_STRUCT(&m_sOutBufSupplier, OMX_PARAM_BUFFERSUPPLIERTYPE);
  m_sOutBufSupplier.nPortIndex = (OMX_U32) PORT_INDEX_OUT;


  // mp4 specific init
  OMX_INIT_STRUCT(&m_sParamMPEG4, OMX_VIDEO_PARAM_MPEG4TYPE);
  m_sParamMPEG4.nPortIndex = (OMX_U32) PORT_INDEX_OUT;
  m_sParamMPEG4.eProfile = OMX_VIDEO_MPEG4ProfileSimple;
  m_sParamMPEG4.eLevel = OMX_VIDEO_MPEG4Level0;
  m_sParamMPEG4.nSliceHeaderSpacing = 0;
  m_sParamMPEG4.bSVH = OMX_FALSE;
  m_sParamMPEG4.bGov = OMX_FALSE;
  m_sParamMPEG4.nPFrames = (m_sOutPortFormat.xFramerate * 2 - 1);  // 2 second intra period for default outport fps
  m_sParamMPEG4.bACPred = OMX_TRUE;
  m_sParamMPEG4.nTimeIncRes = 30; // delta = 2 @ 15 fps
  m_sParamMPEG4.nAllowedPictureTypes = 2; // pframe and iframe
  m_sParamMPEG4.nHeaderExtension = 1; // number of video packet headers per vop
  m_sParamMPEG4.bReversibleVLC = OMX_FALSE;

  // h263 specific init
  OMX_INIT_STRUCT(&m_sParamH263, OMX_VIDEO_PARAM_H263TYPE);
  m_sParamH263.nPortIndex = (OMX_U32) PORT_INDEX_OUT;
  m_sParamH263.nPFrames = (m_sOutPortFormat.xFramerate * 2 - 1); // 2 second intra period for default outport fps
  m_sParamH263.nBFrames = 0;
  m_sParamH263.eProfile = OMX_VIDEO_H263ProfileBaseline;
  m_sParamH263.eLevel = OMX_VIDEO_H263Level10;
  m_sParamH263.bPLUSPTYPEAllowed = OMX_FALSE;
  m_sParamH263.nAllowedPictureTypes = 2;
  m_sParamH263.bForceRoundingTypeToZero = OMX_TRUE;
  m_sParamH263.nPictureHeaderRepetition = 0;
  m_sParamH263.nGOBHeaderInterval = 1;

  // h264 specific init
  OMX_INIT_STRUCT(&m_sParamAVC, OMX_VIDEO_PARAM_AVCTYPE);
  m_sParamAVC.nPortIndex = (OMX_U32) PORT_INDEX_OUT;
  m_sParamAVC.nSliceHeaderSpacing = 0;
  m_sParamAVC.nPFrames = (m_sOutPortFormat.xFramerate * 2 - 1); // 2 second intra period for default outport fps
  m_sParamAVC.nBFrames = 0;
  m_sParamAVC.bUseHadamard = OMX_FALSE;
  m_sParamAVC.nRefFrames = 1;
  m_sParamAVC.nRefIdx10ActiveMinus1 = 1;
  m_sParamAVC.nRefIdx11ActiveMinus1 = 0;
  m_sParamAVC.bEnableUEP = OMX_FALSE;
  m_sParamAVC.bEnableFMO = OMX_FALSE;
  m_sParamAVC.bEnableASO = OMX_FALSE;
  m_sParamAVC.bEnableRS = OMX_FALSE;
  m_sParamAVC.eProfile = OMX_VIDEO_AVCProfileBaseline;
  m_sParamAVC.eLevel = OMX_VIDEO_AVCLevel1;
  m_sParamAVC.nAllowedPictureTypes = 2;
  m_sParamAVC.bFrameMBsOnly = OMX_FALSE;
  m_sParamAVC.bMBAFF = OMX_FALSE;
  m_sParamAVC.bEntropyCodingCABAC = OMX_FALSE;
  m_sParamAVC.bWeightedPPrediction = OMX_FALSE;
  m_sParamAVC.nWeightedBipredicitonMode = 0;
  m_sParamAVC.bconstIpred = OMX_FALSE;
  m_sParamAVC.bDirect8x8Inference = OMX_FALSE;
  m_sParamAVC.bDirectSpatialTemporal = OMX_FALSE;
  m_sParamAVC.nCabacInitIdc = 0;
  m_sParamAVC.eLoopFilterMode = OMX_VIDEO_AVCLoopFilterEnable;

  m_state                   = OMX_StateLoaded;
  m_sExtraData = 0;
  m_sDebugSliceinfo = 0;

  // For H264 enable some parameters in VUI by default
  if (codec_type == OMX_VIDEO_CodingAVC)
  {
    QOMX_VUI_BITSTREAM_RESTRICT parm;
    OMX_INIT_STRUCT(&parm, QOMX_VUI_BITSTREAM_RESTRICT);
    parm.bEnable = OMX_TRUE;
    if (set_parameter(NULL, (OMX_INDEXTYPE)OMX_QcomIndexParamEnableVUIStreamRestrictFlag,
                     (OMX_PTR)&parm)) {
      // Don't treat this as a fatal error
      DEBUG_PRINT_ERROR("Unable to set EnableVUIStreamRestrictFlag as default");
    }
  }

#ifdef _ANDROID_
  char value[PROPERTY_VALUE_MAX] = {0};
  property_get("vidc.venc.debug.sliceinfo", value, "0");
  m_sDebugSliceinfo = (OMX_U32)atoi(value);
  DEBUG_PRINT_HIGH("vidc.venc.debug.sliceinfo value is %d", m_sDebugSliceinfo);
#endif

  if(eRet == OMX_ErrorNone)
  {
    if(pipe(fds))
    {
      DEBUG_PRINT_ERROR("ERROR: pipe creation failed\n");
      eRet = OMX_ErrorInsufficientResources;
    }
    else
    {
      if(fds[0] == 0 || fds[1] == 0)
      {
        if(pipe(fds))
        {
          DEBUG_PRINT_ERROR("ERROR: pipe creation failed\n");
          eRet = OMX_ErrorInsufficientResources;
        }
      }
      if(eRet == OMX_ErrorNone)
      {
        m_pipe_in = fds[0];
        m_pipe_out = fds[1];
      }
    }
    r = pthread_create(&msg_thread_id,0,message_thread,this);

    if(r < 0)
    {
      eRet = OMX_ErrorInsufficientResources;
    }
    else
    {
      r = pthread_create(&async_thread_id,0,async_venc_message_thread,this);
      if(r < 0)
      {
        eRet = OMX_ErrorInsufficientResources;
      }
    }
  }

  DEBUG_PRINT_HIGH("\n Component_init return value = 0x%x", eRet);
  return eRet;
}


/* ======================================================================
FUNCTION
  omx_venc::Setparameter

DESCRIPTION
  OMX Set Parameter method implementation.

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None if successful.

========================================================================== */
OMX_ERRORTYPE  omx_venc::set_parameter(OMX_IN OMX_HANDLETYPE     hComp,
                                       OMX_IN OMX_INDEXTYPE paramIndex,
                                       OMX_IN OMX_PTR        paramData)
{
  OMX_ERRORTYPE eRet = OMX_ErrorNone;


  if(m_state == OMX_StateInvalid)
  {
    DEBUG_PRINT_ERROR("ERROR: Set Param in Invalid State\n");
    return OMX_ErrorInvalidState;
  }
  if(paramData == NULL)
  {
    DEBUG_PRINT_ERROR("ERROR: Get Param in Invalid paramData \n");
    return OMX_ErrorBadParameter;
  }

  /*set_parameter can be called in loaded state
  or disabled port */
  if(m_state == OMX_StateLoaded
     || m_sInPortDef.bEnabled == OMX_FALSE
     || m_sOutPortDef.bEnabled == OMX_FALSE)
  {
    DEBUG_PRINT_LOW("Set Parameter called in valid state");
  }
  else
  {
    DEBUG_PRINT_ERROR("ERROR: Set Parameter called in Invalid State\n");
    return OMX_ErrorIncorrectStateOperation;
  }

  switch(paramIndex)
  {
  case OMX_IndexParamPortDefinition:
    {
      OMX_PARAM_PORTDEFINITIONTYPE *portDefn;
      portDefn = (OMX_PARAM_PORTDEFINITIONTYPE *) paramData;
      DEBUG_PRINT_LOW("set_parameter: OMX_IndexParamPortDefinition H= %d, W = %d\n",
           (int)portDefn->format.video.nFrameHeight,
           (int)portDefn->format.video.nFrameWidth);

      if(PORT_INDEX_IN == portDefn->nPortIndex)
      {
        DEBUG_PRINT_LOW("\n i/p actual cnt requested = %d\n", portDefn->nBufferCountActual);
        DEBUG_PRINT_LOW("\n i/p min cnt requested = %d\n", portDefn->nBufferCountMin);
        DEBUG_PRINT_LOW("\n i/p buffersize requested = %d\n", portDefn->nBufferSize);
        if(handle->venc_set_param(paramData,OMX_IndexParamPortDefinition) != true)
        {
          DEBUG_PRINT_ERROR("\nERROR: venc_set_param input failed");
          return OMX_ErrorUnsupportedSetting;
        }

        DEBUG_PRINT_LOW("\n i/p previous actual cnt = %d\n", m_sInPortDef.nBufferCountActual);
        DEBUG_PRINT_LOW("\n i/p previous min cnt = %d\n", m_sInPortDef.nBufferCountMin);
        memcpy(&m_sInPortDef, portDefn,sizeof(OMX_PARAM_PORTDEFINITIONTYPE));

#ifdef _ANDROID_ICS_
        if (portDefn->format.video.eColorFormat == (OMX_COLOR_FORMATTYPE)QOMX_COLOR_FormatAndroidOpaque) {
            m_sInPortDef.format.video.eColorFormat =
                OMX_COLOR_FormatYUV420SemiPlanar;
            if(secure_session) {
              secure_color_format = (int) QOMX_COLOR_FormatAndroidOpaque;
              mUseProxyColorFormat = false;
              m_input_msg_id = OMX_COMPONENT_GENERATE_ETB;
            } else if(!mUseProxyColorFormat){
              if (!c2d_conv.init()) {
                DEBUG_PRINT_ERROR("\n C2D init failed");
                return OMX_ErrorUnsupportedSetting;
              }
              DEBUG_PRINT_ERROR("\nC2D init is successful");
              mUseProxyColorFormat = true;
              m_input_msg_id = OMX_COMPONENT_GENERATE_ETB_OPQ;
            }
        } else
          mUseProxyColorFormat = false;
#endif
        /*Query Input Buffer Requirements*/
        dev_get_buf_req   (&m_sInPortDef.nBufferCountMin,
                           &m_sInPortDef.nBufferCountActual,
                           &m_sInPortDef.nBufferSize,
                           m_sInPortDef.nPortIndex);

        /*Query ouput Buffer Requirements*/
        dev_get_buf_req   (&m_sOutPortDef.nBufferCountMin,
                           &m_sOutPortDef.nBufferCountActual,
                           &m_sOutPortDef.nBufferSize,
                           m_sOutPortDef.nPortIndex);
        m_sInPortDef.nBufferCountActual = portDefn->nBufferCountActual;
      }
      else if(PORT_INDEX_OUT == portDefn->nPortIndex)
      {
        DEBUG_PRINT_LOW("\n o/p actual cnt requested = %d\n", portDefn->nBufferCountActual);
        DEBUG_PRINT_LOW("\n o/p min cnt requested = %d\n", portDefn->nBufferCountMin);
        DEBUG_PRINT_LOW("\n o/p buffersize requested = %d\n", portDefn->nBufferSize);
        if(handle->venc_set_param(paramData,OMX_IndexParamPortDefinition) != true)
        {
          DEBUG_PRINT_ERROR("\nERROR: venc_set_param output failed");
          return OMX_ErrorUnsupportedSetting;
        }
        memcpy(&m_sOutPortDef,portDefn,sizeof(struct OMX_PARAM_PORTDEFINITIONTYPE));
        update_profile_level(); //framerate , bitrate

        DEBUG_PRINT_LOW("\n o/p previous actual cnt = %d\n", m_sOutPortDef.nBufferCountActual);
        DEBUG_PRINT_LOW("\n o/p previous min cnt = %d\n", m_sOutPortDef.nBufferCountMin);
        m_sOutPortDef.nBufferCountActual = portDefn->nBufferCountActual;
      }
      else
      {
        DEBUG_PRINT_ERROR("ERROR: Set_parameter: Bad Port idx %d",
                    (int)portDefn->nPortIndex);
        eRet = OMX_ErrorBadPortIndex;
      }
      m_sConfigFramerate.xEncodeFramerate = portDefn->format.video.xFramerate;
      m_sConfigBitrate.nEncodeBitrate = portDefn->format.video.nBitrate;
      m_sParamBitrate.nTargetBitrate = portDefn->format.video.nBitrate;
    }
    break;

  case OMX_IndexParamVideoPortFormat:
    {
      OMX_VIDEO_PARAM_PORTFORMATTYPE *portFmt =
      (OMX_VIDEO_PARAM_PORTFORMATTYPE *)paramData;
      DEBUG_PRINT_LOW("set_parameter: OMX_IndexParamVideoPortFormat %d\n",
                  portFmt->eColorFormat);
      //set the driver with the corresponding values
      if(PORT_INDEX_IN == portFmt->nPortIndex)
      {
        if(handle->venc_set_param(paramData,OMX_IndexParamVideoPortFormat) != true)
        {
          return OMX_ErrorUnsupportedSetting;
        }

        DEBUG_PRINT_LOW("set_parameter: OMX_IndexParamVideoPortFormat %d\n",
            portFmt->eColorFormat);
        update_profile_level(); //framerate

#ifdef _ANDROID_ICS_
        if (portFmt->eColorFormat ==
            (OMX_COLOR_FORMATTYPE)QOMX_COLOR_FormatAndroidOpaque) {
            m_sInPortFormat.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
            if(secure_session) {
              secure_color_format = (int) QOMX_COLOR_FormatAndroidOpaque;
              mUseProxyColorFormat = false;
              m_input_msg_id = OMX_COMPONENT_GENERATE_ETB;
            } else if(!mUseProxyColorFormat){
              if (!c2d_conv.init()) {
                DEBUG_PRINT_ERROR("\n C2D init failed");
                return OMX_ErrorUnsupportedSetting;
              }
              DEBUG_PRINT_ERROR("\nC2D init is successful");
              mUseProxyColorFormat = true;
              m_input_msg_id = OMX_COMPONENT_GENERATE_ETB_OPQ;
            }
        }
        else
#endif
        {
            m_sInPortFormat.eColorFormat = portFmt->eColorFormat;
            m_input_msg_id = OMX_COMPONENT_GENERATE_ETB;
            mUseProxyColorFormat = false;
        }
        m_sInPortFormat.xFramerate = portFmt->xFramerate;
      }
      //TODO if no use case for O/P port,delet m_sOutPortFormat
    }
    break;
  case OMX_IndexParamVideoInit:
    {  //TODO, do we need this index set param
      OMX_PORT_PARAM_TYPE* pParam = (OMX_PORT_PARAM_TYPE*)(paramData);
      DEBUG_PRINT_LOW("\n Set OMX_IndexParamVideoInit called");
      break;
    }

  case OMX_IndexParamVideoBitrate:
    {
      OMX_VIDEO_PARAM_BITRATETYPE* pParam = (OMX_VIDEO_PARAM_BITRATETYPE*)paramData;
      DEBUG_PRINT_LOW("set_parameter: OMX_IndexParamVideoBitrate");
      if(handle->venc_set_param(paramData,OMX_IndexParamVideoBitrate) != true)
      {
        return OMX_ErrorUnsupportedSetting;
      }
      m_sParamBitrate.nTargetBitrate = pParam->nTargetBitrate;
      m_sParamBitrate.eControlRate = pParam->eControlRate;
      update_profile_level(); //bitrate
      m_sConfigBitrate.nEncodeBitrate = pParam->nTargetBitrate;
	  m_sInPortDef.format.video.nBitrate = pParam->nTargetBitrate;
      m_sOutPortDef.format.video.nBitrate = pParam->nTargetBitrate;
      DEBUG_PRINT_LOW("\nbitrate = %u", m_sOutPortDef.format.video.nBitrate);
      break;
    }
  case OMX_IndexParamVideoMpeg4:
    {
      OMX_VIDEO_PARAM_MPEG4TYPE* pParam = (OMX_VIDEO_PARAM_MPEG4TYPE*)paramData;
      OMX_VIDEO_PARAM_MPEG4TYPE mp4_param;
      memcpy(&mp4_param, pParam, sizeof(struct OMX_VIDEO_PARAM_MPEG4TYPE));
      DEBUG_PRINT_LOW("set_parameter: OMX_IndexParamVideoMpeg4");
      if(pParam->eProfile == OMX_VIDEO_MPEG4ProfileAdvancedSimple)
      {
#ifdef MAX_RES_1080P
        if(pParam->nBFrames)
        {
          DEBUG_PRINT_HIGH("INFO: Only 1 Bframe is supported");
          mp4_param.nBFrames = 1;
        }
#else
        if(pParam->nBFrames)
        {
          DEBUG_PRINT_ERROR("Warning: B frames not supported\n");
          mp4_param.nBFrames = 0;
        }
#endif
      }
      else
      {
        if(pParam->nBFrames)
        {
          DEBUG_PRINT_ERROR("Warning: B frames not supported\n");
          mp4_param.nBFrames = 0;
        }
      }
      if(handle->venc_set_param(&mp4_param,OMX_IndexParamVideoMpeg4) != true)
      {
        return OMX_ErrorUnsupportedSetting;
      }
      memcpy(&m_sParamMPEG4,pParam, sizeof(struct OMX_VIDEO_PARAM_MPEG4TYPE));
      break;
    }
  case OMX_IndexParamVideoH263:
    {
      OMX_VIDEO_PARAM_H263TYPE* pParam = (OMX_VIDEO_PARAM_H263TYPE*)paramData;
      DEBUG_PRINT_LOW("set_parameter: OMX_IndexParamVideoH263");
      if(handle->venc_set_param(paramData,OMX_IndexParamVideoH263) != true)
      {
        return OMX_ErrorUnsupportedSetting;
      }
      memcpy(&m_sParamH263,pParam, sizeof(struct OMX_VIDEO_PARAM_H263TYPE));
      break;
    }
  case OMX_IndexParamVideoAvc:
    {
      OMX_VIDEO_PARAM_AVCTYPE* pParam = (OMX_VIDEO_PARAM_AVCTYPE*)paramData;
      OMX_VIDEO_PARAM_AVCTYPE avc_param;
      memcpy(&avc_param, pParam, sizeof( struct OMX_VIDEO_PARAM_AVCTYPE));
      DEBUG_PRINT_LOW("set_parameter: OMX_IndexParamVideoAvc");

      if((pParam->eProfile == OMX_VIDEO_AVCProfileHigh)||
         (pParam->eProfile == OMX_VIDEO_AVCProfileMain))
      {
#ifdef MAX_RES_1080P
        if(pParam->nBFrames)
        {
          DEBUG_PRINT_HIGH("INFO: Only 1 Bframe is supported");
          avc_param.nBFrames = 1;
        }
        if(pParam->nRefFrames != 2)
        {
          DEBUG_PRINT_ERROR("Warning: 2 RefFrames are needed, changing RefFrames from %d to 2", pParam->nRefFrames);
          avc_param.nRefFrames = 2;
        }
#else
       if(pParam->nBFrames)
       {
         DEBUG_PRINT_ERROR("Warning: B frames not supported\n");
         avc_param.nBFrames = 0;
       }
       if(pParam->nRefFrames != 1)
       {
         DEBUG_PRINT_ERROR("Warning: Only 1 RefFrame is supported, changing RefFrame from %d to 1)", pParam->nRefFrames);
         avc_param.nRefFrames = 1;
       }
#endif
      }
      else
      {
       if(pParam->nRefFrames != 1)
       {
         DEBUG_PRINT_ERROR("Warning: Only 1 RefFrame is supported, changing RefFrame from %d to 1)", pParam->nRefFrames);
         avc_param.nRefFrames = 1;
       }
       if(pParam->nBFrames)
       {
         DEBUG_PRINT_ERROR("Warning: B frames not supported\n");
         avc_param.nBFrames = 0;
       }
      }
      if(handle->venc_set_param(&avc_param,OMX_IndexParamVideoAvc) != true)
      {
        return OMX_ErrorUnsupportedSetting;
      }

      memcpy(&m_sParamAVC,pParam, sizeof(struct OMX_VIDEO_PARAM_AVCTYPE));
      break;
    }
  case OMX_IndexParamVideoProfileLevelCurrent:
    {
      OMX_VIDEO_PARAM_PROFILELEVELTYPE* pParam = (OMX_VIDEO_PARAM_PROFILELEVELTYPE*)paramData;
      DEBUG_PRINT_LOW("set_parameter: OMX_IndexParamVideoProfileLevelCurrent");
      if(handle->venc_set_param(pParam,OMX_IndexParamVideoProfileLevelCurrent) != true)
      {
        DEBUG_PRINT_ERROR("set_parameter: OMX_IndexParamVideoProfileLevelCurrent failed for Profile: %d "
                          "Level :%d", pParam->eProfile, pParam->eLevel);
        return OMX_ErrorUnsupportedSetting;
      }
      m_sParamProfileLevel.eProfile = pParam->eProfile;
      m_sParamProfileLevel.eLevel = pParam->eLevel;

      if(!strncmp((char *)m_nkind, "OMX.qcom.video.encoder.mpeg4",\
          OMX_MAX_STRINGNAME_SIZE))
      {
          m_sParamMPEG4.eProfile = (OMX_VIDEO_MPEG4PROFILETYPE)m_sParamProfileLevel.eProfile;
          m_sParamMPEG4.eLevel = (OMX_VIDEO_MPEG4LEVELTYPE)m_sParamProfileLevel.eLevel;
          DEBUG_PRINT_LOW("\n MPEG4 profile = %d, level = %d", m_sParamMPEG4.eProfile,
              m_sParamMPEG4.eLevel);
      }
      else if(!strncmp((char *)m_nkind, "OMX.qcom.video.encoder.h263",\
          OMX_MAX_STRINGNAME_SIZE))
      {
          m_sParamH263.eProfile = (OMX_VIDEO_H263PROFILETYPE)m_sParamProfileLevel.eProfile;
          m_sParamH263.eLevel = (OMX_VIDEO_H263LEVELTYPE)m_sParamProfileLevel.eLevel;
          DEBUG_PRINT_LOW("\n H263 profile = %d, level = %d", m_sParamH263.eProfile,
              m_sParamH263.eLevel);
      }
      else if(!strncmp((char *)m_nkind, "OMX.qcom.video.encoder.avc",\
          OMX_MAX_STRINGNAME_SIZE))
      {
          m_sParamAVC.eProfile = (OMX_VIDEO_AVCPROFILETYPE)m_sParamProfileLevel.eProfile;
          m_sParamAVC.eLevel = (OMX_VIDEO_AVCLEVELTYPE)m_sParamProfileLevel.eLevel;
          DEBUG_PRINT_LOW("\n AVC profile = %d, level = %d", m_sParamAVC.eProfile,
              m_sParamAVC.eLevel);
      }
      break;
    }
  case OMX_IndexParamStandardComponentRole:
    {
      OMX_PARAM_COMPONENTROLETYPE *comp_role;
      comp_role = (OMX_PARAM_COMPONENTROLETYPE *) paramData;
      DEBUG_PRINT_LOW("set_parameter: OMX_IndexParamStandardComponentRole %s\n",
                  comp_role->cRole);

      if((m_state == OMX_StateLoaded)&&
          !BITMASK_PRESENT(&m_flags,OMX_COMPONENT_IDLE_PENDING))
      {
         DEBUG_PRINT_LOW("Set Parameter called in valid state");
      }
      else
      {
         DEBUG_PRINT_ERROR("Set Parameter called in Invalid State\n");
         return OMX_ErrorIncorrectStateOperation;
      }

      if(!strncmp((char*)m_nkind, "OMX.qcom.video.encoder.avc",OMX_MAX_STRINGNAME_SIZE))
      {
        if(!strncmp((char*)comp_role->cRole,"video_encoder.avc",OMX_MAX_STRINGNAME_SIZE))
        {
          strlcpy((char*)m_cRole,"video_encoder.avc",OMX_MAX_STRINGNAME_SIZE);
        }
        else
        {
          DEBUG_PRINT_ERROR("ERROR: Setparameter: unknown Index %s\n", comp_role->cRole);
          eRet =OMX_ErrorUnsupportedSetting;
        }
      }
      else if(!strncmp((char*)m_nkind, "OMX.qcom.video.encoder.mpeg4",OMX_MAX_STRINGNAME_SIZE))
      {
        if(!strncmp((const char*)comp_role->cRole,"video_encoder.mpeg4",OMX_MAX_STRINGNAME_SIZE))
        {
          strlcpy((char*)m_cRole,"video_encoder.mpeg4",OMX_MAX_STRINGNAME_SIZE);
        }
        else
        {
          DEBUG_PRINT_ERROR("ERROR: Setparameter: unknown Index %s\n", comp_role->cRole);
          eRet = OMX_ErrorUnsupportedSetting;
        }
      }
      else if(!strncmp((char*)m_nkind, "OMX.qcom.video.encoder.h263",OMX_MAX_STRINGNAME_SIZE))
      {
        if(!strncmp((const char*)comp_role->cRole,"video_encoder.h263",OMX_MAX_STRINGNAME_SIZE))
        {
          strlcpy((char*)m_cRole,"video_encoder.h263",OMX_MAX_STRINGNAME_SIZE);
        }
        else
        {
          DEBUG_PRINT_ERROR("ERROR: Setparameter: unknown Index %s\n", comp_role->cRole);
          eRet =OMX_ErrorUnsupportedSetting;
        }
      }
      else
      {
        DEBUG_PRINT_ERROR("ERROR: Setparameter: unknown param %s\n", m_nkind);
        eRet = OMX_ErrorInvalidComponentName;
      }
      break;
    }

  case OMX_IndexParamPriorityMgmt:
    {
      DEBUG_PRINT_LOW("set_parameter: OMX_IndexParamPriorityMgmt");
      if(m_state != OMX_StateLoaded)
      {
        DEBUG_PRINT_ERROR("ERROR: Set Parameter called in Invalid State\n");
        return OMX_ErrorIncorrectStateOperation;
      }
      OMX_PRIORITYMGMTTYPE *priorityMgmtype = (OMX_PRIORITYMGMTTYPE*) paramData;
      DEBUG_PRINT_LOW("set_parameter: OMX_IndexParamPriorityMgmt %d\n",
                  priorityMgmtype->nGroupID);

      DEBUG_PRINT_LOW("set_parameter: priorityMgmtype %d\n",
                  priorityMgmtype->nGroupPriority);

      m_sPriorityMgmt.nGroupID = priorityMgmtype->nGroupID;
      m_sPriorityMgmt.nGroupPriority = priorityMgmtype->nGroupPriority;

      break;
    }

  case OMX_IndexParamCompBufferSupplier:
    {
      DEBUG_PRINT_LOW("set_parameter: OMX_IndexParamCompBufferSupplier");
      OMX_PARAM_BUFFERSUPPLIERTYPE *bufferSupplierType = (OMX_PARAM_BUFFERSUPPLIERTYPE*) paramData;
      DEBUG_PRINT_LOW("set_parameter: OMX_IndexParamCompBufferSupplier %d\n",
                  bufferSupplierType->eBufferSupplier);
      if(bufferSupplierType->nPortIndex == 0 || bufferSupplierType->nPortIndex ==1)
        m_sInBufSupplier.eBufferSupplier = bufferSupplierType->eBufferSupplier;

      else

        eRet = OMX_ErrorBadPortIndex;

      break;

    }
  case OMX_IndexParamVideoQuantization:
    {
      DEBUG_PRINT_LOW("set_parameter: OMX_IndexParamVideoQuantization\n");
      OMX_VIDEO_PARAM_QUANTIZATIONTYPE *session_qp = (OMX_VIDEO_PARAM_QUANTIZATIONTYPE*) paramData;
      if(session_qp->nPortIndex == PORT_INDEX_OUT)
      {
        if(handle->venc_set_param(paramData, OMX_IndexParamVideoQuantization) != true)
        {
          return OMX_ErrorUnsupportedSetting;
        }
        m_sSessionQuantization.nQpI = session_qp->nQpI;
        m_sSessionQuantization.nQpP = session_qp->nQpP;
      }
      else
      {
        DEBUG_PRINT_ERROR("\nERROR: Unsupported port Index for Session QP setting\n");
        eRet = OMX_ErrorBadPortIndex;
      }
      break;
    }

  case OMX_QcomIndexPortDefn:
    {
      OMX_QCOM_PARAM_PORTDEFINITIONTYPE* pParam =
          (OMX_QCOM_PARAM_PORTDEFINITIONTYPE*)paramData;
      DEBUG_PRINT_LOW("set_parameter: OMX_QcomIndexPortDefn");
      if(pParam->nPortIndex == (OMX_U32)PORT_INDEX_IN)
      {
        if(pParam->nMemRegion > OMX_QCOM_MemRegionInvalid &&
          pParam->nMemRegion < OMX_QCOM_MemRegionMax)
        {
          m_use_input_pmem = OMX_TRUE;
        }
        else
        {
          m_use_input_pmem = OMX_FALSE;
        }
      }
      else if (pParam->nPortIndex == (OMX_U32)PORT_INDEX_OUT)
      {
        if(pParam->nMemRegion > OMX_QCOM_MemRegionInvalid &&
          pParam->nMemRegion < OMX_QCOM_MemRegionMax)
        {
          m_use_output_pmem = OMX_TRUE;
        }
        else
        {
          m_use_output_pmem = OMX_FALSE;
        }
      }
      else
      {
        DEBUG_PRINT_ERROR("ERROR: SetParameter called on unsupported Port Index for QcomPortDefn");
        return OMX_ErrorBadPortIndex;
      }
      break;
    }

  case OMX_IndexParamVideoErrorCorrection:
    {
	    DEBUG_PRINT_LOW("OMX_IndexParamVideoErrorCorrection\n");
      OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE* pParam =
          (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE*)paramData;
      if(!handle->venc_set_param(paramData, OMX_IndexParamVideoErrorCorrection))
      {
        DEBUG_PRINT_ERROR("\nERROR: Request for setting Error Resilience failed");
        return OMX_ErrorUnsupportedSetting;
      }
      memcpy(&m_sErrorCorrection,pParam, sizeof(m_sErrorCorrection));
      break;
    }
  case OMX_IndexParamVideoIntraRefresh:
    {
      DEBUG_PRINT_LOW("set_param:OMX_IndexParamVideoIntraRefresh\n");
      OMX_VIDEO_PARAM_INTRAREFRESHTYPE* pParam =
          (OMX_VIDEO_PARAM_INTRAREFRESHTYPE*)paramData;
      if(!handle->venc_set_param(paramData,OMX_IndexParamVideoIntraRefresh))
      {
        DEBUG_PRINT_ERROR("\nERROR: Request for setting intra refresh failed");
        return OMX_ErrorUnsupportedSetting;
      }
      memcpy(&m_sIntraRefresh, pParam, sizeof(m_sIntraRefresh));
      break;
    }
#ifdef _ANDROID_ICS_
  case OMX_QcomIndexParamVideoMetaBufferMode:
    {
      StoreMetaDataInBuffersParams *pParam =
        (StoreMetaDataInBuffersParams*)paramData;
      if(pParam->nPortIndex == PORT_INDEX_IN)
      {
        if(pParam->bStoreMetaData != meta_mode_enable)
        {
          if(!handle->venc_set_meta_mode(pParam->bStoreMetaData))
          {
            DEBUG_PRINT_ERROR("\nERROR: set Metabuffer mode %d fail",
                         pParam->bStoreMetaData);
            return OMX_ErrorUnsupportedSetting;
          }
          meta_mode_enable = pParam->bStoreMetaData;
          if(meta_mode_enable) {
            m_sInPortDef.nBufferCountActual = 4;
            if(handle->venc_set_param(&m_sInPortDef,OMX_IndexParamPortDefinition) != true)
            {
              DEBUG_PRINT_ERROR("\nERROR: venc_set_param input failed");
              return OMX_ErrorUnsupportedSetting;
            }
          } else {
            /*TODO: reset encoder driver Meta mode*/
            dev_get_buf_req   (&m_sOutPortDef.nBufferCountMin,
                               &m_sOutPortDef.nBufferCountActual,
                               &m_sOutPortDef.nBufferSize,
                               m_sOutPortDef.nPortIndex);
          }
        }
      } else {
          DEBUG_PRINT_ERROR("set_parameter: metamode is "
             "valid for input port only");
          eRet = OMX_ErrorUnsupportedSetting;
      }
      break;
    }
#endif
#ifndef MAX_RES_720P
  case OMX_QcomIndexParamIndexExtraDataType:
    {
      DEBUG_PRINT_LOW("set_parameter: OMX_QcomIndexParamIndexExtraDataType");
      QOMX_INDEXEXTRADATATYPE *pParam = (QOMX_INDEXEXTRADATATYPE *)paramData;
      if (pParam->nIndex == (OMX_INDEXTYPE)OMX_ExtraDataVideoEncoderSliceInfo)
      {
        if (pParam->nPortIndex == PORT_INDEX_OUT)
        {
          if (pParam->bEnabled == OMX_TRUE)
            m_sExtraData |= VEN_EXTRADATA_SLICEINFO;
          else
            m_sExtraData &= ~VEN_EXTRADATA_SLICEINFO;
          DEBUG_PRINT_HIGH("set_param: m_sExtraData=%x", m_sExtraData);
          if(handle->venc_set_param(&m_sExtraData,
              (OMX_INDEXTYPE)OMX_ExtraDataVideoEncoderSliceInfo) != true)
          {
            DEBUG_PRINT_ERROR("ERROR: Setting "
               "OMX_QcomIndexParamIndexExtraDataType failed");
            return OMX_ErrorUnsupportedSetting;
          }
          else
          {
            m_sOutPortDef.nPortIndex = PORT_INDEX_OUT;
            dev_get_buf_req(&m_sOutPortDef.nBufferCountMin,
                            &m_sOutPortDef.nBufferCountActual,
                            &m_sOutPortDef.nBufferSize,
                             m_sOutPortDef.nPortIndex);
            DEBUG_PRINT_HIGH("updated out_buf_req: buffer cnt=%d, "
                "count min=%d, buffer size=%d",
                m_sOutPortDef.nBufferCountActual,
                m_sOutPortDef.nBufferCountMin,
                m_sOutPortDef.nBufferSize);
          }
        }
        else
        {
          DEBUG_PRINT_ERROR("set_parameter: slice information is "
              "valid for output port only");
          eRet = OMX_ErrorUnsupportedIndex;
        }
      }
      else
      {
        DEBUG_PRINT_ERROR("set_parameter: unsupported index (%x), "
            "only slice information extradata is supported", pParam->nIndex);
        eRet = OMX_ErrorUnsupportedIndex;
      }
      break;
    }
#endif
  case OMX_QcomIndexParamVideoMaxAllowedBitrateCheck:
    {
      QOMX_EXTNINDEX_PARAMTYPE* pParam =
         (QOMX_EXTNINDEX_PARAMTYPE*)paramData;
      if(pParam->nPortIndex == PORT_INDEX_OUT)
      {
        handle->m_max_allowed_bitrate_check =
           ((pParam->bEnable == OMX_TRUE) ? true : false);
        DEBUG_PRINT_HIGH("set_parameter: max allowed bitrate check %s",
           ((pParam->bEnable == OMX_TRUE) ? "enabled" : "disabled"));
      }
      else
      {
        DEBUG_PRINT_ERROR("ERROR: OMX_QcomIndexParamVideoMaxAllowedBitrateCheck "
           " called on wrong port(%d)", pParam->nPortIndex);
        return OMX_ErrorBadPortIndex;
      }
      break;
    }
#ifdef MAX_RES_1080P
  case OMX_QcomIndexEnableSliceDeliveryMode:
    {
      QOMX_EXTNINDEX_PARAMTYPE* pParam =
         (QOMX_EXTNINDEX_PARAMTYPE*)paramData;
      if(pParam->nPortIndex == PORT_INDEX_OUT)
      {
        if(!handle->venc_set_param(paramData,
              (OMX_INDEXTYPE)OMX_QcomIndexEnableSliceDeliveryMode))
        {
          DEBUG_PRINT_ERROR("ERROR: Request for setting slice delivery mode failed");
          return OMX_ErrorUnsupportedSetting;
        }
      }
      else
      {
        DEBUG_PRINT_ERROR("ERROR: OMX_QcomIndexEnableSliceDeliveryMode "
           "called on wrong port(%d)", pParam->nPortIndex);
        return OMX_ErrorBadPortIndex;
      }
      break;
    }
#endif
  case OMX_QcomIndexParamSequenceHeaderWithIDR:
    {
      if(!handle->venc_set_param(paramData,
            (OMX_INDEXTYPE)OMX_QcomIndexParamSequenceHeaderWithIDR))
      {
        DEBUG_PRINT_ERROR("ERROR: Request for setting inband sps/pps failed");
        return OMX_ErrorUnsupportedSetting;
      }
      break;
    }
  case OMX_QcomIndexParamEnableVUIStreamRestrictFlag:
    {
      if(!handle->venc_set_param(paramData,
            (OMX_INDEXTYPE)OMX_QcomIndexParamEnableVUIStreamRestrictFlag))
      {
        DEBUG_PRINT_ERROR("ERROR: Request for enabling bitstream_restrict "
                        "flag in VUI failed");
        return OMX_ErrorUnsupportedSetting;
      }
      break;
    }
  case OMX_IndexParamVideoSliceFMO:
  default:
    {
      DEBUG_PRINT_ERROR("ERROR: Setparameter: unknown param %d\n", paramIndex);
      eRet = OMX_ErrorUnsupportedIndex;
      break;
    }
  }
  return eRet;
}

bool omx_venc::update_profile_level()
{
  OMX_U32 eProfile, eLevel;

  if(!handle->venc_get_profile_level(&eProfile,&eLevel))
  {
    DEBUG_PRINT_ERROR("\nFailed to update the profile_level\n");
    return false;
  }

  m_sParamProfileLevel.eProfile = (OMX_VIDEO_MPEG4PROFILETYPE)eProfile;
  m_sParamProfileLevel.eLevel = (OMX_VIDEO_MPEG4LEVELTYPE)eLevel;

  if(!strncmp((char *)m_nkind, "OMX.qcom.video.encoder.mpeg4",\
              OMX_MAX_STRINGNAME_SIZE))
  {
    m_sParamMPEG4.eProfile = (OMX_VIDEO_MPEG4PROFILETYPE)eProfile;
    m_sParamMPEG4.eLevel = (OMX_VIDEO_MPEG4LEVELTYPE)eLevel;
    DEBUG_PRINT_LOW("\n MPEG4 profile = %d, level = %d", m_sParamMPEG4.eProfile,
                    m_sParamMPEG4.eLevel);
  }
  else if(!strncmp((char *)m_nkind, "OMX.qcom.video.encoder.h263",\
                   OMX_MAX_STRINGNAME_SIZE))
  {
    m_sParamH263.eProfile = (OMX_VIDEO_H263PROFILETYPE)eProfile;
    m_sParamH263.eLevel = (OMX_VIDEO_H263LEVELTYPE)eLevel;
    DEBUG_PRINT_LOW("\n H263 profile = %d, level = %d", m_sParamH263.eProfile,
                    m_sParamH263.eLevel);
  }
  else if(!strncmp((char *)m_nkind, "OMX.qcom.video.encoder.avc",\
                   OMX_MAX_STRINGNAME_SIZE))
  {
    m_sParamAVC.eProfile = (OMX_VIDEO_AVCPROFILETYPE)eProfile;
    m_sParamAVC.eLevel = (OMX_VIDEO_AVCLEVELTYPE)eLevel;
    DEBUG_PRINT_LOW("\n AVC profile = %d, level = %d", m_sParamAVC.eProfile,
                    m_sParamAVC.eLevel);
  }
  return true;
}
/* ======================================================================
FUNCTION
  omx_video::SetConfig

DESCRIPTION
  OMX Set Config method implementation

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None if successful.
========================================================================== */
OMX_ERRORTYPE  omx_venc::set_config(OMX_IN OMX_HANDLETYPE      hComp,
                                     OMX_IN OMX_INDEXTYPE configIndex,
                                     OMX_IN OMX_PTR        configData)
{
  if(configData == NULL)
  {
    DEBUG_PRINT_ERROR("ERROR: param is null");
    return OMX_ErrorBadParameter;
  }

  if(m_state == OMX_StateInvalid)
  {
    DEBUG_PRINT_ERROR("ERROR: config called in Invalid state");
    return OMX_ErrorIncorrectStateOperation;
  }

  // params will be validated prior to venc_init
  switch(configIndex)
  {
  case OMX_IndexConfigVideoBitrate:
    {
      OMX_VIDEO_CONFIG_BITRATETYPE* pParam =
        reinterpret_cast<OMX_VIDEO_CONFIG_BITRATETYPE*>(configData);
      DEBUG_PRINT_LOW("\n omx_venc:: set_config(): OMX_IndexConfigVideoBitrate");

      if(pParam->nPortIndex == PORT_INDEX_OUT)
      {
        if(handle->venc_set_config(configData, OMX_IndexConfigVideoBitrate) != true)
        {
          DEBUG_PRINT_ERROR("ERROR: Setting OMX_IndexConfigVideoBitrate failed");
          return OMX_ErrorUnsupportedSetting;
        }

        m_sConfigBitrate.nEncodeBitrate = pParam->nEncodeBitrate;
        m_sParamBitrate.nTargetBitrate = pParam->nEncodeBitrate;
        m_sOutPortDef.format.video.nBitrate = pParam->nEncodeBitrate;
      }
      else
      {
        DEBUG_PRINT_ERROR("ERROR: Unsupported port index: %u", pParam->nPortIndex);
        return OMX_ErrorBadPortIndex;
      }
      break;
    }
  case OMX_IndexConfigVideoFramerate:
    {
      OMX_CONFIG_FRAMERATETYPE* pParam =
        reinterpret_cast<OMX_CONFIG_FRAMERATETYPE*>(configData);
      DEBUG_PRINT_LOW("\n omx_venc:: set_config(): OMX_IndexConfigVideoFramerate");

      if(pParam->nPortIndex == PORT_INDEX_OUT)
      {
        if(handle->venc_set_config(configData, OMX_IndexConfigVideoFramerate) != true)
        {
          DEBUG_PRINT_ERROR("ERROR: Setting OMX_IndexConfigVideoFramerate failed");
          return OMX_ErrorUnsupportedSetting;
        }

        m_sConfigFramerate.xEncodeFramerate = pParam->xEncodeFramerate;
        m_sOutPortDef.format.video.xFramerate = pParam->xEncodeFramerate;
        m_sOutPortFormat.xFramerate = pParam->xEncodeFramerate;
      }
      else
      {
        DEBUG_PRINT_ERROR("ERROR: Unsupported port index: %u", pParam->nPortIndex);
        return OMX_ErrorBadPortIndex;
      }

      break;
    }
  case QOMX_IndexConfigVideoIntraperiod:
    {
      QOMX_VIDEO_INTRAPERIODTYPE* pParam =
        reinterpret_cast<QOMX_VIDEO_INTRAPERIODTYPE*>(configData);

      if(pParam->nPortIndex == PORT_INDEX_OUT)
      {
#ifdef MAX_RES_720P
        if(pParam->nBFrames > 0)
        {
          DEBUG_PRINT_ERROR("B frames not supported\n");
          return OMX_ErrorUnsupportedSetting;
        }
#endif
        if(handle->venc_set_config(configData, (OMX_INDEXTYPE) QOMX_IndexConfigVideoIntraperiod) != true)
        {
          DEBUG_PRINT_ERROR("ERROR: Setting QOMX_IndexConfigVideoIntraperiod failed");
          return OMX_ErrorUnsupportedSetting;
        }
        m_sIntraperiod.nPFrames = pParam->nPFrames;
        m_sIntraperiod.nBFrames = pParam->nBFrames;
        m_sIntraperiod.nIDRPeriod = pParam->nIDRPeriod;

        if(m_sOutPortFormat.eCompressionFormat == OMX_VIDEO_CodingMPEG4)
        {
          m_sParamMPEG4.nPFrames = pParam->nPFrames;
          if(m_sParamMPEG4.eProfile != OMX_VIDEO_MPEG4ProfileSimple)
            m_sParamMPEG4.nBFrames = pParam->nBFrames;
          else
            m_sParamMPEG4.nBFrames = 0;
        }
        else if(m_sOutPortFormat.eCompressionFormat == OMX_VIDEO_CodingH263)
        {
          m_sParamH263.nPFrames = pParam->nPFrames;
        }
        else
        {
          m_sParamAVC.nPFrames = pParam->nPFrames;
          if(m_sParamAVC.eProfile != OMX_VIDEO_AVCProfileBaseline)
            m_sParamAVC.nBFrames = pParam->nBFrames;
          else
            m_sParamAVC.nBFrames = 0;
        }
      }
      else
      {
        DEBUG_PRINT_ERROR("ERROR: (QOMX_IndexConfigVideoIntraperiod) Unsupported port index: %u", pParam->nPortIndex);
        return OMX_ErrorBadPortIndex;
      }

      break;
    }

  case OMX_IndexConfigVideoIntraVOPRefresh:
    {
      OMX_CONFIG_INTRAREFRESHVOPTYPE* pParam =
        reinterpret_cast<OMX_CONFIG_INTRAREFRESHVOPTYPE*>(configData);

      if(pParam->nPortIndex == PORT_INDEX_OUT)
      {
        if(handle->venc_set_config(configData,
            OMX_IndexConfigVideoIntraVOPRefresh) != true)
        {
          DEBUG_PRINT_ERROR("ERROR: Setting OMX_IndexConfigVideoIntraVOPRefresh failed");
          return OMX_ErrorUnsupportedSetting;
        }

        m_sConfigIntraRefreshVOP.IntraRefreshVOP = pParam->IntraRefreshVOP;
      }
      else
      {
        DEBUG_PRINT_ERROR("ERROR: Unsupported port index: %u", pParam->nPortIndex);
        return OMX_ErrorBadPortIndex;
      }

      break;
    }
  case OMX_IndexConfigCommonRotate:
    {
      OMX_CONFIG_ROTATIONTYPE *pParam =
         reinterpret_cast<OMX_CONFIG_ROTATIONTYPE*>(configData);
      OMX_S32 nRotation;

      if(pParam->nPortIndex != PORT_INDEX_IN){
           DEBUG_PRINT_ERROR("ERROR: Unsupported port index: %u", pParam->nPortIndex);
           return OMX_ErrorBadPortIndex;
      }
      if( pParam->nRotation == 0   ||
          pParam->nRotation == 90  ||
          pParam->nRotation == 180 ||
          pParam->nRotation == 270 ) {
          DEBUG_PRINT_HIGH("\nset_config: Rotation Angle %u", pParam->nRotation);
      } else {
          DEBUG_PRINT_ERROR("ERROR: un supported Rotation %u", pParam->nRotation);
          return OMX_ErrorUnsupportedSetting;
      }
      nRotation = pParam->nRotation - m_sConfigFrameRotation.nRotation;
      if(nRotation < 0)
        nRotation = -nRotation;
      if(nRotation == 90 || nRotation == 270) {
          DEBUG_PRINT_HIGH("\nset_config: updating device Dims");
          if(handle->venc_set_config(configData,
             OMX_IndexConfigCommonRotate) != true) {
             DEBUG_PRINT_ERROR("ERROR: Set OMX_IndexConfigCommonRotate failed");
             return OMX_ErrorUnsupportedSetting;
          } else {
                  OMX_U32 nFrameWidth;

                  DEBUG_PRINT_HIGH("\nset_config: updating port Dims");

                  nFrameWidth = m_sInPortDef.format.video.nFrameWidth;
	          m_sInPortDef.format.video.nFrameWidth =
                      m_sInPortDef.format.video.nFrameHeight;
                  m_sInPortDef.format.video.nFrameHeight = nFrameWidth;

                  m_sOutPortDef.format.video.nFrameWidth  =
                      m_sInPortDef.format.video.nFrameWidth;
                  m_sOutPortDef.format.video.nFrameHeight =
                      m_sInPortDef.format.video.nFrameHeight;
                  m_sConfigFrameRotation.nRotation = pParam->nRotation;
           }
       } else {
          m_sConfigFrameRotation.nRotation = pParam->nRotation;
      }
      break;
    }
  case OMX_QcomIndexConfigVideoFramePackingArrangement:
    {
      if(m_sOutPortFormat.eCompressionFormat == OMX_VIDEO_CodingAVC)
      {
        OMX_QCOM_FRAME_PACK_ARRANGEMENT *configFmt =
          (OMX_QCOM_FRAME_PACK_ARRANGEMENT *) configData;
        extra_data_handle.set_frame_pack_data(configFmt);
      }
      else
      {
        DEBUG_PRINT_ERROR("ERROR: FramePackingData not supported for non AVC compression");
      }
      break;
    }
  default:
    DEBUG_PRINT_ERROR("ERROR: unsupported index %d", (int) configIndex);
    break;
  }

  return OMX_ErrorNone;
}

/* ======================================================================
FUNCTION
  omx_venc::ComponentDeInit

DESCRIPTION
  Destroys the component and release memory allocated to the heap.

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None if everything successful.

========================================================================== */
OMX_ERRORTYPE  omx_venc::component_deinit(OMX_IN OMX_HANDLETYPE hComp)
{
  OMX_U32 i = 0;
  DEBUG_PRINT_HIGH("\n omx_venc(): Inside component_deinit()");
  if(OMX_StateLoaded != m_state)
  {
    DEBUG_PRINT_ERROR("WARNING:Rxd DeInit,OMX not in LOADED state %d\n",\
                      m_state);
  }
  if(m_out_mem_ptr)
  {
    DEBUG_PRINT_LOW("Freeing the Output Memory\n");
    for(i=0; i< m_sOutPortDef.nBufferCountActual; i++ )
    {
      free_output_buffer (&m_out_mem_ptr[i]);
    }
    free(m_out_mem_ptr);
    m_out_mem_ptr = NULL;
  }

  /*Check if the input buffers have to be cleaned up*/
  if(m_inp_mem_ptr
#ifdef _ANDROID_ICS_
     && !meta_mode_enable
#endif
     )
  {
    DEBUG_PRINT_LOW("Freeing the Input Memory\n");
    for(i=0; i<m_sInPortDef.nBufferCountActual; i++ )
    {
      free_input_buffer (&m_inp_mem_ptr[i]);
    }


    free(m_inp_mem_ptr);
    m_inp_mem_ptr = NULL;
  }

  // Reset counters in mesg queues
  m_ftb_q.m_size=0;
  m_cmd_q.m_size=0;
  m_etb_q.m_size=0;
  m_ftb_q.m_read = m_ftb_q.m_write =0;
  m_cmd_q.m_read = m_cmd_q.m_write =0;
  m_etb_q.m_read = m_etb_q.m_write =0;

#ifdef _ANDROID_
  // Clear the strong reference
  DEBUG_PRINT_HIGH("Calling m_heap_ptr.clear()\n");
  m_heap_ptr.clear();
#endif // _ANDROID_
  DEBUG_PRINT_HIGH("Calling venc_close()\n");
  handle->venc_close();
  DEBUG_PRINT_HIGH("Deleting HANDLE[%p]\n", handle);
  delete (handle);
  DEBUG_PRINT_HIGH("OMX_Venc:Component Deinit\n");
  return OMX_ErrorNone;
}


OMX_U32 omx_venc::dev_stop( void)
{
  return handle->venc_stop();
}


OMX_U32 omx_venc::dev_pause(void)
{
  return handle->venc_pause();
}

OMX_U32 omx_venc::dev_start(void)
{
  return handle->venc_start();
}

OMX_U32 omx_venc::dev_flush(unsigned port)
{
  return handle->venc_flush(port);
}
OMX_U32 omx_venc::dev_resume(void)
{
  return handle->venc_resume();
}

OMX_U32 omx_venc::dev_start_done(void)
{
  return handle->venc_start_done();
}

OMX_U32 omx_venc::dev_stop_done(void)
{
  return handle->venc_stop_done();
}

bool omx_venc::dev_use_buf(void *buf_addr,unsigned port,unsigned index)
{
  return handle->venc_use_buf(buf_addr,port,index);
}

bool omx_venc::dev_free_buf(void *buf_addr,unsigned port)
{
  return handle->venc_free_buf(buf_addr,port);
}

bool omx_venc::dev_empty_buf(void *buffer, void *pmem_data_buf,unsigned index,unsigned fd)
{
  return  handle->venc_empty_buf(buffer, pmem_data_buf,index,fd);
}

bool omx_venc::dev_fill_buf(void *buffer, void *pmem_data_buf,unsigned index,unsigned fd)
{
  return handle->venc_fill_buf(buffer, pmem_data_buf,index,fd);
}

bool omx_venc::dev_get_seq_hdr(void *buffer, unsigned size, unsigned *hdrlen)
{
  return handle->venc_get_seq_hdr(buffer, size, hdrlen);
}

bool omx_venc::dev_loaded_start()
{
  return handle->venc_loaded_start();
}

bool omx_venc::dev_loaded_stop()
{
  return handle->venc_loaded_stop();
}

bool omx_venc::dev_loaded_start_done()
{
  return handle->venc_loaded_start_done();
}

bool omx_venc::dev_loaded_stop_done()
{
  return handle->venc_loaded_stop_done();
}

bool omx_venc::dev_get_buf_req(OMX_U32 *min_buff_count,
                               OMX_U32 *actual_buff_count,
                               OMX_U32 *buff_size,
                               OMX_U32 port)
{
  return handle->venc_get_buf_req(min_buff_count,
                                  actual_buff_count,
                                  buff_size,
                                  port);

}

bool omx_venc::dev_set_buf_req(OMX_U32 *min_buff_count,
                               OMX_U32 *actual_buff_count,
                               OMX_U32 *buff_size,
                               OMX_U32 port)
{
  return handle->venc_set_buf_req(min_buff_count,
                                  actual_buff_count,
                                  buff_size,
                                  port);

}

int omx_venc::async_message_process (void *context, void* message)
{
  omx_video* omx = NULL;
  struct venc_msg *m_sVenc_msg = NULL;
  OMX_BUFFERHEADERTYPE* omxhdr = NULL;
  struct venc_buffer *temp_buff = NULL;

  if(context == NULL || message == NULL)
  {
    DEBUG_PRINT_ERROR("\nERROR: omx_venc::async_message_process invalid i/p params");
    return -1;
  }
  m_sVenc_msg = (struct venc_msg *)message;

  omx = reinterpret_cast<omx_video*>(context);

  if(m_sVenc_msg->statuscode != VEN_S_SUCCESS)
  {
    DEBUG_PRINT_ERROR("\nERROR: async_msg_process() - Error statuscode = %d\n",
        m_sVenc_msg->statuscode);
    omx->omx_report_error();
  }

  DEBUG_PRINT_LOW("\n omx_venc::async_message_process- msgcode = %d\n",
               m_sVenc_msg->msgcode);
  switch(m_sVenc_msg->msgcode)
  {

  case VEN_MSG_START:
    omx->post_event (NULL,m_sVenc_msg->statuscode,\
                     OMX_COMPONENT_GENERATE_START_DONE);
    break;

  case VEN_MSG_STOP:
    omx->post_event (NULL,m_sVenc_msg->statuscode,\
                     OMX_COMPONENT_GENERATE_STOP_DONE);
    break;

  case VEN_MSG_RESUME:
    omx->post_event (NULL,m_sVenc_msg->statuscode,\
                     OMX_COMPONENT_GENERATE_RESUME_DONE);
    break;

  case VEN_MSG_PAUSE:
    omx->post_event (NULL,m_sVenc_msg->statuscode,\
                     OMX_COMPONENT_GENERATE_PAUSE_DONE);

    break;

  case VEN_MSG_FLUSH_INPUT_DONE:

    omx->post_event (NULL,m_sVenc_msg->statuscode,\
                     OMX_COMPONENT_GENERATE_EVENT_INPUT_FLUSH);
    break;
  case VEN_MSG_FLUSH_OUPUT_DONE:
    omx->post_event (NULL,m_sVenc_msg->statuscode,\
                     OMX_COMPONENT_GENERATE_EVENT_OUTPUT_FLUSH);
    break;
  case VEN_MSG_INPUT_BUFFER_DONE:
    omxhdr = (OMX_BUFFERHEADERTYPE* )\
             m_sVenc_msg->buf.clientdata;

    if(omxhdr == NULL ||
       (((OMX_U32)(omxhdr - omx->m_inp_mem_ptr) > omx->m_sInPortDef.nBufferCountActual) &&
        ((OMX_U32)(omxhdr - omx->meta_buffer_hdr) > omx->m_sInPortDef.nBufferCountActual)))
    {
      omxhdr = NULL;
      m_sVenc_msg->statuscode = VEN_S_EFAIL;
    }

#ifdef _ANDROID_ICS_
      omx->omx_release_meta_buffer(omxhdr);
#endif
    omx->post_event ((unsigned int)omxhdr,m_sVenc_msg->statuscode,
                     OMX_COMPONENT_GENERATE_EBD);
    break;
  case VEN_MSG_OUTPUT_BUFFER_DONE:

    omxhdr = (OMX_BUFFERHEADERTYPE*)m_sVenc_msg->buf.clientdata;

    if( (omxhdr != NULL) &&
        ((OMX_U32)(omxhdr - omx->m_out_mem_ptr)  < omx->m_sOutPortDef.nBufferCountActual))
    {
      if(m_sVenc_msg->buf.len <=  omxhdr->nAllocLen)
      {
        omxhdr->nFilledLen = m_sVenc_msg->buf.len;
        omxhdr->nOffset = m_sVenc_msg->buf.offset;
        omxhdr->nTimeStamp = m_sVenc_msg->buf.timestamp;
        DEBUG_PRINT_LOW("\n o/p TS = %u", (OMX_U32)m_sVenc_msg->buf.timestamp);
        omxhdr->nFlags = m_sVenc_msg->buf.flags;

        /*Use buffer case*/
        if(omx->output_use_buffer && !omx->m_use_output_pmem)
        {
          DEBUG_PRINT_LOW("\n memcpy() for o/p Heap UseBuffer");
          memcpy(omxhdr->pBuffer,
                 (m_sVenc_msg->buf.ptrbuffer),
                  m_sVenc_msg->buf.len);
        }
      }
      else
      {
        omxhdr->nFilledLen = 0;
      }

    }
    else
    {
      omxhdr = NULL;
      m_sVenc_msg->statuscode = VEN_S_EFAIL;
    }

    omx->post_event ((unsigned int)omxhdr,m_sVenc_msg->statuscode,
                     OMX_COMPONENT_GENERATE_FBD);
    break;
  case VEN_MSG_NEED_OUTPUT_BUFFER:
    //TBD what action needs to be done here??
    break;
  default:
    break;
  }
  return 0;
}
bool omx_venc::is_secure_session()
{
  return secure_session;
}
