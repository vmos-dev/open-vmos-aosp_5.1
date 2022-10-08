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
#include<string.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include<unistd.h>
#include <fcntl.h>
#include "video_encoder_device_copper.h"
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
static const unsigned int mpeg4_profile_level_table[][5]=
{
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
static const unsigned int h264_profile_level_table[][5]=
{
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
static const unsigned int h263_profile_level_table[][5]=
{
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

#ifdef INPUT_BUFFER_LOG
FILE *inputBufferFile1;
char inputfilename [] = "/data/input.yuv";
#endif
#ifdef OUTPUT_BUFFER_LOG
FILE *outputBufferFile1;
char outputfilename [] = "/data/output-bitstream.\0\0\0\0";
#endif
//constructor
venc_dev::venc_dev(class omx_venc *venc_class)
{
//nothing to do
venc_handle = venc_class;
etb_count=0;
}

venc_dev::~venc_dev()
{
  //nothing to do
}

void* async_venc_message_thread (void *input)
{
  struct venc_timeout timeout;
  struct venc_msg venc_msg;
   omx_video* omx_venc_base = NULL;
   omx_venc *omx = reinterpret_cast<omx_venc*>(input);
   omx_venc_base = reinterpret_cast<omx_video*>(input);
   OMX_BUFFERHEADERTYPE* omxhdr = NULL;


  prctl(PR_SET_NAME, (unsigned long)"VideoEncCallBackThread", 0, 0, 0);
  timeout.millisec = VEN_TIMEOUT_INFINITE;
  struct v4l2_plane plane;
  struct pollfd pfd;
  struct v4l2_buffer v4l2_buf ={0};
  struct v4l2_event dqevent;
  pfd.events = POLLIN | POLLRDNORM | POLLOUT | POLLWRNORM | POLLRDBAND | POLLPRI;
  pfd.fd = omx->handle->m_nDriver_fd;
  int error_code = 0,rc=0;
  while(1)
  {
    	rc = poll(&pfd, 1, TIMEOUT);
		if (!rc) {
			printf("Poll timedout\n");
			break;
		} else if (rc < 0) {
			printf("Error while polling: %d\n", rc);
			break;
		}
		if ((pfd.revents & POLLIN) || (pfd.revents & POLLRDNORM)) {
			v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
			v4l2_buf.memory = V4L2_MEMORY_USERPTR;
			v4l2_buf.length = 1;
			v4l2_buf.m.planes = &plane;
			rc = ioctl(pfd.fd, VIDIOC_DQBUF, &v4l2_buf);
			if (rc) {
				printf("Failed to dequeue buf: %d from capture capability\n", rc);
				break;
			}
			venc_msg.msgcode=VEN_MSG_OUTPUT_BUFFER_DONE;
			venc_msg.statuscode=VEN_S_SUCCESS;
                        omxhdr=omx_venc_base->m_out_mem_ptr+v4l2_buf.index;
			venc_msg.buf.len= v4l2_buf.m.planes->bytesused;
                        venc_msg.buf.offset = v4l2_buf.m.planes->reserved[1];
                	venc_msg.buf.ptrbuffer = (OMX_U8 *)omx_venc_base->m_pOutput_pmem[v4l2_buf.index].buffer;

			venc_msg.buf.clientdata=(void*)omxhdr;
		} else if((pfd.revents & POLLOUT) || (pfd.revents & POLLWRNORM)) {
			v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
			v4l2_buf.memory = V4L2_MEMORY_USERPTR;
			v4l2_buf.m.planes = &plane;
			rc = ioctl(pfd.fd, VIDIOC_DQBUF, &v4l2_buf);
			if (rc) {
				printf("Failed to dequeue buf: %d from output capability\n", rc);
				break;
			}
                        venc_msg.msgcode=VEN_MSG_INPUT_BUFFER_DONE;
			venc_msg.statuscode=VEN_S_SUCCESS;
                        omxhdr=omx_venc_base->m_inp_mem_ptr+v4l2_buf.index;
                        venc_msg.buf.clientdata=(void*)omxhdr;
		} else if (pfd.revents & POLLPRI){
			rc = ioctl(pfd.fd, VIDIOC_DQEVENT, &dqevent);
			printf("\n Data Recieved = %d \n",dqevent.u.data[0]);
			if(dqevent.u.data[0] == MSM_VIDC_CLOSE_DONE){
				break;
			}
		} else {
			/*TODO: How to handle this case */
			continue;
		}

		if(omx->async_message_process(input,&venc_msg) < 0)
		{
			DEBUG_PRINT_ERROR("\nERROR: Wrong ioctl message");
			break;
		}
  }
  DEBUG_PRINT_HIGH("omx_venc: Async Thread exit\n");
  return NULL;
}

bool venc_dev::venc_open(OMX_U32 codec)
{
  int r;
  unsigned int   alignment = 0,buffer_size = 0, temp =0;

  m_nDriver_fd = open ("/dev/video33",O_RDWR);
  if(m_nDriver_fd == 0)
  {
    DEBUG_PRINT_ERROR("ERROR: Got fd as 0 for msm_vidc_enc, Opening again\n");
    m_nDriver_fd = open ("/dev/video33",O_RDWR);
  }

  if((int)m_nDriver_fd < 0)
  {
    DEBUG_PRINT_ERROR("ERROR: Omx_venc::Comp Init Returning failure\n");
    return false;
  }

  DEBUG_PRINT_LOW("\nm_nDriver_fd = %d\n", m_nDriver_fd);
#ifdef SINGLE_ENCODER_INSTANCE
  OMX_U32 num_instances = 0;
  if(/*ioctl (m_nDriver_fd, VEN_IOCTL_GET_NUMBER_INSTANCES, (void*)&ioctl_msg) < */0 )
  {
    DEBUG_PRINT_ERROR("\nERROR: Request number of encoder instances failed");
  }
  else if (num_instances > 1)
  {
    DEBUG_PRINT_ERROR("\nSecond encoder instance rejected!");
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
  m_sVenc_cfg.inputformat= V4L2_PIX_FMT_NV12;
  if(codec == OMX_VIDEO_CodingMPEG4)
  {
    m_sVenc_cfg.codectype = V4L2_PIX_FMT_MPEG4;
    codec_profile.profile = V4L2_MPEG_VIDEO_MPEG4_PROFILE_SIMPLE;
    profile_level.level = V4L2_MPEG_VIDEO_MPEG4_LEVEL_2;
#ifdef OUTPUT_BUFFER_LOG
    strcat(outputfilename, "m4v");
#endif
  }
  else if(codec == OMX_VIDEO_CodingH263)
  {
    m_sVenc_cfg.codectype = V4L2_PIX_FMT_H263;
    codec_profile.profile = VEN_PROFILE_H263_BASELINE;
    profile_level.level = VEN_LEVEL_H263_20;
#ifdef OUTPUT_BUFFER_LOG
    strcat(outputfilename, "263");
#endif
  }
  if(codec == OMX_VIDEO_CodingAVC)
  {
    m_sVenc_cfg.codectype = V4L2_PIX_FMT_H264;
    codec_profile.profile = V4L2_MPEG_VIDEO_H264_PROFILE_BASELINE;
    profile_level.level = V4L2_MPEG_VIDEO_H264_LEVEL_1_0;
#ifdef OUTPUT_BUFFER_LOG
    strcat(outputfilename, "264");
#endif
  }
  if(/*ioctl (m_nDriver_fd,VEN_IOCTL_SET_BASE_CFG,(void*)&ioctl_msg) < */0 )
  {
    DEBUG_PRINT_ERROR("\nERROR: Request for setting base configuration failed");
    return false;
  }
#ifdef INPUT_BUFFER_LOG
  inputBufferFile1 = fopen (inputfilename, "ab");
#endif
#ifdef OUTPUT_BUFFER_LOG
  outputBufferFile1 = fopen (outputfilename, "ab");
#endif
	int ret;
	struct v4l2_event_subscription sub;
	sub.type=V4L2_EVENT_ALL;
	ret = ioctl(m_nDriver_fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
	if (ret) {
		printf("\n Subscribe Event Failed \n");
		return false;
	}
	struct v4l2_capability cap;
	struct v4l2_fmtdesc fdesc;
	struct v4l2_format fmt;
	struct v4l2_requestbuffers bufreq;

		ret = ioctl(m_nDriver_fd, VIDIOC_QUERYCAP, &cap);
		if (ret) {
		  printf("Failed to query capabilities\n");
		} else {
		  printf("Capabilities: driver_name = %s, card = %s, bus_info = %s,"
				" version = %d, capabilities = %x\n", cap.driver, cap.card,
				cap.bus_info, cap.version, cap.capabilities);
		}
		//printf(" \n VIDIOC_QUERYCAP Successful  \n ");
		ret=0;
		fdesc.type=V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		fdesc.index=0;
		while (ioctl(m_nDriver_fd, VIDIOC_ENUM_FMT, &fdesc) == 0) {
			printf("fmt: description: %s, fmt: %x, flags = %x\n", fdesc.description,
					fdesc.pixelformat, fdesc.flags);
			fdesc.index++;
		}
		//printf("\n VIDIOC_ENUM_FMT CAPTURE Successful  \n ");
		fdesc.type=V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
		fdesc.index=0;
		while (ioctl(m_nDriver_fd, VIDIOC_ENUM_FMT, &fdesc) == 0) {

			printf("fmt: description: %s, fmt: %x, flags = %x\n", fdesc.description,
					fdesc.pixelformat, fdesc.flags);
			fdesc.index++;
		}
		//printf(" \n VIDIOC_ENUM_FMT OUTPUT Successful \n ");

		m_sOutput_buff_property.alignment=m_sInput_buff_property.alignment=4096;

		fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
		fmt.fmt.pix_mp.height = m_sVenc_cfg.input_height;
		fmt.fmt.pix_mp.width = m_sVenc_cfg.input_width;
		fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12;

		ret = ioctl(m_nDriver_fd, VIDIOC_S_FMT, &fmt);
		//printf(" \n VIDIOC_S_FMT OUTPUT Successful \n ");
		m_sInput_buff_property.datasize=fmt.fmt.pix_mp.plane_fmt[0].sizeimage;
		//printf("m_sInput_buff_property.datasize = %d\n",m_sInput_buff_property.datasize);

		fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		fmt.fmt.pix_mp.height = m_sVenc_cfg.input_height;
		fmt.fmt.pix_mp.width = m_sVenc_cfg.input_width;
		fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_H264;

		ret = ioctl(m_nDriver_fd, VIDIOC_S_FMT, &fmt);
		//printf(" \n VIDIOC_S_FMT CAPTURE Successful \n ");
		m_sOutput_buff_property.datasize=fmt.fmt.pix_mp.plane_fmt[0].sizeimage;
		//printf("m_sOutput_buff_property.datasize = %d\n",m_sOutput_buff_property.datasize);
//		struct v4l2_requestbuffers bufreq;

		bufreq.memory = V4L2_MEMORY_USERPTR;
		bufreq.count = 2;

		bufreq.type=V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
		ret = ioctl(m_nDriver_fd,VIDIOC_REQBUFS, &bufreq);
		m_sInput_buff_property.mincount=m_sInput_buff_property.maxcount=m_sInput_buff_property.actualcount=bufreq.count;
		//printf(" \n VIDIOC_REQBUFS OUTPUT Successful \n ");
		//printf("m_sInput_buff_property.datasize = %d\n",m_sInput_buff_property.datasize);
		//printf("m_sInput_buff_property.mincount = %d\n",m_sInput_buff_property.mincount);
		bufreq.type=V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		bufreq.count = 2;
		ret = ioctl(m_nDriver_fd,VIDIOC_REQBUFS, &bufreq);
		m_sOutput_buff_property.mincount=m_sOutput_buff_property.maxcount=m_sOutput_buff_property.actualcount=bufreq.count;
		//printf(" \n VIDIOC_REQBUFS CAPTURE Successful  \n ");
		//printf("m_sInput_buff_property.mincount = %d\n",m_sOutput_buff_property.mincount);

  if(/*ioctl (m_nDriver_fd,VEN_IOCTL_GET_INPUT_BUFFER_REQ,(void*)&ioctl_msg) < */0)
  {
    DEBUG_PRINT_ERROR("\nERROR: Request for getting i/p buffer requirement failed");
    return false;
  }
  if(/*ioctl (m_nDriver_fd,VEN_IOCTL_GET_OUTPUT_BUFFER_REQ,(void*)&ioctl_msg) < */0)
  {
    DEBUG_PRINT_ERROR("\nERROR: Request for getting o/p buffer requirement failed");
    return false;
  }
	///printf("\n \n Setting Profile and Level \n \n ");
  //m_profile_set = false;
  //m_level_set = false;
  if(/*venc_set_profile_level(0, 0)*/0)
  {
    DEBUG_PRINT_HIGH("\n %s(): Init Profile/Level setting success",
        __func__);
  }
  recon_buffers_count = MAX_RECON_BUFFERS;
  return true;
}

void venc_dev::venc_close()
{
  DEBUG_PRINT_LOW("\nvenc_close: fd = %d", m_nDriver_fd);
  if((int)m_nDriver_fd >= 0)
  {
    DEBUG_PRINT_HIGH("\n venc_close(): Calling VEN_IOCTL_CMD_STOP_READ_MSG");
    //(void)ioctl(m_nDriver_fd, VEN_IOCTL_CMD_STOP_READ_MSG,
      //  NULL);
    DEBUG_PRINT_LOW("\nCalling close()\n");

         int rc=0;
	 enum v4l2_buf_type btype;
	 btype = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	 rc = ioctl(m_nDriver_fd, VIDIOC_STREAMOFF, &btype);
	 if (rc) {
		/* STREAMOFF will never fail */
		printf("\n Failed to call streamoff on OUTPUT Port \n");
		}
	 btype = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

	 rc = ioctl(m_nDriver_fd, VIDIOC_STREAMOFF, &btype);
	 if (rc) {
		/* STREAMOFF will never fail */
		printf("\n Failed to call streamoff on CAPTURE Port \n");
		}
        struct v4l2_event_subscription sub;
	sub.type=V4L2_EVENT_ALL;
	rc = ioctl(m_nDriver_fd, VIDIOC_UNSUBSCRIBE_EVENT, &sub);
	if (rc) {
		printf("Failed to get control\n");
		return ;
	}
    close(m_nDriver_fd);
    m_nDriver_fd = -1;
  }
#ifdef INPUT_BUFFER_LOG
  fclose (inputBufferFile1);
#endif
#ifdef OUTPUT_BUFFER_LOG
  fclose (outputBufferFile1);
#endif
}

bool venc_dev::venc_set_buf_req(unsigned long *min_buff_count,
                                unsigned long *actual_buff_count,
                                unsigned long *buff_size,
                                unsigned long port)
{

  unsigned long temp_count = 0;

  if(port == 0)
  {
    if(*actual_buff_count > m_sInput_buff_property.mincount)
    {
      temp_count = m_sInput_buff_property.actualcount;
      m_sInput_buff_property.actualcount = *actual_buff_count;
      if(/*ioctl (m_nDriver_fd,VEN_IOCTL_SET_INPUT_BUFFER_REQ,(void*)&ioctl_msg) < */0)
      {
        DEBUG_PRINT_ERROR("\nERROR: Request for setting i/p buffer requirement failed");
        m_sInput_buff_property.actualcount = temp_count;
        return false;
      }
      DEBUG_PRINT_LOW("\n I/P Count set to %lu\n", *actual_buff_count);
    }
  }
  else
  {
    if(*actual_buff_count > m_sOutput_buff_property.mincount)
    {
	  temp_count = m_sOutput_buff_property.actualcount;
      m_sOutput_buff_property.actualcount = *actual_buff_count;
      if(/*ioctl (m_nDriver_fd,VEN_IOCTL_SET_OUTPUT_BUFFER_REQ,(void*)&ioctl_msg) < */0)
      {
        DEBUG_PRINT_ERROR("\nERROR: Request for setting o/p buffer requirement failed");
		m_sOutput_buff_property.actualcount = temp_count;
        return false;
      }
      DEBUG_PRINT_LOW("\n O/P Count set to %lu\n", *actual_buff_count);
    }
  }

  return true;

}

bool venc_dev::venc_loaded_start()
{
  return true;
}

bool venc_dev::venc_loaded_stop()
{
  return true;
}

bool venc_dev::venc_loaded_start_done()
{
  return true;
}

bool venc_dev::venc_loaded_stop_done()
{
  return true;
}

bool venc_dev::venc_get_seq_hdr(void *buffer,
    unsigned buffer_size, unsigned *header_len)
{
  return true;
}

bool venc_dev::venc_get_buf_req(unsigned long *min_buff_count,
                                unsigned long *actual_buff_count,
                                unsigned long *buff_size,
                                unsigned long port)
{
	struct v4l2_format fmt;
	struct v4l2_requestbuffers bufreq;
	int ret;
  if(port == 0)
  {
    if(/*ioctl (m_nDriver_fd,VEN_IOCTL_GET_INPUT_BUFFER_REQ,(void*)&ioctl_msg) < */0)
    {
      DEBUG_PRINT_ERROR("\nERROR: Request for getting i/p buffer requirement failed");
      return false;
    }

	fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	fmt.fmt.pix_mp.height = m_sVenc_cfg.input_height;
	fmt.fmt.pix_mp.width = m_sVenc_cfg.input_width;
	fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12;
		ret = ioctl(m_nDriver_fd, VIDIOC_G_FMT, &fmt);
		//printf(" \n VIDIOC_S_FMT OUTPUT Successful \n ");
		m_sInput_buff_property.datasize=fmt.fmt.pix_mp.plane_fmt[0].sizeimage;

bufreq.memory = V4L2_MEMORY_USERPTR;
		bufreq.count = 2;

		bufreq.type=V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
		ret = ioctl(m_nDriver_fd,VIDIOC_REQBUFS, &bufreq);
		m_sInput_buff_property.mincount=m_sInput_buff_property.maxcount=m_sInput_buff_property.actualcount=bufreq.count;


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
  }
  else
  {
    if(/*ioctl (m_nDriver_fd,VEN_IOCTL_GET_OUTPUT_BUFFER_REQ,(void*)&ioctl_msg) < */0)
    {
      DEBUG_PRINT_ERROR("\nERROR: Request for getting o/p buffer requirement failed");
      return false;
    }

		fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		fmt.fmt.pix_mp.height = m_sVenc_cfg.input_height;
		fmt.fmt.pix_mp.width = m_sVenc_cfg.input_width;
		fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_H264;

		ret = ioctl(m_nDriver_fd, VIDIOC_S_FMT, &fmt);
		//printf(" \n VIDIOC_S_FMT CAPTURE Successful \n ");
		m_sOutput_buff_property.datasize=fmt.fmt.pix_mp.plane_fmt[0].sizeimage;
		fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		fmt.fmt.pix_mp.height = m_sVenc_cfg.input_height;
		fmt.fmt.pix_mp.width = m_sVenc_cfg.input_width;
		fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_H264;

		ret = ioctl(m_nDriver_fd, VIDIOC_G_FMT, &fmt);
		//printf(" \n VIDIOC_S_FMT CAPTURE Successful \n ");
		m_sOutput_buff_property.datasize=fmt.fmt.pix_mp.plane_fmt[0].sizeimage;


    *min_buff_count = m_sOutput_buff_property.mincount;
    *actual_buff_count = m_sOutput_buff_property.actualcount;
    *buff_size = m_sOutput_buff_property.datasize;
  }

  return true;

}

bool venc_dev::venc_set_param(void *paramData,OMX_INDEXTYPE index )
{
  DEBUG_PRINT_LOW("venc_set_param:: venc-720p\n");
  switch(index)
  {
  case OMX_IndexParamPortDefinition:
    {
      OMX_PARAM_PORTDEFINITIONTYPE *portDefn;
      portDefn = (OMX_PARAM_PORTDEFINITIONTYPE *) paramData;
      DEBUG_PRINT_LOW("venc_set_param: OMX_IndexParamPortDefinition\n");
      if(portDefn->nPortIndex == PORT_INDEX_IN)
      {
        if(!venc_set_color_format(portDefn->format.video.eColorFormat))
        {
          return false;
        }
        if(m_sVenc_cfg.input_height != portDefn->format.video.nFrameHeight ||
          m_sVenc_cfg.input_width != portDefn->format.video.nFrameWidth)
        {
          DEBUG_PRINT_LOW("\n Basic parameter has changed");
          m_sVenc_cfg.input_height = portDefn->format.video.nFrameHeight;
          m_sVenc_cfg.input_width = portDefn->format.video.nFrameWidth;

          if(/*ioctl (m_nDriver_fd,VEN_IOCTL_SET_BASE_CFG,(void*)&ioctl_msg) < */0)
          {
            DEBUG_PRINT_ERROR("\nERROR: Request for setting base config failed");
            return false;
          }

          DEBUG_PRINT_LOW("\n Updating the buffer count/size for the new resolution");
          if(/*ioctl (m_nDriver_fd, VEN_IOCTL_GET_INPUT_BUFFER_REQ,(void*)&ioctl_msg) < */0)
          {
            DEBUG_PRINT_ERROR("\nERROR: Request for getting i/p bufreq failed");
            return false;
          }
          DEBUG_PRINT_LOW("\n Got updated m_sInput_buff_property values: "
                      "datasize = %u, maxcount = %u, actualcnt = %u, "
                      "mincount = %u", m_sInput_buff_property.datasize,
                      m_sInput_buff_property.maxcount, m_sInput_buff_property.actualcount,
                      m_sInput_buff_property.mincount);

          if(/*ioctl (m_nDriver_fd, VEN_IOCTL_GET_OUTPUT_BUFFER_REQ,(void*)&ioctl_msg) < */0)
          {
            DEBUG_PRINT_ERROR("\nERROR: Request for getting o/p bufreq failed");
            return false;
          }

          DEBUG_PRINT_LOW("\n Got updated m_sOutput_buff_property values: "
                      "datasize = %u, maxcount = %u, actualcnt = %u, "
                      "mincount = %u", m_sOutput_buff_property.datasize,
                      m_sOutput_buff_property.maxcount, m_sOutput_buff_property.actualcount,
                      m_sOutput_buff_property.mincount);
          if(/*ioctl (m_nDriver_fd, VEN_IOCTL_SET_OUTPUT_BUFFER_REQ,(void*)&ioctl_msg) <*/ 0)
          {
            DEBUG_PRINT_ERROR("\nERROR: Request for setting o/p bufreq failed");
            return false;
          }

          if((portDefn->nBufferCountActual >= m_sInput_buff_property.mincount) &&
           (portDefn->nBufferCountActual <= m_sInput_buff_property.maxcount))
          {
            m_sInput_buff_property.actualcount = portDefn->nBufferCountActual;
            if(/*ioctl(m_nDriver_fd,VEN_IOCTL_SET_INPUT_BUFFER_REQ,(void*)&ioctl_msg) < */0)
            {
              DEBUG_PRINT_ERROR("\nERROR: Request for setting i/p buffer requirements failed");
              return false;
            }
          }
          if(m_sInput_buff_property.datasize != portDefn->nBufferSize)
          {
            DEBUG_PRINT_ERROR("\nWARNING: Requested i/p bufsize[%u],"
                              "Driver's updated i/p bufsize = %u", portDefn->nBufferSize,
                              m_sInput_buff_property.datasize);
          }
          m_level_set = false;
          if(venc_set_profile_level(0, 0))
          {
            DEBUG_PRINT_HIGH("\n %s(): Profile/Level setting success", __func__);
          }
        }
        else
        {
          if((portDefn->nBufferCountActual >= m_sInput_buff_property.mincount) &&
           (m_sInput_buff_property.maxcount >= portDefn->nBufferCountActual) &&
            (m_sInput_buff_property.datasize == portDefn->nBufferSize))
          {
            m_sInput_buff_property.actualcount = portDefn->nBufferCountActual;
            if(/*ioctl (m_nDriver_fd,VEN_IOCTL_SET_INPUT_BUFFER_REQ,(void*)&ioctl_msg) < */0)
            {
              DEBUG_PRINT_ERROR("\nERROR: ioctl VEN_IOCTL_SET_INPUT_BUFFER_REQ failed");
              return false;
            }
          }
          else
          {
            DEBUG_PRINT_ERROR("\nERROR: Setting Input buffer requirements failed");
            return false;
          }
        }
      }
      else if(portDefn->nPortIndex == PORT_INDEX_OUT)
      {
        if(!venc_set_encode_framerate(portDefn->format.video.xFramerate, 0))
        {
          return false;
        }

        if(!venc_set_target_bitrate(portDefn->format.video.nBitrate, 0))
        {
          return false;
        }

        if( (portDefn->nBufferCountActual >= m_sOutput_buff_property.mincount)
            &&
            (m_sOutput_buff_property.maxcount >= portDefn->nBufferCountActual)
            &&
            (m_sOutput_buff_property.datasize == portDefn->nBufferSize)
          )
        {
          m_sOutput_buff_property.actualcount = portDefn->nBufferCountActual;
          if(/*ioctl (m_nDriver_fd,VEN_IOCTL_SET_OUTPUT_BUFFER_REQ,(void*)&ioctl_msg) < */0)
          {
            DEBUG_PRINT_ERROR("\nERROR: ioctl VEN_IOCTL_SET_OUTPUT_BUFFER_REQ failed");
            return false;
          }
        }
        else
        {
          DEBUG_PRINT_ERROR("\nERROR: Setting Output buffer requirements failed");
          return false;
        }
      }
      else
      {
        DEBUG_PRINT_ERROR("\nERROR: Invalid Port Index for OMX_IndexParamPortDefinition");
      }
      break;
    }
  case OMX_IndexParamVideoPortFormat:
    {
      OMX_VIDEO_PARAM_PORTFORMATTYPE *portFmt;
      portFmt =(OMX_VIDEO_PARAM_PORTFORMATTYPE *)paramData;
      DEBUG_PRINT_LOW("venc_set_param: OMX_IndexParamVideoPortFormat\n");

      if(portFmt->nPortIndex == (OMX_U32) PORT_INDEX_IN)
      {
        if(!venc_set_color_format(portFmt->eColorFormat))
        {
          return false;
        }
      }
      else if(portFmt->nPortIndex == (OMX_U32) PORT_INDEX_OUT)
      {
        if(!venc_set_encode_framerate(portFmt->xFramerate, 0))
        {
          return false;
        }
      }
      else
      {
        DEBUG_PRINT_ERROR("\nERROR: Invalid Port Index for OMX_IndexParamVideoPortFormat");
      }
      break;
    }
  case OMX_IndexParamVideoBitrate:
    {
      OMX_VIDEO_PARAM_BITRATETYPE* pParam;
      pParam = (OMX_VIDEO_PARAM_BITRATETYPE*)paramData;
      DEBUG_PRINT_LOW("venc_set_param: OMX_IndexParamVideoBitrate\n");

      if(pParam->nPortIndex == (OMX_U32) PORT_INDEX_OUT)
      {
        if(!venc_set_target_bitrate(pParam->nTargetBitrate, 0))
        {
          DEBUG_PRINT_ERROR("\nERROR: Target Bit Rate setting failed");
          return false;
        }
        if(!venc_set_ratectrl_cfg(pParam->eControlRate))
        {
          DEBUG_PRINT_ERROR("\nERROR: Rate Control setting failed");
          return false;
        }
      }
      else
      {
        DEBUG_PRINT_ERROR("\nERROR: Invalid Port Index for OMX_IndexParamVideoBitrate");
      }
      break;
    }
  case OMX_IndexParamVideoMpeg4:
    {
      OMX_VIDEO_PARAM_MPEG4TYPE* pParam;
      OMX_U32 bFrames = 0;

      pParam = (OMX_VIDEO_PARAM_MPEG4TYPE*)paramData;
      DEBUG_PRINT_LOW("venc_set_param: OMX_IndexParamVideoMpeg4\n");
      if(pParam->nPortIndex == (OMX_U32) PORT_INDEX_OUT)
      {
        if(!venc_set_voptiming_cfg(pParam->nTimeIncRes))
        {
          DEBUG_PRINT_ERROR("\nERROR: Request for setting vop_timing failed");
          return false;
        }
        m_profile_set = false;
        m_level_set = false;
        if(!venc_set_profile_level (pParam->eProfile, pParam->eLevel))
        {
          DEBUG_PRINT_ERROR("\nERROR: Unsuccessful in updating Profile and level");
          return false;
        }
        else {
          if(pParam->eProfile == OMX_VIDEO_MPEG4ProfileAdvancedSimple)
          {
            if(pParam->nBFrames)
            {
              DEBUG_PRINT_HIGH("INFO: Only 1 Bframe is supported");
              bFrames = 1;
            }
          }
        else
          {
            if(pParam->nBFrames)
            {
              DEBUG_PRINT_ERROR("Warning: B frames not supported\n");
              bFrames = 0;
            }
          }
        }
        if(!venc_set_intra_period (pParam->nPFrames,bFrames))
        {
          DEBUG_PRINT_ERROR("\nERROR: Request for setting intra period failed");
          return false;
        }
        if(!venc_set_multislice_cfg(OMX_IndexParamVideoMpeg4,pParam->nSliceHeaderSpacing))
        {
          DEBUG_PRINT_ERROR("\nERROR: Unsuccessful in updating slice_config");
          return false;
        }
      }
      else
      {
        DEBUG_PRINT_ERROR("\nERROR: Invalid Port Index for OMX_IndexParamVideoMpeg4");
      }
      break;
    }
  case OMX_IndexParamVideoH263:
    {
      OMX_VIDEO_PARAM_H263TYPE* pParam = (OMX_VIDEO_PARAM_H263TYPE*)paramData;
      DEBUG_PRINT_LOW("venc_set_param: OMX_IndexParamVideoH263\n");
      OMX_U32 bFrames = 0;
      if(pParam->nPortIndex == (OMX_U32) PORT_INDEX_OUT)
      {
        m_profile_set = false;
        m_level_set = false;
        if(!venc_set_profile_level (pParam->eProfile, pParam->eLevel))
        {
          DEBUG_PRINT_ERROR("\nERROR: Unsuccessful in updating Profile and level");
          return false;
        }
        if (pParam->nBFrames)
          DEBUG_PRINT_ERROR("\nWARNING: B frame not supported for H.263");

        if(venc_set_intra_period (pParam->nPFrames, bFrames) == false)
        {
          DEBUG_PRINT_ERROR("\nERROR: Request for setting intra period failed");
          return false;
        }
      }
      else
      {
        DEBUG_PRINT_ERROR("\nERROR: Invalid Port Index for OMX_IndexParamVideoH263");
      }
      break;
    }
  case OMX_IndexParamVideoAvc:
    {
      DEBUG_PRINT_LOW("venc_set_param:OMX_IndexParamVideoAvc\n");
      OMX_VIDEO_PARAM_AVCTYPE* pParam = (OMX_VIDEO_PARAM_AVCTYPE*)paramData;
      OMX_U32 bFrames = 0;

      if(pParam->nPortIndex == (OMX_U32) PORT_INDEX_OUT)
      {
        DEBUG_PRINT_LOW("pParam->eProfile :%d ,pParam->eLevel %d\n",
            pParam->eProfile,pParam->eLevel);

        m_profile_set = false;
        m_level_set = false;

        if(!venc_set_profile_level (pParam->eProfile,pParam->eLevel))
        {
          DEBUG_PRINT_ERROR("\nERROR: Unsuccessful in updating Profile and level %d, %d",
                            pParam->eProfile, pParam->eLevel);
          return false;
        }
        else {
          if(pParam->eProfile != OMX_VIDEO_AVCProfileBaseline)
          {
            if(pParam->nBFrames)
            {
              DEBUG_PRINT_HIGH("INFO: Only 1 Bframe is supported");
              bFrames = 1;
            }
          }
        else
          {
            if(pParam->nBFrames)
            {
              DEBUG_PRINT_ERROR("Warning: B frames not supported\n");
              bFrames = 0;
            }
          }
        }
        if(!venc_set_intra_period (pParam->nPFrames, bFrames))
        {
          DEBUG_PRINT_ERROR("\nERROR: Request for setting intra period failed");
          return false;
        }
        if(!venc_set_entropy_config (pParam->bEntropyCodingCABAC, pParam->nCabacInitIdc))
        {
          DEBUG_PRINT_ERROR("\nERROR: Request for setting Entropy failed");
          return false;
        }
        if(!venc_set_inloop_filter (pParam->eLoopFilterMode))
        {
          DEBUG_PRINT_ERROR("\nERROR: Request for setting Inloop filter failed");
		  printf("\n \n Returned here line line 903 \n \n ");
          return false;
        }
        if(!venc_set_multislice_cfg(OMX_IndexParamVideoAvc, pParam->nSliceHeaderSpacing))
        {
          DEBUG_PRINT_ERROR("\nWARNING: Unsuccessful in updating slice_config");
          return false;
        }
      }
      else
      {
        DEBUG_PRINT_ERROR("\nERROR: Invalid Port Index for OMX_IndexParamVideoAvc");
      }
      //TBD, lot of other variables to be updated, yet to decide
      break;
    }
  case OMX_IndexParamVideoIntraRefresh:
    {
      DEBUG_PRINT_LOW("venc_set_param:OMX_IndexParamVideoIntraRefresh\n");
      OMX_VIDEO_PARAM_INTRAREFRESHTYPE *intra_refresh =
        (OMX_VIDEO_PARAM_INTRAREFRESHTYPE *)paramData;
      if(intra_refresh->nPortIndex == (OMX_U32) PORT_INDEX_OUT)
      {
        if(/*venc_set_intra_refresh(intra_refresh->eRefreshMode, intra_refresh->nCirMBs) == */false)
        {
          DEBUG_PRINT_ERROR("\nERROR: Setting Intra refresh failed");
          return false;
        }
      }
      else
      {
        DEBUG_PRINT_ERROR("\nERROR: Invalid Port Index for OMX_IndexParamVideoIntraRefresh");
      }
      break;
    }
  case OMX_IndexParamVideoErrorCorrection:
    {
      DEBUG_PRINT_LOW("venc_set_param:OMX_IndexParamVideoErrorCorrection\n");
      OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *error_resilience =
        (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *)paramData;
      if(error_resilience->nPortIndex == (OMX_U32) PORT_INDEX_OUT)
      {
        if(venc_set_error_resilience(error_resilience) == false)
        {
          DEBUG_PRINT_ERROR("\nERROR: Setting Intra refresh failed");
          return false;
        }
      }
      else
      {
        DEBUG_PRINT_ERROR("\nERROR: Invalid Port Index for OMX_IndexParamVideoErrorCorrection");
      }
      break;
    }
  case OMX_IndexParamVideoProfileLevelCurrent:
    {
      DEBUG_PRINT_LOW("venc_set_param:OMX_IndexParamVideoProfileLevelCurrent\n");
      OMX_VIDEO_PARAM_PROFILELEVELTYPE *profile_level =
      (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)paramData;
      if(profile_level->nPortIndex == (OMX_U32) PORT_INDEX_OUT)
      {
        m_profile_set = false;
        m_level_set = false;
        if(!venc_set_profile_level (profile_level->eProfile,
                                   profile_level->eLevel))
        {
          DEBUG_PRINT_ERROR("\nWARNING: Unsuccessful in updating Profile and level");
          return false;
        }
      }
      else
      {
        DEBUG_PRINT_ERROR("\nERROR: Invalid Port Index for OMX_IndexParamVideoProfileLevelCurrent");
      }
      break;
    }
  case OMX_IndexParamVideoQuantization:
    {
      DEBUG_PRINT_LOW("venc_set_param:OMX_IndexParamVideoQuantization\n");
      OMX_VIDEO_PARAM_QUANTIZATIONTYPE *session_qp =
        (OMX_VIDEO_PARAM_QUANTIZATIONTYPE *)paramData;
      if(session_qp->nPortIndex == (OMX_U32) PORT_INDEX_OUT)
      {
        if(venc_set_session_qp (session_qp->nQpI,
                                session_qp->nQpP,
				session_qp->nQpB) == false)
        {
          DEBUG_PRINT_ERROR("\nERROR: Setting Session QP failed");
          return false;
        }
      }
      else
      {
        DEBUG_PRINT_ERROR("\nERROR: Invalid Port Index for OMX_IndexParamVideoQuantization");
      }
      break;
    }
  case OMX_IndexParamVideoSliceFMO:
  default:
	  DEBUG_PRINT_ERROR("\nERROR: Unsupported parameter in venc_set_param: %u",
      index);
    break;
    //case
  }

  return true;
}

bool venc_dev::venc_set_config(void *configData, OMX_INDEXTYPE index)
{

  DEBUG_PRINT_LOW("\n Inside venc_set_config");

  switch(index)
  {
  case OMX_IndexConfigVideoBitrate:
    {
      OMX_VIDEO_CONFIG_BITRATETYPE *bit_rate = (OMX_VIDEO_CONFIG_BITRATETYPE *)
        configData;
      DEBUG_PRINT_LOW("\n venc_set_config: OMX_IndexConfigVideoBitrate");
      if(bit_rate->nPortIndex == (OMX_U32)PORT_INDEX_OUT)
      {
        if(venc_set_target_bitrate(bit_rate->nEncodeBitrate, 1) == false)
        {
          DEBUG_PRINT_ERROR("\nERROR: Setting Target Bit rate failed");
          return false;
        }
      }
      else
      {
        DEBUG_PRINT_ERROR("\nERROR: Invalid Port Index for OMX_IndexConfigVideoBitrate");
      }
      break;
    }
  case OMX_IndexConfigVideoFramerate:
    {
      OMX_CONFIG_FRAMERATETYPE *frame_rate = (OMX_CONFIG_FRAMERATETYPE *)
        configData;
      DEBUG_PRINT_LOW("\n venc_set_config: OMX_IndexConfigVideoFramerate");
      if(frame_rate->nPortIndex == (OMX_U32)PORT_INDEX_OUT)
      {
        if(venc_set_encode_framerate(frame_rate->xEncodeFramerate, 1) == false)
        {
          DEBUG_PRINT_ERROR("\nERROR: Setting Encode Framerate failed");
          return false;
        }
      }
      else
      {
        DEBUG_PRINT_ERROR("\nERROR: Invalid Port Index for OMX_IndexConfigVideoFramerate");
      }
      break;
    }
  case QOMX_IndexConfigVideoIntraperiod:
    {
      DEBUG_PRINT_LOW("venc_set_param:QOMX_IndexConfigVideoIntraperiod\n");
      QOMX_VIDEO_INTRAPERIODTYPE *intraperiod =
      (QOMX_VIDEO_INTRAPERIODTYPE *)configData;
      if(intraperiod->nPortIndex == (OMX_U32) PORT_INDEX_OUT)
      {
        if(venc_set_intra_period(intraperiod->nPFrames, intraperiod->nBFrames) == false)
        {
          DEBUG_PRINT_ERROR("\nERROR: Request for setting intra period failed");
          return false;
        }
      }
      break;
    }
  case OMX_IndexConfigVideoIntraVOPRefresh:
    {
      OMX_CONFIG_INTRAREFRESHVOPTYPE *intra_vop_refresh = (OMX_CONFIG_INTRAREFRESHVOPTYPE *)
        configData;
      DEBUG_PRINT_LOW("\n venc_set_config: OMX_IndexConfigVideoIntraVOPRefresh");
      if(intra_vop_refresh->nPortIndex == (OMX_U32)PORT_INDEX_OUT)
      {
        if(venc_set_intra_vop_refresh(intra_vop_refresh->IntraRefreshVOP) == false)
        {
          DEBUG_PRINT_ERROR("\nERROR: Setting Encode Framerate failed");
          return false;
        }
      }
      else
      {
        DEBUG_PRINT_ERROR("\nERROR: Invalid Port Index for OMX_IndexConfigVideoFramerate");
      }
      break;
    }
  case OMX_IndexConfigCommonRotate:
    {
      OMX_CONFIG_ROTATIONTYPE *config_rotation =
         reinterpret_cast<OMX_CONFIG_ROTATIONTYPE*>(configData);
      OMX_U32 nFrameWidth;

      DEBUG_PRINT_HIGH("\nvenc_set_config: updating the new Dims");
      nFrameWidth = m_sVenc_cfg.input_width;
      m_sVenc_cfg.input_width  = m_sVenc_cfg.input_height;
      m_sVenc_cfg.input_height = nFrameWidth;
      if(/*ioctl (m_nDriver_fd,VEN_IOCTL_SET_BASE_CFG,(void*)&ioctl_msg) < */0) {
          DEBUG_PRINT_ERROR("\nERROR: Dimension Change for Rotation failed");
          return false;
      }
      break;
    }
  default:
    DEBUG_PRINT_ERROR("\n Unsupported config index = %u", index);
    break;
  }

  return true;
}

unsigned venc_dev::venc_stop( void)
{
    pmem_free();
  return 0;
}

unsigned venc_dev::venc_pause(void)
{
  return 0;
}

unsigned venc_dev::venc_resume(void)
{
  return 0;
}

unsigned venc_dev::venc_start_done(void)
{
  struct venc_msg venc_msg;
  venc_msg.msgcode=VEN_MSG_START;
  venc_msg.statuscode=VEN_S_SUCCESS;
  venc_handle->async_message_process(venc_handle,&venc_msg);
  return 0;
}

unsigned venc_dev::venc_stop_done(void)
{
  struct venc_msg venc_msg;
  venc_msg.msgcode=VEN_MSG_STOP;
  venc_msg.statuscode=VEN_S_SUCCESS;
  venc_handle->async_message_process(venc_handle,&venc_msg);
  return 0;
}

unsigned venc_dev::venc_start(void)
{
	enum v4l2_buf_type buf_type;
	int ret,r;
  DEBUG_PRINT_HIGH("\n %s(): Check Profile/Level set in driver before start",
        __func__);
  if (!venc_set_profile_level(0, 0))
  {
    DEBUG_PRINT_ERROR("\n ERROR: %s(): Driver Profile/Level is NOT SET",
      __func__);
  }
  else
  {
    DEBUG_PRINT_HIGH("\n %s(): Driver Profile[%lu]/Level[%lu] successfully SET",
      __func__, codec_profile.profile, profile_level.level);
  }
  venc_config_print();


  if((codec_profile.profile == V4L2_MPEG_VIDEO_MPEG4_PROFILE_SIMPLE) ||
     (codec_profile.profile == V4L2_MPEG_VIDEO_H264_PROFILE_BASELINE) ||
     (codec_profile.profile == VEN_PROFILE_H263_BASELINE))
    recon_buffers_count = MAX_RECON_BUFFERS - 2;
  else
    recon_buffers_count = MAX_RECON_BUFFERS;

	buf_type=V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        DEBUG_PRINT_LOW("send_command_proxy(): Idle-->Executing\n");

	ret=ioctl(m_nDriver_fd, VIDIOC_STREAMON,&buf_type);

	if (ret) {
		return -1;
	}
	else {
		return 0;
	}
}

OMX_U32 venc_dev::pmem_allocate(OMX_U32 size, OMX_U32 alignment, OMX_U32 count)
{
  OMX_U32 pmem_fd = -1;
  OMX_U32 width, height;
  void *buf_addr = NULL;
  struct pmem_allocation allocation;
  struct venc_recon_addr recon_addr;
  int rc = 0;

#ifdef USE_ION
  recon_buff[count].ion_device_fd = open (MEM_DEVICE,O_RDONLY|O_DSYNC);
  if(recon_buff[count].ion_device_fd < 0)
  {
      DEBUG_PRINT_ERROR("\nERROR: ION Device open() Failed");
      return -1;
  }

  recon_buff[count].alloc_data.len = size;
  recon_buff[count].alloc_data.flags = 0x1 << MEM_HEAP_ID;
  recon_buff[count].alloc_data.align = clip2(alignment);
  if (recon_buff[count].alloc_data.align != 8192)
    recon_buff[count].alloc_data.align = 8192;

  rc = ioctl(recon_buff[count].ion_device_fd,ION_IOC_ALLOC,&recon_buff[count].alloc_data);
  if(rc || !recon_buff[count].alloc_data.handle) {
         DEBUG_PRINT_ERROR("\n ION ALLOC memory failed ");
         recon_buff[count].alloc_data.handle=NULL;
         return -1;
  }

  recon_buff[count].ion_alloc_fd.handle = recon_buff[count].alloc_data.handle;
  rc = ioctl(recon_buff[count].ion_device_fd,ION_IOC_MAP,&recon_buff[count].ion_alloc_fd);
  if(rc) {
        DEBUG_PRINT_ERROR("\n ION MAP failed ");
        recon_buff[count].ion_alloc_fd.fd =-1;
        recon_buff[count].ion_alloc_fd.fd =-1;
        return -1;
  }
  pmem_fd = recon_buff[count].ion_alloc_fd.fd;
#else
  pmem_fd = open(MEM_DEVICE, O_RDWR);

  if ((int)(pmem_fd) < 0)
  {
	DEBUG_PRINT_ERROR("\n Failed to get an pmem handle");
	return -1;
  }

  allocation.size = size;
  allocation.align = clip2(alignment);

  if (allocation.align != 8192)
    allocation.align = 8192;

  if (ioctl(pmem_fd, PMEM_ALLOCATE_ALIGNED, &allocation) < 0)
  {
    DEBUG_PRINT_ERROR("\n Aligment(%u) failed with pmem driver Sz(%lu)",
      allocation.align, allocation.size);
    return -1;
  }
#endif
  buf_addr = mmap(NULL, size,
               PROT_READ | PROT_WRITE,
               MAP_SHARED, pmem_fd, 0);

  if (buf_addr == (void*) MAP_FAILED)
  {
    close(pmem_fd);
    pmem_fd = -1;
    DEBUG_PRINT_ERROR("Error returned in allocating recon buffers buf_addr: %p\n",buf_addr);
#ifdef USE_ION
    if(ioctl(recon_buff[count].ion_device_fd,ION_IOC_FREE,
       &recon_buff[count].alloc_data.handle)) {
      DEBUG_PRINT_ERROR("ion recon buffer free failed");
    }
    recon_buff[count].alloc_data.handle = NULL;
    recon_buff[count].ion_alloc_fd.fd =-1;
    close(recon_buff[count].ion_device_fd);
    recon_buff[count].ion_device_fd =-1;
#endif
    return -1;
  }

  DEBUG_PRINT_HIGH("\n Allocated virt:%p, FD: %d of size %d \n", buf_addr, pmem_fd, size);

  recon_addr.buffer_size = size;
  recon_addr.pmem_fd = pmem_fd;
  recon_addr.offset = 0;
  recon_addr.pbuffer = (unsigned char *)buf_addr;


  if (/*ioctl (m_nDriver_fd,VEN_IOCTL_SET_RECON_BUFFER, (void*)&ioctl_msg) < */0)
  {
    DEBUG_PRINT_ERROR("Failed to set the Recon_buffers\n");
    return -1;
  }

  recon_buff[count].virtual_address = (unsigned char *) buf_addr;
  recon_buff[count].size = size;
  recon_buff[count].offset = 0;
  recon_buff[count].pmem_fd = pmem_fd;

  DEBUG_PRINT_ERROR("\n Allocated virt:%p, FD: %d of size %d at index: %d\n", recon_buff[count].virtual_address,
                     recon_buff[count].pmem_fd, recon_buff[count].size, count);
  return 0;
}

OMX_U32 venc_dev::pmem_free()
{
  int cnt = 0;
  struct venc_recon_addr recon_addr;
  for (cnt = 0; cnt < recon_buffers_count; cnt++)
  {
    if(recon_buff[cnt].pmem_fd)
    {
      recon_addr.pbuffer = recon_buff[cnt].virtual_address;
      recon_addr.offset = recon_buff[cnt].offset;
      recon_addr.pmem_fd = recon_buff[cnt].pmem_fd;
      recon_addr.buffer_size = recon_buff[cnt].size;
      if(/*ioctl(m_nDriver_fd, VEN_IOCTL_FREE_RECON_BUFFER ,&ioctl_msg) < */0)
        DEBUG_PRINT_ERROR("VEN_IOCTL_FREE_RECON_BUFFER failed");
      munmap(recon_buff[cnt].virtual_address, recon_buff[cnt].size);
      close(recon_buff[cnt].pmem_fd);
#ifdef USE_ION
      if(ioctl(recon_buff[cnt].ion_device_fd,ION_IOC_FREE,
         &recon_buff[cnt].alloc_data.handle)) {
        DEBUG_PRINT_ERROR("ion recon buffer free failed");
      }
      recon_buff[cnt].alloc_data.handle = NULL;
      recon_buff[cnt].ion_alloc_fd.fd =-1;
      close(recon_buff[cnt].ion_device_fd);
      recon_buff[cnt].ion_device_fd =-1;
#endif
      DEBUG_PRINT_LOW("\n cleaning Index %d of size %d \n",cnt,recon_buff[cnt].size);
      recon_buff[cnt].pmem_fd = -1;
      recon_buff[cnt].virtual_address = NULL;
      recon_buff[cnt].offset = 0;
      recon_buff[cnt].alignment = 0;
      recon_buff[cnt].size = 0;
    }
  }
  return 0;
}
void venc_dev::venc_config_print()
{

  DEBUG_PRINT_HIGH("\nENC_CONFIG: Codec: %d, Profile %d, level : %d",
                   m_sVenc_cfg.codectype, codec_profile.profile, profile_level.level);

  DEBUG_PRINT_HIGH("\n ENC_CONFIG: Width: %d, Height:%d, Fps: %d",
                   m_sVenc_cfg.input_width, m_sVenc_cfg.input_height,
                   m_sVenc_cfg.fps_num/m_sVenc_cfg.fps_den);

  DEBUG_PRINT_HIGH("\nENC_CONFIG: Bitrate: %d, RC: %d, I-Period: %d",
                   bitrate.target_bitrate, rate_ctrl.rcmode, intra_period.num_pframes);

  DEBUG_PRINT_HIGH("\nENC_CONFIG: qpI: %d, qpP: %d, qpb: %d",
                   session_qp.iframeqp, session_qp.pframqp,session_qp.bframqp);

  DEBUG_PRINT_HIGH("\nENC_CONFIG: VOP_Resolution: %d, Slice-Mode: %d, Slize_Size: %d",
                   voptimecfg.voptime_resolution, multislice.mslice_mode,
                   multislice.mslice_size);

  DEBUG_PRINT_HIGH("\nENC_CONFIG: EntropyMode: %d, CabacModel: %d",
                   entropy.longentropysel, entropy.cabacmodel);

  DEBUG_PRINT_HIGH("\nENC_CONFIG: DB-Mode: %d, alpha: %d, Beta: %d\n",
                   dbkfilter.db_mode, dbkfilter.slicealpha_offset,
                   dbkfilter.slicebeta_offset);

  DEBUG_PRINT_HIGH("\nENC_CONFIG: IntraMB/Frame: %d, HEC: %d\n",
                   intra_refresh.mbcount, hec.header_extension);
}

unsigned venc_dev::venc_flush( unsigned port)
{
  struct venc_bufferflush buffer_index;

  if(port == PORT_INDEX_IN)
  {
	 int rc=0;
	 enum v4l2_buf_type btype;
	 btype = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	 rc = ioctl(m_nDriver_fd, VIDIOC_STREAMOFF, &btype);
	 if (rc) {
		/* STREAMOFF should never fail */
		printf("\n Failed to call streamoff on OUTPUT Port \n");
		return -1;
		}

    return 0;
  }
  else if(port == PORT_INDEX_OUT)
  {
	 int rc=0;
	 enum v4l2_buf_type btype;
	 btype = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	 rc = ioctl(m_nDriver_fd, VIDIOC_STREAMOFF, &btype);
	 if (rc) {
		/* STREAMOFF should never fail  */
		printf("\n Failed to call streamoff on OUTPUT Port \n");
		return -1;
		}

    return 0;

  }
  else
  {
    return -1;
  }
}

//allocating I/P memory from pmem and register with the device


bool venc_dev::venc_use_buf(void *buf_addr, unsigned port,unsigned index)
{

  struct pmem *pmem_tmp;

 struct v4l2_buffer buf;
 struct v4l2_plane plane;
	int rc=0;

  pmem_tmp = (struct pmem *)buf_addr;

  DEBUG_PRINT_LOW("\n venc_use_buf:: pmem_tmp = %p", pmem_tmp);

  if(port == PORT_INDEX_IN)
  {

     buf.index = index;
     buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
     buf.memory = V4L2_MEMORY_USERPTR;
     plane.length = pmem_tmp->size;
     plane.m.userptr = (unsigned long)pmem_tmp->buffer;
     plane.reserved[0] = pmem_tmp->fd;
     plane.reserved[1] = 0;
     plane.data_offset = pmem_tmp->offset;
     buf.m.planes = &plane;
     buf.length = 1;


	 rc = ioctl(m_nDriver_fd, VIDIOC_PREPARE_BUF, &buf);

	if (rc) {
		printf("VIDIOC_PREPARE_BUF Failed at line 1387 \n");
	}

    if(/*ioctl (m_nDriver_fd,VEN_IOCTL_SET_INPUT_BUFFER,&ioctl_msg) < */0)
    {
      DEBUG_PRINT_ERROR("\nERROR: venc_use_buf:set input buffer failed ");
      return false;
    }
  }
  else if(port == PORT_INDEX_OUT)
  {

     buf.index = index;
     buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
     buf.memory = V4L2_MEMORY_USERPTR;
     plane.length = pmem_tmp->size;
     plane.m.userptr = (unsigned long)pmem_tmp->buffer;
     plane.reserved[0] = pmem_tmp->fd;
     plane.reserved[1] = 0;
     plane.data_offset = pmem_tmp->offset;
     buf.m.planes = &plane;
     buf.length = 1;

	rc = ioctl(m_nDriver_fd, VIDIOC_PREPARE_BUF, &buf);

	if (rc) {
		printf("VIDIOC_PREPARE_BUF Failed at line 1414 \n");
	}

    if(/*ioctl (m_nDriver_fd,VEN_IOCTL_SET_OUTPUT_BUFFER,&ioctl_msg) < */0)
    {
      DEBUG_PRINT_ERROR("\nERROR: venc_use_buf:set output buffer failed ");
      return false;
    }
  }
  else
  {
    DEBUG_PRINT_ERROR("\nERROR: venc_use_buf:Invalid Port Index ");
    return false;
  }

  return true;
}

bool venc_dev::venc_free_buf(void *buf_addr, unsigned port)
{
  struct pmem *pmem_tmp;
  struct venc_bufferpayload dev_buffer = {0};

  pmem_tmp = (struct pmem *)buf_addr;

  DEBUG_PRINT_LOW("\n venc_use_buf:: pmem_tmp = %p", pmem_tmp);

  if(port == PORT_INDEX_IN)
  {
    dev_buffer.pbuffer = (OMX_U8 *)pmem_tmp->buffer;
    dev_buffer.fd  = pmem_tmp->fd;
    dev_buffer.maped_size = pmem_tmp->size;
    dev_buffer.sz = pmem_tmp->size;
    dev_buffer.offset = pmem_tmp->offset;
    DEBUG_PRINT_LOW("\n venc_free_buf:pbuffer = %x,fd = %x, offset = %d, maped_size = %d", \
                dev_buffer.pbuffer, \
                dev_buffer.fd, \
                dev_buffer.offset, \
                dev_buffer.maped_size);

    if(/*ioctl (m_nDriver_fd,VEN_IOCTL_CMD_FREE_INPUT_BUFFER,&ioctl_msg) < */0)
    {
      DEBUG_PRINT_ERROR("\nERROR: venc_free_buf: free input buffer failed ");
      return false;
    }
  }
  else if(port == PORT_INDEX_OUT)
  {
    dev_buffer.pbuffer = (OMX_U8 *)pmem_tmp->buffer;
    dev_buffer.fd  = pmem_tmp->fd;
    dev_buffer.sz = pmem_tmp->size;
    dev_buffer.maped_size = pmem_tmp->size;
    dev_buffer.offset = pmem_tmp->offset;

    DEBUG_PRINT_LOW("\n venc_free_buf:pbuffer = %x,fd = %x, offset = %d, maped_size = %d", \
                dev_buffer.pbuffer, \
                dev_buffer.fd, \
                dev_buffer.offset, \
                dev_buffer.maped_size);

    if(/*ioctl (m_nDriver_fd,VEN_IOCTL_CMD_FREE_OUTPUT_BUFFER,&ioctl_msg) < */0)
    {
      DEBUG_PRINT_ERROR("\nERROR: venc_free_buf: free output buffer failed ");
      return false;
    }
  }
  else
  {
    DEBUG_PRINT_ERROR("\nERROR: venc_free_buf:Invalid Port Index ");
    return false;
  }

  return true;
}

bool venc_dev::venc_empty_buf(void *buffer, void *pmem_data_buf,unsigned index,unsigned fd)
{
  struct pmem *temp_buffer;

 struct v4l2_buffer buf;
 struct v4l2_plane plane;
	int rc=0;
  struct OMX_BUFFERHEADERTYPE *bufhdr;

  temp_buffer = (struct pmem *)buffer;


  if(buffer == NULL)
  {
    DEBUG_PRINT_ERROR("\nERROR: venc_etb: buffer is NULL");
    return false;
  }
  bufhdr = (OMX_BUFFERHEADERTYPE *)buffer;

DEBUG_PRINT_LOW("\n Input buffer length %d",bufhdr->nFilledLen);

  if(pmem_data_buf)
  {
    DEBUG_PRINT_LOW("\n Internal PMEM addr for i/p Heap UseBuf: %p", pmem_data_buf);
    plane.m.userptr = (unsigned long)pmem_data_buf;
  }
  else
  {
    DEBUG_PRINT_LOW("\n Shared PMEM addr for i/p PMEM UseBuf/AllocateBuf: %p", bufhdr->pBuffer);
    plane.m.userptr = (unsigned long)bufhdr->pBuffer;
  }

     buf.index = index;
     buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
     buf.memory = V4L2_MEMORY_USERPTR;
     plane.length = bufhdr->nAllocLen;
     plane.bytesused = bufhdr->nFilledLen;
     plane.reserved[0] = fd;
     plane.reserved[1] = 0;
     plane.data_offset = bufhdr->nOffset;
     buf.m.planes = &plane;
     buf.length = 1;


  rc = ioctl(m_nDriver_fd, VIDIOC_QBUF, &buf);
	if (rc) {
		printf("Failed to qbuf to driver");
		return false;
	}

	etb_count++;

	if(etb_count == 1)
	{
	enum v4l2_buf_type buf_type;
	buf_type=V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
		int ret;
	ret = ioctl(m_nDriver_fd, VIDIOC_STREAMON, &buf_type);
	if (ret) {
		printf("Failed to call streamon\n");
	}

	}

  if(/*ioctl(m_nDriver_fd,VEN_IOCTL_CMD_ENCODE_FRAME,&ioctl_msg) < */0)
  {
    /*Generate an async error and move to invalid state*/
    return false;
  }
#ifdef INPUT_BUFFER_LOG

  int y_size = 0;
  int c_offset = 0;

  y_size = m_sVenc_cfg.input_width * m_sVenc_cfg.input_height;
  //chroma offset is y_size aligned to the 2k boundary
  c_offset= (y_size + 2047) & (~(2047));

  if(inputBufferFile1)
  {
    fwrite((const char *)frameinfo.ptrbuffer, y_size, 1,inputBufferFile1);
    fwrite((const char *)(frameinfo.ptrbuffer + c_offset), (y_size>>1), 1,inputBufferFile1);
  }
#endif

  return true;
}
bool venc_dev::venc_fill_buf(void *buffer, void *pmem_data_buf,unsigned index,unsigned fd)
{

  struct pmem *temp_buffer = NULL;
  struct venc_buffer  frameinfo;
  struct v4l2_buffer buf;
  struct v4l2_plane plane;
  int rc=0;
  struct OMX_BUFFERHEADERTYPE *bufhdr;

  if(buffer == NULL)
  {
    return false;
  }
  bufhdr = (OMX_BUFFERHEADERTYPE *)buffer;

if(pmem_data_buf)
  {
    DEBUG_PRINT_LOW("\n Internal PMEM addr for o/p Heap UseBuf: %p", pmem_data_buf);
    plane.m.userptr = (unsigned long)pmem_data_buf;
  }
  else
  {
    DEBUG_PRINT_LOW("\n Shared PMEM addr for o/p PMEM UseBuf/AllocateBuf: %p", bufhdr->pBuffer);
    plane.m.userptr = (unsigned long)bufhdr->pBuffer;
  }

     buf.index = index;
     buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
     buf.memory = V4L2_MEMORY_USERPTR;
     plane.length = bufhdr->nAllocLen;
     plane.bytesused = bufhdr->nFilledLen;
     plane.reserved[0] = fd;
     plane.reserved[1] = 0;
     plane.data_offset = bufhdr->nOffset;
     buf.m.planes = &plane;
     buf.length = 1;

	rc = ioctl(m_nDriver_fd, VIDIOC_QBUF, &buf);
	if (rc) {
		printf("Failed to qbuf to driver");
		return false;
	}


  if(/*ioctl (m_nDriver_fd,VEN_IOCTL_CMD_FILL_OUTPUT_BUFFER,&ioctl_msg) < */0)
  {
    DEBUG_PRINT_ERROR("\nERROR: ioctl VEN_IOCTL_CMD_FILL_OUTPUT_BUFFER failed");
    return false;
  }

  return true;
}

bool venc_dev::venc_set_session_qp(OMX_U32 i_frame_qp, OMX_U32 p_frame_qp,OMX_U32 b_frame_qp)
{
	int rc;
	struct v4l2_control control;

	control.id = V4L2_CID_MPEG_VIDEO_H264_I_FRAME_QP;
	control.value = i_frame_qp;

	printf("Calling IOCTL set control for id=%d, val=%d\n", control.id, control.value);
	rc = ioctl(m_nDriver_fd, VIDIOC_S_CTRL, &control);
	if (rc) {
		printf("Failed to set control\n");
		return false;
	}
	printf("Success IOCTL set control for id=%d, value=%d\n", control.id, control.value);
	session_qp.iframeqp = control.value;

	control.id = V4L2_CID_MPEG_VIDEO_H264_P_FRAME_QP;
	control.value = p_frame_qp;

	printf("Calling IOCTL set control for id=%d, val=%d\n", control.id, control.value);
	rc = ioctl(m_nDriver_fd, VIDIOC_S_CTRL, &control);
	if (rc) {
		printf("Failed to set control\n");
		return false;
	}
	printf("Success IOCTL set control for id=%d, value=%d\n", control.id, control.value);

	session_qp.pframqp = control.value;

	if((codec_profile.profile == V4L2_MPEG_VIDEO_H264_PROFILE_MAIN) ||
     (codec_profile.profile == V4L2_MPEG_VIDEO_H264_PROFILE_HIGH))
	{

	control.id = V4L2_CID_MPEG_VIDEO_H264_B_FRAME_QP;
	control.value = b_frame_qp;

	printf("Calling IOCTL set control for id=%d, val=%d\n", control.id, control.value);
	rc = ioctl(m_nDriver_fd, VIDIOC_S_CTRL, &control);
	if (rc) {
		printf("Failed to set control\n");
		return false;
	}
	printf("Success IOCTL set control for id=%d, value=%d\n", control.id, control.value);

	session_qp.bframqp = control.value;
	}

	if(/*ioctl (m_nDriver_fd,VEN_IOCTL_SET_SESSION_QP,(void*)&ioctl_msg)< */0)
  {
    DEBUG_PRINT_ERROR("\nERROR: Request for setting session qp failed");
    return false;
  }
  return true;
}

bool venc_dev::venc_set_profile_level(OMX_U32 eProfile,OMX_U32 eLevel)
{
  struct venc_profile requested_profile;
  struct ven_profilelevel requested_level;
  unsigned const int *profile_tbl = NULL;
  unsigned long mb_per_frame = 0, mb_per_sec = 0;
  DEBUG_PRINT_LOW("venc_set_profile_level:: eProfile = %d, Level = %d",
    eProfile, eLevel);
  mb_per_frame = ((m_sVenc_cfg.input_height + 15) >> 4)*
                  ((m_sVenc_cfg.input_width + 15) >> 4);
  if((eProfile == 0) && (eLevel == 0) && m_profile_set && m_level_set)
  {
    DEBUG_PRINT_LOW("\n Profile/Level setting complete before venc_start");
    return true;
  }

  DEBUG_PRINT_LOW("\n Validating Profile/Level from table");
  if(!venc_validate_profile_level(&eProfile, &eLevel))
  {
    DEBUG_PRINT_LOW("\nERROR: Profile/Level validation failed");
    return false;
  }

  if(m_sVenc_cfg.codectype == V4L2_PIX_FMT_MPEG4)
  {
    DEBUG_PRINT_LOW("eProfile = %d, OMX_VIDEO_MPEG4ProfileSimple = %d and "
      "OMX_VIDEO_MPEG4ProfileAdvancedSimple = %d", eProfile,
      OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4ProfileAdvancedSimple);
    if(eProfile == OMX_VIDEO_MPEG4ProfileSimple)
    {
      requested_profile.profile = V4L2_MPEG_VIDEO_MPEG4_PROFILE_SIMPLE;
      profile_tbl = (unsigned int const *)
          (&mpeg4_profile_level_table[MPEG4_SP_START]);
      profile_tbl += MPEG4_720P_LEVEL*5;
    }
    else if(eProfile == OMX_VIDEO_MPEG4ProfileAdvancedSimple)
    {
      requested_profile.profile = V4L2_MPEG_VIDEO_MPEG4_PROFILE_ADVANCED_SIMPLE;
      profile_tbl = (unsigned int const *)
          (&mpeg4_profile_level_table[MPEG4_ASP_START]);
      profile_tbl += MPEG4_720P_LEVEL*5;
    }
    else
    {
      DEBUG_PRINT_LOW("\nERROR: Unsupported MPEG4 profile = %u",
        eProfile);
      return false;
    }

    DEBUG_PRINT_LOW("eLevel = %d, OMX_VIDEO_MPEG4Level0 = %d, OMX_VIDEO_MPEG4Level1 = %d,"
      "OMX_VIDEO_MPEG4Level2 = %d, OMX_VIDEO_MPEG4Level3 = %d, OMX_VIDEO_MPEG4Level4 = %d,"
      "OMX_VIDEO_MPEG4Level5 = %d", eLevel, OMX_VIDEO_MPEG4Level0, OMX_VIDEO_MPEG4Level1,
      OMX_VIDEO_MPEG4Level2, OMX_VIDEO_MPEG4Level3, OMX_VIDEO_MPEG4Level4, OMX_VIDEO_MPEG4Level5);

    if(mb_per_frame >= 3600)
    {
      if(requested_profile.profile == V4L2_MPEG_VIDEO_MPEG4_PROFILE_ADVANCED_SIMPLE)
        requested_level.level = V4L2_MPEG_VIDEO_MPEG4_LEVEL_5;
      if(requested_profile.profile == V4L2_MPEG_VIDEO_MPEG4_PROFILE_SIMPLE)
        requested_level.level = V4L2_MPEG_VIDEO_MPEG4_LEVEL_5;
    }
    else
    {
      switch(eLevel)
      {
      case OMX_VIDEO_MPEG4Level0:
        requested_level.level = V4L2_MPEG_VIDEO_MPEG4_LEVEL_0;
        break;
		case OMX_VIDEO_MPEG4Level0b:
        requested_level.level = V4L2_MPEG_VIDEO_MPEG4_LEVEL_0B;
        break;
      case OMX_VIDEO_MPEG4Level1:
        requested_level.level = V4L2_MPEG_VIDEO_MPEG4_LEVEL_1;
        break;
      case OMX_VIDEO_MPEG4Level2:
        requested_level.level = V4L2_MPEG_VIDEO_MPEG4_LEVEL_2;
        break;
      case OMX_VIDEO_MPEG4Level3:
        requested_level.level = V4L2_MPEG_VIDEO_MPEG4_LEVEL_3;
        break;
      case OMX_VIDEO_MPEG4Level4a:
        requested_level.level = V4L2_MPEG_VIDEO_MPEG4_LEVEL_4;
        break;
      case OMX_VIDEO_MPEG4Level5:
        mb_per_sec = mb_per_frame * (m_sVenc_cfg.fps_num / m_sVenc_cfg.fps_den);
		if((requested_profile.profile == V4L2_MPEG_VIDEO_MPEG4_PROFILE_ADVANCED_SIMPLE) && (mb_per_frame >= profile_tbl[0]) &&
           (mb_per_sec >= profile_tbl[1]))
        {
          DEBUG_PRINT_LOW("\nMPEG4 Level 6 is set for 720p resolution");
          requested_level.level = V4L2_MPEG_VIDEO_MPEG4_LEVEL_5;
        }
        else
        {
          DEBUG_PRINT_LOW("\nMPEG4 Level 5 is set for non-720p resolution");
          requested_level.level = V4L2_MPEG_VIDEO_MPEG4_LEVEL_5;
        }
        break;
      default:
        return false;
        // TODO update corresponding levels for MPEG4_LEVEL_3b,MPEG4_LEVEL_6
        break;
      }
    }
  }
  else if(m_sVenc_cfg.codectype == V4L2_PIX_FMT_H263)
  {
    if(eProfile == OMX_VIDEO_H263ProfileBaseline)
    {
      requested_profile.profile = VEN_PROFILE_H263_BASELINE;
    }
    else
    {
      DEBUG_PRINT_LOW("\nERROR: Unsupported H.263 profile = %u",
        requested_profile.profile);
      return false;
    }
    //profile level
    switch(eLevel)
    {
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
  }
  else if(m_sVenc_cfg.codectype == V4L2_PIX_FMT_H264)
  {
    if(eProfile == OMX_VIDEO_AVCProfileBaseline)
    {
      requested_profile.profile = V4L2_MPEG_VIDEO_H264_PROFILE_BASELINE;
    }
    else if(eProfile == OMX_VIDEO_AVCProfileMain)
    {
      requested_profile.profile = V4L2_MPEG_VIDEO_H264_PROFILE_MAIN;
    }
	else if(eProfile == OMX_VIDEO_AVCProfileExtended)
    {
      requested_profile.profile = V4L2_MPEG_VIDEO_H264_PROFILE_EXTENDED;
    }
    else if(eProfile == OMX_VIDEO_AVCProfileHigh)
    {
      requested_profile.profile = V4L2_MPEG_VIDEO_H264_PROFILE_HIGH;
    }
	else if(eProfile == OMX_VIDEO_AVCProfileHigh10)
    {
      requested_profile.profile = V4L2_MPEG_VIDEO_H264_PROFILE_HIGH_10;
    }
	else if(eProfile == OMX_VIDEO_AVCProfileHigh422)
    {
      requested_profile.profile = V4L2_MPEG_VIDEO_H264_PROFILE_HIGH_422;
    }
	else if(eProfile == OMX_VIDEO_AVCProfileHigh444)
    {
      requested_profile.profile = V4L2_MPEG_VIDEO_H264_PROFILE_HIGH_444_PREDICTIVE;
    }
    else
    {
      DEBUG_PRINT_LOW("\nERROR: Unsupported H.264 profile = %u",
        requested_profile.profile);
      return false;
    }
    //profile level
    switch(eLevel)
    {
    case OMX_VIDEO_AVCLevel1:
      requested_level.level = V4L2_MPEG_VIDEO_H264_LEVEL_1_0;
      break;
    case OMX_VIDEO_AVCLevel1b:
      requested_level.level = V4L2_MPEG_VIDEO_H264_LEVEL_1B;
      break;
    case OMX_VIDEO_AVCLevel11:
      requested_level.level = V4L2_MPEG_VIDEO_H264_LEVEL_1_1;
      break;
    case OMX_VIDEO_AVCLevel12:
      requested_level.level = V4L2_MPEG_VIDEO_H264_LEVEL_1_2;
      break;
    case OMX_VIDEO_AVCLevel13:
      requested_level.level = V4L2_MPEG_VIDEO_H264_LEVEL_1_3;
      break;
    case OMX_VIDEO_AVCLevel2:
      requested_level.level = V4L2_MPEG_VIDEO_H264_LEVEL_2_0;
      break;
    case OMX_VIDEO_AVCLevel21:
      requested_level.level = V4L2_MPEG_VIDEO_H264_LEVEL_2_1;
      break;
    case OMX_VIDEO_AVCLevel22:
      requested_level.level = V4L2_MPEG_VIDEO_H264_LEVEL_2_2;
      break;
    case OMX_VIDEO_AVCLevel3:
      requested_level.level = V4L2_MPEG_VIDEO_H264_LEVEL_3_0;
      break;
    case OMX_VIDEO_AVCLevel31:
      requested_level.level = V4L2_MPEG_VIDEO_H264_LEVEL_3_1;
      break;
    case OMX_VIDEO_AVCLevel32:
      requested_level.level = V4L2_MPEG_VIDEO_H264_LEVEL_3_2;
      break;
    case OMX_VIDEO_AVCLevel4:
      requested_level.level = V4L2_MPEG_VIDEO_H264_LEVEL_4_0;
      break;
    case OMX_VIDEO_AVCLevel41:
      requested_level.level = V4L2_MPEG_VIDEO_H264_LEVEL_4_1;
      break;
    case OMX_VIDEO_AVCLevel42:
      requested_level.level = V4L2_MPEG_VIDEO_H264_LEVEL_4_2;
      break;
    case OMX_VIDEO_AVCLevel5:
      requested_level.level = V4L2_MPEG_VIDEO_H264_LEVEL_5_0;
      break;
    case OMX_VIDEO_AVCLevel51:
      requested_level.level = V4L2_MPEG_VIDEO_H264_LEVEL_5_1;
      break;
    default :
      DEBUG_PRINT_ERROR("\nERROR: Unsupported H.264 level= %u",
        requested_level.level);
      return false;
      break;
    }
  }
  if(!m_profile_set)
  {
	int rc;
	struct v4l2_control control;

	control.id = V4L2_CID_MPEG_VIDEO_H264_PROFILE;
	control.value = requested_profile.profile;

	printf("Calling IOCTL set control for id=%d, val=%d\n", control.id, control.value);
	rc = ioctl(m_nDriver_fd, VIDIOC_S_CTRL, &control);
	if (rc) {
		printf("Failed to set control\n");
		return false;
	}
	printf("Success IOCTL set control for id=%d, value=%d\n", control.id, control.value);


    if(/*ioctl (m_nDriver_fd,VEN_IOCTL_SET_CODEC_PROFILE,(void*)&ioctl_msg)< */0)
    {
      DEBUG_PRINT_ERROR("\nERROR: Request for setting profile failed");
      return false;
    }
    codec_profile.profile = control.value;
    m_profile_set = true;
  }

  if(!m_level_set)
  {
	int rc;
	struct v4l2_control control;
	control.id = V4L2_CID_MPEG_VIDEO_H264_LEVEL;
	control.value = requested_level.level;
	printf("Calling IOCTL set control for id=%d, val=%d\n", control.id, control.value);
	rc = ioctl(m_nDriver_fd, VIDIOC_S_CTRL, &control);
	if (rc) {
		printf("Failed to set control\n");
		return false;
	}
	printf("Success IOCTL set control for id=%d, value=%d\n", control.id, control.value);

    if(/*ioctl (m_nDriver_fd,VEN_IOCTL_SET_PROFILE_LEVEL,(void*)&ioctl_msg)< */0)
    {
      DEBUG_PRINT_ERROR("\nERROR: Request for setting profile level failed");
      return false;
    }
    profile_level.level = control.value;
    m_level_set = true;
  }

  return true;
}

bool venc_dev::venc_set_voptiming_cfg( OMX_U32 TimeIncRes)
{

  struct venc_voptimingcfg vop_timing_cfg;

  DEBUG_PRINT_LOW("\n venc_set_voptiming_cfg: TimeRes = %u",
    TimeIncRes);

  vop_timing_cfg.voptime_resolution = TimeIncRes;

  if(/*ioctl (m_nDriver_fd,VEN_IOCTL_SET_VOP_TIMING_CFG,(void*)&ioctl_msg)< */0)
  {
    DEBUG_PRINT_ERROR("\nERROR: Request for setting Vop Timing failed");
    return false;
  }

  voptimecfg.voptime_resolution = vop_timing_cfg.voptime_resolution;
  return true;
}

bool venc_dev::venc_set_intra_period(OMX_U32 nPFrames, OMX_U32 nBFrames)
{

  DEBUG_PRINT_LOW("\n venc_set_intra_period: nPFrames = %u",
    nPFrames);
	int rc;
	struct v4l2_control control;
  if((codec_profile.profile != V4L2_MPEG_VIDEO_MPEG4_PROFILE_ADVANCED_SIMPLE) &&
     (codec_profile.profile != V4L2_MPEG_VIDEO_H264_PROFILE_MAIN) &&
     (codec_profile.profile != V4L2_MPEG_VIDEO_H264_PROFILE_HIGH))
  {
	  nBFrames=0;
  }

	control.id = V4L2_CID_MPEG_VIDC_VIDEO_NUM_P_FRAMES;
	control.value = nPFrames;

	rc = ioctl(m_nDriver_fd, VIDIOC_S_CTRL, &control);
	if (rc) {
		printf("Failed to set control\n");
		return false;
	}
	printf("Success IOCTL set control for id=%d, value=%d\n", control.id, control.value);

        intra_period.num_pframes = control.value;
	control.id = V4L2_CID_MPEG_VIDC_VIDEO_NUM_B_FRAMES;
	control.value = nBFrames;
	printf("Calling IOCTL set control for id=%d, val=%d\n", control.id, control.value);
	rc = ioctl(m_nDriver_fd, VIDIOC_S_CTRL, &control);
	if (rc) {
		printf("Failed to set control\n");
		return false;
	}
	printf("Success IOCTL set control for id=%d, value=%d\n", control.id, control.value);


  if(/*ioctl (m_nDriver_fd,VEN_IOCTL_SET_INTRA_PERIOD,(void*)&ioctl_msg)< */0)
  {
    DEBUG_PRINT_ERROR("\nERROR: Request for setting intra period failed");
    return false;
  }
  intra_period.num_bframes = control.value;
  return true;
}

bool venc_dev::venc_set_entropy_config(OMX_BOOL enable, OMX_U32 i_cabac_level)
{
  //struct venc_entropycfg entropy_cfg;

 // memset(&entropy_cfg,0,sizeof(entropy_cfg));
	int rc;
	struct v4l2_control control;

  DEBUG_PRINT_LOW("\n venc_set_entropy_config: CABAC = %u level: %u", enable, i_cabac_level);

  if(enable &&(codec_profile.profile != V4L2_MPEG_VIDEO_H264_PROFILE_BASELINE)){

	  control.value = V4L2_MPEG_VIDEO_H264_ENTROPY_MODE_CABAC;
	  control.id = V4L2_CID_MPEG_VIDEO_H264_ENTROPY_MODE;

	  printf("Calling IOCTL set control for id=%d, val=%d\n", control.id, control.value);
	rc = ioctl(m_nDriver_fd, VIDIOC_S_CTRL, &control);
	if (rc) {
		printf("Failed to set control\n");
		return false;
	}
	printf("Success IOCTL set control for id=%d, value=%d\n", control.id, control.value);
	entropy.longentropysel = control.value;
	  if (i_cabac_level == 0) {
         control.value = V4L2_CID_MPEG_VIDC_VIDEO_H264_CABAC_MODEL_0;
      }
      else if (i_cabac_level == 1) {
         control.value = V4L2_CID_MPEG_VIDC_VIDEO_H264_CABAC_MODEL_1;
      }
      else if (i_cabac_level == 2) {
         control.value = V4L2_CID_MPEG_VIDC_VIDEO_H264_CABAC_MODEL_2;
      }

	  control.id = V4L2_CID_MPEG_VIDC_VIDEO_H264_CABAC_MODEL;
	  //control.value = entropy_cfg.cabacmodel;
		printf("Calling IOCTL set control for id=%d, val=%d\n", control.id, control.value);
	rc = ioctl(m_nDriver_fd, VIDIOC_S_CTRL, &control);
	if (rc) {
		printf("Failed to set control\n");
		return false;
	}
	printf("Success IOCTL set control for id=%d, value=%d\n", control.id, control.value);
	entropy.longentropysel=control.value;
  }
  else if(!enable){
    control.value =  V4L2_MPEG_VIDEO_H264_ENTROPY_MODE_CAVLC;
	control.id = V4L2_CID_MPEG_VIDEO_H264_ENTROPY_MODE;
	printf("Calling IOCTL set control for id=%d, val=%d\n", control.id, control.value);
	rc = ioctl(m_nDriver_fd, VIDIOC_S_CTRL, &control);
	if (rc) {
		printf("Failed to set control\n");
		return false;
	}
	printf("Success IOCTL set control for id=%d, value=%d\n", control.id, control.value);
	entropy.longentropysel=control.value;
	//entropy_cfg.longentropysel = control.value;
    }
  else{
    DEBUG_PRINT_ERROR("\nInvalid Entropy mode for Baseline Profile");
    return false;
  }

  if(/*ioctl (m_nDriver_fd,VEN_IOCTL_SET_ENTROPY_CFG,(void*)&ioctl_msg)< */0)
  {
    DEBUG_PRINT_ERROR("\nERROR: Request for setting entropy config failed");
    return false;
  }
  //entropy.longentropysel = entropy_cfg.longentropysel;
  //entropy.cabacmodel  = entropy_cfg.cabacmodel;
  return true;
}

bool venc_dev::venc_set_multislice_cfg(OMX_INDEXTYPE Codec, OMX_U32 nSlicesize) // MB
{
	int rc;
	struct v4l2_control control;
  bool status = true;
  //struct venc_multiclicecfg multislice_cfg;

  if((Codec != OMX_IndexParamVideoH263)  && (nSlicesize)){
   // multislice_cfg.mslice_mode = VEN_MSLICE_CNT_MB;
    //multislice_cfg.mslice_size = nSlicesize;
	  control.value =  V4L2_MPEG_VIDEO_MULTI_SICE_MODE_MAX_MB;
    }
  else{
   control.value =  V4L2_MPEG_VIDEO_MULTI_SLICE_MODE_SINGLE;
   //multislice_cfg.mslice_mode = VEN_MSLICE_OFF;
    //multislice_cfg.mslice_size = 0;
  }
	control.id = V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MODE;
	printf("Calling IOCTL set control for id=%d, val=%d\n", control.id, control.value);
	rc = ioctl(m_nDriver_fd, VIDIOC_S_CTRL, &control);
	if (rc) {
		printf("Failed to set control\n");
		return false;
	}
	printf("Success IOCTL set control for id=%d, value=%d\n", control.id, control.value);
	multislice.mslice_mode=control.value;

	if(multislice.mslice_mode!=V4L2_MPEG_VIDEO_MULTI_SLICE_MODE_SINGLE){

	control.id = V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_MB;
	control.value = nSlicesize;
	printf("Calling IOCTL set control for id=%d, val=%d\n", control.id, control.value);
	rc = ioctl(m_nDriver_fd, VIDIOC_S_CTRL, &control);
	if (rc) {
		printf("Failed to set control\n");
		return false;
	}
	printf("Success IOCTL set control for id=%d, value=%d\n", control.id, control.value);
	multislice.mslice_size=control.value;

	}


  if(/*ioctl (m_nDriver_fd, VEN_IOCTL_SET_MULTI_SLICE_CFG,(void*)&ioctl_msg) < */0)
  {
    DEBUG_PRINT_ERROR("\nERROR: Request for setting multi-slice cfg failed");
    status = false;
  }
  else
  {
    //multislice.mslice_mode = multislice_cfg.mslice_mode;
    //multislice.mslice_size = nSlicesize;
  }
  return status;
}

bool venc_dev::venc_set_intra_refresh(OMX_VIDEO_INTRAREFRESHTYPE ir_mode, OMX_U32 irMBs)
{
  bool status = true;
  int rc;
  struct v4l2_control control_mode,control_mbs;
   control_mode.id = V4L2_CID_MPEG_VIDC_VIDEO_INTRA_REFRESH_MODE;
  // There is no disabled mode.  Disabled mode is indicated by a 0 count.
  if (irMBs == 0 || ir_mode == OMX_VIDEO_IntraRefreshMax)
  {
	  control_mode.value = V4L2_CID_MPEG_VIDC_VIDEO_INTRA_REFRESH_NONE;
  }
  else if ((ir_mode == OMX_VIDEO_IntraRefreshCyclic) &&
           (irMBs < ((m_sVenc_cfg.input_width * m_sVenc_cfg.input_height)>>8)))
  {
	  control_mode.value = V4L2_CID_MPEG_VIDC_VIDEO_INTRA_REFRESH_CYCLIC;
	  control_mbs.id=V4L2_CID_MPEG_VIDC_VIDEO_CIR_MBS;
	  control_mbs.value=irMBs;
  }
  else if ((ir_mode == OMX_VIDEO_IntraRefreshAdaptive) &&
           (irMBs < ((m_sVenc_cfg.input_width * m_sVenc_cfg.input_height)>>8)))
  {
	  control_mode.value = V4L2_CID_MPEG_VIDC_VIDEO_INTRA_REFRESH_ADAPTIVE;
	  control_mbs.id=V4L2_CID_MPEG_VIDC_VIDEO_AIR_MBS;
	  control_mbs.value=irMBs;
  }
  else if ((ir_mode == OMX_VIDEO_IntraRefreshBoth) &&
           (irMBs < ((m_sVenc_cfg.input_width * m_sVenc_cfg.input_height)>>8)))
  {
	  control_mode.value = V4L2_CID_MPEG_VIDC_VIDEO_INTRA_REFRESH_CYCLIC_ADAPTIVE;
  }
  else
  {
    DEBUG_PRINT_ERROR("\nERROR: Invalid IntraRefresh Parameters:"
                      "mb count: %d, mb mode:%d", irMBs, ir_mode);
    return false;
  }

	printf("Calling IOCTL set control for id=%d, val=%d\n", control_mode.id, control_mode.value);
	rc = ioctl(m_nDriver_fd, VIDIOC_S_CTRL, &control_mode);
	if (rc) {
		printf("Failed to set control\n");
		return false;
	}
	printf("Success IOCTL set control for id=%d, value=%d\n", control_mode.id, control_mode.value);

	printf("Calling IOCTL set control for id=%d, val=%d\n", control_mbs.id, control_mbs.value);
	rc = ioctl(m_nDriver_fd, VIDIOC_S_CTRL, &control_mbs);
	if (rc) {
		printf("Failed to set control\n");
		return false;
	}
	printf("Success IOCTL set control for id=%d, value=%d\n", control_mbs.id, control_mbs.value);

  if(/*ioctl (m_nDriver_fd,VEN_IOCTL_SET_INTRA_REFRESH,(void*)&ioctl_msg) < */0)
  {
    DEBUG_PRINT_ERROR("\nERROR: Request for setting Intra Refresh failed");
    status = false;
  }
  else
  {
    intra_refresh.irmode = control_mode.value;
    intra_refresh.mbcount = control_mbs.value;
  }
  return status;
}

bool venc_dev::venc_set_error_resilience(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE* error_resilience)
{
   bool status = true;
   struct venc_headerextension hec_cfg;
   struct venc_multiclicecfg multislice_cfg;

   if (m_sVenc_cfg.codectype == V4L2_PIX_FMT_MPEG4) {
      if (error_resilience->bEnableHEC) {
         hec_cfg.header_extension = 1;
      }
      else {
         hec_cfg.header_extension = 0;
      }

      if (/*ioctl (m_nDriver_fd,VEN_IOCTL_SET_HEC,(void*)&ioctl_msg) < */0) {
         DEBUG_PRINT_ERROR("\nERROR: Request for setting HEader Error correction failed");
         return false;
      }
      hec.header_extension = error_resilience->bEnableHEC;
   }

   if (error_resilience->bEnableRVLC) {
     DEBUG_PRINT_ERROR("\n RVLC is not Supported");
     return false;
   }

   if (( m_sVenc_cfg.codectype != OMX_VIDEO_CodingH263) &&
       (error_resilience->bEnableDataPartitioning)) {
     DEBUG_PRINT_ERROR("\n DataPartioning are not Supported for MPEG4/H264");
     return false;
     }

   if (( m_sVenc_cfg.codectype != OMX_VIDEO_CodingH263) &&
            (error_resilience->nResynchMarkerSpacing)) {
     multislice_cfg.mslice_mode = VEN_MSLICE_CNT_BYTE;
       multislice_cfg.mslice_size = error_resilience->nResynchMarkerSpacing;
     }
   else if (m_sVenc_cfg.codectype == OMX_VIDEO_CodingH263 &&
            error_resilience->bEnableDataPartitioning) {
      multislice_cfg.mslice_mode = VEN_MSLICE_GOB;
      multislice_cfg.mslice_size = 0;
      }
      else {
        multislice_cfg.mslice_mode = VEN_MSLICE_OFF;
        multislice_cfg.mslice_size = 0;
        }
   DEBUG_PRINT_LOW("\n %s(): mode = %u, size = %u", __func__, multislice_cfg.mslice_mode,
                   multislice_cfg.mslice_size);
   if (/*ioctl (m_nDriver_fd,VEN_IOCTL_SET_MULTI_SLICE_CFG,(void*)&ioctl_msg) < */0) {
      DEBUG_PRINT_ERROR("\nERROR: Request for setting multi-slice cfg failed");
      status = false;
   }
   else
   {
     multislice.mslice_mode = multislice_cfg.mslice_mode ;
     multislice.mslice_size = multislice_cfg.mslice_size;

   }
   return status;
}

bool venc_dev::venc_set_inloop_filter(OMX_VIDEO_AVCLOOPFILTERTYPE loopfilter)
{
	int rc;
	struct v4l2_control control;
	control.id=V4L2_CID_MPEG_VIDEO_H264_LOOP_FILTER_MODE;
  if (loopfilter == OMX_VIDEO_AVCLoopFilterEnable){
	  control.value=V4L2_MPEG_VIDEO_H264_LOOP_FILTER_MODE_ENABLED;
  }
  else if(loopfilter == OMX_VIDEO_AVCLoopFilterDisable){
	  control.value=V4L2_MPEG_VIDEO_H264_LOOP_FILTER_MODE_DISABLED;
  }
  else if(loopfilter == OMX_VIDEO_AVCLoopFilterDisableSliceBoundary){
	  control.value=V4L2_MPEG_VIDEO_H264_LOOP_FILTER_MODE_DISABLED_AT_SLICE_BOUNDARY;
  }

	  printf("Calling IOCTL set control for id=%d, val=%d\n", control.id, control.value);
	rc = ioctl(m_nDriver_fd, VIDIOC_S_CTRL, &control);
	if (rc) {
		return false;
	}
	printf("Success IOCTL set control for id=%d, value=%d\n", control.id, control.value);

	dbkfilter.db_mode=control.value;

	control.id=V4L2_CID_MPEG_VIDEO_H264_LOOP_FILTER_ALPHA;
	control.value=0;

	printf("Calling IOCTL set control for id=%d, val=%d\n", control.id, control.value);
	rc = ioctl(m_nDriver_fd, VIDIOC_S_CTRL, &control);
	if (rc) {
		return false;
	}
	printf("Success IOCTL set control for id=%d, value=%d\n", control.id, control.value);
	control.id=V4L2_CID_MPEG_VIDEO_H264_LOOP_FILTER_BETA;
	control.value=0;
	printf("Calling IOCTL set control for id=%d, val=%d\n", control.id, control.value);
	rc = ioctl(m_nDriver_fd, VIDIOC_S_CTRL, &control);
	if (rc) {
		return false;
	}
	printf("Success IOCTL set control for id=%d, value=%d\n", control.id, control.value);


  if(/*ioctl (m_nDriver_fd,VEN_IOCTL_SET_DEBLOCKING_CFG,(void*)&ioctl_msg)< */0)
  {
    DEBUG_PRINT_ERROR("\nERROR: Request for setting inloop filter failed");
    return false;
  }

  dbkfilter.slicealpha_offset = dbkfilter.slicebeta_offset = 0;
  return true;
}

bool venc_dev::venc_set_target_bitrate(OMX_U32 nTargetBitrate, OMX_U32 config)
{
  DEBUG_PRINT_LOW("\n venc_set_target_bitrate: bitrate = %u",
    nTargetBitrate);
	struct v4l2_control control;
	int rc;
	control.id = V4L2_CID_MPEG_VIDEO_BITRATE;
	control.value = nTargetBitrate/1000;

	printf("Calling IOCTL set control for id=%d, val=%d\n", control.id, control.value);
	rc = ioctl(m_nDriver_fd, VIDIOC_S_CTRL, &control);
	if (rc) {
		printf("Failed to set control\n");
		return false;
	}
	printf("Success IOCTL set control for id=%d, value=%d\n", control.id, control.value);


  if(/*ioctl (m_nDriver_fd,VEN_IOCTL_SET_TARGET_BITRATE,(void*)&ioctl_msg) < */0)
  {
    DEBUG_PRINT_ERROR("\nERROR: Request for setting bit rate failed");
    return false;
  }
  m_sVenc_cfg.targetbitrate = control.value*1000;
  bitrate.target_bitrate = control.value*1000;
  if(!config)
  {
    m_level_set = false;
    if(venc_set_profile_level(0, 0))
    {
      DEBUG_PRINT_HIGH("Calling set level (Bitrate) with %d\n",profile_level.level);
    }
  }
  return true;
}

bool venc_dev::venc_set_encode_framerate(OMX_U32 encode_framerate, OMX_U32 config)
{

	struct v4l2_control control;
	int rc;
	control.id = V4L2_CID_MPEG_VIDC_VIDEO_FRAME_RATE;
	control.value = encode_framerate;
	printf("Calling IOCTL set control for id=%d, val=%d\n", control.id, control.value);
	rc = ioctl(m_nDriver_fd, VIDIOC_S_CTRL, &control);
	if (rc) {
		printf("Failed to set control\n");
		return false;
	}
	printf("Success IOCTL set control for id=%d, value=%d\n", control.id, control.value);
  if(//ioctl(m_nDriver_fd, VEN_IOCTL_SET_FRAME_RATE,
      /*(void*)&ioctl_msg) < */0)
  {
    DEBUG_PRINT_ERROR("\nERROR: Request for setting framerate failed");
    return false;
  }

  m_sVenc_cfg.fps_den = 1;
  m_sVenc_cfg.fps_num = control.value;
  if(!config)
  {
    m_level_set = false;
    if(venc_set_profile_level(0, 0))
    {
      DEBUG_PRINT_HIGH("Calling set level (Framerate) with %d\n",profile_level.level);
    }
  }
  return true;
}

bool venc_dev::venc_set_color_format(OMX_COLOR_FORMATTYPE color_format)
{
  DEBUG_PRINT_LOW("\n venc_set_color_format: color_format = %u ", color_format);

  if(color_format == OMX_COLOR_FormatYUV420SemiPlanar)
  {
  m_sVenc_cfg.inputformat= VEN_INPUTFMT_NV12_16M2KA;
  }
  else
  {
    DEBUG_PRINT_ERROR("\nWARNING: Unsupported Color format [%d]", color_format);
    m_sVenc_cfg.inputformat= VEN_INPUTFMT_NV12_16M2KA;
    DEBUG_PRINT_HIGH("\n Default color format YUV420SemiPlanar is set");
  }
  if (/*ioctl(m_nDriver_fd, VEN_IOCTL_SET_BASE_CFG, (void*)&ioctl_msg) < */0)
  {
    DEBUG_PRINT_ERROR("\nERROR: Request for setting color format failed");
    return false;
  }
  return true;
}

bool venc_dev::venc_set_intra_vop_refresh(OMX_BOOL intra_vop_refresh)
{
  DEBUG_PRINT_LOW("\n venc_set_intra_vop_refresh: intra_vop = %uc", intra_vop_refresh);
  if(intra_vop_refresh == OMX_TRUE)
  {
    if(/*ioctl(m_nDriver_fd, VEN_IOCTL_CMD_REQUEST_IFRAME, NULL) < */0)
    {
      DEBUG_PRINT_ERROR("\nERROR: Request for setting Intra VOP Refresh failed");
      return false;
    }
  }
  else
  {
    DEBUG_PRINT_ERROR("\nERROR: VOP Refresh is False, no effect");
  }
  return true;
}

bool venc_dev::venc_set_ratectrl_cfg(OMX_VIDEO_CONTROLRATETYPE eControlRate)
{
  bool status = true;
	struct v4l2_control control;
	int rc;
	control.id = V4L2_CID_MPEG_VIDC_VIDEO_RATE_CONTROL;
  switch(eControlRate)
  {
  case OMX_Video_ControlRateDisable:
	  control.value=V4L2_CID_MPEG_VIDC_VIDEO_RATE_CONTROL_OFF;
    break;
  case OMX_Video_ControlRateVariableSkipFrames:
	  control.value=V4L2_CID_MPEG_VIDC_VIDEO_RATE_CONTROL_VBR_VFR;
    break;
  case OMX_Video_ControlRateVariable:
	  control.value=V4L2_CID_MPEG_VIDC_VIDEO_RATE_CONTROL_VBR_CFR;
    break;
  case OMX_Video_ControlRateConstantSkipFrames:
	    control.value=V4L2_CID_MPEG_VIDC_VIDEO_RATE_CONTROL_CBR_VFR;
    break;
  case OMX_Video_ControlRateConstant:
	    control.value=V4L2_CID_MPEG_VIDC_VIDEO_RATE_CONTROL_CBR_CFR;
    break;
  default:
    status = false;
    break;
  }

  if(status)
  {

	  printf("Calling IOCTL set control for id=%d, val=%d\n", control.id, control.value);
	rc = ioctl(m_nDriver_fd, VIDIOC_S_CTRL, &control);
	if (rc) {
		printf("Failed to set control\n");
		return false;
	}
	printf("Success IOCTL set control for id=%d, value=%d\n", control.id, control.value);


    if(/*ioctl (m_nDriver_fd,VEN_IOCTL_SET_RATE_CTRL_CFG,(void*)&ioctl_msg) < */0)
    {
      DEBUG_PRINT_ERROR("\nERROR: Request for setting rate control failed");
      status = false;
    }
    else
      rate_ctrl.rcmode = control.value;
  }
  return status;
}

bool venc_dev::venc_get_profile_level(OMX_U32 *eProfile,OMX_U32 *eLevel)
{
  bool status = true;
  if(eProfile == NULL || eLevel == NULL)
  {
    return false;
  }

  if(m_sVenc_cfg.codectype == V4L2_PIX_FMT_MPEG4)
  {
    switch(codec_profile.profile)
    {
    case V4L2_MPEG_VIDEO_MPEG4_PROFILE_SIMPLE:
      *eProfile = OMX_VIDEO_MPEG4ProfileSimple;
      break;
    case V4L2_MPEG_VIDEO_MPEG4_PROFILE_ADVANCED_SIMPLE:
      *eProfile = OMX_VIDEO_MPEG4ProfileAdvancedSimple;
      break;
    default:
      *eProfile = OMX_VIDEO_MPEG4ProfileMax;
      status = false;
      break;
    }

    if(!status)
    {
      return status;
    }

    //profile level
    switch(profile_level.level)
    {
    case V4L2_MPEG_VIDEO_MPEG4_LEVEL_0:
      *eLevel = OMX_VIDEO_MPEG4Level0;
      break;
	  case V4L2_MPEG_VIDEO_MPEG4_LEVEL_0B:
      *eLevel = OMX_VIDEO_MPEG4Level0b;
      break;
    case V4L2_MPEG_VIDEO_MPEG4_LEVEL_1:
      *eLevel = OMX_VIDEO_MPEG4Level1;
      break;
    case V4L2_MPEG_VIDEO_MPEG4_LEVEL_2:
      *eLevel = OMX_VIDEO_MPEG4Level2;
      break;
    case V4L2_MPEG_VIDEO_MPEG4_LEVEL_3:
      *eLevel = OMX_VIDEO_MPEG4Level3;
      break;
    case V4L2_MPEG_VIDEO_MPEG4_LEVEL_4:
      *eLevel = OMX_VIDEO_MPEG4Level4;
      break;
    case V4L2_MPEG_VIDEO_MPEG4_LEVEL_5:
      *eLevel = OMX_VIDEO_MPEG4Level5;
      break;
    default:
      *eLevel = OMX_VIDEO_MPEG4LevelMax;
      status =  false;
      break;
    }
  }
  else if(m_sVenc_cfg.codectype == V4L2_PIX_FMT_H263)
  {
    if(codec_profile.profile == VEN_PROFILE_H263_BASELINE)
    {
      *eProfile = OMX_VIDEO_H263ProfileBaseline;
    }
    else
    {
      *eProfile = OMX_VIDEO_H263ProfileMax;
      return false;
    }
    switch(profile_level.level)
    {
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
  }
  else if(m_sVenc_cfg.codectype == V4L2_PIX_FMT_H264)
  {
    switch(codec_profile.profile)
    {
    case V4L2_MPEG_VIDEO_H264_PROFILE_BASELINE:
      *eProfile = OMX_VIDEO_AVCProfileBaseline;
      break;
    case V4L2_MPEG_VIDEO_H264_PROFILE_MAIN:
      *eProfile = OMX_VIDEO_AVCProfileMain;
      break;
    case V4L2_MPEG_VIDEO_H264_PROFILE_HIGH:
      *eProfile = OMX_VIDEO_AVCProfileHigh;
      break;
    case V4L2_MPEG_VIDEO_H264_PROFILE_EXTENDED:
      *eProfile = OMX_VIDEO_AVCProfileExtended;
      break;
	case V4L2_MPEG_VIDEO_H264_PROFILE_HIGH_10:
      *eProfile = OMX_VIDEO_AVCProfileHigh10;
      break;
	case V4L2_MPEG_VIDEO_H264_PROFILE_HIGH_422:
      *eProfile = OMX_VIDEO_AVCProfileHigh422;
      break;
	case V4L2_MPEG_VIDEO_H264_PROFILE_HIGH_444_PREDICTIVE:
      *eProfile = OMX_VIDEO_AVCProfileHigh444;
      break;
    default:
      *eProfile = OMX_VIDEO_AVCProfileMax;
      status = false;
      break;
    }

    if(!status)
    {
      return status;
    }

    switch(profile_level.level)
    {
    case V4L2_MPEG_VIDEO_H264_LEVEL_1_0:
      *eLevel = OMX_VIDEO_AVCLevel1;
      break;
    case V4L2_MPEG_VIDEO_H264_LEVEL_1B:
      *eLevel = OMX_VIDEO_AVCLevel1b;
      break;
    case V4L2_MPEG_VIDEO_H264_LEVEL_1_1:
      *eLevel = OMX_VIDEO_AVCLevel11;
      break;
    case V4L2_MPEG_VIDEO_H264_LEVEL_1_2:
      *eLevel = OMX_VIDEO_AVCLevel12;
      break;
    case V4L2_MPEG_VIDEO_H264_LEVEL_1_3:
      *eLevel = OMX_VIDEO_AVCLevel13;
      break;
    case V4L2_MPEG_VIDEO_H264_LEVEL_2_0:
      *eLevel = OMX_VIDEO_AVCLevel2;
      break;
    case V4L2_MPEG_VIDEO_H264_LEVEL_2_1:
      *eLevel = OMX_VIDEO_AVCLevel21;
      break;
    case V4L2_MPEG_VIDEO_H264_LEVEL_2_2:
      *eLevel = OMX_VIDEO_AVCLevel22;
      break;
    case V4L2_MPEG_VIDEO_H264_LEVEL_3_0:
      *eLevel = OMX_VIDEO_AVCLevel3;
      break;
    case V4L2_MPEG_VIDEO_H264_LEVEL_3_1:
      *eLevel = OMX_VIDEO_AVCLevel31;
      break;
    case V4L2_MPEG_VIDEO_H264_LEVEL_3_2:
      *eLevel = OMX_VIDEO_AVCLevel32;
      break;
    case V4L2_MPEG_VIDEO_H264_LEVEL_4_0:
      *eLevel = OMX_VIDEO_AVCLevel4;
      break;
	case V4L2_MPEG_VIDEO_H264_LEVEL_4_1:
      *eLevel = OMX_VIDEO_AVCLevel41;
      break;
	case V4L2_MPEG_VIDEO_H264_LEVEL_4_2:
      *eLevel = OMX_VIDEO_AVCLevel42;
      break;
	case V4L2_MPEG_VIDEO_H264_LEVEL_5_0:
      *eLevel = OMX_VIDEO_AVCLevel5;
      break;
	case V4L2_MPEG_VIDEO_H264_LEVEL_5_1:
      *eLevel = OMX_VIDEO_AVCLevel51;
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

  DEBUG_PRINT_LOW("\n Init profile table for respective codec");
  //validate the ht,width,fps,bitrate and set the appropriate profile and level
  if(m_sVenc_cfg.codectype == V4L2_PIX_FMT_MPEG4)
  {
      if(*eProfile == 0)
      {
        if(!m_profile_set)
        {
          *eProfile = OMX_VIDEO_MPEG4ProfileSimple;
        }
        else
        {
          switch(codec_profile.profile)
          {
          case V4L2_MPEG_VIDEO_MPEG4_PROFILE_SIMPLE:
              *eProfile = OMX_VIDEO_MPEG4ProfileAdvancedSimple;
            break;
          case V4L2_MPEG_VIDEO_MPEG4_PROFILE_ADVANCED_SIMPLE:
              *eProfile = OMX_VIDEO_MPEG4ProfileSimple;
            break;
          default:
            DEBUG_PRINT_LOW("\n %s(): Unknown Error", __func__);
            return false;
          }
        }
      }

      if(*eLevel == 0 && !m_level_set)
      {
        *eLevel = OMX_VIDEO_MPEG4LevelMax;
      }

      if(*eProfile == OMX_VIDEO_MPEG4ProfileSimple)
      {
        profile_tbl = (unsigned int const *)mpeg4_profile_level_table;
      }
      else if(*eProfile == OMX_VIDEO_MPEG4ProfileAdvancedSimple)
      {
        profile_tbl = (unsigned int const *)
          (&mpeg4_profile_level_table[MPEG4_ASP_START]);
      }
      else
      {
        DEBUG_PRINT_LOW("\n Unsupported MPEG4 profile type %lu", *eProfile);
        return false;
      }
  }
  else if(m_sVenc_cfg.codectype == V4L2_PIX_FMT_H264)
  {
      if(*eProfile == 0)
      {
        if(!m_profile_set)
        {
          *eProfile = OMX_VIDEO_AVCProfileBaseline;
        }
        else
        {
          switch(codec_profile.profile)
          {
          case V4L2_MPEG_VIDEO_H264_PROFILE_BASELINE:
            *eProfile = OMX_VIDEO_AVCProfileBaseline;
            break;
          case V4L2_MPEG_VIDEO_H264_PROFILE_MAIN:
            *eProfile = OMX_VIDEO_AVCProfileMain;
            break;
          case V4L2_MPEG_VIDEO_H264_PROFILE_EXTENDED:
            *eProfile = OMX_VIDEO_AVCProfileExtended;
            break;
		  case V4L2_MPEG_VIDEO_H264_PROFILE_HIGH:
            *eProfile = OMX_VIDEO_AVCProfileHigh;
            break;
		  case V4L2_MPEG_VIDEO_H264_PROFILE_HIGH_10:
            *eProfile = OMX_VIDEO_AVCProfileHigh10;
            break;
		  case V4L2_MPEG_VIDEO_H264_PROFILE_HIGH_422:
            *eProfile = OMX_VIDEO_AVCProfileHigh422;
            break;
		  case V4L2_MPEG_VIDEO_H264_PROFILE_HIGH_444_PREDICTIVE:
            *eProfile = OMX_VIDEO_AVCProfileHigh444;
            break;
		  default:
            DEBUG_PRINT_LOW("\n %s(): Unknown Error", __func__);
            return false;
          }
        }
      }

      if(*eLevel == 0 && !m_level_set)
      {
        *eLevel = OMX_VIDEO_AVCLevelMax;
      }

      if(*eProfile == OMX_VIDEO_AVCProfileBaseline)
      {
        profile_tbl = (unsigned int const *)h264_profile_level_table;
      }
      else if(*eProfile == OMX_VIDEO_AVCProfileHigh)
      {
        profile_tbl = (unsigned int const *)
          (&h264_profile_level_table[H264_HP_START]);
      }
      else if(*eProfile == OMX_VIDEO_AVCProfileMain)
      {
        profile_tbl = (unsigned int const *)
          (&h264_profile_level_table[H264_MP_START]);
      }
      else
      {
        DEBUG_PRINT_LOW("\n Unsupported AVC profile type %lu", *eProfile);
        return false;
      }
  }
  else if(m_sVenc_cfg.codectype == V4L2_PIX_FMT_H263)
  {
      if(*eProfile == 0)
      {
        if(!m_profile_set)
        {
          *eProfile = OMX_VIDEO_H263ProfileBaseline;
        }
        else
        {
          switch(codec_profile.profile)
          {
          case VEN_PROFILE_H263_BASELINE:
            *eProfile = OMX_VIDEO_H263ProfileBaseline;
            break;
          default:
            DEBUG_PRINT_LOW("\n %s(): Unknown Error", __func__);
            return false;
          }
        }
      }

      if(*eLevel == 0 && !m_level_set)
      {
        *eLevel = OMX_VIDEO_H263LevelMax;
      }

      if(*eProfile == OMX_VIDEO_H263ProfileBaseline)
      {
        profile_tbl = (unsigned int const *)h263_profile_level_table;
      }
      else
      {
        DEBUG_PRINT_LOW("\n Unsupported H.263 profile type %lu", *eProfile);
        return false;
      }
  }
  else
  {
    DEBUG_PRINT_LOW("\n Invalid codec type");
    return false;
  }

  mb_per_frame = ((m_sVenc_cfg.input_height + 15) >> 4)*
                   ((m_sVenc_cfg.input_width + 15)>> 4);

  if((mb_per_frame >= 3600) && (m_sVenc_cfg.codectype == V4L2_PIX_FMT_MPEG4))
  {
    if(codec_profile.profile == V4L2_MPEG_VIDEO_MPEG4_PROFILE_ADVANCED_SIMPLE)
      profile_level.level = V4L2_MPEG_VIDEO_MPEG4_LEVEL_5;
    if(codec_profile.profile == V4L2_MPEG_VIDEO_MPEG4_PROFILE_SIMPLE)
      profile_level.level = V4L2_MPEG_VIDEO_MPEG4_LEVEL_5;
    {
      new_level = profile_level.level;
      new_profile = codec_profile.profile;
      return true;
    }
  }

  mb_per_sec = mb_per_frame * m_sVenc_cfg.fps_num / m_sVenc_cfg.fps_den;

  do{
      if(mb_per_frame <= (int)profile_tbl[0])
      {
        if(mb_per_sec <= (int)profile_tbl[1])
        {
          if(m_sVenc_cfg.targetbitrate <= (int)profile_tbl[2])
          {
              new_level = (int)profile_tbl[3];
              new_profile = (int)profile_tbl[4];
              profile_level_found = true;
              DEBUG_PRINT_LOW("\n Appropriate profile/level found %d/%d\n", new_profile, new_level);
              break;
          }
        }
      }
      profile_tbl = profile_tbl + 5;
  }while(profile_tbl[0] != 0);

  if (profile_level_found != true)
  {
    DEBUG_PRINT_LOW("\n ERROR: Unsupported profile/level\n");
    return false;
  }

  if((*eLevel == OMX_VIDEO_MPEG4LevelMax) || (*eLevel == OMX_VIDEO_AVCLevelMax)
     || (*eLevel == OMX_VIDEO_H263LevelMax))
  {
    *eLevel = new_level;
  }
  DEBUG_PRINT_HIGH("%s: Returning with eProfile = %lu"
      "Level = %lu", __func__, *eProfile, *eLevel);

  return true;
}
#ifdef _ANDROID_ICS_
bool venc_dev::venc_set_meta_mode(bool mode)
{
  if(/*ioctl(m_nDriver_fd,VEN_IOCTL_SET_METABUFFER_MODE,&ioctl_msg) < */0)
  {
    DEBUG_PRINT_ERROR(" Set meta buffer mode failed");
    return false;
  }
  return true;
}
#endif
