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
* @file OMXZoom.cpp
*
* This file contains functionality for handling zoom configurations.
*
*/

#undef LOG_TAG

#define LOG_TAG "CameraHAL"

#include "CameraHal.h"
#include "OMXCameraAdapter.h"

namespace android {

const int32_t OMXCameraAdapter::ZOOM_STEPS [ZOOM_STAGES] =  {
                                65536, 68157, 70124, 72745,
                                75366, 77988, 80609, 83231,
                                86508, 89784, 92406, 95683,
                                99615, 102892, 106168, 110100,
                                114033, 117965, 122552, 126484,
                                131072, 135660, 140247, 145490,
                                150733, 155976, 161219, 167117,
                                173015, 178913, 185467, 192020,
                                198574, 205783, 212992, 220201,
                                228065, 236585, 244449, 252969,
                                262144, 271319, 281149, 290980,
                                300810, 311951, 322437, 334234,
                                346030, 357827, 370934, 384041,
                                397148, 411566, 425984, 441057,
                                456131, 472515, 488899, 506593,
                                524288 };


status_t OMXCameraAdapter::setParametersZoom(const CameraParameters &params,
                                             BaseCameraAdapter::AdapterState state)
{
    status_t ret = NO_ERROR;
    Mutex::Autolock lock(mZoomLock);

    LOG_FUNCTION_NAME;

    //Immediate zoom should not be avaialable while smooth zoom is running
    if ( ( ZOOM_ACTIVE & state ) != ZOOM_ACTIVE )
        {
        int zoom = params.getInt(CameraParameters::KEY_ZOOM);
        if( ( zoom >= 0 ) && ( zoom < ZOOM_STAGES ) )
            {
            mTargetZoomIdx = zoom;

            //Immediate zoom should be applied instantly ( CTS requirement )
            mCurrentZoomIdx = mTargetZoomIdx;
            if(!mZoomUpdating) {
                doZoom(mCurrentZoomIdx);
                mZoomUpdating = true;
            } else {
                mZoomUpdate = true;
            }

            CAMHAL_LOGDB("Zoom by App %d", zoom);
            }
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::doZoom(int index)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_SCALEFACTORTYPE zoomControl;

    LOG_FUNCTION_NAME;

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        ret = -1;
        }

    if (  ( 0 > index) || ( ( ZOOM_STAGES - 1 ) < index ) )
        {
        CAMHAL_LOGEB("Zoom index %d out of range", index);
        ret = -EINVAL;
        }

    if (mPreviousZoomIndx == index )
        {
        return NO_ERROR;
        }

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT_PTR (&zoomControl, OMX_CONFIG_SCALEFACTORTYPE);
        zoomControl.nPortIndex = OMX_ALL;
        zoomControl.xHeight = ZOOM_STEPS[index];
        zoomControl.xWidth = ZOOM_STEPS[index];

        eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                                OMX_IndexConfigCommonDigitalZoom,
                                &zoomControl);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while applying digital zoom 0x%x", eError);
            ret = -1;
            }
        else
            {
            CAMHAL_LOGDA("Digital zoom applied successfully");
            mPreviousZoomIndx = index;
            }
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::advanceZoom()
{
    status_t ret = NO_ERROR;
    AdapterState state;
    Mutex::Autolock lock(mZoomLock);

    BaseCameraAdapter::getState(state);

    if ( mReturnZoomStatus )
        {
        mCurrentZoomIdx +=mZoomInc;
        mTargetZoomIdx = mCurrentZoomIdx;
        mReturnZoomStatus = false;
        ret = doZoom(mCurrentZoomIdx);
        notifyZoomSubscribers(mCurrentZoomIdx, true);
        }
    else if ( mCurrentZoomIdx != mTargetZoomIdx )
        {
        if ( ZOOM_ACTIVE & state )
            {
            if ( mCurrentZoomIdx < mTargetZoomIdx )
                {
                mZoomInc = 1;
                }
            else
                {
                mZoomInc = -1;
                }

            mCurrentZoomIdx += mZoomInc;
            }
        else
            {
            mCurrentZoomIdx = mTargetZoomIdx;
            }

        ret = doZoom(mCurrentZoomIdx);

        if ( ZOOM_ACTIVE & state )
            {
            if ( mCurrentZoomIdx == mTargetZoomIdx )
                {
                CAMHAL_LOGDB("[Goal Reached] Smooth Zoom notify currentIdx = %d, targetIdx = %d",
                             mCurrentZoomIdx,
                             mTargetZoomIdx);

                if ( NO_ERROR == ret )
                    {

                    ret =  BaseCameraAdapter::setState(CAMERA_STOP_SMOOTH_ZOOM);

                    if ( NO_ERROR == ret )
                        {
                        ret = BaseCameraAdapter::commitState();
                        }
                    else
                        {
                        ret |= BaseCameraAdapter::rollbackState();
                        }

                    }
                mReturnZoomStatus = false;
                notifyZoomSubscribers(mCurrentZoomIdx, true);
                }
            else
                {
                CAMHAL_LOGDB("[Advancing] Smooth Zoom notify currentIdx = %d, targetIdx = %d",
                             mCurrentZoomIdx,
                             mTargetZoomIdx);
                notifyZoomSubscribers(mCurrentZoomIdx, false);
                }
            }
        }
    else if ( (mCurrentZoomIdx == mTargetZoomIdx ) &&
              ( ZOOM_ACTIVE & state ) )
        {
            ret = BaseCameraAdapter::setState(CameraAdapter::CAMERA_STOP_SMOOTH_ZOOM);

            if ( NO_ERROR == ret )
                {
                ret = BaseCameraAdapter::commitState();
                }
            else
                {
                ret |= BaseCameraAdapter::rollbackState();
                }

        }

    if(mZoomUpdate) {
        doZoom(mTargetZoomIdx);
        mZoomUpdate = false;
        mZoomUpdating = true;
    } else {
        mZoomUpdating = false;
    }

    return ret;
}

status_t OMXCameraAdapter::startSmoothZoom(int targetIdx)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    Mutex::Autolock lock(mZoomLock);

    CAMHAL_LOGDB("Start smooth zoom target = %d, mCurrentIdx = %d",
                 targetIdx,
                 mCurrentZoomIdx);

    if ( ( targetIdx >= 0 ) && ( targetIdx < ZOOM_STAGES ) )
        {
        mTargetZoomIdx = targetIdx;
        mZoomParameterIdx = mCurrentZoomIdx;
        mReturnZoomStatus = false;
        }
    else
        {
        CAMHAL_LOGEB("Smooth value out of range %d!", targetIdx);
        ret = -EINVAL;
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::stopSmoothZoom()
{
    status_t ret = NO_ERROR;
    Mutex::Autolock lock(mZoomLock);

    LOG_FUNCTION_NAME;

    if ( mTargetZoomIdx != mCurrentZoomIdx )
        {
        if ( mCurrentZoomIdx < mTargetZoomIdx )
            {
            mZoomInc = 1;
            }
        else
            {
            mZoomInc = -1;
            }
        mReturnZoomStatus = true;
        mReturnZoomStatus = true;
        CAMHAL_LOGDB("Stop smooth zoom mCurrentZoomIdx = %d, mTargetZoomIdx = %d",
                     mCurrentZoomIdx,
                     mTargetZoomIdx);
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

};
