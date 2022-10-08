/*
** Copyright (c) 2011-2012, 2015, The Linux Foundation. All rights reserved.
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

//#define ALOG_NDEBUG 0
#define ALOG_NIDEBUG 0
#define LOG_TAG "QCameraHWI_Parm"
#include <utils/Log.h>

#include <utils/Errors.h>
#include <utils/threads.h>
//#include <binder/MemoryHeapPmem.h>
#include <utils/String16.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <cutils/properties.h>
#include <math.h>
#include <linux/ioctl.h>
#include "QCameraParameters.h"
#include <media/mediarecorder.h>
#include <gralloc_priv.h>

#include "linux/msm_mdp.h"
#include <linux/fb.h>
#include <limits.h>

extern "C" {
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/system_properties.h>
#include <sys/time.h>
#include <stdlib.h>
#include <linux/msm_ion.h>

} // extern "C"

#include "QCameraHWI.h"
#include "QCameraStream.h"

/* QCameraHardwareInterface class implementation goes here*/
/* following code implements the parameter logic of this class*/
#define EXPOSURE_COMPENSATION_MAXIMUM_NUMERATOR 12
#define EXPOSURE_COMPENSATION_MINIMUM_NUMERATOR -12
#define EXPOSURE_COMPENSATION_DEFAULT_NUMERATOR 0
#define EXPOSURE_COMPENSATION_DENOMINATOR 6
#define EXPOSURE_COMPENSATION_STEP ((float (1))/EXPOSURE_COMPENSATION_DENOMINATOR)
#define DEFAULT_CAMERA_AREA "(0,0,0,0,0)" // important: spaces not allowed

#define HDR_HAL_FRAME 2

#define BURST_INTREVAL_MIN 1
#define BURST_INTREVAL_MAX 10
#define BURST_INTREVAL_DEFAULT 1

//Default FPS
#define MINIMUM_FPS 5
#define MAXIMUM_FPS 30
#define DEFAULT_FIXED_FPS 30
#define DEFAULT_FPS 30

//Default Picture Width
#define DEFAULT_PICTURE_WIDTH  640
#define DEFAULT_PICTURE_HEIGHT 480

//Default Video Width
#define DEFAULT_VIDEO_WIDTH 1920
#define DEFAULT_VIDEO_HEIGHT 1088

#define THUMBNAIL_SIZE_COUNT (sizeof(thumbnail_sizes)/sizeof(thumbnail_size_type))
#define DEFAULT_THUMBNAIL_SETTING 4
#define THUMBNAIL_WIDTH_STR "512"
#define THUMBNAIL_HEIGHT_STR "384"
#define THUMBNAIL_SMALL_HEIGHT 144

#define DONT_CARE_COORDINATE -1

//for histogram stats
#define HISTOGRAM_STATS_SIZE 257

//Supported preview fps ranges should be added to this array in the form (minFps,maxFps)
static  android::FPSRange FpsRangesSupported[] = {
            android::FPSRange(MINIMUM_FPS*1000,MAXIMUM_FPS*1000)
        };
#define FPS_RANGES_SUPPORTED_COUNT (sizeof(FpsRangesSupported)/sizeof(FpsRangesSupported[0]))


typedef struct {
    uint32_t aspect_ratio;
    uint32_t width;
    uint32_t height;
} thumbnail_size_type;

static thumbnail_size_type thumbnail_sizes[] = {
{ 7281, 512, 288 }, //1.777778
{ 6826, 480, 288 }, //1.666667
{ 6808, 256, 154 }, //1.66233
{ 6144, 432, 288 }, //1.5
{ 5461, 512, 384 }, //1.333333
{ 5006, 352, 288 }, //1.222222
{ 5461, 320, 240 }, //1.33333
{ 5006, 176, 144 }, //1.222222

};

static struct camera_size_type zsl_picture_sizes[] = {
  { 1280, 960}, // 1.3MP
  { 800, 600}, //SVGA
  { 800, 480}, // WVGA
  { 640, 480}, // VGA
  { 352, 288}, //CIF
  { 320, 240}, // QVGA
  { 176, 144} // QCIF
};

static camera_size_type default_picture_sizes[] = {
  { 4000, 3000}, // 12MP
  { 3264, 2448}, // 8MP
  { 3264, 1836}, // Picture Size to match 1080p,720p AR
  { 3264, 2176}, // Picture Size to match 480p AR
  { 2592, 1944}, // 5MP
  { 2048, 1536}, // 3MP QXGA
  { 1920, 1080}, // HD1080
  { 1600, 1200}, // 2MP UXGA
  { 1280, 960}, // 1.3MP
  { 1280, 720},
  { 720, 480},
  { 800, 480}, // WVGA
  { 640, 480}, // VGA
  { 352, 288}, // CIF
  { 320, 240}, // QVGA
  { 176, 144} // QCIF
};

static int iso_speed_values[] = {
    0, 1, 100, 200, 400, 800, 1600
};

extern int HAL_numOfCameras;
extern qcamera_info_t HAL_cameraInfo[MSM_MAX_CAMERA_SENSORS];
extern mm_camera_t * HAL_camerahandle[MSM_MAX_CAMERA_SENSORS];

namespace android {

static uint32_t  HFR_SIZE_COUNT=2;
static const int PICTURE_FORMAT_JPEG = 1;
static const int PICTURE_FORMAT_RAW = 2;

/********************************************************************/
static const str_map effects[] = {
    { QCameraParameters::EFFECT_NONE,       CAMERA_EFFECT_OFF },
    { QCameraParameters::EFFECT_MONO,       CAMERA_EFFECT_MONO },
    { QCameraParameters::EFFECT_NEGATIVE,   CAMERA_EFFECT_NEGATIVE },
    { QCameraParameters::EFFECT_SOLARIZE,   CAMERA_EFFECT_SOLARIZE },
    { QCameraParameters::EFFECT_SEPIA,      CAMERA_EFFECT_SEPIA },
    { QCameraParameters::EFFECT_POSTERIZE,  CAMERA_EFFECT_POSTERIZE },
    { QCameraParameters::EFFECT_AQUA,       CAMERA_EFFECT_AQUA },
    { QCameraParameters::EFFECT_EMBOSS,     CAMERA_EFFECT_EMBOSS },
    { QCameraParameters::EFFECT_SKETCH,     CAMERA_EFFECT_SKETCH },
    { QCameraParameters::EFFECT_NEON,       CAMERA_EFFECT_NEON }
};

static const str_map iso[] = {
    { QCameraParameters::ISO_AUTO,  CAMERA_ISO_AUTO},
    { QCameraParameters::ISO_HJR,   CAMERA_ISO_DEBLUR},
    { QCameraParameters::ISO_100,   CAMERA_ISO_100},
    { QCameraParameters::ISO_200,   CAMERA_ISO_200},
    { QCameraParameters::ISO_400,   CAMERA_ISO_400},
    { QCameraParameters::ISO_800,   CAMERA_ISO_800 },
    { QCameraParameters::ISO_1600,  CAMERA_ISO_1600 }
};

static const str_map scenemode[] = {
    { QCameraParameters::SCENE_MODE_AUTO,           CAMERA_BESTSHOT_OFF },
    { QCameraParameters::SCENE_MODE_ASD,            CAMERA_BESTSHOT_AUTO },
    { QCameraParameters::SCENE_MODE_ACTION,         CAMERA_BESTSHOT_ACTION },
    { QCameraParameters::SCENE_MODE_PORTRAIT,       CAMERA_BESTSHOT_PORTRAIT },
    { QCameraParameters::SCENE_MODE_LANDSCAPE,      CAMERA_BESTSHOT_LANDSCAPE },
    { QCameraParameters::SCENE_MODE_NIGHT,          CAMERA_BESTSHOT_NIGHT },
    { QCameraParameters::SCENE_MODE_NIGHT_PORTRAIT, CAMERA_BESTSHOT_NIGHT_PORTRAIT },
    { QCameraParameters::SCENE_MODE_THEATRE,        CAMERA_BESTSHOT_THEATRE },
    { QCameraParameters::SCENE_MODE_BEACH,          CAMERA_BESTSHOT_BEACH },
    { QCameraParameters::SCENE_MODE_SNOW,           CAMERA_BESTSHOT_SNOW },
    { QCameraParameters::SCENE_MODE_SUNSET,         CAMERA_BESTSHOT_SUNSET },
    { QCameraParameters::SCENE_MODE_STEADYPHOTO,    CAMERA_BESTSHOT_ANTISHAKE },
    { QCameraParameters::SCENE_MODE_FIREWORKS ,     CAMERA_BESTSHOT_FIREWORKS },
    { QCameraParameters::SCENE_MODE_SPORTS ,        CAMERA_BESTSHOT_SPORTS },
    { QCameraParameters::SCENE_MODE_PARTY,          CAMERA_BESTSHOT_PARTY },
    { QCameraParameters::SCENE_MODE_CANDLELIGHT,    CAMERA_BESTSHOT_CANDLELIGHT },
    { QCameraParameters::SCENE_MODE_BACKLIGHT,      CAMERA_BESTSHOT_BACKLIGHT },
    { QCameraParameters::SCENE_MODE_FLOWERS,        CAMERA_BESTSHOT_FLOWERS },
    { QCameraParameters::SCENE_MODE_AR,             CAMERA_BESTSHOT_AR },
    { QCameraParameters::SCENE_MODE_HDR,            CAMERA_BESTSHOT_AUTO },
};

static const str_map scenedetect[] = {
    { QCameraParameters::SCENE_DETECT_OFF, false  },
    { QCameraParameters::SCENE_DETECT_ON, true },
};

#define DONT_CARE AF_MODE_MAX
// These are listed as the supported focus-modes for cameras with AF
static const str_map focus_modes_auto[] = {
    { QCameraParameters::FOCUS_MODE_AUTO,     AF_MODE_AUTO},
    { QCameraParameters::FOCUS_MODE_INFINITY, AF_MODE_INFINITY },
    { QCameraParameters::FOCUS_MODE_NORMAL,   AF_MODE_NORMAL },
    { QCameraParameters::FOCUS_MODE_MACRO,    AF_MODE_MACRO },
    { QCameraParameters::FOCUS_MODE_CONTINUOUS_PICTURE, AF_MODE_CAF},
    { QCameraParameters::FOCUS_MODE_CONTINUOUS_VIDEO, AF_MODE_CAF },
    // Note that "FIXED" is omitted
};

// These are the supported focus-modes for cameras without AF
static const str_map focus_modes_fixed[] = {
    { QCameraParameters::FOCUS_MODE_FIXED,    AF_MODE_INFINITY },
};

static const str_map selectable_zone_af[] = {
    { QCameraParameters::SELECTABLE_ZONE_AF_AUTO,  AUTO },
    { QCameraParameters::SELECTABLE_ZONE_AF_SPOT_METERING, SPOT },
    { QCameraParameters::SELECTABLE_ZONE_AF_CENTER_WEIGHTED, CENTER_WEIGHTED },
    { QCameraParameters::SELECTABLE_ZONE_AF_FRAME_AVERAGE, AVERAGE }
};

static const str_map autoexposure[] = {
    { QCameraParameters::AUTO_EXPOSURE_FRAME_AVG,  CAMERA_AEC_FRAME_AVERAGE },
    { QCameraParameters::AUTO_EXPOSURE_CENTER_WEIGHTED, CAMERA_AEC_CENTER_WEIGHTED },
    { QCameraParameters::AUTO_EXPOSURE_SPOT_METERING, CAMERA_AEC_SPOT_METERING }
};

// from aeecamera.h
static const str_map whitebalance[] = {
    { QCameraParameters::WHITE_BALANCE_AUTO,            CAMERA_WB_AUTO },
    { QCameraParameters::WHITE_BALANCE_INCANDESCENT,    CAMERA_WB_INCANDESCENT },
    { QCameraParameters::WHITE_BALANCE_FLUORESCENT,     CAMERA_WB_FLUORESCENT },
    { QCameraParameters::WHITE_BALANCE_DAYLIGHT,        CAMERA_WB_DAYLIGHT },
    { QCameraParameters::WHITE_BALANCE_CLOUDY_DAYLIGHT, CAMERA_WB_CLOUDY_DAYLIGHT }
};

static const str_map antibanding[] = {
    { QCameraParameters::ANTIBANDING_OFF,  CAMERA_ANTIBANDING_OFF },
    { QCameraParameters::ANTIBANDING_50HZ, CAMERA_ANTIBANDING_50HZ },
    { QCameraParameters::ANTIBANDING_60HZ, CAMERA_ANTIBANDING_60HZ },
    { QCameraParameters::ANTIBANDING_AUTO, CAMERA_ANTIBANDING_AUTO }
};

static const str_map frame_rate_modes[] = {
        {QCameraParameters::KEY_PREVIEW_FRAME_RATE_AUTO_MODE, FPS_MODE_AUTO},
        {QCameraParameters::KEY_PREVIEW_FRAME_RATE_FIXED_MODE, FPS_MODE_FIXED}
};

static const str_map touchafaec[] = {
    { QCameraParameters::TOUCH_AF_AEC_OFF, false },
    { QCameraParameters::TOUCH_AF_AEC_ON, true }
};

static const str_map hfr[] = {
    { QCameraParameters::VIDEO_HFR_OFF, CAMERA_HFR_MODE_OFF },
    { QCameraParameters::VIDEO_HFR_2X, CAMERA_HFR_MODE_60FPS },
    { QCameraParameters::VIDEO_HFR_3X, CAMERA_HFR_MODE_90FPS },
    { QCameraParameters::VIDEO_HFR_4X, CAMERA_HFR_MODE_120FPS },
};
static const int HFR_VALUES_COUNT = (sizeof(hfr)/sizeof(str_map));

static const str_map flash[] = {
    { QCameraParameters::FLASH_MODE_OFF,  LED_MODE_OFF },
    { QCameraParameters::FLASH_MODE_AUTO, LED_MODE_AUTO },
    { QCameraParameters::FLASH_MODE_ON, LED_MODE_ON },
    { QCameraParameters::FLASH_MODE_TORCH, LED_MODE_TORCH}
};

static const str_map lensshade[] = {
    { QCameraParameters::LENSSHADE_ENABLE, true },
    { QCameraParameters::LENSSHADE_DISABLE, false }
};

static const str_map mce[] = {
    { QCameraParameters::MCE_ENABLE, true },
    { QCameraParameters::MCE_DISABLE, false }
};

static const str_map histogram[] = {
    { QCameraParameters::HISTOGRAM_ENABLE, true },
    { QCameraParameters::HISTOGRAM_DISABLE, false }
};

static const str_map skinToneEnhancement[] = {
    { QCameraParameters::SKIN_TONE_ENHANCEMENT_ENABLE, true },
    { QCameraParameters::SKIN_TONE_ENHANCEMENT_DISABLE, false }
};

static const str_map denoise[] = {
    { QCameraParameters::DENOISE_OFF, false },
    { QCameraParameters::DENOISE_ON, true }
};

static const str_map facedetection[] = {
    { QCameraParameters::FACE_DETECTION_OFF, false },
    { QCameraParameters::FACE_DETECTION_ON, true }
};

static const str_map redeye_reduction[] = {
    { QCameraParameters::REDEYE_REDUCTION_ENABLE, true },
    { QCameraParameters::REDEYE_REDUCTION_DISABLE, false }
};

static const str_map picture_formats[] = {
        {QCameraParameters::PIXEL_FORMAT_JPEG, PICTURE_FORMAT_JPEG},
        {QCameraParameters::PIXEL_FORMAT_RAW, PICTURE_FORMAT_RAW}
};

static const str_map recording_Hints[] = {
        {"false", false},
        {"true",  true}
};

static const str_map preview_formats[] = {
        {QCameraParameters::PIXEL_FORMAT_YUV420SP,   HAL_PIXEL_FORMAT_YCrCb_420_SP},
        {QCameraParameters::PIXEL_FORMAT_YUV420SP_ADRENO, HAL_PIXEL_FORMAT_YCrCb_420_SP_ADRENO},
        {QCameraParameters::PIXEL_FORMAT_YV12, HAL_PIXEL_FORMAT_YV12},
        {QCameraParameters::PIXEL_FORMAT_YUV420P,HAL_PIXEL_FORMAT_YV12},
        {QCameraParameters::PIXEL_FORMAT_NV12, HAL_PIXEL_FORMAT_YCbCr_420_SP}
};

static const preview_format_info_t preview_format_info_list[] = {
  {HAL_PIXEL_FORMAT_YCrCb_420_SP, CAMERA_YUV_420_NV21, CAMERA_PAD_TO_WORD, 2},
  {HAL_PIXEL_FORMAT_YCrCb_420_SP_ADRENO, CAMERA_YUV_420_NV21, CAMERA_PAD_TO_4K, 2},
  {HAL_PIXEL_FORMAT_YCbCr_420_SP, CAMERA_YUV_420_NV12, CAMERA_PAD_TO_WORD, 2},
  {HAL_PIXEL_FORMAT_YV12,         CAMERA_YUV_420_YV12, CAMERA_PAD_TO_WORD, 3}
};

static const str_map zsl_modes[] = {
    { QCameraParameters::ZSL_OFF, false },
    { QCameraParameters::ZSL_ON, true },
};


static const str_map hdr_bracket[] = {
    { QCameraParameters::AE_BRACKET_HDR_OFF,HDR_BRACKETING_OFF},
    { QCameraParameters::AE_BRACKET_HDR,HDR_MODE },
};

typedef enum {
    NORMAL_POWER,
    LOW_POWER
} power_mode;

static const str_map power_modes[] = {
    { QCameraParameters::NORMAL_POWER,NORMAL_POWER },
    { QCameraParameters::LOW_POWER,LOW_POWER }
};

/**************************************************************************/
static int attr_lookup(const str_map arr[], int len, const char *name)
{
    if (name) {
        for (int i = 0; i < len; i++) {
            if (!strcmp(arr[i].desc, name))
                return arr[i].val;
        }
    }
    return NOT_FOUND;
}

bool QCameraHardwareInterface::native_set_parms(
    mm_camera_parm_type_t type, uint16_t length, void *value)
{
    ALOGV("%s : type : %d Value : %d",__func__,type,*((int *)value));
    if(MM_CAMERA_OK != cam_config_set_parm(mCameraId, type,value )) {
        ALOGE("native_set_parms failed: type %d length %d error %s",
            type, length, strerror(errno));
        return false;
    }

    return true;

}

bool QCameraHardwareInterface::native_set_parms(
    mm_camera_parm_type_t type, uint16_t length, void *value, int *result)
{
    *result= cam_config_set_parm(mCameraId, type,value );
    if(MM_CAMERA_OK == *result) {
        ALOGV("native_set_parms: succeeded : %d", *result);
        return true;
    }

    ALOGE("native_set_parms failed: type %d length %d error str %s error# %d",
        type, length, strerror(errno), errno);
    return false;
}

//Filter Picture sizes based on max width and height
/* TBD: do we still need this - except for ZSL? */
void QCameraHardwareInterface::filterPictureSizes(){
    unsigned int i;
    if(mPictureSizeCount <= 0)
        return;
    maxSnapshotWidth = mPictureSizes[0].width;
    maxSnapshotHeight = mPictureSizes[0].height;
   // Iterate through all the width and height to find the max value
    for(i =0; i<mPictureSizeCount;i++){
        if(((maxSnapshotWidth < mPictureSizes[i].width) &&
            (maxSnapshotHeight <= mPictureSizes[i].height))){
            maxSnapshotWidth = mPictureSizes[i].width;
            maxSnapshotHeight = mPictureSizes[i].height;
        }
    }
    if(myMode & CAMERA_ZSL_MODE){
        // due to lack of PMEM we restrict to lower resolution
        mPictureSizesPtr = zsl_picture_sizes;
        mSupportedPictureSizesCount = 7;
    }else{
        mPictureSizesPtr = mPictureSizes;
        mSupportedPictureSizesCount = mPictureSizeCount;
    }
}

static String8 create_sizes_str(const camera_size_type *sizes, int len) {
    String8 str;
    char buffer[32];

    if (len > 0) {
        snprintf(buffer, sizeof(buffer), "%dx%d", sizes[0].width, sizes[0].height);
        str.append(buffer);
    }
    for (int i = 1; i < len; i++) {
        snprintf(buffer, sizeof(buffer), ",%dx%d", sizes[i].width, sizes[i].height);
        str.append(buffer);
    }
    return str;
}

String8 QCameraHardwareInterface::create_values_str(const str_map *values, int len) {
    String8 str;

    if (len > 0) {
        str.append(values[0].desc);
    }
    for (int i = 1; i < len; i++) {
        str.append(",");
        str.append(values[i].desc);
    }
    return str;
}

static String8 create_fps_str(const android:: FPSRange* fps, int len) {
    String8 str;
    char buffer[32];

    if (len > 0) {
        snprintf(buffer, sizeof(buffer), "(%d,%d)", fps[0].minFPS, fps[0].maxFPS);
        str.append(buffer);
    }
    for (int i = 1; i < len; i++) {
        snprintf(buffer, sizeof(buffer), ",(%d,%d)", fps[i].minFPS, fps[i].maxFPS);
        str.append(buffer);
    }
    return str;
}

static String8 create_values_range_str(int min, int max){
    String8 str;
    char buffer[32];

    if(min <= max){
        snprintf(buffer, sizeof(buffer), "%d", min);
        str.append(buffer);

        for (int i = min + 1; i <= max; i++) {
            snprintf(buffer, sizeof(buffer), ",%d", i);
            str.append(buffer);
        }
    }
    return str;
}

static int parse_size(const char *str, int &width, int &height)
{
    // Find the width.
    char *end;
    int w = (int)strtol(str, &end, 10);
    // If an 'x' or 'X' does not immediately follow, give up.
    if ( (*end != 'x') && (*end != 'X') )
        return -1;

    // Find the height, immediately after the 'x'.
    int h = (int)strtol(end+1, 0, 10);

    width = w;
    height = h;

    return 0;
}

bool QCameraHardwareInterface::isValidDimension(int width, int height) {
    bool retVal = false;
    /* This function checks if a given resolution is valid or not.
     * A particular resolution is considered valid if it satisfies
     * the following conditions:
     * 1. width & height should be multiple of 16.
     * 2. width & height should be less than/equal to the dimensions
     *    supported by the camera sensor.
     * 3. the aspect ratio is a valid aspect ratio and is among the
     *    commonly used aspect ratio as determined by the thumbnail_sizes
     *    data structure.
     */

    if( (width == CEILING16(width)) && (height == CEILING16(height))
     && (width <= maxSnapshotWidth)
    && (height <= maxSnapshotHeight) )
    {
        uint32_t pictureAspectRatio = (uint32_t)((width * Q12)/height);
        for(uint32_t i = 0; i < THUMBNAIL_SIZE_COUNT; i++ ) {
            if(thumbnail_sizes[i].aspect_ratio == pictureAspectRatio) {
                retVal = true;
                break;
            }
        }
    }
    return retVal;
}

void QCameraHardwareInterface::hasAutoFocusSupport(){

    ALOGV("%s",__func__);

    if(isZSLMode()){
        mHasAutoFocusSupport = false;
        return;
    }

    if(cam_ops_is_op_supported (mCameraId, MM_CAMERA_OPS_FOCUS )) {
        mHasAutoFocusSupport = true;
    }
    else {
        ALOGV("AutoFocus is not supported");
        mHasAutoFocusSupport = false;
    }

    ALOGV("%s:rc= %d",__func__, mHasAutoFocusSupport);

}

bool QCameraHardwareInterface::supportsSceneDetection() {
   bool rc = cam_config_is_parm_supported(mCameraId,MM_CAMERA_PARM_ASD_ENABLE);
   return rc;
}

bool QCameraHardwareInterface::supportsFaceDetection() {
    bool rc;

    status_t ret = NO_ERROR;
    mm_camera_op_mode_type_t op_mode;

    ret = cam_config_get_parm(mCameraId, MM_CAMERA_PARM_OP_MODE, &op_mode);
    if(ret != NO_ERROR){
        ALOGE("%s: Failed to get Op Mode", __func__);
    }

    ALOGV("%s: OP_Mode is %d, ret=%d, mHdrMode=%d",__func__,op_mode,ret,mHdrMode);
    if ((ret == NO_ERROR) && (op_mode == MM_CAMERA_OP_MODE_VIDEO) && (mHdrMode != HDR_MODE))
    {
        ALOGV("%s: Video mode : FD not supported",__func__);
        return false;
    }
    else{
        rc = cam_config_is_parm_supported(mCameraId,MM_CAMERA_PARM_FD);
        ALOGV("%s: Still mode : FD supported : %d",__func__,rc);
        return rc;
    }
}

bool QCameraHardwareInterface::supportsSelectableZoneAf() {
   bool rc = cam_config_is_parm_supported(mCameraId,MM_CAMERA_PARM_FOCUS_RECT);
   return rc;
}

bool QCameraHardwareInterface::supportsRedEyeReduction() {
   bool rc = cam_config_is_parm_supported(mCameraId,MM_CAMERA_PARM_REDEYE_REDUCTION);
   return rc;
}

static String8 create_str(int16_t *arr, int length){
    String8 str;
    char buffer[32] = {0};

    if(length > 0){
        snprintf(buffer, sizeof(buffer), "%d", arr[0]);
        str.append(buffer);
    }

    for (int i =1;i<length;i++){
        snprintf(buffer, sizeof(buffer), ",%d",arr[i]);
        str.append(buffer);
    }
    return str;
}

bool QCameraHardwareInterface::getMaxPictureDimension(mm_camera_dimension_t *maxDim)
{
    bool ret = NO_ERROR;
    mm_camera_dimension_t dim;

    ret = cam_config_get_parm(mCameraId,
                              MM_CAMERA_PARM_MAX_PICTURE_SIZE, &dim);
    if (ret != NO_ERROR)
        return ret;

    /* Find the first dimension in the mPictureSizes
     * array which is smaller than the max dimension.
     * This will be the valid max picture resolution */
    for (unsigned int i = 0; i < mPictureSizeCount; i++) {
        if ((mPictureSizes[i].width <= dim.width) &&
            (mPictureSizes[i].height <= dim.height)) {
            maxDim->height = mPictureSizes[i].height;
            maxDim->width  = mPictureSizes[i].width;
            break;
        }
    }
    ALOGV("%s: Found Max Picture dimension: %d x %d", __func__,
          maxDim->width, maxDim->height);
    return ret;
}
void QCameraHardwareInterface::loadTables()
{

    bool ret = NO_ERROR;
    ALOGV("%s: E", __func__);

    ret = cam_config_get_parm(mCameraId,
            MM_CAMERA_PARM_PREVIEW_SIZES_CNT, &preview_sizes_count);

    default_sizes_tbl_t preview_sizes_tbl;
    preview_sizes_tbl.tbl_size=preview_sizes_count;
    preview_sizes_tbl.sizes_tbl=&default_preview_sizes[0];
    if(MM_CAMERA_OK != cam_config_get_parm(mCameraId,
                            MM_CAMERA_PARM_DEF_PREVIEW_SIZES, &preview_sizes_tbl)){
        ALOGE("%s:Failed to get default preview sizes",__func__);
    }
    ret = cam_config_get_parm(mCameraId,
                MM_CAMERA_PARM_VIDEO_SIZES_CNT, &video_sizes_count);

    default_sizes_tbl_t video_sizes_tbl;
    video_sizes_tbl.tbl_size=video_sizes_count;
    video_sizes_tbl.sizes_tbl=&default_video_sizes[0];
    if(MM_CAMERA_OK != cam_config_get_parm(mCameraId,
                            MM_CAMERA_PARM_DEF_VIDEO_SIZES, &video_sizes_tbl)){
        ALOGE("%s:Failed to get default video sizes",__func__);
    }

    ret = cam_config_get_parm(mCameraId,
                MM_CAMERA_PARM_THUMB_SIZES_CNT, &thumbnail_sizes_count);

    default_sizes_tbl_t thumbnail_sizes_tbl;
    thumbnail_sizes_tbl.tbl_size=thumbnail_sizes_count;
    thumbnail_sizes_tbl.sizes_tbl=&default_thumbnail_sizes[0];
    if(MM_CAMERA_OK != cam_config_get_parm(mCameraId,
                            MM_CAMERA_PARM_DEF_THUMB_SIZES, &thumbnail_sizes_tbl)){
        ALOGE("%s:Failed to get default thumbnail sizes",__func__);
    }

    ret = cam_config_get_parm(mCameraId,
                MM_CAMERA_PARM_HFR_SIZES_CNT, &hfr_sizes_count);

    default_sizes_tbl_t hfr_sizes_tbl;
    hfr_sizes_tbl.tbl_size=hfr_sizes_count;
    hfr_sizes_tbl.sizes_tbl=&default_hfr_sizes[0];
    if(MM_CAMERA_OK != cam_config_get_parm(mCameraId,
                            MM_CAMERA_PARM_DEF_HFR_SIZES, &hfr_sizes_tbl)){
        ALOGE("%s:Failed to get default HFR  sizes",__func__);
    }
    ALOGV("%s: X", __func__);
}

rat_t getRational(int num, int denom)
{
    rat_t temp = {num, denom};
    return temp;
}

void QCameraHardwareInterface::initDefaultParameters()
{
    bool ret;
    char prop[PROPERTY_VALUE_MAX];
    mm_camera_dimension_t maxDim;
    int rc = MM_CAMERA_OK;
    ALOGV("%s: E", __func__);

    memset(&maxDim, 0, sizeof(mm_camera_dimension_t));
    ret = getMaxPictureDimension(&maxDim);

    if (ret != NO_ERROR) {
        ALOGE("%s: Cannot get Max picture size supported", __func__);
        return;
    }
    if (!maxDim.width || !maxDim.height) {
        maxDim.width = DEFAULT_LIVESHOT_WIDTH;
        maxDim.height = DEFAULT_LIVESHOT_HEIGHT;
    }

    memset(prop, 0, sizeof(prop));
    property_get("persist.camera.snap.format", prop, "0");
    mSnapshotFormat = atoi(prop);
    ALOGV("%s: prop =(%s), snap_format=%d", __func__, prop, mSnapshotFormat);

    //cam_ctrl_dimension_t dim;
    mHFRLevel = 0;
    memset(&mDimension, 0, sizeof(cam_ctrl_dimension_t));
    memset(&mPreviewFormatInfo, 0, sizeof(preview_format_info_t));
    mDimension.video_width     = DEFAULT_VIDEO_WIDTH;
    mDimension.video_height    = DEFAULT_VIDEO_HEIGHT;
    // mzhu mDimension.picture_width   = DEFAULT_STREAM_WIDTH;
    // mzhu mDimension.picture_height  = DEFAULT_STREAM_HEIGHT;
    mDimension.picture_width   = maxDim.width;
    mDimension.picture_height  = maxDim.height;
    mDimension.display_width   = DEFAULT_STREAM_WIDTH;
    mDimension.display_height  = DEFAULT_STREAM_HEIGHT;
    mDimension.orig_picture_dx = mDimension.picture_width;
    mDimension.orig_picture_dy = mDimension.picture_height;
    mDimension.ui_thumbnail_width = DEFAULT_STREAM_WIDTH;
    mDimension.ui_thumbnail_height = DEFAULT_STREAM_HEIGHT;
    mDimension.orig_video_width = DEFAULT_STREAM_WIDTH;
    mDimension.orig_video_height = DEFAULT_STREAM_HEIGHT;

    mDimension.prev_format     = CAMERA_YUV_420_NV21;
    mDimension.enc_format      = CAMERA_YUV_420_NV12;
    if (mSnapshotFormat == 1) {
      mDimension.main_img_format = CAMERA_YUV_422_NV61;
    } else {
      mDimension.main_img_format = CAMERA_YUV_420_NV21;
    }
    mDimension.thumb_format    = CAMERA_YUV_420_NV21;
    ALOGV("%s: main_img_format =%d, thumb_format=%d", __func__,
         mDimension.main_img_format, mDimension.thumb_format);
    mDimension.prev_padding_format = CAMERA_PAD_TO_WORD;

    ret = native_set_parms(MM_CAMERA_PARM_DIMENSION,
                              sizeof(cam_ctrl_dimension_t), (void *) &mDimension);
    if(!ret) {
      ALOGE("MM_CAMERA_PARM_DIMENSION Failed.");
      return;
    }

    hasAutoFocusSupport();

    // Initialize constant parameter strings. This will happen only once in the
    // lifetime of the mediaserver process.
    if (true/*!mParamStringInitialized*/) {
        //filter picture sizes
        filterPictureSizes();
        mPictureSizeValues = create_sizes_str(
                mPictureSizesPtr, mSupportedPictureSizesCount);
        mPreviewSizeValues = create_sizes_str(
                mPreviewSizes,  mPreviewSizeCount);
        mVideoSizeValues = create_sizes_str(
                mVideoSizes,  mVideoSizeCount);

        //Query for max HFR value
        camera_hfr_mode_t maxHFR;
        cam_config_get_parm(mCameraId, MM_CAMERA_PARM_MAX_HFR_MODE, (void *)&maxHFR);
        //Filter HFR values and build parameter string
        String8 str;
        for(int i=0; i<HFR_VALUES_COUNT; i++){
            if(hfr[i].val <= maxHFR){
                if(i>0)	str.append(",");
                str.append(hfr[i].desc);
            }
        }
        mHfrValues = str;
        mHfrSizeValues = create_sizes_str(
                default_hfr_sizes, hfr_sizes_count);
        mFpsRangesSupportedValues = create_fps_str(
            FpsRangesSupported,FPS_RANGES_SUPPORTED_COUNT );
        mParameters.set(
            QCameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE,
            mFpsRangesSupportedValues);
        mParameters.setPreviewFpsRange(MINIMUM_FPS*1000,MAXIMUM_FPS*1000);
        mFlashValues = create_values_str(
            flash, sizeof(flash) / sizeof(str_map));
        mLensShadeValues = create_values_str(
            lensshade,sizeof(lensshade)/sizeof(str_map));
        mMceValues = create_values_str(
            mce,sizeof(mce)/sizeof(str_map));
        mEffectValues = create_values_str(effects, sizeof(effects) / sizeof(str_map));
        mAntibandingValues = create_values_str(
            antibanding, sizeof(antibanding) / sizeof(str_map));
        mIsoValues = create_values_str(iso,sizeof(iso)/sizeof(str_map));
        mAutoExposureValues = create_values_str(
            autoexposure, sizeof(autoexposure) / sizeof(str_map));
        mWhitebalanceValues = create_values_str(
            whitebalance, sizeof(whitebalance) / sizeof(str_map));

        if(mHasAutoFocusSupport){
            mFocusModeValues = create_values_str(
                    focus_modes_auto, sizeof(focus_modes_auto) / sizeof(str_map));
        }

        mSceneModeValues = create_values_str(scenemode, sizeof(scenemode) / sizeof(str_map));

        if(mHasAutoFocusSupport){
            mTouchAfAecValues = create_values_str(
                touchafaec,sizeof(touchafaec)/sizeof(str_map));
        }
        //Currently Enabling Histogram for 8x60
        mHistogramValues = create_values_str(
            histogram,sizeof(histogram)/sizeof(str_map));

        mSkinToneEnhancementValues = create_values_str(
            skinToneEnhancement,sizeof(skinToneEnhancement)/sizeof(str_map));

        mPictureFormatValues = create_values_str(
            picture_formats, sizeof(picture_formats)/sizeof(str_map));

        mZoomSupported=false;
        mMaxZoom=0;
        mm_camera_zoom_tbl_t zmt;
        if(MM_CAMERA_OK != cam_config_get_parm(mCameraId,
                             MM_CAMERA_PARM_MAXZOOM, &mMaxZoom)){
            ALOGE("%s:Failed to get max zoom",__func__);
        }else{

            ALOGV("Max Zoom:%d",mMaxZoom);
            /* Kernel driver limits the max amount of data that can be retreived through a control
            command to 260 bytes hence we conservatively limit to 110 zoom ratios */
            if(mMaxZoom>MAX_ZOOM_RATIOS) {
                ALOGV("%s:max zoom is larger than sizeof zoomRatios table",__func__);
                mMaxZoom=MAX_ZOOM_RATIOS-1;
            }
            zmt.size=mMaxZoom;
            zmt.zoom_ratio_tbl=&zoomRatios[0];
            if(MM_CAMERA_OK != cam_config_get_parm(mCameraId,
                                 MM_CAMERA_PARM_ZOOM_RATIO, &zmt)){
                ALOGE("%s:Failed to get max zoom ratios",__func__);
            }else{
                mZoomSupported=true;
                mZoomRatioValues =  create_str(zoomRatios, mMaxZoom);
            }
        }

        ALOGV("Zoom supported:%d",mZoomSupported);

        denoise_value = create_values_str(
            denoise, sizeof(denoise) / sizeof(str_map));

       if(supportsFaceDetection()) {
            mFaceDetectionValues = create_values_str(
                facedetection, sizeof(facedetection) / sizeof(str_map));
        }

        if(mHasAutoFocusSupport){
            mSelectableZoneAfValues = create_values_str(
                selectable_zone_af, sizeof(selectable_zone_af) / sizeof(str_map));
        }

        mSceneDetectValues = create_values_str(scenedetect, sizeof(scenedetect) / sizeof(str_map));

        mRedeyeReductionValues = create_values_str(
            redeye_reduction, sizeof(redeye_reduction) / sizeof(str_map));

        mZslValues = create_values_str(
            zsl_modes,sizeof(zsl_modes)/sizeof(str_map));

        mParamStringInitialized = true;
    }

    //set supported video sizes
    mParameters.set(QCameraParameters::KEY_SUPPORTED_VIDEO_SIZES, mVideoSizeValues.string());

    //set default video size to first one in supported table
    String8 vSize = create_sizes_str(&mVideoSizes[0], 1);
    mParameters.set(QCameraParameters::KEY_VIDEO_SIZE, vSize.string());

    //Set Preview size
    int default_preview_width, default_preview_height;
    cam_config_get_parm(mCameraId, MM_CAMERA_PARM_DEFAULT_PREVIEW_WIDTH,
            &default_preview_width);
    cam_config_get_parm(mCameraId, MM_CAMERA_PARM_DEFAULT_PREVIEW_HEIGHT,
            &default_preview_height);
    mParameters.setPreviewSize(default_preview_width, default_preview_height);
    mParameters.set(QCameraParameters::KEY_SUPPORTED_PREVIEW_SIZES,
                    mPreviewSizeValues.string());
    mDimension.display_width = default_preview_width;
    mDimension.display_height = default_preview_height;

    //Set Preview Frame Rate
    if(mFps >= MINIMUM_FPS && mFps <= MAXIMUM_FPS) {
        mPreviewFrameRateValues = create_values_range_str(
        MINIMUM_FPS, mFps);
    }else{
        mPreviewFrameRateValues = create_values_range_str(
        MINIMUM_FPS, MAXIMUM_FPS);
    }


    if (cam_config_is_parm_supported(mCameraId, MM_CAMERA_PARM_FPS)) {
        mParameters.set(QCameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES,
                        mPreviewFrameRateValues.string());
     } else {
        mParameters.set(
            QCameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES,
            DEFAULT_FIXED_FPS);
    }

    //Set Preview Frame Rate Modes
    mParameters.setPreviewFrameRateMode("frame-rate-auto");
    mFrameRateModeValues = create_values_str(
            frame_rate_modes, sizeof(frame_rate_modes) / sizeof(str_map));
      if(cam_config_is_parm_supported(mCameraId, MM_CAMERA_PARM_FPS_MODE)){
        mParameters.set(QCameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATE_MODES,
                    mFrameRateModeValues.string());
    }

    //Set Preview Format
    //mParameters.setPreviewFormat("yuv420sp"); // informative
    mParameters.setPreviewFormat(QCameraParameters::PIXEL_FORMAT_YUV420SP);

    mPreviewFormatValues = create_values_str(
        preview_formats, sizeof(preview_formats) / sizeof(str_map));
    mParameters.set(QCameraParameters::KEY_SUPPORTED_PREVIEW_FORMATS,
            mPreviewFormatValues.string());

    //Set Overlay Format
    mParameters.set("overlay-format", HAL_PIXEL_FORMAT_YCbCr_420_SP);
    mParameters.set("max-num-detected-faces-hw", "2");

    // Set supported max faces
    int maxNumFaces = 0;
    if (supportsFaceDetection()) {
        //Query the maximum number of faces supported by hardware.
        if(MM_CAMERA_OK != cam_config_get_parm(mCameraId,
                               MM_CAMERA_PARM_MAX_NUM_FACES_DECT, &maxNumFaces)){
            ALOGE("%s:Failed to get max number of faces supported",__func__);
        }
    }
    mParameters.set(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_HW, maxNumFaces);
    //This paramtere is set to default here. This will be changed by application
    //if it needs to support specific number of faces. See also setParameters.
    mParameters.set(QCameraParameters::KEY_MAX_NUM_REQUESTED_FACES, 2);

    // Set camera features supported flag
    int32_t featureFlag = 0;
    if (supportsFaceDetection()) {
        featureFlag |= 0x00000001; // bit 0 indicate faciral feature
    }
    mParameters.set(QCameraParameters::KEY_SUPPORTED_CAMERA_FEATURES, featureFlag);

    //Set Picture Size
    mParameters.setPictureSize(DEFAULT_PICTURE_WIDTH, DEFAULT_PICTURE_HEIGHT);
    mParameters.set(QCameraParameters::KEY_SUPPORTED_PICTURE_SIZES,
                    mPictureSizeValues.string());

    //Set Preview Frame Rate
    if(mFps >= MINIMUM_FPS && mFps <= MAXIMUM_FPS) {
        mParameters.setPreviewFrameRate(mFps);
    }else{
        mParameters.setPreviewFrameRate(DEFAULT_FPS);
    }

    //Set Picture Format
    mParameters.setPictureFormat("jpeg"); // informative
    mParameters.set(QCameraParameters::KEY_SUPPORTED_PICTURE_FORMATS,
                    mPictureFormatValues);

    mParameters.set(QCameraParameters::KEY_JPEG_QUALITY, "90"); // max quality
    mJpegQuality = 90;
    //Set Video Format
    mParameters.set(QCameraParameters::KEY_VIDEO_FRAME_FORMAT, "yuv420sp");

    //Set Thumbnail parameters
    mParameters.set(QCameraParameters::KEY_JPEG_THUMBNAIL_WIDTH,
                    THUMBNAIL_WIDTH_STR); // informative
    mParameters.set(QCameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT,
                    THUMBNAIL_HEIGHT_STR); // informative
    mDimension.ui_thumbnail_width =
            thumbnail_sizes[DEFAULT_THUMBNAIL_SETTING].width;
    mDimension.ui_thumbnail_height =
            thumbnail_sizes[DEFAULT_THUMBNAIL_SETTING].height;
    mParameters.set(QCameraParameters::KEY_JPEG_THUMBNAIL_QUALITY, "90");
    String8 valuesStr = create_sizes_str(default_thumbnail_sizes, thumbnail_sizes_count);
    mParameters.set(QCameraParameters::KEY_SUPPORTED_JPEG_THUMBNAIL_SIZES,
                valuesStr.string());
    // Define CAMERA_SMOOTH_ZOOM in Android.mk file , to enable smoothzoom
#ifdef CAMERA_SMOOTH_ZOOM
    mParameters.set(QCameraParameters::KEY_SMOOTH_ZOOM_SUPPORTED, "true");
#endif
    if(mZoomSupported){
        mParameters.set(QCameraParameters::KEY_ZOOM_SUPPORTED, "true");
        ALOGV("max zoom is %d", mMaxZoom-1);
        /* mMaxZoom value that the query interface returns is the size
        ALOGV("max zoom is %d", mMaxZoom-1);
        * mMaxZoom value that the query interface returns is the size
         * of zoom table. So the actual max zoom value will be one
         * less than that value.          */

        mParameters.set("max-zoom",mMaxZoom-1);
        mParameters.set(QCameraParameters::KEY_ZOOM_RATIOS,
                            mZoomRatioValues);
    } else
        {
        mParameters.set(QCameraParameters::KEY_ZOOM_SUPPORTED, "false");
    }

    /* Enable zoom support for video application if VPE enabled */
    if(mZoomSupported) {
        mParameters.set("video-zoom-support", "true");
    } else {
        mParameters.set("video-zoom-support", "false");
    }

    //8960 supports Power modes : Low power, Normal Power.
    mParameters.set("power-mode-supported", "true");

    //Set Live shot support
    rc = cam_config_is_parm_supported(mCameraId, MM_CAMERA_PARM_LIVESHOT_MAIN);
    if(!rc) {
        ALOGV("%s:LIVESHOT is  not supported", __func__);
        mParameters.set("video-snapshot-supported", "false");
    } else {
        mParameters.set("video-snapshot-supported", "true");
    }


    //Set default power mode
    mParameters.set(QCameraParameters::KEY_POWER_MODE,"Low_Power");
    //Set Wnr on
    mParameters.set(QCameraParameters::KEY_DENOISE,true);
    //Set Camera Mode
    mParameters.set(QCameraParameters::KEY_CAMERA_MODE,1);
    mParameters.set(QCameraParameters::KEY_AE_BRACKET_HDR,"Off");

    //Set Antibanding
    mParameters.set(QCameraParameters::KEY_ANTIBANDING,
                    QCameraParameters::ANTIBANDING_AUTO);
    mParameters.set(QCameraParameters::KEY_SUPPORTED_ANTIBANDING,
                    mAntibandingValues);

    //Set Effect
    mParameters.set(QCameraParameters::KEY_EFFECT,
                    QCameraParameters::EFFECT_NONE);
    mParameters.set(QCameraParameters::KEY_SUPPORTED_EFFECTS, mEffectValues);

    //Set Auto Exposure
    mParameters.set(QCameraParameters::KEY_AUTO_EXPOSURE,
                    QCameraParameters::AUTO_EXPOSURE_CENTER_WEIGHTED);
    mParameters.set(QCameraParameters::KEY_SUPPORTED_AUTO_EXPOSURE, mAutoExposureValues);

    //Set WhiteBalance
    mParameters.set(QCameraParameters::KEY_WHITE_BALANCE,
                    QCameraParameters::WHITE_BALANCE_AUTO);
    mParameters.set(QCameraParameters::KEY_SUPPORTED_WHITE_BALANCE,mWhitebalanceValues);

    //Set AEC_LOCK
    mParameters.set(QCameraParameters::KEY_AUTO_EXPOSURE_LOCK, "false");
    if(cam_config_is_parm_supported(mCameraId, MM_CAMERA_PARM_AEC_LOCK)){
        mParameters.set(QCameraParameters::KEY_AUTO_EXPOSURE_LOCK_SUPPORTED, "true");
    } else {
        mParameters.set(QCameraParameters::KEY_AUTO_EXPOSURE_LOCK_SUPPORTED, "false");
    }
    //Set AWB_LOCK
    mParameters.set(QCameraParameters::KEY_AUTO_WHITEBALANCE_LOCK, "false");
    if(cam_config_is_parm_supported(mCameraId, MM_CAMERA_PARM_AWB_LOCK))
        mParameters.set(QCameraParameters::KEY_AUTO_WHITEBALANCE_LOCK_SUPPORTED, "true");
    else
        mParameters.set(QCameraParameters::KEY_AUTO_WHITEBALANCE_LOCK_SUPPORTED, "false");

    //Set Focus Mode
    if(mHasAutoFocusSupport){
       mParameters.set(QCameraParameters::KEY_FOCUS_MODE,
                          QCameraParameters::FOCUS_MODE_AUTO);
       mFocusMode = AF_MODE_AUTO;
       mParameters.set(QCameraParameters::KEY_SUPPORTED_FOCUS_MODES,
                          mFocusModeValues);
       mParameters.set(QCameraParameters::KEY_MAX_NUM_FOCUS_AREAS, "1");
       mParameters.set(QCameraParameters::KEY_MAX_NUM_METERING_AREAS, "1");
   } else {
       mParameters.set(QCameraParameters::KEY_FOCUS_MODE,
       QCameraParameters::FOCUS_MODE_FIXED);
       mFocusMode = DONT_CARE;
       mParameters.set(QCameraParameters::KEY_SUPPORTED_FOCUS_MODES,
       QCameraParameters::FOCUS_MODE_FIXED);
       mParameters.set(QCameraParameters::KEY_MAX_NUM_FOCUS_AREAS, "0");
       mParameters.set(QCameraParameters::KEY_MAX_NUM_METERING_AREAS, "0");
   }

    mParameters.set(QCameraParameters::KEY_FOCUS_AREAS, DEFAULT_CAMERA_AREA);
    mParameters.set(QCameraParameters::KEY_METERING_AREAS, DEFAULT_CAMERA_AREA);

    //Set Flash
    if (cam_config_is_parm_supported(mCameraId, MM_CAMERA_PARM_LED_MODE)) {
        mParameters.set(QCameraParameters::KEY_FLASH_MODE,
                        QCameraParameters::FLASH_MODE_OFF);
        mParameters.set(QCameraParameters::KEY_SUPPORTED_FLASH_MODES,
                        mFlashValues);
    }

    //Set Sharpness
    mParameters.set(QCameraParameters::KEY_MAX_SHARPNESS,
            CAMERA_MAX_SHARPNESS);
    mParameters.set(QCameraParameters::KEY_SHARPNESS,
                    CAMERA_DEF_SHARPNESS);

    //Set Contrast
    mParameters.set(QCameraParameters::KEY_MAX_CONTRAST,
            CAMERA_MAX_CONTRAST);
    mParameters.set(QCameraParameters::KEY_CONTRAST,
                    CAMERA_DEF_CONTRAST);

    //Set Saturation
    mParameters.set(QCameraParameters::KEY_MAX_SATURATION,
            CAMERA_MAX_SATURATION);
    mParameters.set(QCameraParameters::KEY_SATURATION,
                    CAMERA_DEF_SATURATION);

    //Set Brightness/luma-adaptaion
    mParameters.set("luma-adaptation", "3");

    mParameters.set(QCameraParameters::KEY_PICTURE_FORMAT,
                    QCameraParameters::PIXEL_FORMAT_JPEG);

    //Set Lensshading
    mParameters.set(QCameraParameters::KEY_LENSSHADE,
                    QCameraParameters::LENSSHADE_ENABLE);
    mParameters.set(QCameraParameters::KEY_SUPPORTED_LENSSHADE_MODES,
                    mLensShadeValues);

    //Set ISO Mode
    mParameters.set(QCameraParameters::KEY_ISO_MODE,
                    QCameraParameters::ISO_AUTO);
    mParameters.set(QCameraParameters::KEY_SUPPORTED_ISO_MODES,
                    mIsoValues);

    //Set MCE
    mParameters.set(QCameraParameters::KEY_MEMORY_COLOR_ENHANCEMENT,
                    QCameraParameters::MCE_ENABLE);
    mParameters.set(QCameraParameters::KEY_SUPPORTED_MEM_COLOR_ENHANCE_MODES,
                    mMceValues);
    //Set HFR
    if (cam_config_is_parm_supported(mCameraId, MM_CAMERA_PARM_HFR)) {
        mParameters.set(QCameraParameters::KEY_VIDEO_HIGH_FRAME_RATE,
                    QCameraParameters::VIDEO_HFR_OFF);
        mParameters.set(QCameraParameters::KEY_SUPPORTED_HFR_SIZES,
                    mHfrSizeValues.string());
        mParameters.set(QCameraParameters::KEY_SUPPORTED_VIDEO_HIGH_FRAME_RATE_MODES,
                    mHfrValues);
    } else{
        mParameters.set(QCameraParameters::KEY_SUPPORTED_HFR_SIZES,"");
    }

    //Set Histogram
    mParameters.set(QCameraParameters::KEY_HISTOGRAM,
                    QCameraParameters::HISTOGRAM_DISABLE);
    mParameters.set(QCameraParameters::KEY_SUPPORTED_HISTOGRAM_MODES,
                    mHistogramValues);

    //Set SkinTone Enhancement
    mParameters.set(QCameraParameters::KEY_SKIN_TONE_ENHANCEMENT,
                    QCameraParameters::SKIN_TONE_ENHANCEMENT_DISABLE);
    mParameters.set("skinToneEnhancement", "0");
    mParameters.set(QCameraParameters::KEY_SUPPORTED_SKIN_TONE_ENHANCEMENT_MODES,
                    mSkinToneEnhancementValues);

    //Set Scene Mode
    mParameters.set(QCameraParameters::KEY_SCENE_MODE,
                    QCameraParameters::SCENE_MODE_AUTO);
    mParameters.set(QCameraParameters::KEY_SUPPORTED_SCENE_MODES,
                    mSceneModeValues);

    //Set Streaming Textures
    mParameters.set("strtextures", "OFF");

    //Set Denoise
    mParameters.set(QCameraParameters::KEY_DENOISE,
                    QCameraParameters::DENOISE_ON);
    mParameters.set(QCameraParameters::KEY_SUPPORTED_DENOISE,
                        denoise_value);
    //Set Touch AF/AEC
    mParameters.set(QCameraParameters::KEY_TOUCH_AF_AEC,
                    QCameraParameters::TOUCH_AF_AEC_OFF);
    mParameters.set(QCameraParameters::KEY_SUPPORTED_TOUCH_AF_AEC,
                    mTouchAfAecValues);
    /* touch-AF ROI for reducing af fail case */
    mParameters.set("touchAfAec-dx","200");
    mParameters.set("touchAfAec-dy","200");

    //Set Scene Detection
    mParameters.set(QCameraParameters::KEY_SCENE_DETECT,
                   QCameraParameters::SCENE_DETECT_OFF);
    mParameters.set(QCameraParameters::KEY_SUPPORTED_SCENE_DETECT,
                    mSceneDetectValues);

    //Set Selectable Zone AF
    mParameters.set(QCameraParameters::KEY_SELECTABLE_ZONE_AF,
                    QCameraParameters::SELECTABLE_ZONE_AF_AUTO);
    mParameters.set(QCameraParameters::KEY_SUPPORTED_SELECTABLE_ZONE_AF,
                    mSelectableZoneAfValues);

    //Set Face Detection
    if(supportsFaceDetection()){
        mParameters.set(QCameraParameters::KEY_FACE_DETECTION,
                        QCameraParameters::FACE_DETECTION_OFF);
        mParameters.set(QCameraParameters::KEY_SUPPORTED_FACE_DETECTION,
                        mFaceDetectionValues);
    }

    //Set Red Eye Reduction
    mParameters.set(QCameraParameters::KEY_REDEYE_REDUCTION,
                    QCameraParameters::REDEYE_REDUCTION_DISABLE);
    mParameters.set(QCameraParameters::KEY_SUPPORTED_REDEYE_REDUCTION,
                    mRedeyeReductionValues);

    //Set ZSL
    mParameters.set(QCameraParameters::KEY_ZSL,
                    QCameraParameters::ZSL_OFF);
    mParameters.set(QCameraParameters::KEY_SUPPORTED_ZSL_MODES,
                    mZslValues);

    //Set Focal length, horizontal and vertical view angles
    focus_distances_info_t focalLength;
    float horizontalViewAngle = 0.0f;
    float verticalViewAngle = 0.0f;
    cam_config_get_parm(mCameraId, MM_CAMERA_PARM_FOCAL_LENGTH,
            (void *)&focalLength);
    mParameters.setFloat(QCameraParameters::KEY_FOCAL_LENGTH,
                    focalLength.focus_distance[0]);
    cam_config_get_parm(mCameraId, MM_CAMERA_PARM_HORIZONTAL_VIEW_ANGLE,
            (void *)&horizontalViewAngle);
    mParameters.setFloat(QCameraParameters::KEY_HORIZONTAL_VIEW_ANGLE,
                    horizontalViewAngle);
    cam_config_get_parm(mCameraId, MM_CAMERA_PARM_VERTICAL_VIEW_ANGLE,
            (void *)&verticalViewAngle);
    mParameters.setFloat(QCameraParameters::KEY_VERTICAL_VIEW_ANGLE,
                    verticalViewAngle);

    //Set Aperture
    float f_number = 0.0f;
    cam_config_get_parm(mCameraId, MM_CAMERA_PARM_F_NUMBER,
            (void *)&f_number);
    mExifValues.f_number = getRational(f_number*F_NUMBER_DECIMAL_PRECISION, F_NUMBER_DECIMAL_PRECISION);

    //Set Exposure Compensation
    mParameters.set(
            QCameraParameters::KEY_MAX_EXPOSURE_COMPENSATION,
            EXPOSURE_COMPENSATION_MAXIMUM_NUMERATOR);
    mParameters.set(
            QCameraParameters::KEY_MIN_EXPOSURE_COMPENSATION,
            EXPOSURE_COMPENSATION_MINIMUM_NUMERATOR);
    mParameters.set(
            QCameraParameters::KEY_EXPOSURE_COMPENSATION,
            EXPOSURE_COMPENSATION_DEFAULT_NUMERATOR);
    mParameters.setFloat(
            QCameraParameters::KEY_EXPOSURE_COMPENSATION_STEP,
            EXPOSURE_COMPENSATION_STEP);

    mParameters.set("num-snaps-per-shutter", 1);

    mParameters.set("capture-burst-captures-values", getZSLQueueDepth());
    mParameters.set("capture-burst-interval-supported", "true");
    mParameters.set("capture-burst-interval-max", BURST_INTREVAL_MAX); /*skip frames*/
    mParameters.set("capture-burst-interval-min", BURST_INTREVAL_MIN); /*skip frames*/
    mParameters.set("capture-burst-interval", BURST_INTREVAL_DEFAULT); /*skip frames*/
    mParameters.set("capture-burst-retroactive", 0);
    mParameters.set("capture-burst-retroactive-max", getZSLQueueDepth());
    mParameters.set("capture-burst-exposures", "");
    mParameters.set("capture-burst-exposures-values",
      "-12,-11,-10,-9,-8,-7,-6,-5,-4,-3,-2,-1,0,1,2,3,4,5,6,7,8,9,10,11,12");
    {
      String8 CamModeStr;
      char buffer[32];
      int flag = 0;

      for (int i = 0; i < HAL_CAM_MODE_MAX; i++) {
        if ( 0 ) { /*exclude some conflicting case*/
        } else {
          if (flag == 0) { /*first item*/
          snprintf(buffer, sizeof(buffer), "%d", i);
          } else {
            snprintf(buffer, sizeof(buffer), ",%d", i);
          }
          flag = 1;
          CamModeStr.append(buffer);
        }
      }
      mParameters.set("camera-mode-values", CamModeStr);
    }

    mParameters.set("ae-bracket-hdr-values",
      create_values_str(hdr_bracket, sizeof(hdr_bracket)/sizeof(str_map) ));

// if(mIs3DModeOn)
//     mParameters.set("3d-frame-format", "left-right");
    mParameters.set("no-display-mode", 0);
    //mUseOverlay = useOverlay();
    mParameters.set("zoom", 0);

    int mNuberOfVFEOutputs;
    ret = cam_config_get_parm(mCameraId, MM_CAMERA_PARM_VFE_OUTPUT_ENABLE, &mNuberOfVFEOutputs);
    if(ret != MM_CAMERA_OK) {
        ALOGE("get parm MM_CAMERA_PARM_VFE_OUTPUT_ENABLE  failed");
        ret = BAD_VALUE;
    }
    if(mNuberOfVFEOutputs == 1)
    {
       mParameters.set(QCameraParameters::KEY_SINGLE_ISP_OUTPUT_ENABLED, "true");
    } else {
       mParameters.set(QCameraParameters::KEY_SINGLE_ISP_OUTPUT_ENABLED, "false");
    }

    if (setParameters(mParameters) != NO_ERROR) {
        ALOGE("Failed to set default parameters?!");
    }

    mNoDisplayMode = 0;
    mLedStatusForZsl = LED_MODE_OFF;

    mInitialized = true;
    strTexturesOn = false;

    ALOGV("%s: X", __func__);
    return;
}

/**
 * Set the camera parameters. This returns BAD_VALUE if any parameter is
 * invalid or not supported.
 */

int QCameraHardwareInterface::setParameters(const char *parms)
{
    QCameraParameters param;
    String8 str = String8(parms);
    param.unflatten(str);
    status_t ret = setParameters(param);
	if(ret == NO_ERROR)
		return 0;
	else
		return -1;
}

/**
 * Set the camera parameters. This returns BAD_VALUE if any parameter is
 * invalid or not supported. */
status_t QCameraHardwareInterface::setParameters(const QCameraParameters& params)
{
    status_t ret = NO_ERROR;

    ALOGV("%s: E", __func__);
//    Mutex::Autolock l(&mLock);
    status_t rc, final_rc = NO_ERROR;

    if ((rc = setPowerMode(params)))                    final_rc = rc;
    if ((rc = setPreviewSize(params)))                  final_rc = rc;
    if ((rc = setVideoSize(params)))                    final_rc = rc;
    if ((rc = setPictureSize(params)))                  final_rc = rc;
    if ((rc = setJpegThumbnailSize(params)))            final_rc = rc;
    if ((rc = setJpegQuality(params)))                  final_rc = rc;
    if ((rc = setEffect(params)))                       final_rc = rc;
    if ((rc = setGpsLocation(params)))                  final_rc = rc;
    if ((rc = setRotation(params)))                     final_rc = rc;
    if ((rc = setZoom(params)))                         final_rc = rc;
    if ((rc = setOrientation(params)))                  final_rc = rc;
    if ((rc = setLensshadeValue(params)))               final_rc = rc;
    if ((rc = setMCEValue(params)))                     final_rc = rc;
    if ((rc = setPictureFormat(params)))                final_rc = rc;
    if ((rc = setSharpness(params)))                    final_rc = rc;
    if ((rc = setSaturation(params)))                   final_rc = rc;
    if ((rc = setSceneMode(params)))                    final_rc = rc;
    if ((rc = setContrast(params)))                     final_rc = rc;
//    if ((rc = setFaceDetect(params)))                   final_rc = rc;
    if ((rc = setStrTextures(params)))                  final_rc = rc;
    if ((rc = setPreviewFormat(params)))                final_rc = rc;
    if ((rc = setSkinToneEnhancement(params)))          final_rc = rc;
    if ((rc = setWaveletDenoise(params)))               final_rc = rc;
    if ((rc = setAntibanding(params)))                  final_rc = rc;
    //    if ((rc = setOverlayFormats(params)))         final_rc = rc;
    if ((rc = setRedeyeReduction(params)))              final_rc = rc;
    if ((rc = setCaptureBurstExp()))                    final_rc = rc;

    const char *str_val = params.get("capture-burst-exposures");
    if ( str_val == NULL || strlen(str_val)==0 ) {
        char burst_exp[PROPERTY_VALUE_MAX];
        memset(burst_exp, 0, sizeof(burst_exp));
        property_get("persist.capture.burst.exposures", burst_exp, "");
        if ( strlen(burst_exp)>0 ) {
            mParameters.set("capture-burst-exposures", burst_exp);
        }
    } else {
      mParameters.set("capture-burst-exposures", str_val);
    }

    if ((rc = setAEBracket(params)))              final_rc = rc;
    //    if ((rc = setDenoise(params)))                final_rc = rc;
    if ((rc = setPreviewFpsRange(params)))              final_rc = rc;
    if((rc = setRecordingHint(params)))                 final_rc = rc;
    if ((rc = setNumOfSnapshot()))                      final_rc = rc;
    if ((rc = setAecAwbLock(params)))                   final_rc = rc;
    if ((rc = setWhiteBalance(params)))                 final_rc = rc;
    const char *str = params.get(QCameraParameters::KEY_SCENE_MODE);
    int32_t value = attr_lookup(scenemode, sizeof(scenemode) / sizeof(str_map), str);

    if((value != NOT_FOUND) && (value == CAMERA_BESTSHOT_OFF )) {
        //if ((rc = setPreviewFrameRateMode(params)))     final_rc = rc;
        if ((rc = setPreviewFrameRate(params)))         final_rc = rc;
        if ((rc = setBrightness(params)))               final_rc = rc;
        if ((rc = setISOValue(params)))                 final_rc = rc;
        if ((rc = setFocusAreas(params)))               final_rc = rc;
        if ((rc = setMeteringAreas(params)))            final_rc = rc;
    }
    if ((rc = setFocusMode(params)))                    final_rc = rc;
    if ((rc = setAutoExposure(params)))                 final_rc = rc;
    if ((rc = setExposureCompensation(params)))         final_rc = rc;
    if ((rc = setFlash(params)))                        final_rc = rc;
    //selectableZoneAF needs to be invoked after continuous AF
    if ((rc = setSelectableZoneAf(params)))             final_rc = rc;
    // setHighFrameRate needs to be done at end, as there can
    // be a preview restart, and need to use the updated parameters
    if ((rc = setHighFrameRate(params)))  final_rc = rc;
    if ((rc = setZSLBurstLookBack(params))) final_rc = rc;
    if ((rc = setZSLBurstInterval(params))) final_rc = rc;
    if ((rc = setNoDisplayMode(params))) final_rc = rc;

    //Update Exiftag values.
    setExifTags();

   ALOGV("%s: X", __func__);
   return final_rc;
}

/** Retrieve the camera parameters.  The buffer returned by the camera HAL
	must be returned back to it with put_parameters, if put_parameters
	is not NULL.
 */
int QCameraHardwareInterface::getParameters(char **parms)
{
    char* rc = NULL;
    String8 str;
    QCameraParameters param = getParameters();
    //param.dump();
    str = param.flatten( );
    rc = (char *)malloc(sizeof(char)*(str.length()+1));
    if(rc != NULL){
        memset(rc, 0, sizeof(char)*(str.length()+1));
        strncpy(rc, str.string(), str.length());
	rc[str.length()] = 0;
	*parms = rc;
    }
    return 0;
}

/** The camera HAL uses its own memory to pass us the parameters when we
	call get_parameters.  Use this function to return the memory back to
	the camera HAL, if put_parameters is not NULL.  If put_parameters
	is NULL, then you have to use free() to release the memory.
*/
void QCameraHardwareInterface::putParameters(char *rc)
{
    free(rc);
    rc = NULL;
}

QCameraParameters& QCameraHardwareInterface::getParameters()
{
    Mutex::Autolock lock(mLock);
    mParameters.set(QCameraParameters::KEY_FOCUS_DISTANCES, mFocusDistance.string());
    const char *str = mParameters.get(QCameraParameters::KEY_SCENE_MODE);
    if (mHasAutoFocusSupport && strcmp(str, "auto")) {
        mParameters.set(QCameraParameters::KEY_FOCUS_MODE,
                                        QCameraParameters::FOCUS_MODE_CONTINUOUS_PICTURE);
    }
    return mParameters;
}

status_t QCameraHardwareInterface::runFaceDetection()
{
    bool ret = true;

    const char *str = mParameters.get(QCameraParameters::KEY_FACE_DETECTION);
    if (str != NULL) {
        int value = attr_lookup(facedetection,
                sizeof(facedetection) / sizeof(str_map), str);
        fd_set_parm_t fd_set_parm;
        int requested_faces = mParameters.getInt(QCameraParameters::KEY_MAX_NUM_REQUESTED_FACES);
        fd_set_parm.fd_mode = value;
        fd_set_parm.num_fd = requested_faces;
        ret = native_set_parms(MM_CAMERA_PARM_FD, sizeof(fd_set_parm_t), (void *)&fd_set_parm);
        return ret ? NO_ERROR : UNKNOWN_ERROR;
    }
    ALOGE("Invalid Face Detection value: %s", (str == NULL) ? "NULL" : str);
    return BAD_VALUE;
}

status_t QCameraHardwareInterface::setSharpness(const QCameraParameters& params)
{
    bool ret = false;
    int rc = MM_CAMERA_OK;
    ALOGV("%s",__func__);
    rc = cam_config_is_parm_supported(mCameraId, MM_CAMERA_PARM_SHARPNESS);
    if(!rc) {
        ALOGV("%s:CONTRAST not supported", __func__);
        return NO_ERROR;
    }
    int sharpness = params.getInt(QCameraParameters::KEY_SHARPNESS);
    if((sharpness < CAMERA_MIN_SHARPNESS
            || sharpness > CAMERA_MAX_SHARPNESS))
        return UNKNOWN_ERROR;

    ALOGV("setting sharpness %d", sharpness);
    mParameters.set(QCameraParameters::KEY_SHARPNESS, sharpness);
    ret = native_set_parms(MM_CAMERA_PARM_SHARPNESS, sizeof(sharpness),
                               (void *)&sharpness);
    return ret ? NO_ERROR : UNKNOWN_ERROR;
}

status_t QCameraHardwareInterface::setSaturation(const QCameraParameters& params)
{
    bool ret = false;
    int rc = MM_CAMERA_OK;
    ALOGV("%s",__func__);
    rc = cam_config_is_parm_supported(mCameraId, MM_CAMERA_PARM_SATURATION);
    if(!rc) {
        ALOGV("%s:MM_CAMERA_PARM_SATURATION not supported", __func__);
        return NO_ERROR;
    }
    int result;
    int saturation = params.getInt(QCameraParameters::KEY_SATURATION);

    if((saturation < CAMERA_MIN_SATURATION)
        || (saturation > CAMERA_MAX_SATURATION))
    return UNKNOWN_ERROR;

    ALOGV("Setting saturation %d", saturation);
    mParameters.set(QCameraParameters::KEY_SATURATION, saturation);
    ret = native_set_parms(MM_CAMERA_PARM_SATURATION, sizeof(saturation),
        (void *)&saturation, (int *)&result);
    if(result != MM_CAMERA_OK)
        ALOGV("Saturation Value: %d is not set as the selected value is not supported", saturation);
    return ret ? NO_ERROR : UNKNOWN_ERROR;
}

status_t QCameraHardwareInterface::setContrast(const QCameraParameters& params)
{
   ALOGV("%s E", __func__ );
   int rc = MM_CAMERA_OK;
   rc = cam_config_is_parm_supported(mCameraId, MM_CAMERA_PARM_CONTRAST);
   if(!rc) {
        ALOGV("%s:CONTRAST not supported", __func__);
        return NO_ERROR;
    }
   const char *str = params.get(QCameraParameters::KEY_SCENE_MODE);
   ALOGV("Contrast : %s",str);
   int32_t value = attr_lookup(scenemode, sizeof(scenemode) / sizeof(str_map), str);
   if(value == CAMERA_BESTSHOT_OFF) {
        int contrast = params.getInt(QCameraParameters::KEY_CONTRAST);
        if((contrast < CAMERA_MIN_CONTRAST)
                || (contrast > CAMERA_MAX_CONTRAST))
        {
            ALOGV("Contrast Value not matching");
            return UNKNOWN_ERROR;
        }
        ALOGV("setting contrast %d", contrast);
        mParameters.set(QCameraParameters::KEY_CONTRAST, contrast);
        ALOGV("Calling Contrast set on Lower layer");
        bool ret = native_set_parms(MM_CAMERA_PARM_CONTRAST, sizeof(contrast),
                                   (void *)&contrast);
        ALOGV("Lower layer returned %d", ret);
        int bestshot_reconfigure;
        cam_config_get_parm(mCameraId, MM_CAMERA_PARM_BESTSHOT_RECONFIGURE,
                            &bestshot_reconfigure);
        if(bestshot_reconfigure) {
             if (mContrast != contrast) {
                  mContrast = contrast;
                 if (mPreviewState == QCAMERA_HAL_PREVIEW_STARTED && ret) {
                      mRestartPreview = 1;
                      pausePreviewForZSL();
                  }
             }
        }
        return ret ? NO_ERROR : UNKNOWN_ERROR;
    } else {
          ALOGV(" Contrast value will not be set " \
          "when the scenemode selected is %s", str);
          return NO_ERROR;
    }
    return BAD_VALUE;
}

status_t QCameraHardwareInterface::setSceneDetect(const QCameraParameters& params)
{
    ALOGV("%s",__func__);
    bool retParm;
    int rc = MM_CAMERA_OK;

    rc = cam_config_is_parm_supported(mCameraId,MM_CAMERA_PARM_ASD_ENABLE);
    if(!rc) {
        ALOGV("%s:MM_CAMERA_PARM_ASD_ENABLE not supported", __func__);
        return NO_ERROR;
    }

    const char *str = params.get(QCameraParameters::KEY_SCENE_DETECT);
    ALOGV("Scene Detect string : %s",str);
    if (str != NULL) {
        int32_t value = attr_lookup(scenedetect, sizeof(scenedetect) / sizeof(str_map), str);
        ALOGV("Scenedetect Value : %d",value);
        if (value != NOT_FOUND) {
            mParameters.set(QCameraParameters::KEY_SCENE_DETECT, str);

            retParm = native_set_parms(MM_CAMERA_PARM_ASD_ENABLE, sizeof(value),
                                       (void *)&value);

            return retParm ? NO_ERROR : UNKNOWN_ERROR;
        }
    }
   return BAD_VALUE;
}

status_t QCameraHardwareInterface::setZoom(const QCameraParameters& params)
{
    status_t rc = NO_ERROR;

    ALOGV("%s: E",__func__);


    if( !( cam_config_is_parm_supported(mCameraId, MM_CAMERA_PARM_ZOOM))) {
        ALOGV("%s:MM_CAMERA_PARM_ZOOM not supported", __func__);
        return NO_ERROR;
    }
    // No matter how many different zoom values the driver can provide, HAL
    // provides applictations the same number of zoom levels. The maximum driver
    // zoom value depends on sensor output (VFE input) and preview size (VFE
    // output) because VFE can only crop and cannot upscale. If the preview size
    // is bigger, the maximum zoom ratio is smaller. However, we want the
    // zoom ratio of each zoom level is always the same whatever the preview
    // size is. Ex: zoom level 1 is always 1.2x, zoom level 2 is 1.44x, etc. So,
    // we need to have a fixed maximum zoom value and do read it from the
    // driver.
    static const int ZOOM_STEP = 1;
    int32_t zoom_level = params.getInt("zoom");
    if(zoom_level >= 0 && zoom_level <= mMaxZoom-1) {
        mParameters.set("zoom", zoom_level);
        int32_t zoom_value = ZOOM_STEP * zoom_level;
        bool ret = native_set_parms(MM_CAMERA_PARM_ZOOM,
            sizeof(zoom_value), (void *)&zoom_value);
        if(ret) {
            mCurrentZoom=zoom_level;
        }
        rc = ret ? NO_ERROR : UNKNOWN_ERROR;
    } else {
        rc = BAD_VALUE;
    }
    ALOGV("%s X",__func__);
    return rc;

}

status_t  QCameraHardwareInterface::setISOValue(const QCameraParameters& params) {

    status_t rc = NO_ERROR;
    ALOGV("%s",__func__);

    rc = cam_config_is_parm_supported(mCameraId, MM_CAMERA_PARM_ISO);
    if(!rc) {
        ALOGV("%s:MM_CAMERA_PARM_ISO not supported", __func__);
        return NO_ERROR;
    }
    const char *str = params.get(QCameraParameters::KEY_ISO_MODE);
    ALOGV("ISO string : %s", str);
    int8_t temp_hjr;
    if (str != NULL) {
        int value = (camera_iso_mode_type)attr_lookup(
          iso, sizeof(iso) / sizeof(str_map), str);
        ALOGV("ISO string : %s", str);
        if (value != NOT_FOUND) {
            camera_iso_mode_type temp = (camera_iso_mode_type) value;
            if (value == CAMERA_ISO_DEBLUR) {
               temp_hjr = true;
               native_set_parms(MM_CAMERA_PARM_HJR, sizeof(int8_t), (void*)&temp_hjr);
               mHJR = value;
            }
            else {
               if (mHJR == CAMERA_ISO_DEBLUR) {
                   temp_hjr = false;
                   native_set_parms(MM_CAMERA_PARM_HJR, sizeof(int8_t), (void*)&temp_hjr);
                   mHJR = value;
               }
            }

            mParameters.set(QCameraParameters::KEY_ISO_MODE, str);
            native_set_parms(MM_CAMERA_PARM_ISO, sizeof(camera_iso_mode_type), (void *)&temp);
            mIsoValue = (int)temp;
            return NO_ERROR;
        }
    }
    return BAD_VALUE;
}

status_t QCameraHardwareInterface::updateFocusDistances()
{
    ALOGV("%s: IN", __FUNCTION__);
    focus_distances_info_t focusDistances;
    if(cam_config_get_parm(mCameraId, MM_CAMERA_PARM_FOCUS_DISTANCES,
      &focusDistances) == MM_CAMERA_OK) {
        String8 str;
        char buffer[32] = {0};
        //set all distances to infinity if focus mode is infinity
        if(mFocusMode == AF_MODE_INFINITY) {
            snprintf(buffer, sizeof(buffer), "Infinity,");
            str.append(buffer);
            snprintf(buffer, sizeof(buffer), "Infinity,");
            str.append(buffer);
            snprintf(buffer, sizeof(buffer), "Infinity");
            str.append(buffer);
        } else {
            snprintf(buffer, sizeof(buffer), "%f", focusDistances.focus_distance[0]);
            str.append(buffer);
            snprintf(buffer, sizeof(buffer), ",%f", focusDistances.focus_distance[1]);
            str.append(buffer);
            snprintf(buffer, sizeof(buffer), ",%f", focusDistances.focus_distance[2]);
            str.append(buffer);
        }
        ALOGV("%s: setting KEY_FOCUS_DISTANCES as %s", __FUNCTION__, str.string());
        mFocusDistance = str;
        return NO_ERROR;
    }
    ALOGE("%s: get CAMERA_PARM_FOCUS_DISTANCES failed!!!", __FUNCTION__);
    return BAD_VALUE;
}

// Parse string like "(1, 2, 3, 4, ..., N)"
// num is pointer to an allocated array of size N
static int parseNDimVector(const char *str, int *num, int N, char delim = ',')
{
    char *start, *end;
    if(num == NULL) {
        ALOGE("Invalid output array (num == NULL)");
        return -1;
    }
    //check if string starts and ends with parantheses
    if(str[0] != '(' || str[strlen(str)-1] != ')') {
        ALOGE("Invalid format of string %s, valid format is (n1, n2, n3, n4 ...)", str);
        return -1;
    }
    start = (char*) str;
    start++;
    for(int i=0; i<N; i++) {
        *(num+i) = (int) strtol(start, &end, 10);
        if(*end != delim && i < N-1) {
            ALOGE("Cannot find delimeter '%c' in string \"%s\". end = %c", delim, str, *end);
            return -1;
        }
        start = end+1;
    }
    return 0;
}

// parse string like "(1, 2, 3, 4, 5),(1, 2, 3, 4, 5),..."
static int parseCameraAreaString(const char* str, int max_num_areas,
                                 camera_area_t *pAreas, int *num_areas_found)
{
    char area_str[32];
    const char *start, *end, *p;
    start = str; end = NULL;
    int values[5], index=0;
    *num_areas_found = 0;

    while(start != NULL) {
       if(*start != '(') {
            ALOGE("%s: error: Ill formatted area string: %s", __func__, str);
            return -1;
       }
       end = strchr(start, ')');
       if(end == NULL) {
            ALOGE("%s: error: Ill formatted area string: %s", __func__, str);
            return -1;
       }
       int i;
       for (i=0,p=start; p<=end; p++, i++) {
           area_str[i] = *p;
       }
       area_str[i] = '\0';
       if(parseNDimVector(area_str, values, 5) < 0){
            ALOGE("%s: error: Failed to parse the area string: %s", __func__, area_str);
            return -1;
       }
       // no more areas than max_num_areas are accepted.
       if(index >= max_num_areas) {
            ALOGE("%s: error: too many areas specified %s", __func__, str);
            return -1;
       }
       pAreas[index].x1 = values[0];
       pAreas[index].y1 = values[1];
       pAreas[index].x2 = values[2];
       pAreas[index].y2 = values[3];
       pAreas[index].weight = values[4];

       index++;
       start = strchr(end, '('); // serach for next '('
    }
    (*num_areas_found) = index;
    return 0;
}
static bool validateCameraAreas(camera_area_t *areas, int num_areas)
{
    for(int i=0; i<num_areas; i++) {

        // handle special case (0, 0, 0, 0, 0)
        if((areas[i].x1 == 0) && (areas[i].y1 == 0)
            && (areas[i].x2 == 0) && (areas[i].y2 == 0) && (areas[i].weight == 0)) {
            continue;
        }
        if(areas[i].x1 < -1000) return false;               // left should be >= -1000
        if(areas[i].y1 < -1000) return false;               // top  should be >= -1000
        if(areas[i].x2 > 1000) return false;                // right  should be <= 1000
        if(areas[i].y2 > 1000) return false;                // bottom should be <= 1000
        if(areas[i].weight <= 0 || areas[i].weight > 1000)  // weight should be in [1, 1000]
            return false;
        if(areas[i].x1 >= areas[i].x2) {                    // left should be < right
            return false;
        }
        if(areas[i].y1 >= areas[i].y2)                      // top should be < bottom
            return false;
    }
    return true;
}

status_t QCameraHardwareInterface::setFocusAreas(const QCameraParameters& params)
{
    ALOGV("%s: E", __func__);
    status_t rc;
    int max_num_af_areas = mParameters.getInt(QCameraParameters::KEY_MAX_NUM_FOCUS_AREAS);
    if(max_num_af_areas == 0) {
        return NO_ERROR;
    }
    const char *str = params.get(QCameraParameters::KEY_FOCUS_AREAS);
    if (str == NULL) {
        ALOGE("%s: Parameter string is null", __func__);
        rc = NO_ERROR;
    } else {
        camera_area_t *areas = new camera_area_t[max_num_af_areas];
        int num_areas_found=0;
        if(parseCameraAreaString(str, max_num_af_areas, areas, &num_areas_found) < 0) {
            ALOGE("%s: Failed to parse the string: %s", __func__, str);
            delete areas;
            return BAD_VALUE;
        }
        for(int i=0; i<num_areas_found; i++) {
            ALOGV("FocusArea[%d] = (%d, %d, %d, %d, %d)", i, (areas[i].x1), (areas[i].y1),
                        (areas[i].x2), (areas[i].y2), (areas[i].weight));
        }
        if(validateCameraAreas(areas, num_areas_found) == false) {
            ALOGE("%s: invalid areas specified : %s", __func__, str);
            delete areas;
            return BAD_VALUE;
        }
        mParameters.set(QCameraParameters::KEY_FOCUS_AREAS, str);
        num_areas_found = 1; //temp; need to change after the multi-roi is enabled

        //if the native_set_parms is called when preview is not started, it
        //crashes in lower layer, so return of preview is not started
        if(mPreviewState == QCAMERA_HAL_PREVIEW_STOPPED) {
            delete areas;
            return NO_ERROR;
        }

        //for special area string (0, 0, 0, 0, 0), set the num_areas_found to 0,
        //so no action is takenby the lower layer
        if(num_areas_found == 1 && (areas[0].x1 == 0) && (areas[0].y1 == 0)
            && (areas[0].x2 == 0) && (areas[0].y2 == 0) && (areas[0].weight == 0)) {
            num_areas_found = 0;
        }
#if 1 //temp solution

        roi_info_t af_roi_value;
        memset(&af_roi_value, 0, sizeof(roi_info_t));
        uint16_t x1, x2, y1, y2, dx, dy;
        int previewWidth, previewHeight;
        this->getPreviewSize(&previewWidth, &previewHeight);
        //transform the coords from (-1000, 1000) to (0, previewWidth or previewHeight)
        x1 = (uint16_t)((areas[0].x1 + 1000.0f)*(previewWidth/2000.0f));
        y1 = (uint16_t)((areas[0].y1 + 1000.0f)*(previewHeight/2000.0f));
        x2 = (uint16_t)((areas[0].x2 + 1000.0f)*(previewWidth/2000.0f));
        y2 = (uint16_t)((areas[0].y2 + 1000.0f)*(previewHeight/2000.0f));
        dx = x2 - x1;
        dy = y2 - y1;

        af_roi_value.num_roi = num_areas_found;
        af_roi_value.roi[0].x = x1;
        af_roi_value.roi[0].y = y1;
        af_roi_value.roi[0].dx = dx;
        af_roi_value.roi[0].dy = dy;
        af_roi_value.is_multiwindow = 0;
        if (native_set_parms(MM_CAMERA_PARM_AF_ROI, sizeof(roi_info_t), (void*)&af_roi_value))
            rc = NO_ERROR;
        else
            rc = BAD_VALUE;
        delete areas;
#endif
#if 0   //better solution with multi-roi, to be enabled later
        af_mtr_area_t afArea;
        afArea.num_area = num_areas_found;

        uint16_t x1, x2, y1, y2, dx, dy;
        int previewWidth, previewHeight;
        this->getPreviewSize(&previewWidth, &previewHeight);

        for(int i=0; i<num_areas_found; i++) {
            //transform the coords from (-1000, 1000) to (0, previewWidth or previewHeight)
            x1 = (uint16_t)((areas[i].x1 + 1000.0f)*(previewWidth/2000.0f));
            y1 = (uint16_t)((areas[i].y1 + 1000.0f)*(previewHeight/2000.0f));
            x2 = (uint16_t)((areas[i].x2 + 1000.0f)*(previewWidth/2000.0f));
            y2 = (uint16_t)((areas[i].y2 + 1000.0f)*(previewHeight/2000.0f));
            dx = x2 - x1;
            dy = y2 - y1;
            afArea.mtr_area[i].x = x1;
            afArea.mtr_area[i].y = y1;
            afArea.mtr_area[i].dx = dx;
            afArea.mtr_area[i].dy = dy;
            afArea.weight[i] = areas[i].weight;
        }

        if(native_set_parms(MM_CAMERA_PARM_AF_MTR_AREA, sizeof(af_mtr_area_t), (void*)&afArea))
            rc = NO_ERROR;
        else
            rc = BAD_VALUE;*/
#endif
    }
    ALOGV("%s: X", __func__);
    return rc;
}

status_t QCameraHardwareInterface::setMeteringAreas(const QCameraParameters& params)
{
    ALOGV("%s: E", __func__);
    status_t rc;
    int max_num_mtr_areas = mParameters.getInt(QCameraParameters::KEY_MAX_NUM_METERING_AREAS);
    if(max_num_mtr_areas == 0) {
        return NO_ERROR;
    }

    const char *str = params.get(QCameraParameters::KEY_METERING_AREAS);
    if (str == NULL) {
        ALOGE("%s: Parameter string is null", __func__);
        rc = NO_ERROR;
    } else {
        camera_area_t *areas = new camera_area_t[max_num_mtr_areas];
        int num_areas_found=0;
        if(parseCameraAreaString(str, max_num_mtr_areas, areas, &num_areas_found) < 0) {
            ALOGE("%s: Failed to parse the string: %s", __func__, str);
            delete areas;
            return BAD_VALUE;
        }
        for(int i=0; i<num_areas_found; i++) {
            ALOGV("MeteringArea[%d] = (%d, %d, %d, %d, %d)", i, (areas[i].x1), (areas[i].y1),
                        (areas[i].x2), (areas[i].y2), (areas[i].weight));
        }
        if(validateCameraAreas(areas, num_areas_found) == false) {
            ALOGE("%s: invalid areas specified : %s", __func__, str);
            delete areas;
            return BAD_VALUE;
        }
        mParameters.set(QCameraParameters::KEY_METERING_AREAS, str);

        //if the native_set_parms is called when preview is not started, it
        //crashes in lower layer, so return of preview is not started
        if(mPreviewState == QCAMERA_HAL_PREVIEW_STOPPED) {
            delete areas;
            return NO_ERROR;
        }

        num_areas_found = 1; //temp; need to change after the multi-roi is enabled

        //for special area string (0, 0, 0, 0, 0), set the num_areas_found to 0,
        //so no action is takenby the lower layer
        if(num_areas_found == 1 && (areas[0].x1 == 0) && (areas[0].y1 == 0)
             && (areas[0].x2 == 0) && (areas[0].y2 == 0) && (areas[0].weight == 0)) {
            num_areas_found = 0;
        }
#if 1
        cam_set_aec_roi_t aec_roi_value;
        uint16_t x1, x2, y1, y2;
        int previewWidth, previewHeight;
        this->getPreviewSize(&previewWidth, &previewHeight);
        //transform the coords from (-1000, 1000) to (0, previewWidth or previewHeight)
        x1 = (uint16_t)((areas[0].x1 + 1000.0f)*(previewWidth/2000.0f));
        y1 = (uint16_t)((areas[0].y1 + 1000.0f)*(previewHeight/2000.0f));
        x2 = (uint16_t)((areas[0].x2 + 1000.0f)*(previewWidth/2000.0f));
        y2 = (uint16_t)((areas[0].y2 + 1000.0f)*(previewHeight/2000.0f));
        delete areas;

        if(num_areas_found == 1) {
            aec_roi_value.aec_roi_enable = AEC_ROI_ON;
            aec_roi_value.aec_roi_type = AEC_ROI_BY_COORDINATE;
            aec_roi_value.aec_roi_position.coordinate.x = (x1+x2)/2;
            aec_roi_value.aec_roi_position.coordinate.y = (y1+y2)/2;
        } else {
            aec_roi_value.aec_roi_enable = AEC_ROI_OFF;
            aec_roi_value.aec_roi_type = AEC_ROI_BY_COORDINATE;
            aec_roi_value.aec_roi_position.coordinate.x = DONT_CARE_COORDINATE;
            aec_roi_value.aec_roi_position.coordinate.y = DONT_CARE_COORDINATE;
        }

        if(native_set_parms(MM_CAMERA_PARM_AEC_ROI, sizeof(cam_set_aec_roi_t), (void *)&aec_roi_value))
            rc = NO_ERROR;
        else
            rc = BAD_VALUE;
#endif
#if 0   //solution including multi-roi, to be enabled later
        aec_mtr_area_t aecArea;
        aecArea.num_area = num_areas_found;

        uint16_t x1, x2, y1, y2, dx, dy;
        int previewWidth, previewHeight;
        this->getPreviewSize(&previewWidth, &previewHeight);

        for(int i=0; i<num_areas_found; i++) {
            //transform the coords from (-1000, 1000) to (0, previewWidth or previewHeight)
            x1 = (uint16_t)((areas[i].x1 + 1000.0f)*(previewWidth/2000.0f));
            y1 = (uint16_t)((areas[i].y1 + 1000.0f)*(previewHeight/2000.0f));
            x2 = (uint16_t)((areas[i].x2 + 1000.0f)*(previewWidth/2000.0f));
            y2 = (uint16_t)((areas[i].y2 + 1000.0f)*(previewHeight/2000.0f));
            dx = x2 - x1;
            dy = y2 - y1;
            aecArea.mtr_area[i].x = x1;
            aecArea.mtr_area[i].y = y1;
            aecArea.mtr_area[i].dx = dx;
            aecArea.mtr_area[i].dy = dy;
            aecArea.weight[i] = areas[i].weight;
        }
        delete areas;

        if(native_set_parms(MM_CAMERA_PARM_AEC_MTR_AREA, sizeof(aec_mtr_area_t), (void*)&aecArea))
            rc = NO_ERROR;
        else
            rc = BAD_VALUE;
#endif
    }
    ALOGV("%s: X", __func__);
    return rc;
}

status_t QCameraHardwareInterface::setFocusMode(const QCameraParameters& params)
{
    const char *str = params.get(QCameraParameters::KEY_FOCUS_MODE);
    const char *prev_str = mParameters.get(QCameraParameters::KEY_FOCUS_MODE);
    bool modesAreSame = strcmp(str, prev_str) == 0;
    ALOGV("%s",__func__);
    if (str != NULL) {
        ALOGV("Focus mode '%s', previous focus mode '%s' (cmp %d)",str, prev_str, strcmp(str, prev_str));

        int32_t value;

        if (mHasAutoFocusSupport){
            value = attr_lookup(focus_modes_auto,
                                    sizeof(focus_modes_auto) / sizeof(str_map), str);
        } else {
            value = attr_lookup(focus_modes_fixed,
                                    sizeof(focus_modes_fixed) / sizeof(str_map), str);
        }

        if (value != NOT_FOUND) {
            mParameters.set(QCameraParameters::KEY_FOCUS_MODE, str);
            mFocusMode = value;

            if(updateFocusDistances() != NO_ERROR) {
               ALOGE("%s: updateFocusDistances failed for %s", __FUNCTION__, str);
               return UNKNOWN_ERROR;
            }
            mParameters.set(QCameraParameters::KEY_FOCUS_DISTANCES, mFocusDistance.string());

            // Do not set the AF state to 'not running';
            // this prevents a bug where an autoFocus followed by a setParameters
            // with the same exact focus mode resulting in dropping the autoFocusEvent
            if(modesAreSame) {
                ALOGV("AF mode unchanged (still '%s'); don't touch CAF", str);
                return NO_ERROR;
            } else {
                ALOGV("AF made has changed to '%s'", str);
            }

            if(mHasAutoFocusSupport){
                bool ret = native_set_parms(MM_CAMERA_PARM_FOCUS_MODE,
                                      sizeof(value),
                                      (void *)&value);

                int cafSupport = false;
                int caf_type=0;
                const char *str_hdr = mParameters.get(QCameraParameters::KEY_SCENE_MODE);
                if(!strcmp(str, QCameraParameters::FOCUS_MODE_CONTINUOUS_VIDEO) ||
                   !strcmp(str, QCameraParameters::FOCUS_MODE_CONTINUOUS_PICTURE)){
                    cafSupport = true;
                    bool rc = false;
                    if(!strcmp(str, QCameraParameters::FOCUS_MODE_CONTINUOUS_VIDEO))
                    {
                        caf_type = 1;
                        rc = native_set_parms(MM_CAMERA_PARM_CAF_TYPE, sizeof(caf_type), (void *)&caf_type);
                    }
                    else if(!strcmp(str, QCameraParameters::FOCUS_MODE_CONTINUOUS_PICTURE))
                    {
                        caf_type = 2;
                        rc = native_set_parms(MM_CAMERA_PARM_CAF_TYPE, sizeof(caf_type), (void *)&caf_type);
                    }
                    ALOGV("caf_type %d rc %d", caf_type, rc);
                }


                ALOGV("Continuous Auto Focus %d", cafSupport);
                if(mAutoFocusRunning && cafSupport){
                  ALOGV("Set auto focus running to false");
                  mAutoFocusRunning = false;
                  if(MM_CAMERA_OK!=cam_ops_action(mCameraId,false,MM_CAMERA_OPS_FOCUS,NULL )) {
                    ALOGE("%s: AF command failed err:%d error %s",__func__, errno,strerror(errno));
                  }
                }
                ret = native_set_parms(MM_CAMERA_PARM_CONTINUOUS_AF, sizeof(cafSupport),
                                       (void *)&cafSupport);
            }

            return NO_ERROR;
        }
        ALOGV("%s:Could not look up str value",__func__);
    }
    ALOGE("Invalid focus mode value: %s", (str == NULL) ? "NULL" : str);
    return BAD_VALUE;
}

status_t QCameraHardwareInterface::setSceneMode(const QCameraParameters& params)
{
    status_t rc = NO_ERROR;
    ALOGV("%s",__func__);

    rc = cam_config_is_parm_supported(mCameraId, MM_CAMERA_PARM_BESTSHOT_MODE);
    if(!rc) {
        ALOGV("%s:Parameter Scenemode is not supported for this sensor", __func__);
        return NO_ERROR;
    }
    const char *str = params.get(QCameraParameters::KEY_SCENE_MODE);
    const char *oldstr = mParameters.get(QCameraParameters::KEY_SCENE_MODE);

    if (str != NULL && oldstr != NULL) {
        int32_t value = attr_lookup(scenemode, sizeof(scenemode) / sizeof(str_map), str);
        if (value != NOT_FOUND) {
            /* Check to see if there was a change of scene mode */
            if(strcmp(str,oldstr)) {
                ALOGV("%s: valued changed from %s to %s",__func__,oldstr, str);

                if (!strcmp(str, "hdr")) {
                    ALOGV("%s: setting num-snaps-per-shutter to 2", __func__);
                    mParameters.set("num-snaps-per-shutter", 2);
                } else {
                    ALOGV("%s: setting num-snaps-per-shutter to 1", __func__);
                    mParameters.set("num-snaps-per-shutter", 1);
                }

                /* Check if we are either transitioning to/from HDR state
                   if yes preview needs restart*/
                if(!strcmp(str, "hdr") || !strcmp(oldstr, "hdr") ) {
                    ALOGV("Changed between HDR/non-HDR states");

                    /* Restart only if preview already running*/
                    if (mPreviewState == QCAMERA_HAL_PREVIEW_STARTED) {
                        ALOGV("Preview in progress,restarting for HDR transition");
                        mParameters.set(QCameraParameters::KEY_SCENE_MODE, str);
                        mRestartPreview = 1;
                        pausePreviewForZSL();
                    }
                }

            }


            mParameters.set(QCameraParameters::KEY_SCENE_MODE, str);
            bool ret = native_set_parms(MM_CAMERA_PARM_BESTSHOT_MODE, sizeof(value),
                                       (void *)&value);
            int bestshot_reconfigure;
            cam_config_get_parm(mCameraId, MM_CAMERA_PARM_BESTSHOT_RECONFIGURE,
                                &bestshot_reconfigure);
            if(bestshot_reconfigure) {
                if (mBestShotMode != value) {
                     mBestShotMode = value;
                     if (mPreviewState == QCAMERA_HAL_PREVIEW_STARTED && ret) {
                           ALOGV("%s:Bestshot trigerring restart",__func__);
                           mRestartPreview = 1;
                           pausePreviewForZSL();
                      }
                 }
            }
            return ret ? NO_ERROR : UNKNOWN_ERROR;
        }
    }
    ALOGE("Invalid scenemode value: %s", (str == NULL) ? "NULL" : str);
    return BAD_VALUE;
}

status_t QCameraHardwareInterface::setSelectableZoneAf(const QCameraParameters& params)
{
    ALOGV("%s",__func__);
    status_t rc = NO_ERROR;
    if(mHasAutoFocusSupport) {
        const char *str = params.get(QCameraParameters::KEY_SELECTABLE_ZONE_AF);
        if (str != NULL) {
            int32_t value = attr_lookup(selectable_zone_af, sizeof(selectable_zone_af) / sizeof(str_map), str);
            if (value != NOT_FOUND) {
                 rc = cam_config_is_parm_supported(mCameraId, MM_CAMERA_PARM_FOCUS_RECT);
                 if(!rc) {
                    ALOGV("SelectableZoneAF  is not supported for this sensor");
                    return NO_ERROR;
                 }else {
                    mParameters.set(QCameraParameters::KEY_SELECTABLE_ZONE_AF, str);
                    bool ret = native_set_parms(MM_CAMERA_PARM_FOCUS_RECT, sizeof(value),
                            (void *)&value);
                    return ret ? NO_ERROR : UNKNOWN_ERROR;
                 }
            }
        }
        ALOGE("Invalid selectable zone af value: %s", (str == NULL) ? "NULL" : str);
        return BAD_VALUE;

    }
    return NO_ERROR;
}

status_t QCameraHardwareInterface::setEffect(const QCameraParameters& params)
{
    ALOGV("%s",__func__);
    status_t rc = NO_ERROR;
    const char *str = params.get(QCameraParameters::KEY_EFFECT);
    int result;
    if (str != NULL) {
        ALOGV("Setting effect %s",str);
        int32_t value = attr_lookup(effects, sizeof(effects) / sizeof(str_map), str);
        if (value != NOT_FOUND) {
           rc = cam_config_is_parm_supported(mCameraId, MM_CAMERA_PARM_EFFECT);
           if(!rc) {
               ALOGV("Camera Effect - %s mode is not supported for this sensor",str);
               return NO_ERROR;
           }else {
               mParameters.set(QCameraParameters::KEY_EFFECT, str);
               bool ret = native_set_parms(MM_CAMERA_PARM_EFFECT, sizeof(value),
                                           (void *)&value,(int *)&result);
                if(result != MM_CAMERA_OK) {
                    ALOGE("Camera Effect: %s is not set as the selected value is not supported ", str);
                }
                int bestshot_reconfigure;
                cam_config_get_parm(mCameraId, MM_CAMERA_PARM_BESTSHOT_RECONFIGURE,
                                    &bestshot_reconfigure);
                if(bestshot_reconfigure) {
                     if (mEffects != value) {
                         mEffects = value;
                         if (mPreviewState == QCAMERA_HAL_PREVIEW_STARTED && ret) {
                               mRestartPreview = 1;
                               pausePreviewForZSL();
                          }
                   }
               }
               return ret ? NO_ERROR : UNKNOWN_ERROR;
          }
        }
    }
    ALOGE("Invalid effect value: %s", (str == NULL) ? "NULL" : str);
    ALOGV("setEffect X");
    return BAD_VALUE;
}

status_t QCameraHardwareInterface::setBrightness(const QCameraParameters& params) {

    ALOGV("%s",__func__);
    status_t rc = NO_ERROR;
    rc = cam_config_is_parm_supported(mCameraId, MM_CAMERA_PARM_BRIGHTNESS);
   if(!rc) {
       ALOGV("MM_CAMERA_PARM_BRIGHTNESS mode is not supported for this sensor");
       return NO_ERROR;
   }
   int brightness = params.getInt("luma-adaptation");
   if (mBrightness !=  brightness) {
       ALOGV(" new brightness value : %d ", brightness);
       mBrightness =  brightness;
       mParameters.set("luma-adaptation", brightness);
       bool ret = native_set_parms(MM_CAMERA_PARM_BRIGHTNESS, sizeof(mBrightness),
                                   (void *)&mBrightness);
        return ret ? NO_ERROR : UNKNOWN_ERROR;
   }

    return NO_ERROR;
}

status_t QCameraHardwareInterface::setAutoExposure(const QCameraParameters& params)
{

    ALOGV("%s",__func__);
    status_t rc = NO_ERROR;
    rc = cam_config_is_parm_supported(mCameraId, MM_CAMERA_PARM_EXPOSURE);
   if(!rc) {
       ALOGV("MM_CAMERA_PARM_EXPOSURE mode is not supported for this sensor");
       return NO_ERROR;
   }
   const char *str = params.get(QCameraParameters::KEY_AUTO_EXPOSURE);
    if (str != NULL) {
        int32_t value = attr_lookup(autoexposure, sizeof(autoexposure) / sizeof(str_map), str);
        if (value != NOT_FOUND) {
            mParameters.set(QCameraParameters::KEY_AUTO_EXPOSURE, str);
            bool ret = native_set_parms(MM_CAMERA_PARM_EXPOSURE, sizeof(value),
                                       (void *)&value);
            return ret ? NO_ERROR : UNKNOWN_ERROR;
        }
    }
    ALOGE("Invalid auto exposure value: %s", (str == NULL) ? "NULL" : str);
    return BAD_VALUE;
}

status_t QCameraHardwareInterface::setExposureCompensation(
        const QCameraParameters & params){
    ALOGV("%s",__func__);
    status_t rc = NO_ERROR;
    rc = cam_config_is_parm_supported(mCameraId, MM_CAMERA_PARM_EXPOSURE_COMPENSATION);
    if(!rc) {
       ALOGV("MM_CAMERA_PARM_EXPOSURE_COMPENSATION mode is not supported for this sensor");
       return NO_ERROR;
    }
    int numerator = params.getInt(QCameraParameters::KEY_EXPOSURE_COMPENSATION);
    if(EXPOSURE_COMPENSATION_MINIMUM_NUMERATOR <= numerator &&
            numerator <= EXPOSURE_COMPENSATION_MAXIMUM_NUMERATOR){
        int16_t  numerator16 = (int16_t)(numerator & 0x0000ffff);
        uint16_t denominator16 = EXPOSURE_COMPENSATION_DENOMINATOR;
        uint32_t  value = 0;
        value = numerator16 << 16 | denominator16;

        const char *sce_str = params.get(QCameraParameters::KEY_SCENE_MODE);
        if (sce_str != NULL) {
            if(!strcmp(sce_str, "sunset")){
                //Exposure comp value in sunset scene mode
                mParameters.set(QCameraParameters::KEY_EXPOSURE_COMPENSATION,
                            -6);
            }else{
                //Exposure comp value for other
                mParameters.set(QCameraParameters::KEY_EXPOSURE_COMPENSATION,
                            numerator);
            }
        }else {
            mParameters.set(QCameraParameters::KEY_EXPOSURE_COMPENSATION,
                            numerator);
        }
        bool ret = native_set_parms(MM_CAMERA_PARM_EXPOSURE_COMPENSATION,
                                    sizeof(value), (void *)&value);
        return ret ? NO_ERROR : UNKNOWN_ERROR;
    }
    ALOGE("Invalid Exposure Compensation");
    return BAD_VALUE;
}

status_t QCameraHardwareInterface::setWhiteBalance(const QCameraParameters& params)
{

    ALOGV("%s",__func__);
    status_t rc = NO_ERROR;
    int result;
    const char *str = NULL;
    rc = cam_config_is_parm_supported(mCameraId, MM_CAMERA_PARM_WHITE_BALANCE);
    if(!rc) {
       ALOGV("MM_CAMERA_PARM_WHITE_BALANCE mode is not supported for this sensor");
       return NO_ERROR;
    }

    const char *sce_str = params.get(QCameraParameters::KEY_SCENE_MODE);
    if (sce_str != NULL) {
        if(!strcmp(sce_str, "sunset")){
            //AWB value in sunset scene mode
            str = QCameraParameters::WHITE_BALANCE_DAYLIGHT;
            mParameters.set(QCameraParameters::KEY_WHITE_BALANCE, str);
        }else if(!strcmp(sce_str, "auto")){
            str = params.get(QCameraParameters::KEY_WHITE_BALANCE);
        }else{
            //AWB in  other scene Mode
            str = QCameraParameters::WHITE_BALANCE_AUTO;
            mParameters.set(QCameraParameters::KEY_WHITE_BALANCE, str);
        }
    }else {
        str = params.get(QCameraParameters::KEY_WHITE_BALANCE);
    }

    if (str != NULL) {
        int32_t value = attr_lookup(whitebalance, sizeof(whitebalance) / sizeof(str_map), str);
        if (value != NOT_FOUND) {
            mParameters.set(QCameraParameters::KEY_WHITE_BALANCE, str);
            bool ret = native_set_parms(MM_CAMERA_PARM_WHITE_BALANCE, sizeof(value),
                                       (void *)&value, (int *)&result);
            if(result != MM_CAMERA_OK) {
                ALOGE("WhiteBalance Value: %s is not set as the selected value is not supported ", str);
            }
            return ret ? NO_ERROR : UNKNOWN_ERROR;
        }
    }
    ALOGE("Invalid whitebalance value: %s", (str == NULL) ? "NULL" : str);
    return BAD_VALUE;
}

status_t QCameraHardwareInterface::setAntibanding(const QCameraParameters& params)
{
    int result;

    ALOGV("%s",__func__);
    status_t rc = NO_ERROR;
    rc = cam_config_is_parm_supported(mCameraId, MM_CAMERA_PARM_ANTIBANDING);
    if(!rc) {
       ALOGV("ANTIBANDING mode is not supported for this sensor");
       return NO_ERROR;
    }
    const char *str = params.get(QCameraParameters::KEY_ANTIBANDING);
    if (str != NULL) {
        int value = (camera_antibanding_type)attr_lookup(
          antibanding, sizeof(antibanding) / sizeof(str_map), str);
        if (value != NOT_FOUND) {
            camera_antibanding_type temp = (camera_antibanding_type) value;
            ALOGV("Antibanding Value : %d",value);
            mParameters.set(QCameraParameters::KEY_ANTIBANDING, str);
            bool ret = native_set_parms(MM_CAMERA_PARM_ANTIBANDING,
                       sizeof(camera_antibanding_type), (void *)&value ,(int *)&result);
            if(result != MM_CAMERA_OK) {
                ALOGE("AntiBanding Value: %s is not supported for the given BestShot Mode", str);
            }
            return ret ? NO_ERROR : UNKNOWN_ERROR;
        }
    }
    ALOGE("Invalid antibanding value: %s", (str == NULL) ? "NULL" : str);

    return BAD_VALUE;
}

status_t QCameraHardwareInterface::setPreviewFrameRate(const QCameraParameters& params)
{
    ALOGV("%s",__func__);
    status_t rc = NO_ERROR;
    rc = cam_config_is_parm_supported(mCameraId, MM_CAMERA_PARM_FPS);
    if(!rc) {
       ALOGV("MM_CAMERA_PARM_FPS is not supported for this sensor");
       return NO_ERROR;
    }
    uint16_t previousFps = (uint16_t)mParameters.getPreviewFrameRate();
    uint16_t fps = (uint16_t)params.getPreviewFrameRate();
    ALOGV("requested preview frame rate  is %u", fps);

    if(mInitialized && (fps == previousFps)){
        ALOGV("No change is FPS Value %d",fps );
        return NO_ERROR;
    }

    if(MINIMUM_FPS <= fps && fps <=MAXIMUM_FPS){
        mParameters.setPreviewFrameRate(fps);
        bool ret = native_set_parms(MM_CAMERA_PARM_FPS,
                sizeof(fps), (void *)&fps);
        return ret ? NO_ERROR : UNKNOWN_ERROR;
    }

    return BAD_VALUE;
}

status_t QCameraHardwareInterface::setPreviewFrameRateMode(const QCameraParameters& params) {

    ALOGV("%s",__func__);
    status_t rc = NO_ERROR;
    rc = cam_config_is_parm_supported(mCameraId, MM_CAMERA_PARM_FPS);
    if(!rc) {
       ALOGV(" CAMERA FPS mode is not supported for this sensor");
       return NO_ERROR;
    }
    rc = cam_config_is_parm_supported(mCameraId, MM_CAMERA_PARM_FPS_MODE);
    if(!rc) {
       ALOGV("CAMERA FPS MODE mode is not supported for this sensor");
       return NO_ERROR;
    }

    const char *previousMode = mParameters.getPreviewFrameRateMode();
    const char *str = params.getPreviewFrameRateMode();
    if (NULL == previousMode) {
        ALOGV("Preview Frame Rate Mode is NULL\n");
        return NO_ERROR;
    }
    if (NULL == str) {
        ALOGV("Preview Frame Rate Mode is NULL\n");
        return NO_ERROR;
    }
    int32_t frameRateMode = attr_lookup(frame_rate_modes, sizeof(frame_rate_modes) / sizeof(str_map),str);
    if(frameRateMode != NOT_FOUND) {
        ALOGV("setPreviewFrameRateMode: %s ", str);
        mParameters.setPreviewFrameRateMode(str);
        bool ret = native_set_parms(MM_CAMERA_PARM_FPS_MODE, sizeof(frameRateMode), (void *)&frameRateMode);
        if(!ret) return ret;
        //set the fps value when chaging modes
        int16_t fps = (uint16_t)params.getPreviewFrameRate();
        if(MINIMUM_FPS <= fps && fps <=MAXIMUM_FPS){
            mParameters.setPreviewFrameRate(fps);
            ret = native_set_parms(MM_CAMERA_PARM_FPS,
                                        sizeof(fps), (void *)&fps);
            return ret ? NO_ERROR : UNKNOWN_ERROR;
        }
        ALOGE("Invalid preview frame rate value: %d", fps);
        return BAD_VALUE;
    }
    ALOGE("Invalid preview frame rate mode value: %s", (str == NULL) ? "NULL" : str);

    return BAD_VALUE;
}

status_t QCameraHardwareInterface::setSkinToneEnhancement(const QCameraParameters& params) {
    ALOGV("%s",__func__);
    status_t rc = NO_ERROR;
    rc = cam_config_is_parm_supported(mCameraId, MM_CAMERA_PARM_SCE_FACTOR);
    if(!rc) {
       ALOGV("SkinToneEnhancement is not supported for this sensor");
       return NO_ERROR;
    }
     int skinToneValue = params.getInt("skinToneEnhancement");
     if (mSkinToneEnhancement != skinToneValue) {
          ALOGV(" new skinTone correction value : %d ", skinToneValue);
          mSkinToneEnhancement = skinToneValue;
          mParameters.set("skinToneEnhancement", skinToneValue);
          bool ret = native_set_parms(MM_CAMERA_PARM_SCE_FACTOR, sizeof(mSkinToneEnhancement),
                        (void *)&mSkinToneEnhancement);
          return ret ? NO_ERROR : UNKNOWN_ERROR;
    }
    return NO_ERROR;
}

status_t QCameraHardwareInterface::setWaveletDenoise(const QCameraParameters& params) {
    ALOGV("%s",__func__);
    status_t rc = NO_ERROR;
    rc = cam_config_is_parm_supported(mCameraId, MM_CAMERA_PARM_WAVELET_DENOISE);
    if(rc != MM_CAMERA_PARM_SUPPORT_SET) {
        ALOGV("Wavelet Denoise is not supported for this sensor");
        /* TO DO */
//        return NO_ERROR;
    }
    const char *str = params.get(QCameraParameters::KEY_DENOISE);
    if (str != NULL) {
        int value = attr_lookup(denoise,
                sizeof(denoise) / sizeof(str_map), str);
        if ((value != NOT_FOUND) &&  (mDenoiseValue != value)) {
            mDenoiseValue =  value;
            mParameters.set(QCameraParameters::KEY_DENOISE, str);

            char prop[PROPERTY_VALUE_MAX];
            memset(prop, 0, sizeof(prop));
            property_get("persist.denoise.process.plates", prop, "1");

            denoise_param_t temp;
            memset(&temp, 0, sizeof(denoise_param_t));
            temp.denoise_enable = value;
            temp.process_plates = atoi(prop);
            ALOGV("Denoise enable=%d, plates=%d", temp.denoise_enable, temp.process_plates);
            bool ret = native_set_parms(MM_CAMERA_PARM_WAVELET_DENOISE, sizeof(temp),
                    (void *)&temp);
            return ret ? NO_ERROR : UNKNOWN_ERROR;
        }
        return NO_ERROR;
    }
    ALOGE("Invalid Denoise value: %s", (str == NULL) ? "NULL" : str);
    return BAD_VALUE;
}

status_t QCameraHardwareInterface::setVideoSize(const QCameraParameters& params)
{
    const char *str= NULL;
    const char *str_t= NULL;
    int old_vid_w = 0, old_vid_h = 0;
    ALOGV("%s: E", __func__);
    str = params.get(QCameraParameters::KEY_VIDEO_SIZE);
    str_t = mParameters.get(CameraParameters::KEY_VIDEO_SIZE);
    if(!str) {
        mParameters.set(QCameraParameters::KEY_VIDEO_SIZE, "");
        //If application didn't set this parameter string, use the values from
        //getPreviewSize() as video dimensions.
        ALOGV("No Record Size requested, use the preview dimensions");
        mVideoWidth = mPreviewWidth;
        mVideoHeight = mPreviewHeight;
    } else {
        //Extract the record witdh and height that application requested.
        ALOGV("%s: requested record size %s", __func__, str);
        if(!parse_size(str, mVideoWidth, mVideoHeight)) {
            parse_size(str_t, old_vid_w, old_vid_h);
            if(old_vid_w != mVideoWidth || old_vid_h != mVideoHeight) {
                mRestartPreview = true; 
                ALOGV("%s: Video sizes changes, Restart preview...", __func__);
            }
            mParameters.set(QCameraParameters::KEY_VIDEO_SIZE, str);
        } else {
            mParameters.set(QCameraParameters::KEY_VIDEO_SIZE, "");
            ALOGE("%s: error :failed to parse parameter record-size (%s)", __func__, str);
            return BAD_VALUE;
        }
    }
    ALOGV("%s: preview dimensions: %dx%d", __func__, mPreviewWidth, mPreviewHeight);
    ALOGV("%s: video dimensions: %dx%d", __func__, mVideoWidth, mVideoHeight);

    ALOGV("%s: X", __func__);
    return NO_ERROR;
}

status_t QCameraHardwareInterface::setCameraMode(const QCameraParameters& params) {
    int32_t value = params.getInt(QCameraParameters::KEY_CAMERA_MODE);
    mParameters.set(QCameraParameters::KEY_CAMERA_MODE,value);

    ALOGV("ZSL is enabled  %d", value);
    if (value == 1) {
        myMode = (camera_mode_t)(myMode | CAMERA_ZSL_MODE);
    } else {
        myMode = (camera_mode_t)(myMode & ~CAMERA_ZSL_MODE);
    }

    return NO_ERROR;
}

status_t QCameraHardwareInterface::setPowerMode(const QCameraParameters& params) {
    uint32_t value = NORMAL_POWER;
    const char *powermode = NULL;

    powermode = params.get(QCameraParameters::KEY_POWER_MODE);
    if (powermode != NULL) {
        value = attr_lookup(power_modes,
                sizeof(power_modes) / sizeof(str_map), powermode);
        if((value == LOW_POWER) || mHFRLevel > 1) {
            ALOGV("Enable Low Power Mode");
            value = LOW_POWER;
            mPowerMode = value;
            mParameters.set(QCameraParameters::KEY_POWER_MODE,"Low_Power");
        } else {
            ALOGV("Enable Normal Power Mode");
            mPowerMode = value;
            mParameters.set(QCameraParameters::KEY_POWER_MODE,"Normal_Power");
        }
    }

    ALOGV("%s Low power mode %s value = %d", __func__,
          value ? "Enabled" : "Disabled", value);
    native_set_parms(MM_CAMERA_PARM_LOW_POWER_MODE, sizeof(value),
                                               (void *)&value);
    return NO_ERROR;
}


status_t QCameraHardwareInterface::setPreviewSize(const QCameraParameters& params)
{
    int width, height;
    params.getPreviewSize(&width, &height);
    ALOGV("################requested preview size %d x %d", width, height);

    // Validate the preview size
    for (size_t i = 0; i <  mPreviewSizeCount; ++i) {
        if (width ==  mPreviewSizes[i].width
           && height ==  mPreviewSizes[i].height) {
            int old_width, old_height;
            mParameters.getPreviewSize(&old_width,&old_height);
            if(width != old_width || height != old_height) {
                mRestartPreview = true;
            }
            mParameters.setPreviewSize(width, height);
            ALOGV("setPreviewSize:  width: %d   heigh: %d", width, height);
            mPreviewWidth = width;
            mPreviewHeight = height;

            mDimension.display_width = mPreviewWidth;
            mDimension.display_height= mPreviewHeight;
            mDimension.orig_video_width = mPreviewWidth;
            mDimension.orig_video_height = mPreviewHeight;
            mDimension.video_width = mPreviewWidth;
            mDimension.video_height = mPreviewHeight;

            return NO_ERROR;
        }
    }
    ALOGE("Invalid preview size requested: %dx%d", width, height);
    return BAD_VALUE;
}
status_t QCameraHardwareInterface::setPreviewFpsRange(const QCameraParameters& params)
{
    ALOGV("%s: E", __func__);
    int minFps,maxFps;
    int prevMinFps, prevMaxFps;
    int rc = NO_ERROR;
    bool found = false;

    mParameters.getPreviewFpsRange(&prevMinFps, &prevMaxFps);
    ALOGV("%s: Existing FpsRange Values:(%d, %d)", __func__, prevMinFps, prevMaxFps);
    params.getPreviewFpsRange(&minFps,&maxFps);
    ALOGV("%s: Requested FpsRange Values:(%d, %d)", __func__, minFps, maxFps);

    if(mInitialized && (minFps == prevMinFps && maxFps == prevMaxFps)) {
        ALOGV("%s: No change in FpsRange", __func__);
        rc = NO_ERROR;
        goto end;
    }
    for(size_t i=0; i<FPS_RANGES_SUPPORTED_COUNT; i++) {
        // if the value is in the supported list
        if(minFps==FpsRangesSupported[i].minFPS && maxFps == FpsRangesSupported[i].maxFPS){
            found = true;
            ALOGV("FPS: i=%d : minFps = %d, maxFps = %d ",i,FpsRangesSupported[i].minFPS,FpsRangesSupported[i].maxFPS );
            mParameters.setPreviewFpsRange(minFps,maxFps);
            // validate the values
            bool valid = true;
            // FPS can not be negative
            if(minFps < 0 || maxFps < 0) valid = false;
            // minFps must be >= maxFps
            if(minFps > maxFps) valid = false;

            if(valid) {
                //Set the FPS mode
                const char *str = (minFps == maxFps) ?
                    QCameraParameters::KEY_PREVIEW_FRAME_RATE_FIXED_MODE:
                    QCameraParameters::KEY_PREVIEW_FRAME_RATE_AUTO_MODE;
                ALOGV("%s FPS_MODE = %s", __func__, str);
                int32_t frameRateMode = attr_lookup(frame_rate_modes,
                        sizeof(frame_rate_modes) / sizeof(str_map),str);
                bool ret;
                ret = native_set_parms(MM_CAMERA_PARM_FPS_MODE, sizeof(int32_t),
                            (void *)&frameRateMode);

                //set FPS values
                uint32_t fps;  //lower 2 bytes specify maxFps and higher 2 bytes specify minFps
                fps = ((uint32_t)(minFps/1000) << 16) + ((uint16_t)(maxFps/1000));
                ret = native_set_parms(MM_CAMERA_PARM_FPS, sizeof(uint32_t), (void *)&fps);
                mParameters.setPreviewFpsRange(minFps, maxFps);
                if(ret)
                    rc = NO_ERROR;
                else {
                    rc = BAD_VALUE;
                    ALOGE("%s: error: native_set_params failed", __func__);
                }
            } else {
                ALOGE("%s: error: invalid FPS range value", __func__);
                rc = BAD_VALUE;
            }
        }
    }
    if(found == false){
            ALOGE("%s: error: FPS range value not supported", __func__);
            rc = BAD_VALUE;
    }
end:
    ALOGV("%s: X", __func__);
    return rc;
}

status_t QCameraHardwareInterface::setJpegThumbnailSize(const QCameraParameters& params){
    int width = params.getInt(QCameraParameters::KEY_JPEG_THUMBNAIL_WIDTH);
    int height = params.getInt(QCameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT);

    ALOGV("requested jpeg thumbnail size %d x %d", width, height);

    // Validate the picture size
    for (unsigned int i = 0; i < thumbnail_sizes_count; ++i) {
       if (width == default_thumbnail_sizes[i].width
         && height == default_thumbnail_sizes[i].height) {
           thumbnailWidth = width;
           thumbnailHeight = height;
           mParameters.set(QCameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, width);
           mParameters.set(QCameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, height);
           return NO_ERROR;
       }
    }
    ALOGE("error: setting jpeg thumbnail size");
    return BAD_VALUE;
}
status_t QCameraHardwareInterface::setPictureSize(const QCameraParameters& params)
{
    int width, height;
    ALOGV("QualcommCameraHardware::setPictureSize E");
    params.getPictureSize(&width, &height);
    ALOGV("requested picture size %d x %d", width, height);

    // Validate the picture size
    for (int i = 0; i < mSupportedPictureSizesCount; ++i) {
        if (width == mPictureSizesPtr[i].width
          && height == mPictureSizesPtr[i].height) {
            int old_width, old_height;
            mParameters.getPictureSize(&old_width,&old_height);
            if(width != old_width || height != old_height) {
                mRestartPreview = true;
            }
            mParameters.setPictureSize(width, height);
            mDimension.picture_width = width;
            mDimension.picture_height = height;
            return NO_ERROR;
        }
    }
    /* Dimension not among the ones in the list. Check if
     * its a valid dimension, if it is, then configure the
     * camera accordingly. else reject it.
     */
    if( isValidDimension(width, height) ) {
        mParameters.setPictureSize(width, height);
        mDimension.picture_width = width;
        mDimension.picture_height = height;
        return NO_ERROR;
    } else
        ALOGE("Invalid picture size requested: %dx%d", width, height);
    return BAD_VALUE;
}

status_t QCameraHardwareInterface::setJpegRotation(int isZsl) {
    return mm_jpeg_encoder_setRotation(mRotation, isZsl);
}

int QCameraHardwareInterface::getJpegRotation(void) {
    return mRotation;
}

int QCameraHardwareInterface::getISOSpeedValue()
{
    const char *iso_str = mParameters.get(QCameraParameters::KEY_ISO_MODE);
    int iso_index = attr_lookup(iso, sizeof(iso) / sizeof(str_map), iso_str);
    int iso_value = iso_speed_values[iso_index];
    return iso_value;
}


status_t QCameraHardwareInterface::setJpegQuality(const QCameraParameters& params) {
    status_t rc = NO_ERROR;
    int quality = params.getInt(QCameraParameters::KEY_JPEG_QUALITY);
    ALOGV("setJpegQuality E");
    if (quality >= 0 && quality <= 100) {
        mParameters.set(QCameraParameters::KEY_JPEG_QUALITY, quality);
        mJpegQuality = quality;
    } else {
        ALOGE("Invalid jpeg quality=%d", quality);
        rc = BAD_VALUE;
    }

    quality = params.getInt(QCameraParameters::KEY_JPEG_THUMBNAIL_QUALITY);
    if (quality >= 0 && quality <= 100) {
        mParameters.set(QCameraParameters::KEY_JPEG_THUMBNAIL_QUALITY, quality);
    } else {
        ALOGE("Invalid jpeg thumbnail quality=%d", quality);
        rc = BAD_VALUE;
    }
    ALOGV("setJpegQuality X");
    return rc;
}

status_t QCameraHardwareInterface::
setNumOfSnapshot() {
    status_t rc = NO_ERROR;

    int num_of_snapshot = getNumOfSnapshots();

    bool result = native_set_parms(MM_CAMERA_PARM_SNAPSHOT_BURST_NUM,
                                   sizeof(int),
                                   (void *)&num_of_snapshot);
    if(!result)
        ALOGE("%s:Failure setting number of snapshots!!!", __func__);
    return rc;
}

status_t QCameraHardwareInterface::setPreviewFormat(const QCameraParameters& params) {
    const char *str = params.getPreviewFormat();
    int32_t previewFormat = attr_lookup(preview_formats, sizeof(preview_formats) / sizeof(str_map), str);
    if(previewFormat != NOT_FOUND) {
        int num = sizeof(preview_format_info_list)/sizeof(preview_format_info_t);
        int i;

        for (i = 0; i < num; i++) {
          if (preview_format_info_list[i].Hal_format == previewFormat) {
            mPreviewFormatInfo = preview_format_info_list[i];
            break;
          }
        }

        if (i == num) {
          mPreviewFormatInfo.mm_cam_format = CAMERA_YUV_420_NV21;
          mPreviewFormatInfo.padding = CAMERA_PAD_TO_WORD;
          return BAD_VALUE;
        }
        bool ret = native_set_parms(MM_CAMERA_PARM_PREVIEW_FORMAT, sizeof(cam_format_t),
                                   (void *)&mPreviewFormatInfo.mm_cam_format);
        mParameters.set(QCameraParameters::KEY_PREVIEW_FORMAT, str);
        mPreviewFormat = mPreviewFormatInfo.mm_cam_format;
        ALOGV("Setting preview format to %d, i =%d, num=%d, hal_format=%d",
             mPreviewFormat, i, num, mPreviewFormatInfo.Hal_format);
        return NO_ERROR;
    } else if ( strTexturesOn ) {
      mPreviewFormatInfo.mm_cam_format = CAMERA_YUV_420_NV21;
      mPreviewFormatInfo.padding = CAMERA_PAD_TO_4K;
    } else {
      mPreviewFormatInfo.mm_cam_format = CAMERA_YUV_420_NV21;
      mPreviewFormatInfo.padding = CAMERA_PAD_TO_WORD;
    }
    ALOGE("Invalid preview format value: %s", (str == NULL) ? "NULL" : str);
    return BAD_VALUE;
}

status_t QCameraHardwareInterface::setStrTextures(const QCameraParameters& params) {
    const char *str = params.get("strtextures");
    const char *prev_str = mParameters.get("strtextures");

    if(str != NULL) {
        if(!strcmp(str,prev_str)) {
            return NO_ERROR;
        }
        int str_size = strlen(str);
        mParameters.set("strtextures", str);
        if(str_size == 2) {
            if(!strncmp(str, "on", str_size) || !strncmp(str, "ON", str_size)){
                ALOGV("Resetting mUseOverlay to false");
                strTexturesOn = true;
                mUseOverlay = false;
            }
        }else if(str_size == 3){
            if (!strncmp(str, "off", str_size) || !strncmp(str, "OFF", str_size)) {
                strTexturesOn = false;
                mUseOverlay = true;
            }
        }

    }
    return NO_ERROR;
}

status_t QCameraHardwareInterface::setFlash(const QCameraParameters& params)
{
    const char *str = NULL;

    ALOGV("%s: E",__func__);
    int rc = cam_config_is_parm_supported(mCameraId, MM_CAMERA_PARM_LED_MODE);
    if(!rc) {
        ALOGV("%s:LED FLASH not supported", __func__);
        return NO_ERROR;
    }

    const char *sce_str = params.get(QCameraParameters::KEY_SCENE_MODE);
    if (sce_str != NULL) {
        if (!strcmp(sce_str, "hdr")) {
            //Flash In HDR
            str = QCameraParameters::FLASH_MODE_OFF;
            mParameters.set(QCameraParameters::KEY_FLASH_MODE, str);
        }else if(!strcmp(sce_str, "auto")){
            //Flash Mode in auto scene mode
            str = params.get(QCameraParameters::KEY_FLASH_MODE);
        }else{
            //FLASH in  scene Mode except auto, hdr
            str = QCameraParameters::FLASH_MODE_AUTO;
            mParameters.set(QCameraParameters::KEY_FLASH_MODE, str);
        }
    }else {
        str = params.get(QCameraParameters::KEY_FLASH_MODE);
    }

    if (str != NULL) {
        int32_t value = attr_lookup(flash, sizeof(flash) / sizeof(str_map), str);
        if (value != NOT_FOUND) {
            mParameters.set(QCameraParameters::KEY_FLASH_MODE, str);
            bool ret = native_set_parms(MM_CAMERA_PARM_LED_MODE,
                                       sizeof(value), (void *)&value);
            mLedStatusForZsl = (led_mode_t)value;
            return ret ? NO_ERROR : UNKNOWN_ERROR;
        }
    }
    ALOGE("Invalid flash mode value: %s", (str == NULL) ? "NULL" : str);

    return BAD_VALUE;
}

status_t QCameraHardwareInterface::setAecAwbLock(const QCameraParameters & params)
{
    ALOGV("%s : E", __func__);
    status_t rc = NO_ERROR;
    int32_t value;
    const char* str;

    //for AEC lock
    str = params.get(QCameraParameters::KEY_AUTO_EXPOSURE_LOCK);
    value = (strcmp(str, "true") == 0)? 1 : 0;
    mParameters.set(QCameraParameters::KEY_AUTO_EXPOSURE_LOCK, str);
    rc = (native_set_parms(MM_CAMERA_PARM_AEC_LOCK, sizeof(int32_t), (void *)(&value))) ?
                            NO_ERROR : UNKNOWN_ERROR;

    //for AWB lock
    str = params.get(QCameraParameters::KEY_AUTO_WHITEBALANCE_LOCK);
    value = (strcmp(str, "true") == 0)? 1 : 0;
    mParameters.set(QCameraParameters::KEY_AUTO_WHITEBALANCE_LOCK, str);
    rc = (native_set_parms(MM_CAMERA_PARM_AWB_LOCK, sizeof(int32_t), (void *)(&value))) ?
                        NO_ERROR : UNKNOWN_ERROR;
    ALOGV("%s : X", __func__);
    return rc;
}

status_t QCameraHardwareInterface::setOverlayFormats(const QCameraParameters& params)
{
    mParameters.set("overlay-format", HAL_PIXEL_FORMAT_YCbCr_420_SP);
    if(mIs3DModeOn == true) {
       int ovFormat = HAL_PIXEL_FORMAT_YCrCb_420_SP|HAL_3D_IN_SIDE_BY_SIDE_L_R|HAL_3D_OUT_SIDE_BY_SIDE;
        mParameters.set("overlay-format", ovFormat);
    }
    return NO_ERROR;
}

status_t QCameraHardwareInterface::setMCEValue(const QCameraParameters& params)
{
    ALOGV("%s",__func__);
    status_t rc = NO_ERROR;
    rc = cam_config_is_parm_supported(mCameraId,MM_CAMERA_PARM_MCE);
   if(!rc) {
       ALOGV("MM_CAMERA_PARM_MCE mode is not supported for this sensor");
       return NO_ERROR;
   }
   const char *str = params.get(QCameraParameters::KEY_MEMORY_COLOR_ENHANCEMENT);
    if (str != NULL) {
        int value = attr_lookup(mce, sizeof(mce) / sizeof(str_map), str);
        if (value != NOT_FOUND) {
            int temp = (int8_t)value;
            ALOGV("%s: setting MCE value of %s", __FUNCTION__, str);
            mParameters.set(QCameraParameters::KEY_MEMORY_COLOR_ENHANCEMENT, str);

            native_set_parms(MM_CAMERA_PARM_MCE, sizeof(int8_t), (void *)&temp);
            return NO_ERROR;
        }
    }
    ALOGE("Invalid MCE value: %s", (str == NULL) ? "NULL" : str);

    return NO_ERROR;
}

status_t QCameraHardwareInterface::setHighFrameRate(const QCameraParameters& params)
{

    bool mCameraRunning;

    int rc = cam_config_is_parm_supported(mCameraId, MM_CAMERA_PARM_HFR);
    if(!rc) {
        ALOGV("%s: MM_CAMERA_PARM_HFR not supported", __func__);
        return NO_ERROR;
    }

    const char *str = params.get(QCameraParameters::KEY_VIDEO_HIGH_FRAME_RATE);
    if (str != NULL) {
        int value = attr_lookup(hfr, sizeof(hfr) / sizeof(str_map), str);
        if (value != NOT_FOUND) {
            mHFRLevel = (int32_t)value;
            //Check for change in HFR value
            const char *oldHfr = mParameters.get(QCameraParameters::KEY_VIDEO_HIGH_FRAME_RATE);
            if(strcmp(oldHfr, str)){
                mParameters.set(QCameraParameters::KEY_VIDEO_HIGH_FRAME_RATE, str);
//              mHFRMode = true;
		mCameraRunning=isPreviewRunning();
                if(mCameraRunning == true) {
//                    mHFRThreadWaitLock.lock();
//                    pthread_attr_t pattr;
//                    pthread_attr_init(&pattr);
//                    pthread_attr_setdetachstate(&pattr, PTHREAD_CREATE_DETACHED);
//                    mHFRThreadRunning = !pthread_create(&mHFRThread,
//                                      &pattr,
//                                      hfr_thread,
//                                      (void*)NULL);
//                    mHFRThreadWaitLock.unlock();
                    stopPreviewInternal();
                    mPreviewState = QCAMERA_HAL_PREVIEW_STOPPED;
                    native_set_parms(MM_CAMERA_PARM_HFR, sizeof(int32_t), (void *)&mHFRLevel);
                    mPreviewState = QCAMERA_HAL_PREVIEW_START;
                    if (startPreview2() == NO_ERROR)
                        mPreviewState = QCAMERA_HAL_PREVIEW_STARTED;
                    return NO_ERROR;
                }
            }
            native_set_parms(MM_CAMERA_PARM_HFR, sizeof(int32_t), (void *)&mHFRLevel);
            return NO_ERROR;
        }
    }
    ALOGE("Invalid HFR value: %s", (str == NULL) ? "NULL" : str);
    return NO_ERROR;
}

status_t QCameraHardwareInterface::setLensshadeValue(const QCameraParameters& params)
{

    int rc = cam_config_is_parm_supported(mCameraId, MM_CAMERA_PARM_ROLLOFF);
    if(!rc) {
        ALOGV("%s:LENS SHADING not supported", __func__);
        return NO_ERROR;
    }

    const char *str = params.get(QCameraParameters::KEY_LENSSHADE);
    if (str != NULL) {
        int value = attr_lookup(lensshade,
                                    sizeof(lensshade) / sizeof(str_map), str);
        if (value != NOT_FOUND) {
            int8_t temp = (int8_t)value;
            mParameters.set(QCameraParameters::KEY_LENSSHADE, str);
            native_set_parms(MM_CAMERA_PARM_ROLLOFF, sizeof(int8_t), (void *)&temp);
            return NO_ERROR;
        }
    }
    ALOGE("Invalid lensShade value: %s", (str == NULL) ? "NULL" : str);
    return BAD_VALUE;
}

status_t QCameraHardwareInterface::setFaceDetect(const QCameraParameters& params)
{
    if(supportsFaceDetection() == false){
        ALOGI("setFaceDetect support is not available");
        return NO_ERROR;
    }

    int requested_faces = params.getInt(QCameraParameters::KEY_MAX_NUM_REQUESTED_FACES);
    int hardware_supported_faces = mParameters.getInt(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_HW);
    if (requested_faces > hardware_supported_faces) {
        requested_faces = hardware_supported_faces;
    }
    mParameters.set(QCameraParameters::KEY_MAX_NUM_REQUESTED_FACES, requested_faces);
    const char *str = params.get(QCameraParameters::KEY_FACE_DETECTION);
    ALOGV("setFaceDetect: %s", str);
    if (str != NULL) {
        fd_set_parm_t fd_set_parm;
        int value = attr_lookup(facedetection,
                sizeof(facedetection) / sizeof(str_map), str);
        mFaceDetectOn = value;
        fd_set_parm.fd_mode = value;
        fd_set_parm.num_fd = requested_faces;
        ALOGV("%s Face detection value = %d, num_fd = %d",__func__, value, requested_faces);
        native_set_parms(MM_CAMERA_PARM_FD, sizeof(fd_set_parm_t), (void *)&fd_set_parm);
        mParameters.set(QCameraParameters::KEY_FACE_DETECTION, str);
        return NO_ERROR;
    }
    ALOGE("Invalid Face Detection value: %s", (str == NULL) ? "NULL" : str);
    return BAD_VALUE;
}
status_t QCameraHardwareInterface::setFaceDetection(const char *str)
{
    if(supportsFaceDetection() == false){
        ALOGV("Face detection is not enabled");
        return NO_ERROR;
    }
    if (str != NULL) {
        int requested_faces = mParameters.getInt(QCameraParameters::KEY_MAX_NUM_REQUESTED_FACES);
        int value = attr_lookup(facedetection,
                                    sizeof(facedetection) / sizeof(str_map), str);
        if (value != NOT_FOUND) {
            fd_set_parm_t fd_set_parm;
            mMetaDataWaitLock.lock();
            mFaceDetectOn = value;
            mMetaDataWaitLock.unlock();
            fd_set_parm.fd_mode = value;
            fd_set_parm.num_fd = requested_faces;
            ALOGV("%s Face detection value = %d, num_fd = %d",__func__, value, requested_faces);
            native_set_parms(MM_CAMERA_PARM_FD, sizeof(fd_set_parm_t), (void *)&fd_set_parm);
            mParameters.set(QCameraParameters::KEY_FACE_DETECTION, str);
            return NO_ERROR;
        }
    }
    ALOGE("Invalid Face Detection value: %s", (str == NULL) ? "NULL" : str);
    return BAD_VALUE;
}

status_t QCameraHardwareInterface::setAEBracket(const QCameraParameters& params)
{
    const char *str;
    if(!cam_config_is_parm_supported(mCameraId,MM_CAMERA_PARM_HDR) || (myMode & CAMERA_ZSL_MODE)) {
        ALOGV("Parameter HDR is not supported for this sensor/ ZSL mode");

        if (myMode & CAMERA_ZSL_MODE) {
            ALOGV("In ZSL mode, reset AEBBracket to HDR_OFF mode");
            exp_bracketing_t temp;
            memset(&temp, 0, sizeof(temp));
            mHdrMode = HDR_BRACKETING_OFF;
            temp.hdr_enable= false;
            temp.mode = HDR_BRACKETING_OFF;
            native_set_parms(MM_CAMERA_PARM_HDR, sizeof(exp_bracketing_t), (void *)&temp);
        }
        return NO_ERROR;
    }

    const char *str2 = params.get(QCameraParameters::KEY_SCENE_MODE);
    if(!strcmp(str2, "hdr")) {
        str="HDR";
    }   else {
        str=QCameraParameters::AE_BRACKET_HDR_OFF;
    }

    if (str != NULL) {
        int value = attr_lookup(hdr_bracket,
                                    sizeof(hdr_bracket) / sizeof(str_map), str);
        exp_bracketing_t temp;
        memset(&temp, 0, sizeof(temp));
        switch (value) {
            case HDR_MODE:
                {
                    mHdrMode = HDR_MODE;
                }
                break;
            case EXP_BRACKETING_MODE:
                {
                    int numFrames = getNumOfSnapshots();
                    const char *str_val = params.get("capture-burst-exposures");
                    if ((str_val != NULL) && (strlen(str_val)>0)) {
                        ALOGV("%s: capture-burst-exposures %s", __FUNCTION__, str_val);

                        mHdrMode = EXP_BRACKETING_MODE;
                        temp.hdr_enable = false;
                        temp.mode = EXP_BRACKETING_MODE;
                        temp.total_frames = (numFrames >  MAX_SNAPSHOT_BUFFERS -2) ? MAX_SNAPSHOT_BUFFERS -2 : numFrames;
                        temp.total_hal_frames = temp.total_frames;
                        strlcpy(temp.values, str_val, MAX_EXP_BRACKETING_LENGTH);
                        ALOGV("%s: setting Exposure Bracketing value of %s, frame (%d)", __FUNCTION__, temp.values, temp.total_hal_frames);
                        native_set_parms(MM_CAMERA_PARM_HDR, sizeof(exp_bracketing_t), (void *)&temp);
                    }
                    else {
                        /* Apps not set capture-burst-exposures, error case fall into bracketing off mode */
                        ALOGV("%s: capture-burst-exposures not set, back to HDR OFF mode", __FUNCTION__);
                        mHdrMode = HDR_BRACKETING_OFF;
                        temp.hdr_enable= false;
                        temp.mode = HDR_BRACKETING_OFF;
                        native_set_parms(MM_CAMERA_PARM_HDR, sizeof(exp_bracketing_t), (void *)&temp);
                    }
                }
                break;
            case HDR_BRACKETING_OFF:
            default:
                {
                    mHdrMode = HDR_BRACKETING_OFF;
                    temp.hdr_enable= false;
                    temp.mode = HDR_BRACKETING_OFF;
                    native_set_parms(MM_CAMERA_PARM_HDR, sizeof(exp_bracketing_t), (void *)&temp);
                }
                break;
        }

        /* save the value*/
        mParameters.set(QCameraParameters::KEY_AE_BRACKET_HDR, str);
    }
    return NO_ERROR;
}

status_t QCameraHardwareInterface::setCaptureBurstExp()
{
    char burst_exp[PROPERTY_VALUE_MAX];
    memset(burst_exp, 0, sizeof(burst_exp));
    property_get("persist.capture.burst.exposures", burst_exp, "");
    if (NULL != burst_exp)
      mParameters.set("capture-burst-exposures", burst_exp);
    return NO_ERROR;
}

status_t QCameraHardwareInterface::setRedeyeReduction(const QCameraParameters& params)
{
    if(supportsRedEyeReduction() == false) {
        ALOGV("Parameter Redeye Reduction is not supported for this sensor");
        return NO_ERROR;
    }

    const char *str = params.get(QCameraParameters::KEY_REDEYE_REDUCTION);
    if (str != NULL) {
        int value = attr_lookup(redeye_reduction, sizeof(redeye_reduction) / sizeof(str_map), str);
        if (value != NOT_FOUND) {
            int8_t temp = (int8_t)value;
            ALOGV("%s: setting Redeye Reduction value of %s", __FUNCTION__, str);
            mParameters.set(QCameraParameters::KEY_REDEYE_REDUCTION, str);

            native_set_parms(MM_CAMERA_PARM_REDEYE_REDUCTION, sizeof(int8_t), (void *)&temp);
            return NO_ERROR;
        }
    }
    ALOGE("Invalid Redeye Reduction value: %s", (str == NULL) ? "NULL" : str);
    return BAD_VALUE;
}

status_t QCameraHardwareInterface::setGpsLocation(const QCameraParameters& params)
{
    const char *method = params.get(QCameraParameters::KEY_GPS_PROCESSING_METHOD);
    if (method) {
        mParameters.set(QCameraParameters::KEY_GPS_PROCESSING_METHOD, method);
    }else {
         mParameters.remove(QCameraParameters::KEY_GPS_PROCESSING_METHOD);
    }

    const char *latitude = params.get(QCameraParameters::KEY_GPS_LATITUDE);
    if (latitude) {
        mParameters.set(QCameraParameters::KEY_GPS_LATITUDE, latitude);
    }else {
         mParameters.remove(QCameraParameters::KEY_GPS_LATITUDE);
    }

    const char *latitudeRef = params.get(QCameraParameters::KEY_GPS_LATITUDE_REF);
    if (latitudeRef) {
        mParameters.set(QCameraParameters::KEY_GPS_LATITUDE_REF, latitudeRef);
    }else {
         mParameters.remove(QCameraParameters::KEY_GPS_LATITUDE_REF);
    }

    const char *longitude = params.get(QCameraParameters::KEY_GPS_LONGITUDE);
    if (longitude) {
        mParameters.set(QCameraParameters::KEY_GPS_LONGITUDE, longitude);
    }else {
         mParameters.remove(QCameraParameters::KEY_GPS_LONGITUDE);
    }

    const char *longitudeRef = params.get(QCameraParameters::KEY_GPS_LONGITUDE_REF);
    if (longitudeRef) {
        mParameters.set(QCameraParameters::KEY_GPS_LONGITUDE_REF, longitudeRef);
    }else {
         mParameters.remove(QCameraParameters::KEY_GPS_LONGITUDE_REF);
    }

    const char *altitudeRef = params.get(QCameraParameters::KEY_GPS_ALTITUDE_REF);
    if (altitudeRef) {
        mParameters.set(QCameraParameters::KEY_GPS_ALTITUDE_REF, altitudeRef);
    }else {
         mParameters.remove(QCameraParameters::KEY_GPS_ALTITUDE_REF);
    }

    const char *altitude = params.get(QCameraParameters::KEY_GPS_ALTITUDE);
    if (altitude) {
        mParameters.set(QCameraParameters::KEY_GPS_ALTITUDE, altitude);
    }else {
         mParameters.remove(QCameraParameters::KEY_GPS_ALTITUDE);
    }

    const char *status = params.get(QCameraParameters::KEY_GPS_STATUS);
    if (status) {
        mParameters.set(QCameraParameters::KEY_GPS_STATUS, status);
    }

    const char *dateTime = params.get(QCameraParameters::KEY_EXIF_DATETIME);
    if (dateTime) {
        mParameters.set(QCameraParameters::KEY_EXIF_DATETIME, dateTime);
    }else {
         mParameters.remove(QCameraParameters::KEY_EXIF_DATETIME);
    }

    const char *timestamp = params.get(QCameraParameters::KEY_GPS_TIMESTAMP);
    if (timestamp) {
        mParameters.set(QCameraParameters::KEY_GPS_TIMESTAMP, timestamp);
    }else {
         mParameters.remove(QCameraParameters::KEY_GPS_TIMESTAMP);
    }
    ALOGV("setGpsLocation X");
    return NO_ERROR;
}

status_t QCameraHardwareInterface::setRotation(const QCameraParameters& params)
{
    status_t rc = NO_ERROR;
    int rotation = params.getInt(QCameraParameters::KEY_ROTATION);
    if (rotation != NOT_FOUND) {
        if (rotation == 0 || rotation == 90 || rotation == 180
            || rotation == 270) {
          mParameters.set(QCameraParameters::KEY_ROTATION, rotation);
          mRotation = rotation;
        } else {
            ALOGE("Invalid rotation value: %d", rotation);
            rc = BAD_VALUE;
        }
    }
    ALOGV("setRotation");
    return rc;
}

status_t QCameraHardwareInterface::setDenoise(const QCameraParameters& params)
{
#if 0
    if(!mCfgControl.mm_camera_is_supported(MM_CAMERA_PARM_WAVELET_DENOISE)) {
        ALOGE("Wavelet Denoise is not supported for this sensor");
        return NO_ERROR;
    }
    const char *str = params.get(QCameraParameters::KEY_DENOISE);
    if (str != NULL) {
        int value = attr_lookup(denoise,
        sizeof(denoise) / sizeof(str_map), str);
        if ((value != NOT_FOUND) &&  (mDenoiseValue != value)) {
        mDenoiseValue =  value;
        mParameters.set(QCameraParameters::KEY_DENOISE, str);
        bool ret = native_set_parms(MM_CAMERA_PARM_WAVELET_DENOISE, sizeof(value),
                                               (void *)&value);
        return ret ? NO_ERROR : UNKNOWN_ERROR;
        }
        return NO_ERROR;
    }
    ALOGE("Invalid Denoise value: %s", (str == NULL) ? "NULL" : str);
#endif
    return BAD_VALUE;
}

status_t QCameraHardwareInterface::setOrientation(const QCameraParameters& params)
{
    const char *str = params.get("orientation");

    if (str != NULL) {
        if (strcmp(str, "portrait") == 0 || strcmp(str, "landscape") == 0) {
            // Camera service needs this to decide if the preview frames and raw
            // pictures should be rotated.
            mParameters.set("orientation", str);
        } else {
            ALOGE("Invalid orientation value: %s", str);
            return BAD_VALUE;
        }
    }
    return NO_ERROR;
}

status_t QCameraHardwareInterface::setPictureFormat(const QCameraParameters& params)
{
    const char * str = params.get(QCameraParameters::KEY_PICTURE_FORMAT);

    if(str != NULL){
        int32_t value = attr_lookup(picture_formats,
                                    sizeof(picture_formats) / sizeof(str_map), str);
        if(value != NOT_FOUND){
            mParameters.set(QCameraParameters::KEY_PICTURE_FORMAT, str);
        } else {
            ALOGE("Invalid Picture Format value: %s", str);
            return BAD_VALUE;
        }
    }
    return NO_ERROR;
}

status_t QCameraHardwareInterface::setRecordingHintValue(const int32_t value)
{
    native_set_parms(MM_CAMERA_PARM_RECORDING_HINT, sizeof(value),
                                           (void *)&value);
    if (value == true){
        native_set_parms(MM_CAMERA_PARM_CAF_ENABLE, sizeof(value),
                                           (void *)&value);
    }
    setDISMode();
    setFullLiveshot();
    return NO_ERROR;
}

status_t QCameraHardwareInterface::setRecordingHint(const QCameraParameters& params)
{

  const char * str = params.get(QCameraParameters::KEY_RECORDING_HINT);

  if(str != NULL){
      int32_t value = attr_lookup(recording_Hints,
                                  sizeof(recording_Hints) / sizeof(str_map), str);
      if(value != NOT_FOUND){
          mRecordingHint = value;
          setRecordingHintValue(mRecordingHint);
          mParameters.set(QCameraParameters::KEY_RECORDING_HINT, str);
          return NO_ERROR;
      } else {
          ALOGE("Invalid Picture Format value: %s", str);
          setDISMode();
          setFullLiveshot();
          return BAD_VALUE;
      }
  }
  setDISMode();
  setFullLiveshot();
  return NO_ERROR;
}

status_t QCameraHardwareInterface::setDISMode() {
  /* Enable DIS only if
   * - Camcorder mode AND
   * - DIS property is set AND
   * - Not in Low power mode. */
  uint32_t value = mRecordingHint && mDisEnabled
                   && !isLowPowerCamcorder();

  ALOGV("%s DIS is %s value = %d", __func__,
          value ? "Enabled" : "Disabled", value);
  native_set_parms(MM_CAMERA_PARM_DIS_ENABLE, sizeof(value),
                                               (void *)&value);
  return NO_ERROR;
}

status_t QCameraHardwareInterface::setFullLiveshot()
{
  /* Enable full size liveshot only if
   * - Camcorder mode AND
   * - Full size liveshot is enabled. */
  uint32_t value = mRecordingHint && mFullLiveshotEnabled
                   && !isLowPowerCamcorder();

  if (((mDimension.picture_width == mVideoWidth) &&
      (mDimension.picture_height == mVideoHeight))) {
    /* If video size matches the live snapshot size
     * turn off full size liveshot to get higher fps. */
    value = 0;
  }

  ALOGV("%s Full size liveshot %s value = %d", __func__,
          value ? "Enabled" : "Disabled", value);
  native_set_parms(MM_CAMERA_PARM_FULL_LIVESHOT, sizeof(value),
                                               (void *)&value);
  return NO_ERROR;
}


isp3a_af_mode_t QCameraHardwareInterface::getAutoFocusMode(
  const QCameraParameters& params)
{
  isp3a_af_mode_t afMode = AF_MODE_MAX;
  afMode = (isp3a_af_mode_t)mFocusMode;
  return afMode;
}

void QCameraHardwareInterface::getPictureSize(int *picture_width,
                                              int *picture_height) const
{
    mParameters.getPictureSize(picture_width, picture_height);
}

void QCameraHardwareInterface::getPreviewSize(int *preview_width,
                                              int *preview_height) const
{
    mParameters.getPreviewSize(preview_width, preview_height);
}

cam_format_t QCameraHardwareInterface::getPreviewFormat() const
{
    cam_format_t foramt = CAMERA_YUV_420_NV21;
    const char *str = mParameters.getPreviewFormat();
    int32_t value = attr_lookup(preview_formats,
                                sizeof(preview_formats)/sizeof(str_map),
                                str);

    if(value != NOT_FOUND) {
        int num = sizeof(preview_format_info_list)/sizeof(preview_format_info_t);
        int i;
        for (i = 0; i < num; i++) {
          if (preview_format_info_list[i].Hal_format == value) {
            foramt = preview_format_info_list[i].mm_cam_format;
            break;
          }
        }
    }

    return foramt;
}

cam_pad_format_t QCameraHardwareInterface::getPreviewPadding() const
{
  return mPreviewFormatInfo.padding;
}

int QCameraHardwareInterface::getJpegQuality() const
{
    return mJpegQuality;
}

int QCameraHardwareInterface::getNumOfSnapshots(void) const
{
    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("persist.camera.snapshot.number", prop, "0");
    ALOGV("%s: prop enable/disable = %d", __func__, atoi(prop));
    if (atoi(prop)) {
        ALOGV("%s: Reading maximum no of snapshots = %d"
             "from properties", __func__, atoi(prop));
        return atoi(prop);
    } else {
        return mParameters.getInt("num-snaps-per-shutter");
    }
}

int QCameraHardwareInterface::
getThumbSizesFromAspectRatio(uint32_t aspect_ratio,
                             int *picture_width,
                             int *picture_height)
{
    for(unsigned int i = 0; i < THUMBNAIL_SIZE_COUNT; i++ ){
        if(thumbnail_sizes[i].aspect_ratio == aspect_ratio)
        {
            *picture_width = thumbnail_sizes[i].width;
            *picture_height = thumbnail_sizes[i].height;
            return NO_ERROR;
        }
    }

    return BAD_VALUE;
}

bool QCameraHardwareInterface::isRawSnapshot()
{
  const char *format = mParameters.getPictureFormat();
    if( format!= NULL &&
       !strcmp(format, QCameraParameters::PIXEL_FORMAT_RAW)){
        return true;
    }
    else{
        return false;
    }
}

status_t QCameraHardwareInterface::setPreviewSizeTable(void)
{
    status_t ret = NO_ERROR;
    mm_camera_dimension_t dim;
    struct camera_size_type* preview_size_table;
    int preview_table_size;
    int i = 0;
    char str[10] = {0};

    /* Initialize table with default values */
    preview_size_table = default_preview_sizes;
    preview_table_size = preview_sizes_count;


    /* Get maximum preview size supported by sensor*/
    memset(&dim, 0, sizeof(mm_camera_dimension_t));
    ret = cam_config_get_parm(mCameraId,
                              MM_CAMERA_PARM_MAX_PREVIEW_SIZE, &dim);
    if (ret != NO_ERROR) {
        ALOGE("%s: Failure getting Max Preview Size supported by camera",
             __func__);
        goto end;
    }

    ALOGV("%s: Max Preview Sizes Supported: %d X %d", __func__,
         dim.width, dim.height);

    for (i = 0; i < preview_table_size; i++) {
        if ((preview_size_table->width <= dim.width) &&
            (preview_size_table->height <= dim.height)) {
            ALOGV("%s: Camera Preview Size Table "
                 "Max width: %d height %d table_size: %d",
                 __func__, preview_size_table->width,
                 preview_size_table->height, preview_table_size - i);
            break;
        }
        preview_size_table++;
    }
    //set preferred preview size to maximum preview size
    sprintf(str, "%dx%d", preview_size_table->width, preview_size_table->height);
    mParameters.set(QCameraParameters::KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO, str);
    ALOGV("KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO = %s", str);

end:
    /* Save the table in global member*/
    mPreviewSizes = preview_size_table;
    mPreviewSizeCount = preview_table_size - i;

    return ret;
}

status_t QCameraHardwareInterface::setPictureSizeTable(void)
{
    status_t ret = NO_ERROR;
    mm_camera_dimension_t dim;
    struct camera_size_type* picture_size_table;
    int picture_table_size;
    int i = 0, count = 0;

    /* Initialize table with default values */
    picture_table_size = sizeof(default_picture_sizes)/
        sizeof(default_picture_sizes[0]);
    picture_size_table = default_picture_sizes;
    mPictureSizes =
        ( struct camera_size_type *)malloc(picture_table_size *
                                           sizeof(struct camera_size_type));
    if (mPictureSizes == NULL) {
        ALOGE("%s: Failre allocating memory to store picture size table",__func__);
        goto end;
    }

    /* Get maximum picture size supported by sensor*/
    memset(&dim, 0, sizeof(mm_camera_dimension_t));
    ret = cam_config_get_parm(mCameraId,
                              MM_CAMERA_PARM_MAX_PICTURE_SIZE, &dim);
    if (ret != NO_ERROR) {
        ALOGE("%s: Failure getting Max Picture Size supported by camera",
             __func__);
        ret = NO_MEMORY;
        free(mPictureSizes);
        mPictureSizes = NULL;
        goto end;
    }

    ALOGV("%s: Max Picture Sizes Supported: %d X %d", __func__,
         dim.width, dim.height);

    for (i = 0; i < picture_table_size; i++) {
        /* We'll store those dimensions whose width AND height
           are less than or equal to maximum supported */
        if ((picture_size_table->width <= dim.width) &&
            (picture_size_table->height <= dim.height)) {
            ALOGV("%s: Camera Picture Size Table "
                 "Max width: %d height %d table_size: %d",
                 __func__, picture_size_table->width,
                 picture_size_table->height, count+1);
            mPictureSizes[count].height = picture_size_table->height;
            mPictureSizes[count].width = picture_size_table->width;
            count++;
        }
        picture_size_table++;
    }
    mPictureSizeCount = count;

end:
     /* In case of error, we use default picture sizes */
     if (ret != NO_ERROR) {
        mPictureSizes = default_picture_sizes;
        mPictureSizeCount = picture_table_size;
    }
    return ret;
}

status_t QCameraHardwareInterface::setVideoSizeTable(void)
{
    status_t ret = NO_ERROR;
    mm_camera_dimension_t dim;
    struct camera_size_type* video_size_table;
    int video_table_size;
    int i = 0, count = 0;
    ALOGV("%s: E", __func__);

    /* Initialize table with default values */
    video_table_size = video_sizes_count;
    video_size_table = default_video_sizes;
    mVideoSizes =
        (struct camera_size_type *)malloc(video_table_size *
                                           sizeof(struct camera_size_type));
    if(mVideoSizes == NULL) {
        ALOGE("%s: error allocating memory to store video size table",__func__);
        ret = BAD_VALUE;
        goto end;
    }

    /* Get maximum video size supported by sensor*/
    memset(&dim, 0, sizeof(mm_camera_dimension_t));
    ret = cam_config_get_parm(mCameraId,
                              MM_CAMERA_PARM_MAX_VIDEO_SIZE, &dim);
    if(ret != NO_ERROR) {
        ALOGE("%s: error getting Max Video Size supported by camera",
             __func__);
        ret = NO_MEMORY;
        free(mVideoSizes);
        mVideoSizes = NULL;
        ret = BAD_VALUE;
        goto end;
    }

    ALOGV("%s: Max Video Size Supported: %d X %d", __func__,
         dim.width, dim.height);

    for(i=0; i < video_table_size; i++) {
        /* We'll store those dimensions whose width AND height
           are less than or equal to maximum supported */
        if((video_size_table->width <= dim.width) &&
            (video_size_table->height <= dim.height)) {
            ALOGV("%s: Supported Video Size [%d] = %dx%d", __func__, count, video_size_table->width,
                                    video_size_table->height);
            mVideoSizes[count].height = video_size_table->height;
            mVideoSizes[count].width = video_size_table->width;
            count++;
        }
        video_size_table++;
    }
    mVideoSizeCount = count;

end:
    ALOGV("%s: X", __func__);
    return ret;
}

void QCameraHardwareInterface::freeVideoSizeTable(void)
{
    if(mVideoSizes != NULL)
    {
        free(mVideoSizes);
    }
    mVideoSizeCount = 0;
}


void QCameraHardwareInterface::freePictureTable(void)
{
    /* If we couldn't allocate memory to store picture table
       we use the picture table pointer to point to default
       picture table array. In that case we cannot free it.*/
    if ((mPictureSizes != default_picture_sizes) && mPictureSizes) {
        free(mPictureSizes);
    }
}

status_t QCameraHardwareInterface::setHistogram(int histogram_en)
{
    ALOGV("setHistogram: E");
    if(mStatsOn == histogram_en) {
        return NO_ERROR;
    }

    mSendData = histogram_en;
    mStatsOn = histogram_en;
    mCurrentHisto = -1;
    mStatSize = sizeof(uint32_t)* HISTOGRAM_STATS_SIZE;

    if (histogram_en == QCAMERA_PARM_ENABLE) {
        /*Currently the Ashmem is multiplying the buffer size with total number
        of buffers and page aligning. This causes a crash in JNI as each buffer
        individually expected to be page aligned  */
        int page_size_minus_1 = getpagesize() - 1;
        int statSize = sizeof (camera_preview_histogram_info );
        int32_t mAlignedStatSize = ((statSize + page_size_minus_1) & (~page_size_minus_1));
#if 0
        mStatHeap =
        new AshmemPool(mAlignedStatSize, 3, statSize, "stat");
        if (!mStatHeap->initialized()) {
            ALOGE("Stat Heap X failed ");
            mStatHeap.clear();
            mStatHeap = NULL;
            return UNKNOWN_ERROR;
        }
#endif
        for(int cnt = 0; cnt<3; cnt++) {
                mStatsMapped[cnt]=mGetMemory(-1, mStatSize, 1, mCallbackCookie);
                if(mStatsMapped[cnt] == NULL) {
                    ALOGE("Failed to get camera memory for stats heap index: %d", cnt);
                    return(-1);
                } else {
                   ALOGV("Received following info for stats mapped data:%p,handle:%p, size:%d,release:%p",
                   mStatsMapped[cnt]->data ,mStatsMapped[cnt]->handle, mStatsMapped[cnt]->size, mStatsMapped[cnt]->release);
                }
                mHistServer.size = sizeof(camera_preview_histogram_info);
#ifdef USE_ION
                if(allocate_ion_memory(&mHistServer, cnt, ION_IOMMU_HEAP_ID) < 0) {
                  ALOGE("%s ION alloc failed\n", __func__);
                  return -1;
                }
#else
		        mHistServer.fd[cnt] = open("/dev/pmem_adsp", O_RDWR|O_SYNC);
		        if(mHistServer.fd[cnt] <= 0) {
			      ALOGE("%s: no pmem for frame %d", __func__, cnt);
			      return -1;
		        }
#endif
                mHistServer.camera_memory[cnt]=mGetMemory(mHistServer.fd[cnt],mHistServer.size, 1, mCallbackCookie);
                if(mHistServer.camera_memory[cnt] == NULL) {
                    ALOGE("Failed to get camera memory for server side histogram index: %d", cnt);
                    return(-1);
                } else {
                   ALOGV("Received following info for server side histogram data:%p,handle:%p, size:%d,release:%p",
                   mHistServer.camera_memory[cnt]->data ,mHistServer.camera_memory[cnt]->handle,
                        mHistServer.camera_memory[cnt]->size, mHistServer.camera_memory[cnt]->release);
                }
                /*Register buffer at back-end*/
                if (NO_ERROR != sendMappingBuf(0, cnt, mHistServer.fd[cnt],
                                                   mHistServer.size, mCameraId,
                                               CAM_SOCK_MSG_TYPE_HIST_MAPPING)) {
                    ALOGE("%s could not send buffer to back-end\n", __func__);
                }
        }
    }
    ALOGV("Setting histogram = %d", histogram_en);
    native_set_parms(MM_CAMERA_PARM_HISTOGRAM, sizeof(int), &histogram_en);
    if(histogram_en == QCAMERA_PARM_DISABLE)
    {
        //release memory
        for(int i=0; i<3; i++){
            if(mStatsMapped[i] != NULL) {
                mStatsMapped[i]->release(mStatsMapped[i]);
            }
            /*Unregister buffer at back-end */
            if (NO_ERROR != sendUnMappingBuf(0, i, mCameraId, CAM_SOCK_MSG_TYPE_HIST_UNMAPPING)) {
              ALOGE("%s could not unregister buffer from back-end\n", __func__);
            }
            if(mHistServer.camera_memory[i] != NULL) {
                mHistServer.camera_memory[i]->release(mHistServer.camera_memory[i]);
            }
            close(mHistServer.fd[i]);
#ifdef USE_ION
            deallocate_ion_memory(&mHistServer, i);
#endif
        }
    }
    return NO_ERROR;
}

status_t QCameraHardwareInterface::setZSLBurstLookBack(const QCameraParameters& params)
{
  const char *v = params.get("capture-burst-retroactive");
  if (v) {
    int look_back = atoi(v);
    ALOGV("%s: look_back =%d", __func__, look_back);
    mParameters.set("capture-burst-retroactive", look_back);
  }
  return NO_ERROR;
}

status_t QCameraHardwareInterface::setZSLBurstInterval(const QCameraParameters& params)
{
  mZslInterval = BURST_INTREVAL_DEFAULT;
  const char *v = params.get("capture-burst-interval");
  if (v) {
    int interval = atoi(v);
    ALOGV("%s: Interval =%d", __func__, interval);
    if(interval < BURST_INTREVAL_MIN ||interval > BURST_INTREVAL_MAX ) {
      return BAD_VALUE;
    }
    mZslInterval =  interval;
  }
  return NO_ERROR;
}

int QCameraHardwareInterface::getZSLBurstInterval( void )
{
  int val;

  if (mZslInterval == BURST_INTREVAL_DEFAULT) {
    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("persist.camera.zsl.interval", prop, "1");
    val = atoi(prop);
    ALOGV("%s: prop interval = %d", __func__, val);
  } else {
    val = mZslInterval;
  }
  return val;
}


int QCameraHardwareInterface::getZSLQueueDepth(void) const
{
    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("persist.camera.zsl.queuedepth", prop, "2");
    ALOGV("%s: prop = %d", __func__, atoi(prop));
    return atoi(prop);
}

int QCameraHardwareInterface::getZSLBackLookCount(void) const
{
    int look_back;
    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("persist.camera.zsl.backlookcnt", prop, "0");
    ALOGV("%s: prop = %d", __func__, atoi(prop));
    look_back = atoi(prop);
    if (look_back == 0 ) {
      look_back = mParameters.getInt("capture-burst-retroactive");
      ALOGV("%s: look_back = %d", __func__, look_back);
    }
    return look_back;
}

bool QCameraHardwareInterface::getFlashCondition(void)
{
    int32_t rc = 0;
    bool flash_cond = false;
    aec_info_for_flash_t lowLightForZSL;

    lowLightForZSL.aec_index_for_zsl = 0;
    lowLightForZSL.zsl_flash_enable = 0;

    if(myMode & CAMERA_ZSL_MODE){
        switch(mLedStatusForZsl) {
            case LED_MODE_ON:
                flash_cond = true;
                break;
            case LED_MODE_AUTO:
                rc = cam_config_get_parm(mCameraId,
                        MM_CAMERA_GET_PARM_LOW_LIGHT_FOR_ZSL, &lowLightForZSL);
                if(MM_CAMERA_OK == rc) {
                    if(lowLightForZSL.zsl_flash_enable != 0)
                        flash_cond = true;
                    else
                        flash_cond = false;
                }
                else
                    ALOGE("%s: Failed to get lowLightForZSL, rc %d", __func__, rc);
                break;
             default:
                break;
        }
    }

    ALOGV("%s: myMode %d, flash mode %d, flash condition %d",
        __func__, myMode, mLedStatusForZsl, flash_cond);
    return flash_cond;
}

//EXIF functions
void QCameraHardwareInterface::deinitExifData()
{
    ALOGV("Clearing EXIF data");
    for(int i=0; i<MAX_EXIF_TABLE_ENTRIES; i++)
    {
        //clear all data
        memset(&mExifData[i], 0x00, sizeof(exif_tags_info_t));
    }
    mExifTableNumEntries = 0;
}

void QCameraHardwareInterface::addExifTag(exif_tag_id_t tagid, exif_tag_type_t type,
                        uint32_t count, uint8_t copy, void *data) {

    if(mExifTableNumEntries >= MAX_EXIF_TABLE_ENTRIES) {
        ALOGE("%s: Number of entries exceeded limit", __func__);
        return;
    }
    int index = mExifTableNumEntries;
    mExifData[index].tag_id = tagid;
    mExifData[index].tag_entry.type = type;
    mExifData[index].tag_entry.count = count;
    mExifData[index].tag_entry.copy = copy;
    if((type == EXIF_RATIONAL) && (count > 1))
        mExifData[index].tag_entry.data._rats = (rat_t *)data;
    if((type == EXIF_RATIONAL) && (count == 1))
        mExifData[index].tag_entry.data._rat = *(rat_t *)data;
    else if(type == EXIF_ASCII)
        mExifData[index].tag_entry.data._ascii = (char *)data;
    else if(type == EXIF_BYTE)
        mExifData[index].tag_entry.data._byte = *(uint8_t *)data;
    else if((type == EXIF_SHORT) && (count > 1))
        mExifData[index].tag_entry.data._shorts = (uint16_t *)data;
    else if((type == EXIF_SHORT) && (count == 1))
        mExifData[index].tag_entry.data._short = *(uint16_t *)data;
    // Increase number of entries
    mExifTableNumEntries++;
}

void QCameraHardwareInterface::initExifData(){
    short val_short;
    char value[PROPERTY_VALUE_MAX];
    if (property_get("ro.product.manufacturer", value, "QCOM-AA") > 0) {
        strncpy(mExifValues.make, value, 19);
        mExifValues.make[19] = '\0';
        addExifTag(EXIFTAGID_MAKE, EXIF_ASCII, strlen(value) + 1, 1, (void *)mExifValues.make);
    } else {
        ALOGE("%s: getExifMaker failed", __func__);
    }

    if (property_get("ro.product.model", value, "QCAM-AA") > 0) {
        strncpy(mExifValues.model, value, 19);
        mExifValues.model[19] = '\0';
        addExifTag(EXIFTAGID_MODEL, EXIF_ASCII, strlen(value) + 1, 1, (void *)mExifValues.model);
    } else {
        ALOGE("%s: getExifModel failed", __func__);
    }

    if(mExifValues.dateTime) {
        addExifTag(EXIFTAGID_EXIF_DATE_TIME_ORIGINAL, EXIF_ASCII,
                20, 1, (void *)mExifValues.dateTime);
        addExifTag(EXIFTAGID_EXIF_DATE_TIME_DIGITIZED, EXIF_ASCII,
                20, 1, (void *)mExifValues.dateTime);
    }
    addExifTag(EXIFTAGID_FOCAL_LENGTH, EXIF_RATIONAL, 1, 1, (void *)&(mExifValues.focalLength));
    addExifTag(EXIFTAGID_ISO_SPEED_RATING,EXIF_SHORT,1,1,(void *)&(mExifValues.isoSpeed));

    // normal f_number is from 1.2 to 22, but I'd like to put some margin.
    if(mExifValues.f_number.num>0 && mExifValues.f_number.num<3200) {
        addExifTag(EXIFTAGID_F_NUMBER,EXIF_RATIONAL,1,1,(void *)&(mExifValues.f_number));
        addExifTag(EXIFTAGID_APERTURE,EXIF_RATIONAL,1,1,(void *)&(mExifValues.f_number));
    }


    if(mExifValues.mGpsProcess) {
        addExifTag(EXIFTAGID_GPS_PROCESSINGMETHOD, EXIF_ASCII,
                EXIF_ASCII_PREFIX_SIZE + strlen(mExifValues.gpsProcessingMethod + EXIF_ASCII_PREFIX_SIZE) + 1,
                1, (void *)mExifValues.gpsProcessingMethod);
    }

    if(mExifValues.mLatitude) {
        addExifTag(EXIFTAGID_GPS_LATITUDE, EXIF_RATIONAL, 3, 1, (void *)mExifValues.latitude);

        if(mExifValues.latRef) {
            addExifTag(EXIFTAGID_GPS_LATITUDE_REF, EXIF_ASCII, 2,
                    1, (void *)mExifValues.latRef);
        }
    }

    if(mExifValues.mLongitude) {
        addExifTag(EXIFTAGID_GPS_LONGITUDE, EXIF_RATIONAL, 3, 1, (void *)mExifValues.longitude);

        if(mExifValues.lonRef) {
            addExifTag(EXIFTAGID_GPS_LONGITUDE_REF, EXIF_ASCII, 2,
                    1, (void *)mExifValues.lonRef);
        }
    }

    if(mExifValues.mAltitude) {
        addExifTag(EXIFTAGID_GPS_ALTITUDE, EXIF_RATIONAL, 1,
                1, (void *)&(mExifValues.altitude));

        addExifTag(EXIFTAGID_GPS_ALTITUDE_REF, EXIF_BYTE, 1, 1, (void *)&mExifValues.mAltitude_ref);
    }

    if(mExifValues.mTimeStamp) {
        time_t unixTime;
        struct tm *UTCTimestamp;

        unixTime = (time_t)mExifValues.mGPSTimestamp;
        UTCTimestamp = gmtime(&unixTime);

        strftime(mExifValues.gpsDateStamp, sizeof(mExifValues.gpsDateStamp), "%Y:%m:%d", UTCTimestamp);
        addExifTag(EXIFTAGID_GPS_DATESTAMP, EXIF_ASCII,
                strlen(mExifValues.gpsDateStamp)+1 , 1, (void *)mExifValues.gpsDateStamp);

        mExifValues.gpsTimeStamp[0] = getRational(UTCTimestamp->tm_hour, 1);
        mExifValues.gpsTimeStamp[1] = getRational(UTCTimestamp->tm_min, 1);
        mExifValues.gpsTimeStamp[2] = getRational(UTCTimestamp->tm_sec, 1);

        addExifTag(EXIFTAGID_GPS_TIMESTAMP, EXIF_RATIONAL,
                3, 1, (void *)mExifValues.gpsTimeStamp);
        ALOGV("EXIFTAGID_GPS_TIMESTAMP set");
    }
    if(mExifValues.exposure_time.num || mExifValues.exposure_time.denom)
        addExifTag(EXIFTAGID_EXPOSURE_TIME, EXIF_RATIONAL, 1, 1, (void *)&mExifValues.exposure_time);

    bool flashCondition = getFlashCondition();
    addExifTag(EXIFTAGID_FLASH, EXIF_SHORT, 1, 1, &flashCondition);
    if (mExifValues.mWbMode == CAMERA_WB_AUTO)
        val_short = 0;
    else
        val_short = 1;
    addExifTag(EXIFTAGID_WHITE_BALANCE, EXIF_SHORT, 1, 1, &val_short);

    addExifTag(EXIFTAGID_SUBSEC_TIME, EXIF_ASCII, 7, 1, (void *)mExifValues.subsecTime);

    addExifTag(EXIFTAGID_SUBSEC_TIME_ORIGINAL, EXIF_ASCII, 7, 1, (void *)mExifValues.subsecTime);

    addExifTag(EXIFTAGID_SUBSEC_TIME_DIGITIZED, EXIF_ASCII, 7, 1, (void *)mExifValues.subsecTime);
}

//Add all exif tags in this function
void QCameraHardwareInterface::setExifTags()
{
    const char *str;

    //set TimeStamp
    str = mParameters.get(QCameraParameters::KEY_EXIF_DATETIME);
    if(str != NULL) {
        strncpy(mExifValues.dateTime, str, 19);
        mExifValues.dateTime[19] = '\0';
    }

    //Set focal length
    int focalLengthValue = (int) (mParameters.getFloat(
            QCameraParameters::KEY_FOCAL_LENGTH) * FOCAL_LENGTH_DECIMAL_PRECISION);

    mExifValues.focalLength = getRational(focalLengthValue, FOCAL_LENGTH_DECIMAL_PRECISION);

    focus_distances_info_t focusDistances;
    status_t rc = NO_ERROR;
    rc = cam_config_get_parm(mCameraId, MM_CAMERA_PARM_FOCAL_LENGTH,(void *)&focusDistances);
    if (rc == MM_CAMERA_OK){
        uint16_t temp1;
        rat_t temp;
        if(mIsoValue == 0) // ISO is auto
        {
            temp1 = (uint16_t)(focusDistances.real_gain + 0.5)*100;
            mExifValues.isoSpeed = temp1;
            ALOGV("The new ISO value is %d", temp1);
        }
        else{
            temp1 = iso_speed_values[mIsoValue];
            mExifValues.isoSpeed = temp1;
            ALOGV("else The new ISO value is %d", temp1);
        }

        if(focusDistances.exp_time <= 0) // avoid zero-divide problem
            focusDistances.exp_time = 0.01668; // expoure time will be 1/60 s

        uint16_t temp2 = (uint16_t)(focusDistances.exp_time * 100000);
        temp2 = (uint16_t)(100000 / temp2);
        temp.num = 1;
        temp.denom = temp2;
        memcpy(&mExifValues.exposure_time, &temp, sizeof(mExifValues.exposure_time));
        ALOGV(" The exposure value is %f", temp2);
    }
    //get time and date from system
    time_t rawtime;
    struct tm * timeinfo;
    time(&rawtime);
    timeinfo = localtime (&rawtime);
    //Write datetime according to EXIF Spec
    //"YYYY:MM:DD HH:MM:SS" (20 chars including \0)
    snprintf(mExifValues.dateTime, 20, "%04d:%02d:%02d %02d:%02d:%02d",
            timeinfo->tm_year + 1900, timeinfo->tm_mon + 1,
            timeinfo->tm_mday, timeinfo->tm_hour,
            timeinfo->tm_min, timeinfo->tm_sec);
    //set gps tags

    struct timeval tv;
    gettimeofday(&tv, NULL);
    snprintf(mExifValues.subsecTime, 7, "%06ld", tv.tv_usec);

    mExifValues.mWbMode = mParameters.getInt(QCameraParameters::KEY_WHITE_BALANCE);
    setExifTagsGPS();
}

void QCameraHardwareInterface::setExifTagsGPS()
{
    const char *str = NULL;

    //Set GPS processing method
    str = mParameters.get(QCameraParameters::KEY_GPS_PROCESSING_METHOD);
    if(str != NULL) {
        memcpy(mExifValues.gpsProcessingMethod, ExifAsciiPrefix, EXIF_ASCII_PREFIX_SIZE);
        strncpy(mExifValues.gpsProcessingMethod + EXIF_ASCII_PREFIX_SIZE, str,
                GPS_PROCESSING_METHOD_SIZE - 1);
        mExifValues.gpsProcessingMethod[EXIF_ASCII_PREFIX_SIZE + GPS_PROCESSING_METHOD_SIZE-1] = '\0';
        ALOGV("EXIFTAGID_GPS_PROCESSINGMETHOD = %s %s", mExifValues.gpsProcessingMethod,
                mExifValues.gpsProcessingMethod+8);
        mExifValues.mGpsProcess  = true;
    }else{
        mExifValues.mGpsProcess = false;
    }
    str = NULL;

    //Set Latitude
    str = mParameters.get(QCameraParameters::KEY_GPS_LATITUDE);
    if(str != NULL) {
        parseGPSCoordinate(str, mExifValues.latitude);
        ALOGV("EXIFTAGID_GPS_LATITUDE = %s", str);

        //set Latitude Ref
        float latitudeValue = mParameters.getFloat(QCameraParameters::KEY_GPS_LATITUDE);
        if(latitudeValue < 0.0f) {
            mExifValues.latRef[0] = 'S';
        } else {
            mExifValues.latRef[0] = 'N';
        }
        mExifValues.latRef[1] = '\0';
        mExifValues.mLatitude = true;
        mParameters.set(QCameraParameters::KEY_GPS_LATITUDE_REF,mExifValues.latRef);
        ALOGV("EXIFTAGID_GPS_LATITUDE_REF = %s", mExifValues.latRef);
    }else{
        mExifValues.mLatitude = false;
    }

    //set Longitude
    str = NULL;
    str = mParameters.get(QCameraParameters::KEY_GPS_LONGITUDE);
    if(str != NULL) {
        parseGPSCoordinate(str, mExifValues.longitude);
        ALOGV("EXIFTAGID_GPS_LONGITUDE = %s", str);

        //set Longitude Ref
        float longitudeValue = mParameters.getFloat(QCameraParameters::KEY_GPS_LONGITUDE);
        if(longitudeValue < 0.0f) {
            mExifValues.lonRef[0] = 'W';
        } else {
            mExifValues.lonRef[0] = 'E';
        }
        mExifValues.lonRef[1] = '\0';
        mExifValues.mLongitude = true;
        ALOGV("EXIFTAGID_GPS_LONGITUDE_REF = %s", mExifValues.lonRef);
        mParameters.set(QCameraParameters::KEY_GPS_LONGITUDE_REF, mExifValues.lonRef);
    }else{
        mExifValues.mLongitude = false;
    }

    //set Altitude
    str = mParameters.get(QCameraParameters::KEY_GPS_ALTITUDE);
    if(str != NULL) {
        double value = atof(str);
        mExifValues.mAltitude_ref = 0;
        if(value < 0){
            mExifValues.mAltitude_ref = 1;
            value = -value;
        }
        mExifValues.altitude = getRational(value*1000, 1000);
        mExifValues.mAltitude = true;
        //set AltitudeRef
        mParameters.set(QCameraParameters::KEY_GPS_ALTITUDE_REF, mExifValues.mAltitude_ref);
        ALOGV("EXIFTAGID_GPS_ALTITUDE = %f", value);
    }else{
        mExifValues.mAltitude = false;
    }

    //set Gps TimeStamp
    str = NULL;
    str = mParameters.get(QCameraParameters::KEY_GPS_TIMESTAMP);
    if(str != NULL) {
        mExifValues.mTimeStamp = true;
        mExifValues.mGPSTimestamp = atol(str);
    }else{
        mExifValues.mTimeStamp = false;
    }
}

//latlonString is string formatted coordinate
//coord is rat_t[3]
void QCameraHardwareInterface::parseGPSCoordinate(const char *latlonString, rat_t* coord)
{
    if(coord == NULL) {
        ALOGE("%s: error, invalid argument coord == NULL", __func__);
        return;
    }
    float degF = fabs(atof(latlonString));
    float minF = (degF- (int) degF) * 60;
    float secF = (minF - (int) minF) * 60;

    coord[0] = getRational((int) degF, 1);
    coord[1] = getRational((int) minF, 1);
    coord[2] = getRational((int) (secF * 10000), 10000);
}

bool QCameraHardwareInterface::isLowPowerCamcorder() {

    if (mPowerMode == LOW_POWER)
        return true;

    if(mHFRLevel > 1) /* hard code the value now. Need to move tgtcommon to camear.h */
        return true;

    return false;
}

status_t QCameraHardwareInterface::setNoDisplayMode(const QCameraParameters& params)
{
    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("persist.camera.nodisplay", prop, "0");
    int prop_val = atoi(prop);

    if (prop_val == 0) {
        const char *str_val  = params.get("no-display-mode");
        if(str_val && strlen(str_val) > 0) {
            mNoDisplayMode = atoi(str_val);
        } else {
            mNoDisplayMode = 0;
        }
        ALOGV("Param mNoDisplayMode =%d", mNoDisplayMode);
    } else {
        mNoDisplayMode = prop_val;
        ALOGV("prop mNoDisplayMode =%d", mNoDisplayMode);
    }
    return NO_ERROR;
}

status_t QCameraHardwareInterface::setCAFLockCancel(void)
{
    ALOGV("%s : E", __func__);

    //for CAF unlock
    if(MM_CAMERA_OK!=cam_ops_action(mCameraId,false,MM_CAMERA_OPS_FOCUS,NULL )) {
        ALOGE("%s: AF command failed err:%d error %s",__func__, errno,strerror(errno));
        return -1;
    }

    ALOGV("%s : X", __func__);
    return NO_ERROR;
}

void QCameraHardwareInterface::prepareVideoPicture(bool disable){
    String8 str;
    char buffer[32];

    if(disable) {
        sprintf(buffer, "%dx%d", mDimension.video_width, mDimension.video_height);
        str.append(buffer);

        mParameters.setPictureSize(mDimension.video_width, mDimension.video_height);
        mParameters.set(QCameraParameters::KEY_SUPPORTED_PICTURE_SIZES,
                str.string());
        ALOGV("%s: Video Picture size supported = %d X %d",
                __func__,mDimension.video_width,mDimension.video_height);
    }else{
        //Set Picture Size
        mParameters.setPictureSize(mDimension.picture_width, mDimension.picture_height);
        mParameters.set(QCameraParameters::KEY_SUPPORTED_PICTURE_SIZES,
                mPictureSizeValues.string());
    }
}

}; /*namespace android */
