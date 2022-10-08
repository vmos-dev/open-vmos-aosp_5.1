package com.android.testingcamera;

import android.content.res.Resources;
import android.graphics.ImageFormat;
import android.graphics.PixelFormat;
import android.os.AsyncTask;
import android.os.SystemClock;
import android.renderscript.Allocation;
import android.renderscript.Element;
import android.renderscript.Matrix4f;
import android.renderscript.RenderScript;
import android.renderscript.Script;
import android.renderscript.ScriptGroup;
import android.renderscript.ScriptIntrinsicColorMatrix;
import android.renderscript.Type;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceView;

/**
 *  Process preview callback data for display.
 *  This is done by constructing a two-step Renderscript group,
 *  the first of which converts from various YUV formats to 8bpp YUV, and
 *  the second of which converts from YUV to RGB.
 *
 *  The processing is done in a background thread, and the result is produced
 *  into an Allocation that's backed by a SurfaceView
 */
class CallbackProcessor {
    private SurfaceView mCallbackView;
    private Surface mCallbackSurface;

    private Object mTaskLock = new Object();

    private RenderScript mRS;
    private Allocation mAllocationIn;
    private Allocation mAllocationOut;
    private ScriptGroup mConverter;

    private int mWidth;
    private int mHeight;
    private int mFormat;

    private boolean mDone = false;
    private boolean mTaskInProgress = false;

    static final private int kStopTimeout = 2000; // ms

    private static final String TAG = "CallbackProcessor";

    public CallbackProcessor(int width, int height, int format,
            Resources res, SurfaceView callbackView,
            int viewWidth, int viewHeight,
            RenderScript rs) {
        mWidth = width;
        mHeight = height;
        mFormat = format;
        mRS = rs;
        mCallbackView = callbackView;

        int inputSize = TestingCamera.getCallbackBufferSize(mWidth, mHeight,
                mFormat);
        mAllocationIn = Allocation.createSized(mRS, Element.U8(mRS), inputSize);

        Type.Builder tb = new Type.Builder(mRS, Element.RGBA_8888(mRS));
        tb.setX(viewWidth);
        tb.setY(viewHeight);
        Type outType = tb.create();

        mAllocationOut = Allocation.createTyped(mRS, outType,
                Allocation.USAGE_IO_OUTPUT | Allocation.USAGE_SCRIPT);

        ScriptC_callback swizzleScript = new ScriptC_callback(mRS);
        swizzleScript.bind_yuv_in(mAllocationIn);
        swizzleScript.invoke_init_convert(mWidth, mHeight,
            mFormat, viewWidth, viewHeight);
        Script.KernelID swizzleId;

        switch (mFormat) {
        case ImageFormat.NV21:
            swizzleId = swizzleScript.getKernelID_convert_semiplanar();
            break;
        case ImageFormat.YV12:
            swizzleId = swizzleScript.getKernelID_convert_planar();
            break;
        case ImageFormat.YUY2:
            swizzleId = swizzleScript.getKernelID_convert_interleaved();
            break;
        case ImageFormat.UNKNOWN:
        default:
            swizzleId = swizzleScript.getKernelID_convert_unknown();
        }

        ScriptGroup.Builder b = new ScriptGroup.Builder(rs);
        b.addKernel(swizzleId);
        mConverter = b.create();

        mConverter.setOutput(swizzleId, mAllocationOut);
    }

    public boolean stop() {
        synchronized(mTaskLock) {
            mDone = true;
            long startTime = SystemClock.elapsedRealtime();
            while (mTaskInProgress) {
                try {
                    mTaskLock.wait(kStopTimeout);
                } catch (InterruptedException e) {
                    // ignored, keep waiting
                }
                long endTime = SystemClock.elapsedRealtime();
                if (endTime - startTime > kStopTimeout) {
                    return false;
                }
            }
        }
        mAllocationOut.setSurface(null);
        return true;
    }

    public void displayCallback(byte[] data) {
        synchronized(mTaskLock) {
            if (mTaskInProgress || mDone) return;
            mTaskInProgress = true;
        }
        if (mCallbackSurface == null) {
            mCallbackView.getHolder().setFormat(PixelFormat.RGBA_8888);
            mCallbackSurface = mCallbackView.getHolder().getSurface();
            if (mCallbackSurface == null) return;
            mAllocationOut.setSurface(mCallbackSurface);
        }
        new ProcessCallbackTask().execute(data);
    }

    private class ProcessCallbackTask extends AsyncTask<byte[], Void, Boolean> {

        @Override
        protected Boolean doInBackground(byte[]... datas) {
            byte[] data = datas[0];

            mAllocationIn.copyFrom(data);
            mConverter.execute();
            mAllocationOut.ioSend();

            synchronized(mTaskLock) {
                mTaskInProgress = false;
                mTaskLock.notify();
            }
            return true;
        }

        @Override
        protected void onPostExecute(Boolean result) {
        }
    }

}
