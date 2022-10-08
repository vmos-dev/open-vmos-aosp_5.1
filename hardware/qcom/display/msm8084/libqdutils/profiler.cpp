/*
 * Copyright (c) 2011-2012, The Linux Foundation. All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of The Linux Foundation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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

#define LOG_NDDEBUG 0
#define __STDC_FORMAT_MACROS 1
#include <inttypes.h>

#include "profiler.h"

#ifdef DEBUG_CALC_FPS


ANDROID_SINGLETON_STATIC_INSTANCE(qdutils::CalcFps) ;

namespace qdutils {

CalcFps::CalcFps() {
    debug_fps_level = 0;
    Init();
}

CalcFps::~CalcFps() {
}

void CalcFps::Init() {
    char prop[PROPERTY_VALUE_MAX];
    property_get("debug.gr.calcfps", prop, "0");
    debug_fps_level = atoi(prop);
    if (debug_fps_level > MAX_DEBUG_FPS_LEVEL) {
        ALOGW("out of range value for debug.gr.calcfps, using 0");
        debug_fps_level = 0;
    }

    ALOGD("DEBUG_CALC_FPS: %d", debug_fps_level);
    populate_debug_fps_metadata();
}

void CalcFps::Fps() {
    if (debug_fps_level > 0)
        calc_fps(ns2us(systemTime()));
}

void CalcFps::populate_debug_fps_metadata(void)
{
    char prop[PROPERTY_VALUE_MAX];

    /*defaults calculation of fps to based on number of frames*/
    property_get("debug.gr.calcfps.type", prop, "0");
    debug_fps_metadata.type = (debug_fps_metadata_t::DfmType) atoi(prop);

    /*defaults to 1000ms*/
    property_get("debug.gr.calcfps.timeperiod", prop, "1000");
    debug_fps_metadata.time_period = atoi(prop);

    property_get("debug.gr.calcfps.period", prop, "10");
    debug_fps_metadata.period = atoi(prop);

    if (debug_fps_metadata.period > MAX_FPS_CALC_PERIOD_IN_FRAMES) {
        debug_fps_metadata.period = MAX_FPS_CALC_PERIOD_IN_FRAMES;
    }

    /* default ignorethresh_us: 500 milli seconds */
    property_get("debug.gr.calcfps.ignorethresh_us", prop, "500000");
    debug_fps_metadata.ignorethresh_us = atoi(prop);

    debug_fps_metadata.framearrival_steps =
        (unsigned int)(debug_fps_metadata.ignorethresh_us / 16666);

    if (debug_fps_metadata.framearrival_steps > MAX_FRAMEARRIVAL_STEPS) {
        debug_fps_metadata.framearrival_steps = MAX_FRAMEARRIVAL_STEPS;
        debug_fps_metadata.ignorethresh_us =
            debug_fps_metadata.framearrival_steps * 16666;
    }

    /* 2ms margin of error for the gettimeofday */
    debug_fps_metadata.margin_us = 2000;

    for (unsigned int i = 0; i < MAX_FRAMEARRIVAL_STEPS; i++)
        debug_fps_metadata.accum_framearrivals[i] = 0;

    debug_fps_metadata.curr_frame = 0;

    ALOGD("period: %u", debug_fps_metadata.period);
    ALOGD("ignorethresh_us: %"PRId64, debug_fps_metadata.ignorethresh_us);
}

void CalcFps::print_fps(float fps)
{
    if (debug_fps_metadata_t::DFM_FRAMES == debug_fps_metadata.type)
        ALOGD("FPS for last %d frames: %3.2f", debug_fps_metadata.period, fps);
    else
        ALOGD("FPS for last (%f ms, %d frames): %3.2f",
              debug_fps_metadata.time_elapsed,
              debug_fps_metadata.curr_frame, fps);

    debug_fps_metadata.curr_frame = 0;
    debug_fps_metadata.time_elapsed = 0.0;

    if (debug_fps_level > 1) {
        ALOGD("Frame Arrival Distribution:");
        for (unsigned int i = 0;
             i < ((debug_fps_metadata.framearrival_steps / 6) + 1);
             i++) {
            ALOGD("%"PRId64" %"PRId64" %"PRId64" %"PRId64" %"PRId64" %"PRId64,
                  debug_fps_metadata.accum_framearrivals[i*6],
                  debug_fps_metadata.accum_framearrivals[i*6+1],
                  debug_fps_metadata.accum_framearrivals[i*6+2],
                  debug_fps_metadata.accum_framearrivals[i*6+3],
                  debug_fps_metadata.accum_framearrivals[i*6+4],
                  debug_fps_metadata.accum_framearrivals[i*6+5]);
        }

        /* We are done with displaying, now clear the stats */
        for (unsigned int i = 0;
             i < debug_fps_metadata.framearrival_steps;
             i++)
            debug_fps_metadata.accum_framearrivals[i] = 0;
    }
    return;
}

void CalcFps::calc_fps(nsecs_t currtime_us)
{
    static nsecs_t oldtime_us = 0;

    nsecs_t diff = currtime_us - oldtime_us;

    oldtime_us = currtime_us;

    if (debug_fps_metadata_t::DFM_FRAMES == debug_fps_metadata.type &&
        diff > debug_fps_metadata.ignorethresh_us) {
        return;
    }

    if (debug_fps_metadata.curr_frame < MAX_FPS_CALC_PERIOD_IN_FRAMES) {
        debug_fps_metadata.framearrivals[debug_fps_metadata.curr_frame] = diff;
    }

    debug_fps_metadata.curr_frame++;

    if (debug_fps_level > 1) {
        unsigned int currstep =
            (unsigned int)(diff + debug_fps_metadata.margin_us) / 16666;

        if (currstep < debug_fps_metadata.framearrival_steps) {
            debug_fps_metadata.accum_framearrivals[currstep-1]++;
        }
    }

    if (debug_fps_metadata_t::DFM_FRAMES == debug_fps_metadata.type) {
        if (debug_fps_metadata.curr_frame == debug_fps_metadata.period) {
            /* time to calculate and display FPS */
            nsecs_t sum = 0;
            for (unsigned int i = 0; i < debug_fps_metadata.period; i++)
                sum += debug_fps_metadata.framearrivals[i];
            print_fps(float(float(debug_fps_metadata.period * 1000000) /
                                                              (float)sum));
        }
    }
    else if (debug_fps_metadata_t::DFM_TIME == debug_fps_metadata.type) {
        debug_fps_metadata.time_elapsed += (float)((float)diff/1000.0);
        if (debug_fps_metadata.time_elapsed >= debug_fps_metadata.time_period) {
            float fps = float(1000.0 * debug_fps_metadata.curr_frame/
                                            debug_fps_metadata.time_elapsed);
            print_fps(fps);
        }
    }
    return;
}
};//namespace qomutils
#endif
