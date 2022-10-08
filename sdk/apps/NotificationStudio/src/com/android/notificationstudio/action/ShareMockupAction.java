/*
 * Copyright 2012 The Android Open Source Project
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

package com.android.notificationstudio.action;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.net.Uri;
import android.util.Log;
import android.view.View;
import android.widget.Toast;

import com.android.notificationstudio.R;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.Date;

public class ShareMockupAction {
    private static final String TAG = ShareMockupAction.class.getSimpleName();

    private static final SimpleDateFormat FILE_NAME =
            new SimpleDateFormat("'notification.'yyyyMMdd'.'HHmmss'.png'");

    public static void launch(Activity activity, CharSequence title) {
        // take a picture of the current mockup
        View v = activity.findViewById(R.id.preview);
        int w = v.getMeasuredWidth();
        int h = v.getMeasuredHeight();
        Bitmap mockup = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
        Canvas c = new Canvas(mockup);
        v.layout(0, 0, w, h);
        v.draw(c);

        // write the mockup to a temp file
        ByteArrayOutputStream bytes = new ByteArrayOutputStream();
        mockup.compress(Bitmap.CompressFormat.PNG, 100, bytes);
        File f = new File(activity.getExternalCacheDir(), FILE_NAME.format(new Date()));
        FileOutputStream fo = null;
        try {
            f.createNewFile();
            fo = new FileOutputStream(f);
            fo.write(bytes.toByteArray());
        } catch (IOException e) {
            String msg = "Error writing mockup file";
            Log.w(TAG, msg, e);
            Toast.makeText(activity, msg, Toast.LENGTH_SHORT).show();
            return;
        } finally {
            if (fo != null)
               try { fo.close(); } catch (Exception e) { }
        }

        // launch intent to send the mockup image
        Intent share = new Intent(Intent.ACTION_SEND);
        share.setType("image/png");
        share.putExtra(Intent.EXTRA_STREAM, Uri.fromFile(f.getAbsoluteFile()));
        activity.startActivity(Intent.createChooser(share, title));
    }

}
