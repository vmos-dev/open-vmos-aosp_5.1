/*
 * Copyright (C) 2013 The Android Open Source Project
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

package com.android.testingcamera2.v1;

import android.content.Context;
import android.graphics.ImageFormat;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CameraMetadata;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.CaptureRequest.Builder;
import android.util.Size;
import android.media.Image;
import android.media.ImageReader;
import android.media.MediaCodec;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;
import android.util.Size;
import android.view.Surface;
import android.view.SurfaceHolder;

import com.android.ex.camera2.blocking.BlockingCameraManager;
import com.android.ex.camera2.blocking.BlockingCameraManager.BlockingOpenException;
import com.android.ex.camera2.blocking.BlockingStateCallback;
import com.android.ex.camera2.blocking.BlockingSessionCallback;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * A camera controller class that runs in its own thread, to
 * move camera ops off the UI. Generally thread-safe.
 */
public class CameraOps {

    public static interface Listener {
        void onCameraOpened(String cameraId, CameraCharacteristics characteristics);
    }

    private static final String TAG = "CameraOps";
    private static final boolean VERBOSE = Log.isLoggable(TAG, Log.VERBOSE);

    private final HandlerThread mOpsThread;
    private final Handler mOpsHandler;

    private final CameraManager mCameraManager;
    private final BlockingCameraManager mBlockingCameraManager;
    private final BlockingStateCallback mDeviceListener =
            new BlockingStateCallback();

    private CameraDevice mCamera;
    private CameraCaptureSession mSession;

    private ImageReader mCaptureReader;
    private CameraCharacteristics mCameraCharacteristics;

    private int mEncodingBitRate;
    private int mDeviceOrientation;

    private CaptureRequest.Builder mPreviewRequestBuilder;
    private CaptureRequest.Builder mRecordingRequestBuilder;
    List<Surface> mOutputSurfaces = new ArrayList<Surface>(2);
    private Surface mPreviewSurface;
    // How many JPEG buffers do we want to hold on to at once
    private static final int MAX_CONCURRENT_JPEGS = 2;

    private static final int STATUS_ERROR = 0;
    private static final int STATUS_UNINITIALIZED = 1;
    private static final int STATUS_OK = 2;
    // low encoding bitrate(bps), used by small resolution like 640x480.
    private static final int ENC_BIT_RATE_LOW = 2000000;
    // high encoding bitrate(bps), used by large resolution like 1080p.
    private static final int ENC_BIT_RATE_HIGH = 10000000;
    private static final Size DEFAULT_SIZE = new Size(640, 480);
    private static final Size HIGH_RESOLUTION_SIZE = new Size(1920, 1080);

    private static final long IDLE_WAIT_MS = 2000;
    // General short wait timeout for most state transitions
    private static final long STATE_WAIT_MS = 500;

    private int mStatus = STATUS_UNINITIALIZED;

    CameraRecordingStream mRecordingStream;
    private final Listener mListener;
    private final Handler mListenerHandler;

    private void checkOk() {
        if (mStatus < STATUS_OK) {
            throw new IllegalStateException(String.format("Device not OK: %d", mStatus ));
        }
    }

    private CameraOps(Context ctx, Listener listener, Handler handler) throws ApiFailureException {
        mCameraManager = (CameraManager) ctx.getSystemService(Context.CAMERA_SERVICE);
        if (mCameraManager == null) {
            throw new ApiFailureException("Can't connect to camera manager!");
        }
        mBlockingCameraManager = new BlockingCameraManager(mCameraManager);

        mOpsThread = new HandlerThread("CameraOpsThread");
        mOpsThread.start();
        mOpsHandler = new Handler(mOpsThread.getLooper());

        mRecordingStream = new CameraRecordingStream();
        mStatus = STATUS_OK;

        mListener = listener;
        mListenerHandler = handler;
    }

    static public CameraOps create(Context ctx, Listener listener, Handler handler)
            throws ApiFailureException {
        return new CameraOps(ctx, listener, handler);
    }

    public String[] getDevices() throws ApiFailureException{
        checkOk();
        try {
            return mCameraManager.getCameraIdList();
        } catch (CameraAccessException e) {
            throw new ApiFailureException("Can't query device set", e);
        }
    }

    public void registerCameraListener(CameraManager.AvailabilityCallback listener)
            throws ApiFailureException {
        checkOk();
        mCameraManager.registerAvailabilityCallback(listener, mOpsHandler);
    }

    public CameraCharacteristics getCameraCharacteristics() {
        checkOk();
        if (mCameraCharacteristics == null) {
            throw new IllegalStateException("CameraCharacteristics is not available");
        }
        return mCameraCharacteristics;
    }

    public void closeDevice()
            throws ApiFailureException {
        checkOk();
        mCameraCharacteristics = null;

        if (mCamera == null) return;

        try {
            mCamera.close();
        } catch (Exception e) {
            throw new ApiFailureException("can't close device!", e);
        }

        mCamera = null;
        mSession = null;
    }

    private void minimalOpenCamera() throws ApiFailureException {
        if (mCamera == null) {
            final String[] devices;
            final CameraCharacteristics characteristics;

            try {
                devices = mCameraManager.getCameraIdList();
                if (devices == null || devices.length == 0) {
                    throw new ApiFailureException("no devices");
                }
                mCamera = mBlockingCameraManager.openCamera(devices[0],
                        mDeviceListener, mOpsHandler);
                mCameraCharacteristics = mCameraManager.getCameraCharacteristics(mCamera.getId());
                characteristics = mCameraCharacteristics;
            } catch (CameraAccessException e) {
                throw new ApiFailureException("open failure", e);
            } catch (BlockingOpenException e) {
                throw new ApiFailureException("open async failure", e);
            }

            // Dispatch listener event
            if (mListener != null && mListenerHandler != null) {
                mListenerHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        mListener.onCameraOpened(devices[0], characteristics);
                    }
                });
            }
        }

        mStatus = STATUS_OK;
    }

    private void configureOutputs(List<Surface> outputs) throws CameraAccessException {
        BlockingSessionCallback sessionListener = new BlockingSessionCallback();
        mCamera.createCaptureSession(outputs, sessionListener, mOpsHandler);
        mSession = sessionListener.waitAndGetSession(IDLE_WAIT_MS);
    }

    /**
     * Set up SurfaceView dimensions for camera preview
     */
    public void minimalPreviewConfig(SurfaceHolder previewHolder) throws ApiFailureException {

        minimalOpenCamera();
        try {
            CameraCharacteristics properties =
                    mCameraManager.getCameraCharacteristics(mCamera.getId());

            Size[] previewSizes = null;
            Size sz = DEFAULT_SIZE;
            if (properties != null) {
                previewSizes =
                        properties.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP).
                        getOutputSizes(previewHolder.getClass());
            }

            if (previewSizes != null && previewSizes.length != 0 &&
                    Arrays.asList(previewSizes).contains(HIGH_RESOLUTION_SIZE)) {
                sz = HIGH_RESOLUTION_SIZE;
            }
            Log.i(TAG, "Set preview size to " + sz.toString());
            previewHolder.setFixedSize(sz.getWidth(), sz.getHeight());
            mPreviewSurface = previewHolder.getSurface();
        }  catch (CameraAccessException e) {
            throw new ApiFailureException("Error setting up minimal preview", e);
        }
    }


    /**
     * Update current preview with user-specified control inputs.
     */
    public void updatePreview(CameraControls controls) {
        if (VERBOSE) {
            Log.v(TAG, "updatePreview - begin");
        }

        updateCaptureRequest(mPreviewRequestBuilder, controls);

        try {
            // Insert a one-time request if any triggers were set into the request
            if (hasTriggers(mPreviewRequestBuilder)) {
                mSession.capture(mPreviewRequestBuilder.build(), /*listener*/null, /*handler*/null);
                removeTriggers(mPreviewRequestBuilder);

                if (VERBOSE) {
                    Log.v(TAG, "updatePreview - submitted extra one-shot capture with triggers");
                }
            } else {
                if (VERBOSE) {
                    Log.v(TAG, "updatePreview - no triggers, regular repeating request");
                }
            }

            // TODO: add capture result listener
            mSession.setRepeatingRequest(mPreviewRequestBuilder.build(),
                    /*listener*/null, /*handler*/null);
        } catch (CameraAccessException e) {
            Log.e(TAG, "Update camera preview failed");
        }

        if (VERBOSE) {
            Log.v(TAG, "updatePreview - end");
        }
    }

    private static boolean hasTriggers(Builder requestBuilder) {
        if (requestBuilder == null) {
            return false;
        }

        Integer afTrigger = requestBuilder.get(CaptureRequest.CONTROL_AF_TRIGGER);
        Integer aePrecaptureTrigger = requestBuilder.get(
                CaptureRequest.CONTROL_AE_PRECAPTURE_TRIGGER);

        if (VERBOSE) {
            Log.v(TAG, String.format("hasTriggers - afTrigger = %s, aePreCaptureTrigger = %s",
                    afTrigger, aePrecaptureTrigger));
        }


        if (afTrigger != null && afTrigger != CaptureRequest.CONTROL_AF_TRIGGER_IDLE) {
            return true;
        }


        if (aePrecaptureTrigger != null
                && aePrecaptureTrigger != CaptureRequest.CONTROL_AE_PRECAPTURE_TRIGGER_IDLE) {
            return true;
        }

        return false;
    }

    private static void removeTriggers(Builder requestBuilder) {
        if (requestBuilder == null) {
            return;
        }

        requestBuilder.set(CaptureRequest.CONTROL_AF_TRIGGER,
                CaptureRequest.CONTROL_AF_TRIGGER_IDLE);
        requestBuilder.set(CaptureRequest.CONTROL_AE_PRECAPTURE_TRIGGER,
                CaptureRequest.CONTROL_AE_PRECAPTURE_TRIGGER_IDLE);
    }

    /**
     * Update current device orientation (0~360 degrees)
     */
    public void updateOrientation(int orientation) {
        mDeviceOrientation = orientation;
    }

    /**
     * Configure streams and run minimal preview
     */
    public void minimalPreview(SurfaceHolder previewHolder, CameraControls camCtl)
            throws ApiFailureException {

        minimalOpenCamera();

        if (mPreviewSurface == null) {
            throw new ApiFailureException("Preview surface is not created");
        }
        try {
            List<Surface> outputSurfaces = new ArrayList<Surface>(/*capacity*/1);
            outputSurfaces.add(mPreviewSurface);

            configureOutputs(outputSurfaces);

            mPreviewRequestBuilder = mCamera.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);
            updateCaptureRequest(mPreviewRequestBuilder, camCtl);

            mPreviewRequestBuilder.addTarget(mPreviewSurface);

            mSession.setRepeatingRequest(mPreviewRequestBuilder.build(), null, null);
        } catch (CameraAccessException e) {
            throw new ApiFailureException("Error setting up minimal preview", e);
        }
    }

    public void minimalJpegCapture(final CaptureCallback listener, CaptureResultListener l,
            Handler h, CameraControls cameraControl) throws ApiFailureException {
        minimalOpenCamera();

        try {
            CameraCharacteristics properties =
                    mCameraManager.getCameraCharacteristics(mCamera.getId());
            Size[] jpegSizes = null;
            if (properties != null) {
                jpegSizes = properties.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP).
                        getOutputSizes(ImageFormat.JPEG);
            }
            int width = 640;
            int height = 480;

            if (jpegSizes != null && jpegSizes.length > 0) {
                width = jpegSizes[0].getWidth();
                height = jpegSizes[0].getHeight();
            }

            if (mCaptureReader == null || mCaptureReader.getWidth() != width ||
                    mCaptureReader.getHeight() != height) {
                if (mCaptureReader != null) {
                    mCaptureReader.close();
                }
                mCaptureReader = ImageReader.newInstance(width, height,
                        ImageFormat.JPEG, MAX_CONCURRENT_JPEGS);
            }

            List<Surface> outputSurfaces = new ArrayList<Surface>(/*capacity*/1);
            outputSurfaces.add(mCaptureReader.getSurface());

            configureOutputs(outputSurfaces);

            CaptureRequest.Builder captureBuilder =
                    mCamera.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE);
            captureBuilder.set(CaptureRequest.JPEG_ORIENTATION, getOrientationHint());

            captureBuilder.addTarget(mCaptureReader.getSurface());

            updateCaptureRequest(captureBuilder, cameraControl);

            ImageReader.OnImageAvailableListener readerListener =
                    new ImageReader.OnImageAvailableListener() {
                @Override
                public void onImageAvailable(ImageReader reader) {
                    Image i = null;
                    try {
                        i = reader.acquireNextImage();
                        listener.onCaptureAvailable(i);
                    } finally {
                        if (i != null) {
                            i.close();
                        }
                    }
                }
            };
            mCaptureReader.setOnImageAvailableListener(readerListener, h);

            mSession.capture(captureBuilder.build(), l, mOpsHandler);
        } catch (CameraAccessException e) {
            throw new ApiFailureException("Error in minimal JPEG capture", e);
        }
    }

    public void startRecording(Context applicationContext, boolean useMediaCodec, int outputFormat)
            throws ApiFailureException {
        minimalOpenCamera();
        Size recordingSize = getRecordingSize();
        int orientationHint = getOrientationHint();
        try {
            if (mRecordingRequestBuilder == null) {
                mRecordingRequestBuilder =
                        mCamera.createCaptureRequest(CameraDevice.TEMPLATE_RECORD);
            }
            // Setup output stream first
            mRecordingStream.configure(
                    applicationContext, recordingSize, useMediaCodec, mEncodingBitRate,
                    orientationHint, outputFormat);
            mRecordingStream.onConfiguringOutputs(mOutputSurfaces, /* detach */false);
            mRecordingStream.onConfiguringRequest(mRecordingRequestBuilder, /* detach */false);

            // TODO: For preview, create preview stream class, and do the same thing like recording.
            mOutputSurfaces.add(mPreviewSurface);
            mRecordingRequestBuilder.addTarget(mPreviewSurface);

            // Start camera streaming and recording.
            configureOutputs(mOutputSurfaces);
            mSession.setRepeatingRequest(mRecordingRequestBuilder.build(), null, null);
            mRecordingStream.start();
        } catch (CameraAccessException e) {
            throw new ApiFailureException("Error start recording", e);
        }
    }

    public void stopRecording(Context ctx) throws ApiFailureException {
        try {
            /**
             * <p>
             * Only stop camera recording stream.
             * </p>
             * <p>
             * FIXME: There is a race condition to be fixed in CameraDevice.
             * Basically, when stream closes, encoder and its surface is
             * released, while it still takes some time for camera to finish the
             * output to that surface. Then it cause camera in bad state.
             * </p>
             */
            mRecordingStream.onConfiguringRequest(mRecordingRequestBuilder, /* detach */true);
            mRecordingStream.onConfiguringOutputs(mOutputSurfaces, /* detach */true);

            // Remove recording surface before calling RecordingStream.stop,
            // since that invalidates the surface.
            configureOutputs(mOutputSurfaces);

            mRecordingStream.stop(ctx);

            mSession.setRepeatingRequest(mRecordingRequestBuilder.build(), null, null);
        } catch (CameraAccessException e) {
            throw new ApiFailureException("Error stop recording", e);
        }
    }

    /**
     * Flush all current requests and in-progress work
     */
    public void flush() throws ApiFailureException {
        minimalOpenCamera();
        try {
            mSession.abortCaptures();
        } catch (CameraAccessException e) {
            throw new ApiFailureException("Error flushing", e);
        }
    }

    private int getOrientationHint() {
        // snap to {0, 90, 180, 270}
        int orientation = ((int)Math.round(mDeviceOrientation/90.0)*90) % 360;

        CameraCharacteristics properties = getCameraCharacteristics();
        int sensorOrientation = properties.get(CameraCharacteristics.SENSOR_ORIENTATION);

        // TODO: below calculation is for back-facing camera only
        // front-facing camera should use:
        // return (sensorOrientation - orientation +360) % 360;
        return (sensorOrientation + orientation) % 360;
    }

    private Size getRecordingSize() throws ApiFailureException {
        try {
            CameraCharacteristics properties =
                    mCameraManager.getCameraCharacteristics(mCamera.getId());

            Size[] recordingSizes = null;
            if (properties != null) {
                recordingSizes =
                        properties.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP).
                        getOutputSizes(MediaCodec.class);
            }

            mEncodingBitRate = ENC_BIT_RATE_LOW;
            if (recordingSizes == null || recordingSizes.length == 0) {
                Log.w(TAG, "Unable to get recording sizes, default to 640x480");
                return DEFAULT_SIZE;
            } else {
                /**
                 * TODO: create resolution selection widget on UI, then use the
                 * select size. For now, return HIGH_RESOLUTION_SIZE if it
                 * exists in the processed size list, otherwise return default
                 * size
                 */
                if (Arrays.asList(recordingSizes).contains(HIGH_RESOLUTION_SIZE)) {
                    mEncodingBitRate = ENC_BIT_RATE_HIGH;
                    return HIGH_RESOLUTION_SIZE;
                } else {
                    // Fallback to default size when HD size is not found.
                    Log.w(TAG,
                            "Unable to find the requested size " + HIGH_RESOLUTION_SIZE.toString()
                            + " Fallback to " + DEFAULT_SIZE.toString());
                    return DEFAULT_SIZE;
                }
            }
        } catch (CameraAccessException e) {
            throw new ApiFailureException("Error setting up video recording", e);
        }
    }

    private void updateCaptureRequest(CaptureRequest.Builder builder,
            CameraControls cameraControl) {
        if (cameraControl != null) {
            // Update the manual control metadata for capture request
            // may disable 3A routines.
            updateCaptureRequest(builder, cameraControl.getManualControls());
            // Update the AF control metadata for capture request (if manual is not used)
            updateCaptureRequest(builder, cameraControl.getAfControls());
        }
    }

    private void updateCaptureRequest(CaptureRequest.Builder builder,
            CameraManualControls manualControls) {
        if (manualControls == null) {
            return;
        }

        if (manualControls.isManualControlEnabled()) {
            Log.e(TAG, "update request: " + manualControls.getSensitivity());
            builder.set(CaptureRequest.CONTROL_MODE,
                    CameraMetadata.CONTROL_MODE_OFF);
            builder.set(CaptureRequest.SENSOR_SENSITIVITY,
                    manualControls.getSensitivity());
            builder.set(CaptureRequest.SENSOR_FRAME_DURATION,
                    manualControls.getFrameDuration());
            builder.set(CaptureRequest.SENSOR_EXPOSURE_TIME,
                    manualControls.getExposure());

            if (VERBOSE) {
                Log.v(TAG, "updateCaptureRequest - manual - control.mode = OFF");
            }
        } else {
            builder.set(CaptureRequest.CONTROL_MODE,
                    CameraMetadata.CONTROL_MODE_AUTO);

            if (VERBOSE) {
                Log.v(TAG, "updateCaptureRequest - manual - control.mode = AUTO");
            }
        }
    }

    private void updateCaptureRequest(CaptureRequest.Builder builder,
            CameraAutoFocusControls cameraAfControl) {
        if (cameraAfControl == null) {
            return;
        }

        if (cameraAfControl.isAfControlEnabled()) {
            builder.set(CaptureRequest.CONTROL_AF_MODE, cameraAfControl.getAfMode());

            Integer afTrigger = cameraAfControl.consumePendingTrigger();

            if (afTrigger != null) {
                builder.set(CaptureRequest.CONTROL_AF_TRIGGER, afTrigger);
            }

            if (VERBOSE) {
                Log.v(TAG, "updateCaptureRequest - AF - set trigger to " + afTrigger);
            }
        }
    }

    public interface CaptureCallback {
        void onCaptureAvailable(Image capture);
    }

    public static abstract class CaptureResultListener
            extends CameraCaptureSession.CaptureCallback {}
}
