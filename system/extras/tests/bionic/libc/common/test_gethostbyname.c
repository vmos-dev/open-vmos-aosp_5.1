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
#define _GNU_SOURCE 1
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <netdb.h>

int  main( int  argc, char**  argv )
{
    char*            hostname = "localhost";
    struct hostent*  hent;
    int    i, ret;

    if (argc > 1)
        hostname = argv[1];

    hent = gethostbyname(hostname);
    if (hent == NULL) {
        printf("gethostbyname(%s) returned NULL !!\n", hostname);
        return 1;
    }
    printf( "gethostbyname(%s) returned:\n", hostname);
    printf( "  name: %s\n", hent->h_name );
    printf( "  aliases:" );
    for (i = 0; hent->h_aliases[i] != NULL; i++)
        printf( " %s", hent->h_aliases[i] );
    printf( "\n" );
    printf( "  address type: " );
    switch (hent->h_addrtype) {
        case AF_INET:  printf( "AF_INET\n"); break;
        case AF_INET6: printf( "AF_INET6\n"); break;
        default: printf("UNKNOWN (%d)\n", hent->h_addrtype);
    }
    printf( "  address: " );
    switch (hent->h_addrtype) {
        case AF_INET:
            {
                const char*  dot = "";
                for (i = 0; i < hent->h_length; i++) {
                    printf("%s%d", dot, ((unsigned char*)hent->h_addr)[i]);
                    dot = ".";
                }
            }
            break;

        default:
            for (i = 0; i < hent->h_length; i++) {
                printf( "%02x", ((unsigned char*)hent->h_addr)[i] );
            }
    }
    printf("\n");
    return 0;
}
