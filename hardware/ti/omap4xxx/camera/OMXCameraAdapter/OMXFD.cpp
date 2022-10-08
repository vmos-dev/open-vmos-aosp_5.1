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
* @file OMXFD.cpp
*
* This file contains functionality for handling face detection.
*
*/

#undef LOG_TAG

#define LOG_TAG "CameraHAL"

#include "CameraHal.h"
#include "OMXCameraAdapter.h"

#define FACE_DETECTION_THRESHOLD 80

// constants used for face smooth filtering
static const int HorizontalFilterThreshold = 40;
static const int VerticalFilterThreshold = 40;
static const int HorizontalFaceSizeThreshold = 30;
static const int VerticalFaceSizeThreshold = 30;


namespace android {

status_t OMXCameraAdapter::setParametersFD(const CameraParameters &params,
                                           BaseCameraAdapter::AdapterState state)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::startFaceDetection()
{
    status_t ret = NO_ERROR;

    Mutex::Autolock lock(mFaceDetectionLock);

    ret = setFaceDetection(true, mDeviceOrientation);
    if (ret != NO_ERROR) {
        goto out;
    }

    if ( mFaceDetectionRunning ) {
        mFDSwitchAlgoPriority = true;
    }

    // Note: White balance will not be face prioritized, since
    // the algorithm needs full frame statistics, and not face
    // regions alone.

    faceDetectionNumFacesLastOutput = 0;
 out:
    return ret;
}

status_t OMXCameraAdapter::stopFaceDetection()
{
    status_t ret = NO_ERROR;
    const char *str = NULL;
    BaseCameraAdapter::AdapterState state;
    BaseCameraAdapter::getState(state);

    Mutex::Autolock lock(mFaceDetectionLock);

    ret = setFaceDetection(false, mDeviceOrientation);
    if (ret != NO_ERROR) {
        goto out;
    }

    // Reset 3A settings
    ret = setParameters3A(mParams, state);
    if (ret != NO_ERROR) {
        goto out;
    }

    if (mPending3Asettings) {
        apply3Asettings(mParameters3A);
    }

    faceDetectionNumFacesLastOutput = 0;
 out:
    return ret;
}

void OMXCameraAdapter::pauseFaceDetection(bool pause)
{
    Mutex::Autolock lock(mFaceDetectionLock);
    // pausing will only take affect if fd is already running
    if (mFaceDetectionRunning) {
        mFaceDetectionPaused = pause;
        faceDetectionNumFacesLastOutput = 0;
    }
}

status_t OMXCameraAdapter::setFaceDetection(bool enable, OMX_U32 orientation)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_OBJDETECTIONTYPE objDetection;

    LOG_FUNCTION_NAME;

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        ret = -EINVAL;
        }

    if ( NO_ERROR == ret )
        {
        if ( orientation > 270 ) {
            orientation = 0;
        }

        OMX_INIT_STRUCT_PTR (&objDetection, OMX_CONFIG_OBJDETECTIONTYPE);
        objDetection.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;
        objDetection.nDeviceOrientation = orientation;
        if  ( enable )
            {
            objDetection.bEnable = OMX_TRUE;
            }
        else
            {
            objDetection.bEnable = OMX_FALSE;
            }

        eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                                ( OMX_INDEXTYPE ) OMX_IndexConfigImageFaceDetection,
                                &objDetection);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while configuring face detection 0x%x", eError);
            ret = -1;
            }
        else
            {
            CAMHAL_LOGDA("Face detection configured successfully");
            }
        }

    if ( NO_ERROR == ret )
        {
        ret = setExtraData(enable, mCameraAdapterParameters.mPrevPortIndex, OMX_FaceDetection);

        if ( NO_ERROR != ret )
            {
            CAMHAL_LOGEA("Error while configuring face detection extra data");
            }
        else
            {
            CAMHAL_LOGDA("Face detection extra data configured successfully");
            }
        }

    if ( NO_ERROR == ret )
        {
        mFaceDetectionRunning = enable;
        mFaceDetectionPaused = !enable;
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::detectFaces(OMX_BUFFERHEADERTYPE* pBuffHeader,
                                       sp<CameraFDResult> &result,
                                       size_t previewWidth,
                                       size_t previewHeight)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_TI_FACERESULT *faceResult;
    OMX_OTHER_EXTRADATATYPE *extraData;
    OMX_FACEDETECTIONTYPE *faceData;
    OMX_TI_PLATFORMPRIVATE *platformPrivate;
    camera_frame_metadata_t *faces;

    LOG_FUNCTION_NAME;

    if ( OMX_StateExecuting != mComponentState ) {
        CAMHAL_LOGEA("OMX component is not in executing state");
        return NO_INIT;
    }

    if ( NULL == pBuffHeader ) {
        CAMHAL_LOGEA("Invalid Buffer header");
        return-EINVAL;
    }

    platformPrivate = (OMX_TI_PLATFORMPRIVATE *) (pBuffHeader->pPlatformPrivate);
    if ( NULL != platformPrivate ) {
        if ( sizeof(OMX_TI_PLATFORMPRIVATE) == platformPrivate->nSize ) {
            CAMHAL_LOGVB("Size = %d, sizeof = %d, pAuxBuf = 0x%x, pAuxBufSize= %d, pMetaDataBufer = 0x%x, nMetaDataSize = %d",
                         platformPrivate->nSize,
                         sizeof(OMX_TI_PLATFORMPRIVATE),
                         platformPrivate->pAuxBuf1,
                         platformPrivate->pAuxBufSize1,
                         platformPrivate->pMetaDataBuffer,
                         platformPrivate->nMetaDataSize);
        } else {
            CAMHAL_LOGDB("OMX_TI_PLATFORMPRIVATE size mismatch: expected = %d, received = %d",
                         ( unsigned int ) sizeof(OMX_TI_PLATFORMPRIVATE),
                         ( unsigned int ) platformPrivate->nSize);
            return -EINVAL;
        }
    }  else {
        CAMHAL_LOGDA("Invalid OMX_TI_PLATFORMPRIVATE");
        return-EINVAL;
    }


    if ( 0 >= platformPrivate->nMetaDataSize ) {
        CAMHAL_LOGDB("OMX_TI_PLATFORMPRIVATE nMetaDataSize is size is %d",
                     ( unsigned int ) platformPrivate->nMetaDataSize);
        return -EINVAL;
    }

    extraData = getExtradata((OMX_OTHER_EXTRADATATYPE *) (platformPrivate->pMetaDataBuffer),
            (OMX_EXTRADATATYPE)OMX_FaceDetection);

    if ( NULL != extraData ) {
        CAMHAL_LOGVB("Size = %d, sizeof = %d, eType = 0x%x, nDataSize= %d, nPortIndex = 0x%x, nVersion = 0x%x",
                     extraData->nSize,
                     sizeof(OMX_OTHER_EXTRADATATYPE),
                     extraData->eType,
                     extraData->nDataSize,
                     extraData->nPortIndex,
                     extraData->nVersion);
    } else {
        CAMHAL_LOGDA("Invalid OMX_OTHER_EXTRADATATYPE");
        return -EINVAL;
    }

    faceData = ( OMX_FACEDETECTIONTYPE * ) extraData->data;
    if ( NULL != faceData ) {
        if ( sizeof(OMX_FACEDETECTIONTYPE) == faceData->nSize ) {
            CAMHAL_LOGVB("Faces detected %d",
                         faceData->ulFaceCount,
                         faceData->nSize,
                         sizeof(OMX_FACEDETECTIONTYPE),
                         faceData->eCameraView,
                         faceData->nPortIndex,
                         faceData->nVersion);
        } else {
            CAMHAL_LOGDB("OMX_FACEDETECTIONTYPE size mismatch: expected = %d, received = %d",
                         ( unsigned int ) sizeof(OMX_FACEDETECTIONTYPE),
                         ( unsigned int ) faceData->nSize);
            return -EINVAL;
        }
    } else {
        CAMHAL_LOGEA("Invalid OMX_FACEDETECTIONTYPE");
        return -EINVAL;
    }

    ret = encodeFaceCoordinates(faceData, &faces, previewWidth, previewHeight);

    if ( NO_ERROR == ret ) {
        result = new CameraFDResult(faces);
    } else {
        result.clear();
        result = NULL;
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::encodeFaceCoordinates(const OMX_FACEDETECTIONTYPE *faceData,
                                                 camera_frame_metadata_t **pFaces,
                                                 size_t previewWidth,
                                                 size_t previewHeight)
{
    status_t ret = NO_ERROR;
    camera_face_t *faces;
    camera_frame_metadata_t *faceResult;
    size_t hRange, vRange;
    double tmp;

    LOG_FUNCTION_NAME;

    if ( NULL == faceData ) {
        CAMHAL_LOGEA("Invalid OMX_FACEDETECTIONTYPE parameter");
        return EINVAL;
    }

    LOG_FUNCTION_NAME

    hRange = CameraFDResult::RIGHT - CameraFDResult::LEFT;
    vRange = CameraFDResult::BOTTOM - CameraFDResult::TOP;

    faceResult = ( camera_frame_metadata_t * ) malloc(sizeof(camera_frame_metadata_t));
    if ( NULL == faceResult ) {
        return -ENOMEM;
    }

    if ( 0 < faceData->ulFaceCount ) {
        int orient_mult;
        int trans_left, trans_top, trans_right, trans_bot;

        faces = ( camera_face_t * ) malloc(sizeof(camera_face_t)*faceData->ulFaceCount);
        if ( NULL == faces ) {
            return -ENOMEM;
        }

        /**
        / * When device is 180 degrees oriented to the sensor, need to translate
        / * the output from Ducati to what Android expects
        / * Ducati always gives face coordinates in this form, irrespective of
        / * rotation, i.e (l,t) always represents the point towards the left eye
        / * and top of hair.
        / * (l, t)
        / *   ---------------
        / *   -   ,,,,,,,   -
        / *   -  |       |  -
        / *   -  |<a   <a|  -
        / *   - (|   ^   |) -
        / *   -  |  -=-  |  -
        / *   -   \_____/   -
        / *   ---------------
        / *               (r, b)
        / *
        / * However, Android expects the coords to be in respect with what the
        / * sensor is viewing, i.e Android expects sensor to see this with (l,t)
        / * and (r,b) like so:
        / * (l, t)
        / *   ---------------
        / *   -    _____    -
        / *   -   /     \   -
        / *   -  |  -=-  |  -
        / *   - (|   ^   |) -
        / *   -  |a>   a>|  -
        / *   -  |       |  -
        / *   -   ,,,,,,,   -
        / *   ---------------
        / *               (r, b)
          */

        if (mDeviceOrientation == 180) {
            orient_mult = -1;
            trans_left = 2; // right is now left
            trans_top = 3; // bottom is now top
            trans_right = 0; // left is now right
            trans_bot = 1; // top is not bottom
        } else {
            orient_mult = 1;
            trans_left = 0; // left
            trans_top = 1; // top
            trans_right = 2; // right
            trans_bot = 3; // bottom
        }

        int j = 0, i = 0;
        for ( ; j < faceData->ulFaceCount ; j++)
            {
             OMX_S32 nLeft = 0;
             OMX_S32 nTop = 0;
             //Face filtering
             //For real faces, it is seen that the h/w passes a score >=80
             //For false faces, we seem to get even a score of 70 sometimes.
             //In order to avoid any issue at application level, we filter
             //<=70 score here.
            if(faceData->tFacePosition[j].nScore <= FACE_DETECTION_THRESHOLD)
             continue;

            if (mDeviceOrientation == 180) {
                // from sensor pov, the left pos is the right corner of the face in pov of frame
                nLeft = faceData->tFacePosition[j].nLeft + faceData->tFacePosition[j].nWidth;
                nTop =  faceData->tFacePosition[j].nTop + faceData->tFacePosition[j].nHeight;
            } else {
                nLeft = faceData->tFacePosition[j].nLeft;
                nTop =  faceData->tFacePosition[j].nTop;
            }

            tmp = ( double ) nLeft / ( double ) previewWidth;
            tmp *= hRange;
            tmp -= hRange/2;
            faces[i].rect[trans_left] = tmp;

            tmp = ( double ) nTop / ( double )previewHeight;
            tmp *= vRange;
            tmp -= vRange/2;
            faces[i].rect[trans_top] = tmp;

            tmp = ( double ) faceData->tFacePosition[j].nWidth / ( double ) previewWidth;
            tmp *= hRange;
            tmp *= orient_mult;
            faces[i].rect[trans_right] = faces[i].rect[trans_left] + tmp;

            tmp = ( double ) faceData->tFacePosition[j].nHeight / ( double ) previewHeight;
            tmp *= vRange;
            tmp *= orient_mult;
            faces[i].rect[trans_bot] = faces[i].rect[trans_top] + tmp;

            faces[i].score = faceData->tFacePosition[j].nScore;
            faces[i].id = 0;
            faces[i].left_eye[0] = CameraFDResult::INVALID_DATA;
            faces[i].left_eye[1] = CameraFDResult::INVALID_DATA;
            faces[i].right_eye[0] = CameraFDResult::INVALID_DATA;
            faces[i].right_eye[1] = CameraFDResult::INVALID_DATA;
            faces[i].mouth[0] = CameraFDResult::INVALID_DATA;
            faces[i].mouth[1] = CameraFDResult::INVALID_DATA;
            i++;
            }

        faceResult->number_of_faces = i;
        faceResult->faces = faces;

        for (int i = 0; i  < faceResult->number_of_faces; i++)
        {
            int centerX = (faces[i].rect[trans_left] + faces[i].rect[trans_right] ) / 2;
            int centerY = (faces[i].rect[trans_top] + faces[i].rect[trans_bot] ) / 2;

            int sizeX = (faces[i].rect[trans_right] - faces[i].rect[trans_left] ) ;
            int sizeY = (faces[i].rect[trans_bot] - faces[i].rect[trans_top] ) ;

            for (int j = 0; j < faceDetectionNumFacesLastOutput; j++)
            {
                int tempCenterX = (faceDetectionLastOutput[j].rect[trans_left] +
                                  faceDetectionLastOutput[j].rect[trans_right] ) / 2;
                int tempCenterY = (faceDetectionLastOutput[j].rect[trans_top] +
                                  faceDetectionLastOutput[j].rect[trans_bot] ) / 2;
                int tempSizeX = (faceDetectionLastOutput[j].rect[trans_right] -
                                faceDetectionLastOutput[j].rect[trans_left] ) ;
                int tempSizeY = (faceDetectionLastOutput[j].rect[trans_bot] -
                                faceDetectionLastOutput[j].rect[trans_top] ) ;

                if ( (abs(tempCenterX - centerX) < HorizontalFilterThreshold) &&
                     (abs(tempCenterY - centerY) < VerticalFilterThreshold) )
                {
                    // Found Face. It did not move too far.
                    // Now check size of rectangle compare to last output
                    if ( (abs (tempSizeX -sizeX) < HorizontalFaceSizeThreshold) &&
                         (abs (tempSizeY -sizeY) < VerticalFaceSizeThreshold) )
                    {
                        // Rectangle is almost same as last time
                        // Output exactly what was done for this face last time.
                        faces[i] = faceDetectionLastOutput[j];
                    }
                    else
                    {
                        // TODO(XXX): Rectangle size changed but position is same.
                        // Possibly we can apply just positional correctness.
                    }
                }
            }
        }

        // Save this output for next iteration
        for (int i = 0; i  < faceResult->number_of_faces; i++)
        {
            faceDetectionLastOutput[i] = faces[i];
        }
        faceDetectionNumFacesLastOutput = faceResult->number_of_faces;
    } else {
        faceResult->number_of_faces = 0;
        faceResult->faces = NULL;
    }

    *pFaces = faceResult;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

};
