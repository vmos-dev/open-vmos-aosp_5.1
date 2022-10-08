
    .arch armv7-a
    .text
    .global csc_ARGB8888_to_YUV420SP_NEON
    .type   csc_ARGB8888_to_YUV420SP_NEON, %function
csc_ARGB8888_to_YUV420SP_NEON:
    .fnstart

    @r0     pDstY
    @r1     pDstUV
    @r2     pSrcRGB
    @r3     nWidth
    @r4     pDstY2 = pDstY + nWidth
    @r5     pSrcRGB2 = pSrcRGB + nWidthx2
    @r6     temp7, nWidth16m
    @r7     temp6, accumilator
    @r8     temp5, nWidthTemp
    @r9     temp4, Raw RGB565
    @r10    temp3, r,g,b
    @r11    temp2, immediate operand
    @r12    temp1, nHeight
    @r14    temp0, debugging pointer

    .equ CACHE_LINE_SIZE, 32
    .equ PRE_LOAD_OFFSET, 6

    stmfd       sp!, {r4-r12,r14}       @ backup registers
    ldr         r12, [sp, #40]           @ load nHeight
    @ldr         r14, [sp, #44]          @ load pTest
    add         r4, r0, r3             @r4: pDstY2 = pDstY + nWidth
    add         r5, r2, r3, lsl #2     @r5: pSrcRGB2 = tmpSrcRGB + nWidthx4
    sub         r8, r3, #16                @r8: nWidthTmp = nWidth -16

    @q0: temp1, R
    @q1: temp2, GB
    @q2: R
    @q3: G
    @q4: B
    @q5: temp3, output


    vmov.u16 q6, #66 @coefficient assignment
    vmov.u16 q7, #129
    vmov.u16 q8, #25
    vmov.u16 q9,  #0x8080  @ 128<<8 + 128

    vmov.u16 q10, #0x1000  @ 16<<8 + 128
    vorr.u16 q10, #0x0080

    vmov.u16 q11, #38 @#-38
    vmov.u16 q12, #74 @#-74
    vmov.u16 q13, #112
    vmov.u16 q14, #94 @#-94
    vmov.u16 q15, #18 @#-18




LOOP_NHEIGHT2:
    stmfd       sp!, {r12}       @ backup registers

LOOP_NWIDTH16:
    pld         [r2, #(CACHE_LINE_SIZE*PRE_LOAD_OFFSET)]
   @-------------------------------------------YUV ------------------------------------------
    vmov.u16 q14, #94 @#94
    vmov.u16 q15, #18 @#18
    vld4.8   {d0,d1,d2,d3}, [r2]! @loadRGB interleavely
    vld4.8   {d4,d5,d6,d7}, [r2]! @loadRGB interleavely


    vmov.u16 d8,d2
    vmov.u16 d9,d6
    vmov.u16 d10,d1
    vmov.u16 d11,d5
    vmov.u16 d12,d0
    vmov.u16 d13,d4

    vand.u16 q4,#0x00FF  @R
    vand.u16 q5,#0x00FF  @G
    vand.u16 q6,#0x00FF  @B

    vmov.u16 q8,q9   @ CalcU()
    vmla.u16 q8,q6,q13  @112 * B[k]
    vmls.u16 q8,q4,q11  @q0:U -(38 * R[k]) @128<<6+ 32 + u>>2
    vmls.u16 q8,q5,q12  @-(74 * G[k])
    vshr.u16 q8,q8, #8  @(128<<8+ 128 + u)>>8

    vmov.u16 q7,q9      @CalcV()
    vmla.u16 q7,q4,q13  @112 * R[k]
    vmls.u16 q7,q5,q14  @q0:U -(94 * G[k])  @128<<6+ 32 + v>>2
    vmls.u16 q7,q6,q15  @-(18 * B[k])
    vshr.u16 q7,q7, #8  @(128<<8+ 128 + v)>>8


    vtrn.8 q8,q7
    vst1.8  {q8}, [r1]!    @write UV component to yuv420_buffer+linear_ylanesiez

    @-------------------------------------------Y ------------------------------------------

    vmov.u16 q14, #66 @#66
    vmov.u16 q15, #129 @#129
    vmov.u16 q8, #25 @#25

    @CalcY_Y()

    vmul.u16 q7,q4,q14  @q0 = 66 *R[k]
    vmla.u16 q7,q5,q15  @q0 += 129 *G[k]
    vmla.u16 q7,q6,q8  @q0 += 25 *B[k]

    vadd.u16 q7,q7,q10
    vshr.u16 q7,q7, #8

    vmov.u16 d8,d2
    vmov.u16 d9,d6
    vmov.u16 d10,d1
    vmov.u16 d11,d5
    vmov.u16 d12,d0
    vmov.u16 d13,d4

    vshr.u16 q4,q4,#8  @R
    vshr.u16 q5,q5,#8  @G
    vshr.u16 q6,q6,#8  @B

    vmul.u16 q0,q4,q14  @q0 = 66 *R[k]
    vmla.u16 q0,q5,q15  @q0 += 129 *G[k]
    vmla.u16 q0,q6,q8  @q0 += 25 *B[k]
    vadd.u16 q0,q0,q10
    vshr.u16 q0,q0, #8

    vtrn.8 q7,q0
    vst1.8  {q7}, [r0]!@write to Y to yuv420_buffer



   @-------------------------------------------Y ------------------------------------------

            @---------------------------------------------Y1-------------------------------------------

    pld         [r5, #(CACHE_LINE_SIZE*PRE_LOAD_OFFSET)]
    vld4.8   {d0,d1,d2,d3}, [r5]! @loadRGB interleavely
    vld4.8   {d4,d5,d6,d7}, [r5]! @loadRGB interleavely

    vmov.u16 d8,d2
    vmov.u16 d9,d6
    vmov.u16 d10,d1
    vmov.u16 d11,d5
    vmov.u16 d12,d0
    vmov.u16 d13,d4


    vand.u16 q4,#0x00FF  @R
    vand.u16 q5,#0x00FF  @G
    vand.u16 q6,#0x00FF  @B



    vmul.u16 q7,q4,q14  @q0 = 66 *R[k]
    vmla.u16 q7,q5,q15  @q0 += 129 *G[k]
    vmla.u16 q7,q6,q8  @q0 += 25 *B[k]
    vadd.u16 q7,q7,q10
    vshr.u16 q7,q7, #8

    vmov.u16 d8,d2
    vmov.u16 d9,d6
    vmov.u16 d10,d1
    vmov.u16 d11,d5
    vmov.u16 d12,d0
    vmov.u16 d13,d4

    vshr.u16 q4,q4,#8  @R
    vshr.u16 q5,q5,#8  @G
    vshr.u16 q6,q6,#8  @B

    vmul.u16 q0,q4,q14  @q0 = 66 *R[k]
    vmla.u16 q0,q5,q15  @q0 += 129 *G[k]
    vmla.u16 q0,q6,q8  @q0 += 25 *B[k]
    vadd.u16 q0,q0,q10
    vshr.u16 q0,q0, #8

    vtrn.8 q7,q0
    vst1.8  {q7}, [r4]!@write to Y to yuv420_buffer

    subs r8,r8,#16                       @nWidth16--
    BPL LOOP_NWIDTH16                @if nWidth16>0
    @-----------------------------------unaligned ---------------------------------------

    adds r8,r8,#16 @ + 16 - 2
    BEQ NO_UNALIGNED  @in case that nWidht is multiple of 16
LOOP_NWIDTH2:
    @----------------------------------pDstRGB1--Y------------------------------------------
    @stmfd sp!, {r14} @backup r14


    ldr r9,  [r2], #4 @loadRGB  int
    ldr r12,  [r2], #4 @loadRGB  int

    mov r10, r9,lsr #16    @copy to r10
    mov r14, r12    @copy to r10

    ldr r6, =0x000000FF
    and r10, r10, r6 @R: (rgbIn[k] & 0xF800) >> 10;
    ldr r6, =0x00FF0000
    and r14, r14, r6 @R: (rgbIn[k] & 0xF800) >> 10;
    add r10,r10,r14

    mov r11, #66 @accumilator += R*66
    mul r7, r10, r11

    mov r10, r9,lsr #8    @copy to r10
    mov r14, r12,lsl #8    @copy to r10

    ldr r6, =0x000000FF
    and r10, r10, r6 @G:
    ldr r6, =0x00FF0000
    and r14, r14, r6 @G:
    add r10,r10,r14

    mov r11, #129 @accumilator += G *129
    mla r7, r10, r11, r7

    mov r10, r9    @copy to r10
    mov r14, r12,lsl #16    @copy to r10

    ldr r6, =0x000000FF
    and r10, r10, r6 @B
    ldr r6, =0x00FF0000
    and r14, r14, r6 @B
    add r10,r10,r14

    mov r11, #25 @accumilator 1 -= B *25
    mla r7, r10, r11, r7

    ldr r6, =0x10801080
    add  r7, r6

    lsr r7, #8
    strb r7, [r0],#1
    lsr r7,#16
    strb r7, [r0],#1
    @ldmfd sp!, {r14} @load r14


    @----------------------------------pDstRGB2--UV------------------------------------------

    mov r10, r9    @copy to r10
    ldr  r7,=0x00008080
    mov  r12,r7

    ldr r6, =0x000000FF
    and r10, r10, r6 @B:

    mov r11, #112 @accumilator += B*112
    mla r7, r10, r11, r7


    mov r11, #18 @accumilator -= B*18
    mul r11, r10, r11
    sub r12, r12, r11




    mov r10, r9, lsr #16    @copy to r10
    ldr r6, =0x000000FF
    and r10, r10, r6 @R: (rgbIn[k] & 0xF800) >> 10;

    mov r11, #38 @accumilator -= R *38
    mul r11, r10, r11
    sub r7, r7, r11

    mov r11, #112 @accumilator  = R *112
    mla r12, r10, r11, r12

    mov r10, r9,lsr #8    @copy to r10
    ldr r6, =0x000000FF
    and r10, r10, r6  @G: (rgbIn[k] & 0x07E0) >> 5;

    mov r11, #74 @accumilator -= G*74
    mul r11, r10, r11
    sub r7, r7, r11

    mov r11, #94 @accumilator -= G*94
    mul r11, r10, r11
    sub r12, r12, r11

    lsr r7, #8 @ >>8
    strb r7, [r1],#1
    lsr r12, #8 @ >>8
    strb r12, [r1],#1

    @----------------------------------pDstRGB2--Y------------------------------------------
    @stmfd sp!, {r14} @backup r14


    ldr r9,  [r5], #4 @loadRGB  int
    ldr r12,  [r5], #4 @loadRGB  int

    mov r10, r9,lsr #16    @copy to r10
    mov r14, r12    @copy to r10

    ldr r6, =0x000000FF
    and r10, r10, r6 @R: (rgbIn[k] & 0xF800) >> 10;
    ldr r6, =0x00FF0000
    and r14, r14, r6 @R: (rgbIn[k] & 0xF800) >> 10;
    add r10,r10,r14

    mov r11, #66 @accumilator += R*66
    mul r7, r10, r11

    mov r10, r9,lsr #8    @copy to r10
    mov r14, r12,lsl #8    @copy to r10

    ldr r6, =0x000000FF
    and r10, r10, r6 @G:
    ldr r6, =0x00FF0000
    and r14, r14, r6 @G:
    add r10,r10,r14

    mov r11, #129 @accumilator += G *129
    mla r7, r10, r11, r7

    mov r10, r9    @copy to r10
    mov r14, r12,lsl #16    @copy to r10

    ldr r6, =0x000000FF
    and r10, r10, r6 @B
    ldr r6, =0x00FF0000
    and r14, r14, r6 @B
    add r10,r10,r14




    mov r11, #25 @accumilator 1 -= B *25
    mla r7, r10, r11, r7

    ldr r6, =0x10801080
    add  r7, r6
    lsr r7, #8

    strb r7, [r4],#1
    lsr r7,#16
    strb r7, [r4],#1
    @ldmfd sp!, {r14} @load r14


    subs r8,r8,#2                      @ nWidth2 -= 2
    BGT LOOP_NWIDTH2                @ if nWidth2>0


NO_UNALIGNED: @in case that nWidht is multiple of 16

    @-----------------------------------------------------------------------------
    sub         r8, r3, #16                @r8: nWidthTmp = nWidth -16
    add r0, r0,  r3   @pDstY +  nwidth
    add r2, r2, r3, lsl #2    @pSrcRGB +  nwidthx4
    add r4, r4,  r3   @pDstY2 +  nwidth
    add r5, r5, r3, lsl #2   @pSrcRGB2 +  nwidthx4

    ldmfd sp!, {r12}
    subs r12,r12,#2                       @nHeight -=2
    BGT LOOP_NHEIGHT2                @if nHeight2>0

    ldmfd       sp!, {r4-r12,pc}       @ backup registers
    .fnend
