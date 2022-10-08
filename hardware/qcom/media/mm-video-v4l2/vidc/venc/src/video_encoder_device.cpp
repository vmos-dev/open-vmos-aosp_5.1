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
#include<string.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include<unistd.h>
#include <fcntl.h>
#include "video_encoder_device.h"
#include "omx_video_encoder.h"
#ifdef USE_ION
#include <linux/msm_ion.h>
#endif

#define MPEG4_SP_START 0
#define MPEG4_ASP_START (MPEG4_SP_START + 8)
#define MPEG4_720P_LEVEL 6
#define H263_BP_START 0
#define H264_BP_START 0
#define H264_HP_START (H264_BP_START + 13)
#define H264_MP_START (H264_BP_START + 26)

/* MPEG4 profile and level table*/
static const unsigned int mpeg4_profile_level_table[][5]= {
    /*max mb per frame, max mb per sec, max bitrate, level, profile*/
    {99,1485,64000,OMX_VIDEO_MPEG4Level0,OMX_VIDEO_MPEG4ProfileSimple},
    {99,1485,64000,OMX_VIDEO_MPEG4Level1,OMX_VIDEO_MPEG4ProfileSimple},
    {396,5940,128000,OMX_VIDEO_MPEG4Level2,OMX_VIDEO_MPEG4ProfileSimple},
    {396,11880,384000,OMX_VIDEO_MPEG4Level3,OMX_VIDEO_MPEG4ProfileSimple},
    {1200,36000,4000000,OMX_VIDEO_MPEG4Level4a,OMX_VIDEO_MPEG4ProfileSimple},
    {1620,40500,8000000,OMX_VIDEO_MPEG4Level5,OMX_VIDEO_MPEG4ProfileSimple},
    {3600,108000,12000000,OMX_VIDEO_MPEG4Level5,OMX_VIDEO_MPEG4ProfileSimple},
    {0,0,0,0,0},

    {99,1485,128000,OMX_VIDEO_MPEG4Level0,OMX_VIDEO_MPEG4ProfileAdvancedSimple},
    {99,1485,128000,OMX_VIDEO_MPEG4Level1,OMX_VIDEO_MPEG4ProfileAdvancedSimple},
    {396,5940,384000,OMX_VIDEO_MPEG4Level2,OMX_VIDEO_MPEG4ProfileAdvancedSimple},
    {396,11880,768000,OMX_VIDEO_MPEG4Level3,OMX_VIDEO_MPEG4ProfileAdvancedSimple},
    {792,23760,3000000,OMX_VIDEO_MPEG4Level4,OMX_VIDEO_MPEG4ProfileAdvancedSimple},
    {1620,48600,8000000,OMX_VIDEO_MPEG4Level5,OMX_VIDEO_MPEG4ProfileAdvancedSimple},
    {0,0,0,0,0},
};

/* H264 profile and level table*/
static const unsigned int h264_profile_level_table[][5]= {
    /*max mb per frame, max mb per sec, max bitrate, level, profile*/
    {99,1485,64000,OMX_VIDEO_AVCLevel1,OMX_VIDEO_AVCProfileBaseline},
    {99,1485,128000,OMX_VIDEO_AVCLevel1b,OMX_VIDEO_AVCProfileBaseline},
    {396,3000,192000,OMX_VIDEO_AVCLevel11,OMX_VIDEO_AVCProfileBaseline},
    {396,6000,384000,OMX_VIDEO_AVCLevel12,OMX_VIDEO_AVCProfileBaseline},
    {396,11880,768000,OMX_VIDEO_AVCLevel13,OMX_VIDEO_AVCProfileBaseline},
    {396,11880,2000000,OMX_VIDEO_AVCLevel2,OMX_VIDEO_AVCProfileBaseline},
    {792,19800,4000000,OMX_VIDEO_AVCLevel21,OMX_VIDEO_AVCProfileBaseline},
    {1620,20250,4000000,OMX_VIDEO_AVCLevel22,OMX_VIDEO_AVCProfileBaseline},
    {1620,40500,10000000,OMX_VIDEO_AVCLevel3,OMX_VIDEO_AVCProfileBaseline},
    {3600,108000,14000000,OMX_VIDEO_AVCLevel31,OMX_VIDEO_AVCProfileBaseline},
    {5120,216000,20000000,OMX_VIDEO_AVCLevel32,OMX_VIDEO_AVCProfileBaseline},
    {8192,245760,20000000,OMX_VIDEO_AVCLevel4,OMX_VIDEO_AVCProfileBaseline},
    {0,0,0,0,0},

    {99,1485,64000,OMX_VIDEO_AVCLevel1,OMX_VIDEO_AVCProfileHigh},
    {99,1485,160000,OMX_VIDEO_AVCLevel1b,OMX_VIDEO_AVCProfileHigh},
    {396,3000,240000,OMX_VIDEO_AVCLevel11,OMX_VIDEO_AVCProfileHigh},
    {396,6000,480000,OMX_VIDEO_AVCLevel12,OMX_VIDEO_AVCProfileHigh},
    {396,11880,960000,OMX_VIDEO_AVCLevel13,OMX_VIDEO_AVCProfileHigh},
    {396,11880,2500000,OMX_VIDEO_AVCLevel2,OMX_VIDEO_AVCProfileHigh},
    {792,19800,5000000,OMX_VIDEO_AVCLevel21,OMX_VIDEO_AVCProfileHigh},
    {1620,20250,5000000,OMX_VIDEO_AVCLevel22,OMX_VIDEO_AVCProfileHigh},
    {1620,40500,12500000,OMX_VIDEO_AVCLevel3,OMX_VIDEO_AVCProfileHigh},
    {3600,108000,17500000,OMX_VIDEO_AVCLevel31,OMX_VIDEO_AVCProfileHigh},
    {5120,216000,25000000,OMX_VIDEO_AVCLevel32,OMX_VIDEO_AVCProfileHigh},
    {8192,245760,25000000,OMX_VIDEO_AVCLevel4,OMX_VIDEO_AVCProfileHigh},
    {0,0,0,0,0},

    {99,1485,64000,OMX_VIDEO_AVCLevel1,OMX_VIDEO_AVCProfileMain},
    {99,1485,128000,OMX_VIDEO_AVCLevel1b,OMX_VIDEO_AVCProfileMain},
    {396,3000,192000,OMX_VIDEO_AVCLevel11,OMX_VIDEO_AVCProfileMain},
    {396,6000,384000,OMX_VIDEO_AVCLevel12,OMX_VIDEO_AVCProfileMain},
    {396,11880,768000,OMX_VIDEO_AVCLevel13,OMX_VIDEO_AVCProfileMain},
    {396,11880,2000000,OMX_VIDEO_AVCLevel2,OMX_VIDEO_AVCProfileMain},
    {792,19800,4000000,OMX_VIDEO_AVCLevel21,OMX_VIDEO_AVCProfileMain},
    {1620,20250,4000000,OMX_VIDEO_AVCLevel22,OMX_VIDEO_AVCProfileMain},
    {1620,40500,10000000,OMX_VIDEO_AVCLevel3,OMX_VIDEO_AVCProfileMain},
    {3600,108000,14000000,OMX_VIDEO_AVCLevel31,OMX_VIDEO_AVCProfileMain},
    {5120,216000,20000000,OMX_VIDEO_AVCLevel32,OMX_VIDEO_AVCProfileMain},
    {8192,245760,20000000,OMX_VIDEO_AVCLevel4,OMX_VIDEO_AVCProfileMain},
    {0,0,0,0,0}

};

/* H263 profile and level table*/
static const unsigned int h263_profile_level_table[][5]= {
    /*max mb per frame, max mb per sec, max bitrate, level, profile*/
    {99,1485,64000,OMX_VIDEO_H263Level10,OMX_VIDEO_H263ProfileBaseline},
    {396,5940,128000,OMX_VIDEO_H263Level20,OMX_VIDEO_H263ProfileBaseline},
    {396,11880,384000,OMX_VIDEO_H263Level30,OMX_VIDEO_H263ProfileBaseline},
    {396,11880,2048000,OMX_VIDEO_H263Level40,OMX_VIDEO_H263ProfileBaseline},
    {99,1485,128000,OMX_VIDEO_H263Level45,OMX_VIDEO_H263ProfileBaseline},
    {396,19800,4096000,OMX_VIDEO_H263Level50,OMX_VIDEO_H263ProfileBaseline},
    {810,40500,8192000,OMX_VIDEO_H263Level60,OMX_VIDEO_H263ProfileBaseline},
    {1620,81000,16384000,OMX_VIDEO_H263Level70,OMX_VIDEO_H263ProfileBaseline},
    {0,0,0,0,0}
};

#define Log2(number, power)  { OMX_U32 temp = number; power = 0; while( (0 == (temp & 0x1)) &&  power < 16) { temp >>=0x1; power++; } }
#define Q16ToFraction(q,num,den) { OMX_U32 power; Log2(q,power);  num = q >> power; den = 0x1 << (16 - power); }

#define BUFFER_LOG_LOC "/data/misc/media"

//constructor
venc_dev::venc_dev(class omx_venc *venc_class)
{
    m_max_allowed_bitrate_check = false;
    m_eLevel = 0;
    m_eProfile = 0;
    pthread_mutex_init(&loaded_start_stop_mlock, NULL);
    pthread_cond_init (&loaded_start_stop_cond, NULL);
    memset(&m_debug,0,sizeof(m_debug));

    char property_value[PROPERTY_VALUE_MAX] = {0};
    property_value[0] = '\0';
    property_get("vidc.enc.log.in", property_value, "0");
    m_debug.in_buffer_log = atoi(property_value);

    property_value[0] = '\0';
    property_get("vidc.enc.log.out", property_value, "0");
    m_debug.out_buffer_log = atoi(property_value);
    snprintf(m_debug.log_loc, PROPERTY_VAL_MAX,
             "%s", BUFFER_LOG_LOC);

    DEBUG_PRINT_LOW("venc_dev constructor");
}

venc_dev::~venc_dev()
{
    pthread_cond_destroy(&loaded_start_stop_cond);
    pthread_mutex_destroy(&loaded_start_stop_mlock);
    DEBUG_PRINT_LOW("venc_dev distructor");
}

void* async_venc_message_thread (void *input)
{
    struct venc_ioctl_msg ioctl_msg ={NULL,NULL};
    struct venc_timeout timeout;
    struct venc_msg venc_msg;
    int error_code = 0;
    omx_venc *omx = reinterpret_cast<omx_venc*>(input);

    prctl(PR_SET_NAME, (unsigned long)"VideoEncCallBackThread", 0, 0, 0);
    timeout.millisec = VEN_TIMEOUT_INFINITE;

    while (1) {
        ioctl_msg.in = NULL;
        ioctl_msg.out = (void*)&venc_msg;

        /*Wait for a message from the video decoder driver*/
        error_code = ioctl(omx->handle->m_nDriver_fd,VEN_IOCTL_CMD_READ_NEXT_MSG,(void *)&ioctl_msg);

        if (error_code == -512) { // ERESTARTSYS
            DEBUG_PRINT_ERROR("ERESTARTSYS received in ioctl read next msg!");
        } else if (error_code <0) {
            DEBUG_PRINT_LOW("ioctl VEN_IOCTL_CMD_READ_NEXT_MSG failed");
            break;
        } else if (omx->async_message_process(input,&venc_msg) < 0) {
            DEBUG_PRINT_ERROR("ERROR: Wrong ioctl message");
            break;
        }
    }
    DEBUG_PRINT_HIGH("omx_venc: Async Thread exit");
    return NULL;
}

int venc_dev::venc_extradata_log_buffers(char *buffer_addr)
{
    return OMX_ErrorUnsupportedSetting;
}

int venc_dev::venc_output_log_buffers(const char *buffer_addr, int buffer_len)
{
    if (m_debug.out_buffer_log && !m_debug.outfile) {
        int size = 0;
        if(m_sVenc_cfg.codectype == VEN_CODEC_MPEG4) {
           size = snprintf(m_debug.outfile_name, PROPERTY_VALUE_MAX, "%s/output_enc_%d_%d_%p.m4v",
                           m_debug.log_loc, m_sVenc_cfg.input_width, m_sVenc_cfg.input_height, this);
        } else if(m_sVenc_cfg.codectype == VEN_CODEC_H264) {
           size = snprintf(m_debug.outfile_name, PROPERTY_VALUE_MAX, "%s/output_enc_%d_%d_%p.264",
                           m_debug.log_loc, m_sVenc_cfg.input_width, m_sVenc_cfg.input_height, this);
        } else if(m_sVenc_cfg.codectype == VENC_CODEC_H263) {
           size = snprintf(m_debug.outfile_name, PROPERTY_VALUE_MAX, "%s/output_enc_%d_%d_%p.263",
                           m_debug.log_loc, m_sVenc_cfg.input_width, m_sVenc_cfg.input_height, this);
        } else if(m_sVenc_cfg.codectype == VENC_CODEC_VP8) {
           size = snprintf(m_debug.outfile_name, PROPERTY_VALUE_MAX, "%s/output_enc_%d_%d_%p.ivf",
                           m_debug.log_loc, m_sVenc_cfg.input_width, m_sVenc_cfg.input_height, this);
        }
        if ((size > PROPERTY_VALUE_MAX) || (size < 0)) {
            DEBUG_PRINT_ERROR("Failed to open output file: %s for logging as size:%d",
                               m_debug.outfile_name, size);
            return -1;
        }
        m_debug.outfile = fopen(m_debug.outfile_name, "ab");
        if (!m_debug.outfile) {
            DEBUG_PRINT_ERROR("Failed to open output file: %s for logging errno:%d",
                              m_debug.outfile_name, errno);
            m_debug.outfile_name[0] = '\0';
            return -1;
        }
    }
    if (m_debug.outfile && buffer_len) {
        DEBUG_PRINT_LOW("%s buffer_len:%d", __func__, buffer_len);
        fwrite(buffer_addr, buffer_len, 1, m_debug.outfile);
    }
    return 0;
}

bool venc_dev::venc_get_output_log_flag()
{
    return (m_debug.out_buffer_log == 1);
}

int venc_dev::venc_input_log_buffers(OMX_BUFFERHEADERTYPE *pbuffer, void *pmem_data_buf, int framelen) {
    if (!m_debug.infile) {
        int size = snprintf(m_debug.infile_name, PROPERTY_VALUE_MAX, "%s/input_enc_%d_%d_%p.yuv",
                           m_debug.log_loc, m_sVenc_cfg.input_width,
                           m_sVenc_cfg.input_height, this);
        if ((size > PROPERTY_VALUE_MAX) || (size < 0)) {
            DEBUG_PRINT_ERROR("Failed to open input file: %s for logging size:%d",
                               m_debug.infile_name, size);
            return -1;
        }
        m_debug.infile = fopen (m_debug.infile_name, "ab");
        if (!m_debug.infile) {
            DEBUG_PRINT_HIGH("Failed to open input file: %s for logging", m_debug.infile);
            m_debug.infile_name[0] = '\0';
            return -1;
        }
    }
    if (m_debug.infile && pbuffer && pbuffer->nFilledLen) {
#ifdef MAX_RES_1080P
       int y_size = 0;
       int c_offset = 0;
       unsigned char *buf_addr = NULL;

       y_size = m_sVenc_cfg.input_width * m_sVenc_cfg.input_height;
       //chroma offset is y_size aligned to the 2k boundary
       c_offset= (y_size + 2047) & (~(2047));

       if (pmem_data_buf) {
           DEBUG_PRINT_LOW("Internal PMEM addr for i/p Heap UseBuf: %p", pmem_data_buf);
           buf_addr = (OMX_U8 *)pmem_data_buf;
       } else {
           DEBUG_PRINT_LOW("Shared PMEM addr for i/p PMEM UseBuf/AllocateBuf: %p", bufhdr->pBuffer);
           buf_addr = (unsigned char *)mmap(NULL,
                      ((encoder_media_buffer_type *)pbuffer->pBuffer)->meta_handle->data[2],
                      PROT_READ|PROT_WRITE, MAP_SHARED,
                      ((encoder_media_buffer_type *)pbuffer->pBuffer)->meta_handle->data[0], 0);
       }

       if (m_debug.infile) {
           fwrite((const char *)buf_addr, y_size, 1, m_debug.infile);
           fwrite((const char *)(buf_addr + c_offset), (y_size>>1), 1, m_debug.infile);
       }

       if (!pmem_data_buf) {
           munmap (buf_addr, ((encoder_media_buffer_type *)pbuffer->pBuffer)->meta_handle->data[2]);
       }
#else
       if (m_debug.infile) {
           OMX_U8* ptrbuffer = NULL;
           if (pmem_data_buf) {
               DEBUG_PRINT_LOW("Internal PMEM addr for i/p Heap UseBuf: %p", pmem_data_buf);
               ptrbuffer = (OMX_U8 *)pmem_data_buf;
           } else {
               DEBUG_PRINT_LOW("Shared PMEM addr for i/p PMEM UseBuf/AllocateBuf: %p", bufhdr->pBuffer);
               ptrbuffer = (OMX_U8 *)bufhdr->pBuffer;
           }
           fwrite((const char *)ptrbuffer, framelen, 1, m_debug.infile);
       }

#endif
    }
    return 0;
}

bool venc_dev::venc_open(OMX_U32 codec)
{
    struct venc_ioctl_msg ioctl_msg = {NULL,NULL};
    int r;
    unsigned int   alignment = 0,buffer_size = 0, temp =0;

    m_nDriver_fd = open ("/dev/msm_vidc_enc",O_RDWR|O_NONBLOCK);

    if (m_nDriver_fd == 0) {
        DEBUG_PRINT_ERROR("ERROR: Got fd as 0 for msm_vidc_enc, Opening again");
        m_nDriver_fd = open ("/dev/msm_vidc_enc",O_RDWR|O_NONBLOCK);
    }

    if ((int)m_nDriver_fd < 0) {
        DEBUG_PRINT_ERROR("ERROR: Omx_venc::Comp Init Returning failure");
        return false;
    }

    DEBUG_PRINT_LOW("m_nDriver_fd = %d", m_nDriver_fd);
#ifdef SINGLE_ENCODER_INSTANCE
    OMX_U32 num_instances = 0;
    ioctl_msg.in = NULL;
    ioctl_msg.out = &num_instances;

    if (ioctl (m_nDriver_fd, VEN_IOCTL_GET_NUMBER_INSTANCES, (void*)&ioctl_msg) < 0 ) {
        DEBUG_PRINT_ERROR("ERROR: Request number of encoder instances failed");
    } else if (num_instances > 1) {
        DEBUG_PRINT_ERROR("Second encoder instance rejected!");
        venc_close();
        return false;
    }

#endif
    // set the basic configuration of the video encoder driver
    m_sVenc_cfg.input_width = OMX_CORE_QCIF_WIDTH;
    m_sVenc_cfg.input_height= OMX_CORE_QCIF_HEIGHT;
    m_sVenc_cfg.dvs_width = OMX_CORE_QCIF_WIDTH;
    m_sVenc_cfg.dvs_height = OMX_CORE_QCIF_HEIGHT;
    m_sVenc_cfg.fps_num = 30;
    m_sVenc_cfg.fps_den = 1;
    m_sVenc_cfg.targetbitrate = 64000;
#ifdef MAX_RES_1080P
    m_sVenc_cfg.inputformat= VEN_INPUTFMT_NV12_16M2KA;
#else
    m_sVenc_cfg.inputformat= VEN_INPUTFMT_NV12;
#endif
    // initializing QP range parameters
    qp_range.minqp = 2;

    if (codec == OMX_VIDEO_CodingAVC)
        qp_range.maxqp = 51;
    else
        qp_range.maxqp = 31;

    if (codec == OMX_VIDEO_CodingMPEG4) {
        m_sVenc_cfg.codectype = VEN_CODEC_MPEG4;
        codec_profile.profile = VEN_PROFILE_MPEG4_SP;
        profile_level.level = VEN_LEVEL_MPEG4_2;
    } else if (codec == OMX_VIDEO_CodingH263) {
        m_sVenc_cfg.codectype = VEN_CODEC_H263;
        codec_profile.profile = VEN_PROFILE_H263_BASELINE;
        profile_level.level = VEN_LEVEL_H263_20;
    }

    if (codec == OMX_VIDEO_CodingAVC) {
        m_sVenc_cfg.codectype = VEN_CODEC_H264;
        codec_profile.profile = VEN_PROFILE_H264_BASELINE;
        profile_level.level = VEN_LEVEL_H264_1p1;
    }

    ioctl_msg.in = (void*)&m_sVenc_cfg;
    ioctl_msg.out = NULL;

    if (ioctl (m_nDriver_fd,VEN_IOCTL_SET_BASE_CFG,(void*)&ioctl_msg) < 0 ) {
        DEBUG_PRINT_ERROR("ERROR: Request for setting base configuration failed");
        return false;
    }

    // Get the I/P and O/P buffer requirements
    ioctl_msg.in = NULL;
    ioctl_msg.out = (void*)&m_sInput_buff_property;

    if (ioctl (m_nDriver_fd,VEN_IOCTL_GET_INPUT_BUFFER_REQ,(void*)&ioctl_msg) < 0) {
        DEBUG_PRINT_ERROR("ERROR: Request for getting i/p buffer requirement failed");
        return false;
    }

    ioctl_msg.in = NULL;
    ioctl_msg.out = (void*)&m_sOutput_buff_property;

    if (ioctl (m_nDriver_fd,VEN_IOCTL_GET_OUTPUT_BUFFER_REQ,(void*)&ioctl_msg) < 0) {
        DEBUG_PRINT_ERROR("ERROR: Request for getting o/p buffer requirement failed");
        return false;
    }

    m_profile_set = false;
    m_level_set = false;

    if (venc_set_profile_level(0, 0)) {
        DEBUG_PRINT_HIGH("%s(): Init Profile/Level setting success",
                __func__);
    }

    recon_buffers_count = MAX_RECON_BUFFERS;
    ltrmode.ltr_mode = 0;
    ltrcount.ltr_count = 0;
    ltrperiod.ltr_period = 0;

    return true;
}

void venc_dev::venc_close()
{
    DEBUG_PRINT_LOW("venc_close: fd = %d", m_nDriver_fd);

    if ((int)m_nDriver_fd >= 0) {
        DEBUG_PRINT_HIGH("venc_close(): Calling VEN_IOCTL_CMD_STOP_READ_MSG");
        (void)ioctl(m_nDriver_fd, VEN_IOCTL_CMD_STOP_READ_MSG,
                NULL);
        DEBUG_PRINT_LOW("Calling close()");
        close(m_nDriver_fd);
        m_nDriver_fd = -1;
    }

    if (m_debug.infile) {
        fclose(m_debug.infile);
        m_debug.infile = NULL;
    }
    if (m_debug.outfile) {
        fclose(m_debug.outfile);
        m_debug.outfile = NULL;
    }

}

bool venc_dev::venc_set_buf_req(unsigned long *min_buff_count,
        unsigned long *actual_buff_count,
        unsigned long *buff_size,
        unsigned long port)
{
    struct venc_ioctl_msg ioctl_msg = {NULL,NULL};
    unsigned long temp_count = 0;

    if (port == 0) {
        if (*actual_buff_count > m_sInput_buff_property.mincount) {
            temp_count = m_sInput_buff_property.actualcount;
            m_sInput_buff_property.actualcount = *actual_buff_count;
            ioctl_msg.in = (void*)&m_sInput_buff_property;
            ioctl_msg.out = NULL;

            if (ioctl (m_nDriver_fd,VEN_IOCTL_SET_INPUT_BUFFER_REQ,(void*)&ioctl_msg) < 0) {
                DEBUG_PRINT_ERROR("ERROR: Request for setting i/p buffer requirement failed");
                m_sInput_buff_property.actualcount = temp_count;
                return false;
            }

            DEBUG_PRINT_LOW("I/P Count set to %lu", *actual_buff_count);
        }
    } else {
        if (*actual_buff_count > m_sOutput_buff_property.mincount) {
            temp_count = m_sOutput_buff_property.actualcount;
            m_sOutput_buff_property.actualcount = *actual_buff_count;
            ioctl_msg.in = (void*)&m_sOutput_buff_property;
            ioctl_msg.out = NULL;

            if (ioctl (m_nDriver_fd,VEN_IOCTL_SET_OUTPUT_BUFFER_REQ,(void*)&ioctl_msg) < 0) {
                DEBUG_PRINT_ERROR("ERROR: Request for setting o/p buffer requirement failed");
                m_sOutput_buff_property.actualcount = temp_count;
                return false;
            }

            DEBUG_PRINT_LOW("O/P Count set to %lu", *actual_buff_count);
        }
    }

    return true;

}

bool venc_dev::venc_loaded_start()
{
    struct timespec ts;
    int status = 0;

    if (ioctl (m_nDriver_fd,VEN_IOCTL_CMD_START, NULL) < 0) {
        DEBUG_PRINT_ERROR("ERROR: VEN_IOCTL_CMD_START failed");
        return false;
    }

    if (clock_gettime(CLOCK_REALTIME, &ts) < 0) {
        DEBUG_PRINT_ERROR("%s: clock_gettime failed", __func__);
        return false;
    }

    ts.tv_sec += 1;
    pthread_mutex_lock(&loaded_start_stop_mlock);
    DEBUG_PRINT_LOW("%s: wait on start done", __func__);
    status = pthread_cond_timedwait(&loaded_start_stop_cond,
            &loaded_start_stop_mlock, &ts);

    if (status != 0) {
        DEBUG_PRINT_ERROR("%s: error status = %d, %s", __func__,
                status, strerror(status));
        pthread_mutex_unlock(&loaded_start_stop_mlock);
        return false;
    }

    DEBUG_PRINT_LOW("%s: wait over on start done", __func__);
    pthread_mutex_unlock(&loaded_start_stop_mlock);
    DEBUG_PRINT_LOW("%s: venc_loaded_start success", __func__);
    return true;
}

bool venc_dev::venc_loaded_stop()
{
    struct timespec ts;
    int status = 0;

    if (ioctl (m_nDriver_fd,VEN_IOCTL_CMD_STOP, NULL) < 0) {
        DEBUG_PRINT_ERROR("ERROR: VEN_IOCTL_CMD_STOP failed");
        return false;
    }

    if (clock_gettime(CLOCK_REALTIME, &ts) < 0) {
        DEBUG_PRINT_ERROR("%s: clock_gettime failed", __func__);
        return false;
    }

    ts.tv_sec += 1;
    pthread_mutex_lock(&loaded_start_stop_mlock);
    DEBUG_PRINT_LOW("%s: wait on stop done", __func__);
    status = pthread_cond_timedwait(&loaded_start_stop_cond,
            &loaded_start_stop_mlock, &ts);

    if (status != 0) {
        DEBUG_PRINT_ERROR("%s: error status = %d, %s", __func__,
                status, strerror(status));
        pthread_mutex_unlock(&loaded_start_stop_mlock);
        return false;
    }

    DEBUG_PRINT_LOW("%s: wait over on stop done", __func__);
    pthread_mutex_unlock(&loaded_start_stop_mlock);
    DEBUG_PRINT_LOW("%s: venc_loaded_stop success", __func__);
    return true;
}

bool venc_dev::venc_loaded_start_done()
{
    pthread_mutex_lock(&loaded_start_stop_mlock);
    DEBUG_PRINT_LOW("%s: signal start done", __func__);
    pthread_cond_signal(&loaded_start_stop_cond);
    pthread_mutex_unlock(&loaded_start_stop_mlock);
    return true;
}

bool venc_dev::venc_loaded_stop_done()
{
    pthread_mutex_lock(&loaded_start_stop_mlock);
    DEBUG_PRINT_LOW("%s: signal stop done", __func__);
    pthread_cond_signal(&loaded_start_stop_cond);
    pthread_mutex_unlock(&loaded_start_stop_mlock);
    return true;
}

bool venc_dev::venc_get_seq_hdr(void *buffer,
        unsigned buffer_size, OMX_U32 *header_len)
{
    struct venc_ioctl_msg ioctl_msg = {NULL,NULL};
    int i = 0;
    DEBUG_PRINT_HIGH("venc_dev::venc_get_seq_hdr");
    venc_seqheader seq_in, seq_out;
    seq_in.hdrlen = 0;
    seq_in.bufsize = buffer_size;
    seq_in.hdrbufptr = (unsigned char*)buffer;

    if (seq_in.hdrbufptr == NULL) {
        DEBUG_PRINT_ERROR("ERROR: malloc for sequence header failed");
        return false;
    }

    DEBUG_PRINT_LOW("seq_in: buf=%x, sz=%d, hdrlen=%d", seq_in.hdrbufptr,
            seq_in.bufsize, seq_in.hdrlen);

    ioctl_msg.in = (void*)&seq_in;
    ioctl_msg.out = (void*)&seq_out;

    if (ioctl (m_nDriver_fd,VEN_IOCTL_GET_SEQUENCE_HDR,(void*)&ioctl_msg) < 0) {
        DEBUG_PRINT_ERROR("ERROR: Request for getting sequence header failed");
        return false;
    }

    if (seq_out.hdrlen == 0) {
        DEBUG_PRINT_ERROR("ERROR: Seq header returned zero length header");
        DEBUG_PRINT_ERROR("seq_out: buf=%x, sz=%d, hdrlen=%d", seq_out.hdrbufptr,
                seq_out.bufsize, seq_out.hdrlen);
        return false;
    }

    *header_len = seq_out.hdrlen;
    DEBUG_PRINT_LOW("seq_out: buf=%x, sz=%d, hdrlen=%d", seq_out.hdrbufptr,
            seq_out.bufsize, seq_out.hdrlen);

    return true;
}

bool venc_dev::venc_get_capability_ltrcount(unsigned long *min,
        unsigned long *max, unsigned long *step_size)
{
    struct venc_ioctl_msg ioctl_msg = {NULL,NULL};
    venc_range  cap_ltr_count;
    ioctl_msg.in = NULL;
    ioctl_msg.out = (void*)&cap_ltr_count;

    if (ioctl (m_nDriver_fd, VEN_IOCTL_GET_CAPABILITY_LTRCOUNT,
                (void*)&ioctl_msg) < 0) {
        DEBUG_PRINT_ERROR("ERROR: Get LTR Capability failed");
        return false;
    } else {
        *min = cap_ltr_count.min;
        *max = cap_ltr_count.max;
        *step_size = cap_ltr_count.step_size;
        DEBUG_PRINT_HIGH("LTR Capability: min=%x, max=%d, step_size=%d",
                *min, *max, *step_size);
    }

    return true;
}

bool venc_dev::venc_get_buf_req(unsigned long *min_buff_count,
        unsigned long *actual_buff_count,
        unsigned long *buff_size,
        unsigned long port)
{
    struct venc_ioctl_msg ioctl_msg = {NULL,NULL};

    if (port == 0) {
        ioctl_msg.in = NULL;
        ioctl_msg.out = (void*)&m_sInput_buff_property;

        if (ioctl (m_nDriver_fd,VEN_IOCTL_GET_INPUT_BUFFER_REQ,(void*)&ioctl_msg) < 0) {
            DEBUG_PRINT_ERROR("ERROR: Request for getting i/p buffer requirement failed");
            return false;
        }

        *min_buff_count = m_sInput_buff_property.mincount;
        *actual_buff_count = m_sInput_buff_property.actualcount;
#ifdef USE_ION
        // For ION memory allocations of the allocated buffer size
        // must be 4k aligned, hence aligning the input buffer
        // size to 4k.
        m_sInput_buff_property.datasize = (m_sInput_buff_property.datasize + 4095)
            & (~4095);
#endif
        *buff_size = m_sInput_buff_property.datasize;
    } else {
        ioctl_msg.in = NULL;
        ioctl_msg.out = (void*)&m_sOutput_buff_property;

        if (ioctl (m_nDriver_fd,VEN_IOCTL_GET_OUTPUT_BUFFER_REQ,(void*)&ioctl_msg) < 0) {
            DEBUG_PRINT_ERROR("ERROR: Request for getting o/p buffer requirement failed");
            return false;
        }

        *min_buff_count = m_sOutput_buff_property.mincount;
        *actual_buff_count = m_sOutput_buff_property.actualcount;
        *buff_size = m_sOutput_buff_property.datasize;
    }

    return true;

}

bool venc_dev::venc_set_param(void *paramData,OMX_INDEXTYPE index )
{
    venc_ioctl_msg ioctl_msg = {NULL,NULL};
    DEBUG_PRINT_LOW("venc_set_param:: venc-720p");

    switch (index) {
        case OMX_IndexParamPortDefinition:
            {
                OMX_PARAM_PORTDEFINITIONTYPE *portDefn;
                portDefn = (OMX_PARAM_PORTDEFINITIONTYPE *) paramData;
                DEBUG_PRINT_HIGH("venc_set_param: OMX_IndexParamPortDefinition");

                if (portDefn->nPortIndex == PORT_INDEX_IN) {

                    if (!venc_set_encode_framerate(portDefn->format.video.xFramerate, 0)) {
                        return false;
                    }

                    if (!venc_set_color_format(portDefn->format.video.eColorFormat)) {
                        return false;
                    }

                    DEBUG_PRINT_LOW("Basic parameter has changed");
                    m_sVenc_cfg.input_height = portDefn->format.video.nFrameHeight;
                    m_sVenc_cfg.input_width = portDefn->format.video.nFrameWidth;

                    ioctl_msg.in = (void*)&m_sVenc_cfg;
                    ioctl_msg.out = NULL;

                    if (ioctl (m_nDriver_fd,VEN_IOCTL_SET_BASE_CFG,(void*)&ioctl_msg) < 0) {
                        DEBUG_PRINT_ERROR("ERROR: Request for setting base config failed");
                        return false;
                    }

                    DEBUG_PRINT_HIGH("WxH (%dx%d), codec (%d), fps(nr/dr) (%d/%d), bitrate (%d), "
                            "color_format (%d)", m_sVenc_cfg.input_width, m_sVenc_cfg.input_height,
                            m_sVenc_cfg.codectype, m_sVenc_cfg.fps_num, m_sVenc_cfg.fps_den,
                            m_sVenc_cfg.targetbitrate, m_sVenc_cfg.inputformat);

                    DEBUG_PRINT_LOW("Updating the buffer count/size for the new resolution");
                    ioctl_msg.in = NULL;
                    ioctl_msg.out = (void*)&m_sInput_buff_property;

                    if (ioctl (m_nDriver_fd, VEN_IOCTL_GET_INPUT_BUFFER_REQ,(void*)&ioctl_msg) < 0) {
                        DEBUG_PRINT_ERROR("ERROR: Request for getting i/p bufreq failed");
                        return false;
                    }

                    DEBUG_PRINT_HIGH("Got updated m_sInput_buff_property values: "
                            "datasize = %u, maxcount = %u, actualcnt = %u, "
                            "mincount = %u", m_sInput_buff_property.datasize,
                            m_sInput_buff_property.maxcount, m_sInput_buff_property.actualcount,
                            m_sInput_buff_property.mincount);

                    ioctl_msg.in = NULL;
                    ioctl_msg.out = (void*)&m_sOutput_buff_property;

                    if (ioctl (m_nDriver_fd, VEN_IOCTL_GET_OUTPUT_BUFFER_REQ,(void*)&ioctl_msg) < 0) {
                        DEBUG_PRINT_ERROR("ERROR: Request for getting o/p bufreq failed");
                        return false;
                    }

                    DEBUG_PRINT_HIGH("Got updated m_sOutput_buff_property values: "
                            "datasize = %u, maxcount = %u, actualcnt = %u, "
                            "mincount = %u", m_sOutput_buff_property.datasize,
                            m_sOutput_buff_property.maxcount, m_sOutput_buff_property.actualcount,
                            m_sOutput_buff_property.mincount);
                    ioctl_msg.in = (void*)&m_sOutput_buff_property;
                    ioctl_msg.out = NULL;

                    if (ioctl (m_nDriver_fd, VEN_IOCTL_SET_OUTPUT_BUFFER_REQ,(void*)&ioctl_msg) < 0) {
                        DEBUG_PRINT_ERROR("ERROR: Request for setting o/p bufreq failed");
                        return false;
                    }

                    if ((portDefn->nBufferCountActual >= m_sInput_buff_property.mincount) &&
                            (portDefn->nBufferCountActual <= m_sInput_buff_property.maxcount)) {
                        m_sInput_buff_property.actualcount = portDefn->nBufferCountActual;
                        ioctl_msg.in = (void*)&m_sInput_buff_property;
                        ioctl_msg.out = NULL;

                        if (ioctl(m_nDriver_fd,VEN_IOCTL_SET_INPUT_BUFFER_REQ,(void*)&ioctl_msg) < 0) {
                            DEBUG_PRINT_ERROR("ERROR: Request for setting i/p buffer requirements failed");
                            return false;
                        }
                    }

                    if (m_sInput_buff_property.datasize != portDefn->nBufferSize) {
                        DEBUG_PRINT_ERROR("WARNING: Requested i/p bufsize[%u],"
                                "Driver's updated i/p bufsize = %u", portDefn->nBufferSize,
                                m_sInput_buff_property.datasize);
                    }

                    m_level_set = false;

                    if (venc_set_profile_level(0, 0)) {
                        DEBUG_PRINT_LOW("%s(): Profile/Level setting success", __func__);
                    }
                } else if (portDefn->nPortIndex == PORT_INDEX_OUT) {
                    if (!venc_set_target_bitrate(portDefn->format.video.nBitrate, 0)) {
                        return false;
                    }

                    if ( (portDefn->nBufferCountActual >= m_sOutput_buff_property.mincount)
                            &&
                            (m_sOutput_buff_property.maxcount >= portDefn->nBufferCountActual)
                            &&
                            (m_sOutput_buff_property.datasize == portDefn->nBufferSize)
                       ) {
                        m_sOutput_buff_property.actualcount = portDefn->nBufferCountActual;
                        ioctl_msg.in = (void*)&m_sOutput_buff_property;
                        ioctl_msg.out = NULL;

                        if (ioctl (m_nDriver_fd,VEN_IOCTL_SET_OUTPUT_BUFFER_REQ,(void*)&ioctl_msg) < 0) {
                            DEBUG_PRINT_ERROR("ERROR: ioctl VEN_IOCTL_SET_OUTPUT_BUFFER_REQ failed");
                            return false;
                        }
                    } else {
                        DEBUG_PRINT_ERROR("ERROR: Setting Output buffer requirements failed");
                        return false;
                    }
                } else {
                    DEBUG_PRINT_ERROR("ERROR: Invalid Port Index for OMX_IndexParamPortDefinition");
                }

                break;
            }
        case OMX_IndexParamVideoPortFormat:
            {
                OMX_VIDEO_PARAM_PORTFORMATTYPE *portFmt;
                portFmt =(OMX_VIDEO_PARAM_PORTFORMATTYPE *)paramData;
                DEBUG_PRINT_LOW("venc_set_param: OMX_IndexParamVideoPortFormat");

                if (portFmt->nPortIndex == (OMX_U32) PORT_INDEX_IN) {
                    if (!venc_set_color_format(portFmt->eColorFormat)) {
                        return false;
                    }
                } else if (portFmt->nPortIndex == (OMX_U32) PORT_INDEX_OUT) {
                    if (!venc_set_encode_framerate(portFmt->xFramerate, 0)) {
                        return false;
                    }
                } else {
                    DEBUG_PRINT_ERROR("ERROR: Invalid Port Index for OMX_IndexParamVideoPortFormat");
                }

                break;
            }
        case OMX_IndexParamVideoBitrate:
            {
                OMX_VIDEO_PARAM_BITRATETYPE* pParam;
                pParam = (OMX_VIDEO_PARAM_BITRATETYPE*)paramData;
                DEBUG_PRINT_LOW("venc_set_param: OMX_IndexParamVideoBitrate");

                if (pParam->nPortIndex == (OMX_U32) PORT_INDEX_OUT) {
                    if (!venc_set_target_bitrate(pParam->nTargetBitrate, 0)) {
                        DEBUG_PRINT_ERROR("ERROR: Target Bit Rate setting failed");
                        return false;
                    }

                    if (!venc_set_ratectrl_cfg(pParam->eControlRate)) {
                        DEBUG_PRINT_ERROR("ERROR: Rate Control setting failed");
                        return false;
                    }
                } else {
                    DEBUG_PRINT_ERROR("ERROR: Invalid Port Index for OMX_IndexParamVideoBitrate");
                }

                break;
            }
        case OMX_IndexParamVideoMpeg4:
            {
                OMX_VIDEO_PARAM_MPEG4TYPE* pParam;
                OMX_U32 bFrames = 0;

                pParam = (OMX_VIDEO_PARAM_MPEG4TYPE*)paramData;
                DEBUG_PRINT_LOW("venc_set_param: OMX_IndexParamVideoMpeg4");

                if (pParam->nPortIndex == (OMX_U32) PORT_INDEX_OUT) {
                    if (!venc_set_voptiming_cfg(pParam->nTimeIncRes)) {
                        DEBUG_PRINT_ERROR("ERROR: Request for setting vop_timing failed");
                        return false;
                    }

                    m_profile_set = false;
                    m_level_set = false;

                    if (!venc_set_profile_level (pParam->eProfile, pParam->eLevel)) {
                        DEBUG_PRINT_ERROR("ERROR: Unsuccessful in updating Profile and level");
                        return false;
                    }

#ifdef MAX_RES_1080P
                    else {
                        if (pParam->eProfile == OMX_VIDEO_MPEG4ProfileAdvancedSimple) {
                            if (pParam->nBFrames) {
                                DEBUG_PRINT_HIGH("INFO: Only 1 Bframe is supported");
                                bFrames = 1;
                            }
                        } else {
                            if (pParam->nBFrames) {
                                DEBUG_PRINT_ERROR("Warning: B frames not supported");
                                bFrames = 0;
                            }
                        }
                    }

#endif

                    if (!venc_set_intra_period (pParam->nPFrames,bFrames)) {
                        DEBUG_PRINT_ERROR("ERROR: Request for setting intra period failed");
                        return false;
                    }

                    if (!venc_set_multislice_cfg(OMX_IndexParamVideoMpeg4,pParam->nSliceHeaderSpacing)) {
                        DEBUG_PRINT_ERROR("ERROR: Unsuccessful in updating slice_config");
                        return false;
                    }
                } else {
                    DEBUG_PRINT_ERROR("ERROR: Invalid Port Index for OMX_IndexParamVideoMpeg4");
                }

                break;
            }
        case OMX_IndexParamVideoH263:
            {
                OMX_VIDEO_PARAM_H263TYPE* pParam = (OMX_VIDEO_PARAM_H263TYPE*)paramData;
                DEBUG_PRINT_LOW("venc_set_param: OMX_IndexParamVideoH263");
                OMX_U32 bFrames = 0;

                if (pParam->nPortIndex == (OMX_U32) PORT_INDEX_OUT) {
                    m_profile_set = false;
                    m_level_set = false;

                    if (!venc_set_profile_level (pParam->eProfile, pParam->eLevel)) {
                        DEBUG_PRINT_ERROR("ERROR: Unsuccessful in updating Profile and level");
                        return false;
                    }

                    if (pParam->nBFrames)
                        DEBUG_PRINT_ERROR("WARNING: B frame not supported for H.263");

                    if (venc_set_intra_period (pParam->nPFrames, bFrames) == false) {
                        DEBUG_PRINT_ERROR("ERROR: Request for setting intra period failed");
                        return false;
                    }
                } else {
                    DEBUG_PRINT_ERROR("ERROR: Invalid Port Index for OMX_IndexParamVideoH263");
                }

                break;
            }
        case OMX_IndexParamVideoAvc:
            {
                DEBUG_PRINT_LOW("venc_set_param:OMX_IndexParamVideoAvc");
                OMX_VIDEO_PARAM_AVCTYPE* pParam = (OMX_VIDEO_PARAM_AVCTYPE*)paramData;
                OMX_U32 bFrames = 0;

                if (pParam->nPortIndex == (OMX_U32) PORT_INDEX_OUT) {
                    DEBUG_PRINT_LOW("pParam->eProfile :%d ,pParam->eLevel %d",
                            pParam->eProfile,pParam->eLevel);

                    m_profile_set = false;
                    m_level_set = false;

                    if (!venc_set_profile_level (pParam->eProfile,pParam->eLevel)) {
                        DEBUG_PRINT_ERROR("ERROR: Unsuccessful in updating Profile and level %d, %d",
                                pParam->eProfile, pParam->eLevel);
                        return false;
                    }

#ifdef MAX_RES_1080P
                    else {
                        if (pParam->eProfile != OMX_VIDEO_AVCProfileBaseline) {
                            if (pParam->nBFrames) {
                                DEBUG_PRINT_HIGH("INFO: Only 1 Bframe is supported");
                                bFrames = 1;
                            }
                        } else {
                            if (pParam->nBFrames) {
                                DEBUG_PRINT_ERROR("Warning: B frames not supported");
                                bFrames = 0;
                            }
                        }
                    }

#endif

                    if (!venc_set_intra_period (pParam->nPFrames, bFrames)) {
                        DEBUG_PRINT_ERROR("ERROR: Request for setting intra period failed");
                        return false;
                    }

                    if (!venc_set_entropy_config (pParam->bEntropyCodingCABAC, pParam->nCabacInitIdc)) {
                        DEBUG_PRINT_ERROR("ERROR: Request for setting Entropy failed");
                        return false;
                    }

                    if (!venc_set_inloop_filter (pParam->eLoopFilterMode)) {
                        DEBUG_PRINT_ERROR("ERROR: Request for setting Inloop filter failed");
                        return false;
                    }

                    if (!venc_set_multislice_cfg(OMX_IndexParamVideoAvc, pParam->nSliceHeaderSpacing)) {
                        DEBUG_PRINT_ERROR("WARNING: Unsuccessful in updating slice_config");
                        return false;
                    }
                } else {
                    DEBUG_PRINT_ERROR("ERROR: Invalid Port Index for OMX_IndexParamVideoAvc");
                }

                //TBD, lot of other variables to be updated, yet to decide
                break;
            }
        case OMX_IndexParamVideoIntraRefresh:
            {
                DEBUG_PRINT_LOW("venc_set_param:OMX_IndexParamVideoIntraRefresh");
                OMX_VIDEO_PARAM_INTRAREFRESHTYPE *intra_refresh =
                    (OMX_VIDEO_PARAM_INTRAREFRESHTYPE *)paramData;

                if (intra_refresh->nPortIndex == (OMX_U32) PORT_INDEX_OUT) {
                    if (venc_set_intra_refresh(intra_refresh->eRefreshMode, intra_refresh->nCirMBs) == false) {
                        DEBUG_PRINT_ERROR("ERROR: Setting Intra refresh failed");
                        return false;
                    }
                } else {
                    DEBUG_PRINT_ERROR("ERROR: Invalid Port Index for OMX_IndexParamVideoIntraRefresh");
                }

                break;
            }
        case OMX_IndexParamVideoErrorCorrection:
            {
                DEBUG_PRINT_LOW("venc_set_param:OMX_IndexParamVideoErrorCorrection");
                OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *error_resilience =
                    (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *)paramData;

                if (error_resilience->nPortIndex == (OMX_U32) PORT_INDEX_OUT) {
                    if (venc_set_error_resilience(error_resilience) == false) {
                        DEBUG_PRINT_ERROR("ERROR: Setting Intra refresh failed");
                        return false;
                    }
                } else {
                    DEBUG_PRINT_ERROR("ERROR: Invalid Port Index for OMX_IndexParamVideoErrorCorrection");
                }

                break;
            }
        case OMX_IndexParamVideoProfileLevelCurrent:
            {
                DEBUG_PRINT_LOW("venc_set_param:OMX_IndexParamVideoProfileLevelCurrent");
                OMX_VIDEO_PARAM_PROFILELEVELTYPE *profile_level =
                    (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)paramData;

                if (profile_level->nPortIndex == (OMX_U32) PORT_INDEX_OUT) {
                    m_profile_set = false;
                    m_level_set = false;

                    if (!venc_set_profile_level (profile_level->eProfile,
                                profile_level->eLevel)) {
                        DEBUG_PRINT_ERROR("WARNING: Unsuccessful in updating Profile and level");
                        return false;
                    }
                } else {
                    DEBUG_PRINT_ERROR("ERROR: Invalid Port Index for OMX_IndexParamVideoProfileLevelCurrent");
                }

                break;
            }
        case OMX_IndexParamVideoQuantization:
            {
                DEBUG_PRINT_LOW("venc_set_param:OMX_IndexParamVideoQuantization");
                OMX_VIDEO_PARAM_QUANTIZATIONTYPE *session_qp =
                    (OMX_VIDEO_PARAM_QUANTIZATIONTYPE *)paramData;

                if (session_qp->nPortIndex == (OMX_U32) PORT_INDEX_OUT) {
                    if (venc_set_session_qp (session_qp->nQpI,
                                session_qp->nQpP) == false) {
                        DEBUG_PRINT_ERROR("ERROR: Setting Session QP failed");
                        return false;
                    }
                } else {
                    DEBUG_PRINT_ERROR("ERROR: Invalid Port Index for OMX_IndexParamVideoQuantization");
                }

                break;
            }

        case OMX_QcomIndexParamVideoQPRange:
            {
                DEBUG_PRINT_LOW("venc_set_param:OMX_QcomIndexParamVideoQPRange");
                OMX_QCOM_VIDEO_PARAM_QPRANGETYPE *qp_range =
                    (OMX_QCOM_VIDEO_PARAM_QPRANGETYPE *)paramData;

                if (qp_range->nPortIndex == (OMX_U32) PORT_INDEX_OUT) {
                    if (venc_set_qp_range (qp_range->minQP,
                                qp_range->maxQP) == false) {
                        DEBUG_PRINT_ERROR("ERROR: Setting QP Range failed");
                        return false;
                    }
                } else {
                    DEBUG_PRINT_ERROR("ERROR: Invalid Port Index for OMX_QcomIndexParamVideoQPRange");
                }

                break;
            }

        case OMX_ExtraDataVideoEncoderSliceInfo:
            {
                DEBUG_PRINT_LOW("venc_set_param: OMX_ExtraDataVideoEncoderSliceInfo");
                OMX_U32 extra_data = *(OMX_U32 *)paramData;

                if (venc_set_extradata(extra_data) == false) {
                    DEBUG_PRINT_ERROR("ERROR: Setting "
                            "OMX_ExtraDataVideoEncoderSliceInfo failed");
                    return false;
                }

                break;
            }
        case OMX_ExtraDataVideoLTRInfo:
            {
                DEBUG_PRINT_LOW("venc_set_param: OMX_ExtraDataVideoLTRInfo");
                OMX_U32 extra_data = *(OMX_U32 *)paramData;

                if (venc_set_extradata(extra_data) == false) {
                    DEBUG_PRINT_ERROR("ERROR: Setting "
                            "OMX_ExtraDataVideoLTRInfo failed");
                    return false;
                }

                break;
            }
        case OMX_QcomIndexEnableSliceDeliveryMode:
            {
                QOMX_EXTNINDEX_PARAMTYPE* pParam =
                    (QOMX_EXTNINDEX_PARAMTYPE*)paramData;

                if (pParam->nPortIndex == PORT_INDEX_OUT) {
                    if (venc_set_slice_delivery_mode(pParam->bEnable) == false) {
                        DEBUG_PRINT_ERROR("Setting slice delivery mode failed");
                        return OMX_ErrorUnsupportedSetting;
                    }
                } else {
                    DEBUG_PRINT_ERROR("OMX_QcomIndexEnableSliceDeliveryMode "
                            "called on wrong port(%d)", pParam->nPortIndex);
                    return OMX_ErrorBadPortIndex;
                }

                break;
            }
        case OMX_QcomIndexEnableH263PlusPType:
            {
                QOMX_EXTNINDEX_PARAMTYPE* pParam =
                    (QOMX_EXTNINDEX_PARAMTYPE*)paramData;
                DEBUG_PRINT_LOW("OMX_QcomIndexEnableH263PlusPType");

                if (pParam->nPortIndex == PORT_INDEX_OUT) {
                    if (venc_set_plusptype(pParam->bEnable) == false) {
                        DEBUG_PRINT_ERROR("Setting PlusPType failed for H263");
                        return OMX_ErrorUnsupportedSetting;
                    }
                } else {
                    DEBUG_PRINT_ERROR("OMX_QcomIndexEnableH263PlusPType "
                            "called on wrong port(%d)", pParam->nPortIndex);
                    return OMX_ErrorBadPortIndex;
                }

                break;
            }
        case QOMX_IndexParamVideoLTRMode:
            {
                QOMX_VIDEO_PARAM_LTRMODE_TYPE* pParam =
                    (QOMX_VIDEO_PARAM_LTRMODE_TYPE*)paramData;

                if (pParam->nPortIndex == PORT_INDEX_OUT) {
                    if (!venc_set_ltrmode(pParam->eLTRMode)) {
                        DEBUG_PRINT_ERROR("Setting ltr mode failed");
                        return OMX_ErrorUnsupportedSetting;
                    }
                } else {
                    DEBUG_PRINT_ERROR("QOMX_IndexParamVideoLTRMode "
                            "called on wrong port(%d)", pParam->nPortIndex);
                    return OMX_ErrorBadPortIndex;
                }

                break;
            }
        case QOMX_IndexParamVideoLTRCount:
            {
                QOMX_VIDEO_PARAM_LTRCOUNT_TYPE* pParam =
                    (QOMX_VIDEO_PARAM_LTRCOUNT_TYPE*)paramData;

                if (pParam->nPortIndex == PORT_INDEX_OUT) {
                    if (!venc_set_ltrcount(pParam->nCount)) {
                        DEBUG_PRINT_ERROR("Setting ltr count failed");
                        return OMX_ErrorUnsupportedSetting;
                    }
                } else {
                    DEBUG_PRINT_ERROR("QOMX_IndexParamVideoLTRCount "
                            "called on wrong port(%d)", pParam->nPortIndex);
                    return OMX_ErrorBadPortIndex;
                }

                break;
            }
        case OMX_IndexParamVideoSliceFMO:
        default:
            DEBUG_PRINT_ERROR("venc_set_param: Unsupported index 0x%x", index);
            break;
    }

    return true;
}

bool venc_dev::venc_set_config(void *configData, OMX_INDEXTYPE index)
{
    venc_ioctl_msg ioctl_msg = {NULL,NULL};
    DEBUG_PRINT_LOW("Inside venc_set_config");

    switch (index) {
        case OMX_IndexConfigVideoBitrate:
            {
                OMX_VIDEO_CONFIG_BITRATETYPE *bit_rate = (OMX_VIDEO_CONFIG_BITRATETYPE *)
                    configData;

                if (m_max_allowed_bitrate_check &&
                        !venc_max_allowed_bitrate_check(bit_rate->nEncodeBitrate)) {
                    DEBUG_PRINT_ERROR("Max Allowed Bitrate Check failed");
                    return false;
                }

                DEBUG_PRINT_LOW("venc_set_config: OMX_IndexConfigVideoBitrate");

                if (bit_rate->nPortIndex == (OMX_U32)PORT_INDEX_OUT) {
                    if (venc_set_target_bitrate(bit_rate->nEncodeBitrate, 1) == false) {
                        DEBUG_PRINT_ERROR("ERROR: Setting Target Bit rate failed");
                        return false;
                    }
                } else {
                    DEBUG_PRINT_ERROR("ERROR: Invalid Port Index for OMX_IndexConfigVideoBitrate");
                }

                break;
            }
        case OMX_IndexConfigVideoFramerate:
            {
                OMX_CONFIG_FRAMERATETYPE *frame_rate = (OMX_CONFIG_FRAMERATETYPE *)
                    configData;
                DEBUG_PRINT_LOW("venc_set_config: OMX_IndexConfigVideoFramerate");

                if (frame_rate->nPortIndex == (OMX_U32)PORT_INDEX_OUT) {
                    if (venc_set_encode_framerate(frame_rate->xEncodeFramerate, 1) == false) {
                        DEBUG_PRINT_ERROR("ERROR: Setting Encode Framerate failed");
                        return false;
                    }
                } else {
                    DEBUG_PRINT_ERROR("ERROR: Invalid Port Index for OMX_IndexConfigVideoFramerate");
                }

                break;
            }
        case QOMX_IndexConfigVideoIntraperiod:
            {
                DEBUG_PRINT_LOW("venc_set_param:QOMX_IndexConfigVideoIntraperiod");
                QOMX_VIDEO_INTRAPERIODTYPE *intraperiod =
                    (QOMX_VIDEO_INTRAPERIODTYPE *)configData;

                if (intraperiod->nPortIndex == (OMX_U32) PORT_INDEX_OUT) {
                    if (venc_set_intra_period(intraperiod->nPFrames, intraperiod->nBFrames) == false) {
                        DEBUG_PRINT_ERROR("ERROR: Request for setting intra period failed");
                        return false;
                    }
                }

                break;
            }
        case OMX_IndexConfigVideoIntraVOPRefresh:
            {
                OMX_CONFIG_INTRAREFRESHVOPTYPE *intra_vop_refresh = (OMX_CONFIG_INTRAREFRESHVOPTYPE *)
                    configData;
                DEBUG_PRINT_LOW("venc_set_config: OMX_IndexConfigVideoIntraVOPRefresh");

                if (intra_vop_refresh->nPortIndex == (OMX_U32)PORT_INDEX_OUT) {
                    if (venc_set_intra_vop_refresh(intra_vop_refresh->IntraRefreshVOP) == false) {
                        DEBUG_PRINT_ERROR("ERROR: Setting Encode Framerate failed");
                        return false;
                    }
                } else {
                    DEBUG_PRINT_ERROR("ERROR: Invalid Port Index for OMX_IndexConfigVideoFramerate");
                }

                break;
            }
        case OMX_IndexConfigCommonRotate:
            {
                OMX_CONFIG_ROTATIONTYPE *config_rotation =
                    reinterpret_cast<OMX_CONFIG_ROTATIONTYPE*>(configData);
                venc_ioctl_msg ioctl_msg = {NULL,NULL};
                OMX_U32 nFrameWidth;

                DEBUG_PRINT_HIGH("venc_set_config: updating the new Dims");
                nFrameWidth = m_sVenc_cfg.input_width;
                m_sVenc_cfg.input_width  = m_sVenc_cfg.input_height;
                m_sVenc_cfg.input_height = nFrameWidth;
                ioctl_msg.in = (void*)&m_sVenc_cfg;
                ioctl_msg.out = NULL;

                if (ioctl (m_nDriver_fd,VEN_IOCTL_SET_BASE_CFG,(void*)&ioctl_msg) < 0) {
                    DEBUG_PRINT_ERROR("ERROR: Dimension Change for Rotation failed");
                    return false;
                }

                break;
            }
        case QOMX_IndexConfigVideoLTRPeriod:
            {
                QOMX_VIDEO_CONFIG_LTRPERIOD_TYPE* pParam =
                    (QOMX_VIDEO_CONFIG_LTRPERIOD_TYPE*)configData;

                if (pParam->nPortIndex == PORT_INDEX_OUT) {
                    if (!venc_set_ltrperiod(pParam->nFrames)) {
                        DEBUG_PRINT_ERROR("Setting ltr period failed");
                        return OMX_ErrorUnsupportedSetting;
                    }
                } else {
                    DEBUG_PRINT_ERROR("QOMX_IndexConfigVideoLTRPeriod "
                            "called on wrong port(%d)", pParam->nPortIndex);
                    return OMX_ErrorBadPortIndex;
                }

                break;
            }
        case QOMX_IndexConfigVideoLTRUse:
            {
                QOMX_VIDEO_CONFIG_LTRUSE_TYPE* pParam =
                    (QOMX_VIDEO_CONFIG_LTRUSE_TYPE*)configData;

                if (pParam->nPortIndex == PORT_INDEX_OUT) {
                    if (!venc_set_ltruse(pParam->nID, pParam->nFrames)) {
                        DEBUG_PRINT_ERROR("Setting ltr use failed");
                        return OMX_ErrorUnsupportedSetting;
                    }
                } else {
                    DEBUG_PRINT_ERROR("QOMX_IndexConfigVideoLTRUse "
                            "called on wrong port(%d)", pParam->nPortIndex);
                    return OMX_ErrorBadPortIndex;
                }

                break;
            }
        default:
            DEBUG_PRINT_ERROR("venc_set_config: Unsupported index = 0x%x", index);
            break;
    }

    return true;
}

unsigned venc_dev::venc_stop( void)
{
#ifdef MAX_RES_1080P
    pmem_free();
#endif
    return ioctl(m_nDriver_fd,VEN_IOCTL_CMD_STOP,NULL);
}

unsigned venc_dev::venc_pause(void)
{
    return ioctl(m_nDriver_fd,VEN_IOCTL_CMD_PAUSE,NULL);
}

unsigned venc_dev::venc_resume(void)
{
    return ioctl(m_nDriver_fd,VEN_IOCTL_CMD_RESUME,NULL) ;
}

unsigned venc_dev::venc_start_done(void)
{
    return 0;
}

unsigned venc_dev::venc_set_message_thread_id(pthread_t)
{
    return 0;
}

unsigned venc_dev::venc_start(void)
{
    DEBUG_PRINT_HIGH("%s(): Check Profile/Level set in driver before start",
            __func__);

    if (!venc_set_profile_level(0, 0)) {
        DEBUG_PRINT_ERROR("ERROR: %s(): Driver Profile/Level is NOT SET",
                __func__);
    } else {
        DEBUG_PRINT_HIGH("%s(): Driver Profile[%lu]/Level[%lu] successfully SET",
                __func__, codec_profile.profile, profile_level.level);
    }

    if (m_max_allowed_bitrate_check &&
            !venc_max_allowed_bitrate_check(bitrate.target_bitrate)) {
        DEBUG_PRINT_ERROR("Maximum Allowed Bitrate Check failed");
        return -1;
    }

    venc_config_print();

#ifdef MAX_RES_1080P

    if ((codec_profile.profile == VEN_PROFILE_MPEG4_SP) ||
            (codec_profile.profile == VEN_PROFILE_H264_BASELINE) ||
            (codec_profile.profile == VEN_PROFILE_H263_BASELINE))
        recon_buffers_count = MAX_RECON_BUFFERS - 2;
    else
        recon_buffers_count = MAX_RECON_BUFFERS;

    if (ltrmode.ltr_mode == (unsigned long)QOMX_VIDEO_LTRMode_Auto) {
        recon_buffers_count = MAX_RECON_BUFFERS;
        DEBUG_PRINT_HIGH("ltr mode enabled, so set recon buffers "
                "count to %d", recon_buffers_count);
    }

    if (!venc_allocate_recon_buffers())
        return ioctl(m_nDriver_fd, VEN_IOCTL_CMD_START, NULL);
    else {
        DEBUG_PRINT_ERROR("Failed in creating Recon buffers");
        return -1;
    }

#else
    return ioctl(m_nDriver_fd, VEN_IOCTL_CMD_START, NULL);
#endif
}

#ifdef MAX_RES_1080P
OMX_U32 venc_dev::venc_allocate_recon_buffers()
{
    OMX_U32 yuv_size;
    struct venc_ioctl_msg ioctl_msg;
    struct venc_recon_buff_size recon_buff_size;

    recon_buff_size.width =  ((m_sVenc_cfg.input_width + 15) / 16) * 16;
    recon_buff_size.height = ((m_sVenc_cfg.input_height + 15) / 16 ) * 16;

    DEBUG_PRINT_LOW("Width %d, Height %d, w_round %d, h_round %d", m_sVenc_cfg.input_width,
            m_sVenc_cfg.input_height, recon_buff_size.width, recon_buff_size.height);

    ioctl_msg.in = NULL;
    ioctl_msg.out = (void*)&recon_buff_size;

    if (ioctl (m_nDriver_fd,VEN_IOCTL_GET_RECON_BUFFER_SIZE, (void*)&ioctl_msg) < 0) {
        DEBUG_PRINT_ERROR("VEN_IOCTL_GET_RECON_BUFFER_SIZE Failed for width: %d, Height %d" ,
                recon_buff_size.width, recon_buff_size.height);
        return OMX_ErrorInsufficientResources;
    }

    DEBUG_PRINT_HIGH("Width %d, Height %d, w_round %d, h_round %d, yuv_size %d alignment %d count %d",
            m_sVenc_cfg.input_width, m_sVenc_cfg.input_height, recon_buff_size.width,
            recon_buff_size.height, recon_buff_size.size, recon_buff_size.alignment,
            recon_buffers_count);

    for (int i = 0; i < recon_buffers_count; i++) {
        if (pmem_allocate(recon_buff_size.size, recon_buff_size.alignment,i)) {
            DEBUG_PRINT_ERROR("Error returned in allocating recon buffers");
            return -1;
        }
    }

    return 0;
}
OMX_U32 venc_dev::pmem_allocate(OMX_U32 size, OMX_U32 alignment, OMX_U32 count)
{
    OMX_U32 pmem_fd = -1;
    OMX_U32 width, height;
    void *buf_addr = NULL;
    struct venc_ioctl_msg ioctl_msg;
    struct venc_recon_addr recon_addr;
    int rc = 0;

#ifdef USE_ION
    recon_buff[count].ion_device_fd = open (MEM_DEVICE,O_RDONLY);

    if (recon_buff[count].ion_device_fd < 0) {
        DEBUG_PRINT_ERROR("ERROR: ION Device open() Failed");
        return -1;
    }

    recon_buff[count].alloc_data.len = size;
#ifdef MAX_RES_720P
    recon_buff[count].alloc_data.ION_HEAP_MASK = ION_HEAP(MEM_HEAP_ID);
#else
    recon_buff[count].alloc_data.ION_HEAP_MASK = (ION_HEAP(MEM_HEAP_ID) |
            ION_HEAP(ION_IOMMU_HEAP_ID));
#endif
    recon_buff[count].alloc_data.flags = ION_FLAG_CACHED;
    recon_buff[count].alloc_data.align = clip2(alignment);

    if (recon_buff[count].alloc_data.align != 8192)
        recon_buff[count].alloc_data.align = 8192;

    rc = ioctl(recon_buff[count].ion_device_fd,ION_IOC_ALLOC,&recon_buff[count].alloc_data);

    if (rc || !recon_buff[count].alloc_data.handle) {
        DEBUG_PRINT_ERROR("ION ALLOC memory failed ");
        recon_buff[count].alloc_data.handle=NULL;
        return -1;
    }

    recon_buff[count].ion_alloc_fd.handle = recon_buff[count].alloc_data.handle;
    rc = ioctl(recon_buff[count].ion_device_fd,ION_IOC_MAP,&recon_buff[count].ion_alloc_fd);

    if (rc) {
        DEBUG_PRINT_ERROR("ION MAP failed ");
        recon_buff[count].ion_alloc_fd.fd =-1;
        recon_buff[count].ion_alloc_fd.fd =-1;
        return -1;
    }

    pmem_fd = recon_buff[count].ion_alloc_fd.fd;
#else
    struct pmem_allocation allocation;
    pmem_fd = open(MEM_DEVICE, O_RDWR);

    if ((int)(pmem_fd) < 0) {
        DEBUG_PRINT_ERROR("Failed to get an pmem handle");
        return -1;
    }

    allocation.size = size;
    allocation.align = clip2(alignment);

    if (allocation.align != 8192)
        allocation.align = 8192;

    if (ioctl(pmem_fd, PMEM_ALLOCATE_ALIGNED, &allocation) < 0) {
        DEBUG_PRINT_ERROR("Aligment(%u) failed with pmem driver Sz(%lu)",
                allocation.align, allocation.size);
        return -1;
    }

#endif
    buf_addr = mmap(NULL, size,
            PROT_READ | PROT_WRITE,
            MAP_SHARED, pmem_fd, 0);

    if (buf_addr == (void*) MAP_FAILED) {
        close(pmem_fd);
        pmem_fd = -1;
        DEBUG_PRINT_ERROR("Error returned in allocating recon buffers buf_addr: %p",buf_addr);
#ifdef USE_ION

        if (ioctl(recon_buff[count].ion_device_fd,ION_IOC_FREE,
                    &recon_buff[count].alloc_data.handle)) {
            DEBUG_PRINT_LOW("ion recon buffer free failed");
        }

        recon_buff[count].alloc_data.handle = NULL;
        recon_buff[count].ion_alloc_fd.fd =-1;
        close(recon_buff[count].ion_device_fd);
        recon_buff[count].ion_device_fd =-1;
#endif
        return -1;
    }

    DEBUG_PRINT_HIGH("Allocated virt:%p, FD: %d of size %d", buf_addr, pmem_fd, size);

    recon_addr.buffer_size = size;
    recon_addr.pmem_fd = pmem_fd;
    recon_addr.offset = 0;
    recon_addr.pbuffer = (unsigned char *)buf_addr;

    ioctl_msg.in = (void*)&recon_addr;
    ioctl_msg.out = NULL;

    if (ioctl (m_nDriver_fd,VEN_IOCTL_SET_RECON_BUFFER, (void*)&ioctl_msg) < 0) {
        DEBUG_PRINT_ERROR("Failed to set the Recon_buffers");
        return -1;
    }

    recon_buff[count].virtual_address = (unsigned char *) buf_addr;
    recon_buff[count].size = size;
    recon_buff[count].offset = 0;
    recon_buff[count].pmem_fd = pmem_fd;

    DEBUG_PRINT_ERROR("Allocated virt:%p, FD: %d of size %d at index: %d", recon_buff[count].virtual_address,
            recon_buff[count].pmem_fd, recon_buff[count].size, count);
    return 0;
}

OMX_U32 venc_dev::pmem_free()
{
    int cnt = 0;
    struct venc_ioctl_msg ioctl_msg;
    struct venc_recon_addr recon_addr;

    for (cnt = 0; cnt < recon_buffers_count; cnt++) {
        if (recon_buff[cnt].pmem_fd) {
            recon_addr.pbuffer = recon_buff[cnt].virtual_address;
            recon_addr.offset = recon_buff[cnt].offset;
            recon_addr.pmem_fd = recon_buff[cnt].pmem_fd;
            recon_addr.buffer_size = recon_buff[cnt].size;
            ioctl_msg.in = (void*)&recon_addr;
            ioctl_msg.out = NULL;

            if (ioctl(m_nDriver_fd, VEN_IOCTL_FREE_RECON_BUFFER ,&ioctl_msg) < 0)
                DEBUG_PRINT_ERROR("VEN_IOCTL_FREE_RECON_BUFFER failed");

            munmap(recon_buff[cnt].virtual_address, recon_buff[cnt].size);
            close(recon_buff[cnt].pmem_fd);
#ifdef USE_ION

            if (ioctl(recon_buff[cnt].ion_device_fd,ION_IOC_FREE,
                        &recon_buff[cnt].alloc_data.handle)) {
                DEBUG_PRINT_LOW("ion recon buffer free failed");
            }

            recon_buff[cnt].alloc_data.handle = NULL;
            recon_buff[cnt].ion_alloc_fd.fd =-1;
            close(recon_buff[cnt].ion_device_fd);
            recon_buff[cnt].ion_device_fd =-1;
#endif
            DEBUG_PRINT_LOW("cleaning Index %d of size %d",cnt,recon_buff[cnt].size);
            recon_buff[cnt].pmem_fd = -1;
            recon_buff[cnt].virtual_address = NULL;
            recon_buff[cnt].offset = 0;
            recon_buff[cnt].alignment = 0;
            recon_buff[cnt].size = 0;
        }
    }

    return 0;
}
#endif

void venc_dev::venc_config_print()
{

    DEBUG_PRINT_HIGH("ENC_CONFIG: Codec: %d, Profile %d, level : %d",
            m_sVenc_cfg.codectype, codec_profile.profile, profile_level.level);

    DEBUG_PRINT_HIGH("ENC_CONFIG: Width: %d, Height:%d, Fps: %d",
            m_sVenc_cfg.input_width, m_sVenc_cfg.input_height,
            m_sVenc_cfg.fps_num/m_sVenc_cfg.fps_den);

    DEBUG_PRINT_HIGH("ENC_CONFIG: Bitrate: %d, RC: %d, I-Period: %d",
            bitrate.target_bitrate, rate_ctrl.rcmode, intra_period.num_pframes);

    DEBUG_PRINT_HIGH("ENC_CONFIG: qpI: %d, qpP: %d, qpb: 0",
            session_qp.iframeqp, session_qp.pframqp);

    DEBUG_PRINT_HIGH("ENC_CONFIG: minQP: %d, maxQP: %d",
            qp_range.minqp, qp_range.maxqp);

    DEBUG_PRINT_HIGH("ENC_CONFIG: VOP_Resolution: %d, Slice-Mode: %d, Slize_Size: %d",
            voptimecfg.voptime_resolution, multislice.mslice_mode,
            multislice.mslice_size);

    DEBUG_PRINT_HIGH("ENC_CONFIG: EntropyMode: %d, CabacModel: %d",
            entropy.longentropysel, entropy.cabacmodel);

    DEBUG_PRINT_HIGH("ENC_CONFIG: DB-Mode: %d, alpha: %d, Beta: %d",
            dbkfilter.db_mode, dbkfilter.slicealpha_offset,
            dbkfilter.slicebeta_offset);

    DEBUG_PRINT_HIGH("ENC_CONFIG: IntraMB/Frame: %d, HEC: %d",
            intra_refresh.mbcount, hec.header_extension);
}

unsigned venc_dev::venc_flush( unsigned port)
{
    struct venc_ioctl_msg ioctl_msg;
    struct venc_bufferflush buffer_index;

    if (port == PORT_INDEX_IN) {
        DEBUG_PRINT_HIGH("Calling Input Flush");
        buffer_index.flush_mode = VEN_FLUSH_INPUT;
        ioctl_msg.in = (void*)&buffer_index;
        ioctl_msg.out = NULL;

        return ioctl (m_nDriver_fd,VEN_IOCTL_CMD_FLUSH,(void*)&ioctl_msg);
    } else if (port == PORT_INDEX_OUT) {
        DEBUG_PRINT_HIGH("Calling Output Flush");
        buffer_index.flush_mode = VEN_FLUSH_OUTPUT;
        ioctl_msg.in = (void*)&buffer_index;
        ioctl_msg.out = NULL;
        return ioctl (m_nDriver_fd,VEN_IOCTL_CMD_FLUSH,(void*)&ioctl_msg);
    } else {
        return -1;
    }
}

//allocating I/P memory from pmem and register with the device


bool venc_dev::venc_use_buf(void *buf_addr, unsigned port,unsigned)
{
    struct venc_ioctl_msg ioctl_msg = {NULL,NULL};
    struct pmem *pmem_tmp;
    struct venc_bufferpayload dev_buffer = {0};
    struct venc_allocatorproperty buff_alloc_property = {0};

    pmem_tmp = (struct pmem *)buf_addr;

    DEBUG_PRINT_LOW("venc_use_buf:: pmem_tmp = %p", pmem_tmp);

    if (port == PORT_INDEX_IN) {
        dev_buffer.pbuffer = (OMX_U8 *)pmem_tmp->buffer;
        dev_buffer.fd  = pmem_tmp->fd;
        dev_buffer.maped_size = pmem_tmp->size;
        dev_buffer.sz = pmem_tmp->size;
        dev_buffer.offset = pmem_tmp->offset;

        if ((m_sVenc_cfg.input_height %16 !=0) || (m_sVenc_cfg.input_width%16 != 0)) {
            unsigned long ht = m_sVenc_cfg.input_height;
            unsigned long wd = m_sVenc_cfg.input_width;
            unsigned int luma_size, luma_size_2k;

            ht = (ht + 15) & ~15;
            wd = (wd + 15) & ~15;

            luma_size = ht * wd;
            luma_size_2k = (luma_size + 2047) & ~2047;

            dev_buffer.sz = luma_size_2k + ((luma_size/2 + 2047) & ~2047);
#ifdef USE_ION
            ioctl_msg.in = NULL;
            ioctl_msg.out = (void*)&buff_alloc_property;

            if (ioctl (m_nDriver_fd,VEN_IOCTL_GET_INPUT_BUFFER_REQ,(void*)&ioctl_msg) < 0) {
                DEBUG_PRINT_ERROR("ERROR: venc_use_buf:get input buffer failed ");
                return false;
            }

            if (buff_alloc_property.alignment < 4096) {
                dev_buffer.sz = ((dev_buffer.sz + 4095) & ~4095);
            } else {
                dev_buffer.sz = ((dev_buffer.sz + (buff_alloc_property.alignment - 1)) &
                        ~(buff_alloc_property.alignment - 1));
            }

#endif
            dev_buffer.maped_size = dev_buffer.sz;
        }

        ioctl_msg.in  = (void*)&dev_buffer;
        ioctl_msg.out = NULL;

        DEBUG_PRINT_LOW("venc_use_buf:pbuffer = %x,fd = %x, offset = %d, maped_size = %d", \
                dev_buffer.pbuffer, \
                dev_buffer.fd, \
                dev_buffer.offset, \
                dev_buffer.maped_size);

        if (ioctl (m_nDriver_fd,VEN_IOCTL_SET_INPUT_BUFFER,&ioctl_msg) < 0) {
            DEBUG_PRINT_ERROR("ERROR: venc_use_buf:set input buffer failed ");
            return false;
        }
    } else if (port == PORT_INDEX_OUT) {
        dev_buffer.pbuffer = (OMX_U8 *)pmem_tmp->buffer;
        dev_buffer.fd  = pmem_tmp->fd;
        dev_buffer.sz = pmem_tmp->size;
        dev_buffer.maped_size = pmem_tmp->size;
        dev_buffer.offset = pmem_tmp->offset;
        ioctl_msg.in  = (void*)&dev_buffer;
        ioctl_msg.out = NULL;

        DEBUG_PRINT_LOW("venc_use_buf:pbuffer = %x,fd = %x, offset = %d, maped_size = %d", \
                dev_buffer.pbuffer, \
                dev_buffer.fd, \
                dev_buffer.offset, \
                dev_buffer.maped_size);

        if (ioctl (m_nDriver_fd,VEN_IOCTL_SET_OUTPUT_BUFFER,&ioctl_msg) < 0) {
            DEBUG_PRINT_ERROR("ERROR: venc_use_buf:set output buffer failed ");
            return false;
        }
    } else {
        DEBUG_PRINT_ERROR("ERROR: venc_use_buf:Invalid Port Index ");
        return false;
    }

    return true;
}

bool venc_dev::venc_free_buf(void *buf_addr, unsigned port)
{
    struct venc_ioctl_msg ioctl_msg = {NULL,NULL};
    struct pmem *pmem_tmp;
    struct venc_bufferpayload dev_buffer = {0};

    pmem_tmp = (struct pmem *)buf_addr;

    DEBUG_PRINT_LOW("venc_use_buf:: pmem_tmp = %p", pmem_tmp);

    if (port == PORT_INDEX_IN) {
        dev_buffer.pbuffer = (OMX_U8 *)pmem_tmp->buffer;
        dev_buffer.fd  = pmem_tmp->fd;
        dev_buffer.maped_size = pmem_tmp->size;
        dev_buffer.sz = pmem_tmp->size;
        dev_buffer.offset = pmem_tmp->offset;
        ioctl_msg.in  = (void*)&dev_buffer;
        ioctl_msg.out = NULL;

        DEBUG_PRINT_LOW("venc_free_buf:pbuffer = %x,fd = %x, offset = %d, maped_size = %d", \
                dev_buffer.pbuffer, \
                dev_buffer.fd, \
                dev_buffer.offset, \
                dev_buffer.maped_size);

        if (ioctl (m_nDriver_fd,VEN_IOCTL_CMD_FREE_INPUT_BUFFER,&ioctl_msg) < 0) {
            DEBUG_PRINT_ERROR("ERROR: venc_free_buf: free input buffer failed ");
            return false;
        }
    } else if (port == PORT_INDEX_OUT) {
        dev_buffer.pbuffer = (OMX_U8 *)pmem_tmp->buffer;
        dev_buffer.fd  = pmem_tmp->fd;
        dev_buffer.sz = pmem_tmp->size;
        dev_buffer.maped_size = pmem_tmp->size;
        dev_buffer.offset = pmem_tmp->offset;
        ioctl_msg.in  = (void*)&dev_buffer;
        ioctl_msg.out = NULL;

        DEBUG_PRINT_LOW("venc_free_buf:pbuffer = %x,fd = %x, offset = %d, maped_size = %d", \
                dev_buffer.pbuffer, \
                dev_buffer.fd, \
                dev_buffer.offset, \
                dev_buffer.maped_size);

        if (ioctl (m_nDriver_fd,VEN_IOCTL_CMD_FREE_OUTPUT_BUFFER,&ioctl_msg) < 0) {
            DEBUG_PRINT_ERROR("ERROR: venc_free_buf: free output buffer failed ");
            return false;
        }
    } else {
        DEBUG_PRINT_ERROR("ERROR: venc_free_buf:Invalid Port Index ");
        return false;
    }

    return true;
}

bool venc_dev::venc_color_align(OMX_BUFFERHEADERTYPE *buffer,
                OMX_U32 width, OMX_U32 height)
{
    DEBUG_PRINT_ERROR("%s not implemented!", __func__);
    return OMX_ErrorUnsupportedSetting;
}

bool venc_dev::venc_empty_buf(void *buffer, void *pmem_data_buf,unsigned,unsigned)
{
    struct venc_buffer frameinfo;
    struct pmem *temp_buffer;
    struct venc_ioctl_msg ioctl_msg;
    struct OMX_BUFFERHEADERTYPE *bufhdr;

    if (buffer == NULL) {
        DEBUG_PRINT_ERROR("ERROR: venc_etb: buffer is NULL");
        return false;
    }

    bufhdr = (OMX_BUFFERHEADERTYPE *)buffer;

    DEBUG_PRINT_LOW("Input buffer length %d",bufhdr->nFilledLen);

    if (pmem_data_buf) {
        DEBUG_PRINT_LOW("Internal PMEM addr for i/p Heap UseBuf: %p", pmem_data_buf);
        frameinfo.ptrbuffer = (OMX_U8 *)pmem_data_buf;
    } else {
        DEBUG_PRINT_LOW("Shared PMEM addr for i/p PMEM UseBuf/AllocateBuf: %p", bufhdr->pBuffer);
        frameinfo.ptrbuffer = (OMX_U8 *)bufhdr->pBuffer;
    }

    frameinfo.clientdata = (void *) buffer;
    frameinfo.sz = bufhdr->nFilledLen;
    frameinfo.len = bufhdr->nFilledLen;
    frameinfo.flags = bufhdr->nFlags;
    frameinfo.offset = bufhdr->nOffset;
    frameinfo.timestamp = bufhdr->nTimeStamp;
    DEBUG_PRINT_LOW("i/p TS = %u", (OMX_U32)frameinfo.timestamp);
    ioctl_msg.in = &frameinfo;
    ioctl_msg.out = NULL;

    DEBUG_PRINT_LOW("DBG: i/p frameinfo: bufhdr->pBuffer = %p, ptrbuffer = %p, offset = %u, len = %u",
            bufhdr->pBuffer, frameinfo.ptrbuffer, frameinfo.offset, frameinfo.len);

    if (ioctl(m_nDriver_fd,VEN_IOCTL_CMD_ENCODE_FRAME,&ioctl_msg) < 0) {
        /*Generate an async error and move to invalid state*/
        return false;
    }

    if (m_debug.in_buffer_log) {
        venc_input_log_buffers(bufhdr, pmem_data_bufr, frameinfo.len);
    }

    return true;
}
bool venc_dev::venc_fill_buf(void *buffer, void *pmem_data_buf,unsigned,unsigned)
{
    struct venc_ioctl_msg ioctl_msg = {NULL,NULL};
    struct pmem *temp_buffer = NULL;
    struct venc_buffer  frameinfo;
    struct OMX_BUFFERHEADERTYPE *bufhdr;

    if (buffer == NULL) {
        return false;
    }

    bufhdr = (OMX_BUFFERHEADERTYPE *)buffer;

    if (pmem_data_buf) {
        DEBUG_PRINT_LOW("Internal PMEM addr for o/p Heap UseBuf: %p", pmem_data_buf);
        frameinfo.ptrbuffer = (OMX_U8 *)pmem_data_buf;
    } else {
        DEBUG_PRINT_LOW("Shared PMEM addr for o/p PMEM UseBuf/AllocateBuf: %p", bufhdr->pBuffer);
        frameinfo.ptrbuffer = (OMX_U8 *)bufhdr->pBuffer;
    }

    frameinfo.clientdata = buffer;
    frameinfo.sz = bufhdr->nAllocLen;
    frameinfo.flags = bufhdr->nFlags;
    frameinfo.offset = bufhdr->nOffset;

    ioctl_msg.in = &frameinfo;
    ioctl_msg.out = NULL;
    DEBUG_PRINT_LOW("DBG: o/p frameinfo: bufhdr->pBuffer = %p, ptrbuffer = %p, offset = %u, len = %u",
            bufhdr->pBuffer, frameinfo.ptrbuffer, frameinfo.offset, frameinfo.len);

    if (ioctl (m_nDriver_fd,VEN_IOCTL_CMD_FILL_OUTPUT_BUFFER,&ioctl_msg) < 0) {
        DEBUG_PRINT_ERROR("ERROR: ioctl VEN_IOCTL_CMD_FILL_OUTPUT_BUFFER failed");
        return false;
    }

    return true;
}

bool venc_dev::venc_set_slice_delivery_mode(OMX_BOOL enable)
{
    venc_ioctl_msg ioctl_msg = {NULL,NULL};
    DEBUG_PRINT_HIGH("Set slice_delivery_mode: %d", enable);

    if (multislice.mslice_mode == VEN_MSLICE_CNT_MB) {
        if (ioctl(m_nDriver_fd, VEN_IOCTL_SET_SLICE_DELIVERY_MODE) < 0) {
            DEBUG_PRINT_ERROR("Request for setting slice delivery mode failed");
            return false;
        }
    } else {
        DEBUG_PRINT_ERROR("WARNING: slice_mode[%d] is not VEN_MSLICE_CNT_MB to set "
                "slice delivery mode to the driver.", multislice.mslice_mode);
    }

    return true;
}

bool venc_dev::venc_set_plusptype(OMX_BOOL enable)
{
    venc_ioctl_msg ioctl_msg = {NULL,NULL};
    struct venc_plusptype plusptype = {0};
    DEBUG_PRINT_LOW("Set plusptype: %d", enable);
    plusptype.plusptype_enable = enable;
    ioctl_msg.in = (void*)&plusptype;
    ioctl_msg.out = NULL;

    if (ioctl(m_nDriver_fd, VEN_IOCTL_SET_H263_PLUSPTYPE,(void*)&ioctl_msg) < 0) {
        DEBUG_PRINT_ERROR("Request for setting plusptype for h263 failed");
        return false;
    }

    return true;
}

bool venc_dev::venc_set_ltrmode(QOMX_VIDEO_LTRMODETYPE mode)
{
    venc_ioctl_msg ioctl_msg = {NULL,NULL};
    venc_ltrmode ltr_mode;
    ltr_mode.ltr_mode = (unsigned long)mode;
    DEBUG_PRINT_HIGH("Set ltr mode: %d", mode);
    ioctl_msg.in = (void*)&ltr_mode;
    ioctl_msg.out = NULL;

    if (ioctl (m_nDriver_fd, VEN_IOCTL_SET_LTRMODE, (void*)&ioctl_msg) < 0) {
        DEBUG_PRINT_ERROR("ERROR: Setting ltrmode failed");
        return false;
    }

    ltrmode.ltr_mode = (unsigned long)mode;
    return true;
}

bool venc_dev::venc_set_ltrcount(OMX_U32 count)
{
    venc_ioctl_msg ioctl_msg = {NULL,NULL};
    venc_ltrcount ltr_count;
    ltr_count.ltr_count = (unsigned long)count;
    DEBUG_PRINT_HIGH("Set ltr count: %d", count);
    ioctl_msg.in = (void*)&ltr_count;
    ioctl_msg.out = NULL;

    if (ioctl (m_nDriver_fd, VEN_IOCTL_SET_LTRCOUNT, (void*)&ioctl_msg) < 0) {
        DEBUG_PRINT_ERROR("ERROR: Setting ltrcount failed");
        return false;
    }

    ltrcount.ltr_count = (unsigned long)count;
    return true;
}

bool venc_dev::venc_set_ltrperiod(OMX_U32 period)
{
    venc_ioctl_msg ioctl_msg = {NULL,NULL};
    venc_ltrperiod ltr_period;
    ltr_period.ltr_period = (unsigned long)period;
    DEBUG_PRINT_HIGH("Set ltr period: %d", period);
    ioctl_msg.in = (void*)&ltr_period;
    ioctl_msg.out = NULL;

    if (ioctl (m_nDriver_fd, VEN_IOCTL_SET_LTRPERIOD, (void*)&ioctl_msg) < 0) {
        DEBUG_PRINT_ERROR("ERROR: Setting ltrperiod failed");
        return false;
    }

    ltrperiod.ltr_period = (unsigned long)period;
    return true;
}

bool venc_dev::venc_set_ltruse(OMX_U32 id, OMX_U32 frames)
{
    venc_ioctl_msg ioctl_msg = {NULL,NULL};
    venc_ltruse ltr_use;
    ltr_use.ltr_id = (unsigned long)id;
    ltr_use.ltr_frames = (unsigned long)frames;
    DEBUG_PRINT_HIGH("Set ltr use: id = %d, ltr_frames = %d", id, frames);
    ioctl_msg.in = (void*)&ltr_use;
    ioctl_msg.out = NULL;

    if (ioctl (m_nDriver_fd, VEN_IOCTL_SET_LTRUSE, (void*)&ioctl_msg) < 0) {
        DEBUG_PRINT_ERROR("ERROR: Setting ltruse failed");
        return false;
    }

    return true;
}

bool venc_dev::venc_set_extradata(OMX_U32 extra_data)
{
    venc_ioctl_msg ioctl_msg = {NULL,NULL};
    DEBUG_PRINT_HIGH("venc_set_extradata:: %x", extra_data);
    ioctl_msg.in = (void*)&extra_data;
    ioctl_msg.out = NULL;

    if (ioctl (m_nDriver_fd, VEN_IOCTL_SET_EXTRADATA, (void*)&ioctl_msg) < 0) {
        DEBUG_PRINT_ERROR("ERROR: Request for setting extradata failed");
        return false;
    }

    return true;
}

bool venc_dev::venc_set_session_qp(OMX_U32 i_frame_qp, OMX_U32 p_frame_qp)
{
    venc_ioctl_msg ioctl_msg = {NULL,NULL};
    struct venc_sessionqp qp = {0, 0};
    DEBUG_PRINT_HIGH("venc_set_session_qp:: i_frame_qp = %d, p_frame_qp = %d", i_frame_qp,
            p_frame_qp);

    qp.iframeqp = i_frame_qp;
    qp.pframqp = p_frame_qp;

    ioctl_msg.in = (void*)&qp;
    ioctl_msg.out = NULL;

    if (ioctl (m_nDriver_fd,VEN_IOCTL_SET_SESSION_QP,(void*)&ioctl_msg)< 0) {
        DEBUG_PRINT_ERROR("ERROR: Request for setting session qp failed");
        return false;
    }

    session_qp.iframeqp = i_frame_qp;
    session_qp.pframqp = p_frame_qp;

    return true;
}

bool venc_dev::venc_set_qp_range(OMX_U32 min_qp, OMX_U32 max_qp)
{
    venc_ioctl_msg ioctl_msg = {NULL,NULL};
    struct venc_qprange qp = {0, 0};
    DEBUG_PRINT_LOW("venc_set_qp_range:: min_qp = %d, max_qp = %d", min_qp,
            max_qp);

    qp.minqp = min_qp;
    qp.maxqp = max_qp;

    ioctl_msg.in = (void*)&qp;
    ioctl_msg.out = NULL;

    if (ioctl (m_nDriver_fd,VEN_IOCTL_SET_QP_RANGE,(void*)&ioctl_msg)< 0) {
        DEBUG_PRINT_ERROR("ERROR: Request for setting qp range failed");
        return false;
    }

    qp_range.minqp= min_qp;
    qp_range.maxqp= max_qp;

    return true;
}

bool venc_dev::venc_set_profile_level(OMX_U32 eProfile,OMX_U32 eLevel)
{
    venc_ioctl_msg ioctl_msg = {NULL,NULL};
    struct venc_profile requested_profile;
    struct ven_profilelevel requested_level;
    unsigned const int *profile_tbl = NULL;
    unsigned long mb_per_frame = 0, mb_per_sec = 0;
    DEBUG_PRINT_HIGH("venc_set_profile_level:: eProfile = %d, Level = %d",
            eProfile, eLevel);
    mb_per_frame = ((m_sVenc_cfg.input_height + 15) >> 4)*
        ((m_sVenc_cfg.input_width + 15) >> 4);

    if ((eProfile == 0) && (eLevel == 0) && m_profile_set && m_level_set) {
        DEBUG_PRINT_HIGH("Set profile/level was done already");
        return true;
    }

    if (eProfile && eLevel) {
        /* non-zero values will be set by user, saving the same*/
        m_eProfile = eProfile;
        m_eLevel = eLevel;
        DEBUG_PRINT_HIGH("Save profile/level (%d/%d) for max allowed bitrate check",
                m_eProfile, m_eLevel);
    }

    DEBUG_PRINT_LOW("Validating Profile/Level from table");

    if (!venc_validate_profile_level(&eProfile, &eLevel)) {
        DEBUG_PRINT_LOW("ERROR: Profile/Level validation failed");
        return false;
    }

    if (m_sVenc_cfg.codectype == VEN_CODEC_MPEG4) {
        DEBUG_PRINT_LOW("eProfile = %d, OMX_VIDEO_MPEG4ProfileSimple = %d and "
                "OMX_VIDEO_MPEG4ProfileAdvancedSimple = %d", eProfile,
                OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4ProfileAdvancedSimple);

        if (eProfile == OMX_VIDEO_MPEG4ProfileSimple) {
            requested_profile.profile = VEN_PROFILE_MPEG4_SP;
            profile_tbl = (unsigned int const *)
                (&mpeg4_profile_level_table[MPEG4_SP_START]);
            profile_tbl += MPEG4_720P_LEVEL*5;
        } else if (eProfile == OMX_VIDEO_MPEG4ProfileAdvancedSimple) {
            requested_profile.profile = VEN_PROFILE_MPEG4_ASP;
            profile_tbl = (unsigned int const *)
                (&mpeg4_profile_level_table[MPEG4_ASP_START]);
            profile_tbl += MPEG4_720P_LEVEL*5;
        } else {
            DEBUG_PRINT_LOW("ERROR: Unsupported MPEG4 profile = %u",
                    eProfile);
            return false;
        }

        DEBUG_PRINT_LOW("eLevel = %d, OMX_VIDEO_MPEG4Level0 = %d, OMX_VIDEO_MPEG4Level1 = %d,"
                "OMX_VIDEO_MPEG4Level2 = %d, OMX_VIDEO_MPEG4Level3 = %d, OMX_VIDEO_MPEG4Level4 = %d,"
                "OMX_VIDEO_MPEG4Level5 = %d", eLevel, OMX_VIDEO_MPEG4Level0, OMX_VIDEO_MPEG4Level1,
                OMX_VIDEO_MPEG4Level2, OMX_VIDEO_MPEG4Level3, OMX_VIDEO_MPEG4Level4, OMX_VIDEO_MPEG4Level5);

        if (mb_per_frame >= 3600) {
            if (requested_profile.profile == VEN_PROFILE_MPEG4_ASP)
                requested_level.level = VEN_LEVEL_MPEG4_5;

            if (requested_profile.profile == VEN_PROFILE_MPEG4_SP)
                requested_level.level = VEN_LEVEL_MPEG4_6;
        } else {
            switch (eLevel) {
                case OMX_VIDEO_MPEG4Level0:
                    requested_level.level = VEN_LEVEL_MPEG4_0;
                    break;
                case OMX_VIDEO_MPEG4Level1:
                    requested_level.level = VEN_LEVEL_MPEG4_1;
                    break;
                case OMX_VIDEO_MPEG4Level2:
                    requested_level.level = VEN_LEVEL_MPEG4_2;
                    break;
                case OMX_VIDEO_MPEG4Level3:
                    requested_level.level = VEN_LEVEL_MPEG4_3;
                    break;
                case OMX_VIDEO_MPEG4Level4a:
                    requested_level.level = VEN_LEVEL_MPEG4_4;
                    break;
                case OMX_VIDEO_MPEG4Level5:
                    mb_per_sec = mb_per_frame * (m_sVenc_cfg.fps_num / m_sVenc_cfg.fps_den);

                    if ((requested_profile.profile == VEN_PROFILE_MPEG4_SP) && (mb_per_frame >= profile_tbl[0]) &&
                            (mb_per_sec >= profile_tbl[1])) {
                        DEBUG_PRINT_LOW("MPEG4 Level 6 is set for 720p resolution");
                        requested_level.level = VEN_LEVEL_MPEG4_6;
                    } else {
                        DEBUG_PRINT_LOW("MPEG4 Level 5 is set for non-720p resolution");
                        requested_level.level = VEN_LEVEL_MPEG4_5;
                    }

                    break;
                default:
                    return false;
                    // TODO update corresponding levels for MPEG4_LEVEL_3b,MPEG4_LEVEL_6
                    break;
            }
        }
    } else if (m_sVenc_cfg.codectype == VEN_CODEC_H263) {
        if (eProfile == OMX_VIDEO_H263ProfileBaseline) {
            requested_profile.profile = VEN_PROFILE_H263_BASELINE;
        } else {
            DEBUG_PRINT_LOW("ERROR: Unsupported H.263 profile = %u",
                    requested_profile.profile);
            return false;
        }

        //profile level
        switch (eLevel) {
            case OMX_VIDEO_H263Level10:
                requested_level.level = VEN_LEVEL_H263_10;
                break;
            case OMX_VIDEO_H263Level20:
                requested_level.level = VEN_LEVEL_H263_20;
                break;
            case OMX_VIDEO_H263Level30:
                requested_level.level = VEN_LEVEL_H263_30;
                break;
            case OMX_VIDEO_H263Level40:
                requested_level.level = VEN_LEVEL_H263_40;
                break;
            case OMX_VIDEO_H263Level45:
                requested_level.level = VEN_LEVEL_H263_45;
                break;
            case OMX_VIDEO_H263Level50:
                requested_level.level = VEN_LEVEL_H263_50;
                break;
            case OMX_VIDEO_H263Level60:
                requested_level.level = VEN_LEVEL_H263_60;
                break;
            case OMX_VIDEO_H263Level70:
                requested_level.level = VEN_LEVEL_H263_70;
                break;
            default:
                return false;
                break;
        }
    } else if (m_sVenc_cfg.codectype == VEN_CODEC_H264) {
        if (eProfile == OMX_VIDEO_AVCProfileBaseline) {
            requested_profile.profile = VEN_PROFILE_H264_BASELINE;
        } else if (eProfile == OMX_VIDEO_AVCProfileMain) {
            requested_profile.profile = VEN_PROFILE_H264_MAIN;
        } else if (eProfile == OMX_VIDEO_AVCProfileHigh) {
            requested_profile.profile = VEN_PROFILE_H264_HIGH;
        } else {
            DEBUG_PRINT_LOW("ERROR: Unsupported H.264 profile = %u",
                    requested_profile.profile);
            return false;
        }

        //profile level
        switch (eLevel) {
            case OMX_VIDEO_AVCLevel1:
                requested_level.level = VEN_LEVEL_H264_1;
                break;
            case OMX_VIDEO_AVCLevel1b:
                requested_level.level = VEN_LEVEL_H264_1b;
                break;
            case OMX_VIDEO_AVCLevel11:
                requested_level.level = VEN_LEVEL_H264_1p1;
                break;
            case OMX_VIDEO_AVCLevel12:
                requested_level.level = VEN_LEVEL_H264_1p2;
                break;
            case OMX_VIDEO_AVCLevel13:
                requested_level.level = VEN_LEVEL_H264_1p3;
                break;
            case OMX_VIDEO_AVCLevel2:
                requested_level.level = VEN_LEVEL_H264_2;
                break;
            case OMX_VIDEO_AVCLevel21:
                requested_level.level = VEN_LEVEL_H264_2p1;
                break;
            case OMX_VIDEO_AVCLevel22:
                requested_level.level = VEN_LEVEL_H264_2p2;
                break;
            case OMX_VIDEO_AVCLevel3:
                requested_level.level = VEN_LEVEL_H264_3;
                break;
            case OMX_VIDEO_AVCLevel31:
                requested_level.level = VEN_LEVEL_H264_3p1;
                break;
            case OMX_VIDEO_AVCLevel32:
                requested_level.level = VEN_LEVEL_H264_3p2;
                break;
            case OMX_VIDEO_AVCLevel4:
                requested_level.level = VEN_LEVEL_H264_4;
                break;
            default :
                DEBUG_PRINT_ERROR("ERROR: Unsupported H.264 level= %u",
                        requested_level.level);
                return false;
                break;
        }
    }

    if (!m_profile_set) {
        ioctl_msg.in = (void*)&requested_profile;
        ioctl_msg.out = NULL;

        if (ioctl (m_nDriver_fd,VEN_IOCTL_SET_CODEC_PROFILE,(void*)&ioctl_msg)< 0) {
            DEBUG_PRINT_ERROR("ERROR: Request for setting profile failed");
            return false;
        }

        codec_profile.profile = requested_profile.profile;
        m_profile_set = true;
        DEBUG_PRINT_HIGH("Set codec profile = 0x%x", codec_profile.profile);
    }

    if (!m_level_set) {
        ioctl_msg.in = (void*)&requested_level;
        ioctl_msg.out = NULL;

        if (ioctl (m_nDriver_fd,VEN_IOCTL_SET_PROFILE_LEVEL,(void*)&ioctl_msg)< 0) {
            DEBUG_PRINT_ERROR("ERROR: Request for setting profile level failed");
            return false;
        }

        profile_level.level = requested_level.level;
        m_level_set = true;
        DEBUG_PRINT_HIGH("Set codec level = 0x%x", profile_level.level);
    }

    return true;
}

bool venc_dev::venc_set_voptiming_cfg( OMX_U32 TimeIncRes)
{
    venc_ioctl_msg ioctl_msg = {NULL,NULL};
    struct venc_voptimingcfg vop_timing_cfg;

    DEBUG_PRINT_HIGH("venc_set_voptiming_cfg: TimeRes = %u",
            TimeIncRes);

    vop_timing_cfg.voptime_resolution = TimeIncRes;

    ioctl_msg.in = (void*)&vop_timing_cfg;
    ioctl_msg.out = NULL;

    if (ioctl (m_nDriver_fd,VEN_IOCTL_SET_VOP_TIMING_CFG,(void*)&ioctl_msg)< 0) {
        DEBUG_PRINT_ERROR("ERROR: Request for setting Vop Timing failed");
        return false;
    }

    voptimecfg.voptime_resolution = vop_timing_cfg.voptime_resolution;
    return true;
}

bool venc_dev::venc_set_intra_period(OMX_U32 nPFrames, OMX_U32 nBFrames)
{
    venc_ioctl_msg ioctl_msg = {NULL,NULL};
    struct venc_intraperiod intraperiod_cfg;

    DEBUG_PRINT_LOW("venc_set_intra_period: nPFrames = %u",
            nPFrames);
    intraperiod_cfg.num_pframes = nPFrames;

    if ((codec_profile.profile == VEN_PROFILE_MPEG4_ASP) ||
            (codec_profile.profile == VEN_PROFILE_H264_MAIN) ||
            (codec_profile.profile == VEN_PROFILE_H264_HIGH)) {
#ifdef MAX_RES_1080P

        if (nBFrames) {
            DEBUG_PRINT_HIGH("INFO: Only 1 Bframe is supported");
            intraperiod_cfg.num_bframes = 1;
        } else
            intraperiod_cfg.num_bframes = 0;

#else

        if (nBFrames) {
            DEBUG_PRINT_ERROR("B frames not supported");
            intraperiod_cfg.num_bframes = 0;
        } else {
            DEBUG_PRINT_ERROR("B frames not supported");
            intraperiod_cfg.num_bframes = 0;
        }

#endif
    } else
        intraperiod_cfg.num_bframes = 0;

    DEBUG_PRINT_HIGH("venc_set_intra_period: nPFrames = %u nBFrames = %u",
            intraperiod_cfg.num_pframes, intraperiod_cfg.num_bframes);
    ioctl_msg.in = (void*)&intraperiod_cfg;
    ioctl_msg.out = NULL;

    if (ioctl (m_nDriver_fd,VEN_IOCTL_SET_INTRA_PERIOD,(void*)&ioctl_msg)< 0) {
        DEBUG_PRINT_ERROR("ERROR: Request for setting intra period failed");
        return false;
    }

    intra_period.num_pframes = intraperiod_cfg.num_pframes;
    intra_period.num_bframes = intraperiod_cfg.num_bframes;
    return true;
}

bool venc_dev::venc_set_entropy_config(OMX_BOOL enable, OMX_U32 i_cabac_level)
{
    venc_ioctl_msg ioctl_msg = {NULL,NULL};
    struct venc_entropycfg entropy_cfg;

    memset(&entropy_cfg,0,sizeof(entropy_cfg));
    DEBUG_PRINT_LOW("venc_set_entropy_config: CABAC = %u level: %u", enable, i_cabac_level);

    if (enable &&(codec_profile.profile != VEN_PROFILE_H264_BASELINE)) {
        entropy_cfg.longentropysel = VEN_ENTROPY_MODEL_CABAC;

        if (i_cabac_level == 0) {
            entropy_cfg.cabacmodel = VEN_CABAC_MODEL_0;
        }

#ifdef MAX_RES_1080P
        else {
            DEBUG_PRINT_HIGH("Invalid model set (%d) defaulting to  model 0",i_cabac_level);
            entropy_cfg.cabacmodel = VEN_CABAC_MODEL_0;
        }

#else
        else if (i_cabac_level == 1) {
            entropy_cfg.cabacmodel = VEN_CABAC_MODEL_1;
        } else if (i_cabac_level == 2) {
            entropy_cfg.cabacmodel = VEN_CABAC_MODEL_2;
        }

#endif
    } else if (!enable) {
        entropy_cfg.longentropysel = VEN_ENTROPY_MODEL_CAVLC;
    } else {
        DEBUG_PRINT_ERROR("Invalid Entropy mode for Baseline Profile");
        return false;
    }

    ioctl_msg.in = (void*)&entropy_cfg;
    ioctl_msg.out = NULL;

    if (ioctl (m_nDriver_fd,VEN_IOCTL_SET_ENTROPY_CFG,(void*)&ioctl_msg)< 0) {
        DEBUG_PRINT_ERROR("ERROR: Request for setting entropy config failed");
        return false;
    }

    entropy.longentropysel = entropy_cfg.longentropysel;
    entropy.cabacmodel  = entropy_cfg.cabacmodel;
    return true;
}

bool venc_dev::venc_set_multislice_cfg(OMX_INDEXTYPE Codec, OMX_U32 nSlicesize) // MB
{
    venc_ioctl_msg ioctl_msg = {NULL, NULL};
    bool status = true;
    struct venc_multiclicecfg multislice_cfg;

    if ((Codec != OMX_IndexParamVideoH263)  && (nSlicesize)) {
        multislice_cfg.mslice_mode = VEN_MSLICE_CNT_MB;
        multislice_cfg.mslice_size = nSlicesize;
    } else {
        multislice_cfg.mslice_mode = VEN_MSLICE_OFF;
        multislice_cfg.mslice_size = 0;
    }

    DEBUG_PRINT_LOW("%s(): mode = %u, size = %u", __func__, multislice_cfg.mslice_mode,
            multislice_cfg.mslice_size);

    ioctl_msg.in = (void*)&multislice_cfg;
    ioctl_msg.out = NULL;

    if (ioctl (m_nDriver_fd, VEN_IOCTL_SET_MULTI_SLICE_CFG,(void*)&ioctl_msg) < 0) {
        DEBUG_PRINT_ERROR("ERROR: Request for setting multi-slice cfg failed");
        status = false;
    } else {
        multislice.mslice_mode = multislice_cfg.mslice_mode;
        multislice.mslice_size = nSlicesize;
    }

    return status;
}

bool venc_dev::venc_set_intra_refresh(OMX_VIDEO_INTRAREFRESHTYPE ir_mode, OMX_U32 irMBs)
{
    venc_ioctl_msg ioctl_msg = {NULL, NULL};
    bool status = true;
    struct venc_intrarefresh intraRefresh_cfg;

    // There is no disabled mode.  Disabled mode is indicated by a 0 count.
    if (irMBs == 0 || ir_mode == OMX_VIDEO_IntraRefreshMax) {
        intraRefresh_cfg.irmode = VEN_IR_OFF;
        intraRefresh_cfg.mbcount = 0;
    } else if ((ir_mode == OMX_VIDEO_IntraRefreshCyclic) &&
            (irMBs < ((m_sVenc_cfg.input_width * m_sVenc_cfg.input_height)>>8))) {
        intraRefresh_cfg.irmode = VEN_IR_CYCLIC;
        intraRefresh_cfg.mbcount = irMBs;
    } else {
        DEBUG_PRINT_ERROR("ERROR: Invalid IntraRefresh Parameters:"
                "mb count: %d, mb mode:%d", irMBs, ir_mode);
        return false;
    }

    ioctl_msg.in = (void*)&intraRefresh_cfg;
    ioctl_msg.out = NULL;

    if (ioctl (m_nDriver_fd,VEN_IOCTL_SET_INTRA_REFRESH,(void*)&ioctl_msg) < 0) {
        DEBUG_PRINT_ERROR("ERROR: Request for setting Intra Refresh failed");
        status = false;
    } else {
        intra_refresh.irmode = intraRefresh_cfg.irmode;
        intra_refresh.mbcount = intraRefresh_cfg.mbcount;
    }

    return status;
}

bool venc_dev::venc_set_error_resilience(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE* error_resilience)
{
    venc_ioctl_msg ioctl_msg = {NULL, NULL};
    bool status = true;
    struct venc_headerextension hec_cfg;
    struct venc_multiclicecfg multislice_cfg;

    if (m_sVenc_cfg.codectype == OMX_VIDEO_CodingMPEG4) {
        if (error_resilience->bEnableHEC) {
            hec_cfg.header_extension = 1;
        } else {
            hec_cfg.header_extension = 0;
        }

        ioctl_msg.in = (void*)&hec_cfg;
        ioctl_msg.out = NULL;

        if (ioctl (m_nDriver_fd,VEN_IOCTL_SET_HEC,(void*)&ioctl_msg) < 0) {
            DEBUG_PRINT_ERROR("ERROR: Request for setting HEader Error correction failed");
            return false;
        }

        hec.header_extension = error_resilience->bEnableHEC;
    }

    if (error_resilience->bEnableRVLC) {
        DEBUG_PRINT_ERROR("RVLC is not Supported");
        return false;
    }

    if (( m_sVenc_cfg.codectype != OMX_VIDEO_CodingH263) &&
            (error_resilience->bEnableDataPartitioning)) {
        DEBUG_PRINT_ERROR("DataPartioning are not Supported for MPEG4/H264");
        return false;
    }

    if (( m_sVenc_cfg.codectype != OMX_VIDEO_CodingH263) &&
            (error_resilience->nResynchMarkerSpacing)) {
        multislice_cfg.mslice_mode = VEN_MSLICE_CNT_BYTE;
        multislice_cfg.mslice_size = error_resilience->nResynchMarkerSpacing;
    } else if (m_sVenc_cfg.codectype == OMX_VIDEO_CodingH263 &&
            error_resilience->bEnableDataPartitioning) {
        multislice_cfg.mslice_mode = VEN_MSLICE_GOB;
        multislice_cfg.mslice_size = 0;
    } else {
        multislice_cfg.mslice_mode = VEN_MSLICE_OFF;
        multislice_cfg.mslice_size = 0;
    }

    DEBUG_PRINT_LOW("%s(): mode = %u, size = %u", __func__, multislice_cfg.mslice_mode,
            multislice_cfg.mslice_size);
    ioctl_msg.in = (void*)&multislice_cfg;
    ioctl_msg.out = NULL;

    if (ioctl (m_nDriver_fd,VEN_IOCTL_SET_MULTI_SLICE_CFG,(void*)&ioctl_msg) < 0) {
        DEBUG_PRINT_ERROR("ERROR: Request for setting multi-slice cfg failed");
        status = false;
    } else {
        multislice.mslice_mode = multislice_cfg.mslice_mode ;
        multislice.mslice_size = multislice_cfg.mslice_size;

    }

    return status;
}

bool venc_dev::venc_set_inloop_filter(OMX_VIDEO_AVCLOOPFILTERTYPE loopfilter)
{
    venc_ioctl_msg ioctl_msg = {NULL,NULL};
    struct venc_dbcfg filter_cfg;

    memset(&filter_cfg, 0, sizeof(filter_cfg));
    DEBUG_PRINT_LOW("venc_set_inloop_filter: %u",loopfilter);

    if (loopfilter == OMX_VIDEO_AVCLoopFilterEnable) {
        filter_cfg.db_mode = VEN_DB_ALL_BLKG_BNDRY;
    } else if (loopfilter == OMX_VIDEO_AVCLoopFilterDisable) {
        filter_cfg.db_mode = VEN_DB_DISABLE;
    } else if (loopfilter == OMX_VIDEO_AVCLoopFilterDisableSliceBoundary) {
        filter_cfg.db_mode = VEN_DB_SKIP_SLICE_BNDRY;
    }

    filter_cfg.slicealpha_offset = filter_cfg.slicebeta_offset = 0;

    ioctl_msg.in = (void*)&filter_cfg;
    ioctl_msg.out = NULL;

    if (ioctl (m_nDriver_fd,VEN_IOCTL_SET_DEBLOCKING_CFG,(void*)&ioctl_msg)< 0) {
        DEBUG_PRINT_ERROR("ERROR: Request for setting inloop filter failed");
        return false;
    }

    dbkfilter.db_mode = filter_cfg.db_mode;
    dbkfilter.slicealpha_offset = dbkfilter.slicebeta_offset = 0;
    return true;
}

bool venc_dev::venc_set_target_bitrate(OMX_U32 nTargetBitrate, OMX_U32 config)
{
    venc_ioctl_msg ioctl_msg = {NULL, NULL};
    struct venc_targetbitrate bitrate_cfg;

    DEBUG_PRINT_HIGH("venc_set_target_bitrate: bitrate = %u",
            nTargetBitrate);
    bitrate_cfg.target_bitrate = nTargetBitrate ;
    ioctl_msg.in = (void*)&bitrate_cfg;
    ioctl_msg.out = NULL;

    if (ioctl (m_nDriver_fd,VEN_IOCTL_SET_TARGET_BITRATE,(void*)&ioctl_msg) < 0) {
        DEBUG_PRINT_ERROR("ERROR: Request for setting bit rate failed");
        return false;
    }

    m_sVenc_cfg.targetbitrate = nTargetBitrate;
    bitrate.target_bitrate = nTargetBitrate;

    if (!config) {
        m_level_set = false;

        if (venc_set_profile_level(0, 0)) {
            DEBUG_PRINT_LOW("Calling set level (Bitrate) with %d",profile_level.level);
        }
    }

    return true;
}

bool venc_dev::venc_set_encode_framerate(OMX_U32 encode_framerate, OMX_U32 config)
{
    venc_ioctl_msg ioctl_msg = {NULL, NULL};
    struct venc_framerate frame_rate_cfg;

    Q16ToFraction(encode_framerate,frame_rate_cfg.fps_numerator,frame_rate_cfg.fps_denominator);

    DEBUG_PRINT_HIGH("venc_set_encode_framerate: framerate(Q16) = %u, NR: %d, DR: %d",
            encode_framerate,frame_rate_cfg.fps_numerator,frame_rate_cfg.fps_denominator);

    ioctl_msg.in = (void*)&frame_rate_cfg;
    ioctl_msg.out = NULL;

    if (ioctl(m_nDriver_fd, VEN_IOCTL_SET_FRAME_RATE,
                (void*)&ioctl_msg) < 0) {
        DEBUG_PRINT_ERROR("ERROR: Request for setting framerate failed");
        return false;
    }

    m_sVenc_cfg.fps_den = frame_rate_cfg.fps_denominator;
    m_sVenc_cfg.fps_num = frame_rate_cfg.fps_numerator;

    if (!config) {
        m_level_set = false;

        if (venc_set_profile_level(0, 0)) {
            DEBUG_PRINT_LOW("Calling set level (Framerate) with %d",profile_level.level);
        }
    }

    return true;
}

bool venc_dev::venc_set_color_format(OMX_COLOR_FORMATTYPE color_format)
{
    venc_ioctl_msg ioctl_msg = {NULL, NULL};

    if (color_format == OMX_COLOR_FormatYUV420SemiPlanar) {
#ifdef MAX_RES_1080P
        m_sVenc_cfg.inputformat= VEN_INPUTFMT_NV12_16M2KA;
#else
        m_sVenc_cfg.inputformat = VEN_INPUTFMT_NV12;
#endif
    } else {
        DEBUG_PRINT_ERROR("WARNING: Unsupported Color format [%d]", color_format);
#ifdef MAX_RES_1080P
        m_sVenc_cfg.inputformat= VEN_INPUTFMT_NV12_16M2KA;
#else
        m_sVenc_cfg.inputformat = VEN_INPUTFMT_NV12;
#endif
        DEBUG_PRINT_HIGH("Default color format YUV420SemiPlanar is set");
    }

    ioctl_msg.in = (void*)&m_sVenc_cfg;
    ioctl_msg.out = NULL;

    if (ioctl(m_nDriver_fd, VEN_IOCTL_SET_BASE_CFG, (void*)&ioctl_msg) < 0) {
        DEBUG_PRINT_ERROR("ERROR: Request for setting color format failed");
        return false;
    }

    return true;
}

bool venc_dev::venc_set_intra_vop_refresh(OMX_BOOL intra_vop_refresh)
{
    DEBUG_PRINT_LOW("venc_set_intra_vop_refresh: intra_vop = %uc", intra_vop_refresh);

    if (intra_vop_refresh == OMX_TRUE) {
        if (ioctl(m_nDriver_fd, VEN_IOCTL_CMD_REQUEST_IFRAME, NULL) < 0) {
            DEBUG_PRINT_ERROR("ERROR: Request for setting Intra VOP Refresh failed");
            return false;
        }
    } else {
        DEBUG_PRINT_ERROR("ERROR: VOP Refresh is False, no effect");
    }

    return true;
}

bool venc_dev::venc_set_ratectrl_cfg(OMX_VIDEO_CONTROLRATETYPE eControlRate)
{
    venc_ioctl_msg ioctl_msg = {NULL,NULL};
    bool status = true;
    struct venc_ratectrlcfg ratectrl_cfg;

    //rate control
    switch (eControlRate) {
        case OMX_Video_ControlRateDisable:
            ratectrl_cfg.rcmode = VEN_RC_OFF;
            break;
        case OMX_Video_ControlRateVariableSkipFrames:
            ratectrl_cfg.rcmode = VEN_RC_VBR_VFR;
            break;
        case OMX_Video_ControlRateVariable:
            ratectrl_cfg.rcmode = VEN_RC_VBR_CFR;
            break;
        case OMX_Video_ControlRateConstantSkipFrames:
            ratectrl_cfg.rcmode = VEN_RC_CBR_VFR;
            break;
        case OMX_Video_ControlRateConstant:
            ratectrl_cfg.rcmode = VEN_RC_CBR_CFR;
            break;
        default:
            status = false;
            break;
    }

    if (status) {
        ioctl_msg.in = (void*)&ratectrl_cfg;
        ioctl_msg.out = NULL;

        if (ioctl (m_nDriver_fd,VEN_IOCTL_SET_RATE_CTRL_CFG,(void*)&ioctl_msg) < 0) {
            DEBUG_PRINT_ERROR("ERROR: Request for setting rate control failed");
            status = false;
        } else
            rate_ctrl.rcmode = ratectrl_cfg.rcmode;
    }

    return status;
}

bool venc_dev::venc_get_profile_level(OMX_U32 *eProfile,OMX_U32 *eLevel)
{
    bool status = true;

    if (eProfile == NULL || eLevel == NULL) {
        return false;
    }

    if (m_sVenc_cfg.codectype == VEN_CODEC_MPEG4) {
        switch (codec_profile.profile) {
            case VEN_PROFILE_MPEG4_SP:
                *eProfile = OMX_VIDEO_MPEG4ProfileSimple;
                break;
            case VEN_PROFILE_MPEG4_ASP:
                *eProfile = OMX_VIDEO_MPEG4ProfileAdvancedSimple;
                break;
            default:
                *eProfile = OMX_VIDEO_MPEG4ProfileMax;
                status = false;
                break;
        }

        if (!status) {
            return status;
        }

        //profile level
        switch (profile_level.level) {
            case VEN_LEVEL_MPEG4_0:
                *eLevel = OMX_VIDEO_MPEG4Level0;
                break;
            case VEN_LEVEL_MPEG4_1:
                *eLevel = OMX_VIDEO_MPEG4Level1;
                break;
            case VEN_LEVEL_MPEG4_2:
                *eLevel = OMX_VIDEO_MPEG4Level2;
                break;
            case VEN_LEVEL_MPEG4_3:
                *eLevel = OMX_VIDEO_MPEG4Level3;
                break;
            case VEN_LEVEL_MPEG4_4:
                *eLevel = OMX_VIDEO_MPEG4Level4a;
                break;
            case VEN_LEVEL_MPEG4_5:
            case VEN_LEVEL_MPEG4_6:
                *eLevel = OMX_VIDEO_MPEG4Level5;
                break;
            default:
                *eLevel = OMX_VIDEO_MPEG4LevelMax;
                status =  false;
                break;
        }
    } else if (m_sVenc_cfg.codectype == VEN_CODEC_H263) {
        if (codec_profile.profile == VEN_PROFILE_H263_BASELINE) {
            *eProfile = OMX_VIDEO_H263ProfileBaseline;
        } else {
            *eProfile = OMX_VIDEO_H263ProfileMax;
            return false;
        }

        switch (profile_level.level) {
            case VEN_LEVEL_H263_10:
                *eLevel = OMX_VIDEO_H263Level10;
                break;
            case VEN_LEVEL_H263_20:
                *eLevel = OMX_VIDEO_H263Level20;
                break;
            case VEN_LEVEL_H263_30:
                *eLevel = OMX_VIDEO_H263Level30;
                break;
            case VEN_LEVEL_H263_40:
                *eLevel = OMX_VIDEO_H263Level40;
                break;
            case VEN_LEVEL_H263_45:
                *eLevel = OMX_VIDEO_H263Level45;
                break;
            case VEN_LEVEL_H263_50:
                *eLevel = OMX_VIDEO_H263Level50;
                break;
            case VEN_LEVEL_H263_60:
                *eLevel = OMX_VIDEO_H263Level60;
                break;
            case VEN_LEVEL_H263_70:
                *eLevel = OMX_VIDEO_H263Level70;
                break;
            default:
                *eLevel = OMX_VIDEO_H263LevelMax;
                status = false;
                break;
        }
    } else if (m_sVenc_cfg.codectype == VEN_CODEC_H264) {
        switch (codec_profile.profile) {
            case VEN_PROFILE_H264_BASELINE:
                *eProfile = OMX_VIDEO_AVCProfileBaseline;
                break;
            case VEN_PROFILE_H264_MAIN:
                *eProfile = OMX_VIDEO_AVCProfileMain;
                break;
            case VEN_PROFILE_H264_HIGH:
                *eProfile = OMX_VIDEO_AVCProfileHigh;
                break;
            default:
                *eProfile = OMX_VIDEO_AVCProfileMax;
                status = false;
                break;
        }

        if (!status) {
            return status;
        }

        switch (profile_level.level) {
            case VEN_LEVEL_H264_1:
                *eLevel = OMX_VIDEO_AVCLevel1;
                break;
            case VEN_LEVEL_H264_1b:
                *eLevel = OMX_VIDEO_AVCLevel1b;
                break;
            case VEN_LEVEL_H264_1p1:
                *eLevel = OMX_VIDEO_AVCLevel11;
                break;
            case VEN_LEVEL_H264_1p2:
                *eLevel = OMX_VIDEO_AVCLevel12;
                break;
            case VEN_LEVEL_H264_1p3:
                *eLevel = OMX_VIDEO_AVCLevel13;
                break;
            case VEN_LEVEL_H264_2:
                *eLevel = OMX_VIDEO_AVCLevel2;
                break;
            case VEN_LEVEL_H264_2p1:
                *eLevel = OMX_VIDEO_AVCLevel21;
                break;
            case VEN_LEVEL_H264_2p2:
                *eLevel = OMX_VIDEO_AVCLevel22;
                break;
            case VEN_LEVEL_H264_3:
                *eLevel = OMX_VIDEO_AVCLevel3;
                break;
            case VEN_LEVEL_H264_3p1:
                *eLevel = OMX_VIDEO_AVCLevel31;
                break;
            case VEN_LEVEL_H264_3p2:
                *eLevel = OMX_VIDEO_AVCLevel32;
                break;
            case VEN_LEVEL_H264_4:
                *eLevel = OMX_VIDEO_AVCLevel4;
                break;
            default :
                *eLevel = OMX_VIDEO_AVCLevelMax;
                status = false;
                break;
        }
    }

    return status;
}

bool venc_dev::venc_validate_profile_level(OMX_U32 *eProfile, OMX_U32 *eLevel)
{
    OMX_U32 new_profile = 0, new_level = 0;
    unsigned const int *profile_tbl = NULL;
    OMX_U32 mb_per_frame, mb_per_sec;
    bool profile_level_found = false;

    DEBUG_PRINT_LOW("Init profile table for respective codec");

    //validate the ht,width,fps,bitrate and set the appropriate profile and level
    if (m_sVenc_cfg.codectype == VEN_CODEC_MPEG4) {
        if (*eProfile == 0) {
            if (!m_profile_set) {
                *eProfile = OMX_VIDEO_MPEG4ProfileSimple;
            } else {
                switch (codec_profile.profile) {
                    case VEN_PROFILE_MPEG4_ASP:
                        *eProfile = OMX_VIDEO_MPEG4ProfileAdvancedSimple;
                        break;
                    case VEN_PROFILE_MPEG4_SP:
                        *eProfile = OMX_VIDEO_MPEG4ProfileSimple;
                        break;
                    default:
                        DEBUG_PRINT_LOW("%s(): Unknown Error", __func__);
                        return false;
                }
            }
        }

        if (*eLevel == 0 && !m_level_set) {
            *eLevel = OMX_VIDEO_MPEG4LevelMax;
        }

        if (*eProfile == OMX_VIDEO_MPEG4ProfileSimple) {
            profile_tbl = (unsigned int const *)mpeg4_profile_level_table;
        } else if (*eProfile == OMX_VIDEO_MPEG4ProfileAdvancedSimple) {
            profile_tbl = (unsigned int const *)
                (&mpeg4_profile_level_table[MPEG4_ASP_START]);
        } else {
            DEBUG_PRINT_LOW("Unsupported MPEG4 profile type %lu", *eProfile);
            return false;
        }
    } else if (m_sVenc_cfg.codectype == VEN_CODEC_H264) {
        if (*eProfile == 0) {
            if (!m_profile_set) {
                *eProfile = OMX_VIDEO_AVCProfileBaseline;
            } else {
                switch (codec_profile.profile) {
                    case VEN_PROFILE_H264_BASELINE:
                        *eProfile = OMX_VIDEO_AVCProfileBaseline;
                        break;
                    case VEN_PROFILE_H264_MAIN:
                        *eProfile = OMX_VIDEO_AVCProfileMain;
                        break;
                    case VEN_PROFILE_H264_HIGH:
                        *eProfile = OMX_VIDEO_AVCProfileHigh;
                        break;
                    default:
                        DEBUG_PRINT_LOW("%s(): Unknown Error", __func__);
                        return false;
                }
            }
        }

        if (*eLevel == 0 && !m_level_set) {
            *eLevel = OMX_VIDEO_AVCLevelMax;
        }

        if (*eProfile == OMX_VIDEO_AVCProfileBaseline) {
            profile_tbl = (unsigned int const *)h264_profile_level_table;
        } else if (*eProfile == OMX_VIDEO_AVCProfileHigh) {
            profile_tbl = (unsigned int const *)
                (&h264_profile_level_table[H264_HP_START]);
        } else if (*eProfile == OMX_VIDEO_AVCProfileMain) {
            profile_tbl = (unsigned int const *)
                (&h264_profile_level_table[H264_MP_START]);
        } else {
            DEBUG_PRINT_LOW("Unsupported AVC profile type %lu", *eProfile);
            return false;
        }
    } else if (m_sVenc_cfg.codectype == VEN_CODEC_H263) {
        if (*eProfile == 0) {
            if (!m_profile_set) {
                *eProfile = OMX_VIDEO_H263ProfileBaseline;
            } else {
                switch (codec_profile.profile) {
                    case VEN_PROFILE_H263_BASELINE:
                        *eProfile = OMX_VIDEO_H263ProfileBaseline;
                        break;
                    default:
                        DEBUG_PRINT_LOW("%s(): Unknown Error", __func__);
                        return false;
                }
            }
        }

        if (*eLevel == 0 && !m_level_set) {
            *eLevel = OMX_VIDEO_H263LevelMax;
        }

        if (*eProfile == OMX_VIDEO_H263ProfileBaseline) {
            profile_tbl = (unsigned int const *)h263_profile_level_table;
        } else {
            DEBUG_PRINT_LOW("Unsupported H.263 profile type %lu", *eProfile);
            return false;
        }
    } else {
        DEBUG_PRINT_LOW("Invalid codec type");
        return false;
    }

    mb_per_frame = ((m_sVenc_cfg.input_height + 15) >> 4)*
        ((m_sVenc_cfg.input_width + 15)>> 4);

    if ((mb_per_frame >= 3600) && (m_sVenc_cfg.codectype == VEN_CODEC_MPEG4)) {
        if (codec_profile.profile == VEN_PROFILE_MPEG4_ASP)
            profile_level.level = VEN_LEVEL_MPEG4_5;

        if (codec_profile.profile == VEN_PROFILE_MPEG4_SP)
            profile_level.level = VEN_LEVEL_MPEG4_6;

        {
            new_level = profile_level.level;
            new_profile = codec_profile.profile;
            return true;
        }
    }

    mb_per_sec = mb_per_frame * m_sVenc_cfg.fps_num / m_sVenc_cfg.fps_den;

    do {
        if (mb_per_frame <= (int)profile_tbl[0]) {
            if (mb_per_sec <= (int)profile_tbl[1]) {
                if (m_sVenc_cfg.targetbitrate <= (int)profile_tbl[2]) {
                    new_level = (int)profile_tbl[3];
                    new_profile = (int)profile_tbl[4];
                    profile_level_found = true;
                    DEBUG_PRINT_LOW("Appropriate profile/level found %d/%d", new_profile, new_level);
                    break;
                }
            }
        }

        profile_tbl = profile_tbl + 5;
    } while (profile_tbl[0] != 0);

    if (profile_level_found != true) {
        DEBUG_PRINT_LOW("ERROR: Unsupported profile/level");
        return false;
    }

    if ((*eLevel == OMX_VIDEO_MPEG4LevelMax) || (*eLevel == OMX_VIDEO_AVCLevelMax)
            || (*eLevel == OMX_VIDEO_H263LevelMax)) {
        *eLevel = new_level;
    }

    DEBUG_PRINT_LOW("%s: Returning with eProfile = %lu"
            "Level = %lu", __func__, *eProfile, *eLevel);

    return true;
}

bool venc_dev::venc_max_allowed_bitrate_check(OMX_U32 nTargetBitrate)
{
    unsigned const int *profile_tbl = NULL;

    switch (m_sVenc_cfg.codectype) {
        case VEN_CODEC_MPEG4:

            if (m_eProfile == OMX_VIDEO_MPEG4ProfileSimple) {
                profile_tbl = (unsigned int const *)mpeg4_profile_level_table;
            } else if (m_eProfile == OMX_VIDEO_MPEG4ProfileAdvancedSimple) {
                profile_tbl = (unsigned int const *)
                    (&mpeg4_profile_level_table[MPEG4_ASP_START]);
            } else {
                DEBUG_PRINT_ERROR("Unsupported MPEG4 profile type %lu", m_eProfile);
                return false;
            }

            break;
        case VEN_CODEC_H264:

            if (m_eProfile == OMX_VIDEO_AVCProfileBaseline) {
                profile_tbl = (unsigned int const *)h264_profile_level_table;
            } else if (m_eProfile == OMX_VIDEO_AVCProfileHigh) {
                profile_tbl = (unsigned int const *)
                    (&h264_profile_level_table[H264_HP_START]);
            } else if (m_eProfile == OMX_VIDEO_AVCProfileMain) {
                profile_tbl = (unsigned int const *)
                    (&h264_profile_level_table[H264_MP_START]);
            } else {
                DEBUG_PRINT_ERROR("Unsupported AVC profile type %lu", m_eProfile);
                return false;
            }

            break;
        case VEN_CODEC_H263:

            if (m_eProfile == OMX_VIDEO_H263ProfileBaseline) {
                profile_tbl = (unsigned int const *)h263_profile_level_table;
            } else {
                DEBUG_PRINT_ERROR("Unsupported H.263 profile type %lu", m_eProfile);
                return false;
            }

            break;
        default:
            DEBUG_PRINT_ERROR("%s: unknown codec type", __func__);
            return false;
    }

    while (profile_tbl[0] != 0) {
        if (profile_tbl[3] == m_eLevel) {
            if (nTargetBitrate > profile_tbl[2]) {
                DEBUG_PRINT_ERROR("Max. supported bitrate for Profile[%d] & Level[%d]"
                        " is %u", m_eProfile, m_eLevel, profile_tbl[2]);
                return false;
            }
        }

        profile_tbl += 5;
    }

    return true;
}

#ifdef _ANDROID_ICS_
bool venc_dev::venc_set_meta_mode(bool mode)
{
    venc_ioctl_msg ioctl_msg = {NULL,NULL};
    ioctl_msg.in = &mode;
    DEBUG_PRINT_HIGH("Set meta buffer mode: %d", mode);

    if (ioctl(m_nDriver_fd,VEN_IOCTL_SET_METABUFFER_MODE,&ioctl_msg) < 0) {
        DEBUG_PRINT_ERROR(" Set meta buffer mode failed");
        return false;
    }

    return true;
}
#endif
