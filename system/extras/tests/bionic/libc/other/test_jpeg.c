/*
 * Copyright (C) 2008 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the 
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED 
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/* this small program is used to measure the performance of libjpeg decompression
 * algorithm...
 */

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include "jpeglib.h"
#include <setjmp.h>
#ifdef HAVE_ANDROID_OS
#include <hardware_legacy/qemu_tracing.h>
#endif

#define  USE_STDIO

#define  CHUNK    32768

typedef struct {
    struct jpeg_source_mgr  jpeg_mgr;
    char*                   base;
    char*                   cursor;
    char*                   end;
} SourceMgrRec, *SourceMgr;

static void
_source_init_source(j_decompress_ptr cinfo)
{
    SourceMgr  src = (SourceMgr) cinfo->src;

    src->jpeg_mgr.next_input_byte = (unsigned char*)src->base,
    src->jpeg_mgr.bytes_in_buffer = src->end - src->base;
}

static int
_source_fill_input_buffer(j_decompress_ptr cinfo)
{
    SourceMgr  src = (SourceMgr) cinfo->src;

    cinfo->err->error_exit((j_common_ptr)cinfo);
    return FALSE;
}

static void
_source_skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
    SourceMgr  src = (SourceMgr) cinfo->src;

    if (src->jpeg_mgr.next_input_byte + num_bytes > (unsigned char*)src->end ) {
        cinfo->err->error_exit((j_common_ptr)cinfo);
    }

    src->jpeg_mgr.next_input_byte += num_bytes;
    src->jpeg_mgr.bytes_in_buffer -= num_bytes;
}

static int
_source_resync_to_restart( j_decompress_ptr cinfo, int desired)
{
    SourceMgr  src = (SourceMgr) cinfo->src;

    src->jpeg_mgr.next_input_byte = (unsigned char*)src->base;
    src->jpeg_mgr.bytes_in_buffer = src->end - src->base;
    return TRUE;
}

static void
_source_term_source(j_decompress_ptr  cinfo)
{
    // nothing to do
}

static void
_source_init( SourceMgr  src, char*  base, long  size )
{
    src->base   = base;
    src->cursor = base;
    src->end    = base + size;

    src->jpeg_mgr.init_source       = _source_init_source;
    src->jpeg_mgr.fill_input_buffer = _source_fill_input_buffer;
    src->jpeg_mgr.skip_input_data   = _source_skip_input_data;
    src->jpeg_mgr.resync_to_restart = _source_resync_to_restart;
    src->jpeg_mgr.term_source       = _source_term_source;
}


typedef struct {
    struct jpeg_error_mgr   jpeg_mgr;
    jmp_buf                 jumper;
    int volatile            error;

} ErrorMgrRec, *ErrorMgr;

static void _error_exit(j_common_ptr cinfo)
{
    ErrorMgr error = (ErrorMgr) cinfo->err;

    (*error->jpeg_mgr.output_message) (cinfo);

    /* Let the memory manager delete any temp files before we die */
    longjmp(error->jumper, -1);
}

#ifdef USE_STDIO
int decompress(FILE*  input_file, int  dct_method, int  disable_rgb)
#else
int decompress(char*  data, long  fsize)
#endif
{
    ErrorMgrRec             errmgr;
    SourceMgrRec            sourcemgr;
    struct jpeg_decompress_struct  cinfo;
    int volatile            error = 0;
    jmp_buf                 jumper;
    int                     isRGB;
    char*                   pixels;
    JSAMPLE*                temprow;

    memset( &cinfo, 0, sizeof(cinfo) );
    memset( &errmgr, 0, sizeof(errmgr) );
    jpeg_create_decompress(&cinfo);
    cinfo.err         = jpeg_std_error(&errmgr.jpeg_mgr);
#if 0
    errmgr.jpeg_mgr.error_exit = _error_exit;
    errmgr.error      = 0;
#endif

    if (setjmp(errmgr.jumper) != 0) {
        fprintf(stderr, "returning error from jpeglib ---\n" );
        goto Exit;
    }

#ifdef USE_STDIO
    /* Specify data source for decompression */
    jpeg_stdio_src(&cinfo, input_file);
#else
    _source_init( &sourcemgr, data, fsize );
    cinfo.src = &sourcemgr.jpeg_mgr;
#endif

    jpeg_read_header(&cinfo, 1);

    if (3 == cinfo.num_components && JCS_RGB == cinfo.out_color_space)
        isRGB = 1;
    else if (1 == cinfo.num_components && JCS_GRAYSCALE == cinfo.out_color_space)
        isRGB = 0;  // could use Index8 config if we want...
    else {
        fprintf( stderr, "unsupported jpeg colorspace %d with %d components\n",
                  cinfo.jpeg_color_space, cinfo.num_components );
        goto Exit;
    }

    cinfo.dct_method = dct_method;
    if (disable_rgb)
        cinfo.out_color_space = JCS_YCbCr;

    jpeg_start_decompress(&cinfo);

    temprow = calloc( cinfo.num_components * cinfo.output_width, sizeof(JSAMPLE) );

    {
        unsigned  y;
        for (y = 0; y < cinfo.output_height; y++) {
            JSAMPLE*  rowptr = temprow;
            (void)jpeg_read_scanlines(&cinfo, &rowptr, 1);
        }
    }
    jpeg_finish_decompress(&cinfo);

    free( temprow );
Exit:
    jpeg_destroy_decompress(&cinfo);
    return error;
}


#define  DEFAULT_REPEAT  10

static void usage(void)
{
    fprintf(stderr, "usage: test_jpeg [options] filename.jpg [filename2.jpg ...]\n" );
    fprintf(stderr, "options:  -r NN   repeat count  (default %d)\n", DEFAULT_REPEAT );
    fprintf(stderr, "          -d N    idct method   (0=default, 1=fastest, 2=slow, 3=float)\n" );
    fprintf(stderr, "          -C      no RGB color conversion (YCbCr instead)\n" );
    exit(1);
}

static double
get_time_usec( void )
{
#ifdef HAVE_ANDROID_OS
    struct timespec  ts;

    if ( clock_gettime( CLOCK_MONOTONIC, &ts ) < 0 )
        fprintf(stderr, "clock_gettime: %s\n", strerror(errno) );

    return ts.tv_sec*1e6 + ts.tv_nsec*1e-3;
#else
    struct timeval  tv;
    if (gettimeofday( &tv, NULL ) < 0)
        fprintf(stderr, "gettimeofday: %s\n", strerror(errno) );

    return tv.tv_sec*1000000. + tv.tv_usec*1.0;
#endif
}


int  main( int  argc, char**  argv )
{
    FILE*  f;
    int    repeat_count      = DEFAULT_REPEAT;
    int    dct_method        = JDCT_DEFAULT;
    int    disable_rgb       = 0;
    double  usec0, usec1;

    if (argc < 2)
        usage();

    for ( ; argc > 1 && argv[1][0] == '-'; argc--, argv++) {
        const char*  arg = &argv[1][1];
        switch (arg[0]) {
            case 'r':
                if (arg[1] == 0) {
                    if (argc < 3)
                        usage();
                    arg = argv[2];
                    argc--;
                    argv++;
                } else
                    arg += 1;

                repeat_count = strtol(arg, NULL, 10);

                if (repeat_count <= 0)
                    repeat_count = 1;
                break;

            case 'C':
                disable_rgb = 1;
                break;

            case 'd':
                if (arg[1] == 0) {
                    if (argc < 3)
                        usage();
                    arg = argv[2];
                    argc--;
                    argv++;
                } else
                    arg += 1;

                dct_method = strtol(arg, NULL, 10);
                switch (dct_method) {
                    case 0:
                        dct_method = JDCT_DEFAULT;
                        break;
                    case 1:
                        dct_method = JDCT_IFAST;
                        break;
                    case 2:
                        dct_method = JDCT_ISLOW;
                        break;
                    case 3:
                        dct_method = JDCT_FLOAT;
                        break;
                    default:
                        usage();
                }
                break;

            default:
                usage();
        }
    }

    for ( ; argc > 1; argc--, argv++ )
    {
        long   fsize;
        char*   data;
        FILE*  f = fopen( argv[1], "rb" );
        int    rr;

        if (f == NULL) {
            fprintf(stderr, "could not open '%s': %s\n", argv[1], strerror(errno) );
            continue;
        }

        fseek( f, 0, SEEK_END );
        fsize = ftell(f);
        fseek( f, 0, SEEK_SET );

        usec0 = get_time_usec();
#ifdef HAVE_ANDROID_OS
        qemu_start_tracing();
#endif
#ifdef USE_STDIO
        for ( rr = repeat_count; rr > 0; rr-- ) {
            fseek( f, 0, SEEK_SET );
            decompress(f, dct_method, disable_rgb);
        }
        fclose( f );
#else

        data = malloc( fsize );
        if (data == NULL) {
            if (fsize > 0)
                fprintf(stderr, "could not allocate %ld bytes to load '%s'\n", fsize, argv[1] );
            fclose(f);
            continue;
        }
        fread( data, 1, fsize, f );
        fclose(f);

        usec1 = get_time_usec() - usec0;
        printf( "compressed load:     %10.2f ms (%ld bytes)\n", usec1*1e-3, fsize );

        usec0 = get_time_usec();
        for ( rr = repeat_count; rr > 0; rr -- )
        {
            decompress( data, fsize );
        }
        free( data );
#endif
#ifdef HAVE_ANDROID_OS
        qemu_stop_tracing();
#endif
        usec1 = get_time_usec() - usec0;
        printf( "decompression took:  %10.3f ms (%.2f KB/s, %d passes)\n", usec1/1e3, fsize*(1e6/1024)*repeat_count/usec1, repeat_count );
    }
    return 0;
}
