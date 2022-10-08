/*--------------------------------------------------------------------------
Copyright (c) 2010-2011, Code Aurora Forum. All rights reserved.

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
/*============================================================================
                    V E N C _ T E S T. C P P

DESCRIPTION

 This is the OMX test app .

REFERENCES

============================================================================*/

//usage
// FILE QVGA MP4 24 384000 100 enc_qvga.yuv QVGA_24.m4v
// FILE QCIF MP4 15 96000 0 foreman.qcif.yuv output_qcif.m4v
// FILE VGA MP4 24 1200000 218 enc_vga.yuv vga_output.m4v
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/mman.h>
//#include <sys/time.h>
#include <time.h>
#include <sys/ioctl.h>
#include <limits.h>
#include <string.h>
//#include <sys/stat.h>
#include "OMX_QCOMExtns.h"
#include "OMX_Core.h"

#define QCOM_EXT 1

#include "OMX_Core.h"
#include "OMX_Video.h"
#include "OMX_Component.h"
#include "camera_test.h"
#include "fb_test.h"
#include "venc_util.h"
#include "extra_data_handler.h"
#ifdef USE_ION
#include <linux/msm_ion.h>
#endif

//////////////////////////
// MACROS
//////////////////////////

#define CHK(result) if (result != OMX_ErrorNone) { E("*************** error *************"); exit(0); }
#define TEST_LOG
#ifdef VENC_SYSLOG
#include "cutils/log.h"
/// Debug message macro
#define D(fmt, ...) LOGE("venc_test Debug %s::%d "fmt"\n",              \
                         __FUNCTION__, __LINE__,                        \
                         ## __VA_ARGS__)

/// Error message macro
#define E(fmt, ...) LOGE("venc_test Error %s::%d "fmt"\n",            \
                         __FUNCTION__, __LINE__,                      \
                         ## __VA_ARGS__)

#else
     #ifdef TEST_LOG
       #define D(fmt, ...) fprintf(stderr, "venc_test Debug %s::%d "fmt"\n",   \
                            __FUNCTION__, __LINE__,                     \
                            ## __VA_ARGS__)

     /// Error message macro
      #define E(fmt, ...) fprintf(stderr, "venc_test Error %s::%d "fmt"\n", \
                            __FUNCTION__, __LINE__,                   \
                            ## __VA_ARGS__)
     #else
      #define D(fmt, ...)
      #define E(fmt, ...)
         #endif

#endif

//////////////////////////
// CONSTANTS
//////////////////////////
static const int MAX_MSG = 100;
//#warning do not hardcode these use port definition
static const int PORT_INDEX_IN = 0;
static const int PORT_INDEX_OUT = 1;

static const int NUM_IN_BUFFERS = 10;
static const int NUM_OUT_BUFFERS = 10;

unsigned int num_in_buffers = 0;
unsigned int num_out_buffers = 0;

//////////////////////////
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
#define FractionToQ16(q,num,den) { OMX_U32 power; Log2(den,power); q = num << (16 - power); }

//////////////////////////
// TYPES
//////////////////////////
struct ProfileType
{
   OMX_VIDEO_CODINGTYPE eCodec;
   OMX_VIDEO_MPEG4LEVELTYPE eLevel;
   OMX_VIDEO_CONTROLRATETYPE eControlRate;
   OMX_VIDEO_AVCSLICEMODETYPE eSliceMode;
   OMX_U32 nFrameWidth;
   OMX_U32 nFrameHeight;
   OMX_U32 nFrameBytes;
#ifdef BADGER
   OMX_U32 nFramestride;
   OMX_U32 nFrameScanlines;
   OMX_U32 nFrameRead;
#endif
   OMX_U32 nBitrate;
   float nFramerate;
   char* cInFileName;
   char* cOutFileName;
   OMX_U32 nUserProfile;
};

enum MsgId
{
   MSG_ID_OUTPUT_FRAME_DONE,
   MSG_ID_INPUT_FRAME_DONE,
   MSG_ID_MAX
};
union MsgData
{
   struct
   {
      OMX_BUFFERHEADERTYPE* pBuffer;
   } sBitstreamData;
};
struct Msg
{
   MsgId id;
   MsgData data;
};
struct MsgQ
{
   Msg q[MAX_MSG];
   int head;
   int size;
};

enum Mode
{
   MODE_PREVIEW,
   MODE_DISPLAY,
   MODE_PROFILE,
   MODE_FILE_ENCODE,
   MODE_LIVE_ENCODE
};

enum ResyncMarkerType
{
   RESYNC_MARKER_NONE,     ///< No resync marker
   RESYNC_MARKER_BYTE,     ///< BYTE Resync marker for MPEG4, H.264
   RESYNC_MARKER_MB,       ///< MB resync marker for MPEG4, H.264
   RESYNC_MARKER_GOB       ///< GOB resync marker for H.263
};

union DynamicConfigData
{
   OMX_VIDEO_CONFIG_BITRATETYPE bitrate;
   OMX_CONFIG_FRAMERATETYPE framerate;
   QOMX_VIDEO_INTRAPERIODTYPE intraperiod;
   OMX_CONFIG_INTRAREFRESHVOPTYPE intravoprefresh;
   OMX_CONFIG_ROTATIONTYPE rotation;
   float f_framerate;
};

struct DynamicConfig
{
   bool pending;
   unsigned frame_num;
   OMX_INDEXTYPE config_param;
   union DynamicConfigData config_data;
};

#ifdef USE_ION
struct enc_ion
{
   int ion_device_fd;
   struct ion_allocation_data alloc_data;
   struct ion_fd_data ion_alloc_fd;
};
#endif

//////////////////////////
// MODULE VARS
//////////////////////////
static pthread_mutex_t m_mutex;
static pthread_cond_t m_signal;
static MsgQ m_sMsgQ;

//#warning determine how many buffers we really have
OMX_STATETYPE m_eState = OMX_StateInvalid;
OMX_COMPONENTTYPE m_sComponent;
OMX_HANDLETYPE m_hHandle = NULL;
OMX_BUFFERHEADERTYPE* m_pOutBuffers[NUM_OUT_BUFFERS] = {NULL};
OMX_BUFFERHEADERTYPE* m_pInBuffers[NUM_IN_BUFFERS] = {NULL};
OMX_BOOL m_bInFrameFree[NUM_IN_BUFFERS];

ProfileType m_sProfile;

static int m_nFramePlay = 0;
static int m_eMode = MODE_PREVIEW;
static int m_nInFd = -1;
static int m_nOutFd = -1;
static int m_nTimeStamp = 0;
static int m_nFrameIn = 0; // frames pushed to encoder
static int m_nFrameOut = 0; // frames returned by encoder
static int m_nAVCSliceMode = 0;
static bool m_bWatchDogKicked = false;
FILE  *m_pDynConfFile = NULL;
static struct DynamicConfig dynamic_config;

/* Statistics Logging */
static long long tot_bufsize = 0;
int ebd_cnt=0, fbd_cnt=0;

#ifdef USE_ION
static const char* PMEM_DEVICE = "/dev/ion";
#elif MAX_RES_720P
static const char* PMEM_DEVICE = "/dev/pmem_adsp";
#elif MAX_RES_1080P_EBI
static const char* PMEM_DEVICE  = "/dev/pmem_adsp";
#elif MAX_RES_1080P
static const char* PMEM_DEVICE = "/dev/pmem_smipool";
#else
#error PMEM_DEVICE cannot be determined.
#endif

#ifdef USE_ION
struct enc_ion ion_data;
#endif
//////////////////////////
// MODULE FUNCTIONS
//////////////////////////

void* PmemMalloc(OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO* pMem, int nSize)
{
   void *pvirt = NULL;
   int rc = 0;

   if (!pMem)
      return NULL;

#ifdef USE_ION
  ion_data.ion_device_fd = open (PMEM_DEVICE,O_RDONLY);
  if(ion_data.ion_device_fd < 0)
  {
      E("\nERROR: ION Device open() Failed");
      return NULL;
  }
  nSize = (nSize + 4095) & (~4095);
  ion_data.alloc_data.len = nSize;
  ion_data.alloc_data.heap_id_mask = 0x1 << ION_CP_MM_HEAP_ID;
  ion_data.alloc_data.align = 4096;
  ion_data.alloc_data.flags = 0;

  rc = ioctl(ion_data.ion_device_fd,ION_IOC_ALLOC,&ion_data.alloc_data);
  if(rc || !ion_data.alloc_data.handle) {
         E("\n ION ALLOC memory failed ");
         ion_data.alloc_data.handle=NULL;
         return NULL;
  }

  ion_data.ion_alloc_fd.handle = ion_data.alloc_data.handle;
  rc = ioctl(ion_data.ion_device_fd,ION_IOC_MAP,&ion_data.ion_alloc_fd);
  if(rc) {
        E("\n ION MAP failed ");
        ion_data.ion_alloc_fd.fd =-1;
        ion_data.ion_alloc_fd.fd =-1;
        return NULL;
  }
  pMem->pmem_fd = ion_data.ion_alloc_fd.fd;
#else
   pMem->pmem_fd = open(PMEM_DEVICE, O_RDWR);
   if ((int)(pMem->pmem_fd) < 0)
      return NULL;
   nSize = (nSize + 4095) & (~4095);
#endif
   pMem->offset = 0;
   pvirt = mmap(NULL, nSize,
                PROT_READ | PROT_WRITE,
                MAP_SHARED, pMem->pmem_fd, pMem->offset);
   if (pvirt == (void*) MAP_FAILED)
   {
      close(pMem->pmem_fd);
      pMem->pmem_fd = -1;
#ifdef USE_ION
    if(ioctl(ion_data.ion_device_fd,ION_IOC_FREE,
       &ion_data.alloc_data.handle)) {
      E("ion recon buffer free failed");
    }
    ion_data.alloc_data.handle = NULL;
    ion_data.ion_alloc_fd.fd =-1;
    close(ion_data.ion_device_fd);
    ion_data.ion_device_fd =-1;
#endif
      return NULL;
   }
   D("allocated pMem->fd = %d pvirt=0x%x, pMem->phys=0x%x, size = %d", pMem->pmem_fd,
       pvirt, pMem->offset, nSize);
   return pvirt;
}

int PmemFree(OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO* pMem, void* pvirt, int nSize)
{
   if (!pMem || !pvirt)
      return -1;

   nSize = (nSize + 4095) & (~4095);
   munmap(pvirt, nSize);
   close(pMem->pmem_fd);
   pMem->pmem_fd = -1;
#ifdef USE_ION
   if(ioctl(ion_data.ion_device_fd,ION_IOC_FREE,
         &ion_data.alloc_data.handle)) {
        E("ion recon buffer free failed");
   }
   ion_data.alloc_data.handle = NULL;
   ion_data.ion_alloc_fd.fd =-1;
   close(ion_data.ion_device_fd);
   ion_data.ion_device_fd =-1;
#endif
   return 0;
}
void PrintFramePackArrangement(OMX_QCOM_FRAME_PACK_ARRANGEMENT framePackingArrangement)
{
    printf("id (%d)\n",
           framePackingArrangement.id);
    printf("cancel_flag (%d)\n",
           framePackingArrangement.cancel_flag);
    printf("type (%d)\n",
           framePackingArrangement.type);
    printf("quincunx_sampling_flag (%d)\n",
           framePackingArrangement.quincunx_sampling_flag);
   printf("content_interpretation_type (%d)\n",
          framePackingArrangement.content_interpretation_type);
   printf("spatial_flipping_flag (%d)\n",
          framePackingArrangement.spatial_flipping_flag);
   printf("frame0_flipped_flag (%d)\n",
          framePackingArrangement.frame0_flipped_flag);
   printf("field_views_flag (%d)\n",
          framePackingArrangement.field_views_flag);
   printf("current_frame_is_frame0_flag (%d)\n",
          framePackingArrangement.current_frame_is_frame0_flag);
   printf("frame0_self_contained_flag (%d)\n",
          framePackingArrangement.frame0_self_contained_flag);
   printf("frame1_self_contained_flag (%d)\n",
          framePackingArrangement.frame1_self_contained_flag);
   printf("frame0_grid_position_x (%d)\n",
          framePackingArrangement.frame0_grid_position_x);
   printf("frame0_grid_position_y (%d)\n",
          framePackingArrangement.frame0_grid_position_y);
   printf("frame1_grid_position_x (%d)\n",
          framePackingArrangement.frame1_grid_position_x);
   printf("frame1_grid_position_y (%d)\n",
          framePackingArrangement.frame1_grid_position_y);
   printf("reserved_byte (%d)\n",
          framePackingArrangement.reserved_byte);
   printf("repetition_period (%d)\n",
          framePackingArrangement.repetition_period);
   printf("extension_flag (%d)\n",
          framePackingArrangement.extension_flag);
}
void SetState(OMX_STATETYPE eState)
{
#define GOTO_STATE(eState)                      \
   case eState:                                 \
      {                                         \
         D("Going to state " # eState"...");            \
         OMX_SendCommand(m_hHandle,                     \
                         OMX_CommandStateSet,           \
                         (OMX_U32) eState,              \
                         NULL);                         \
         while (m_eState != eState)                     \
         {                                              \
            sleep(1);                               \
         }                                              \
         D("Now in state " # eState);                   \
         break;                                         \
      }

   switch (eState)
   {
      GOTO_STATE(OMX_StateLoaded);
      GOTO_STATE(OMX_StateIdle);
      GOTO_STATE(OMX_StateExecuting);
      GOTO_STATE(OMX_StateInvalid);
      GOTO_STATE(OMX_StateWaitForResources);
      GOTO_STATE(OMX_StatePause);
   }
}
////////////////////////////////////////////////////////////////////////////////
OMX_ERRORTYPE ConfigureEncoder()
{
   OMX_ERRORTYPE result = OMX_ErrorNone;
   unsigned const int *profile_tbl = (unsigned int const *)mpeg4_profile_level_table;
   OMX_U32 mb_per_sec, mb_per_frame;
   bool profile_level_found = false;
   OMX_U32 eProfile,eLevel;

   OMX_PARAM_PORTDEFINITIONTYPE portdef; // OMX_IndexParamPortDefinition
#ifdef QCOM_EXT
      OMX_QCOM_PARAM_PORTDEFINITIONTYPE qPortDefnType;
#endif
   portdef.nPortIndex = (OMX_U32) 0; // input
   result = OMX_GetParameter(m_hHandle,
                             OMX_IndexParamPortDefinition,
                             &portdef);
   E("\n OMX_IndexParamPortDefinition Get Paramter on input port");
   CHK(result);
   portdef.format.video.nFrameWidth = m_sProfile.nFrameWidth;
   portdef.format.video.nFrameHeight = m_sProfile.nFrameHeight;

   E ("\n Height %d width %d bit rate %d",portdef.format.video.nFrameHeight
      ,portdef.format.video.nFrameWidth,portdef.format.video.nBitrate);
   result = OMX_SetParameter(m_hHandle,
                             OMX_IndexParamPortDefinition,
                             &portdef);
   E("\n OMX_IndexParamPortDefinition Set Paramter on input port");
   CHK(result);
   // once more to get proper buffer size
   result = OMX_GetParameter(m_hHandle,
                             OMX_IndexParamPortDefinition,
                             &portdef);
   E("\n OMX_IndexParamPortDefinition Get Paramter on input port, 2nd pass");
   CHK(result);
   // update size accordingly
   m_sProfile.nFrameBytes = portdef.nBufferSize;
   portdef.nPortIndex = (OMX_U32) 1; // output
   result = OMX_GetParameter(m_hHandle,
                             OMX_IndexParamPortDefinition,
                             &portdef);
   E("\n OMX_IndexParamPortDefinition Get Paramter on output port");
   CHK(result);
   portdef.format.video.nFrameWidth = m_sProfile.nFrameWidth;
   portdef.format.video.nFrameHeight = m_sProfile.nFrameHeight;
   portdef.format.video.nBitrate = m_sProfile.nBitrate;
   FractionToQ16(portdef.format.video.xFramerate,(int) (m_sProfile.nFramerate * 2),2);
   result = OMX_SetParameter(m_hHandle,
                             OMX_IndexParamPortDefinition,
                             &portdef);
   E("\n OMX_IndexParamPortDefinition Set Paramter on output port");
   CHK(result);

#ifdef QCOM_EXT

qPortDefnType.nPortIndex = PORT_INDEX_IN;
qPortDefnType.nMemRegion = OMX_QCOM_MemRegionEBI1;
qPortDefnType.nSize = sizeof(OMX_QCOM_PARAM_PORTDEFINITIONTYPE);

result = OMX_SetParameter(m_hHandle,
                             (OMX_INDEXTYPE)OMX_QcomIndexPortDefn,
                             &qPortDefnType);

#endif
   if (!m_sProfile.nUserProfile) // profile not set by user, go ahead with table calculation
   {
   //validate the ht,width,fps,bitrate and set the appropriate profile and level
   if(m_sProfile.eCodec == OMX_VIDEO_CodingMPEG4)
   {
     profile_tbl = (unsigned int const *)mpeg4_profile_level_table;
   }
   else if(m_sProfile.eCodec == OMX_VIDEO_CodingAVC)
   {
     profile_tbl = (unsigned int const *)h264_profile_level_table;
   }
   else if(m_sProfile.eCodec == OMX_VIDEO_CodingH263)
   {
     profile_tbl = (unsigned int const *)h263_profile_level_table;
   }

   mb_per_frame = ((m_sProfile.nFrameHeight+15)>>4)*
                ((m_sProfile.nFrameWidth+15)>>4);

   mb_per_sec = mb_per_frame*(m_sProfile.nFramerate);

   do{
      if(mb_per_frame <= (int)profile_tbl[0])
      {
          if(mb_per_sec <= (int)profile_tbl[1])
          {
            if(m_sProfile.nBitrate <= (int)profile_tbl[2])
            {
              eLevel = (int)profile_tbl[3];
              eProfile = (int)profile_tbl[4];
              E("\n profile/level found: %d/%d\n",eProfile/eLevel);
              profile_level_found = true;
              break;
            }
          }
      }
      profile_tbl = profile_tbl + 5;
   }while(profile_tbl[0] != 0);

   if ( profile_level_found != true )
   {
     E("\n Error: Unsupported profile/level\n");
     return OMX_ErrorNone;
   }
   }
   else // Profile set by user!
   {
      eProfile = m_sProfile.nUserProfile;
      eLevel = 0;
   }
   if (m_sProfile.eCodec == OMX_VIDEO_CodingH263)
   {
      D("Configuring H263...");

      OMX_VIDEO_PARAM_H263TYPE h263;
      result = OMX_GetParameter(m_hHandle,
                                OMX_IndexParamVideoH263,
                                &h263);
      CHK(result);
      h263.nPortIndex = (OMX_U32) PORT_INDEX_OUT;
      h263.nPFrames = m_sProfile.nFramerate * 2 - 1; // intra period
      h263.nBFrames = 0;
      h263.eProfile = (OMX_VIDEO_H263PROFILETYPE)eProfile;
      h263.eLevel = (OMX_VIDEO_H263LEVELTYPE)eLevel;
      h263.bPLUSPTYPEAllowed = OMX_FALSE;
      h263.nAllowedPictureTypes = 2;
      h263.bForceRoundingTypeToZero = OMX_TRUE;
      h263.nPictureHeaderRepetition = 0;
      h263.nGOBHeaderInterval = 1;
      result = OMX_SetParameter(m_hHandle,
                                OMX_IndexParamVideoH263,
                                &h263);
   }
   else
   {
      D("Configuring MP4/H264...");

      OMX_VIDEO_PARAM_PROFILELEVELTYPE profileLevel; // OMX_IndexParamVideoProfileLevelCurrent
      profileLevel.nPortIndex = (OMX_U32) PORT_INDEX_OUT;
      profileLevel.eProfile = eProfile;
      profileLevel.eLevel =  eLevel;
      result = OMX_SetParameter(m_hHandle,
                                OMX_IndexParamVideoProfileLevelCurrent,
                                &profileLevel);
      E("\n OMX_IndexParamVideoProfileLevelCurrent Set Paramter port");
      CHK(result);
      //profileLevel.eLevel = (OMX_U32) m_sProfile.eLevel;
      result = OMX_GetParameter(m_hHandle,
                                OMX_IndexParamVideoProfileLevelCurrent,
                                &profileLevel);
      E("\n OMX_IndexParamVideoProfileLevelCurrent Get Paramter port");
      D ("\n Profile = %d level = %d",profileLevel.eProfile,profileLevel.eLevel);
      CHK(result);

        if (m_sProfile.eCodec == OMX_VIDEO_CodingMPEG4)
        {
        OMX_VIDEO_PARAM_MPEG4TYPE mp4; // OMX_IndexParamVideoMpeg4
       result = OMX_GetParameter(m_hHandle,
                                 OMX_IndexParamVideoMpeg4,
                                 &mp4);
       CHK(result);
       mp4.nPortIndex = (OMX_U32) PORT_INDEX_OUT;
       mp4.nTimeIncRes = 1000;
       result = OMX_SetParameter(m_hHandle,
                                 OMX_IndexParamVideoMpeg4,
                                 &mp4);
       CHK(result);
         }
   }
   if (m_sProfile.eCodec == OMX_VIDEO_CodingAVC)
   {
#if 1
/////////////C A B A C ///A N D/////D E B L O C K I N G /////////////////

      OMX_VIDEO_PARAM_AVCTYPE avcdata;
      avcdata.nPortIndex = (OMX_U32)PORT_INDEX_OUT;
      result = OMX_GetParameter(m_hHandle,
                                OMX_IndexParamVideoAvc,
                                &avcdata);
      CHK(result);
// TEST VALUES (CHANGE FOR DIFF CONFIG's)
    avcdata.eLoopFilterMode = OMX_VIDEO_AVCLoopFilterEnable;
//      avcdata.eLoopFilterMode = OMX_VIDEO_AVCLoopFilterDisable;
//    avcdata.eLoopFilterMode = OMX_VIDEO_AVCLoopFilterDisableSliceBoundary;
   avcdata.bEntropyCodingCABAC = OMX_FALSE;
//   avcdata.bEntropyCodingCABAC = OMX_TRUE;
   avcdata.nCabacInitIdc = 1;
///////////////////////////////////////////////

      result = OMX_SetParameter(m_hHandle,
                                OMX_IndexParamVideoAvc,
                                &avcdata);
      CHK(result);

/////////////C A B A C ///A N D/////D E B L O C K I N G /////////////////
#endif
   }

   OMX_VIDEO_PARAM_BITRATETYPE bitrate; // OMX_IndexParamVideoBitrate
   bitrate.nPortIndex = (OMX_U32)PORT_INDEX_OUT;
   result = OMX_GetParameter(m_hHandle,
                             OMX_IndexParamVideoBitrate,
                             &bitrate);
   E("\n OMX_IndexParamVideoBitrate Get Paramter port");
   CHK(result);
   bitrate.eControlRate = m_sProfile.eControlRate;
   bitrate.nTargetBitrate = m_sProfile.nBitrate;
   result = OMX_SetParameter(m_hHandle,
                             OMX_IndexParamVideoBitrate,
                             &bitrate);
   E("\n OMX_IndexParamVideoBitrate Set Paramter port");
   CHK(result);

   OMX_VIDEO_PARAM_PORTFORMATTYPE framerate; // OMX_IndexParamVidePortFormat
   framerate.nPortIndex = 0;
   result = OMX_GetParameter(m_hHandle,
                             OMX_IndexParamVideoPortFormat,
                             &framerate);
   E("\n OMX_IndexParamVideoPortFormat Get Paramter port");
   CHK(result);
   FractionToQ16(framerate.xFramerate,(int) (m_sProfile.nFramerate * 2),2);
   result = OMX_SetParameter(m_hHandle,
                             OMX_IndexParamVideoPortFormat,
                             &framerate);
   E("\n OMX_IndexParamVideoPortFormat Set Paramter port");
   CHK(result);

#if 1
///////////////////I N T R A P E R I O D ///////////////////

      QOMX_VIDEO_INTRAPERIODTYPE intra;

      intra.nPortIndex = (OMX_U32) PORT_INDEX_OUT; // output
      result = OMX_GetConfig(m_hHandle,
                             (OMX_INDEXTYPE) QOMX_IndexConfigVideoIntraperiod,
                             (OMX_PTR) &intra);

      if (result == OMX_ErrorNone)
      {
         intra.nPFrames = (OMX_U32) (2 * m_sProfile.nFramerate - 1); //setting I
                                                                     //frame interval to
                                                                     //2 x framerate
         intra.nIDRPeriod = 1; //every I frame is an IDR
         intra.nPortIndex = (OMX_U32) PORT_INDEX_OUT;
         result = OMX_SetConfig(m_hHandle,
                                (OMX_INDEXTYPE) QOMX_IndexConfigVideoIntraperiod,
                                (OMX_PTR) &intra);
      }
      else
      {
         E("failed to get state", 0, 0, 0);
      }


///////////////////I N T R A P E R I O D ///////////////////
#endif

#if 1
///////////////////E R R O R C O R R E C T I O N ///////////////////

      ResyncMarkerType eResyncMarkerType = RESYNC_MARKER_NONE;
      unsigned long int nResyncMarkerSpacing = 0;
      OMX_BOOL enableHEC = OMX_FALSE;

//For Testing ONLY
   if (m_sProfile.eCodec == OMX_VIDEO_CodingMPEG4)
   {
// MPEG4
//      eResyncMarkerType = RESYNC_MARKER_BYTE;
//      nResyncMarkerSpacing = 1920;
      eResyncMarkerType = RESYNC_MARKER_MB;
      nResyncMarkerSpacing = 50;
      enableHEC = OMX_TRUE;
   }
   else if (m_sProfile.eCodec == OMX_VIDEO_CodingH263)
   {
//H263
      eResyncMarkerType = RESYNC_MARKER_GOB;
      nResyncMarkerSpacing = 0;
   }
   else if (m_sProfile.eCodec == OMX_VIDEO_CodingAVC)
   {
//H264
//      eResyncMarkerType = RESYNC_MARKER_BYTE;
//      nResyncMarkerSpacing = 1920;

      //nResyncMarkerSpacing sets the slice size in venc_set_multislice_cfg
      //
      //As of 9/24/10, it is known that the firmware has a bitstream
      //corruption issue when RateControl and multislice are enabled for 720P
      //So, disabling multislice for 720P when ratecontrol is enabled until
      //the firmware issue is resolved.

      if ( ( (m_sProfile.nFrameWidth == 1280) && (m_sProfile.nFrameHeight = 720) ) &&
           (m_sProfile.eControlRate  != OMX_Video_ControlRateDisable) )
      {
         eResyncMarkerType = RESYNC_MARKER_NONE;
         nResyncMarkerSpacing = 0;
      }
      else
      {
         eResyncMarkerType = RESYNC_MARKER_MB;
          nResyncMarkerSpacing = 50;
      }
   }

   OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE errorCorrection; //OMX_IndexParamVideoErrorCorrection
   errorCorrection.nPortIndex = (OMX_U32) PORT_INDEX_OUT; // output
   result = OMX_GetParameter(m_hHandle,
                             (OMX_INDEXTYPE) OMX_IndexParamVideoErrorCorrection,
                             (OMX_PTR) &errorCorrection);

   errorCorrection.bEnableRVLC = OMX_FALSE;
   errorCorrection.bEnableDataPartitioning = OMX_FALSE;

      if ((eResyncMarkerType == RESYNC_MARKER_BYTE) &&
         (m_sProfile.eCodec == OMX_VIDEO_CodingMPEG4)){
            errorCorrection.bEnableResync = OMX_TRUE;
            errorCorrection.nResynchMarkerSpacing = nResyncMarkerSpacing;
            errorCorrection.bEnableHEC = enableHEC;
            }
      else if ((eResyncMarkerType == RESYNC_MARKER_BYTE) &&
               (m_sProfile.eCodec == OMX_VIDEO_CodingAVC)){
         errorCorrection.bEnableResync = OMX_TRUE;
         errorCorrection.nResynchMarkerSpacing = nResyncMarkerSpacing;
         }
      else if ((eResyncMarkerType == RESYNC_MARKER_GOB) &&
               (m_sProfile.eCodec == OMX_VIDEO_CodingH263)){
         errorCorrection.bEnableResync = OMX_FALSE;
         errorCorrection.nResynchMarkerSpacing = nResyncMarkerSpacing;
         errorCorrection.bEnableDataPartitioning = OMX_TRUE;
         }

      result = OMX_SetParameter(m_hHandle,
                            (OMX_INDEXTYPE) OMX_IndexParamVideoErrorCorrection,
                            (OMX_PTR) &errorCorrection);
   CHK(result);

      if (eResyncMarkerType == RESYNC_MARKER_MB){
         if (m_sProfile.eCodec == OMX_VIDEO_CodingAVC){
            OMX_VIDEO_PARAM_AVCTYPE avcdata;
            avcdata.nPortIndex = (OMX_U32) PORT_INDEX_OUT; // output
            result = OMX_GetParameter(m_hHandle,
                                      OMX_IndexParamVideoAvc,
                                      (OMX_PTR) &avcdata);
            CHK(result);
            if (result == OMX_ErrorNone)
            {
               avcdata.nSliceHeaderSpacing = nResyncMarkerSpacing;
               result = OMX_SetParameter(m_hHandle,
                                         OMX_IndexParamVideoAvc,
                                         (OMX_PTR) &avcdata);
               CHK(result);

            }
         }
         else if(m_sProfile.eCodec == OMX_VIDEO_CodingMPEG4){
            OMX_VIDEO_PARAM_MPEG4TYPE mp4;
            mp4.nPortIndex = (OMX_U32) PORT_INDEX_OUT; // output
            result = OMX_GetParameter(m_hHandle,
                                      OMX_IndexParamVideoMpeg4,
                                      (OMX_PTR) &mp4);
            CHK(result);

            if (result == OMX_ErrorNone)
            {
               mp4.nSliceHeaderSpacing = nResyncMarkerSpacing;
               result = OMX_SetParameter(m_hHandle,
                                         OMX_IndexParamVideoMpeg4,
                                         (OMX_PTR) &mp4);
               CHK(result);
            }
         }
         }

///////////////////E R R O R C O R R E C T I O N ///////////////////
#endif

#if 1
///////////////////I N T R A R E F R E S H///////////////////
      bool bEnableIntraRefresh = OMX_TRUE;

      if (result == OMX_ErrorNone)
      {
         OMX_VIDEO_PARAM_INTRAREFRESHTYPE ir; // OMX_IndexParamVideoIntraRefresh
         ir.nPortIndex = (OMX_U32) PORT_INDEX_OUT; // output
         result = OMX_GetParameter(m_hHandle,
                                   OMX_IndexParamVideoIntraRefresh,
                                   (OMX_PTR) &ir);
         if (result == OMX_ErrorNone)
         {
            if (bEnableIntraRefresh)
            {
               ir.eRefreshMode = OMX_VIDEO_IntraRefreshCyclic;
               ir.nCirMBs = 5;
               result = OMX_SetParameter(m_hHandle,
                                         OMX_IndexParamVideoIntraRefresh,
                                         (OMX_PTR) &ir);
               CHK(result);
            }
         }
      }
#endif
#if 1
///////////////////FRAMEPACKING DATA///////////////////
      OMX_QCOM_FRAME_PACK_ARRANGEMENT framePackingArrangement;
      FILE *m_pConfigFile;
      char m_configFilename [128] = "/data/configFile.cfg";
      memset(&framePackingArrangement, 0, sizeof(framePackingArrangement));
      m_pConfigFile = fopen(m_configFilename, "r");
      if (m_pConfigFile != NULL)
      {
         //read all frame packing data
         framePackingArrangement.nPortIndex = (OMX_U32)PORT_INDEX_OUT;
         int totalSizeToRead = FRAME_PACK_SIZE * sizeof(OMX_U32);
         char *pFramePack = (char *) &(framePackingArrangement.id);
         while ( ( (fscanf(m_pConfigFile, "%d", pFramePack)) != EOF ) &&
                 (totalSizeToRead != 0) )
         {
            //printf("Addr = %p, Value read = %d, sizeToRead remaining=%d\n",
            //       pFramePack, *pFramePack, totalSizeToRead);
            pFramePack += sizeof(OMX_U32);
            totalSizeToRead -= sizeof(OMX_U32);
         }
         //close the file.
         fclose(m_pConfigFile);

         printf("Frame Packing data from config file:\n");
         PrintFramePackArrangement(framePackingArrangement);
      }
      else
      {
         D("\n Config file does not exist or could not be opened.");
         //set the default values
         framePackingArrangement.nPortIndex = (OMX_U32)PORT_INDEX_OUT;
         framePackingArrangement.id = 123;
         framePackingArrangement.cancel_flag = false;
         framePackingArrangement.type = 3;
         framePackingArrangement.quincunx_sampling_flag = false;
         framePackingArrangement.content_interpretation_type = 0;
         framePackingArrangement.spatial_flipping_flag = true;
         framePackingArrangement.frame0_flipped_flag = false;
         framePackingArrangement.field_views_flag = false;
         framePackingArrangement.current_frame_is_frame0_flag = false;
         framePackingArrangement.frame0_self_contained_flag = true;
         framePackingArrangement.frame1_self_contained_flag = false;
         framePackingArrangement.frame0_grid_position_x = 3;
         framePackingArrangement.frame0_grid_position_y = 15;
         framePackingArrangement.frame1_grid_position_x = 11;
         framePackingArrangement.frame1_grid_position_y = 7;
         framePackingArrangement.reserved_byte = 0;
         framePackingArrangement.repetition_period = 16381;
         framePackingArrangement.extension_flag = false;

         printf("Frame Packing Defaults :\n");
         PrintFramePackArrangement(framePackingArrangement);
      }
      result = OMX_SetConfig(m_hHandle,
                (OMX_INDEXTYPE)OMX_QcomIndexConfigVideoFramePackingArrangement,
                (OMX_PTR) &framePackingArrangement);
      CHK(result);

//////////////////////OMX_VIDEO_PARAM_INTRAREFRESHTYPE///////////////////
#endif

   OMX_CONFIG_FRAMERATETYPE enc_framerate; // OMX_IndexConfigVideoFramerate
   enc_framerate.nPortIndex = (OMX_U32)PORT_INDEX_OUT;
   result = OMX_GetConfig(m_hHandle,
                          OMX_IndexConfigVideoFramerate,
                          &enc_framerate);
   CHK(result);
   FractionToQ16(enc_framerate.xEncodeFramerate,(int) (m_sProfile.nFramerate * 2),2);
   result = OMX_SetConfig(m_hHandle,
                          OMX_IndexConfigVideoFramerate,
                          &enc_framerate);
   CHK(result);
   return OMX_ErrorNone;
}
////////////////////////////////////////////////////////////////////////////////
void SendMessage(MsgId id, MsgData* data)
{
   pthread_mutex_lock(&m_mutex);
   if (m_sMsgQ.size >= MAX_MSG)
   {
      E("main msg m_sMsgQ is full");
      return;
   }
   m_sMsgQ.q[(m_sMsgQ.head + m_sMsgQ.size) % MAX_MSG].id = id;
   if (data)
      m_sMsgQ.q[(m_sMsgQ.head + m_sMsgQ.size) % MAX_MSG].data = *data;
   ++m_sMsgQ.size;
   pthread_cond_signal(&m_signal);
   pthread_mutex_unlock(&m_mutex);
}
////////////////////////////////////////////////////////////////////////////////
void PopMessage(Msg* msg)
{
   pthread_mutex_lock(&m_mutex);
   while (m_sMsgQ.size == 0)
   {
      pthread_cond_wait(&m_signal, &m_mutex);
   }
   *msg = m_sMsgQ.q[m_sMsgQ.head];
   --m_sMsgQ.size;
   m_sMsgQ.head = (m_sMsgQ.head + 1) % MAX_MSG;
   pthread_mutex_unlock(&m_mutex);
}
////////////////////////////////////////////////////////////////////////////////
OMX_ERRORTYPE EVT_CB(OMX_IN OMX_HANDLETYPE hComponent,
                     OMX_IN OMX_PTR pAppData,
                     OMX_IN OMX_EVENTTYPE eEvent,
                     OMX_IN OMX_U32 nData1,
                     OMX_IN OMX_U32 nData2,
                     OMX_IN OMX_PTR pEventData)
{
#define SET_STATE(eState)                                   \
   case eState:                                             \
      {                                                     \
         D("" # eState " complete");                        \
         m_eState = eState;                                 \
         break;                                             \
      }

   if (eEvent == OMX_EventCmdComplete)
   {
      if ((OMX_COMMANDTYPE) nData1 == OMX_CommandStateSet)
      {
         switch ((OMX_STATETYPE) nData2)
         {
            SET_STATE(OMX_StateLoaded);
            SET_STATE(OMX_StateIdle);
            SET_STATE(OMX_StateExecuting);
            SET_STATE(OMX_StateInvalid);
            SET_STATE(OMX_StateWaitForResources);
            SET_STATE(OMX_StatePause);
         default:
            E("invalid state %d", (int) nData2);
          }
      }
   }

   else if (eEvent == OMX_EventError)
   {
      E("OMX_EventError");
   }

   else
   {
      E("unexpected event %d", (int) eEvent);
   }
   return OMX_ErrorNone;
}
////////////////////////////////////////////////////////////////////////////////
OMX_ERRORTYPE EBD_CB(OMX_IN OMX_HANDLETYPE hComponent,
                     OMX_IN OMX_PTR pAppData,
                     OMX_IN OMX_BUFFERHEADERTYPE* pBuffer)
{
   D("Got EBD callback ts=%lld", pBuffer->nTimeStamp);

   for (int i = 0; i < num_in_buffers; i++)
   {
      // mark this buffer ready for use again
      if (m_pInBuffers[i] == pBuffer)
      {

         D("Marked input buffer idx %d as free, buf %p", i, pBuffer->pBuffer);
         m_bInFrameFree[i] = OMX_TRUE;
         break;
      }
   }

   if (m_eMode == MODE_LIVE_ENCODE)
   {
      CameraTest_ReleaseFrame(pBuffer->pBuffer,
                              ((OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO*)pBuffer->pAppPrivate));
   }
   else
   {
      // wake up main thread and tell it to send next frame
      MsgData data;
      data.sBitstreamData.pBuffer = pBuffer;
      SendMessage(MSG_ID_INPUT_FRAME_DONE,
                  &data);

   }
   return OMX_ErrorNone;
}
////////////////////////////////////////////////////////////////////////////////
OMX_ERRORTYPE FBD_CB(OMX_OUT OMX_HANDLETYPE hComponent,
                     OMX_OUT OMX_PTR pAppData,
                     OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer)
{
   D("Got FBD callback ts=%lld", pBuffer->nTimeStamp);

   static long long prevTime = 0;
   long long currTime = GetTimeStamp();

   m_bWatchDogKicked = true;

   /* Empty Buffers should not be counted */
   if(pBuffer->nFilledLen !=0)
   {
      /* Counting Buffers supplied from OpneMax Encoder */
      fbd_cnt++;
      tot_bufsize += pBuffer->nFilledLen;
   }
   if (prevTime != 0)
   {
      long long currTime = GetTimeStamp();
      D("FBD_DELTA = %lld\n", currTime - prevTime);
   }
   prevTime = currTime;

   if (m_eMode == MODE_PROFILE)
   {
      // if we are profiling we are not doing file I/O
      // so just give back to encoder
      if (OMX_FillThisBuffer(m_hHandle, pBuffer) != OMX_ErrorNone)
      {
         E("empty buffer failed for profiling");
      }
   }
   else
   {
      // wake up main thread and tell it to write to file
      MsgData data;
      data.sBitstreamData.pBuffer = pBuffer;
      SendMessage(MSG_ID_OUTPUT_FRAME_DONE,
                  &data);
   }
   return OMX_ErrorNone;
}
////////////////////////////////////////////////////////////////////////////////
OMX_ERRORTYPE VencTest_Initialize()
{
   OMX_ERRORTYPE result = OMX_ErrorNone;
   static OMX_CALLBACKTYPE sCallbacks = {EVT_CB, EBD_CB, FBD_CB};
   int i;

   for (i = 0; i < num_in_buffers; i++)
   {
      m_pInBuffers[i] = NULL;
   }

   result = OMX_Init();
   CHK(result);

   if (m_sProfile.eCodec == OMX_VIDEO_CodingMPEG4)
   {
        result = OMX_GetHandle(&m_hHandle,
                             "OMX.qcom.video.encoder.mpeg4",
                             NULL,
                             &sCallbacks);
     // CHK(result);
   }
   else if (m_sProfile.eCodec == OMX_VIDEO_CodingH263)
   {
      result = OMX_GetHandle(&m_hHandle,
                             "OMX.qcom.video.encoder.h263",
                             NULL,
                             &sCallbacks);
      CHK(result);
   }
   else
   {
      result = OMX_GetHandle(&m_hHandle,
                             "OMX.qcom.video.encoder.avc",
                             NULL,
                             &sCallbacks);
      CHK(result);
   }


   result = ConfigureEncoder();
   CHK(result);

   return result;
}

////////////////////////////////////////////////////////////////////////////////
OMX_ERRORTYPE VencTest_RegisterYUVBuffer(OMX_BUFFERHEADERTYPE** ppBufferHeader,
                                         OMX_U8 *pBuffer,
                                         OMX_PTR pAppPrivate)
{
   OMX_ERRORTYPE result = OMX_ErrorNone;
#if 0
   D("register buffer");
   if ((result = OMX_AllocateBuffer(m_hHandle,
                               ppBufferHeader,
                               (OMX_U32) PORT_INDEX_IN,
                               pAppPrivate,
                               m_sProfile.nFrameBytes
                               )) != OMX_ErrorNone)
   {
      E("use buffer failed");
   }
   else
   {
     E("Allocate Buffer Success %x", (*ppBufferHeader)->pBuffer);
   }
  #endif
   D("register buffer");
   D("Calling UseBuffer for Input port");
   if ((result = OMX_UseBuffer(m_hHandle,
                               ppBufferHeader,
                               (OMX_U32) PORT_INDEX_IN,
                               pAppPrivate,
                               m_sProfile.nFrameBytes,
                               pBuffer)) != OMX_ErrorNone)
   {
      E("use buffer failed");
   }

   return result;
}
////////////////////////////////////////////////////////////////////////////////
OMX_ERRORTYPE VencTest_EncodeFrame(void* pYUVBuff,
                                   long long nTimeStamp)
{
   OMX_ERRORTYPE result = OMX_ErrorUndefined;
   D("calling OMX empty this buffer");
   for (int i = 0; i < num_in_buffers; i++)
   {
      if (pYUVBuff == m_pInBuffers[i]->pBuffer)
      {
         m_pInBuffers[i]->nTimeStamp = nTimeStamp;
    D("Sending Buffer - %x", m_pInBuffers[i]->pBuffer);
         result = OMX_EmptyThisBuffer(m_hHandle,
                                      m_pInBuffers[i]);
         /* Counting Buffers supplied to OpenMax Encoder */
         if(OMX_ErrorNone == result)
            ebd_cnt++;
         CHK(result);
         break;
      }
   }
   return result;
}
////////////////////////////////////////////////////////////////////////////////
OMX_ERRORTYPE VencTest_Exit(void)
{
   int i;
   OMX_ERRORTYPE result = OMX_ErrorNone;
   D("trying to exit venc");

   D("going to idle state");
   SetState(OMX_StateIdle);


   D("going to loaded state");
   //SetState(OMX_StateLoaded);
      OMX_SendCommand(m_hHandle,
                      OMX_CommandStateSet,
                      (OMX_U32) OMX_StateLoaded,
                       NULL);

      for (i = 0; i < num_in_buffers; i++)
   {
      D("free buffer");
      if (m_pInBuffers[i]->pBuffer)
      {
        // free(m_pInBuffers[i]->pBuffer);
         result = OMX_FreeBuffer(m_hHandle,
                                 PORT_INDEX_IN,
                                 m_pInBuffers[i]);
         CHK(result);
      }
      else
      {
         E("buffer %d is null", i);
         result = OMX_ErrorUndefined;
         CHK(result);
      }
   }
   for (i = 0; i < num_out_buffers; i++)
   {
      D("free buffer");
      if (m_pOutBuffers[i]->pBuffer)
      {
         free(m_pOutBuffers[i]->pBuffer);
         result = OMX_FreeBuffer(m_hHandle,
                                 PORT_INDEX_OUT,
                                 m_pOutBuffers[i]);
         CHK(result);

      }
      else
      {
         E("buffer %d is null", i);
         result = OMX_ErrorUndefined;
         CHK(result);
      }
   }

     while (m_eState != OMX_StateLoaded)
     {
        sleep(1);
     }
   D("component_deinit...");
   result = OMX_Deinit();
   CHK(result);

   D("venc is exiting...");
   return result;
}
////////////////////////////////////////////////////////////////////////////////

void VencTest_ReadDynamicConfigMsg()
{
  char frame_n[8], config[16], param[8];
  char *dest = frame_n;
  bool end = false;
  int cntr, nparam = 0;
  memset(&dynamic_config, 0, sizeof(struct DynamicConfig));
  do
  {
    cntr = -1;
    do
    {
      dest[++cntr] = fgetc(m_pDynConfFile);
    } while(dest[cntr] != ' ' && dest[cntr] != '\t' && dest[cntr] != '\n' && dest[cntr] != '\r' && !feof(m_pDynConfFile));
    if (dest[cntr] == '\n' || dest[cntr] == '\r')
      end = true;
    dest[cntr] = NULL;
    if (dest == frame_n)
      dest = config;
    else if (dest == config)
      dest = param;
    else
      end = true;
    nparam++;
  } while (!end && !feof(m_pDynConfFile));

  if (nparam > 1)
  {
    dynamic_config.pending = true;
    dynamic_config.frame_num = atoi(frame_n);
    if (!strcmp(config, "bitrate"))
    {
      dynamic_config.config_param = OMX_IndexConfigVideoBitrate;
      dynamic_config.config_data.bitrate.nPortIndex = PORT_INDEX_OUT;
      dynamic_config.config_data.bitrate.nEncodeBitrate = strtoul(param, NULL, 10);
    }
    else if (!strcmp(config, "framerate"))
    {
      dynamic_config.config_param = OMX_IndexConfigVideoFramerate;
      dynamic_config.config_data.framerate.nPortIndex = PORT_INDEX_OUT;
      dynamic_config.config_data.f_framerate = atof(param);
    }
    else if (!strcmp(config, "iperiod"))
    {
      dynamic_config.config_param = (OMX_INDEXTYPE)QOMX_IndexConfigVideoIntraperiod;
      dynamic_config.config_data.intraperiod.nPortIndex = PORT_INDEX_OUT;
      dynamic_config.config_data.intraperiod.nPFrames = strtoul(param, NULL, 10) - 1;
      dynamic_config.config_data.intraperiod.nIDRPeriod = 1; // This value is ignored in OMX component
    }
    else if (!strcmp(config, "ivoprefresh"))
    {
      dynamic_config.config_param = OMX_IndexConfigVideoIntraVOPRefresh;
      dynamic_config.config_data.intravoprefresh.nPortIndex = PORT_INDEX_OUT;
      dynamic_config.config_data.intravoprefresh.IntraRefreshVOP = OMX_TRUE;
    }
    else if (!strcmp(config, "rotation"))
    {
      dynamic_config.config_param = OMX_IndexConfigCommonRotate;
      dynamic_config.config_data.rotation.nPortIndex = PORT_INDEX_OUT;
      dynamic_config.config_data.rotation.nRotation = strtoul(param, NULL, 10);
    }
    else
    {
      E("UNKNOWN CONFIG PARAMETER: %s!", config);
      dynamic_config.pending = false;
    }
  }
  else if (feof(m_pDynConfFile))
  {
    fclose(m_pDynConfFile);
    m_pDynConfFile = NULL;
  }
}

void VencTest_ProcessDynamicConfigurationFile()
{
  do
  {
    if (dynamic_config.pending)
    {
      if(m_nFrameIn == dynamic_config.frame_num)
      {
        if (dynamic_config.config_param == OMX_IndexConfigVideoFramerate)
        {
          m_sProfile.nFramerate = dynamic_config.config_data.f_framerate;
          FractionToQ16(dynamic_config.config_data.framerate.xEncodeFramerate,
                        (int)(m_sProfile.nFramerate * 2), 2);
        }
        if (OMX_SetConfig(m_hHandle, dynamic_config.config_param,
            &dynamic_config.config_data) != OMX_ErrorNone)
          E("ERROR: Setting dynamic config to OMX param[0x%x]", dynamic_config.config_param);
        dynamic_config.pending = false;
      }
      else if (m_nFrameIn > dynamic_config.frame_num)
      {
        E("WARNING: Config change requested in passed frame(%d)", dynamic_config.frame_num);
        dynamic_config.pending = false;
      }
    }
    if (!dynamic_config.pending)
      VencTest_ReadDynamicConfigMsg();
  } while (!dynamic_config.pending && m_pDynConfFile);
}

////////////////////////////////////////////////////////////////////////////////
OMX_ERRORTYPE VencTest_ReadAndEmpty(OMX_BUFFERHEADERTYPE* pYUVBuffer)
{
   OMX_ERRORTYPE result = OMX_ErrorNone;
#ifdef T_ARM
#ifdef MAX_RES_720P
   if (read(m_nInFd,
            pYUVBuffer->pBuffer,
            m_sProfile.nFrameBytes) != m_sProfile.nFrameBytes)
   {
      return OMX_ErrorUndefined;
   }
#elif BADGER
   int bytes;
   E("will read YUV now: %d bytes to buffer %p\n", m_sProfile.nFrameRead, pYUVBuffer->pBuffer);
   E("W: %d H: %d Str: %d scal: %d \n", m_sProfile.nFrameWidth, m_sProfile.nFrameHeight,
		m_sProfile.nFramestride, m_sProfile.nFrameScanlines);
   bytes = read(m_nInFd, pYUVBuffer->pBuffer, m_sProfile.nFrameRead);
   if (bytes != m_sProfile.nFrameRead) {
		E("read failed: %d != %d\n", read, m_sProfile.nFrameRead);
		return OMX_ErrorUndefined;
   }
   E("\n\nRead %d bytes\n\n\n", m_sProfile.nFrameRead);
#else
         OMX_U32 bytestoread = m_sProfile.nFrameWidth*m_sProfile.nFrameHeight;
         // read Y first
         if (read(m_nInFd,
              pYUVBuffer->pBuffer,
              bytestoread) != bytestoread)
            return OMX_ErrorUndefined;

         // check alignment for offset to C
         OMX_U32 offset_to_c = m_sProfile.nFrameWidth * m_sProfile.nFrameHeight;

         const OMX_U32 C_2K = (1024*2),
            MASK_2K = C_2K-1,
            IMASK_2K = ~MASK_2K;

         if (offset_to_c & MASK_2K)
         {
            // offset to C is not 2k aligned, adjustment is required
            offset_to_c = (offset_to_c & IMASK_2K) + C_2K;
         }

         bytestoread = m_sProfile.nFrameWidth*m_sProfile.nFrameHeight/2;
         // read C
         if (read(m_nInFd,
              pYUVBuffer->pBuffer + offset_to_c,
              bytestoread)!= bytestoread)
            return OMX_ErrorUndefined;
#endif
#else
   {
	  char * pInputbuf = (char *)(pYUVBuffer->pBuffer) ;
	      read(m_nInFd,pInputbuf,m_sProfile.nFrameBytes) ;

   }
#endif
   if (m_pDynConfFile)
     VencTest_ProcessDynamicConfigurationFile();
   D("about to call VencTest_EncodeFrame...");
   pthread_mutex_lock(&m_mutex);
   ++m_nFrameIn;
#ifdef BADGER
   pYUVBuffer->nFilledLen = m_sProfile.nFrameRead;
#else
   pYUVBuffer->nFilledLen = m_sProfile.nFrameBytes;
#endif
   D("Called Buffer with Data filled length %d",pYUVBuffer->nFilledLen);

      result = VencTest_EncodeFrame(pYUVBuffer->pBuffer,
                                 m_nTimeStamp);

   m_nTimeStamp += (1000000) / m_sProfile.nFramerate;
   CHK(result);
   pthread_mutex_unlock(&m_mutex);
   return result;
}
////////////////////////////////////////////////////////////////////////////////
void PreviewCallback(int nFD,
                     int nOffset,
                     void* pPhys,
                     void* pVirt,
                     long long nTimeStamp)
{

   D("================= preview frame %d, phys=0x%x, nTimeStamp(millis)=%lld",
     m_nFrameIn+1, pPhys, (nTimeStamp / 1000));

   if (m_nFrameIn == m_nFramePlay &&
       m_nFramePlay != 0)
   {
      // we will stop camera after last frame is encoded.
      // for now just ignore input frames

      CameraTest_ReleaseFrame(pPhys, pVirt);
      return;
   }

   // see if we should stop
   pthread_mutex_lock(&m_mutex);
   ++m_nFrameIn;
   pthread_mutex_unlock(&m_mutex);


   if (m_eMode == MODE_LIVE_ENCODE)
   {

      OMX_ERRORTYPE result;

      // register new camera buffers with encoder
      int i;
      for (i = 0; i < num_in_buffers; i++)
      {
         if (m_pInBuffers[i] != NULL &&
             m_pInBuffers[i]->pBuffer == pPhys)
         {
            break;
         }
         else if (m_pInBuffers[i] == NULL)
         {
            D("registering buffer...");
            result = VencTest_RegisterYUVBuffer(&m_pInBuffers[i],
                                                (OMX_U8*) pPhys,
                                                (OMX_PTR) pVirt); // store virt in app private field
            D("register done");
            CHK(result);
            break;
         }
      }

      if (i == num_in_buffers)
      {
         E("There are more camera buffers than we thought");
         CHK(1);
      }

      // encode the yuv frame

      D("StartEncodeTime=%lld", GetTimeStamp());
      result = VencTest_EncodeFrame(pPhys,
                                    nTimeStamp);
      CHK(result);
     // FBTest_DisplayImage(nFD, nOffset);
   }
   else
   {
     // FBTest_DisplayImage(nFD, nOffset);
      CameraTest_ReleaseFrame(pPhys, pVirt);
   }
}
////////////////////////////////////////////////////////////////////////////////
void usage(char* filename)
{
   char* fname = strrchr(filename, (int) '/');
   fname = (fname == NULL) ? filename : fname;

   fprintf(stderr, "usage: %s LIVE <QCIF|QVGA> <MP4|H263> <FPS> <BITRATE> <NFRAMES> <OUTFILE>\n", fname);
   fprintf(stderr, "usage: %s FILE <QCIF|QVGA> <MP4|H263 <FPS> <BITRATE> <NFRAMES> <INFILE> <OUTFILE> ", fname);
   fprintf(stderr, "<Dynamic config file - opt> <Rate Control - opt> <AVC Slice Mode - opt>\n", fname);
   fprintf(stderr, "usage: %s PROFILE <QCIF|QVGA> <MP4|H263 <FPS> <BITRATE> <NFRAMES> <INFILE>\n", fname);
   fprintf(stderr, "usage: %s PREVIEW <QCIF|QVGA> <FPS> <NFRAMES>\n", fname);
   fprintf(stderr, "usage: %s DISPLAY <QCIF|QVGA> <FPS> <NFRAMES> <INFILE>\n", fname);
   fprintf(stderr, "\n       BITRATE - bitrate in kbps\n");
   fprintf(stderr, "       FPS - frames per second\n");
   fprintf(stderr, "       NFRAMES - number of frames to play, 0 for infinite\n");
   fprintf(stderr, "       RateControl (Values 0 - 4 for RC_OFF, RC_CBR_CFR, RC_CBR_VFR, RC_VBR_CFR, RC_VBR_VFR\n");
   exit(1);
}

bool parseWxH(char *str, OMX_U32 *width, OMX_U32 *height)
{
   bool parseOK = false;
   const char delimiters[] = " x*,";
   char *token, *dupstr, *temp;
   OMX_U32 w, h;

   dupstr = strdup(str);
   token = strtok_r(dupstr, delimiters, &temp);
   if (token)
   {
       w = strtoul(token, NULL, 10);
       token = strtok_r(NULL, delimiters, &temp);
       if (token)
       {
           h = strtoul(token, NULL, 10);
           if (w != ULONG_MAX && h != ULONG_MAX)
           {
#ifdef MAX_RES_720P
              if ((w * h >> 8) <= 3600)
              {
                 parseOK = true;
                 *width = w;
                 *height = h;
                 }
#else
              if ((w * h >> 8) <= 8160)
              {
                 parseOK = true;
                 *width = w;
                 *height = h;
                 }
#endif
              else
                 E("\nInvalid dimensions %dx%d",w,h);
              }
           }
       }
   free(dupstr);
   return parseOK;
}

////////////////////////////////////////////////////////////////////////////////
void parseArgs(int argc, char** argv)
{
   int dyn_file_arg = argc;
   if (argc == 1)
   {
      usage(argv[0]);
   }
   else if (strcmp("PREVIEW", argv[1]) == 0 ||
            strcmp("preview", argv[1]) == 0)
   {
      m_eMode = MODE_PREVIEW;
      if (argc != 5)
      {
         usage(argv[0]);
      }
   }
   else if (strcmp("DISPLAY", argv[1]) == 0 ||
            strcmp("display", argv[1]) == 0)
   {
      m_eMode = MODE_DISPLAY;
      if (argc != 6)
      {
         usage(argv[0]);
      }
      m_sProfile.cInFileName = argv[5];
      m_sProfile.cOutFileName = NULL;
   }
   else if (strcmp("LIVE", argv[1]) == 0 ||
            strcmp("live", argv[1]) == 0)
   {//263
      m_eMode = MODE_LIVE_ENCODE;
      if (argc != 8)
      {
         usage(argv[0]);
      }
      m_sProfile.cInFileName = NULL;
      m_sProfile.cOutFileName = argv[7];
   }
   else if (strcmp("FILE", argv[1]) == 0 ||
            strcmp("file", argv[1]) == 0)
   {//263
      m_eMode = MODE_FILE_ENCODE;

      if(argc < 9 || argc > 13)
      {
          usage(argv[0]);
      }
      else
      {
         if (argc > 9)
            dyn_file_arg = 9;

         if (argc > 10)
         {
           m_sProfile.eControlRate = OMX_Video_ControlRateVariable;
            int RC = atoi(argv[10]);

            switch (RC)
            {
            case 0:
               m_sProfile.eControlRate  = OMX_Video_ControlRateDisable ;//VENC_RC_NONE
               break;
            case 1:
               m_sProfile.eControlRate  = OMX_Video_ControlRateConstant;//VENC_RC_CBR_CFR
               break;

            case 2:
               m_sProfile.eControlRate  = OMX_Video_ControlRateConstantSkipFrames;//VENC_RC_CBR_VFR
               break;

            case 3:
               m_sProfile.eControlRate  =OMX_Video_ControlRateVariable ;//VENC_RC_VBR_CFR
               break;

            case 4:
               m_sProfile.eControlRate  = OMX_Video_ControlRateVariableSkipFrames;//VENC_RC_VBR_VFR
               break;

           default:
               E("invalid rate control selection");
               m_sProfile.eControlRate = OMX_Video_ControlRateVariable; //VENC_RC_VBR_CFR
               break;
            }
         }

         if (argc > 11)
         {
            int profile_argi = 11;
            if(!strcmp(argv[3], "H264") || !strcmp(argv[3], "h264"))
            {
               profile_argi = 12;
               D("\nSetting AVCSliceMode ... ");
               int AVCSliceMode = atoi(argv[11]);
               switch(AVCSliceMode)
               {
               case 0:
                  m_sProfile.eSliceMode = OMX_VIDEO_SLICEMODE_AVCDefault;
                  break;

               case 1:
                  m_sProfile.eSliceMode = OMX_VIDEO_SLICEMODE_AVCMBSlice;
                  break;

               case 2:
                  m_sProfile.eSliceMode = OMX_VIDEO_SLICEMODE_AVCByteSlice;
                  break;

               default:
                  E("invalid Slice Mode");
                  m_sProfile.eSliceMode = OMX_VIDEO_SLICEMODE_AVCDefault;
                  break;
              }
            }
            if (profile_argi < argc)
            {
               if (!strncmp(argv[profile_argi], "0x", 2) || !strncmp(argv[profile_argi], "0x", 2))
               {
                  m_sProfile.nUserProfile = strtoul(argv[profile_argi], NULL, 16);
               }
               else
               {
                  m_sProfile.nUserProfile = strtoul(argv[profile_argi], NULL, 10);
               }
               if (!m_sProfile.nUserProfile || m_sProfile.nUserProfile == ULONG_MAX)
               {
                  E("invalid specified Profile %s, using default", argv[profile_argi]);
                  m_sProfile.nUserProfile = 0;
               }
            }
         }
      }
      m_sProfile.cInFileName = argv[7];
      m_sProfile.cOutFileName = argv[8];
   }
   else if (strcmp("PROFILE", argv[1]) == 0 ||
            strcmp("profile", argv[1]) == 0)
   {//263
      m_eMode = MODE_PROFILE;
      if (argc != 8)
      {
         usage(argv[0]);
      }
      m_sProfile.cInFileName = argv[7];
      m_sProfile.cOutFileName = NULL;
   }
   else
   {
      usage(argv[0]);
   }


   if (strcmp("QCIF", argv[2]) == 0 ||
       strcmp("qcif", argv[2]) == 0)
   {
      m_sProfile.nFrameWidth = 176;
      m_sProfile.nFrameHeight = 144;
      m_sProfile.nFrameBytes = 176*144*3/2;
      m_sProfile.eLevel = OMX_VIDEO_MPEG4Level0;
   }
   else if (strcmp("QVGA", argv[2]) == 0 ||
            strcmp("qvga", argv[2]) == 0)
   {
      m_sProfile.nFrameWidth = 320;
      m_sProfile.nFrameHeight = 240;
      m_sProfile.nFrameBytes = 320*240*3/2;
      m_sProfile.eLevel = OMX_VIDEO_MPEG4Level1;
   }


    else if (strcmp("VGA", argv[2]) == 0 ||
            strcmp("vga", argv[2]) == 0)
   {
      m_sProfile.nFrameWidth = 640;
      m_sProfile.nFrameHeight = 480;
      m_sProfile.nFrameBytes = 640*480*3/2;
      m_sProfile.eLevel = OMX_VIDEO_MPEG4Level1;
   }

    else if (strcmp("WVGA", argv[2]) == 0 ||
            strcmp("wvga", argv[2]) == 0)
   {
      m_sProfile.nFrameWidth = 800;
      m_sProfile.nFrameHeight = 480;
      m_sProfile.nFrameBytes = 800*480*3/2;
      m_sProfile.eLevel = OMX_VIDEO_MPEG4Level1;
   }
  else if (strcmp("CIF", argv[2]) == 0 ||
            strcmp("cif", argv[2]) == 0)
   {
      m_sProfile.nFrameWidth = 352;
      m_sProfile.nFrameHeight = 288;
      m_sProfile.nFrameBytes = 352*288*3/2;
      m_sProfile.eLevel = OMX_VIDEO_MPEG4Level1;
   }
   else if (strcmp("720", argv[2]) == 0)
   {
      m_sProfile.nFrameWidth = 1280;
      m_sProfile.nFrameHeight = 720;
      m_sProfile.nFrameBytes = 720*1280*3/2;
      m_sProfile.eLevel = OMX_VIDEO_MPEG4Level1;
   }
   else if (strcmp("1080", argv[2]) == 0)
   {
      m_sProfile.nFrameWidth = 1920;
      m_sProfile.nFrameHeight = 1080;
      m_sProfile.nFrameBytes = 1920*1080*3/2;
      m_sProfile.eLevel = OMX_VIDEO_MPEG4Level1;
   }
   else if (parseWxH(argv[2], &m_sProfile.nFrameWidth, &m_sProfile.nFrameHeight))
   {
      m_sProfile.nFrameBytes = m_sProfile.nFrameWidth*m_sProfile.nFrameHeight*3/2;
      m_sProfile.eLevel = OMX_VIDEO_MPEG4Level1;
   }
   else
   {
      usage(argv[0]);
   }

#ifdef BADGER
   m_sProfile.nFramestride =  (m_sProfile.nFrameWidth + 31) & (~31);
   m_sProfile.nFrameScanlines = (m_sProfile.nFrameHeight + 31) & (~31);
   m_sProfile.nFrameBytes = ((m_sProfile.nFramestride * m_sProfile.nFrameScanlines * 3/2) + 4095) & (~4095);
   E("stride: %d, Scanlines: %d, Size: %d",
     m_sProfile.nFramestride, m_sProfile.nFrameScanlines, m_sProfile.nFrameBytes);
   m_sProfile.nFrameRead = m_sProfile.nFramestride * m_sProfile.nFrameScanlines * 3/2;
#endif
   if (m_eMode == MODE_DISPLAY ||
       m_eMode == MODE_PREVIEW)
   {
      m_sProfile.nFramerate = atof(argv[3]);
      m_nFramePlay = atoi(argv[4]);

   }
   else if (m_eMode == MODE_LIVE_ENCODE ||
            m_eMode == MODE_FILE_ENCODE ||
            m_eMode == MODE_PROFILE)
   {
      if ((!strcmp(argv[3], "MP4")) || (!strcmp(argv[3], "mp4")))
      {
         m_sProfile.eCodec = OMX_VIDEO_CodingMPEG4;
      }
      else if ((!strcmp(argv[3], "H263")) || (!strcmp(argv[3], "h263")))
      {
         m_sProfile.eCodec = OMX_VIDEO_CodingH263;
      }
      else if ((!strcmp(argv[3], "H264")) || (!strcmp(argv[3], "h264")))
      {
         m_sProfile.eCodec = OMX_VIDEO_CodingAVC;
      }
      else
      {
         usage(argv[0]);
      }

      m_sProfile.nFramerate = atof(argv[4]);
      m_sProfile.nBitrate = atoi(argv[5]);
//      m_sProfile.eControlRate = OMX_Video_ControlRateVariable;
      m_nFramePlay = atoi(argv[6]);
      if (dyn_file_arg < argc)
      {
        m_pDynConfFile = fopen(argv[dyn_file_arg], "r");
        if (!m_pDynConfFile)
          E("ERROR: Cannot open dynamic config file: %s", argv[dyn_file_arg]);
        else
        {
          memset(&dynamic_config, 0, sizeof(struct DynamicConfig));
        }
      }
   }
}

void* Watchdog(void* data)
{
   while (1)
   {
      sleep(1000);
      if (m_bWatchDogKicked == true)
         m_bWatchDogKicked = false;
      else
         E("watchdog has not been kicked. we may have a deadlock");
   }
   return NULL;
}

int main(int argc, char** argv)
{
   OMX_U8* pvirt = NULL;
   int result;
   float enc_time_sec=0.0,enc_time_usec=0.0;

   m_nInFd = -1;
   m_nOutFd = -1;
   m_nTimeStamp = 0;
   m_nFrameIn = 0;
   m_nFrameOut = 0;

   memset(&m_sMsgQ, 0, sizeof(MsgQ));
   parseArgs(argc, argv);

   D("fps=%d, bitrate=%d, width=%d, height=%d",
     m_sProfile.nFramerate,
     m_sProfile.nBitrate,
     m_sProfile.nFrameWidth,
     m_sProfile.nFrameHeight);


   //if (m_eMode != MODE_PREVIEW && m_eMode != MODE_DISPLAY)
   //{
     // pthread_t wd;
     // pthread_create(&wd, NULL, Watchdog, NULL);
   //}

   for (int x = 0; x < num_in_buffers; x++)
   {
      // mark all buffers as ready to use
      m_bInFrameFree[x] = OMX_TRUE;
   }


    if (m_eMode != MODE_PROFILE)
   {
      #if T_ARM
	   m_nOutFd = open(m_sProfile.cOutFileName, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
      #else
	  m_nOutFd = open(m_sProfile.cOutFileName,0);
      #endif
      if (m_nOutFd < 0)
      {
         E("could not open output file %s", m_sProfile.cOutFileName);
         CHK(1);
      }
   }

   pthread_mutex_init(&m_mutex, NULL);
   pthread_cond_init(&m_signal, NULL);

   if (m_eMode != MODE_PREVIEW)
   {
      VencTest_Initialize();
   }

   ////////////////////////////////////////
   // Camera + Encode
   ////////////////////////////////////////
   if (m_eMode == MODE_LIVE_ENCODE)
   {
     CameraTest_Initialize(m_sProfile.nFramerate,
                            m_sProfile.nFrameWidth,
                            m_sProfile.nFrameHeight,
                            PreviewCallback);
      CameraTest_Run();
   }

   if (m_eMode == MODE_FILE_ENCODE ||
       m_eMode == MODE_PROFILE)
   {
      int i;
      #if T_ARM
      m_nInFd = open(m_sProfile.cInFileName, O_RDONLY);
      #else
      m_nInFd = open(m_sProfile.cInFileName,1);
      #endif
	  if (m_nInFd < 0)
      {
         E("could not open input file");
         CHK(1);

      }
      D("going to idle state");
      //SetState(OMX_StateIdle);
      OMX_SendCommand(m_hHandle,
                      OMX_CommandStateSet,
                      (OMX_U32) OMX_StateIdle,
                       NULL);

      OMX_PARAM_PORTDEFINITIONTYPE portDef;

      portDef.nPortIndex = 0;
      result = OMX_GetParameter(m_hHandle, OMX_IndexParamPortDefinition, &portDef);
      CHK(result);

      D("allocating Input buffers");
      num_in_buffers = portDef.nBufferCountActual;
      for (i = 0; i < portDef.nBufferCountActual; i++)
      {
         OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO* pMem = new OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO;
         pvirt = (OMX_U8*)PmemMalloc(pMem, m_sProfile.nFrameBytes);

         if(pvirt == NULL)
         {
            CHK(1);
         }
         result = VencTest_RegisterYUVBuffer(&m_pInBuffers[i],
                                             (OMX_U8*) pvirt,
                                             (OMX_PTR) pMem);
         CHK(result);
      }
   }
   else if (m_eMode == MODE_LIVE_ENCODE)
   {
       D("going to idle state");
       //SetState(OMX_StateIdle);
       OMX_SendCommand(m_hHandle,
                       OMX_CommandStateSet,
                       (OMX_U32) OMX_StateIdle,
                        NULL);
   }

   int i;
   OMX_PARAM_PORTDEFINITIONTYPE portDef;

   portDef.nPortIndex = 1;
   result = OMX_GetParameter(m_hHandle, OMX_IndexParamPortDefinition, &portDef);
   CHK(result);

   D("allocating output buffers");
   D("Calling UseBuffer for Output port");
   num_out_buffers = portDef.nBufferCountActual;
   for (i = 0; i < portDef.nBufferCountActual; i++)
   {
      void* pBuff;

      pBuff = malloc(portDef.nBufferSize);
     D("portDef.nBufferSize = %d ",portDef.nBufferSize);
      result = OMX_UseBuffer(m_hHandle,
                             &m_pOutBuffers[i],
                             (OMX_U32) PORT_INDEX_OUT,
                             NULL,
                             portDef.nBufferSize,
                             (OMX_U8*) pBuff);
      CHK(result);
   }
   D("allocate done");

        // D("Going to state " # eState"...");

         while (m_eState != OMX_StateIdle)
         {
            sleep(1);
         }
         //D("Now in state " # eState);


   D("going to executing state");
   SetState(OMX_StateExecuting);
   for (i = 0; i < num_out_buffers; i++)
   {
      D("filling buffer %d", i);
      result = OMX_FillThisBuffer(m_hHandle, m_pOutBuffers[i]);
      //sleep(1000);
      CHK(result);
   }

   if (m_eMode == MODE_FILE_ENCODE)
   {
      // encode the first frame to kick off the whole process
      VencTest_ReadAndEmpty(m_pInBuffers[0]);
    //  FBTest_DisplayImage(((PmemBuffer*) m_pInBuffers[0]->pAppPrivate)->fd,0);
   }

   if (m_eMode == MODE_PROFILE)
   {
      int i;

      // read several frames into memory
      D("reading frames into memory");
      for (i = 0; i < num_in_buffers; i++)
      {
        D("[%d] address 0x%x",i, m_pInBuffers[i]->pBuffer);
#ifdef MAX_RES_720P
         read(m_nInFd,
              m_pInBuffers[i]->pBuffer,
              m_sProfile.nFrameBytes);
#else
         // read Y first
         read(m_nInFd,
              m_pInBuffers[i]->pBuffer,
              m_sProfile.nFrameWidth*m_sProfile.nFrameHeight);

         // check alignment for offset to C
         OMX_U32 offset_to_c = m_sProfile.nFrameWidth * m_sProfile.nFrameHeight;

         const OMX_U32 C_2K = (1024*2),
            MASK_2K = C_2K-1,
            IMASK_2K = ~MASK_2K;

         if (offset_to_c & MASK_2K)
         {
            // offset to C is not 2k aligned, adjustment is required
            offset_to_c = (offset_to_c & IMASK_2K) + C_2K;
         }

         // read C
         read(m_nInFd,
              m_pInBuffers[i]->pBuffer + offset_to_c,
              m_sProfile.nFrameWidth*m_sProfile.nFrameHeight/2);
#endif

      }

     // FBTest_Initialize(m_sProfile.nFrameWidth, m_sProfile.nFrameHeight);

      // loop over the mem-resident frames and encode them
      D("beging playing mem-resident frames...");
      for (i = 0; m_nFramePlay == 0 || i < m_nFramePlay; i++)
      {
         int idx = i % num_in_buffers;
         if (m_bInFrameFree[idx] == OMX_FALSE)
         {
            int j;
            E("the expected buffer is not free, but lets find another");

            idx = -1;

            // lets see if we can find another free buffer
            for (j = 0; j < num_in_buffers; j++)
            {
               if(m_bInFrameFree[j])
               {
                  idx = j;
                  break;
               }
            }
         }

         // if we have a free buffer let's encode it
         if (idx >= 0)
         {
            D("encode frame %d...m_pInBuffers[idx]->pBuffer=0x%x", i,m_pInBuffers[idx]->pBuffer);
            m_bInFrameFree[idx] = OMX_FALSE;
            VencTest_EncodeFrame(m_pInBuffers[idx]->pBuffer,
                                 m_nTimeStamp);
            D("display frame %d...", i);
        //    FBTest_DisplayImage(((PmemBuffer*) m_pInBuffers[idx]->pAppPrivate)->fd,0);
            m_nTimeStamp += 1000000 / m_sProfile.nFramerate;
         }
         else
         {
            E("wow, no buffers are free, performance "
              "is not so good. lets just sleep some more");

         }
         D("sleep for %d microsec", 1000000/m_sProfile.nFramerate);
         sleep (1000000 / m_sProfile.nFramerate);
      }
     // FBTest_Exit();
   }

   Msg msg;
   bool bQuit = false;
   while ((m_eMode == MODE_FILE_ENCODE || m_eMode == MODE_LIVE_ENCODE) &&
          !bQuit)
   {
      PopMessage(&msg);
      switch (msg.id)
      {
      //////////////////////////////////
      // FRAME IS ENCODED
      //////////////////////////////////
      case MSG_ID_INPUT_FRAME_DONE:
         /*pthread_mutex_lock(&m_mutex);
         ++m_nFrameOut;
         if (m_nFrameOut == m_nFramePlay && m_nFramePlay != 0)
         {
            bQuit = true;
         }
         pthread_mutex_unlock(&m_mutex);*/

         if (!bQuit && m_eMode == MODE_FILE_ENCODE)
         {
            D("pushing another frame down to encoder");
            if (VencTest_ReadAndEmpty(msg.data.sBitstreamData.pBuffer))
            {
               // we have read the last frame
               D("main is exiting...");
               bQuit = true;
            }
         }
       break;
      case MSG_ID_OUTPUT_FRAME_DONE:
         D("================ writing frame %d = %d bytes to output file",
           m_nFrameOut+1,
           msg.data.sBitstreamData.pBuffer->nFilledLen);
         D("StopEncodeTime=%lld", GetTimeStamp());


		 write(m_nOutFd,
               msg.data.sBitstreamData.pBuffer->pBuffer,
               msg.data.sBitstreamData.pBuffer->nFilledLen);


         result = OMX_FillThisBuffer(m_hHandle,
                                     msg.data.sBitstreamData.pBuffer);

         if (result != OMX_ErrorNone)
         {
            CHK(result);
         }

         pthread_mutex_lock(&m_mutex);
         ++m_nFrameOut;
         if (m_nFrameOut == m_nFramePlay && m_nFramePlay != 0)
         {
            bQuit = true;
         }
         pthread_mutex_unlock(&m_mutex);
         break;

      default:
         E("invalid msg id %d", (int) msg.id);
      } // end switch (msg.id)

/*  // TO UNCOMMENT FOR PAUSE TESTINGS
      if(m_nFrameOut == 10)
      {
         E("\nGoing to Pause state\n");
         SetState(OMX_StatePause);
         sleep(3);
//REQUEST AN I FRAME AFTER PAUSE
         OMX_CONFIG_INTRAREFRESHVOPTYPE voprefresh;
         voprefresh.nPortIndex = (OMX_U32)PORT_INDEX_OUT;
         voprefresh.IntraRefreshVOP = OMX_TRUE;
         result = OMX_SetConfig(m_hHandle,
                                   OMX_IndexConfigVideoIntraVOPRefresh,
                                   &voprefresh);
         E("\n OMX_IndexConfigVideoIntraVOPRefresh Set Paramter port");
         CHK(result);
         E("\nGoing to executing state\n");
         SetState(OMX_StateExecuting);
      }
*/
   } // end while (!bQuit)


   if (m_eMode == MODE_LIVE_ENCODE)
   {
      CameraTest_Exit();
      close(m_nOutFd);
   }
   else if (m_eMode == MODE_FILE_ENCODE ||
            m_eMode == MODE_PROFILE)
   {
      // deallocate pmem buffers
      for (int i = 0; i < num_in_buffers; i++)
      {
         PmemFree((OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO*)m_pInBuffers[i]->pAppPrivate,
                  m_pInBuffers[i]->pBuffer,
                  m_sProfile.nFrameBytes);
         delete (OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO*) m_pInBuffers[i]->pAppPrivate;
      }
      close(m_nInFd);

      if (m_eMode == MODE_FILE_ENCODE)
      {
         close(m_nOutFd);
      }
      if (m_pDynConfFile)
      {
        fclose(m_pDynConfFile);
        m_pDynConfFile = NULL;
      }
   }

   if (m_eMode != MODE_PREVIEW)
   {
      D("exit encoder test");
      VencTest_Exit();
   }

   pthread_mutex_destroy(&m_mutex);
   pthread_cond_destroy(&m_signal);

   /* Time Statistics Logging */
   if(0 != m_sProfile.nFramerate)
   {
      enc_time_usec = m_nTimeStamp - (1000000 / m_sProfile.nFramerate);
      enc_time_sec =enc_time_usec/1000000;
      if(0 != enc_time_sec)
      {
         printf("Total Frame Rate: %f",ebd_cnt/enc_time_sec);
         printf("\nEncoder Bitrate :%lf Kbps",(tot_bufsize*8)/(enc_time_sec*1000));
      }
   }
   else
   {
      printf("\n\n Encode Time is zero");
   }
   printf("\nTotal Number of Frames :%d",ebd_cnt);
   printf("\nNumber of dropped frames during encoding:%d\n",ebd_cnt-fbd_cnt);
   /* End of Time Statistics Logging */

   D("main has exited");
   return 0;
}
