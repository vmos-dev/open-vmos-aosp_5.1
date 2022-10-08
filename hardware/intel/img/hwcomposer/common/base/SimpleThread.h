/*
// Copyright (c) 2014 Intel Corporation 
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#ifndef SIMPLE_THREAD_H
#define SIMPLE_THREAD_H

#include <utils/threads.h>

#define DECLARE_THREAD(THREADNAME, THREADOWNER) \
    class THREADNAME: public Thread { \
    public: \
        THREADNAME(THREADOWNER *owner) { mOwner = owner; } \
        THREADNAME() { mOwner = NULL; } \
    private: \
        virtual bool threadLoop() { return mOwner->threadLoop(); } \
    private: \
        THREADOWNER *mOwner; \
    }; \
    friend class THREADNAME; \
    bool threadLoop(); \
    sp<THREADNAME> mThread;


#endif /* SIMPLE_THREAD_H */

