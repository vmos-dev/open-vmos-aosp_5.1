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
#include "video_encoder_test.h"

#define DEBUG_PRINT printf
/************************************************************************/
/*        #DEFINES                          */
/************************************************************************/

#define VOP_START_CODE 0x000001B6
#define SHORT_HEADER_START_CODE 0x00008000
#define H264_START_CODE         0x00000001

/************************************************************************/
/*        STATIC VARIABLES                          */
/************************************************************************/

static int Code_type;
static int total_frames = 0;
static unsigned int header_code = 0;
static pthread_mutex_t read_lock;

static unsigned int read_frame ( unsigned char *dataptr,unsigned int length,
                                 FILE * inputBufferFile
                                );
static unsigned clp2(unsigned x)
{
        x = x - 1;
        x = x | (x >> 1);
        x = x | (x >> 2);
        x = x | (x >> 4);
        x = x | (x >> 8);
        x = x | (x >>16);
        return x + 1;
}


static void* video_thread (void *);
static void* async_thread (void *);



int main (int argc, char **argv)
{
  struct video_encoder_context *encoder_context = NULL;
  char *file_name = NULL;
  FILE *file_ptr = NULL;
  int temp1 =0,temp2 =0;
  int error = 1;
  unsigned int i = 0;

  file_name = argv [1];
  file_ptr = fopen (file_name,"rb");

  if (file_ptr == NULL)
  {
    DEBUG_PRINT("\n File is not located ");
    return -1;
  }


  encoder_context = (struct video_encoder_context *) \
                   calloc (sizeof (struct video_encoder_context),1);
  if (encoder_context == NULL)
  {
    return -1;
  }
  encoder_context->outputBufferFile = NULL;
  encoder_context->inputBufferFile = NULL;
  encoder_context->video_driver_fd = -1;
  encoder_context->inputBufferFile = file_ptr;
  encoder_context->input_width = 176;
  encoder_context->input_height = 144;
  encoder_context->codectype = VEN_CODEC_MPEG4;
  encoder_context->fps_num = 60;
  encoder_context->fps_den = 2;
  encoder_context->inputformat = VEN_INPUTFMT_NV12;
  encoder_context->targetbitrate = 128000;

  file_ptr = fopen ("/data/output.m4v","wb");
  if (file_ptr == NULL)
  {
    DEBUG_PRINT("\n File can't be created");
    free (encoder_context);
    return -1;
  }
  encoder_context->outputBufferFile = file_ptr;

   switch (atoi(argv[2]))
   {
   case 0:
     DEBUG_PRINT("\n MPEG4 codec selected");
     encoder_context->codectype = VEN_CODEC_MPEG4;
     Code_type = 0;
     break;
   case 1:
     DEBUG_PRINT("\n H.263");
     encoder_context->codectype = VEN_CODEC_H263;
     Code_type = 0;
     break;
   case 2:
     DEBUG_PRINT("\n H.264");
     encoder_context->codectype = VEN_CODEC_H264;
     Code_type = 1;
     break;
   default:
     DEBUG_PRINT("\n Wrong codec type");
     error = -1;
     break;
   }

   if (error != -1)
   {
     temp1 = atoi(argv[3]);
     temp2 = atoi(argv[4]);

     if (((temp1%16) != 0) || ((temp2%16) != 0))
     {
       error = -1;
     }
     else
     {
      encoder_context->input_width = temp1;
      encoder_context->input_height = temp2;
     }
   }

   switch (atoi(argv[5]))
   {
   case 0:
     DEBUG_PRINT("\n No Sink");
     encoder_context->outputBufferFile = NULL;
     break;
   }

   if (error != -1)
   {
     encoder_context->targetbitrate = atoi (argv[6]);
   }

   if ( error != -1 && (init_encoder (encoder_context) == -1 ))
   {
      DEBUG_PRINT("\n Init decoder fails ");
      error = -1;
   }
   DEBUG_PRINT("\n Decoder open successfull");


   /*Allocate input and output buffers*/
   if (error != -1 && (allocate_buffer (0,encoder_context)== -1))
   {
     DEBUG_PRINT("\n Error in input Buffer allocation");
     error = -1;
   }

   if (error != -1 && (allocate_buffer (1,encoder_context)== -1))
   {
     DEBUG_PRINT("\n Error in output Buffer allocation");
     error = -1;
   }


   if (error != -1 && (start_encoding (encoder_context) == -1))
   {
     DEBUG_PRINT("\n Error in start decoding call");
     error = -1;
   }

   if (error != -1 && (stop_encoding (encoder_context) == -1))
   {
     DEBUG_PRINT("\n Error in stop decoding call");
     error = -1;
   }

   DEBUG_PRINT("\n De-init the decoder");
   if ((deinit_encoder (encoder_context) == -1))
   {
      error = -1;
   }


  (void)free_buffer (INPUT_BUFFER,encoder_context);
  (void)free_buffer (OUTPUT_BUFFER,encoder_context);

  if (encoder_context->inputBufferFile != NULL)
  {
   fclose (encoder_context->inputBufferFile);
  }
  if (encoder_context->outputBufferFile != NULL)
  {
    fclose (encoder_context->outputBufferFile);
  }
  DEBUG_PRINT ("\n Total Number of frames decoded %d",total_frames);
  DEBUG_PRINT("\n closing the driver");
  free (encoder_context);

  return error;
}

int init_encoder ( struct video_encoder_context *init_decode )
{
  struct venc_ioctl_msg ioctl_msg = {NULL,NULL};
  struct venc_basecfg basecfg;
  struct video_queue_context *queue_ptr = NULL;
  struct venc_ratectrlcfg ratecrl;
  pthread_mutexattr_t init_values;
  struct venc_profile profile;
  struct ven_profilelevel profilelevel;

  DEBUG_PRINT("\n Before calling the open");

  init_decode->video_driver_fd = open ("/dev/msm_vidc_enc", \
                     O_RDWR | O_NONBLOCK);



  if (init_decode->video_driver_fd < 0)
  {
    DEBUG_PRINT("\n Open failed");
    return -1;
  }

  basecfg.codectype = init_decode->codectype;
  basecfg.dvs_height = 0;
  basecfg.dvs_width = 0;
  basecfg.fps_den = init_decode->fps_den;
  basecfg.fps_num = init_decode->fps_num;
  basecfg.input_height = init_decode->input_height;
  basecfg.input_width = init_decode->input_width;
  basecfg.inputformat = init_decode->inputformat;
  basecfg.targetbitrate = init_decode->targetbitrate;

  /*Initialize Decoder with codec type and resolution*/
  ioctl_msg.in = &basecfg;
  ioctl_msg.out = NULL;

  if (ioctl (init_decode->video_driver_fd,VEN_IOCTL_SET_BASE_CFG,
         (void*)&ioctl_msg) < 0)
  {
    DEBUG_PRINT("\n Set base config type failed");
    return -1;
  }

  /*Initialize Decoder with codec type and resolution*/
  DEBUG_PRINT ("\n Switch off rate control");
  ioctl_msg.in = &ratecrl;
  ioctl_msg.out = NULL;
  ratecrl.rcmode = VEN_RC_OFF;
  if (ioctl (init_decode->video_driver_fd,VEN_IOCTL_SET_RATE_CTRL_CFG,
         (void*)&ioctl_msg) < 0)
  {
    DEBUG_PRINT("\n Set rate control failed");
    return -1;
  }

  if (basecfg.codectype == VEN_CODEC_H264)
  {
    DEBUG_PRINT ("\n Set the VEN_IOCTL_SET_CODEC_PROFILE High");
    ioctl_msg.in = &profile;
    ioctl_msg.out = NULL;
    profile.profile = VEN_PROFILE_H264_BASELINE;
    if (ioctl (init_decode->video_driver_fd,VEN_IOCTL_SET_CODEC_PROFILE,
           (void*)&ioctl_msg) < 0)
    {
      DEBUG_PRINT("\n Set VEN_IOCTL_SET_CODEC_PROFILE failed");
      return -1;
    }

    DEBUG_PRINT ("\n Set the VEN_IOCTL_SET_CODEC_PROFILE High");
    ioctl_msg.in = &profilelevel;
    ioctl_msg.out = NULL;
    profilelevel.level = VEN_LEVEL_H264_1p1;
    if (ioctl (init_decode->video_driver_fd,VEN_IOCTL_SET_PROFILE_LEVEL,
           (void*)&ioctl_msg) < 0)
    {
      DEBUG_PRINT("\n Set VEN_IOCTL_SET_CODEC_PROFILE failed");
      return -1;
    }

    if (basecfg.input_width > 720)
    {
      DEBUG_PRINT ("\n Set the VEN_IOCTL_SET_CODEC_PROFILE High");
      ioctl_msg.in = &profile;
      ioctl_msg.out = NULL;
      profile.profile = VEN_PROFILE_H264_HIGH;
      if (ioctl (init_decode->video_driver_fd,VEN_IOCTL_SET_CODEC_PROFILE,
             (void*)&ioctl_msg) < 0)
      {
        DEBUG_PRINT("\n Set VEN_IOCTL_SET_CODEC_PROFILE failed");
        return -1;
      }

      DEBUG_PRINT ("\n Set the VEN_IOCTL_SET_CODEC_PROFILE High");
      ioctl_msg.in = &profilelevel;
      ioctl_msg.out = NULL;
      profilelevel.level = VEN_LEVEL_H264_3p1;
      if (ioctl (init_decode->video_driver_fd,VEN_IOCTL_SET_PROFILE_LEVEL,
             (void*)&ioctl_msg) < 0)
      {
        DEBUG_PRINT("\n Set VEN_IOCTL_SET_CODEC_PROFILE failed");
        return -1;
      }
    }
  }

  DEBUG_PRINT("\n Query Input bufffer requirements");
  /*Get the Buffer requirements for input and output ports*/



  ioctl_msg.in = NULL;
  ioctl_msg.out = &init_decode->input_buffer;

  if (ioctl (init_decode->video_driver_fd,VEN_IOCTL_GET_INPUT_BUFFER_REQ,
         (void*)&ioctl_msg) < 0)
  {
    DEBUG_PRINT("\n Requesting for input buffer requirements failed");
    return -1;
  }

  DEBUG_PRINT("\n input Size=%d min count =%d actual count = %d", \
              (int)init_decode->input_buffer.datasize,\
              (int)init_decode->input_buffer.mincount,\
              (int)init_decode->input_buffer.actualcount);


  ioctl_msg.in = &init_decode->input_buffer;
  ioctl_msg.out = NULL;
  init_decode->input_buffer.actualcount = init_decode->input_buffer.mincount + 2;

  if (ioctl (init_decode->video_driver_fd,VEN_IOCTL_SET_INPUT_BUFFER_REQ,
         (void*)&ioctl_msg) < 0)
  {
    DEBUG_PRINT("\n Set Buffer Requirements Failed");
    return -1;
  }


  DEBUG_PRINT("\n Query output bufffer requirements");
  ioctl_msg.in = NULL;
  ioctl_msg.out = &init_decode->output_buffer;

  if (ioctl (init_decode->video_driver_fd,VEN_IOCTL_GET_OUTPUT_BUFFER_REQ,
         (void*)&ioctl_msg) < 0)
  {
    DEBUG_PRINT("\n Requesting for output buffer requirements failed");
    return -1;
  }

  DEBUG_PRINT("\n output Size=%d min count =%d actual count = %d", \
              (int)init_decode->output_buffer.datasize,\
              (int)init_decode->output_buffer.mincount,\
              (int)init_decode->output_buffer.actualcount);

  /*Create Queue related data structures*/
  queue_ptr = &init_decode->queue_context;
  queue_ptr->commandq_size = 50;
  queue_ptr->dataq_size = 50;

  sem_init(&queue_ptr->sem_message,0, 0);
  sem_init(&init_decode->sem_synchronize,0, 0);

  pthread_mutexattr_init (&init_values);
  pthread_mutex_init (&queue_ptr->mutex,&init_values);
  pthread_mutex_init (&read_lock,&init_values);
  DEBUG_PRINT("\n create Queues");
  queue_ptr->ptr_cmdq = (struct video_msgq*) \
                        calloc (sizeof (struct video_msgq),
                  queue_ptr->commandq_size);
  queue_ptr->ptr_dataq = (struct video_msgq*) \
              calloc (sizeof (struct video_msgq),
                  queue_ptr->dataq_size
                  );

  if ( queue_ptr->ptr_cmdq == NULL ||
     queue_ptr->ptr_dataq == NULL
    )
  {
    return -1;
  }
  DEBUG_PRINT("\n create Threads");
  /*Create two threads*/
    if ( (pthread_create (&init_decode->videothread_id,NULL,video_thread,
            init_decode) < 0) ||
         (pthread_create (&init_decode->asyncthread_id,NULL,async_thread,
            init_decode) < 0)
    )
  {
    return -1;
  }

  return 1;
}



int free_buffer ( unsigned int  buffer_dir,
                  struct video_encoder_context *encoder_context
                 )
{
  unsigned int buffercount = 0,i=0;
  struct venc_bufferpayload **ptemp = NULL;

  if (encoder_context == NULL)
  {
    return -1;
  }

  if (buffer_dir == INPUT_BUFFER && encoder_context->ptr_inputbuffer)
  {
      buffercount = encoder_context->input_buffer.actualcount;
      ptemp = encoder_context->ptr_inputbuffer;

    for (i=0;i<buffercount;i++)
    {
      if (ptemp [i])
      {
        if (ptemp [i]->fd != -1)
        {
          munmap ( ptemp [i]->pbuffer,ptemp [i]->maped_size);
          ptemp [i]->pbuffer = NULL;
          close (ptemp [i]->fd);
        }
        free (ptemp [i]);
        ptemp [i] = NULL;
      }
    }
    free (encoder_context->ptr_inputbuffer);
    encoder_context->ptr_inputbuffer = NULL;
  }
  else if ( buffer_dir == OUTPUT_BUFFER && encoder_context->ptr_outputbuffer )
  {
    buffercount = encoder_context->output_buffer.actualcount;
    ptemp = encoder_context->ptr_outputbuffer;

    if (ptemp)
    {
      for (i=0;i<buffercount;i++)
      {
        if (ptemp [i])
        {
          if (ptemp [i]->fd != -1)
          {
            munmap ( ptemp [i]->pbuffer,ptemp [i]->maped_size);
            ptemp [i]->pbuffer = NULL;
            close (ptemp [i]->fd);
          }
          free (ptemp [i]);
          ptemp [i] = NULL;
        }
      }
      free (ptemp);
      encoder_context->ptr_outputbuffer = NULL;
    }
  }

  return 1;
}

int allocate_buffer ( unsigned int buffer_dir,
                      struct video_encoder_context *encoder_context
                    )
{
  struct venc_bufferpayload **ptemp = NULL;
  struct venc_ioctl_msg ioctl_msg = {NULL,NULL};
  unsigned int buffercount = 0,i=0,alignedsize=0;
  unsigned int buffersize = 0;

  if ( encoder_context == NULL)
  {
    DEBUG_PRINT ("\nallocate_buffer: context is NULL");
    return -1;
  }

  if ( buffer_dir == INPUT_BUFFER )
  {
        /*Check if buffers are allocated*/
    if (encoder_context->ptr_inputbuffer != NULL)
    {
      DEBUG_PRINT ("\nallocate_buffer: encoder_context->ptr_inputbuffer is set");
      return -1;
    }

    buffercount = encoder_context->input_buffer.actualcount;
    alignedsize = encoder_context->input_buffer.alignment;
    buffersize = encoder_context->input_buffer.datasize;
    buffersize = (buffersize + alignedsize) & (~alignedsize);
  }
  else if (buffer_dir == OUTPUT_BUFFER)
  {
    /*Check if buffers are allocated*/
    if (encoder_context->ptr_outputbuffer != NULL)
    {
      DEBUG_PRINT ("\nallocate_buffer: Double allcoate output");
      return -1;
    }

    buffercount = encoder_context->output_buffer.actualcount;
    alignedsize = encoder_context->output_buffer.alignment;
    buffersize = encoder_context->output_buffer.datasize;
    buffersize = (buffersize + alignedsize) & (~alignedsize);

  }
  else
  {
    DEBUG_PRINT ("\nallocate_buffer: Wrong buffer directions");
    return -1;
  }

  ptemp = (struct venc_bufferpayload **)\
  calloc (sizeof (struct venc_bufferpayload *),buffercount);

  if (ptemp == NULL)
  {
    DEBUG_PRINT ("\nallocate_buffer: venc_bufferpayload failure");
    return -1;
  }


  if (buffer_dir == OUTPUT_BUFFER)
  {
    DEBUG_PRINT ("\nallocate_buffer: OUT");
    encoder_context->ptr_outputbuffer = ptemp;
  }
  else
  {
    DEBUG_PRINT ("\nallocate_buffer: IN");
    encoder_context->ptr_inputbuffer = ptemp;
  }

  /*Allocate buffer headers*/
  for (i=0; i< buffercount; i++)
  {
    ptemp [i] = (struct venc_bufferpayload*)\
    calloc (sizeof (struct venc_bufferpayload),1);

    if (ptemp [i] == NULL)
    {
      DEBUG_PRINT ("\nallocate_buffer: ptemp [i] calloc failure");
      return -1;
    }
    ptemp [i]->fd = -1;
  }

  for (i=0; i< buffercount; i++)
  {
    ptemp [i]->fd = open ("/dev/pmem_adsp",O_RDWR);

    if (ptemp [i]->fd < 0)
    {
      DEBUG_PRINT ("\nallocate_buffer: open pmem_adsp failed");
      return -1;
    }

    ptemp [i]->pbuffer = mmap(NULL,clp2(buffersize),PROT_READ|PROT_WRITE,
                                 MAP_SHARED,ptemp [i]->fd,0);
    DEBUG_PRINT ("\n pmem fd = %d virt addr = %p",ptemp [i]->fd,\
                  ptemp [i]->pbuffer);
    if (ptemp [i]->pbuffer == MAP_FAILED)
    {
      ptemp [i]->pbuffer = NULL;
      DEBUG_PRINT ("\nallocate_buffer: MMAP failed");
      return -1;
    }
    ptemp [i]->sz = buffersize;
    ptemp [i]->maped_size = clp2 (buffersize);

    ioctl_msg.in  = ptemp [i];
    ioctl_msg.out = NULL;

    if (buffer_dir == OUTPUT_BUFFER)
    {
      if (ioctl (encoder_context->video_driver_fd,VEN_IOCTL_SET_OUTPUT_BUFFER,
           &ioctl_msg) < 0)
      {
        DEBUG_PRINT ("\nallocate_buffer: Set Output Buffer IOCTL failed");
        return -1;
      }
    }
    else
    {
      if (ioctl (encoder_context->video_driver_fd,VEN_IOCTL_SET_INPUT_BUFFER,
           &ioctl_msg) < 0)
      {
        DEBUG_PRINT ("\nallocate_buffer: Set input Buffer IOCTL failed");
        return -1;
      }
    }

  }
  DEBUG_PRINT ("\nallocate_buffer: Success");
  return 1;
}



int start_encoding (struct video_encoder_context *encoder_context)
{
  struct venc_ioctl_msg ioctl_msg = {NULL,NULL};
  struct venc_buffer enc_buffer;
  unsigned int i = 0;
  unsigned int data_len =0;


  if (encoder_context == NULL)
  {
    return -1;
  }

  if (ioctl (encoder_context->video_driver_fd,VEN_IOCTL_CMD_START,
         NULL) < 0)
  {
    DEBUG_PRINT("\n Start failed");
    return -1;
  }

  DEBUG_PRINT("\n Start Issued successfully waiting for Start Done");
  /*Wait for Start command response*/
  sem_wait (&encoder_context->sem_synchronize);

  /*Push output Buffers*/
  i = 0;
  while (i < encoder_context->output_buffer.actualcount)
  {
    enc_buffer.clientdata = (void *)encoder_context->ptr_outputbuffer [i];
    enc_buffer.flags = 0;
    enc_buffer.sz = encoder_context->ptr_outputbuffer [i]->sz;
    enc_buffer.len = 0;
    enc_buffer.ptrbuffer = encoder_context->ptr_outputbuffer [i]->pbuffer;
    enc_buffer.offset = 0;
    enc_buffer.timestamp = 0;

    DEBUG_PRINT ("\n Client Data on output = %p",(void *)enc_buffer.clientdata);
    ioctl_msg.in = &enc_buffer;
    ioctl_msg.out = NULL;

    if (ioctl (encoder_context->video_driver_fd,
           VEN_IOCTL_CMD_FILL_OUTPUT_BUFFER,&ioctl_msg) < 0)
    {
      DEBUG_PRINT("\n fill output frame failed");
      return -1;
    }
    i++;
  }


  /*push input buffers*/
  i = 0;
  while (i < encoder_context->input_buffer.actualcount)
  {
    DEBUG_PRINT("\n Read  Frame from File");

    enc_buffer.clientdata = (void *)encoder_context->ptr_inputbuffer [i];
    enc_buffer.flags = 0;
    enc_buffer.sz = encoder_context->ptr_inputbuffer [i]->sz;
    enc_buffer.len = 0;
    enc_buffer.ptrbuffer = encoder_context->ptr_inputbuffer [i]->pbuffer;
    enc_buffer.offset = 0;
    enc_buffer.timestamp = total_frames *
                ((encoder_context->fps_den * 1000000)/encoder_context->fps_num);
    enc_buffer.len = (encoder_context->input_height *
                     encoder_context->input_width *3)/2;
    data_len = read_frame ( enc_buffer.ptrbuffer,
                            enc_buffer.len,
                            encoder_context->inputBufferFile);
    if (data_len == 0)
    {
      DEBUG_PRINT("\n Length is zero error");
      return -1;
    }
    enc_buffer.len = data_len;
    DEBUG_PRINT("\n Read  Frame from File szie = %d",(int)data_len);

    DEBUG_PRINT ("\n Client Data on output = %p",(void *)enc_buffer.clientdata);
    ioctl_msg.in = &enc_buffer;
    ioctl_msg.out = NULL;

    if (ioctl (encoder_context->video_driver_fd,
           VEN_IOCTL_CMD_ENCODE_FRAME,&ioctl_msg) < 0)
    {
      DEBUG_PRINT("\n Encode input frame failed");
      return -1;
    }
    total_frames++;
    i++;
  }
  DEBUG_PRINT ("\n Wait for EOS");
  /*Wait for EOS or Error condition*/
  sem_wait (&encoder_context->sem_synchronize);
  DEBUG_PRINT ("\n Reached EOS");

  return 1;
}

int stop_encoding  (struct video_encoder_context *encoder_context)
{
  struct venc_ioctl_msg ioctl_msg = {NULL,NULL};
  struct venc_bufferflush buffer_flush;

  if (encoder_context == NULL)
  {
    return -1;
  }
  buffer_flush.flush_mode = VEN_FLUSH_INPUT;
  ioctl_msg.in = &buffer_flush;
  ioctl_msg.out = NULL;

  if (ioctl(encoder_context->video_driver_fd,VEN_IOCTL_CMD_FLUSH,
         &ioctl_msg) < 0)
  {
    DEBUG_PRINT("\n Flush input failed");
  }
  else
  {
       sem_wait (&encoder_context->sem_synchronize);
  }

  buffer_flush.flush_mode = VEN_FLUSH_OUTPUT;
  ioctl_msg.in = &buffer_flush;
  ioctl_msg.out = NULL;

  if (ioctl(encoder_context->video_driver_fd,VEN_IOCTL_CMD_FLUSH,
            &ioctl_msg) < 0)
  {
    DEBUG_PRINT("\n Flush output failed");
  }
  else
  {
     sem_wait (&encoder_context->sem_synchronize);
  }

  DEBUG_PRINT("\n Stop VEN_IOCTL_CMD_STOP");
  if (ioctl(encoder_context->video_driver_fd,VEN_IOCTL_CMD_STOP,NULL) < 0)
  {
    DEBUG_PRINT("\n Stop failed");
  }
  else
  {
     sem_wait (&encoder_context->sem_synchronize);
  }
  return 1;
}

int deinit_encoder (struct video_encoder_context *init_decode)
{
  if (init_decode == NULL)
  {
    return -1;
  }

  /*Close the driver*/
  if (init_decode->video_driver_fd != -1)
  {
    close (init_decode->video_driver_fd);
  }

  if (init_decode->queue_context.ptr_cmdq)
  {
    free (init_decode->queue_context.ptr_cmdq);
    init_decode->queue_context.ptr_cmdq = NULL;
  }

  if (init_decode->queue_context.ptr_dataq)
  {
    free (init_decode->queue_context.ptr_dataq);
    init_decode->queue_context.ptr_dataq = NULL;
  }

  sem_destroy (&init_decode->queue_context.sem_message);
  sem_destroy (&init_decode->sem_synchronize);

  pthread_mutex_destroy(&init_decode->queue_context.mutex);
  pthread_mutex_destroy (&read_lock);

  return 1;
}

static void* video_thread (void *context)
{
   struct video_encoder_context *encoder_context = NULL;
   struct video_msgq *queueitem = NULL;
   struct venc_ioctl_msg ioctl_msg = {NULL,NULL};
   struct venc_bufferpayload *tempbuffer = NULL;
   struct venc_buffer enc_buffer;
   unsigned int data_len =0;


   if (context == NULL)
   {
     DEBUG_PRINT("\n video thread recieved NULL context");
     return NULL;
   }
   encoder_context = (struct video_encoder_context *) context;

   /* Thread function which will accept commands from async thread
    * or main thread
   */
   while (1)
   {
      queueitem = queue_get_cmd (&encoder_context ->queue_context);
      if (queueitem != NULL)
      {
        switch (queueitem->cmd)
        {
        case VEN_MSG_START:
          DEBUG_PRINT("\n recived start done command");
            sem_post (&encoder_context->sem_synchronize);
          break;

        case VEN_MSG_STOP:
          DEBUG_PRINT("\n recieved stop done");
          sem_post (&encoder_context->sem_synchronize);
          break;

        case VEN_MSG_INPUT_BUFFER_DONE:

          tempbuffer = (struct venc_bufferpayload *)queueitem->clientdata;
          if (tempbuffer == NULL)
          {
            DEBUG_PRINT("\n FATAL ERROR input buffer address is bad");
            sem_post (&encoder_context->sem_synchronize);
            break;
          }
          tempbuffer->filled_len = (encoder_context->input_height *
                             encoder_context->input_width *3)/2;

          data_len = read_frame ( tempbuffer->pbuffer,
                                  tempbuffer->filled_len,
                                  encoder_context->inputBufferFile);

          if (data_len == 0)
          {
            DEBUG_PRINT ("\n End of stream reached");
            sem_post (&encoder_context->sem_synchronize);
            break;
          }
          enc_buffer.clientdata = (void *)tempbuffer;
          enc_buffer.flags = 0;
          enc_buffer.ptrbuffer = tempbuffer->pbuffer;
          enc_buffer.sz = tempbuffer->sz;
          enc_buffer.len = tempbuffer->filled_len;
          enc_buffer.offset = 0;
          enc_buffer.timestamp = total_frames *
                ((encoder_context->fps_den * 1000000)/encoder_context->fps_num);

          /*TODO: Time stamp needs to be updated*/
          ioctl_msg.in = &enc_buffer;
          ioctl_msg.out = NULL;
          total_frames++;
          if (ioctl(encoder_context->video_driver_fd,VEN_IOCTL_CMD_ENCODE_FRAME,
               &ioctl_msg) < 0)
          {
            DEBUG_PRINT("\n Decoder frame failed");
            sem_post (&encoder_context->sem_synchronize);
          }
          DEBUG_PRINT("\n Input buffer done send next buffer current value = %d",\
                      total_frames);
          break;

        case VEN_MSG_OUTPUT_BUFFER_DONE:

          tempbuffer = (struct venc_bufferpayload *)queueitem->clientdata;
          if (tempbuffer == NULL)
          {
            DEBUG_PRINT("\n FATAL ERROR input buffer address is bad");
            sem_post (&encoder_context->sem_synchronize);
            break;
          }

         if (encoder_context->outputBufferFile != NULL)
         {
           fwrite (tempbuffer->pbuffer,1,tempbuffer->filled_len,
                encoder_context->outputBufferFile);
         }


         DEBUG_PRINT("\n recieved output buffer consume outbuffer");
         DEBUG_PRINT("\nValues outputbuffer->bufferaddr = %p",\
                     tempbuffer->pbuffer);
         enc_buffer.clientdata = (void *)tempbuffer;
         enc_buffer.flags = 0;
         enc_buffer.sz = tempbuffer->sz;
         enc_buffer.len = 0;
         enc_buffer.ptrbuffer = tempbuffer->pbuffer;
         enc_buffer.offset = 0;
         enc_buffer.timestamp = 0;

         ioctl_msg.in = &enc_buffer;
         ioctl_msg.out = NULL;

         if (ioctl (encoder_context->video_driver_fd,
              VEN_IOCTL_CMD_FILL_OUTPUT_BUFFER,&ioctl_msg) < 0)
         {
           DEBUG_PRINT("\n Decoder frame failed");
           return NULL;
         }

         break;

        case VEN_MSG_FLUSH_INPUT_DONE:
          DEBUG_PRINT("\n Flush input complete");
          sem_post (&encoder_context->sem_synchronize);
          break;

        case VEN_MSG_FLUSH_OUPUT_DONE:
          DEBUG_PRINT("\n Flush output complete");
          sem_post (&encoder_context->sem_synchronize);
          break;
        }

        if (queueitem->cmd == VEN_MSG_STOP)
        {
          DEBUG_PRINT("\n Playback has ended thread will exit");
          return NULL;
        }
      }
      else
      {
        DEBUG_PRINT("\n Error condition recieved NULL from Queue");
      }

   }
}

static void* async_thread (void *context)
{
  struct video_encoder_context *encoder_context = NULL;
  struct video_msgq queueitem ;
  struct venc_msg venc_msg;
  struct venc_bufferpayload *tempbuffer = NULL;
  struct venc_ioctl_msg ioctl_msg = {NULL,NULL};
  int result = -1;

  if (context == NULL)
  {
    DEBUG_PRINT("\n aynsc thread recieved NULL context");
    return NULL;
  }
  encoder_context = (struct video_encoder_context *) context;
  DEBUG_PRINT("\n Entering the async thread");

  while (1)
  {
    ioctl_msg.in = NULL;
    ioctl_msg.out = (void*)&venc_msg;
    DEBUG_PRINT ("\n Sizeof venc_msginfo = %d ",sizeof (venc_msg));
    DEBUG_PRINT("\n Address of Venc msg in async thread %p",\
                ioctl_msg.out);
    if (ioctl (encoder_context->video_driver_fd,VEN_IOCTL_CMD_READ_NEXT_MSG,\
         (void*)&ioctl_msg) < 0)
    {
      DEBUG_PRINT("\n Error in ioctl read next msg");
    }
    else
    {
      switch (venc_msg.msgcode)
      {
      case VEN_MSG_START:
      case VEN_MSG_STOP:
      case VEN_MSG_INDICATION:
        DEBUG_PRINT("\nSTOP/START Indiacation");
        queueitem.cmd = venc_msg.msgcode;
        queueitem.status = venc_msg.statuscode;
        queueitem.clientdata = NULL;
        break;

      case VEN_MSG_INPUT_BUFFER_DONE:
        DEBUG_PRINT("\nINPUT buffer done Indiacation");
        queueitem.cmd = venc_msg.msgcode;
        queueitem.status = venc_msg.statuscode;
        queueitem.clientdata = (void *)venc_msg.buf.clientdata;
        DEBUG_PRINT("\nInput Client data pointer is %p",queueitem.clientdata);
        tempbuffer = (struct venc_bufferpayload *) queueitem.clientdata;
        DEBUG_PRINT ("\n Input Address of tempbuffer %p",tempbuffer);
        tempbuffer->filled_len = venc_msg.buf.len;
        DEBUG_PRINT ("\n Input value of tempbuffer tempbuffer->filled_len %d",(int)tempbuffer->filled_len);
        break;
      case VEN_MSG_OUTPUT_BUFFER_DONE:
        DEBUG_PRINT("\nOUPUT buffer done Indiacation");
        queueitem.cmd = venc_msg.msgcode;
        queueitem.status = venc_msg.statuscode;
        queueitem.clientdata = (void *)venc_msg.buf.clientdata;
        DEBUG_PRINT("\nOutput Client data pointer is %p",queueitem.clientdata);
        tempbuffer = (struct venc_bufferpayload *) queueitem.clientdata;
        DEBUG_PRINT ("\n Output Address of tempbuffer %p",tempbuffer);
        tempbuffer->filled_len = venc_msg.buf.len;
        DEBUG_PRINT ("\n Output value of tempbuffer tempbuffer->filled_len %d",(int)tempbuffer->filled_len);
        break;

      default:
        DEBUG_PRINT("\nIn Default of get next message %d",(int)venc_msg.msgcode);
        queueitem.cmd = venc_msg.msgcode;
        queueitem.status = venc_msg.statuscode;
        queueitem.clientdata = NULL;
        break;
      }
      result = queue_post_cmdq (&encoder_context->queue_context,&queueitem);
      while (result == 0)
      {
         result = queue_post_cmdq (&encoder_context->queue_context,&queueitem);
      }

      if (result == -1)
      {
        DEBUG_PRINT("\n FATAL ERROR WITH Queue");
      }
    }
    if (venc_msg.msgcode == VEN_MSG_STOP)
    {
      /*Thread can exit at this point*/
      return NULL;
    }
  }
}


static unsigned int read_frame (unsigned char *dataptr, unsigned int length,
                                FILE * inputBufferFile)
{

  unsigned int readOffset = 0;
  int bytes_read = 0;
  unsigned int code = 0;
  int found = 0;

  DEBUG_PRINT ("\n Inside the readframe");

  if (dataptr == NULL && length == 0)
  {
    DEBUG_PRINT ("\n dataptr = %p length = %d",dataptr,(int)length);
    return 0;
  }

  pthread_mutex_lock(&read_lock);
  bytes_read = fread(&dataptr[readOffset],1,length,inputBufferFile);
  pthread_mutex_unlock(&read_lock);

  return bytes_read;
}
