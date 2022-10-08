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
#include "message_queue.h"

int check_if_queue_empty ( unsigned int queuetocheck, void* queuecontext )
{
    struct video_queue_context *ptr_q = NULL;
    /*
     * queuetocheck - 0 command queue
     * queuetocheck - 1 data queue
     */
    if ( queuecontext == NULL || (queuetocheck > 1 ) )
    {
        return 1;
    }
    ptr_q = (struct video_queue_context *)queuecontext;

    if (queuetocheck == 0)
    {
      if (ptr_q->read_comq == ptr_q->write_comq)
      {
          return 1;
      }
    }
    else if (queuetocheck == 1)
    {
        if (ptr_q->write_dataq == ptr_q->read_dataq)
        {
            return 1;
        }
    }

    return 0;
}



struct video_msgq * queue_get_cmd (void* queuecontext )
{
    struct video_queue_context *ptr_q = NULL;
    struct video_msgq *pitem = NULL;

    if( NULL == queuecontext )
    {
        printf("\n queue_get_cmd: Invalid Input parameter\n");
        return NULL;
    }

    ptr_q = (struct video_queue_context *)queuecontext;

    /* Wait on the semaphore till it is released */
    sem_wait(&ptr_q->sem_message);

    /* Lock the mutex to protect the critical section */
    pthread_mutex_lock(&ptr_q->mutex);

    if (ptr_q->read_comq != ptr_q->write_comq)
    {
        pitem = &ptr_q->ptr_cmdq [ptr_q->read_comq];
        ptr_q->read_comq = (ptr_q->read_comq + 1) % \
                            ptr_q->commandq_size;
    }
    else if (ptr_q->write_dataq != ptr_q->read_dataq)
    {
        pitem = &ptr_q->ptr_dataq [ptr_q->read_dataq];
        ptr_q->read_dataq = (ptr_q->read_dataq + 1) % \
                            ptr_q->dataq_size;
    }

    /* Unlock the mutex to release the critical section */
    pthread_mutex_unlock(&ptr_q->mutex);

    return pitem;
}


int queue_post_cmdq ( void* queuecontext,
                     struct video_msgq *pitem
                     )
{
    struct video_queue_context *ptr_q = NULL;

    if (pitem == NULL || queuecontext == NULL)
    {
        return -1;
    }
    ptr_q = (struct video_queue_context *)queuecontext;

    /* Lock the mutex to protect the critical section */
    pthread_mutex_lock(&ptr_q->mutex);

    if ((ptr_q->write_comq + 1) % ptr_q->commandq_size == ptr_q->read_comq)
    {
        printf("\n QUEUE is FULL");
        /* Unlock the mutex to release the critical section */
        pthread_mutex_unlock(&ptr_q->mutex);
        return 0;
    }
    else
    {
        /* Store the command in the Message Queue & increment write offset */
        memcpy ( &ptr_q->ptr_cmdq [ptr_q->write_comq],pitem, \
                 sizeof (struct video_msgq));
        ptr_q->write_comq = (ptr_q->write_comq + 1) % ptr_q->commandq_size;
    }

    /* Unlock the mutex to release the critical section */
    pthread_mutex_unlock(&ptr_q->mutex);

    /* Post the semaphore */
    sem_post(&ptr_q->sem_message);
    return 1;
}


int queue_post_dataq ( void *queuecontext,
                       struct video_msgq *pitem
                      )
{
    struct video_queue_context *ptr_q = NULL;

    if (pitem == NULL || queuecontext == NULL)
    {
        return -1;
    }
    ptr_q = (struct video_queue_context *)queuecontext;

    /* Lock the mutex to protect the critical section */
    pthread_mutex_lock(&ptr_q->mutex);

    if ((ptr_q->write_dataq + 1) % ptr_q->dataq_size == ptr_q->read_dataq)
    {
        printf("\n QUEUE is FULL");
        /* Unlock the mutex to release the critical section */
        pthread_mutex_unlock(&ptr_q->mutex);
        return 0;
    }
    else
    {
        /* Store the command in the Message Queue & increment write offset */
        memcpy ( &ptr_q->ptr_dataq [ptr_q->write_dataq],pitem, \
                 sizeof (struct video_msgq));
        ptr_q->write_dataq = (ptr_q->write_dataq + 1) % ptr_q->dataq_size;
    }

    /* Unlock the mutex to release the critical section */
    pthread_mutex_unlock(&ptr_q->mutex);

    /* Post the semaphore */
    sem_post(&ptr_q->sem_message);
    return 1;

}
