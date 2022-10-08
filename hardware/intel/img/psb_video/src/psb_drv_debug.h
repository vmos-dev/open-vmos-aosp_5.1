/*
 * Copyright (c) 2011 Intel Corporation. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Fei Jiang <fei.jiang@intel.com>
 *
 */

#ifndef _PSB_DEBUG_H_
#define _PSB_DEBUG_H_

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include "psb_buffer.h"

/* #define VA_EMULATOR 1 */

#ifdef DEBUG_TRACE
#ifndef ASSERT
#define ASSERT  assert
#endif

#ifndef IMG_ASSERT
#define IMG_ASSERT  assert
#endif

#else /* DEBUG_TRACE */

#undef ASSERT
#undef IMG_ASSERT
#define ASSERT(x)
#define IMG_ASSERT(x)

#endif /* DEBUG_TRACE */

/****************************
 * debug level structures
 ****************************/
typedef enum
{
    VIDEO_DEBUG_ERROR	    =   0x1,
    VIDEO_DEBUG_WARNING	    =   0x2,
    VIDEO_DEBUG_GENERAL	    =   0x4,
    VIDEO_DEBUG_INIT        =   0x8,
    VIDEO_DEBUG_ENTRY       =   0x10,
    VIDEO_DECODE_DEBUG      =   0x100,
    VIDEO_ENCODE_DEBUG      =   0x200,
    VIDEO_DISPLAY_DEBUG     =   0x400,

    VIDEO_ENCODE_PDUMP     =   0x1000,
    VIDEO_ENCODE_HEADER    =   0x2000,
} DEBUG_LEVEL;

/****************************
 * trace level structures
 ****************************/
typedef enum
{
    VABUF_TRACE		=   0x1,
    CMDMSG_TRACE	=   0x2,
    LLDMA_TRACE	    =   0x4,
    AUXBUF_TRACE    =   0x8,
} TRACE_LEVEL;

/****************************
 * debug-trace option structures
 ****************************/
typedef enum
{
    TIME_DEBUG		=   0x1,
    THREAD_DEBUG    =   0x2,
    PRINT_TO_LOGCAT	=   0x10,
    PRINT_TO_FILE   =   0x20,
} DEBUG_TRACE_OPTION;

FILE *psb_video_debug_fp;
int  debug_fp_count;
int psb_video_debug_level;
int psb_video_debug_option;

FILE *psb_video_trace_fp;
int psb_video_trace_level;
int psb_video_trace_option;

FILE *psb_dump_vabuf_fp;
FILE *psb_dump_vabuf_verbose_fp;
FILE *psb_dump_yuvbuf_fp;

int psb_video_dump_cmdbuf;
uint32_t g_hexdump_offset;

void psb__debug_w(uint32_t val, char *fmt, uint32_t bit_to, uint32_t bit_from);

#define DW(wd, sym, to, from) psb__debug_w(((uint32_t *)pasDmaList)[wd], "LLDMA: " #sym " = %d\n", to, from);
#define DWH(wd, sym, to, from) psb__debug_w(((uint32_t *)pasDmaList)[wd], "LLDMA: " #sym " = %08x\n", to, from);

void psb__open_log(void);
void psb__close_log(void);
int psb_parse_config(char *env, char *env_value);
void drv_debug_msg(DEBUG_LEVEL debug_level, const char *msg, ...);
void psb__trace_message(const char *msg, ...);

/*
 * Dump contents of buffer object after command submission
 */
void psb__debug_schedule_hexdump(const char *name, psb_buffer_p buf, uint32_t offset, uint32_t size);
void psb__hexdump(unsigned char *addr, int size);

void debug_dump_cmdbuf(uint32_t *cmd_idx, uint32_t cmd_size_in_bytes);

#define DEBUG_FAILURE           while(vaStatus) {drv_debug_msg(VIDEO_DEBUG_ERROR, "%s fails with '%d' at %s:%d\n", __FUNCTION__, vaStatus, __FILE__, __LINE__);break;}
#define DEBUG_FAILURE_RET       while(ret)      {drv_debug_msg(VIDEO_DEBUG_ERROR, "%s fails with '%s' at %s:%d\n", __FUNCTION__, strerror(ret < 0 ? -ret : ret), __FILE__, __LINE__);break;}

#define DEBUG_FUNC_ENTER while(1) {drv_debug_msg(VIDEO_DEBUG_ENTRY, "%s enter.\n", __FUNCTION__); break;}
#define DEBUG_FUNC_EXIT while(1) {drv_debug_msg(VIDEO_DEBUG_ENTRY, "%s exit.\n", __FUNCTION__); break;}

uint32_t debug_cmd_start[MAX_CMD_COUNT];
uint32_t debug_cmd_size[MAX_CMD_COUNT];
uint32_t debug_cmd_count;
uint32_t debug_lldma_count;
uint32_t debug_lldma_start;

#define MAX_DUMP_COUNT  20
const char * debug_dump_name[MAX_DUMP_COUNT];
psb_buffer_p debug_dump_buf[MAX_DUMP_COUNT];
uint32_t     debug_dump_offset[MAX_DUMP_COUNT];
uint32_t     debug_dump_size[MAX_DUMP_COUNT];
uint32_t     debug_dump_count;

int psb_cmdbuf_dump(unsigned int *buffer, int byte_size);
int psb__dump_va_buffers(object_buffer_p obj_buffer);
int psb__dump_va_buffers_verbose(object_buffer_p obj_buffer);

#endif /* _PSB_DEBUG_H_ */
