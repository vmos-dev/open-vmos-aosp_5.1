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
#include <stdio.h>
#include <stdlib.h>
#include "queue.h"
#include<fcntl.h>
#include<sys/ioctl.h>
#include <sys/mman.h>
#include <linux/msm_vidc_enc.h>
#include<pthread.h>
#include <semaphore.h>
#include <stdio.h>

#define INPUT_BUFFER 0
#define OUTPUT_BUFFER 1

struct video_encoder_context
{
	unsigned long   input_width;
  unsigned long   input_height;
	unsigned long   codectype;
  unsigned long   fps_num;
  unsigned long   fps_den;
  unsigned long   targetbitrate;
  unsigned long   inputformat;

	struct venc_allocatorproperty input_buffer;
  struct venc_allocatorproperty output_buffer;
	struct venc_bufferpayload     **ptr_inputbuffer;
	struct venc_bufferpayload     **ptr_outputbuffer;
	struct video_queue_context    queue_context;
	int                           video_driver_fd;

	FILE * inputBufferFile;
	FILE * outputBufferFile;

  pthread_t videothread_id;
	pthread_t asyncthread_id;
	sem_t sem_synchronize;
};

int init_encoder ( struct video_encoder_context *init_decode );
int allocate_buffer ( unsigned int buffer_dir,
					            struct video_encoder_context *decode_context
					          );
int free_buffer ( unsigned int buffer_dir,
				          struct video_encoder_context *decode_context
				         );
int start_encoding (struct video_encoder_context *decode_context);
int stop_encoding  (struct video_encoder_context *decode_context);
int deinit_encoder (struct video_encoder_context *init_decode);
