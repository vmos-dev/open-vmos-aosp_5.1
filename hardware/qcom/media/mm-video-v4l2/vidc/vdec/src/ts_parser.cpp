/*--------------------------------------------------------------------------
Copyright (c) 2010-2013, The Linux Foundation. All rights reserved.

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
#include "ts_parser.h"
#include "vidc_debug.h"

#define DEBUG DEBUG_PRINT_ERROR

void omx_time_stamp_reorder::set_timestamp_reorder_mode(bool mode)
{
    reorder_ts = mode;
}

void omx_time_stamp_reorder::enable_debug_print(bool flag)
{
    print_debug = flag;
}

omx_time_stamp_reorder::~omx_time_stamp_reorder()
{
    delete_list();
}

omx_time_stamp_reorder::omx_time_stamp_reorder()
{
    reorder_ts = false;
    phead = pcurrent = NULL;
    error = false;
    print_debug = false;
}

void omx_time_stamp_reorder::delete_list()
{
    time_stamp_list *ptemp;

    if (!phead) return;

    while (phead->next != phead) {
        ptemp = phead;
        phead = phead->next;
        phead->prev = ptemp->prev;
        ptemp->prev->next = phead;
        delete ptemp;
    }

    delete phead;
    phead = NULL;
}

bool omx_time_stamp_reorder::get_current_list()
{
    if (!phead) {
        if (!add_new_list()) {
            handle_error();
            return false;
        }
    }

    pcurrent = phead->prev;
    return true;
}

bool omx_time_stamp_reorder::update_head()
{
    time_stamp_list *ptemp;

    if (!phead) return false;

    if (phead->next != phead) {
        ptemp = phead;
        phead = ptemp->next;
        phead->prev = ptemp->prev;
        ptemp->prev->next = phead;
        delete ptemp;
    }

    return true;
}

bool omx_time_stamp_reorder::add_new_list()
{
    bool status = true;
    time_stamp_list *ptemp = NULL;

    if (!phead) {
        ptemp = phead = new time_stamp_list;

        if (!phead) {
            handle_error();
            status = false;
            return status;
        }

        phead->prev = phead->next = phead;
    } else {
        ptemp = new time_stamp_list;

        if (!ptemp) {
            handle_error();
            status = false;
            return status;
        }

        ptemp->prev = phead->prev;
        ptemp->next = phead;
        phead->prev->next = ptemp;
        phead->prev = ptemp;
    }

    ptemp->entries_filled = 0;

    for (int i=0; i < TIME_SZ; i++) {
        ptemp->input_timestamps[i].in_use = false;
        ptemp->input_timestamps[i].timestamps = -1;
    }

    return status;
}

bool omx_time_stamp_reorder::insert_timestamp(OMX_BUFFERHEADERTYPE *header)
{
    OMX_TICKS *table_entry = NULL;

    if (!reorder_ts || error || !header) {
        if (error || !header)
            DEBUG("Invalid condition in insert_timestamp %p", header);

        return false;
    }

    if (!get_current_list()) {
        handle_error();
        return false;
    }

    if (pcurrent->entries_filled > (TIME_SZ - 1)) {
        DEBUG("Table full return error");
        handle_error();
        return false;
    }

    if (header->nFlags & OMX_BUFFERFLAG_CODECCONFIG) {
        return true;
    }

    if ((header->nFlags & OMX_BUFFERFLAG_EOS) && !header->nFilledLen) {
        DEBUG("EOS with zero length recieved");

        if (!add_new_list()) {
            handle_error();
            return false;
        }

        return true;
    }

    for (int i = 0; i < TIME_SZ && !table_entry; i++) {
        if (!pcurrent->input_timestamps[i].in_use) {
            table_entry = &pcurrent->input_timestamps[i].timestamps;
            pcurrent->input_timestamps[i].in_use = true;
            pcurrent->entries_filled++;
        }
    }

    if (!table_entry) {
        DEBUG("All entries in use");
        handle_error();
        return false;
    }

    *table_entry = header->nTimeStamp;

    if (print_debug)
        DEBUG("Time stamp inserted %lld", header->nTimeStamp);

    if (header->nFlags & OMX_BUFFERFLAG_EOS) {
        if (!add_new_list()) {
            handle_error();
            return false;
        }
    }

    return true;
}

bool omx_time_stamp_reorder::remove_time_stamp(OMX_TICKS ts, bool is_interlaced = false)
{
    unsigned int num_ent_remove = (is_interlaced)?2:1;

    if (!reorder_ts || error) {
        DEBUG("not in avi mode");
        return false;
    }

    if (!phead || !phead->entries_filled) return false;

    for (int i=0; i < TIME_SZ && num_ent_remove; i++) {
        if (phead->input_timestamps[i].in_use && phead->input_timestamps[i].timestamps == ts) {
            phead->input_timestamps[i].in_use = false;
            phead->entries_filled--;
            num_ent_remove--;

            if (print_debug)
                DEBUG("Removed TS %lld", ts);
        }
    }

    if (!phead->entries_filled) {
        if (!update_head()) {
            handle_error();
            return false;
        }
    }

    return true;
}

void omx_time_stamp_reorder::flush_timestamp()
{
    delete_list();
}

bool omx_time_stamp_reorder::get_next_timestamp(OMX_BUFFERHEADERTYPE *header, bool is_interlaced)
{
    timestamp *element = NULL,*duplicate = NULL;
    bool status = false;

    if (!reorder_ts || error || !header) {
        if (error || !header)
            DEBUG("Invalid condition in insert_timestamp %p", header);

        return false;
    }

    if (!phead || !phead->entries_filled) return false;

    for (int i=0; i < TIME_SZ; i++) {
        if (phead->input_timestamps[i].in_use) {
            status = true;

            if (!element)
                element = &phead->input_timestamps[i];
            else {
                if (element->timestamps > phead->input_timestamps[i].timestamps) {
                    element = &phead->input_timestamps[i];
                    duplicate = NULL;
                } else if (element->timestamps == phead->input_timestamps[i].timestamps)
                    duplicate = &phead->input_timestamps[i];
            }
        }
    }

    if (element) {
        phead->entries_filled--;
        header->nTimeStamp = element->timestamps;

        if (print_debug)
            DEBUG("Getnext Time stamp %lld", header->nTimeStamp);

        element->in_use = false;
    }

    if (is_interlaced && duplicate) {
        phead->entries_filled--;
        duplicate->in_use = false;
    } else if (is_interlaced && !duplicate) {
        element = NULL;

        for (int i=0; i < TIME_SZ; i++) {
            if (phead->input_timestamps[i].in_use) {
                if (!element)
                    element = &phead->input_timestamps[i];
                else if (element->timestamps > phead->input_timestamps[i].timestamps)
                    element = &phead->input_timestamps[i];
            }
        }

        if (element) {
            phead->entries_filled--;
            header->nTimeStamp = element->timestamps;
            element->in_use = false;
        }
    }

    if (!phead->entries_filled) {
        if (!update_head()) {
            handle_error();
            return false;
        }
    }

    return status;
}
