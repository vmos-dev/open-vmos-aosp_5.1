/*--------------------------------------------------------------------------
Copyright (c) 2010-2012, The Linux Foundation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of The Linux Foundation nor
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
#ifndef __DTSPARSER_H
#define __DTSPARSER_H

#include "OMX_Core.h"
#include "OMX_QCOMExtns.h"
#include "qc_omx_component.h"

#include<stdlib.h>

#include <stdio.h>
#include <inttypes.h>

#ifdef _ANDROID_
extern "C" {
#include<utils/Log.h>
}
#else
#define ALOGE(fmt, args...) fprintf(stderr, fmt, ##args)
#endif /* _ANDROID_ */

class omx_time_stamp_reorder
{
    public:
        omx_time_stamp_reorder();
        ~omx_time_stamp_reorder();
        void set_timestamp_reorder_mode(bool flag);
        void enable_debug_print(bool flag);
        bool insert_timestamp(OMX_BUFFERHEADERTYPE *header);
        bool get_next_timestamp(OMX_BUFFERHEADERTYPE *header, bool is_interlaced);
        bool remove_time_stamp(OMX_TICKS ts, bool is_interlaced);
        void flush_timestamp();

    private:
#define TIME_SZ 64
        typedef struct timestamp {
            OMX_TICKS timestamps;
            bool in_use;
        } timestamp;
        typedef struct time_stamp_list {
            timestamp input_timestamps[TIME_SZ];
            time_stamp_list *next;
            time_stamp_list *prev;
            unsigned int entries_filled;
        } time_stamp_list;
        bool error;
        time_stamp_list *phead,*pcurrent;
        bool get_current_list();
        bool add_new_list();
        bool update_head();
        void delete_list();
        void handle_error() {
            ALOGE("Error handler called for TS Parser");

            if (error)
                return;

            error = true;
            delete_list();
        }
        bool reorder_ts;
        bool print_debug;
};
#endif
