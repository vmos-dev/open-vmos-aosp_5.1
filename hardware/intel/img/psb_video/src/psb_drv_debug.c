/*
 * Copyright (c) 2011 Intel Corporation. All Rights Reserved.
 * Copyright (c) Imagination Technologies Limited, UK
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

#include <va/va_backend.h>
#include <va/va_backend_tpi.h>
#include <va/va_backend_egl.h>
#include <va/va_drmcommon.h>
#include <stdlib.h>

#include "psb_drv_video.h"
#include "psb_drv_debug.h"
#include "psb_buffer.h"
#include "psb_cmdbuf.h"
#include "psb_surface.h"
#include "hwdefs/mem_io.h"
#include "hwdefs/msvdx_offsets.h"
#include "hwdefs/dma_api.h"
#include "hwdefs/reg_io2.h"
#include "hwdefs/msvdx_vec_reg_io2.h"
#include "hwdefs/msvdx_vdmc_reg_io2.h"
#include "hwdefs/msvdx_mtx_reg_io2.h"
#include "hwdefs/msvdx_dmac_linked_list.h"
#include "hwdefs/msvdx_rendec_mtx_slice_cntrl_reg_io2.h"
#include "hwdefs/dxva_cmdseq_msg.h"
#include "hwdefs/dxva_fw_ctrl.h"
#include "hwdefs/fwrk_msg_mem_io.h"
#include "hwdefs/dxva_msg.h"
#include "hwdefs/msvdx_cmds_io2.h"

void psb__open_log(void)
{
    char log_fn[1024] = {0};
    unsigned int suffix;

    if ((psb_video_debug_fp != NULL) && (psb_video_debug_fp != stderr)) {
        debug_fp_count++;
    } else {
        /* psb video info debug */
        if (psb_parse_config("PSB_VIDEO_DEBUG", &log_fn[0]) == 0) {
            suffix = 0xffff & ((unsigned int)time(NULL));
            snprintf(log_fn + strnlen(log_fn, 1024),
                     (1024 - 8 - strnlen(log_fn, 1024)),
                     ".%d.%d", getpid(), suffix);
            psb_video_debug_fp = fopen(log_fn, "w");
            if (psb_video_debug_fp == 0) {
                drv_debug_msg(VIDEO_DEBUG_ERROR, "Log file %s open failed, reason %s, fall back to stderr\n",
                                   log_fn, strerror(errno));
                psb_video_debug_fp = stderr;
            } else {
                drv_debug_msg(VIDEO_DEBUG_GENERAL, "Log file %s open successfully\n", log_fn);
                debug_fp_count++;
            }
#ifdef ANDROID
            ALOGD("PSB_VIDEO_DEBUG is enabled.\n");
#endif
        } else {
            psb_video_debug_fp = NULL;
        }
    }

    if(psb_parse_config("PSB_VIDEO_TRACE", &log_fn[0]) == 0) {
        unsigned int suffix = 0xffff & ((unsigned int)time(NULL));
        time_t curtime;
        
        log_fn[1024 - 8] = '\0';
        if(strncmp(log_fn, "/dev/stdout", sizeof("/dev/stdout")) != 0)
            sprintf(log_fn + strlen(log_fn), ".%d", suffix);
        psb_video_trace_fp = fopen(log_fn, "w");
        if (psb_video_trace_fp == NULL)
            psb_video_trace_fp = stderr;
        time(&curtime);
        fprintf(psb_video_trace_fp, "---- %s\n---- Start Trace ----\n", ctime(&curtime));
        debug_dump_count = 0;
        g_hexdump_offset = 0;
#ifdef ANDROID
        ALOGD("PSB_VIDEO_TRACE is enabled.\n");
#endif
    } else {
        psb_video_trace_fp = NULL;
    }

    /* debug level include error, warning, general, init, entry, ...... */
    if(psb_parse_config("PSB_VIDEO_DEBUG_LEVEL", &log_fn[0]) == 0) {
        psb_video_debug_level = atoi(log_fn);
#ifdef ANDROID
        ALOGD("psb_video_debug_level is %d parsed.\n", psb_video_debug_level);
#endif
    } else {
        psb_video_debug_level = 0x1;
    }

    /* control debug output option, logcat output or print to file */
    if(psb_parse_config("PSB_VIDEO_DEBUG_OPTION", &log_fn[0]) == 0) {
        psb_video_debug_option = atoi(log_fn);
#ifdef ANDROID
        ALOGD("psb_video_debug_option is %d parsed.\n", psb_video_debug_option);
#endif
    } else {
        psb_video_debug_option = 0;
    }

    /* trace level include vabuf, cmdmsg buf, aux buffer, lldma */
    if(psb_parse_config("PSB_VIDEO_TRACE_LEVEL", &log_fn[0]) == 0) {
        psb_video_trace_level = atoi(log_fn);
#ifdef ANDROID
        ALOGD("psb_video_trace_level is %d parsed.\n", psb_video_trace_level);
#endif
    } else {
        psb_video_trace_level = 0;
    }

    /* control trace output option, logcat output or print to file */
    if(psb_parse_config("PSB_VIDEO_TRACE_OPTION", &log_fn[0]) == 0) {
        psb_video_trace_option = atoi(log_fn);
#ifdef ANDROID
        ALOGD("psb_video_debug_option is %d parsed.\n", psb_video_trace_option);
#endif
    } else {
        psb_video_trace_option = 0;
    }

    /* cmdbuf dump, every frame decoded cmdbuf dump to /data/ctrlAlloc%i.txt */
    if(psb_parse_config("PSB_VIDEO_DUMP_CMDBUF", &log_fn[0]) == 0) {
        if(strstr(log_fn, "true") != NULL)
            psb_video_dump_cmdbuf = TRUE;
        else
            psb_video_dump_cmdbuf = FALSE;
#ifdef ANDROID
        ALOGD("PSB_VIDEO_DUMP_CMDBUF is %d.\n", psb_video_dump_cmdbuf);
#endif
    } else {
        psb_video_dump_cmdbuf = FALSE;
    }

    /* psb video va buffers dump */
    if(psb_parse_config("PSB_VIDEO_DUMP_VABUF", &log_fn[0]) == 0) {
        unsigned int suffix = 0xffff & ((unsigned int)time(NULL));
        /* Make sure there is space left for suffix */
        log_fn[1024 - 12] = '\0';
        if(strncmp(log_fn, "/dev/stdout", sizeof("/dev/stdout")) != 0)
            snprintf(log_fn + strlen(log_fn), 11, ".%d", suffix);
        psb_dump_vabuf_fp = fopen(log_fn, "w");
#ifdef ANDROID
        ALOGD("PSB_VIDEO_DUMP_VABUF is enabled.\n");
#endif
    } else {
        psb_dump_vabuf_fp = NULL;
    }

    /* psb video va buffer verbose dump */
    if(psb_parse_config("PSB_VIDEO_DUMP_VABUF_VERBOSE", &log_fn[0]) == 0) {
        unsigned int suffix = 0xffff & ((unsigned int)time(NULL));
        /* Make sure there is space left for suffix */
        log_fn[1024 - 12] = '\0';
        if(strncmp(log_fn, "/dev/stdout", sizeof("/dev/stdout")) != 0)
            snprintf(log_fn + strlen(log_fn), 11, ".%d", suffix);
        psb_dump_vabuf_verbose_fp = fopen(log_fn, "w");
#ifdef ANDROID
        ALOGD("PSB_VIDEO_DUMP_VABUF_VERBOSE is enabled.\n");
#endif
    } else {
        psb_dump_vabuf_verbose_fp = NULL;
    }

    /* dump decoded surface to a yuv file */
    if(psb_parse_config("PSB_VIDEO_DUMP_YUVBUF", &log_fn[0]) == 0) {
        unsigned int suffix = 0xffff & ((unsigned int)time(NULL));
        /* Make sure there is space left for suffix */
        log_fn[1024 - 12] = '\0';
        if(strncmp(log_fn, "/dev/stdout", sizeof("/dev/stdout")) != 0)
            snprintf(log_fn + strlen(log_fn), 11, ".%d", suffix);
        psb_dump_yuvbuf_fp = fopen(log_fn, "ab");
#ifdef ANDROID
        ALOGD("PSB_VIDEO_DUMP_YUVBUF is enabled.\n");
#endif
    } else {
        psb_dump_yuvbuf_fp = NULL;
    }
}

void psb__close_log(void)
{
    if ((psb_video_debug_fp != NULL) & (psb_video_debug_fp != stderr)) {
        debug_fp_count--;
        if (debug_fp_count == 0) {
            fclose(psb_video_debug_fp);
            psb_video_debug_fp = NULL;
        }
    }

    if(psb_video_trace_fp != NULL) {
        fclose(psb_video_trace_fp);
        psb_video_trace_fp = NULL;
    }

    if(psb_dump_vabuf_fp != NULL) {
        fclose(psb_dump_vabuf_fp);
        psb_dump_vabuf_fp = NULL;
    }

    if(psb_dump_vabuf_verbose_fp != NULL) {
        fclose(psb_dump_vabuf_verbose_fp);
        psb_dump_vabuf_verbose_fp = NULL;
    }

    if(psb_dump_yuvbuf_fp != NULL) {
        fclose(psb_dump_yuvbuf_fp);
        psb_dump_yuvbuf_fp = NULL;
    }

    return;
}

/*
 * read a config "env" for libva.conf or from environment setting
 * liva.conf has higher priority
 * return 0: the "env" is set, and the value is copied into env_value
 *        1: the env is not set
 */
int psb_parse_config(char *env, char *env_value)
{
    char *token, *value, *saveptr;
    char oneline[1024];
    FILE *fp = NULL;
    char *env_ptr;

    if (env == NULL)
        return 1;

    fp = fopen("/etc/psbvideo.conf", "r");
    while (fp && (fgets(oneline, 1024, fp) != NULL)) {
        if (strlen(oneline) == 1)
            continue;
        token = strtok_r(oneline, "=\n", &saveptr);
        value = strtok_r(NULL, "=\n", &saveptr);

        if (NULL == token || NULL == value)
            continue;

        if (strcmp(token, env) == 0) {
            if (env_value)
                strcpy(env_value, value);

            fclose(fp);

            return 0;
        }
    }
    if (fp)
        fclose(fp);

    env_ptr = getenv(env);
    if (env_ptr) {
        if (env_value)
            strncpy(env_value, env_ptr, strlen(env_ptr));

        return 0;
    }

    return 1;
}

void drv_debug_msg(DEBUG_LEVEL debug_level, const char *msg, ...)
{
    va_list args;

#ifdef ANDROID
    if (debug_level == VIDEO_DEBUG_ERROR) {
        va_start(args, msg);
        char tag[128];
        (void)tag;
        sprintf(tag, "pvr_drv_video ");
        __android_log_vprint(ANDROID_LOG_ERROR, tag, msg, args);
        va_end(args);
    }

    if ((psb_video_debug_option & PRINT_TO_LOGCAT) && (debug_level & psb_video_debug_level)) {
        va_start(args, msg);
        char tag[128];
        (void)tag;
        if (psb_video_debug_option & THREAD_DEBUG)
            sprintf(tag, "pvr_drv_video[%d:0x%08lx]", getpid(), pthread_self());
        else
            sprintf(tag, "pvr_drv_video ");
        __android_log_vprint(ANDROID_LOG_DEBUG, tag, msg, args);
        va_end(args);
    }
#endif

    if (!psb_video_debug_fp && (psb_video_debug_level & VIDEO_DEBUG_ERROR))
        psb_video_debug_fp = stderr;
    if (psb_video_debug_fp && (psb_video_debug_option & PRINT_TO_FILE) &&
            (debug_level & psb_video_debug_level)) {
        if (psb_video_debug_option & TIME_DEBUG)
            fprintf(psb_video_debug_fp, "TickCount - [0x%08lx], ",
                    GetTickCount());
        if (psb_video_debug_option & THREAD_DEBUG)
            fprintf(psb_video_debug_fp, "Thread - (%d:0x%08lx) ",
                    getpid(), pthread_self());
        va_start(args, msg);
        vfprintf(psb_video_debug_fp, msg, args);
        va_end(args);
        fflush(psb_video_debug_fp);
        //fsync(fileno(psb_video_debug_fp));
    }
}

int psb__dump_I420_buffers(
    psb_surface_p psb_surface,
    short __maybe_unused srcx,
    short srcy,
    unsigned short srcw,
    unsigned short srch)
{
    unsigned char *mapped_buffer;
    unsigned char *mapped_buffer1, *mapped_buffer2;

    if (psb_dump_yuvbuf_fp) {
        psb_buffer_map(&psb_surface->buf, (unsigned char**)&mapped_buffer);
        if(mapped_buffer == NULL)
            return VA_STATUS_ERROR_INVALID_BUFFER;

        int j,k;
        mapped_buffer1 = mapped_buffer + psb_surface->stride * srcy;
        mapped_buffer2 = mapped_buffer + psb_surface->stride * (srch + srcy / 2);
        mapped_buffer = mapped_buffer2;

        for(j = 0; j < srch; ++j)
        {
            fwrite(mapped_buffer1,  srcw, 1, psb_dump_yuvbuf_fp);
            mapped_buffer1 += psb_surface->stride;
        }

         for(j = 0; j < srch /2; ++j) {
            for(k = 0; k < srcw; ++k) {
                if((k%2) == 0)fwrite(mapped_buffer2, 1, 1, psb_dump_yuvbuf_fp);

                mapped_buffer2++;
            }
            mapped_buffer2 += psb_surface->stride - srcw;
        }

        mapped_buffer2 = mapped_buffer;
        for(j = 0; j < srch /2; ++j)
        {
            for(k = 0; k < srcw; ++k)
            {
                if((k % 2) == 1)
                    fwrite(mapped_buffer2, 1, 1, psb_dump_yuvbuf_fp);
                mapped_buffer2++;
            }
            mapped_buffer2 += psb_surface->stride-srcw;
        }

        psb_buffer_unmap(&psb_surface->buf);
    }

    return 0;
}

int psb__dump_NV12_buffers(
    psb_surface_p psb_surface,
    short __maybe_unused srcx,
    short __maybe_unused srcy,
    unsigned short srcw,
    unsigned short srch)
{
    unsigned char *mapped_buffer;
    unsigned char *mapped_start;

    if (psb_dump_yuvbuf_fp) {
        psb_buffer_map(&psb_surface->buf, (unsigned char **)&mapped_buffer);
        if(mapped_buffer == NULL)
            return VA_STATUS_ERROR_INVALID_BUFFER;

        int i;
        int row = srch;

        mapped_start = mapped_buffer;
        for(i = 0; i < row; ++i)
        {
            fwrite(mapped_buffer,  srcw, 1, psb_dump_yuvbuf_fp);
            mapped_buffer += psb_surface->stride;
        }

        mapped_buffer = mapped_start + psb_surface->chroma_offset;
        for(i = 0; i < row/2; ++i)
        {
            fwrite(mapped_buffer,  srcw, 1, psb_dump_yuvbuf_fp);
            mapped_buffer += psb_surface->stride;
        }
        psb_buffer_unmap(&psb_surface->buf);
    }

    return 0;
}

int psb_cmdbuf_dump(unsigned int *buffer, int byte_size)
{
    static int c=0;
    static char pFileName[50];

    if (psb_video_dump_cmdbuf == FALSE)
        return 0;

    sprintf( pFileName , "/data/ctrlAlloc%i.txt", c++);
    FILE* pF = fopen(pFileName,"w");
    if(pF == NULL) {
        return 1;
    }

    int idx=0;
    unsigned int x;
    while( idx <  byte_size / 4 )
    {
        unsigned int cmd = buffer[idx++];
        fprintf( pF , "Command Word: %08X\n" , cmd  );
        switch( cmd&0xf0000000  )
        {
            case 0x70000000:
            {
                fprintf( pF , "%04X 2NDPASSDEBLOCK\n" , (idx-1)*4  );
                for( x=0;x< 5 ;x++)
                {
                    fprintf( pF ,"\t\t%08X\n",
                        buffer[idx]        );
                    idx++;

                }

                break;
            }
            case 0x90000000:
            {
                fprintf( pF , "%04X HEADER\n" , (idx-1)*4  );
                for( x=0;x< 7 ;x++)
                {
                    fprintf( pF ,"\t\t%08X\n",
                        buffer[idx]        );
                    idx++;

                }

                break;
            }

            case 0x10000000:
            {
                fprintf( pF , "%04X CMD_REGVALPAIR_WRITE ( %08X )\n", (idx-1)*4 , cmd);
                unsigned int addr = cmd&0xffff;
                unsigned int count = (cmd>>16)&0xff;
                for( x=0;x< count ;x++)
                {
                    fprintf( pF ,"\t\t%08X %08X\n",
                        addr ,
                        buffer[idx]    );
                    idx+=1;
                    addr+=4;

                }
                break;
            }

            case 0x50000000:
            {
                fprintf( pF , "%04X CMD_RENDEC_BLOCK( %08X )\n", (idx-1)*4 , cmd);
                unsigned int  count    = (cmd>>16)&0x00ff;
                unsigned int  uiAddr = (cmd &0xffff );            /* to do,  limit this */

                for( x=0;x< count ;x++)
                {
                    fprintf( pF ,"\t\t%08X %08X\n",
                        uiAddr ,
                        buffer[idx++]    );
                    uiAddr+= 4;

                }
                break;
            }
            case 0xd0000000:
            {
                fprintf( pF , "%04X CMD_NEXT_SEG\n", (idx-1)*4 );
                fprintf( pF , "wrong\n");
                goto done;

                break;
            }
            case 0xb0000000:
            {
                fprintf( pF , "%04X SR SETUP %08x\n" , (idx-1)*4  , cmd );
                for( x=0;x< 2 ;x++)
                {
                    fprintf( pF ,"\t\t%08X\n",
                        buffer[idx]        );
                    idx++;

                }
                break;
            }

            case 0xf0000000:
            {
                fprintf( pF , "%04X CMD_PARSE_HEADER %08x\n" , (idx-1)*4  , cmd );
                for( x=0;x< 8 ;x++)
                {
                    fprintf( pF ,"\t\t%08X\n",
                        buffer[idx]        );
                    idx++;

                }
                break;
            }

        case 0x60000000:
            goto done;

            default:
                fprintf( pF , "%04X %08x\n" ,(idx-1)*4 , cmd);


        }


    }
done:
    fclose( pF );
    return 0;

}

/********************* trace debug start *************************/

void psb__trace_message(const char *msg, ...)
{
    va_list args;

#ifdef ANDROID
    if ((psb_video_trace_option & PRINT_TO_LOGCAT) && msg) {
        va_start(args, msg);
        char tag[128];
        (void)tag;
        sprintf(tag, "pvr_drv_video ");
        __android_log_vprint(ANDROID_LOG_DEBUG, tag, msg, args);
        va_end(args);
    }
#endif

    if (psb_video_trace_fp && (psb_video_trace_option & PRINT_TO_FILE)) {
        if (msg) {
            va_start(args, msg);
            vfprintf(psb_video_trace_fp, msg, args);
            va_end(args);
        } else {
            fflush(psb_video_trace_fp);
            //fsync(fileno(psb_video_trace_fp));
        }
    }
}

void psb__debug_w(uint32_t val, char *fmt, uint32_t bit_to, uint32_t bit_from)
{
    if (bit_to < 31) {
        val &= ~(0xffffffff << (bit_to + 1));
    }
    val = val >> bit_from;
    psb__trace_message(fmt, val);
}

#define DBH(fmt, arg...)        psb__trace_message(fmt, ##arg)
#define DB(fmt, arg1, arg...)        psb__trace_message("[%08x] %08x = " fmt, ((unsigned char *) arg1) - cmd_start, *arg1, ##arg)

/* See also MsvdxGpuSim() in msvdxgpu.c */
void debug_dump_cmdbuf(uint32_t *cmd_idx, uint32_t cmd_size_in_bytes)
{
    uint32_t cmd_size = cmd_size_in_bytes / sizeof(uint32_t);
    uint32_t *cmd_end = cmd_idx + cmd_size;
    unsigned char *cmd_start = (unsigned char *)cmd_idx;
    struct {
        unsigned int start;
        unsigned int end;
        char *name;
    } msvdx_regs[11] = {{0x00000000, 0x000003FF, "MTX_MTX"},
        {0x00000400, 0x0000047F, "VDMC_MTX"},
        {0x00000480, 0x000004FF, "VDEB_MTX"},
        {0x00000500, 0x000005FF, "DMAC_MTX"},
        {0x00000600, 0x000006FF, "SYS_MTX"},
        {0x00000700, 0x000007FF, "VEC_IQRAM_MTX"},
        {0x00000800, 0x00000FFF, "VEC_MTX"},
        {0x00001000, 0x00001FFF, "CMD_MTX"},
        {0x00002000, 0x00002FFF, "VEC_RAM_MTX"},
        {0x00003000, 0x00004FFF, "VEC_VLC_M"},
        {0x00005000, 0xFFFFFFFF, "OUT_OF_RANGE"}
    };

    DBH("CMD BUFFER [%08x] - [%08x], %08x bytes, %08x dwords\n", (uint32_t) cmd_idx, cmd_end, cmd_size_in_bytes, cmd_size);
    while (cmd_idx < cmd_end) {
        uint32_t cmd = *cmd_idx;
        /* What about CMD_MAGIC_BEGIN ?*/

        switch (cmd & CMD_MASK) {
        case CMD_NOP: {
            DB("CMD_NOPE\n", cmd_idx);
            cmd_idx++;
            break;
        }
        case CMD_REGVALPAIR_WRITE: {
            uint32_t count = (cmd & 0xfff0000) >> 16;
            uint32_t reg = cmd & 0xffff;
            DB("CMD_REGVALPAIR_WRITE count = 0x%08x\n", cmd_idx, count);
            cmd_idx++;

            while (count--) {
                int i;
                for (i = 0; i < 10; i++) {
                    if ((reg >= msvdx_regs[i].start) &&
                        (reg <= msvdx_regs[i].end))
                        break;
                }
                psb__trace_message("%s_%04x\n", msvdx_regs[i].name, reg);
                reg += 4;
                DB("value\n", cmd_idx);
                cmd_idx++;
            }
            break;
        }
        case CMD_RENDEC_WRITE: {
            uint32_t encoding;
            uint32_t count = (cmd & CMD_RENDEC_COUNT_MASK) >> CMD_RENDEC_COUNT_SHIFT;
            DB("CMD_RENDEC_WRITE count = %08x\n", cmd_idx, count);
            cmd_idx++;

            DB("RENDEC_SL_HDR\n", cmd_idx);
            cmd_idx++;

            DB("RENDEC_SL_NULL\n", cmd_idx);
            cmd_idx++;

            do {
                uint32_t chk_hdr = *cmd_idx;
                count = 1 + ((chk_hdr & 0x07FF0000) >> 16);
                uint32_t start_address = (chk_hdr & 0x0000FFF0) >> 4;
                encoding = (chk_hdr & 0x07);
                if ((count == 1) && (encoding == 7)) {
                    count = 0;
                    DB("SLICE_SEPARATOR\n", cmd_idx);
                } else {
                    DB("RENDEC_CK_HDR #symbols = %d address = %08x encoding = %01x\n", cmd_idx, count, start_address, encoding);
                }
                cmd_idx++;

                while (count && (count < 0x1000)) {
                    DB("value\n", cmd_idx);
                    cmd_idx++;

                    count -= 2;
                }
            } while (encoding != 0x07);

            break;
        }
        case CMD_RENDEC_BLOCK: {
            uint32_t count = (cmd & 0xff0000) >> 16;
            uint32_t rendec = cmd & 0xffff;
            DB("CMD_RENDEC_BLOCK count = 0x%08x\n", cmd_idx, count);
            cmd_idx++;

            while (count--) {
                psb__trace_message("%04x\n", rendec);
                rendec += 4;
                DB("value\n", cmd_idx);
                cmd_idx++;
            }
            break;
        }

        case CMD_COMPLETION: {
            if (*cmd_idx == PSB_RELOC_MAGIC) {
                DB("CMD_(S)LLDMA (assumed)\n", cmd_idx);
                cmd_idx++;

            } else {
                DB("CMD_COMPLETION\n", cmd_idx);
                cmd_idx++;

//              DB("interrupt\n", cmd_idx);
//              cmd_idx++;
            }
            break;
        }
        case CMD_HEADER: {
            uint32_t context = cmd & CMD_HEADER_CONTEXT_MASK;
            DB("CMD_HEADER context = %08x\n", cmd_idx, context);
            cmd_idx++;
            DB("StatusBufferAddress\n", cmd_idx);
            cmd_idx++;
            DB("PreloadSave\n", cmd_idx);
            cmd_idx++;
            DB("PreloadRestore\n", cmd_idx);
            cmd_idx++;
            break;
        }
        case CMD_CONDITIONAL_SKIP: {
            DB("CMD_CONDITIONAL_SKIP\n", cmd_idx);
            cmd_idx++;
            DB("vlc table address\n", cmd_idx);
            break;
        }
        case CMD_CTRL_ALLOC_HEADER: {
            psb__trace_message("CMD_CTRL_ALLOC_HEADER count = 0x%08x\n", sizeof(CTRL_ALLOC_HEADER));
            uint32_t count = sizeof(CTRL_ALLOC_HEADER)/4;
            while (count) {
                    DB("value\n", cmd_idx);
                    cmd_idx++;
                    count--;
            }
            break;
        }
        case CMD_LLDMA: {
            DB("CMD_LLDMA\n", cmd_idx);
            cmd_idx++;
            break;
        }
        case CMD_SR_SETUP: {
            DB("CMD_SR_SETUP\n", cmd_idx);
            cmd_idx++;
            DB("offset in bits\n", cmd_idx);
            cmd_idx++;
            DB("size in bytes\n", cmd_idx);
            cmd_idx++;
            break;
        }
        case CMD_SLLDMA: {
            DB("CMD_SLLDMA\n", cmd_idx);
            cmd_idx++;
            break;
        }
        case CMD_DMA: {
            DB("CMD_DMA\n", cmd_idx);
            cmd_idx++;
            DB("cmd dma address\n", cmd_idx);
            break;
        }
        default:
            if (*cmd_idx == PSB_RELOC_MAGIC) {
                DB("CMD_(S)LLDMA (assumed)\n", cmd_idx);
                cmd_idx++;

            } else {
                DB("*** Unknown command ***\n", cmd_idx);
                cmd_idx++;
            }
            break;
        } /* switch */
    } /* while */
}

/********************* trace debug end *************************/

/********************* dump buffer when flush cmdbuf - start *************************/
void psb__debug_schedule_hexdump(const char *name, psb_buffer_p buf, uint32_t offset, uint32_t size)
{
    ASSERT(debug_dump_count < MAX_DUMP_COUNT);
    debug_dump_name[debug_dump_count] = name;
    debug_dump_buf[debug_dump_count] = buf;
    debug_dump_offset[debug_dump_count] = offset;
    debug_dump_size[debug_dump_count] = size;
    debug_dump_count++;
}

static void psb__hexdump2(unsigned char *p, int offset, int size)
{
    if (offset + size > 8)
        size = 8 - offset;
    psb__trace_message("[%04x]", g_hexdump_offset);
    g_hexdump_offset += offset;
    g_hexdump_offset += size;
    while (offset-- > 0) {
        psb__trace_message(" --");
    }
    while (size-- > 0) {
        psb__trace_message(" %02x", *p++);
    }
    psb__trace_message("\n");
}

void psb__hexdump(unsigned char *addr, int size)
{
    unsigned char *p = (unsigned char *) addr;

    int offset = g_hexdump_offset % 8;
    g_hexdump_offset -= offset;
    if (offset) {
        psb__hexdump2(p, offset, size);
        size -= 8 - offset;
        p += 8 - offset;
    }

    while (1) {
        if (size < 8) {
            if (size > 0) {
                psb__hexdump2(p, 0, size);
            }
            return;
        }
        psb__trace_message("[%04x] %02x %02x %02x %02x %02x %02x %02x %02x\n", g_hexdump_offset, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
        p += 8;
        size -= 8;
        g_hexdump_offset += 8;
    }
}
/********************* dump buffer when flush cmdbuf - end*************************/

int  psb__dump_va_buffers(object_buffer_p obj_buffer)
{
    unsigned int j,k;
    unsigned char *mapped_buffer;
    int print_num;

    if(psb_dump_vabuf_fp) {
        fprintf(psb_dump_vabuf_fp, "%s", buffer_type_to_string(obj_buffer->type));
        print_num = fprintf(psb_dump_vabuf_fp, "BUFF SIZE :%d	NUMELEMENTS:%d BUFF INFO:\n", obj_buffer->size, obj_buffer->num_elements);

        switch(obj_buffer->type) {
            case VAPictureParameterBufferType:
            case VAIQMatrixBufferType:
            case VASliceParameterBufferType:
                j=0;
                for(k=0;k < obj_buffer->size;++k)
                    print_num = fprintf(psb_dump_vabuf_fp,"0x%02x ,",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                    fprintf(psb_dump_vabuf_fp,"\n ");
                break;

            case VASliceGroupMapBufferType:
            case VABitPlaneBufferType:
                psb_buffer_map(obj_buffer->psb_buffer, (unsigned char **)&mapped_buffer);
                if(mapped_buffer == NULL)
                    return VA_STATUS_ERROR_INVALID_BUFFER;

                for(j=0; j<obj_buffer->size;++j) {
                    if(j%16 == 0) fprintf(psb_dump_vabuf_fp,"\n");
                    for(k=0;k < obj_buffer->num_elements;++k)
                        fprintf(psb_dump_vabuf_fp,"0x%02x   ",*((unsigned char *)(mapped_buffer+obj_buffer->num_elements*j+k)));
                }

                psb_buffer_unmap(obj_buffer->psb_buffer);
                break;

            case VASliceDataBufferType:
            case VAProtectedSliceDataBufferType:
                fprintf(psb_dump_vabuf_fp,"first 256 bytes:\n");
                psb_buffer_map(obj_buffer->psb_buffer, (unsigned char **)&mapped_buffer);
                if (!mapped_buffer)
                    break;
                for(j=0; j<256;++j) {
                    if(j%16 == 0) fprintf(psb_dump_vabuf_fp,"\n");
                    for(k=0;k < obj_buffer->num_elements;++k)
                        fprintf(psb_dump_vabuf_fp,"0x%02x   ",*((unsigned char *)(mapped_buffer+obj_buffer->num_elements*j+k)));
                }
                psb_buffer_unmap(obj_buffer->psb_buffer);
                break;

            default:
                break;

        }
        fprintf(psb_dump_vabuf_fp, "\n");
        fflush(psb_dump_vabuf_fp);
        fsync(fileno(psb_dump_vabuf_fp));
    }

    return 0;
}

int  psb__dump_va_buffers_verbose(object_buffer_p obj_buffer)
{
    unsigned int j,k;
    unsigned char *mapped_buffer;
    if(psb_dump_vabuf_verbose_fp) {
        fprintf(psb_dump_vabuf_verbose_fp, "%s", buffer_type_to_string(obj_buffer->type));
        fprintf(psb_dump_vabuf_verbose_fp, "BUFF SIZE :%d	NUMELEMENTS:%d BUFF INFO:\n", obj_buffer->size, obj_buffer->num_elements);
        switch(obj_buffer->type) {
            case VAPictureParameterBufferType:
                for(j=0; j < 340; j = j+20) {
                    if(j==0) fprintf(psb_dump_vabuf_verbose_fp,"\nCurrPic:\n");
                    else fprintf(psb_dump_vabuf_verbose_fp,"\nReferenceFrames%d\n", j / 20);
                    fprintf(psb_dump_vabuf_verbose_fp,"picture_id:");
                    for(k=0;k < 4;++k)
                        fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                        fprintf(psb_dump_vabuf_verbose_fp,"    frame_idx:");
                    for(k=4;k < 8;++k)
                        fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                        fprintf(psb_dump_vabuf_verbose_fp,"    flags:");
                    for(k=8;k < 12;++k)
                        fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                        fprintf(psb_dump_vabuf_verbose_fp,"    TopFieldOrderCnt:");
                    for(k=12;k < 16;++k)
                        fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                        fprintf(psb_dump_vabuf_verbose_fp,"    BottomFieldOrderCnt:");
                    for(k=16;k < 20;++k)
                        fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                }
                j=340;k=0;
                fprintf(psb_dump_vabuf_verbose_fp,"\npicture_width_in_mbs_minus1:");
                for(k=0;k < 2;++k)
                    fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                j=342;k=0;
                fprintf(psb_dump_vabuf_verbose_fp, "\npicture_height_in_mbs_minus1:");
                for(k=0;k < 2;++k)
                    fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                j=344;k=0;
                fprintf(psb_dump_vabuf_verbose_fp,  "\nbit_depth_luma_minus8:");
                fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                j=345;k=0;
                fprintf(psb_dump_vabuf_verbose_fp, "\nbit_depth_chroma_minus8:");
                fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                j=346;k=0;
                fprintf(psb_dump_vabuf_verbose_fp, "\nnum_ref_frames:");
                fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                j=348;k=0;
                fprintf(psb_dump_vabuf_verbose_fp,"\nseq_fields_value:");
                for(k=0;k < 4;++k)
                    fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                j=352;k=0;
                fprintf(psb_dump_vabuf_verbose_fp,"\nnum_slice_groups_minus1:");
                fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                j=353;k=0;
                fprintf(psb_dump_vabuf_verbose_fp,"\nslice_group_map_type:");
                fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                j=354;k=0;
                fprintf(psb_dump_vabuf_verbose_fp, "\nslice_group_change_rate_minus1:");
                for(k=0;k < 2;++k)
                    fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                j=356;k=0;
                fprintf(psb_dump_vabuf_verbose_fp,"\npic_init_qp_minus26:");
                fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                j=357;k=0;
                fprintf(psb_dump_vabuf_verbose_fp,"\npic_init_qs_minus26:");
                fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                j=358;k=0;
                fprintf(psb_dump_vabuf_verbose_fp,"\nchroma_qp_index_offset:");
                fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                j=359;k=0;
                fprintf(psb_dump_vabuf_verbose_fp, "\nsecond_chroma_qp_index_offset:");
                fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                j=360;k=0;
                fprintf(psb_dump_vabuf_verbose_fp,"\npic_fields_value:");
                for(k=0;k < 4;++k)
                    fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                j=364;k=0;
                fprintf(psb_dump_vabuf_verbose_fp,"\nframe_num:");
                for(k=0;k < 2;++k)
                    fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                break;

            case VAIQMatrixBufferType:
                for(j=0;j<96;j=j+16) {
                    fprintf(psb_dump_vabuf_verbose_fp,"\nScalingList4x4_%d:", j/16);
                    for(k=0; k<16;++k) {
                        if(k%4 == 0) fprintf(psb_dump_vabuf_verbose_fp, "\n");
                        fprintf(psb_dump_vabuf_verbose_fp,"0x%02x   ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                    }
                }
                for(j=96;j<224;j=j+64) {
                    fprintf(psb_dump_vabuf_verbose_fp,"\nScalingList4x4_%d:",( j-96)/64);
                    for(k=0; k<64;++k) {
                        if(k%8 == 0) fprintf(psb_dump_vabuf_verbose_fp, "\n");
                        fprintf(psb_dump_vabuf_verbose_fp, "0x%02x   ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                    }
                }
                break;

            case VASliceParameterBufferType:
                j=0;k=0;
                fprintf(psb_dump_vabuf_verbose_fp,"\nslice_data_size:");
                for(k=0;k < 4;++k)
                fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                j=4;k=0;
                fprintf(psb_dump_vabuf_verbose_fp,"\nslice_data_offset:");
                for(k=0;k < 4;++k)
                    fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                j=8;k=0;
                fprintf(psb_dump_vabuf_verbose_fp,"\nslice_data_flag:");
                for(k=0;k < 4;++k)
                fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                j=12;k=0;
                fprintf(psb_dump_vabuf_verbose_fp,"\nslice_data_bit_offset:");
                for(k=0;k < 2;++k)
                fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                j=14;k=0;
                fprintf(psb_dump_vabuf_verbose_fp,"\nfirst_mb_in_slice:");
                for(k=0;k < 2;++k)
                fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                j=16;k=0;
                fprintf(psb_dump_vabuf_verbose_fp,"\nslice_type:");
                fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                j=17;k=0;
                fprintf(psb_dump_vabuf_verbose_fp,"\ndirect_spatial_mv_pred_flag:");
                fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                j=18;k=0;
                fprintf(psb_dump_vabuf_verbose_fp,  "\nnum_ref_idx_l0_active_minus1:");
                fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                j=19;k=0;
                fprintf(psb_dump_vabuf_verbose_fp, "\nnum_ref_idx_l1_active_minus1:");
                fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                j=20;k=0;
                fprintf(psb_dump_vabuf_verbose_fp,"\ncabac_init_idc:");
                fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                j=21;k=0;
                fprintf(psb_dump_vabuf_verbose_fp,"\nslice_qp_delta:");
                fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                j=22;k=0;
                fprintf(psb_dump_vabuf_verbose_fp, "\ndisable_deblocking_filter_idc:");
                fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                j=23;k=0;
                fprintf(psb_dump_vabuf_verbose_fp,"\nslice_alpha_c0_offset_div2:");
                fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                j=24;k=0;
                fprintf(psb_dump_vabuf_verbose_fp,"\nslice_beta_offset_div2:");
                fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                for(j=28; j < 668; j = j+20) {
                    fprintf(psb_dump_vabuf_verbose_fp,"\nRefPicList0 ListIndex=%d\n", (j -28)/ 20);
                    fprintf(psb_dump_vabuf_verbose_fp,"picture_id:");
                    for(k=0;k < 4;++k)
                        fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                        fprintf(psb_dump_vabuf_verbose_fp,"   frame_idx:");
                    for(k=4;k < 8;++k)
                        fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                        fprintf(psb_dump_vabuf_verbose_fp,"   flags:");
                    for(k=8;k < 12;++k)
                        fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                        fprintf(psb_dump_vabuf_verbose_fp,"   TopFieldOrderCnt:");
                    for(k=12;k < 16;++k)
                        fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                        fprintf(psb_dump_vabuf_verbose_fp,"   BottomFieldOrderCnt:");
                    for(k=16;k < 20;++k)
                        fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                    }
                for(j=668; j < 1308; j = j+20) {
                    fprintf(psb_dump_vabuf_verbose_fp,"\nRefPicList1 ListIndex=%d\n", (j -668)/ 20);
                    fprintf(psb_dump_vabuf_verbose_fp,"picture_id:");
                    for(k=0;k < 4;++k)
                        fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                        fprintf(psb_dump_vabuf_verbose_fp,"   frame_idx:");
                    for(k=4;k < 8;++k)
                        fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                        fprintf(psb_dump_vabuf_verbose_fp,"   flags:");
                    for(k=8;k < 12;++k)
                        fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                        fprintf(psb_dump_vabuf_verbose_fp,"   TopFieldOrderCnt:");
                    for(k=12;k < 16;++k)
                        fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                        fprintf(psb_dump_vabuf_verbose_fp,"   BottomFieldOrderCnt:");
                    for(k=16;k < 20;++k)
                        fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                    }
                j=1308;k=0;
                fprintf(psb_dump_vabuf_verbose_fp,"\nluma_log2_weight_denom:");
                fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
j=1309;k=0;
                fprintf(psb_dump_vabuf_verbose_fp,"\nchroma_log2_weight_denom:");
                fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                j=1310;k=0;
                fprintf(psb_dump_vabuf_verbose_fp,"\nluma_weight_l0_flag:");
                fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                j=1312;k=0;
                fprintf(psb_dump_vabuf_verbose_fp,"\nluma_weight_l0:");
                for(j=1312;j<1376;j=j+2) {
                    if((j-1312)%16 == 0)fprintf(psb_dump_vabuf_verbose_fp,"\n");
                    fprintf(psb_dump_vabuf_verbose_fp,"     :");
                    for(k=0;k < 2;++k)
                        fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                }
                fprintf(psb_dump_vabuf_verbose_fp,"\nluma_offset_l0:");
                for(j=1376;j<1440;j=j+2) {
                    if((j-1376)%16 == 0) fprintf(psb_dump_vabuf_verbose_fp,"\n");
                    fprintf(psb_dump_vabuf_verbose_fp,"     ");
                    for(k=0;k < 2;++k)
                        fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                }
                j=1440;k=0;
                fprintf(psb_dump_vabuf_verbose_fp,"\nchroma_weight_l0_flag:");
                fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                j=1442;k=0;
                fprintf(psb_dump_vabuf_verbose_fp,"\nchroma_weight_l0:");
                for(j=1442;j<1570;j=j+4) {
                    if((j-1442)%16 == 0) fprintf(psb_dump_vabuf_verbose_fp,"\n");
                    fprintf(psb_dump_vabuf_verbose_fp,"     ");
                    for(k=0;k < 2;++k)
                        fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                        fprintf(psb_dump_vabuf_verbose_fp," , ");
                    for(k=2;k < 4;++k)
                        fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));

                }

                fprintf(psb_dump_vabuf_verbose_fp,"\nchroma_offset_l0:");
                for(j=1570;j<1698;j=j+4) {
                    if((j-1570)%16 == 0) fprintf(psb_dump_vabuf_verbose_fp,"\n");
                    fprintf(psb_dump_vabuf_verbose_fp,"     ");
                    for(k=0;k < 2;++k)
                        fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                        fprintf(psb_dump_vabuf_verbose_fp," , ");
                    for(k=2;k < 4;++k)
                        fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                }
                j=1698;k=0;
                fprintf(psb_dump_vabuf_verbose_fp,"\nluma_weight_l1_flag:");
                fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                fprintf(psb_dump_vabuf_verbose_fp,"\nluma_weight_l1:");
                for(j=1700;j<1764;j=j+2) {
                    if((j-1700)%16 == 0) fprintf(psb_dump_vabuf_verbose_fp,"\n");
                    fprintf(psb_dump_vabuf_verbose_fp,"     ");
                    for(k=0;k < 2;++k)
                    fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                }
                fprintf(psb_dump_vabuf_verbose_fp,"\nluma_offset_l1:");
                for(j=1764;j<1828;j=j+2) {
                    if((j-1764)%16 == 0) fprintf(psb_dump_vabuf_verbose_fp,"\n");
                    fprintf(psb_dump_vabuf_verbose_fp,"     ");
                    for(k=0;k < 2;++k)
                        fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                }
                j=1828;k=0;
                fprintf(psb_dump_vabuf_verbose_fp,"\nchroma_weight_l1_flag:");
                fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                fprintf(psb_dump_vabuf_verbose_fp,"\nchroma_weight_l1:");
                for(j=1830;j<1958;j=j+4) {
                    if((j-1830)%16 == 0) fprintf(psb_dump_vabuf_verbose_fp,"\n");
                    fprintf(psb_dump_vabuf_verbose_fp,"     ");
                    for(k=0;k < 2;++k)
                        fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                        fprintf(psb_dump_vabuf_verbose_fp," , ");
                    for(k=2;k < 4;++k)
                        fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                }
                fprintf(psb_dump_vabuf_verbose_fp,"\nchroma_offset_l1:");
                for(j=1958;j<2086;j=j+4) {
                    if((j-1958)%16 == 0) fprintf(psb_dump_vabuf_verbose_fp,"\n");
                    fprintf(psb_dump_vabuf_verbose_fp,"     ");
                    for(k=0;k < 2;++k)
                        fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                        fprintf(psb_dump_vabuf_verbose_fp," , ");
                    for(k=2;k < 4;++k)
                    fprintf(psb_dump_vabuf_verbose_fp,"0x%02x ",*((unsigned char *)(obj_buffer->buffer_data+obj_buffer->num_elements*j+k)));
                }
                break;

            case VASliceGroupMapBufferType:
                psb_buffer_map(obj_buffer->psb_buffer, (unsigned char **)&mapped_buffer);
                if(mapped_buffer == NULL)
                    return VA_STATUS_ERROR_INVALID_BUFFER;

                for(j=0; j<obj_buffer->size;++j) {
                    if(j%16 == 0) fprintf(psb_dump_vabuf_verbose_fp,"\n");
                    for(k=0;k < obj_buffer->num_elements;++k)
                        fprintf(psb_dump_vabuf_verbose_fp,"0x%02x   ",*((unsigned char *)(mapped_buffer+obj_buffer->num_elements*j+k)));
                }
                psb_buffer_unmap(obj_buffer->psb_buffer);
                break;

            case VASliceDataBufferType:
            case VAProtectedSliceDataBufferType:
                fprintf(psb_dump_vabuf_verbose_fp,"first 256 bytes:\n");
                psb_buffer_map(obj_buffer->psb_buffer,(unsigned char **)&mapped_buffer);
                if(mapped_buffer == NULL)
                    return VA_STATUS_ERROR_INVALID_BUFFER;
                for(j=0; j<256;++j) {
                    if(j%16 == 0) fprintf(psb_dump_vabuf_verbose_fp,"\n");
                    for(k=0;k < obj_buffer->num_elements;++k)
                        fprintf(psb_dump_vabuf_verbose_fp,"0x%02x   ",*((unsigned char *)(mapped_buffer+obj_buffer->num_elements*j+k)));
                }
                psb_buffer_unmap(obj_buffer->psb_buffer);
                break;
            default:
                break;

            }
        fprintf(psb_dump_vabuf_verbose_fp, "\n");
        fflush(psb_dump_vabuf_verbose_fp);
        fsync(fileno(psb_dump_vabuf_verbose_fp));
    }
    return 0;
}

