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

#ifndef INCLUDE_PROFILER
#define INCLUDE_PROFILER

#include <stdio.h>
#include <utils/Singleton.h>
#include <cutils/properties.h>
#include <cutils/log.h>

#ifndef DEBUG_CALC_FPS
#define CALC_FPS() ((void)0)
#define CALC_INIT() ((void)0)
#else
#define CALC_FPS() qdutils::CalcFps::getInstance().Fps()
#define CALC_INIT() qdutils::CalcFps::getInstance().Init()
using namespace android;
namespace qdutils {
class CalcFps : public Singleton<CalcFps> {
    public:
    CalcFps();
    ~CalcFps();

    void Init();
    void Fps();

    private:
    static const unsigned int MAX_FPS_CALC_PERIOD_IN_FRAMES = 128;
    static const unsigned int MAX_FRAMEARRIVAL_STEPS = 50;
    static const unsigned int MAX_DEBUG_FPS_LEVEL = 2;

    struct debug_fps_metadata_t {
        /*fps calculation based on time or number of frames*/
        enum DfmType {
            DFM_FRAMES = 0,
            DFM_TIME   = 1,
        };

        DfmType type;

        /* indicates how much time do we wait till we calculate FPS */
        unsigned long time_period;

        /*indicates how much time elapsed since we report fps*/
        float time_elapsed;

        /* indicates how many frames do we wait till we calculate FPS */
        unsigned int period;
        /* current frame, will go upto period, and then reset */
        unsigned int curr_frame;
        /* frame will arrive at a multiple of 16666 us at the display.
           This indicates how many steps to consider for our calculations.
           For example, if framearrival_steps = 10, then the frame that arrived
           after 166660 us or more will be ignored.
           */
        unsigned int framearrival_steps;
        /* ignorethresh_us = framearrival_steps * 16666 */
        nsecs_t      ignorethresh_us;
        /* used to calculate the actual frame arrival step, the times might not be
           accurate
           */
        unsigned int margin_us;

        /* actual data storage */
        nsecs_t      framearrivals[MAX_FPS_CALC_PERIOD_IN_FRAMES];
        nsecs_t      accum_framearrivals[MAX_FRAMEARRIVAL_STEPS];
    };

    private:
    void populate_debug_fps_metadata(void);
    void print_fps(float fps);
    void calc_fps(nsecs_t currtime_us);

    private:
    debug_fps_metadata_t debug_fps_metadata;
    unsigned int debug_fps_level;
};
};//namespace qdutils
#endif

#endif // INCLUDE_PROFILER
