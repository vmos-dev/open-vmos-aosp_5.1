/**
 * Copyright(c) 2011 Trusted Logic.   All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name Trusted Logic nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __SCHANNEL6_PROTOCOL_H__
#define __SCHANNEL6_PROTOCOL_H__

#include "s_type.h"

/**
 * This header file defines some structures needed for the secure channel
 * protocol. See your Product Reference Manual for a specification of the
 * SChannel protocol.
 */
// jroux to do : remove
#undef SMC_PROTOCOL_VERSION
#define SMC_PROTOCOL_VERSION 0x06000000

/**
 * Time representation.
 */
typedef uint64_t SCTIME;

#define SCTIME_IMMEDIATE ((uint64_t) 0x0000000000000000ULL)
#define SCTIME_INFINITE  ((uint64_t) 0xFFFFFFFFFFFFFFFFULL)

/*
 * Message types
 */
#define SCX_CREATE_DEVICE_CONTEXT   0x02
#define SCX_DESTROY_DEVICE_CONTEXT  0xFD
#define SCX_REGISTER_SHARED_MEMORY  0xF7
#define SCX_RELEASE_SHARED_MEMORY   0xF9
#define SCX_OPEN_CLIENT_SESSION     0xF0
#define SCX_CLOSE_CLIENT_SESSION    0xF2
#define SCX_INVOKE_CLIENT_COMMAND   0xF5
#define SCX_CANCEL_CLIENT_OPERATION 0xF4
#define SCX_MANAGEMENT              0xFE

/*
 * Shared mem flags
 */
#define SCX_SHARED_MEM_FLAG_INPUT   1
#define SCX_SHARED_MEM_FLAG_OUTPUT  2
#define SCX_SHARED_MEM_FLAG_INOUT   3

/*
 * Parameter types
 */
#define SCX_PARAM_TYPE_NONE                     0x0
#define SCX_PARAM_TYPE_VALUE_INPUT              0x1
#define SCX_PARAM_TYPE_VALUE_OUTPUT             0x2
#define SCX_PARAM_TYPE_VALUE_INOUT              0x3
#define SCX_PARAM_TYPE_MEMREF_TEMP_INPUT        0x5
#define SCX_PARAM_TYPE_MEMREF_TEMP_OUTPUT       0x6
#define SCX_PARAM_TYPE_MEMREF_TEMP_INOUT        0x7
#define SCX_PARAM_TYPE_MEMREF_INPUT             0xD
#define SCX_PARAM_TYPE_MEMREF_OUTPUT            0xE
#define SCX_PARAM_TYPE_MEMREF_INOUT             0xF

#define SCX_PARAM_TYPE_INPUT_FLAG                0x1
#define SCX_PARAM_TYPE_OUTPUT_FLAG               0x2
#define SCX_PARAM_TYPE_MEMREF_FLAG               0x4
#define SCX_PARAM_TYPE_REGISTERED_MEMREF_FLAG    0x8

#define SCX_PARAM_TYPE_IS_TMPREF(nParamType) (((nParamType) & (SCX_PARAM_TYPE_MEMREF_FLAG | SCX_PARAM_TYPE_REGISTERED_MEMREF_FLAG)) == SCX_PARAM_TYPE_MEMREF_FLAG)

#define SCX_MAKE_PARAM_TYPES(t0, t1, t2, t3) ((t0) | ((t1) << 4) | ((t2) << 8) | ((t3) << 12))
#define SCX_GET_PARAM_TYPE(t, i) (((t) >> (4*i)) & 0xF)

/*
 * return origins
 */
#define SCX_ORIGIN_COMMS       2
#define SCX_ORIGIN_TEE         3
#define SCX_ORIGIN_TRUSTED_APP 4

/*
 * Login types
 */
#include "schannel6_logins.h"

/**
 * Command parameters.
 */
typedef struct
{
   uint32_t    a;
   uint32_t    b;
}SCHANNEL6_COMMAND_PARAM_VALUE;

typedef struct
{
   uint32_t    nDescriptor;
   uint32_t    nSize;
   uint32_t    nOffset;     /* Socket: 4 weak bits of the address (for alignement checks) */

}SCHANNEL6_COMMAND_PARAM_TEMP_MEMREF;

typedef struct
{
   S_HANDLE    hBlock;
   uint32_t    nSize;
   uint32_t    nOffset;

}SCHANNEL6_COMMAND_PARAM_MEMREF;

typedef union
{
   SCHANNEL6_COMMAND_PARAM_VALUE        sValue;
   SCHANNEL6_COMMAND_PARAM_TEMP_MEMREF  sTempMemref;
   SCHANNEL6_COMMAND_PARAM_MEMREF       sMemref;

} SCHANNEL6_COMMAND_PARAM;

typedef struct
{
   uint32_t a;
   uint32_t b;
} SCHANNEL6_ANSWER_PARAM_VALUE;

typedef struct
{
   uint32_t _ignored;
   uint32_t nSize;
} SCHANNEL6_ANSWER_PARAM_SIZE;

typedef union
{
   SCHANNEL6_ANSWER_PARAM_SIZE  sSize;
   SCHANNEL6_ANSWER_PARAM_VALUE sValue;
} SCHANNEL6_ANSWER_PARAM;

/**
 * Command messages.
 */
 typedef struct
{
   uint8_t                       nMessageSize;
   uint8_t                       nMessageType;
   uint16_t                      nMessageInfo;
   uint32_t                      nOperationID; /* an opaque Normal World identifier for the operation */
}SCHANNEL6_COMMAND_HEADER;

typedef struct
{
   uint8_t                       nMessageSize;
   uint8_t                       nMessageType;
   uint16_t                      nMessageInfo_RFU;
   uint32_t                      nOperationID; /* an opaque Normal World identifier for the operation */
   uint32_t                      nDeviceContextID; /* an opaque Normal World identifier for the device context */
}SCHANNEL6_CREATE_DEVICE_CONTEXT_COMMAND;

typedef struct
{
   uint8_t                       nMessageSize;
   uint8_t                       nMessageType;
   uint16_t                      nParamTypes;
   uint32_t                      nOperationID; /* an opaque Normal World identifier for the operation */
   S_HANDLE                      hDeviceContext;
   S_HANDLE                      hClientSession;
   uint64_t                      sTimeout;
   uint32_t                      nCancellationID;
   uint32_t                      nClientCommandIdentifier;
   SCHANNEL6_COMMAND_PARAM       sParams[4];
}SCHANNEL6_INVOKE_CLIENT_COMMAND_COMMAND;

typedef struct
{
   uint8_t                       nMessageSize;
   uint8_t                       nMessageType;
   uint16_t                      nParamTypes;
   uint32_t                      nOperationID; /* an opaque Normal World identifier for the operation */
   S_HANDLE                      hDeviceContext;
   uint32_t                      nCancellationID;
   SCTIME                        sTimeout;
   S_UUID                        sDestinationUUID;
   SCHANNEL6_COMMAND_PARAM       sParams[4];
   uint32_t                      nLoginType;
   uint8_t                       sLoginData[20]; /* Size depends on the login type. */

}SCHANNEL6_OPEN_CLIENT_SESSION_COMMAND;

typedef struct
{
   uint8_t                       nMessageSize;
   uint8_t                       nMessageType;
   uint16_t                      nMemoryFlags;
   uint32_t                      nOperationID; /* an opaque Normal World identifier for the operation */
   S_HANDLE                      hDeviceContext;
   uint32_t                      nBlockID;
   uint32_t                      nSharedMemSize;
   uint32_t                      nSharedMemStartOffset;
   uint32_t                      nSharedMemDescriptors[8];

}SCHANNEL6_REGISTER_SHARED_MEMORY_COMMAND;

typedef struct
{
   uint8_t                       nMessageSize;
   uint8_t                       nMessageType;
   uint16_t                      nMessageInfo_RFU;
   uint32_t                      nOperationID; /* an opaque Normal World identifier for the operation */
   S_HANDLE                      hDeviceContext;
   S_HANDLE                      hBlock;

}SCHANNEL6_RELEASE_SHARED_MEMORY_COMMAND;

typedef struct
{
   uint8_t                       nMessageSize;
   uint8_t                       nMessageType;
   uint16_t                      nMessageInfo_RFU;
   uint32_t                      nOperationID; /* an opaque Normal World identifier for the operation */
   S_HANDLE                      hDeviceContext;
   S_HANDLE                      hClientSession;
   uint32_t                      nCancellationID;

}SCHANNEL6_CANCEL_CLIENT_OPERATION_COMMAND;

typedef struct
{
   uint8_t                       nMessageSize;
   uint8_t                       nMessageType;
   uint16_t                      nMessageInfo_RFU;
   uint32_t                      nOperationID; /* an opaque Normal World identifier for the operation */
   S_HANDLE                      hDeviceContext;
   S_HANDLE                      hClientSession;

}SCHANNEL6_CLOSE_CLIENT_SESSION_COMMAND;

typedef struct
{
   uint8_t                       nMessageSize;
   uint8_t                       nMessageType;
   uint16_t                      nMessageInfo_RFU;
   uint32_t                      nOperationID; /* an opaque Normal World identifier for the operation */
   S_HANDLE                      hDeviceContext;

}SCHANNEL6_DESTROY_DEVICE_CONTEXT_COMMAND;

#define SCHANNEL6_MANAGEMENT_COMMAND_HIBERNATE            1
#define SCHANNEL6_MANAGEMENT_COMMAND_SHUTDOWN             2
#define SCHANNEL6_MANAGEMENT_COMMAND_PREPARE_FOR_CORE_OFF 3
#define SCHANNEL6_MANAGEMENT_COMMAND_RESUME_FROM_CORE_OFF 4

typedef struct
{
   uint8_t                       nMessageSize;
   uint8_t                       nMessageType;
   uint16_t                      nCommand;
   uint32_t                      nOperationID; /* an opaque Normal World identifier for the operation */
   uint32_t                      nW3BSize;
   uint32_t                      nW3BStartOffset;
#ifdef SCHANNEL_TRUSTZONE
   uint32_t                      nSharedMemDescriptors[128];
#endif
}SCHANNEL6_MANAGEMENT_COMMAND;

typedef union
{
   SCHANNEL6_COMMAND_HEADER                            sHeader;
   SCHANNEL6_CREATE_DEVICE_CONTEXT_COMMAND             sCreateDeviceContext;
   SCHANNEL6_DESTROY_DEVICE_CONTEXT_COMMAND            sDestroyDeviceContext;
   SCHANNEL6_OPEN_CLIENT_SESSION_COMMAND               sOpenClientSession;
   SCHANNEL6_CLOSE_CLIENT_SESSION_COMMAND              sCloseClientSession;
   SCHANNEL6_REGISTER_SHARED_MEMORY_COMMAND            sRegisterSharedMemory;
   SCHANNEL6_RELEASE_SHARED_MEMORY_COMMAND             sReleaseSharedMemory;
   SCHANNEL6_INVOKE_CLIENT_COMMAND_COMMAND             sInvokeClientCommand;
   SCHANNEL6_CANCEL_CLIENT_OPERATION_COMMAND           sCancelClientOperation;
   SCHANNEL6_MANAGEMENT_COMMAND                        sManagement;

}SCHANNEL6_COMMAND;

/**
 * Answer messages.
 */
typedef struct
{
   uint8_t                   nMessageSize;
   uint8_t                   nMessageType;
   uint16_t                  nMessageInfo;
   uint32_t                  nOperationID;
   uint32_t                  nErrorCode;
}SCHANNEL6_ANSWER_HEADER;

typedef struct
{
   uint8_t                   nMessageSize;
   uint8_t                   nMessageType;
   uint16_t                  nMessageInfo_RFU;
   uint32_t                  nOperationID;
   uint32_t                  nErrorCode;
   S_HANDLE                  hDeviceContext;
}SCHANNEL6_CREATE_DEVICE_CONTEXT_ANSWER;

typedef struct
{
   uint8_t                   nMessageSize;
   uint8_t                   nMessageType;
   uint8_t                   nReturnOrigin;
   uint8_t                   __nReserved;
   uint32_t                  nOperationID;
   uint32_t                  nErrorCode;
   SCHANNEL6_ANSWER_PARAM    sAnswers[4];

}SCHANNEL6_INVOKE_CLIENT_COMMAND_ANSWER;

typedef struct
{
   uint8_t                   nMessageSize;
   uint8_t                   nMessageType;
   uint8_t                   nReturnOrigin;
   uint8_t                   __nReserved;
   uint32_t                  nOperationID;
   uint32_t                  nErrorCode;
   S_HANDLE                  hClientSession;
   SCHANNEL6_ANSWER_PARAM    sAnswers[4];
}SCHANNEL6_OPEN_CLIENT_SESSION_ANSWER;

typedef struct
{
   uint8_t                   nMessageSize;
   uint8_t                   nMessageType;
   uint16_t                  nMessageInfo_RFU;
   uint32_t                  nOperationID;
   uint32_t                  nErrorCode;
}SCHANNEL6_CLOSE_CLIENT_SESSION_ANSWER;

typedef struct
{
   uint8_t                   nMessageSize;
   uint8_t                   nMessageType;
   uint16_t                  nMessageInfo_RFU;
   uint32_t                  nOperationID;
   uint32_t                  nErrorCode;
   S_HANDLE                  hBlock;

}SCHANNEL6_REGISTER_SHARED_MEMORY_ANSWER;

typedef struct
{
   uint8_t                   nMessageSize;
   uint8_t                   nMessageType;
   uint16_t                  nMessageInfo_RFU;
   uint32_t                  nOperationID;
   uint32_t                  nErrorCode;
   uint32_t                  nBlockID;

}SCHANNEL6_RELEASE_SHARED_MEMORY_ANSWER;

typedef struct
{
   uint8_t                   nMessageSize;
   uint8_t                   nMessageType;
   uint16_t                  nMessageInfo_RFU;
   uint32_t                  nOperationID;
   uint32_t                  nErrorCode;
   uint32_t                  nDeviceContextID;

}SCHANNEL6_DESTROY_DEVICE_CONTEXT_ANSWER;

typedef struct
{
   uint8_t                   nMessageSize;
   uint8_t                   nMessageType;
   uint16_t                  nMessageInfo_RFU;
   uint32_t                  nOperationID;
   uint32_t                  nErrorCode;

}SCHANNEL6_CANCEL_CLIENT_OPERATION_ANSWER;

typedef struct
{
   uint8_t                   nMessageSize;
   uint8_t                   nMessageType;
   uint16_t                  nMessageInfo_RFU;
   uint32_t                  nOperationID;
   uint32_t                  nErrorCode;

}SCHANNEL6_MANAGEMENT_ANSWER;

typedef union
{
   SCHANNEL6_ANSWER_HEADER                    sHeader;
   SCHANNEL6_CREATE_DEVICE_CONTEXT_ANSWER     sCreateDeviceContext;
   SCHANNEL6_OPEN_CLIENT_SESSION_ANSWER       sOpenClientSession;
   SCHANNEL6_REGISTER_SHARED_MEMORY_ANSWER    sRegisterSharedMemory;
   SCHANNEL6_RELEASE_SHARED_MEMORY_ANSWER     sReleaseSharedMemory;
   SCHANNEL6_INVOKE_CLIENT_COMMAND_ANSWER     sInvokeClientCommand;
   SCHANNEL6_DESTROY_DEVICE_CONTEXT_ANSWER    sDestroyDeviceContext;
   SCHANNEL6_CANCEL_CLIENT_OPERATION_ANSWER   sCancelClientOperation;
   SCHANNEL6_CLOSE_CLIENT_SESSION_ANSWER      sCloseClientSession;
   SCHANNEL6_MANAGEMENT_ANSWER                sManagement;

}SCHANNEL6_ANSWER;


#endif /* __SCHANNEL6_PROTOCOL_H__ */
