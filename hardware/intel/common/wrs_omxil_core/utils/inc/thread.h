/*
 * thread.h, thread class
 *
 * Copyright (c) 2009-2010 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __THREAD_H
#define __THREAD_H

#include <pthread.h>

class RunnableInterface {
public:
    RunnableInterface() {};
    virtual ~RunnableInterface() {};

    virtual void Run(void) = 0;
};

class Thread : public RunnableInterface {
public:
    Thread();
    Thread(RunnableInterface *r);
    ~Thread();

    int Start(void);
    int Join(void);

protected:
    /*
     * overriden by the derived class
     * when the class is derived from Thread class
     */
    virtual void Run(void);

private:
    static void *Instance(void *);

    RunnableInterface *r;
    pthread_t id;
    bool created;

    pthread_mutex_t lock;
};

#endif /* __THREAD_H */
