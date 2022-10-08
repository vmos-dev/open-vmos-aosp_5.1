/*--------------------------------------------------------------------------
Copyright (c) 2012, The Linux Foundation. All rights reserved.

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
#include <dlfcn.h>
#include "C2DColorConverter.h"

using namespace android;
class omx_c2d_conv {
public:
    omx_c2d_conv();
    ~omx_c2d_conv();
    bool init();
    void destroy();
    bool open(unsigned int height,unsigned int width,
              ColorConvertFormat src,
              ColorConvertFormat dest);
    bool convert(int src_fd, void *src_base, void *src_viraddr,
                 int dest_fd, void *dest_base, void *dest_viraddr);
    bool get_buffer_size(int port,unsigned int &buf_size);
    int get_src_format();
    void close();
private:
     C2DColorConverterBase *c2dcc;
    void *mLibHandle;
    ColorConvertFormat src_format;
    createC2DColorConverter_t *mConvertOpen;
    destroyC2DColorConverter_t *mConvertClose;
};
