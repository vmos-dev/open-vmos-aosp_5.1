/*
 * Copyright (c) 2011 Trusted Logic S.A.
 * All Rights Reserved.
 *
 * This software is the confidential and proprietary information of
 * Trusted Logic S.A. ("Confidential Information"). You shall not
 * disclose such Confidential Information and shall use it only in
 * accordance with the terms of the license agreement you entered
 * into with Trusted Logic S.A.
 *
 * TRUSTED LOGIC S.A. MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE
 * SUITABILITY OF THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, OR NON-INFRINGEMENT. TRUSTED LOGIC S.A. SHALL
 * NOT BE LIABLE FOR ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING,
 * MODIFYING OR DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 */
#ifndef   __WVDRM_PROTOCOL_H__
#define   __WVDRM_PROTOCOL_H__

#include <common_secure_driver_protocol.h>

/* 45544DF9-B1DF-9BEE-D0B9-0C98CE3B41F6 */
#define WVDRM_UUID {0x45544DF9, 0xB1DF, 0x9BEE, {0xD0, 0xB9, 0x0C, 0x98, 0xCE, 0x3B, 0x41, 0xF6}}

/*
 * Persistently install the DRM "key box" previously wrapped
 * with WRAP_KEYBOX
 *
 * Param #0: MEMREF_INPUT:
 *    The encrypted keybox
 */
#define WVDRM_INSTALL_KEYBOX 0x00001000

/*
 * Test if a keybox is provisioned and optionnally get its key data
 *
 * #0:
 *   - NONE: for testing if the keybox is valid (returns S_ERROR_ITEM_NOT_FOUND if not)
 *   - MEMREF_OUTPUT: to actually get the key data
 */
#define WVDRM_GET_KEY_DATA 0x00001001

/*
 * Generate random data
 *
 * #0:
 *   - MEMREF_OUTPUT: buffer to fill with random data
 */
#define WVDRM_GET_RANDOM 0x00001002

/*
 * Get the device ID
 *
 * #0: MEMREF_OUTPUT: filled with the device ID
 */
#define WVDRM_GET_DEVICE_ID 0x00001003

/*
 * Optionnally decrypt a keybox with a transport key
 * and wrap it with a device specific key. The result
 * can be later passed to INSTALL_KEYBOX
 *
 * #0: MEMREF_INPUT: the input keybox
 *      - either in cleartext if param #2 is NONE
 *      - or encrypted with the key in param #2
 * #1: MEMREF_OUTPUT: the resulting wrapped key box
 * #2:
 *     - NONE: param#0 is the clear-text keybox
 *     - MEMREF_INPUT: a transport key, in which case
 *       param#0 is the encryption with AES-CBC-128 of the
 *       keybox with an IV filled with zeros
 */
#define WVDRM_WRAP_KEYBOX 0x00001004

/*
 * Unwrap an asset key. The asset key is stored in transient memory
 * but available globally to all sessons. There can be only one asset key
 * at a time.
 *
 * #0: MEMREF_INPUT
 */
#define WVDRM_SET_ENTITLEMENT_KEY 0x00002000

/*
 * Decrypt the ECM (Entitlement Control Message = content key) using the asset key.
 * Store the flags associated with the ECM. These flags will be later used, e.g.,
 * to activate HDCP protection. Also returns the flags.
 *
 * #0: MEMREF_INPUT
 * #1: VALUE_OUTPUT: a=flags
 *
 */
#define WVDRM_DERIVE_CONTROL_WORD 0x00002001

/*
 * Decrypt a chunk of content from a non-secure buffer into
 * a secure buffer opaquely referred to as an offset within
 * the Decrypted-Encoded-Buffer part of the carveout.
 *
 * #0: MEMREF_INPUT: the encrypted content
 * #1: VALUE_INPUT:
 *     [in]  a=physical address of the ION handle, b=size of the handle
 * #2: MEMREF_INOUT: the IV
 * #3: VALUE_INOUT:
 *     [in]  a=offset from the physical address of the ION handle, b=max size
 *     [out] b=actual size or required size
 */
#define WVDRM_DECRYPT_VIDEO 0x00002002

/*
 * Decrypt a chunk of content into a non-secure buffer. This
 * must be used only for audio content.
 *
 * #0: MEMREF_INPUT: the encrypted content
 * #1: MEMREF_OUTPUT: the decrypted content
 * #2: MEMREF_INOUT: the IV
 */
#define WVDRM_DECRYPT_AUDIO 0x00002003

/*
 * Enter in secure playback.
 */
#define WVDRM_ENTER_SECURE_PLAYBACK COMMON_SECURE_DRIVER_ENTER_SECURE_PLAYBACK

/*
 * Exit in secure playback.
 */
#define WVDRM_EXIT_SECURE_PLAYBACK COMMON_SECURE_DRIVER_EXIT_SECURE_PLAYBACK

#endif /* __WVDRM_PROTOCOL_H__ */
