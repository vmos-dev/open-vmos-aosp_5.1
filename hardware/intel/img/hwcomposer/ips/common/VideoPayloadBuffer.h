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
#ifndef VIDEO_PAYLOAD_BUFFER_H
#define VIDEO_PAYLOAD_BUFFER_H

#include <utils/Timers.h>
namespace android {
namespace intel {

struct VideoPayloadBuffer {
    // transform made by clients (clients to hwc)
    int client_transform;
    int metadata_transform;
    int rotated_width;
    int rotated_height;
    int surface_protected;
    int force_output_method;
    uint32_t rotated_buffer_handle;
    uint32_t renderStatus;
    unsigned int used_by_widi;
    int bob_deinterlace;
    int tiling;
    uint32_t width;
    uint32_t height;
    uint32_t luma_stride;
    uint32_t chroma_u_stride;
    uint32_t chroma_v_stride;
    uint32_t format;
    uint32_t khandle;
    int64_t  timestamp;

    uint32_t rotate_luma_stride;
    uint32_t rotate_chroma_u_stride;
    uint32_t rotate_chroma_v_stride;

    nsecs_t hwc_timestamp;
    uint32_t layer_transform;

    void *native_window;
    uint32_t scaling_khandle;
    uint32_t scaling_width;
    uint32_t scaling_height;

    uint32_t scaling_luma_stride;
    uint32_t scaling_chroma_u_stride;
    uint32_t scaling_chroma_v_stride;

    uint32_t crop_width;
    uint32_t crop_height;

    uint32_t coded_width;
    uint32_t coded_height;
};


// force output method values
enum {
    FORCE_OUTPUT_INVALID = 0,
    FORCE_OUTPUT_GPU,
    FORCE_OUTPUT_OVERLAY,
    FORCE_OUTPUT_SW_DECODE,
};


} // namespace intel
} // namespace android


#endif // VIDEO_PAYLOAD_BUFFER_H


