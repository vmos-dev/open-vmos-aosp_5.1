package com.android.testingcamera;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.media.ExifInterface;
import android.media.MediaScannerConnection.OnScanCompletedListener;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.app.DialogFragment;
import android.content.Intent;
import android.text.method.ScrollingMovementMethod;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;

class SnapshotDialogFragment extends DialogFragment
                implements OnScanCompletedListener{

    private ImageView mInfoImage;
    private TextView mInfoText;
    private Button mOkButton;
    private Button mSaveButton;
    private Button mSaveAndViewButton;

    private byte[] mJpegImage;
    private boolean mSaved = false;
    private boolean mViewWhenReady = false;
        private Uri mSavedUri = null;

    public SnapshotDialogFragment() {
        // Empty constructor required for DialogFragment
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_snapshot, container);

        mOkButton = (Button) view.findViewById(R.id.snapshot_ok);
        mOkButton.setOnClickListener(mOkButtonListener);

        mSaveButton = (Button) view.findViewById(R.id.snapshot_save);
        mSaveButton.setOnClickListener(mSaveButtonListener);

        mSaveAndViewButton = (Button) view.findViewById(R.id.snapshot_view);
        mSaveAndViewButton.setOnClickListener(mSaveAndViewButtonListener);

        mInfoImage = (ImageView) view.findViewById(R.id.snapshot_image);
        mInfoText= (TextView) view.findViewById(R.id.snapshot_text);
        mInfoText.setMovementMethod(new ScrollingMovementMethod());

        if (mJpegImage != null) {
            new AsyncTask<byte[], Integer, Bitmap>() {
                @Override
                protected Bitmap doInBackground(byte[]... params) {
                    byte[] jpegImage = params[0];
                    BitmapFactory.Options opts = new BitmapFactory.Options();
                    opts.inJustDecodeBounds = true;
                    BitmapFactory.decodeByteArray(jpegImage, 0,
                            jpegImage.length, opts);
                    // Keep image at less than 1 MP.
                    if (opts.outWidth > 1024 || opts.outHeight > 1024) {
                        int scaleFactorX = opts.outWidth / 1024 + 1;
                        int scaleFactorY = opts.outHeight / 1024 + 1;
                        int scaleFactor = scaleFactorX > scaleFactorY ?
                            scaleFactorX : scaleFactorY;
                        opts.inSampleSize = scaleFactor;
                    }
                    opts.inJustDecodeBounds = false;
                    Bitmap img = BitmapFactory.decodeByteArray(jpegImage, 0,
                            jpegImage.length, opts);
                    return img;
                }

                @Override
                protected void onPostExecute(Bitmap img) {
                    mInfoImage.setImageBitmap(img);
                }
            }.execute(mJpegImage);
        }

        getDialog().setTitle("Snapshot result");
        return view;
    }

    public OnClickListener mOkButtonListener = new OnClickListener() {
        @Override
        public void onClick(View v) {
            dismiss();
        }
    };

    public OnClickListener mSaveButtonListener = new OnClickListener() {
        @Override
        public void onClick(View v) {
            saveFile();
        }
    };

    public OnClickListener mSaveAndViewButtonListener = new OnClickListener() {
        @Override
        public void onClick(View v) {
            saveFile();
            viewFile();
        }
    };

    public void updateImage(byte[] image) {
        mJpegImage = image;
    }

    private String getAttrib(ExifInterface exif, String tag) {
        String attribute = exif.getAttribute(tag);
        return (attribute == null) ? "???" : attribute;
    }

    private void saveFile() {
        if (!mSaved) {
            TestingCamera parent = (TestingCamera) getActivity();
            parent.log("Saving image");

            File targetFile = parent.getOutputMediaFile(TestingCamera.MEDIA_TYPE_IMAGE);
            if (targetFile == null) {
                parent.logE("Unable to create file name");
                return;
            }
            parent.logIndent(1);
            parent.log("File name: " + targetFile.toString());
            try {
                FileOutputStream out = new FileOutputStream(targetFile);
                out.write(mJpegImage);
                out.close();
                mSaved = true;
                parent.notifyMediaScannerOfFile(targetFile, this);
                updateExif(targetFile);
            } catch (IOException e) {
                parent.logE("Unable to save file: " + e.getMessage());
            }
            parent.logIndent(-1);
        }
    }

    private void updateExif(File targetFile) {
        ((TestingCamera) getActivity()).log("Extracting EXIF");
        try {
            ExifInterface exif = new ExifInterface(targetFile.toString());

            String aperture = getAttrib(exif, ExifInterface.TAG_APERTURE);

            String dateTime = getAttrib(exif, ExifInterface.TAG_DATETIME);
            String exposureTime = getAttrib(exif, ExifInterface.TAG_EXPOSURE_TIME);
            int flash = exif.getAttributeInt(ExifInterface.TAG_FLASH, 0);
            double focalLength = exif.getAttributeDouble(ExifInterface.TAG_FOCAL_LENGTH, 0);

            double gpsAltitude = exif.getAltitude(Double.NaN);
            String gpsDatestamp = getAttrib(exif, ExifInterface.TAG_GPS_DATESTAMP);
            float[] gpsCoords = new float[2];
            if (!exif.getLatLong(gpsCoords)) {
                gpsCoords[0] = Float.NaN;
                gpsCoords[1] = Float.NaN;
            }
            String gpsProcessingMethod = getAttrib(exif, ExifInterface.TAG_GPS_PROCESSING_METHOD);
            String gpsTimestamp = getAttrib(exif, ExifInterface.TAG_GPS_TIMESTAMP);

            int width = exif.getAttributeInt(ExifInterface.TAG_IMAGE_WIDTH, 0);
            int height = exif.getAttributeInt(ExifInterface.TAG_IMAGE_LENGTH, 0);
            String iso = getAttrib(exif, ExifInterface.TAG_ISO);
            String make = getAttrib(exif, ExifInterface.TAG_MAKE);
            String model = getAttrib(exif, ExifInterface.TAG_MODEL);
            int orientationVal = exif.getAttributeInt(ExifInterface.TAG_ORIENTATION, -1);
            int whiteBalance = exif.getAttributeInt(ExifInterface.TAG_WHITE_BALANCE, 0);

            final String[] orientationStrings= new String[] {
                "UNDEFINED",
                "NORMAL",
                "FLIP_HORIZONTAL",
                "ROTATE_180",
                "FLIP_VERTICAL",
                "TRANSPOSE",
                "ROTATE_90",
                "TRANSVERSE",
                "ROTATE_270"
            };
            if (orientationVal >= orientationStrings.length) {
                orientationVal = 0;
            }
            String orientation = orientationStrings[orientationVal];

            StringBuilder exifInfo = new StringBuilder();
            exifInfo.append("EXIF information for ").
                append(targetFile.toString()).append("\n\n");
            exifInfo.append("Size: ").
                append(width).append(" x ").append(height).append("\n");
            exifInfo.append("Make: ").
                append(make).append("\n");
            exifInfo.append("Model: ").
                append(model).append("\n");
            exifInfo.append("Orientation: ").
                append(orientation).append("\n");
            exifInfo.append("Aperture: ").
                append(aperture).append("\n");
            exifInfo.append("Focal length: ").
                append(focalLength).append("\n");
            exifInfo.append("Exposure time: ").
                append(exposureTime).append("\n");
            exifInfo.append("ISO: ").
                append(iso).append("\n");
            exifInfo.append("Flash: ").
                append(flash).append("\n");
            exifInfo.append("White balance: ").
                append(whiteBalance).append("\n");
            exifInfo.append("Date/Time: ").
                append(dateTime).append("\n");
            exifInfo.append("GPS altitude: ").
                append(gpsAltitude).append("\n");
            exifInfo.append("GPS latitude: ").
                append(gpsCoords[0]).append("\n");
            exifInfo.append("GPS longitude: ").
                append(gpsCoords[1]).append("\n");
            exifInfo.append("GPS datestamp: ").
                append(gpsDatestamp).append("\n");
            exifInfo.append("GPS timestamp: ").
                append(gpsTimestamp).append("\n");
            exifInfo.append("GPS processing method: ").
                append(gpsProcessingMethod).append("\n");
            mInfoText.setText(exifInfo.toString());

        } catch (IOException e) {
            ((TestingCamera) getActivity()).logE("Unable to extract EXIF: " + e.getMessage());
        }
    }

    private synchronized void viewFile() {
        if (!mSaved) return;
        if (mSavedUri != null) {
            ((TestingCamera) getActivity()).log("Viewing file");
            mViewWhenReady = false;
            getActivity().startActivity(new Intent(Intent.ACTION_VIEW, mSavedUri));
        } else {
            mViewWhenReady = true;
        }
    }

    @Override
    public synchronized void onScanCompleted(String path, Uri uri) {
        mSavedUri = uri;
        if (mViewWhenReady) viewFile();
    }
}
