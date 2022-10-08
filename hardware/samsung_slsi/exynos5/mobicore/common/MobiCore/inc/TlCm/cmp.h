/** @addtogroup CMP
 * Content Management Protocol Definitions.
 *
 * The CMP (Content Management Protocol) is based on the TCI (Trustlet Control
 * Interface) and defines commands/responses between the content management
 * trustlet (CMTL) and the content management trustlet connector (CMTLC) and/or
 * the remote backend.
 *
 * @{
 *
 * @file
 * CMP global definitions.
 * Various components need access to (sub-)structures defined and used by CMP;
 * these common definitions are made available through this header file.
 *
 * <!-- Copyright Giesecke & Devrient GmbH 2009-2012 -->
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

#ifndef CMP_H_
#define CMP_H_

#include "mcContainer.h"
#include "mcUuid.h"
#include "mcVersionInfo.h"
#include "version.h"

typedef uint32_t cmpCommandId_t;
typedef uint32_t cmpResponseId_t;
typedef uint32_t cmpReturnCode_t;

/** Responses have bit 31 set */
#define RSP_ID_MASK (1U << 31)
#define RSP_ID(cmdId) (((uint32_t)(cmdId)) | RSP_ID_MASK)
#define IS_CMD(cmdId) ((((uint32_t)(cmdId)) & RSP_ID_MASK) == 0)
#define IS_RSP(cmdId) ((((uint32_t)(cmdId)) & RSP_ID_MASK) == RSP_ID_MASK)

/**
 * CMP command header.
 */
typedef struct {
    /** Command ID. */
    cmpCommandId_t commandId; 
} cmpCommandHeader_t;

/**
 * CMP response header.
 */
typedef struct{
    /** Response ID (must be command ID | RSP_ID_MASK ). */
    cmpResponseId_t     responseId; 
    /** Return code of command. */
    cmpReturnCode_t     returnCode; 
} cmpResponseHeader_t;

/** SHA256 checksum. */
typedef struct {
    uint8_t data[32];
} cmpSha256_t;

/** Key size of encryption algorithm used for secure messaging. */
#define CMP_MSG_KEY_SIZE    32

/** Block size of the encryption algorithm used for secure messaging. */
#define CMP_MSG_CRYPTO_BLOCK_SIZE  16

/** Total number of padding bytes required to encrypt data of given size. */
#define CMP_ED_PADDING(netsize) (CMP_MSG_CRYPTO_BLOCK_SIZE - (netsize) % CMP_MSG_CRYPTO_BLOCK_SIZE)

/** Total number of bytes used for message authentication code (MAC). */
#define CMP_MAC_SIZE 32 // HMAC-SHA256

/** Total number of bytes used for PSS signature in GENERATE AUTH TOKEN command. */
#define CMP_GEN_AUTH_TOKEN_PSS_SIZE   256

/** Message authentication code. */
typedef struct {
    uint8_t mac[CMP_MAC_SIZE];
} cmpMac_t;

/** 64-bit random number. */
typedef struct {
    uint8_t data[8];
} cmpRnd8_t;

/** 256-bit random number. */
typedef struct {
    uint8_t data[32];
} cmpRnd32_t;

/** Version tags. */
typedef enum {
    CMP_VERSION_TAG1 = 0x00000001, // Deprecated.
    CMP_VERSION_TAG2 = 0x00000002,
} cmpVersionTag_t;

/** Version data for version tag 1. */
typedef struct {
    uint32_t number;
} cmpVersionData1_t;

/** Version data for version tag 2. */
typedef struct {
    mcVersionInfo_t versionInfo;
} cmpVersionData2_t;

/** Version data. */
typedef union {
    cmpVersionData1_t versionData1;
    cmpVersionData2_t versionData2;
} cmpVersionData_t;

/** @defgroup MC_CMP_CMD_GET_VERSION
* @{ */

/** @defgroup MC_CMP_CMD_GET_VERSION_CMD Command
* @{ */

/** GetVersion command. */
typedef struct {
    cmpCommandHeader_t cmdHeader;
} cmpCmdGetVersion_t;

/** @} */ 

/** @defgroup MC_CMP_CMD_GET_VERSION_RSP Response
* @{ */

/** GetSuid response. */
typedef struct {
cmpResponseHeader_t rspHeader;
    cmpVersionTag_t tag;
    cmpVersionData_t data;
} cmpRspGetVersion_t;

/** @} */

/** @} */ 

/** @defgroup MC_CMP_CMD_GENERATE_AUTH_TOKEN
 * @{ */

/** @defgroup MC_CMP_CMD_GENERATE_AUTH_TOKEN_CMD Command
 * @{ */

typedef struct {
    cmpCommandHeader_t cmdHeader;
    mcSuid_t suid;
    mcSymmetricKey_t kSocAuth;
    uint32_t kid;
} cmpGenAuthTokenCmdSdata_t;

typedef struct {
    cmpGenAuthTokenCmdSdata_t sdata;
    uint8_t pssSignature[CMP_GEN_AUTH_TOKEN_PSS_SIZE];
} cmpGenAuthTokenCmd_t;

/** GenAuthToken command. */
typedef struct {
    cmpGenAuthTokenCmd_t cmd;
} cmpCmdGenAuthToken_t;

/** @} */ 

/** @defgroup MC_CMP_CMD_GENERATE_AUTH_TOKEN_RSP Response
 * @{ */

typedef struct {
    cmpResponseHeader_t rspHeader;
    // No MAC.
} cmpGenAuthTokenRsp_t;

/** GenAuthToken response. */
typedef struct {
    cmpGenAuthTokenRsp_t rsp;
    mcSoAuthTokenCont_t soAuthCont;
} cmpRspGenAuthToken_t;

/** @} */ 

/** @} */ 

/** @defgroup MC_CMP_CMD_BEGIN_SOC_AUTHENTICATION
 * @{ */

/** @defgroup MC_CMP_CMD_BEGIN_SOC_AUTHENTICATION_CMD Command
 * @{ */

typedef struct {
    cmpCommandHeader_t cmdHeader;
} cmpBeginSocAuthenticationCmd_t;

/** BeginSocAuthentication command. */
typedef struct {
    cmpBeginSocAuthenticationCmd_t cmd;
    mcSoAuthTokenCont_t soAuthTokenCont;
} cmpCmdBeginSocAuthentication_t;

/** @} */ 

/** @defgroup MC_CMP_CMD_BEGIN_SOC_AUTHENTICATION_RSP Response
 * @{ */

typedef struct {
    cmpResponseHeader_t rspHeader;
    mcSuid_t suid;
    cmpRnd8_t rnd1;
} cmpBeginSocAuthenticationRspSdata_t;

typedef struct {
    cmpBeginSocAuthenticationRspSdata_t sdata;
    cmpMac_t mac;
} cmpBeginSocAuthenticationRsp_t;

/** BeginSocAuthentication response. */
typedef struct {
    cmpBeginSocAuthenticationRsp_t rsp;
} cmpRspBeginSocAuthentication_t;

/** @} */ 

/** @} */ 

/** @defgroup MC_CMP_CMD_BEGIN_ROOT_AUTHENTICATION
 * @{ */

/** @defgroup MC_CMP_CMD_BEGIN_ROOT_AUTHENTICATION_CMD Command
 * @{ */

typedef struct {
    cmpCommandHeader_t cmdHeader;
} cmpBeginRootAuthenticationCmd_t;

/** BeginRootAuthentication command. */
typedef struct {
    cmpBeginRootAuthenticationCmd_t cmd;
    mcSoRootCont_t soRootCont;
} cmpCmdBeginRootAuthentication_t;

/** @} */ 

/** @defgroup MC_CMP_CMD_BEGIN_ROOT_AUTHENTICATION_RSP Response
 * @{ */

typedef struct {
    cmpResponseHeader_t rspHeader;
    mcSuid_t suid;
    cmpRnd8_t rnd1;
} cmpBeginRootAuthenticationRspSdata_t;

typedef struct {
    cmpBeginRootAuthenticationRspSdata_t sdata;
    cmpMac_t mac;
} cmpBeginRootAuthenticationRsp_t;

/** BeginRootAuthentication response. */
typedef struct {
    cmpBeginRootAuthenticationRsp_t rsp;
} cmpRspBeginRootAuthentication_t;

/** @} */ 

/** @} */ 

/** @defgroup MC_CMP_CMD_BEGIN_SP_AUTHENTICATION
 * @{ */

/** @defgroup MC_CMP_CMD_BEGIN_SP_AUTHENTICATION_CMD Command
 * @{ */
typedef struct {
    cmpCommandHeader_t cmdHeader;
    mcSpid_t spid;
} cmpBeginSpAuthenticationCmdSdata_t;

typedef struct {
    cmpBeginSpAuthenticationCmdSdata_t sdata;
} cmpBeginSpAuthenticationCmd_t;

/** BeginSpAuthentication command. */
typedef struct {
    cmpBeginSpAuthenticationCmd_t cmd;
    mcSoRootCont_t soRootCont;
    mcSoSpCont_t soSpCont;
} cmpCmdBeginSpAuthentication_t;

/** @} */

/** @defgroup MC_CMP_CMD_BEGIN_SP_AUTHENTICATION_RSP Response
 * @{ */
typedef struct {
    cmpResponseHeader_t rspHeader;
    mcSuid_t suid;
    mcSpid_t spid;
    cmpRnd8_t rnd1;
} cmpBeginSpAuthenticationRspSdata_t;

typedef struct {
    cmpBeginSpAuthenticationRspSdata_t sdata;
    cmpMac_t mac;
} cmpBeginSpAuthenticationRsp_t;

/** BeginSpAuthentication response. */
typedef struct {
    cmpBeginSpAuthenticationRsp_t rsp;
} cmpRspBeginSpAuthentication_t;

/** @} */ 

/** @} */ 

/** @defgroup MC_CMP_CMD_AUTHENTICATE 
 * @{ */

/** @defgroup MC_CMP_CMD_AUTHENTICATE_CMD Command
 * @{ */
typedef struct {
    mcSuid_t suid;
    uint32_t entityId;
    cmpRnd8_t rnd2;
    cmpRnd8_t rnd1;
    cmpRnd32_t k2;
} cmpAuthMsgEdata_t;

typedef struct {
    cmpAuthMsgEdata_t edata;
    uint8_t padding[CMP_ED_PADDING(sizeof(cmpAuthMsgEdata_t))];
} cmpAuthCmdEd_t;

typedef struct {
    cmpCommandHeader_t cmdHeader;
    cmpAuthCmdEd_t ed;
} cmpAuthCmdSdata_t; 

typedef struct {
    cmpAuthCmdSdata_t sdata;
    cmpMac_t mac;
} cmpAuthenticateCmd_t;

/** Authenticate command. */
typedef struct {
    cmpAuthenticateCmd_t cmd;
} cmpCmdAuthenticate_t;

/** @} */ 

/** @defgroup MC_CMP_CMD_AUTHENTICATE_RSP Response
 * @{ */
typedef struct {
    mcSuid_t suid;
    uint32_t entityId;
    cmpRnd8_t rnd1;
    cmpRnd8_t rnd2;
    cmpRnd32_t k1;
} cmpAuthRspEdata_t;

typedef struct {
    cmpAuthRspEdata_t edata;
    uint8_t padding[CMP_ED_PADDING(sizeof(cmpAuthRspEdata_t))];
} cmpAuthRspEd_t;

typedef struct {
    cmpResponseHeader_t rspHeader;
    cmpAuthRspEd_t ed;
} cmpAuthRspSdata_t;

typedef struct {
    cmpAuthRspSdata_t sdata;
    cmpMac_t mac;
} cmpAuthenticateRsp_t;

/** Authenticate response. */
typedef struct {
    cmpAuthenticateRsp_t rsp;
} cmpRspAuthenticate_t;

/** @} */ 

/** @} */ 

/** @defgroup MC_CMP_CMD_ROOT_CONT_REGISTER_ACTIVATE 
 * @{ */
 
/** @defgroup MC_CMP_CMD_ROOT_CONT_REGISTER_ACTIVATE_CMD Command
 * @{ */
 
typedef struct {
    mcSymmetricKey_t kRootAuth;
} cmpRootRegActMsgEdata_t;

typedef struct {
    cmpRootRegActMsgEdata_t edata;
    uint8_t padding[CMP_ED_PADDING(sizeof(cmpRootRegActMsgEdata_t))];
} cmpRootRegActCmdEd_t;

typedef struct {
    cmpCommandHeader_t cmdHeader;
    mcRootid_t rootid;
    cmpRootRegActCmdEd_t ed;
} cmpRootRegActCmdSdata_t;

typedef struct {
    cmpRootRegActCmdSdata_t sdata;
    cmpMac_t mac;
} cmpRootContRegisterActivateCmd_t;

/** RootContRegisterActivate command. */
typedef struct {
    cmpRootContRegisterActivateCmd_t cmd;
} cmpCmdRootContRegisterActivate_t;

/** @} */ 

/** @defgroup MC_CMP_CMD_ROOT_CONT_REGISTER_ACTIVATE_RSP Response
 * @{ */
 
typedef struct {
    mcSoRootCont_t soRootCont;
} cmpRootRegActRspEdata_t;

typedef struct {
    cmpRootRegActRspEdata_t edata;
    uint8_t padding[CMP_ED_PADDING(sizeof(cmpRootRegActRspEdata_t))];
} cmpRootRegActRspEd_t;

typedef struct {
    cmpResponseHeader_t rspHeader;
    cmpRootRegActRspEd_t ed;
} cmpRootRegActRspSdata_t;

typedef struct {
    cmpRootRegActRspSdata_t sdata;
    cmpMac_t mac;
} cmpRootContRegisterActivateRsp_t;

/** RooContRegisterActivate response. */
typedef struct {
    cmpRootContRegisterActivateRsp_t rsp;
    mcSoRootCont_t soRootCont;
} cmpRspRootContRegisterActivate_t;

/** @} */

/** @} */

/** @defgroup MC_CMP_CMD_ROOT_CONT_UNREGISTER 
 * @{ */

/** @defgroup MC_CMP_CMD_ROOT_CONT_UNREGISTER_CMD Command
 * @{ */

typedef struct {
    mcSuid_t suid;
    mcSoAuthTokenCont_t soAuthTokenCont;
} cmpRootUnregMsgEdata_t;

typedef struct {
    cmpRootUnregMsgEdata_t edata;
    uint8_t padding[CMP_ED_PADDING(sizeof(cmpRootUnregMsgEdata_t))];
} cmpRootUnregCmdEd_t;

typedef struct {
    cmpCommandHeader_t cmdHeader;
    cmpRootUnregCmdEd_t ed;
} cmpRootUnregCmdSdata_t;

typedef struct {
    cmpRootUnregCmdSdata_t sdata;
    cmpMac_t mac;
} cmpRootContUnregisterCmd_t;

/** RootContUnregister command. */
typedef struct {
    cmpRootContUnregisterCmd_t cmd;
} cmpCmdRootContUnregister_t;

/** @} */ 

/** @defgroup MC_CMP_CMD_ROOT_CONT_UNREGISTER_RSP Response
 * @{ */

typedef struct {
    mcSuid_t suid;
} cmpRootUnregRspEdata_t;

typedef struct {
    cmpRootUnregRspEdata_t edata;
    uint8_t padding[CMP_ED_PADDING(sizeof(cmpRootUnregRspEdata_t))];
} cmpRootUnregRspEd_t;

typedef struct {
    cmpResponseHeader_t rspHeader;
    cmpRootUnregRspEd_t ed;
} cmpRootUnregRspSdata_t;

typedef struct {
    cmpRootUnregRspSdata_t sdata;
    cmpMac_t mac;
} cmpRootContUnregisterRsp_t;

/** RootContUnregister response. */
typedef struct {
    cmpRootContUnregisterRsp_t rsp;
    mcSoAuthTokenCont_t soAuthTokenCont;
} cmpRspRootContUnregister_t;

/** @} */ 

/** @} */ 

/** @defgroup MC_CMP_CMD_ROOT_CONT_LOCK_BY_ROOT
 * @{ */

/** @defgroup MC_CMP_CMD_ROOT_CONT_LOCK_BY_ROOT_CMD Command
 * @{ */

typedef struct {
    cmpCommandHeader_t cmdHeader;
} cmpRootLockByRootCmdSdata_t;

typedef struct {
    cmpRootLockByRootCmdSdata_t sdata;
    cmpMac_t mac;
} cmpRootContLockByRootCmd_t;

/** RootContLockByRoot command. */
typedef struct {
    cmpRootContLockByRootCmd_t cmd;
} cmpCmdRootContLockByRoot_t;

/** @} */ 

/** @defgroup MC_CMP_CMD_ROOT_CONT_LOCK_BY_ROOT_RSP Response
 * @{ */

typedef struct {
    mcSoRootCont_t soRootCont;
} cmpRootLockByRootRspEdata_t;

typedef struct {
    cmpRootLockByRootRspEdata_t edata;
    uint8_t padding[CMP_ED_PADDING(sizeof(cmpRootLockByRootRspEdata_t))];
} cmpRootLockByRootRspEd_t;

typedef struct {
    cmpResponseHeader_t rspHeader;
    cmpRootLockByRootRspEd_t ed;
} cmpRootLockByRootRspSdata_t;

typedef struct {
    cmpRootLockByRootRspSdata_t sdata;
    cmpMac_t mac;
} cmpRootContLockByRootRsp_t;

/** RootContLockByRoot response. */
typedef struct {
    cmpRootContLockByRootRsp_t rsp;
    mcSoRootCont_t soRootCont;
} cmpRspRootContLockByRoot_t;

/** @} */ 

/** @} */ 

/** @defgroup MC_CMP_CMD_ROOT_CONT_UNLOCK_BY_ROOT
 * @{ */

/** @defgroup MC_CMP_CMD_ROOT_CONT_UNLOCK_BY_ROOT_CMD Command
 * @{ */

typedef struct {
    cmpCommandHeader_t cmdHeader;
} cmpRootUnlockByRootCmdSdata_t;

typedef struct {
    cmpRootUnlockByRootCmdSdata_t sdata;
    cmpMac_t mac;
} cmpRootContUnlockByRootCmd_t;

/** RootContUnlockByRoot command. */
typedef struct {
    cmpRootContUnlockByRootCmd_t cmd;
} cmpCmdRootContUnlockByRoot_t;

/** @} */ 

/** @defgroup MC_CMP_CMD_ROOT_CONT_UNLOCK_BY_ROOT_RSP Response
 * @{ */

typedef struct {
    mcSoRootCont_t soRootCont;
} cmpRootUnlockByRootRspEdata_t;

typedef struct {
    cmpRootUnlockByRootRspEdata_t edata;
    uint8_t padding[CMP_ED_PADDING(sizeof(cmpRootUnlockByRootRspEdata_t))];
} cmpRootUnlockByRootRspEd_t;

typedef struct {
    cmpResponseHeader_t rspHeader;
    cmpRootUnlockByRootRspEd_t ed;
} cmpRootUnlockByRootRspSdata_t;

typedef struct {
    cmpRootUnlockByRootRspSdata_t sdata;
    cmpMac_t mac;
} cmpRootContUnlockByRootRsp_t;

/** RootContUnlockByRoot response. */
typedef struct {
    cmpRootContUnlockByRootRsp_t rsp;
    mcSoRootCont_t soRootCont;
} cmpRspRootContUnlockByRoot_t;

/** @} */ 

/** @} */ 

/** @defgroup MC_CMP_CMD_SP_CONT_REGISTER_ACTIVATE
 * @{ */

/** @defgroup MC_CMP_CMD_SP_CONT_REGISTER_ACTIVATE_CMD Command
 * @{ */

typedef struct {
    mcSymmetricKey_t kSpAuth;
} cmpSpRegActMsgEdata_t;

typedef struct {
    cmpSpRegActMsgEdata_t edata;
    uint8_t padding[CMP_ED_PADDING(sizeof(cmpSpRegActMsgEdata_t))];
} cmpSpRegActCmdEd_t;

typedef struct {
    cmpCommandHeader_t cmdHeader;
    mcSpid_t spid;
    cmpSpRegActCmdEd_t ed;
} cmpSpRegActCmdSdata_t;

typedef struct {
    cmpSpRegActCmdSdata_t sdata;
    cmpMac_t mac;
} cmpSpContRegisterActivateCmd_t;

/** SpContRegisterActivate command. */
typedef struct {
    cmpSpContRegisterActivateCmd_t cmd;
} cmpCmdSpContRegisterActivate_t;

/** @} */ 

/** @defgroup MC_CMP_CMD_SP_CONT_REGISTER_ACTIVATE_RSP Response
 * @{ */

typedef struct {
    mcSoRootCont_t soRootCont;
    mcSoSpCont_t soSpCont;
} cmpSpRegActRspEdata_t;

typedef struct {
    cmpSpRegActRspEdata_t edata;
    uint8_t padding[CMP_ED_PADDING(sizeof(cmpSpRegActRspEdata_t))];
} cmpSpRegActRspEd_t;

typedef struct {
    cmpResponseHeader_t rspHeader;
    cmpSpRegActRspEd_t ed;
} cmpSpRegActRspSdata_t;

typedef struct {
    cmpSpRegActRspSdata_t sdata;
    cmpMac_t mac;
} cmpSpContRegisterActivateRsp_t;

/** SpContRegisterActivate response. */
typedef struct {
    cmpSpContRegisterActivateRsp_t rsp;
    mcSoRootCont_t soRootCont;
    mcSoSpCont_t soSpCont;
} cmpRspSpContRegisterActivate_t;

/** @} */ 

/** @} */ 

/** @defgroup MC_CMP_CMD_SP_CONT_REGISTER
 * @{ */

/** @defgroup MC_CMP_CMD_SP_CONT_REGISTER_CMD Command
 * @{ */

typedef struct {
    mcSymmetricKey_t kSpAuth;
} cmpSpRegisterMsgEdata_t;

typedef struct {
    cmpSpRegisterMsgEdata_t edata;
    uint8_t padding[CMP_ED_PADDING(sizeof(cmpSpRegisterMsgEdata_t))];
} cmpSpRegisterCmdEd_t;

typedef struct {
    cmpCommandHeader_t cmdHeader;
    mcSpid_t spid;
    cmpSpRegisterCmdEd_t ed;
} cmpSpRegisterCmdSdata_t;

typedef struct {
    cmpSpRegisterCmdSdata_t sdata;
    cmpMac_t mac;
} cmpSpContRegisterCmd_t;

/** SpContRegister command. */
typedef struct {
    cmpSpContRegisterCmd_t cmd;
} cmpCmdSpContRegister_t;

/** @} */ 

/** @defgroup MC_CMP_CMD_SP_CONT_REGISTER_RSP Response
 * @{ */

typedef struct {
    mcSoRootCont_t soRootCont;
    mcSoSpCont_t soSpCont;
} cmpSpRegisterRspEdata_t;

typedef struct {
    cmpSpRegisterRspEdata_t edata;
    uint8_t padding[CMP_ED_PADDING(sizeof(cmpSpRegisterRspEdata_t))];
} cmpSpRegisterRspEd_t;

typedef struct {
    cmpResponseHeader_t rspHeader;
    cmpSpRegisterRspEd_t ed;
} cmpSpRegisterRspSdata_t;

typedef struct {
    cmpSpRegisterRspSdata_t sdata;
    cmpMac_t mac;
} cmpSpContRegisterRsp_t;

/** SpContRegister response. */
typedef struct {
    cmpSpContRegisterRsp_t rsp;
    mcSoRootCont_t soRootCont;
    mcSoSpCont_t soSpCont;
} cmpRspSpContRegister_t;

/** @} */ 

/** @} */ 

/** @defgroup MC_CMP_CMD_SP_CONT_ACTIVATE
 * @{ */

/** @defgroup MC_CMP_CMD_SP_CONT_ACTIVATE_CMD Command
 * @{ */

typedef struct {
    mcSymmetricKey_t kSpAuth;
} cmpSpActivateMsgEdata_t;

typedef struct {
    cmpSpActivateMsgEdata_t edata;
    uint8_t padding[CMP_ED_PADDING(sizeof(cmpSpActivateMsgEdata_t))];
} cmpSpActivateCmdEd_t;

typedef struct {
    cmpCommandHeader_t cmdHeader;
    mcSpid_t spid;
    cmpSpActivateCmdEd_t ed;
} cmpSpActivateCmdSdata_t;

typedef struct {
    cmpSpActivateCmdSdata_t sdata;
    cmpMac_t mac;
} cmpSpContActivateCmd_t;

/** SpContActivate command. */
typedef struct {
    cmpSpContActivateCmd_t cmd;
} cmpCmdSpContActivate_t;

/** @} */ 

/** @defgroup MC_CMP_CMD_SP_CONT_ACTIVATE_RSP Response
 * @{ */

typedef struct {
    mcSoSpCont_t soSpCont;
} cmpSpActivateRspEdata_t;

typedef struct {
    cmpSpActivateRspEdata_t edata;
    uint8_t padding[CMP_ED_PADDING(sizeof(cmpSpActivateRspEdata_t))];
} cmpSpActivateRspEd_t;

typedef struct {
    cmpResponseHeader_t rspHeader;
    cmpSpActivateRspEd_t ed;
} cmpSpActivateRspSdata_t;

typedef struct {
    cmpSpActivateRspSdata_t sdata;
    cmpMac_t mac;
} cmpSpContActivateRsp_t;

/** SpContActivate response. */
typedef struct {
    cmpSpContActivateRsp_t rsp;
    mcSoSpCont_t soSpCont;
} cmpRspSpContActivate_t;

/** @} */ 

/** @} */ 

/** @defgroup MC_CMP_CMD_SP_CONT_UNREGISTER 
 * @{ */

/** @defgroup MC_CMP_CMD_SP_CONT_UNREGISTER_CMD Command
 * @{ */

typedef struct {
    cmpCommandHeader_t cmdHeader;
    mcSpid_t spid;
} cmpSpContUnregCmdSdata_t;

typedef struct {
    cmpSpContUnregCmdSdata_t sdata;
    cmpMac_t mac;
} cmpSpContUnregisterCmd_t;

/** SpContUnregister command. */
typedef struct {
    cmpSpContUnregisterCmd_t cmd;
} cmpCmdSpContUnregister_t;

/** @} */ 

/** @defgroup MC_CMP_CMD_SP_CONT_UNREGISTER_RSP Response
 * @{ */

typedef struct {
    mcSoRootCont_t soRootCont;
} cmpSpUnregRspEdata_t;

typedef struct {
    cmpSpUnregRspEdata_t edata;
    uint8_t padding[CMP_ED_PADDING(sizeof(cmpSpUnregRspEdata_t))];
} cmpSpUnregRspEd_t;

typedef struct {
    cmpResponseHeader_t rspHeader;
    cmpSpUnregRspEd_t ed;
} cmpSpContUnregRspSdata_t;

typedef struct {
    cmpSpContUnregRspSdata_t sdata;
    cmpMac_t mac;
} cmpSpContUnregisterRsp_t;

/** SpContUnregister response. */
typedef struct {
    cmpSpContUnregisterRsp_t rsp;
    mcSoRootCont_t soRootCont;
} cmpRspSpContUnregister_t;

/** @} */ 

/** @} */ 

/** @defgroup MC_CMP_CMD_SP_CONT_LOCK_BY_ROOT 
 * @{ */

/** @defgroup MC_CMP_CMD_SP_CONT_LOCK_BY_ROOT_CMD Command
 * @{ */

typedef struct {
    cmpCommandHeader_t cmdHeader; 
    mcSpid_t spid;
} cmpSpLockByRootCmdSdata_t;

typedef struct {
    cmpSpLockByRootCmdSdata_t sdata;
    cmpMac_t mac;
} cmpSpContLockByRootCmd_t;

/** SpContLockByRoot command. */
typedef struct {
    cmpSpContLockByRootCmd_t cmd;
    mcSoSpCont_t soSpCont;
} cmpCmdSpContLockByRoot_t;

/** @} */ 

/** @defgroup MC_CMP_CMD_SP_CONT_LOCK_BY_ROOT_RSP Response
 * @{ */

typedef struct {
    mcSoSpCont_t soSpCont;
} cmpSpLockByRootRspEdata_t;

typedef struct {
    cmpSpLockByRootRspEdata_t edata;
    uint8_t padding[CMP_ED_PADDING(sizeof(cmpSpLockByRootRspEdata_t))];
} cmpSpLockByRootRspEd_t;

typedef struct {
    cmpResponseHeader_t rspHeader;
    cmpSpLockByRootRspEd_t ed;
} cmpSpLockByRootRspSdata_t;

typedef struct {
    cmpSpLockByRootRspSdata_t sdata;
    cmpMac_t mac;
} cmpSpContLockByRootRsp_t;

/** SpContLockByRoot response. */
typedef struct {
    cmpSpContLockByRootRsp_t rsp;
    mcSoSpCont_t soSpCont;
} cmpRspSpContLockByRoot_t;

/** @} */ 

/** @} */ 

/** @defgroup MC_CMP_CMD_SP_CONT_UNLOCK_BY_ROOT 
 * @{ */

/** @defgroup MC_CMP_CMD_SP_CONT_UNLOCK_BY_ROOT_CMD Command
 * @{ */

typedef struct {
    cmpCommandHeader_t cmdHeader; 
    mcSpid_t spid;
} cmpSpUnlockByRootCmdSdata_t;

typedef struct {
    cmpSpUnlockByRootCmdSdata_t sdata;
    cmpMac_t mac;
} cmpSpContUnlockByRootCmd_t;

/** SpContUnlockByRoot command. */
typedef struct {
    cmpSpContUnlockByRootCmd_t cmd;
    mcSoSpCont_t soSpCont;
} cmpCmdSpContUnlockByRoot_t;

/** @} */ 

/** @defgroup MC_CMP_CMD_SP_CONT_UNLOCK_BY_ROOT_RSP Response
 * @{ */

typedef struct {
    mcSoSpCont_t soSpCont;
} cmpSpUnlockByRootRspEdata_t;

typedef struct {
    cmpSpUnlockByRootRspEdata_t edata;
    uint8_t padding[CMP_ED_PADDING(sizeof(cmpSpUnlockByRootRspEdata_t))];
} cmpSpUnlockByRootRspEd_t;

typedef struct {
    cmpResponseHeader_t rspHeader;
    cmpSpUnlockByRootRspEd_t ed;
} cmpSpUnlockByRootRspSdata_t;

typedef struct {
    cmpSpUnlockByRootRspSdata_t sdata;
    cmpMac_t mac;
} cmpSpContUnlockByRootRsp_t;

/** SpContUnlockByRoot response. */
typedef struct {
    cmpSpContUnlockByRootRsp_t rsp;
    mcSoSpCont_t soSpCont;
} cmpRspSpContUnlockByRoot_t;

/** @} */ 

/** @} */ 

/** @defgroup MC_CMP_CMD_SP_CONT_LOCK_BY_SP
 * @{ */

/** @defgroup MC_CMP_CMD_SP_CONT_LOCK_BY_SP_CMD Command
 * @{ */

typedef struct {
    cmpCommandHeader_t cmdHeader; 
    mcSpid_t spid;
} cmpSpLockBySpCmdSdata_t;

typedef struct {
    cmpSpLockBySpCmdSdata_t sdata;
    cmpMac_t mac;
} cmpSpContLockBySpCmd_t;

/** SpContLockBySp command. */
typedef struct {
    cmpSpContLockBySpCmd_t cmd;
} cmpCmdSpContLockBySp_t;

/** @} */ 

/** @defgroup MC_CMP_CMD_SP_CONT_LOCK_BY_SP_RSP Respose
 * @{ */

typedef struct {
    mcSoSpCont_t soSpCont;
} cmpSpLockBySpRspEdata_t;

typedef struct {
    cmpSpLockBySpRspEdata_t edata;
    uint8_t padding[CMP_ED_PADDING(sizeof(cmpSpLockBySpRspEdata_t))];
} cmpSpLockBySpRspEd_t;

typedef struct {
    cmpResponseHeader_t rspHeader;
    cmpSpLockBySpRspEd_t ed;
} cmpSpLockBySpRspSdata_t;

typedef struct {
    cmpSpLockBySpRspSdata_t sdata;
    cmpMac_t mac;
} cmpSpContLockBySpRsp_t;

/** SpContLockBySp response. */
typedef struct {
    cmpSpContLockBySpRsp_t rsp;
    mcSoSpCont_t soSpCont;
} cmpRspSpContLockBySp_t;

/** @} */ 

/** @} */ 

/** @defgroup MC_CMP_CMD_SP_CONT_UNLOCK_BY_SP
 * @{ */

/** @defgroup MC_CMP_CMD_SP_CONT_UNLOCK_BY_SP_CMD Command
 * @{ */

typedef struct {
    cmpCommandHeader_t cmdHeader; 
    mcSpid_t spid;
} cmpSpUnlockBySpCmdSdata_t;

typedef struct {
    cmpSpUnlockBySpCmdSdata_t sdata;
    cmpMac_t mac;
} cmpSpContUnlockBySpCmd_t;

/** SpContUnlockBySp command. */
typedef struct {
    cmpSpContUnlockBySpCmd_t cmd;
} cmpCmdSpContUnlockBySp_t;

/** @} */ 

/** @defgroup MC_CMP_CMD_SP_CONT_UNLOCK_BY_SP_RSP Response
 * @{ */

typedef struct {
    mcSoSpCont_t soSpCont;
} cmpSpUnlockBySpRspEdata_t;

typedef struct {
    cmpSpUnlockBySpRspEdata_t edata;
    uint8_t padding[CMP_ED_PADDING(sizeof(cmpSpUnlockBySpRspEdata_t))];
} cmpSpUnlockBySpRspEd_t;

typedef struct {
    cmpResponseHeader_t rspHeader;
    cmpSpUnlockBySpRspEd_t ed;
} cmpSpUnlockBySpRspSdata_t;

typedef struct {
    cmpSpUnlockBySpRspSdata_t sdata;
    cmpMac_t mac;
} cmpSpContUnlockBySpRsp_t;

/** SpContUnlockBySp response. */
typedef struct {
    cmpSpContUnlockBySpRsp_t rsp;
    mcSoSpCont_t soSpCont;
} cmpRspSpContUnlockBySp_t;

/** @} */ 

/** @} */ 

/** @defgroup MC_CMP_CMD_TLT_CONT_REGISTER 
 * @{ */

/** @defgroup MC_CMP_CMD_TLT_CONT_REGISTER_CMD Command
 * @{ */

typedef struct {
    mcSymmetricKey_t kSpTltEnc;
} cmpTltRegMsgEdata_t;

typedef struct {
    cmpTltRegMsgEdata_t edata;
    uint8_t padding[CMP_ED_PADDING(sizeof(cmpTltRegMsgEdata_t))];
} cmpTltRegCmdEd_t;

typedef struct {
    cmpCommandHeader_t cmdHeader;
    mcSpid_t spid;
    mcUuid_t uuid;
    cmpTltRegCmdEd_t ed;
} cmpTltRegCmdSdata_t;

typedef struct {
    cmpTltRegCmdSdata_t sdata;
    cmpMac_t mac;
} cmpTltContRegisterCmd_t;

/** TltContRegister command. */
typedef struct {
    cmpTltContRegisterCmd_t cmd;
} cmpCmdTltContRegister_t;

/** @} */ 

/** @defgroup MC_CMP_CMD_TLT_CONT_REGISTER_RSP Response
 * @{ */

typedef struct {
    mcSoSpCont_t soSpCont;
    mcSoTltCont_t soTltCont;
} cmpTltRegRspEdata_t;

typedef struct {
    cmpTltRegRspEdata_t edata;
    uint8_t padding[CMP_ED_PADDING(sizeof(cmpTltRegRspEdata_t))];
} cmpTltRegRspEd_t;

typedef struct {
    cmpResponseHeader_t rspHeader;
    cmpTltRegRspEd_t ed;
} cmpTltRegRspSdata_t;

typedef struct {
    cmpTltRegRspSdata_t sdata;
    cmpMac_t mac;
} cmpTltContRegisterRsp_t;

/** TltContRegister response. */
typedef struct {
    cmpTltContRegisterRsp_t rsp;
    mcSoSpCont_t soSpCont;
    mcSoTltCont_t soTltCont;
} cmpRspTltContRegister_t;

/** @} */ 

/** @} */ 

/** @defgroup MC_CMP_CMD_TLT_CONT_ACTIVATE 
 * @{ */

/** @defgroup MC_CMP_CMD_TLT_CONT_ACTIVATE_CMD Command
 * @{ */

typedef struct {
    cmpCommandHeader_t cmdHeader;
    mcSpid_t spid;
    mcUuid_t uuid;
} cmpTltActCmdSdata_t;

typedef struct {
    cmpTltActCmdSdata_t sdata;
    cmpMac_t mac;
} cmpTltContActivateCmd_t;

/** TltContActivate command. */
typedef struct {
    cmpTltContActivateCmd_t cmd;
    mcSoTltCont_t soTltCont;
} cmpCmdTltContActivate_t;

/** @} */ 

/** @defgroup MC_CMP_CMD_TLT_CONT_ACTIVATE_RSP Response
 * @{ */

typedef struct {
    mcSoTltCont_t soTltCont;
} cmpTltActRspEdata_t;

typedef struct {
    cmpTltActRspEdata_t edata;
    uint8_t padding[CMP_ED_PADDING(sizeof(cmpTltActRspEdata_t))];
} cmpTltActRspEd_t;

typedef struct {
    cmpResponseHeader_t rspHeader;
    cmpTltActRspEd_t ed;
} cmpTltActRspSdata_t;

typedef struct {
    cmpTltActRspSdata_t sdata;
    cmpMac_t mac;
} cmpTltContActivateRsp_t;

/** TltContActivate response. */
typedef struct {
    cmpTltContActivateRsp_t rsp;
    mcSoTltCont_t soTltCont;
} cmpRspTltContActivate_t;

/** @} */ 

/** @} */ 

/** @defgroup MC_CMP_CMD_TLT_CONT_REGISTER_ACTIVATE 
 * @{ */

/** @defgroup MC_CMP_CMD_TLT_CONT_REGISTER_ACTIVATE_CMD Command
 * @{ */

typedef struct {
    mcSymmetricKey_t kSpTltEnc;
} cmpTltRegActMsgEdata_t;

typedef struct {
    cmpTltRegActMsgEdata_t edata;
    uint8_t padding[CMP_ED_PADDING(sizeof(cmpTltRegActMsgEdata_t))];
} cmpTltRegActCmdEd_t;

typedef struct {
    cmpCommandHeader_t cmdHeader;
    mcSpid_t spid;
    mcUuid_t uuid;
    cmpTltRegActCmdEd_t ed;
} cmpTltRegActCmdSdata_t;

typedef struct {
    cmpTltRegActCmdSdata_t sdata;
    cmpMac_t mac;
} cmpTltContRegisterActivateCmd_t;

/** TltContRegisterActivate command. */
typedef struct {
    cmpTltContRegisterActivateCmd_t cmd;
} cmpCmdTltContRegisterActivate_t;

/** @} */ 

/** @defgroup MC_CMP_CMD_TLT_CONT_REGISTER_ACTIVATE_RSP Response
 * @{ */

typedef struct {
    mcSoSpCont_t soSpCont;
    mcSoTltCont_t soTltCont;
} cmpTltRegActRspEdata_t;

typedef struct {
    cmpTltRegActRspEdata_t edata;
    uint8_t padding[CMP_ED_PADDING(sizeof(cmpTltRegActRspEdata_t))];
} cmpTltRegActRspEd_t;

typedef struct {
    cmpResponseHeader_t rspHeader;
    cmpTltRegActRspEd_t ed;
} cmpTltRegActRspSdata_t;

typedef struct {
    cmpTltRegActRspSdata_t sdata;
    cmpMac_t mac;
} cmpTltContRegisterActivateRsp_t;

/** TltContRegisterActivate response. */
typedef struct {
    cmpTltContRegisterActivateRsp_t rsp;
    mcSoSpCont_t soSpCont;
    mcSoTltCont_t soTltCont;
} cmpRspTltContRegisterActivate_t;

/** @} */ 

/** @} */ 

/** @defgroup MC_CMP_CMD_TLT_CONT_UNREGISTER
 * @{ */

/** @defgroup MC_CMP_CMD_TLT_CONT_UNREGISTER_CMD Command
 * @{ */

typedef struct {
    cmpCommandHeader_t cmdHeader;
    mcSpid_t spid;
    mcUuid_t uuid;
} cmpTltUnregCmdSdata_t;

typedef struct {
    cmpTltUnregCmdSdata_t sdata;
    cmpMac_t mac;
} cmpTltContUnregisterCmd_t;

/** TltContUnregister command. */
typedef struct {
    cmpTltContUnregisterCmd_t cmd;
} cmpCmdTltContUnregister_t;

/** @} */ 

/** @defgroup MC_CMP_CMD_TLT_CONT_UNREGISTER_RSP Response
 * @{ */

typedef struct {
    mcSoSpCont_t soSpCont;
} cmpTltUnregRspEdata_t;

typedef struct {
    cmpTltUnregRspEdata_t edata;
    uint8_t padding[CMP_ED_PADDING(sizeof(cmpTltUnregRspEdata_t))];
} cmpTltUnregRspEd_t;

typedef struct {
    cmpResponseHeader_t rspHeader;
    cmpTltUnregRspEd_t ed;
} cmpTltUnregRspSdata_t;

typedef struct {
    cmpTltUnregRspSdata_t sdata;
    cmpMac_t mac;
} cmpTltContUnregisterRsp_t;

/** TltContUnregister response. */
typedef struct {
    cmpTltContUnregisterRsp_t rsp;
    mcSoSpCont_t soSpCont;
} cmpRspTltContUnregister_t;

/** @} */ 

/** @} */ 

/** @defgroup MC_CMP_CMD_TLT_CONT_LOCK_BY_SP 
 * @{ */

/** @defgroup MC_CMP_CMD_TLT_CONT_LOCK_BY_SP_CMD Command
 * @{ */

typedef struct {
    cmpCommandHeader_t cmdHeader;
    mcSpid_t spid;
    mcUuid_t uuid;
} cmpTltLockBySpCmdSdata_t;

typedef struct {
    cmpTltLockBySpCmdSdata_t sdata;
    cmpMac_t mac;
} cmpTltContLockBySpCmd_t;

/** TltContLockBySp command. */
typedef struct {
    cmpTltContLockBySpCmd_t cmd;
    mcSoTltCont_t soTltCont;
} cmpCmdTltContLockBySp_t;

/** @} */ 

/** @defgroup MC_CMP_CMD_TLT_CONT_LOCK_BY_SP_RSP Response
 * @{ */

typedef struct {
    mcSoTltCont_t soTltCont;
} cmpTltLockBySpRspEdata_t;

typedef struct {
    cmpTltLockBySpRspEdata_t edata;
    uint8_t padding[CMP_ED_PADDING(sizeof(cmpTltLockBySpRspEdata_t))];
} cmpTltLockBySpRspEd_t;

typedef struct {
    cmpResponseHeader_t rspHeader;
    cmpTltLockBySpRspEd_t ed;
} cmpTltLockBySpRspSdata_t;

typedef struct {
    cmpTltLockBySpRspSdata_t sdata;
    cmpMac_t mac;
} cmpTltContLockBySpRsp_t;

/** TltContLockBySp response. */
typedef struct {
    cmpTltContLockBySpRsp_t rsp;
    mcSoTltCont_t soTltCont;
} cmpRspTltContLockBySp_t;

/** @} */ 

/** @} */ 

/** @defgroup MC_CMP_CMD_TLT_CONT_UNLOCK_BY_SP 
 * @{ */

/** @defgroup MC_CMP_CMD_TLT_CONT_UNLOCK_BY_SP_CMD Command
 * @{ */

typedef struct {
    cmpCommandHeader_t cmdHeader;
    mcSpid_t spid;
    mcUuid_t uuid;
} cmpTltUnlockBySpCmdSdata_t;

typedef struct {
    cmpTltUnlockBySpCmdSdata_t sdata;
    cmpMac_t mac;
} cmpTltContUnlockBySpCmd_t;

/** TltContUnlockBySp command. */
typedef struct {
    cmpTltContUnlockBySpCmd_t cmd;
    mcSoTltCont_t soTltCont;
} cmpCmdTltContUnlockBySp_t;

/** @} */ 

/** @defgroup MC_CMP_CMD_TLT_CONT_UNLOCK_BY_SP_RSP Response
 * @{ */

typedef struct {
    mcSoTltCont_t soTltCont;
} cmpTltUnlockBySpRspEdata_t;

typedef struct {
    cmpTltUnlockBySpRspEdata_t edata;
    uint8_t padding[CMP_ED_PADDING(sizeof(cmpTltUnlockBySpRspEdata_t))];
} cmpTltUnlockBySpRspEd_t;

typedef struct {
    cmpResponseHeader_t rspHeader;
    cmpTltUnlockBySpRspEd_t ed;
} cmpTltUnlockBySpRspSdata_t;

typedef struct {
    cmpTltUnlockBySpRspSdata_t sdata;
    cmpMac_t mac;
} cmpTltContUnlockBySpRsp_t;

/** TltContUnlockBySp response. */
typedef struct {
    cmpTltContUnlockBySpRsp_t rsp;
    mcSoTltCont_t soTltCont;
} cmpRspTltContUnlockBySp_t;

/** @} */

/** @} */ 

/** @defgroup MC_CMP_CMD_GET_SUID
* @{ */

/** @defgroup MC_CMP_CMD_GET_SUID_CMD Command
* @{ */

/** GetSuid command. */
typedef struct {
    cmpCommandHeader_t cmdHeader;
} cmpCmdGetSuid_t;

/** @} */ 

/** @defgroup MC_CMP_CMD_GET_SUID_RSP Response
* @{ */

/** GetSuid response. */
typedef struct {
cmpResponseHeader_t rspHeader;
    mcSuid_t suid;
} cmpRspGetSuid_t;

/** @} */

/** @} */ 

/** @defgroup MC_CMP_CMD_AUTHENTICATE_TERMINATE
* @{ */

/** @defgroup MC_CMP_CMD_AUTHENTICATE_TERMINATE_CMD Command
* @{ */

typedef struct {
    cmpCommandHeader_t cmdHeader;
} cmpAuthenticateTerminateCmdSdata_t;

typedef struct {
    cmpAuthenticateTerminateCmdSdata_t sdata;
    cmpMac_t mac;
} cmpAuthenticateTerminateCmd_t;

/** AuthenticateTerminate command. */
typedef struct {
    cmpAuthenticateTerminateCmd_t cmd;
} cmpCmdAuthenticateTerminate_t;

/** @} */ 

/** @defgroup MC_CMP_CMD_AUTHENTICATE_TERMINATE_RSP Response
* @{ */

typedef struct {
    cmpResponseHeader_t rspHeader;
} cmpAuthenticateTerminateRspSdata_t;

typedef struct {
    cmpAuthenticateTerminateRspSdata_t sdata;
    cmpMac_t mac;
} cmpTerminateAutenticateRsp_t;

/** AuthenticateTerminate response. */
typedef struct {
    cmpTerminateAutenticateRsp_t rsp;
} cmpRspAuthenticateTerminate_t;

/** @} */

/** @} */ 

/** @defgroup MC_CMP_CMD_TLT_CONT_PERSONALIZE
 * @{ */
 
/** @defgroup MC_CMP_CMD_TLT_CONT_PERSONALIZE_CMD Command
 * @{ */
 
typedef struct {
    mcPid_t pid;
    mcCoDataCont_t persoData;
} cmpTltContPersonalizeCmdEdata_t;

typedef struct {
    cmpTltContPersonalizeCmdEdata_t edata;
    uint8_t padding_[CMP_ED_PADDING(sizeof(cmpTltContPersonalizeCmdEdata_t))];
} cmpTltContPersonalizeCmdEd_t;

typedef struct {
    cmpCommandHeader_t cmdHeader;
    mcSpid_t spid;
    mcUuid_t uuid;
    uint32_t edLen;
    cmpTltContPersonalizeCmdEd_t ed;
} cmpTltContPersonalizeCmdSdata_t;

typedef struct {
    cmpTltContPersonalizeCmdSdata_t sdata;
    cmpMac_t mac_;
} cmpTltContPersonalizeCmd_t;

/** TltContPersonalize command. */
typedef struct {
    cmpTltContPersonalizeCmd_t cmd;
    mcSoTltCont_t soTltCont_;
} cmpCmdTltContPersonalize_t;

/** @} */ 

/** @defgroup MC_CMP_CMD_TLT_CONT_PERSONLIZE_RSP Response
 * @{ */
 
typedef struct {
    mcSoDataCont_t soDataCont;
} cmpTltContPersonalizeRspEdata_t;

typedef struct {
    cmpTltContPersonalizeRspEdata_t edata;
    uint8_t padding_[CMP_ED_PADDING(sizeof(cmpTltContPersonalizeRspEdata_t))];
} cmpTltContPersonalizeRspEd_t;

typedef struct {
    cmpResponseHeader_t rspHeader;
    uint32_t edLen;
    cmpTltContPersonalizeRspEd_t ed;
} cmpTltContPersonalizeRspSdata_t;

typedef struct {
    cmpTltContPersonalizeRspSdata_t sdata;
    cmpMac_t mac_;
} cmpTltContPersonalizeRsp_t;

/** TltContPersonalize response. */
typedef struct {
    cmpTltContPersonalizeRsp_t rsp;
    mcSoDataCont_t soDataCont_;
} cmpRspTltContPersonalize_t;


/** @} */

/** @} */


#endif // CMP_H_

/** @} */

