/*
 *
 * Copyright 2012 Samsung Electronics S.LSI Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License")
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

/*
 * @file    csc_tiled_to_linear_y.s
 * @brief   SEC_OMX specific define. It support MFC 6.x tiled.
 * @author  ShinWon Lee (shinwon.lee@samsung.com)
 * @version 1.0
 * @history
 *   2012.02.01 : Create
 */

/*
 * Converts tiled data to linear for mfc 6.x
 * 1. Y of NV12T to Y of YUV420P
 * 2. Y of NV12T to Y of YUV420S
 *
 * @param dst
 *   Y address of YUV420[out]
 *
 * @param src
 *   Y address of NV12T[in]
 *
 * @param yuv420_width
 *   real width of YUV420[in]. It should be even.
 *
 * @param yuv420_height
 *   real height of YUV420[in] It should be even.
 *
 */
    .arch armv7-a
    .text
    .global csc_tiled_to_linear_y_neon
    .type   csc_tiled_to_linear_y_neon, %function
csc_tiled_to_linear_y_neon:
    .fnstart

    .equ CACHE_LINE_SIZE, 64
    .equ PRE_LOAD_OFFSET, 6

    @r0     y_dst
    @r1     y_src
    @r2     width
    @r3     height
    @r4     temp3
    @r5     i
    @r6     j
    @r7     dst_offset
    @r8     src_offset
    @r9     aligned_height
    @r10    aligned_width
    @r11    tiled_width
    @r12    temp1
    @r14    temp2

    stmfd       sp!, {r4-r12,r14}       @ backup registers
    ldr         r4, [sp, #40]           @ r4 = height

    bic         r9, r3, #0xF            @ aligned_height = height & (~0xF)
    bic         r10, r2, #0xF           @ aligned_width = width & (~0xF)
    add         r11, r2, #15            @ tiled_width = ((width + 15) >> 4) << 4
    mov         r11, r11, asr #4
    mov         r11, r11, lsl #4

    mov         r5, #0
LOOP_MAIN_ALIGNED_HEIGHT:
    mul         r8, r11, r5             @ src_offset = tiled_width * i
    mov         r6, #0
    add         r8, r1, r8              @ src_offset = y_src + src_offset
LOOP_MAIN_ALIGNED_WIDTH:
    pld         [r8, #(CACHE_LINE_SIZE*PRE_LOAD_OFFSET)]
    vld1.8      {q0, q1}, [r8]!
    vld1.8      {q2, q3}, [r8]!
    pld         [r8, #(CACHE_LINE_SIZE*PRE_LOAD_OFFSET)]
    vld1.8      {q4, q5}, [r8]!
    vld1.8      {q6, q7}, [r8]!
    mul         r12, r2, r5             @ temp1 = width * i + j;
    pld         [r8, #(CACHE_LINE_SIZE*PRE_LOAD_OFFSET)]
    vld1.8      {q8, q9}, [r8]!
    add         r12, r12, r6
    vld1.8      {q10, q11}, [r8]!
    add         r7, r0, r12             @ dst_offset = y_dst + temp1
    pld         [r8, #(CACHE_LINE_SIZE*PRE_LOAD_OFFSET)]
    vld1.8      {q12, q13}, [r8]!
    vld1.8      {q14, q15}, [r8]!

    vst1.8      {q0}, [r7], r2
    vst1.8      {q1}, [r7], r2
    vst1.8      {q2}, [r7], r2
    vst1.8      {q3}, [r7], r2
    vst1.8      {q4}, [r7], r2
    vst1.8      {q5}, [r7], r2
    vst1.8      {q6}, [r7], r2
    vst1.8      {q7}, [r7], r2
    vst1.8      {q8}, [r7], r2
    vst1.8      {q9}, [r7], r2
    vst1.8      {q10}, [r7], r2
    vst1.8      {q11}, [r7], r2
    vst1.8      {q12}, [r7], r2
    vst1.8      {q13}, [r7], r2
    add         r6, r6, #16
    vst1.8      {q14}, [r7], r2
    cmp         r6, r10
    vst1.8      {q15}, [r7], r2
    blt         LOOP_MAIN_ALIGNED_WIDTH

MAIN_REMAIN_WIDTH_START:
    cmp         r10, r2                 @ if (aligned_width != width) {
    beq         MAIN_REMAIN_WIDTH_END

    mul         r8, r11, r5             @ src_offset = (tiled_width * i) + (j << 4);
    add         r8, r8, r6, lsl #4
    add         r8, r1, r8              @ r8 = y_src + src_offset

    mul         r12, r2, r5             @ temp1 = width * i + j;
    add         r12, r12, r6
    add         r7, r0, r12             @ r7 = y_dst + temp1
    sub         r14, r2, r6             @ r14 = width - j

    stmfd       sp!, {r0-r1}            @ backup registers
    mov         r1, #0
LOOP_MAIN_REMAIN_HEIGHT:
    mov         r0, #0                  @ r0 is index in memcpy
LOOP_MAIN_REMAIN_WIDTH:
    ldrh        r4, [r8], #2
    strh        r4, [r7], #2
    add         r0, #2
    cmp         r0, r14
    blt         LOOP_MAIN_REMAIN_WIDTH

    sub         r8, r8, r14
    sub         r7, r7, r14
    add         r8, r8, #16
    add         r7, r7, r2

    add         r1, #1
    cmp         r1, #16
    blt         LOOP_MAIN_REMAIN_HEIGHT
    ldmfd       sp!, {r0-r1}            @ restore registers
MAIN_REMAIN_WIDTH_END:

    add         r5, r5, #16
    cmp         r5, r9
    blt         LOOP_MAIN_ALIGNED_HEIGHT

REMAIN_HEIGHT_START:
    cmp         r9, r3                  @ if (aligned_height != height) {
    beq         REMAIN_HEIGHT_END

    mov         r6, #0
LOOP_REMAIN_HEIGHT_WIDTH16:
    mul         r8, r11, r5             @ src_offset = (tiled_width * i) + (j << 4)
    add         r8, r8, r6, lsl #4
    add         r8, r1, r8              @ src_offset = y_src + src_offset

    mul         r12, r2, r5             @ temp1 = width * i + j;
    add         r12, r12, r6
    add         r7, r0, r12             @ r7 = y_dst + temp1

    sub         r12, r3, r9
    mov         r14, #0
LOOP_REMAIN_HEIGHT_WIDTH16_HEIGHT1:
    vld1.8      {q0}, [r8]!
    vld1.8      {q1}, [r8]!
    vst1.8      {q0}, [r7], r2
    vst1.8      {q1}, [r7], r2

    add         r14, r14, #2
    cmp         r14, r12
    blt         LOOP_REMAIN_HEIGHT_WIDTH16_HEIGHT1

    add         r6, r6, #16
    cmp         r6, r10
    blt         LOOP_REMAIN_HEIGHT_WIDTH16

REMAIN_HEIGHT_REMAIN_WIDTH_START:
    cmp         r10, r2
    beq         REMAIN_HEIGHT_REMAIN_WIDTH_END
    mul         r8, r11, r5             @ src_offset = (tiled_width * i) + (j << 4)
    add         r8, r8, r6, lsl #4
    add         r8, r1, r8              @ src_offset = y_src + src_offset

    mul         r12, r2, r5             @ temp1 = width * i + j;
    add         r12, r12, r6
    add         r7, r0, r12             @ r7 = y_dst + temp1

    stmfd       sp!, {r0-r1,r3}         @ backup registers
    mov         r0, #0
    sub         r1, r3, r9
LOOP_REMAIN_HEIGHT_REMAIN_WIDTH_HEIGHT1:

    sub         r14, r2, r6
    mov         r4, #0
LOOP_REMAIN_HEIGHT_REMAIN_WIDTH_HEIGHT1_WIDTHx:
    ldrh        r3, [r8], #2
    strh        r3, [r7], #2
    add         r4, #2
    cmp         r4, r14
    blt         LOOP_REMAIN_HEIGHT_REMAIN_WIDTH_HEIGHT1_WIDTHx

    sub         r8, r8, r14
    sub         r7, r7, r14
    add         r8, r8, #16
    add         r7, r7, r2

    add         r0, r0, #1
    cmp         r0, r1
    blt         LOOP_REMAIN_HEIGHT_REMAIN_WIDTH_HEIGHT1
    ldmfd       sp!, {r0-r1,r3}            @ restore registers

REMAIN_HEIGHT_REMAIN_WIDTH_END:

REMAIN_HEIGHT_END:

RESTORE_REG:
    ldmfd       sp!, {r4-r12,r15}       @ restore registers

    .fnend
