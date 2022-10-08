/* Copyright (c) 2012, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <hardware/gps.h>
#include <cutils/properties.h>
#include "loc_target.h"
#include "loc_log.h"
#include "log_util.h"

#define APQ8064_ID_1 "109"
#define APQ8064_ID_2 "153"
#define MPQ8064_ID_1 "130"
#define MSM8930_ID_1 "142"
#define MSM8930_ID_2 "116"
#define APQ8030_ID_1 "157"
#define APQ8074_ID_1 "184"

#define LINE_LEN 100
#define STR_LIQUID    "Liquid"
#define STR_SURF      "Surf"
#define STR_MTP       "MTP"
#define STR_APQ       "apq"
#define IS_STR_END(c) ((c) == '\0' || (c) == '\n' || (c) == '\r')
#define LENGTH(s) (sizeof(s) - 1)
#define GPS_CHECK_NO_ERROR 0
#define GPS_CHECK_NO_GPS_HW 1

static int gss_fd = 0;

static int read_a_line(const char * file_path, char * line, int line_size)
{
    FILE *fp;
    int result = 0;

    * line = '\0';
    fp = fopen(file_path, "r" );
    if( fp == NULL ) {
        LOC_LOGE("open failed: %s: %s\n", file_path, strerror(errno));
        result = -1;
    } else {
        int len;
        fgets(line, line_size, fp);
        len = strlen(line);
        len = len < line_size - 1? len : line_size - 1;
        line[len] = '\0';
        LOC_LOGD("cat %s: %s", file_path, line);
        fclose(fp);
    }
    return result;
}

unsigned int get_target(void)
{
    unsigned int target = TARGET_DEFAULT;

    char hw_platform[]      = "/sys/devices/system/soc/soc0/hw_platform";
    char id[]               = "/sys/devices/system/soc/soc0/id";
    char mdm[]              = "/dev/mdm"; // No such file or directory

    char rd_hw_platform[LINE_LEN];
    char rd_id[LINE_LEN];
    char rd_mdm[LINE_LEN];
    char baseband[LINE_LEN];

    property_get("ro.baseband", baseband, "");
    read_a_line(hw_platform, rd_hw_platform, LINE_LEN);
    read_a_line( id, rd_id, LINE_LEN);

    if( !memcmp(baseband, STR_APQ, LENGTH(STR_APQ)) ){
        if( !memcmp(rd_id, MPQ8064_ID_1, LENGTH(MPQ8064_ID_1))
            && IS_STR_END(rd_id[LENGTH(MPQ8064_ID_1)]) )
            target = TARGET_MPQ;
        else
            target = TARGET_APQ_SA;
    }
    else {
        if( (!memcmp(rd_hw_platform, STR_LIQUID, LENGTH(STR_LIQUID))
             && IS_STR_END(rd_hw_platform[LENGTH(STR_LIQUID)])) ||
            (!memcmp(rd_hw_platform, STR_SURF,   LENGTH(STR_SURF))
             && IS_STR_END(rd_hw_platform[LENGTH(STR_SURF)])) ||
            (!memcmp(rd_hw_platform, STR_MTP,   LENGTH(STR_MTP))
             && IS_STR_END(rd_hw_platform[LENGTH(STR_MTP)]))) {

            if (!read_a_line( mdm, rd_mdm, LINE_LEN))
                target = TARGET_MDM;
        }
        else if( (!memcmp(rd_id, MSM8930_ID_1, LENGTH(MSM8930_ID_1))
                   && IS_STR_END(rd_id[LENGTH(MSM8930_ID_1)])) ||
                  (!memcmp(rd_id, MSM8930_ID_2, LENGTH(MSM8930_ID_2))
                   && IS_STR_END(rd_id[LENGTH(MSM8930_ID_2)])) )
             target = TARGET_MSM_NO_SSC;
    }
    return target;
}
