/*--------------------------------------------------------------------------
Copyright (c) 2010-2012, Code Aurora Forum. All rights reserved.

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
/*
    An Open max test application ....
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <time.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include "OMX_QCOMExtns.h"
#include <sys/time.h>

#ifdef _ANDROID_
#include <binder/MemoryHeapBase.h>

extern "C"{
#include<utils/Log.h>
}
#define LOG_TAG "OMX-VDEC-TEST"
#define DEBUG_PRINT
#define DEBUG_PRINT_ERROR ALOGE

//#define __DEBUG_DIVX__ // Define this macro to print (through logcat)
                         // the kind of frames packed per buffer and
                         // timestamps adjustments for divx.

//#define TEST_TS_FROM_SEI // Define this macro to calculate the timestamps
                           // from the SEI and VUI data for H264

#else
#include <glib.h>
#define strlcpy g_strlcpy

#define ALOGE(fmt, args...) fprintf(stderr, fmt, ##args)
#define DEBUG_PRINT printf
#define DEBUG_PRINT_ERROR printf
#endif /* _ANDROID_ */

#include "OMX_Core.h"
#include "OMX_Component.h"
#include "OMX_QCOMExtns.h"
extern "C" {
#include "queue.h"
}

#include <inttypes.h>
#include <linux/msm_mdp.h>
#include <linux/fb.h>

/************************************************************************/
/*              #DEFINES                            */
/************************************************************************/
#define DELAY 66
#define false 0
#define true 1
#define VOP_START_CODE 0x000001B6
#define SHORT_HEADER_START_CODE 0x00008000
#define MPEG2_FRAME_START_CODE 0x00000100
#define MPEG2_SEQ_START_CODE 0x000001B3
#define VC1_START_CODE  0x00000100
#define VC1_FRAME_START_CODE  0x0000010D
#define VC1_FRAME_FIELD_CODE  0x0000010C
#define VC1_SEQUENCE_START_CODE 0x0000010F
#define VC1_ENTRY_POINT_START_CODE 0x0000010E
#define NUMBER_OF_ARBITRARYBYTES_READ  (4 * 1024)
#define VC1_SEQ_LAYER_SIZE_WITHOUT_STRUCTC 32
#define VC1_SEQ_LAYER_SIZE_V1_WITHOUT_STRUCTC 16
static int previous_vc1_au = 0;
#define CONFIG_VERSION_SIZE(param) \
    param.nVersion.nVersion = CURRENT_OMX_SPEC_VERSION;\
    param.nSize = sizeof(param);

#define FAILED(result) (result != OMX_ErrorNone)

#define SUCCEEDED(result) (result == OMX_ErrorNone)
#define SWAPBYTES(ptrA, ptrB) { char t = *ptrA; *ptrA = *ptrB; *ptrB = t;}
#define SIZE_NAL_FIELD_MAX  4
#define MDP_DEINTERLACE 0x80000000

#define ALLOCATE_BUFFER 0

#ifdef MAX_RES_720P
#define PMEM_DEVICE "/dev/pmem_adsp"
#elif MAX_RES_1080P_EBI
#define PMEM_DEVICE "/dev/pmem_adsp"
#elif MAX_RES_1080P
#define PMEM_DEVICE "/dev/pmem_smipool"
#endif

//#define USE_EXTERN_PMEM_BUF

/************************************************************************/
/*              GLOBAL DECLARATIONS                     */
/************************************************************************/
#ifdef _ANDROID_
using namespace android;
#endif

typedef enum {
  CODEC_FORMAT_H264 = 1,
  CODEC_FORMAT_MP4,
  CODEC_FORMAT_H263,
  CODEC_FORMAT_VC1,
  CODEC_FORMAT_DIVX,
  CODEC_FORMAT_MPEG2,
  CODEC_FORMAT_MAX = CODEC_FORMAT_MPEG2
} codec_format;

typedef enum {
  FILE_TYPE_DAT_PER_AU = 1,
  FILE_TYPE_ARBITRARY_BYTES,
  FILE_TYPE_COMMON_CODEC_MAX,

  FILE_TYPE_START_OF_H264_SPECIFIC = 10,
  FILE_TYPE_264_NAL_SIZE_LENGTH = FILE_TYPE_START_OF_H264_SPECIFIC,

  FILE_TYPE_START_OF_MP4_SPECIFIC = 20,
  FILE_TYPE_PICTURE_START_CODE = FILE_TYPE_START_OF_MP4_SPECIFIC,

  FILE_TYPE_START_OF_VC1_SPECIFIC = 30,
  FILE_TYPE_RCV = FILE_TYPE_START_OF_VC1_SPECIFIC,
  FILE_TYPE_VC1,

  FILE_TYPE_START_OF_DIVX_SPECIFIC = 40,
  FILE_TYPE_DIVX_4_5_6 = FILE_TYPE_START_OF_DIVX_SPECIFIC,
  FILE_TYPE_DIVX_311,

  FILE_TYPE_START_OF_MPEG2_SPECIFIC = 50,
  FILE_TYPE_MPEG2_START_CODE = FILE_TYPE_START_OF_MPEG2_SPECIFIC

} file_type;

typedef enum {
  GOOD_STATE = 0,
  PORT_SETTING_CHANGE_STATE,
  ERROR_STATE
} test_status;

typedef enum {
  FREE_HANDLE_AT_LOADED = 1,
  FREE_HANDLE_AT_IDLE,
  FREE_HANDLE_AT_EXECUTING,
  FREE_HANDLE_AT_PAUSE
} freeHandle_test;

struct temp_egl {
    int pmem_fd;
    int offset;
};

static int (*Read_Buffer)(OMX_BUFFERHEADERTYPE  *pBufHdr );

int inputBufferFileFd;

FILE * outputBufferFile;
FILE * seqFile;
int takeYuvLog = 0;
int displayYuv = 0;
int displayWindow = 0;
int realtime_display = 0;

Queue *etb_queue = NULL;
Queue *fbd_queue = NULL;

pthread_t ebd_thread_id;
pthread_t fbd_thread_id;
void* ebd_thread(void*);
void* fbd_thread(void*);

pthread_mutex_t etb_lock;
pthread_mutex_t fbd_lock;
pthread_mutex_t lock;
pthread_cond_t cond;
pthread_mutex_t eos_lock;
pthread_cond_t eos_cond;
pthread_mutex_t enable_lock;

sem_t etb_sem;
sem_t fbd_sem;
sem_t seq_sem;
sem_t in_flush_sem, out_flush_sem;

OMX_PARAM_PORTDEFINITIONTYPE portFmt;
OMX_PORT_PARAM_TYPE portParam;
OMX_ERRORTYPE error;
OMX_COLOR_FORMATTYPE color_fmt;
static bool input_use_buffer = false,output_use_buffer = false;
QOMX_VIDEO_DECODER_PICTURE_ORDER picture_order;

#ifdef MAX_RES_1080P
unsigned int color_fmt_type = 1;
#else
unsigned int color_fmt_type = 0;
#endif

#define CLR_KEY  0xe8fd
#define COLOR_BLACK_RGBA_8888 0x00000000
#define FRAMEBUFFER_32

static int fb_fd = -1;
static struct fb_var_screeninfo vinfo;
static struct fb_fix_screeninfo finfo;
static struct mdp_overlay overlay, *overlayp;
static struct msmfb_overlay_data ov_front;
static int vid_buf_front_id;
static char tempbuf[16];
int overlay_fb(struct OMX_BUFFERHEADERTYPE *pBufHdr);
void overlay_set();
void overlay_unset();
void render_fb(struct OMX_BUFFERHEADERTYPE *pBufHdr);
int disable_output_port();
int enable_output_port();
int output_port_reconfig();
void free_output_buffers();
int open_display();
void close_display();
/************************************************************************/
/*              GLOBAL INIT                 */
/************************************************************************/
int input_buf_cnt = 0;
int height =0, width =0;
int sliceheight = 0, stride = 0;
int used_ip_buf_cnt = 0;
unsigned free_op_buf_cnt = 0;
volatile int event_is_done = 0;
int ebd_cnt= 0, fbd_cnt = 0;
int bInputEosReached = 0;
int bOutputEosReached = 0;
char in_filename[512];
char seq_file_name[512];
unsigned char seq_enabled = 0;
bool anti_flickering = true;
unsigned char flush_input_progress = 0, flush_output_progress = 0;
unsigned cmd_data = ~(unsigned)0, etb_count = 0;

char curr_seq_command[100];
OMX_S64 timeStampLfile = 0;
int fps = 30;
unsigned int timestampInterval = 33333;
codec_format  codec_format_option;
file_type     file_type_option;
freeHandle_test freeHandle_option;
int nalSize = 0;
int sent_disabled = 0;
int waitForPortSettingsChanged = 1;
test_status currentStatus = GOOD_STATE;
struct timeval t_start = {0, 0}, t_end = {0, 0};

//* OMX Spec Version supported by the wrappers. Version = 1.1 */
const OMX_U32 CURRENT_OMX_SPEC_VERSION = 0x00000101;
OMX_COMPONENTTYPE* dec_handle = 0;

OMX_BUFFERHEADERTYPE  **pInputBufHdrs = NULL;
OMX_BUFFERHEADERTYPE  **pOutYUVBufHdrs= NULL;

static OMX_BOOL use_external_pmem_buf = OMX_FALSE;

int rcv_v1=0;
static struct temp_egl **p_eglHeaders = NULL;
static unsigned use_buf_virt_addr[32];

OMX_QCOM_PLATFORM_PRIVATE_LIST      *pPlatformList = NULL;
OMX_QCOM_PLATFORM_PRIVATE_ENTRY     *pPlatformEntry = NULL;
OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO *pPMEMInfo = NULL;


static int bHdrflag = 0;

/* Performance related variable*/
//QPERF_INIT(render_fb);
//QPERF_INIT(client_decode);

/************************************************************************/
/*              GLOBAL FUNC DECL                        */
/************************************************************************/
int Init_Decoder();
int Play_Decoder();
int run_tests();

/**************************************************************************/
/*              STATIC DECLARATIONS                       */
/**************************************************************************/
static int video_playback_count = 1;
static int open_video_file ();
static int Read_Buffer_From_DAT_File(OMX_BUFFERHEADERTYPE  *pBufHdr );
static int Read_Buffer_ArbitraryBytes(OMX_BUFFERHEADERTYPE  *pBufHdr);
static int Read_Buffer_From_Vop_Start_Code_File(OMX_BUFFERHEADERTYPE  *pBufHdr);
static int Read_Buffer_From_Mpeg2_Start_Code(OMX_BUFFERHEADERTYPE  *pBufHdr);
static int Read_Buffer_From_Size_Nal(OMX_BUFFERHEADERTYPE  *pBufHdr);
static int Read_Buffer_From_RCV_File_Seq_Layer(OMX_BUFFERHEADERTYPE  *pBufHdr);
static int Read_Buffer_From_RCV_File(OMX_BUFFERHEADERTYPE  *pBufHdr);
static int Read_Buffer_From_VC1_File(OMX_BUFFERHEADERTYPE  *pBufHdr);
static int Read_Buffer_From_DivX_4_5_6_File(OMX_BUFFERHEADERTYPE  *pBufHdr);
static int Read_Buffer_From_DivX_311_File(OMX_BUFFERHEADERTYPE  *pBufHdr);

static OMX_ERRORTYPE Allocate_Buffer ( OMX_COMPONENTTYPE *dec_handle,
                                       OMX_BUFFERHEADERTYPE  ***pBufHdrs,
                                       OMX_U32 nPortIndex,
                                       long bufCntMin, long bufSize);

static OMX_ERRORTYPE use_input_buffer(OMX_COMPONENTTYPE      *dec_handle,
                                OMX_BUFFERHEADERTYPE ***bufferHdr,
                                OMX_U32              nPortIndex,
                                OMX_U32              bufSize,
                                long                 bufcnt);

static OMX_ERRORTYPE use_output_buffer(OMX_COMPONENTTYPE      *dec_handle,
                                OMX_BUFFERHEADERTYPE ***bufferHdr,
                                OMX_U32              nPortIndex,
                                OMX_U32              bufSize,
                                long                 bufcnt);

static OMX_ERRORTYPE use_output_buffer_multiple_fd(OMX_COMPONENTTYPE      *dec_handle,
                                                   OMX_BUFFERHEADERTYPE ***bufferHdr,
                                                   OMX_U32              nPortIndex,
                                                   OMX_U32              bufSize,
                                                   long                 bufcnt);

static OMX_ERRORTYPE EventHandler(OMX_IN OMX_HANDLETYPE hComponent,
                                  OMX_IN OMX_PTR pAppData,
                                  OMX_IN OMX_EVENTTYPE eEvent,
                                  OMX_IN OMX_U32 nData1, OMX_IN OMX_U32 nData2,
                                  OMX_IN OMX_PTR pEventData);
static OMX_ERRORTYPE EmptyBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
                                     OMX_IN OMX_PTR pAppData,
                                     OMX_IN OMX_BUFFERHEADERTYPE* pBuffer);
static OMX_ERRORTYPE FillBufferDone(OMX_OUT OMX_HANDLETYPE hComponent,
                                    OMX_OUT OMX_PTR pAppData,
                                    OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer);

static void do_freeHandle_and_clean_up(bool isDueToError);

#ifndef USE_ION
static bool align_pmem_buffers(int pmem_fd, OMX_U32 buffer_size,
                                  OMX_U32 alignment);
#endif
void getFreePmem();
static  int clip2(int x)
    {
        x = x -1;
        x = x | x >> 1;
        x = x | x >> 2;
        x = x | x >> 4;
        x = x | x >> 16;
        x = x + 1;
        return x;
    }
void wait_for_event(void)
{
    DEBUG_PRINT("Waiting for event\n");
    pthread_mutex_lock(&lock);
    while (event_is_done == 0) {
        pthread_cond_wait(&cond, &lock);
    }
    event_is_done = 0;
    pthread_mutex_unlock(&lock);
    DEBUG_PRINT("Running .... get the event\n");
}

void event_complete(void )
{
    pthread_mutex_lock(&lock);
    if (event_is_done == 0) {
        event_is_done = 1;
        pthread_cond_broadcast(&cond);
    }
    pthread_mutex_unlock(&lock);
}
int get_next_command(FILE *seq_file)
{
    int i = -1;
    do{
        i++;
        if(fread(&curr_seq_command[i], 1, 1, seq_file) != 1)
            return -1;
    }while(curr_seq_command[i] != '\n');
    curr_seq_command[i] = 0;
    printf("\n cmd_str = %s", curr_seq_command);
    return 0;
}

int process_current_command(const char *seq_command)
{
    char *data_str = NULL;
    unsigned int data = 0, bufCnt = 0, i = 0;
    int frameSize;

    if(strstr(seq_command, "pause") == seq_command)
    {
        printf("\n\n $$$$$   PAUSE    $$$$$");
        data_str = (char*)seq_command + strlen("pause") + 1;
        data = atoi(data_str);
        printf("\n After frame number %u", data);
        cmd_data = data;
        sem_wait(&seq_sem);
        if (!bOutputEosReached && !bInputEosReached)
        {
            printf("\n Sending PAUSE cmd to OMX compt");
            OMX_SendCommand(dec_handle, OMX_CommandStateSet, OMX_StatePause,0);
            wait_for_event();
            printf("\n EventHandler for PAUSE DONE");
        }
        else
            seq_enabled = 0;
    }
    else if(strstr(seq_command, "sleep") == seq_command)
    {
        printf("\n\n $$$$$   SLEEP    $$$$$");
        data_str = (char*)seq_command + strlen("sleep") + 1;
        data = atoi(data_str);
        printf("\n Sleep Time = %u ms", data);
        usleep(data*1000);
    }
    else if(strstr(seq_command, "resume") == seq_command)
    {
        printf("\n\n $$$$$   RESUME    $$$$$");
        printf("\n Immediate effect");
        printf("\n Sending RESUME cmd to OMX compt");
        OMX_SendCommand(dec_handle, OMX_CommandStateSet, OMX_StateExecuting,0);
        wait_for_event();
        printf("\n EventHandler for RESUME DONE");
    }
    else if(strstr(seq_command, "flush") == seq_command)
    {
        printf("\n\n $$$$$   FLUSH    $$$$$");
        data_str = (char*)seq_command + strlen("flush") + 1;
        data = atoi(data_str);
        printf("\n After frame number %u", data);
        if (previous_vc1_au)
        {
            printf("\n Flush not allowed on Field boundary");
            return 0;
        }
        cmd_data = data;
        sem_wait(&seq_sem);
        if (!bOutputEosReached && !bInputEosReached)
        {
            printf("\n Sending FLUSH cmd to OMX compt");
            flush_input_progress = 1;
            flush_output_progress = 1;
            OMX_SendCommand(dec_handle, OMX_CommandFlush, OMX_ALL, 0);
            wait_for_event();
            printf("\n EventHandler for FLUSH DONE");
            printf("\n Post EBD_thread flush sem");
            sem_post(&in_flush_sem);
            printf("\n Post FBD_thread flush sem");
            sem_post(&out_flush_sem);
        }
        else
            seq_enabled = 0;
    }
    else if(strstr(seq_command, "disable_op") == seq_command)
    {
        printf("\n\n $$$$$   DISABLE OP PORT    $$$$$");
        data_str = (char*)seq_command + strlen("disable_op") + 1;
        data = atoi(data_str);
        printf("\n After frame number %u", data);
        cmd_data = data;
        sem_wait(&seq_sem);
        printf("\n Sending DISABLE OP cmd to OMX compt");
        if (disable_output_port() != 0)
        {
            printf("\n ERROR: While DISABLE OP...");
            do_freeHandle_and_clean_up(true);
            return -1;
        }
        else
            printf("\n EventHandler for DISABLE OP");
    }
    else if(strstr(seq_command, "enable_op") == seq_command)
    {
        printf("\n\n $$$$$   ENABLE OP PORT    $$$$$");
        data_str = (char*)seq_command + strlen("enable_op") + 1;
        printf("\n Sending ENABLE OP cmd to OMX compt");
        if (enable_output_port() != 0)
        {
            printf("\n ERROR: While ENABLE OP...");
            do_freeHandle_and_clean_up(true);
            return -1;
        }
        else
            printf("\n EventHandler for ENABLE OP");
    }
    else
    {
        printf("\n\n $$$$$   INVALID CMD    $$$$$");
        printf("\n seq_command[%s] is invalid", seq_command);
        seq_enabled = 0;
    }
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
void* ebd_thread(void* pArg)
{
  while(currentStatus != ERROR_STATE)
  {
    int readBytes =0;
    OMX_BUFFERHEADERTYPE* pBuffer = NULL;

    if(flush_input_progress)
    {
        DEBUG_PRINT("\n EBD_thread flush wait start");
        sem_wait(&in_flush_sem);
        DEBUG_PRINT("\n EBD_thread flush wait complete");
    }

    sem_wait(&etb_sem);
    pthread_mutex_lock(&etb_lock);
    pBuffer = (OMX_BUFFERHEADERTYPE *) pop(etb_queue);
    pthread_mutex_unlock(&etb_lock);
    if(pBuffer == NULL)
    {
      DEBUG_PRINT_ERROR("Error - No etb pBuffer to dequeue\n");
      continue;
    }

    pBuffer->nOffset = 0;
    if((readBytes = Read_Buffer(pBuffer)) > 0) {
        pBuffer->nFilledLen = readBytes;
        DEBUG_PRINT("%s: Timestamp sent(%lld)", __FUNCTION__, pBuffer->nTimeStamp);
        OMX_EmptyThisBuffer(dec_handle,pBuffer);
        etb_count++;
    }
    else
    {
        pBuffer->nFlags |= OMX_BUFFERFLAG_EOS;
        bInputEosReached = true;
        pBuffer->nFilledLen = readBytes;
        DEBUG_PRINT("%s: Timestamp sent(%lld)", __FUNCTION__, pBuffer->nTimeStamp);
        OMX_EmptyThisBuffer(dec_handle,pBuffer);
        DEBUG_PRINT("EBD::Either EOS or Some Error while reading file\n");
        etb_count++;
        break;
    }
  }
  return NULL;
}

void* fbd_thread(void* pArg)
{
  long unsigned act_time = 0, display_time = 0, render_time = 5e3, lipsync = 15e3;
  struct timeval t_avsync = {0, 0}, base_avsync = {0, 0};
  float total_time = 0;
  int canDisplay = 1, contigous_drop_frame = 0, bytes_written = 0, ret = 0;
  OMX_S64 base_timestamp = 0, lastTimestamp = 0;
  OMX_BUFFERHEADERTYPE *pBuffer = NULL, *pPrevBuff = NULL;
  pthread_mutex_lock(&eos_lock);

  DEBUG_PRINT("First Inside %s\n", __FUNCTION__);
  while(currentStatus != ERROR_STATE && !bOutputEosReached)
  {
    pthread_mutex_unlock(&eos_lock);
    DEBUG_PRINT("Inside %s\n", __FUNCTION__);
    if(flush_output_progress)
    {
        DEBUG_PRINT("\n FBD_thread flush wait start");
        sem_wait(&out_flush_sem);
        DEBUG_PRINT("\n FBD_thread flush wait complete");
    }
    sem_wait(&fbd_sem);
    pthread_mutex_lock(&enable_lock);
    if (sent_disabled)
    {
      pthread_mutex_unlock(&enable_lock);
      pthread_mutex_lock(&fbd_lock);
      if (free_op_buf_cnt == portFmt.nBufferCountActual)
        free_output_buffers();
      pthread_mutex_unlock(&fbd_lock);
      pthread_mutex_lock(&eos_lock);
      continue;
    }
    pthread_mutex_unlock(&enable_lock);
    if (anti_flickering)
      pPrevBuff = pBuffer;
    pthread_mutex_lock(&fbd_lock);
    pBuffer = (OMX_BUFFERHEADERTYPE *)pop(fbd_queue);
    pthread_mutex_unlock(&fbd_lock);
    if (pBuffer == NULL)
    {
      if (anti_flickering)
        pBuffer = pPrevBuff;
      DEBUG_PRINT("Error - No pBuffer to dequeue\n");
      pthread_mutex_lock(&eos_lock);
      continue;
    }
    else if (pBuffer->nFilledLen > 0)
    {
        if (!fbd_cnt)
        {
            gettimeofday(&t_start, NULL);
        }
      fbd_cnt++;
      DEBUG_PRINT("%s: fbd_cnt(%d) Buf(%p) Timestamp(%lld)",
        __FUNCTION__, fbd_cnt, pBuffer, pBuffer->nTimeStamp);
      canDisplay = 1;
      if (realtime_display)
      {
        if (pBuffer->nTimeStamp != (lastTimestamp + timestampInterval))
        {
            DEBUG_PRINT("Unexpected timestamp[%lld]! Expected[%lld]\n",
                pBuffer->nTimeStamp, lastTimestamp + timestampInterval);
        }
        lastTimestamp = pBuffer->nTimeStamp;
        gettimeofday(&t_avsync, NULL);
        if (!base_avsync.tv_sec && !base_avsync.tv_usec)
        {
          display_time = 0;
          base_avsync = t_avsync;
          base_timestamp = pBuffer->nTimeStamp;
          DEBUG_PRINT("base_avsync Sec(%lu) uSec(%lu) base_timestamp(%lld)",
              base_avsync.tv_sec, base_avsync.tv_usec, base_timestamp);
        }
        else
        {
          act_time = (t_avsync.tv_sec - base_avsync.tv_sec) * 1e6
                     + t_avsync.tv_usec - base_avsync.tv_usec;
          display_time = pBuffer->nTimeStamp - base_timestamp;
          DEBUG_PRINT("%s: act_time(%lu) display_time(%lu)",
              __FUNCTION__, act_time, display_time);
               //Frame rcvd on time
          if (((act_time + render_time) >= (display_time - lipsync) &&
               (act_time + render_time) <= (display_time + lipsync)) ||
               //Display late frame
               (contigous_drop_frame > 5))
              display_time = 0;
          else if ((act_time + render_time) < (display_time - lipsync))
              //Delaying early frame
              display_time -= (lipsync + act_time + render_time);
          else
          {
              //Dropping late frame
              canDisplay = 0;
              contigous_drop_frame++;
          }
        }
      }
      if (displayYuv && canDisplay)
      {
          if (display_time)
              usleep(display_time);
          ret = overlay_fb(pBuffer);
          if (ret != 0)
          {
            printf("\nERROR in overlay_fb, disabling display!");
            close_display();
            displayYuv = 0;
          }
          usleep(render_time);
          contigous_drop_frame = 0;
      }

      if (takeYuvLog)
      {
          bytes_written = fwrite((const char *)pBuffer->pBuffer,
                                  pBuffer->nFilledLen,1,outputBufferFile);
          if (bytes_written < 0) {
              DEBUG_PRINT("\nFillBufferDone: Failed to write to the file\n");
          }
          else {
              DEBUG_PRINT("\nFillBufferDone: Wrote %d YUV bytes to the file\n",
                            bytes_written);
          }
      }
      if (pBuffer->nFlags & OMX_BUFFERFLAG_EXTRADATA)
      {
        OMX_OTHER_EXTRADATATYPE *pExtra;
        DEBUG_PRINT(">> BUFFER WITH EXTRA DATA RCVD <<<");
        pExtra = (OMX_OTHER_EXTRADATATYPE *)
                 ((unsigned)(pBuffer->pBuffer + pBuffer->nOffset +
                  pBuffer->nFilledLen + 3)&(~3));
        while(pExtra &&
              (OMX_U8*)pExtra < (pBuffer->pBuffer + pBuffer->nAllocLen) &&
              pExtra->eType != OMX_ExtraDataNone )
        {
          DEBUG_PRINT("ExtraData : pBuf(%p) BufTS(%lld) Type(%x) DataSz(%u)",
               pBuffer, pBuffer->nTimeStamp, pExtra->eType, pExtra->nDataSize);
          switch (pExtra->eType)
          {
            case OMX_ExtraDataInterlaceFormat:
            {
              OMX_STREAMINTERLACEFORMAT *pInterlaceFormat = (OMX_STREAMINTERLACEFORMAT *)pExtra->data;
              DEBUG_PRINT("OMX_ExtraDataInterlaceFormat: Buf(%p) TSmp(%lld) IntPtr(%p) Fmt(%x)",
                pBuffer->pBuffer, pBuffer->nTimeStamp,
                pInterlaceFormat, pInterlaceFormat->nInterlaceFormats);
              break;
            }
            case OMX_ExtraDataFrameInfo:
            {
              OMX_QCOM_EXTRADATA_FRAMEINFO *frame_info = (OMX_QCOM_EXTRADATA_FRAMEINFO *)pExtra->data;
              DEBUG_PRINT("OMX_ExtraDataFrameInfo: Buf(%p) TSmp(%lld) PicType(%u) IntT(%u) ConMB(%u)",
                pBuffer->pBuffer, pBuffer->nTimeStamp, frame_info->ePicType,
                frame_info->interlaceType, frame_info->nConcealedMacroblocks);
              DEBUG_PRINT(" FrmRate(%u), AspRatioX(%u), AspRatioY(%u) ",
                frame_info->nFrameRate, frame_info->aspectRatio.aspectRatioX,
                frame_info->aspectRatio.aspectRatioY);
              DEBUG_PRINT("PANSCAN numWindows(%d)", frame_info->panScan.numWindows);
              for (int i = 0; i < frame_info->panScan.numWindows; i++)
              {
                DEBUG_PRINT("WINDOW Lft(%d) Tp(%d) Rgt(%d) Bttm(%d)",
                  frame_info->panScan.window[i].x,
                  frame_info->panScan.window[i].y,
                  frame_info->panScan.window[i].dx,
                  frame_info->panScan.window[i].dy);
              }
              break;
            }
            break;
            case OMX_ExtraDataConcealMB:
            {
              OMX_U8 data = 0, *data_ptr = (OMX_U8 *)pExtra->data;
              OMX_U32 concealMBnum = 0, bytes_cnt = 0;
              while (bytes_cnt < pExtra->nDataSize)
              {
                data = *data_ptr;
                while (data)
                {
                  concealMBnum += (data&0x01);
                  data >>= 1;
                }
                data_ptr++;
                bytes_cnt++;
              }
              DEBUG_PRINT("OMX_ExtraDataConcealMB: Buf(%p) TSmp(%lld) ConcealMB(%u)",
                pBuffer->pBuffer, pBuffer->nTimeStamp, concealMBnum);
            }
            break;
            default:
              DEBUG_PRINT_ERROR("Unknown Extrata!");
          }
          if (pExtra->nSize < (pBuffer->nAllocLen - (OMX_U32)pExtra))
            pExtra = (OMX_OTHER_EXTRADATATYPE *) (((OMX_U8 *) pExtra) + pExtra->nSize);
          else
          {
            DEBUG_PRINT_ERROR("ERROR: Extradata pointer overflow buffer(%p) extra(%p)",
              pBuffer, pExtra);
            pExtra = NULL;
          }
        }
      }
    }
    if(pBuffer->nFlags & QOMX_VIDEO_BUFFERFLAG_EOSEQ)
    {
        DEBUG_PRINT("\n");
        DEBUG_PRINT("***************************************************\n");
        DEBUG_PRINT("FillBufferDone: End Of Sequence Received\n");
        DEBUG_PRINT("***************************************************\n");
    }
    if(pBuffer->nFlags & OMX_BUFFERFLAG_DATACORRUPT)
    {
      DEBUG_PRINT("\n");
      DEBUG_PRINT("***************************************************\n");
      DEBUG_PRINT("FillBufferDone: OMX_BUFFERFLAG_DATACORRUPT Received\n");
      DEBUG_PRINT("***************************************************\n");
    }
    /********************************************************************/
    /* De-Initializing the open max and relasing the buffers and */
    /* closing the files.*/
    /********************************************************************/
    if (pBuffer->nFlags & OMX_BUFFERFLAG_EOS )
    {
      OMX_QCOM_FRAME_PACK_ARRANGEMENT framePackingArrangement;
      OMX_GetConfig(dec_handle,
                   (OMX_INDEXTYPE)OMX_QcomIndexConfigVideoFramePackingArrangement,
                    &framePackingArrangement);
      PrintFramePackArrangement(framePackingArrangement);

      gettimeofday(&t_end, NULL);
      total_time = ((float) ((t_end.tv_sec - t_start.tv_sec) * 1e6
                     + t_end.tv_usec - t_start.tv_usec))/ 1e6;
      //total frames is fbd_cnt - 1 since the start time is
      //recorded after the first frame is decoded.
      printf("\nAvg decoding frame rate=%f\n", (fbd_cnt - 1)/total_time);

      DEBUG_PRINT("***************************************************\n");
      DEBUG_PRINT("FillBufferDone: End Of Stream Reached\n");
      DEBUG_PRINT("***************************************************\n");
      pthread_mutex_lock(&eos_lock);
      bOutputEosReached = true;
      break;
    }

    pthread_mutex_lock(&enable_lock);
    if (flush_output_progress || sent_disabled)
    {
        pBuffer->nFilledLen = 0;
        pBuffer->nFlags &= ~OMX_BUFFERFLAG_EXTRADATA;
        pthread_mutex_lock(&fbd_lock);
        if ( pPrevBuff != NULL ) {
            push(fbd_queue, (void *)pPrevBuff);
            pPrevBuff = NULL;
        }
        if(push(fbd_queue, (void *)pBuffer) < 0)
        {
          DEBUG_PRINT_ERROR("Error in enqueueing fbd_data\n");
        }
        else
          sem_post(&fbd_sem);
        pthread_mutex_unlock(&fbd_lock);
    }
    else
    {
        if (!anti_flickering)
          pPrevBuff = pBuffer;
        if (pPrevBuff)
        {
          pthread_mutex_lock(&fbd_lock);
          pthread_mutex_lock(&eos_lock);
          if (!bOutputEosReached)
          {
              if ( OMX_FillThisBuffer(dec_handle, pPrevBuff) == OMX_ErrorNone ) {
                  free_op_buf_cnt--;
              }
          }
          pthread_mutex_unlock(&eos_lock);
          pthread_mutex_unlock(&fbd_lock);
        }
    }
    pthread_mutex_unlock(&enable_lock);
    if(cmd_data <= fbd_cnt)
    {
      sem_post(&seq_sem);
      printf("\n Posted seq_sem Frm(%d) Req(%d)", fbd_cnt, cmd_data);
      cmd_data = ~(unsigned)0;
    }
    pthread_mutex_lock(&eos_lock);
  }
  if(seq_enabled)
  {
      seq_enabled = 0;
      sem_post(&seq_sem);
      printf("\n Posted seq_sem in EOS \n");
  }
  pthread_cond_broadcast(&eos_cond);
  pthread_mutex_unlock(&eos_lock);
  return NULL;
}

OMX_ERRORTYPE EventHandler(OMX_IN OMX_HANDLETYPE hComponent,
                           OMX_IN OMX_PTR pAppData,
                           OMX_IN OMX_EVENTTYPE eEvent,
                           OMX_IN OMX_U32 nData1, OMX_IN OMX_U32 nData2,
                           OMX_IN OMX_PTR pEventData)
{
    DEBUG_PRINT("Function %s \n", __FUNCTION__);

    switch(eEvent) {
        case OMX_EventCmdComplete:
            DEBUG_PRINT("\n OMX_EventCmdComplete \n");
            if(OMX_CommandPortDisable == (OMX_COMMANDTYPE)nData1)
            {
                DEBUG_PRINT("*********************************************\n");
                DEBUG_PRINT("Recieved DISABLE Event Command Complete[%d]\n",nData2);
                DEBUG_PRINT("*********************************************\n");
            }
            else if(OMX_CommandPortEnable == (OMX_COMMANDTYPE)nData1)
            {
                DEBUG_PRINT("*********************************************\n");
                DEBUG_PRINT("Recieved ENABLE Event Command Complete[%d]\n",nData2);
                DEBUG_PRINT("*********************************************\n");
                if (currentStatus == PORT_SETTING_CHANGE_STATE)
                  currentStatus = GOOD_STATE;
                pthread_mutex_lock(&enable_lock);
                sent_disabled = 0;
                pthread_mutex_unlock(&enable_lock);
            }
            else if(OMX_CommandFlush == (OMX_COMMANDTYPE)nData1)
            {
                DEBUG_PRINT("*********************************************\n");
                DEBUG_PRINT("Received FLUSH Event Command Complete[%d]\n",nData2);
                DEBUG_PRINT("*********************************************\n");
                if (nData2 == 0)
                    flush_input_progress = 0;
                else if (nData2 == 1)
                    flush_output_progress = 0;
            }
            if (!flush_input_progress && !flush_output_progress)
                event_complete();
            break;

        case OMX_EventError:
            printf("*********************************************\n");
            printf("Received OMX_EventError Event Command !\n");
            printf("*********************************************\n");
            currentStatus = ERROR_STATE;
            if (OMX_ErrorInvalidState == (OMX_ERRORTYPE)nData1 ||
                OMX_ErrorHardware == (OMX_ERRORTYPE)nData1)
            {
              printf("Invalid State or hardware error \n");
              if(event_is_done == 0)
              {
                DEBUG_PRINT("Event error in the middle of Decode \n");
                pthread_mutex_lock(&eos_lock);
                bOutputEosReached = true;
                pthread_mutex_unlock(&eos_lock);
                if(seq_enabled)
                {
                    seq_enabled = 0;
                    sem_post(&seq_sem);
                    printf("\n Posted seq_sem in ERROR");
                }
              }
            }
            if (waitForPortSettingsChanged)
            {
	            waitForPortSettingsChanged = 0;
	            event_complete();
            }
            break;
        case OMX_EventPortSettingsChanged:
            DEBUG_PRINT("OMX_EventPortSettingsChanged port[%d]\n", nData1);
            currentStatus = PORT_SETTING_CHANGE_STATE;
            if (waitForPortSettingsChanged)
            {
	            waitForPortSettingsChanged = 0;
	            event_complete();
            }
            else
            {
                pthread_mutex_lock(&eos_lock);
                pthread_cond_broadcast(&eos_cond);
                pthread_mutex_unlock(&eos_lock);
            }
            break;

        case OMX_EventBufferFlag:
            DEBUG_PRINT("OMX_EventBufferFlag port[%d] flags[%x]\n", nData1, nData2);
            if (nData1 == 1 && (nData2 & OMX_BUFFERFLAG_EOS)) {
                pthread_mutex_lock(&eos_lock);
                bOutputEosReached = true;
                pthread_mutex_unlock(&eos_lock);
                if(seq_enabled)
                {
                    seq_enabled = 0;
                    sem_post(&seq_sem);
                    printf("\n Posted seq_sem in OMX_EventBufferFlag");
                }
            }
            else
            {
                DEBUG_PRINT_ERROR("OMX_EventBufferFlag Event not handled\n");
            }
            break;
        case OMX_EventIndexsettingChanged:
            DEBUG_PRINT("OMX_EventIndexSettingChanged Interlace mode[%x]\n", nData1);
            break;
        default:
            DEBUG_PRINT_ERROR("ERROR - Unknown Event \n");
            break;
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE EmptyBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
                              OMX_IN OMX_PTR pAppData,
                              OMX_IN OMX_BUFFERHEADERTYPE* pBuffer)
{
    int readBytes =0; int bufCnt=0;
    OMX_ERRORTYPE result;

    DEBUG_PRINT("Function %s cnt[%d]\n", __FUNCTION__, ebd_cnt);
    ebd_cnt++;


    if(bInputEosReached) {
        DEBUG_PRINT("*****EBD:Input EoS Reached************\n");
        return OMX_ErrorNone;
    }

    pthread_mutex_lock(&etb_lock);
    if(push(etb_queue, (void *) pBuffer) < 0)
    {
       DEBUG_PRINT_ERROR("Error in enqueue  ebd data\n");
       return OMX_ErrorUndefined;
    }
    pthread_mutex_unlock(&etb_lock);
    sem_post(&etb_sem);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE FillBufferDone(OMX_OUT OMX_HANDLETYPE hComponent,
                             OMX_OUT OMX_PTR pAppData,
                             OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer)
{
    DEBUG_PRINT("Inside %s callback_count[%d] \n", __FUNCTION__, fbd_cnt);

    /* Test app will assume there is a dynamic port setting
     * In case that there is no dynamic port setting, OMX will not call event cb,
     * instead OMX will send empty this buffer directly and we need to clear an event here
     */
    if(waitForPortSettingsChanged)
    {
      waitForPortSettingsChanged = 0;
      if(displayYuv)
        overlay_set();
      event_complete();
    }

    pthread_mutex_lock(&fbd_lock);
    free_op_buf_cnt++;
    if(push(fbd_queue, (void *)pBuffer) < 0)
    {
      pthread_mutex_unlock(&fbd_lock);
      DEBUG_PRINT_ERROR("Error in enqueueing fbd_data\n");
      return OMX_ErrorUndefined;
    }
    pthread_mutex_unlock(&fbd_lock);
    sem_post(&fbd_sem);

    return OMX_ErrorNone;
}

int main(int argc, char **argv)
{
    int i=0;
    int bufCnt=0;
    int num=0;
    int outputOption = 0;
    int test_option = 0;
    int pic_order = 0;
    OMX_ERRORTYPE result;
    sliceheight = height = 144;
    stride = width = 176;

    if (argc < 2)
    {
      printf("To use it: ./mm-vdec-omx-test <clip location> \n");
      printf("Command line argument is also available\n");
      return -1;
    }

    strlcpy(in_filename, argv[1], strlen(argv[1])+1);
    if(argc > 2)
    {
      codec_format_option = (codec_format)atoi(argv[2]);
      // file_type, out_op, tst_op, nal_sz, disp_win, rt_dis, (fps), color, pic_order
      int param[9] = {2, 1, 1, 0, 0, 0, 0xFF, 0xFF, 0xFF};
      int next_arg = 3, idx = 0;
      while (argc > next_arg && idx < 9)
      {
        if (strlen(argv[next_arg]) > 2)
        {
          strlcpy(seq_file_name, argv[next_arg],strlen(argv[next_arg]) + 1);
          next_arg = argc;
        }
        else
          param[idx++] = atoi(argv[next_arg++]);
      }
      idx = 0;
      file_type_option = (file_type)param[idx++];
      if (codec_format_option == CODEC_FORMAT_H264 && file_type_option == 3)
      {
        nalSize = param[idx++];
        if (nalSize != 2 && nalSize != 4)
        {
          printf("Error - Can't pass NAL length size = %d\n", nalSize);
          return -1;
        }
      }
      outputOption = param[idx++];
      test_option = param[idx++];
      displayWindow = param[idx++];
      if (displayWindow > 0)
        printf("Only entire display window supported! Ignoring value\n");
      realtime_display = param[idx++];
      if (realtime_display)
      {
        takeYuvLog = 0;
        if(param[idx] != 0xFF)
        {
          fps = param[idx++];
          timestampInterval = 1e6 / fps;
        }
      }
      color_fmt_type = (param[idx] != 0xFF)? param[idx++] : color_fmt_type;
      pic_order = (param[idx] != 0xFF)? param[idx++] : 0;
      printf("Executing DynPortReconfig QCIF 144 x 176 \n");
    }
    else
    {
      printf("Command line argument is available\n");
      printf("To use it: ./mm-vdec-omx-test <clip location> <codec_type> \n");
      printf("           <input_type: 1. per AU(.dat), 2. arbitrary, 3.per NAL/frame>\n");
      printf("           <output_type> <test_case> <size_nal if H264>\n\n\n");
      printf(" *********************************************\n");
      printf(" ENTER THE TEST CASE YOU WOULD LIKE TO EXECUTE\n");
      printf(" *********************************************\n");
      printf(" 1--> H264\n");
      printf(" 2--> MP4\n");
      printf(" 3--> H263\n");
      printf(" 4--> VC1\n");
      printf(" 5--> DivX\n");
      printf(" 6--> MPEG2\n");
      fflush(stdin);
      fgets(tempbuf,sizeof(tempbuf),stdin);
      sscanf(tempbuf,"%d",&codec_format_option);
      fflush(stdin);
      if (codec_format_option > CODEC_FORMAT_MAX)
      {
          printf(" Wrong test case...[%d] \n", codec_format_option);
          return -1;
      }
      printf(" *********************************************\n");
      printf(" ENTER THE TEST CASE YOU WOULD LIKE TO EXECUTE\n");
      printf(" *********************************************\n");
      printf(" 1--> PER ACCESS UNIT CLIP (.dat). Clip only available for H264 and Mpeg4\n");
      printf(" 2--> ARBITRARY BYTES (need .264/.264c/.m4v/.263/.rcv/.vc1/.m2v)\n");
      if (codec_format_option == CODEC_FORMAT_H264)
      {
        printf(" 3--> NAL LENGTH SIZE CLIP (.264c)\n");
      }
      else if ( (codec_format_option == CODEC_FORMAT_MP4) || (codec_format_option == CODEC_FORMAT_H263) )
      {
        printf(" 3--> MP4 VOP or H263 P0 SHORT HEADER START CODE CLIP (.m4v or .263)\n");
      }
      else if (codec_format_option == CODEC_FORMAT_VC1)
      {
        printf(" 3--> VC1 clip Simple/Main Profile (.rcv)\n");
        printf(" 4--> VC1 clip Advance Profile (.vc1)\n");
      }
      else if (codec_format_option == CODEC_FORMAT_DIVX)
      {
          printf(" 3--> DivX 4, 5, 6 clip (.cmp)\n");
#ifdef MAX_RES_1080P
          printf(" 4--> DivX 3.11 clip \n");
#endif
      }
      else if (codec_format_option == CODEC_FORMAT_MPEG2)
      {
        printf(" 3--> MPEG2 START CODE CLIP (.m2v)\n");
      }

      fflush(stdin);
      fgets(tempbuf,sizeof(tempbuf),stdin);
      sscanf(tempbuf,"%d",&file_type_option);
      fflush(stdin);
      if (codec_format_option == CODEC_FORMAT_H264 && file_type_option == 3)
      {
        printf(" Enter Nal length size [2 or 4] \n");
        fgets(tempbuf,sizeof(tempbuf),stdin);
        sscanf(tempbuf,"%d",&nalSize);
        if (nalSize != 2 && nalSize != 4)
        {
          printf("Error - Can't pass NAL length size = %d\n", nalSize);
          return -1;
        }
      }

      printf(" *********************************************\n");
      printf(" Output buffer option:\n");
      printf(" *********************************************\n");
      printf(" 0 --> No display and no YUV log\n");
      printf(" 1 --> Diplay YUV\n");
      printf(" 2 --> Take YUV log\n");
      printf(" 3 --> Display YUV and take YUV log\n");
      fflush(stdin);
      fgets(tempbuf,sizeof(tempbuf),stdin);
      sscanf(tempbuf,"%d",&outputOption);
      fflush(stdin);

      printf(" *********************************************\n");
      printf(" ENTER THE TEST CASE YOU WOULD LIKE TO EXECUTE\n");
      printf(" *********************************************\n");
      printf(" 1 --> Play the clip till the end\n");
      printf(" 2 --> Run compliance test. Do NOT expect any display for most option. \n");
      printf("       Please only see \"TEST SUCCESSFULL\" to indicate test pass\n");
      fflush(stdin);
      fgets(tempbuf,sizeof(tempbuf),stdin);
      sscanf(tempbuf,"%d",&test_option);
      fflush(stdin);

      if (outputOption == 1 || outputOption == 3)
      {
          printf(" *********************************************\n");
          printf(" ENTER THE PORTION OF DISPLAY TO USE\n");
          printf(" *********************************************\n");
          printf(" 0 --> Entire Screen\n");
          printf(" 1 --> 1/4 th of the screen starting from top left corner to middle \n");
          printf(" 2 --> 1/4 th of the screen starting from middle to top right corner \n");
          printf(" 3 --> 1/4 th of the screen starting from middle to bottom left \n");
          printf(" 4 --> 1/4 th of the screen starting from middle to bottom right \n");
          printf("       Please only see \"TEST SUCCESSFULL\" to indidcate test pass\n");
          fflush(stdin);
          fgets(tempbuf,sizeof(tempbuf),stdin);
          sscanf(tempbuf,"%d",&displayWindow);
          fflush(stdin);
          if(displayWindow > 0)
          {
              printf(" Curently display window 0 only supported; ignoring other values\n");
              displayWindow = 0;
          }
      }

      if (outputOption == 1 || outputOption == 3)
      {
          printf(" *********************************************\n");
          printf(" DO YOU WANT TEST APP TO RENDER in Real time \n");
          printf(" 0 --> NO\n 1 --> YES\n");
          printf(" Warning: For H264, it require one NAL per frame clip.\n");
          printf("          For Arbitrary bytes option, Real time display is not recommended\n");
          printf(" *********************************************\n");
          fflush(stdin);
          fgets(tempbuf,sizeof(tempbuf),stdin);
          sscanf(tempbuf,"%d",&realtime_display);
          fflush(stdin);
      }


      if (realtime_display)
      {
          printf(" *********************************************\n");
          printf(" ENTER THE CLIP FPS\n");
          printf(" Exception: Timestamp extracted from clips will be used.\n");
          printf(" *********************************************\n");
          fflush(stdin);
          fgets(tempbuf,sizeof(tempbuf),stdin);
          sscanf(tempbuf,"%d",&fps);
          fflush(stdin);
          timestampInterval = 1000000/fps;
      }

      printf(" *********************************************\n");
      printf(" ENTER THE COLOR FORMAT \n");
      printf(" 0 --> Semiplanar \n 1 --> Tile Mode\n");
      printf(" *********************************************\n");
      fflush(stdin);
      fgets(tempbuf,sizeof(tempbuf),stdin);
      sscanf(tempbuf,"%d",&color_fmt_type);
      fflush(stdin);

      printf(" *********************************************\n");
      printf(" Output picture order option: \n");
      printf(" *********************************************\n");
      printf(" 0 --> Display order\n 1 --> Decode order\n");
      fflush(stdin);
      fgets(tempbuf,sizeof(tempbuf),stdin);
      sscanf(tempbuf,"%d",&pic_order);
      fflush(stdin);
    }
    if (file_type_option >= FILE_TYPE_COMMON_CODEC_MAX)
    {
      switch (codec_format_option)
      {
        case CODEC_FORMAT_H264:
          file_type_option = (file_type)(FILE_TYPE_START_OF_H264_SPECIFIC + file_type_option - FILE_TYPE_COMMON_CODEC_MAX);
          break;
        case CODEC_FORMAT_DIVX:
          file_type_option = (file_type)(FILE_TYPE_START_OF_DIVX_SPECIFIC + file_type_option - FILE_TYPE_COMMON_CODEC_MAX);
          break;
        case CODEC_FORMAT_MP4:
        case CODEC_FORMAT_H263:
          file_type_option = (file_type)(FILE_TYPE_START_OF_MP4_SPECIFIC + file_type_option - FILE_TYPE_COMMON_CODEC_MAX);
          break;
        case CODEC_FORMAT_VC1:
          file_type_option = (file_type)(FILE_TYPE_START_OF_VC1_SPECIFIC + file_type_option - FILE_TYPE_COMMON_CODEC_MAX);
          break;
        case CODEC_FORMAT_MPEG2:
          file_type_option = (file_type)(FILE_TYPE_START_OF_MPEG2_SPECIFIC + file_type_option - FILE_TYPE_COMMON_CODEC_MAX);
          break;
        default:
          printf("Error: Unknown code %d\n", codec_format_option);
      }
    }

    CONFIG_VERSION_SIZE(picture_order);
    picture_order.eOutputPictureOrder = QOMX_VIDEO_DISPLAY_ORDER;
    if (pic_order == 1)
      picture_order.eOutputPictureOrder = QOMX_VIDEO_DECODE_ORDER;

    if (outputOption == 0)
    {
      displayYuv = 0;
      takeYuvLog = 0;
      realtime_display = 0;
    }
    else if (outputOption == 1)
    {
      displayYuv = 1;
    }
    else if (outputOption == 2)
    {
      takeYuvLog = 1;
      realtime_display = 0;
    }
    else if (outputOption == 3)
    {
      displayYuv = 1;
      takeYuvLog = !realtime_display;
    }
    else
    {
      printf("Wrong option. Assume you want to see the YUV display\n");
      displayYuv = 1;
    }

    if (test_option == 2)
    {
      printf(" *********************************************\n");
      printf(" ENTER THE COMPLIANCE TEST YOU WOULD LIKE TO EXECUTE\n");
      printf(" *********************************************\n");
      printf(" 1 --> Call Free Handle at the OMX_StateLoaded\n");
      printf(" 2 --> Call Free Handle at the OMX_StateIdle\n");
      printf(" 3 --> Call Free Handle at the OMX_StateExecuting\n");
      printf(" 4 --> Call Free Handle at the OMX_StatePause\n");
      fflush(stdin);
      fgets(tempbuf,sizeof(tempbuf),stdin);
      sscanf(tempbuf,"%d",&freeHandle_option);
      fflush(stdin);
    }
    else
    {
      freeHandle_option = (freeHandle_test)0;
    }

    printf("Input values: inputfilename[%s]\n", in_filename);
    printf("*******************************************************\n");
    pthread_cond_init(&cond, 0);
    pthread_cond_init(&eos_cond, 0);
    pthread_mutex_init(&eos_lock, 0);
    pthread_mutex_init(&lock, 0);
    pthread_mutex_init(&etb_lock, 0);
    pthread_mutex_init(&fbd_lock, 0);
    pthread_mutex_init(&enable_lock, 0);
    if (-1 == sem_init(&etb_sem, 0, 0))
    {
      printf("Error - sem_init failed %d\n", errno);
    }
    if (-1 == sem_init(&fbd_sem, 0, 0))
    {
      printf("Error - sem_init failed %d\n", errno);
    }
    if (-1 == sem_init(&seq_sem, 0, 0))
    {
      printf("Error - sem_init failed %d\n", errno);
    }
    if (-1 == sem_init(&in_flush_sem, 0, 0))
    {
      printf("Error - sem_init failed %d\n", errno);
    }
    if (-1 == sem_init(&out_flush_sem, 0, 0))
    {
      printf("Error - sem_init failed %d\n", errno);
    }
    etb_queue = alloc_queue();
    if (etb_queue == NULL)
    {
      printf("\n Error in Creating etb_queue\n");
      return -1;
    }

    fbd_queue = alloc_queue();
    if (fbd_queue == NULL)
    {
      printf("\n Error in Creating fbd_queue\n");
      free_queue(etb_queue);
      return -1;
    }

    if(0 != pthread_create(&fbd_thread_id, NULL, fbd_thread, NULL))
    {
      printf("\n Error in Creating fbd_thread \n");
      free_queue(etb_queue);
      free_queue(fbd_queue);
      return -1;
    }

    if (displayYuv)
    {
      if (open_display() != 0)
      {
        printf("\n Error opening display! Video won't be displayed...");
        displayYuv = 0;
      }
    }

    run_tests();
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&lock);
    pthread_mutex_destroy(&etb_lock);
    pthread_mutex_destroy(&fbd_lock);
    pthread_mutex_destroy(&enable_lock);
    pthread_cond_destroy(&eos_cond);
    pthread_mutex_destroy(&eos_lock);
    if (-1 == sem_destroy(&etb_sem))
    {
      DEBUG_PRINT_ERROR("Error - sem_destroy failed %d\n", errno);
    }
    if (-1 == sem_destroy(&fbd_sem))
    {
      DEBUG_PRINT_ERROR("Error - sem_destroy failed %d\n", errno);
    }
    if (-1 == sem_destroy(&seq_sem))
    {
      DEBUG_PRINT_ERROR("Error - sem_destroy failed %d\n", errno);
    }
    if (-1 == sem_destroy(&in_flush_sem))
    {
      DEBUG_PRINT_ERROR("Error - sem_destroy failed %d\n", errno);
    }
    if (-1 == sem_destroy(&out_flush_sem))
    {
      DEBUG_PRINT_ERROR("Error - sem_destroy failed %d\n", errno);
    }
    if (displayYuv)
      close_display();
    return 0;
}

int run_tests()
{
  int cmd_error = 0;
  DEBUG_PRINT("Inside %s\n", __FUNCTION__);
  waitForPortSettingsChanged = 1;

  if(file_type_option == FILE_TYPE_DAT_PER_AU) {
    Read_Buffer = Read_Buffer_From_DAT_File;
  }
  else if(file_type_option == FILE_TYPE_ARBITRARY_BYTES) {
    Read_Buffer = Read_Buffer_ArbitraryBytes;
  }
  else if(codec_format_option == CODEC_FORMAT_H264) {
    Read_Buffer = Read_Buffer_From_Size_Nal;
  }
  else if((codec_format_option == CODEC_FORMAT_H263) ||
          (codec_format_option == CODEC_FORMAT_MP4)) {
    Read_Buffer = Read_Buffer_From_Vop_Start_Code_File;
  }
  else if (codec_format_option == CODEC_FORMAT_MPEG2) {
    Read_Buffer = Read_Buffer_From_Mpeg2_Start_Code;
  }
  else if(file_type_option == FILE_TYPE_DIVX_4_5_6) {
    Read_Buffer = Read_Buffer_From_DivX_4_5_6_File;
  }
#ifdef MAX_RES_1080P
  else if(file_type_option == FILE_TYPE_DIVX_311) {
    Read_Buffer = Read_Buffer_From_DivX_311_File;
  }
#endif
  else if(file_type_option == FILE_TYPE_RCV) {
    Read_Buffer = Read_Buffer_From_RCV_File;
  }
  else if(file_type_option == FILE_TYPE_VC1) {
    Read_Buffer = Read_Buffer_From_VC1_File;
  }

  DEBUG_PRINT("file_type_option %d!\n", file_type_option);

  switch(file_type_option)
  {
    case FILE_TYPE_DAT_PER_AU:
    case FILE_TYPE_ARBITRARY_BYTES:
    case FILE_TYPE_264_NAL_SIZE_LENGTH:
    case FILE_TYPE_PICTURE_START_CODE:
    case FILE_TYPE_MPEG2_START_CODE:
    case FILE_TYPE_RCV:
    case FILE_TYPE_VC1:
    case FILE_TYPE_DIVX_4_5_6:
#ifdef MAX_RES_1080P
      case FILE_TYPE_DIVX_311:
#endif
      if(Init_Decoder()!= 0x00)
      {
        DEBUG_PRINT_ERROR("Error - Decoder Init failed\n");
        return -1;
      }
      if(Play_Decoder() != 0x00)
      {
        return -1;
      }
      break;
    default:
      DEBUG_PRINT_ERROR("Error - Invalid Entry...%d\n",file_type_option);
      break;
  }

  anti_flickering = true;
  if(strlen(seq_file_name))
  {
        seqFile = fopen (seq_file_name, "rb");
        if (seqFile == NULL)
        {
            DEBUG_PRINT_ERROR("Error - Seq file %s could NOT be opened\n",
                              seq_file_name);
            return -1;
        }
        else
        {
            DEBUG_PRINT("Seq file %s is opened \n", seq_file_name);
            seq_enabled = 1;
            anti_flickering = false;
        }
  }

  pthread_mutex_lock(&eos_lock);
  while (bOutputEosReached == false && cmd_error == 0)
  {
    if(seq_enabled)
    {
        pthread_mutex_unlock(&eos_lock);
        if(!get_next_command(seqFile))
            cmd_error = process_current_command(curr_seq_command);
        else
        {
            printf("\n Error in get_next_cmd or EOF");
            seq_enabled = 0;
        }
        pthread_mutex_lock(&eos_lock);
    }
    else
        pthread_cond_wait(&eos_cond, &eos_lock);

    if (currentStatus == PORT_SETTING_CHANGE_STATE)
    {
      pthread_mutex_unlock(&eos_lock);
      cmd_error = output_port_reconfig();
      pthread_mutex_lock(&eos_lock);
    }
  }
  pthread_mutex_unlock(&eos_lock);

  // Wait till EOS is reached...
  if(bOutputEosReached)
    do_freeHandle_and_clean_up(currentStatus == ERROR_STATE);
  return 0;
}

int Init_Decoder()
{
    DEBUG_PRINT("Inside %s \n", __FUNCTION__);
    OMX_ERRORTYPE omxresult;
    OMX_U32 total = 0;
    char vdecCompNames[50];
    typedef OMX_U8* OMX_U8_PTR;
    char *role ="video_decoder";

    static OMX_CALLBACKTYPE call_back = {&EventHandler, &EmptyBufferDone, &FillBufferDone};

    int i = 0;
    long bufCnt = 0;

    /* Init. the OpenMAX Core */
    DEBUG_PRINT("\nInitializing OpenMAX Core....\n");
    omxresult = OMX_Init();

    if(OMX_ErrorNone != omxresult) {
        DEBUG_PRINT_ERROR("\n Failed to Init OpenMAX core");
        return -1;
    }
    else {
        DEBUG_PRINT_ERROR("\nOpenMAX Core Init Done\n");
    }

    /* Query for video decoders*/
    OMX_GetComponentsOfRole(role, &total, 0);
    DEBUG_PRINT("\nTotal components of role=%s :%d", role, total);

    if(total)
    {
        /* Allocate memory for pointers to component name */
        OMX_U8** vidCompNames = (OMX_U8**)malloc((sizeof(OMX_U8*))*total);
        if (vidCompNames == NULL) {
            DEBUG_PRINT_ERROR("\nFailed to allocate vidCompNames\n");
            return -1;
        }

        for (i = 0; i < total; ++i) {
            vidCompNames[i] = (OMX_U8*)malloc(sizeof(OMX_U8)*OMX_MAX_STRINGNAME_SIZE);
            if (vidCompNames[i] == NULL) {
                DEBUG_PRINT_ERROR("\nFailed to allocate vidCompNames[%d]\n", i);
                return -1;
            }
        }
        OMX_GetComponentsOfRole(role, &total, vidCompNames);
        DEBUG_PRINT("\nComponents of Role:%s\n", role);
        for (i = 0; i < total; ++i) {
            DEBUG_PRINT("\nComponent Name [%s]\n",vidCompNames[i]);
            free(vidCompNames[i]);
        }
        free(vidCompNames);
    }
    else {
        DEBUG_PRINT_ERROR("No components found with Role:%s", role);
    }

    if (codec_format_option == CODEC_FORMAT_H264)
    {
      strlcpy(vdecCompNames, "OMX.qcom.video.decoder.avc", 27);
      //strlcpy(vdecCompNames, "OMX.SEC.qcom.video.decoder.avc", 31);
    }
    else if (codec_format_option == CODEC_FORMAT_MP4)
    {
      strlcpy(vdecCompNames, "OMX.qcom.video.decoder.mpeg4", 29);
    }
    else if (codec_format_option == CODEC_FORMAT_H263)
    {
      strlcpy(vdecCompNames, "OMX.qcom.video.decoder.h263", 28);
    }
    else if (codec_format_option == CODEC_FORMAT_VC1)
    {
      strlcpy(vdecCompNames, "OMX.qcom.video.decoder.vc1", 27);
    }
    else if (codec_format_option == CODEC_FORMAT_MPEG2)
    {
      strlcpy(vdecCompNames, "OMX.qcom.video.decoder.mpeg2", 29);
    }
    else if (file_type_option == FILE_TYPE_RCV)
    {
      strlcpy(vdecCompNames, "OMX.qcom.video.decoder.wmv", 27);
    }
    else if (file_type_option == FILE_TYPE_DIVX_4_5_6)
    {
      strlcpy(vdecCompNames, "OMX.qcom.video.decoder.divx", 28);
    }
#ifdef MAX_RES_1080P
    else if (file_type_option == FILE_TYPE_DIVX_311)
    {
      strlcpy(vdecCompNames, "OMX.qcom.video.decoder.divx311", 31);
    }
#endif
    else
    {
      DEBUG_PRINT_ERROR("Error: Unsupported codec %d\n", codec_format_option);
      return -1;
    }

    omxresult = OMX_GetHandle((OMX_HANDLETYPE*)(&dec_handle),
                              (OMX_STRING)vdecCompNames, NULL, &call_back);
    if (FAILED(omxresult)) {
        DEBUG_PRINT_ERROR("\nFailed to Load the component:%s\n", vdecCompNames);
        return -1;
    }
    else
    {
        DEBUG_PRINT("\nComponent %s is in LOADED state\n", vdecCompNames);
    }

    QOMX_VIDEO_QUERY_DECODER_INSTANCES decoder_instances;
    omxresult = OMX_GetConfig(dec_handle,
                 (OMX_INDEXTYPE)OMX_QcomIndexQueryNumberOfVideoDecInstance,
                              &decoder_instances);
    DEBUG_PRINT("\n Number of decoder instances %d",
                      decoder_instances.nNumOfInstances);

    /* Get the port information */
    CONFIG_VERSION_SIZE(portParam);
    omxresult = OMX_GetParameter(dec_handle, OMX_IndexParamVideoInit,
                                (OMX_PTR)&portParam);

    if(FAILED(omxresult)) {
        DEBUG_PRINT_ERROR("ERROR - Failed to get Port Param\n");
        return -1;
    }
    else
    {
        DEBUG_PRINT("portParam.nPorts:%d\n", portParam.nPorts);
        DEBUG_PRINT("portParam.nStartPortNumber:%d\n", portParam.nStartPortNumber);
    }

    /* Set the compression format on i/p port */
    if (codec_format_option == CODEC_FORMAT_H264)
    {
      portFmt.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
    }
    else if (codec_format_option == CODEC_FORMAT_MP4)
    {
      portFmt.format.video.eCompressionFormat = OMX_VIDEO_CodingMPEG4;
    }
    else if (codec_format_option == CODEC_FORMAT_H263)
    {
      portFmt.format.video.eCompressionFormat = OMX_VIDEO_CodingH263;
    }
    else if (codec_format_option == CODEC_FORMAT_VC1)
    {
      portFmt.format.video.eCompressionFormat = OMX_VIDEO_CodingWMV;
    }
    else if (codec_format_option == CODEC_FORMAT_DIVX)
    {
      portFmt.format.video.eCompressionFormat =
          (OMX_VIDEO_CODINGTYPE)QOMX_VIDEO_CodingDivx;
    }
    else if (codec_format_option == CODEC_FORMAT_MPEG2)
    {
      portFmt.format.video.eCompressionFormat = OMX_VIDEO_CodingMPEG2;
    }
    else
    {
      DEBUG_PRINT_ERROR("Error: Unsupported codec %d\n", codec_format_option);
    }


    return 0;
}

int Play_Decoder()
{
    OMX_VIDEO_PARAM_PORTFORMATTYPE videoportFmt = {0};
    int i, bufCnt, index = 0;
    int frameSize=0;
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE* pBuffer = NULL;
    DEBUG_PRINT("Inside %s \n", __FUNCTION__);

    /* open the i/p and o/p files based on the video file format passed */
    if(open_video_file()) {
        DEBUG_PRINT_ERROR("Error in opening video file\n");
        return -1;
    }

    OMX_QCOM_PARAM_PORTDEFINITIONTYPE inputPortFmt;
    memset(&inputPortFmt, 0, sizeof(OMX_QCOM_PARAM_PORTDEFINITIONTYPE));
    CONFIG_VERSION_SIZE(inputPortFmt);
    inputPortFmt.nPortIndex = 0;  // input port
    switch (file_type_option)
    {
      case FILE_TYPE_DAT_PER_AU:
      case FILE_TYPE_PICTURE_START_CODE:
      case FILE_TYPE_MPEG2_START_CODE:
      case FILE_TYPE_RCV:
      case FILE_TYPE_VC1:
#ifdef MAX_RES_1080P
      case FILE_TYPE_DIVX_311:
#endif
      {
        inputPortFmt.nFramePackingFormat = OMX_QCOM_FramePacking_OnlyOneCompleteFrame;
        break;
      }

      case FILE_TYPE_ARBITRARY_BYTES:
      case FILE_TYPE_264_NAL_SIZE_LENGTH:
      case FILE_TYPE_DIVX_4_5_6:
      {
        inputPortFmt.nFramePackingFormat = OMX_QCOM_FramePacking_Arbitrary;
        break;
      }

      default:
        inputPortFmt.nFramePackingFormat = OMX_QCOM_FramePacking_Unspecified;
    }
    OMX_SetParameter(dec_handle,(OMX_INDEXTYPE)OMX_QcomIndexPortDefn,
                     (OMX_PTR)&inputPortFmt);
#ifdef USE_EXTERN_PMEM_BUF
    OMX_QCOM_PARAM_PORTDEFINITIONTYPE outPortFmt;
    memset(&outPortFmt, 0, sizeof(OMX_QCOM_PARAM_PORTDEFINITIONTYPE));
    CONFIG_VERSION_SIZE(outPortFmt);
    outPortFmt.nPortIndex = 1;  // output port
    outPortFmt.nCacheAttr = OMX_QCOM_CacheAttrNone;
    outPortFmt.nMemRegion = OMX_QCOM_MemRegionSMI;
    OMX_SetParameter(dec_handle,(OMX_INDEXTYPE)OMX_QcomIndexPortDefn,
                     (OMX_PTR)&outPortFmt);

    OMX_QCOM_PLATFORMPRIVATE_EXTN outPltPvtExtn;
    memset(&outPltPvtExtn, 0, sizeof(OMX_QCOM_PLATFORMPRIVATE_EXTN));
    CONFIG_VERSION_SIZE(outPltPvtExtn);
    outPltPvtExtn.nPortIndex = 1;  // output port
    outPltPvtExtn.type = OMX_QCOM_PLATFORM_PRIVATE_PMEM;
    OMX_SetParameter(dec_handle,(OMX_INDEXTYPE)OMX_QcomIndexPlatformPvt,
                     (OMX_PTR)&outPltPvtExtn);
    use_external_pmem_buf = OMX_TRUE;
#endif
    QOMX_ENABLETYPE extra_data;
    extra_data.bEnable = OMX_TRUE;
#if 0
    OMX_SetParameter(dec_handle,(OMX_INDEXTYPE)OMX_QcomIndexParamInterlaceExtraData,
                     (OMX_PTR)&extra_data);
#endif
#if 0
    OMX_SetParameter(dec_handle,(OMX_INDEXTYPE)OMX_QcomIndexParamConcealMBMapExtraData,
                     (OMX_PTR)&extra_data);
#endif
#if 1
    OMX_SetParameter(dec_handle,(OMX_INDEXTYPE)OMX_QcomIndexParamFrameInfoExtraData,
                     (OMX_PTR)&extra_data);
#endif
#ifdef TEST_TS_FROM_SEI
    OMX_SetParameter(dec_handle,(OMX_INDEXTYPE)OMX_QcomIndexParamH264TimeInfo,
                     (OMX_PTR)&extra_data);
#endif
#if 0
    extra_data.bEnable = OMX_FALSE;
    OMX_SetParameter(dec_handle,(OMX_INDEXTYPE)OMX_QcomIndexParamConcealMBMapExtraData,
                     (OMX_PTR)&extra_data);
#endif
    /* Query the decoder outport's min buf requirements */
    CONFIG_VERSION_SIZE(portFmt);

    /* Port for which the Client needs to obtain info */
    portFmt.nPortIndex = portParam.nStartPortNumber;

    OMX_GetParameter(dec_handle,OMX_IndexParamPortDefinition,&portFmt);
    DEBUG_PRINT("\nDec: Min Buffer Count %d\n", portFmt.nBufferCountMin);
    DEBUG_PRINT("\nDec: Buffer Size %d\n", portFmt.nBufferSize);

    if(OMX_DirInput != portFmt.eDir) {
        printf ("\nDec: Expect Input Port\n");
        return -1;
    }
#ifdef MAX_RES_1080P
    if( (codec_format_option == CODEC_FORMAT_DIVX) &&
        (file_type_option == FILE_TYPE_DIVX_311) ) {

            int off;

            if ( read(inputBufferFileFd, &width, 4 ) == -1 ) {
                DEBUG_PRINT_ERROR("\nFailed to read width for divx\n");
                return  -1;
            }

            DEBUG_PRINT("\nWidth for DIVX = %d\n", width);

            if ( read(inputBufferFileFd, &height, 4 ) == -1 ) {
                DEBUG_PRINT_ERROR("\nFailed to read height for divx\n");
                return  -1;
            }

            DEBUG_PRINT("\nHeight for DIVX = %u\n", height);
            sliceheight = height;
            stride = width;
    }
#endif

    bufCnt = 0;
    portFmt.format.video.nFrameHeight = height;
    portFmt.format.video.nFrameWidth  = width;
    portFmt.format.video.xFramerate = fps;
    OMX_SetParameter(dec_handle,OMX_IndexParamPortDefinition, (OMX_PTR)&portFmt);
    OMX_GetParameter(dec_handle,OMX_IndexParamPortDefinition, &portFmt);
    DEBUG_PRINT("\nDec: New Min Buffer Count %d", portFmt.nBufferCountMin);
    CONFIG_VERSION_SIZE(videoportFmt);
#ifdef MAX_RES_720P
    if(color_fmt_type == 0)
    {
        color_fmt = OMX_COLOR_FormatYUV420SemiPlanar;
    }
    else
    {
        color_fmt = (OMX_COLOR_FORMATTYPE)
           QOMX_COLOR_FormatYUV420PackedSemiPlanar64x32Tile2m8ka;
    }
#elif _COPPER_
        color_fmt = OMX_COLOR_FormatYUV420SemiPlanar;
#else
       color_fmt = (OMX_COLOR_FORMATTYPE)
           QOMX_COLOR_FormatYUV420PackedSemiPlanar64x32Tile2m8ka;
#endif

    while (ret == OMX_ErrorNone)
    {
        videoportFmt.nPortIndex = 1;
        videoportFmt.nIndex = index;
        ret = OMX_GetParameter(dec_handle, OMX_IndexParamVideoPortFormat,
          (OMX_PTR)&videoportFmt);

        if((ret == OMX_ErrorNone) && (videoportFmt.eColorFormat ==
           color_fmt))
        {
            DEBUG_PRINT("\n Format[%u] supported by OMX Decoder", color_fmt);
            break;
        }
        index++;
    }

    if(ret == OMX_ErrorNone)
    {
        if(OMX_SetParameter(dec_handle, OMX_IndexParamVideoPortFormat,
            (OMX_PTR)&videoportFmt) != OMX_ErrorNone)
        {
            DEBUG_PRINT_ERROR("\n Setting Tile format failed");
            return -1;
        }
    }
    else
    {
        DEBUG_PRINT_ERROR("\n Error in retrieving supported color formats");
        return -1;
    }
    picture_order.nPortIndex = 1;
    DEBUG_PRINT("\nSet picture order\n");
    if(OMX_SetParameter(dec_handle,
	   (OMX_INDEXTYPE)OMX_QcomIndexParamVideoDecoderPictureOrder,
       (OMX_PTR)&picture_order) != OMX_ErrorNone)
    {
        printf("\n ERROR: Setting picture order!");
        return -1;
    }
    DEBUG_PRINT("\nVideo format: W x H (%d x %d)",
      portFmt.format.video.nFrameWidth,
      portFmt.format.video.nFrameHeight);
    if(codec_format_option == CODEC_FORMAT_H264)
    {
        OMX_VIDEO_CONFIG_NALSIZE naluSize;
        naluSize.nNaluBytes = nalSize;
        DEBUG_PRINT("\n Nal length is %d index %d",nalSize,OMX_IndexConfigVideoNalSize);
        OMX_SetConfig(dec_handle,OMX_IndexConfigVideoNalSize,(OMX_PTR)&naluSize);
        DEBUG_PRINT("SETTING THE NAL SIZE to %d\n",naluSize.nNaluBytes);
    }
    DEBUG_PRINT("\nOMX_SendCommand Decoder -> IDLE\n");
    OMX_SendCommand(dec_handle, OMX_CommandStateSet, OMX_StateIdle,0);

    input_buf_cnt = portFmt.nBufferCountActual;
    DEBUG_PRINT("Transition to Idle State succesful...\n");

#if ALLOCATE_BUFFER
       // Allocate buffer on decoder's i/p port
       error = Allocate_Buffer(dec_handle, &pInputBufHdrs, portFmt.nPortIndex,
                               portFmt.nBufferCountActual, portFmt.nBufferSize);
       if (error != OMX_ErrorNone) {
           DEBUG_PRINT_ERROR("Error - OMX_AllocateBuffer Input buffer error\n");
           return -1;
       }
       else {
           DEBUG_PRINT("\nOMX_AllocateBuffer Input buffer success\n");
       }
#else
       // Use buffer on decoder's i/p port
          input_use_buffer = true;
          DEBUG_PRINT_ERROR("\n before OMX_UseBuffer %p", &pInputBufHdrs);
          error =  use_input_buffer(dec_handle,
                             &pInputBufHdrs,
                              portFmt.nPortIndex,
                              portFmt.nBufferSize,
                              portFmt.nBufferCountActual);
          if (error != OMX_ErrorNone) {
             DEBUG_PRINT_ERROR("ERROR - OMX_UseBuffer Input buffer failed");
             return -1;
          }
          else {
             DEBUG_PRINT("OMX_UseBuffer Input buffer success\n");
          }
#endif
       portFmt.nPortIndex = portParam.nStartPortNumber+1;
       // Port for which the Client needs to obtain info

    OMX_GetParameter(dec_handle,OMX_IndexParamPortDefinition,&portFmt);
    DEBUG_PRINT("nMin Buffer Count=%d", portFmt.nBufferCountMin);
    DEBUG_PRINT("nBuffer Size=%d", portFmt.nBufferSize);
    if(OMX_DirOutput != portFmt.eDir) {
        DEBUG_PRINT_ERROR("Error - Expect Output Port\n");
        return -1;
    }

#ifndef USE_EGL_IMAGE_TEST_APP
    if (use_external_pmem_buf)
    {
        DEBUG_PRINT_ERROR("\n Use External pmem buf: OMX_UseBuffer %p", &pInputBufHdrs);
        error =  use_output_buffer_multiple_fd(dec_handle,
                                               &pOutYUVBufHdrs,
                                               portFmt.nPortIndex,
                                               portFmt.nBufferSize,
                                               portFmt.nBufferCountActual);
    }
    else
    {
        /* Allocate buffer on decoder's o/p port */
        error = Allocate_Buffer(dec_handle, &pOutYUVBufHdrs, portFmt.nPortIndex,
                                portFmt.nBufferCountActual, portFmt.nBufferSize);
    }
    free_op_buf_cnt = portFmt.nBufferCountActual;
    if (error != OMX_ErrorNone) {
        DEBUG_PRINT_ERROR("Error - OMX_AllocateBuffer Output buffer error\n");
        return -1;
    }
    else
    {
        DEBUG_PRINT("OMX_AllocateBuffer Output buffer success\n");
    }
#else
    DEBUG_PRINT_ERROR("\n before OMX_UseBuffer %p", &pInputBufHdrs);
    error =  use_output_buffer(dec_handle,
                       &pOutYUVBufHdrs,
                        portFmt.nPortIndex,
                        portFmt.nBufferSize,
                        portFmt.nBufferCountActual);
    free_op_buf_cnt = portFmt.nBufferCountActual;
    if (error != OMX_ErrorNone) {
       DEBUG_PRINT_ERROR("ERROR - OMX_UseBuffer Input buffer failed");
       return -1;
    }
    else {
       DEBUG_PRINT("OMX_UseBuffer Input buffer success\n");
    }
#endif
    wait_for_event();
    if (currentStatus == ERROR_STATE)
    {
      do_freeHandle_and_clean_up(true);
      return -1;
    }

    if (freeHandle_option == FREE_HANDLE_AT_IDLE)
    {
      OMX_STATETYPE state = OMX_StateInvalid;
      OMX_GetState(dec_handle, &state);
      if (state == OMX_StateIdle)
      {
        DEBUG_PRINT("Decoder is in OMX_StateIdle and trying to call OMX_FreeHandle \n");
        do_freeHandle_and_clean_up(false);
      }
      else
      {
        DEBUG_PRINT_ERROR("Error - Decoder is in state %d and trying to call OMX_FreeHandle \n", state);
        do_freeHandle_and_clean_up(true);
      }
      return -1;
    }


    DEBUG_PRINT("OMX_SendCommand Decoder -> Executing\n");
    OMX_SendCommand(dec_handle, OMX_CommandStateSet, OMX_StateExecuting,0);
    wait_for_event();
    if (currentStatus == ERROR_STATE)
    {
      do_freeHandle_and_clean_up(true);
      return -1;
    }
    if (pOutYUVBufHdrs == NULL)
    {
        DEBUG_PRINT_ERROR("Error - pOutYUVBufHdrs is NULL\n");
        return -1;
    }
    for(bufCnt=0; bufCnt < portFmt.nBufferCountActual; ++bufCnt) {
        DEBUG_PRINT("OMX_FillThisBuffer on output buf no.%d\n",bufCnt);
        if (pOutYUVBufHdrs[bufCnt] == NULL)
        {
            DEBUG_PRINT_ERROR("Error - pOutYUVBufHdrs[%d] is NULL\n", bufCnt);
            return -1;
        }
        pOutYUVBufHdrs[bufCnt]->nOutputPortIndex = 1;
        pOutYUVBufHdrs[bufCnt]->nFlags &= ~OMX_BUFFERFLAG_EOS;
        ret = OMX_FillThisBuffer(dec_handle, pOutYUVBufHdrs[bufCnt]);
        if (OMX_ErrorNone != ret)
            DEBUG_PRINT_ERROR("Error - OMX_FillThisBuffer failed with result %d\n", ret);
        else
        {
            DEBUG_PRINT("OMX_FillThisBuffer success!\n");
            free_op_buf_cnt--;
        }
    }

    used_ip_buf_cnt = input_buf_cnt;

    rcv_v1 = 0;

    //QPERF_START(client_decode);
    if (codec_format_option == CODEC_FORMAT_VC1)
    {
      pInputBufHdrs[0]->nOffset = 0;
      if(file_type_option == FILE_TYPE_RCV)
      {
      frameSize = Read_Buffer_From_RCV_File_Seq_Layer(pInputBufHdrs[0]);
      pInputBufHdrs[0]->nFilledLen = frameSize;
          DEBUG_PRINT("After Read_Buffer_From_RCV_File_Seq_Layer, "
              "frameSize %d\n", frameSize);
      }
      else if(file_type_option == FILE_TYPE_VC1)
      {
          bHdrflag = 1;
          pInputBufHdrs[0]->nFilledLen = Read_Buffer(pInputBufHdrs[0]);
          bHdrflag = 0;
          DEBUG_PRINT_ERROR("After 1st Read_Buffer for VC1, "
              "pInputBufHdrs[0]->nFilledLen %d\n", pInputBufHdrs[0]->nFilledLen);
      }
      else
      {
          pInputBufHdrs[0]->nFilledLen = Read_Buffer(pInputBufHdrs[0]);
          DEBUG_PRINT("After Read_Buffer pInputBufHdrs[0]->nFilledLen %d\n",
              pInputBufHdrs[0]->nFilledLen);
      }

      pInputBufHdrs[0]->nInputPortIndex = 0;
      pInputBufHdrs[0]->nOffset = 0;
      pInputBufHdrs[0]->nFlags = 0;

      ret = OMX_EmptyThisBuffer(dec_handle, pInputBufHdrs[0]);
      if (ret != OMX_ErrorNone)
      {
          DEBUG_PRINT_ERROR("ERROR - OMX_EmptyThisBuffer failed with result %d\n", ret);
          do_freeHandle_and_clean_up(true);
          return -1;
      }
      else
      {
          etb_count++;
          DEBUG_PRINT("OMX_EmptyThisBuffer success!\n");
      }
      i = 1;
    }
    else
    {
      i = 0;
    }

    for (i; i < used_ip_buf_cnt;i++) {
      pInputBufHdrs[i]->nInputPortIndex = 0;
      pInputBufHdrs[i]->nOffset = 0;
      if((frameSize = Read_Buffer(pInputBufHdrs[i])) <= 0 ){
        DEBUG_PRINT("NO FRAME READ\n");
        pInputBufHdrs[i]->nFilledLen = frameSize;
        pInputBufHdrs[i]->nInputPortIndex = 0;
        pInputBufHdrs[i]->nFlags |= OMX_BUFFERFLAG_EOS;;
        bInputEosReached = true;

        OMX_EmptyThisBuffer(dec_handle, pInputBufHdrs[i]);
        etb_count++;
        DEBUG_PRINT("File is small::Either EOS or Some Error while reading file\n");
        break;
      }
      pInputBufHdrs[i]->nFilledLen = frameSize;
      pInputBufHdrs[i]->nInputPortIndex = 0;
      pInputBufHdrs[i]->nFlags = 0;
//pBufHdr[bufCnt]->pAppPrivate = this;
      DEBUG_PRINT("%s: Timestamp sent(%lld)", __FUNCTION__, pInputBufHdrs[i]->nTimeStamp);
      ret = OMX_EmptyThisBuffer(dec_handle, pInputBufHdrs[i]);
      if (OMX_ErrorNone != ret) {
          DEBUG_PRINT_ERROR("ERROR - OMX_EmptyThisBuffer failed with result %d\n", ret);
          do_freeHandle_and_clean_up(true);
          return -1;
      }
      else {
          DEBUG_PRINT("OMX_EmptyThisBuffer success!\n");
          etb_count++;
      }
    }

    if(0 != pthread_create(&ebd_thread_id, NULL, ebd_thread, NULL))
    {
      printf("\n Error in Creating fbd_thread \n");
      free_queue(etb_queue);
      free_queue(fbd_queue);
      return -1;
    }

    // wait for event port settings changed event
    DEBUG_PRINT("wait_for_event: dyn reconfig");
    wait_for_event();
    DEBUG_PRINT("wait_for_event: dyn reconfig rcvd, currentStatus %d\n",
                  currentStatus);
    if (currentStatus == ERROR_STATE)
    {
      printf("Error - ERROR_STATE\n");
      do_freeHandle_and_clean_up(true);
      return -1;
    }
    else if (currentStatus == PORT_SETTING_CHANGE_STATE)
    {
      if (output_port_reconfig() != 0)
        return -1;
    }

    if (freeHandle_option == FREE_HANDLE_AT_EXECUTING)
    {
      OMX_STATETYPE state = OMX_StateInvalid;
      OMX_GetState(dec_handle, &state);
      if (state == OMX_StateExecuting)
      {
        DEBUG_PRINT("Decoder is in OMX_StateExecuting and trying to call OMX_FreeHandle \n");
        do_freeHandle_and_clean_up(false);
      }
      else
      {
        DEBUG_PRINT_ERROR("Error - Decoder is in state %d and trying to call OMX_FreeHandle \n", state);
        do_freeHandle_and_clean_up(true);
      }
      return -1;
    }
    else if (freeHandle_option == FREE_HANDLE_AT_PAUSE)
    {
      OMX_SendCommand(dec_handle, OMX_CommandStateSet, OMX_StatePause,0);
      wait_for_event();

      OMX_STATETYPE state = OMX_StateInvalid;
      OMX_GetState(dec_handle, &state);
      if (state == OMX_StatePause)
      {
        DEBUG_PRINT("Decoder is in OMX_StatePause and trying to call OMX_FreeHandle \n");
        do_freeHandle_and_clean_up(false);
      }
      else
      {
        DEBUG_PRINT_ERROR("Error - Decoder is in state %d and trying to call OMX_FreeHandle \n", state);
        do_freeHandle_and_clean_up(true);
      }
      return -1;
    }

    return 0;
}

static OMX_ERRORTYPE Allocate_Buffer ( OMX_COMPONENTTYPE *dec_handle,
                                       OMX_BUFFERHEADERTYPE  ***pBufHdrs,
                                       OMX_U32 nPortIndex,
                                       long bufCntMin, long bufSize)
{
    DEBUG_PRINT("Inside %s \n", __FUNCTION__);
    OMX_ERRORTYPE error=OMX_ErrorNone;
    long bufCnt=0;

    DEBUG_PRINT("pBufHdrs = %x,bufCntMin = %d\n", pBufHdrs, bufCntMin);
    *pBufHdrs= (OMX_BUFFERHEADERTYPE **)
                   malloc(sizeof(OMX_BUFFERHEADERTYPE)*bufCntMin);

    for(bufCnt=0; bufCnt < bufCntMin; ++bufCnt) {
        DEBUG_PRINT("OMX_AllocateBuffer No %d \n", bufCnt);
        error = OMX_AllocateBuffer(dec_handle, &((*pBufHdrs)[bufCnt]),
                                   nPortIndex, NULL, bufSize);
    }

    return error;
}

static OMX_ERRORTYPE use_input_buffer ( OMX_COMPONENTTYPE *dec_handle,
                                  OMX_BUFFERHEADERTYPE  ***pBufHdrs,
                                  OMX_U32 nPortIndex,
                                  OMX_U32 bufSize,
                                  long bufCntMin)
{
    DEBUG_PRINT("Inside %s \n", __FUNCTION__);
    OMX_ERRORTYPE error=OMX_ErrorNone;
    long bufCnt=0;
    OMX_U8* pvirt = NULL;

    *pBufHdrs= (OMX_BUFFERHEADERTYPE **)
                   malloc(sizeof(OMX_BUFFERHEADERTYPE)* bufCntMin);
    if(*pBufHdrs == NULL){
        DEBUG_PRINT_ERROR("\n m_inp_heap_ptr Allocation failed ");
        return OMX_ErrorInsufficientResources;
     }

    for(bufCnt=0; bufCnt < bufCntMin; ++bufCnt) {
        // allocate input buffers
      DEBUG_PRINT("OMX_UseBuffer No %d %d \n", bufCnt, bufSize);
      pvirt = (OMX_U8*) malloc (bufSize);
      if(pvirt == NULL){
        DEBUG_PRINT_ERROR("\n pvirt Allocation failed ");
        return OMX_ErrorInsufficientResources;
     }
      error = OMX_UseBuffer(dec_handle, &((*pBufHdrs)[bufCnt]),
                              nPortIndex, NULL, bufSize, pvirt);
       }
    return error;
}

static OMX_ERRORTYPE use_output_buffer ( OMX_COMPONENTTYPE *dec_handle,
                                  OMX_BUFFERHEADERTYPE  ***pBufHdrs,
                                  OMX_U32 nPortIndex,
                                  OMX_U32 bufSize,
                                  long bufCntMin)
{
    DEBUG_PRINT("Inside %s \n", __FUNCTION__);
    OMX_ERRORTYPE error=OMX_ErrorNone;
    long bufCnt=0;
    OMX_U8* pvirt = NULL;

    *pBufHdrs= (OMX_BUFFERHEADERTYPE **)
                   malloc(sizeof(OMX_BUFFERHEADERTYPE)* bufCntMin);
    if(*pBufHdrs == NULL){
        DEBUG_PRINT_ERROR("\n m_inp_heap_ptr Allocation failed ");
        return OMX_ErrorInsufficientResources;
     }
    output_use_buffer = true;
    p_eglHeaders = (struct temp_egl **)
                    malloc(sizeof(struct temp_egl *)* bufCntMin);
    if (!p_eglHeaders){
        DEBUG_PRINT_ERROR("\n EGL allocation failed");
        return OMX_ErrorInsufficientResources;
    }

    for(bufCnt=0; bufCnt < bufCntMin; ++bufCnt) {
        // allocate input buffers
      DEBUG_PRINT("OMX_UseBuffer No %d %d \n", bufCnt, bufSize);
      p_eglHeaders[bufCnt] = (struct temp_egl*)
                         malloc(sizeof(struct temp_egl));
      if(!p_eglHeaders[bufCnt]) {
          DEBUG_PRINT_ERROR("\n EGL allocation failed");
          return OMX_ErrorInsufficientResources;
      }
      p_eglHeaders[bufCnt]->pmem_fd = open(PMEM_DEVICE,O_RDWR);
      p_eglHeaders[bufCnt]->offset = 0;
      if(p_eglHeaders[bufCnt]->pmem_fd < 0) {
          DEBUG_PRINT_ERROR("\n open failed %s",PMEM_DEVICE);
          return OMX_ErrorInsufficientResources;
      }

#ifndef USE_ION
      /* TBD - this commenting is dangerous */
      align_pmem_buffers(p_eglHeaders[bufCnt]->pmem_fd, bufSize,
                                  8192);
#endif
      DEBUG_PRINT_ERROR("\n allocation size %d pmem fd %d",bufSize,p_eglHeaders[bufCnt]->pmem_fd);
      pvirt = (unsigned char *)mmap(NULL,bufSize,PROT_READ|PROT_WRITE,
                      MAP_SHARED,p_eglHeaders[bufCnt]->pmem_fd,0);
      DEBUG_PRINT_ERROR("\n Virtaul Address %p Size %d",pvirt,bufSize);
      if (pvirt == MAP_FAILED) {
        DEBUG_PRINT_ERROR("\n mmap failed for buffers");
        return OMX_ErrorInsufficientResources;
      }
        use_buf_virt_addr[bufCnt] = (unsigned)pvirt;
        error = OMX_UseEGLImage(dec_handle, &((*pBufHdrs)[bufCnt]),
                              nPortIndex, pvirt,(void *)p_eglHeaders[bufCnt]);
       }
    return error;
}

static OMX_ERRORTYPE use_output_buffer_multiple_fd ( OMX_COMPONENTTYPE *dec_handle,
                                  OMX_BUFFERHEADERTYPE  ***pBufHdrs,
                                  OMX_U32 nPortIndex,
                                  OMX_U32 bufSize,
                                  long bufCntMin)
{
    DEBUG_PRINT("Inside %s \n", __FUNCTION__);
    OMX_ERRORTYPE error=OMX_ErrorNone;
    long bufCnt=0;
    OMX_U8* pvirt = NULL;

    *pBufHdrs= (OMX_BUFFERHEADERTYPE **)
                   malloc(sizeof(OMX_BUFFERHEADERTYPE)* bufCntMin);
    if(*pBufHdrs == NULL){
        DEBUG_PRINT_ERROR("\n m_inp_heap_ptr Allocation failed ");
        return OMX_ErrorInsufficientResources;
     }
    pPlatformList = (OMX_QCOM_PLATFORM_PRIVATE_LIST *)
        malloc(sizeof(OMX_QCOM_PLATFORM_PRIVATE_LIST)* bufCntMin);

    if(pPlatformList == NULL){
        DEBUG_PRINT_ERROR("\n pPlatformList Allocation failed ");
        return OMX_ErrorInsufficientResources;
     }

    pPlatformEntry = (OMX_QCOM_PLATFORM_PRIVATE_ENTRY *)
        malloc(sizeof(OMX_QCOM_PLATFORM_PRIVATE_ENTRY)* bufCntMin);

    if(pPlatformEntry == NULL){
        DEBUG_PRINT_ERROR("\n pPlatformEntry Allocation failed ");
        return OMX_ErrorInsufficientResources;
     }

    pPMEMInfo = (OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO *)
        malloc(sizeof(OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO)* bufCntMin);

    if(pPMEMInfo == NULL){
        DEBUG_PRINT_ERROR("\n pPMEMInfo Allocation failed ");
        return OMX_ErrorInsufficientResources;
     }

    //output_use_buffer = true;
    for(bufCnt=0; bufCnt < bufCntMin; ++bufCnt) {
        // allocate input buffers
      DEBUG_PRINT("OMX_UseBuffer_multiple_fd No %d %d \n", bufCnt, bufSize);

      pPlatformEntry[bufCnt].type       = OMX_QCOM_PLATFORM_PRIVATE_PMEM;
      pPlatformEntry[bufCnt].entry      = &pPMEMInfo[bufCnt];
      // Initialize the Platform List
      pPlatformList[bufCnt].nEntries    = 1;
      pPlatformList[bufCnt].entryList   = &pPlatformEntry[bufCnt];
      pPMEMInfo[bufCnt].offset          =  0;
      pPMEMInfo[bufCnt].pmem_fd = open(PMEM_DEVICE,O_RDWR);;
      if(pPMEMInfo[bufCnt].pmem_fd < 0) {
          DEBUG_PRINT_ERROR("\n open failed %s",PMEM_DEVICE);
          return OMX_ErrorInsufficientResources;
      }
#ifndef USE_ION
      /* TBD - this commenting is dangerous */
      align_pmem_buffers(pPMEMInfo[bufCnt].pmem_fd, bufSize,
                                  8192);
#endif
      DEBUG_PRINT("\n allocation size %d pmem fd 0x%x",bufSize,pPMEMInfo[bufCnt].pmem_fd);
      pvirt = (unsigned char *)mmap(NULL,bufSize,PROT_READ|PROT_WRITE,
                      MAP_SHARED,pPMEMInfo[bufCnt].pmem_fd,0);
      getFreePmem();
      DEBUG_PRINT("\n Virtaul Address %p Size %d pmem_fd=0x%x",pvirt,bufSize,pPMEMInfo[bufCnt].pmem_fd);
      if (pvirt == MAP_FAILED) {
        DEBUG_PRINT_ERROR("\n mmap failed for buffers");
        return OMX_ErrorInsufficientResources;
      }
      use_buf_virt_addr[bufCnt] = (unsigned)pvirt;
      error = OMX_UseBuffer(dec_handle, &((*pBufHdrs)[bufCnt]),
                            nPortIndex, &pPlatformList[bufCnt], bufSize, pvirt);
    }
    return error;
}
static void do_freeHandle_and_clean_up(bool isDueToError)
{
    int bufCnt = 0;
    OMX_STATETYPE state = OMX_StateInvalid;
    OMX_GetState(dec_handle, &state);
    if (state == OMX_StateExecuting || state == OMX_StatePause)
    {
      DEBUG_PRINT("Requesting transition to Idle");
      OMX_SendCommand(dec_handle, OMX_CommandStateSet, OMX_StateIdle, 0);
      wait_for_event();
    }
    OMX_GetState(dec_handle, &state);
    if (state == OMX_StateIdle)
    {
      DEBUG_PRINT("Requesting transition to Loaded");
      OMX_SendCommand(dec_handle, OMX_CommandStateSet, OMX_StateLoaded, 0);
      for(bufCnt=0; bufCnt < input_buf_cnt; ++bufCnt)
      {
         if (pInputBufHdrs[bufCnt]->pBuffer && input_use_buffer)
         {
            free(pInputBufHdrs[bufCnt]->pBuffer);
            pInputBufHdrs[bufCnt]->pBuffer = NULL;
            DEBUG_PRINT_ERROR("\nFree(pInputBufHdrs[%d]->pBuffer)",bufCnt);
         }
         OMX_FreeBuffer(dec_handle, 0, pInputBufHdrs[bufCnt]);
      }
      if (pInputBufHdrs)
      {
         free(pInputBufHdrs);
         pInputBufHdrs = NULL;
      }
      for(bufCnt = 0; bufCnt < portFmt.nBufferCountActual; ++bufCnt) {
        if (output_use_buffer && p_eglHeaders) {
            if(p_eglHeaders[bufCnt]) {
               munmap (pOutYUVBufHdrs[bufCnt]->pBuffer,
                       pOutYUVBufHdrs[bufCnt]->nAllocLen);
               close(p_eglHeaders[bufCnt]->pmem_fd);
               p_eglHeaders[bufCnt]->pmem_fd = -1;
               free(p_eglHeaders[bufCnt]);
               p_eglHeaders[bufCnt] = NULL;
            }
        }
        if (use_external_pmem_buf)
        {
            DEBUG_PRINT("Freeing in external pmem case: buffer=0x%x, pmem_fd=0x%d",
                              pOutYUVBufHdrs[bufCnt]->pBuffer,
                              pPMEMInfo[bufCnt].pmem_fd);
            if (pOutYUVBufHdrs[bufCnt]->pBuffer)
            {
                munmap (pOutYUVBufHdrs[bufCnt]->pBuffer,
                        pOutYUVBufHdrs[bufCnt]->nAllocLen);
            }
            if (&pPMEMInfo[bufCnt])
            {
                close(pPMEMInfo[bufCnt].pmem_fd);
                pPMEMInfo[bufCnt].pmem_fd = -1;
            }
        }
        OMX_FreeBuffer(dec_handle, 1, pOutYUVBufHdrs[bufCnt]);
      }
      if(p_eglHeaders) {
          free(p_eglHeaders);
          p_eglHeaders = NULL;
      }
      if (pPMEMInfo)
      {
          DEBUG_PRINT("Freeing in external pmem case:PMEM");
          free(pPMEMInfo);
          pPMEMInfo = NULL;
      }
      if (pPlatformEntry)
      {
          DEBUG_PRINT("Freeing in external pmem case:ENTRY");
          free(pPlatformEntry);
          pPlatformEntry = NULL;
      }
      if (pPlatformList)
      {
          DEBUG_PRINT("Freeing in external pmem case:LIST");
          free(pPlatformList);
          pPlatformList = NULL;
      }
      wait_for_event();
    }

    DEBUG_PRINT("[OMX Vdec Test] - Free handle decoder\n");
    OMX_ERRORTYPE result = OMX_FreeHandle(dec_handle);
    if (result != OMX_ErrorNone)
    {
       DEBUG_PRINT_ERROR("[OMX Vdec Test] - OMX_FreeHandle error. Error code: %d\n", result);
    }
    dec_handle = NULL;

    /* Deinit OpenMAX */
    DEBUG_PRINT("[OMX Vdec Test] - De-initializing OMX \n");
    OMX_Deinit();

    DEBUG_PRINT("[OMX Vdec Test] - closing all files\n");
    if(inputBufferFileFd != -1)
    {
        close(inputBufferFileFd);
        inputBufferFileFd = -1;
    }

    DEBUG_PRINT("[OMX Vdec Test] - after free inputfile\n");

    if (takeYuvLog && outputBufferFile) {
        fclose(outputBufferFile);
        outputBufferFile = NULL;
    }
    DEBUG_PRINT("[OMX Vdec Test] - after free outputfile\n");

    if(etb_queue)
    {
      free_queue(etb_queue);
      etb_queue = NULL;
    }
    DEBUG_PRINT("[OMX Vdec Test] - after free etb_queue \n");
    if(fbd_queue)
    {
      free_queue(fbd_queue);
      fbd_queue = NULL;
    }
    DEBUG_PRINT("[OMX Vdec Test] - after free iftb_queue\n");
    printf("*****************************************\n");
    if (isDueToError)
      printf("************...TEST FAILED...************\n");
    else
      printf("**********...TEST SUCCESSFULL...*********\n");
    printf("*****************************************\n");
}

static int Read_Buffer_From_DAT_File(OMX_BUFFERHEADERTYPE  *pBufHdr)
{
    long frameSize=0;
    char temp_buffer[10];
    char temp_byte;
    int bytes_read=0;
    int i=0;
    unsigned char *read_buffer=NULL;
    char c = '1'; //initialize to anything except '\0'(0)
    char inputFrameSize[12];
    int count =0; char cnt =0;
    memset(temp_buffer, 0, sizeof(temp_buffer));

    DEBUG_PRINT("Inside %s \n", __FUNCTION__);

    while (cnt < 10)
    /* Check the input file format, may result in infinite loop */
    {
        DEBUG_PRINT("loop[%d] count[%d]\n",cnt,count);
        count = read( inputBufferFileFd, &inputFrameSize[cnt], 1);
        if(inputFrameSize[cnt] == '\0' )
          break;
        cnt++;
    }
    inputFrameSize[cnt]='\0';
    frameSize = atoi(inputFrameSize);
    pBufHdr->nFilledLen = 0;

    /* get the frame length */
    lseek64(inputBufferFileFd, -1, SEEK_CUR);
    bytes_read = read(inputBufferFileFd, pBufHdr->pBuffer, frameSize);

    DEBUG_PRINT("Actual frame Size [%d] bytes_read using fread[%d]\n",
                  frameSize, bytes_read);

    if(bytes_read == 0 || bytes_read < frameSize ) {
        DEBUG_PRINT("Bytes read Zero After Read frame Size \n");
        DEBUG_PRINT("Checking VideoPlayback Count:video_playback_count is:%d\n",
                       video_playback_count);
        return 0;
    }
    pBufHdr->nTimeStamp = timeStampLfile;
    timeStampLfile += timestampInterval;
    return bytes_read;
}

static int Read_Buffer_ArbitraryBytes(OMX_BUFFERHEADERTYPE  *pBufHdr)
{
    int bytes_read=0;
    DEBUG_PRINT("Inside %s \n", __FUNCTION__);
    bytes_read = read(inputBufferFileFd, pBufHdr->pBuffer, NUMBER_OF_ARBITRARYBYTES_READ);
    if(bytes_read == 0) {
        DEBUG_PRINT("Bytes read Zero After Read frame Size \n");
        DEBUG_PRINT("Checking VideoPlayback Count:video_playback_count is:%d\n",
                      video_playback_count);
        return 0;
    }
#ifdef TEST_TS_FROM_SEI
    if (timeStampLfile == 0)
      pBufHdr->nTimeStamp = 0;
    else
      pBufHdr->nTimeStamp = LLONG_MAX;
#else
    pBufHdr->nTimeStamp = timeStampLfile;
#endif
    timeStampLfile += timestampInterval;
    return bytes_read;
}

static int Read_Buffer_From_Vop_Start_Code_File(OMX_BUFFERHEADERTYPE  *pBufHdr)
{
    unsigned int readOffset = 0;
    int bytes_read = 0;
    unsigned int code = 0;
    pBufHdr->nFilledLen = 0;
    static unsigned int header_code = 0;

    DEBUG_PRINT("Inside %s", __FUNCTION__);

    do
    {
      //Start codes are always byte aligned.
      bytes_read = read(inputBufferFileFd, &pBufHdr->pBuffer[readOffset], 1);
      if(bytes_read == 0 || bytes_read == -1)
      {
          DEBUG_PRINT("Bytes read Zero \n");
          break;
      }
      code <<= 8;
      code |= (0x000000FF & pBufHdr->pBuffer[readOffset]);
      //VOP start code comparision
      if (readOffset>3)
      {
        if(!header_code ){
          if( VOP_START_CODE == code)
          {
            header_code = VOP_START_CODE;
          }
          else if ( (0xFFFFFC00 & code) == SHORT_HEADER_START_CODE )
          {
            header_code = SHORT_HEADER_START_CODE;
          }
        }
        if ((header_code == VOP_START_CODE) && (code == VOP_START_CODE))
        {
            //Seek backwards by 4
            lseek64(inputBufferFileFd, -4, SEEK_CUR);
            readOffset-=3;
            break;
        }
        else if (( header_code == SHORT_HEADER_START_CODE ) && ( SHORT_HEADER_START_CODE == (code & 0xFFFFFC00)))
        {
            //Seek backwards by 4
            lseek64(inputBufferFileFd, -4, SEEK_CUR);
            readOffset-=3;
            break;
        }
      }
      readOffset++;
    }while (1);
    pBufHdr->nTimeStamp = timeStampLfile;
    timeStampLfile += timestampInterval;
    return readOffset;
}
static int Read_Buffer_From_Mpeg2_Start_Code(OMX_BUFFERHEADERTYPE  *pBufHdr)
{
  unsigned int readOffset = 0;
  int bytesRead = 0;
  unsigned int code = 0;
  pBufHdr->nFilledLen = 0;
  static unsigned int firstParse = true;
  unsigned int seenFrame = false;

  DEBUG_PRINT("Inside %s", __FUNCTION__);

  /* Read one byte at a time. Construct the code every byte in order to
   * compare to the start codes. Keep looping until we've read in a complete
   * frame, which can be either just a picture start code + picture, or can
   * include the sequence header as well
   */
  while (1) {
    bytesRead = read(inputBufferFileFd, &pBufHdr->pBuffer[readOffset], 1);

    /* Exit the loop if we can't read any more bytes */
    if (bytesRead == 0 || bytesRead == -1) {
      break;
    }

    /* Construct the code one byte at a time */
    code <<= 8;
    code |= (0x000000FF & pBufHdr->pBuffer[readOffset]);

    /* Can't compare the code to MPEG2 start codes until we've read the
     * first four bytes
     */
    if (readOffset >= 3) {

      /* If this is the first time we're reading from the file, then we
       * need to throw away the system start code information at the
       * beginning. We can just look for the first sequence header.
       */
      if (firstParse) {
        if (code == MPEG2_SEQ_START_CODE) {
          /* Seek back by 4 bytes and reset code so that we can skip
           * down to the common case below.
           */
          lseek(inputBufferFileFd, -4, SEEK_CUR);
          code = 0;
          readOffset -= 3;
          firstParse = false;
          continue;
        }
      }

      /* If we have already parsed a frame and we see a sequence header, then
       * the sequence header is part of the next frame so we seek back and
       * break.
       */
      if (code == MPEG2_SEQ_START_CODE) {
        if (seenFrame) {
          lseek(inputBufferFileFd, -4, SEEK_CUR);
          readOffset -= 3;
          break;
        }
        /* If we haven't seen a frame yet, then read in all the data until we
         * either see another frame start code or sequence header start code.
         */
      } else if (code == MPEG2_FRAME_START_CODE) {
        if (!seenFrame) {
          seenFrame = true;
        } else {
          lseek(inputBufferFileFd, -4, SEEK_CUR);
          readOffset -= 3;
          break;
        }
      }
    }

    readOffset++;
  }

  pBufHdr->nTimeStamp = timeStampLfile;
  timeStampLfile += timestampInterval;
  return readOffset;
}


static int Read_Buffer_From_Size_Nal(OMX_BUFFERHEADERTYPE  *pBufHdr)
{
    // NAL unit stream processing
    char temp_size[SIZE_NAL_FIELD_MAX];
    int i = 0;
    int j = 0;
    unsigned int size = 0;   // Need to make sure that uint32 has SIZE_NAL_FIELD_MAX (4) bytes
    int bytes_read = 0;

    // read the "size_nal_field"-byte size field
    bytes_read = read(inputBufferFileFd, pBufHdr->pBuffer + pBufHdr->nOffset, nalSize);
    if (bytes_read == 0 || bytes_read == -1)
    {
      DEBUG_PRINT("Failed to read frame or it might be EOF\n");
      return 0;
    }

    for (i=0; i<SIZE_NAL_FIELD_MAX-nalSize; i++)
    {
      temp_size[SIZE_NAL_FIELD_MAX - 1 - i] = 0;
    }

    /* Due to little endiannes, Reorder the size based on size_nal_field */
    for (j=0; i<SIZE_NAL_FIELD_MAX; i++, j++)
    {
      temp_size[SIZE_NAL_FIELD_MAX - 1 - i] = pBufHdr->pBuffer[pBufHdr->nOffset + j];
    }
    size = (unsigned int)(*((unsigned int *)(temp_size)));

    // now read the data
    bytes_read = read(inputBufferFileFd, pBufHdr->pBuffer + pBufHdr->nOffset + nalSize, size);
    if (bytes_read != size)
    {
      DEBUG_PRINT_ERROR("Failed to read frame\n");
    }

    pBufHdr->nTimeStamp = timeStampLfile;
    timeStampLfile += timestampInterval;

    return bytes_read + nalSize;
}

static int Read_Buffer_From_RCV_File_Seq_Layer(OMX_BUFFERHEADERTYPE  *pBufHdr)
{
    unsigned int readOffset = 0, size_struct_C = 0;
    unsigned int startcode = 0;
    pBufHdr->nFilledLen = 0;
    pBufHdr->nFlags = 0;

    DEBUG_PRINT("Inside %s \n", __FUNCTION__);

    read(inputBufferFileFd, &startcode, 4);

    /* read size of struct C as it need not be 4 always*/
    read(inputBufferFileFd, &size_struct_C, 4);

    /* reseek to beginning of sequence header */
    lseek64(inputBufferFileFd, -8, SEEK_CUR);

    if ((startcode & 0xFF000000) == 0xC5000000)
    {

      DEBUG_PRINT("Read_Buffer_From_RCV_File_Seq_Layer size_struct_C: %d\n", size_struct_C);

      readOffset = read(inputBufferFileFd, pBufHdr->pBuffer, VC1_SEQ_LAYER_SIZE_WITHOUT_STRUCTC + size_struct_C);

    }
    else if((startcode & 0xFF000000) == 0x85000000)
    {
      // .RCV V1 file

      rcv_v1 = 1;

      DEBUG_PRINT("Read_Buffer_From_RCV_File_Seq_Layer size_struct_C: %d\n", size_struct_C);

      readOffset = read(inputBufferFileFd, pBufHdr->pBuffer, VC1_SEQ_LAYER_SIZE_V1_WITHOUT_STRUCTC + size_struct_C);

    }
    else
    {
      DEBUG_PRINT_ERROR("Error: Unknown VC1 clip format %x\n", startcode);
    }

#if 0
    {
      int i=0;
      printf("Read_Buffer_From_RCV_File, length %d readOffset %d\n", readOffset, readOffset);
      for (i=0; i<36; i++)
      {
        printf("0x%.2x ", pBufHdr->pBuffer[i]);
        if (i%16 == 15) {
          printf("\n");
        }
      }
      printf("\n");
    }
#endif
    return readOffset;
}

static int Read_Buffer_From_RCV_File(OMX_BUFFERHEADERTYPE  *pBufHdr)
{
    unsigned int readOffset = 0;
    unsigned int len = 0;
    unsigned int key = 0;
    DEBUG_PRINT("Inside %s \n", __FUNCTION__);

    DEBUG_PRINT("Read_Buffer_From_RCV_File - nOffset %d\n", pBufHdr->nOffset);
    if(rcv_v1)
    {
      /* for the case of RCV V1 format, the frame header is only of 4 bytes and has
         only the frame size information */
        readOffset = read(inputBufferFileFd, &len, 4);
        DEBUG_PRINT("Read_Buffer_From_RCV_File - framesize %d %x\n", len, len);

    }
    else
    {
      /* for a regular RCV file, 3 bytes comprise the frame size and 1 byte for key*/
        readOffset = read(inputBufferFileFd, &len, 3);
        DEBUG_PRINT("Read_Buffer_From_RCV_File - framesize %d %x\n", len, len);

        readOffset = read(inputBufferFileFd, &key, 1);
      if ( (key & 0x80) == false)
      {
        DEBUG_PRINT("Read_Buffer_From_RCV_File - Non IDR frame key %x\n", key);
       }

    }

    if(!rcv_v1)
    {
      /* There is timestamp field only for regular RCV format and not for RCV V1 format*/
        readOffset = read(inputBufferFileFd, &pBufHdr->nTimeStamp, 4);
        DEBUG_PRINT("Read_Buffer_From_RCV_File - timeStamp %d\n", pBufHdr->nTimeStamp);
        pBufHdr->nTimeStamp *= 1000;
    }
    else
    {
        pBufHdr->nTimeStamp = timeStampLfile;
        timeStampLfile += timestampInterval;
    }

    if(len > pBufHdr->nAllocLen)
    {
       DEBUG_PRINT_ERROR("Error in sufficient buffer framesize %d, allocalen %d noffset %d\n",len,pBufHdr->nAllocLen, pBufHdr->nOffset);
       readOffset = read(inputBufferFileFd, pBufHdr->pBuffer+pBufHdr->nOffset,
                         pBufHdr->nAllocLen - pBufHdr->nOffset);

       loff_t off = (len - readOffset)*1LL;
       lseek64(inputBufferFileFd, off ,SEEK_CUR);
       return readOffset;
    }
    else {
        readOffset = read(inputBufferFileFd, pBufHdr->pBuffer+pBufHdr->nOffset, len);
    }
    if (readOffset != len)
    {
      DEBUG_PRINT("EOS reach or Reading error %d, %s \n", readOffset, strerror( errno ));
      return 0;
    }

#if 0
    {
      int i=0;
      printf("Read_Buffer_From_RCV_File, length %d readOffset %d\n", len, readOffset);
      for (i=0; i<64; i++)
      {
        printf("0x%.2x ", pBufHdr->pBuffer[i]);
        if (i%16 == 15) {
          printf("\n");
        }
      }
      printf("\n");
    }
#endif

    return readOffset;
}

static int Read_Buffer_From_VC1_File(OMX_BUFFERHEADERTYPE  *pBufHdr)
{
    static int timeStampLfile = 0;
    OMX_U8 *pBuffer = pBufHdr->pBuffer + pBufHdr->nOffset;
    DEBUG_PRINT("Inside %s \n", __FUNCTION__);
    unsigned int readOffset = 0;
    int bytes_read = 0;
    unsigned int code = 0, total_bytes = 0;
    int startCode_cnt = 0;
    int bSEQflag = 0;
    int bEntryflag = 0;
    unsigned int SEQbytes = 0;
    int numStartcodes = 0;

    numStartcodes = bHdrflag?1:2;

    do
    {
      if (total_bytes == pBufHdr->nAllocLen)
      {
        DEBUG_PRINT_ERROR("Buffer overflow!");
        break;
      }
      //Start codes are always byte aligned.
      bytes_read = read(inputBufferFileFd, &pBuffer[readOffset],1 );

      if(!bytes_read)
      {
          DEBUG_PRINT("\n Bytes read Zero \n");
          break;
      }
      total_bytes++;
      code <<= 8;
      code |= (0x000000FF & pBufHdr->pBuffer[readOffset]);

     if(!bSEQflag && (code == VC1_SEQUENCE_START_CODE)) {
        if(startCode_cnt) bSEQflag = 1;
      }

      if(!bEntryflag && ( code == VC1_ENTRY_POINT_START_CODE)) {
         if(startCode_cnt) bEntryflag = 1;
      }

      if(code == VC1_FRAME_START_CODE || code == VC1_FRAME_FIELD_CODE)
      {
         startCode_cnt++ ;
      }

      //VOP start code comparision
      if(startCode_cnt == numStartcodes)
      {
        if (VC1_FRAME_START_CODE == (code & 0xFFFFFFFF) ||
            VC1_FRAME_FIELD_CODE == (code & 0xFFFFFFFF))
        {
          previous_vc1_au = 0;
          if(VC1_FRAME_FIELD_CODE == (code & 0xFFFFFFFF))
          {
              previous_vc1_au = 1;
          }

          if(!bHdrflag && (bSEQflag || bEntryflag)) {
             lseek(inputBufferFileFd,-(SEQbytes+4),SEEK_CUR);
             readOffset -= (SEQbytes+3);
          }
          else {
            //Seek backwards by 4
            lseek64(inputBufferFileFd, -4, SEEK_CUR);
            readOffset-=3;
          }

          while(pBufHdr->pBuffer[readOffset-1] == 0)
            readOffset--;

          break;
        }
      }
      readOffset++;
      if(bSEQflag || bEntryflag) {
        SEQbytes++;
      }
    }while (1);

    pBufHdr->nTimeStamp = timeStampLfile;
    timeStampLfile += timestampInterval;

#if 0
    {
      int i=0;
      printf("Read_Buffer_From_VC1_File, readOffset %d\n", readOffset);
      for (i=0; i<64; i++)
      {
        printf("0x%.2x ", pBufHdr->pBuffer[i]);
        if (i%16 == 15) {
          printf("\n");
        }
      }
      printf("\n");
    }
#endif

    return readOffset;
}

static int Read_Buffer_From_DivX_4_5_6_File(OMX_BUFFERHEADERTYPE  *pBufHdr)
{
#define MAX_NO_B_FRMS 3 // Number of non-b-frames packed in each buffer
#define N_PREV_FRMS_B 1 // Number of previous non-b-frames packed
                        // with a set of consecutive b-frames
#define FRM_ARRAY_SIZE (MAX_NO_B_FRMS + N_PREV_FRMS_B)
    char *p_buffer = NULL;
    unsigned int offset_array[FRM_ARRAY_SIZE];
    int byte_cntr, pckt_end_idx;
    unsigned int read_code = 0, bytes_read, byte_pos = 0, frame_type;
    unsigned int i, b_frm_idx, b_frames_found = 0, vop_set_cntr = 0;
    bool pckt_ready = false;
#ifdef __DEBUG_DIVX__
    char pckt_type[20];
    int pckd_frms = 0;
    static unsigned long long int total_bytes = 0;
    static unsigned long long int total_frames = 0;
#endif //__DEBUG_DIVX__

    DEBUG_PRINT("Inside %s \n", __FUNCTION__);

    do {
      p_buffer = (char *)pBufHdr->pBuffer + byte_pos;

      bytes_read = read(inputBufferFileFd, p_buffer, NUMBER_OF_ARBITRARYBYTES_READ);
      byte_pos += bytes_read;
      for (byte_cntr = 0; byte_cntr < bytes_read && !pckt_ready; byte_cntr++) {
        read_code <<= 8;
        ((char*)&read_code)[0] = p_buffer[byte_cntr];
        if (read_code == VOP_START_CODE) {
          if (++byte_cntr < bytes_read) {
            frame_type = p_buffer[byte_cntr];
            frame_type &= 0x000000C0;
#ifdef __DEBUG_DIVX__
            switch (frame_type) {
              case 0x00: pckt_type[pckd_frms] = 'I'; break;
              case 0x40: pckt_type[pckd_frms] = 'P'; break;
              case 0x80: pckt_type[pckd_frms] = 'B'; break;
              default: pckt_type[pckd_frms] = 'X';
            }
            pckd_frms++;
#endif // __DEBUG_DIVX__
            offset_array[vop_set_cntr] = byte_pos - bytes_read + byte_cntr - 4;
            if (frame_type == 0x80) { // B Frame found!
              if (!b_frames_found) {
                // Try to packet N_PREV_FRMS_B previous frames
                // with the next consecutive B frames
                i = N_PREV_FRMS_B;
                while ((vop_set_cntr - i) < 0 && i > 0) i--;
                b_frm_idx = vop_set_cntr - i;
                if (b_frm_idx > 0) {
                  pckt_end_idx = b_frm_idx;
                  pckt_ready = true;
#ifdef __DEBUG_DIVX__
                  pckt_type[b_frm_idx] = '\0';
                  total_frames += b_frm_idx;
#endif //__DEBUG_DIVX__
                }
              }
              b_frames_found++;
            } else if (b_frames_found) {
              pckt_end_idx = vop_set_cntr;
              pckt_ready = true;
#ifdef __DEBUG_DIVX__
              pckt_type[pckd_frms - 1] = '\0';
              total_frames += pckd_frms - 1;
#endif //__DEBUG_DIVX__
            } else if (vop_set_cntr == (FRM_ARRAY_SIZE -1)) {
              pckt_end_idx = MAX_NO_B_FRMS;
              pckt_ready = true;
#ifdef __DEBUG_DIVX__
              pckt_type[pckt_end_idx] = '\0';
              total_frames += pckt_end_idx;
#endif //__DEBUG_DIVX__
            } else
              vop_set_cntr++;
          } else {
            // The vop start code was found in the last 4 bytes,
            // seek backwards by 4 to include this start code
            // with the next buffer.
            lseek64(inputBufferFileFd, -4, SEEK_CUR);
            byte_pos -= 4;
#ifdef __DEBUG_DIVX__
            pckd_frms--;
#endif //__DEBUG_DIVX__
          }
        }
      }
      if (pckt_ready) {
          loff_t off = (byte_pos - offset_array[pckt_end_idx]);
          if ( lseek64(inputBufferFileFd, -1LL*off , SEEK_CUR) == -1 ){
              DEBUG_PRINT_ERROR("lseek64 with offset = %lld failed with errno %d"
                                ", current position =0x%llx", -1LL*off,
                                errno, lseek64(inputBufferFileFd, 0, SEEK_CUR));
          }
      }
      else {
          char eofByte;
          int ret = read(inputBufferFileFd, &eofByte, 1 );
          if ( ret == 0 ) {
              offset_array[vop_set_cntr] = byte_pos;
              pckt_end_idx = vop_set_cntr;
              pckt_ready = true;
#ifdef __DEBUG_DIVX__
              pckt_type[pckd_frms] = '\0';
              total_frames += pckd_frms;
#endif //__DEBUG_DIVX__
          }
          else if (ret == 1){
              if ( lseek64(inputBufferFileFd, -1, SEEK_CUR ) == -1 ){
                  DEBUG_PRINT_ERROR("lseek64 failed with errno = %d, "
                                    "current fileposition = %llx",
                                    errno,
                                    lseek64(inputBufferFileFd, 0, SEEK_CUR));
              }
          }
          else {
              DEBUG_PRINT_ERROR("Error when checking for EOF");
          }
      }
    } while (!pckt_ready);
    pBufHdr->nFilledLen = offset_array[pckt_end_idx];
    pBufHdr->nTimeStamp = timeStampLfile;
    timeStampLfile += timestampInterval;
#ifdef __DEBUG_DIVX__
    total_bytes += pBufHdr->nFilledLen;
    ALOGE("[DivX] Packet: Type[%s] Size[%u] TS[%lld] TB[%llx] NFrms[%lld]\n",
      pckt_type, pBufHdr->nFilledLen, pBufHdr->nTimeStamp,
	  total_bytes, total_frames);
#endif //__DEBUG_DIVX__
    return pBufHdr->nFilledLen;
}

static int Read_Buffer_From_DivX_311_File(OMX_BUFFERHEADERTYPE  *pBufHdr)
{
    static OMX_S64 timeStampLfile = 0;
    char *p_buffer = NULL;
    bool pkt_ready = false;
    unsigned int frame_type = 0;
    unsigned int bytes_read = 0;
    unsigned int frame_size = 0;
    unsigned int num_bytes_size = 4;
    unsigned int num_bytes_frame_type = 1;
    unsigned int n_offset = pBufHdr->nOffset;

    DEBUG_PRINT("Inside %s \n", __FUNCTION__);

    pBufHdr->nTimeStamp = timeStampLfile;

    if (pBufHdr != NULL)
    {
        p_buffer = (char *)pBufHdr->pBuffer + pBufHdr->nOffset;
    }
    else
    {
        DEBUG_PRINT("\n ERROR:Read_Buffer_From_DivX_311_File: pBufHdr is NULL\n");
        return 0;
    }

    if (p_buffer == NULL)
    {
        DEBUG_PRINT("\n ERROR:Read_Buffer_From_DivX_311_File: p_bufhdr is NULL\n");
        return 0;
    }

    //Read first frame based on size
    //DivX 311 frame - 4 byte header with size followed by the frame

    bytes_read = read(inputBufferFileFd, &frame_size, num_bytes_size);

    DEBUG_PRINT("Read_Buffer_From_DivX_311_File: Frame size = %d\n", frame_size);
    n_offset += read(inputBufferFileFd, p_buffer, frame_size);

    pBufHdr->nTimeStamp = timeStampLfile;

    timeStampLfile += timestampInterval;

    //the packet is ready to be sent
    DEBUG_PRINT("\nReturning Read Buffer from Divx 311: TS=[%ld], Offset=[%d]\n",
           (long int)pBufHdr->nTimeStamp,
           n_offset );

    return n_offset;
}

static int open_video_file ()
{
    int error_code = 0;
    char outputfilename[512];
    DEBUG_PRINT("Inside %s filename=%s\n", __FUNCTION__, in_filename);

    if ( (inputBufferFileFd = open( in_filename, O_RDONLY | O_LARGEFILE) ) == -1 ){
        DEBUG_PRINT_ERROR("Error - i/p file %s could NOT be opened errno = %d\n",
                          in_filename, errno);
        error_code = -1;
    }
    else {
        DEBUG_PRINT_ERROR("i/p file %s is opened \n", in_filename);
    }

    if (takeYuvLog) {
        strlcpy(outputfilename, "yuvframes.yuv", 14);
        outputBufferFile = fopen (outputfilename, "ab");
        if (outputBufferFile == NULL)
        {
          DEBUG_PRINT_ERROR("ERROR - o/p file %s could NOT be opened\n", outputfilename);
          error_code = -1;
        }
        else
        {
          DEBUG_PRINT("O/p file %s is opened \n", outputfilename);
        }
    }
    return error_code;
}

void swap_byte(char *pByte, int nbyte)
{
  int i=0;

  for (i=0; i<nbyte/2; i++)
  {
    pByte[i] ^= pByte[nbyte-i-1];
    pByte[nbyte-i-1] ^= pByte[i];
    pByte[i] ^= pByte[nbyte-i-1];
  }
}

int drawBG(void)
{
    int result;
    int i;
#ifdef FRAMEBUFFER_32
    long * p;
#else
    short * p;
#endif
    void *fb_buf = mmap (NULL, finfo.smem_len,PROT_READ|PROT_WRITE, MAP_SHARED, fb_fd, 0);

    if (fb_buf == MAP_FAILED)
    {
        printf("ERROR: Framebuffer MMAP failed!\n");
        close(fb_fd);
        return -1;
    }

    vinfo.yoffset = 0;
    p = (long *)fb_buf;

    for (i=0; i < vinfo.xres * vinfo.yres; i++)
    {
    #ifdef FRAMEBUFFER_32
        *p++ = COLOR_BLACK_RGBA_8888;
    #else
        *p++ = CLR_KEY;
    #endif
    }

    if (ioctl(fb_fd, FBIOPAN_DISPLAY, &vinfo) < 0)
    {
        printf("ERROR: FBIOPAN_DISPLAY failed! line=%d\n", __LINE__);
        return -1;
    }

    DEBUG_PRINT("drawBG success!\n");
    return 0;
}

void overlay_set()
{
    overlayp = &overlay;
    overlayp->src.width  = stride;
    overlayp->src.height = sliceheight;
#ifdef MAX_RES_720P
    overlayp->src.format = MDP_Y_CRCB_H2V2;
    if(color_fmt == (OMX_COLOR_FORMATTYPE)QOMX_COLOR_FormatYUV420PackedSemiPlanar64x32Tile2m8ka)
    {
        overlayp->src.format = MDP_Y_CRCB_H2V2_TILE;
    }
#endif
#ifdef MAX_RES_1080P
    overlayp->src.format = MDP_Y_CBCR_H2V2_TILE;
#endif
    overlayp->src_rect.x = 0;
    overlayp->src_rect.y = 0;
    overlayp->src_rect.w = width;
    overlayp->src_rect.h = height;

    if(width >= vinfo.xres)
    {
        overlayp->dst_rect.x = 0;
        overlayp->dst_rect.w = vinfo.xres;
    }
    else
    {
        overlayp->dst_rect.x = (vinfo.xres - width)/2;
        overlayp->dst_rect.w = width;
    }

    if(height >= vinfo.yres)
    {
        overlayp->dst_rect.h = (overlayp->dst_rect.w * height)/width;
        overlayp->dst_rect.y = 0;
        if (overlayp->dst_rect.h < vinfo.yres)
            overlayp->dst_rect.y = (vinfo.yres - overlayp->dst_rect.h)/2;
    }
    else
    {
        overlayp->dst_rect.y = (vinfo.yres - height)/2;
        overlayp->dst_rect.h = height;
    }

    overlayp->z_order = 0;
    printf("overlayp->dst_rect.x = %u \n", overlayp->dst_rect.x);
    printf("overlayp->dst_rect.y = %u \n", overlayp->dst_rect.y);
    printf("overlayp->dst_rect.w = %u \n", overlayp->dst_rect.w);
    printf("overlayp->dst_rect.h = %u \n", overlayp->dst_rect.h);

    overlayp->alpha = 0x0;
    overlayp->transp_mask = 0xFFFFFFFF;
    overlayp->flags = 0;
    overlayp->is_fg = 0;

    overlayp->id = MSMFB_NEW_REQUEST;
    vid_buf_front_id = ioctl(fb_fd, MSMFB_OVERLAY_SET, overlayp);
    if (vid_buf_front_id < 0)
    {
        printf("ERROR: MSMFB_OVERLAY_SET failed! line=%d\n", __LINE__);
    }
    vid_buf_front_id = overlayp->id;
    DEBUG_PRINT("\n vid_buf_front_id = %u", vid_buf_front_id);
    drawBG();
    displayYuv = 2;
}

int overlay_fb(struct OMX_BUFFERHEADERTYPE *pBufHdr)
{
    OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO *pPMEMInfo = NULL;
    struct msmfb_overlay_data ov_front;
    memset(&ov_front, 0, sizeof(struct msmfb_overlay_data));
#if defined(_ANDROID_) && !defined(USE_EGL_IMAGE_TEST_APP) && !defined(USE_EXTERN_PMEM_BUF)
    MemoryHeapBase *vheap = NULL;
#endif

    DEBUG_PRINT("overlay_fb:");
    ov_front.id = overlayp->id;
    if (pBufHdr->pPlatformPrivate == NULL)
    {
        ALOGE("overlay_fb: pPlatformPrivate is null");
        return -1;
    }
    pPMEMInfo  = (OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO *)
                ((OMX_QCOM_PLATFORM_PRIVATE_LIST *)
                    pBufHdr->pPlatformPrivate)->entryList->entry;
    if (pPMEMInfo == NULL)
    {

        ALOGE("overlay_fb: pmem_info is null");
        return -1;
    }
#if defined(_ANDROID_) && !defined(USE_EGL_IMAGE_TEST_APP) && !defined(USE_EXTERN_PMEM_BUF)
    vheap = (MemoryHeapBase*)pPMEMInfo->pmem_fd;
#endif

#if defined(_ANDROID_) && !defined(USE_EGL_IMAGE_TEST_APP) && !defined(USE_EXTERN_PMEM_BUF)
    ov_front.data.memory_id = vheap->getHeapID();
#else
    ov_front.data.memory_id = pPMEMInfo->pmem_fd;
#endif

    ov_front.data.offset = pPMEMInfo->offset;

    DEBUG_PRINT("\n ov_front.data.memory_id = %d", ov_front.data.memory_id);
    DEBUG_PRINT("\n ov_front.data.offset = %u", ov_front.data.offset);
    if (ioctl(fb_fd, MSMFB_OVERLAY_PLAY, (void*)&ov_front))
    {
        printf("\nERROR! MSMFB_OVERLAY_PLAY failed at frame (Line %d)\n",
            __LINE__);
        return -1;
    }
    DEBUG_PRINT("\nMSMFB_OVERLAY_PLAY successfull");
    return 0;
}

void overlay_unset()
{
    if (ioctl(fb_fd, MSMFB_OVERLAY_UNSET, &vid_buf_front_id))
    {
        printf("\nERROR! MSMFB_OVERLAY_UNSET failed! (Line %d)\n", __LINE__);
    }
}

void render_fb(struct OMX_BUFFERHEADERTYPE *pBufHdr)
{
    unsigned int addr = 0;
    OMX_OTHER_EXTRADATATYPE *pExtraData = 0;
    OMX_QCOM_EXTRADATA_FRAMEINFO *pExtraFrameInfo = 0;
    OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO *pPMEMInfo = NULL;
    unsigned int destx, desty,destW, destH;
#if defined(_ANDROID_) && !defined(USE_EGL_IMAGE_TEST_APP) && !defined(USE_EXTERN_PMEM_BUF)
    MemoryHeapBase *vheap = NULL;
#endif

    unsigned int end = (unsigned int)(pBufHdr->pBuffer + pBufHdr->nAllocLen);

    struct mdp_blit_req *e;
    union {
        char dummy[sizeof(struct mdp_blit_req_list) +
               sizeof(struct mdp_blit_req) * 1];
        struct mdp_blit_req_list list;
    } img;

    if (fb_fd < 0)
    {
        DEBUG_PRINT_ERROR("Warning: /dev/fb0 is not opened!\n");
        return;
    }

    img.list.count = 1;
    e = &img.list.req[0];

    addr = (unsigned int)(pBufHdr->pBuffer + pBufHdr->nFilledLen);
    // align to a 4 byte boundary
    addr = (addr + 3) & (~3);

    // read to the end of existing extra data sections
    pExtraData = (OMX_OTHER_EXTRADATATYPE*)addr;

    while (addr < end && pExtraData->eType != OMX_ExtraDataFrameInfo)
    {
            addr += pExtraData->nSize;
            pExtraData = (OMX_OTHER_EXTRADATATYPE*)addr;
    }

    if (pExtraData->eType != OMX_ExtraDataFrameInfo)
    {
       DEBUG_PRINT_ERROR("pExtraData->eType %d pExtraData->nSize %d\n",pExtraData->eType,pExtraData->nSize);
    }
    pExtraFrameInfo = (OMX_QCOM_EXTRADATA_FRAMEINFO *)pExtraData->data;

   pPMEMInfo  = (OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO *)
                ((OMX_QCOM_PLATFORM_PRIVATE_LIST *)
                    pBufHdr->pPlatformPrivate)->entryList->entry;
#if defined(_ANDROID_) && !defined(USE_EGL_IMAGE_TEST_APP) && !defined(USE_EXTERN_PMEM_BUF)
    vheap = (MemoryHeapBase *)pPMEMInfo->pmem_fd;
#endif


    DEBUG_PRINT_ERROR("DecWidth %d DecHeight %d\n",portFmt.format.video.nStride,portFmt.format.video.nSliceHeight);
    DEBUG_PRINT_ERROR("DispWidth %d DispHeight %d\n",portFmt.format.video.nFrameWidth,portFmt.format.video.nFrameHeight);



    e->src.width = portFmt.format.video.nStride;
    e->src.height = portFmt.format.video.nSliceHeight;
    e->src.format = MDP_Y_CBCR_H2V2;
        e->src.offset = pPMEMInfo->offset;
#if defined(_ANDROID_) && !defined(USE_EGL_IMAGE_TEST_APP) && !defined(USE_EXTERN_PMEM_BUF)
    e->src.memory_id = vheap->getHeapID();
#else
    e->src.memory_id = pPMEMInfo->pmem_fd;
#endif

    DEBUG_PRINT_ERROR("pmemOffset %d pmemID %d\n",e->src.offset,e->src.memory_id);

    e->dst.width = vinfo.xres;
    e->dst.height = vinfo.yres;
    e->dst.format = MDP_RGB_565;
    e->dst.offset = 0;
    e->dst.memory_id = fb_fd;

    e->transp_mask = 0xffffffff;
    DEBUG_PRINT("Frame interlace type %d!\n", pExtraFrameInfo->interlaceType);
    if(pExtraFrameInfo->interlaceType != OMX_QCOM_InterlaceFrameProgressive)
    {
       DEBUG_PRINT("Interlaced Frame!\n");
       e->flags = MDP_DEINTERLACE;
    }
    else
        e->flags = 0;
    e->alpha = 0xff;

    switch(displayWindow)
    {
    case 1: destx = 0;
            desty = 0;
            destW = vinfo.xres/2;
            destH = vinfo.yres/2;
            break;
    case 2: destx = vinfo.xres/2;
            desty = 0;
            destW = vinfo.xres/2;
            destH = vinfo.yres/2;
            break;

    case 3: destx = 0;
            desty = vinfo.yres/2;
            destW = vinfo.xres/2;
            destH = vinfo.yres/2;
            break;
     case 4: destx = vinfo.xres/2;
            desty = vinfo.yres/2;
            destW = vinfo.xres/2;
            destH = vinfo.yres/2;
            break;
     case 0:
     default:
            destx = 0;
            desty = 0;
            destW = vinfo.xres;
            destH = vinfo.yres;
    }


        if(portFmt.format.video.nFrameWidth < destW)
          destW = portFmt.format.video.nFrameWidth ;


        if(portFmt.format.video.nFrameHeight < destH)
           destH = portFmt.format.video.nFrameHeight;

    e->dst_rect.x = destx;
    e->dst_rect.y = desty;
    e->dst_rect.w = destW;
    e->dst_rect.h = destH;

    //e->dst_rect.w = 800;
    //e->dst_rect.h = 480;

    e->src_rect.x = 0;
    e->src_rect.y = 0;
    e->src_rect.w = portFmt.format.video.nFrameWidth;
    e->src_rect.h = portFmt.format.video.nFrameHeight;

    //e->src_rect.w = portFmt.format.video.nStride;
    //e->src_rect.h = portFmt.format.video.nSliceHeight;

    if (ioctl(fb_fd, MSMFB_BLIT, &img)) {
        DEBUG_PRINT_ERROR("MSMFB_BLIT ioctl failed!\n");
        return;
    }

    if (ioctl(fb_fd, FBIOPAN_DISPLAY, &vinfo) < 0) {
        DEBUG_PRINT_ERROR("FBIOPAN_DISPLAY failed! line=%d\n", __LINE__);
        return;
    }

    DEBUG_PRINT("render_fb complete!\n");
}

int disable_output_port()
{
    DEBUG_PRINT("DISABLING OP PORT\n");
    pthread_mutex_lock(&enable_lock);
    sent_disabled = 1;
    // Send DISABLE command
    OMX_SendCommand(dec_handle, OMX_CommandPortDisable, 1, 0);
    pthread_mutex_unlock(&enable_lock);
    // wait for Disable event to come back
    wait_for_event();
    if(p_eglHeaders) {
        free(p_eglHeaders);
        p_eglHeaders = NULL;
    }
    if (pPMEMInfo)
    {
        DEBUG_PRINT("Freeing in external pmem case:PMEM");
        free(pPMEMInfo);
        pPMEMInfo = NULL;
    }
    if (pPlatformEntry)
    {
        DEBUG_PRINT("Freeing in external pmem case:ENTRY");
        free(pPlatformEntry);
        pPlatformEntry = NULL;
    }
    if (pPlatformList)
    {
        DEBUG_PRINT("Freeing in external pmem case:LIST");
        free(pPlatformList);
        pPlatformList = NULL;
    }
    if (currentStatus == ERROR_STATE)
    {
      do_freeHandle_and_clean_up(true);
      return -1;
    }
    DEBUG_PRINT("OP PORT DISABLED!\n");
    return 0;
}

int enable_output_port()
{
    int bufCnt = 0;
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    DEBUG_PRINT("ENABLING OP PORT\n");
    // Send Enable command
    OMX_SendCommand(dec_handle, OMX_CommandPortEnable, 1, 0);
#ifndef USE_EGL_IMAGE_TEST_APP
    /* Allocate buffer on decoder's o/p port */
    portFmt.nPortIndex = 1;
    if (use_external_pmem_buf)
    {
        DEBUG_PRINT("Enable op port: calling use_buffer_mult_fd\n");
        error =  use_output_buffer_multiple_fd(dec_handle,
                                               &pOutYUVBufHdrs,
                                               portFmt.nPortIndex,
                                               portFmt.nBufferSize,
                                               portFmt.nBufferCountActual);
    }
    else
    {
        error = Allocate_Buffer(dec_handle, &pOutYUVBufHdrs, portFmt.nPortIndex,
                                portFmt.nBufferCountActual, portFmt.nBufferSize);
    }
    if (error != OMX_ErrorNone) {
        DEBUG_PRINT_ERROR("Error - OMX_AllocateBuffer Output buffer error\n");
        return -1;
    }
    else
    {
        DEBUG_PRINT("OMX_AllocateBuffer Output buffer success\n");
        free_op_buf_cnt = portFmt.nBufferCountActual;
    }
#else
    error =  use_output_buffer(dec_handle,
                       &pOutYUVBufHdrs,
                        portFmt.nPortIndex,
                        portFmt.nBufferSize,
                        portFmt.nBufferCountActual);
    free_op_buf_cnt = portFmt.nBufferCountActual;
    if (error != OMX_ErrorNone) {
       DEBUG_PRINT_ERROR("ERROR - OMX_UseBuffer Input buffer failed");
       return -1;
    }
    else {
       DEBUG_PRINT("OMX_UseBuffer Input buffer success\n");
    }

#endif
    // wait for enable event to come back
    wait_for_event();
    if (currentStatus == ERROR_STATE)
    {
      do_freeHandle_and_clean_up(true);
      return -1;
    }
    if (pOutYUVBufHdrs == NULL)
    {
        DEBUG_PRINT_ERROR("Error - pOutYUVBufHdrs is NULL\n");
        return -1;
    }
    for(bufCnt=0; bufCnt < portFmt.nBufferCountActual; ++bufCnt) {
        DEBUG_PRINT("OMX_FillThisBuffer on output buf no.%d\n",bufCnt);
        if (pOutYUVBufHdrs[bufCnt] == NULL)
        {
            DEBUG_PRINT_ERROR("Error - pOutYUVBufHdrs[%d] is NULL\n", bufCnt);
            return -1;
        }
        pOutYUVBufHdrs[bufCnt]->nOutputPortIndex = 1;
        pOutYUVBufHdrs[bufCnt]->nFlags &= ~OMX_BUFFERFLAG_EOS;
        ret = OMX_FillThisBuffer(dec_handle, pOutYUVBufHdrs[bufCnt]);
        if (OMX_ErrorNone != ret) {
            DEBUG_PRINT_ERROR("ERROR - OMX_FillThisBuffer failed with result %d\n", ret);
        }
        else
        {
            DEBUG_PRINT("OMX_FillThisBuffer success!\n");
            free_op_buf_cnt--;
        }
    }
    DEBUG_PRINT("OP PORT ENABLED!\n");
    return 0;
}

int output_port_reconfig()
{
    DEBUG_PRINT("PORT_SETTING_CHANGE_STATE\n");
    if (disable_output_port() != 0)
	    return -1;

    /* Port for which the Client needs to obtain info */
    portFmt.nPortIndex = 1;
    OMX_GetParameter(dec_handle,OMX_IndexParamPortDefinition,&portFmt);
    DEBUG_PRINT("Min Buffer Count=%d", portFmt.nBufferCountMin);
    DEBUG_PRINT("Buffer Size=%d", portFmt.nBufferSize);
    if(OMX_DirOutput != portFmt.eDir) {
        DEBUG_PRINT_ERROR("Error - Expect Output Port\n");
        return -1;
    }
    height = portFmt.format.video.nFrameHeight;
    width = portFmt.format.video.nFrameWidth;
    stride = portFmt.format.video.nStride;
    sliceheight = portFmt.format.video.nSliceHeight;

    if (displayYuv == 2)
    {
      DEBUG_PRINT("Reconfiguration at middle of playback...");
      close_display();
      if (open_display() != 0)
      {
        printf("\n Error opening display! Video won't be displayed...");
        displayYuv = 0;
      }
    }

    if (displayYuv)
        overlay_set();

    if (enable_output_port() != 0)
	    return -1;
    DEBUG_PRINT("PORT_SETTING_CHANGE DONE!\n");
	return 0;
}

void free_output_buffers()
{
    int index = 0;
    OMX_BUFFERHEADERTYPE *pBuffer = (OMX_BUFFERHEADERTYPE *)pop(fbd_queue);
    while (pBuffer) {
        DEBUG_PRINT("\n pOutYUVBufHdrs %p p_eglHeaders %p output_use_buffer %d",
               pOutYUVBufHdrs,p_eglHeaders,output_use_buffer);
        if(pOutYUVBufHdrs && p_eglHeaders && output_use_buffer)
        {
           index = pBuffer - pOutYUVBufHdrs[0];
           DEBUG_PRINT("\n Index of free buffer %d",index);
           DEBUG_PRINT("\n Address freed %p size freed %d",pBuffer->pBuffer,
                  pBuffer->nAllocLen);
           munmap((void *)use_buf_virt_addr[index],pBuffer->nAllocLen);
           if(p_eglHeaders[index])
           {
               close(p_eglHeaders[index]->pmem_fd);
               free(p_eglHeaders[index]);
               p_eglHeaders[index] = NULL;
           }
        }

        if (pOutYUVBufHdrs && use_external_pmem_buf)
        {
            index = pBuffer - pOutYUVBufHdrs[0];
            DEBUG_PRINT("\n Address freed %p size freed %d,virt=0x%x,pmem_fd=0x%x",
                              pBuffer->pBuffer,
                              pBuffer->nAllocLen,
                              use_buf_virt_addr[index],
                              pPMEMInfo[index].pmem_fd);
            munmap((void *)use_buf_virt_addr[index],pBuffer->nAllocLen);
            getFreePmem();
            use_buf_virt_addr[index] = -1;
            if (&pPMEMInfo[index])
            {
                close(pPMEMInfo[index].pmem_fd);
                pPMEMInfo[index].pmem_fd = -1;
            }
        }
        DEBUG_PRINT("\n Free output buffer");
        OMX_FreeBuffer(dec_handle, 1, pBuffer);
        pBuffer = (OMX_BUFFERHEADERTYPE *)pop(fbd_queue);
    }
}

#ifndef USE_ION
static bool align_pmem_buffers(int pmem_fd, OMX_U32 buffer_size,
                                  OMX_U32 alignment)
{
  struct pmem_allocation allocation;
  allocation.size = buffer_size;
  allocation.align = clip2(alignment);

  if (allocation.align < 4096)
  {
    allocation.align = 4096;
  }
  if (ioctl(pmem_fd, PMEM_ALLOCATE_ALIGNED, &allocation) < 0)
  {
    DEBUG_PRINT_ERROR("\n Aligment failed with pmem driver");
    return false;
  }
  return true;
}
#endif

int open_display()
{
#ifdef _ANDROID_
  DEBUG_PRINT("\n Opening /dev/graphics/fb0");
  fb_fd = open("/dev/graphics/fb0", O_RDWR);
#else
  DEBUG_PRINT("\n Opening /dev/fb0");
  fb_fd = open("/dev/fb0", O_RDWR);
#endif
  if (fb_fd < 0) {
    printf("[omx_vdec_test] - ERROR - can't open framebuffer!\n");
    return -1;
  }

  DEBUG_PRINT("\n fb_fd = %d", fb_fd);
  if (ioctl(fb_fd, FBIOGET_FSCREENINFO, &finfo) < 0)
  {
    printf("[omx_vdec_test] - ERROR - can't retrieve fscreenInfo!\n");
    close(fb_fd);
    return -1;
  }
  if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo) < 0)
  {
    printf("[omx_vdec_test] - ERROR - can't retrieve vscreenInfo!\n");
    close(fb_fd);
    return -1;
  }
  return 0;
}

void close_display()
{
  overlay_unset();
  close(fb_fd);
  fb_fd = -1;
}

void getFreePmem()
{
#ifndef USE_ION
   int ret = -1;
   /*Open pmem device and query free pmem*/
   int pmem_fd = open (PMEM_DEVICE,O_RDWR);

   if(pmem_fd < 0) {
     ALOGE("Unable to open pmem device");
     return;
   }
   struct pmem_freespace fs;
   ret = ioctl(pmem_fd, PMEM_GET_FREE_SPACE, &fs);
   if(ret) {
     ALOGE("IOCTL to query pmem free space failed");
     goto freespace_query_failed;
   }
   ALOGE("Available free space %lx largest chunk %lx\n", fs.total, fs.largest);
freespace_query_failed:
   close(pmem_fd);
#endif
}
