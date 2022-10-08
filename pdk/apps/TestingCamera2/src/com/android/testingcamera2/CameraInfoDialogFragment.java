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

import java.lang.reflect.Array;
import java.util.Locale;

import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.content.Context;
import android.content.DialogInterface;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CameraMetadata;
import android.os.Bundle;

/**
 * A simple dialog that writes out a given camera's camera characteristics into its message.
 *
 * <p>Does not depend on the rest of TestingCamera for operation.</p>
 *
 */
public class CameraInfoDialogFragment extends DialogFragment {

    private String mCameraId;

    public CameraInfoDialogFragment() {
        super();
        mCameraId = null;
    }

    /**
     * Set the camera ID for which to display the information.
     *
     * <p>Only effective if called before showing the dialog.</p>
     */
    public void setCameraId(String cameraId) {
        mCameraId = cameraId;
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());

        String title = String.format("Info: Camera %s", mCameraId);

        CameraCharacteristics info = null;
        if (mCameraId != null) {
            try {
                info = ((CameraManager) getActivity().getSystemService(Context.CAMERA_SERVICE)).
                        getCameraCharacteristics(mCameraId);
            } catch (CameraAccessException e) {
                TLog.e(String.format("Can't get characteristics for camera %s: %s", mCameraId, e));
            }
        }

        String infoText = formatCameraCharacteristics(info);

        builder.setTitle(title)
               .setMessage(infoText)
               .setPositiveButton(R.string.camera_info_dialog_ok_button,
                       new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int id) {
                       // do nothing, dialog fragment will hide itself
                   }
               });

        return builder.create();
    }

    /**
     * Convert camera characteristics into a key = values list for display
     * @param info camera characteristics to format
     * @return a multi-line string containing the list of key = value pairs
     */
    // Assumes every value type has a reasonable toString()
    private String formatCameraCharacteristics(CameraCharacteristics info) {
        String infoText;
        if (info != null) {
            StringBuilder infoBuilder = new StringBuilder("Camera characteristics:\n\n");

            for (CameraCharacteristics.Key<?> key : info.getKeys()) {
                infoBuilder.append(String.format(Locale.US, "%s:  ", key.getName()));

                Object val = info.get(key);
                if (val.getClass().isArray()) {
                    // Iterate an array-type value
                    // Assumes camera characteristics won't have arrays of arrays as values
                    int len = Array.getLength(val);
                    infoBuilder.append("[ ");
                    for (int i = 0; i < len; i++) {
                        infoBuilder.append(String.format(Locale.US,
                                "%s%s",
                                Array.get(val, i),
                                (i + 1 == len ) ? "" : ", "));
                    }
                    infoBuilder.append(" ]\n\n");
                } else {
                    // Single value
                    infoBuilder.append(String.format(Locale.US, "%s\n\n", val.toString()));
                }
            }
            infoText = infoBuilder.toString();
        } else {
            infoText = "No info";
        }
        return infoText;
    }

}
