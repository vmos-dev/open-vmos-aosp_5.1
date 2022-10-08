/*
**
** Copyright 2008, The Android Open Source Project
** Copyright 2012, Samsung Electronics Co. LTD
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

/*!
 * \file      MetadataConverter.cpp
 * \brief     source file for Metadata converter ( for camera hal2 implementation )
 * \author    Sungjoong Kang(sj3.kang@samsung.com)
 * \date      2012/05/31
 *
 * <b>Revision History: </b>
 * - 2012/05/31 : Sungjoong Kang(sj3.kang@samsung.com) \n
 *   Initial Release
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "MetadataConverter"
#include <utils/Log.h>

#include "MetadataConverter.h"
#include "exynos_format.h"

namespace android {


MetadataConverter::MetadataConverter()
{
    return;
}


MetadataConverter::~MetadataConverter()
{
    ALOGV("DEBUG(%s)destroy!!:", __FUNCTION__);
    return;
}

status_t MetadataConverter::CheckEntryTypeMismatch(camera_metadata_entry_t * entry,
    uint8_t type)
{
    if (!(entry->type==type))
    {
        ALOGV("DEBUG(%s):Metadata Missmatch tag(%s) type (%d) count(%d)",
            __FUNCTION__, get_camera_metadata_tag_name(entry->tag), entry->type, entry->count);
        return BAD_VALUE;
    }
    return NO_ERROR;
}

status_t MetadataConverter::CheckEntryTypeMismatch(camera_metadata_entry_t * entry,
    uint8_t type, size_t count)
{
    if (!((entry->type==type)&&(entry->count==count)))
    {
        ALOGV("DEBUG(%s):Metadata Missmatch tag(%s) type (%d) count(%d)",
            __FUNCTION__, get_camera_metadata_tag_name(entry->tag), entry->type, entry->count);
        return BAD_VALUE;
    }
    return NO_ERROR;
}

status_t MetadataConverter::ToInternalShot(camera_metadata_t * request, struct camera2_shot_ext * dst_ext)
{
    uint32_t    num_entry = 0;
    uint32_t    index = 0;
    uint32_t    i = 0;
    uint32_t    cnt = 0;
    camera_metadata_entry_t curr_entry;
    struct camera2_shot * dst = NULL;

    if (request == NULL || dst_ext == NULL)
        return BAD_VALUE;

    memset((void*)dst_ext, 0, sizeof(struct camera2_shot_ext));
    dst = &dst_ext->shot;

    dst->magicNumber = 0x23456789;
    dst->ctl.aa.aeTargetFpsRange[0] = 15;
    dst->ctl.aa.aeTargetFpsRange[1] = 30;
    dst->ctl.aa.aeExpCompensation = 5;

    num_entry = (uint32_t)get_camera_metadata_entry_count(request);
    for (index = 0 ; index < num_entry ; index++) {

        if (get_camera_metadata_entry(request, index, &curr_entry)==0) {
            switch (curr_entry.tag) {

            case ANDROID_LENS_FOCUS_DISTANCE:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_FLOAT, 1))
                    break;
                dst->ctl.lens.focusDistance = curr_entry.data.f[0];
                break;

            case ANDROID_LENS_APERTURE:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_FLOAT, 1))
                    break;
                dst->ctl.lens.aperture = curr_entry.data.f[0];
                break;

            case ANDROID_LENS_FOCAL_LENGTH:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_FLOAT, 1))
                    break;
                dst->ctl.lens.focalLength = curr_entry.data.f[0];
                break;

            case ANDROID_LENS_FILTER_DENSITY:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_FLOAT, 1))
                    break;
                dst->ctl.lens.filterDensity = curr_entry.data.f[0];
                break;

            case ANDROID_LENS_OPTICAL_STABILIZATION_MODE:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_BYTE, 1))
                    break;
                dst->ctl.lens.opticalStabilizationMode =
                    (enum optical_stabilization_mode)curr_entry.data.u8[0];
                break;


            case ANDROID_SENSOR_TIMESTAMP:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_INT64, 1))
                    break;
                dst->dm.sensor.timeStamp = curr_entry.data.i64[0];
                break;


            case ANDROID_SENSOR_SENSITIVITY:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_INT32, 1))
                    break;
                dst->dm.aa.isoValue = curr_entry.data.i32[0];
                break;

            case ANDROID_SENSOR_EXPOSURE_TIME:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_INT64, 1))
                    break;
                dst->dm.sensor.exposureTime = curr_entry.data.i64[0];
                break;


            case ANDROID_FLASH_MODE:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_BYTE, 1))
                    break;
                dst->ctl.flash.flashMode = (enum flash_mode)(curr_entry.data.u8[0] + 1);
                break;

            case ANDROID_FLASH_FIRING_POWER:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_BYTE, 1))
                    break;
                dst->ctl.flash.firingPower = curr_entry.data.u8[0];
                break;

            case ANDROID_FLASH_FIRING_TIME:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_INT64, 1))
                    break;
                dst->ctl.flash.firingTime = curr_entry.data.i64[0];
                break;



            case ANDROID_SCALER_CROP_REGION:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_INT32, 4))
                    break;
                for (i=0 ; i<3; i++)
                    dst->ctl.scaler.cropRegion[i] = ALIGN(curr_entry.data.i32[i], 2);
                break;


            case ANDROID_JPEG_QUALITY:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_INT32, 1))
                    break;
                dst->ctl.jpeg.quality= curr_entry.data.i32[0];
                break;

            case ANDROID_JPEG_THUMBNAIL_SIZE:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_INT32, 2))
                    break;
                for (i=0 ; i<curr_entry.count ; i++)
                    dst->ctl.jpeg.thumbnailSize[i] = curr_entry.data.i32[i];
                break;

            case ANDROID_JPEG_THUMBNAIL_QUALITY:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_INT32, 1))
                    break;
                dst->ctl.jpeg.thumbnailQuality= curr_entry.data.i32[0];
                break;

            case ANDROID_JPEG_GPS_COORDINATES:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_DOUBLE, 3))
                    break;
                for (i=0 ; i<curr_entry.count ; i++)
                    dst->ctl.jpeg.gpsCoordinates[i] = curr_entry.data.d[i];
                break;

            case ANDROID_JPEG_GPS_PROCESSING_METHOD:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_BYTE))
                    break;
                if (curr_entry.count > 32)
                    cnt = 32;
                else
                    cnt = curr_entry.count;
                for (i = 0 ; i < cnt ; i++)
                    dst_ext->gpsProcessingMethod[i] = curr_entry.data.u8[i];
                break;

            case ANDROID_JPEG_GPS_TIMESTAMP:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_INT64, 1))
                    break;
                dst->ctl.jpeg.gpsTimestamp = curr_entry.data.i64[0];
                break;

            case ANDROID_JPEG_ORIENTATION:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_INT32, 1))
                    break;
                dst->ctl.jpeg.orientation = curr_entry.data.i32[0];
                break;



            case ANDROID_STATISTICS_FACE_DETECT_MODE:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_BYTE, 1))
                    break;
                dst->ctl.stats.faceDetectMode = (enum facedetect_mode)(curr_entry.data.u8[0] + 1);
                break;

            case ANDROID_CONTROL_CAPTURE_INTENT:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_BYTE, 1))
                    break;
                dst->ctl.aa.captureIntent = (enum aa_capture_intent)curr_entry.data.u8[0];
                break;

            case ANDROID_CONTROL_MODE:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_BYTE, 1))
                    break;
                dst->ctl.aa.mode = (enum aa_mode)(curr_entry.data.u8[0] + 1);
                break;


            case ANDROID_CONTROL_VIDEO_STABILIZATION_MODE:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_BYTE, 1))
                    break;
                dst->ctl.aa.videoStabilizationMode = curr_entry.data.u8[0];
                break;

            case ANDROID_CONTROL_AE_MODE:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_BYTE, 1))
                    break;
                dst->ctl.aa.aeMode = (enum aa_aemode)(curr_entry.data.u8[0] + 2);
                // skip locked mode
                if (dst->ctl.aa.aeMode == AA_AEMODE_LOCKED)
                    dst->ctl.aa.aeMode = AA_AEMODE_OFF;
                ALOGV("DEBUG(%s): ANDROID_CONTROL_AE_MODE (%d)",  __FUNCTION__, dst->ctl.aa.aeMode);
                break;

            case ANDROID_CONTROL_AE_LOCK:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_BYTE, 1))
                    break;
                dst_ext->ae_lock = (enum ae_lockmode)(curr_entry.data.u8[0]);
                break;

            case ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_INT32, 1))
                    break;
                dst->ctl.aa.aeExpCompensation = curr_entry.data.i32[0] + 5;
                break;

            case ANDROID_CONTROL_AWB_MODE:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_BYTE, 1))
                    break;
                dst->ctl.aa.awbMode = (enum aa_awbmode)(curr_entry.data.u8[0] + 2);
                // skip locked mode
                if (dst->ctl.aa.awbMode == AA_AWBMODE_LOCKED)
                    dst->ctl.aa.awbMode = AA_AWBMODE_OFF;
                dst_ext->awb_mode_dm = (enum aa_awbmode)(curr_entry.data.u8[0] + 2);
                break;

            case ANDROID_CONTROL_AWB_LOCK:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_BYTE, 1))
                    break;
                dst_ext->awb_lock = (enum awb_lockmode)(curr_entry.data.u8[0]);
                break;

            case ANDROID_CONTROL_AF_MODE:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_BYTE, 1))
                    break;
                dst->ctl.aa.afMode = (enum aa_afmode)(curr_entry.data.u8[0] + 1);
                if (dst->ctl.aa.afMode == AA_AFMODE_OFF)
                    dst->ctl.aa.afMode = AA_AFMODE_MANUAL;
                break;

            case ANDROID_CONTROL_AF_REGIONS:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_INT32, 5))
                    break;
                for (i=0 ; i<curr_entry.count ; i++)
                    dst->ctl.aa.afRegions[i] = curr_entry.data.i32[i];
                break;

            case ANDROID_CONTROL_AE_REGIONS:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_INT32, 5))
                    break;
                for (i=0 ; i<curr_entry.count ; i++)
                    dst->ctl.aa.aeRegions[i] = curr_entry.data.i32[i];
                break;


            case ANDROID_REQUEST_ID:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_INT32, 1))
                    break;
                dst->ctl.request.id = curr_entry.data.i32[0];
                ALOGV("DEBUG(%s): ANDROID_REQUEST_ID (%d)",  __FUNCTION__,  dst->ctl.request.id);
                break;

            case ANDROID_REQUEST_METADATA_MODE:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_BYTE, 1))
                    break;
                dst->ctl.request.metadataMode = (enum metadata_mode)curr_entry.data.u8[0];
                ALOGV("DEBUG(%s): ANDROID_REQUEST_METADATA_MODE (%d)",  __FUNCTION__, (int)( dst->ctl.request.metadataMode));
                break;

            case ANDROID_REQUEST_OUTPUT_STREAMS:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_INT32))
                    break;

                dst->ctl.request.outputStreams[0] = 0;
                for (i=0 ; i<curr_entry.count ; i++) {
                    ALOGV("DEBUG(%s): OUTPUT_STREAM[%d] = %d ",  __FUNCTION__, i, curr_entry.data.i32[i]);
                    dst->ctl.request.outputStreams[0] |= (1 << curr_entry.data.i32[i]);
                }
                break;

            case ANDROID_REQUEST_INPUT_STREAMS:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_INT32))
                    break;

                for (i=0 ; i<curr_entry.count ; i++) {
                    dst_ext->reprocessInput = curr_entry.data.i32[0];
                    ALOGV("DEBUG(%s): ANDROID_REQUEST_INPUT_STREAMS[%d] = %d ",  __FUNCTION__, i, dst_ext->reprocessInput);
                }
                break;

            case ANDROID_REQUEST_TYPE:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_BYTE, 1))
                    break;
                dst_ext->isReprocessing = curr_entry.data.u8[0];
                ALOGV("DEBUG(%s): ANDROID_REQUEST_TYPE (%d)",  __FUNCTION__, dst_ext->isReprocessing);
                break;

            case ANDROID_REQUEST_FRAME_COUNT:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_INT32, 1))
                    break;
                dst->ctl.request.frameCount = curr_entry.data.i32[0];
                ALOGV("DEBUG(%s): ANDROID_REQUEST_FRAME_COUNT (%d)",  __FUNCTION__, dst->ctl.request.frameCount);
                break;

            case ANDROID_CONTROL_SCENE_MODE:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_BYTE, 1))
                    break;
                dst->ctl.aa.sceneMode = (enum aa_scene_mode)(curr_entry.data.u8[0] + 1);
                break;

            case ANDROID_CONTROL_AE_TARGET_FPS_RANGE:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_INT32, 2))
                    break;
                for (i=0 ; i<curr_entry.count ; i++)
                    dst->ctl.aa.aeTargetFpsRange[i] = curr_entry.data.i32[i];
                break;

            default:
                ALOGV("DEBUG(%s):Bad Metadata tag (%d)",  __FUNCTION__, curr_entry.tag);
                break;
            }
        }
    }
    if (dst->ctl.aa.mode != AA_CONTROL_USE_SCENE_MODE)
        dst->ctl.aa.sceneMode = AA_SCENE_MODE_UNSUPPORTED;
    ApplySceneModeParameters(request, dst_ext);
    return NO_ERROR;
}

status_t MetadataConverter::ApplySceneModeParameters(camera_metadata_t * request, struct camera2_shot_ext * dst_ext)
{
    camera_metadata_entry_t curr_entry;
    struct camera2_shot * dst = NULL;

    ALOGV("DEBUG(%s):", __FUNCTION__);

    dst = &(dst_ext->shot);

    switch (dst->ctl.aa.sceneMode) {

    case AA_SCENE_MODE_ACTION:
        dst->ctl.aa.mode = AA_CONTROL_USE_SCENE_MODE;
        if (dst->ctl.aa.aeMode != AA_AEMODE_LOCKED)
            dst->ctl.aa.aeMode = AA_AEMODE_ON;
        dst->ctl.aa.awbMode = AA_AWBMODE_WB_AUTO;
        dst_ext->awb_mode_dm = AA_AWBMODE_WB_AUTO;
        dst->ctl.aa.isoMode = AA_ISOMODE_AUTO;
        dst->ctl.aa.isoValue = 0;
        dst->ctl.aa.aeTargetFpsRange[0] = 30;
        dst->ctl.aa.aeTargetFpsRange[1] = 30;

        dst->ctl.noise.mode = PROCESSING_MODE_FAST;
        dst->ctl.noise.strength = 0;
        dst->ctl.edge.mode = PROCESSING_MODE_FAST;
        dst->ctl.edge.strength = 0;

        dst->ctl.color.saturation = 3; // means '0'
        break;

    case AA_SCENE_MODE_PARTY:
        dst->ctl.aa.mode = AA_CONTROL_USE_SCENE_MODE;
        if (dst->ctl.aa.aeMode != AA_AEMODE_LOCKED)
            dst->ctl.aa.aeMode = AA_AEMODE_ON;
        dst->ctl.aa.awbMode = AA_AWBMODE_WB_AUTO;
        dst_ext->awb_mode_dm = AA_AWBMODE_WB_AUTO;
        dst->ctl.aa.isoMode = AA_ISOMODE_MANUAL;
        dst->ctl.aa.isoValue = 200;
        dst->ctl.aa.aeTargetFpsRange[0] = 15;
        dst->ctl.aa.aeTargetFpsRange[1] = 30;

        dst->ctl.noise.mode = PROCESSING_MODE_FAST;
        dst->ctl.noise.strength = 0;
        dst->ctl.edge.mode = PROCESSING_MODE_FAST;
        dst->ctl.edge.strength = 0;

        dst->ctl.color.saturation = 4; // means '+1'
        break;

    case AA_SCENE_MODE_SUNSET:
        dst->ctl.aa.mode = AA_CONTROL_USE_SCENE_MODE;
        if (dst->ctl.aa.aeMode != AA_AEMODE_LOCKED)
            dst->ctl.aa.aeMode = AA_AEMODE_ON;
        dst->ctl.aa.awbMode = AA_AWBMODE_WB_DAYLIGHT;
        dst_ext->awb_mode_dm = AA_AWBMODE_WB_DAYLIGHT;
        dst->ctl.aa.isoMode = AA_ISOMODE_AUTO;
        dst->ctl.aa.isoValue = 0;
        dst->ctl.aa.aeTargetFpsRange[0] = 15;
        dst->ctl.aa.aeTargetFpsRange[1] = 30;

        dst->ctl.noise.mode = PROCESSING_MODE_FAST;
        dst->ctl.noise.strength = 0;
        dst->ctl.edge.mode = PROCESSING_MODE_FAST;
        dst->ctl.edge.strength = 0;

        dst->ctl.color.saturation = 3; // means '0'
        break;

    case AA_SCENE_MODE_NIGHT:
        dst->ctl.aa.mode = AA_CONTROL_USE_SCENE_MODE;
        dst->ctl.aa.aeMode = AA_AEMODE_ON; // AE_LOCK is prohibited
        dst->ctl.aa.awbMode = AA_AWBMODE_WB_AUTO;
        dst_ext->awb_mode_dm = AA_AWBMODE_WB_AUTO;
        dst->ctl.aa.isoMode = AA_ISOMODE_AUTO;
        dst->ctl.aa.isoValue = 0;
        dst->ctl.aa.aeTargetFpsRange[0] = 8;
        dst->ctl.aa.aeTargetFpsRange[1] = 30;

        dst->ctl.noise.mode = PROCESSING_MODE_FAST;
        dst->ctl.noise.strength = 0;
        dst->ctl.edge.mode = PROCESSING_MODE_FAST;
        dst->ctl.edge.strength = 0;

        dst->ctl.color.saturation = 3; // means '0'
        break;

    case AA_SCENE_MODE_FACE_PRIORITY:
        dst->ctl.aa.mode = AA_CONTROL_AUTO;
        if ((dst->ctl.aa.aeMode != AA_AEMODE_LOCKED) && (dst->ctl.aa.aeMode < AA_AEMODE_ON_AUTO_FLASH))
            dst->ctl.aa.aeMode = AA_AEMODE_ON;
        dst->ctl.aa.sceneMode = AA_SCENE_MODE_FACE_PRIORITY;
        dst->ctl.aa.isoMode = AA_ISOMODE_AUTO;
        dst->ctl.aa.isoValue = 0;
        dst->ctl.noise.mode = PROCESSING_MODE_OFF;
        dst->ctl.noise.strength = 0;
        dst->ctl.edge.mode = PROCESSING_MODE_OFF;
        dst->ctl.edge.strength = 0;
        dst->ctl.color.saturation = 3; // means '0'
        break;

    default:
        dst->ctl.aa.mode = AA_CONTROL_AUTO;
        if ((dst->ctl.aa.aeMode != AA_AEMODE_LOCKED) && (dst->ctl.aa.aeMode < AA_AEMODE_ON_AUTO_FLASH))
            dst->ctl.aa.aeMode = AA_AEMODE_ON;
        dst->ctl.aa.sceneMode = AA_SCENE_MODE_UNSUPPORTED;
        dst->ctl.aa.isoMode = AA_ISOMODE_AUTO;
        dst->ctl.aa.isoValue = 0;
        dst->ctl.noise.mode = PROCESSING_MODE_OFF;
        dst->ctl.noise.strength = 0;
        dst->ctl.edge.mode = PROCESSING_MODE_OFF;
        dst->ctl.edge.strength = 0;
        dst->ctl.color.saturation = 3; // means '0'
        break;
    }

    return NO_ERROR;
}


status_t MetadataConverter::ToDynamicMetadata(struct camera2_shot_ext * metadata_ext, camera_metadata_t * dst)
{
    status_t    res;
    struct camera2_shot * metadata = &metadata_ext->shot;
    uint8_t  byteData;
    uint32_t intData;

    if (0 != add_camera_metadata_entry(dst, ANDROID_REQUEST_ID,
                &(metadata->ctl.request.id), 1))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_REQUEST_METADATA_MODE,
                &(metadata->ctl.request.metadataMode), 1))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_REQUEST_FRAME_COUNT,
                &(metadata->ctl.request.frameCount), 1))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_SENSOR_TIMESTAMP,
                &metadata->dm.sensor.timeStamp, 1))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_SENSOR_EXPOSURE_TIME,
                &metadata->dm.sensor.exposureTime, 1))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_LENS_APERTURE,
                &metadata->dm.lens.aperture, 1))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_CONTROL_AE_TARGET_FPS_RANGE,
                &(metadata->ctl.aa.aeTargetFpsRange), 2))
        return NO_MEMORY;

    ALOGV("(%s): ID(%d) METAMODE(%d) FrameCnt(%d) Timestamp(%lld) exposure(%lld) aper(%f)", __FUNCTION__,
       metadata->ctl.request.id, metadata->ctl.request.metadataMode, metadata->ctl.request.frameCount,
       metadata->dm.sensor.timeStamp, metadata->dm.sensor.exposureTime, metadata->dm.lens.aperture);


    byteData = metadata_ext->awb_mode_dm- 2;
    if (0 != add_camera_metadata_entry(dst, ANDROID_CONTROL_AWB_MODE,
                &byteData, 1))
        return NO_MEMORY;

    byteData = metadata->dm.aa.aeMode - 1;
    if (0 != add_camera_metadata_entry(dst, ANDROID_CONTROL_AE_MODE,
                &byteData, 1))
        return NO_MEMORY;

    byteData = metadata->ctl.aa.afMode - 1;
    if (0 != add_camera_metadata_entry(dst, ANDROID_CONTROL_AF_MODE,
                &byteData, 1))
        return NO_MEMORY;

    byteData = metadata->ctl.aa.sceneMode - 1;
    if (0 != add_camera_metadata_entry(dst, ANDROID_CONTROL_SCENE_MODE,
                &byteData, 1))
        return NO_MEMORY;

    intData = metadata->ctl.aa.aeExpCompensation - 5;
    if (0 != add_camera_metadata_entry(dst, ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION,
                &intData, 1))
        return NO_MEMORY;

    byteData = metadata->dm.stats.faceDetectMode - 1;
    if (0 != add_camera_metadata_entry(dst, ANDROID_STATISTICS_FACE_DETECT_MODE,
                &byteData, 1))
        return NO_MEMORY;

    int maxFacecount = CAMERA2_MAX_FACES;
    if (0 != add_camera_metadata_entry(dst, ANDROID_STATISTICS_INFO_MAX_FACE_COUNT,
                &maxFacecount, 1))
        return NO_MEMORY;

    int tempFaceCount = 0;
    for (int i = 0; i < CAMERA2_MAX_FACES; i++) {
        if (metadata->dm.stats.faceIds[i] > 0) {
            mataFaceIds[tempFaceCount] = metadata->dm.stats.faceIds[i];
            // clipping fd score because the max face score of android is 100
            if (metadata->dm.stats.faceScores[i] > 100)
                metaFaceScores[tempFaceCount] = 100;
            else
                metaFaceScores[tempFaceCount] = metadata->dm.stats.faceScores[i];

            memcpy(&mataFaceLandmarks[tempFaceCount][0], &metadata->dm.stats.faceLandmarks[i][0], 6*sizeof(uint32_t));
            memcpy(&metaFaceRectangles[tempFaceCount][0], &metadata->dm.stats.faceRectangles[i][0], 4*sizeof(uint32_t));
            tempFaceCount++;
        }
    }

    if (tempFaceCount > 0) {
        if (0 != add_camera_metadata_entry(dst, ANDROID_STATISTICS_FACE_RECTANGLES,
                    &metaFaceRectangles, 4 * tempFaceCount))
            return NO_MEMORY;

        if (0 != add_camera_metadata_entry(dst, ANDROID_STATISTICS_FACE_LANDMARKS,
                    &mataFaceLandmarks, 6 * tempFaceCount))
            return NO_MEMORY;

        if (0 != add_camera_metadata_entry(dst, ANDROID_STATISTICS_FACE_IDS,
                    &mataFaceIds, tempFaceCount))
            return NO_MEMORY;

        if (0 != add_camera_metadata_entry(dst, ANDROID_STATISTICS_FACE_SCORES,
                    &metaFaceScores, tempFaceCount))
            return NO_MEMORY;
    }
    if (0 != add_camera_metadata_entry(dst, ANDROID_SENSOR_SENSITIVITY,
                &metadata->dm.aa.isoValue, 1))
        return NO_MEMORY;

    // Need a four-entry crop region
    uint32_t cropRegion[4] = {
        metadata->ctl.scaler.cropRegion[0],
        metadata->ctl.scaler.cropRegion[1],
        metadata->ctl.scaler.cropRegion[2],
        0
    };
    if (0 != add_camera_metadata_entry(dst, ANDROID_SCALER_CROP_REGION,
                cropRegion, 4))
        return NO_MEMORY;

    byteData = metadata->dm.aa.aeState - 1;
    if (0 != add_camera_metadata_entry(dst, ANDROID_CONTROL_AE_STATE,
                &byteData, 1))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_CONTROL_AWB_STATE,
                &(metadata->dm.aa.awbState), 1))
        return NO_MEMORY;


    if (0 != add_camera_metadata_entry(dst, ANDROID_JPEG_ORIENTATION,
                &metadata->ctl.jpeg.orientation, 1))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_JPEG_QUALITY,
                &metadata->ctl.jpeg.quality, 1))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_JPEG_THUMBNAIL_SIZE,
                &metadata->ctl.jpeg.thumbnailSize, 2))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_JPEG_THUMBNAIL_QUALITY,
                &metadata->ctl.jpeg.thumbnailQuality, 1))
        return NO_MEMORY;


    if (0 != add_camera_metadata_entry(dst, ANDROID_JPEG_GPS_COORDINATES,
                &(metadata->ctl.jpeg.gpsCoordinates), 3))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_JPEG_GPS_PROCESSING_METHOD,
                &(metadata_ext->gpsProcessingMethod), 32))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_JPEG_GPS_TIMESTAMP,
                &(metadata->ctl.jpeg.gpsTimestamp), 1))
        return NO_MEMORY;
    ALOGV("(%s): AWB(%d) AE(%d) SCENE(%d)  AEComp(%d) AF(%d)", __FUNCTION__,
       metadata_ext->awb_mode_dm- 2, metadata->dm.aa.aeMode - 1, metadata->ctl.aa.sceneMode - 1,
       metadata->ctl.aa.aeExpCompensation, metadata->ctl.aa.afMode - 1);


    if (metadata->ctl.request.metadataMode == METADATA_MODE_NONE) {
        ALOGV("DEBUG(%s): METADATA_MODE_NONE", __FUNCTION__);
        return NO_ERROR;
    }
    return NO_ERROR;


}

}; // namespace android
