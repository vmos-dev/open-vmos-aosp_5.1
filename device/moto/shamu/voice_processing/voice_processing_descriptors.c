/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <hardware/audio_effect.h>
//------------------------------------------------------------------------------
// Effect descriptors
//------------------------------------------------------------------------------

// UUIDs for effect types have been generated from http://www.itu.int/ITU-T/asn1/uuid.html
// as the pre processing effects are not defined by OpenSL ES

// Acoustic Echo Cancellation 3770d8a0-4a7c-11e4-ac15-0002a5d5c51b
const effect_descriptor_t qcom_product_aec_descriptor = {
        { 0x7b491460, 0x8d4d, 0x11e0, 0xbd61, { 0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b } }, // type
        { 0x3770d8a0, 0x4a7c, 0x11e4, 0xac15, { 0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b } }, // uuid
        EFFECT_CONTROL_API_VERSION,
        (EFFECT_FLAG_TYPE_PRE_PROC|EFFECT_FLAG_DEVICE_IND),
        0,
        0,
        "Acoustic Echo Canceler",
        "Qualcomm Fluence for Shamu"
};

// Noise suppression 5a96f440-4a7c-11e4-8477-0002a5d5c51b
const effect_descriptor_t qcom_product_ns_descriptor = {
        { 0x58b4b260, 0x8e06, 0x11e0, 0xaa8e, { 0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b } }, // type
        { 0x5a96f440, 0x4a7c, 0x11e4, 0x8477, { 0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b } }, // uuid
        EFFECT_CONTROL_API_VERSION,
        (EFFECT_FLAG_TYPE_PRE_PROC|EFFECT_FLAG_DEVICE_IND),
        0,
        0,
        "Noise Suppression",
        "Qualcomm Fluence for Shamu"
};

//ENABLE_AGC
// Automatic Gain Control e6cdbac0-4a7c-11e4-a018-0002a5d5c51b
//const effect_descriptor_t qcom_product_agc_descriptor = {
//        { 0x0a8abfe0, 0x654c, 0x11e0, 0xba26, { 0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b } }, // type
//        { 0xe6cdbac0, 0x4a7c, 0x11e4, 0xa018, { 0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b } }, // uuid
//        EFFECT_CONTROL_API_VERSION,
//        (EFFECT_FLAG_TYPE_PRE_PROC|EFFECT_FLAG_DEVICE_IND),
//        0,
//        0,
//        "Automatic Gain Control",
//        "Qualcomm Fluence for Shamu"
//};

