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

import android.content.Context;

import com.android.notificationstudio.model.EditableItem;
import com.android.notificationstudio.model.EditableItemConstants;

public class CodeGenerator implements EditableItemConstants {

    private static final String INDENT = "\n    ";
    private static final String STYLE_INDENT = INDENT + "    ";

    public static String generate(Context context) {

        StringBuilder sb = new StringBuilder("new Notification.Builder(context)");

        if (SMALL_ICON.hasValue())
            sb.append(INDENT + ".setSmallIcon(" + getResourceVar(context, SMALL_ICON) + ")");
        if (CONTENT_TITLE.hasValue())
            sb.append(INDENT + ".setContentTitle(" + quote(CONTENT_TITLE) + ")");
        if (CONTENT_TEXT.hasValue())
            sb.append(INDENT + ".setContentText(" + quote(CONTENT_TEXT) + ")");
        if (SUB_TEXT.hasValue())
            sb.append(INDENT + ".setSubText(" + quote(SUB_TEXT) + ")");
        if (LARGE_ICON.hasValue())
            sb.append(INDENT + ".setLargeIcon(largeIconBitmap)");
        if (CONTENT_INFO.hasValue())
            sb.append(INDENT + ".setContentInfo(" + quote(CONTENT_INFO) + ")");
        if (NUMBER.hasValue())
            sb.append(INDENT + ".setNumber(" + NUMBER.getValueInt() + ")");
        if (WHEN.hasValue())
            sb.append(INDENT + ".setWhen(" + WHEN.getValueLong() + ")");
        if (PROGRESS.hasValue() && PROGRESS.getValueBool())
            sb.append(INDENT + ".setProgress(0, 0, true)");
        if (USES_CHRON.hasValue())
            sb.append(INDENT + ".setUsesChronometer(" + USES_CHRON.getValueBool() + ")");
        if (ACTION1_ICON.hasValue())
            generateAction(sb, ACTION1_ICON, ACTION1_TEXT, "action1PendingIntent");
        if (ACTION2_ICON.hasValue())
            generateAction(sb, ACTION2_ICON, ACTION2_TEXT, "action2PendingIntent");
        if (ACTION3_ICON.hasValue())
            generateAction(sb, ACTION3_ICON, ACTION3_TEXT, "action3PendingIntent");

        if (STYLE.hasValue())
            generateStyle(sb);

        sb.append(INDENT + ".build();");
        return sb.toString();
    }

    private static void generateStyle(StringBuilder sb) {
        Integer styleValue = STYLE.getValueInt();
        if (STYLE_BIG_PICTURE.equals(styleValue)) {
            sb.append(INDENT + ".setStyle(new Notification.BigPictureStyle()");
            if (PICTURE.hasValue())
                sb.append(STYLE_INDENT + ".bigPicture(pictureBitmap)");
        }
        if (STYLE_BIG_TEXT.equals(styleValue)) {
            sb.append(INDENT + ".setStyle(new Notification.BigTextStyle()");
            if (BIG_TEXT.hasValue())
                sb.append(STYLE_INDENT + ".bigText(" + quote(BIG_TEXT) + ")");
        }
        if (STYLE_INBOX.equals(styleValue)) {
            sb.append(INDENT + ".setStyle(new Notification.InboxStyle()");
            if (LINES.hasValue()) {
                for (String line : LINES.getValueString().split("\\n")) {
                    sb.append(STYLE_INDENT + ".addLine(" + quote(line) + ")");
                }
            }
        }
        if (BIG_CONTENT_TITLE.hasValue())
            sb.append(STYLE_INDENT + ".setBigContentTitle(" + quote(BIG_CONTENT_TITLE) + ")");
        if (SUMMARY_TEXT.hasValue())
            sb.append(STYLE_INDENT + ".setSummaryText(" + quote(SUMMARY_TEXT) + ")");

        sb.append(")");
    }

    private static void generateAction(StringBuilder sb,
            EditableItem icon, EditableItem text, String intentName) {
        sb.append(INDENT +
            ".addAction(" + icon.getValueInt() + ", " + quote(text) + ", " + intentName + ")");
    }

    private static String quote(EditableItem text) {
        return quote(text.getValueString());
    }

    private static String quote(String text) {
        return text != null ? "\"" + text.replace("\"", "\\\"") + "\"" : "null";
    }

    private static String getResourceVar(Context context, EditableItem item) {
        int resId = item.getValueInt();
        String packageName = context.getResources().getResourcePackageName(resId);
        String type = context.getResources().getResourceTypeName(resId);
        String entryName = context.getResources().getResourceEntryName(resId);
        return packageName + ".R." + type + "." + entryName;
    }

}
