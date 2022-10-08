/* Copyright (c) 2012-2013, The Linux Foundataion. All rights reserved.
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

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <semaphore.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <ui/DisplayInfo.h>
#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/ISurfaceComposer.h>

#include <system/camera.h>
#include <camera/Camera.h>
#include <camera/ICamera.h>
#include <camera/CameraParameters.h>

#include <utils/RefBase.h>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <cutils/properties.h>
#include <cutils/memory.h>

#include "qcamera_test.h"

namespace qcamera {

using namespace android;

int CameraContext::JpegIdx = 0;

/*===========================================================================
 * FUNCTION   : previewCallback
 *
 * DESCRIPTION: preview callback preview mesages are enabled
 *
 * PARAMETERS :
 *   @mem : preview buffer
 *
 * RETURN     : None
 *==========================================================================*/
void CameraContext::previewCallback(const sp<IMemory>& mem)
{
    printf("PREVIEW Callback 0x%x", ( unsigned int ) mem->pointer());
    uint8_t *ptr = (uint8_t*) mem->pointer();
    printf("PRV_CB: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x",
           ptr[0],
           ptr[1],
           ptr[2],
           ptr[3],
           ptr[4],
           ptr[5],
           ptr[6],
           ptr[7],
           ptr[8],
           ptr[9]);
}

/*===========================================================================
 * FUNCTION   : saveFile
 *
 * DESCRIPTION: helper function for saving buffers on filesystem
 *
 * PARAMETERS :
 *   @mem : buffer to save to filesystem
 *   @path: File path
 *
 * RETURN     : status_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
status_t CameraContext::saveFile(const sp<IMemory>& mem, String8 path)
{
    unsigned char *buff = NULL;
    int size;
    int fd = -1;

    if (mem == NULL) {
        return BAD_VALUE;
    }

    fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0655);
    if(fd < 0) {
        printf("Unable to open file %s %s\n", path.string(), strerror(fd));
        return -errno;
    }

    size = mem->size();
    if (size <= 0) {
        printf("IMemory object is of zero size\n");
        close(fd);
        return BAD_VALUE;
    }

    buff = (unsigned char *)mem->pointer();
    if (!buff) {
        printf("Buffer pointer is invalid\n");
        close(fd);
        return BAD_VALUE;
    }

    if ( size != write(fd, buff, size) ) {
        printf("Bad Write error (%d)%s\n",
               errno,
               strerror(errno));
        close(fd);
        return INVALID_OPERATION;
    }

    printf("%s: buffer=%08X, size=%d stored at %s\n",
           __FUNCTION__, (int)buff, size, path.string());

    if (fd >= 0)
        close(fd);

    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : notify
 *
 * DESCRIPTION: notify callback
 *
 * PARAMETERS :
 *   @msgType : type of callback
 *   @ext1: extended parameters
 *   @ext2: extended parameters
 *
 * RETURN     : None
 *==========================================================================*/
void CameraContext::notify(int32_t msgType, int32_t ext1, int32_t ext2)
{
    printf("Notify cb: %d %d %d\n", msgType, ext1, ext2);

    if ( msgType & CAMERA_MSG_FOCUS ) {
        printf("AutoFocus %s \n",
               (ext1) ? "OK" : "FAIL");
    }

    if ( msgType & CAMERA_MSG_SHUTTER ) {
        printf("Shutter done \n");
    }

    if ( msgType & CAMERA_MSG_ERROR) {
        printf("Camera Test CAMERA_MSG_ERROR\n");
        stopPreview();
        closeCamera();
    }
}

/*===========================================================================
 * FUNCTION   : postData
 *
 * DESCRIPTION: handles data callbacks
 *
 * PARAMETERS :
 *   @msgType : type of callback
 *   @dataPtr: buffer data
 *   @metadata: additional metadata where available
 *
 * RETURN     : None
 *==========================================================================*/
void CameraContext::postData(int32_t msgType,
                             const sp<IMemory>& dataPtr,
                             camera_frame_metadata_t *metadata)
{
    printf("Data cb: %d\n", msgType);

    if ( msgType & CAMERA_MSG_PREVIEW_FRAME ) {
        previewCallback(dataPtr);
    }

    if ( msgType & CAMERA_MSG_RAW_IMAGE ) {
        printf("RAW done \n");
    }

    if (msgType & CAMERA_MSG_POSTVIEW_FRAME) {
        printf("Postview frame \n");
    }

    if (msgType & CAMERA_MSG_COMPRESSED_IMAGE ) {
        printf("JPEG done\n");
        String8 jpegPath;
        jpegPath = jpegPath.format("/sdcard/img_%d.jpg", JpegIdx);
        saveFile(dataPtr, jpegPath);
        JpegIdx++;
    }

    if ( ( msgType & CAMERA_MSG_PREVIEW_METADATA ) &&
         ( NULL != metadata ) ) {
        printf("Face detected %d \n", metadata->number_of_faces);
    }
}

/*===========================================================================
 * FUNCTION   : postDataTimestamp
 *
 * DESCRIPTION: handles recording callbacks
 *
 * PARAMETERS :
 *   @timestamp : timestamp of buffer
 *   @msgType : type of buffer
 *   @dataPtr : buffer data
 *
 * RETURN     : None
 *==========================================================================*/
void CameraContext::postDataTimestamp(nsecs_t timestamp,
                                      int32_t msgType,
                                      const sp<IMemory>& dataPtr)
{
    printf("Recording cb: %d %lld %p\n", msgType, timestamp, dataPtr.get());
}

/*===========================================================================
 * FUNCTION   : printSupportedParams
 *
 * DESCRIPTION: dump common supported parameters
 *
 * PARAMETERS : None
 *
 * RETURN     : None
 *==========================================================================*/
void CameraContext::printSupportedParams()
{
    printf("\n\r\tSupported Cameras: %s",
           mParams.get("camera-indexes")? mParams.get("camera-indexes") : "NULL");
    printf("\n\r\tSupported Picture Sizes: %s",
           mParams.get(CameraParameters::KEY_SUPPORTED_PICTURE_SIZES)?
           mParams.get(CameraParameters::KEY_SUPPORTED_PICTURE_SIZES) : "NULL");
    printf("\n\r\tSupported Picture Formats: %s",
           mParams.get(CameraParameters::KEY_SUPPORTED_PICTURE_FORMATS)?
           mParams.get(CameraParameters::KEY_SUPPORTED_PICTURE_FORMATS) : "NULL");
    printf("\n\r\tSupported Preview Sizes: %s",
           mParams.get(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES)?
           mParams.get(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES) : "NULL");
    printf("\n\r\tSupported Preview Formats: %s",
           mParams.get(CameraParameters::KEY_SUPPORTED_PREVIEW_FORMATS)?
           mParams.get(CameraParameters::KEY_SUPPORTED_PREVIEW_FORMATS) : "NULL");
    printf("\n\r\tSupported Preview Frame Rates: %s",
           mParams.get(CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES)?
           mParams.get(CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES) : "NULL");
    printf("\n\r\tSupported Thumbnail Sizes: %s",
           mParams.get(CameraParameters::KEY_SUPPORTED_JPEG_THUMBNAIL_SIZES)?
           mParams.get(CameraParameters::KEY_SUPPORTED_JPEG_THUMBNAIL_SIZES) : "NULL");
    printf("\n\r\tSupported Whitebalance Modes: %s",
           mParams.get(CameraParameters::KEY_SUPPORTED_WHITE_BALANCE)?
           mParams.get(CameraParameters::KEY_SUPPORTED_WHITE_BALANCE) : "NULL");
    printf("\n\r\tSupported Effects: %s",
           mParams.get(CameraParameters::KEY_SUPPORTED_EFFECTS)?
           mParams.get(CameraParameters::KEY_SUPPORTED_EFFECTS) : "NULL");
    printf("\n\r\tSupported Scene Modes: %s",
           mParams.get(CameraParameters::KEY_SUPPORTED_SCENE_MODES)?
           mParams.get(CameraParameters::KEY_SUPPORTED_SCENE_MODES) : "NULL");
    printf("\n\r\tSupported Focus Modes: %s",
           mParams.get(CameraParameters::KEY_SUPPORTED_FOCUS_MODES)?
           mParams.get(CameraParameters::KEY_SUPPORTED_FOCUS_MODES) : "NULL");
    printf("\n\r\tSupported Antibanding Options: %s",
           mParams.get(CameraParameters::KEY_SUPPORTED_ANTIBANDING)?
           mParams.get(CameraParameters::KEY_SUPPORTED_ANTIBANDING) : "NULL");
    printf("\n\r\tSupported Flash Modes: %s",
           mParams.get(CameraParameters::KEY_SUPPORTED_FLASH_MODES)?
           mParams.get(CameraParameters::KEY_SUPPORTED_FLASH_MODES) : "NULL");
    printf("\n\r\tSupported Focus Areas: %d",
           mParams.getInt(CameraParameters::KEY_MAX_NUM_FOCUS_AREAS));
    printf("\n\r\tSupported FPS ranges : %s",
           mParams.get(CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE)?
           mParams.get(CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE) : "NULL");
    printf("\n\r\tFocus Distances: %s \n",
           mParams.get(CameraParameters::KEY_FOCUS_DISTANCES)?
           mParams.get(CameraParameters::KEY_FOCUS_DISTANCES) : "NULL");
}

/*===========================================================================
 * FUNCTION   : createPreviewSurface
 *
 * DESCRIPTION: helper function for creating preview surfaces
 *
 * PARAMETERS :
 *   @width : preview width
 *   @height: preview height
 *   @pixFormat : surface pixelformat
 *
 * RETURN     : status_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
status_t CameraContext::createPreviewSurface(unsigned int width,
                                             unsigned int height,
                                             int32_t pixFormat)
{
    int ret = NO_ERROR;
    DisplayInfo dinfo;
    sp<IBinder> display(SurfaceComposerClient::getBuiltInDisplay(
                        ISurfaceComposer::eDisplayIdMain));
    SurfaceComposerClient::getDisplayInfo(display, &dinfo);
    unsigned int previewWidth, previewHeight;

    if ( dinfo.w < width ) {
        previewWidth = dinfo.w;
    } else {
        previewWidth = width;
    }

    if ( dinfo.h < height ) {
        previewHeight = dinfo.h;
    } else {
        previewHeight = height;
    }

    mClient = new SurfaceComposerClient();

    if ( NULL == mClient.get() ) {
        printf("Unable to establish connection to Surface Composer \n");
        return NO_INIT;
    }

    mSurfaceControl = mClient->createSurface(String8("QCamera_Test"),
                                             previewWidth,
                                             previewHeight,
                                             pixFormat,
                                             0);
    if ( NULL == mSurfaceControl.get() ) {
        printf("Unable to create preview surface \n");
        return NO_INIT;
    }

    mPreviewSurface = mSurfaceControl->getSurface();
    if ( NULL != mPreviewSurface.get() ) {
        mClient->openGlobalTransaction();
        ret |= mSurfaceControl->setLayer(0x7fffffff);
        ret |= mSurfaceControl->setPosition(0, 0);
        ret |= mSurfaceControl->setSize(previewWidth, previewHeight);
        ret |= mSurfaceControl->show();
        mClient->closeGlobalTransaction();

        if ( NO_ERROR != ret ) {
            printf("Preview surface configuration failed! \n");
        }
    } else {
        ret = NO_INIT;
    }

    return ret;
}

/*===========================================================================
 * FUNCTION   : destroyPreviewSurface
 *
 * DESCRIPTION: closes previously open preview surface
 *
 * PARAMETERS : None
 *
 * RETURN     : status_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
status_t CameraContext::destroyPreviewSurface()
{
    if ( NULL != mPreviewSurface.get() ) {
        mPreviewSurface.clear();
    }

    if ( NULL != mSurfaceControl.get() ) {
        mSurfaceControl->clear();
        mSurfaceControl.clear();
    }

    if ( NULL != mClient.get() ) {
        mClient->dispose();
        mClient.clear();
    }

    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : ~CameraContext
 *
 * DESCRIPTION: camera context destructor
 *
 * PARAMETERS : None
 *
 * RETURN     : None
 *==========================================================================*/
CameraContext::~CameraContext()
{
    stopPreview();
    closeCamera();
}

/*===========================================================================
 * FUNCTION   : openCamera
 *
 * DESCRIPTION: connects to and initializes camera
 *
 * PARAMETERS : None
 *
 * RETURN     : status_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
status_t  CameraContext::openCamera()
{
    if ( NULL != mCamera.get() ) {
        printf("Camera already open! \n");
        return NO_ERROR;
    }

    printf("openCamera(camera_index=%d)\n", mCameraIndex);
    mCamera = Camera::connect(mCameraIndex);

    if ( NULL == mCamera.get() ) {
        printf("Unable to connect to CameraService\n");
        return NO_INIT;
    }

    mParams = mCamera->getParameters();
    mParams.getSupportedPreviewSizes(mSupportedPreviewSizes);
    mParams.getSupportedPictureSizes(mSupportedPictureSizes);
    mCurrentPictureSizeIdx = mSupportedPictureSizes.size() / 2;
    mCurrentPreviewSizeIdx = mSupportedPreviewSizes.size() / 2;

    mCamera->setListener(this);
    mHardwareActive = true;

    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : getNumberOfCameras
 *
 * DESCRIPTION: returns the number of supported camera by the system
 *
 * PARAMETERS : None
 *
 * RETURN     : supported camera count
 *==========================================================================*/
int CameraContext::getNumberOfCameras()
{
    int ret = -1;

    if ( NULL != mCamera.get() ) {
        ret = mCamera->getNumberOfCameras();
    }

    return ret;
}

/*===========================================================================
 * FUNCTION   : closeCamera
 *
 * DESCRIPTION: closes a previously the initialized camera reference
 *
 * PARAMETERS : None
 *
 * RETURN     : status_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
status_t CameraContext::closeCamera()
{
    if ( NULL == mCamera.get() ) {
        return NO_INIT;
    }

    mCamera->disconnect();
    mCamera.clear();
    mHardwareActive = false;
    mPreviewRunning = false;

    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : startPreview
 *
 * DESCRIPTION: starts camera preview
 *
 * PARAMETERS : None
 *
 * RETURN     : status_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
status_t CameraContext::startPreview()
{
    int ret = NO_ERROR;
    int previewWidth, previewHeight;
    Size currentPreviewSize = mSupportedPreviewSizes.itemAt(mCurrentPreviewSizeIdx);
    Size currentPictureSize = mSupportedPictureSizes.itemAt(mCurrentPictureSizeIdx);

    if ( mPreviewRunning || !mHardwareActive ) {
        printf("Preview already running or camera not active! \n");
        return NO_INIT;
    }

    if (mResizePreview) {
        previewWidth = currentPreviewSize.width;
        previewHeight = currentPreviewSize.height;

        ret = createPreviewSurface(previewWidth,
                                   previewHeight,
                                   HAL_PIXEL_FORMAT_YCrCb_420_SP);
        if (  NO_ERROR != ret ) {
            printf("Error while creating preview surface\n");
            return ret;
        }

        mParams.setPreviewSize(previewWidth, previewHeight);
        mParams.setPictureSize(currentPictureSize.width, currentPictureSize.height);

        ret |= mCamera->setParameters(mParams.flatten());
        ret |= mCamera->setPreviewDisplay(mPreviewSurface);
        ret |= mCamera->startPreview();
        if ( NO_ERROR != ret ) {
            printf("Preview start failed! \n");
            return ret;
        }

        mPreviewRunning = true;
        mResizePreview = false;
    }

    return ret;
}

/*===========================================================================
 * FUNCTION   : autoFocus
 *
 * DESCRIPTION: Triggers autofocus
 *
 * PARAMETERS : None
 *
 * RETURN     : status_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
status_t CameraContext::autoFocus()
{
    status_t ret = NO_ERROR;

    if ( mPreviewRunning ) {
        ret = mCamera->autoFocus();
    }

    return ret;
}

/*===========================================================================
 * FUNCTION   : enablePreviewCallbacks
 *
 * DESCRIPTION: Enables preview callback messages
 *
 * PARAMETERS : None
 *
 * RETURN     : status_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
status_t CameraContext::enablePreviewCallbacks()
{
    if ( mHardwareActive ) {
        mCamera->setPreviewCallbackFlags(CAMERA_FRAME_CALLBACK_FLAG_ENABLE_MASK);
    }

    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : takePicture
 *
 * DESCRIPTION: triggers image capture
 *
 * PARAMETERS : None
 *
 * RETURN     : status_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
status_t CameraContext::takePicture()
{
    status_t ret = NO_ERROR;

    if ( mPreviewRunning ) {
        ret = mCamera->takePicture(CAMERA_MSG_COMPRESSED_IMAGE|CAMERA_MSG_RAW_IMAGE);
    }

    return ret;
}

/*===========================================================================
 * FUNCTION   : stopPreview
 *
 * DESCRIPTION: stops camera preview
 *
 * PARAMETERS : None
 *
 * RETURN     : status_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
status_t CameraContext::stopPreview()
{
    status_t ret = NO_ERROR;

    if ( mHardwareActive ) {
        mCamera->stopPreview();
        ret = destroyPreviewSurface();
    }

    mPreviewRunning  = false;
    mResizePreview = true;

    return ret;
}

/*===========================================================================
 * FUNCTION   : resumePreview
 *
 * DESCRIPTION: resumes camera preview after image capture
 *
 * PARAMETERS : None
 *
 * RETURN     : status_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
status_t CameraContext::resumePreview()
{
    status_t ret = NO_ERROR;

    if ( mHardwareActive ) {
        ret = mCamera->startPreview();
    } else {
        ret = NO_INIT;
    }

    return ret;
}

/*===========================================================================
 * FUNCTION   : nextPreviewSize
 *
 * DESCRIPTION: Iterates through all supported preview sizes.
 *
 * PARAMETERS : None
 *
 * RETURN     : status_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
status_t CameraContext::nextPreviewSize()
{
    if ( mHardwareActive ) {
        mCurrentPreviewSizeIdx += 1;
        mCurrentPreviewSizeIdx %= mSupportedPreviewSizes.size();
        Size previewSize = mSupportedPreviewSizes.itemAt(mCurrentPreviewSizeIdx);
        mParams.setPreviewSize(previewSize.width,
                               previewSize.height);
        mResizePreview = true;

        if ( mPreviewRunning ) {
            mCamera->stopPreview();
            mCamera->setParameters(mParams.flatten());
            mCamera->startPreview();
        } else {
            mCamera->setParameters(mParams.flatten());
        }
    }

    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : getCurrentPreviewSize
 *
 * DESCRIPTION: queries the currently configured preview size
 *
 * PARAMETERS :
 *  @previewSize : preview size currently configured
 *
 * RETURN     : status_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
status_t CameraContext::getCurrentPreviewSize(Size &previewSize)
{
    if ( mHardwareActive ) {
        previewSize = mSupportedPreviewSizes.itemAt(mCurrentPreviewSizeIdx);
    }

    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : nextPictureSize
 *
 * DESCRIPTION: Iterates through all supported picture sizes.
 *
 * PARAMETERS : None
 *
 * RETURN     : status_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
status_t CameraContext::nextPictureSize()
{
    if ( mHardwareActive ) {
        mCurrentPictureSizeIdx += 1;
        mCurrentPictureSizeIdx %= mSupportedPictureSizes.size();
        Size pictureSize = mSupportedPictureSizes.itemAt(mCurrentPictureSizeIdx);
        mParams.setPictureSize(pictureSize.width,
                               pictureSize.height);
        mCamera->setParameters(mParams.flatten());
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : getCurrentPictureSize
 *
 * DESCRIPTION: queries the currently configured picture size
 *
 * PARAMETERS :
 *  @pictureSize : picture size currently configured
 *
 * RETURN     : status_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
status_t CameraContext::getCurrentPictureSize(Size &pictureSize)
{
    if ( mHardwareActive ) {
        pictureSize = mSupportedPictureSizes.itemAt(mCurrentPictureSizeIdx);
    }

    return NO_ERROR;
}

}; //namespace qcamera ends here

using namespace qcamera;

/*===========================================================================
 * FUNCTION   : printMenu
 *
 * DESCRIPTION: prints the available camera options
 *
 * PARAMETERS :
 *  @currentCamera : camera context currently being used
 *
 * RETURN     : None
 *==========================================================================*/
void printMenu(sp<CameraContext> currentCamera)
{
    Size currentPictureSize, currentPreviewSize;

    assert(currentCamera.get());

    currentCamera->getCurrentPictureSize(currentPictureSize);
    currentCamera->getCurrentPreviewSize(currentPreviewSize);

    printf("\n\n=========== FUNCTIONAL TEST MENU ===================\n\n");

    printf(" \n\nSTART / STOP / GENERAL SERVICES \n");
    printf(" -----------------------------\n");
    printf("   %c. Switch camera - Current Index: %d\n",
            SWITCH_CAMERA_CMD,
            currentCamera->getCameraIndex());
    printf("   %c. Resume Preview after capture \n",
            RESUME_PREVIEW_CMD);
    printf("   %c. Quit \n",
            EXIT_CMD);
    printf("   %c. Camera Capability Dump",
            DUMP_CAPS_CMD);

    printf(" \n\n PREVIEW SUB MENU \n");
    printf(" -----------------------------\n");
    printf("   %c. Start Preview\n",
            START_PREVIEW_CMD);
    printf("   %c. Stop Preview\n",
            STOP_PREVIEW_CMD);
    printf("   %c. Preview size:  %dx%d\n",
           CHANGE_PREVIEW_SIZE_CMD,
           currentPreviewSize.width,
           currentPreviewSize.height);
    printf("   %c. Enable preview frames\n",
            ENABLE_PRV_CALLBACKS_CMD);
    printf("   %c. Trigger autofocus \n",
            AUTOFOCUS_CMD);

    printf(" \n\n IMAGE CAPTURE SUB MENU \n");
    printf(" -----------------------------\n");
    printf("   %c. Take picture/Full Press\n",
            TAKEPICTURE_CMD);
    printf("   %c. Picture size:  %dx%d\n",
           CHANGE_PICTURE_SIZE_CMD,
           currentPictureSize.width,
           currentPictureSize.height);

    printf("\n");
    printf("   Choice: ");
}

/*===========================================================================
 * FUNCTION   : functionalTest
 *
 * DESCRIPTION: queries and executes client supplied commands for testing a
 *              particular camera.
 *
 * PARAMETERS :
 *  @availableCameras : List with all cameras supported
 *
 * RETURN     : status_t type of status
 *              NO_ERROR  -- continue testing
 *              none-zero -- quit test
 *==========================================================================*/
status_t functionalTest(Vector< sp<CameraContext> > &availableCameras)
{
    int cmd;
    status_t stat = NO_ERROR;
    static int currentCameraIdx = 0;

    assert(availableCameras.size());

    sp<CameraContext> currentCamera = availableCameras.itemAt(currentCameraIdx);
    printMenu(currentCamera);
    cmd = getchar();

    switch (cmd) {
    case SWITCH_CAMERA_CMD:
        {
            currentCameraIdx++;
            currentCameraIdx %= availableCameras.size();
            currentCamera = availableCameras.itemAt(currentCameraIdx);
        }
        break;

    case RESUME_PREVIEW_CMD:
        {
            stat = currentCamera->resumePreview();
        }
        break;

    case START_PREVIEW_CMD:
        {
            stat = currentCamera->startPreview();
        }
        break;

    case STOP_PREVIEW_CMD:
        {
            stat = currentCamera->stopPreview();
        }
        break;

    case CHANGE_PREVIEW_SIZE_CMD:
        {
            stat = currentCamera->nextPreviewSize();
        }
        break;

    case CHANGE_PICTURE_SIZE_CMD:
        {
            stat = currentCamera->nextPictureSize();
        }
        break;

    case DUMP_CAPS_CMD:
        {
            currentCamera->printSupportedParams();
        }
        break;

    case AUTOFOCUS_CMD:
        {
            stat = currentCamera->autoFocus();
        }
        break;

    case TAKEPICTURE_CMD:
        {
            stat = currentCamera->takePicture();
        }
        break;

    case ENABLE_PRV_CALLBACKS_CMD:
        {
            stat = currentCamera->enablePreviewCallbacks();
        }
        break;

    case EXIT_CMD:
        {
            currentCamera->stopPreview();
            return -1;
        }

        break;

    default:
        {
        }
        break;
    }
    printf("Command status 0x%x \n", stat);

    return NO_ERROR;
}

int main()
{
    sp<ProcessState> proc(ProcessState::self());
    Vector< sp<CameraContext> > availableCameras;
    sp<CameraContext> camera;
    int i = 0;

    ProcessState::self()->startThreadPool();

    do {
        camera = new CameraContext(i);
        if ( NULL == camera.get() ) {
            return NO_INIT;
        }

        status_t stat = camera->openCamera();
        if ( NO_ERROR != stat ) {
            printf("Error encountered during camera open \n");
            return stat;
        }

        availableCameras.add(camera);
        i++;
    } while ( i < camera->getNumberOfCameras() ) ;

    while ( true ) {
        if ( NO_ERROR != functionalTest(availableCameras) ) {
            break;
        }
    };

    for ( size_t j = 0 ; j < availableCameras.size() ; j++ ) {
        camera = availableCameras.itemAt(j);
        camera->closeCamera();
        camera.clear();
    }

    availableCameras.clear();

    return 0;
}
