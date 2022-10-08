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



#include "Semaphore.h"
#include "ErrorUtils.h"
#include <utils/Log.h>
#include <time.h>

namespace android {

/**
   @brief Constructor for the semaphore class

   @param none
   @return none
 */
Semaphore::Semaphore()
{
    ///Initialize the semaphore to NULL
    mSemaphore = NULL;
}

/**
   @brief Destructor of the semaphore class

   @param none
   @return none

 */
Semaphore::~Semaphore()
{
    Release();
}

/**
   @brief: Releases semaphore

   @param count >=0
   @return NO_ERROR On Success
   @return One of the android error codes based on semaphore de-initialization
 */

status_t Semaphore::Release()
{
    int status = 0;

    ///Destroy only if the semaphore has been created
    if(mSemaphore)
        {
        status = sem_destroy(mSemaphore);

        free(mSemaphore);

        mSemaphore = NULL;
        }

    ///Initialize the semaphore and return the status
    return ErrorUtils::posixToAndroidError(status);

}

/**
   @brief Create the semaphore with initial count value

   @param count >=0
   @return NO_ERROR On Success
   @return NO_MEMORY If unable to allocate memory for the semaphore
   @return BAD_VALUE If an invalid count value is passed (<0)
   @return One of the android error codes based on semaphore initialization
 */

status_t Semaphore::Create(int count)
{
    status_t ret = NO_ERROR;

    ///count cannot be less than zero
    if(count<0)
        {
        return BAD_VALUE;
        }

    ret = Release();
    if ( NO_ERROR != ret )
        {
        return ret;
        }

    ///allocate memory for the semaphore
    mSemaphore = (sem_t*)malloc(sizeof(sem_t)) ;

    ///if memory is unavailable, return error
    if(!mSemaphore)
        {
        return NO_MEMORY;
        }

    ///Initialize the semaphore and return the status
    return ErrorUtils::posixToAndroidError(sem_init(mSemaphore, 0x00, count));

}

/**
   @brief Wait operation

   @param none
   @return BAD_VALUE if the semaphore is not initialized
   @return NO_ERROR On success
   @return One of the android error codes based on semaphore wait operation
 */
status_t Semaphore::Wait()
{
    ///semaphore should have been created first
    if(!mSemaphore)
        {
        return BAD_VALUE;
        }

    ///Wait and return the status after signalling
    return ErrorUtils::posixToAndroidError(sem_wait(mSemaphore));


}


/**
   @brief Signal operation

   @param none
     @return BAD_VALUE if the semaphore is not initialized
     @return NO_ERROR On success
     @return One of the android error codes based on semaphore signal operation
   */

status_t Semaphore::Signal()
{
    ///semaphore should have been created first
    if(!mSemaphore)
        {
        return BAD_VALUE;
        }

    ///Post to the semaphore
    return ErrorUtils::posixToAndroidError(sem_post(mSemaphore));

}

/**
   @brief Current semaphore count

   @param none
   @return Current count value of the semaphore
 */
int Semaphore::Count()
{
    int val;

    ///semaphore should have been created first
    if(!mSemaphore)
        {
        return BAD_VALUE;
        }

    ///get the value of the semaphore
    sem_getvalue(mSemaphore, &val);

    return val;
}

/**
   @brief Wait operation with a timeout

     @param timeoutMicroSecs The timeout period in micro seconds
     @return BAD_VALUE if the semaphore is not initialized
     @return NO_ERROR On success
     @return One of the android error codes based on semaphore wait operation
   */

status_t Semaphore::WaitTimeout(int timeoutMicroSecs)
{
    status_t ret = NO_ERROR;

    struct timespec timeSpec;
    struct timeval currentTime;

    ///semaphore should have been created first
    if( NULL == mSemaphore)
        {
        ret = BAD_VALUE;
        }

    if ( NO_ERROR == ret )
        {

        ///setup the timeout values - timeout is specified in seconds and nanoseconds
        gettimeofday(&currentTime, NULL);
        timeSpec.tv_sec = currentTime.tv_sec;
        timeSpec.tv_nsec = currentTime.tv_usec * 1000;
        timeSpec.tv_sec += ( timeoutMicroSecs / 1000000 );
        timeSpec.tv_nsec += ( timeoutMicroSecs % 1000000) * 1000;

        ///Wait for the timeout or signal and return the result based on whichever event occurred first
        ret = sem_timedwait(mSemaphore, &timeSpec);
        }

    if ( NO_ERROR != ret )
      {
        Signal();
        Create(0);
      }

    return ret;
}


};


