/*
Copyright (c) 2009, QUALCOMM USA, INC.

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

·         Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer. 

·         Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution. 

·         Neither the name of the QUALCOMM USA, INC.  nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>

#define LOG_TAG "libgps-rpc"
#include <utils/Log.h>

#define PRINT(x...) do {                                    \
        fprintf(stdout, "%s(%d) ", __FUNCTION__, __LINE__); \
        fprintf(stdout, ##x);                               \
        LOGD(x);                               \
    } while(0)

#ifdef DEBUG
#define D PRINT
#else
#define D(x...) do { } while(0)
#endif

#ifdef VERBOSE
#define V PRINT
#else
#define V(x...) do { } while(0)
#endif

#define E(x...) do {                                        \
        fprintf(stderr, "%s(%d) ", __FUNCTION__, __LINE__); \
        fprintf(stderr, ##x);                               \
        LOGE(x);                                            \
    } while(0)

#define FAILIF(cond, msg...) do {                                              \
        if (__builtin_expect (cond, 0)) {                                      \
            fprintf(stderr, "%s:%s:(%d): ", __FILE__, __FUNCTION__, __LINE__); \
            fprintf(stderr, ##msg);                                            \
            LOGE(##msg);                                                       \
        }                                                                      \
    } while(0)

#endif/*DEBUG_H*/
