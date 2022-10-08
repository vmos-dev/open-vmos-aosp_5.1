/*
 * Copyright (c) 2011, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */



#include <copybit.h>
#include "gralloc_priv.h"
#include "gr.h"

#define COPYBIT_SUCCESS 0
#define COPYBIT_FAILURE -1

int convertYV12toYCrCb420SP(const copybit_image_t *src,private_handle_t *yv12_handle);

/*
 * Function to convert the c2d format into an equivalent Android format
 *
 * @param: source buffer handle
 * @param: destination image
 *
 * @return: return status
 */
int convert_yuv_c2d_to_yuv_android(private_handle_t *hnd,
                                   struct copybit_image_t const *rhs);


/*
 * Function to convert the Android format into an equivalent C2D format
 *
 * @param: source buffer handle
 * @param: destination image
 *
 * @return: return status
 */
int convert_yuv_android_to_yuv_c2d(private_handle_t *hnd,
                                   struct copybit_image_t const *rhs);
