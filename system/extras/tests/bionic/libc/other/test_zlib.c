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
/* this small program is used to measure the performance of zlib's inflate
 * algorithm...
 */

/* most code lifted from the public-domain http://www.zlib.net/zpipe.c */

#include <zlib.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>

#define  CHUNK    32768

int def(FILE *source, FILE *dest, int level)
{
    int ret, flush;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit(&strm, level);
    if (ret != Z_OK)
        return ret;

    /* compress until end of file */
    do {
        strm.avail_in = fread(in, 1, CHUNK, source);
        if (ferror(source)) {
            (void)deflateEnd(&strm);
            return Z_ERRNO;
        }
        flush = feof(source) ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = in;

        /* run deflate() on input until output buffer not full, finish
        compression if all of source has been read in */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = deflate(&strm, flush);    /* no bad return value */
            have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                (void)deflateEnd(&strm);
                return Z_ERRNO;
            }
        } while (strm.avail_out == 0);

        /* done when last data in file processed */
    } while (flush != Z_FINISH);

    /* clean up and return */
    (void)deflateEnd(&strm);
    return Z_OK;
}


int inf(FILE *source)
{
    int ret;
    unsigned have;
    z_stream strm;
    static unsigned char in[CHUNK];
    static unsigned char out[CHUNK];

    /* allocate inflate state */
    strm.zalloc   = Z_NULL;
    strm.zfree    = Z_NULL;
    strm.opaque   = Z_NULL;
    strm.avail_in = 0;
    strm.next_in  = Z_NULL;
    ret = inflateInit(&strm);
    if (ret != Z_OK)
        return ret;

    /* decompress until deflate stream ends or end of file */
    do {
        strm.avail_in = fread(in, 1, CHUNK, source);
        if (ferror(source)) {
            (void)inflateEnd(&strm);
            return Z_ERRNO;
        }
        if (strm.avail_in == 0)
            break;
        strm.next_in = in;

        /* run inflate() on input until output buffer not full */
        do {
            strm.avail_out = CHUNK;
            strm.next_out  = out;
            ret = inflate(&strm, Z_NO_FLUSH);
            switch (ret) {
                case Z_NEED_DICT:
                    ret = Z_DATA_ERROR;     /* and fall through */
                case Z_DATA_ERROR:
                case Z_MEM_ERROR:
                    (void)inflateEnd(&strm);
                    return ret;
            }
        } while (strm.avail_out == 0);

        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);

    /* clean up and return */
    (void)inflateEnd(&strm);
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

#define  DEFAULT_REPEAT  10
#define  DEFAULT_LEVEL   9

static void usage(void)
{
    fprintf(stderr, "usage: test_zlib [options] filename [filename2 ...]\n" );
    fprintf(stderr, "options:  -r NN   repeat count  (default %d)\n", DEFAULT_REPEAT );
    fprintf(stderr, "          -N      set compression level (default %d)\n", DEFAULT_LEVEL );
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
    char   tempfile[256];
    int    repeat_count      = DEFAULT_REPEAT;
    int    compression_level = DEFAULT_LEVEL;
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

            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
                compression_level = arg[0] - '0';
                break;

            default:
                usage();
        }
    }

    sprintf(tempfile, "/tmp/ztest.%d", getpid() );

    for ( ; argc > 1; argc--, argv++ )
    {
        /* first, compress the file into a temporary storage */
        FILE*  f   = fopen(argv[1], "rb");
        FILE*  out = NULL;
        long   fsize;
        int    ret, rr;

        if (f == NULL) {
            fprintf(stderr, "could not open '%s': %s\n", argv[1], strerror(errno) );
            continue;
        }

        printf( "testing %s\n", argv[1] );
        fseek( f, 0, SEEK_END );
        fsize = ftell(f);
        fseek( f, 0, SEEK_SET );

        out = fopen( tempfile, "wb" );
        if (out == NULL) {
            fprintf(stderr, "could not create '%s': %s\n", tempfile, strerror(errno));
            fclose(f);
            continue;
        }

        usec0 = get_time_usec();

        ret = def( f, out, compression_level );

        usec1 = get_time_usec() - usec0;
        printf( "compression took:   %10.3f ms  (%.2f KB/s)\n", usec1/1e3, fsize*(1e6/1024)/usec1 );

        fclose( out );
        fclose(f);

        usec0 = get_time_usec();
        f    = fopen( tempfile, "rb" );

        for ( rr = repeat_count; rr > 0; rr -- )
        {
            fseek( f, 0, SEEK_SET );
            inf(f);
        }
        fclose( f );
        usec1 = get_time_usec() - usec0;
        printf( "decompression took: %10.3f ms (%.2f KB/s, %d passes)\n", usec1/1e3, fsize*(1e6/1024)*repeat_count/usec1, repeat_count );
    }

    unlink(tempfile);
    return 0;
}
