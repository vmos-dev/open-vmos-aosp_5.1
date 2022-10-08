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
 * @file    csc_tiled_to_linear_uv_deinterleave_neon.s
 * @brief   SEC_OMX specific define. It support MFC 6.x tiled.
 * @author  ShinWon Lee (shinwon.lee@samsung.com)
 * @version 1.0
 * @history
 *   2012.02.01 : Create
 */

/*
 * Converts and Deinterleave tiled data to linear for mfc 6.x
 * 1. UV of NV12T to Y of YUV420P
 *
 * @param u_dst
 *   U address of YUV420[out]
 *
 * @param v_dst
 *   V address of YUV420[out]
 *
 * @param uv_src
 *   UV address of NV12T[in]
 *
 * @param yuv420_width
 *   real width of YUV420[in]. It should be even.
 *
 * @param yuv420_height
 *   real height of YUV420[in] It should be even.
 */

    .arch armv7-a
    .text
    .global csc_tiled_to_linear_uv_deinterleave_neon
    .type   csc_tiled_to_linear_uv_deinterleave_neon, %function
csc_tiled_to_linear_uv_deinterleave_neon:
    .fnstart

    .equ CACHE_LINE_SIZE, 64
    .equ PRE_LOAD_OFFSET, 6

    @r0     u_dst
    @r1     v_dst
    @r2     uv_src
    @r3     width
    @r4     height
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

    bic         r9, r4, #0x7            @ aligned_height = height & (~0x7)
    bic         r10, r3, #0xF           @ aligned_width = width & (~0xF)
    add         r11, r3, #15            @ tiled_width = ((width + 15) >> 4) << 4
    mov         r11, r11, asr #4
    mov         r11, r11, lsl #4

    mov         r5, #0
LOOP_MAIN_ALIGNED_HEIGHT:
    mul         r8, r11, r5             @ src_offset = tiled_width * i
    mov         r6, #0
    add         r8, r2, r8              @ src_offset = uv_src + src_offset
LOOP_MAIN_ALIGNED_WIDTH:
    mov         r12, r3, asr #1         @ temp1 = (width >> 1) * i + (j >> 1)
    mul         r12, r12, r5

    pld         [r8, #(CACHE_LINE_SIZE*PRE_LOAD_OFFSET)]
    vld2.8      {q0, q1}, [r8]!
    add         r12, r12, r6, asr #1
    vld2.8      {q2, q3}, [r8]!
    add         r7, r0, r12             @ dst_offset = u_dst + temp1
    pld         [r8, #(CACHE_LINE_SIZE*PRE_LOAD_OFFSET)]
    vld2.8      {q4, q5}, [r8]!
    mov         r14, r3, asr #1         @ temp2 = width / 2
    vld2.8      {q6, q7}, [r8]!

    vst1.8      {d0}, [r7], r14
    vst1.8      {d1}, [r7], r14
    vst1.8      {d4}, [r7], r14
    vst1.8      {d5}, [r7], r14
    vst1.8      {d8}, [r7], r14
    vst1.8      {d9}, [r7], r14
    vst1.8      {d12}, [r7], r14
    vst1.8      {d13}, [r7], r14

    add         r7, r1, r12             @ dst_offset = v_dst + temp1

    vst1.8      {d2}, [r7], r14
    vst1.8      {d3}, [r7], r14
    vst1.8      {d6}, [r7], r14
    vst1.8      {d7}, [r7], r14
    vst1.8      {d10}, [r7], r14
    vst1.8      {d11}, [r7], r14
    add         r6, r6, #16
    vst1.8      {d14}, [r7], r14
    cmp         r6, r10
    vst1.8      {d15}, [r7], r14
    blt         LOOP_MAIN_ALIGNED_WIDTH

MAIN_REMAIN_WIDTH_START:
    cmp         r10, r3                 @ if (aligned_width != width) {
    beq         MAIN_REMAIN_WIDTH_END
    stmfd       sp!, {r0-r2,r4}         @ backup registers
    mul         r8, r11, r5             @ src_offset = (tiled_width * i) + (j << 3)
    add         r8, r8, r6, lsl #3
    add         r8, r2, r8              @ r8 = uv_src + src_offset
    mov         r12, r3, asr #1         @ temp1 = (width >> 1) * i + (j >> 1)
    mul         r12, r12, r5
    add         r12, r12, r6, asr #1
    add         r7, r0, r12             @ r7 = u_dst + temp1
    add         r12, r1, r12            @ r12 = v_dst + temp1
    sub         r14, r3, r6             @ r14 = (width - j) / 2
    mov         r14, r14, asr #1

    mov         r4, #0
LOOP_MAIN_REMAIN_HEIGHT:
    mov         r0, #0                  @ r0 is index in de-interleave
LOOP_MAIN_REMAIN_WIDTH:
    ldrb        r1, [r8], #1
    ldrb        r2, [r8], #1
    strb        r1, [r7], #1
    strb        r2, [r12], #1
    add         r0, #1
    cmp         r0, r14
    blt         LOOP_MAIN_REMAIN_WIDTH

    sub         r8, r8, r14, lsl #1
    sub         r7, r7, r14
    sub         r12, r12, r14
    add         r8, r8, #16
    add         r7, r7, r3, asr #1
    add         r12, r12, r3, asr #1

    add         r4, #1
    cmp         r4, #8
    blt         LOOP_MAIN_REMAIN_HEIGHT
    ldmfd       sp!, {r0-r2,r4}         @ restore registers
MAIN_REMAIN_WIDTH_END:

    add         r5, r5, #8
    cmp         r5, r9
    blt         LOOP_MAIN_ALIGNED_HEIGHT

REMAIN_HEIGHT_START:
    cmp         r9, r4                  @ if (aligned_height != height) {
    beq         REMAIN_HEIGHT_END

    mov         r6, #0
LOOP_REMAIN_HEIGHT_WIDTH16:
    mul         r8, r11, r5             @ src_offset = (tiled_width * i) + (j << 3)
    add         r8, r8, r6, lsl #3
    add         r8, r2, r8              @ src_offset = uv_src + src_offset

    mov         r12, r3, asr #1         @ temp1 = (width >> 1) * i + (j >> 1)
    mul         r12, r12, r5
    add         r12, r12, r6, asr #1
    add         r7, r0, r12             @ r7 = u_dst + temp1
    add         r12, r1, r12            @ r12 = v_dst + temp1
    mov         r14, r3, asr #1         @ temp2 = width / 2

    stmfd       sp!, {r0-r1}            @ backup registers
    mov         r0, #0
    sub         r1, r4, r9
LOOP_REMAIN_HEIGHT_WIDTH16_HEIGHT1:
    vld2.8      {d0, d1}, [r8]!
    vst1.8      {d0}, [r7], r14
    vst1.8      {d1}, [r12], r14

    add         r0, r0, #1
    cmp         r0, r1
    blt         LOOP_REMAIN_HEIGHT_WIDTH16_HEIGHT1
    ldmfd       sp!, {r0-r1}            @ restore registers

    add         r6, r6, #16
    cmp         r6, r10
    blt         LOOP_REMAIN_HEIGHT_WIDTH16

REMAIN_HEIGHT_REMAIN_WIDTH_START:
    cmp         r10, r3
    beq         REMAIN_HEIGHT_REMAIN_WIDTH_END
    mul         r8, r11, r5             @ src_offset = (tiled_width * i) + (j << 3)
    add         r8, r8, r6, lsl #3
    add         r8, r2, r8              @ src_offset = uv_src + src_offset

    mov         r12, r3, asr #1         @ temp1 = (width >> 1) * i + (j >> 1)
    mul         r12, r12, r5
    add         r12, r12, r6, asr #1
    add         r7, r0, r12             @ r7 = u_dst + temp1
    add         r12, r1, r12            @ r12 = v_dst + temp1
    sub         r14, r3, r6             @ r14 = (width - j) /2
    mov         r14, r14, asr #1

    stmfd       sp!, {r0-r2,r4-r5}            @ backup registers
    mov         r0, #0
    sub         r1, r4, r9
LOOP_REMAIN_HEIGHT_REMAIN_WIDTH_HEIGHT1:

    mov         r4, #0
LOOP_REMAIN_HEIGHT_REMAIN_WIDTH_HEIGHT1_WIDTHx:
    ldrb        r2, [r8], #1
    ldrb        r5, [r8], #1
    strb        r2, [r7], #1
    strb        r5, [r12], #1
    add         r4, #1
    cmp         r4, r14
    blt         LOOP_REMAIN_HEIGHT_REMAIN_WIDTH_HEIGHT1_WIDTHx

    sub         r8, r8, r14, lsl #1
    sub         r7, r7, r14
    sub         r12, r12, r14
    add         r8, r8, #16
    add         r7, r7, r3, asr #1
    add         r12, r12, r3, asr #1

    add         r0, r0, #1
    cmp         r0, r1
    blt         LOOP_REMAIN_HEIGHT_REMAIN_WIDTH_HEIGHT1
    ldmfd       sp!, {r0-r2,r4-r5}            @ restore registers

REMAIN_HEIGHT_REMAIN_WIDTH_END:

REMAIN_HEIGHT_END:

RESTORE_REG:
    ldmfd       sp!, {r4-r12,r15}       @ restore registers

    .fnend
