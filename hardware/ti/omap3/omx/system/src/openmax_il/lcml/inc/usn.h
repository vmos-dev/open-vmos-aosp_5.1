
/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
/* ====================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
* ==================================================================== */

/** Usn.h
 *  The LCML header file contains the definitions used by 
 *   the component to access USN specific iteams.
 *   copied from USN.h and iUlg.h located in dsp 
 */



#define MDN_MONO_CHANNEL                0x0001
#define MDN_STEREO_INTERLEAVED          0x0002
#define MDN_STEREO_NON_INTERLEAVED      0x0003
#define MDN_MONO_DUPLICATED             0x0004

typedef enum {
    USN_GPPMSG_PLAY          = 0x0100,
    USN_GPPMSG_STOP          = 0x0200,
    USN_GPPMSG_PAUSE         = 0x0300,
    USN_GPPMSG_ALGCTRL       = 0x0400,
    USN_GPPMSG_STRMCTRL      = 0x0500,
    USN_GPPMSG_SET_BUFF      = 0x0600,
    USN_GPPMSG_SET_STRM_NODE = 0x0700,
    USN_GPPMSG_GET_NODE_PTR  = 0x0800
}USN_HostToNodeCmd;

typedef enum {  
    USN_DSPACK_STOP          = 0x0200,
    USN_DSPACK_PAUSE         = 0x0300,
    USN_DSPACK_ALGCTRL       = 0x0400,
    USN_DSPACK_STRMCTRL      = 0x0500,
    USN_DSPMSG_BUFF_FREE     = 0x0600,
    USN_DSPACK_SET_STRM_NODE = 0x0700,
    USN_DSPACK_GET_NODE_PTR  = 0x0800,
    USN_DSPMSG_EVENT         = 0x0E00
}USN_NodeToHostCmd;

typedef enum {
    USN_ERR_NONE,
    USN_ERR_WARNING,
    USN_ERR_PROCESS,
    USN_ERR_PAUSE,
    USN_ERR_STOP,
    USN_ERR_ALGCTRL,
    USN_ERR_STRMCTRL,
    USN_ERR_UNKNOWN_MSG
} USN_ErrTypes;


typedef enum {
    IUALG_OK                  = 0x0000,
    IUALG_WARN_CONCEALED      = 0x0100,
    IUALG_WARN_UNDERFLOW      = 0x0200,
    IUALG_WARN_OVERFLOW       = 0x0300,
    IUALG_WARN_ENDOFDATA      = 0x0400,
    IUALG_WARN_PLAYCOMPLETED  = 0x0500,
    IUALG_ERR_BAD_HANDLE      = 0x0F00,
    IUALG_ERR_DATA_CORRUPT    = 0x0F01,
    IUALG_ERR_NOT_SUPPORTED   = 0x0F02,
    IUALG_ERR_ARGUMENT        = 0x0F03,
    IUALG_ERR_NOT_READY       = 0x0F04,
    IUALG_ERR_GENERAL         = 0x0FFF,
    IUALG_ERR_INSUFF_BUFFER   = 0x8401
}IUALG_Event;

typedef enum {
    USN_STRMCMD_PLAY,
    USN_STRMCMD_PAUSE,
    USN_STRMCMD_STOP,
    USN_STRMCMD_SETCODECPARAMS,
    USN_STRMCMD_IDLE,   
    USN_STRMCMD_FLUSH    
}USN_StrmCmd;


