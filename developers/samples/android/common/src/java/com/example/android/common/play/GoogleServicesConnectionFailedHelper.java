/**
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

import android.app.Dialog;
import android.content.IntentSender;
import android.support.v4.app.FragmentActivity;

import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.GooglePlayServicesClient;

/**
 * Helper to handle errors from Google Play Services connections.
 *
 */
public class GoogleServicesConnectionFailedHelper implements
        GooglePlayServicesClient.OnConnectionFailedListener {

    FragmentActivity mActivity;
    int mRequestCode = -1;

    public GoogleServicesConnectionFailedHelper(FragmentActivity mActivity, int requestCode) {
        this.mActivity = mActivity;
        mRequestCode = requestCode;
    }

    @Override
    public void onConnectionFailed(ConnectionResult connectionResult) {

        /*
         * Google Play services can resolve some errors it detects.
         * If the error has a resolution, try sending an Intent to
         * start a Google Play services activity that can resolve
         * error.
         */
        if (connectionResult.hasResolution()) {
            try {
                // Start an Activity that tries to resolve the error
                connectionResult.startResolutionForResult(mActivity, mRequestCode);
                /*
                 * Thrown if Google Play services canceled the original
                 * PendingIntent
                 */
            } catch (IntentSender.SendIntentException e) {
                // Log the error
                e.printStackTrace();
            }
        } else {
            /*
             * If no resolution is available, display a dialog to the
             * user with the error.
             */
            PlayHelper.ErrorDialogFragment fragment = new PlayHelper.ErrorDialogFragment();
            fragment.setDialog(new Dialog(mActivity));
            fragment.show(mActivity.getSupportFragmentManager(), null);

        }
    }
}

