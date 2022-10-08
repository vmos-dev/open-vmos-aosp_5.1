/*--------------------------------------------------------------------------
Copyright (c) 2010-2013, The Linux Foundation. All rights reserved.

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
#ifndef __OMX_VENC_DEV__
#define __OMX_VENC_DEV__

#include "OMX_Types.h"
#include "OMX_Core.h"
#include "OMX_QCOMExtns.h"
#include "qc_omx_component.h"
#include "omx_video_common.h"
#include <linux/msm_vidc_enc.h>
#include <pthread.h>
#include <linux/videodev2.h>
#include <poll.h>
#define TIMEOUT 5000
#define MAX_RECON_BUFFERS 4

void* async_venc_message_thread (void *);

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
        unsigned venc_set_message_thread_id(pthread_t);
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
        bool venc_max_allowed_bitrate_check(OMX_U32 nTargetBitrate);
        bool venc_get_seq_hdr(void *, unsigned, OMX_U32 *);
        bool venc_loaded_start(void);
        bool venc_loaded_stop(void);
        bool venc_loaded_start_done(void);
        bool venc_loaded_stop_done(void);
        bool venc_get_output_log_flag();
        int venc_output_log_buffers(const char *buffer_addr, int buffer_len);
        int venc_input_log_buffers(OMX_BUFFERHEADERTYPE *buffer, void* pmem_data_buf, int framelen);
        int venc_extradata_log_buffers(char *buffer_addr);
        bool venc_get_capability_ltrcount(OMX_U32 *, OMX_U32 *, OMX_U32 *);
        OMX_U32 m_nDriver_fd;
        bool m_profile_set;
        bool m_level_set;
        pthread_mutex_t loaded_start_stop_mlock;
        pthread_cond_t loaded_start_stop_cond;
        struct venc_debug_cap m_debug;

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
        int m_eProfile;
        int m_eLevel;
        int etb_count;
    private:
        struct venc_basecfg             m_sVenc_cfg;
        struct venc_ratectrlcfg         rate_ctrl;
        struct venc_targetbitrate       bitrate;
        struct venc_intraperiod         intra_period;
        struct venc_profile             codec_profile;
        struct ven_profilelevel         profile_level;
        struct venc_switch              set_param;
        struct venc_voptimingcfg        time_inc;
        struct venc_allocatorproperty   m_sInput_buff_property;
        struct venc_allocatorproperty   m_sOutput_buff_property;
        struct venc_sessionqp           session_qp;
        struct venc_qprange             qp_range;
        struct venc_multiclicecfg       multislice;
        struct venc_entropycfg          entropy;
        struct venc_dbcfg               dbkfilter;
        struct venc_intrarefresh        intra_refresh;
        struct venc_headerextension     hec;
        struct venc_voptimingcfg        voptimecfg;
        struct venc_seqheader           seqhdr;
        struct venc_ltrmode             ltrmode;
        struct venc_ltrcount            ltrcount;
        struct venc_ltrperiod           ltrperiod;

        bool venc_set_profile_level(OMX_U32 eProfile,OMX_U32 eLevel);
        bool venc_set_intra_period(OMX_U32 nPFrames, OMX_U32 nBFrames);
        bool venc_set_target_bitrate(OMX_U32 nTargetBitrate, OMX_U32 config);
        bool venc_set_ratectrl_cfg(OMX_VIDEO_CONTROLRATETYPE eControlRate);
        bool venc_set_qp_range(OMX_U32 min_qp, OMX_U32 max_qp);
        bool venc_set_session_qp(OMX_U32 i_frame_qp, OMX_U32 p_frame_qp);
        bool venc_set_extradata(OMX_U32 extra_data);
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
        bool venc_set_slice_delivery_mode(OMX_BOOL enable);
        bool venc_set_plusptype(OMX_BOOL enable);
        bool venc_set_ltrmode(QOMX_VIDEO_LTRMODETYPE mode);
        bool venc_set_ltrcount(OMX_U32 count);
        bool venc_set_ltrperiod(OMX_U32 period);
        bool venc_set_ltruse(OMX_U32 id, OMX_U32 frames);
        bool venc_color_align(OMX_BUFFERHEADERTYPE *buffer, OMX_U32 width, OMX_U32 height);
#ifdef MAX_RES_1080P
        OMX_U32 pmem_free();
        OMX_U32 pmem_allocate(OMX_U32 size, OMX_U32 alignment, OMX_U32 count);
        OMX_U32 venc_allocate_recon_buffers();
        inline int clip2(int x) {
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

#endif
