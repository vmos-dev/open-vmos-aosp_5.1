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
 * @file    csc_linear_to_tiled_crop_neon.s
 * @brief   SEC_OMX specific define
 * @author  ShinWon Lee (shinwon.lee@samsung.com)
 * @version 1.0
 * @history
 *   2012.02.01 : Create
 */

/*
 * Interleave src1, src2 to dst
 *
 * @param dest
 *   dst address[out]
 *
 * @param src1
 *   src1 address[in]
 *
 * @param src2
 *   src2 address[in]
 *
 * @param src_size
 *   src_size or src1
 */

    .arch armv7-a
    .text
    .global csc_interleave_memcpy_neon
    .type   csc_interleave_memcpy_neon, %function
csc_interleave_memcpy_neon:
    .fnstart

    @r0     dest
    @r1     src1
    @r2     src2
    @r3     src_size
    @r4
    @r5
    @r6
    @r7
    @r8     temp1
    @r9     temp2
    @r10    dest_addr
    @r11    src1_addr
    @r12    src2_addr
    @r14    i

    stmfd       sp!, {r8-r12,r14}       @ backup registers

    mov         r10, r0
    mov         r11, r1
    mov         r12, r2
    mov         r14, r3

    cmp         r14, #128
    blt         LESS_THAN_128

LOOP_128:
    vld1.8      {q0}, [r11]!
    vld1.8      {q2}, [r11]!
    vld1.8      {q4}, [r11]!
    vld1.8      {q6}, [r11]!
    vld1.8      {q8}, [r11]!
    vld1.8      {q10}, [r11]!
    vld1.8      {q12}, [r11]!
    vld1.8      {q14}, [r11]!
    vld1.8      {q1}, [r12]!
    vld1.8      {q3}, [r12]!
    vld1.8      {q5}, [r12]!
    vld1.8      {q7}, [r12]!
    vld1.8      {q9}, [r12]!
    vld1.8      {q11}, [r12]!
    vld1.8      {q13}, [r12]!
    vld1.8      {q15}, [r12]!

    vst2.8      {q0, q1}, [r10]!
    vst2.8      {q2, q3}, [r10]!
    vst2.8      {q4, q5}, [r10]!
    vst2.8      {q6, q7}, [r10]!
    vst2.8      {q8, q9}, [r10]!
    vst2.8      {q10, q11}, [r10]!
    vst2.8      {q12, q13}, [r10]!
    vst2.8      {q14, q15}, [r10]!

    sub         r14, #128
    cmp         r14, #128
    bgt         LOOP_128

LESS_THAN_128:
    cmp         r14, #0
    beq         RESTORE_REG

LOOP_1:
    ldrb        r8, [r11], #1
    ldrb        r9, [r12], #1
    strb        r8, [r10], #1
    strb        r9, [r10], #1
    subs        r14, #1
    bne         LOOP_1

RESTORE_REG:
    ldmfd       sp!, {r8-r12,r15}       @ restore registers
    .fnend
