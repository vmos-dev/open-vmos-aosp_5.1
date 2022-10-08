/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <unistd.h>
#include <utils/Errors.h>



#define LOG_TAG "MessageQueue"
#include <utils/Log.h>

#include "MessageQueue.h"

namespace TIUTILS {

/**
   @brief Constructor for the message queue class

   @param none
   @return none
 */
MessageQueue::MessageQueue()
{
    LOG_FUNCTION_NAME;

    int fds[2] = {-1,-1};
    android::status_t stat;

    stat = pipe(fds);

    if ( 0 > stat )
        {
        MSGQ_LOGEB("Error while openning pipe: %s", strerror(stat) );
        this->fd_read = 0;
        this->fd_write = 0;
        mHasMsg = false;
        }
    else
        {
        this->fd_read = fds[0];
        this->fd_write = fds[1];

        mHasMsg = false;
        }

    LOG_FUNCTION_NAME_EXIT;
}

/**
   @brief Destructor for the semaphore class

   @param none
   @return none
 */
MessageQueue::~MessageQueue()
{
    LOG_FUNCTION_NAME;

    if(this->fd_read >= 0)
        {
        close(this->fd_read);
        }

    if(this->fd_write >= 0)
        {
        close(this->fd_write);
        }

    LOG_FUNCTION_NAME_EXIT;
}

/**
   @brief Get a message from the queue

   @param msg Message structure to hold the message to be retrieved
   @return android::NO_ERROR On success
   @return android::BAD_VALUE if the message pointer is NULL
   @return android::NO_INIT If the file read descriptor is not set
   @return android::UNKNOWN_ERROR if the read operation fromthe file read descriptor fails
 */
android::status_t MessageQueue::get(Message* msg)
{
    LOG_FUNCTION_NAME;

    if(!msg)
        {
        MSGQ_LOGEA("msg is NULL");
        LOG_FUNCTION_NAME_EXIT;
        return android::BAD_VALUE;
        }

    if(!this->fd_read)
        {
        MSGQ_LOGEA("read descriptor not initialized for message queue");
        LOG_FUNCTION_NAME_EXIT;
        return android::NO_INIT;
        }

    char* p = (char*) msg;
    size_t read_bytes = 0;

    while( read_bytes  < sizeof(*msg) )
        {
        int err = read(this->fd_read, p, sizeof(*msg) - read_bytes);

        if( err < 0 )
            {
            MSGQ_LOGEB("read() error: %s", strerror(errno));
            return android::UNKNOWN_ERROR;
            }
        else
            {
            read_bytes += err;
            }
        }

    MSGQ_LOGDB("MQ.get(%d,%p,%p,%p,%p)", msg->command, msg->arg1,msg->arg2,msg->arg3,msg->arg4);

    mHasMsg = false;

    LOG_FUNCTION_NAME_EXIT;

    return 0;
}

/**
   @brief Get the input file descriptor of the message queue

   @param none
   @return file read descriptor
 */

int MessageQueue::getInFd()
{
    return this->fd_read;
}

/**
   @brief Constructor for the message queue class

   @param fd file read descriptor
   @return none
 */

void MessageQueue::setInFd(int fd)
{
    LOG_FUNCTION_NAME;

    if ( -1 != this->fd_read )
        {
        close(this->fd_read);
        }

    this->fd_read = fd;

    LOG_FUNCTION_NAME_EXIT;
}

/**
   @brief Queue a message

   @param msg Message structure to hold the message to be retrieved
   @return android::NO_ERROR On success
   @return android::BAD_VALUE if the message pointer is NULL
   @return android::NO_INIT If the file write descriptor is not set
   @return android::UNKNOWN_ERROR if the write operation fromthe file write descriptor fails
 */

android::status_t MessageQueue::put(Message* msg)
{
    LOG_FUNCTION_NAME;

    char* p = (char*) msg;
    size_t bytes = 0;

    if(!msg)
        {
        MSGQ_LOGEA("msg is NULL");
        LOG_FUNCTION_NAME_EXIT;
        return android::BAD_VALUE;
        }

    if(!this->fd_write)
        {
        MSGQ_LOGEA("write descriptor not initialized for message queue");
        LOG_FUNCTION_NAME_EXIT;
        return android::NO_INIT;
        }


    MSGQ_LOGDB("MQ.put(%d,%p,%p,%p,%p)", msg->command, msg->arg1,msg->arg2,msg->arg3,msg->arg4);

    while( bytes  < sizeof(msg) )
        {
        int err = write(this->fd_write, p, sizeof(*msg) - bytes);

        if( err < 0 )
            {
            MSGQ_LOGEB("write() error: %s", strerror(errno));
            LOG_FUNCTION_NAME_EXIT;
            return android::UNKNOWN_ERROR;
            }
        else
            {
            bytes += err;
            }
        }

    MSGQ_LOGDA("MessageQueue::put EXIT");

    LOG_FUNCTION_NAME_EXIT;
    return 0;
}


/**
   @brief Returns if the message queue is empty or not

   @param none
   @return true If the queue is empty
   @return false If the queue has at least one message
 */
bool MessageQueue::isEmpty()
{
    LOG_FUNCTION_NAME;

    struct pollfd pfd;

    pfd.fd = this->fd_read;
    pfd.events = POLLIN;
    pfd.revents = 0;

    if(!this->fd_read)
        {
        MSGQ_LOGEA("read descriptor not initialized for message queue");
        LOG_FUNCTION_NAME_EXIT;
        return android::NO_INIT;
        }


    if( -1 == poll(&pfd,1,0) )
        {
        MSGQ_LOGEB("poll() error: %s", strerror(errno));
        LOG_FUNCTION_NAME_EXIT;
        return false;
        }

    if(pfd.revents & POLLIN)
        {
        mHasMsg = true;
        }
    else
        {
        mHasMsg = false;
        }

    LOG_FUNCTION_NAME_EXIT;
    return !mHasMsg;
}

void MessageQueue::clear()
{
    if(!this->fd_read)
        {
        MSGQ_LOGEA("read descriptor not initialized for message queue");
        LOG_FUNCTION_NAME_EXIT;
        return;
        }

    Message msg;
    while(!isEmpty())
        {
        get(&msg);
        }

}


/**
   @brief Force whether the message queue has message or not

   @param hasMsg Whether the queue has a message or not
   @return none
 */
void MessageQueue::setMsg(bool hasMsg)
    {
    mHasMsg = hasMsg;
    }


/**
   @briefWait for message in maximum three different queues with a timeout

   @param queue1 First queue. At least this should be set to a valid queue pointer
   @param queue2 Second queue. Optional.
   @param queue3 Third queue. Optional.
   @param timeout The timeout value (in micro secs) to wait for a message in any of the queues
   @return android::NO_ERROR On success
   @return android::BAD_VALUE If queue1 is NULL
   @return android::NO_INIT If the file read descriptor of any of the provided queues is not set
 */
android::status_t MessageQueue::waitForMsg(MessageQueue *queue1, MessageQueue *queue2, MessageQueue *queue3, int timeout)
    {
    LOG_FUNCTION_NAME;

    int n =1;
    struct pollfd pfd[3];

    if(!queue1)
        {
        MSGQ_LOGEA("queue1 pointer is NULL");
        LOG_FUNCTION_NAME_EXIT;
        return android::BAD_VALUE;
        }

    pfd[0].fd = queue1->getInFd();
    if(!pfd[0].fd)
        {
        MSGQ_LOGEA("read descriptor not initialized for message queue1");
        LOG_FUNCTION_NAME_EXIT;
        return android::NO_INIT;
        }
    pfd[0].events = POLLIN;
    pfd[0].revents = 0;
    if(queue2)
        {
        MSGQ_LOGDA("queue2 not-null");
        pfd[1].fd = queue2->getInFd();
        if(!pfd[1].fd)
            {
            MSGQ_LOGEA("read descriptor not initialized for message queue2");
            LOG_FUNCTION_NAME_EXIT;
            return android::NO_INIT;
            }

        pfd[1].events = POLLIN;
        pfd[1].revents = 0;
        n++;
        }

    if(queue3)
        {
        MSGQ_LOGDA("queue3 not-null");
        pfd[2].fd = queue3->getInFd();
        if(!pfd[2].fd)
            {
            MSGQ_LOGEA("read descriptor not initialized for message queue3");
            LOG_FUNCTION_NAME_EXIT;
            return android::NO_INIT;
            }

        pfd[2].events = POLLIN;
        pfd[2].revents = 0;
        n++;
        }


    int ret = poll(pfd, n, timeout);
    if(ret==0)
        {
        LOG_FUNCTION_NAME_EXIT;
        return ret;
        }

    if(ret<android::NO_ERROR)
        {
        MSGQ_LOGEB("Message queue returned error %d", ret);
        LOG_FUNCTION_NAME_EXIT;
        return ret;
        }

    if (pfd[0].revents & POLLIN)
        {
        queue1->setMsg(true);
        }

    if(queue2)
        {
        if (pfd[1].revents & POLLIN)
            {
            queue2->setMsg(true);
            }
        }

    if(queue3)
        {
        if (pfd[2].revents & POLLIN)
            {
            queue3->setMsg(true);
            }
        }

    LOG_FUNCTION_NAME_EXIT;
    return ret;
    }

};
