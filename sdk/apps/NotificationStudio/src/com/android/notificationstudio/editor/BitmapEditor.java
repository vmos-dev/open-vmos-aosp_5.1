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

package com.android.notificationstudio.editor;

import android.content.res.Resources;
import android.graphics.Bitmap;
import android.widget.ImageView;

import com.android.notificationstudio.R;

public class BitmapEditor extends IconEditor {

    @Override
    protected void setImage(ImageView imageView, Object value) {
        imageView.setImageBitmap((Bitmap) value);
    }

    protected int getIconSize(Resources res) {
        return res.getDimensionPixelSize(R.dimen.editor_icon_size_large);
    }

}
