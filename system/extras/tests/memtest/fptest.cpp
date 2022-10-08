/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <sched.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/mman.h>

#ifdef __ARM_NEON__
#include <arm_neon.h>
#endif


typedef long long nsecs_t;
static nsecs_t gTime;
float data_f[1024 * 128];

static nsecs_t system_time()
{
    struct timespec t;
    t.tv_sec = t.tv_nsec = 0;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return nsecs_t(t.tv_sec)*1000000000LL + t.tv_nsec;
}

static void startTime()
{
    gTime = system_time();
}

static void endTime(const char *str, double ops)
{
    nsecs_t t = system_time() - gTime;
    double ds = ((double)t) / 1e9;
    printf("Test: %s, %f Mops\n", str, ops / ds / 1e6);
}


static void test_mad() {
    for(int i=0; i<1020; i++) {
        data_f[i] = i;
    }

    startTime();

    float total = 0;
    // Do ~1 billion ops
    for (int ct=0; ct < (1000 * (1000 / 20)); ct++) {
        for (int i=0; i < 1000; i++) {
            data_f[i] = (data_f[i] * 0.02f +
                         data_f[i+1] * 0.04f +
                         data_f[i+2] * 0.05f +
                         data_f[i+3] * 0.1f +
                         data_f[i+4] * 0.2f +
                         data_f[i+5] * 0.2f +
                         data_f[i+6] * 0.1f +
                         data_f[i+7] * 0.05f +
                         data_f[i+8] * 0.04f +
                         data_f[i+9] * 0.02f + 1.f);
        }
    }

    endTime("scalar mad", 1e9);
}


#ifdef __ARM_NEON__

static void test_fma() {
    for(int i=0; i<1020 * 4; i++) {
        data_f[i] = i;
    }
    float32x4_t c0_02 = vdupq_n_f32(0.02f);
    float32x4_t c0_04 = vdupq_n_f32(0.04f);
    float32x4_t c0_05 = vdupq_n_f32(0.05f);
    float32x4_t c0_10 = vdupq_n_f32(0.1f);
    float32x4_t c0_20 = vdupq_n_f32(0.2f);
    float32x4_t c1_00 = vdupq_n_f32(1.0f);

    startTime();

    float total = 0;
    // Do ~1 billion ops
    for (int ct=0; ct < (1000 * (1000 / 80)); ct++) {
        for (int i=0; i < 1000; i++) {
            float32x4_t t;
            t = vmulq_f32(vld1q_f32((float32_t *)&data_f[i]), c0_02);
            t = vmlaq_f32(t, vld1q_f32((float32_t *)&data_f[i+4]), c0_04);
            t = vmlaq_f32(t, vld1q_f32((float32_t *)&data_f[i+8]), c0_05);
            t = vmlaq_f32(t, vld1q_f32((float32_t *)&data_f[i+12]), c0_10);
            t = vmlaq_f32(t, vld1q_f32((float32_t *)&data_f[i+16]), c0_20);
            t = vmlaq_f32(t, vld1q_f32((float32_t *)&data_f[i+20]), c0_20);
            t = vmlaq_f32(t, vld1q_f32((float32_t *)&data_f[i+24]), c0_10);
            t = vmlaq_f32(t, vld1q_f32((float32_t *)&data_f[i+28]), c0_05);
            t = vmlaq_f32(t, vld1q_f32((float32_t *)&data_f[i+32]), c0_04);
            t = vmlaq_f32(t, vld1q_f32((float32_t *)&data_f[i+36]), c0_02);
            t = vaddq_f32(t, c1_00);
            vst1q_f32((float32_t *)&data_f[i], t);
        }
    }

    endTime("neon fma", 1e9);
}
#endif

int fp_test(int argc, char** argv) {
    test_mad();

#ifdef __ARM_NEON__
    test_fma();
#endif

    return 0;
}




