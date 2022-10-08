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

#ifndef QCAMERA_TEST_H
#define QCAMERA_TEST_H

namespace qcamera {

using namespace android;

typedef enum qcamera_test_cmds_t {
    SWITCH_CAMERA_CMD = 'A',
    RESUME_PREVIEW_CMD = '[',
    START_PREVIEW_CMD = '1',
    STOP_PREVIEW_CMD = '2',
    CHANGE_PREVIEW_SIZE_CMD = '4',
    CHANGE_PICTURE_SIZE_CMD = '5',
    DUMP_CAPS_CMD = 'E',
    AUTOFOCUS_CMD = 'f',
    TAKEPICTURE_CMD = 'p',
    ENABLE_PRV_CALLBACKS_CMD = '&',
    EXIT_CMD = 'q'
} qcamera_test_cmds;

class CameraContext : public CameraListener {
public:

    CameraContext(int cameraIndex) :
        mCameraIndex(cameraIndex),
        mResizePreview(true),
        mHardwareActive(false),
        mPreviewRunning(false),
        mCamera(NULL),
        mClient(NULL),
        mSurfaceControl(NULL),
        mPreviewSurface(NULL) {}

    status_t openCamera();
    status_t closeCamera();

    status_t startPreview();
    status_t stopPreview();
    status_t resumePreview();
    status_t autoFocus();
    status_t enablePreviewCallbacks();
    status_t takePicture();

    status_t nextPreviewSize();
    status_t getCurrentPreviewSize(Size &previewSize);

    status_t nextPictureSize();
    status_t getCurrentPictureSize(Size &pictureSize);

    void printSupportedParams();

    int getCameraIndex() { return mCameraIndex; }
    int getNumberOfCameras();

    virtual ~CameraContext();

    virtual void notify(int32_t msgType, int32_t ext1, int32_t ext2);
    virtual void postData(int32_t msgType,
                          const sp<IMemory>& dataPtr,
                          camera_frame_metadata_t *metadata);

    virtual void postDataTimestamp(nsecs_t timestamp,
                                   int32_t msgType,
                                   const sp<IMemory>& dataPtr);

private:

    status_t createPreviewSurface(unsigned int width,
                                  unsigned int height,
                                  int32_t pixFormat);
    status_t destroyPreviewSurface();

    status_t saveFile(const sp<IMemory>& mem, String8 path);
    void previewCallback(const sp<IMemory>& mem);

    static int JpegIdx;
    int mCameraIndex;
    bool mResizePreview;
    bool mHardwareActive;
    bool mPreviewRunning;

    sp<Camera> mCamera;
    sp<SurfaceComposerClient> mClient;
    sp<SurfaceControl> mSurfaceControl;
    sp<Surface> mPreviewSurface;
    CameraParameters mParams;

    int mCurrentPreviewSizeIdx;
    int mCurrentPictureSizeIdx;
    Vector<Size> mSupportedPreviewSizes;
    Vector<Size> mSupportedPictureSizes;
};

}; //namespace qcamera

#endif
