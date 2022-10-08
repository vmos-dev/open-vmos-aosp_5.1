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

package com.android.notificationstudio.generator;
import static com.android.notificationstudio.model.EditableItem.ACTION1_ICON;
import static com.android.notificationstudio.model.EditableItem.ACTION1_TEXT;
import static com.android.notificationstudio.model.EditableItem.ACTION2_ICON;
import static com.android.notificationstudio.model.EditableItem.ACTION2_TEXT;
import static com.android.notificationstudio.model.EditableItem.ACTION3_ICON;
import static com.android.notificationstudio.model.EditableItem.ACTION3_TEXT;
import static com.android.notificationstudio.model.EditableItem.BIG_CONTENT_TITLE;
import static com.android.notificationstudio.model.EditableItem.BIG_TEXT;
import static com.android.notificationstudio.model.EditableItem.CONTENT_INFO;
import static com.android.notificationstudio.model.EditableItem.CONTENT_TEXT;
import static com.android.notificationstudio.model.EditableItem.CONTENT_TITLE;
import static com.android.notificationstudio.model.EditableItem.LARGE_ICON;
import static com.android.notificationstudio.model.EditableItem.LINES;
import static com.android.notificationstudio.model.EditableItem.NUMBER;
import static com.android.notificationstudio.model.EditableItem.PICTURE;
import static com.android.notificationstudio.model.EditableItem.PROGRESS;
import static com.android.notificationstudio.model.EditableItem.SMALL_ICON;
import static com.android.notificationstudio.model.EditableItem.STYLE;
import static com.android.notificationstudio.model.EditableItem.SUB_TEXT;
import static com.android.notificationstudio.model.EditableItem.SUMMARY_TEXT;
import static com.android.notificationstudio.model.EditableItem.USES_CHRON;
import static com.android.notificationstudio.model.EditableItem.WHEN;

import android.app.Notification;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.support.v4.app.NotificationCompat;
import android.support.v4.app.NotificationCompat.BigPictureStyle;
import android.support.v4.app.NotificationCompat.BigTextStyle;
import android.support.v4.app.NotificationCompat.InboxStyle;

import com.android.notificationstudio.model.EditableItemConstants;

public class NotificationGenerator implements EditableItemConstants {

    public static Notification build(Context context) {

        PendingIntent noop = PendingIntent.getActivity(context, 0, new Intent(), 0);

        NotificationCompat.Builder builder = new NotificationCompat.Builder(context);
        if (SMALL_ICON.hasValue())
            builder.setSmallIcon(SMALL_ICON.getValueInt());
        if (CONTENT_TITLE.hasValue())
            builder.setContentTitle(CONTENT_TITLE.getValueString());
        if (CONTENT_TEXT.hasValue())
            builder.setContentText(CONTENT_TEXT.getValueString());
        if (SUB_TEXT.hasValue())
            builder.setSubText(SUB_TEXT.getValueString());
        if (LARGE_ICON.hasValue())
            builder.setLargeIcon(LARGE_ICON.getValueBitmap());
        if (CONTENT_INFO.hasValue())
            builder.setContentInfo(CONTENT_INFO.getValueString());
        if (NUMBER.hasValue())
            builder.setNumber(NUMBER.getValueInt());
        if (WHEN.hasValue())
            builder.setWhen(WHEN.getValueLong());
        if (PROGRESS.hasValue() && PROGRESS.getValueBool())
            builder.setProgress(0, 0, true);
        if (USES_CHRON.hasValue())
            builder.setUsesChronometer(USES_CHRON.getValueBool());
        if (ACTION1_ICON.hasValue())
            builder.addAction(ACTION1_ICON.getValueInt(), ACTION1_TEXT.getValueString(), noop);
        if (ACTION2_ICON.hasValue())
            builder.addAction(ACTION2_ICON.getValueInt(), ACTION2_TEXT.getValueString(), noop);
        if (ACTION3_ICON.hasValue())
            builder.addAction(ACTION3_ICON.getValueInt(), ACTION3_TEXT.getValueString(), noop);

        if (STYLE.hasValue())
            generateStyle(builder);

        // for older OSes
        builder.setContentIntent(noop);

        return builder.build();
    }

    private static void generateStyle(NotificationCompat.Builder builder) {
        Integer styleValue = STYLE.getValueInt();

        if (STYLE_BIG_PICTURE.equals(styleValue)) {
            BigPictureStyle bigPicture = new NotificationCompat.BigPictureStyle();
            if (PICTURE.hasValue())
                bigPicture.bigPicture(PICTURE.getValueBitmap());
            if (BIG_CONTENT_TITLE.hasValue())
                bigPicture.setBigContentTitle(BIG_CONTENT_TITLE.getValueString());
            if (SUMMARY_TEXT.hasValue())
                bigPicture.setSummaryText(SUMMARY_TEXT.getValueString());
            builder.setStyle(bigPicture);
        } else if (STYLE_BIG_TEXT.equals(styleValue)) {
            BigTextStyle bigText = new NotificationCompat.BigTextStyle();
            if (BIG_TEXT.hasValue())
                bigText.bigText(BIG_TEXT.getValueString());
            if (BIG_CONTENT_TITLE.hasValue())
                bigText.setBigContentTitle(BIG_CONTENT_TITLE.getValueString());
            if (SUMMARY_TEXT.hasValue())
                bigText.setSummaryText(SUMMARY_TEXT.getValueString());
            builder.setStyle(bigText);
        } else if (STYLE_INBOX.equals(styleValue)) {
            InboxStyle inboxStyle = new NotificationCompat.InboxStyle();
            if (LINES.hasValue()) {
                for (String line : LINES.getValueString().split("\\n")) {
                    inboxStyle.addLine(line);
                }
            }
            if (BIG_CONTENT_TITLE.hasValue())
                inboxStyle.setBigContentTitle(BIG_CONTENT_TITLE.getValueString());
            if (SUMMARY_TEXT.hasValue())
                inboxStyle.setSummaryText(SUMMARY_TEXT.getValueString());
            builder.setStyle(inboxStyle);
        }
    }

}
