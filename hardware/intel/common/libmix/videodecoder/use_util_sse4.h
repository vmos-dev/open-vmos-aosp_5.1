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

#include <emmintrin.h>
#include <x86intrin.h>

inline void stream_memcpy(void* dst_buff, const void* src_buff, size_t size)
{
    bool isAligned = (((size_t)(src_buff) | (size_t)(dst_buff)) & 0xF) == 0;
    if (!isAligned) {
        memcpy(dst_buff, src_buff, size);
        return;
    }

    static const size_t regs_count = 8;

    __m128i xmm_data0, xmm_data1, xmm_data2, xmm_data3;
    __m128i xmm_data4, xmm_data5, xmm_data6, xmm_data7;

    size_t remain_data = size & (regs_count * sizeof(xmm_data0) - 1);
    size_t end_position = 0;

    __m128i* pWb_buff = (__m128i*)dst_buff;
    __m128i* pWb_buff_end = pWb_buff + ((size - remain_data) >> 4);
    __m128i* pWc_buff = (__m128i*)src_buff;

    /*sync the wc memory data*/
    _mm_mfence();

    while (pWb_buff < pWb_buff_end)
    {
        xmm_data0  = _mm_stream_load_si128(pWc_buff);
        xmm_data1  = _mm_stream_load_si128(pWc_buff + 1);
        xmm_data2  = _mm_stream_load_si128(pWc_buff + 2);
        xmm_data3  = _mm_stream_load_si128(pWc_buff + 3);
        xmm_data4  = _mm_stream_load_si128(pWc_buff + 4);
        xmm_data5  = _mm_stream_load_si128(pWc_buff + 5);
        xmm_data6  = _mm_stream_load_si128(pWc_buff + 6);
        xmm_data7  = _mm_stream_load_si128(pWc_buff + 7);

        pWc_buff += regs_count;
        _mm_store_si128(pWb_buff, xmm_data0);
        _mm_store_si128(pWb_buff + 1, xmm_data1);
        _mm_store_si128(pWb_buff + 2, xmm_data2);
        _mm_store_si128(pWb_buff + 3, xmm_data3);
        _mm_store_si128(pWb_buff + 4, xmm_data4);
        _mm_store_si128(pWb_buff + 5, xmm_data5);
        _mm_store_si128(pWb_buff + 6, xmm_data6);
        _mm_store_si128(pWb_buff + 7, xmm_data7);

        pWb_buff += regs_count;
    }

    /*copy data by 16 bytes step from the remainder*/
    if (remain_data >= 16)
    {
        size = remain_data;
        remain_data = size & 15;
        end_position = size >> 4;
        for (size_t i = 0; i < end_position; ++i)
        {
            pWb_buff[i] = _mm_stream_load_si128(pWc_buff + i);
        }
    }

    /*copy the remainder data, if it still existed*/
    if (remain_data)
    {
        __m128i temp_data = _mm_stream_load_si128(pWc_buff + end_position);

        char* psrc_buf = (char*)(&temp_data);
        char* pdst_buf = (char*)(pWb_buff + end_position);

        for (size_t i = 0; i < remain_data; ++i)
        {
            pdst_buf[i] = psrc_buf[i];
        }
    }

}
