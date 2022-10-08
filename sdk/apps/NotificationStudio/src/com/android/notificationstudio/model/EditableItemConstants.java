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

package com.android.notificationstudio.model;

import com.android.notificationstudio.R;

public interface EditableItemConstants {

    public static final int TYPE_TEXT = 0;
    public static final int TYPE_DROP_DOWN = 1;
    public static final int TYPE_RESOURCE_ID = 2;
    public static final int TYPE_BITMAP = 3;
    public static final int TYPE_INT = 4;
    public static final int TYPE_DATETIME = 5;
    public static final int TYPE_BOOLEAN = 6;
    public static final int TYPE_TEXT_LINES = 7;

    public static final int CATEGORY_MAIN = R.string.properties;
    public static final int CATEGORY_STYLE = R.string.style;
    public static final int CATEGORY_ACTION1 = R.string.action_1;
    public static final int CATEGORY_ACTION2 = R.string.action_2;
    public static final int CATEGORY_ACTION3 = R.string.action_3;

    public static final Integer PRESET_CUSTOM = R.string.preset_custom;
    public static final Integer PRESET_BASIC = R.string.preset_basic;
    public static final Integer PRESET_EMAIL = R.string.preset_email;
    public static final Integer PRESET_PHOTO = R.string.preset_photo;

    public static final Integer STYLE_NONE = R.string.style_none;
    public static final Integer STYLE_BIG_PICTURE = R.string.style_big_picture;
    public static final Integer STYLE_BIG_TEXT = R.string.style_big_text;
    public static final Integer STYLE_INBOX = R.string.style_inbox;

    public static final Object[] SMALL_ICONS = new Object[] {
        android.R.drawable.stat_sys_warning,
        android.R.drawable.stat_sys_download_done,
        android.R.drawable.stat_notify_chat,
        android.R.drawable.stat_notify_sync,
        android.R.drawable.stat_notify_more,
        android.R.drawable.stat_notify_sdcard,
        android.R.drawable.stat_sys_data_bluetooth,
        android.R.drawable.stat_notify_voicemail,
        android.R.drawable.stat_sys_speakerphone,
        android.R.drawable.ic_menu_camera,
        android.R.drawable.ic_menu_share,
        R.drawable.ic_notification_multiple_mail_holo_dark
    };

    public static final Object[] ACTION_ICONS = SMALL_ICONS;

    public static final int[] LARGE_ICONS = new int[]{
      R.drawable.romainguy_rockaway,
      R.drawable.android_logo,
      R.drawable.romain,
      R.drawable.ic_notification_multiple_mail_holo_dark
    };

    public static final int[] PICTURES = LARGE_ICONS;

}
