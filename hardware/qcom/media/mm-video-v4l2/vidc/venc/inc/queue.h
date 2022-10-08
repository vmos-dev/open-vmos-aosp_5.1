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
#ifndef QUEUE_H
#define QUEUE_H

#include<pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <string.h>

/* Message Queue structure */
struct video_msgq {
    /* Command to be executed */
    unsigned int cmd;

    unsigned int status;

    /* Client-specific data */
    void *clientdata;
};


/* Thread & Message Queue information */
struct video_queue_context {
    /* Message Queue related members */
    pthread_mutex_t  mutex;
    sem_t sem_message;
    int commandq_size;
    int dataq_size;
    struct video_msgq *ptr_dataq;
    struct video_msgq *ptr_cmdq;
    int write_dataq ;
    int read_dataq;
    int write_comq ;
    int read_comq ;

};

int check_if_queue_empty ( unsigned int queuetocheck,void* queuecontext );

struct video_msgq * queue_get_cmd ( void* queuecontext );



int queue_post_cmdq ( void *queuecontext,
        struct video_msgq *post_msg
        );

int queue_post_dataq ( void *queuecontext,
        struct video_msgq *post_msg
        );

#endif /* QUEUE_H */
