/* Copyright (c) 2014, The Linux Foundation. All rights reserved.
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

#include "cam_intf.h"

void *get_pointer_of(cam_intf_parm_type_t meta_id,
        const metadata_buffer_t* metadata)
{
    switch(meta_id) {
        case CAM_INTF_META_HISTOGRAM:
            return POINTER_OF_META(CAM_INTF_META_HISTOGRAM, metadata);
        case CAM_INTF_META_FACE_DETECTION:
            return POINTER_OF_META(CAM_INTF_META_FACE_DETECTION, metadata);
        case CAM_INTF_META_AUTOFOCUS_DATA:
            return POINTER_OF_META(CAM_INTF_META_AUTOFOCUS_DATA, metadata);
        case CAM_INTF_META_CROP_DATA:
            return POINTER_OF_META(CAM_INTF_META_CROP_DATA, metadata);
        case CAM_INTF_META_PREP_SNAPSHOT_DONE:
            return POINTER_OF_META(CAM_INTF_META_PREP_SNAPSHOT_DONE, metadata);
        case CAM_INTF_META_GOOD_FRAME_IDX_RANGE:
            return POINTER_OF_META(CAM_INTF_META_GOOD_FRAME_IDX_RANGE, metadata);
        case CAM_INTF_META_ASD_HDR_SCENE_DATA:
            return POINTER_OF_META(CAM_INTF_META_ASD_HDR_SCENE_DATA, metadata);
        case CAM_INTF_META_ASD_SCENE_TYPE:
            return POINTER_OF_META(CAM_INTF_META_ASD_SCENE_TYPE, metadata);
        case CAM_INTF_META_CHROMATIX_LITE_ISP:
            return POINTER_OF_META(CAM_INTF_META_CHROMATIX_LITE_ISP, metadata);
        case CAM_INTF_META_CHROMATIX_LITE_PP:
            return POINTER_OF_META(CAM_INTF_META_CHROMATIX_LITE_PP, metadata);
        case CAM_INTF_META_CHROMATIX_LITE_AE:
            return POINTER_OF_META(CAM_INTF_META_CHROMATIX_LITE_AE, metadata);
        case CAM_INTF_META_CHROMATIX_LITE_AWB:
            return POINTER_OF_META(CAM_INTF_META_CHROMATIX_LITE_AWB, metadata);
        case CAM_INTF_META_CHROMATIX_LITE_AF:
            return POINTER_OF_META(CAM_INTF_META_CHROMATIX_LITE_AF, metadata);
        case CAM_INTF_META_CHROMATIX_LITE_ASD:
            return POINTER_OF_META(CAM_INTF_META_CHROMATIX_LITE_ASD, metadata);
        case CAM_INTF_META_FRAME_NUMBER_VALID:
            return POINTER_OF_META(CAM_INTF_META_FRAME_NUMBER_VALID, metadata);
        case CAM_INTF_META_URGENT_FRAME_NUMBER_VALID:
            return POINTER_OF_META(CAM_INTF_META_URGENT_FRAME_NUMBER_VALID, metadata);
        case CAM_INTF_META_FRAME_DROPPED:
            return POINTER_OF_META(CAM_INTF_META_FRAME_DROPPED, metadata);
        case CAM_INTF_META_FRAME_NUMBER:
            return POINTER_OF_META(CAM_INTF_META_FRAME_NUMBER, metadata);
        case CAM_INTF_META_URGENT_FRAME_NUMBER:
            return POINTER_OF_META(CAM_INTF_META_URGENT_FRAME_NUMBER, metadata);
        case CAM_INTF_META_COLOR_CORRECT_MODE:
            return POINTER_OF_META(CAM_INTF_META_COLOR_CORRECT_MODE, metadata);
        case CAM_INTF_META_COLOR_CORRECT_TRANSFORM:
            return POINTER_OF_META(CAM_INTF_META_COLOR_CORRECT_TRANSFORM, metadata);
        case CAM_INTF_META_COLOR_CORRECT_GAINS:
            return POINTER_OF_META(CAM_INTF_META_COLOR_CORRECT_GAINS, metadata);
        case CAM_INTF_META_PRED_COLOR_CORRECT_TRANSFORM:
            return POINTER_OF_META(CAM_INTF_META_PRED_COLOR_CORRECT_TRANSFORM, metadata);
        case CAM_INTF_META_PRED_COLOR_CORRECT_GAINS:
            return POINTER_OF_META(CAM_INTF_META_PRED_COLOR_CORRECT_GAINS, metadata);
        case CAM_INTF_META_AEC_ROI:
            return POINTER_OF_META(CAM_INTF_META_AEC_ROI, metadata);
        case CAM_INTF_META_AEC_STATE:
            return POINTER_OF_META(CAM_INTF_META_AEC_STATE, metadata);
        case CAM_INTF_PARM_FOCUS_MODE:
            return POINTER_OF_META(CAM_INTF_PARM_FOCUS_MODE, metadata);
        case CAM_INTF_META_AF_ROI:
            return POINTER_OF_META(CAM_INTF_META_AF_ROI, metadata);
        case CAM_INTF_META_AF_STATE:
            return POINTER_OF_META(CAM_INTF_META_AF_STATE, metadata);
        case CAM_INTF_PARM_WHITE_BALANCE:
            return POINTER_OF_META(CAM_INTF_PARM_WHITE_BALANCE, metadata);
        case CAM_INTF_META_AWB_REGIONS:
            return POINTER_OF_META(CAM_INTF_META_AWB_REGIONS, metadata);
        case CAM_INTF_META_AWB_STATE:
            return POINTER_OF_META(CAM_INTF_META_AWB_STATE, metadata);
        case CAM_INTF_META_BLACK_LEVEL_LOCK:
            return POINTER_OF_META(CAM_INTF_META_BLACK_LEVEL_LOCK, metadata);
        case CAM_INTF_META_MODE:
            return POINTER_OF_META(CAM_INTF_META_MODE, metadata);
        case CAM_INTF_META_EDGE_MODE:
            return POINTER_OF_META(CAM_INTF_META_EDGE_MODE, metadata);
        case CAM_INTF_META_FLASH_POWER:
            return POINTER_OF_META(CAM_INTF_META_FLASH_POWER, metadata);
        case CAM_INTF_META_FLASH_FIRING_TIME:
            return POINTER_OF_META(CAM_INTF_META_FLASH_FIRING_TIME, metadata);
        case CAM_INTF_META_FLASH_MODE:
            return POINTER_OF_META(CAM_INTF_META_FLASH_MODE, metadata);
        case CAM_INTF_META_FLASH_STATE:
            return POINTER_OF_META(CAM_INTF_META_FLASH_STATE, metadata);
        case CAM_INTF_META_HOTPIXEL_MODE:
            return POINTER_OF_META(CAM_INTF_META_HOTPIXEL_MODE, metadata);
        case CAM_INTF_META_LENS_APERTURE:
            return POINTER_OF_META(CAM_INTF_META_LENS_APERTURE, metadata);
        case CAM_INTF_META_LENS_FILTERDENSITY:
            return POINTER_OF_META(CAM_INTF_META_LENS_FILTERDENSITY, metadata);
        case CAM_INTF_META_LENS_FOCAL_LENGTH:
            return POINTER_OF_META(CAM_INTF_META_LENS_FOCAL_LENGTH, metadata);
        case CAM_INTF_META_LENS_FOCUS_DISTANCE:
            return POINTER_OF_META(CAM_INTF_META_LENS_FOCUS_DISTANCE, metadata);
        case CAM_INTF_META_LENS_FOCUS_RANGE:
            return POINTER_OF_META(CAM_INTF_META_LENS_FOCUS_RANGE, metadata);
        case CAM_INTF_META_LENS_STATE:
            return POINTER_OF_META(CAM_INTF_META_LENS_STATE, metadata);
        case CAM_INTF_META_LENS_OPT_STAB_MODE:
            return POINTER_OF_META(CAM_INTF_META_LENS_OPT_STAB_MODE, metadata);
        case CAM_INTF_META_NOISE_REDUCTION_MODE:
            return POINTER_OF_META(CAM_INTF_META_NOISE_REDUCTION_MODE, metadata);
        case CAM_INTF_META_NOISE_REDUCTION_STRENGTH:
            return POINTER_OF_META(CAM_INTF_META_NOISE_REDUCTION_STRENGTH, metadata);
        case CAM_INTF_META_SCALER_CROP_REGION:
            return POINTER_OF_META(CAM_INTF_META_SCALER_CROP_REGION, metadata);
        case CAM_INTF_META_SCENE_FLICKER:
            return POINTER_OF_META(CAM_INTF_META_SCENE_FLICKER, metadata);
        case CAM_INTF_META_SENSOR_EXPOSURE_TIME:
            return POINTER_OF_META(CAM_INTF_META_SENSOR_EXPOSURE_TIME, metadata);
        case CAM_INTF_META_SENSOR_FRAME_DURATION:
            return POINTER_OF_META(CAM_INTF_META_SENSOR_FRAME_DURATION, metadata);
        case CAM_INTF_META_SENSOR_SENSITIVITY:
            return POINTER_OF_META(CAM_INTF_META_SENSOR_SENSITIVITY, metadata);
        case CAM_INTF_META_SENSOR_TIMESTAMP:
            return POINTER_OF_META(CAM_INTF_META_SENSOR_TIMESTAMP, metadata);
        case CAM_INTF_META_SHADING_MODE:
            return POINTER_OF_META(CAM_INTF_META_SHADING_MODE, metadata);
        case CAM_INTF_META_STATS_FACEDETECT_MODE:
            return POINTER_OF_META(CAM_INTF_META_STATS_FACEDETECT_MODE, metadata);
        case CAM_INTF_META_STATS_HISTOGRAM_MODE:
            return POINTER_OF_META(CAM_INTF_META_STATS_HISTOGRAM_MODE, metadata);
        case CAM_INTF_META_STATS_SHARPNESS_MAP_MODE:
            return POINTER_OF_META(CAM_INTF_META_STATS_SHARPNESS_MAP_MODE, metadata);
        case CAM_INTF_META_STATS_SHARPNESS_MAP:
            return POINTER_OF_META(CAM_INTF_META_STATS_SHARPNESS_MAP, metadata);
        case CAM_INTF_META_TONEMAP_CURVES:
            return POINTER_OF_META(CAM_INTF_META_TONEMAP_CURVES, metadata);
        case CAM_INTF_META_LENS_SHADING_MAP:
            return POINTER_OF_META(CAM_INTF_META_LENS_SHADING_MAP, metadata);
        case CAM_INTF_META_AEC_INFO:
            return POINTER_OF_META(CAM_INTF_META_AEC_INFO, metadata);
        case CAM_INTF_META_SENSOR_INFO:
            return POINTER_OF_META(CAM_INTF_META_SENSOR_INFO, metadata);
        case CAM_INTF_META_ASD_SCENE_CAPTURE_TYPE:
            return POINTER_OF_META(CAM_INTF_META_ASD_SCENE_CAPTURE_TYPE, metadata);
        case CAM_INTF_PARM_EFFECT:
            return POINTER_OF_META(CAM_INTF_PARM_EFFECT, metadata);
        case CAM_INTF_META_PRIVATE_DATA:
            return POINTER_OF_META(CAM_INTF_META_PRIVATE_DATA, metadata);
        case CAM_INTF_PARM_HAL_VERSION:
            return POINTER_OF_META(CAM_INTF_PARM_HAL_VERSION, metadata);
        case CAM_INTF_PARM_ANTIBANDING:
            return POINTER_OF_META(CAM_INTF_PARM_ANTIBANDING, metadata);
        case CAM_INTF_PARM_EXPOSURE_COMPENSATION:
            return POINTER_OF_META(CAM_INTF_PARM_EXPOSURE_COMPENSATION, metadata);
        case CAM_INTF_PARM_EV_STEP:
            return POINTER_OF_META(CAM_INTF_PARM_EV_STEP, metadata);
        case CAM_INTF_PARM_AEC_LOCK:
            return POINTER_OF_META(CAM_INTF_PARM_AEC_LOCK, metadata);
        case CAM_INTF_PARM_FPS_RANGE:
            return POINTER_OF_META(CAM_INTF_PARM_FPS_RANGE, metadata);
        case CAM_INTF_PARM_AWB_LOCK:
            return POINTER_OF_META(CAM_INTF_PARM_AWB_LOCK, metadata);
        case CAM_INTF_PARM_BESTSHOT_MODE:
            return POINTER_OF_META(CAM_INTF_PARM_BESTSHOT_MODE, metadata);
        case CAM_INTF_PARM_DIS_ENABLE:
            return POINTER_OF_META(CAM_INTF_PARM_DIS_ENABLE, metadata);
        case CAM_INTF_PARM_LED_MODE:
            return POINTER_OF_META(CAM_INTF_PARM_LED_MODE, metadata);
        case CAM_INTF_PARM_QUERY_FLASH4SNAP:
            return POINTER_OF_META(CAM_INTF_PARM_QUERY_FLASH4SNAP, metadata);
        case CAM_INTF_PARM_EXPOSURE:
            return POINTER_OF_META(CAM_INTF_PARM_EXPOSURE, metadata);
        case CAM_INTF_PARM_SHARPNESS:
            return POINTER_OF_META(CAM_INTF_PARM_SHARPNESS, metadata);
        case CAM_INTF_PARM_CONTRAST:
            return POINTER_OF_META(CAM_INTF_PARM_CONTRAST, metadata);
        case CAM_INTF_PARM_SATURATION:
            return POINTER_OF_META(CAM_INTF_PARM_SATURATION, metadata);
        case CAM_INTF_PARM_BRIGHTNESS:
            return POINTER_OF_META(CAM_INTF_PARM_BRIGHTNESS, metadata);
        case CAM_INTF_PARM_ISO:
            return POINTER_OF_META(CAM_INTF_PARM_ISO, metadata);
        case CAM_INTF_PARM_ZOOM:
            return POINTER_OF_META(CAM_INTF_PARM_ZOOM, metadata);
        case CAM_INTF_PARM_ROLLOFF:
            return POINTER_OF_META(CAM_INTF_PARM_ROLLOFF, metadata);
        case CAM_INTF_PARM_MODE:
            return POINTER_OF_META(CAM_INTF_PARM_MODE, metadata);
        case CAM_INTF_PARM_AEC_ALGO_TYPE:
            return POINTER_OF_META(CAM_INTF_PARM_AEC_ALGO_TYPE, metadata);
        case CAM_INTF_PARM_FOCUS_ALGO_TYPE:
            return POINTER_OF_META(CAM_INTF_PARM_FOCUS_ALGO_TYPE, metadata);
        case CAM_INTF_PARM_AEC_ROI:
            return POINTER_OF_META(CAM_INTF_PARM_AEC_ROI, metadata);
        case CAM_INTF_PARM_AF_ROI:
            return POINTER_OF_META(CAM_INTF_PARM_AF_ROI, metadata);
        case CAM_INTF_PARM_SCE_FACTOR:
            return POINTER_OF_META(CAM_INTF_PARM_SCE_FACTOR, metadata);
        case CAM_INTF_PARM_FD:
            return POINTER_OF_META(CAM_INTF_PARM_FD, metadata);
        case CAM_INTF_PARM_MCE:
            return POINTER_OF_META(CAM_INTF_PARM_MCE, metadata);
        case CAM_INTF_PARM_HFR:
            return POINTER_OF_META(CAM_INTF_PARM_HFR, metadata);
        case CAM_INTF_PARM_REDEYE_REDUCTION:
            return POINTER_OF_META(CAM_INTF_PARM_REDEYE_REDUCTION, metadata);
        case CAM_INTF_PARM_WAVELET_DENOISE:
            return POINTER_OF_META(CAM_INTF_PARM_WAVELET_DENOISE, metadata);
        case CAM_INTF_PARM_HISTOGRAM:
            return POINTER_OF_META(CAM_INTF_PARM_HISTOGRAM, metadata);
        case CAM_INTF_PARM_ASD_ENABLE:
            return POINTER_OF_META(CAM_INTF_PARM_ASD_ENABLE, metadata);
        case CAM_INTF_PARM_RECORDING_HINT:
            return POINTER_OF_META(CAM_INTF_PARM_RECORDING_HINT, metadata);
        case CAM_INTF_PARM_HDR:
            return POINTER_OF_META(CAM_INTF_PARM_HDR, metadata);
        case CAM_INTF_PARM_FRAMESKIP:
            return POINTER_OF_META(CAM_INTF_PARM_FRAMESKIP, metadata);
        case CAM_INTF_PARM_ZSL_MODE:
            return POINTER_OF_META(CAM_INTF_PARM_ZSL_MODE, metadata);
        case CAM_INTF_PARM_HDR_NEED_1X:
            return POINTER_OF_META(CAM_INTF_PARM_HDR_NEED_1X, metadata);
        case CAM_INTF_PARM_LOCK_CAF:
            return POINTER_OF_META(CAM_INTF_PARM_LOCK_CAF, metadata);
        case CAM_INTF_PARM_VIDEO_HDR:
            return POINTER_OF_META(CAM_INTF_PARM_VIDEO_HDR, metadata);
        case CAM_INTF_PARM_VT:
            return POINTER_OF_META(CAM_INTF_PARM_VT, metadata);
        case CAM_INTF_PARM_GET_CHROMATIX:
            return POINTER_OF_META(CAM_INTF_PARM_GET_CHROMATIX, metadata);
        case CAM_INTF_PARM_SET_RELOAD_CHROMATIX:
            return POINTER_OF_META(CAM_INTF_PARM_SET_RELOAD_CHROMATIX, metadata);
        case CAM_INTF_PARM_GET_AFTUNE:
            return POINTER_OF_META(CAM_INTF_PARM_GET_AFTUNE, metadata);
        case CAM_INTF_PARM_SET_RELOAD_AFTUNE:
            return POINTER_OF_META(CAM_INTF_PARM_SET_RELOAD_AFTUNE, metadata);
        case CAM_INTF_PARM_SET_AUTOFOCUSTUNING:
            return POINTER_OF_META(CAM_INTF_PARM_SET_AUTOFOCUSTUNING, metadata);
        case CAM_INTF_PARM_SET_VFE_COMMAND:
            return POINTER_OF_META(CAM_INTF_PARM_SET_VFE_COMMAND, metadata);
        case CAM_INTF_PARM_SET_PP_COMMAND:
            return POINTER_OF_META(CAM_INTF_PARM_SET_PP_COMMAND, metadata);
        case CAM_INTF_PARM_MAX_DIMENSION:
            return POINTER_OF_META(CAM_INTF_PARM_MAX_DIMENSION, metadata);
        case CAM_INTF_PARM_RAW_DIMENSION:
            return POINTER_OF_META(CAM_INTF_PARM_RAW_DIMENSION, metadata);
        case CAM_INTF_PARM_TINTLESS:
            return POINTER_OF_META(CAM_INTF_PARM_TINTLESS, metadata);
        case CAM_INTF_PARM_EZTUNE_CMD:
            return POINTER_OF_META(CAM_INTF_PARM_EZTUNE_CMD, metadata);
        case CAM_INTF_PARM_RDI_MODE:
            return POINTER_OF_META(CAM_INTF_PARM_RDI_MODE, metadata);
        case CAM_INTF_PARM_BURST_NUM:
            return POINTER_OF_META(CAM_INTF_PARM_BURST_NUM, metadata);
        case CAM_INTF_PARM_RETRO_BURST_NUM:
            return POINTER_OF_META(CAM_INTF_PARM_RETRO_BURST_NUM, metadata);
        case CAM_INTF_PARM_BURST_LED_ON_PERIOD:
            return POINTER_OF_META(CAM_INTF_PARM_BURST_LED_ON_PERIOD, metadata);
        case CAM_INTF_META_STREAM_INFO:
            return POINTER_OF_META(CAM_INTF_META_STREAM_INFO, metadata);
        case CAM_INTF_META_AEC_MODE:
            return POINTER_OF_META(CAM_INTF_META_AEC_MODE, metadata);
        case CAM_INTF_META_AEC_PRECAPTURE_TRIGGER:
            return POINTER_OF_META(CAM_INTF_META_AEC_PRECAPTURE_TRIGGER, metadata);
        case CAM_INTF_META_AF_TRIGGER:
            return POINTER_OF_META(CAM_INTF_META_AF_TRIGGER, metadata);
        case CAM_INTF_META_CAPTURE_INTENT:
            return POINTER_OF_META(CAM_INTF_META_CAPTURE_INTENT, metadata);
        case CAM_INTF_META_DEMOSAIC:
            return POINTER_OF_META(CAM_INTF_META_DEMOSAIC, metadata);
        case CAM_INTF_META_SHARPNESS_STRENGTH:
            return POINTER_OF_META(CAM_INTF_META_SHARPNESS_STRENGTH, metadata);
        case CAM_INTF_META_GEOMETRIC_MODE:
            return POINTER_OF_META(CAM_INTF_META_GEOMETRIC_MODE, metadata);
        case CAM_INTF_META_GEOMETRIC_STRENGTH:
            return POINTER_OF_META(CAM_INTF_META_GEOMETRIC_STRENGTH, metadata);
        case CAM_INTF_META_LENS_SHADING_MAP_MODE:
            return POINTER_OF_META(CAM_INTF_META_LENS_SHADING_MAP_MODE, metadata);
        case CAM_INTF_META_SHADING_STRENGTH:
            return POINTER_OF_META(CAM_INTF_META_SHADING_STRENGTH, metadata);
        case CAM_INTF_META_TONEMAP_MODE:
            return POINTER_OF_META(CAM_INTF_META_TONEMAP_MODE, metadata);
        case CAM_INTF_META_STREAM_ID:
            return POINTER_OF_META(CAM_INTF_META_STREAM_ID, metadata);
        case CAM_INTF_PARM_STATS_DEBUG_MASK:
            return POINTER_OF_META(CAM_INTF_PARM_STATS_DEBUG_MASK, metadata);
        case CAM_INTF_PARM_STATS_AF_PAAF:
            return POINTER_OF_META(CAM_INTF_PARM_STATS_AF_PAAF, metadata);
        case CAM_INTF_PARM_FOCUS_BRACKETING:
            return POINTER_OF_META(CAM_INTF_PARM_FOCUS_BRACKETING, metadata);
        case CAM_INTF_PARM_FLASH_BRACKETING:
            return POINTER_OF_META(CAM_INTF_PARM_FLASH_BRACKETING, metadata);
        case CAM_INTF_META_JPEG_GPS_COORDINATES:
            return POINTER_OF_META(CAM_INTF_META_JPEG_GPS_COORDINATES, metadata);
        case CAM_INTF_META_JPEG_GPS_PROC_METHODS:
            return POINTER_OF_META(CAM_INTF_META_JPEG_GPS_PROC_METHODS, metadata);
        case CAM_INTF_META_JPEG_GPS_TIMESTAMP:
            return POINTER_OF_META(CAM_INTF_META_JPEG_GPS_TIMESTAMP, metadata);
        case CAM_INTF_META_JPEG_ORIENTATION:
            return POINTER_OF_META(CAM_INTF_META_JPEG_ORIENTATION, metadata);
        case CAM_INTF_META_JPEG_QUALITY:
            return POINTER_OF_META(CAM_INTF_META_JPEG_QUALITY, metadata);
        case CAM_INTF_META_JPEG_THUMB_QUALITY:
            return POINTER_OF_META(CAM_INTF_META_JPEG_THUMB_QUALITY, metadata);
        case CAM_INTF_META_JPEG_THUMB_SIZE:
            return POINTER_OF_META(CAM_INTF_META_JPEG_THUMB_SIZE, metadata);
        case CAM_INTF_META_TEST_PATTERN_DATA:
            return POINTER_OF_META(CAM_INTF_META_TEST_PATTERN_DATA, metadata);
        case CAM_INTF_META_PROFILE_TONE_CURVE:
            return POINTER_OF_META(CAM_INTF_META_PROFILE_TONE_CURVE, metadata);
        case CAM_INTF_META_OTP_WB_GRGB:
            return POINTER_OF_META(CAM_INTF_META_OTP_WB_GRGB, metadata);
        case CAM_INTF_PARM_CAC:
            return POINTER_OF_META(CAM_INTF_PARM_CAC, metadata);
        case CAM_INTF_META_NEUTRAL_COL_POINT:
            return POINTER_OF_META(CAM_INTF_META_NEUTRAL_COL_POINT, metadata);
        case CAM_INTF_PARM_CDS_MODE:
            return POINTER_OF_META(CAM_INTF_PARM_CDS_MODE, metadata);
        case CAM_INTF_PARM_ROTATION:
          return POINTER_OF_META(CAM_INTF_PARM_ROTATION, metadata);
        default:
            return NULL;
    }
}

uint32_t get_size_of(cam_intf_parm_type_t param_id)
{
    metadata_buffer_t* metadata = NULL;
    switch(param_id) {
        case CAM_INTF_META_HISTOGRAM:
            return SIZE_OF_PARAM(CAM_INTF_META_HISTOGRAM, metadata);
        case CAM_INTF_META_FACE_DETECTION:
            return SIZE_OF_PARAM(CAM_INTF_META_FACE_DETECTION, metadata);
        case CAM_INTF_META_AUTOFOCUS_DATA:
            return SIZE_OF_PARAM(CAM_INTF_META_AUTOFOCUS_DATA, metadata);
        case CAM_INTF_META_CROP_DATA:
            return SIZE_OF_PARAM(CAM_INTF_META_CROP_DATA, metadata);
        case CAM_INTF_META_PREP_SNAPSHOT_DONE:
            return SIZE_OF_PARAM(CAM_INTF_META_PREP_SNAPSHOT_DONE, metadata);
        case CAM_INTF_META_GOOD_FRAME_IDX_RANGE:
            return SIZE_OF_PARAM(CAM_INTF_META_GOOD_FRAME_IDX_RANGE, metadata);
        case CAM_INTF_META_ASD_HDR_SCENE_DATA:
            return SIZE_OF_PARAM(CAM_INTF_META_ASD_HDR_SCENE_DATA, metadata);
        case CAM_INTF_META_ASD_SCENE_TYPE:
            return SIZE_OF_PARAM(CAM_INTF_META_ASD_SCENE_TYPE, metadata);
        case CAM_INTF_META_CHROMATIX_LITE_ISP:
            return SIZE_OF_PARAM(CAM_INTF_META_CHROMATIX_LITE_ISP, metadata);
        case CAM_INTF_META_CHROMATIX_LITE_PP:
            return SIZE_OF_PARAM(CAM_INTF_META_CHROMATIX_LITE_PP, metadata);
        case CAM_INTF_META_CHROMATIX_LITE_AE:
            return SIZE_OF_PARAM(CAM_INTF_META_CHROMATIX_LITE_AE, metadata);
        case CAM_INTF_META_CHROMATIX_LITE_AWB:
            return SIZE_OF_PARAM(CAM_INTF_META_CHROMATIX_LITE_AWB, metadata);
        case CAM_INTF_META_CHROMATIX_LITE_AF:
            return SIZE_OF_PARAM(CAM_INTF_META_CHROMATIX_LITE_AF, metadata);
        case CAM_INTF_META_CHROMATIX_LITE_ASD:
            return SIZE_OF_PARAM(CAM_INTF_META_CHROMATIX_LITE_ASD, metadata);
        case CAM_INTF_META_FRAME_NUMBER_VALID:
            return SIZE_OF_PARAM(CAM_INTF_META_FRAME_NUMBER_VALID, metadata);
        case CAM_INTF_META_URGENT_FRAME_NUMBER_VALID:
            return SIZE_OF_PARAM(CAM_INTF_META_URGENT_FRAME_NUMBER_VALID, metadata);
        case CAM_INTF_META_FRAME_DROPPED:
            return SIZE_OF_PARAM(CAM_INTF_META_FRAME_DROPPED, metadata);
        case CAM_INTF_META_FRAME_NUMBER:
            return SIZE_OF_PARAM(CAM_INTF_META_FRAME_NUMBER, metadata);
        case CAM_INTF_META_URGENT_FRAME_NUMBER:
            return SIZE_OF_PARAM(CAM_INTF_META_URGENT_FRAME_NUMBER, metadata);
        case CAM_INTF_META_COLOR_CORRECT_MODE:
            return SIZE_OF_PARAM(CAM_INTF_META_COLOR_CORRECT_MODE, metadata);
        case CAM_INTF_META_COLOR_CORRECT_TRANSFORM:
            return SIZE_OF_PARAM(CAM_INTF_META_COLOR_CORRECT_TRANSFORM, metadata);
        case CAM_INTF_META_COLOR_CORRECT_GAINS:
            return SIZE_OF_PARAM(CAM_INTF_META_COLOR_CORRECT_GAINS, metadata);
        case CAM_INTF_META_PRED_COLOR_CORRECT_TRANSFORM:
            return SIZE_OF_PARAM(CAM_INTF_META_PRED_COLOR_CORRECT_TRANSFORM, metadata);
        case CAM_INTF_META_PRED_COLOR_CORRECT_GAINS:
            return SIZE_OF_PARAM(CAM_INTF_META_PRED_COLOR_CORRECT_GAINS, metadata);
        case CAM_INTF_META_AEC_ROI:
            return SIZE_OF_PARAM(CAM_INTF_META_AEC_ROI, metadata);
        case CAM_INTF_META_AEC_STATE:
            return SIZE_OF_PARAM(CAM_INTF_META_AEC_STATE, metadata);
        case CAM_INTF_PARM_FOCUS_MODE:
            return SIZE_OF_PARAM(CAM_INTF_PARM_FOCUS_MODE, metadata);
        case CAM_INTF_META_AF_ROI:
            return SIZE_OF_PARAM(CAM_INTF_META_AF_ROI, metadata);
        case CAM_INTF_META_AF_STATE:
            return SIZE_OF_PARAM(CAM_INTF_META_AF_STATE, metadata);
        case CAM_INTF_PARM_WHITE_BALANCE:
            return SIZE_OF_PARAM(CAM_INTF_PARM_WHITE_BALANCE, metadata);
        case CAM_INTF_META_AWB_REGIONS:
            return SIZE_OF_PARAM(CAM_INTF_META_AWB_REGIONS, metadata);
        case CAM_INTF_META_AWB_STATE:
            return SIZE_OF_PARAM(CAM_INTF_META_AWB_STATE, metadata);
        case CAM_INTF_META_BLACK_LEVEL_LOCK:
            return SIZE_OF_PARAM(CAM_INTF_META_BLACK_LEVEL_LOCK, metadata);
        case CAM_INTF_META_MODE:
            return SIZE_OF_PARAM(CAM_INTF_META_MODE, metadata);
        case CAM_INTF_META_EDGE_MODE:
            return SIZE_OF_PARAM(CAM_INTF_META_EDGE_MODE, metadata);
        case CAM_INTF_META_FLASH_POWER:
            return SIZE_OF_PARAM(CAM_INTF_META_FLASH_POWER, metadata);
        case CAM_INTF_META_FLASH_FIRING_TIME:
            return SIZE_OF_PARAM(CAM_INTF_META_FLASH_FIRING_TIME, metadata);
        case CAM_INTF_META_FLASH_MODE:
            return SIZE_OF_PARAM(CAM_INTF_META_FLASH_MODE, metadata);
        case CAM_INTF_META_FLASH_STATE:
            return SIZE_OF_PARAM(CAM_INTF_META_FLASH_STATE, metadata);
        case CAM_INTF_META_HOTPIXEL_MODE:
            return SIZE_OF_PARAM(CAM_INTF_META_HOTPIXEL_MODE, metadata);
        case CAM_INTF_META_LENS_APERTURE:
            return SIZE_OF_PARAM(CAM_INTF_META_LENS_APERTURE, metadata);
        case CAM_INTF_META_LENS_FILTERDENSITY:
            return SIZE_OF_PARAM(CAM_INTF_META_LENS_FILTERDENSITY, metadata);
        case CAM_INTF_META_LENS_FOCAL_LENGTH:
            return SIZE_OF_PARAM(CAM_INTF_META_LENS_FOCAL_LENGTH, metadata);
        case CAM_INTF_META_LENS_FOCUS_DISTANCE:
            return SIZE_OF_PARAM(CAM_INTF_META_LENS_FOCUS_DISTANCE, metadata);
        case CAM_INTF_META_LENS_FOCUS_RANGE:
            return SIZE_OF_PARAM(CAM_INTF_META_LENS_FOCUS_RANGE, metadata);
        case CAM_INTF_META_LENS_STATE:
            return SIZE_OF_PARAM(CAM_INTF_META_LENS_STATE, metadata);
        case CAM_INTF_META_LENS_OPT_STAB_MODE:
            return SIZE_OF_PARAM(CAM_INTF_META_LENS_OPT_STAB_MODE, metadata);
        case CAM_INTF_META_NOISE_REDUCTION_MODE:
            return SIZE_OF_PARAM(CAM_INTF_META_NOISE_REDUCTION_MODE, metadata);
        case CAM_INTF_META_NOISE_REDUCTION_STRENGTH:
            return SIZE_OF_PARAM(CAM_INTF_META_NOISE_REDUCTION_STRENGTH, metadata);
        case CAM_INTF_META_SCALER_CROP_REGION:
            return SIZE_OF_PARAM(CAM_INTF_META_SCALER_CROP_REGION, metadata);
        case CAM_INTF_META_SCENE_FLICKER:
            return SIZE_OF_PARAM(CAM_INTF_META_SCENE_FLICKER, metadata);
        case CAM_INTF_META_SENSOR_EXPOSURE_TIME:
            return SIZE_OF_PARAM(CAM_INTF_META_SENSOR_EXPOSURE_TIME, metadata);
        case CAM_INTF_META_SENSOR_FRAME_DURATION:
            return SIZE_OF_PARAM(CAM_INTF_META_SENSOR_FRAME_DURATION, metadata);
        case CAM_INTF_META_SENSOR_SENSITIVITY:
            return SIZE_OF_PARAM(CAM_INTF_META_SENSOR_SENSITIVITY, metadata);
        case CAM_INTF_META_SENSOR_TIMESTAMP:
            return SIZE_OF_PARAM(CAM_INTF_META_SENSOR_TIMESTAMP, metadata);
        case CAM_INTF_META_SHADING_MODE:
            return SIZE_OF_PARAM(CAM_INTF_META_SHADING_MODE, metadata);
        case CAM_INTF_META_STATS_FACEDETECT_MODE:
            return SIZE_OF_PARAM(CAM_INTF_META_STATS_FACEDETECT_MODE, metadata);
        case CAM_INTF_META_STATS_HISTOGRAM_MODE:
            return SIZE_OF_PARAM(CAM_INTF_META_STATS_HISTOGRAM_MODE, metadata);
        case CAM_INTF_META_STATS_SHARPNESS_MAP_MODE:
            return SIZE_OF_PARAM(CAM_INTF_META_STATS_SHARPNESS_MAP_MODE, metadata);
        case CAM_INTF_META_STATS_SHARPNESS_MAP:
            return SIZE_OF_PARAM(CAM_INTF_META_STATS_SHARPNESS_MAP, metadata);
        case CAM_INTF_META_TONEMAP_CURVES:
            return SIZE_OF_PARAM(CAM_INTF_META_TONEMAP_CURVES, metadata);
        case CAM_INTF_META_LENS_SHADING_MAP:
            return SIZE_OF_PARAM(CAM_INTF_META_LENS_SHADING_MAP, metadata);
        case CAM_INTF_META_AEC_INFO:
            return SIZE_OF_PARAM(CAM_INTF_META_AEC_INFO, metadata);
        case CAM_INTF_META_SENSOR_INFO:
            return SIZE_OF_PARAM(CAM_INTF_META_SENSOR_INFO, metadata);
        case CAM_INTF_META_ASD_SCENE_CAPTURE_TYPE:
            return SIZE_OF_PARAM(CAM_INTF_META_ASD_SCENE_CAPTURE_TYPE, metadata);
        case CAM_INTF_PARM_EFFECT:
            return SIZE_OF_PARAM(CAM_INTF_PARM_EFFECT, metadata);
        case CAM_INTF_META_PRIVATE_DATA:
            return SIZE_OF_PARAM(CAM_INTF_META_PRIVATE_DATA, metadata);
        case CAM_INTF_PARM_HAL_VERSION:
            return SIZE_OF_PARAM(CAM_INTF_PARM_HAL_VERSION, metadata);
        case CAM_INTF_PARM_ANTIBANDING:
            return SIZE_OF_PARAM(CAM_INTF_PARM_ANTIBANDING, metadata);
        case CAM_INTF_PARM_EXPOSURE_COMPENSATION:
            return SIZE_OF_PARAM(CAM_INTF_PARM_EXPOSURE_COMPENSATION, metadata);
        case CAM_INTF_PARM_EV_STEP:
            return SIZE_OF_PARAM(CAM_INTF_PARM_EV_STEP, metadata);
        case CAM_INTF_PARM_AEC_LOCK:
            return SIZE_OF_PARAM(CAM_INTF_PARM_AEC_LOCK, metadata);
        case CAM_INTF_PARM_FPS_RANGE:
            return SIZE_OF_PARAM(CAM_INTF_PARM_FPS_RANGE, metadata);
        case CAM_INTF_PARM_AWB_LOCK:
            return SIZE_OF_PARAM(CAM_INTF_PARM_AWB_LOCK, metadata);
        case CAM_INTF_PARM_BESTSHOT_MODE:
            return SIZE_OF_PARAM(CAM_INTF_PARM_BESTSHOT_MODE, metadata);
        case CAM_INTF_PARM_DIS_ENABLE:
            return SIZE_OF_PARAM(CAM_INTF_PARM_DIS_ENABLE, metadata);
        case CAM_INTF_PARM_LED_MODE:
            return SIZE_OF_PARAM(CAM_INTF_PARM_LED_MODE, metadata);
        case CAM_INTF_PARM_QUERY_FLASH4SNAP:
            return SIZE_OF_PARAM(CAM_INTF_PARM_QUERY_FLASH4SNAP, metadata);
        case CAM_INTF_PARM_EXPOSURE:
            return SIZE_OF_PARAM(CAM_INTF_PARM_EXPOSURE, metadata);
        case CAM_INTF_PARM_SHARPNESS:
            return SIZE_OF_PARAM(CAM_INTF_PARM_SHARPNESS, metadata);
        case CAM_INTF_PARM_CONTRAST:
            return SIZE_OF_PARAM(CAM_INTF_PARM_CONTRAST, metadata);
        case CAM_INTF_PARM_SATURATION:
            return SIZE_OF_PARAM(CAM_INTF_PARM_SATURATION, metadata);
        case CAM_INTF_PARM_BRIGHTNESS:
            return SIZE_OF_PARAM(CAM_INTF_PARM_BRIGHTNESS, metadata);
        case CAM_INTF_PARM_ISO:
            return SIZE_OF_PARAM(CAM_INTF_PARM_ISO, metadata);
        case CAM_INTF_PARM_ZOOM:
            return SIZE_OF_PARAM(CAM_INTF_PARM_ZOOM, metadata);
        case CAM_INTF_PARM_ROLLOFF:
            return SIZE_OF_PARAM(CAM_INTF_PARM_ROLLOFF, metadata);
        case CAM_INTF_PARM_MODE:
            return SIZE_OF_PARAM(CAM_INTF_PARM_MODE, metadata);
        case CAM_INTF_PARM_AEC_ALGO_TYPE:
            return SIZE_OF_PARAM(CAM_INTF_PARM_AEC_ALGO_TYPE, metadata);
        case CAM_INTF_PARM_FOCUS_ALGO_TYPE:
            return SIZE_OF_PARAM(CAM_INTF_PARM_FOCUS_ALGO_TYPE, metadata);
        case CAM_INTF_PARM_AEC_ROI:
            return SIZE_OF_PARAM(CAM_INTF_PARM_AEC_ROI, metadata);
        case CAM_INTF_PARM_AF_ROI:
            return SIZE_OF_PARAM(CAM_INTF_PARM_AF_ROI, metadata);
        case CAM_INTF_PARM_SCE_FACTOR:
            return SIZE_OF_PARAM(CAM_INTF_PARM_SCE_FACTOR, metadata);
        case CAM_INTF_PARM_FD:
            return SIZE_OF_PARAM(CAM_INTF_PARM_FD, metadata);
        case CAM_INTF_PARM_MCE:
            return SIZE_OF_PARAM(CAM_INTF_PARM_MCE, metadata);
        case CAM_INTF_PARM_HFR:
            return SIZE_OF_PARAM(CAM_INTF_PARM_HFR, metadata);
        case CAM_INTF_PARM_REDEYE_REDUCTION:
            return SIZE_OF_PARAM(CAM_INTF_PARM_REDEYE_REDUCTION, metadata);
        case CAM_INTF_PARM_WAVELET_DENOISE:
            return SIZE_OF_PARAM(CAM_INTF_PARM_WAVELET_DENOISE, metadata);
        case CAM_INTF_PARM_HISTOGRAM:
            return SIZE_OF_PARAM(CAM_INTF_PARM_HISTOGRAM, metadata);
        case CAM_INTF_PARM_ASD_ENABLE:
            return SIZE_OF_PARAM(CAM_INTF_PARM_ASD_ENABLE, metadata);
        case CAM_INTF_PARM_RECORDING_HINT:
            return SIZE_OF_PARAM(CAM_INTF_PARM_RECORDING_HINT, metadata);
        case CAM_INTF_PARM_HDR:
            return SIZE_OF_PARAM(CAM_INTF_PARM_HDR, metadata);
        case CAM_INTF_PARM_FRAMESKIP:
            return SIZE_OF_PARAM(CAM_INTF_PARM_FRAMESKIP, metadata);
        case CAM_INTF_PARM_ZSL_MODE:
            return SIZE_OF_PARAM(CAM_INTF_PARM_ZSL_MODE, metadata);
        case CAM_INTF_PARM_HDR_NEED_1X:
            return SIZE_OF_PARAM(CAM_INTF_PARM_HDR_NEED_1X, metadata);
        case CAM_INTF_PARM_LOCK_CAF:
            return SIZE_OF_PARAM(CAM_INTF_PARM_LOCK_CAF, metadata);
        case CAM_INTF_PARM_VIDEO_HDR:
            return SIZE_OF_PARAM(CAM_INTF_PARM_VIDEO_HDR, metadata);
        case CAM_INTF_PARM_VT:
            return SIZE_OF_PARAM(CAM_INTF_PARM_VT, metadata);
        case CAM_INTF_PARM_GET_CHROMATIX:
            return SIZE_OF_PARAM(CAM_INTF_PARM_GET_CHROMATIX, metadata);
        case CAM_INTF_PARM_SET_RELOAD_CHROMATIX:
            return SIZE_OF_PARAM(CAM_INTF_PARM_SET_RELOAD_CHROMATIX, metadata);
        case CAM_INTF_PARM_GET_AFTUNE:
            return SIZE_OF_PARAM(CAM_INTF_PARM_GET_AFTUNE, metadata);
        case CAM_INTF_PARM_SET_RELOAD_AFTUNE:
            return SIZE_OF_PARAM(CAM_INTF_PARM_SET_RELOAD_AFTUNE, metadata);
        case CAM_INTF_PARM_SET_AUTOFOCUSTUNING:
            return SIZE_OF_PARAM(CAM_INTF_PARM_SET_AUTOFOCUSTUNING, metadata);
        case CAM_INTF_PARM_SET_VFE_COMMAND:
            return SIZE_OF_PARAM(CAM_INTF_PARM_SET_VFE_COMMAND, metadata);
        case CAM_INTF_PARM_SET_PP_COMMAND:
            return SIZE_OF_PARAM(CAM_INTF_PARM_SET_PP_COMMAND, metadata);
        case CAM_INTF_PARM_MAX_DIMENSION:
            return SIZE_OF_PARAM(CAM_INTF_PARM_MAX_DIMENSION, metadata);
        case CAM_INTF_PARM_RAW_DIMENSION:
            return SIZE_OF_PARAM(CAM_INTF_PARM_RAW_DIMENSION, metadata);
        case CAM_INTF_PARM_TINTLESS:
            return SIZE_OF_PARAM(CAM_INTF_PARM_TINTLESS, metadata);
        case CAM_INTF_PARM_EZTUNE_CMD:
            return SIZE_OF_PARAM(CAM_INTF_PARM_EZTUNE_CMD, metadata);
        case CAM_INTF_PARM_RDI_MODE:
            return SIZE_OF_PARAM(CAM_INTF_PARM_RDI_MODE, metadata);
        case CAM_INTF_PARM_BURST_NUM:
            return SIZE_OF_PARAM(CAM_INTF_PARM_BURST_NUM, metadata);
        case CAM_INTF_PARM_RETRO_BURST_NUM:
            return SIZE_OF_PARAM(CAM_INTF_PARM_RETRO_BURST_NUM, metadata);
        case CAM_INTF_PARM_BURST_LED_ON_PERIOD:
            return SIZE_OF_PARAM(CAM_INTF_PARM_BURST_LED_ON_PERIOD, metadata);
        case CAM_INTF_META_STREAM_INFO:
            return SIZE_OF_PARAM(CAM_INTF_META_STREAM_INFO, metadata);
        case CAM_INTF_META_AEC_MODE:
            return SIZE_OF_PARAM(CAM_INTF_META_AEC_MODE, metadata);
        case CAM_INTF_META_AEC_PRECAPTURE_TRIGGER:
            return SIZE_OF_PARAM(CAM_INTF_META_AEC_PRECAPTURE_TRIGGER, metadata);
        case CAM_INTF_META_AF_TRIGGER:
            return SIZE_OF_PARAM(CAM_INTF_META_AF_TRIGGER, metadata);
        case CAM_INTF_META_CAPTURE_INTENT:
            return SIZE_OF_PARAM(CAM_INTF_META_CAPTURE_INTENT, metadata);
        case CAM_INTF_META_DEMOSAIC:
            return SIZE_OF_PARAM(CAM_INTF_META_DEMOSAIC, metadata);
        case CAM_INTF_META_SHARPNESS_STRENGTH:
            return SIZE_OF_PARAM(CAM_INTF_META_SHARPNESS_STRENGTH, metadata);
        case CAM_INTF_META_GEOMETRIC_MODE:
            return SIZE_OF_PARAM(CAM_INTF_META_GEOMETRIC_MODE, metadata);
        case CAM_INTF_META_GEOMETRIC_STRENGTH:
            return SIZE_OF_PARAM(CAM_INTF_META_GEOMETRIC_STRENGTH, metadata);
        case CAM_INTF_META_LENS_SHADING_MAP_MODE:
            return SIZE_OF_PARAM(CAM_INTF_META_LENS_SHADING_MAP_MODE, metadata);
        case CAM_INTF_META_SHADING_STRENGTH:
            return SIZE_OF_PARAM(CAM_INTF_META_SHADING_STRENGTH, metadata);
        case CAM_INTF_META_TONEMAP_MODE:
            return SIZE_OF_PARAM(CAM_INTF_META_TONEMAP_MODE, metadata);
        case CAM_INTF_META_STREAM_ID:
            return SIZE_OF_PARAM(CAM_INTF_META_STREAM_ID, metadata);
        case CAM_INTF_PARM_STATS_DEBUG_MASK:
            return SIZE_OF_PARAM(CAM_INTF_PARM_STATS_DEBUG_MASK, metadata);
        case CAM_INTF_PARM_STATS_AF_PAAF:
            return SIZE_OF_PARAM(CAM_INTF_PARM_STATS_AF_PAAF, metadata);
        case CAM_INTF_PARM_FOCUS_BRACKETING:
            return SIZE_OF_PARAM(CAM_INTF_PARM_FOCUS_BRACKETING, metadata);
        case CAM_INTF_PARM_FLASH_BRACKETING:
            return SIZE_OF_PARAM(CAM_INTF_PARM_FLASH_BRACKETING, metadata);
        case CAM_INTF_META_JPEG_GPS_COORDINATES:
            return SIZE_OF_PARAM(CAM_INTF_META_JPEG_GPS_COORDINATES, metadata);
        case CAM_INTF_META_JPEG_GPS_PROC_METHODS:
            return SIZE_OF_PARAM(CAM_INTF_META_JPEG_GPS_PROC_METHODS, metadata);
        case CAM_INTF_META_JPEG_GPS_TIMESTAMP:
            return SIZE_OF_PARAM(CAM_INTF_META_JPEG_GPS_TIMESTAMP, metadata);
        case CAM_INTF_META_JPEG_ORIENTATION:
            return SIZE_OF_PARAM(CAM_INTF_META_JPEG_ORIENTATION, metadata);
        case CAM_INTF_META_JPEG_QUALITY:
            return SIZE_OF_PARAM(CAM_INTF_META_JPEG_QUALITY, metadata);
        case CAM_INTF_META_JPEG_THUMB_QUALITY:
            return SIZE_OF_PARAM(CAM_INTF_META_JPEG_THUMB_QUALITY, metadata);
        case CAM_INTF_META_JPEG_THUMB_SIZE:
            return SIZE_OF_PARAM(CAM_INTF_META_JPEG_THUMB_SIZE, metadata);
        case CAM_INTF_META_TEST_PATTERN_DATA:
            return SIZE_OF_PARAM(CAM_INTF_META_TEST_PATTERN_DATA, metadata);
        case CAM_INTF_META_PROFILE_TONE_CURVE:
            return SIZE_OF_PARAM(CAM_INTF_META_PROFILE_TONE_CURVE, metadata);
        case CAM_INTF_META_OTP_WB_GRGB:
            return SIZE_OF_PARAM(CAM_INTF_META_OTP_WB_GRGB, metadata);
        case CAM_INTF_PARM_CAC:
            return SIZE_OF_PARAM(CAM_INTF_PARM_CAC, metadata);
        case CAM_INTF_META_NEUTRAL_COL_POINT:
            return SIZE_OF_PARAM(CAM_INTF_META_NEUTRAL_COL_POINT, metadata);
        case CAM_INTF_PARM_CDS_MODE:
            return SIZE_OF_PARAM(CAM_INTF_PARM_CDS_MODE, metadata);
        case CAM_INTF_PARM_ROTATION:
          return SIZE_OF_PARAM(CAM_INTF_PARM_ROTATION, metadata);
        default:
            return 0;
    }
    return 0;
}
