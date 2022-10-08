/*
 * Copyright 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


package com.example.android.common.play;

import android.app.Activity;
import android.app.Dialog;
import android.os.Bundle;
import android.support.v4.app.DialogFragment;
import android.support.v4.app.FragmentActivity;

import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.GooglePlayServicesUtil;

/**
 * Helper class for Google Play Services functions.
 * <ul>
 *     <li>
 *         Checks availability
 *     </li>
 *     <li>
 *         Validates version for version bound features
 *     </li>
 * </ul>
 */
public class PlayHelper {

    /**
     * Checks for Google Play Services installation on the device. If found, validates the
     * installed version against the requested version. If the service is installed but
     * can't be used, the utility initiates a recovery with user intervention.
     *
     * @param context The context to be associated with the request. For compatibility with 1.6+,
     *                subclass {@link FragmentActivity}.
     * @param requestCode If we need to download Google Play Services, the download activity will be
     *                    started using {@link Activity#startActivityForResult(Intent, int)}
     * @param versionCode The minimum required version of the library.
     * @return True, if successful.
     */
    public static boolean checkGooglePlayServiceAvailability(
            FragmentActivity context, int requestCode, int versionCode) {

        // Query for the status of Google Play services on the device
        int statusCode = GooglePlayServicesUtil
                .isGooglePlayServicesAvailable(context);

        if ((statusCode == ConnectionResult.SUCCESS )
                &&  (GooglePlayServicesUtil.GOOGLE_PLAY_SERVICES_VERSION_CODE >=  versionCode)) {
            return true;
        } else {
            if (GooglePlayServicesUtil.isUserRecoverableError(statusCode)) {
                Dialog eDialog = GooglePlayServicesUtil.getErrorDialog(statusCode,
                        context, requestCode);
                // If Google Play services can provide an error dialog
                if (eDialog != null) {
                    // Create a new DialogFragment for the error dialog
                    ErrorDialogFragment errorFragment =
                            new ErrorDialogFragment();
                    // Set the dialog in the DialogFragment
                    errorFragment.setDialog(eDialog);
                    // Show the error dialog in the DialogFragment
                    errorFragment.show(
                            context.getSupportFragmentManager(),
                            "Activity Recognition");
                }
            } else {
                return false;
            }
        }
        return false;
    }

    /**
     * Checks for Google Play Services installation on the device. If the service is installed but
     * can't be used, the utility initiates a recovery with user intervention.
     *
     * @param context The context to be associated with the request. For compatibility with 1.6+,
     *                subclass {@link FragmentActivity}.
     * @return True, if successful.
     */
    public static boolean checkGooglePlayServiceAvailability(FragmentActivity context,
            int requestCode) {
        return checkGooglePlayServiceAvailability(context, requestCode, -1);
    }

    // Define a DialogFragment that displays the error dialog
    public static class ErrorDialogFragment extends DialogFragment {
        // Global field to contain the error dialog
        private Dialog mDialog;
        // Default constructor. Sets the dialog field to null
        public ErrorDialogFragment() {
            super();
            mDialog = null;
        }
        // Set the dialog to display
        public void setDialog(Dialog dialog) {
            mDialog = dialog;
        }
        // Return a Dialog to the DialogFragment.
        @Override
        public Dialog onCreateDialog(Bundle savedInstanceState) {
            return mDialog;
        }
    }

}
