/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.testingcamera2;

import java.util.ArrayList;
import java.util.Date;
import java.util.LinkedList;
import java.util.List;
import java.util.Objects;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.nio.ShortBuffer;
import java.nio.channels.Channels;
import java.nio.channels.WritableByteChannel;
import java.text.SimpleDateFormat;

import android.content.Context;
import android.graphics.ImageFormat;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.DngCreator;
import android.hardware.camera2.TotalCaptureResult;
import android.hardware.camera2.params.StreamConfigurationMap;
import android.media.Image;
import android.media.ImageReader;
import android.os.Environment;
import android.os.SystemClock;
import android.util.Size;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.Surface;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.Spinner;
import android.widget.AdapterView.OnItemSelectedListener;

public class ImageReaderSubPane extends TargetSubPane {

    private static final int NO_FORMAT = -1;
    private static final int NO_SIZE = -1;
    private static final int NO_IMAGE = -1;
    private static final int MAX_BUFFER_COUNT = 25;
    private static final int DEFAULT_BUFFER_COUNT = 3;

    enum OutputFormat {
        JPEG(ImageFormat.JPEG),
        RAW16(ImageFormat.RAW_SENSOR),
        RAW10(ImageFormat.RAW10),
        YUV_420_888(ImageFormat.YUV_420_888);

        public final int imageFormat;

        OutputFormat(int imageFormat) {
            this.imageFormat = imageFormat;
        }
    };

    private Surface mSurface;

    private final Spinner mFormatSpinner;
    private final List<OutputFormat> mFormats = new ArrayList<>();

    private final Spinner mSizeSpinner;
    private Size[] mSizes;
    private final Spinner mCountSpinner;
    private Integer[] mCounts;

    private final ImageView mImageView;

    private int mCurrentCameraOrientation = 0;
    private int mCurrentUiOrientation = 0;

    private int mCurrentFormatId = NO_FORMAT;
    private int mCurrentSizeId = NO_SIZE;
    private CameraControlPane mCurrentCamera;

    private OutputFormat mConfiguredFormat = null;
    private Size mConfiguredSize = null;
    private int mConfiguredCount = 0;

    private ImageReader mReader = null;
    private final LinkedList<Image> mCurrentImages = new LinkedList<>();
    private int mCurrentImageIdx = NO_IMAGE;

    private int mRawShiftFactor = 0;
    private int mRawShiftRow = 0;
    private int mRawShiftCol = 0;

    public ImageReaderSubPane(Context context, AttributeSet attrs) {
        super(context, attrs);

        LayoutInflater inflater =
                (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);

        inflater.inflate(R.layout.imagereader_target_subpane, this);
        this.setOrientation(VERTICAL);

        mFormatSpinner =
                (Spinner) this.findViewById(R.id.target_subpane_image_reader_format_spinner);
        mFormatSpinner.setOnItemSelectedListener(mFormatSpinnerListener);

        mSizeSpinner = (Spinner) this.findViewById(R.id.target_subpane_image_reader_size_spinner);
        mSizeSpinner.setOnItemSelectedListener(mSizeSpinnerListener);

        mCountSpinner =
                (Spinner) this.findViewById(R.id.target_subpane_image_reader_count_spinner);
        mCounts = new Integer[MAX_BUFFER_COUNT];
        for (int i = 0; i < mCounts.length; i++) {
            mCounts[i] = i + 1;
        }
        mCountSpinner.setAdapter(new ArrayAdapter<>(getContext(), R.layout.spinner_item,
                        mCounts));
        mCountSpinner.setSelection(DEFAULT_BUFFER_COUNT - 1);

        mImageView = (ImageView) this.findViewById(R.id.target_subpane_image_reader_view);

        Button b = (Button) this.findViewById(R.id.target_subpane_image_reader_prev_button);
        b.setOnClickListener(mPrevButtonListener);

        b = (Button) this.findViewById(R.id.target_subpane_image_reader_next_button);
        b.setOnClickListener(mNextButtonListener);

        b = (Button) this.findViewById(R.id.target_subpane_image_reader_save_button);
        b.setOnClickListener(mSaveButtonListener);
    }

    @Override
    public void setTargetCameraPane(CameraControlPane target) {
        mCurrentCamera = target;
        if (target != null) {
            updateFormats();
        } else {
            mSizeSpinner.setAdapter(null);
            mCurrentSizeId = NO_SIZE;
        }
    }

    @Override
    public void setUiOrientation(int orientation) {
        mCurrentUiOrientation = orientation;
    }

    private void updateFormats() {
        if (mCurrentCamera == null) {
            mFormatSpinner.setAdapter(null);
            mCurrentFormatId = NO_FORMAT;
            updateSizes();
            return;
        }

        OutputFormat oldFormat = null;
        if (mCurrentFormatId != NO_FORMAT) {
            oldFormat = mFormats.get(mCurrentFormatId);
        }

        CameraCharacteristics info = mCurrentCamera.getCharacteristics();
        StreamConfigurationMap streamConfigMap =
                info.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);

        mFormats.clear();
        for (OutputFormat format : OutputFormat.values()) {
            if (streamConfigMap.isOutputSupportedFor(format.imageFormat)) {
                mFormats.add(format);
            }
        }

        int newSelectionId = 0;
        for (int i = 0; i < mFormats.size(); i++) {
            if (mFormats.get(i).equals(oldFormat)) {
                newSelectionId = i;
                break;
            }
        }

        String[] outputFormatItems = new String[mFormats.size()];
        for (int i = 0; i < outputFormatItems.length; i++) {
            outputFormatItems[i] = mFormats.get(i).toString();
        }

        mFormatSpinner.setAdapter(new ArrayAdapter<>(getContext(), R.layout.spinner_item,
                        outputFormatItems));
        mFormatSpinner.setSelection(newSelectionId);
        mCurrentFormatId = newSelectionId;

        // Map sensor orientation to Surface.ROTATE_* constants
        final int SENSOR_ORIENTATION_TO_SURFACE_ROTATE = 90;
        mCurrentCameraOrientation = info.get(CameraCharacteristics.SENSOR_ORIENTATION) /
                SENSOR_ORIENTATION_TO_SURFACE_ROTATE;

        // Get the max white level for raw data if any
        Integer maxLevel = info.get(CameraCharacteristics.SENSOR_INFO_WHITE_LEVEL);
        if (maxLevel != null) {
            int l = maxLevel;
            // Find number of bits to shift to map from 0..WHITE_LEVEL to 0..255
            for (mRawShiftFactor = 0; l > 255; mRawShiftFactor++) l >>= 1;
        } else {
            mRawShiftFactor = 0;
        }

        Integer cfa = info.get(CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT);
        if (cfa != null) {
            switch (cfa) {
                case CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGGB:
                    mRawShiftRow = 0;
                    mRawShiftCol = 0;
                    break;
                case CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GRBG:
                    mRawShiftRow = 0;
                    mRawShiftCol = 1;
                    break;
                case CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GBRG:
                    mRawShiftRow = 1;
                    mRawShiftCol = 0;
                    break;
                case CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_BGGR:
                    mRawShiftRow = 1;
                    mRawShiftCol = 1;
                    break;
                case CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGB:
                    mRawShiftRow = 0;
                    mRawShiftCol = 0;

                    break;
            }
        }
        updateSizes();
    }

    private void updateSizes() {

        if (mCurrentCamera == null) {
            mSizeSpinner.setAdapter(null);
            mCurrentSizeId = NO_SIZE;
            return;
        }

        Size oldSize = null;
        if (mCurrentSizeId != NO_SIZE) {
            oldSize = mSizes[mCurrentSizeId];
        }

        CameraCharacteristics info = mCurrentCamera.getCharacteristics();
        StreamConfigurationMap streamConfigMap =
                info.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);

        mSizes = streamConfigMap.getOutputSizes(mFormats.get(mCurrentFormatId).imageFormat);

        int newSelectionId = 0;
        for (int i = 0; i < mSizes.length; i++) {
            if (mSizes[i].equals(oldSize)) {
                newSelectionId = i;
                break;
            }
        }
        String[] outputSizeItems = new String[mSizes.length];
        for (int i = 0; i < outputSizeItems.length; i++) {
            outputSizeItems[i] = mSizes[i].toString();
        }

        mSizeSpinner.setAdapter(new ArrayAdapter<>(getContext(), R.layout.spinner_item,
                        outputSizeItems));
        mSizeSpinner.setSelection(newSelectionId);
        mCurrentSizeId = newSelectionId;
    }

    private void updateImage() {
        if (mCurrentImageIdx == NO_IMAGE) return;
        Image img = mCurrentImages.get(mCurrentImageIdx);

        // Find rough scale factor to fit image into imageview to minimize processing overhead
        // Want to be one factor too large
        int SCALE_FACTOR = 2;
        while (mConfiguredSize.getWidth() > (mImageView.getWidth() * SCALE_FACTOR << 1) ) {
            SCALE_FACTOR <<= 1;
        }

        Bitmap imgBitmap = null;
        switch (img.getFormat()) {
            case ImageFormat.JPEG: {
                ByteBuffer jpegBuffer = img.getPlanes()[0].getBuffer();
                jpegBuffer.rewind();
                byte[] jpegData = new byte[jpegBuffer.limit()];
                jpegBuffer.get(jpegData);
                BitmapFactory.Options opts = new BitmapFactory.Options();
                opts.inSampleSize = SCALE_FACTOR;
                imgBitmap = BitmapFactory.decodeByteArray(jpegData, 0, jpegData.length, opts);
                break;
            }
            case ImageFormat.YUV_420_888: {
                ByteBuffer yBuffer = img.getPlanes()[0].getBuffer();
                yBuffer.rewind();
                int w = mConfiguredSize.getWidth() / SCALE_FACTOR;
                int h = mConfiguredSize.getHeight() / SCALE_FACTOR;
                byte[] row = new byte[mConfiguredSize.getWidth()];
                int[] imgArray = new int[w * h];
                for (int y = 0, j = 0; y < h; y++) {
                    yBuffer.position(y * SCALE_FACTOR * mConfiguredSize.getWidth());
                    yBuffer.get(row);
                    for (int x = 0, i = 0; x < w; x++) {
                        int yval = row[i] & 0xFF;
                        imgArray[j] = Color.rgb(yval, yval, yval);
                        i += SCALE_FACTOR;
                        j++;
                    }
                }
                imgBitmap = Bitmap.createBitmap(imgArray, w, h, Bitmap.Config.ARGB_8888);
                break;
            }
            case ImageFormat.RAW_SENSOR: {
                ShortBuffer rawBuffer = img.getPlanes()[0].getBuffer().asShortBuffer();
                rawBuffer.rewind();
                // Very rough nearest-neighbor downsample for display
                int w = mConfiguredSize.getWidth() / SCALE_FACTOR;
                int h = mConfiguredSize.getHeight() / SCALE_FACTOR;
                short[] redRow = new short[mConfiguredSize.getWidth()];
                short[] blueRow = new short[mConfiguredSize.getWidth()];
                int[] imgArray = new int[w * h];
                for (int y = 0, j = 0; y < h; y++) {
                    // Align to start of red row in the pair to sample from
                    rawBuffer.position(
                        (y * SCALE_FACTOR + mRawShiftRow) * mConfiguredSize.getWidth());
                    rawBuffer.get(redRow);
                    // Align to start of blue row in the pair to sample from
                    rawBuffer.position(
                        (y * SCALE_FACTOR + 1 - mRawShiftRow) * mConfiguredSize.getWidth());
                    rawBuffer.get(blueRow);
                    for (int x = 0, i = 0; x < w; x++, i += SCALE_FACTOR, j++) {
                        int r = redRow[i + mRawShiftCol] >> mRawShiftFactor;
                        int g = redRow[i + 1 - mRawShiftCol] >> mRawShiftFactor;
                        int b = blueRow[i + 1 - mRawShiftCol] >> mRawShiftFactor;
                        imgArray[j] = Color.rgb(r,g,b);
                    }
                }
                imgBitmap = Bitmap.createBitmap(imgArray, w, h, Bitmap.Config.ARGB_8888);
                break;
            }
            case ImageFormat.RAW10: {
                TLog.e("RAW10 viewing not implemented");
                break;
            }
        }
        if (imgBitmap != null) {
            mImageView.setImageBitmap(imgBitmap);
        }
    }

    @Override
    public Surface getOutputSurface() {
        if (mCurrentSizeId == NO_SIZE ||
                mCurrentFormatId == NO_FORMAT) {
            return null;
        }
        Size s = mSizes[mCurrentSizeId];
        OutputFormat f = mFormats.get(mCurrentFormatId);
        int c = (Integer) mCountSpinner.getSelectedItem();
        if (mReader == null ||
                !Objects.equals(mConfiguredSize, s) ||
                !Objects.equals(mConfiguredFormat, f) ||
                mConfiguredCount != c) {

            if (mReader != null) {
                mReader.close();
                mCurrentImages.clear();
                mCurrentImageIdx = NO_IMAGE;
            }
            mReader = ImageReader.newInstance(s.getWidth(), s.getHeight(), f.imageFormat, c);
            mReader.setOnImageAvailableListener(mImageListener, null);
            mConfiguredSize = s;
            mConfiguredFormat = f;
            mConfiguredCount = c;
        }
        return mReader.getSurface();
    }

    private final OnItemSelectedListener mFormatSpinnerListener = new OnItemSelectedListener() {
        @Override
        public void onItemSelected(AdapterView<?> parent, View view, int pos, long id) {
            mCurrentFormatId = pos;
            updateSizes();
        };

        @Override
        public void onNothingSelected(AdapterView<?> parent) {
            mCurrentFormatId = NO_FORMAT;
        };
    };

    private final OnItemSelectedListener mSizeSpinnerListener = new OnItemSelectedListener() {
        @Override
        public void onItemSelected(AdapterView<?> parent, View view, int pos, long id) {
            mCurrentSizeId = pos;
        };

        @Override
        public void onNothingSelected(AdapterView<?> parent) {
            mCurrentSizeId = NO_SIZE;
        };
    };

    private final OnClickListener mPrevButtonListener = new OnClickListener() {
        @Override
        public void onClick(View v) {
            if (mCurrentImageIdx != NO_IMAGE) {
                int prevIdx = mCurrentImageIdx;
                mCurrentImageIdx = (mCurrentImageIdx == 0) ?
                        (mCurrentImages.size() - 1) : (mCurrentImageIdx - 1);
                if (prevIdx != mCurrentImageIdx) {
                    updateImage();
                }
            }
        }
    };

    private final OnClickListener mNextButtonListener = new OnClickListener() {
        @Override
        public void onClick(View v) {
            if (mCurrentImageIdx != NO_IMAGE) {
                int prevIdx = mCurrentImageIdx;
                mCurrentImageIdx = (mCurrentImageIdx == mCurrentImages.size() - 1) ?
                        0 : (mCurrentImageIdx + 1);
                if (prevIdx != mCurrentImageIdx) {
                    updateImage();
                }
            }
        }
    };

    private final OnClickListener mSaveButtonListener = new OnClickListener() {
        @Override
        public void onClick(View v) {
            // TODO: Make async and coordinate with onImageAvailable
            if (mCurrentImageIdx != NO_IMAGE) {
                Image img = mCurrentImages.get(mCurrentImageIdx);
                try {
                    String name = saveImage(img);
                    TLog.i("Saved image as %s", name);
                } catch (IOException e) {
                    TLog.e("Can't save file:", e);
                }
            }
        }
    };

    private final ImageReader.OnImageAvailableListener mImageListener =
            new ImageReader.OnImageAvailableListener() {
        @Override
        public void onImageAvailable(ImageReader reader) {
            while (mCurrentImages.size() >= reader.getMaxImages()) {
                Image oldest = mCurrentImages.remove();
                oldest.close();
                mCurrentImageIdx = Math.min(mCurrentImageIdx - 1, 0);
            }
            mCurrentImages.add(reader.acquireNextImage());
            if (mCurrentImageIdx == NO_IMAGE) {
                mCurrentImageIdx = 0;
            }
            updateImage();
        }
    };

    private String saveImage(Image img) throws IOException {
        long timestamp = img.getTimestamp();
        File output = getOutputImageFile(img.getFormat(), timestamp);
        try (FileOutputStream out = new FileOutputStream(output)) {
            switch(img.getFormat()) {
                case ImageFormat.JPEG: {
                    writeJpegImage(img, out);
                    break;
                }
                case ImageFormat.YUV_420_888: {
                    writeYuvImage(img, out);
                    break;
                }
                case ImageFormat.RAW_SENSOR: {
                    writeDngImage(img, out);
                    break;
                }
                case ImageFormat.RAW10: {
                    TLog.e("RAW10 saving not implemented");
                    break;
                }
            }
        }
        return output.getName();
    }

    private void writeDngImage(Image img, OutputStream out) throws IOException {
        if (img.getFormat() != ImageFormat.RAW_SENSOR) {
            throw new IOException(
                    String.format("Unexpected Image format: %d, expected ImageFormat.RAW_SENSOR",
                            img.getFormat()));
        }
        long timestamp = img.getTimestamp();
        if (mCurrentCamera == null) {
            TLog.e("No camera availble for camera info, not saving DNG (timestamp %d)",
                    timestamp);
            throw new IOException("No camera info available");
        }
        TotalCaptureResult result = mCurrentCamera.getResultAt(timestamp);
        if (result == null) {
            TLog.e("No result matching raw image found, not saving DNG (timestamp %d)",
                    timestamp);
            throw new IOException("No matching result found");
        }
        CameraCharacteristics info = mCurrentCamera.getCharacteristics();
        try (DngCreator writer = new DngCreator(info, result)) {
            writer.writeImage(out, img);
        }
    }

    private void writeJpegImage(Image img, OutputStream out) throws IOException {
        if (img.getFormat() != ImageFormat.JPEG) {
            throw new IOException(
                    String.format("Unexpected Image format: %d, expected ImageFormat.JPEG",
                            img.getFormat()));
        }
        WritableByteChannel outChannel = Channels.newChannel(out);
        ByteBuffer jpegData = img.getPlanes()[0].getBuffer();
        jpegData.rewind();
        outChannel.write(jpegData);
    }

    private void writeYuvImage(Image img, OutputStream out)
            throws IOException {
        if (img.getFormat() != ImageFormat.YUV_420_888) {
            throw new IOException(
                    String.format("Unexpected Image format: %d, expected ImageFormat.YUV_420_888",
                            img.getFormat()));
        }
        WritableByteChannel outChannel = Channels.newChannel(out);
        for (int plane = 0; plane < 3; plane++) {
            Image.Plane colorPlane = img.getPlanes()[plane];
            ByteBuffer colorData = colorPlane.getBuffer();
            int subsampleFactor = (plane == 0) ? 1 : 2;
            int colorW = img.getWidth() / subsampleFactor;
            int colorH = img.getHeight() / subsampleFactor;
            colorData.rewind();
            colorData.limit(colorData.capacity());
            if (colorPlane.getPixelStride() == 1) {
                // Can write contiguous rows
                for (int y = 0, rowStart = 0; y < colorH;
                        y++, rowStart += colorPlane.getRowStride()) {
                    colorData.limit(rowStart + colorW);
                    colorData.position(rowStart);
                    outChannel.write(colorData);
                }
            } else {
                // Need to pack rows
                byte[] row = new byte[colorW * colorPlane.getPixelStride()];
                byte[] packedRow = new byte[colorW];
                ByteBuffer packedRowBuffer = ByteBuffer.wrap(packedRow);
                for (int y = 0, rowStart = 0; y < colorH;
                        y++, rowStart += colorPlane.getRowStride()) {
                    colorData.position(rowStart);
                    colorData.get(row);
                    for (int x = 0, i = 0; x < colorW;
                            x++, i += colorPlane.getPixelStride()) {
                        packedRow[x] = row[i];
                    }
                    packedRowBuffer.rewind();
                    outChannel.write(packedRowBuffer);
                }
            }
        }
    }

    File getOutputImageFile(int type, long timestamp){
        // To be safe, you should check that the SDCard is mounted
        // using Environment.getExternalStorageState() before doing this.

        String state = Environment.getExternalStorageState();
        if (!Environment.MEDIA_MOUNTED.equals(state)) {
            return null;
        }

        File mediaStorageDir = new File(Environment.getExternalStoragePublicDirectory(
                  Environment.DIRECTORY_DCIM), "TestingCamera2");
        // This location works best if you want the created images to be shared
        // between applications and persist after your app has been uninstalled.

        // Create the storage directory if it does not exist
        if (!mediaStorageDir.exists()){
            if (!mediaStorageDir.mkdirs()){
                TLog.e("Failed to create directory for pictures/video");
                return null;
            }
        }

        // Create a media file name

        // Find out time now in the Date and boottime time bases.
        long nowMs = new Date().getTime();
        long nowBootTimeNs = SystemClock.elapsedRealtimeNanos();

        // Convert timestamp from boottime time base to the Date timebase
        // Slightly approximate, but close enough
        final long NS_PER_MS = 1000000l;
        long timestampMs = (nowMs * NS_PER_MS - nowBootTimeNs + timestamp) / NS_PER_MS;

        String timeStamp = new SimpleDateFormat("yyyy-MM-dd_HH-mm-ss_SSS").
                format(new Date(timestampMs));
        File mediaFile = null;
        switch(type) {
            case ImageFormat.JPEG:
                mediaFile = new File(mediaStorageDir.getPath() + File.separator +
                        "IMG_"+ timeStamp + ".jpg");
                break;
            case ImageFormat.YUV_420_888:
                mediaFile = new File(mediaStorageDir.getPath() + File.separator +
                        "IMG_"+ timeStamp + ".yuv");
                break;
            case ImageFormat.RAW_SENSOR:
                mediaFile = new File(mediaStorageDir.getPath() + File.separator +
                        "IMG_"+ timeStamp + ".dng");
                break;
            case ImageFormat.RAW10:
                mediaFile = new File(mediaStorageDir.getPath() + File.separator +
                        "IMG_"+ timeStamp + ".raw10");
                break;
        }

        return mediaFile;
    }

}
