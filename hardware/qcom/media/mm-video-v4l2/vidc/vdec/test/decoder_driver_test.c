/*--------------------------------------------------------------------------
Copyright (c) 2010-2011, 2013, The Linux Foundation. All rights reserved.

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
#include "decoder_driver_test.h"

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
static int Read_Buffer_From_DAT_File( unsigned char *dataptr, unsigned int length,
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
    struct video_decoder_context *decoder_context = NULL;
    char *file_name = NULL;
    FILE *file_ptr = NULL;
    int temp1 =0,temp2 =0;
    int error = 1;
    unsigned int i = 0;

    file_name = argv [1];
    file_ptr = fopen (file_name,"rb");

    if (file_ptr == NULL) {
        DEBUG_PRINT("\n File is not located ");
        return -1;
    }


    decoder_context = (struct video_decoder_context *) \
              calloc (sizeof (struct video_decoder_context),1);

    if (decoder_context == NULL) {
        return -1;
    }

    decoder_context->outputBufferFile = NULL;
    decoder_context->inputBufferFile = NULL;
    decoder_context->video_driver_fd = -1;
    decoder_context->inputBufferFile = file_ptr;

    file_ptr = fopen ("/data/output.yuv","wb");

    if (file_ptr == NULL) {
        DEBUG_PRINT("\n File can't be created");
        free (decoder_context);
        return -1;
    }

    decoder_context->outputBufferFile = file_ptr;

    switch (atoi(argv[2])) {
        case 0:
            DEBUG_PRINT("\n MPEG4 codec selected");
            decoder_context->decoder_format = VDEC_CODECTYPE_MPEG4;
            Code_type = 0;
            break;
        case 1:
            DEBUG_PRINT("\n H.263");
            decoder_context->decoder_format = VDEC_CODECTYPE_H263;
            Code_type = 0;
            break;
        case 2:
            DEBUG_PRINT("\n H.264");
            decoder_context->decoder_format = VDEC_CODECTYPE_H264;
            Code_type = 1;
            break;
        default:
            DEBUG_PRINT("\n Wrong codec type");
            error = -1;
            break;
    }

    if (error != -1) {
        temp1 = atoi(argv[3]);
        temp2 = atoi(argv[4]);

        if (((temp1%16) != 0) || ((temp2%16) != 0)) {
            error = -1;
        } else {
            decoder_context->video_resoultion.frame_height = temp1;
            decoder_context->video_resoultion.frame_width = temp2;
        }
    }

    switch (atoi(argv[5])) {
        case 0:
            DEBUG_PRINT("\n No Sink");
            decoder_context->outputBufferFile = NULL;
            break;
    }

    if ( error != -1 && (init_decoder (decoder_context) == -1 )) {
        DEBUG_PRINT("\n Init decoder fails ");
        error = -1;
    }

    DEBUG_PRINT("\n Decoder open successfull");


    /*Allocate input and output buffers*/
    if (error != -1 && (allocate_buffer (VDEC_BUFFER_TYPE_INPUT,
                    decoder_context)== -1)) {
        DEBUG_PRINT("\n Error in input Buffer allocation");
        error = -1;
    }

    if (error != -1 && (allocate_buffer (VDEC_BUFFER_TYPE_OUTPUT,
                    decoder_context)== -1)) {
        DEBUG_PRINT("\n Error in output Buffer allocation");
        error = -1;
    }


    if (error != -1 && (start_decoding (decoder_context) == -1)) {
        DEBUG_PRINT("\n Error in start decoding call");
        error = -1;
    }

    if (error != -1 && (stop_decoding (decoder_context) == -1)) {
        DEBUG_PRINT("\n Error in stop decoding call");
        error = -1;
    }

    DEBUG_PRINT("\n De-init the decoder");

    if ((deinit_decoder (decoder_context) == -1)) {
        error = -1;
    }


    (void)free_buffer (VDEC_BUFFER_TYPE_INPUT,decoder_context);
    (void)free_buffer (VDEC_BUFFER_TYPE_OUTPUT,decoder_context);

    if (decoder_context->inputBufferFile != NULL) {
        fclose (decoder_context->inputBufferFile);
    }

    if (decoder_context->outputBufferFile != NULL) {
        fclose (decoder_context->outputBufferFile);
    }

    DEBUG_PRINT ("\n Total Number of frames decoded %d",total_frames);
    DEBUG_PRINT("\n closing the driver");
    free (decoder_context);

    return error;
}

int init_decoder ( struct video_decoder_context *init_decode )
{
    struct vdec_ioctl_msg ioctl_msg = {NULL,NULL};
    struct video_queue_context *queue_ptr = NULL;
#ifdef MAX_RES_720P
    enum vdec_output_fromat output_format = VDEC_YUV_FORMAT_NV12;
#endif
#ifdef MAX_RES_1080P
    enum vdec_output_fromat output_format  = VDEC_YUV_FORMAT_TILE_4x2;
#endif

    pthread_mutexattr_t init_values;

    DEBUG_PRINT("\n Before calling the open");

    init_decode->video_driver_fd = open ("/dev/msm_vidc_dec", \
            O_RDWR | O_NONBLOCK);



    if (init_decode->video_driver_fd < 0) {
        DEBUG_PRINT("\n Open failed");
        return -1;
    }


    /*Initialize Decoder with codec type and resolution*/
    ioctl_msg.in = &init_decode->decoder_format;
    ioctl_msg.out = NULL;

    if (ioctl (init_decode->video_driver_fd,VDEC_IOCTL_SET_CODEC,
                (void*)&ioctl_msg) < 0) {
        DEBUG_PRINT("\n Set codec type failed");
        return -1;
    }

    /*Set the output format*/
    ioctl_msg.in = &output_format;
    ioctl_msg.out = NULL;

    if (ioctl (init_decode->video_driver_fd,VDEC_IOCTL_SET_OUTPUT_FORMAT,
                (void*)&ioctl_msg) < 0) {
        DEBUG_PRINT("\n Set output format failed");
        return -1;
    }

    ioctl_msg.in = &init_decode->video_resoultion;
    ioctl_msg.out = NULL;

    if (ioctl (init_decode->video_driver_fd,VDEC_IOCTL_SET_PICRES,
                (void*)&ioctl_msg) < 0) {
        DEBUG_PRINT("\n Set Resolution failed");
        return -1;
    }

    DEBUG_PRINT("\n After Set Resolution");

    DEBUG_PRINT("\n Query Input bufffer requirements");
    /*Get the Buffer requirements for input and output ports*/

    init_decode->input_buffer.buffer_type = VDEC_BUFFER_TYPE_INPUT;
    ioctl_msg.in = NULL;
    ioctl_msg.out = &init_decode->input_buffer;

    if (ioctl (init_decode->video_driver_fd,VDEC_IOCTL_GET_BUFFER_REQ,
                (void*)&ioctl_msg) < 0) {
        DEBUG_PRINT("\n Requesting for input buffer requirements failed");
        return -1;
    }

    DEBUG_PRINT("\n input Size=%d min count =%d actual count = %d", \
            init_decode->input_buffer.buffer_size,\
            init_decode->input_buffer.mincount,\
            init_decode->input_buffer.actualcount);


    init_decode->input_buffer.buffer_type = VDEC_BUFFER_TYPE_INPUT;
    ioctl_msg.in = &init_decode->input_buffer;
    ioctl_msg.out = NULL;
    init_decode->input_buffer.actualcount = init_decode->input_buffer.mincount + 2;

    if (ioctl (init_decode->video_driver_fd,VDEC_IOCTL_SET_BUFFER_REQ,
                (void*)&ioctl_msg) < 0) {
        DEBUG_PRINT("\n Set Buffer Requirements Failed");
        return -1;
    }


    DEBUG_PRINT("\n Query output bufffer requirements");
    init_decode->output_buffer.buffer_type = VDEC_BUFFER_TYPE_OUTPUT;
    ioctl_msg.in = NULL;
    ioctl_msg.out = &init_decode->output_buffer;

    if (ioctl (init_decode->video_driver_fd,VDEC_IOCTL_GET_BUFFER_REQ,
                (void*)&ioctl_msg) < 0) {
        DEBUG_PRINT("\n Requesting for output buffer requirements failed");
        return -1;
    }

    DEBUG_PRINT("\n output Size=%d min count =%d actual count = %d", \
            init_decode->output_buffer.buffer_size,\
            init_decode->output_buffer.mincount,\
            init_decode->output_buffer.actualcount);

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
       ) {
        return -1;
    }

    DEBUG_PRINT("\n create Threads");

    /*Create two threads*/
    if ( (pthread_create (&init_decode->videothread_id,NULL,video_thread,
                    init_decode) < 0) ||
            (pthread_create (&init_decode->asyncthread_id,NULL,async_thread,
                     init_decode) < 0)
       ) {
        return -1;
    }

    return 1;
}



int free_buffer ( enum vdec_buffer buffer_dir,
        struct video_decoder_context *decode_context
        )
{
    unsigned int buffercount = 0,i=0;
    struct vdec_bufferpayload **ptemp = NULL;

    if (decode_context == NULL) {
        return -1;
    }

    if (buffer_dir == VDEC_BUFFER_TYPE_INPUT && decode_context->ptr_inputbuffer) {
        buffercount = decode_context->input_buffer.actualcount;
        ptemp = decode_context->ptr_inputbuffer;

        for (i=0; i<buffercount; i++) {
            if (ptemp [i]) {
                if (ptemp [i]->pmem_fd != -1) {
                    munmap ( ptemp [i]->bufferaddr,ptemp [i]->mmaped_size);
                    ptemp [i]->bufferaddr = NULL;
                    close (ptemp [i]->pmem_fd);
                }

                free (ptemp [i]);
                ptemp [i] = NULL;
            }
        }

        free (decode_context->ptr_inputbuffer);
        decode_context->ptr_inputbuffer = NULL;
    } else if ( buffer_dir == VDEC_BUFFER_TYPE_OUTPUT ) {
        buffercount = decode_context->output_buffer.actualcount;
        ptemp = decode_context->ptr_outputbuffer;

        if (decode_context->ptr_respbuffer) {
            for (i=0; i<buffercount; i++) {
                if (decode_context->ptr_respbuffer [i]) {
                    free (decode_context->ptr_respbuffer[i]);
                    decode_context->ptr_respbuffer [i] = NULL;
                }
            }

            free (decode_context->ptr_respbuffer);
            decode_context->ptr_respbuffer = NULL;
        }

        if (ptemp) {
            for (i=0; i<buffercount; i++) {
                if (ptemp [i]) {
                    if (ptemp [i]->pmem_fd != -1) {
                        munmap ( ptemp [i]->bufferaddr,ptemp [i]->mmaped_size);
                        ptemp [i]->bufferaddr = NULL;
                        close (ptemp [i]->pmem_fd);
                    }

                    free (ptemp [i]);
                    ptemp [i] = NULL;
                }
            }

            free (ptemp);
            decode_context->ptr_outputbuffer = NULL;
        }
    }

    return 1;
}

int allocate_buffer ( enum vdec_buffer buffer_dir,
        struct video_decoder_context *decode_context
        )
{
    struct vdec_setbuffer_cmd setbuffers;
    struct vdec_bufferpayload **ptemp = NULL;
    struct vdec_ioctl_msg ioctl_msg = {NULL,NULL};
    unsigned int buffercount = 0,i=0,alignedsize=0;
    unsigned int buffersize = 0;

    if ( decode_context == NULL) {
        DEBUG_PRINT ("\nallocate_buffer: context is NULL");
        return -1;
    }

    if ( buffer_dir == VDEC_BUFFER_TYPE_INPUT ) {
        /*Check if buffers are allocated*/
        if (decode_context->ptr_inputbuffer != NULL) {
            DEBUG_PRINT ("\nallocate_buffer: decode_context->ptr_inputbuffer is set");
            return -1;
        }

        buffercount = decode_context->input_buffer.actualcount;
        alignedsize = decode_context->input_buffer.alignment;
        buffersize = decode_context->input_buffer.buffer_size;
        buffersize = (buffersize + alignedsize) & (~alignedsize);
    } else if (buffer_dir == VDEC_BUFFER_TYPE_OUTPUT) {
        /*Check if buffers are allocated*/
        if (decode_context->ptr_outputbuffer != NULL) {
            DEBUG_PRINT ("\nallocate_buffer: Double allcoate output");
            return -1;
        }

        buffercount = decode_context->output_buffer.actualcount;
        alignedsize = decode_context->output_buffer.alignment;
        buffersize = decode_context->output_buffer.buffer_size;
        buffersize = (buffersize + alignedsize) & (~alignedsize);

        decode_context->ptr_respbuffer = (struct vdec_output_frameinfo  **)\
                         calloc (sizeof (struct vdec_output_frameinfo *),buffercount);

        if (decode_context->ptr_respbuffer == NULL) {
            DEBUG_PRINT ("\n Allocate failure ptr_respbuffer");
            return -1;
        }

        for (i=0; i< buffercount; i++) {
            decode_context->ptr_respbuffer [i] = (struct vdec_output_frameinfo *)\
                                 calloc (sizeof (struct vdec_output_frameinfo),buffercount);

            if (decode_context->ptr_respbuffer [i] == NULL) {
                DEBUG_PRINT ("\nfailed to allocate vdec_output_frameinfo");
                return -1;
            }
        }
    } else {
        DEBUG_PRINT ("\nallocate_buffer: Wrong buffer directions");
        return -1;
    }

    ptemp = (struct vdec_bufferpayload **)\
        calloc (sizeof (struct vdec_bufferpayload *),buffercount);

    if (ptemp == NULL) {
        DEBUG_PRINT ("\nallocate_buffer: vdec_bufferpayload failure");
        return -1;
    }


    if (buffer_dir == VDEC_BUFFER_TYPE_OUTPUT) {
        DEBUG_PRINT ("\nallocate_buffer: OUT");
        decode_context->ptr_outputbuffer = ptemp;
    } else {
        DEBUG_PRINT ("\nallocate_buffer: IN");
        decode_context->ptr_inputbuffer = ptemp;
    }

    /*Allocate buffer headers*/
    for (i=0; i< buffercount; i++) {
        ptemp [i] = (struct vdec_bufferpayload*)\
                calloc (sizeof (struct vdec_bufferpayload),1);

        if (ptemp [i] == NULL) {
            DEBUG_PRINT ("\nallocate_buffer: ptemp [i] calloc failure");
            return -1;
        }

        if (buffer_dir == VDEC_BUFFER_TYPE_OUTPUT) {
            decode_context->ptr_respbuffer [i]->client_data = \
                                      (void *) ptemp [i];
        }

        ptemp [i]->pmem_fd = -1;

    }

    for (i=0; i< buffercount; i++) {
        ptemp [i]->pmem_fd = open ("/dev/pmem_adsp",O_RDWR);

        if (ptemp [i]->pmem_fd < 0) {
            DEBUG_PRINT ("\nallocate_buffer: open pmem_adsp failed");
            return -1;
        }

        ptemp [i]->bufferaddr = mmap(NULL,clp2(buffersize),PROT_READ|PROT_WRITE,
                MAP_SHARED,ptemp [i]->pmem_fd,0);
        DEBUG_PRINT ("\n pmem fd = %d virt addr = %p",ptemp [i]->pmem_fd,\
                ptemp [i]->bufferaddr);

        if (ptemp [i]->bufferaddr == MAP_FAILED) {
            ptemp [i]->bufferaddr = NULL;
            DEBUG_PRINT ("\nallocate_buffer: MMAP failed");
            return -1;
        }

        ptemp [i]->buffer_len = buffersize;
        ptemp [i]->mmaped_size = clp2 (buffersize);

        setbuffers.buffer_type = buffer_dir;
        memcpy (&setbuffers.buffer,ptemp [i],sizeof (struct vdec_bufferpayload));

        ioctl_msg.in  = &setbuffers;
        ioctl_msg.out = NULL;

        if (ioctl (decode_context->video_driver_fd,VDEC_IOCTL_SET_BUFFER,
                    &ioctl_msg) < 0) {
            DEBUG_PRINT ("\nallocate_buffer: Set Buffer IOCTL failed");
            return -1;
        }

    }

    DEBUG_PRINT ("\nallocate_buffer: Success");
    return 1;
}



int start_decoding (struct video_decoder_context *decode_context)
{
    struct vdec_ioctl_msg ioctl_msg = {NULL,NULL};
    struct vdec_input_frameinfo frameinfo;
    struct vdec_fillbuffer_cmd fillbuffer;
    unsigned int i = 0;
    unsigned int data_len =0;

    memset ((unsigned char*)&frameinfo,0,sizeof (struct vdec_input_frameinfo));
    memset ((unsigned char*)&fillbuffer,0,sizeof (struct vdec_fillbuffer_cmd));

    if (decode_context == NULL) {
        return -1;
    }

    if (ioctl (decode_context->video_driver_fd,VDEC_IOCTL_CMD_START,
                NULL) < 0) {
        DEBUG_PRINT("\n Start failed");
        return -1;
    }

    DEBUG_PRINT("\n Start Issued successfully waiting for Start Done");
    /*Wait for Start command response*/
    sem_wait (&decode_context->sem_synchronize);

    /*Push output Buffers*/
    i = 0;

    while (i < decode_context->output_buffer.mincount) {
        fillbuffer.buffer.buffer_len =
            decode_context->ptr_outputbuffer [i]->buffer_len;
        fillbuffer.buffer.bufferaddr =
            decode_context->ptr_outputbuffer [i]->bufferaddr;
        fillbuffer.buffer.offset =
            decode_context->ptr_outputbuffer [i]->offset;
        fillbuffer.buffer.pmem_fd =
            decode_context->ptr_outputbuffer [i]->pmem_fd;
        fillbuffer.client_data = (void *)decode_context->ptr_respbuffer [i];
        DEBUG_PRINT ("\n Client Data on output = %p",fillbuffer.client_data);
        ioctl_msg.in = &fillbuffer;
        ioctl_msg.out = NULL;

        if (ioctl (decode_context->video_driver_fd,
                    VDEC_IOCTL_FILL_OUTPUT_BUFFER,&ioctl_msg) < 0) {
            DEBUG_PRINT("\n Decoder frame failed");
            return -1;
        }

        i++;
    }


    /*push input buffers*/
    i = 0;

    while (i < decode_context->input_buffer.mincount) {
        DEBUG_PRINT("\n Read  Frame from File");
        data_len = read_frame ( decode_context->ptr_inputbuffer [i]->bufferaddr,
                decode_context->ptr_inputbuffer [i]->buffer_len,
                decode_context->inputBufferFile);

        if (data_len == 0) {
            DEBUG_PRINT("\n Length is zero error");
            return -1;
        }

        DEBUG_PRINT("\n Read  Frame from File szie = %u",data_len);
        frameinfo.bufferaddr =
            decode_context->ptr_inputbuffer [i]->bufferaddr;
        frameinfo.offset = 0;
        frameinfo.pmem_fd = decode_context->ptr_inputbuffer [i]->pmem_fd;
        frameinfo.pmem_offset = decode_context->ptr_inputbuffer [i]->offset;
        frameinfo.datalen = data_len;
        frameinfo.client_data = (struct vdec_bufferpayload *)\
                    decode_context->ptr_inputbuffer [i];
        /*TODO: Time stamp needs to be updated*/
        ioctl_msg.in = &frameinfo;
        ioctl_msg.out = NULL;

        if (ioctl (decode_context->video_driver_fd,VDEC_IOCTL_DECODE_FRAME,
                    &ioctl_msg) < 0) {
            DEBUG_PRINT("\n Decoder frame failed");
            return -1;
        }

        total_frames++;
        i++;
    }

    DEBUG_PRINT ("\n Wait for EOS");
    /*Wait for EOS or Error condition*/
    sem_wait (&decode_context->sem_synchronize);
    DEBUG_PRINT ("\n Reached EOS");

    return 1;
}

int stop_decoding  (struct video_decoder_context *decode_context)
{
    struct vdec_ioctl_msg ioctl_msg = {NULL,NULL};
    enum vdec_bufferflush flush_dir = VDEC_FLUSH_TYPE_INPUT;

    if (decode_context == NULL) {
        return -1;
    }

    ioctl_msg.in = &flush_dir;
    ioctl_msg.out = NULL;

    if (ioctl(decode_context->video_driver_fd,VDEC_IOCTL_CMD_FLUSH,
                &ioctl_msg) < 0) {
        DEBUG_PRINT("\n Flush input failed");
    } else {
        sem_wait (&decode_context->sem_synchronize);
    }

    flush_dir = VDEC_FLUSH_TYPE_OUTPUT;
    ioctl_msg.in = &flush_dir;
    ioctl_msg.out = NULL;

    if (ioctl(decode_context->video_driver_fd,VDEC_IOCTL_CMD_FLUSH,
                &ioctl_msg) < 0) {
        DEBUG_PRINT("\n Flush output failed");
    } else {
        sem_wait (&decode_context->sem_synchronize);
    }

    DEBUG_PRINT("\n Stop VDEC_IOCTL_CMD_STOP");

    if (ioctl(decode_context->video_driver_fd,VDEC_IOCTL_CMD_STOP,
                NULL) < 0) {
        DEBUG_PRINT("\n Stop failed");
    } else {
        sem_wait (&decode_context->sem_synchronize);
    }

    return 1;
}

int deinit_decoder (struct video_decoder_context *init_decode)
{
    if (init_decode == NULL) {
        return -1;
    }

    /*Close the driver*/
    if (init_decode->video_driver_fd != -1) {
        close (init_decode->video_driver_fd);
    }

    if (init_decode->queue_context.ptr_cmdq) {
        free (init_decode->queue_context.ptr_cmdq);
        init_decode->queue_context.ptr_cmdq = NULL;
    }

    if (init_decode->queue_context.ptr_dataq) {
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
    struct video_decoder_context *decode_context = NULL;
    struct video_msgq *queueitem = NULL;
    struct vdec_ioctl_msg ioctl_msg = {NULL,NULL};
    struct vdec_input_frameinfo frameinfo;
    struct vdec_fillbuffer_cmd fillbuffer;
    struct vdec_output_frameinfo *outputbuffer = NULL;
    struct vdec_bufferpayload *tempbuffer = NULL;
    unsigned int data_len =0;


    if (context == NULL) {
        DEBUG_PRINT("\n video thread recieved NULL context");
        return NULL;
    }

    decode_context = (struct video_decoder_context *) context;

    /* Thread function which will accept commands from async thread
     * or main thread
     */
    while (1) {
        queueitem = queue_get_cmd (&decode_context ->queue_context);

        if (queueitem != NULL) {
            switch (queueitem->cmd) {
                case VDEC_MSG_EVT_HW_ERROR:
                    DEBUG_PRINT("\n FATAL ERROR ");
                    break;
                case VDEC_MSG_RESP_INPUT_FLUSHED:
                    DEBUG_PRINT("\n Input Buffer Flushed");
                    break;
                case VDEC_MSG_RESP_OUTPUT_FLUSHED:
                    DEBUG_PRINT("\n Output buffer Flushed");
                    break;
                case VDEC_MSG_RESP_START_DONE:
                    DEBUG_PRINT("\n recived start done command");
                    sem_post (&decode_context->sem_synchronize);
                    break;

                case VDEC_MSG_RESP_STOP_DONE:
                    DEBUG_PRINT("\n recieved stop done");
                    sem_post (&decode_context->sem_synchronize);
                    break;

                case VDEC_MSG_RESP_INPUT_BUFFER_DONE:

                    tempbuffer = (struct vdec_bufferpayload *)queueitem->clientdata;

                    if (tempbuffer == NULL) {
                        DEBUG_PRINT("\n FATAL ERROR input buffer address is bad");
                        sem_post (&decode_context->sem_synchronize);
                        break;
                    }

                    data_len = read_frame ( tempbuffer->bufferaddr,
                            tempbuffer->buffer_len,
                            decode_context->inputBufferFile
                            );

                    if (data_len == 0) {
                        DEBUG_PRINT ("\n End of stream reached");
                        sem_post (&decode_context->sem_synchronize);
                        break;
                    }

                    frameinfo.bufferaddr = tempbuffer->bufferaddr;
                    frameinfo.offset = 0;
                    frameinfo.pmem_fd = tempbuffer->pmem_fd;
                    frameinfo.pmem_offset = tempbuffer->offset;
                    frameinfo.datalen = data_len;
                    frameinfo.client_data = (struct vdec_bufferpayload *)\
                                tempbuffer;
                    /*TODO: Time stamp needs to be updated*/
                    ioctl_msg.in = &frameinfo;
                    ioctl_msg.out = NULL;
                    total_frames++;

                    if (ioctl(decode_context->video_driver_fd,VDEC_IOCTL_DECODE_FRAME,
                                &ioctl_msg) < 0) {
                        DEBUG_PRINT("\n Decoder frame failed");
                        sem_post (&decode_context->sem_synchronize);
                    }

                    DEBUG_PRINT("\n Input buffer done send next buffer current value = %d",\
                            total_frames);
                    break;

                case VDEC_MSG_RESP_OUTPUT_BUFFER_DONE:

                    outputbuffer = (struct vdec_output_frameinfo *)\
                               queueitem->clientdata;
                    DEBUG_PRINT("\n Value of client Data in VT %p",queueitem->clientdata);

                    if (outputbuffer == NULL || outputbuffer->bufferaddr == NULL ||
                            outputbuffer->client_data == NULL
                       ) {
                        DEBUG_PRINT("\n FATAL ERROR output buffer is bad");
                        DEBUG_PRINT("\nValues outputbuffer = %p",outputbuffer);

                        if (outputbuffer != NULL) {
                            DEBUG_PRINT("\nValues outputbuffer->bufferaddr = %p",\
                                    outputbuffer->bufferaddr);
                            DEBUG_PRINT("\nValues outputbuffer->client_data = %p",\
                                    outputbuffer->client_data);
                        }

                        sem_post (&decode_context->sem_synchronize);
                        break;
                    }


                    if (outputbuffer->len == 0) {
                        DEBUG_PRINT("\n Filled Length is zero Close decoding");
                        sem_post (&decode_context->sem_synchronize);
                        break;
                    }

                    if (decode_context->outputBufferFile != NULL) {
                        fwrite (outputbuffer->bufferaddr,1,outputbuffer->len,
                                decode_context->outputBufferFile);
                    }

                    tempbuffer = (struct vdec_bufferpayload *)\
                             outputbuffer->client_data;

                    DEBUG_PRINT("\n recieved output buffer consume outbuffer");
                    DEBUG_PRINT("\nValues outputbuffer->bufferaddr = %p",\
                            outputbuffer->bufferaddr);
                    DEBUG_PRINT ("\n Vir address of allocated buffer %p",\
                            tempbuffer->bufferaddr);
                    fillbuffer.buffer.buffer_len = tempbuffer->buffer_len;
                    fillbuffer.buffer.bufferaddr = tempbuffer->bufferaddr;
                    fillbuffer.buffer.offset = tempbuffer->offset;
                    fillbuffer.buffer.pmem_fd = tempbuffer->pmem_fd;
                    fillbuffer.client_data = (void *)outputbuffer;

                    ioctl_msg.in = &fillbuffer;
                    ioctl_msg.out = NULL;

                    if (ioctl (decode_context->video_driver_fd,
                                VDEC_IOCTL_FILL_OUTPUT_BUFFER,&ioctl_msg) < 0) {
                        DEBUG_PRINT("\n Decoder frame failed");
                        return NULL;
                    }

                    break;

                case VDEC_MSG_RESP_FLUSH_INPUT_DONE:
                    DEBUG_PRINT("\n Flush input complete");
                    sem_post (&decode_context->sem_synchronize);
                    break;

                case VDEC_MSG_RESP_FLUSH_OUTPUT_DONE:
                    DEBUG_PRINT("\n Flush output complete");
                    sem_post (&decode_context->sem_synchronize);
                    break;
            }

            if (queueitem->cmd == VDEC_MSG_RESP_STOP_DONE) {
                DEBUG_PRINT("\n Playback has ended thread will exit");
                return NULL;
            }
        } else {
            DEBUG_PRINT("\n Error condition recieved NULL from Queue");
        }

    }
}

static void* async_thread (void *context)
{
    struct video_decoder_context *decode_context = NULL;
    struct vdec_output_frameinfo *outputframe = NULL;
    struct video_msgq queueitem ;
    struct vdec_msginfo vdec_msg;
    struct vdec_ioctl_msg ioctl_msg = {NULL,NULL};
    int result = -1;

    if (context == NULL) {
        DEBUG_PRINT("\n aynsc thread recieved NULL context");
        return NULL;
    }

    decode_context = (struct video_decoder_context *) context;
    DEBUG_PRINT("\n Entering the async thread");

    while (1) {
        ioctl_msg.in = NULL;

        ioctl_msg.out = (void*)&vdec_msg;
        DEBUG_PRINT ("\n Sizeof vdec_msginfo = %d ",sizeof (vdec_msg));
        DEBUG_PRINT("\n Address of Vdec msg in async thread %p",\
                ioctl_msg.out);

        if (ioctl (decode_context->video_driver_fd,VDEC_IOCTL_GET_NEXT_MSG,\
                    (void*)&ioctl_msg) < 0) {
            DEBUG_PRINT("\n Error in ioctl read next msg");
        } else {
            switch (vdec_msg.msgcode) {
                case VDEC_MSG_RESP_FLUSH_INPUT_DONE:
                case VDEC_MSG_RESP_FLUSH_OUTPUT_DONE:
                case VDEC_MSG_RESP_START_DONE:
                case VDEC_MSG_RESP_STOP_DONE:
                case VDEC_MSG_EVT_HW_ERROR:
                    DEBUG_PRINT("\nioctl read next msg");
                    queueitem.cmd = vdec_msg.msgcode;
                    queueitem.status = vdec_msg.status_code;
                    queueitem.clientdata = NULL;
                    break;

                case VDEC_MSG_RESP_INPUT_FLUSHED:
                case VDEC_MSG_RESP_INPUT_BUFFER_DONE:

                    queueitem.cmd = vdec_msg.msgcode;
                    queueitem.status = vdec_msg.status_code;
                    queueitem.clientdata = (void *)\
                                   vdec_msg.msgdata.input_frame_clientdata;
                    break;

                case VDEC_MSG_RESP_OUTPUT_FLUSHED:
                case VDEC_MSG_RESP_OUTPUT_BUFFER_DONE:
                    queueitem.cmd = vdec_msg.msgcode;
                    queueitem.status = vdec_msg.status_code;
                    outputframe = (struct vdec_output_frameinfo *)\
                              vdec_msg.msgdata.output_frame.client_data;
                    DEBUG_PRINT ("\n Client Data value in %p", \
                            vdec_msg.msgdata.output_frame.client_data);
                    outputframe->bufferaddr = vdec_msg.msgdata.output_frame.bufferaddr;
                    outputframe->framesize.bottom = \
                                    vdec_msg.msgdata.output_frame.framesize.bottom;
                    outputframe->framesize.left = \
                                      vdec_msg.msgdata.output_frame.framesize.left;
                    outputframe->framesize.right = \
                                       vdec_msg.msgdata.output_frame.framesize.right;
                    outputframe->framesize.top = \
                                     vdec_msg.msgdata.output_frame.framesize.top;
                    outputframe->framesize = vdec_msg.msgdata.output_frame.framesize;
                    outputframe->len = vdec_msg.msgdata.output_frame.len;
                    outputframe->time_stamp = vdec_msg.msgdata.output_frame.time_stamp;
                    queueitem.clientdata = (void *)outputframe;
                    DEBUG_PRINT ("\n Client Data value Copy %p",queueitem.clientdata);
                    break;

                default:
                    DEBUG_PRINT("\nIn Default of get next message %d",vdec_msg.msgcode);
                    queueitem.cmd = vdec_msg.msgcode;
                    queueitem.status = vdec_msg.status_code;
                    queueitem.clientdata = NULL;
                    break;
            }

            result = queue_post_cmdq (&decode_context->queue_context,&queueitem);

            while (result == 0) {
                result = queue_post_cmdq (&decode_context->queue_context,
                        &queueitem);
            }

            if (result == -1) {
                DEBUG_PRINT("\n FATAL ERROR WITH Queue");
            }
        }

        if (vdec_msg.msgcode == VDEC_MSG_RESP_STOP_DONE) {
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

    if (dataptr == NULL || length == 0) {
        DEBUG_PRINT ("\n dataptr = %p length = %u",dataptr,length);
        return 0;
    }

    if (!Code_type) {
        /* Start of Critical Section*/
        pthread_mutex_lock(&read_lock);

        do {
            //Start codes are always byte aligned.
            bytes_read = fread(&dataptr[readOffset],1, 1,inputBufferFile);

            if ( !bytes_read) {
                DEBUG_PRINT("\n Bytes read Zero \n");
                break;
            }

            code <<= 8;
            code |= (0x000000FF & dataptr[readOffset]);

            //VOP start code comparision
            if (readOffset>3) {
                if (!header_code ) {
                    if ( VOP_START_CODE == code) {
                        DEBUG_PRINT ("\n Found VOP Code");
                        header_code = VOP_START_CODE;
                    } else if ( (0xFFFFFC00 & code) == SHORT_HEADER_START_CODE ) {
                        header_code = SHORT_HEADER_START_CODE;
                    }
                }

                if ((header_code == VOP_START_CODE) && (code == VOP_START_CODE)) {
                    //Seek backwards by 4
                    fseek(inputBufferFile, -4, SEEK_CUR);
                    readOffset-=4;
                    found = 1;
                    break;

                } else if (( header_code == SHORT_HEADER_START_CODE ) &&
                        ( SHORT_HEADER_START_CODE == (code & 0xFFFFFC00))) {
                    //Seek backwards by 4
                    fseek(inputBufferFile, -4, SEEK_CUR);
                    readOffset-=4;
                    found = 1;
                    break;
                }
            }

            readOffset++;
        } while (readOffset < length);

        pthread_mutex_unlock(&read_lock);

        /* End of Critical Section*/
        if (found == 1) {
            //DEBUG_PRINT ("Found a Frame");
            return (readOffset+1);
        } else {
            //DEBUG_PRINT ("No Frames detected");
            return 0;
        }
    } else {

        readOffset = Read_Buffer_From_DAT_File(dataptr,length,inputBufferFile);

        if (total_frames == 0) {
            bytes_read = Read_Buffer_From_DAT_File(&dataptr[readOffset],
                    (length-readOffset),
                    inputBufferFile);
            readOffset += bytes_read;
        }

        return (readOffset);
    }

}

static int Read_Buffer_From_DAT_File(unsigned char *dataptr, unsigned int length,
        FILE * inputBufferFile)
{


    long frameSize=0;
    char temp_buffer[10];
    char temp_byte;
    int bytes_read=0;
    int i=0;
    unsigned char *read_buffer=NULL;
    char c = '1'; //initialize to anything except '\0'(0)
    char inputFrameSize[12];
    int count =0;
    unsigned int cnt =0;
    memset(temp_buffer, 0, sizeof(temp_buffer));

    while (cnt < 10)
        /* Check the input file format, may result in infinite loop */
    {
        count  = fread(&inputFrameSize[cnt],1,1,inputBufferFile);

        if (inputFrameSize[cnt] == '\0' )
            break;

        cnt++;
    }

    inputFrameSize[cnt]='\0';
    frameSize = atoi(inputFrameSize);
    //length = 0;
    DEBUG_PRINT ("\n Frame Size is %ld",frameSize);

    /* get the frame length */
    fseek(inputBufferFile, -1, SEEK_CUR);
    bytes_read = fread(dataptr, 1, frameSize,  inputBufferFile);

    if (bytes_read == 0 || bytes_read < frameSize ) {
        return 0;
    }

    return bytes_read;
}
