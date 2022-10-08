/*
 $License:
    Copyright (C) 2012 InvenSense Corporation, All Rights Reserved.
 $
 */

/******************************************************************************
 *
 * $Id:$
 *
 *****************************************************************************/

#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#endif
#ifdef LINUX
#include <sys/select.h>
#endif
#include <time.h>
#include <string.h>

int ConsoleKbhit(void)
{
#ifdef _WIN32
    return _kbhit();
#else
    struct timeval tv;
    fd_set read_fd;

    tv.tv_sec=0;
    tv.tv_usec=0;
    FD_ZERO(&read_fd);
    FD_SET(0, &read_fd);

    if(select(1, &read_fd, NULL, NULL, &tv) == -1)
        return 0;

    if(FD_ISSET(0, &read_fd))
        return 1;

    return 0;
#endif
}

char ConsoleGetChar(void)
{
#ifdef _WIN32
    return _getch();
#else
    return getchar();
#endif
}
