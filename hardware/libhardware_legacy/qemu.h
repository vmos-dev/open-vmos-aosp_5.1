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
#ifndef _libs_hardware_qemu_h
#define _libs_hardware_qemu_h

#ifdef __cplusplus
extern "C" {
#endif

#ifdef QEMU_HARDWARE

/* returns 1 iff we're running in the emulator */
extern int  qemu_check(void);

/* a structure used to hold enough state to connect to a given
 * QEMU communication channel, either through a qemud socket or
 * a serial port.
 *
 * initialize the structure by zero-ing it out
 */
typedef struct {
    char   is_inited;
    char   is_available;
    char   is_qemud;
    char   is_qemud_old;
    char   is_tty;
    int    fd;
    char   device[32];
} QemuChannel;

/* try to open a qemu communication channel.
 * returns a file descriptor on success, or -1 in case of
 * error.
 *
 * 'channel' must be a QemuChannel structure that is empty
 * on the first call. You can call this function several
 * time to re-open the channel using the same 'channel'
 * object to speed things a bit.
 */
extern int  qemu_channel_open( QemuChannel*  channel,
                               const char*   name,
                               int           mode );

/* create a command made of a 4-hexchar prefix followed
 * by the content. the prefix contains the content's length
 * in hexadecimal coding.
 *
 * 'buffer' must be at last 6 bytes
 * returns -1 in case of overflow, or the command's total length
 * otherwise (i.e. content length + 4)
 */
extern int  qemu_command_format( char*        buffer, 
                                 int          buffer_size,
                                 const char*  format,
                                 ... );

/* directly sends a command through the 'hw-control' channel.
 * this will open the channel, send the formatted command, then
 * close the channel automatically.
 * returns 0 on success, or -1 on error.
 */
extern int  qemu_control_command( const char*  fmt, ... );

/* sends a question to the hw-control channel, then receive an answer in
 * a user-allocated buffer. returns the length of the answer, or -1
 * in case of error.
 *
 * 'question' *must* have been formatted through qemu_command_format
 */
extern int  qemu_control_query( const char*  question, int  questionlen,
                                char*        answer,   int  answersize );

#endif /* QEMU_HARDWARE */

/* use QEMU_FALLBACK(call) to call a QEMU-specific callback  */
/* use QEMU_FALLBACK_VOID(call) if the function returns void */
#ifdef QEMU_HARDWARE
#  define  QEMU_FALLBACK(x)  \
    do { \
        if (qemu_check()) \
            return qemu_ ## x ; \
    } while (0)
#  define  QEMU_FALLBACK_VOID(x)  \
    do { \
        if (qemu_check()) { \
            qemu_ ## x ; \
            return; \
        } \
    } while (0)
#else
#  define  QEMU_FALLBACK(x)       ((void)0)
#  define  QEMU_FALLBACK_VOID(x)  ((void)0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* _libs_hardware_qemu_h */
