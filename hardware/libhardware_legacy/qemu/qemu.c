/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
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

/* this file contains various functions used by all libhardware modules
 * that support QEMU emulation
 */
#include "qemu.h"
#define  LOG_TAG  "hardware-qemu"
#include <cutils/log.h>
#include <cutils/properties.h>
#include <cutils/sockets.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdarg.h>

#define  QEMU_DEBUG  0

#if QEMU_DEBUG
#  define  D(...)   ALOGD(__VA_ARGS__)
#else
#  define  D(...)   ((void)0)
#endif

#include "hardware/qemu_pipe.h"

int
qemu_check(void)
{
    static int  in_qemu = -1;

    if (__builtin_expect(in_qemu < 0,0)) {
        char  propBuf[PROPERTY_VALUE_MAX];
        property_get("ro.kernel.qemu", propBuf, "");
        in_qemu = (propBuf[0] == '1');
    }
    return in_qemu;
}

static int
qemu_fd_write( int  fd, const char*  cmd, int  len )
{
    int  len2;
    do {
        len2 = write(fd, cmd, len);
    } while (len2 < 0 && errno == EINTR);
    return len2;
}

static int
qemu_fd_read( int  fd, char*  buff, int  len )
{
    int  len2;
    do {
        len2 = read(fd, buff, len);
    } while (len2 < 0 && errno == EINTR);
    return len2;
}

static int
qemu_channel_open_qemud_pipe( QemuChannel*  channel,
                              const char*   name )
{
    int   fd;
    char  pipe_name[512];

    snprintf(pipe_name, sizeof(pipe_name), "qemud:%s", name);
    fd = qemu_pipe_open(pipe_name);
    if (fd < 0) {
        D("no qemud pipe: %s", strerror(errno));
        return -1;
    }

    channel->is_qemud = 1;
    channel->fd       = fd;
    return 0;
}

static int
qemu_channel_open_qemud( QemuChannel*  channel,
                         const char*   name )
{
    int   fd, ret, namelen = strlen(name);
    char  answer[2];

    fd = socket_local_client( "qemud",
                              ANDROID_SOCKET_NAMESPACE_RESERVED,
                              SOCK_STREAM );
    if (fd < 0) {
        D("no qemud control socket: %s", strerror(errno));
        return -1;
    }

    /* send service name to connect */
    if (qemu_fd_write(fd, name, namelen) != namelen) {
        D("can't send service name to qemud: %s",
           strerror(errno));
        close(fd);
        return -1;
    }

    /* read answer from daemon */
    if (qemu_fd_read(fd, answer, 2) != 2 ||
        answer[0] != 'O' || answer[1] != 'K') {
        D("cant' connect to %s service through qemud", name);
        close(fd);
        return -1;
    }

    channel->is_qemud = 1;
    channel->fd       = fd;
    return 0;
}


static int
qemu_channel_open_qemud_old( QemuChannel*  channel,
                             const char*   name )
{
    int  fd;

    snprintf(channel->device, sizeof channel->device,
                "qemud_%s", name);

    fd = socket_local_client( channel->device,
                              ANDROID_SOCKET_NAMESPACE_RESERVED,
                              SOCK_STREAM );
    if (fd < 0) {
        D("no '%s' control socket available: %s",
            channel->device, strerror(errno));
        return -1;
    }

    close(fd);
    channel->is_qemud_old = 1;
    return 0;
}


static int
qemu_channel_open_tty( QemuChannel*  channel,
                       const char*   name,
                       int           mode )
{
    char   key[PROPERTY_KEY_MAX];
    char   prop[PROPERTY_VALUE_MAX];
    int    ret;

    ret = snprintf(key, sizeof key, "ro.kernel.android.%s", name);
    if (ret >= (int)sizeof key)
        return -1;

    if (property_get(key, prop, "") == 0) {
        D("no kernel-provided %s device name", name);
        return -1;
    }

    ret = snprintf(channel->device, sizeof channel->device,
                    "/dev/%s", prop);
    if (ret >= (int)sizeof channel->device) {
        D("%s device name too long: '%s'", name, prop);
        return -1;
    }

    channel->is_tty = !memcmp("/dev/tty", channel->device, 8);
    return 0;
}

int
qemu_channel_open( QemuChannel*  channel,
                   const char*   name,
                   int           mode )
{
    int  fd = -1;

    /* initialize the channel is needed */
    if (!channel->is_inited)
    {
        channel->is_inited = 1;

        do {
            if (qemu_channel_open_qemud_pipe(channel, name) == 0)
                break;

            if (qemu_channel_open_qemud(channel, name) == 0)
                break;

            if (qemu_channel_open_qemud_old(channel, name) == 0)
                break;

            if (qemu_channel_open_tty(channel, name, mode) == 0)
                break;

            channel->is_available = 0;
            return -1;
        } while (0);

        channel->is_available = 1;
    }

    /* try to open the file */
    if (!channel->is_available) {
        errno = ENOENT;
        return -1;
    }

    if (channel->is_qemud) {
        return dup(channel->fd);
    }

    if (channel->is_qemud_old) {
        do {
            fd = socket_local_client( channel->device,
                                      ANDROID_SOCKET_NAMESPACE_RESERVED,
                                      SOCK_STREAM );
        } while (fd < 0 && errno == EINTR);
    }
    else /* /dev/ttySn ? */
    {
        do {
            fd = open(channel->device, mode);
        } while (fd < 0 && errno == EINTR);

        /* disable ECHO on serial lines */
        if (fd >= 0 && channel->is_tty) {
            struct termios  ios;
            tcgetattr( fd, &ios );
            ios.c_lflag = 0;  /* disable ECHO, ICANON, etc... */
            tcsetattr( fd, TCSANOW, &ios );
        }
    }
    return fd;
}


static int
qemu_command_vformat( char*        buffer,
                      int          buffer_size,
                      const char*  format,
                      va_list      args )
{
    char     header[5];
    int      len;

    if (buffer_size < 6)
        return -1;

    len = vsnprintf(buffer+4, buffer_size-4, format, args);
    if (len >= buffer_size-4)
        return -1;

    snprintf(header, sizeof header, "%04x", len);
    memcpy(buffer, header, 4);
    return len + 4;
}

extern int
qemu_command_format( char*        buffer,
                     int          buffer_size,
                     const char*  format,
                     ... )
{
    va_list  args;
    int      ret;

    va_start(args, format);
    ret = qemu_command_format(buffer, buffer_size, format, args);
    va_end(args);
    return ret;
}


static int
qemu_control_fd(void)
{
    static QemuChannel  channel[1];
    int                 fd;

    fd = qemu_channel_open( channel, "hw-control", O_RDWR );
    if (fd < 0) {
        D("%s: could not open control channel: %s", __FUNCTION__,
          strerror(errno));
    }
    return fd;
}

static int
qemu_control_send(const char*  cmd, int  len)
{
    int  fd, len2;

    if (len < 0) {
        errno = EINVAL;
        return -1;
    }

    fd = qemu_control_fd();
    if (fd < 0)
        return -1;

    len2 = qemu_fd_write(fd, cmd, len);
    close(fd);
    if (len2 != len) {
        D("%s: could not send everything %d < %d",
          __FUNCTION__, len2, len);
        return -1;
    }
    return 0;
}


int
qemu_control_command( const char*  fmt, ... )
{
    va_list  args;
    char     command[256];
    int      len, fd;

    va_start(args, fmt);
    len = qemu_command_vformat( command, sizeof command, fmt, args );
    va_end(args);

    if (len < 0 || len >= (int)sizeof command) {
        if (len < 0) {
            D("%s: could not send: %s", __FUNCTION__, strerror(errno));
        } else {
            D("%s: too large %d > %d", __FUNCTION__, len, (int)(sizeof command));
        }
        errno = EINVAL;
        return -1;
    }

    return qemu_control_send( command, len );
}

extern int  qemu_control_query( const char*  question, int  questionlen,
                                char*        answer,   int  answersize )
{
    int   ret, fd, len, result = -1;
    char  header[5], *end;

    if (questionlen <= 0) {
        errno = EINVAL;
        return -1;
    }

    fd = qemu_control_fd();
    if (fd < 0)
        return -1;

    ret = qemu_fd_write( fd, question, questionlen );
    if (ret != questionlen) {
        D("%s: could not write all: %d < %d", __FUNCTION__,
          ret, questionlen);
        goto Exit;
    }

    /* read a 4-byte header giving the length of the following content */
    ret = qemu_fd_read( fd, header, 4 );
    if (ret != 4) {
        D("%s: could not read header (%d != 4)",
          __FUNCTION__, ret);
        goto Exit;
    }

    header[4] = 0;
    len = strtol( header, &end,  16 );
    if ( len < 0 || end == NULL || end != header+4 || len > answersize ) {
        D("%s: could not parse header: '%s'",
          __FUNCTION__, header);
        goto Exit;
    }

    /* read the answer */
    ret = qemu_fd_read( fd, answer, len );
    if (ret != len) {
        D("%s: could not read all of answer %d < %d",
          __FUNCTION__, ret, len);
        goto Exit;
    }

    result = len;

Exit:
    close(fd);
    return result;
}
