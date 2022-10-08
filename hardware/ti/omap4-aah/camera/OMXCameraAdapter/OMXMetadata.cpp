/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
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

/**
* @file OMX3A.cpp
*
* This file contains functionality for handling 3A configurations.
*
*/

#undef LOG_TAG

#define LOG_TAG "OMXMetaData"

#include "OMXCameraAdapter.h"
#include <camera/CameraMetadata.h>

namespace Ti {
namespace Camera {

#ifdef OMAP_ENHANCEMENT_CPCAM
camera_memory_t * OMXCameraAdapter::getMetaData(const OMX_PTR plat_pvt,
                                                camera_request_memory allocator) const
{
    camera_memory_t * ret = NULL;

    OMX_OTHER_EXTRADATATYPE *extraData;
    OMX_FACEDETECTIONTYPE *faceData = NULL;
    OMX_TI_WHITEBALANCERESULTTYPE * WBdata = NULL;
    OMX_TI_VECTSHOTINFOTYPE *shotInfo = NULL;
    OMX_TI_LSCTABLETYPE *lscTbl = NULL;
    camera_metadata_t *metaData;
    size_t offset = 0;

    size_t metaDataSize = sizeof(camera_metadata_t);

    extraData = getExtradata(plat_pvt, (OMX_EXTRADATATYPE) OMX_FaceDetection);
    if ( NULL != extraData ) {
        faceData = ( OMX_FACEDETECTIONTYPE * ) extraData->data;
        metaDataSize += faceData->ulFaceCount * sizeof(camera_metadata_face_t);
    }

    extraData = getExtradata(plat_pvt, (OMX_EXTRADATATYPE) OMX_WhiteBalance);
    if ( NULL != extraData ) {
        WBdata = ( OMX_TI_WHITEBALANCERESULTTYPE * ) extraData->data;
    }

    extraData = getExtradata(plat_pvt, (OMX_EXTRADATATYPE) OMX_TI_VectShotInfo);
    if ( NULL != extraData ) {
        shotInfo = ( OMX_TI_VECTSHOTINFOTYPE * ) extraData->data;
    }

    extraData = getExtradata(plat_pvt, (OMX_EXTRADATATYPE) OMX_TI_LSCTable);
    if ( NULL != extraData ) {
        lscTbl = ( OMX_TI_LSCTABLETYPE * ) extraData->data;
        metaDataSize += OMX_TI_LSC_GAIN_TABLE_SIZE;
    }

    ret = allocator(-1, metaDataSize, 1, NULL);
    if ( NULL == ret ) {
        return NULL;
    } else {
        metaData = static_cast<camera_metadata_t *> (ret->data);
        offset += sizeof(camera_metadata_t);
    }

    if ( NULL != faceData ) {
        metaData->number_of_faces = 0;
        int idx = 0;
        metaData->faces_offset = offset;
        struct camera_metadata_face *faces = reinterpret_cast<struct camera_metadata_face *> (static_cast<char*>(ret->data) + offset);
        for ( int j = 0; j < faceData->ulFaceCount ; j++ ) {
            if(faceData->tFacePosition[j].nScore <= FACE_DETECTION_THRESHOLD) {
                continue;
            }
            idx = metaData->number_of_faces;
            metaData->number_of_faces++;
            // TODO: Rework and re-use encodeFaceCoordinates()
            faces[idx].left  = faceData->tFacePosition[j].nLeft;
            faces[idx].top = faceData->tFacePosition[j].nTop;
            faces[idx].bottom = faceData->tFacePosition[j].nWidth;
            faces[idx].right = faceData->tFacePosition[j].nHeight;
        }
        offset += sizeof(camera_metadata_face_t) * metaData->number_of_faces;
    }

    if ( NULL != WBdata ) {
        metaData->awb_temp = WBdata->nColorTemperature;
        metaData->gain_b = WBdata->nGainB;
        metaData->gain_gb = WBdata->nGainGB;
        metaData->gain_gr = WBdata->nGainGR;
        metaData->gain_r = WBdata->nGainR;
        metaData->offset_b = WBdata->nOffsetB;
        metaData->offset_gb = WBdata->nOffsetGB;
        metaData->offset_gr = WBdata->nOffsetGR;
        metaData->offset_r = WBdata->nOffsetR;
    }

    if ( NULL != lscTbl ) {
        metaData->lsc_table_applied = lscTbl->bApplied;
        metaData->lsc_table_size = OMX_TI_LSC_GAIN_TABLE_SIZE;
        metaData->lsc_table_offset = offset;
        uint8_t *lsc_table = reinterpret_cast<uint8_t *> (static_cast<char*>(ret->data) + offset);
        memcpy(lsc_table, lscTbl->pGainTable, OMX_TI_LSC_GAIN_TABLE_SIZE);
        offset += metaData->lsc_table_size;
    }

    if ( NULL != shotInfo ) {
        metaData->frame_number = shotInfo->nFrameNum;
        metaData->shot_number = shotInfo->nConfigId;
        metaData->analog_gain = shotInfo->nAGain;
        metaData->analog_gain_req = shotInfo->nReqGain;
        metaData->analog_gain_min = shotInfo->nGainMin;
        metaData->analog_gain_max = shotInfo->nGainMax;
        metaData->analog_gain_error = shotInfo->nSenAGainErr;
        metaData->analog_gain_dev = shotInfo->nDevAGain;
        metaData->exposure_time = shotInfo->nExpTime;
        metaData->exposure_time_req = shotInfo->nReqExpTime;
        metaData->exposure_time_min = shotInfo->nExpMin;
        metaData->exposure_time_max = shotInfo->nExpMax;
        metaData->exposure_time_dev = shotInfo->nDevExpTime;
        metaData->exposure_time_error = shotInfo->nSenExpTimeErr;
        metaData->exposure_compensation_req = shotInfo->nReqEC;
        metaData->exposure_dev = shotInfo->nDevEV;
    }

    return ret;
}
#endif

status_t OMXCameraAdapter::encodePreviewMetadata(camera_frame_metadata_t *meta, const OMX_PTR plat_pvt)
{
    status_t ret = NO_ERROR;
#ifdef OMAP_ENHANCEMENT_CPCAM
    OMX_OTHER_EXTRADATATYPE *extraData = NULL;

    extraData = getExtradata(plat_pvt, (OMX_EXTRADATATYPE) OMX_TI_VectShotInfo);

    if ( (NULL != extraData) && (NULL != extraData->data) ) {
        OMX_TI_VECTSHOTINFOTYPE *shotInfo;
        shotInfo = (OMX_TI_VECTSHOTINFOTYPE*) extraData->data;

        meta->analog_gain = shotInfo->nAGain;
        meta->exposure_time = shotInfo->nExpTime;
    } else {
        meta->analog_gain = -1;
        meta->exposure_time = -1;
    }

    // Send metadata event only after any value has been changed
    if ((metadataLastAnalogGain == meta->analog_gain) &&
        (metadataLastExposureTime == meta->exposure_time)) {
        ret = NOT_ENOUGH_DATA;
    } else {
        metadataLastAnalogGain = meta->analog_gain;
        metadataLastExposureTime = meta->exposure_time;
    }
#else
    // no-op in non enhancement mode
    CAMHAL_UNUSED(meta);
    CAMHAL_UNUSED(plat_pvt);
#endif

    return ret;
}

} // namespace Camera
} // namespace Ti
