
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
 * @file g726decsocket_ti.h
 *
 * This is an header file for an audio G726 decoder that is fully
 * compliant with the OMX Audio specification.
 * This the file is used internally by the component
 * in its code and it contains UUID of G726 decoder socket node.
 *
 * @path  $(CSLPATH)\OMAPSW_MPU\linux\audio\src\openmax_il\g726_dec\inc\
 *
 * @rev 1.0
 */
/* --------------------------------------------------------------------------- */
#include "usn.h"
#include "decode_common_ti.h"

#ifndef G726DECSOCKET_TI_H
#define G726DECSOCKET_TI_H
/* ======================================================================= */
/** G726DECSOCKET_TI_UUID: This struct contains the UUID of G726 decoder socket
 * node on DSP.
 */
/* ==================================================================== */
struct DSP_UUID G726DECSOCKET_TI_UUID = {
    0x6a62177b, 0x7311, 0x47b0, 0x9e, 0x49, {
        0xfd, 0xec, 0xb3, 0x0d, 0xc4, 0x9f
    }
};


#endif /* G726DECSOCKET_TI_H */

