/*--------------------------------------------------------------------------
Copyright (c) 2010-2011, Code Aurora Forum. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Code Aurora nor
      the names of its contributors may be used to endorse or promote
      products derived from this software without specific prior written
      permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
--------------------------------------------------------------------------*/

#ifndef __OMX_VIDEO_COMMON_H__
#define __OMX_VIDEO_COMMON_H__
//////////////////////////////////////////////////////////////////////////////
//                             Include Files
//////////////////////////////////////////////////////////////////////////////

#include<stdlib.h>
#include <stdio.h>
#ifdef USE_ION
#include <linux/msm_ion.h>
#endif

#define OMX_VIDEO_DEC_NUM_INPUT_BUFFERS   2
#define OMX_VIDEO_DEC_NUM_OUTPUT_BUFFERS  2

#ifdef FEATURE_QTV_WVGA_ENABLE
#define OMX_VIDEO_DEC_INPUT_BUFFER_SIZE   (256*1024)
#else
#define OMX_VIDEO_DEC_INPUT_BUFFER_SIZE   (128*1024)
#endif

#define OMX_CORE_CONTROL_CMDQ_SIZE   100
#define OMX_CORE_QCIF_HEIGHT         144
#define OMX_CORE_QCIF_WIDTH          176
#define OMX_CORE_VGA_HEIGHT          480
#define OMX_CORE_VGA_WIDTH           640
#define OMX_CORE_WVGA_HEIGHT         480
#define OMX_CORE_WVGA_WIDTH          800

enum PortIndexType
{
  PORT_INDEX_IN = 0,
  PORT_INDEX_OUT = 1,
  PORT_INDEX_BOTH = -1,
  PORT_INDEX_NONE = -2
};

struct pmem
{
  void *buffer;
  int fd;
  unsigned offset;
  unsigned size;
};
#ifdef USE_ION
struct venc_ion
{
    int ion_device_fd;
    struct ion_fd_data fd_ion_data;
    struct ion_allocation_data ion_alloc_data;
};
#endif
#endif // __OMX_VIDEO_COMMON_H__




