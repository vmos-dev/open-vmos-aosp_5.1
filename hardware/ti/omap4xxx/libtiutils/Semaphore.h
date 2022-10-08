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



#include <utils/Errors.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

namespace android {

class Semaphore
{
public:

    Semaphore();
    ~Semaphore();

    //Release semaphore
    status_t Release();

    ///Create the semaphore with initial count value
    status_t Create(int count=0);

    ///Wait operation
    status_t Wait();

    ///Signal operation
    status_t Signal();

    ///Current semaphore count
    int Count();

    ///Wait operation with a timeout
    status_t WaitTimeout(int timeoutMicroSecs);

private:
    sem_t *mSemaphore;

};

};
