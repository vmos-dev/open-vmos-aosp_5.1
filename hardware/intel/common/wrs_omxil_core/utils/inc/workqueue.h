/*
 * workqueue.h, workqueue class
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

#ifndef __WORKQUEUE_H
#define __WORKQUEUE_H

#include <pthread.h>
#include <list.h>

#include <thread.h>

class WorkableInterface {
public:
    virtual ~WorkableInterface() {};

    virtual void Work(void) = 0;
};

class WorkQueue : public Thread, public WorkableInterface
{
public:
    WorkQueue();
    /*
     * if WorkQueue has the pending works not proccessed yet,
     * WorkQueue::Run() calls its own Work() instead of the derived class's.
     * becaused a derived class does not exist.
     */
    ~WorkQueue();

    /* start & stop & pause & resume work thread */
    int StartWork(bool executing);
    void StopWork(void);
    void PauseWork(void);
    void ResumeWork(void);

    /* the class inheriting WorkQueue uses this method */
    void ScheduleWork(void);
    /* the class implementing WorkableInterface uses this method */
    void ScheduleWork(WorkableInterface *wi);
    /*
     * FIXME (BUG)
     *  must be called before the class implementing WorkableInterface or
     *  inheriting WorkQueue is destructed,
     *  and don't call ScheduleWork() anymore right before destructing
     *  the class.
     */
    void FlushWork(void);
    /* remove all scheduled works matched with wi from workqueue list */
    void CancelScheduledWork(WorkableInterface *wi);

private:
    /* inner class for flushing */
    class FlushBarrier : public WorkableInterface
    {
    public:
        FlushBarrier();
        ~FlushBarrier();

        /*
         * FIXME (BUG)
         *  it has a potential bug that signal() could be called earlier
         *  than wait() could be.
         */
        void WaitCompletion(void);

    private:
        virtual void Work(void); /* WorkableInterface */

        pthread_mutex_t cplock;
        pthread_cond_t complete;
    };

    virtual void Run(void); /* RunnableInterface */

    /* overriden by the class inheriting WorkQueue class */
    virtual void Work(void); /* WorkableInterface */

    /*
     * FIXME (BUG)
     *  if a class implementing WorkableInterface disapears earlier
     *  than WorkQueue then wi is not valid anymore and causes fault.
     */
    void DoWork(WorkableInterface *wi);

    struct list *works;
    pthread_mutex_t wlock;
    pthread_cond_t wcond;

    /* executing & pause */
    bool wait_for_works;
    bool executing;

    pthread_mutex_t executing_lock;
    pthread_cond_t executing_wait;
    pthread_cond_t paused_wait;

    int stop;
};

#endif /* __WORKQUEUE_H */
