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
* @file OMXExif.cpp
*
* This file contains functionality for handling EXIF insertion.
*
*/

#include "CameraHal.h"
#include "OMXCameraAdapter.h"
#include <math.h>

namespace Ti {
namespace Camera {

status_t OMXCameraAdapter::setParametersEXIF(const android::CameraParameters &params,
                                             BaseCameraAdapter::AdapterState state)
{
    status_t ret = NO_ERROR;
    const char *valstr = NULL;
    double gpsPos;

    LOG_FUNCTION_NAME;

    if( ( valstr = params.get(android::CameraParameters::KEY_GPS_LATITUDE) ) != NULL )
        {
        gpsPos = strtod(valstr, NULL);

        if ( convertGPSCoord(gpsPos,
                             mEXIFData.mGPSData.mLatDeg,
                             mEXIFData.mGPSData.mLatMin,
                             mEXIFData.mGPSData.mLatSec,
                             mEXIFData.mGPSData.mLatSecDiv ) == NO_ERROR )
            {

            if ( 0 < gpsPos )
                {
                strncpy(mEXIFData.mGPSData.mLatRef, GPS_NORTH_REF, GPS_REF_SIZE);
                }
            else
                {
                strncpy(mEXIFData.mGPSData.mLatRef, GPS_SOUTH_REF, GPS_REF_SIZE);
                }

            mEXIFData.mGPSData.mLatValid = true;
            }
        else
            {
            mEXIFData.mGPSData.mLatValid = false;
            }
        }
    else
        {
        mEXIFData.mGPSData.mLatValid = false;
        }

    if( ( valstr = params.get(android::CameraParameters::KEY_GPS_LONGITUDE) ) != NULL )
        {
        gpsPos = strtod(valstr, NULL);

        if ( convertGPSCoord(gpsPos,
                             mEXIFData.mGPSData.mLongDeg,
                             mEXIFData.mGPSData.mLongMin,
                             mEXIFData.mGPSData.mLongSec,
                             mEXIFData.mGPSData.mLongSecDiv) == NO_ERROR )
            {

            if ( 0 < gpsPos )
                {
                strncpy(mEXIFData.mGPSData.mLongRef, GPS_EAST_REF, GPS_REF_SIZE);
                }
            else
                {
                strncpy(mEXIFData.mGPSData.mLongRef, GPS_WEST_REF, GPS_REF_SIZE);
                }

            mEXIFData.mGPSData.mLongValid= true;
            }
        else
            {
            mEXIFData.mGPSData.mLongValid = false;
            }
        }
    else
        {
        mEXIFData.mGPSData.mLongValid = false;
        }

    if( ( valstr = params.get(android::CameraParameters::KEY_GPS_ALTITUDE) ) != NULL )
        {
        gpsPos = strtod(valstr, NULL);
        mEXIFData.mGPSData.mAltitude = floor(fabs(gpsPos));
        if (gpsPos < 0) {
            mEXIFData.mGPSData.mAltitudeRef = 1;
        } else {
            mEXIFData.mGPSData.mAltitudeRef = 0;
        }
        mEXIFData.mGPSData.mAltitudeValid = true;
        }
    else
        {
        mEXIFData.mGPSData.mAltitudeValid= false;
        }

    if( (valstr = params.get(android::CameraParameters::KEY_GPS_TIMESTAMP)) != NULL )
        {
        long gpsTimestamp = strtol(valstr, NULL, 10);
        struct tm *timeinfo = gmtime( ( time_t * ) & (gpsTimestamp) );
        if ( NULL != timeinfo )
            {
            mEXIFData.mGPSData.mTimeStampHour = timeinfo->tm_hour;
            mEXIFData.mGPSData.mTimeStampMin = timeinfo->tm_min;
            mEXIFData.mGPSData.mTimeStampSec = timeinfo->tm_sec;
            mEXIFData.mGPSData.mTimeStampValid = true;
            }
        else
            {
            mEXIFData.mGPSData.mTimeStampValid = false;
            }
        }
    else
        {
        mEXIFData.mGPSData.mTimeStampValid = false;
        }

    if( ( valstr = params.get(android::CameraParameters::KEY_GPS_TIMESTAMP) ) != NULL )
        {
        long gpsDatestamp = strtol(valstr, NULL, 10);
        struct tm *timeinfo = gmtime( ( time_t * ) & (gpsDatestamp) );
        if ( NULL != timeinfo )
            {
            strftime(mEXIFData.mGPSData.mDatestamp, GPS_DATESTAMP_SIZE, "%Y:%m:%d", timeinfo);
            mEXIFData.mGPSData.mDatestampValid = true;
            }
        else
            {
            mEXIFData.mGPSData.mDatestampValid = false;
            }
        }
    else
        {
        mEXIFData.mGPSData.mDatestampValid = false;
        }

    if( ( valstr = params.get(android::CameraParameters::KEY_GPS_PROCESSING_METHOD) ) != NULL )
        {
        strncpy(mEXIFData.mGPSData.mProcMethod, valstr, GPS_PROCESSING_SIZE-1);
        mEXIFData.mGPSData.mProcMethodValid = true;
        }
    else
        {
        mEXIFData.mGPSData.mProcMethodValid = false;
        }

    if( ( valstr = params.get(TICameraParameters::KEY_GPS_MAPDATUM) ) != NULL )
        {
        strncpy(mEXIFData.mGPSData.mMapDatum, valstr, GPS_MAPDATUM_SIZE-1);
        mEXIFData.mGPSData.mMapDatumValid = true;
        }
    else
        {
        mEXIFData.mGPSData.mMapDatumValid = false;
        }

    if( ( valstr = params.get(TICameraParameters::KEY_GPS_VERSION) ) != NULL )
        {
        strncpy(mEXIFData.mGPSData.mVersionId, valstr, GPS_VERSION_SIZE-1);
        mEXIFData.mGPSData.mVersionIdValid = true;
        }
    else
        {
        mEXIFData.mGPSData.mVersionIdValid = false;
        }

    if( ( valstr = params.get(TICameraParameters::KEY_EXIF_MODEL ) ) != NULL )
        {
        CAMHAL_LOGVB("EXIF Model: %s", valstr);
        strncpy(mEXIFData.mModel, valstr, EXIF_MODEL_SIZE - 1);
        mEXIFData.mModelValid= true;
        }
    else
        {
        mEXIFData.mModelValid= false;
        }

    if( ( valstr = params.get(TICameraParameters::KEY_EXIF_MAKE ) ) != NULL )
        {
        CAMHAL_LOGVB("EXIF Make: %s", valstr);
        strncpy(mEXIFData.mMake, valstr, EXIF_MAKE_SIZE - 1);
        mEXIFData.mMakeValid = true;
        }
    else
        {
        mEXIFData.mMakeValid= false;
        }


    if( ( valstr = params.get(android::CameraParameters::KEY_FOCAL_LENGTH) ) != NULL ) {
        CAMHAL_LOGVB("EXIF Focal length: %s", valstr);
        ExifElementsTable::stringToRational(valstr,
                                            &mEXIFData.mFocalNum,
                                            &mEXIFData.mFocalDen);
    } else {
        mEXIFData.mFocalNum = 0;
        mEXIFData.mFocalDen = 0;
    }


    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::setupEXIF()
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_TI_CONFIG_SHAREDBUFFER sharedBuffer;
    OMX_TI_CONFIG_EXIF_TAGS *exifTags;
    unsigned char *startPtr = NULL;
    unsigned char *sharedPtr = NULL;
    struct timeval sTv;
    struct tm *pTime;
    OMXCameraPortParameters * capData = NULL;
    CameraBuffer *memmgr_buf_array;
    int buf_size = 0;

    LOG_FUNCTION_NAME;

    sharedBuffer.pSharedBuff = NULL;
    capData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        ret = -EINVAL;
        }

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT_PTR (&sharedBuffer, OMX_TI_CONFIG_SHAREDBUFFER);
        sharedBuffer.nPortIndex = mCameraAdapterParameters.mImagePortIndex;

        //We allocate the shared buffer dynamically based on the
        //requirements of the EXIF tags. The additional buffers will
        //get stored after the EXIF configuration structure and the pointers
        //will contain offsets within the shared buffer itself.
        buf_size = sizeof(OMX_TI_CONFIG_EXIF_TAGS) +
                          ( EXIF_MODEL_SIZE ) +
                          ( EXIF_MAKE_SIZE ) +
                          ( EXIF_DATE_TIME_SIZE ) +
                          ( GPS_MAPDATUM_SIZE ) +
                          ( GPS_PROCESSING_SIZE );
        buf_size = ((buf_size+4095)/4096)*4096;
        sharedBuffer.nSharedBuffSize = buf_size;

        memmgr_buf_array = mMemMgr.allocateBufferList(0, 0, NULL, buf_size, 1);
        sharedBuffer.pSharedBuff = (OMX_U8*)camera_buffer_get_omx_ptr(&memmgr_buf_array[0]);
        startPtr =  ( OMX_U8 * ) memmgr_buf_array[0].opaque;

        if ( NULL == startPtr)
            {
            CAMHAL_LOGEA("No resources to allocate OMX shared buffer");
            ret = -1;
            }

        //Extra data begins right after the EXIF configuration structure.
        sharedPtr = startPtr + sizeof(OMX_TI_CONFIG_EXIF_TAGS);
        }

    if ( NO_ERROR == ret )
        {
        exifTags = ( OMX_TI_CONFIG_EXIF_TAGS * ) startPtr;
        OMX_INIT_STRUCT_PTR (exifTags, OMX_TI_CONFIG_EXIF_TAGS);
        exifTags->nPortIndex = mCameraAdapterParameters.mImagePortIndex;

        eError = OMX_GetConfig(mCameraAdapterParameters.mHandleComp,
                               ( OMX_INDEXTYPE ) OMX_TI_IndexConfigExifTags,
                               &sharedBuffer );
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while retrieving EXIF configuration structure 0x%x", eError);
            ret = -1;
            }
        }

    if ( NO_ERROR == ret )
        {
        if ( ( OMX_TI_TagReadWrite == exifTags->eStatusModel ) &&
              ( mEXIFData.mModelValid ) )
            {
            strncpy(( char * ) sharedPtr,
                    mEXIFData.mModel,
                    EXIF_MODEL_SIZE - 1);

            exifTags->pModelBuff = ( OMX_S8 * ) ( sharedPtr - startPtr );
            exifTags->ulModelBuffSizeBytes = strlen((char*)sharedPtr) + 1;
            sharedPtr += EXIF_MODEL_SIZE;
            exifTags->eStatusModel = OMX_TI_TagUpdated;
            }

         if ( ( OMX_TI_TagReadWrite == exifTags->eStatusMake) &&
               ( mEXIFData.mMakeValid ) )
             {
             strncpy( ( char * ) sharedPtr,
                      mEXIFData.mMake,
                      EXIF_MAKE_SIZE - 1);

             exifTags->pMakeBuff = ( OMX_S8 * ) ( sharedPtr - startPtr );
             exifTags->ulMakeBuffSizeBytes = strlen((char*)sharedPtr) + 1;
             sharedPtr += EXIF_MAKE_SIZE;
             exifTags->eStatusMake = OMX_TI_TagUpdated;
             }

        if ( ( OMX_TI_TagReadWrite == exifTags->eStatusFocalLength ))
        {
            if (mEXIFData.mFocalNum || mEXIFData.mFocalDen ) {
                exifTags->ulFocalLength[0] = (OMX_U32) mEXIFData.mFocalNum;
                exifTags->ulFocalLength[1] = (OMX_U32) mEXIFData.mFocalDen;
                CAMHAL_LOGVB("exifTags->ulFocalLength = [%u] [%u]",
                             (unsigned int)(exifTags->ulFocalLength[0]),
                             (unsigned int)(exifTags->ulFocalLength[1]));
                exifTags->eStatusFocalLength = OMX_TI_TagUpdated;
            }
        }

         if ( OMX_TI_TagReadWrite == exifTags->eStatusDateTime )
             {
             int status = gettimeofday (&sTv, NULL);
             pTime = localtime (&sTv.tv_sec);
             if ( ( 0 == status ) && ( NULL != pTime ) )
                {
                snprintf(( char * ) sharedPtr, EXIF_DATE_TIME_SIZE,
                         "%04d:%02d:%02d %02d:%02d:%02d",
                         pTime->tm_year + 1900,
                         pTime->tm_mon + 1,
                         pTime->tm_mday,
                         pTime->tm_hour,
                         pTime->tm_min,
                         pTime->tm_sec );
                }

             exifTags->pDateTimeBuff = ( OMX_S8 * ) ( sharedPtr - startPtr );
             sharedPtr += EXIF_DATE_TIME_SIZE;
             exifTags->ulDateTimeBuffSizeBytes = EXIF_DATE_TIME_SIZE;
             exifTags->eStatusDateTime = OMX_TI_TagUpdated;
             }

         if ( OMX_TI_TagReadWrite == exifTags->eStatusImageWidth )
             {
             exifTags->ulImageWidth = capData->mWidth;
             exifTags->eStatusImageWidth = OMX_TI_TagUpdated;
             }

         if ( OMX_TI_TagReadWrite == exifTags->eStatusImageHeight )
             {
             exifTags->ulImageHeight = capData->mHeight;
             exifTags->eStatusImageHeight = OMX_TI_TagUpdated;
             }

         if ( ( OMX_TI_TagReadWrite == exifTags->eStatusGpsLatitude ) &&
              ( mEXIFData.mGPSData.mLatValid ) )
            {
            exifTags->ulGpsLatitude[0] = abs(mEXIFData.mGPSData.mLatDeg);
            exifTags->ulGpsLatitude[2] = abs(mEXIFData.mGPSData.mLatMin);
            exifTags->ulGpsLatitude[4] = abs(mEXIFData.mGPSData.mLatSec);
            exifTags->ulGpsLatitude[1] = 1;
            exifTags->ulGpsLatitude[3] = 1;
            exifTags->ulGpsLatitude[5] = abs(mEXIFData.mGPSData.mLatSecDiv);
            exifTags->eStatusGpsLatitude = OMX_TI_TagUpdated;
            }

        if ( ( OMX_TI_TagReadWrite == exifTags->eStatusGpslatitudeRef ) &&
             ( mEXIFData.mGPSData.mLatValid ) )
            {
            exifTags->cGpslatitudeRef[0] = ( OMX_S8 ) mEXIFData.mGPSData.mLatRef[0];
            exifTags->cGpslatitudeRef[1] = '\0';
            exifTags->eStatusGpslatitudeRef = OMX_TI_TagUpdated;
            }

         if ( ( OMX_TI_TagReadWrite == exifTags->eStatusGpsLongitude ) &&
              ( mEXIFData.mGPSData.mLongValid ) )
            {
            exifTags->ulGpsLongitude[0] = abs(mEXIFData.mGPSData.mLongDeg);
            exifTags->ulGpsLongitude[2] = abs(mEXIFData.mGPSData.mLongMin);
            exifTags->ulGpsLongitude[4] = abs(mEXIFData.mGPSData.mLongSec);
            exifTags->ulGpsLongitude[1] = 1;
            exifTags->ulGpsLongitude[3] = 1;
            exifTags->ulGpsLongitude[5] = abs(mEXIFData.mGPSData.mLongSecDiv);
            exifTags->eStatusGpsLongitude = OMX_TI_TagUpdated;
            }

        if ( ( OMX_TI_TagReadWrite == exifTags->eStatusGpsLongitudeRef ) &&
             ( mEXIFData.mGPSData.mLongValid ) )
            {
            exifTags->cGpsLongitudeRef[0] = ( OMX_S8 ) mEXIFData.mGPSData.mLongRef[0];
            exifTags->cGpsLongitudeRef[1] = '\0';
            exifTags->eStatusGpsLongitudeRef = OMX_TI_TagUpdated;
            }

        if ( ( OMX_TI_TagReadWrite == exifTags->eStatusGpsAltitude ) &&
             ( mEXIFData.mGPSData.mAltitudeValid) )
            {
            exifTags->ulGpsAltitude[0] = ( OMX_U32 ) mEXIFData.mGPSData.mAltitude;
            exifTags->ulGpsAltitude[1] = 1;
            exifTags->eStatusGpsAltitude = OMX_TI_TagUpdated;
            }

        if ( ( OMX_TI_TagReadWrite == exifTags->eStatusGpsAltitudeRef ) &&
             ( mEXIFData.mGPSData.mAltitudeValid) )
            {
            exifTags->ucGpsAltitudeRef = (OMX_U8) mEXIFData.mGPSData.mAltitudeRef;
            exifTags->eStatusGpsAltitudeRef = OMX_TI_TagUpdated;
            }

        if ( ( OMX_TI_TagReadWrite == exifTags->eStatusGpsMapDatum ) &&
             ( mEXIFData.mGPSData.mMapDatumValid ) )
            {
            memcpy(sharedPtr, mEXIFData.mGPSData.mMapDatum, GPS_MAPDATUM_SIZE);

            exifTags->pGpsMapDatumBuff = ( OMX_S8 * ) ( sharedPtr - startPtr );
            exifTags->ulGpsMapDatumBuffSizeBytes = GPS_MAPDATUM_SIZE;
            exifTags->eStatusGpsMapDatum = OMX_TI_TagUpdated;
            sharedPtr += GPS_MAPDATUM_SIZE;
            }

        if ( ( OMX_TI_TagReadWrite == exifTags->eStatusGpsProcessingMethod ) &&
             ( mEXIFData.mGPSData.mProcMethodValid ) )
            {
            exifTags->pGpsProcessingMethodBuff = ( OMX_S8 * ) ( sharedPtr - startPtr );
            memcpy(sharedPtr, ExifAsciiPrefix, sizeof(ExifAsciiPrefix));
            sharedPtr += sizeof(ExifAsciiPrefix);

            memcpy(sharedPtr,
                   mEXIFData.mGPSData.mProcMethod,
                   ( GPS_PROCESSING_SIZE - sizeof(ExifAsciiPrefix) ) );
            exifTags->ulGpsProcessingMethodBuffSizeBytes = GPS_PROCESSING_SIZE;
            exifTags->eStatusGpsProcessingMethod = OMX_TI_TagUpdated;
            sharedPtr += GPS_PROCESSING_SIZE;
            }

        if ( ( OMX_TI_TagReadWrite == exifTags->eStatusGpsVersionId ) &&
             ( mEXIFData.mGPSData.mVersionIdValid ) )
            {
            exifTags->ucGpsVersionId[0] = ( OMX_U8 ) mEXIFData.mGPSData.mVersionId[0];
            exifTags->ucGpsVersionId[1] =  ( OMX_U8 ) mEXIFData.mGPSData.mVersionId[1];
            exifTags->ucGpsVersionId[2] = ( OMX_U8 ) mEXIFData.mGPSData.mVersionId[2];
            exifTags->ucGpsVersionId[3] = ( OMX_U8 ) mEXIFData.mGPSData.mVersionId[3];
            exifTags->eStatusGpsVersionId = OMX_TI_TagUpdated;
            }

        if ( ( OMX_TI_TagReadWrite == exifTags->eStatusGpsTimeStamp ) &&
             ( mEXIFData.mGPSData.mTimeStampValid ) )
            {
            exifTags->ulGpsTimeStamp[0] = mEXIFData.mGPSData.mTimeStampHour;
            exifTags->ulGpsTimeStamp[2] = mEXIFData.mGPSData.mTimeStampMin;
            exifTags->ulGpsTimeStamp[4] = mEXIFData.mGPSData.mTimeStampSec;
            exifTags->ulGpsTimeStamp[1] = 1;
            exifTags->ulGpsTimeStamp[3] = 1;
            exifTags->ulGpsTimeStamp[5] = 1;
            exifTags->eStatusGpsTimeStamp = OMX_TI_TagUpdated;
            }

        if ( ( OMX_TI_TagReadWrite == exifTags->eStatusGpsDateStamp ) &&
             ( mEXIFData.mGPSData.mDatestampValid ) )
            {
            strncpy( ( char * ) exifTags->cGpsDateStamp,
                         ( char * ) mEXIFData.mGPSData.mDatestamp,
                         GPS_DATESTAMP_SIZE );
            exifTags->eStatusGpsDateStamp = OMX_TI_TagUpdated;
            }

        eError = OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                               ( OMX_INDEXTYPE ) OMX_TI_IndexConfigExifTags,
                               &sharedBuffer );

        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while setting EXIF configuration 0x%x", eError);
            ret = -1;
            }
        }

    if ( NULL != memmgr_buf_array )
        {
        mMemMgr.freeBufferList(memmgr_buf_array);
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::setupEXIF_libjpeg(ExifElementsTable* exifTable,
                                             OMX_TI_ANCILLARYDATATYPE* pAncillaryData,
                                             OMX_TI_WHITEBALANCERESULTTYPE* pWhiteBalanceData)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    struct timeval sTv;
    struct tm *pTime;
    OMXCameraPortParameters * capData = NULL;

    LOG_FUNCTION_NAME;

    capData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];

    if ((NO_ERROR == ret) && (mEXIFData.mModelValid)) {
        ret = exifTable->insertElement(TAG_MODEL, mEXIFData.mModel);
    }

     if ((NO_ERROR == ret) && (mEXIFData.mMakeValid)) {
        ret = exifTable->insertElement(TAG_MAKE, mEXIFData.mMake);
     }

    if ((NO_ERROR == ret)) {
        if (mEXIFData.mFocalNum || mEXIFData.mFocalDen) {
            char temp_value[256]; // arbitrarily long string
            snprintf(temp_value,
                    sizeof(temp_value)/sizeof(char),
                    "%u/%u",
                    mEXIFData.mFocalNum,
                    mEXIFData.mFocalDen);
            ret = exifTable->insertElement(TAG_FOCALLENGTH, temp_value);

        }
    }

    if ((NO_ERROR == ret)) {
        int status = gettimeofday (&sTv, NULL);
        pTime = localtime (&sTv.tv_sec);
        char temp_value[EXIF_DATE_TIME_SIZE + 1];
        if ((0 == status) && (NULL != pTime)) {
            snprintf(temp_value, EXIF_DATE_TIME_SIZE,
                     "%04d:%02d:%02d %02d:%02d:%02d",
                     pTime->tm_year + 1900,
                     pTime->tm_mon + 1,
                     pTime->tm_mday,
                     pTime->tm_hour,
                     pTime->tm_min,
                     pTime->tm_sec );
            ret = exifTable->insertElement(TAG_DATETIME, temp_value);
        }
     }

    if ((NO_ERROR == ret)) {
        char temp_value[5];
        snprintf(temp_value, sizeof(temp_value)/sizeof(char), "%lu", capData->mWidth);
        ret = exifTable->insertElement(TAG_IMAGE_WIDTH, temp_value);
     }

    if ((NO_ERROR == ret)) {
        char temp_value[5];
        snprintf(temp_value, sizeof(temp_value)/sizeof(char), "%lu", capData->mHeight);
        ret = exifTable->insertElement(TAG_IMAGE_LENGTH, temp_value);
     }

    if ((NO_ERROR == ret) && (mEXIFData.mGPSData.mLatValid)) {
        char temp_value[256]; // arbitrarily long string
        snprintf(temp_value,
                 sizeof(temp_value)/sizeof(char) - 1,
                 "%d/%d,%d/%d,%d/%d",
                 abs(mEXIFData.mGPSData.mLatDeg), 1,
                 abs(mEXIFData.mGPSData.mLatMin), 1,
                 abs(mEXIFData.mGPSData.mLatSec), abs(mEXIFData.mGPSData.mLatSecDiv));
        ret = exifTable->insertElement(TAG_GPS_LAT, temp_value);
    }

    if ((NO_ERROR == ret) && (mEXIFData.mGPSData.mLatValid)) {
        ret = exifTable->insertElement(TAG_GPS_LAT_REF, mEXIFData.mGPSData.mLatRef);
    }

    if ((NO_ERROR == ret) && (mEXIFData.mGPSData.mLongValid)) {
        char temp_value[256]; // arbitrarily long string
        snprintf(temp_value,
                 sizeof(temp_value)/sizeof(char) - 1,
                 "%d/%d,%d/%d,%d/%d",
                 abs(mEXIFData.mGPSData.mLongDeg), 1,
                 abs(mEXIFData.mGPSData.mLongMin), 1,
                 abs(mEXIFData.mGPSData.mLongSec), abs(mEXIFData.mGPSData.mLongSecDiv));
        ret = exifTable->insertElement(TAG_GPS_LONG, temp_value);
    }

    if ((NO_ERROR == ret) && (mEXIFData.mGPSData.mLongValid)) {
        ret = exifTable->insertElement(TAG_GPS_LONG_REF, mEXIFData.mGPSData.mLongRef);
    }

    if ((NO_ERROR == ret) && (mEXIFData.mGPSData.mAltitudeValid)) {
        char temp_value[256]; // arbitrarily long string
        snprintf(temp_value,
                 sizeof(temp_value)/sizeof(char) - 1,
                 "%d/%d",
                 abs( mEXIFData.mGPSData.mAltitude), 1);
        ret = exifTable->insertElement(TAG_GPS_ALT, temp_value);
    }

    if ((NO_ERROR == ret) && (mEXIFData.mGPSData.mAltitudeValid)) {
        char temp_value[5];
        snprintf(temp_value,
                 sizeof(temp_value)/sizeof(char) - 1,
                 "%d", mEXIFData.mGPSData.mAltitudeRef);
        ret = exifTable->insertElement(TAG_GPS_ALT_REF, temp_value);
    }

    if ((NO_ERROR == ret) && (mEXIFData.mGPSData.mMapDatumValid)) {
        ret = exifTable->insertElement(TAG_GPS_MAP_DATUM, mEXIFData.mGPSData.mMapDatum);
    }

    if ((NO_ERROR == ret) && (mEXIFData.mGPSData.mProcMethodValid)) {
        char temp_value[GPS_PROCESSING_SIZE];

        memcpy(temp_value, ExifAsciiPrefix, sizeof(ExifAsciiPrefix));
        memcpy(temp_value + sizeof(ExifAsciiPrefix),
                mEXIFData.mGPSData.mProcMethod,
                (GPS_PROCESSING_SIZE - sizeof(ExifAsciiPrefix)));
        ret = exifTable->insertElement(TAG_GPS_PROCESSING_METHOD, temp_value);
    }

    if ((NO_ERROR == ret) && (mEXIFData.mGPSData.mVersionIdValid)) {
        char temp_value[256]; // arbitrarily long string
        snprintf(temp_value,
                 sizeof(temp_value)/sizeof(char) - 1,
                 "%d,%d,%d,%d",
                 mEXIFData.mGPSData.mVersionId[0],
                 mEXIFData.mGPSData.mVersionId[1],
                 mEXIFData.mGPSData.mVersionId[2],
                 mEXIFData.mGPSData.mVersionId[3]);
        ret = exifTable->insertElement(TAG_GPS_VERSION_ID, temp_value);
    }

    if ((NO_ERROR == ret) && (mEXIFData.mGPSData.mTimeStampValid)) {
        char temp_value[256]; // arbitrarily long string
        snprintf(temp_value,
                 sizeof(temp_value)/sizeof(char) - 1,
                 "%d/%d,%d/%d,%d/%d",
                 mEXIFData.mGPSData.mTimeStampHour, 1,
                 mEXIFData.mGPSData.mTimeStampMin, 1,
                 mEXIFData.mGPSData.mTimeStampSec, 1);
        ret = exifTable->insertElement(TAG_GPS_TIMESTAMP, temp_value);
    }

    if ((NO_ERROR == ret) && (mEXIFData.mGPSData.mDatestampValid) ) {
        ret = exifTable->insertElement(TAG_GPS_DATESTAMP, mEXIFData.mGPSData.mDatestamp);
    }

    if (NO_ERROR == ret) {
        const char* exif_orient =
                ExifElementsTable::degreesToExifOrientation(mPictureRotation);

        if (exif_orient) {
           ret = exifTable->insertElement(TAG_ORIENTATION, exif_orient);
        }
    }

    // fill in short and ushort tags
    if (NO_ERROR == ret) {
        char temp_value[2];
        temp_value[1] = '\0';

        // AWB
        if (mParameters3A.WhiteBallance == OMX_WhiteBalControlAuto) {
            temp_value[0] = '0';
        } else {
            temp_value[0] = '1';
        }
        exifTable->insertElement(TAG_WHITEBALANCE, temp_value);

        // MeteringMode
        // TODO(XXX): only supporting this metering mode at the moment, may change in future
        temp_value[0] = '2';
        exifTable->insertElement(TAG_METERING_MODE, temp_value);

        // ExposureProgram
        // TODO(XXX): only supporting this exposure program at the moment, may change in future
        temp_value[0] = '3';
        exifTable->insertElement(TAG_EXPOSURE_PROGRAM, temp_value);

        // ColorSpace
        temp_value[0] = '1';
        exifTable->insertElement(TAG_COLOR_SPACE, temp_value);

        temp_value[0] = '2';
        exifTable->insertElement(TAG_SENSING_METHOD, temp_value);

        temp_value[0] = '1';
        exifTable->insertElement(TAG_CUSTOM_RENDERED, temp_value);
    }

    if (pAncillaryData && (NO_ERROR == ret)) {
        unsigned int numerator = 0, denominator = 0;
        char temp_value[256];
        unsigned int temp_num = 0;

        // DigitalZoomRatio
        snprintf(temp_value,
                 sizeof(temp_value)/sizeof(char),
                 "%u/%u",
                 pAncillaryData->nDigitalZoomFactor, 1024);
        exifTable->insertElement(TAG_DIGITALZOOMRATIO, temp_value);

        // ExposureTime
        snprintf(temp_value,
                 sizeof(temp_value)/sizeof(char),
                 "%u/%u",
                 pAncillaryData->nExposureTime, 1000000);
        exifTable->insertElement(TAG_EXPOSURETIME, temp_value);

        // ApertureValue and FNumber
        snprintf(temp_value,
                 sizeof(temp_value)/sizeof(char),
                 "%u/%u",
                 pAncillaryData->nApertureValue, 100);
        exifTable->insertElement(TAG_FNUMBER, temp_value);
        exifTable->insertElement(TAG_APERTURE, temp_value);

        // ISO
        snprintf(temp_value,
                 sizeof(temp_value)/sizeof(char),
                 "%u,0,0",
                 pAncillaryData->nCurrentISO);
        exifTable->insertElement(TAG_ISO_EQUIVALENT, temp_value);

        // ShutterSpeed
        snprintf(temp_value,
                 sizeof(temp_value)/sizeof(char),
                 "%f",
                 log(pAncillaryData->nExposureTime) / log(2));
        ExifElementsTable::stringToRational(temp_value, &numerator, &denominator);
        snprintf(temp_value, sizeof(temp_value)/sizeof(char), "%u/%u", numerator, denominator);
        exifTable->insertElement(TAG_SHUTTERSPEED, temp_value);

        // Flash
        if (mParameters3A.FlashMode == OMX_IMAGE_FlashControlAuto) {
            if(pAncillaryData->nFlashStatus) temp_num = 0x19; // Flash fired, auto mode
            else temp_num = 0x18; // Flash did not fire, auto mode
        } else if (mParameters3A.FlashMode == OMX_IMAGE_FlashControlOn) {
            if(pAncillaryData->nFlashStatus) temp_num = 0x9; // Flash fired, compulsory flash mode
            else temp_num = 0x10; // Flash did not fire, compulsory flash mode
        } else if(pAncillaryData->nFlashStatus) {
            temp_num = 0x1; // Flash fired
        } else {
            temp_num = 0x0; // Flash did not fire
        }
        snprintf(temp_value,
                 sizeof(temp_value)/sizeof(char),
                 "%u", temp_num);
        exifTable->insertElement(TAG_FLASH, temp_value);

        if (pWhiteBalanceData) {
            unsigned int lightsource = 0;
            unsigned int colourtemp = pWhiteBalanceData->nColorTemperature;
            bool flash_fired = (temp_num & 0x1); // value from flash above

            // stole this from framework/tools_library/src/tools_sys_exif_tags.c
            if( colourtemp <= 3200 ) {
                lightsource = 3; // Tungsten
            } else if( colourtemp > 3200 && colourtemp <= 4800 ) {
                lightsource = 2; // Fluorescent
            } else if( colourtemp > 4800 && colourtemp <= 5500 ) {
                lightsource = 1; // Daylight
            } else if( colourtemp > 5500 && colourtemp <= 6500 ) {
                lightsource = 9; // Fine weather
            } else if( colourtemp > 6500 ) {
                lightsource = 10; // Cloudy weather
            }

            if(flash_fired) {
                lightsource = 4; // Flash
            }

            snprintf(temp_value,
                    sizeof(temp_value)/sizeof(char),
                    "%u", lightsource);
            exifTable->insertElement(TAG_LIGHT_SOURCE, temp_value);
        }
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::convertGPSCoord(double coord,
                                           int &deg,
                                           int &min,
                                           int &sec,
                                           int &secDivisor)
{
    double tmp;

    LOG_FUNCTION_NAME;

    if ( coord == 0 ) {

        CAMHAL_LOGE("Invalid GPS coordinate");

        return -EINVAL;
    }

    deg = (int) floor(fabs(coord));
    tmp = ( fabs(coord) - floor(fabs(coord)) ) * GPS_MIN_DIV;
    min = (int) floor(tmp);
    tmp = ( tmp - floor(tmp) ) * ( GPS_SEC_DIV * GPS_SEC_ACCURACY );
    sec = (int) floor(tmp);
    secDivisor = GPS_SEC_ACCURACY;

    if( sec >= ( GPS_SEC_DIV * GPS_SEC_ACCURACY ) ) {
        sec = 0;
        min += 1;
    }

    if( min >= 60 ) {
        min = 0;
        deg += 1;
    }

    LOG_FUNCTION_NAME_EXIT;

    return NO_ERROR;
}

} // namespace Camera
} // namespace Ti
