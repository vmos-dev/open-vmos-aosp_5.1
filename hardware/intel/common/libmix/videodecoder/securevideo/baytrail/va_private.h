/*
* Copyright (c) 2009-2011 Intel Corporation.  All rights reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/


#ifndef __VA_PRIVATE_H__
#define __VA_PRIVATE_H__
#include <va/va.h>
#define ENABLE_PAVP_LINUX                   1
// Misc parameter for encoder
#define  VAEncMiscParameterTypePrivate     -2
// encryption parameters for PAVP
#define  VAEncryptionParameterBufferType   -3

typedef struct _VAEncMiscParameterPrivate
{
    unsigned int target_usage; // Valid values 1-7 for AVC & MPEG2.
    unsigned int reserved[7];  // Reserved for future use.
} VAEncMiscParameterPrivate;

/*VAEncrytpionParameterBuffer*/
typedef struct _VAEncryptionParameterBuffer
{
    //Not used currently
    unsigned int encryptionSupport;
    //Not used currently
    unsigned int hostEncryptMode;
    // For IV, Counter input
    unsigned int pavpAesCounter[2][4];
    // not used currently
    unsigned int pavpIndex;
    // PAVP mode, CTR, CBC, DEDE etc
    unsigned int pavpCounterMode;
    unsigned int pavpEncryptionType;
    // not used currently
    unsigned int pavpInputSize[2];
    // not used currently
    unsigned int pavpBufferSize[2];
    // not used currently
    VABufferID   pvap_buf;
    // set to TRUE if protected media
    unsigned int pavpHasBeenEnabled;
    // not used currently
    unsigned int IntermmediatedBufReq;
    // not used currently
    unsigned int uiCounterIncrement;
    // AppId: PAVP sessin Index from application
    unsigned int app_id;

} VAEncryptionParameterBuffer;

#endif
