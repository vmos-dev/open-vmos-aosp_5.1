/*******************************************************************************
 *
 * Reference APIs needed to support Widevine's crypto algorithms.
 *
 ******************************************************************************/

#ifndef _OEMCRYPTO_AES_H
#define _OEMCRYPTO_AES_H

typedef	unsigned char	OEMCrypto_UINT8;
typedef char		OEMCrypto_INT8;
typedef	unsigned int	OEMCrypto_UINT32;
typedef	unsigned int	OEMCrypto_SECURE_BUFFER;


typedef enum OEMCryptoResult {
  OEMCrypto_SUCCESS = 0,
  OEMCrypto_ERROR_INIT_FAILED,
  OEMCrypto_ERROR_TERMINATE_FAILED,
  OEMCrypto_ERROR_ENTER_SECURE_PLAYBACK_FAILED,
  OEMCrypto_ERROR_EXIT_SECURE_PLAYBACK_FAILED,
  OEMCrypto_ERROR_SHORT_BUFFER,
  OEMCrypto_ERROR_NO_DEVICE_KEY,
  OEMCrypto_ERROR_NO_ASSET_KEY,
  OEMCrypto_ERROR_KEYBOX_INVALID,
  OEMCrypto_ERROR_NO_KEYDATA,
  OEMCrypto_ERROR_NO_CW,
  OEMCrypto_ERROR_DECRYPT_FAILED,
  OEMCrypto_ERROR_WRITE_KEYBOX,
  OEMCrypto_ERROR_WRAP_KEYBOX,
  OEMCrypto_ERROR_BAD_MAGIC,
  OEMCrypto_ERROR_BAD_CRC,
  OEMCrypto_ERROR_NO_DEVICEID,
  OEMCrypto_ERROR_RNG_FAILED,
  OEMCrypto_ERROR_RNG_NOT_SUPPORTED,
  OEMCrypto_ERROR_SETUP
} OEMCryptoResult;


#ifdef __cplusplus
extern "C" {
#endif

#define OEMCrypto_Initialize _oec01
#define OEMCrypto_Terminate _oec02
#define OEMCrypto_SetEntitlementKey _oec03
#define OEMCrypto_DeriveControlWord _oec04
#define OEMCrypto_DecryptVideo _oec05
#define OEMCrypto_DecryptAudio _oec06
#define OEMCrypto_InstallKeybox _oec07
#define OEMCrypto_GetKeyData _oec08
#define OEMCrypto_IsKeyboxValid _oec09
#define OEMCrypto_GetRandom _oec10
#define OEMCrypto_GetDeviceID _oec11
#define OEMCrypto_EnterSecurePlayback _oec12
#define OEMCrypto_ExitSecurePlayback _oec13
#define OEMCrypto_WrapKeybox _oec14

/*
 * OEMCrypto_Initialize
 *
 * Description:
 *   Initializes the crypto hardware
 *
 * Parameters:
 *   N/A
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_INIT_FAILED failed to initialize crypto hardware
 */
OEMCryptoResult OEMCrypto_Initialize(void);


/*
 * OEMCrypto_Terminate
 *
 * Description:
 *   The API closes the crypto operation and releases all resources used.
 *
 * Parameters:
 *   N/A
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_TERMINATE_FAILED failed to de-initialize crypto hardware
 */
OEMCryptoResult OEMCrypto_Terminate(void);

/*
 * OEMCrypto_EnterSecurePlayback
 *
 * Description:
 *   Configures the security processor for secure decryption.  This may involve
 *   setting up firewall regions.  It is called when the decrypt session for an
 *   asset is established.
 *
 * Parameters:
 *   N/A
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_ENTER_SECURE_PLAYBACK_FAILED
 */
OEMCryptoResult OEMCrypto_EnterSecurePlayback(void);

/*
 * OEMCrypto_ExitSecurePlayback
 *
 * Description:
 *   Exit the secure playback mode.  This may involve releasing the firewall regions.  It is
 *   called when the decrypt session for an asset is closed.
 *
 * Parameters:
 *   N/A
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_EXIT_SECURE_PLAYBACK_FAILED
 */
OEMCryptoResult OEMCrypto_ExitSecurePlayback(void);

/*
 * OEMCrypto_SetEntitlementKey
 *
 * Description:
 *   Decrypt the entitlement (EMM) key, also known as the asset key,
 *   using the encrypted device key (Device Key field) in the Widevine Keybox.
 *
 *   As shown in Figure 1 on the next page, Step 1 uses an OEM root key to decrypt
 *   (AES-128-ECB) the Device Key in the Keybox; the result is “latched” in hardware
 *   key ladder.
 *
 *   Step 2 uses the “latched” clear device key to decrypt (AES-128-ECB) the
 *   entitlement key passed in as the *emmKey parameter and “latched” the clear
 *   entitlement key in hardware for the next operation.
 *
 * Parameters:
 *    emmKey (in) - pointer to the encrypted entitlement key
 *    emmKeyLength (in) – length of entitlement key in bytes
 *
 * Returns:
 *    OEMCrypto_SUCCESS success
 *    OEMCrypto_ERROR_NO_DEVICE_KEY failed to decrypt device key
 *    OEMCrypto_ERROR_NO_ASSET_KEY  failed to decrypt asset key
 *    OEMCrypto_ERROR_KEYBOX_INVALID cannot decrypt and read from Keybox
 */

OEMCryptoResult OEMCrypto_SetEntitlementKey(const OEMCrypto_UINT8* emmKey,
                                            const OEMCrypto_UINT32 emmKeyLength);

/*
 * OEMCrypto_DeriveControlWord
 *
 * Description:
 *   Using the active key ladder key from OEMCrypto_SetEntitlementKey(), decrypts
 *   (AES-128-CBC, iv=0) the 32-byte ECM referenced by the *ecm parameter; returns in
 *   *flags the first clear 4 bytes data. “Latched” the clear bytes [4..20] as the
 *   clear control word for subsequent payload decryption operation.
 *
 * Parameters:
 *   ecm (in) - points to encrypted ECM data
 *   length (in) – length of encrypted ECM data in bytes
 *   flags (out) - points to buffer to receive 4 byte clear flag value
 *
 *  Returns:
 *    OEMCrypto_SUCCESS success
 *    OEMCrypto_ERROR_NO_CW cannot decrypt control word
*/

OEMCryptoResult OEMCrypto_DeriveControlWord(const OEMCrypto_UINT8* ecm,
                                            const OEMCrypto_UINT32 length,
                                            OEMCrypto_UINT32* flags);


/*
 * OEMCrypto_DecryptVideo
 *
 * Description:
 *
 * The API decrypts (AES-128-CBC) the video payload in the buffer referenced by
 * the *input parameter into the secure buffer referenced by the output
 * parameter, using the control word “latched” in the active hardware key
 * ladder. If inputLength is not a multiple of the crypto block size (16 bytes),
 * the API handles the residual bytes using CipherText Stealing (CTS).
 *
 * Parameters:
 *   iv (in/out) - If iv is NULL, then no decryption is required, i.e. the packets are
 *                 already clear.  Otherwise, iv references the AES initialization
 *                 vector. Note that the updated IV  after processing the final crypto
 *                 block must be passed back out in *iv.
 *   input (in) - buffer containing the encrypted data
 *   inputLength (in) - number of bytes in the input payload, which may not be a multiple of 16 bytes
 *   output (in) – reference to the secure buffer which will receive the decrypted data
 *   outputLength (out) - number of bytes written into the secure buffer
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_DECRYPT_FAILED failed decryption
*/

OEMCryptoResult
OEMCrypto_DecryptVideo(const OEMCrypto_UINT8* iv,
                       const OEMCrypto_UINT8* input, const OEMCrypto_UINT32 inputLength,
                       OEMCrypto_UINT32 output_handle, OEMCrypto_UINT32 output_offset, OEMCrypto_UINT32 *outputLength);


/*
 * OEMCrypto_DecryptAudio
 *
 * Description:
 *   The API decrypts (AES-128-CBC) the audio payload in the buffer referenced by
 *   the *input parameter into the non-secure buffer referenced by the output
 *   parameter, using the control word “latched” in the active hardware key
 *   ladder. If inputLength is not a multiple of the crypto block size (16 bytes),
 *   the API handles the residual bytes using CipherText Stealing (CTS).
 *
 *   OEMCrypto_DecryptAudio must make sure that it cannot be used to decrypt a video
 *   stream into non-firewalled buffers, by verifying that no video packets are
 *   processed.
 *
 * Parameters:
 *   iv (in/out) - If iv is NULL, then no decryption is required, i.e. the packets are
 *                 already clear.  Otherwise, iv references the AES initialization
 *                 vector. Note that the updated IV  after processing the final crypto
 *                 block must be passed back out in *iv.
 *   input (in) - buffer containing the encrypted data
 *   inputLength (in) - number of bytes in the input payload, which may not be a multiple of 16 bytes
 *   output (in) – reference to the non-secure buffer which will receive the decrypted data
 *   outputLength (out) - number of bytes written into the non-secure buffer
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_DECRYPT_FAILED failed decryption
*/
OEMCryptoResult
OEMCrypto_DecryptAudio(const OEMCrypto_UINT8* iv,
                       const OEMCrypto_UINT8* input, const OEMCrypto_UINT32 inputLength,
                       OEMCrypto_UINT8 *output, OEMCrypto_UINT32 *outputLength);


/*
 * OEMCrypto_InstallKeybox
 *
 * Description:
 *   Unwrap and store the keybox to persistent memory. The device key must be stored
 *   securely. The device key will be decrypted and
 *   “latched” into hardware key ladder by OEMCrypto_SetEntitlementKey.
 *
 *   This function is used once to load the keybox onto the device at provisioning time.
 *
 * Parameters:
 *   keybox (in) - Pointer to clear keybox data.  Must have been wrapped with OEMCrypto_WrapKeybox
 *   keyboxLength (in) - Length of the keybox data in bytes
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_WRITE_KEYBOX failed to handle and store Keybox
 */

OEMCryptoResult OEMCrypto_InstallKeybox(OEMCrypto_UINT8 *keybox,
                                        OEMCrypto_UINT32 keyBoxLength);


/*
 * OEMCrypto_IsKeyboxValid
 *
 * Description:
 *   Validate the Widevine Keybox stored on the device.
 *
 * The API performs two verification steps on the Keybox. It first verifies the MAGIC
 * field contains a valid signature (i.e. ‘k’’b’’o’’x’). The API then computes the
 * CRC using CRC-32-IEEE 802.3 standard and compares the checksum to the CRC stored
 * in the Keybox. The CRC is computed over the entire Keybox excluding the 4 bytes
 * CRC (i.e. Keybox[0..123].
 *
 * Parameters:
 *   none
 *
 * Returns:
 *   OEMCrypto_SUCCESS
 *   OEMCrypto_ERROR_BAD_MAGIC
 *   OEMCrypto_ERROR_BAD_CRC
 */

OEMCryptoResult OEMCrypto_IsKeyboxValid(void);


/*
 * OEMCrypto_GetDeviceID
 *
 * Description:
 *   Retrieve the device's unique identifier from the Keybox.
 *
 * Parameters:
 *   deviceId (out) - pointer to the buffer that receives the Device ID
 *   idLength (in/out) - on input, size of the caller's device ID buffer.
 *        On output, the number of bytes written into the buffer.
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_SHORT_BUFFER if the buffer is too small to return the device ID
 *   OEMCrypto_ERROR_NO_DEVICEID failed to return Device Id
 */
OEMCryptoResult OEMCrypto_GetDeviceID(OEMCrypto_UINT8* deviceID,
                                      OEMCrypto_UINT32 *idLength);


/*
 * OEMCrypto_GetKeyData
 *
 * Description:
 *   Returns the Key Data field from the Keybox. The Key Data field does not need to be
 *   encrypted by an OEM root key, but may be if desired.
 *
 *   If the Key Data field was encrypted with an OEM root key when the Keybox was stored
 *   on the device, then this function should decrypt it and return the clear Key Data.
 *   If the Key Data was not encrypted, then this function should just access and return
 *   the clear Key data.
 *
 * Parameters:
 *   keyData (out) - pointer to the buffer to hold the Key Data field from the Keybox
 *   dataLength (in/out) - on input, the allocated buffer size.  On output, the number
 *       of bytes in KeyData.
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_SHORT_BUFFER if the buffer is too small to return the KeyData
 *   OEMCrypto_ERROR_NO_KEYDATA failed to return KeyData
 */
OEMCryptoResult OEMCrypto_GetKeyData(OEMCrypto_UINT8* keyData,
                                     OEMCrypto_UINT32 *keyDataLength);


/*
 * OEMCrypto_GetRandom
 *
 * Description:
 *   Returns a buffer filled with hardware-generated random bytes, if supported by the hardware.
 *
 * Parameters:
 *   randomData (out) - Points to the buffer that should recieve the random data.
 *   dataLength (in)  - Length of the random data buffer in bytes.
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_RNG_FAILED failed to generate random number
 *   OEMCrypto_ERROR_RNG_NOT_SUPPORTED function not supported
 */

OEMCryptoResult OEMCrypto_GetRandom(OEMCrypto_UINT8* randomData,
                                    OEMCrypto_UINT32 dataLength);

/*
 * OEMCrypto_WrapKeybox
 *
 * Description:
 *   Wrap the Keybox with a key derived for the device key.
 *
 * Parameters:
 *   keybox (in) - Pointer to keybox data.
 *   keyboxLength - Length of the Keybox data in bytes
 *   wrappedKeybox (out) - Pointer to wrapped keybox
 *   wrappedKeyboxLength (out) - Pointer to the length of the wrapped keybox in bytes
 *   transportKey (in) - An optional AES transport key. If provided, the parameter
 *                       keybox is passed encrypted with this transport key with AES-CBC
 *                       and a null IV
 *   transportKeyLength - number of bytes in the transportKey
 *
 * Returns:
 *   OEMCrypto_SUCCESS success
 *   OEMCrypto_ERROR_WRAP_KEYBOX failed to wrap Keybox
 */

OEMCryptoResult OEMCrypto_WrapKeybox(OEMCrypto_UINT8 *keybox,
                                     OEMCrypto_UINT32 keyBoxLength,
                                     OEMCrypto_UINT8 *wrappedKeybox,
                                     OEMCrypto_UINT32 *wrappedKeyBoxLength,
                                     OEMCrypto_UINT8 *transportKey,
                                     OEMCrypto_UINT32 transportKeyLength);

#ifdef __cplusplus
}
#endif

#endif

/***************************** End of File *****************************/
