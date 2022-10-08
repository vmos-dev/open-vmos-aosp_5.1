/*
 * Copyright (C) 2008 The Android Open Source Project
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

#ifndef _ASEC_H
#define _ASEC_H

struct asec_superblock {
#define ASEC_SB_MAGIC 0xc0def00d
    unsigned int magic;

#define ASEC_SB_VER 1
    unsigned char ver;

#define ASEC_SB_C_CIPHER_NONE    0
#define ASEC_SB_C_CIPHER_TWOFISH 1
#define ASEC_SB_C_CIPHER_AES     2
    unsigned char c_cipher;

#define ASEC_SB_C_CHAIN_NONE 0
    unsigned char c_chain;

#define ASEC_SB_C_OPTS_NONE 0
#define ASEC_SB_C_OPTS_EXT4 1
    unsigned char c_opts;

#define ASEC_SB_C_MODE_NONE 0
    unsigned char c_mode;
} __attribute__((packed));

#endif
