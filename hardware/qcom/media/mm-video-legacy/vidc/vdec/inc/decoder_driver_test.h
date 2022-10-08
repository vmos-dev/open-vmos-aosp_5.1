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
#include "message_queue.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <inttypes.h>
#include <linux/msm_vidc_dec.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>

struct video_decoder_context
{
    enum vdec_codec               decoder_format;
    enum vdec_output_fromat       output_format;
    struct vdec_picsize           video_resoultion;
    struct vdec_allocatorproperty input_buffer;
    struct vdec_allocatorproperty output_buffer;
    struct vdec_bufferpayload     **ptr_inputbuffer;
    struct vdec_bufferpayload     **ptr_outputbuffer;
    struct vdec_output_frameinfo  **ptr_respbuffer;
    struct video_queue_context    queue_context;
    int                           video_driver_fd;

    FILE * inputBufferFile;
    FILE * outputBufferFile;

    pthread_t videothread_id;
    pthread_t asyncthread_id;
    sem_t sem_synchronize;
};

int init_decoder ( struct video_decoder_context *init_decode );
int allocate_buffer ( enum vdec_buffer,
                      struct video_decoder_context *decode_context
                     );
int free_buffer ( enum vdec_buffer,
                  struct video_decoder_context *decode_context
                 );
int start_decoding (struct video_decoder_context *decode_context);
int stop_decoding  (struct video_decoder_context *decode_context);
int deinit_decoder (struct video_decoder_context *init_decode);
