/*
 *
 * Copyright 2012 Samsung Electronics S.LSI Co. LTD
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

/*
 * @file        srp_api.c
 * @brief
 * @author      Yunji Kim (yunji.kim@samsung.com)
 * @version     1.1.0
 * @history
 *   2012.02.28 : Create
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#include "srp_api.h"

#define LOG_NDEBUG 1
#define LOG_TAG "libsrpapi"
#include <utils/Log.h>

static struct srp_buf_info ibuf_info;
static struct srp_buf_info obuf_info;
static struct srp_buf_info pcm_info;

static int srp_dev = -1;
static int srp_block_mode = SRP_INIT_BLOCK_MODE;

int SRP_Create(int block_mode)
{
    if (srp_dev == -1) {
        srp_block_mode = block_mode;
        srp_dev = open(SRP_DEV_NAME, O_RDWR |
                    ((block_mode == SRP_INIT_NONBLOCK_MODE) ? O_NDELAY : 0));
        if (srp_dev > 0)
            return srp_dev;
        else
            return SRP_ERROR_OPEN_FAIL;
    }

    ALOGE("%s: Device is already opened", __func__);
    return SRP_ERROR_ALREADY_OPEN;
}

int SRP_Init()
{
    int ret = SRP_RETURN_OK;
    unsigned int mmapped_size = 0;

    if (srp_dev != -1) {
        ret = ioctl(srp_dev, SRP_INIT);
        if (ret < 0)
            return ret;

        /* mmap for OBUF */
        ret = ioctl(srp_dev, SRP_GET_MMAP_SIZE, &mmapped_size);
        if (ret < 0) {
            ALOGE("%s: SRP_GET_MMAP_SIZE is failed", __func__);
            return SRP_ERROR_OBUF_MMAP;
        }
        obuf_info.mmapped_addr = mmap(0, mmapped_size,
                    PROT_READ | PROT_WRITE, MAP_SHARED, srp_dev, 0);
        if (!obuf_info.mmapped_addr) {
            ALOGE("%s: mmap is failed", __func__);
            return SRP_ERROR_OBUF_MMAP;
        }
        obuf_info.mmapped_size = mmapped_size;

        ret = SRP_RETURN_OK;
    } else {
        ALOGE("%s: Device is not ready", __func__);
        ret = SRP_ERROR_NOT_READY; /* device is not created */
    }

    return ret;
}

int SRP_Decode(void *buff, int size_byte)
{
    int ret = SRP_RETURN_OK;

    if (srp_dev != -1) {
        if (size_byte > 0) {
            ALOGV("%s: Send data to RP (%d bytes)", __func__, size_byte);

            ret = write(srp_dev, buff, size_byte);  /* Write Buffer to RP Driver */
            if (ret < 0) {
                if (ret != SRP_ERROR_IBUF_OVERFLOW)
                    ALOGE("SRP_Decode returned error code: %d", ret);
            }
            return ret; /* Write Success */
        } else {
            return ret;
        }
    }

    ALOGE("%s: Device is not ready", __func__);
    return SRP_ERROR_NOT_READY;
}

int SRP_Send_EOS(void)
{
    if (srp_dev != -1)
        return ioctl(srp_dev, SRP_SEND_EOS);

    return SRP_ERROR_NOT_READY;
}

int SRP_SetParams(int id, unsigned long val)
{
    if (srp_dev != -1)
        return 0; /* not yet */

    return SRP_ERROR_NOT_READY;
}

int SRP_GetParams(int id, unsigned long *pval)
{
    if (srp_dev != -1)
        return ioctl(srp_dev, id, pval);

    return SRP_ERROR_NOT_READY;
}

int SRP_Flush(void)
{
    if (srp_dev != -1)
        return ioctl(srp_dev, SRP_FLUSH);

    return SRP_ERROR_NOT_READY;
}

int SRP_Get_PCM(void **addr, unsigned int *size)
{
    int ret = SRP_RETURN_OK;

    if (srp_dev != -1) {
        ret = read(srp_dev, &pcm_info, 0);
        if (ret == -1) {
            *size = 0;
            ALOGE("%s: PCM read fail", __func__);
            return SRP_ERROR_OBUF_READ;
        }

        *addr = pcm_info.addr;
        *size = pcm_info.size;
    } else {
        return SRP_ERROR_NOT_READY;
    }

    return ret; /* Read Success */
}

int SRP_Get_Dec_Info(struct srp_dec_info *dec_info)
{
    int ret;

    if (srp_dev != -1) {
        ret = ioctl(srp_dev, SRP_GET_DEC_INFO, dec_info);
        if (ret < 0) {
            ALOGE("%s: Failed to get dec info", __func__);
            return SRP_ERROR_GETINFO_FAIL;
        }

        ALOGV("numChannels(%d), samplingRate(%d)", dec_info->channels, dec_info->sample_rate);

        ret = SRP_RETURN_OK;
    } else {
        ret = SRP_ERROR_NOT_READY;
    }

    return ret;
}

int SRP_Get_Ibuf_Info(void **addr, unsigned int *size, unsigned int *num)
{
    int ret = SRP_RETURN_OK;

    if (srp_dev != -1) {
        ret = ioctl(srp_dev, SRP_GET_IBUF_INFO, &ibuf_info);
        if (ret == -1) {
            ALOGE("%s: Failed to get Ibuf info", __func__);
            return SRP_ERROR_IBUF_INFO;
        }

        *addr = ibuf_info.addr;
        *size = ibuf_info.size;
        *num = ibuf_info.num;

        if (*num == 0) {
            ALOGE("%s: IBUF num is 0", __func__);
            return SRP_ERROR_INVALID_SETTING;
        }

        ret = SRP_RETURN_OK;
    } else {
        ret = SRP_ERROR_NOT_READY;
    }

    return ret;
}

int SRP_Get_Obuf_Info(void **addr, unsigned int *size, unsigned int *num)
{
    int ret = SRP_RETURN_OK;

    if (srp_dev != -1) {
        if (obuf_info.addr == NULL) {
            ret = ioctl(srp_dev, SRP_GET_OBUF_INFO, &obuf_info);
            if (ret < 0) {
                ALOGE("%s: SRP_GET_OBUF_INFO is failed", __func__);
                return SRP_ERROR_OBUF_INFO;
            }
        }

        *addr = obuf_info.addr;
        *size = obuf_info.size;
        *num = obuf_info.num;

        if (*num == 0) {
            ALOGE("%s: OBUF num is 0", __func__);
            return SRP_ERROR_INVALID_SETTING;
        }

        ret = SRP_RETURN_OK;
    } else {
        ret = SRP_ERROR_NOT_READY;
    }

    return ret;
}

int SRP_Deinit(void)
{
    if (srp_dev != -1) {
        munmap(obuf_info.mmapped_addr, obuf_info.mmapped_size);
        return ioctl(srp_dev, SRP_DEINIT);
    }

    return SRP_ERROR_NOT_READY;
}

int SRP_Terminate(void)
{
    int ret;

    if (srp_dev != -1) {
        ret = close(srp_dev);

        if (ret == 0) {
            srp_dev = -1; /* device closed */
            return SRP_RETURN_OK;
        }
    }

    return SRP_ERROR_NOT_READY;
}

int SRP_IsOpen(void)
{
    if (srp_dev == -1) {
        ALOGV("%s: Device is not opened", __func__);
        return 0;
    }

    ALOGV("%s: Device is opened", __func__);
    return 1;
}
