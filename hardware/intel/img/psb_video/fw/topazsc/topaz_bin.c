/*
 * Copyright (c) 2011 Intel Corporation. All Rights Reserved.
 * Copyright (c) Imagination Technologies Limited, UK
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Zeng Li <zeng.li@intel.com>
 *
 */

#include <stdio.h>

#include "H263Firmware_bin.h"
#include "H263FirmwareCBR_bin.h"
#include "H263FirmwareVBR_bin.h"
#include "H264Firmware_bin.h"
#include "H264FirmwareCBR_bin.h"
#include "H264FirmwareVBR_bin.h"
#include "H264FirmwareVCM_bin.h"
#include "MPG4Firmware_bin.h"
#include "MPG4FirmwareCBR_bin.h"
#include "MPG4FirmwareVBR_bin.h"

#define FW_VER 146 /* DDKv146 release */
#define FW_FILE_NAME "topaz_fw.bin"

struct topaz_fw_info_item_s {
    unsigned short ver;
    unsigned short codec;

    unsigned int  text_size;
    unsigned int data_size;
    unsigned int data_location;
};
typedef struct topaz_fw_info_item_s topaz_fw_info_item_t;

enum topaz_fw_codec_e {
    FW_JPEG = 0,
    FW_H264_NO_RC,
    FW_H264_VBR,
    FW_H264_CBR,
    FW_H264_VCM,
    FW_H263_NO_RC,
    FW_H263_VBR,
    FW_H263_CBR,
    FW_MPEG4_NO_RC,
    FW_MPEG4_VBR,
    FW_MPEG4_CBR,
    FW_NUM
};
typedef enum topaz_fw_codec_e topaz_fw_codec_t;

struct fw_table_s {
    topaz_fw_codec_t index;
    topaz_fw_info_item_t header;
    unsigned int *fw_text;
    unsigned int *fw_data;
};
typedef struct fw_table_s fw_table_t;

int main()
{
    FILE *fp = NULL;
    topaz_fw_codec_t iter = FW_H264_NO_RC;
    unsigned int size = 0;
    unsigned int i;

    fw_table_t topaz_fw_table[] = {
        /* index   header
         * { ver, codec, text_size, data_size, date_location }
         * fw_text fw_data */
        { 0, {0, 0, 0, 0, 0}, NULL, NULL },
        {
            FW_H264_NO_RC,
            {
                FW_VER,
                FW_H264_NO_RC,
                ui32H264_MTXTOPAZFWTextSize,
                ui32H264_MTXTOPAZFWDataSize,
                ui32H264_MTXTOPAZFWDataLocation
            },
            aui32H264_MTXTOPAZFWText, aui32H264_MTXTOPAZFWData
        },

        {
            FW_H264_VBR,
            {
                FW_VER,
                FW_H264_VBR,
                ui32H264VBR_MTXTOPAZFWTextSize,
                ui32H264VBR_MTXTOPAZFWDataSize,
                ui32H264VBR_MTXTOPAZFWDataLocation
            },
            aui32H264VBR_MTXTOPAZFWText, aui32H264VBR_MTXTOPAZFWData
        },

        {
            FW_H264_CBR,
            {
                FW_VER,
                FW_H264_CBR,
                ui32H264CBR_MTXTOPAZFWTextSize,
                ui32H264CBR_MTXTOPAZFWDataSize,
                ui32H264CBR_MTXTOPAZFWDataLocation
            },
            aui32H264CBR_MTXTOPAZFWText,
            aui32H264CBR_MTXTOPAZFWData
        },

        {
            FW_H264_VCM,
            {
                FW_VER,
                FW_H264_VCM,
                ui32H264VCM_MTXTOPAZFWTextSize,
                ui32H264VCM_MTXTOPAZFWDataSize,
                ui32H264VCM_MTXTOPAZFWDataLocation
            },
            aui32H264VCM_MTXTOPAZFWText,
            aui32H264VCM_MTXTOPAZFWData
        },

        {
            FW_H263_NO_RC,
            {
                FW_VER,
                FW_H263_NO_RC,
                ui32H263_MTXTOPAZFWTextSize,
                ui32H263_MTXTOPAZFWDataSize,
                ui32H263_MTXTOPAZFWDataLocation
            },
            aui32H263_MTXTOPAZFWText,
            aui32H263_MTXTOPAZFWData
        },

        {
            FW_H263_VBR,
            {
                FW_VER,
                FW_H263_VBR,
                ui32H263VBR_MTXTOPAZFWTextSize,
                ui32H263VBR_MTXTOPAZFWDataSize,
                ui32H263VBR_MTXTOPAZFWDataLocation
            },
            aui32H263VBR_MTXTOPAZFWText,
            aui32H263VBR_MTXTOPAZFWData
        },

        {
            FW_H263_CBR,
            {
                FW_VER,
                FW_H263_CBR,
                ui32H263CBR_MTXTOPAZFWTextSize,
                ui32H263CBR_MTXTOPAZFWDataSize,
                ui32H263CBR_MTXTOPAZFWDataLocation
            },
            aui32H263CBR_MTXTOPAZFWText,
            aui32H263CBR_MTXTOPAZFWData
        },

        {
            FW_MPEG4_NO_RC,
            {
                FW_VER,
                FW_MPEG4_NO_RC,
                ui32MPG4_MTXTOPAZFWTextSize,
                ui32MPG4_MTXTOPAZFWDataSize,
                ui32MPG4_MTXTOPAZFWDataLocation
            },
            aui32MPG4_MTXTOPAZFWText,
            aui32MPG4_MTXTOPAZFWData
        },

        {
            FW_MPEG4_VBR,
            {
                FW_VER,
                FW_MPEG4_VBR,
                ui32MPG4VBR_MTXTOPAZFWTextSize,
                ui32MPG4VBR_MTXTOPAZFWDataSize,
                ui32MPG4VBR_MTXTOPAZFWDataLocation
            },
            aui32MPG4VBR_MTXTOPAZFWText,
            aui32MPG4VBR_MTXTOPAZFWData
        },

        {
            FW_MPEG4_CBR,
            {
                FW_VER,
                FW_MPEG4_CBR,
                ui32MPG4CBR_MTXTOPAZFWTextSize,
                ui32MPG4CBR_MTXTOPAZFWDataSize,
                ui32MPG4CBR_MTXTOPAZFWDataLocation
            },
            aui32MPG4CBR_MTXTOPAZFWText,
            aui32MPG4CBR_MTXTOPAZFWData
        }
    };

    /* open file  */
    fp = fopen(FW_FILE_NAME, "w");

    if (NULL == fp)
        return -1;
    /* write fw table into the file */
    while (iter < FW_NUM) {
        /* record the size use bytes */
        topaz_fw_table[iter].header.data_size *= 4;
        topaz_fw_table[iter].header.text_size *= 4;

        /* write header */
        fwrite(&(topaz_fw_table[iter].header), sizeof(topaz_fw_table[iter].header), 1, fp);

        /* write text */
        size = topaz_fw_table[iter].header.text_size;
        fwrite(topaz_fw_table[iter].fw_text, 1, size, fp);

        /* write data */
        size = topaz_fw_table[iter].header.data_size;
        fwrite(topaz_fw_table[iter].fw_data, 1, size, fp);

        ++iter;
    }

    /* close file */
    fclose(fp);

    return 0;
}
