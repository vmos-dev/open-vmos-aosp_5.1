/**
 * @file   tci.h
 * @brief  Contains TCI (Trustlet Control
 * Interface) definitions and data structures
 *
 * Copyright Giesecke & Devrient GmbH 2012
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

#ifndef __TCI_H__
#define __TCI_H__


typedef uint32_t tciCommandId_t;
typedef uint32_t tciResponseId_t;
typedef uint32_t tciReturnCode_t;


/**< Responses have bit 31 set */
#define RSP_ID_MASK (1U << 31)
#define RSP_ID(cmdId) (((uint32_t)(cmdId)) | RSP_ID_MASK)
#define IS_CMD(cmdId) ((((uint32_t)(cmdId)) & RSP_ID_MASK) == 0)
#define IS_RSP(cmdId) ((((uint32_t)(cmdId)) & RSP_ID_MASK) == RSP_ID_MASK)


/**
 * Return codes
 */
#define RET_OK                    0
#define RET_ERR_UNKNOWN_CMD       1
#define RET_ERR_NOT_SUPPORTED     2
#define RET_ERR_INVALID_BUFFER    3
#define RET_ERR_INVALID_KEY_SIZE  4
#define RET_ERR_INVALID_KEY_TYPE  5
#define RET_ERR_INVALID_LENGTH    6
#define RET_ERR_INVALID_EXPONENT  7
#define RET_ERR_KEY_GENERATION    8
#define RET_ERR_SIGN              9
#define RET_ERR_VERIFY            10
#define RET_ERR_DIGEST            11
#define RET_ERR_SECURE_OBJECT     12
#define RET_ERR_INTERNAL_ERROR    13
/* ... add more error codes when needed */


/**
 * TCI command header.
 */
typedef struct{
    tciCommandId_t commandId; /**< Command ID */
} tciCommandHeader_t;


/**
 * TCI response header.
 */
typedef struct{
    tciResponseId_t     responseId; /**< Response ID (must be command ID | RSP_ID_MASK )*/
    tciReturnCode_t     returnCode; /**< Return code of command */
} tciResponseHeader_t;

#endif // __TCI_H__
