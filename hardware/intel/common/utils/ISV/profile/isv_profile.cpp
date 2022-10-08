/*
 * Copyright (C) 2014 Intel Corporation.  All rights reserved.
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
 *
 */
#include <libexpat/expat.h>
#include <string.h>
#include <stdio.h>
#include <utils/Log.h>
#include "isv_profile.h"

#undef LOG_TAG
#define LOG_TAG "ISVProfile"

#define QCIF_AREA (176 * 144)

#define DEFAULT_XML_FILE "/etc/video_isv_profile.xml"

using namespace android;
static const char StatusOn[][5] = {"1frc", "1vpp"};

ISVProfile::ISVProfile(const uint32_t width, const uint32_t height)
{
    int i;

    mWidth = width;
    mHeight = height;

    mCurrentFilter = 0;
    mCurrentFrcTab = 0;
    mDefaultVPPStatus = 0;
    mDefaultFRCStatus = 0;

    mStatus = 0;

    memset(mConfigs, 0, sizeof(ISVConfig) * ProcFilterCount);

    for (i = 0; i < MAX_TAB_SIZE; i++) {
        mFrcRates[i].input_fps = 0;
        mFrcRates[i].rate = FRC_RATE_1X;
    }

    /* get the vpp global setting */
    //getGlobalStatus();

    /* load the config data from XML file */
    getDataFromXmlFile();

    /* update the filter status according to the configs */
    updateFilterStatus();

    /* dump data for debug */
    dumpConfigData();
}

ISVProfile::~ISVProfile()
{
}

FRC_RATE ISVProfile::getFRCRate(uint32_t inputFps)
{
    FRC_RATE rate = FRC_RATE_1X;
    int i;

    for (i = 0; i < MAX_TAB_SIZE; i++) {
        if (mFrcRates[i].input_fps == inputFps) {
            rate = mFrcRates[i].rate;
            break;
        }
    }
    return rate;
}

uint32_t ISVProfile::getFilterStatus()
{
    return mStatus;
}

bool ISVProfile::isVPPOn()
{
    int32_t status = getGlobalStatus();
    return (status != -1) ? (((status & VPP_COMMON_ON) != 0) ? true : false) : false;
}

bool ISVProfile::isFRCOn()
{
    int32_t status = getGlobalStatus();
    return (status != -1) ? (((status & VPP_FRC_ON) != 0) ? true : false) : false;
}

void ISVProfile::updateFilterStatus() {
    int i;
    uint32_t area = mWidth * mHeight;

    for (i = 1; i < ProcFilterCount; i++) {
        /* check config */
        if (mConfigs[i].enabled == false)
            continue;

        if (area > mConfigs[i].minResolution && area <= mConfigs[i].maxResolution)
            mStatus |= 1 << i;
        /* we should cover QCIF */
        else if (area == mConfigs[i].minResolution && area == QCIF_AREA)
            mStatus |= 1 << i;
    }
}

int ISVProfile::getFilterID(const char * name)
{
    int index = 0;

    if (strcmp(name, "ProcFilterNoiseReduction") == 0)
        index = ProcFilterNoiseReduction;
    else if (strcmp(name, "ProcFilterDeinterlacing") == 0)
        index = ProcFilterDeinterlacing;
    else if (strcmp(name, "ProcFilterSharpening") == 0)
        index = ProcFilterSharpening;
    else if (strcmp(name, "ProcFilterColorBalance") == 0)
        index = ProcFilterColorBalance;
    else if (strcmp(name, "ProcFilterDeblocking") == 0)
        index = ProcFilterDeblocking;
    else if (strcmp(name, "ProcFilterFrameRateConversion") == 0)
        index = ProcFilterFrameRateConversion;
    else if (strcmp(name, "ProcFilterSkinToneEnhancement") == 0)
        index = ProcFilterSkinToneEnhancement;
    else if (strcmp(name, "ProcFilterTotalColorCorrection") == 0)
        index = ProcFilterTotalColorCorrection;
    else if (strcmp(name, "ProcFilterNonLinearAnamorphicScaling") == 0)
        index = ProcFilterNonLinearAnamorphicScaling;
    else if (strcmp(name, "ProcFilterImageStabilization") == 0)
        index = ProcFilterImageStabilization;
    else
        index = 0;

    mCurrentFilter = index;

    return index;
}

uint32_t ISVProfile::getResolution(const char * name)
{
    uint32_t width = 0, height = 0;
    char *p = NULL, *str = NULL;
    int32_t lenth = strlen(name);

    str = (char*)malloc(lenth+1);
    if (NULL == str) {
        ALOGE("%s: failed to malloc buffer", __func__);
        return 0;
    }
    strncpy(str, name, lenth);
    str[lenth] = '\0';

    p = strtok(str, "x");
    if (p)
        width = atoi(p);
    p = strtok(NULL, "x");
    if (p)
        height = atoi(p);

    if (str) {
        free(str);
        str = NULL;
    }
    return width * height;
}

void ISVProfile::getConfigData(const char *name, const char **atts)
{
    int attIndex = 0;

    if (strcmp(name, "VideoPostProcessSettings") == 0) {
        return;
    } else if (strcmp(name, "Filter") == 0) {
        if (strcmp(atts[attIndex], "name") == 0) {
            if (getFilterID(atts[attIndex + 1]) == 0) {
                ALOGE("Couldn't parase the filter %s\n", atts[attIndex+1]);
            }
        } else {
            ALOGE("couldn't handle \"%s\" element for Filter\n", name);
        }
    } else if (strcmp(name, "enabled") == 0) {
        if (mCurrentFilter) {
            if (!strcmp(atts[attIndex], "value") && !strcmp(atts[attIndex + 1], "true"))
                mConfigs[mCurrentFilter].enabled = true;
        } else {
            ALOGE("Skip this element(%s) becaue this filter couldn't be supported\n", name);
        }
    } else if (strcmp(name, "minResolution") == 0) {
        if (mCurrentFilter && !strcmp(atts[attIndex], "value")) {
            if (!strcmp(atts[attIndex + 1], "0"))
                mConfigs[mCurrentFilter].minResolution = 0;
            else if (!strcmp(atts[attIndex + 1], "FFFFFFFF"))
                mConfigs[mCurrentFilter].minResolution = 0xFFFFFFFF;
            else
                mConfigs[mCurrentFilter].minResolution = getResolution(atts[attIndex + 1]);
        } else {
            ALOGE("Skip this element(%s) becaue this filter couldn't be supported\n", name);
        }
    } else if (strcmp(name, "maxResolution") == 0) {
        if (mCurrentFilter && !strcmp(atts[attIndex], "value")) {
            if (!strcmp(atts[attIndex + 1], "0"))
                mConfigs[mCurrentFilter].maxResolution = 0;
            else if (!strcmp(atts[attIndex + 1], "FFFFFFFF"))
                mConfigs[mCurrentFilter].maxResolution = 0xFFFFFFFF;
            else
                mConfigs[mCurrentFilter].maxResolution = getResolution(atts[attIndex + 1]);
        } else {
            ALOGE("Skip this element(%s) becaue this filter couldn't be supported\n", name);
        }
    } else if (strcmp(name, "FRCRate") == 0) {
        if (mCurrentFilter == ProcFilterFrameRateConversion) {
            if (!strcmp(atts[attIndex], "input") && !strcmp(atts[attIndex + 2], "rate")) {
                mFrcRates[mCurrentFrcTab].input_fps = atoi(atts[attIndex + 1]);
                if (!strcmp(atts[attIndex + 3], "2"))
                    mFrcRates[mCurrentFrcTab].rate = FRC_RATE_2X;
                else if (!strcmp(atts[attIndex + 3], "2.5"))
                    mFrcRates[mCurrentFrcTab].rate = FRC_RATE_2_5X;
                else if (!strcmp(atts[attIndex + 3], "4"))
                    mFrcRates[mCurrentFrcTab].rate = FRC_RATE_4X;
                else
                     mFrcRates[mCurrentFrcTab].rate = FRC_RATE_1X;

                /* update the pointer */
                if (mCurrentFrcTab < MAX_TAB_SIZE)
                    mCurrentFrcTab++;
            }
        } else {
            ALOGE("\"FRCRate\" element is only for ProcFilterFrameRateConversion\n");
        }
    } else if (strcmp(name, "parameter") == 0) {
        /* <parameter /> */
        handleFilterParameter(name, atts);
    } else if (strcmp(name, "Parameter") == 0) {
        /* <Parameter /> */
        handleCommonParameter(name, atts);
    } else
        ALOGE("Couldn't handle this element %s!\n", name);
}

void ISVProfile::handleFilterParameter(const char *name, const char **atts)
{
    int attIndex = 0;

    if (!mCurrentFilter) {
        ALOGE("\"%s\" must be in Filter element\n");
        return;
    }

    if (strcmp(atts[attIndex], "name") || strcmp(atts[attIndex + 2], "value")) {
        ALOGE("\"%s\" or \"%s\" couldn't match the %s format\n", atts[attIndex], atts[attIndex + 2], name);
        return;
    }

}

void ISVProfile::handleCommonParameter(const char *name, const char **atts)
{
    int attIndex = 0;

    if (strcmp(atts[attIndex], "name") || strcmp(atts[attIndex + 2], "value")) {
        ALOGE("\"%s\" or \"%s\" couldn't match the %s format\n", atts[attIndex], atts[attIndex + 2], name);
        return;
    }

    /* The default status of VPP */
    if (strcmp(atts[attIndex + 1], "DefaultVPPStatus") == 0)
        mDefaultVPPStatus = atoi(atts[attIndex + 3]);
    /* The default status of FRC */
    else if (strcmp(atts[attIndex + 1], "DefaultFRCStatus") == 0)
        mDefaultFRCStatus = atoi(atts[attIndex + 3]);
}

void ISVProfile::startElement(void *userData, const char *name, const char **atts)
{
    ISVProfile *profile = (ISVProfile *)userData;

    profile->getConfigData(name, atts);
}

void ISVProfile::endElement(void *userData, const char *name)
{
    ISVProfile *profile = (ISVProfile *)userData;

    if (!strcmp(name, "Filter"))
        profile->mCurrentFilter = 0;
}

void ISVProfile::getDataFromXmlFile()
{
    int done;
    void *pBuf = NULL;
    FILE *fp = NULL;

    fp = ::fopen(DEFAULT_XML_FILE, "r");
    if (NULL == fp) {
        ALOGE("@%s, line:%d, couldn't open profile %s", __func__, __LINE__, DEFAULT_XML_FILE);
        return;
    }

    XML_Parser parser = ::XML_ParserCreate(NULL);
    if (NULL == parser) {
        ALOGE("@%s, line:%d, parser is NULL", __func__, __LINE__);
        goto exit;
    }
    ::XML_SetUserData(parser, this);
    ::XML_SetElementHandler(parser, startElement, endElement);

    pBuf = malloc(mBufSize);
    if (NULL == pBuf) {
        ALOGE("@%s, line:%d, failed to malloc buffer", __func__, __LINE__);
        goto exit;
    }

    do {
        int len = (int)::fread(pBuf, 1, mBufSize, fp);
        if (!len) {
            if (ferror(fp)) {
                clearerr(fp);
                goto exit;
            }
        }
        done = len < mBufSize;
        if (XML_Parse(parser, (const char *)pBuf, len, done) == XML_STATUS_ERROR) {
            ALOGE("@%s, line:%d, XML_Parse error", __func__, __LINE__);
            goto exit;
        }
    } while (!done);

exit:
    if (parser)
        ::XML_ParserFree(parser);
    if (pBuf)
        free(pBuf);
    if (fp)
    ::fclose(fp);
}

int32_t ISVProfile::getGlobalStatus()
{
    char path[80];
    int userId = 0;
    int32_t status = 0;
    FILE *setting_handle, *config_handle;

    snprintf(path, 80, "/data/user/%d/com.intel.vpp/shared_prefs/vpp_settings.xml", userId);
    ALOGV("%s: %s",__func__, path);
    setting_handle = fopen(path, "r");
    if(setting_handle == NULL) {
        ALOGE("%s: failed to open file %s\n", __func__, path);

        /* Read the Filter config file to get default value */
        config_handle = fopen(DEFAULT_XML_FILE, "r");
        if (config_handle == NULL) {
            ALOGE("%s: failed to open file %s\n", __func__, DEFAULT_XML_FILE);
            return -1;
        }

        char xml_buf[MAX_BUF_SIZE + 1] = {0};
        memset(xml_buf, 0, MAX_BUF_SIZE);
        if (fread(xml_buf, 1, MAX_BUF_SIZE, config_handle) <= 0) {
            ALOGE("%s: failed to read config xml file!\n", __func__);
            fclose(config_handle);
            return -1;
        }
        xml_buf[MAX_BUF_SIZE] = '\0';

        if (strstr(xml_buf, "name=\"DefaultVPPStatus\" value=\"1\"") != NULL)
            status |= VPP_COMMON_ON;
        if (strstr(xml_buf, "name=\"DefaultFRCStatus\" value=\"1\"") != NULL)
            status |= VPP_FRC_ON;

        ALOGV("%s: using the default status: VPP=%d, FRC=%d\n", __func__,
            ((status & VPP_COMMON_ON) == 0) ? 0 : 1,
            ((status & VPP_FRC_ON) == 0) ? 0: 1);

        fclose(config_handle);
        return status;
    }

    const int MAXLEN = 1024;
    char buf[MAXLEN] = {0};
    memset(buf, 0 ,MAXLEN);
    if(fread(buf, 1, MAXLEN, setting_handle) <= 0) {
        ALOGE("%s: failed to read vpp config file %d", __func__, userId);
        fclose(setting_handle);
        return -1;
    }
    buf[MAXLEN - 1] = '\0';

    if(strstr(buf, StatusOn[0]) != NULL)
        status |= VPP_FRC_ON;

    if(strstr(buf, StatusOn[1]) != NULL)
        status |= VPP_COMMON_ON;

    fclose(setting_handle);
    return status;
}

void ISVProfile::dumpConfigData()
{
    uint32_t i, j;
    char filterNames[][50] = {
        "ProcFilterNone",
        "ProcFilterNoiseReduction",
        "ProcFilterDeinterlacing",
        "ProcFilterSharpening",
        "ProcFilterColorBalance",
        "ProcFilterDeblocking",
        "ProcFilterFrameRateConversion",
        "ProcFilterSkinToneEnhancement",
        "ProcFilterTotalColorCorrection",
        "ProcFilterNonLinearAnamorphicScaling",
        "ProcFilterImageStabilization"
    };
    char rateNames[][20] = {
        "FRC_RATE_0X",
        "FRC_RATE_1X",
        "FRC_RATE_2X",
        "FRC_RATE_2_5X",
        "FRC_RATE_4X",
    };

    ALOGV("========== VPP filter configs:==========\n");
    for (i = 1; i < ProcFilterCount; i++) {
        ALOGV("name=%s, enabled=%s, minResolution=%d, maxResolution=%d, isOn=%s\n",
            filterNames[i],
            (mConfigs[i].enabled == true) ? "true" : "false",
            mConfigs[i].minResolution,
            mConfigs[i].maxResolution,
            ((mStatus & (1 << i)) == 0) ? "false" : "true");
        if (mConfigs[i].paraSize) {
            ALOGV("\t\t parameters: ");
            for(j = 0; j < mConfigs[i].paraSize; j++)
                ALOGE("%s=%f,", mConfigs[i].paraTables[j].name, mConfigs[i].paraTables[j].value);
            ALOGV("\n");
        }
    }

    ALOGV("========== FRC rate configs:===========\n");
    for (i = 0; i < MAX_TAB_SIZE; i++) {
        if (mFrcRates[i].input_fps == 0)
            break;
        ALOGV("input_fps=%d, rate=%s\n", mFrcRates[i].input_fps, rateNames[mFrcRates[i].rate]);
    }

    ALOGI("========== common parameter configs:===========\n");
    ALOGI("mDefaultVPPStatus=%d\n", mDefaultVPPStatus);
    ALOGI("mDefaultFRCStatus=%d\n", mDefaultFRCStatus);

}
