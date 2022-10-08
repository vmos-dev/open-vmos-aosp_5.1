/*
 * Copyright (C) 2014 The Android Open Source Project
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

//#define LOG_NDEBUG 0
#define LOG_TAG "audio_utils_primitives_tests"

#include <vector>
#include <cutils/log.h>
#include <gtest/gtest.h>
#include <audio_utils/primitives.h>
#include <audio_utils/format.h>
#include <audio_utils/channels.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

static const int32_t lim16pos = (1 << 15) - 1;
static const int32_t lim16neg = -(1 << 15);
static const int32_t lim24pos = (1 << 23) - 1;
static const int32_t lim24neg = -(1 << 23);

inline void testClamp16(float f)
{
    int16_t ival = clamp16_from_float(f / (1 << 15));

    // test clamping
    ALOGV("clamp16_from_float(%f) = %d\n", f, ival);
    if (f > lim16pos) {
        EXPECT_EQ(ival, lim16pos);
    } else if (f < lim16neg) {
        EXPECT_EQ(ival, lim16neg);
    }

    // if in range, make sure round trip clamp and conversion is correct.
    if (f < lim16pos - 1. && f > lim16neg + 1.) {
        int ival2 = clamp16_from_float(float_from_i16(ival));
        int diff = abs(ival - ival2);
        EXPECT_LE(diff, 1);
    }
}

inline void testClamp24(float f)
{
    int32_t ival = clamp24_from_float(f / (1 << 23));

    // test clamping
    ALOGV("clamp24_from_float(%f) = %d\n", f, ival);
    if (f > lim24pos) {
        EXPECT_EQ(ival, lim24pos);
    } else if (f < lim24neg) {
        EXPECT_EQ(ival, lim24neg);
    }

    // if in range, make sure round trip clamp and conversion is correct.
    if (f < lim24pos - 1. && f > lim24neg + 1.) {
        int ival2 = clamp24_from_float(float_from_q8_23(ival));
        int diff = abs(ival - ival2);
        EXPECT_LE(diff, 1);
    }
}

template<typename T>
void checkMonotone(const T *ary, size_t size)
{
    for (size_t i = 1; i < size; ++i) {
        EXPECT_LT(ary[i-1], ary[i]);
    }
}

TEST(audio_utils_primitives, clamp_to_int) {
    static const float testArray[] = {
            -NAN, -INFINITY,
            -1.e20, -32768., 63.9,
            -3.5, -3.4, -2.5, 2.4, -1.5, -1.4, -0.5, -0.2, 0., 0.2, 0.5, 0.8,
            1.4, 1.5, 1.8, 2.4, 2.5, 2.6, 3.4, 3.5,
            32767., 32768., 1.e20,
            INFINITY, NAN };

    for (size_t i = 0; i < ARRAY_SIZE(testArray); ++i) {
        testClamp16(testArray[i]);
    }
    for (size_t i = 0; i < ARRAY_SIZE(testArray); ++i) {
        testClamp24(testArray[i]);
    }

    // used for ULP testing (tweaking the lsb of the float)
    union {
        int32_t i;
        float f;
    } val;
    int32_t res;

    // check clampq4_27_from_float()
    val.f = 16.;
    res = clampq4_27_from_float(val.f);
    EXPECT_EQ(res, 0x7fffffff);
    val.i--;
    res = clampq4_27_from_float(val.f);
    EXPECT_LE(res, 0x7fffffff);
    EXPECT_GE(res, 0x7fff0000);
    val.f = -16.;
    res = clampq4_27_from_float(val.f);
    EXPECT_EQ(res, (int32_t)0x80000000); // negative
    val.i++;
    res = clampq4_27_from_float(val.f);
    EXPECT_GE(res, (int32_t)0x80000000); // negative
    EXPECT_LE(res, (int32_t)0x80008000); // negative

    // check u4_28_from_float and u4_12_from_float
    uint32_t ures;
    uint16_t ures16;
    val.f = 16.;
    ures = u4_28_from_float(val.f);
    EXPECT_EQ(ures, 0xffffffff);
    ures16 = u4_12_from_float(val.f);
    EXPECT_EQ(ures16, 0xffff);

    val.f = -1.;
    ures = u4_28_from_float(val.f);
    EXPECT_EQ(ures, 0);
    ures16 = u4_12_from_float(val.f);
    EXPECT_EQ(ures16, 0);

    // check float_from_u4_28 and float_from_u4_12 (roundtrip)
    for (uint32_t v = 0x100000; v <= 0xff000000; v += 0x100000) {
        ures = u4_28_from_float(float_from_u4_28(v));
        EXPECT_EQ(ures, v);
    }
    for (uint32_t v = 0; v <= 0xffff; ++v) { // uint32_t prevents overflow
        ures16 = u4_12_from_float(float_from_u4_12(v));
        EXPECT_EQ(ures16, v);
    }
}

TEST(audio_utils_primitives, memcpy) {
    // test round-trip.
    int16_t *i16ref = new int16_t[65536];
    int16_t *i16ary = new int16_t[65536];
    int32_t *i32ary = new int32_t[65536];
    float *fary = new float[65536];
    uint8_t *pary = new uint8_t[65536*3];

    for (size_t i = 0; i < 65536; ++i) {
        i16ref[i] = i16ary[i] = i - 32768;
    }

    // do round-trip testing i16 and float
    memcpy_to_float_from_i16(fary, i16ary, 65536);
    memset(i16ary, 0, 65536 * sizeof(i16ary[0]));
    checkMonotone(fary, 65536);

    memcpy_to_i16_from_float(i16ary, fary, 65536);
    memset(fary, 0, 65536 * sizeof(fary[0]));
    checkMonotone(i16ary, 65536);

    // TODO make a template case for the following?

    // do round-trip testing p24 to i16 and float
    memcpy_to_p24_from_i16(pary, i16ary, 65536);
    memset(i16ary, 0, 65536 * sizeof(i16ary[0]));

    // check an intermediate format at a position(???)
#if 0
    printf("pary[%d].0 = %u  pary[%d].1 = %u  pary[%d].2 = %u\n",
            1025, (unsigned) pary[1025*3],
            1025, (unsigned) pary[1025*3+1],
            1025, (unsigned) pary[1025*3+2]
            );
#endif

    memcpy_to_float_from_p24(fary, pary, 65536);
    memset(pary, 0, 65536 * 3 * sizeof(pary[0]));
    checkMonotone(fary, 65536);

    memcpy_to_p24_from_float(pary, fary, 65536);
    memset(fary, 0, 65536 * sizeof(fary[0]));

    memcpy_to_i16_from_p24(i16ary, pary, 65536);
    memset(pary, 0, 65536 * 3 * sizeof(pary[0]));
    checkMonotone(i16ary, 65536);

    // do round-trip testing q8_23 to i16 and float
    memcpy_to_q8_23_from_i16(i32ary, i16ary, 65536);
    memset(i16ary, 0, 65536 * sizeof(i16ary[0]));
    checkMonotone(i32ary, 65536);

    memcpy_to_float_from_q8_23(fary, i32ary, 65536);
    memset(i32ary, 0, 65536 * sizeof(i32ary[0]));
    checkMonotone(fary, 65536);

    memcpy_to_q8_23_from_float_with_clamp(i32ary, fary, 65536);
    memset(fary, 0, 65536 * sizeof(fary[0]));
    checkMonotone(i32ary, 65536);

    memcpy_to_i16_from_q8_23(i16ary, i32ary, 65536);
    memset(i32ary, 0, 65536 * sizeof(i32ary[0]));
    checkMonotone(i16ary, 65536);

    // do round-trip testing i32 to i16 and float
    memcpy_to_i32_from_i16(i32ary, i16ary, 65536);
    memset(i16ary, 0, 65536 * sizeof(i16ary[0]));
    checkMonotone(i32ary, 65536);

    memcpy_to_float_from_i32(fary, i32ary, 65536);
    memset(i32ary, 0, 65536 * sizeof(i32ary[0]));
    checkMonotone(fary, 65536);

    memcpy_to_i32_from_float(i32ary, fary, 65536);
    memset(fary, 0, 65536 * sizeof(fary[0]));
    checkMonotone(i32ary, 65536);

    memcpy_to_i16_from_i32(i16ary, i32ary, 65536);
    memset(i32ary, 0, 65536 * sizeof(i32ary[0]));
    checkMonotone(i16ary, 65536);

    // do partial round-trip testing q4_27 to i16 and float
    memcpy_to_float_from_i16(fary, i16ary, 65536);
    //memset(i16ary, 0, 65536 * sizeof(i16ary[0])); // not cleared: we don't do full roundtrip

    memcpy_to_q4_27_from_float(i32ary, fary, 65536);
    memset(fary, 0, 65536 * sizeof(fary[0]));
    checkMonotone(i32ary, 65536);

    memcpy_to_float_from_q4_27(fary, i32ary, 65536);
    memset(i32ary, 0, 65536 * sizeof(i32ary[0]));
    checkMonotone(fary, 65536);

    // at the end, our i16ary must be the same. (Monotone should be equivalent to this)
    EXPECT_EQ(memcmp(i16ary, i16ref, 65536*sizeof(i16ary[0])), 0);

    delete[] i16ref;
    delete[] i16ary;
    delete[] i32ary;
    delete[] fary;
    delete[] pary;
}

template<typename T>
void checkMonotoneOrZero(const T *ary, size_t size)
{
    T least = 0;

    for (size_t i = 1; i < size; ++i) {
        if (ary[i]) {
            EXPECT_LT(least, ary[i]);
            least = ary[i];
        }
    }
}

TEST(audio_utils_primitives, memcpy_by_channel_mask) {
    uint32_t dst_mask;
    uint32_t src_mask;
    uint16_t *u16ref = new uint16_t[65536];
    uint16_t *u16ary = new uint16_t[65536];

    for (size_t i = 0; i < 65536; ++i) {
        u16ref[i] = i;
    }

    // Test when src mask is 0.  Everything copied is zero.
    src_mask = 0;
    dst_mask = 0x8d;
    memset(u16ary, 0x99, 65536 * sizeof(u16ref[0]));
    memcpy_by_channel_mask(u16ary, dst_mask, u16ref, src_mask, sizeof(u16ref[0]),
            65536 / popcount(dst_mask));
    EXPECT_EQ(nonZeroMono16((int16_t*)u16ary, 65530), 0);

    // Test when dst_mask is 0.  Nothing should be copied.
    src_mask = 0;
    dst_mask = 0;
    memset(u16ary, 0, 65536 * sizeof(u16ref[0]));
    memcpy_by_channel_mask(u16ary, dst_mask, u16ref, src_mask, sizeof(u16ref[0]),
            65536);
    EXPECT_EQ(nonZeroMono16((int16_t*)u16ary, 65530), 0);

    // Test when masks are the same.  One to one copy.
    src_mask = dst_mask = 0x8d;
    memset(u16ary, 0x99, 65536 * sizeof(u16ref[0]));
    memcpy_by_channel_mask(u16ary, dst_mask, u16ref, src_mask, sizeof(u16ref[0]), 555);
    EXPECT_EQ(memcmp(u16ary, u16ref, 555 * sizeof(u16ref[0]) * popcount(dst_mask)), 0);

    // Test with a gap in source:
    // Input 3 samples, output 4 samples, one zero inserted.
    src_mask = 0x8c;
    dst_mask = 0x8d;
    memset(u16ary, 0x9, 65536 * sizeof(u16ary[0]));
    memcpy_by_channel_mask(u16ary, dst_mask, u16ref, src_mask, sizeof(u16ref[0]),
            65536 / popcount(dst_mask));
    checkMonotoneOrZero(u16ary, 65536);
    EXPECT_EQ(nonZeroMono16((int16_t*)u16ary, 65536), 65536 * 3 / 4 - 1);

    // Test with a gap in destination:
    // Input 4 samples, output 3 samples, one deleted
    src_mask = 0x8d;
    dst_mask = 0x8c;
    memset(u16ary, 0x9, 65536 * sizeof(u16ary[0]));
    memcpy_by_channel_mask(u16ary, dst_mask, u16ref, src_mask, sizeof(u16ref[0]),
            65536 / popcount(src_mask));
    checkMonotone(u16ary, 65536 * 3 / 4);

    delete[] u16ref;
    delete[] u16ary;
}

void memcpy_by_channel_mask2(void *dst, uint32_t dst_mask,
        const void *src, uint32_t src_mask, size_t sample_size, size_t count)
{
    int8_t idxary[32];
    uint32_t src_channels = popcount(src_mask);
    uint32_t dst_channels =
            memcpy_by_index_array_initialization(idxary, 32, dst_mask, src_mask);

    memcpy_by_index_array(dst, dst_channels, src, src_channels, idxary, sample_size, count);
}

// a modified version of the memcpy_by_channel_mask test
// but using 24 bit type and memcpy_by_index_array()
TEST(audio_utils_primitives, memcpy_by_index_array) {
    uint32_t dst_mask;
    uint32_t src_mask;
    typedef struct {uint8_t c[3];} __attribute__((__packed__)) uint8x3_t;
    uint8x3_t *u24ref = new uint8x3_t[65536];
    uint8x3_t *u24ary = new uint8x3_t[65536];
    uint16_t *u16ref = new uint16_t[65536];
    uint16_t *u16ary = new uint16_t[65536];

    EXPECT_EQ(sizeof(uint8x3_t), 3); // 3 bytes per struct

    // tests prepare_index_array_from_masks()
    EXPECT_EQ(memcpy_by_index_array_initialization(NULL, 0, 0x8d, 0x8c), 4);
    EXPECT_EQ(memcpy_by_index_array_initialization(NULL, 0, 0x8c, 0x8d), 3);

    for (size_t i = 0; i < 65536; ++i) {
        u16ref[i] = i;
    }
    memcpy_to_p24_from_i16((uint8_t*)u24ref, (int16_t*)u16ref, 65536);

    // Test when src mask is 0.  Everything copied is zero.
    src_mask = 0;
    dst_mask = 0x8d;
    memset(u24ary, 0x99, 65536 * sizeof(u24ary[0]));
    memcpy_by_channel_mask2(u24ary, dst_mask, u24ref, src_mask, sizeof(u24ref[0]),
            65536 / popcount(dst_mask));
    memcpy_to_i16_from_p24((int16_t*)u16ary, (uint8_t*)u24ary, 65536);
    EXPECT_EQ(nonZeroMono16((int16_t*)u16ary, 65530), 0);

    // Test when dst_mask is 0.  Nothing should be copied.
    src_mask = 0;
    dst_mask = 0;
    memset(u24ary, 0, 65536 * sizeof(u24ary[0]));
    memcpy_by_channel_mask2(u24ary, dst_mask, u24ref, src_mask, sizeof(u24ref[0]),
            65536);
    memcpy_to_i16_from_p24((int16_t*)u16ary, (uint8_t*)u24ary, 65536);
    EXPECT_EQ(nonZeroMono16((int16_t*)u16ary, 65530), 0);

    // Test when masks are the same.  One to one copy.
    src_mask = dst_mask = 0x8d;
    memset(u24ary, 0x99, 65536 * sizeof(u24ary[0]));
    memcpy_by_channel_mask2(u24ary, dst_mask, u24ref, src_mask, sizeof(u24ref[0]), 555);
    EXPECT_EQ(memcmp(u24ary, u24ref, 555 * sizeof(u24ref[0]) * popcount(dst_mask)), 0);

    // Test with a gap in source:
    // Input 3 samples, output 4 samples, one zero inserted.
    src_mask = 0x8c;
    dst_mask = 0x8d;
    memset(u24ary, 0x9, 65536 * sizeof(u24ary[0]));
    memcpy_by_channel_mask2(u24ary, dst_mask, u24ref, src_mask, sizeof(u24ref[0]),
            65536 / popcount(dst_mask));
    memcpy_to_i16_from_p24((int16_t*)u16ary, (uint8_t*)u24ary, 65536);
    checkMonotoneOrZero(u16ary, 65536);
    EXPECT_EQ(nonZeroMono16((int16_t*)u16ary, 65536), 65536 * 3 / 4 - 1);

    // Test with a gap in destination:
    // Input 4 samples, output 3 samples, one deleted
    src_mask = 0x8d;
    dst_mask = 0x8c;
    memset(u24ary, 0x9, 65536 * sizeof(u24ary[0]));
    memcpy_by_channel_mask2(u24ary, dst_mask, u24ref, src_mask, sizeof(u24ref[0]),
            65536 / popcount(src_mask));
    memcpy_to_i16_from_p24((int16_t*)u16ary, (uint8_t*)u24ary, 65536);
    checkMonotone(u16ary, 65536 * 3 / 4);

    delete[] u16ref;
    delete[] u16ary;
    delete[] u24ref;
    delete[] u24ary;
}

TEST(audio_utils_channels, adjust_channels) {
    uint16_t *u16ref = new uint16_t[65536];
    uint16_t *u16expand = new uint16_t[65536*2];
    uint16_t *u16ary = new uint16_t[65536];

    // reference buffer always increases
    for (size_t i = 0; i < 65536; ++i) {
        u16ref[i] = i;
    }

    // expand channels from stereo to quad.
    adjust_channels(u16ref /*in_buff*/, 2 /*in_channels*/,
            u16expand /*out_buff*/, 4 /*out_channels*/,
            sizeof(u16ref[0]) /*sample_size_in_bytes*/,
            sizeof(u16ref[0])*65536 /*num_in_bytes*/);

    // expanded buffer must increase (or be zero)
    checkMonotoneOrZero(u16expand, 65536*2);

    // contract channels back to stereo.
    adjust_channels(u16expand /*in_buff*/, 4 /*in_channels*/,
            u16ary /*out_buff*/, 2 /*out_channels*/,
            sizeof(u16expand[0]) /*sample_size_in_bytes*/,
            sizeof(u16expand[0])*65536*2 /*num_in_bytes*/);

    // must be identical to original.
    EXPECT_EQ(memcmp(u16ary, u16ref, sizeof(u16ref[0])*65536), 0);

    delete[] u16ref;
    delete[] u16expand;
    delete[] u16ary;
}
