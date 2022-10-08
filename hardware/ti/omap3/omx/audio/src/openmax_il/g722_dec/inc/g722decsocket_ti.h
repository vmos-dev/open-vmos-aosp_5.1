
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
/* =============================================================================
 *             Texas Instruments OMAP(TM) Platform Software
 *  (c) Copyright Texas Instruments, Incorporated.  All Rights Reserved.
 *
 *  Use of this software is controlled by the terms and conditions found
 *  in the license agreement under which this software has been supplied.
 * ============================================================================ */
/**
 * @file g722decsocket_ti.h
 *
 * This is an header file for an audio G722 decoder that is fully
 * compliant with the OMX Audio specification.
 * This the file is used internally by the component
 * in its code and it contains UUID of G722 decoder socket node.
 *
 * @path  $(CSLPATH)\OMAPSW_MPU\linux\audio\src\openmax_il\g722_dec\inc\
 *
 * @rev 1.0
 */
/* --------------------------------------------------------------------------- */


#ifndef G722DECSOCKET_TI_H
#define G722DECSOCKET_TI_H
/* ======================================================================= */
/** G722DECSOCKET_TI_UUID: This struct contains the UUID of G722 decoder socket
 * node on DSP.
 */
/* ==================================================================== */
struct DSP_UUID G722DECSOCKET_TI_UUID = {
    0x586f2c8f, 0x172e, 0x4c3a, 0xbf, 0x13, {
        0x4c, 0x2e, 0xdb, 0x60, 0x15, 0x58
    }
};

#endif /* G722DECSOCKET_TI_H */
