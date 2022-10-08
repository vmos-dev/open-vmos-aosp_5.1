/*
 * INTEL CONFIDENTIAL
 * Copyright 2007 Intel Corporation. All Rights Reserved.
 * Copyright 2005-2007 Imagination Technologies Limited. All Rights Reserved.
 *
 * The source code contained or described herein and all documents related to
 * the source code ("Material") are owned by Intel Corporation or its suppliers
 * or licensors. Title to the Material remains with Intel Corporation or its
 * suppliers and licensors. The Material may contain trade secrets and
 * proprietary and confidential information of Intel Corporation and its
 * suppliers and licensors, and is protected by worldwide copyright and trade
 * secret laws and treaty provisions. No part of the Material may be used,
 * copied, reproduced, modified, published, uploaded, posted, transmitted,
 * distributed, or disclosed in any way without Intel's prior express written
 * permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery
 * of the Materials, either expressly, by implication, inducement, estoppel or
 * otherwise. Any license under such intellectual property rights must be
 * express and approved by Intel in writing.
 */


/*
 * Authors:
 *    Elaine Wang <zhaohan.ren@intel.com>
 *
 */

#include <stdio.h>

#include "JPEGMasterFirmware_bin.h"

#include "H264MasterFirmware_bin.h"
#include "H264MasterFirmwareCBR_bin.h"
#include "H264MasterFirmwareVBR_bin.h"
#include "H264MasterFirmwareVCM_bin.h"
#include "H264MasterFirmwareLLRC_bin.h"
#include "H264MasterFirmwareALL_bin.h"

#include "H263MasterFirmware_bin.h"
#include "H263MasterFirmwareCBR_bin.h"
#include "H263MasterFirmwareVBR_bin.h"
//#include "H263MasterFirmwareLLRC_bin.h"

#include "MPG2MasterFirmware_bin.h"
#include "MPG2MasterFirmwareCBR_bin.h"
#include "MPG2MasterFirmwareVBR_bin.h"
//#include "MPG2MasterFirmwareLLRC_bin.h"

#include "MPG4MasterFirmware_bin.h"
#include "MPG4MasterFirmwareCBR_bin.h"
#include "MPG4MasterFirmwareVBR_bin.h"
#include "MPG4MasterFirmwareLLRC_bin.h"

#include "H264MVCMasterFirmware_bin.h"
#include "H264MVCMasterFirmwareCBR_bin.h"
#include "H264MVCMasterFirmwareVBR_bin.h"
#include "H264MVCMasterFirmwareLLRC_bin.h"

#include "thread0_bin.h"

#include "Primary_VRL.txt"


#define FW_VER 0x5D
#define FW_FILE_NAME_A0 "topazhp_fw.bin"
#define FW_FILE_NAME_B0 "topazhp_fw_b0.bin"

static const unsigned char pad_val = 0x0;


#define FW_MASTER_INFO(codec,prefix) \
        { FW_MASTER_##codec,\
          { FW_VER,\
            FW_MASTER_##codec,\
            ui32##prefix##_MasterMTXTOPAZFWTextSize,\
            ui32##prefix##_MasterMTXTOPAZFWDataSize,\
            ui32##prefix##_MasterMTXTOPAZFWDataOrigin\
          },\
          aui32##prefix##_MasterMTXTOPAZFWText, aui32##prefix##_MasterMTXTOPAZFWData \
        }

#define FW_SLAVE_INFO(codec,prefix) \
        { FW_SLAVE_##codec,\
          { FW_VER,\
            FW_SLAVE_##codec,\
            ui32##prefix##_SlaveMTXTOPAZFWTextSize,\
            ui32##prefix##_SlaveMTXTOPAZFWDataSize,\
            ui32##prefix##_SlaveMTXTOPAZFWDataOrigin\
          },\
          aui32##prefix##_SlaveMTXTOPAZFWText, aui32##prefix##_SlaveMTXTOPAZFWData \
        }

struct topaz_fw_info_item_s {
    unsigned short ver;
    unsigned short codec;

    unsigned int text_size;
    unsigned int data_size;
    unsigned int data_location;
};

typedef struct topaz_fw_info_item_s topaz_fw_info_item_t;

/* Commented out all the SLAVE structure according to DDK */
enum topaz_fw_codec_e {
        FW_MASTER_JPEG = 0,                         //!< JPEG
        FW_MASTER_H264_NO_RC,           //!< H264 with no rate control
        FW_MASTER_H264_VBR,                     //!< H264 variable bitrate
        FW_MASTER_H264_CBR,                     //!< H264 constant bitrate
        FW_MASTER_H264_VCM,                     //!< H264 video conferance mode
        FW_MASTER_H264_LLRC,            //!< H264 low-latency rate control
        FW_MASTER_H264ALL,
        FW_MASTER_H263_NO_RC,           //!< H263 with no rate control
        FW_MASTER_H263_VBR,                     //!< H263 variable bitrate
        FW_MASTER_H263_CBR,                     //!< H263 constant bitrate
        FW_MASTER_MPEG4_NO_RC,          //!< MPEG4 with no rate control
        FW_MASTER_MPEG4_VBR,            //!< MPEG4 variable bitrate
        FW_MASTER_MPEG4_CBR,            //!< MPEG4 constant bitrate
        FW_MASTER_MPEG2_NO_RC,          //!< MPEG2 with no rate control
        FW_MASTER_MPEG2_VBR,            //!< MPEG2 variable bitrate
        FW_MASTER_MPEG2_CBR,            //!< MPEG2 constant bitrate
        FW_MASTER_H264MVC_NO_RC,        //!< MVC H264 with no rate control
        FW_MASTER_H264MVC_CBR,          //!< MVC H264 constant bitrate
        FW_MASTER_H264MVC_VBR,          //!< MVC H264 variable bitrate
        FW_MASTER_H264MVC_LLRC,         //!< MVC H264 low-latency rate control
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

#define SECURE_ALIGN 1
#define B0_ALIGN 4
#define SECURE_VRL_HEADER 728
#define SECURE_FIP_HEADER 296

struct fw_table_A0 {
    unsigned int addr;
    unsigned int text_size_bytes;
    unsigned int data_size_bytes;
    unsigned int data_relocation;
};

static void create_firmware_A0(fw_table_t *tng_fw_table, FILE *fp)
{
    struct fw_table_A0 sec_t[FW_NUM];
    const unsigned int ui_secure_align = SECURE_ALIGN - 1;
    unsigned int i = 0;
    int iter = 0;
    int size = 0;

    printf("fm num is %d\n", FW_NUM);
    /////////////////////////////////////////////////
    size = SECURE_VRL_HEADER + SECURE_FIP_HEADER;

    for (iter = 0; iter < size; iter++) {
        fwrite(&pad_val, 1, 1, fp);
    }
    /////////////////////////////////////////////////
    iter = 0;

    sec_t[iter].addr = FW_NUM * 16 + SECURE_VRL_HEADER + SECURE_FIP_HEADER;
    sec_t[iter].text_size_bytes = tng_fw_table[iter].header.text_size * 4;
    sec_t[iter].data_size_bytes = tng_fw_table[iter].header.data_size * 4;
    sec_t[iter].data_relocation = tng_fw_table[iter].header.data_location;
    fwrite(&(sec_t[iter]), sizeof(struct fw_table_A0), 1, fp);

    iter = 1;


    /* write fw table into the file */
    while (iter < FW_NUM) {
        sec_t[iter - 1].text_size_bytes = (sec_t[iter - 1].text_size_bytes + ui_secure_align) & (~ui_secure_align);
        sec_t[iter - 1].data_size_bytes = (sec_t[iter - 1].data_size_bytes + ui_secure_align) & (~ui_secure_align);

        sec_t[iter].addr = sec_t[iter-1].addr + sec_t[iter-1].text_size_bytes + sec_t[iter-1].data_size_bytes;

        sec_t[iter].text_size_bytes = tng_fw_table[iter].header.text_size * 4;
        sec_t[iter].data_size_bytes = tng_fw_table[iter].header.data_size * 4;
	sec_t[iter].data_relocation = tng_fw_table[iter].header.data_location;
        fwrite(&(sec_t[iter]), sizeof(struct fw_table_A0), 1, fp);
        ++iter;
    }

    sec_t[iter - 1].text_size_bytes = (sec_t[iter - 1].text_size_bytes + ui_secure_align) & (~ui_secure_align);
    sec_t[iter - 1].data_size_bytes = (sec_t[iter - 1].data_size_bytes + ui_secure_align) & (~ui_secure_align);

    iter = 0;
    while (iter < FW_NUM) {

        /* write text */
        size = tng_fw_table[iter].header.text_size * 4;
        fwrite(tng_fw_table[iter].fw_text, 1, size, fp);
        for (i = 0; i < (sec_t[iter].text_size_bytes - size); i++)
            fwrite(&pad_val, 1, 1, fp);


        /* write data */
        size = tng_fw_table[iter].header.data_size * 4;
        fwrite(tng_fw_table[iter].fw_data, 1, size, fp);
        for (i = 0; i < (sec_t[iter].data_size_bytes - size); i++)
            fwrite(&pad_val, 1, 1, fp);

        fflush(fp);

        ++iter;
    }
    return ;
}

struct fw_table_B0 {
    unsigned int addr;
    unsigned int text_size_dword;
    unsigned int data_size_dword;
    unsigned int text_size_bytes;
};

static const char FIP_Header[] = {
	0x24, 0x56, 0x45, 0x43,
	0x01, 0x00, 0x01, 0x00,
	0x4a, 0x00, 0x00, 0x00,
	0x8e, 0x00, 0x00, 0x00,
	0x22, 0x00, 0x9c, 0x00,
	0x0e, 0x00, 0x00, 0x00
};
static void create_firmware_B0(fw_table_t *tng_fw_table, FILE *fp)
{
    struct fw_table_B0 sec_t[FW_NUM];
    const unsigned int i_align = B0_ALIGN - 1;
    const unsigned int data_loco = 0x0ffff;
    unsigned int i = 0;
    int iter = 0;
    int size = 0;

    printf("fm num is %d\n", FW_NUM);
    /////////////////////////////////////////////////
    size = sizeof(FIP_Header);
    fwrite(FIP_Header, size, 1, fp);
    printf("FIP header is %d, 296\n", size);

    size = SECURE_FIP_HEADER - sizeof(FIP_Header);
    for (iter = 0; iter < size; iter++) {
        fwrite(&pad_val, 1, 1, fp);
    }
    /////////////////////////////////////////////////
    iter = 0;

    sec_t[iter].addr = FW_NUM * 16 + SECURE_VRL_HEADER + SECURE_FIP_HEADER;
    sec_t[iter].text_size_bytes = tng_fw_table[iter].header.data_location & data_loco;
    sec_t[iter].text_size_dword = sec_t[iter].text_size_bytes >> 2;

    sec_t[iter].data_size_dword = (tng_fw_table[iter].header.data_size + i_align) & (~i_align);
    fwrite(&(sec_t[iter]), sizeof(struct fw_table_B0), 1, fp);

    iter = 1;

    /* write fw table into the file */
    while (iter < FW_NUM) {
        sec_t[iter].addr = sec_t[iter-1].addr +
		sec_t[iter-1].text_size_bytes +
		(sec_t[iter-1].data_size_dword << 2);

        sec_t[iter].text_size_bytes = tng_fw_table[iter].header.data_location & data_loco;
        sec_t[iter].text_size_dword = sec_t[iter].text_size_bytes >> 2;

        sec_t[iter].data_size_dword = (tng_fw_table[iter].header.data_size + i_align) & (~i_align);
        fwrite(&(sec_t[iter]), sizeof(struct fw_table_B0), 1, fp);
        ++iter;
    }

    iter = 0;
    while (iter < FW_NUM) {

        /* write text */
        size = tng_fw_table[iter].header.text_size * 4;
        fwrite(tng_fw_table[iter].fw_text, 1, size, fp);
        for (i = 0; i < (sec_t[iter].text_size_bytes - size); i++)
            fwrite(&pad_val, 1, 1, fp);


        /* write data */
        size = tng_fw_table[iter].header.data_size * 4;
        fwrite(tng_fw_table[iter].fw_data, 1, size, fp);
        for (i = 0; i < ((sec_t[iter].data_size_dword << 2) - size); i++)
            fwrite(&pad_val, 1, 1, fp);

        fflush(fp);

        ++iter;
    }
    return ;
}

static void normal_firmware(fw_table_t *topaz_fw_table, FILE *fp)
{
    unsigned int i = 0;
    topaz_fw_codec_t iter = 0;
    int size = 0;

    printf("fm num is %d\n", FW_NUM);

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
}



int main()
{
    FILE *fp = NULL;

    fw_table_t topaz_fw_table[] = {
        FW_MASTER_INFO(JPEG, JPEG),	//FW_MASTER_JPEG = 0,                         //!< JPEG
        FW_MASTER_INFO(H264_NO_RC, H264),//FW_MASTER_H264_NO_RC,           //!< H264 with no rate control
        FW_MASTER_INFO(H264_VBR, H264VBR),//FW_MASTER_H264_VBR,                     //!< H264 variable bitrate
        FW_MASTER_INFO(H264_CBR, H264CBR),//FW_MASTER_H264_CBR,                     //!< H264 constant bitrate
        FW_MASTER_INFO(H264_VCM, H264VCM),//FW_MASTER_H264_VCM,                     //!< H264 video conferance mode
        FW_MASTER_INFO(H264_LLRC, H264LLRC),//FW_MASTER_H264_LLRC,            //!< H264 low-latency rate control
        FW_MASTER_INFO(H264ALL, H264ALL),
        FW_MASTER_INFO(H263_NO_RC, H263),//FW_MASTER_H263_NO_RC,           //!< H263 with no rate control
        FW_MASTER_INFO(H263_VBR, H263VBR),//FW_MASTER_H263_VBR,                     //!< H263 variable bitrate
        FW_MASTER_INFO(H263_CBR, H263CBR),//FW_MASTER_H263_CBR,                     //!< H263 constant bitrate
        FW_MASTER_INFO(MPEG4_NO_RC, MPG4),//FW_MASTER_MPEG4_NO_RC,          //!< MPEG4 with no rate control
        FW_MASTER_INFO(MPEG4_VBR, MPG4VBR),//FW_MASTER_MPEG4_VBR,            //!< MPEG4 variable bitrate
        FW_MASTER_INFO(MPEG4_CBR, MPG4CBR),//FW_MASTER_MPEG4_CBR,            //!< MPEG4 constant bitrate
        FW_MASTER_INFO(MPEG2_NO_RC, MPG2),//FW_MASTER_MPEG2_NO_RC,          //!< MPEG2 with no rate control
        FW_MASTER_INFO(MPEG2_VBR, MPG2VBR),//FW_MASTER_MPEG2_VBR,            //!< MPEG2 variable bitrate
        FW_MASTER_INFO(MPEG2_CBR, MPG2CBR),//FW_MASTER_MPEG2_CBR,            //!< MPEG2 constant bitrate
        FW_MASTER_INFO(H264MVC_NO_RC, H264MVC),//FW_MASTER_H264MVC_NO_RC,        //!< MVC H264 with no rate control
        FW_MASTER_INFO(H264MVC_CBR, H264MVCCBR),//FW_MASTER_H264MVC_CBR,          //!< MVC H264 constant bitrate
        FW_MASTER_INFO(H264MVC_VBR, H264MVCVBR),//FW_MASTER_H264MVC_VBR,          //!< MVC H264 variable bitrate
        FW_MASTER_INFO(H264MVC_LLRC, H264MVCLLRC),//FW_MASTER_H264MVC_LLRC,         //!< MVC H264 low-latency rate control
    };

#ifdef B0_DEBUG
    printf("fw_num is %d, fw = %d\n", FW_NUM, (int)sizeof(topaz_fw_table));
    for(i = 0; i < FW_NUM; i++) {
        topaz_fw_table[i].index = i;
        topaz_fw_table[i].header.ver = FW_VER;
        topaz_fw_table[i].header.codec = FW_MASTER_H264_NO_RC;
        topaz_fw_table[i].header.text_size = 6883;
        topaz_fw_table[i].header.data_size = 2997;
        topaz_fw_table[i].header.data_location = 0x82886b90;
        topaz_fw_table[i].fw_text = aui32H264_MasterMTXTOPAZFWText;
        topaz_fw_table[i].fw_data = aui32H264_MasterMTXTOPAZFWData;
    }
#endif
  
    /* open file  */
    fp = fopen(FW_FILE_NAME_A0, "w");

    if (NULL == fp)
        return -1;

    create_firmware_A0(topaz_fw_table, fp);

    /* close file */
    fclose(fp);

    fp = fopen(FW_FILE_NAME_B0, "w");

    if (NULL == fp)
        return -1;

    create_firmware_B0(topaz_fw_table, fp);

    /* close file */
    fclose(fp);

    return 0;
}
