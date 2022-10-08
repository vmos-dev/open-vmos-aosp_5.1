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
#ifndef WSBM_WRAPPER_H
#define WSBM_WRAPPER_H

#if defined(__cplusplus)
extern "C" {
#endif

extern int psbWsbmInitialize(int drmFD);
extern void psbWsbmTakedown();
extern int psbWsbmAllocateFromUB(uint32_t size, uint32_t align, void ** buf, void *user_pt);
extern int psbWsbmAllocateTTMBuffer(uint32_t size, uint32_t align,void ** buf);
extern int psbWsbmDestroyTTMBuffer(void * buf);
extern void * psbWsbmGetCPUAddress(void * buf);
extern uint32_t psbWsbmGetGttOffset(void * buf);
extern int psbWsbmWrapTTMBuffer(uint32_t handle, void **buf);
extern int psbWsbmWrapTTMBuffer2(uint32_t handle, void **buf);
extern int psbWsbmCreateFromUB(void *buf, uint32_t size, void *vaddr);
extern int psbWsbmUnReference(void *buf);
extern int psbWsbmWaitIdle(void *buf);
uint32_t psbWsbmGetKBufHandle(void *buf);

#if defined(__cplusplus)
}
#endif

#endif /*WSBM_WRAPPER_H*/
