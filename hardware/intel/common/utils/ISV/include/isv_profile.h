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

#ifndef __ISV_PROFILE_H
#define __ISV_PROFILE_H

#define MAX_BUF_SIZE (4 * 1024)
#define MAX_TAB_SIZE (10)
#define MAX_STRING_LEN (50)

#include <utils/RefBase.h>
using namespace android;

typedef enum _FRC_RATE {
    FRC_RATE_1X = 1,
    FRC_RATE_2X,
    FRC_RATE_2_5X,
    FRC_RATE_4X
} FRC_RATE;

typedef enum {
    VPP_COMMON_ON   = 1,        // VPP is on
    VPP_FRC_ON      = 1 << 1,   // FRC is on
} VPP_SETTING_STATUS;

typedef struct _ISVParameter {
    char name[MAX_STRING_LEN];
    float value;
} ISVParameter;

typedef struct _ISVConfig {
    bool enabled;
    uint32_t minResolution;
    uint32_t maxResolution;
    //bool isOn;
    ISVParameter paraTables[MAX_TAB_SIZE];
    uint32_t paraSize;
} ISVConfig;

typedef struct _ISVFRCRate {
    uint32_t input_fps;
    FRC_RATE rate;
} ISVFRCRate;

//FIXME: need align to ProcFilterType
typedef enum _FilterType {
    FilterNone                          = 0x00000001,
    FilterNoiseReduction                = 0x00000002,
    FilterDeinterlacing                 = 0x00000004,
    FilterSharpening                    = 0x00000008,
    FilterColorBalance                  = 0x00000010,
    FilterDeblocking                    = 0x00000020,
    FilterFrameRateConversion           = 0x00000040,
    FilterSkinToneEnhancement           = 0x00000080,
    FilterTotalColorCorrection          = 0x00000100,
    FilterNonLinearAnamorphicScaling    = 0x00000200,
    FilterImageStabilization            = 0x00000400,
} FilterType;

class ISVProfile : public RefBase
{
public:
    ISVProfile(const uint32_t width, const uint32_t height);
    ~ISVProfile();

    /* get the global ISV setting status */
    FRC_RATE getFRCRate(uint32_t inputFps);

    /* get filter config data
     * the filters' status are saved in uint32_t
     */
    uint32_t getFilterStatus();

    /* the global setting for VPP */
    static bool isVPPOn();

    /* the global setting for FRC */
    static bool isFRCOn();

private:
    /* Read the global setting for ISV */
    static int32_t getGlobalStatus();

    /* Get the config data from XML file */
    void getDataFromXmlFile(void);

    /* Update the filter status */
    void updateFilterStatus();

    /* handle the XML file */
    static void startElement(void *userData, const char *name, const char **atts);
    static void endElement(void *userData, const char *name);
    int getFilterID(const char * name);
    uint32_t getResolution(const char * name);
    void getConfigData(const char *name, const char **atts);
    void handleFilterParameter(const char *name, const char **atts);
    void handleCommonParameter(const char *name, const char **atts);

    /* dump the config data */
    void dumpConfigData();

    typedef enum _ProcFilterType {
        ProcFilterNone = 0,
        ProcFilterNoiseReduction,
        ProcFilterDeinterlacing,
        ProcFilterSharpening,
        ProcFilterColorBalance,
        ProcFilterDeblocking,
        ProcFilterFrameRateConversion,
        ProcFilterSkinToneEnhancement,
        ProcFilterTotalColorCorrection,
        ProcFilterNonLinearAnamorphicScaling,
        ProcFilterImageStabilization,
        ProcFilterCount
    } ProcFilterType;

private:
    uint32_t mWidth;
    uint32_t mHeight;

    /* The default value of VPP/FRC.
     * They will be read from config xml file.
     */
    int32_t mDefaultVPPStatus;
    int32_t mDefaultFRCStatus;

    /* the filters' status according to resolution
     * bit 0  used for ProcFilterNone
     * bit 1  used for ProcFilterNoiseReduction
     * ...
     * bit 10 used for ProcFilterImageStabilization
     */
    uint32_t mStatus;

    ISVConfig mConfigs[ProcFilterCount];
    uint32_t mCurrentFilter; //used by parasing xml file
    ISVFRCRate mFrcRates[MAX_TAB_SIZE];
    uint32_t mCurrentFrcTab;

    static const int mBufSize = MAX_BUF_SIZE;
};

#endif /* __ISV_PROFILE_H */
