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
#ifndef WSBM_H__
#define WSBM_H__

#include <ips/common/WsbmWrapper.h>

/**
 * Class: WSBM
 * A wrapper class to use libwsbm functionalities
 */
class Wsbm
{
private:
    int mDrmFD;
public:
    Wsbm(int drmFD);
    ~Wsbm();
    bool initialize();
    void deinitialize();
    bool allocateTTMBuffer(uint32_t size, uint32_t align,void ** buf);
    bool allocateTTMBufferUB(uint32_t size, uint32_t align, void ** buf, void *user_pt);
    bool destroyTTMBuffer(void * buf);
    void * getCPUAddress(void * buf);
    uint32_t getGttOffset(void * buf);
    bool wrapTTMBuffer(uint32_t handle, void **buf);
    bool unreferenceTTMBuffer(void *buf);
    bool waitIdleTTMBuffer(void *buf);
    uint32_t getKBufHandle(void *buf);
private:
    bool mInitialized;
};

#endif /*__INTEL_WSBM_H__*/
