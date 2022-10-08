/*
* Copyright (c) 2009-2011 Intel Corporation.  All rights reserved.
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

#ifndef VIDEO_FRAME_INFO_H_
#define VIDEO_FRAME_INFO_H_

#define MAX_NUM_NALUS 16

typedef struct {
    uint8_t  type;      // nalu type + nal_ref_idc
    uint32_t offset;    // offset to the pointer of the encrypted data
    uint8_t* data;      // if the nalu is encrypted, this field is useless; if current NALU is SPS/PPS, data is the pointer to clear SPS/PPS data
    uint32_t length;    // nalu length
} nalu_info_t;

typedef struct {
    uint8_t* data;      // pointer to the encrypted data
    uint32_t size;      // encrypted data size
    uint32_t num_nalus; // number of NALU
    nalu_info_t nalus[MAX_NUM_NALUS];
} frame_info_t;

#endif
