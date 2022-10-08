/**
 * @file
 * Driver ID definition.
 *
 * <!-- Copyright Giesecke & Devrient GmbH 2011-2012 -->
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef RTMDRVID_H_
#define RTMDRVID_H_

#define MC_DRV_VENDOR_ID_SHIFT     (16)
#define MC_DRV_VENDOR_ID_MASK      (0xFFFF << MC_DRV_VENDOR_ID_SHIFT)
#define MC_DRV_NUMBER_MASK         (0x0000FFFF)

/** MobiCore vendor IDs. */
typedef enum {
    MC_DRV_VENDOR_ID_GD   = 0 << MC_DRV_VENDOR_ID_SHIFT,
} mcDrvVendorId_t;

/** MobiCore GD driver numbers. */
typedef enum {
	MC_DRV_NUMBER_INVALID = 0,
	MC_DRV_NUMBER_CRYPTO  = 1,
    MC_DRV_NUMBER_KEYPAD  = 2,
    /** Last GD driver number reserved for pre-installed drivers. 
     * GD driver numbers up to this constant may not be used for loadable drivers. */
    MC_DRV_NUMBER_LAST_PRE_INSTALLED = 100,
} mcDrvNumber_t;

/** MobiCore driver IDs for Trustlets. */
typedef enum {
	MC_DRV_ID_INVALID = MC_DRV_VENDOR_ID_GD | MC_DRV_NUMBER_INVALID,
	MC_DRV_ID_CRYPTO  = MC_DRV_VENDOR_ID_GD | MC_DRV_NUMBER_CRYPTO,
    MC_DRV_ID_KEYPAD  = MC_DRV_VENDOR_ID_GD | MC_DRV_NUMBER_KEYPAD,
    /** Last GD driver ID reserved for pre-installed drivers. 
     * GD driver IDs up to this constant may not be used for loadable drivers. */
    MC_DRV_ID_LAST_PRE_INSTALLED = MC_DRV_VENDOR_ID_GD | MC_DRV_NUMBER_LAST_PRE_INSTALLED,
} mcDriverId_t;

#endif /* RTMDRVID_H_ */
