/*--------------------------------------------------------------------------
Copyright (c) 2012, Code Aurora Forum. All rights reserved.

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
#ifndef __OMX_VENC_DEV__
#define __OMX_VENC_DEV__

#include "OMX_Types.h"
#include "OMX_Core.h"
#include "OMX_QCOMExtns.h"
#include "qc_omx_component.h"
#include "omx_video_common.h"
#include "omx_video_base.h"
#include "omx_video_encoder.h"
#include <linux/videodev2.h>
#include <poll.h>
#define TIMEOUT 5000
#define MAX_RECON_BUFFERS 4

void* async_venc_message_thread (void *);

struct msm_venc_switch{
	unsigned char	status;
};

struct msm_venc_allocatorproperty{
	unsigned long	 mincount;
	unsigned long	 maxcount;
	unsigned long	 actualcount;
	unsigned long	 datasize;
	unsigned long	 suffixsize;
	unsigned long	 alignment;
	unsigned long	 bufpoolid;
};

struct msm_venc_basecfg{
	unsigned long	input_width;
	unsigned long	input_height;
	unsigned long	dvs_width;
	unsigned long	dvs_height;
	unsigned long	codectype;
	unsigned long	fps_num;
	unsigned long	fps_den;
	unsigned long	targetbitrate;
	unsigned long	inputformat;
};

struct msm_venc_profile{
	unsigned long	profile;
};
struct msm_venc_profilelevel{
	unsigned long	level;
};

struct msm_venc_sessionqp{
	unsigned long	iframeqp;
	unsigned long	pframqp;
	unsigned long	bframqp;
};

struct msm_venc_qprange{
	unsigned long	maxqp;
	unsigned long	minqp;
};
struct msm_venc_intraperiod{
	unsigned long	num_pframes;
	unsigned long	num_bframes;
};
struct msm_venc_seqheader{
	unsigned char *hdrbufptr;
	unsigned long	bufsize;
	unsigned long	hdrlen;
};

struct msm_venc_capability{
	unsigned long	codec_types;
	unsigned long	maxframe_width;
	unsigned long	maxframe_height;
	unsigned long	maxtarget_bitrate;
	unsigned long	maxframe_rate;
	unsigned long	input_formats;
	unsigned char	dvs;
};

struct msm_venc_entropycfg{
	unsigned longentropysel;
	unsigned long	cabacmodel;
};

struct msm_venc_dbcfg{
	unsigned long	db_mode;
	unsigned long	slicealpha_offset;
	unsigned long	slicebeta_offset;
};

struct msm_venc_intrarefresh{
	unsigned long	irmode;
	unsigned long	mbcount;
};

struct msm_venc_multiclicecfg{
	unsigned long	mslice_mode;
	unsigned long	mslice_size;
};

struct msm_venc_bufferflush{
	unsigned long	flush_mode;
};

struct msm_venc_ratectrlcfg{
	unsigned long	rcmode;
};

struct	msm_venc_voptimingcfg{
	unsigned long	voptime_resolution;
};
struct msm_venc_framerate{
	unsigned long	fps_denominator;
	unsigned long	fps_numerator;
};

struct msm_venc_targetbitrate{
	unsigned long	target_bitrate;
};


struct msm_venc_rotation{
	unsigned long	rotation;
};

struct msm_venc_timeout{
	 unsigned long	millisec;
};

struct msm_venc_headerextension{
	 unsigned long	header_extension;
};

class venc_dev
{
public:
  venc_dev(class omx_venc *venc_class); //constructor
  ~venc_dev(); //des

  bool venc_open(OMX_U32);
  void venc_close();
  unsigned venc_stop(void);
  unsigned venc_pause(void);
  unsigned venc_start(void);
  unsigned venc_flush(unsigned);
#ifdef _ANDROID_ICS_
  bool venc_set_meta_mode(bool);
#endif
  unsigned venc_resume(void);
  unsigned venc_start_done(void);
  unsigned venc_stop_done(void);
  bool venc_use_buf(void*, unsigned,unsigned);
  bool venc_free_buf(void*, unsigned);
  bool venc_empty_buf(void *, void *,unsigned,unsigned);
  bool venc_fill_buf(void *, void *,unsigned,unsigned);

  bool venc_get_buf_req(unsigned long *,unsigned long *,
                        unsigned long *,unsigned long);
  bool venc_set_buf_req(unsigned long *,unsigned long *,
                        unsigned long *,unsigned long);
  bool venc_set_param(void *,OMX_INDEXTYPE);
  bool venc_set_config(void *configData, OMX_INDEXTYPE index);
  bool venc_get_profile_level(OMX_U32 *eProfile,OMX_U32 *eLevel);
  bool venc_get_seq_hdr(void *, unsigned, unsigned *);
  bool venc_loaded_start(void);
  bool venc_loaded_stop(void);
  bool venc_loaded_start_done(void);
  bool venc_loaded_stop_done(void);
  OMX_U32 m_nDriver_fd;
  bool m_profile_set;
  bool m_level_set;
  struct recon_buffer {
	  unsigned char* virtual_address;
	  int pmem_fd;
	  int size;
	  int alignment;
	  int offset;
#ifdef USE_ION
          int ion_device_fd;
          struct ion_allocation_data alloc_data;
          struct ion_fd_data ion_alloc_fd;
#endif
	  };

  recon_buffer recon_buff[MAX_RECON_BUFFERS];
  int recon_buffers_count;
  bool m_max_allowed_bitrate_check;
  int etb_count;
  class omx_venc *venc_handle;
private:
  struct msm_venc_basecfg             m_sVenc_cfg;
  struct msm_venc_ratectrlcfg         rate_ctrl;
  struct msm_venc_targetbitrate       bitrate;
  struct msm_venc_intraperiod         intra_period;
  struct msm_venc_profile             codec_profile;
  struct msm_venc_profilelevel        profile_level;
  struct msm_venc_switch              set_param;
  struct msm_venc_voptimingcfg        time_inc;
  struct msm_venc_allocatorproperty   m_sInput_buff_property;
  struct msm_venc_allocatorproperty   m_sOutput_buff_property;
  struct msm_venc_sessionqp           session_qp;
  struct msm_venc_multiclicecfg       multislice;
  struct msm_venc_entropycfg          entropy;
  struct msm_venc_dbcfg               dbkfilter;
  struct msm_venc_intrarefresh        intra_refresh;
  struct msm_venc_headerextension     hec;
  struct msm_venc_voptimingcfg        voptimecfg;

  bool venc_set_profile_level(OMX_U32 eProfile,OMX_U32 eLevel);
  bool venc_set_intra_period(OMX_U32 nPFrames, OMX_U32 nBFrames);
  bool venc_set_target_bitrate(OMX_U32 nTargetBitrate, OMX_U32 config);
  bool venc_set_ratectrl_cfg(OMX_VIDEO_CONTROLRATETYPE eControlRate);
  bool venc_set_session_qp(OMX_U32 i_frame_qp, OMX_U32 p_frame_qp,OMX_U32 b_frame_qp);
  bool venc_set_encode_framerate(OMX_U32 encode_framerate, OMX_U32 config);
  bool venc_set_intra_vop_refresh(OMX_BOOL intra_vop_refresh);
  bool venc_set_color_format(OMX_COLOR_FORMATTYPE color_format);
  bool venc_validate_profile_level(OMX_U32 *eProfile, OMX_U32 *eLevel);
  bool venc_set_multislice_cfg(OMX_INDEXTYPE codec, OMX_U32 slicesize);
  bool venc_set_entropy_config(OMX_BOOL enable, OMX_U32 i_cabac_level);
  bool venc_set_inloop_filter(OMX_VIDEO_AVCLOOPFILTERTYPE loop_filter);
  bool venc_set_intra_refresh (OMX_VIDEO_INTRAREFRESHTYPE intrarefresh, OMX_U32 nMBs);
  bool venc_set_error_resilience(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE* error_resilience);
  bool venc_set_voptiming_cfg(OMX_U32 nTimeIncRes);
  void venc_config_print();
#ifdef MAX_RES_1080P
  OMX_U32 pmem_free();
  OMX_U32 pmem_allocate(OMX_U32 size, OMX_U32 alignment, OMX_U32 count);
  OMX_U32 venc_allocate_recon_buffers();
  inline int clip2(int x)
  {
	  x = x -1;
	  x = x | x >> 1;
	  x = x | x >> 2;
	  x = x | x >> 4;
	  x = x | x >> 16;
	  x = x + 1;
	  return x;
  }
#endif
};

enum instance_state {
	MSM_VIDC_CORE_UNINIT_DONE = 0x0001,
	MSM_VIDC_CORE_INIT,
	MSM_VIDC_CORE_INIT_DONE,
	MSM_VIDC_OPEN,
	MSM_VIDC_OPEN_DONE,
	MSM_VIDC_LOAD_RESOURCES,
	MSM_VIDC_LOAD_RESOURCES_DONE,
	MSM_VIDC_START,
	MSM_VIDC_START_DONE,
	MSM_VIDC_STOP,
	MSM_VIDC_STOP_DONE,
	MSM_VIDC_RELEASE_RESOURCES,
	MSM_VIDC_RELEASE_RESOURCES_DONE,
	MSM_VIDC_CLOSE,
	MSM_VIDC_CLOSE_DONE,
	MSM_VIDC_CORE_UNINIT,
};
#endif
